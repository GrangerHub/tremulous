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

/* ******************************************************************************** */
// Path Handling Functions
/* ******************************************************************************** */

static void fs_mkdir(const char *directory) {
	void *os_path = fsc_string_to_os_path(directory);
	fsc_mkdir(os_path);
	fsc_free(os_path); }

static void fs_mkdir_in_range(char *base, char *position, qboolean for_file) {
	// Base and position should be part of the same string, where base represents
	//    the beginning of the string and position is the part where actual directory
	//    creation starts.
	// If for_file is set, the final part of the path will not be created as a directory.
	while(1) {
		if(*position == '/') {
			*position = 0;
			fs_mkdir(base);
			*position = '/'; }
		if(!*position) {
			if(!for_file) fs_mkdir(base);
			return; }
		++position; } }

static qboolean fs_generate_subpath(fsc_stream_t *stream, const char *path, int flags) {
	// Writes path to stream with sanitization and other operations based on flags
	// Returns qtrue on success, qfalse on error
	int old_position = stream->position;

	if(flags & FS_NO_SANITIZE) {
		// If sanitize disabled, just write out the string
		fsc_stream_append_string(stream, path); }

	else {
		// Perform sanitize
		const char *conversion_table = fsc_get_qpath_conversion_table();
		const char *index, *start, *end;
		char sanitized_path[128];
		int sanitized_path_length = 0;

		// Determine the range of alphanumeric characters
		index = path;
		start = end = 0;
		while(*index) {
			char current = conversion_table[*(unsigned char *)index];
			if((current >= '0' && current <= '9') || (current >= 'a' && current <= 'z') ||
					(current >= 'A' && current <= 'Z')) {
				if(!start) start = index;
				end = index; }
			++index; }
		if(!start) return qfalse;

		// Generate sanitized path
		for(index=start; index<=end; ++index) {
			char current = conversion_table[*(unsigned char *)index];
			if(current == '/' && !(flags & FS_ALLOW_SLASH)) current = '_';
			if(sanitized_path_length >= sizeof(sanitized_path)-1) continue;
			sanitized_path[sanitized_path_length++] = current; }
		sanitized_path[sanitized_path_length] = 0;

		// Check for possible backwards path
		if(strstr(sanitized_path, "..")) return qfalse;
		if(strstr(sanitized_path, "::")) return qfalse;

		// Check for disallowed extensions
		if(!Q_stricmp(sanitized_path + sanitized_path_length - 4, ".qvm")) return qfalse;
		if(!(flags & FS_ALLOW_PK3) && sanitized_path_length >= 4 &&
				!Q_stricmp(sanitized_path + sanitized_path_length - 4, ".pk3")) return qfalse;
		if(!(flags & FS_ALLOW_DLL)) {
			if(Sys_DllExtension(sanitized_path)) return qfalse;
			// Do some extra checks to be sure
			if(sanitized_path_length >= 4 &&
					!Q_stricmp(sanitized_path + sanitized_path_length - 4, ".dll")) return qfalse;
			if(sanitized_path_length >= 3 &&
					!Q_stricmp(sanitized_path + sanitized_path_length - 3, ".so")) return qfalse;
			if(sanitized_path_length >= 6 &&
					!Q_stricmp(sanitized_path + sanitized_path_length - 6, ".dylib")) return qfalse; }
		if(!(flags & FS_ALLOW_SPECIAL_CFG) && (!Q_stricmp(sanitized_path, Q3CONFIG_CFG) ||
				!Q_stricmp(sanitized_path, "autoexec.cfg"))) return qfalse;

		// Write out the string
		fsc_stream_append_string(stream, sanitized_path); }

	// Create directories for path
	if(flags & FS_CREATE_DIRECTORIES_FOR_FILE) {
		fs_mkdir_in_range(stream->data, stream->data + old_position, qtrue); }
	else if(flags & FS_CREATE_DIRECTORIES) {
		fs_mkdir_in_range(stream->data, stream->data + old_position, qfalse); }

	return qtrue; }

unsigned int fs_generate_path(const char *path1, const char *path2, const char *path3,
		int path1_flags, int path2_flags, int path3_flags,
		char *target, unsigned int target_size) {
	// Concatenates paths, adding '/' character as seperator, with sanitization
	//    and directory creation based on flags
	// Returns output length on success, 0 on error (overflow or sanitize fail)

	fsc_stream_t stream = {target, 0, target_size, 0};

	if(path1) {
		if(!fs_generate_subpath(&stream, path1, path1_flags)) return 0; }

	if(path2) {
		if(path1) fsc_stream_append_string(&stream, "/");
		if(!fs_generate_subpath(&stream, path2, path2_flags)) return 0; }

	if(path3) {
		if(path1 || path2) fsc_stream_append_string(&stream, "/");
		if(!fs_generate_subpath(&stream, path3, path3_flags)) return 0; }

	if(!stream.position || stream.overflowed) {
		*target = 0;
		return 0; }
	return stream.position; }

unsigned int fs_generate_path_sourcedir(int source_dir_id, const char *path1, const char *path2,
		int path1_flags, int path2_flags, char *target, unsigned int target_size) {
	// Generates path prefixed by source directory
	if(!fs_sourcedirs[source_dir_id].active) return 0;
	return fs_generate_path(fs_sourcedirs[source_dir_id].path_cvar->string, path1, path2, FS_NO_SANITIZE,
				path1_flags, path2_flags, target, target_size); }

unsigned int fs_generate_path_writedir(const char *path1, const char *path2, int path1_flags, int path2_flags,
		char *target, unsigned int target_size) {
	// Generates path prefixed by write directory
	if(fs_read_only) return 0;
	return fs_generate_path_sourcedir(0, path1, path2, path1_flags, path2_flags, target, target_size); }

/* ******************************************************************************** */
// Direct file access functions
/* ******************************************************************************** */

void *fs_open_file(const char *path, const char *mode) {
	// This is basically a string-path version of fsc_open_file
	void *os_path = fsc_string_to_os_path(path);
	void *handle = fsc_open_file(os_path, mode);
	fsc_free(os_path);
	return handle; }

void fs_rename_file(const char *source, const char *target) {
	// Basically a string-path version of fsc_rename_file
	void *source_os_path = fsc_string_to_os_path(source);
	void *target_os_path = fsc_string_to_os_path(target);
	fsc_rename_file(source_os_path, target_os_path);
	fsc_free(source_os_path);
	fsc_free(target_os_path); }

void fs_delete_file(const char *path) {
	// Basically a string-path version of fsc_delete_file
	void *os_path = fsc_string_to_os_path(path);
	fsc_delete_file(os_path);
	fsc_free(os_path); }

void FS_HomeRemove( const char *homePath ) {
	char path[FS_MAX_PATH];
	if(!fs_generate_path_writedir(FS_GetCurrentGameDir(), homePath, 0, FS_ALLOW_SLASH,
				path, sizeof(path))) {
		Com_Printf("WARNING: FS_HomeRemove on %s failed due to invalid path\n", homePath);
		return; }

	fs_delete_file(path); }

qboolean FS_FileInPathExists(const char *testpath) {
	void *handle = fs_open_file(testpath, "rb");
	if(handle) {
		fsc_fclose(handle);
		return qtrue; }
	return qfalse; }

