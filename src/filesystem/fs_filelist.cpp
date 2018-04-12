/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2017 Noah Metzger (chomenor@gmail.com)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifdef NEW_FILESYSTEM
#include "fslocal.h"

// Limit max files returned by searches to avoid Z_Malloc overflows
#define	MAX_FOUND_FILES	32768

#define FL_FLAG_USE_PURE_LIST 1
#define FL_FLAG_IGNORE_TAPAK0 2

typedef struct {
	const char *extension;
	const char *filter;
	int flags;
} filelist_query_t;

typedef struct {
	const filelist_query_t *query;
	int extension_length;
	int crop_length;
	fsc_stack_t temp_stack;
	int file_depth;
	int directory_depth;
} filelist_work_t;

/* ******************************************************************************** */
// Sort key handling
/* ******************************************************************************** */

typedef struct {
	int length;
	char key[];
} file_list_sort_key_t;

static file_list_sort_key_t *generate_sort_key(const fsc_file_t *file, fsc_stack_t *stack, const filelist_query_t *query) {
	char buffer[1024];
	fsc_stream_t stream = {buffer, 0, sizeof(buffer), 0};
	file_list_sort_key_t *key;

	fs_generate_file_sort_key(file, &stream, (query->flags & FL_FLAG_USE_PURE_LIST) ? qtrue : qfalse);

	key = (file_list_sort_key_t *)fsc_stack_retrieve(stack, fsc_stack_allocate(stack, sizeof(*key) + stream.position));
	key->length = stream.position;
	fsc_memcpy(key->key, stream.data, stream.position);
	return key; }

static int compare_sort_keys(file_list_sort_key_t *key1, file_list_sort_key_t *key2) {
	return fsc_memcmp(key2->key, key1->key, key1->length < key2->length ? key1->length : key2->length); }

/* ******************************************************************************** */
// File list generation
/* ******************************************************************************** */

typedef struct {
	fs_hashtable_entry_t hte;
	char *string;
	const fsc_file_t *file;
	file_list_sort_key_t *sort_key;
} temp_file_set_entry_t;

static char *allocate_string(const char *string) {
	int length = strlen(string);
	char *copy = (char *)Z_Malloc(length + 1);
	Com_Memcpy(copy, string, length);
	copy[length] = 0;
	return copy; }

static void temp_file_set_insert(fs_hashtable_t *ht, const fsc_file_t *file, file_list_sort_key_t *sort_key, const char *path) {
	unsigned int hash = fsc_string_hash(path, 0);
	fs_hashtable_iterator_t it = fs_hashtable_iterate(ht, hash, qfalse);
	temp_file_set_entry_t *entry;

	while((entry = (temp_file_set_entry_t *)fs_hashtable_next(&it))) {
		// Check if file is match
		if(strcmp(path, entry->string)) continue;

		// Matching file found - replace it if new file is higher precedence
		if(compare_sort_keys(sort_key, entry->sort_key) < 0) {
			entry->file = file;
			entry->sort_key = sort_key; }
		return; }

	// No matching file - create new entry
	entry = (temp_file_set_entry_t *)Z_Malloc(sizeof(*entry));
	entry->file = file;
	entry->sort_key = sort_key;
	entry->string = allocate_string(path);
	fs_hashtable_insert(ht, &entry->hte, hash); }

static qboolean check_path_enabled(fsc_stream_t *stream, const filelist_work_t *flw) {
	// Returns qtrue if path stored in stream matches filter/extension criteria, qfalse otherwise
	if(!stream->position) return qfalse;
	if(flw->query->extension) {
		if(stream->position < flw->extension_length) return qfalse;
		if(Q_stricmpn(stream->data + stream->position - flw->extension_length,
				flw->query->extension, flw->extension_length)) return qfalse; }
	if(flw->query->filter) {
		if(!Com_FilterPath((char *)flw->query->filter, stream->data, qfalse)) return qfalse; }
	return qtrue; }

static qboolean check_file_enabled(const fsc_file_t *file, const filelist_work_t *flw) {
	// Returns qtrue if file is valid to use, qfalse otherwise
	int disabled_flags = FD_FLAG_FILELIST_QUERY;
	if(flw->query->flags & FL_FLAG_USE_PURE_LIST) disabled_flags |= FD_FLAG_CHECK_PURE;
	if(fs_file_disabled(file, disabled_flags)) return qfalse;
	if((flw->query->flags & FL_FLAG_IGNORE_TAPAK0) && file->sourcetype == FSC_SOURCETYPE_PK3 &&
			((fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3))->pk3_hash == 2430342401u) return qfalse;
	return qtrue; }

