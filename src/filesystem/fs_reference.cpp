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

#define MAX_REFERENCE_SET_ENTRIES 2048
#define MAX_PURE_CHECKSUM_CACHE 256

/* ******************************************************************************** */
// Reference Set
/* ******************************************************************************** */

typedef struct {
	fs_hashtable_entry_t hte;
	fsc_file_direct_t *pak;
	int position;	// lower = higher priority
} reference_set_entry_t;

typedef fs_hashtable_t reference_set_t;

static qboolean reference_set_add(reference_set_t *reference_set, fsc_file_direct_t *pak, int position) {
	// Returns qtrue on success, qfalse if already inserted or maximum hit
	// This function should be called in lowest-to-highest position order
	fs_hashtable_iterator_t it = fs_hashtable_iterate(reference_set, pak->pk3_hash, qfalse);
	reference_set_entry_t *entry;

	if(reference_set->element_count >= MAX_REFERENCE_SET_ENTRIES) return qfalse;

	while((entry = (reference_set_entry_t *)fs_hashtable_next(&it))) {
		if(entry->pak->pk3_hash == pak->pk3_hash) {
			// File with same hash - only keep the higher precedence file
			if(entry->pak != pak && position == entry->position &&
					fs_compare_file((fsc_file_t *)entry->pak, (fsc_file_t *)pak, qfalse) > 0) entry->pak = pak;
			return qfalse; } }

	entry = (reference_set_entry_t *)S_Malloc(sizeof(reference_set_entry_t));
	entry->pak = pak;
	entry->position = position;
	fs_hashtable_insert(reference_set, (fs_hashtable_entry_t *)entry, pak->pk3_hash);
	return qtrue; }

/* ******************************************************************************** */
// Reference List
/* ******************************************************************************** */

typedef struct {
	fsc_file_direct_t *pak;
	int position;
} reference_list_entry_t;

typedef struct {
	reference_list_entry_t *entries;
	int entry_count;
} reference_list_t;

static void reference_set_to_reference_list(fs_hashtable_t *reference_set, reference_list_t *reference_list) {
	fs_hashtable_iterator_t it = fs_hashtable_iterate(reference_set, 0, qtrue);
	reference_set_entry_t *entry;
	int count = 0;

	reference_list->entry_count = reference_set->element_count;
	reference_list->entries = (reference_list_entry_t *)Z_Malloc(reference_list->entry_count * sizeof(*reference_list->entries));

	while((entry = (reference_set_entry_t *)fs_hashtable_next(&it))) {
		if(count >= reference_list->entry_count) Com_Error(ERR_FATAL, "reference_set_to_reference_list overflow");
		reference_list->entries[count].position = entry->position;
		reference_list->entries[count].pak = entry->pak;
		++count; }
	if(count != reference_list->entry_count) Com_Error(ERR_FATAL, "reference_set_to_reference_list underflow"); }

static void free_reference_list(reference_list_t *reference_list) {
	Z_Free(reference_list->entries); }

static int compare_reference_entry(const reference_list_entry_t *entry1, const reference_list_entry_t *entry2,
		qboolean use_pure_list) {
	if(entry1->position < entry2->position) return -1;
	if(entry2->position < entry1->position) return 1;
	return fs_compare_file((fsc_file_t *)entry1->pak, (fsc_file_t *)entry2->pak, use_pure_list); }

static int compare_reference_entry_qsort_pure(const void *entry1, const void *entry2) {
	return compare_reference_entry((const reference_list_entry_t *)entry1, (const reference_list_entry_t *)entry2, qtrue); }

static int compare_reference_entry_qsort_nonpure(const void *entry1, const void *entry2) {
	return compare_reference_entry((const reference_list_entry_t *)entry1, (const reference_list_entry_t *)entry2, qfalse); }

static void sort_reference_list(reference_list_t *reference_list, qboolean use_pure_list) {
	qsort(reference_list->entries, reference_list->entry_count, sizeof(*reference_list->entries),
			use_pure_list ? compare_reference_entry_qsort_pure : compare_reference_entry_qsort_nonpure); }

/* ******************************************************************************** */
// Reference String Building
/* ******************************************************************************** */