qboolean FS_FileExists(const char *file) {
	char path[FS_MAX_PATH];
	if(!fs_generate_path_sourcedir(0, FS_GetCurrentGameDir(), file, 0, FS_ALLOW_SLASH,
			path, sizeof(path))) return qfalse;
	return FS_FileInPathExists(path); }

/* ******************************************************************************** */
// File read cache
/* ******************************************************************************** */

typedef struct cache_entry_s {
	unsigned int size;
	int lock_count;
	int stage;

	const fsc_file_t *file;
	unsigned int file_size;
	unsigned int file_timestamp;

	struct cache_entry_s *next_position;
	unsigned int lookup_hash;
	struct cache_entry_s *next_lookup;
	struct cache_entry_s *prev_lookup;
} cache_entry_t;

// ***** Cache lookup table *****

#define CACHE_LOOKUP_TABLE_SIZE 4096
static cache_entry_t *cache_lookup_table[CACHE_LOOKUP_TABLE_SIZE];

static unsigned int cache_hash(const fsc_file_t *file) {
	if(!file) return 0;
	return fsc_string_hash((const char *)STACKPTR(file->qp_name_ptr), (const char *)STACKPTR(file->qp_dir_ptr)); }

static void cache_lookup_table_register(cache_entry_t *entry) {
	int position = entry->lookup_hash % CACHE_LOOKUP_TABLE_SIZE;
	entry->next_lookup = cache_lookup_table[position];
	entry->prev_lookup = 0;
	if(cache_lookup_table[position]) cache_lookup_table[position]->prev_lookup = entry;
	cache_lookup_table[position] = entry; }

static void cache_lookup_table_deregister(cache_entry_t *entry) {
	int position = entry->lookup_hash % CACHE_LOOKUP_TABLE_SIZE;
	if(entry->next_lookup) entry->next_lookup->prev_lookup = entry->prev_lookup;
	if(entry->prev_lookup) entry->prev_lookup->next_lookup = entry->next_lookup;
	else cache_lookup_table[position] = entry->next_lookup; }

static void cache_lookup_table_deregister_range(cache_entry_t *start, cache_entry_t *end) {
	while(start && start != end) {
		cache_lookup_table_deregister(start);
		start = start->next_position; } }

static qboolean cache_entry_matches_file(const fsc_file_t *file, const cache_entry_t *cache_entry) {
	// We need a little more extensive check than just comparing the file pointer,
	// because fsc_load_file can reuse an existing file object for a modified file in specific cases
	if(file != cache_entry->file) return qfalse;
	if(cache_entry->file_size != cache_entry->file->filesize) return qfalse;
	if(file->sourcetype == FSC_SOURCETYPE_DIRECT &&
			cache_entry->file_timestamp != ((fsc_file_direct_t *)file)->os_timestamp) return qfalse;
	return qtrue; }

static cache_entry_t *cache_lookup_search(const fsc_file_t *file) {
	cache_entry_t *entry = cache_lookup_table[cache_hash(file) % CACHE_LOOKUP_TABLE_SIZE];
	cache_entry_t *best_entry = 0;

	while(entry) {
		if((!best_entry || entry->stage > best_entry->stage) && cache_entry_matches_file(file, entry)) {
			best_entry = entry; }
		entry = entry->next_lookup; }

	return best_entry; }

// ***** Cache data store *****

#define CACHE_ENTRY_DATA(cache_entry) ((char *)(cache_entry) + sizeof(cache_entry_t))
#define CACHE_ALIGN(ptr) (((uintptr_t)(ptr) + 15) & ~15)

int cache_stage = 0;
int cache_size = 0;
cache_entry_t *base_entry;
cache_entry_t *head_entry;	// Last entry created. Null if just initialized.

static cache_entry_t *cache_allocate(const fsc_file_t *file, unsigned int size) {
	unsigned int required_space = size + sizeof(cache_entry_t);
	int wrapped_around = 0;
	cache_entry_t *lead_entry = head_entry;		// Entry preceding new entry (can be null)
	cache_entry_t *limit_entry = lead_entry ? lead_entry->next_position : 0;	// Entry following new entry (can be null)
	char *start_point, *end_point;	// Range of memory available for new entry: [start, end)
	cache_entry_t *new_entry;

	while(1) {
		// Check if we have enough space yet
		start_point = (char *)CACHE_ALIGN(lead_entry ? (char *)lead_entry + lead_entry->size
				+ sizeof(cache_entry_t) : (char *)base_entry);
		end_point = limit_entry ? (char *)limit_entry : (char *)base_entry + cache_size;
		if(end_point < start_point) {
			Com_Error(ERR_FATAL, "fscache buffer position fault"); }
		if(end_point - start_point >= required_space) break;

		// Wraparound check
		if(!limit_entry) {
			if(!head_entry || wrapped_around++) return 0;
			lead_entry = 0;
			limit_entry = base_entry;
			continue; }

		// Don't advance limit over a locked entry
		while(limit_entry && limit_entry->lock_count) {
			lead_entry = limit_entry;
			limit_entry = lead_entry->next_position; }

		// Advance limit
		if(limit_entry) limit_entry = limit_entry->next_position; }

	// We have space for a new entry
	new_entry = (cache_entry_t *)start_point;

	// Deregister entries we are overwriting
	if(lead_entry) {
		cache_lookup_table_deregister_range(lead_entry->next_position, limit_entry);
		lead_entry->next_position = new_entry; }
	else if(head_entry) {
		cache_lookup_table_deregister_range(base_entry, limit_entry); }

	new_entry->next_position = limit_entry;
	head_entry = new_entry;

	new_entry->size = size;
	new_entry->lock_count = 0;
	new_entry->stage = cache_stage;
	new_entry->file = file;
	new_entry->file_size = file ? file->filesize : 0;
	new_entry->file_timestamp = file && file->sourcetype == FSC_SOURCETYPE_DIRECT ? ((fsc_file_direct_t *)file)->os_timestamp : 0;
	new_entry->lookup_hash = cache_hash(file);

	cache_lookup_table_register(new_entry);

	return new_entry; }

void fs_cache_initialize(void) {
	// This gets called directly from Com_Init, after the config files have been read,
	// to allow setting fs_read_cache_megs in the normal config file instead of the command line.
#ifdef DEDICATED
	cvar_t *cache_megs_cvar = Cvar_Get("fs_read_cache_megs", "4", CVAR_LATCH|CVAR_ARCHIVE);
#else
	cvar_t *cache_megs_cvar = Cvar_Get("fs_read_cache_megs", "64", CVAR_LATCH|CVAR_ARCHIVE);
#endif
	int cache_megs = cache_megs_cvar->integer;
	if(cache_megs < 0) cache_megs = 0;
	if(cache_megs > 1024) cache_megs = 1024;

	cache_size = cache_megs << 20;
	base_entry = (cache_entry_t *)fsc_malloc(cache_size);
	head_entry = 0; }

void fs_advance_cache_stage(void) {
	// Causes existing files in cache to be recopied to the front of the cache on reference
	// This may be called between level loads to help with performance
	// It should not have any functional impact; it is only for optimization purposes
	++cache_stage; }