static void cut_stream(const fsc_stream_t *source, fsc_stream_t *target, int start_pos, int end_pos) {
	target->position = 0;
	if(start_pos < end_pos) fsc_write_stream_data(target, source->data + start_pos, end_pos - start_pos);
	fsc_stream_append_string(target, ""); }

static void temp_file_set_populate(const fsc_directory_t *base, fs_hashtable_t *output, filelist_work_t *flw) {
	char path_buffer[FS_FILE_BUFFER_SIZE];
	fsc_stream_t path_stream = {path_buffer, 0, sizeof(path_buffer), 0};
	char string_buffer[FS_FILE_BUFFER_SIZE];
	fsc_stream_t string_stream = {string_buffer, 0, sizeof(string_buffer), 0};
	fsc_file_t *file;
	fsc_directory_t *directory;

	file = (fsc_file_t *)STACKPTR(base->sub_file);
	while(file) {
		if(check_file_enabled(file, flw)) {
			int i;
			file_list_sort_key_t *sort_key = 0;
			int depth = 0;

			path_stream.position = 0;
			fs_file_to_stream(file, &path_stream, qfalse, qfalse, qfalse, qfalse);
			for(i=flw->crop_length; i<path_stream.position; ++i) {
				if(path_stream.data[i] == '/') {
					++depth;
					if(depth < flw->directory_depth) {
						// Process directory
						cut_stream(&path_stream, &string_stream, flw->crop_length, i+1);
						if(check_path_enabled(&string_stream, flw)) {
							if(output->element_count >= MAX_FOUND_FILES) return;
							if(!sort_key) sort_key = generate_sort_key(file, &flw->temp_stack, flw->query);
							temp_file_set_insert(output, file, sort_key, string_stream.data); } } } }

			if(depth < flw->file_depth) {
				// Process file
				cut_stream(&path_stream, &string_stream, flw->crop_length, path_stream.position);
				if(check_path_enabled(&string_stream, flw)) {
					if(output->element_count >= MAX_FOUND_FILES) return;
					if(!sort_key) sort_key = generate_sort_key(file, &flw->temp_stack, flw->query);
					temp_file_set_insert(output, file, sort_key, string_stream.data); } } }

		file = (fsc_file_t *)STACKPTR(file->next_in_directory); }

	// Process subdirectories
	directory = (fsc_directory_t *)STACKPTR(base->sub_directory);
	while(directory) {
		temp_file_set_populate(directory, output, flw);
		directory = (fsc_directory_t *)STACKPTR(directory->peer_directory); } }

static int temp_file_list_compare_element(const temp_file_set_entry_t *element1, const temp_file_set_entry_t *element2) {
	if(element1->file != element2->file) {
		int sort_result = compare_sort_keys(element1->sort_key, element2->sort_key);
		if(sort_result) return sort_result; }
	return Q_stricmp(element1->string, element2->string); }

static int temp_file_list_qsort(const void *element1, const void *element2) {
	return temp_file_list_compare_element(*(const temp_file_set_entry_t **)element1,
			*(const temp_file_set_entry_t **)element2); }

static char **temp_file_set_to_file_list(fs_hashtable_t *file_set, int *numfiles_out) {
	int i;
	fs_hashtable_iterator_t it;
	temp_file_set_entry_t *entry;
	int position = 0;
	char **output = (char **)Z_Malloc(sizeof(*output) * (file_set->element_count + 1));
	temp_file_set_entry_t **temp_list = (temp_file_set_entry_t **)Z_Malloc(sizeof(*temp_list) * file_set->element_count);

	// Transfer entries from file set hashtable to temporary list
	it = fs_hashtable_iterate(file_set, 0, qtrue);
	while((entry = (temp_file_set_entry_t *)fs_hashtable_next(&it))) {
		if(position >= file_set->element_count) {
			Com_Error(ERR_FATAL, "fs_filelist.c->temp_file_set_to_file_list element_count overflow"); }
		temp_list[position++] = entry; }
	if(position != file_set->element_count) {
		Com_Error(ERR_FATAL, "fs_filelist.c->temp_file_set_to_file_list element_count underflow"); }

	// Sort the list
	qsort(temp_list, file_set->element_count, sizeof(*temp_list), temp_file_list_qsort);

	// Transfer strings from list to output array
	for(i=0; i<file_set->element_count; ++i) {
		output[i] = temp_list[i]->string; }
	output[i] = 0;

	if(numfiles_out) *numfiles_out = file_set->element_count;
	Z_Free(temp_list);
	return output; }