static void reference_name_string(fsc_file_direct_t *pak, fsc_stream_t *stream) {
	const char *mod_dir = pak->qp_mod_ptr ? (const char *)STACKPTR(pak->qp_mod_ptr) : com_basegame->string;

	// Replace basemod with com_basegame to avoid issues
	// Servers should avoid using basemod for download-enabled paks if possible
	if(!Q_stricmp(mod_dir, "basemod")) mod_dir = com_basegame->string;

	// Patch mod dir capitalization
	if(!Q_stricmp(mod_dir, com_basegame->string)) mod_dir = com_basegame->string;
	if(!Q_stricmp(mod_dir, FS_GetCurrentGameDir())) mod_dir = FS_GetCurrentGameDir();

	fsc_stream_append_string(stream, mod_dir);
	fsc_stream_append_string(stream, "/");
	fsc_stream_append_string(stream, (const char *)STACKPTR(pak->f.qp_name_ptr)); }

static void reference_hash_string(fsc_file_direct_t *pak, fsc_stream_t *stream) {
	char buffer[20];
	Com_sprintf(buffer, sizeof(buffer), "%i", pak->pk3_hash);
	fsc_stream_append_string(stream, buffer); }

static void reference_list_to_stream(reference_list_t *reference_list, fsc_stream_t *stream,
		void (data_function)(fsc_file_direct_t *pak, fsc_stream_t *stream)) {
	int i;
	for(i=0; i<reference_list->entry_count; ++i) {
		if(stream->position) fsc_stream_append_string(stream, " ");
		data_function(reference_list->entries[i].pak, stream);
		if(stream->overflowed) break; } }

static qboolean reference_list_to_buffer(reference_list_t *reference_list, char *output, unsigned int output_size,
			void (data_function)(fsc_file_direct_t *pak, fsc_stream_t *stream)) {
	// Returns qtrue on success, qfalse on overflow
	fsc_stream_t stream = {output, 0, output_size, 0};
	*output = 0;	// In case there are 0 references
	reference_list_to_stream(reference_list, &stream, data_function);
	if(stream.overflowed) {
		*output = 0;
		return qfalse; }
	return qtrue; }

/* ******************************************************************************** */
// Pure Validation
/* ******************************************************************************** */

// Pure checksum entry handling

typedef struct {
	const fsc_file_direct_t *pk3;
	int data_size;
	int pure_checksum;
	int checksum_feed;
} pure_checksum_entry_t;

static void get_pure_checksum_entry_callback(void *context, char *data, int size) {
	pure_checksum_entry_t **entry = (pure_checksum_entry_t **)context;
	*entry = (pure_checksum_entry_t *)fsc_malloc(sizeof(pure_checksum_entry_t) + size);
	(*entry)->data_size = size;
	Com_Memcpy((char *)&(*entry)->checksum_feed + 4, data, size); }

static pure_checksum_entry_t *get_pure_checksum_entry(const fsc_file_direct_t *pk3) {
	pure_checksum_entry_t *entry = 0;
	fsc_load_pk3(STACKPTR(pk3->os_path_ptr), &fs, 0, 0, get_pure_checksum_entry_callback, &entry);
	if(entry) entry->pk3 = pk3;
	return entry; }

static void update_pure_checksum_entry(pure_checksum_entry_t *entry, int checksum_feed) {
	entry->checksum_feed = LittleLong(checksum_feed);
	entry->pure_checksum = fsc_block_checksum(&entry->checksum_feed, entry->data_size + 4); }

// Pure checksum cache

typedef struct pure_checksum_node_s {
	struct pure_checksum_node_s *next;
	pure_checksum_entry_t *entry;
	int rank;
} pure_checksum_node_t;

static int pure_checksum_rank = 0;
static pure_checksum_node_t *pure_checksum_cache;