// ***** Cache debugging *****

static int fs_cache_entrycount_direct(void) {
	cache_entry_t *entry = base_entry;
	int count = 0;

	if(!head_entry) return 0;
	do {
		++count;
	} while((entry = entry->next_position));

	return count; }

static int fs_cache_entrycount_lookuptable(void) {
	int i;
	cache_entry_t *entry;
	int count = 0;

	for(i=0; i<CACHE_LOOKUP_TABLE_SIZE; ++i) {
		entry = cache_lookup_table[i];
		while(entry) {
			++count;
			entry = entry->next_lookup; } }

	return count; }

void fs_readcache_debug(void) {
	// Debug prints the contents of the cache
	cache_entry_t *entry = base_entry;
	char data[1000];
	fsc_stream_t stream = {data, 0, sizeof(data), 0};
	int index_counter = 0;

	if(!head_entry) return;

	#define ADD_STRING(string) fsc_stream_append_string(&stream, string)

	do {
		stream.position = 0;
		if(!entry->file) {
			ADD_STRING(va("Null File Index(%i) Position(%i) Size(%i) Stage(%i) Lockcount(%i)",
					index_counter, (int)((char *)entry - (char *)base_entry), entry->size, entry->stage, entry->lock_count));
		} else {
			ADD_STRING("File(");
			fs_file_to_stream(entry->file, &stream, qtrue, qtrue, qtrue, qfalse);
			ADD_STRING(va(") Index(%i) Position(%i) Size(%i) Stage(%i) Lockcount(%i)",
					index_counter, (int)((char *)entry - (char *)base_entry), entry->size, entry->stage, entry->lock_count)); }
		if(entry == head_entry) ADD_STRING(" <head entry>");
		ADD_STRING("\n\n");
		Com_Printf("%s", stream.data);
		++index_counter;
	} while((entry = entry->next_position));

	Com_Printf("entry count from direct iteration: %i\n", fs_cache_entrycount_direct());
	Com_Printf("entry count from lookup table: %i\n", fs_cache_entrycount_lookuptable()); }

/* ******************************************************************************** */
// Data reading
/* ******************************************************************************** */

static cache_entry_t *cache_search_current_stage(const fsc_file_t *file) {
	// Performs cache search, and if entry doesn't match current stage, try to create current stage duplicate.
	cache_entry_t *entry = cache_lookup_search(file);
	if(!entry) return 0;

	if(entry->stage != cache_stage) {
		cache_entry_t *new_entry;
		++entry->lock_count;
		new_entry = cache_allocate(file, entry->size);
		--entry->lock_count;
		if(new_entry) {
			fsc_memcpy(CACHE_ENTRY_DATA(new_entry), CACHE_ENTRY_DATA(entry), entry->size);
			return new_entry; } }

	return entry; }