/* ******************************************************************************** */
// Main file list function (list_files)
/* ******************************************************************************** */

static fsc_directory_t *get_start_directory(const char *path, int length) {
	// Path can be null to start at base directory
	char path2[FSC_MAX_QPATH];
	fsc_hashtable_iterator_t hti;
	fsc_stackptr_t directory_ptr;
	fsc_directory_t *directory = 0;

	// Make path lowercase and crop to length
	if(path) {
		Q_strncpyz(path2, path, sizeof(path2));
		if(length < sizeof(path2)) path2[length] = 0;
		Q_strlwr(path2); }

	// Look for directory entry
	fsc_hashtable_open(&fs.directories, path ? fsc_string_hash(path2, 0) : 0, &hti);
	while((directory_ptr = fsc_hashtable_next(&hti))) {
		directory = (fsc_directory_t *)STACKPTR(directory_ptr);
		if(path) {
			if(!Q_stricmp((const char *)STACKPTR(directory->qp_dir_ptr), path2)) break;}
		else {
			if(directory->qp_dir_ptr == 0) break; } }

	if(!directory_ptr) return 0;
	return directory; }

static char **list_files(const char *path, int *numfiles_out, filelist_query_t *query) {
	int path_length = strlen(path);
	fsc_directory_t *start_directory;
	fs_hashtable_t temp_file_set;
	filelist_work_t flw;
	char **result;
	int start_time = 0;
	int default_depth = 2;	// Emulate original filesystem max depth (max slash-separated sections in output)

	if(fs_debug_filelist->integer) {
		start_time = Sys_Milliseconds();
		Com_Printf("********** file list query **********\n");
		Com_Printf("path: %s\nextension: %s\nfilter: %s\n", path, query->extension, query->filter); }

	// Initialize temp structures
	Com_Memset(&flw, 0, sizeof(flw));
	flw.query = query;
	flw.extension_length = query->extension ? strlen(query->extension) : 0;
	fsc_stack_initialize(&flw.temp_stack);
	fs_hashtable_initialize(&temp_file_set, MAX_FOUND_FILES);

	// Determine start directory
	if(path_length && (path[path_length-1] == '/' || path[path_length-1] == '\\')) {
		--path_length;
		++default_depth; }
	if(path_length) {
		start_directory = get_start_directory(path, path_length); }
	else {
		start_directory = get_start_directory(0, 0);
		++default_depth; }

	if(start_directory) {
		// Determine depths
		flw.file_depth = flw.directory_depth = default_depth;
		if(query->filter) flw.file_depth = flw.directory_depth = 256;
		if(flw.extension_length) {
			// Optimization to skip processing path types blocked by extension anyway
			if(query->extension[flw.extension_length-1] == '/') flw.file_depth = 0;
			else flw.directory_depth = 0; }

		// Determine prefix length
		if(!query->filter && start_directory->qp_dir_ptr) {
			flw.crop_length = strlen((const char *)STACKPTR(start_directory->qp_dir_ptr)) + 1; }

		// Populate file set
		temp_file_set_populate(start_directory, &temp_file_set, &flw); }

	// Generate file list
	result = temp_file_set_to_file_list(&temp_file_set, numfiles_out);

	if(fs_debug_filelist->integer) {
		Com_Printf("result: %i elements\n", temp_file_set.element_count);
		Com_Printf("temp stack usage: %u\n", fsc_stack_get_export_size(&flw.temp_stack));
		Com_Printf("time: %i\n", Sys_Milliseconds() - start_time); }

	fs_hashtable_free(&temp_file_set, 0);
	fsc_stack_free(&flw.temp_stack);
	return result; }

void FS_FreeFileList( char **list ) {
	int i;
	if(!list ) return;

	for(i=0; list[i]; i++) {
		Z_Free(list[i]); }

	Z_Free(list); }

/* ******************************************************************************** */
// Mod directory listing (FS_GetModList)
/* ******************************************************************************** */

#define MAX_MOD_DIRS 128

typedef struct {
	char *mod_dirs[MAX_MOD_DIRS];
	int count;
} mod_dir_list_t;

static void add_mod_dir_to_list(mod_dir_list_t *list, const char *mod_dir) {
	int i;
	if(list->count >= MAX_MOD_DIRS) return;
	for(i=0; i<list->count; ++i) if(!Q_stricmp(list->mod_dirs[i], mod_dir)) return;
	list->mod_dirs[list->count++] = CopyString(mod_dir); }