static int get_pure_checksum_for_pk3(const fsc_file_direct_t *pk3, int checksum_feed) {
	int entry_count = 0;
	pure_checksum_node_t *node = pure_checksum_cache;
	pure_checksum_node_t *deletion_node = 0;

	while(node) {
		if(node->entry && node->entry->pk3 == pk3) {
			// Use existing entry
			if(node->entry->checksum_feed != checksum_feed) {
				update_pure_checksum_entry(node->entry, checksum_feed); }
			node->rank = ++pure_checksum_rank;
			return node->entry->pure_checksum; }

		if(!deletion_node || node->rank < deletion_node->rank) deletion_node = node;

		++entry_count;
		node = node->next; }

	// Prepare new node
	if(entry_count >= MAX_PURE_CHECKSUM_CACHE) {
		node = deletion_node;
		if(node->entry) fsc_free(node->entry); }
	else {
		node = (pure_checksum_node_t *)S_Malloc(sizeof(*node));
		node->next = pure_checksum_cache;
		pure_checksum_cache = node; }

	// Create new entry
	node->entry = get_pure_checksum_entry(pk3);
	if(!node->entry) return 0;
	update_pure_checksum_entry(node->entry, checksum_feed);
	node->rank = ++pure_checksum_rank;
	return node->entry->pure_checksum; }

static int get_pure_checksum_for_file(const fsc_file_t *file, int checksum_feed) {
	if(!file) return 0;
	if(file->sourcetype != FSC_SOURCETYPE_PK3) return 0;
	return get_pure_checksum_for_pk3((const fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3), checksum_feed); }

// Pure validation string building

static void add_referenced_pure_pk3s(fsc_stream_t *stream, fs_hashtable_t *referenced_paks) {
	int i;
	reference_list_t reference_list;
	char buffer[20];
	int lump_checksum = 0;

	reference_set_to_reference_list(referenced_paks, &reference_list);
	sort_reference_list(&reference_list, qtrue);

	for(i=0; i<reference_list.entry_count; ++i) {
		int pure_checksum = get_pure_checksum_for_pk3(reference_list.entries[i].pak, checksum_feed);

		if(fs_debug_references->integer) {
			char temp[FS_FILE_BUFFER_SIZE];
			fs_file_to_buffer((fsc_file_t *)reference_list.entries[i].pak, temp, sizeof(temp), qtrue, qtrue, qtrue, qtrue);
			Com_Printf("adding pak to pure validation list: %s\n", temp); }

		lump_checksum ^= pure_checksum;
		Com_sprintf(buffer, sizeof(buffer), " %i", pure_checksum);
		fsc_stream_append_string(stream, buffer); }

	Com_sprintf(buffer, sizeof(buffer), " %i ", checksum_feed ^ lump_checksum ^ reference_list.entry_count);
	fsc_stream_append_string(stream, buffer);

	free_reference_list(&reference_list); }

static void build_pure_validation_string(char *output, unsigned int output_size, fs_hashtable_t *referenced_paks) {
	fsc_stream_t stream = {output, 0, output_size, 0};
	char buffer[50];
	int cgame_checksum = get_pure_checksum_for_file(fs_general_lookup("vm/cgame.qvm", LOOKUPFLAG_IGNORE_CURRENT_MAP, qfalse), checksum_feed);
	int ui_checksum = get_pure_checksum_for_file(fs_general_lookup("vm/ui.qvm", LOOKUPFLAG_IGNORE_CURRENT_MAP, qfalse), checksum_feed);

	Com_sprintf(buffer, sizeof(buffer), "%i %i @", cgame_checksum, ui_checksum);
	fsc_stream_append_string(&stream, buffer);

	if(fs_full_pure_validation->integer && connected_server_sv_pure != 2) {
		add_referenced_pure_pk3s(&stream, referenced_paks); }
	else {
		Com_sprintf(buffer, sizeof(buffer), " %i %i ", cgame_checksum, checksum_feed ^ cgame_checksum ^ 1);
		fsc_stream_append_string(&stream, buffer); } }

/* ******************************************************************************** */
// Referenced Paks
/* ******************************************************************************** */

// The "referenced_paks" set is filled by logging game references to pk3 files
// It currently serves two purposes:
// 1) To generate the pure validation string when fs_full_pure_validation is 1
//		(Although I have never seen any server that requires this to connect)
// 2) As a component of the download pak list creation
//		(Although in virtually all cases it is redundant to just using the current map)
// Basically this section could probably be deprecated with no noticable effects,
// but is kept for now just to be on the safe side for compatibility purposes