char *fs_read_data(const fsc_file_t *file, const char *path, unsigned int *size_out) {
	// Input can be either file or path, not both.
	// Returns null on error, otherwise result needs to be freed by fs_free_data.

	cache_entry_t *cache_entry = 0;
	char *data = 0;
	void *os_path = 0;
	void *fsc_file_handle = 0;
	unsigned int size;

	// Ensure we have file or path set but not both
	if((file && path) || (!file && !path)) Com_Error(ERR_DROP, "Invalid parameters to fs_read_data.");

	// Mark the file in reference tracking
	if(file) fs_register_reference(file);

	// Print leading debug info
	if(fs_debug_fileio->integer) {
		if(file) {
			char buffer[FS_FILE_BUFFER_SIZE];
			fs_file_to_buffer(file, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
			Com_Printf("********** read data from indexed file **********\nfile: %s\n", buffer); }
		else {
			Com_Printf("********** read data from path **********\npath: %s\n", path); } }

	// Check if file is already available from cache
	if(file) {
		cache_entry = cache_search_current_stage(file);
		if(cache_entry) {
			++cache_entry->lock_count;
			if(size_out) *size_out = cache_entry->size - 1;
			if(fs_debug_fileio->integer) Com_Printf("result: loaded %u bytes from cache\n", cache_entry->size - 1);
			return CACHE_ENTRY_DATA(cache_entry); } }

	// Derive os_path in case of path parameter or direct sourcetype file
	if(path) {
		os_path = fsc_string_to_os_path(path);
		if(!os_path) goto error; }
	else if(file && file->sourcetype == FSC_SOURCETYPE_DIRECT) os_path = STACKPTR(((fsc_file_direct_t *)file)->os_path_ptr);

	// Obtain handle (if applicable) and size (including null terminating byte)
	if(os_path) {
		fsc_file_handle = fsc_open_file(os_path, "rb");
		if(path) fsc_free(os_path);
		if(!fsc_file_handle) goto error;

		fsc_fseek(fsc_file_handle, 0, FSC_SEEK_END);
		size = fsc_ftell(fsc_file_handle);

		fsc_fseek(fsc_file_handle, 0, FSC_SEEK_SET); }
	else size = file->filesize;

	// Set a file size limit of about 2GB as a catch-all to avoid overflow conditions
	// The game shouldn't normally need to read such big files
	if(size > 2147418112) {
		Com_Printf("WARNING: Excessive file size in fs_read_data\n");
		goto error; }

	// Obtain buffer from cache or malloc
	if(size < cache_size / 3) {
		// Don't use more than 1/3 of the cache for a single file to avoid flushing smaller files
		cache_entry = cache_allocate(file, size + 1); }
	if(cache_entry) {
		++cache_entry->lock_count;
		data = CACHE_ENTRY_DATA(cache_entry); }
	else data = (char *)fsc_malloc(size + 1);

	// Extract data into buffer
	if(fsc_file_handle) {
		int read_size = fsc_fread(data, size + 1, fsc_file_handle);
		fsc_fclose(fsc_file_handle);
		if(read_size != size) goto error; }
	else {
		if(fsc_extract_file(file, data, &fs, 0)) goto error; }
	data[size] = 0;

	if(size_out) *size_out = size;
	if(fs_debug_fileio->integer) Com_Printf("result: loaded %u bytes from file\n", size);
	return data;

	// Free buffer if there was an error extracting data
	error:
	if(fs_debug_fileio->integer) Com_Printf("result: failed to load file\n");
	if(cache_entry) {
		cache_entry->file = 0;
		cache_entry->lock_count = 0; }
	else if(data) fsc_free(data);
	if(size_out) *size_out = 0;
	return 0; }

void fs_free_data(char *data) {
	if(!data) Com_Error(ERR_DROP, "Null parameter to fs_free_data.");
	if(data >= (char *)base_entry && data < (char *)base_entry + cache_size) {
		cache_entry_t *cache_entry = (cache_entry_t *)(data - sizeof(cache_entry_t));
		if(cache_entry->lock_count <= 0) Com_Error(ERR_DROP, "fs_free_data on invalid or already freed entry.");
		--cache_entry->lock_count; }
	else {
		fsc_free(data); } }

char *fs_read_shader(const fsc_shader_t *shader) {
	// Returns shader text allocated in Z_Malloc, or null if there was an error
	unsigned int size = shader->end_position - shader->start_position;
	char *source_data;
	char *shader_data;

	if(size > 10000) return 0;
	source_data = fs_read_data((const fsc_file_t *)STACKPTR(shader->source_file_ptr), 0, 0);
	if(!source_data) return 0;

	shader_data = (char *)Z_Malloc(size + 1);
	fsc_memcpy(shader_data, source_data + shader->start_position, size);
	shader_data[size] = 0;
	fs_free_data(source_data);
	return shader_data; }

/* ******************************************************************************** */
// File handle management
/* ******************************************************************************** */

typedef enum {
	FS_HANDLE_INVALID,
	FS_HANDLE_CACHE_READ,
	FS_HANDLE_DIRECT_READ,
	FS_HANDLE_PK3_READ,
	FS_HANDLE_WRITE,
	FS_HANDLE_PIPE
} fs_handle_type_t;

typedef struct {
	fs_handle_type_t type;
	fs_handle_owner_t owner;
	char *debug_path;
} fs_handle_t;

typedef struct {
	fs_handle_t h;
	char *data;
	unsigned int position;
	unsigned int size;
} fs_cache_read_handle_t;

typedef struct {
	fs_handle_t h;
	void *fsc_handle;
} fs_direct_read_handle_t;

typedef struct {
	fs_handle_t h;
	const fsc_file_frompk3_t *file;
	void *fsc_handle;
	unsigned int position;
} fs_pk3_read_handle_t;

typedef struct {
	fs_handle_t h;
	void *fsc_handle;
	qboolean sync;
} fs_write_handle_t;

typedef struct {
	fs_handle_t h;
	FILE *handle;
} fs_pipe_handle_t;

// Note fileHandle_t values are incremented by 1 compared to the indices in this array.
#define MAX_HANDLES 64
fs_handle_t *handles[MAX_HANDLES];

static fileHandle_t fs_allocate_handle(fs_handle_type_t type) {
	int index;
	int size;

	// Locate free handle
	for(index=0; index<MAX_HANDLES; ++index) {
		if(!handles[index]) break; }
	if(index >= MAX_HANDLES) Com_Error(ERR_DROP, "fs_allocate_handle failed to find free handle");

	// Get size
	switch(type) {
		case FS_HANDLE_CACHE_READ: size = sizeof(fs_cache_read_handle_t); break;
		case FS_HANDLE_DIRECT_READ: size = sizeof(fs_direct_read_handle_t); break;
		case FS_HANDLE_PK3_READ: size = sizeof(fs_pk3_read_handle_t); break;
		case FS_HANDLE_WRITE: size = sizeof(fs_write_handle_t); break;
		case FS_HANDLE_PIPE: size = sizeof(fs_pipe_handle_t); break;
		default: Com_Error(ERR_DROP, "fs_allocate_handle with invalid type"); }

	// Allocate
	handles[index] = (fs_handle_t *)Z_Malloc(size);
	handles[index]->type = type;
	handles[index]->owner = FS_HANDLEOWNER_SYSTEM;
	return index + 1; }

static void fs_free_handle(fileHandle_t handle) {
	int index = handle - 1;
	if(index < 0 || index > MAX_HANDLES || !handles[index]) Com_Error(ERR_DROP, "fs_free_handle on invalid handle");
	Z_Free(handles[index]);
	handles[index] = 0; }

static fs_handle_t *fs_get_handle_entry(fileHandle_t handle) {
	// Returns null if handle is invalid
	int index = handle - 1;
	if(index < 0 || index > MAX_HANDLES || !handles[index]) return 0;
	return handles[index]; }

/* ******************************************************************************** */
// Cache read handle operations
/* ******************************************************************************** */

static fileHandle_t fs_cache_read_handle_open(const fsc_file_t *file, const char *path, unsigned int *size_out) {
	// Only file or path should be set, not both
	// Does not include sanity check on path
	// Returns handle on success, null on error

	fileHandle_t handle = fs_allocate_handle(FS_HANDLE_CACHE_READ);
	fs_cache_read_handle_t *handle_entry = (fs_cache_read_handle_t *)fs_get_handle_entry(handle);

	// Set up handle entry
	handle_entry->data = fs_read_data(file, path, &handle_entry->size);
	if(!handle_entry->data) {
		fs_free_handle(handle);
		return 0; }
	handle_entry->position = 0;

	// Set debug path
	if(file) {
		char buffer[FS_FILE_BUFFER_SIZE];
		fs_file_to_buffer(file, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
		handle_entry->h.debug_path = CopyString(buffer); }
	else {
		handle_entry->h.debug_path = CopyString(path); }

	if(size_out) *size_out = handle_entry->size;
	return handle; }

static unsigned int fs_cache_read_handle_read(char *buffer, unsigned int length, fs_cache_read_handle_t *handle_entry) {
	// Don't read past end of file...
	if(length > handle_entry->size - handle_entry->position) length = handle_entry->size - handle_entry->position;

	// Read data to buffer and advance position
	fsc_memcpy(buffer, handle_entry->data + handle_entry->position, length);
	handle_entry->position += length;
	return length; }

static int fs_cache_read_handle_seek(fs_cache_read_handle_t *handle_entry, int offset, fsOrigin_t origin_mode) {
	unsigned int origin;
	unsigned int offset_origin;

	// Get origin
	switch(origin_mode) {
		case FS_SEEK_CUR: origin = handle_entry->position; break;
		case FS_SEEK_END: origin = handle_entry->size; break;
		case FS_SEEK_SET: origin = 0; break;
		default: Com_Error(ERR_DROP, "fs_cache_read_handle_seek with invalid origin mode"); }

	// Get offset_origin and correct overflow conditions
	offset_origin = origin + offset;
	if(offset < 0 && offset_origin > origin) offset_origin = 0;
	if((offset > 0 && offset_origin < origin) || offset_origin > handle_entry->size) offset_origin = handle_entry->size;

	// Write the new position
	handle_entry->position = offset_origin;

	if(offset_origin == origin + offset) return 0;
	return -1; }

static void fs_cache_read_handle_close(fs_cache_read_handle_t *handle_entry) {
	fs_free_data(handle_entry->data); }

/* ******************************************************************************** */
// Direct read handle operations
/* ******************************************************************************** */

fileHandle_t fs_direct_read_handle_open(const fsc_file_t *file, const char *path, unsigned int *size_out) {
	// Only file or path should be set, not both
	// Does not include sanity check on path
	// Returns handle on success, null on error
	char debug_path[FS_MAX_PATH];
	void *os_path;
	void *fsc_handle;
	fileHandle_t handle;
	fs_direct_read_handle_t *handle_entry;

	if(file) {
		if(file->sourcetype != FSC_SOURCETYPE_DIRECT) Com_Error(ERR_FATAL, "fs_direct_read_handle_open on non direct file");
		os_path = STACKPTR(((fsc_file_direct_t *)file)->os_path_ptr);
		fs_file_to_buffer((fsc_file_t *)file, debug_path, sizeof(debug_path), qtrue, qtrue, qtrue, qfalse); }
	else {
		os_path = fsc_string_to_os_path(path);
		Q_strncpyz(debug_path, path, sizeof(debug_path)); }

	if(fs_debug_fileio->integer) Com_Printf("********** opening direct read handle **********\npath: %s\n", debug_path);

	fsc_handle = fsc_open_file(os_path, "rb");
	if(!file) fsc_free(os_path);
	if(!fsc_handle) {
		if(fs_debug_fileio->integer) Com_Printf("result: failed to open file\n");
		return 0; }

	// Set up handle entry
	handle = fs_allocate_handle(FS_HANDLE_DIRECT_READ);
	handle_entry = (fs_direct_read_handle_t *)fs_get_handle_entry(handle);
	handle_entry->fsc_handle = fsc_handle;
	handle_entry->h.debug_path = CopyString(debug_path);

	// Get size
	if(size_out) {
		fsc_fseek(fsc_handle, 0, FSC_SEEK_END);
		*size_out = fsc_ftell(fsc_handle);
		fsc_fseek(fsc_handle, 0, FSC_SEEK_SET); }

	if(fs_debug_fileio->integer) Com_Printf("result: success\n");
	return handle; }

static unsigned int fs_direct_read_handle_read(char *buffer, unsigned int length, fs_direct_read_handle_t *handle_entry) {
	return fsc_fread(buffer, length, handle_entry->fsc_handle); }

static int fs_direct_read_handle_seek(fs_direct_read_handle_t *handle_entry, int offset, fsOrigin_t origin_mode) {
	// Get type
	fsc_seek_type_t type;
	switch(origin_mode) {
		case FS_SEEK_CUR: type = FSC_SEEK_CUR; break;
		case FS_SEEK_END: type = FSC_SEEK_END; break;
		case FS_SEEK_SET: type = FSC_SEEK_SET; break;
		default: Com_Error(ERR_DROP, "fs_direct_read_handle_seek with invalid origin mode"); }
	return fsc_fseek(handle_entry->fsc_handle, offset, type); }

static void fs_direct_read_handle_close(fs_direct_read_handle_t *handle_entry) {
	fsc_fclose(handle_entry->fsc_handle); }

/* ******************************************************************************** */
// Pk3 read handle operations
/* ******************************************************************************** */

static fileHandle_t fs_pk3_read_handle_open(const fsc_file_t *file) {
	// Returns handle on success, null on error
	char debug_path[FS_MAX_PATH];
	void *fsc_handle;
	fileHandle_t handle;
	fs_pk3_read_handle_t *handle_entry;

	if(file->sourcetype != FSC_SOURCETYPE_PK3) Com_Error(ERR_FATAL, "fs_pk3_read_handle_open on non pk3 file");
	fs_file_to_buffer((fsc_file_t *)file, debug_path, sizeof(debug_path), qtrue, qtrue, qtrue, qfalse);

	if(fs_debug_fileio->integer) Com_Printf("********** opening pk3 read handle **********\nfile: %s\n", debug_path);

	fsc_handle = fsc_pk3_handle_open((fsc_file_frompk3_t *)file, 16384, &fs, 0);
	if(!fsc_handle) {
		if(fs_debug_fileio->integer) Com_Printf("result: failed to open file\n");
		return 0; }

	// Set up handle entry
	handle = fs_allocate_handle(FS_HANDLE_PK3_READ);
	handle_entry = (fs_pk3_read_handle_t *)fs_get_handle_entry(handle);
	handle_entry->file = (fsc_file_frompk3_t *)file;
	handle_entry->fsc_handle = fsc_handle;
	handle_entry->position = 0;
	handle_entry->h.debug_path = CopyString(debug_path);

	if(fs_debug_fileio->integer) Com_Printf("result: success\n");
	return handle; }

static unsigned int fs_pk3_read_handle_read(char *buffer, unsigned int length, fs_pk3_read_handle_t *handle_entry) {
	unsigned int max_length = handle_entry->file->f.filesize - handle_entry->position;
	if(length > max_length) length = max_length;
	length = fsc_pk3_handle_read(handle_entry->fsc_handle, buffer, length);
	handle_entry->position += length;
	return length; }

static int fs_pk3_read_handle_seek(fs_pk3_read_handle_t *handle_entry, int offset, fsOrigin_t origin_mode) {
	// Uses very similar, not very efficient, method as the original filesystem
	// This function is very rarely used but needs to be supported for mod compatibility
	unsigned int origin;
	unsigned int offset_origin;

	// Get origin
	switch(origin_mode) {
		case FS_SEEK_CUR: origin = handle_entry->position; break;
		case FS_SEEK_END: origin = handle_entry->file->f.filesize; break;
		case FS_SEEK_SET: origin = 0; break;
		default: Com_Error(ERR_DROP, "fs_pk3_read_handle_seek with invalid origin mode"); }

	// Get offset_origin and correct overflow conditions
	offset_origin = origin + offset;
	if(offset < 0 && offset_origin > origin) offset_origin = 0;
	if((offset > 0 && offset_origin < origin) || offset_origin > handle_entry->file->f.filesize)
		offset_origin = handle_entry->file->f.filesize;

	// If seeking to end, just set the position
	if(offset_origin >= handle_entry->file->f.filesize) {
		handle_entry->position = handle_entry->file->f.filesize;
		return 0; }

	// If seeking backwards, reset the handle
	if(offset_origin < handle_entry->position) {
		fsc_pk3_handle_close(handle_entry->fsc_handle);
		handle_entry->fsc_handle = fsc_pk3_handle_open(handle_entry->file, 16384, &fs, 0);
		if(!handle_entry->fsc_handle) Com_Error(ERR_FATAL, "fs_pk3_read_handle_seek failed to reopen handle");
		handle_entry->position = 0; }

	// Seek forwards by reading data to a temp buffer
	while(handle_entry->position < offset_origin) {
		char buffer[65536];
		unsigned int read_amount;
		unsigned int read_target = offset_origin - handle_entry->position;
		if(read_target > sizeof(buffer)) read_target = sizeof(buffer);
		read_amount = fsc_pk3_handle_read(handle_entry->fsc_handle, buffer, read_target);
		handle_entry->position += read_amount;
		if(read_amount != read_target) return -1; }

	if(offset_origin == origin + offset) return 0;
	return -1; }

static void fs_pk3_read_handle_close(fs_pk3_read_handle_t *handle_entry) {
	fsc_pk3_handle_close(handle_entry->fsc_handle); }

/* ******************************************************************************** */
// Write handle operations
/* ******************************************************************************** */

static fileHandle_t fs_write_handle_open(const char *path, qboolean append, qboolean sync) {
	// Does not include directory creation or sanity checks
	// Returns handle on success, null on error
	fileHandle_t handle;
	fs_write_handle_t *handle_entry;
	void *os_path = fsc_string_to_os_path(path);
	void *fsc_handle;

	if(fs_debug_fileio->integer) Com_Printf("********** opening write handle **********\npath: %s\n", path);

	if(!os_path) {
		if(fs_debug_fileio->integer) Com_Printf("result: failed to convert to os path\n");
		return 0; }

	// Attempt to open the file
	if(append) fsc_handle = fsc_open_file(os_path, "ab");
	else fsc_handle = fsc_open_file(os_path, "wb");
	fsc_free(os_path);
	if(!fsc_handle) {
		if(fs_debug_fileio->integer) Com_Printf("result: failed to open file\n");
		return 0; }

	// Set up handle entry
	handle = fs_allocate_handle(FS_HANDLE_WRITE);
	handle_entry = (fs_write_handle_t *)fs_get_handle_entry(handle);
	handle_entry->fsc_handle = fsc_handle;
	handle_entry->sync = sync;
	handle_entry->h.debug_path = CopyString(path);
	if(fs_debug_fileio->integer) Com_Printf("result: success\n");
	return handle; }

static void fs_write_handle_write(fileHandle_t handle, const char *data, unsigned int length) {
	fs_write_handle_t *handle_entry = (fs_write_handle_t *)fs_get_handle_entry(handle);
	if(!handle_entry || handle_entry->h.type != FS_HANDLE_WRITE) Com_Error(ERR_DROP, "fs_write_handle_write on invalid handle");

	fsc_fwrite(data, length, handle_entry->fsc_handle);
	if(handle_entry->sync) fsc_fflush(handle_entry->fsc_handle); }

static void fs_write_handle_flush(fileHandle_t handle, qboolean enable_sync) {
	fs_write_handle_t *handle_entry = (fs_write_handle_t *)fs_get_handle_entry(handle);
	if(!handle_entry || handle_entry->h.type != FS_HANDLE_WRITE) Com_Error(ERR_DROP, "fs_write_handle_flush on invalid handle");

	fsc_fflush(handle_entry->fsc_handle);
	if(enable_sync) handle_entry->sync = qtrue; }

static int fs_write_handle_seek(fs_write_handle_t *handle_entry, int offset, fsOrigin_t origin_mode) {
	// Get type
	fsc_seek_type_t type;
	switch(origin_mode) {
		case FS_SEEK_CUR: type = FSC_SEEK_CUR; break;
		case FS_SEEK_END: type = FSC_SEEK_END; break;
		case FS_SEEK_SET: type = FSC_SEEK_SET; break;
		default: Com_Error(ERR_DROP, "fs_write_handle_seek with invalid origin mode"); }
	return fsc_fseek(handle_entry->fsc_handle, offset, type); }

static void fs_write_handle_close(fs_write_handle_t *handle_entry) {
	fsc_fclose(handle_entry->fsc_handle); }

/* ******************************************************************************** */
// Pipe Files
/* ******************************************************************************** */

fileHandle_t FS_FCreateOpenPipeFile(const char *filename) {
	char path[FS_MAX_PATH];
	FILE *fifo = 0;
	fileHandle_t handle;
	fs_pipe_handle_t *handle_entry;

	if(fs_generate_path_writedir(FS_GetCurrentGameDir(), filename,
			0, FS_ALLOW_SLASH|FS_CREATE_DIRECTORIES_FOR_FILE, path, sizeof(path))) {
		fifo = Sys_Mkfifo(path); }

	if(!fifo) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Could not create new com_pipefile at %s. "
			"com_pipefile will not be used.\n", path );
		return 0; }

	//if(fs_debug->integer) {
	//	Com_Printf( "FS_FCreateOpenPipeFile: %s\n", ospath ); }

	// Set up handle entry
	handle = fs_allocate_handle(FS_HANDLE_PIPE);
	handle_entry = (fs_pipe_handle_t *)fs_get_handle_entry(handle);
	handle_entry->handle = fifo;
	handle_entry->h.debug_path = CopyString(filename);
	return handle; }