static void generate_mod_dir_list2(fsc_directory_t *base, mod_dir_list_t *list) {
	fsc_file_t *file;
	fsc_directory_t *directory;
	const char *last_mod_dir = 0;

	file = (fsc_file_t *)STACKPTR(base->sub_file);
	while(file) {
		if(file->sourcetype == FSC_SOURCETYPE_DIRECT && fsc_is_file_enabled(file, &fs)) {
			const char *mod_dir = fsc_get_mod_dir(file, &fs);
			if(mod_dir != last_mod_dir) {
				add_mod_dir_to_list(list, fsc_get_mod_dir(file, &fs));
				last_mod_dir = mod_dir; } }

		file = (fsc_file_t *)STACKPTR(file->next_in_directory); }

	directory = (fsc_directory_t *)STACKPTR(base->sub_directory);
	while(directory) {
		generate_mod_dir_list2(directory, list);
		directory = (fsc_directory_t *)STACKPTR(directory->peer_directory); } }

static int mod_list_qsort(const void *element1, const void *element2) {
	return Q_stricmp(*(const char **)element1, *(const char **)element2); }

static void generate_mod_dir_list(mod_dir_list_t *list) {
	list->count = 0;
	generate_mod_dir_list2(get_start_directory(0, 0), list);
	qsort(list->mod_dirs, list->count, sizeof(*list->mod_dirs), mod_list_qsort); }

static void mod_list_free(mod_dir_list_t *list) {
	int i;
	for(i=0; i<list->count; ++i) Z_Free(list->mod_dirs[i]); }

static int FS_GetModList(char *listbuf, int bufsize) {
	int i;
	int nTotal = 0;		// Amount of buffer used so far
	int nMods = 0;		// Number of mods
	mod_dir_list_t list;
	char description[49];
	int mod_name_length;
	int description_length;

	generate_mod_dir_list(&list);

	for(i=0; i<list.count; ++i) {
		char *mod_name = list.mod_dirs[i];
		if(!Q_stricmp(mod_name, com_basegame->string)) continue;
		if(!Q_stricmp(mod_name, "basemod")) continue;

		FS_GetModDescription(mod_name, description, sizeof(description));
		mod_name_length = strlen(mod_name) + 1;
		description_length = strlen(description) + 1;

		if(nTotal + mod_name_length + description_length < bufsize) {
			strcpy(listbuf, mod_name);
			listbuf += mod_name_length;
			strcpy(listbuf, description);
			listbuf += description_length;
			nTotal += mod_name_length + description_length;
			++nMods; } }

	mod_list_free(&list);
	return nMods; }

/* ******************************************************************************** */
// External file list functions
/* ******************************************************************************** */

char **FS_ListFilteredFiles(const char *path, const char *extension, const char *filter,
		int *numfiles_out, qboolean allowNonPureFilesOnDisk) {
	filelist_query_t query = {extension, filter, allowNonPureFilesOnDisk ? 0 : FL_FLAG_USE_PURE_LIST};
	return list_files(path, numfiles_out, &query); }

char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) {
	return FS_ListFilteredFiles( path, extension, NULL, numfiles, qfalse ); }

int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	int i;
	int flags = FL_FLAG_USE_PURE_LIST;
	int nFiles = 0;
	int nTotal = 0;
	int nLen;
	char **pFiles = NULL;

	*listbuf = 0;

	if(!Q_stricmp(path, "$modlist")) {
		return FS_GetModList(listbuf, bufsize); }

	if(!Q_stricmp(path, "demos")) {
		// Check for new demos before displaying the UI demo menu
		fs_auto_refresh(); }

	if(!Q_stricmp(path, "models/players") && extension && !Q_stricmp(extension, "/")
			&& Q_stricmp(current_mod_dir, BASETA)) {
		// Special case to block missionpack pak0.pk3 models from the standard non-TA model list
		// which doesn't handle their skin setting correctly
		flags |= FL_FLAG_IGNORE_TAPAK0; }

	{
		filelist_query_t query = {extension, 0, flags};
		pFiles = list_files(path, &nFiles, &query);
	}

	for (i =0; i < nFiles; i++) {
		nLen = strlen(pFiles[i]) + 1;
		if (nTotal + nLen + 1 < bufsize) {
			strcpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal += nLen;
		}
		else {
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList(pFiles);

	return nFiles;
}

#endif	// NEW_FILESYSTEM