static reference_set_t referenced_paks;

void fs_register_reference(const fsc_file_t *file) {
	// Adds the source pk3 of the given file to the current referenced paks set
	if(file->sourcetype != FSC_SOURCETYPE_PK3) return;

	// Don't register references for certain extensions
	if(file->qp_ext_ptr) {
		int i;
		static const char *special_extensions[] = {"shader", "txt", "cfg", "config", "bot", "arena", "menu"};
		const char *extension = (const char *)STACKPTR(file->qp_ext_ptr);
		for(i=0; i<ARRAY_LEN(special_extensions); ++i) {
			if(!Q_stricmp(extension, special_extensions[i])) return; } }

	// Don't register reference in certain special cases
	if(!Q_stricmp((const char *)STACKPTR(file->qp_name_ptr), "qagame") &&
			file->qp_ext_ptr && !Q_stricmp((const char *)STACKPTR(file->qp_ext_ptr), "qvm") &&
			file->qp_dir_ptr && !Q_stricmp((const char *)STACKPTR(file->qp_dir_ptr), "vm")) return;
	if(file->qp_dir_ptr && !Q_stricmp((const char *)STACKPTR(file->qp_dir_ptr), "levelshots")) return;

	// Initialize referenced_paks if it isn't already
	if(!referenced_paks.bucket_count) {
		fs_hashtable_initialize(&referenced_paks, 32); }

	// Add the reference
	if(reference_set_add(&referenced_paks, (fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3), 0)) {
		if(fs_debug_references->integer) {
			char temp[FS_FILE_BUFFER_SIZE];
			fs_file_to_buffer(file, temp, sizeof(temp), qtrue, qtrue, qtrue, qfalse);
			Com_Printf("recording reference: %s\n", temp); } } }

void FS_ClearPakReferences( int flags ) {
	if(fs_debug_references->integer) Com_Printf("clearing referenced paks\n");
	fs_hashtable_reset(&referenced_paks, 0); }

const char *FS_ReferencedPakNames( void ) {
	// This is just used for a certain debug command now
	static char buffer[1000];
	reference_list_t reference_list;
	reference_set_to_reference_list(&referenced_paks, &reference_list);
	sort_reference_list(&reference_list, qfalse);
	reference_list_to_buffer(&reference_list, buffer, sizeof(buffer), reference_name_string);
	free_reference_list(&reference_list);
	return buffer; }

const char *FS_ReferencedPakPureChecksums( void ) {
	// Returns a space separated string containing the pure checksums of all referenced pk3 files.
	// Servers with sv_pure set will get this string back from clients for pure validation
	// The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
	static char buffer[1000];
	build_pure_validation_string(buffer, sizeof(buffer), &referenced_paks);
	return buffer; }

/* ******************************************************************************** */
// Download / Pure list building
/* ******************************************************************************** */

typedef struct {
	reference_set_t *target;
	int position;
	qboolean download;
	const char *rule_name;
} reference_set_work_t;

#define PAK_NAME(pak) char pak_name[FS_FILE_BUFFER_SIZE]; \
	fs_file_to_buffer((fsc_file_t *)pak, pak_name, sizeof(pak_name), qtrue, qtrue, qtrue, qfalse)

static qboolean is_pak_downloadable(reference_set_work_t *ref_work, fsc_file_direct_t *pak) {
	const char *mod_dir;
	char pak_name[FSC_MAX_QPATH];
	fsc_stream_t stream = {pak_name, 0, sizeof(pak_name), 0};
	reference_name_string(pak, &stream);

	// Don't put paks that fail the id pak check in download list because clients won't download
	// them anyway and may throw an error
#ifndef STANDALONE
	if(FS_idPak(pak_name, BASEGAME, FS_NODOWNLOAD_PAKS) || FS_idPak(pak_name, BASETA, FS_NODOWNLOAD_PAKS_TEAMARENA)) {
		if(fs_debug_references->integer) {
			Com_Printf("NOTE: Skipping pak reference due to ID pak name\n"); }
		return qfalse; }
#endif

	// Print warning if pak is from an inactive mod dir
	mod_dir = fsc_get_mod_dir((fsc_file_t *)pak, &fs);
	if(Q_stricmp(mod_dir, com_basegame->string) && Q_stricmp(mod_dir, current_mod_dir) &&
			Q_stricmp(mod_dir, "basemod")) {
		PAK_NAME(pak);
		Com_Printf("WARNING: Download list file %s from rule %s is from an inactive mod dir."
			" This will cause problems for original filesystem clients."
			" Consider moving this file or changing the active mod to include it.\n",
			pak_name, ref_work->rule_name); }

	return qtrue; }