static unsigned int fs_pipe_handle_read(void *buffer, int len, fs_pipe_handle_t *handle_entry) {
	return fread(buffer, 1, len, handle_entry->handle); }

static void fs_pipe_handle_close(fs_pipe_handle_t *handle_entry) {
	fclose(handle_entry->handle); }

/* ******************************************************************************** */
// Common handle operations
/* ******************************************************************************** */

void fs_handle_close(fileHandle_t handle) {
	// Get handle entry
	fs_handle_t *handle_entry;
	if(!handle) {
		Com_Printf("^1WARNING: fs_handle_close on null handle\n");
		return; }
	handle_entry = fs_get_handle_entry(handle);
	if(!handle_entry) {
		Com_Printf("^1WARNING: fs_handle_close on invalid handle\n");
		return; }

	switch(handle_entry->type) {
		case FS_HANDLE_CACHE_READ: fs_cache_read_handle_close((fs_cache_read_handle_t *)handle_entry); break;
		case FS_HANDLE_DIRECT_READ: fs_direct_read_handle_close((fs_direct_read_handle_t *)handle_entry); break;
		case FS_HANDLE_PK3_READ: fs_pk3_read_handle_close((fs_pk3_read_handle_t *)handle_entry); break;
		case FS_HANDLE_WRITE: fs_write_handle_close((fs_write_handle_t *)handle_entry); break;
		case FS_HANDLE_PIPE: fs_pipe_handle_close((fs_pipe_handle_t *)handle_entry); break;
		default: Com_Error(ERR_DROP, "fs_handle_close invalid handle type"); }

	Z_Free(handle_entry->debug_path);
	fs_free_handle(handle); }