static void reference_set_work_add(reference_set_work_t *ref_work, fsc_file_direct_t *pk3) {
	if(fs_debug_references->integer) {
		PAK_NAME(pk3);
		Com_Printf("********** %s list reference **********\n", ref_work->download ? "download" : "pure");
		Com_Printf("rule: %s\nposition: %i\npak: %s\nhash: %i\n",
				ref_work->rule_name, ref_work->position, pak_name, (int)pk3->pk3_hash); }
	if(ref_work->download && !is_pak_downloadable(ref_work, pk3)) return;
	reference_set_add(ref_work->target, pk3, ref_work->position); }

typedef enum {
	PAKCATEGORY_ACTIVE_MOD,
	PAKCATEGORY_BASEGAME,
	PAKCATEGORY_INACTIVE_MOD
} pakcategory_t;

static pakcategory_t get_pak_category(const fsc_file_direct_t *pak) {
	int mod_dir_state = fs_get_mod_dir_state(fsc_get_mod_dir((fsc_file_t *)pak, &fs));
	if(mod_dir_state == 3) return PAKCATEGORY_ACTIVE_MOD;
	if(mod_dir_state >= 1) return PAKCATEGORY_BASEGAME;
	return PAKCATEGORY_INACTIVE_MOD; }

static void add_paks_by_category(reference_set_work_t *ref_work, pakcategory_t category) {
	// Add all loaded paks in specified category to the reference set
	int i;
	fsc_hashtable_iterator_t hti;
	fsc_pk3_hash_map_entry_t *hash_entry;

	for(i=0; i<fs.pk3_hash_lookup.bucket_count; ++i) {
		fsc_hashtable_open(&fs.pk3_hash_lookup, i, &hti);
		while((hash_entry = (fsc_pk3_hash_map_entry_t *)STACKPTR(fsc_hashtable_next(&hti)))) {
			fsc_file_direct_t *pk3 = (fsc_file_direct_t *)STACKPTR(hash_entry->pk3);
			if(fs_file_disabled((fsc_file_t *)pk3, 0)) continue;
			if(get_pak_category(pk3) != category) continue;
			reference_set_work_add(ref_work, pk3); } } }

static void add_referenced_paks(reference_set_work_t *ref_work) {
	// Add all current referenced paks to the reference set
	fs_hashtable_iterator_t it = fs_hashtable_iterate(&referenced_paks, 0, qtrue);
	reference_set_entry_t *entry;
	while((entry = (reference_set_entry_t *)fs_hashtable_next(&it))) {
		reference_set_work_add(ref_work, entry->pak); } }

static void add_pak_containing_file(reference_set_work_t *ref_work, const char *name) {
	// Add the pak containing the specified file to the reference set
	const fsc_file_t *file = fs_general_lookup(name, LOOKUPFLAG_IGNORE_CURRENT_MAP|LOOKUPFLAG_PK3_SOURCE_ONLY, qfalse);
	if(!file || file->sourcetype != FSC_SOURCETYPE_PK3) {
		Com_Printf("WARNING: %s list rule %s failed to locate pk3\n",
				ref_work->download ? "Download" : "Pure", ref_work->rule_name);
		return; }
	reference_set_work_add(ref_work, (fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3)); }

static void add_pak_by_name(reference_set_work_t *ref_work, const char *name) {
	// Add all paks matching the specified filename to the reference set
	int count = 0;
	fsc_hashtable_iterator_t hti;
	const fsc_file_t *file;
	char mod_dir[FSC_MAX_MODDIR];
	char filename[FSC_MAX_QPATH];
	const char *remainder = 0;

	fsc_get_leading_directory(name, mod_dir, sizeof(mod_dir), &remainder);
	if(remainder) {
		if(!fs_generate_path(remainder, 0, 0, 0, 0, 0, filename, sizeof(filename))) return; }
	else {
		*mod_dir = 0;
		if(!fs_generate_path(name, 0, 0, 0, 0, 0, filename, sizeof(filename))) return; }

	fsc_hashtable_open(&fs.files, fsc_string_hash(filename, 0), &hti);
	while((file = (const fsc_file_t *)STACKPTR(fsc_hashtable_next(&hti)))) {
		if(file->sourcetype != FSC_SOURCETYPE_DIRECT) continue;
		if(!((fsc_file_direct_t *)file)->pk3_hash) continue;
		if(fs_file_disabled(file, 0)) continue;

		if(file->qp_dir_ptr) continue;
		if(!file->qp_ext_ptr || Q_stricmp((const char *)STACKPTR(file->qp_ext_ptr), "pk3")) continue;
		if(Q_stricmp((const char *)STACKPTR(file->qp_name_ptr), filename)) continue;
		if(*mod_dir && Q_stricmp(fsc_get_mod_dir(file, &fs), mod_dir)) continue;

		reference_set_work_add(ref_work, (fsc_file_direct_t *)file);
		++count; }

	if(count != 1) Com_Printf("WARNING: %s list rule %s %s\n",
			ref_work->download ? "Download" : "Pure", ref_work->rule_name,
			count ? "found multiple pk3s" : "failed to locate pk3"); }

static void add_paks_from_manifest_string(reference_set_t *target, const char *string, qboolean download) {
	reference_set_work_t ref_work = {target, 0, download, 0};

	while(1) {
		char *token = COM_ParseExt((char **)&string, qfalse);
		if(!*token) break;

		ref_work.rule_name = token;

		if(!Q_stricmp(token, "-")) {
			++ref_work.position; }
		else if(!Q_stricmp(token, "*mod_paks")) {
			add_paks_by_category(&ref_work, PAKCATEGORY_ACTIVE_MOD); }
		else if(!Q_stricmp(token, "*base_paks")) {
			add_paks_by_category(&ref_work, PAKCATEGORY_BASEGAME); }
		else if(!Q_stricmp(token, "*inactivemod_paks")) {
			add_paks_by_category(&ref_work, PAKCATEGORY_INACTIVE_MOD); }
		else if(!Q_stricmp(token, "*referenced_paks")) {
			add_referenced_paks(&ref_work); }
		else if(!Q_stricmp(token, "*currentmap_pak")) {
			add_pak_containing_file(&ref_work, va("maps/%s.bsp", Cvar_VariableString("mapname"))); }
		else if(!Q_stricmp(token, "*cgame_pak")) {
			add_pak_containing_file(&ref_work, "vm/cgame.qvm"); }
		else if(!Q_stricmp(token, "*ui_pak")) {
			add_pak_containing_file(&ref_work, "vm/ui.qvm"); }
		else {
			add_pak_by_name(&ref_work, token); } } }

/* ******************************************************************************** */
// Server Download List Handling
/* ******************************************************************************** */

static reference_set_t download_paks;

void fs_set_download_list(void) {
	// Called by server to set "sv_referencedPaks" and "sv_referencedPakNames"
	// Also updates the download_paks structure above which is used to match
	//    path string from the client to the actual pk3 path

	// Clear out old download set
	if(download_paks.bucket_count) fs_hashtable_reset(&download_paks, 0);
	else fs_hashtable_initialize(&download_paks, 32);

	// Add new download paks
	add_paks_from_manifest_string(&download_paks, fs_download_manifest->string, qtrue);

	// Generate and set the reference strings
	{	reference_list_t reference_list;
		char reference_hashes[1000];
		char reference_names[1000];
		qboolean overflow = qfalse;

		reference_set_to_reference_list(&download_paks, &reference_list);
		sort_reference_list(&reference_list, qfalse);
		if(!reference_list_to_buffer(&reference_list, reference_hashes, sizeof(reference_hashes),
				reference_hash_string)) overflow = qtrue;
		if(!reference_list_to_buffer(&reference_list, reference_names, sizeof(reference_names),
				reference_name_string)) overflow = qtrue;
		free_reference_list(&reference_list);

		if(overflow) {
			Com_Printf("WARNING: Download list overflowed\n");
			Cvar_Set("sv_referencedPaks", "");
			Cvar_Set("sv_referencedPakNames", ""); }
		else {
			Cvar_Set("sv_referencedPaks", reference_hashes);
			Cvar_Set("sv_referencedPakNames", reference_names); } } }