void fs_close_all_handles(void) {
	// Can be used when the whole program is terminating, just to be safe
	int i;
	for(i=0; i<MAX_HANDLES; ++i) {
		if(handles[i]) fs_handle_close(i+1); } }

static unsigned int fs_handle_read(char *buffer, unsigned int length, fileHandle_t handle) {
	// Get handle entry
	fs_handle_t *handle_entry = fs_get_handle_entry(handle);
	if(!handle_entry) {
		Com_Error(ERR_DROP, "fs_handle_read on invalid handle"); }

	switch(handle_entry->type) {
		case FS_HANDLE_CACHE_READ: return fs_cache_read_handle_read(buffer, length, (fs_cache_read_handle_t *)handle_entry);
		case FS_HANDLE_DIRECT_READ: return fs_direct_read_handle_read(buffer, length, (fs_direct_read_handle_t *)handle_entry);
		case FS_HANDLE_PK3_READ: return fs_pk3_read_handle_read(buffer, length, (fs_pk3_read_handle_t *)handle_entry);
		case FS_HANDLE_PIPE: return fs_pipe_handle_read(buffer, length, (fs_pipe_handle_t *)handle_entry);
		default: Com_Error(ERR_DROP, "fs_handle_read invalid handle type"); }
	return 0; }

static int fs_handle_seek(fileHandle_t handle, int offset, fsOrigin_t origin_mode) {
	// Get handle entry
	fs_handle_t *handle_entry = fs_get_handle_entry(handle);
	if(!handle_entry) {
		Com_Error(ERR_DROP, "fs_handle_seek on invalid handle"); }

	switch(handle_entry->type) {
		case FS_HANDLE_CACHE_READ: return fs_cache_read_handle_seek((fs_cache_read_handle_t *)handle_entry, offset, origin_mode);
		case FS_HANDLE_DIRECT_READ: return fs_direct_read_handle_seek((fs_direct_read_handle_t *)handle_entry, offset, origin_mode);
		case FS_HANDLE_PK3_READ: return fs_pk3_read_handle_seek((fs_pk3_read_handle_t *)handle_entry, offset, origin_mode);
		case FS_HANDLE_WRITE: return fs_write_handle_seek((fs_write_handle_t *)handle_entry, offset, origin_mode);
		default: Com_Error(ERR_DROP, "fs_handle_seek invalid handle type"); }
	return 1; }