fileHandle_t fs_open_download_pak(const char *path, unsigned int *size_out) {
	// Used by the server UDP download system to open paks instead of the regular
	//    read functions, because the download path may not correspond exactly
	//    to the filesystem path for various reasons
	// Also uses a direct read handle instead of the normal read method to avoid
	//    lagging the server by reading a big pk3 all at once
	char buffer[FSC_MAX_QPATH];
	fs_hashtable_iterator_t it = fs_hashtable_iterate(&download_paks, 0, qtrue);
	reference_set_entry_t *entry;

	// Search all the current referenced paks to find one with a matching path
	while((entry = (reference_set_entry_t *)fs_hashtable_next(&it))) {
		fsc_stream_t stream = {buffer, 0, sizeof(buffer), 0};
		reference_name_string(entry->pak, &stream);
		fsc_stream_append_string(&stream, ".pk3");
		if(!Q_stricmp(stream.data, path)) {
			// Found match
			return fs_direct_read_handle_open((fsc_file_t *)entry->pak, 0, size_out); } }

	return 0; }

/* ******************************************************************************** */
// Server Pure List Handling
/* ******************************************************************************** */

#define SYSTEMINFO_RESERVED_SIZE 256
static void fs_set_pure_list2(reference_list_t *pure_list) {
	char hash_string[BIG_INFO_STRING];
	char name_string[BIG_INFO_STRING];
	int systeminfo_base_length;

	if(!reference_list_to_buffer(pure_list, hash_string, sizeof(hash_string), reference_hash_string)) {
		Com_Printf("WARNING: Setting sv_pure to 0 due to pure list overflow. Remove some"
			" paks from the server or adjust the pure manifest if you want to use sv_pure.\n");
		Cvar_Set("sv_pure", 0);
		return; }

	if(!*hash_string) {
		Com_Printf("WARNING: Setting sv_pure to 0 due to empty pure list.\n");
		Cvar_Set("sv_pure", "0");
		return; }

	systeminfo_base_length = strlen(Cvar_InfoString_Big(CVAR_SYSTEMINFO));
	if(systeminfo_base_length + strlen(hash_string) + SYSTEMINFO_RESERVED_SIZE >= BIG_INFO_STRING) {
		Com_Printf("WARNING: Setting sv_pure to 0 due to systeminfo overflow. Remove some"
			" paks from the server or adjust the pure manifest if you want to use sv_pure.\n");
		Cvar_Set("sv_pure", "0");
		return; }

	Cvar_Set("sv_paks", hash_string);

	// It should be fine to leave sv_pakNames empty if it overflowed since it is normally
	// only used for informational purposes anyway
	if(reference_list_to_buffer(pure_list, name_string, sizeof(name_string), reference_name_string) &&
			systeminfo_base_length + strlen(hash_string) + strlen(name_string) +
			SYSTEMINFO_RESERVED_SIZE < BIG_INFO_STRING) {
		Cvar_Set("sv_pakNames", name_string); } }

void fs_set_pure_list(void) {
	Cvar_Set("sv_paks", "");
	Cvar_Set("sv_pakNames", "");

	if(Cvar_VariableIntegerValue("sv_pure")) {
		fs_hashtable_t pure_list_set;
		reference_list_t pure_list;
		fs_hashtable_initialize(&pure_list_set, 2048);
		add_paks_from_manifest_string(&pure_list_set, fs_pure_manifest->string, qfalse);
		reference_set_to_reference_list(&pure_list_set, &pure_list);
		fs_hashtable_free(&pure_list_set, 0);
		sort_reference_list(&pure_list, qfalse);
		fs_set_pure_list2(&pure_list);
		free_reference_list(&pure_list); } }

#endif	// NEW_FILESYSTEM