static unsigned int fs_handle_ftell(fileHandle_t handle) {
	// Get handle entry
	fs_handle_t *handle_entry = fs_get_handle_entry(handle);
	if(!handle_entry) Com_Error(ERR_DROP, "fs_handle_ftell on invalid handle");

	switch(handle_entry->type) {
		case FS_HANDLE_CACHE_READ: return ((fs_cache_read_handle_t *)(handle_entry))->position;
		case FS_HANDLE_WRITE: return fsc_ftell(((fs_write_handle_t *)(handle_entry))->fsc_handle);
		default: Com_Error(ERR_DROP, "fs_handle_ftell invalid handle type"); }
	return 0; }

static void fs_handle_set_owner(fileHandle_t handle, fs_handle_owner_t owner) {
	fs_handle_t *handle_entry = fs_get_handle_entry(handle);
	if(!handle_entry) Com_Error(ERR_DROP, "fs_handle_set_owner on invalid handle");
	handle_entry->owner = owner; }

fs_handle_owner_t fs_handle_get_owner(fileHandle_t handle) {
	fs_handle_t *handle_entry = fs_get_handle_entry(handle);
	if(!handle_entry) return FS_HANDLEOWNER_SYSTEM;
	return handle_entry->owner; }

static const char *identify_handle_type(fs_handle_type_t type) {
	if(type == FS_HANDLE_CACHE_READ) return "cache read";
	if(type == FS_HANDLE_DIRECT_READ) return "direct read";
	if(type == FS_HANDLE_PK3_READ) return "pk3 read";
	if(type == FS_HANDLE_WRITE) return "write";
	if(type == FS_HANDLE_PIPE) return "pipe";
	return "unknown"; }

static const char *identify_handle_owner(fs_handle_owner_t owner) {
	if(owner == FS_HANDLEOWNER_SYSTEM) return "system";
	if(owner == FS_HANDLEOWNER_CGAME) return "cgame";
	if(owner == FS_HANDLEOWNER_UI) return "ui";
	if(owner == FS_HANDLEOWNER_QAGAME) return "qagame";
	return "unknown"; }

void fs_print_handle_list(void) {
	int i;
	for(i=0; i<MAX_HANDLES; ++i) {
		if(!handles[i]) continue;
		Com_Printf("********** handle %i **********\ntype: %s\nowner: %s\npath: %s\n",
				i+1, identify_handle_type(handles[i]->type), identify_handle_owner(handles[i]->owner),
				handles[i]->debug_path); } }

void fs_close_owner_handles(fs_handle_owner_t owner) {
	// Can be called when a VM is shutting down to avoid leaked handles
	int i;
	for(i=0; i<MAX_HANDLES; ++i) {
		if(handles[i] && handles[i]->owner == owner) {
			Com_Printf("^1*****************\nWARNING: Auto-closing possible leaked handle\n"
					"type: %s\nowner: %s\npath: %s\n*****************\n", identify_handle_type(handles[i]->type),
					identify_handle_owner(handles[i]->owner), handles[i]->debug_path);
			fs_handle_close(i+1); } } }

/* ******************************************************************************** */
// Journal Data File Functions
/* ******************************************************************************** */

void fs_write_journal_data(const char *data, unsigned int length) {
	// Use length 0 to indicate file not found
	if(!com_journalDataFile || com_journal->integer != 1) return;
	FS_Write(&length, sizeof(length), com_journalDataFile);
	if(length) FS_Write(data, length, com_journalDataFile);
	FS_Flush(com_journalDataFile); }

char *fs_read_journal_data(void) {
	// Returns next piece of data from journal data file, or null if not available
	// If data was returned, result needs to be freed by fs_free_data.
	unsigned int length;
	int r;
	char *data;

	if(!com_journalDataFile || com_journal->integer != 2) return 0;

	r = FS_Read(&length, sizeof(length), com_journalDataFile);
	if(r != sizeof(length)) return 0;
	if(!length) return 0;

	// Obtain buffer from cache or malloc
	{	cache_entry_t *cache_entry = cache_allocate(0, length + 1);
		if(cache_entry) {
			++cache_entry->lock_count;
			data = CACHE_ENTRY_DATA(cache_entry); }
		else data = (char *)fsc_malloc(length + 1); }

	// Attempt to read data
	r = FS_Read(data, length, com_journalDataFile);
	if(r != length) Com_Error(ERR_FATAL, "Failed to read data from journal data file");
	data[length] = 0;
	return data; }

/* ******************************************************************************** */
// Config File Operations
/* ******************************************************************************** */

fileHandle_t fs_open_settings_file_write(const char *filename) {
	// This is used for writing the primary auto-saved settings file, e.g. q3config.cfg.
	// The save directory will be adjusted depending on the fs_mod_settings value.
	char path[FS_MAX_PATH];
	const char *mod_dir;

	if(fs_mod_settings->integer) {
		mod_dir = FS_GetCurrentGameDir(); }
	else {
		mod_dir = com_basegame->string; }

	if(!fs_generate_path_writedir(mod_dir, filename, FS_CREATE_DIRECTORIES, FS_ALLOW_SPECIAL_CFG,
			path, sizeof(path))) return 0;
	return fs_write_handle_open(path, qfalse, qfalse); }

/* ******************************************************************************** */
// Misc Handle Operations
/* ******************************************************************************** */

static long open_index_read_handle(const char *filename, fileHandle_t *handle, int lookup_flags, qboolean allow_direct_handle) {
	// Can be called with a null filehandle pointer for a size/existance check
	const fsc_file_t *fscfile;
	unsigned int size = 0;

	// Get the file
	fscfile = fs_general_lookup(filename, lookup_flags, qfalse);
	if(!fscfile || (long)(fscfile->filesize) <= 0) {
		if(handle) *handle = 0;
		return -1; }
	if(!handle) return (long)(fscfile->filesize);

	// Get the handle
	if(allow_direct_handle && fscfile->sourcetype == FSC_SOURCETYPE_PK3 && fscfile->filesize > 65536) {
		*handle = fs_pk3_read_handle_open(fscfile);
		size = fscfile->filesize; }
	else if(allow_direct_handle && fscfile->sourcetype == FSC_SOURCETYPE_DIRECT && fscfile->filesize > 65536) {
		*handle = fs_direct_read_handle_open(fscfile, 0, &size); }
	else {
		*handle = fs_cache_read_handle_open(fscfile, 0, &size); }
	if(!*handle) return -1;

	// Verify size is valid
	if((long)size <= 0) {
		fs_free_handle(*handle);
		*handle = 0;
		return -1; }

	return (long)size; }

long FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE) {
	return open_index_read_handle(filename, file, 0, qfalse); }

long FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp) {
	int i;
	char path[FS_MAX_PATH];
	unsigned int size = -1;
	*fp = 0;

	for(i=0; i<FS_SOURCEDIR_COUNT; ++i) {
		if(fs_generate_path_sourcedir(i, filename, 0, FS_ALLOW_SLASH, 0, path, sizeof(path))) {
			*fp = fs_cache_read_handle_open(0, path, &size);
			if(*fp) break; } }

	if(!*fp) return -1;
	return size; }

static fileHandle_t open_write_handle_path_gen(const char *mod_dir, const char *path, qboolean append,
		qboolean sync, int flags) {
	// Includes directory creation and sanity checks
	// Returns handle on success, null on error
	char full_path[FS_MAX_PATH];

	if(!fs_generate_path_writedir(mod_dir, path, FS_CREATE_DIRECTORIES,
			FS_ALLOW_SLASH|FS_CREATE_DIRECTORIES_FOR_FILE|flags, full_path, sizeof(full_path))) return 0;

	return fs_write_handle_open(full_path, append, sync); }

fileHandle_t FS_FOpenFileWrite(const char *filename) {
	return open_write_handle_path_gen(FS_GetCurrentGameDir(), filename, qfalse, qfalse, 0); }

fileHandle_t FS_SV_FOpenFileWrite(const char *filename) {
	return open_write_handle_path_gen(0, filename, qfalse, qfalse, 0); }

fileHandle_t FS_FOpenFileAppend(const char *filename) {
	return open_write_handle_path_gen(FS_GetCurrentGameDir(), filename, qtrue, qfalse, 0); }

int FS_FOpenFileByModeOwner(const char *qpath, fileHandle_t *f, fsMode_t mode, fs_handle_owner_t owner) {
	// Can be called with a null filehandle pointer for a size/existance check
	int size = 0;
	fileHandle_t handle = 0;

	if(!f && mode != FS_READ) {
		Com_Error(ERR_DROP, "FS_FOpenFileByMode: null handle pointer with non-read mode"); }

	if(mode == FS_READ) {
		if(owner != FS_HANDLEOWNER_SYSTEM) {
			int lookup_flags = 0;
			if(owner == FS_HANDLEOWNER_QAGAME) {
				// Ignore pure list for server VM. This prevents the server mod from being affected
				// by the pure list in a local game. Since the initial qagame init is done before the
				// pure list setup, the behavior would be inconsistent anyway.
				lookup_flags |= LOOKUPFLAG_IGNORE_PURE_LIST; }
			else {
				// For other VMs, allow some of the extensions from the list in the original FS_FOpenFileReadDir
				// to access direct sourcetype files regardless of pure mode, for compatibility purposes
				// NOTE: Consider enabling this regardless of extension?
				if(COM_CompareExtension(qpath, ".cfg") || COM_CompareExtension(qpath, ".menu") ||
						COM_CompareExtension(qpath, ".game") || COM_CompareExtension(qpath, ".dat"))
					lookup_flags |= LOOKUPFLAG_DIRECT_SOURCE_ALLOW_UNPURE; }

			if((lookup_flags & (LOOKUPFLAG_IGNORE_PURE_LIST|LOOKUPFLAG_DIRECT_SOURCE_ALLOW_UNPURE)) ||
					fs_connected_server_pure_state() != 1) {
				// Try to read directly from disk first, because some mods do weird things involving
				// files that were just written/currently open that don't work with cache handles
				char path[FS_MAX_PATH];
				if(fs_generate_path_writedir(FS_GetCurrentGameDir(), qpath, 0, FS_ALLOW_SLASH,
						path, sizeof(path))) {
					handle = fs_direct_read_handle_open(0, path, (unsigned int *)&size); } }

			// Use read with direct handle support option, to optimize for mods like UI Enhanced that do
			// bulk .bsp handle opens on startup which would be very slow using normal cache handles
			if(!handle) size = open_index_read_handle(qpath, f ? &handle : 0, lookup_flags, qtrue); }

		// Engine reads don't do anything fancy so just use the basic method
		else size = open_index_read_handle(qpath, f ? &handle : 0, 0, qfalse);

		// Verify size is valid
		if(size <= 0) {
			if(handle) fs_free_handle(handle);
			handle = 0;
			size = -1; } }
	else if(mode == FS_WRITE) {
		handle = open_write_handle_path_gen(FS_GetCurrentGameDir(), qpath, qfalse, qfalse, 0); }
	else if(mode == FS_APPEND_SYNC) {
		handle = open_write_handle_path_gen(FS_GetCurrentGameDir(), qpath, qtrue, qtrue, 0); }
	else if(mode == FS_APPEND) {
		handle = open_write_handle_path_gen(FS_GetCurrentGameDir(), qpath, qtrue, qfalse, 0); }
	else {
		Com_Error(ERR_DROP, "FS_FOpenFileByMode: bad mode"); }

	if(f) {
		// Caller wants to keep the handle
		*f = handle;
		if(handle) {
			fs_handle_set_owner(handle, owner);
			return size; }
		else {
			return -1; } }
	else {
		// Size check only
		if(handle) fs_free_handle(handle);
		return size; } }

int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode) {
	return FS_FOpenFileByModeOwner(qpath, f, mode, FS_HANDLEOWNER_SYSTEM); }

void FS_FCloseFile(fileHandle_t f) {
	if(!f) {
		Com_DPrintf("FS_FCloseFile on null handle\n");
		return; }
	fs_handle_close(f); }

int FS_Read(void *buffer, int len, fileHandle_t f) {
	return fs_handle_read((char *)buffer, len, f); }

int FS_Read2(void *buffer, int len, fileHandle_t f) {
	// This seems pretty much identical to FS_Read in the original filesystem as well
	return FS_Read(buffer, len, f); }

int FS_Write(const void *buffer, int len, fileHandle_t h) {
	fs_write_handle_write(h, (const char *)buffer, len);
	return len; }

int FS_Seek(fileHandle_t f, long offset, int origin) {
	return fs_handle_seek(f, offset, (fsOrigin_t)origin); }

int FS_FTell(fileHandle_t f) {
	return fs_handle_ftell(f); }

void FS_Flush(fileHandle_t f) {
	fs_write_handle_flush(f, qfalse); }

void FS_ForceFlush(fileHandle_t f) {
	fs_write_handle_flush(f, qtrue); }

/* ******************************************************************************** */
// Misc Data Operations
/* ******************************************************************************** */

long FS_ReadFile(const char *qpath, void **buffer) {
	// Returns -1 and nulls buffer on error. Returns size and sets buffer on success.
	// On success result buffer must be freed with FS_FreeFile.
	// Can be called with null buffer for size check.
	const fsc_file_t *file;
	unsigned int len;

	file = fs_general_lookup(qpath, 0, qfalse);

	if(!file) {
		// File not found
		if(buffer) *buffer = 0;
		return -1; }

	if(!buffer) {
		// Size check
		return (long)file->filesize; }

	*buffer = fs_read_data(file, 0, &len);
	return (long)len; }

void FS_FreeFile(void *buffer) {
	fs_free_data((char *)buffer); }

void FS_WriteFile( const char *qpath, const void *buffer, int size ) {
	// Copy from original filesystem
	fileHandle_t f;

	if ( !qpath || !buffer ) {
		Com_Error( ERR_FATAL, "FS_WriteFile: NULL parameter" );
	}

	f = FS_FOpenFileWrite( qpath );
	if ( !f ) {
		Com_Printf( "Failed to open %s\n", qpath );
		return;
	}

	FS_Write( buffer, size, f );

	FS_FCloseFile( f );
}

#endif	// NEW_FILESYSTEM
