/*
===========================================================================
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

#ifdef _MSC_VER
#pragma warning(disable:4477)	// warning about void ptrs from STACKPTR macros
#endif

/* ******************************************************************************** */
// Definitions
/* ******************************************************************************** */

#define FSC_CACHE_VERSION 6

#define FSC_MAX_QPATH 256	// Buffer size including null terminator
#define FSC_MAX_MODDIR 32	// Buffer size including null terminator

#define	FSC_MAX_TOKEN_CHARS 1024	// based on q_shared.h
#define FSC_MAX_SHADER_NAME FSC_MAX_TOKEN_CHARS

#define FSC_CONTENTS_PK3 1
#define FSC_CONTENTS_DLPK3 2
#define FSC_CONTENTS_SHADER 4
#define FSC_CONTENTS_CROSSHAIR 8

/* ******************************************************************************** */
// Misc (fsc_misc.c)
/* ******************************************************************************** */

typedef struct fsc_filesystem_s fsc_filesystem_t;

// ***** Misc *****

unsigned int fsc_string_hash(const char *input1, const char *input2);
unsigned int fsc_fs_size_estimate(fsc_filesystem_t *fs);

// ***** Standard Data Stream *****

typedef struct {
	char *data;
	unsigned int position;
	unsigned int size;
	int overflowed;
} fsc_stream_t;

int fsc_write_stream_data(fsc_stream_t *stream, void *data, unsigned int length);
void fsc_stream_append_string(fsc_stream_t *stream, const char *string);
int fsc_read_stream_data(fsc_stream_t *stream, void *output, unsigned int length);

// ***** Standard Stack *****

typedef unsigned int fsc_stackptr_t;

typedef struct {
	unsigned int position;
	// data added after structure
} fsc_stack_bucket_t;

typedef struct {
	fsc_stack_bucket_t **buckets;
	int buckets_position;
	int buckets_size;
} fsc_stack_t;

void fsc_stack_initialize(fsc_stack_t *stack);
fsc_stackptr_t fsc_stack_allocate(fsc_stack_t *stack, unsigned int size);
void *fsc_stack_retrieve(const fsc_stack_t *stack, const fsc_stackptr_t pointer);
void fsc_stack_free(fsc_stack_t *stack);
unsigned int fsc_stack_get_export_size(fsc_stack_t *stack);
int fsc_stack_export(fsc_stack_t *stack, fsc_stream_t *stream);
int fsc_stack_import(fsc_stack_t *stack, fsc_stream_t *stream);

// ***** Standard Hash Table *****

#define FSC_HASHTABLE_MAX_BUCKETS (10 << 20)

// This needs to be the first field of each entry of the hash table, so each entry can be casted to this type
typedef struct {
	fsc_stackptr_t next;
} fsc_hashtable_entry_t;

typedef struct {
	fsc_stackptr_t *buckets;
	fsc_stack_t *stack;
	int bucket_count;
	int utilization;
} fsc_hashtable_t;

typedef struct {
	fsc_stack_t *stack;
	fsc_stackptr_t *next_ptr;
} fsc_hashtable_iterator_t;

void fsc_hashtable_initialize(fsc_hashtable_t *ht, fsc_stack_t *stack, int bucket_count);
void fsc_hashtable_open(fsc_hashtable_t *ht, unsigned int hash, fsc_hashtable_iterator_t *iterator);
fsc_stackptr_t fsc_hashtable_next(fsc_hashtable_iterator_t *iterator);
void fsc_hashtable_insert(fsc_stackptr_t entry_ptr, unsigned int hash, fsc_hashtable_t *ht);
void fsc_hashtable_free(fsc_hashtable_t *ht);
unsigned int fsc_hashtable_get_export_size(fsc_hashtable_t *ht);
int fsc_hashtable_export(fsc_hashtable_t *ht, fsc_stream_t *stream);
int fsc_hashtable_import(fsc_hashtable_t *ht, fsc_stack_t *stack, fsc_stream_t *stream);

// ***** Standard String Repository *****

typedef struct stringrepository_entry_s {
	// Hash table compliance
	fsc_hashtable_entry_t hte;
	// data added after structure
} stringrepository_entry_t;

fsc_stackptr_t fsc_string_repository_getentry(const char *input, int allocate, fsc_hashtable_t *string_repository, fsc_stack_t *stack);	// stringrepository_entry_t
fsc_stackptr_t fsc_string_repository_getstring(const char *input, int allocate, fsc_hashtable_t *string_repository, fsc_stack_t *stack);	// char *

// ***** Qpath Handling *****

const char *fsc_get_qpath_conversion_table(void);
int fsc_process_qpath(const char *input, char *buffer, const char **qp_dir, const char **qp_name, const char **qp_ext);
int fsc_get_leading_directory(const char *input, char *output, int buffer_length, const char **remainder);

// ***** Error Handling *****

#define FSC_ERROR_GENERAL 0
#define FSC_ERROR_EXTRACT 1
#define FSC_ERROR_PK3FILE 2		// current_element: fsc_file_t (fsc_file_direct_t)
#define FSC_ERROR_SHADERFILE 3	// current_element: fsc_file_t
#define FSC_ERROR_CROSSHAIRFILE 4	// current_element: fsc_file_t

typedef struct {
	void (*handler)(int id, const char *msg, void *current_element, void *context);
	void *context;
} fsc_errorhandler_t;

void fsc_initialize_errorhandler(fsc_errorhandler_t *errorhandler,
		void (*handler)(int id, const char *msg, void *current_element, void *context), void *context);
void fsc_report_error(fsc_errorhandler_t *errorhandler, int id, const char *msg, void *current_element);

/* ******************************************************************************** */
// Game Parsing Support (fsc_gameparse.c)
/* ******************************************************************************** */

void fsc_SkipRestOfLine ( char **data );
char *fsc_COM_ParseExt( char *com_token, char **data_p, int allowLineBreaks );
int fsc_SkipBracedSection(char **program, int depth);

/* ******************************************************************************** */
// Hash Calculation (fsc_md4.c / fsc_sha256.c)
/* ******************************************************************************** */

unsigned int fsc_block_checksum(const void *buffer, int length);
void fsc_calculate_sha256(const char *data, unsigned int size, unsigned char *output);

/* ******************************************************************************** */
// OS Library Interface (fsc_os.c)
/* ******************************************************************************** */

typedef struct {
	void *os_path;
	char *qpath_with_mod_dir;
	unsigned int os_timestamp;
	unsigned int filesize;
} iterate_data_t;

typedef enum {
	FSC_SEEK_SET,
	FSC_SEEK_CUR,
	FSC_SEEK_END
} fsc_seek_type_t;

void *fsc_string_to_os_path(const char *path);		// WARNING: Result must be freed by caller using fsc_free!!!
char *fsc_os_path_to_string(const void *os_path);		// WARNING: Result must be freed by caller using fsc_free!!!
int fsc_os_path_size(const void *os_path);
int fsc_compare_os_path(const void *path1, const void *path2);

void iterate_directory(void *search_os_path, void (operation)(iterate_data_t *file_data,
			void *iterate_context), void *iterate_context);

int fsc_rename_file(void *source_os_path, void *target_os_path);
int fsc_delete_file(void *os_path);
int fsc_mkdir(void *os_path);
void *fsc_open_file(const void *os_path, const char *mode);
void fsc_fclose(void *fp);
unsigned int fsc_fread(void *dest, int size, void *fp);
unsigned int fsc_fwrite(const void *src, int size, void *fp);
void fsc_fflush(void *fp);
int fsc_fseek(void *fp, int offset, fsc_seek_type_t type);
int fsc_fseek_set(void *fp, unsigned int offset);
unsigned int fsc_ftell(void *fp);
void fsc_memcpy(void *dst, const void *src, unsigned int size);
int fsc_memcmp(const void *str1, const void *str2, unsigned int size);
void fsc_memset(void *dst, int value, unsigned int size);
void fsc_strncpy(char *dst, const char *src, unsigned int size);
void fsc_strncpy_lower(char *dst, const char *src, unsigned int size);
int fsc_strcmp(const char *str1, const char *str2);
int fsc_stricmp(const char *str1, const char *str2);
int fsc_strlen(const char *str);
void *fsc_malloc(unsigned int size);
void *fsc_calloc(unsigned int size);
void fsc_free(void *allocation);

/* ******************************************************************************** */
// Main Filesystem (fsc_main.c)
/* ******************************************************************************** */

// ***** Core Filesystem Structures *****

#define FSC_SOURCETYPE_DIRECT 1
#define FSC_SOURCETYPE_PK3 2

#define FSC_FILEFLAG_DLPK3 1

typedef struct fsc_file_s {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	// Identification
	fsc_stackptr_t qp_dir_ptr;
	fsc_stackptr_t qp_name_ptr;
	fsc_stackptr_t qp_ext_ptr;		// null for no extension

	unsigned int filesize;
	fsc_stackptr_t next_in_directory;	// Iteration
	unsigned short flags;
	unsigned short sourcetype;
} fsc_file_t;

typedef struct {
	fsc_file_t f;

	// Enable / Disable
	int refresh_count;

	int source_dir_id;
	fsc_stackptr_t os_path_ptr;
	unsigned int os_timestamp;
	fsc_stackptr_t qp_mod_ptr;
	fsc_stackptr_t pk3dir_ptr;

	// Only relevant if this file is a pk3
	unsigned int pk3_hash;

	// For resource tallies
	unsigned int pk3_subfile_count;
	unsigned int shader_file_count;
	unsigned int shader_count;
} fsc_file_direct_t;

typedef struct {
	fsc_file_t f;
	fsc_stackptr_t source_pk3;	// fsc_file_direct_t
	unsigned int header_position;
	unsigned int compressed_size;
	short compression_method;
} fsc_file_frompk3_t;

typedef struct {
	fsc_hashtable_entry_t hte;
	fsc_stackptr_t pk3;
} fsc_pk3_hash_map_entry_t;

typedef struct {
	// Identifies the sourcetype - 1 and 2 are reserved for FSC_SOURCETYPE_DIRECT and FSC_SOURCETYPE_PK3
	int sourcetype_id;

	// Returns 1 if active, 0 otherwise.
	int (*is_file_active)(const fsc_file_t *file, const fsc_filesystem_t *fs);

	// Should always return a valid static string.
	const char *(*get_mod_dir)(const fsc_file_t *file, const fsc_filesystem_t *fs);

	// Buffer should be length file->filesize
	// Returns 0 on success, 1 on failure.
	int (*extract_data)(const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh);
} fsc_sourcetype_t;

typedef struct {
	// Useful for info print messages and to determine
	// a reasonable size for building exported hashtables
	int valid_pk3_count;
	int pk3_subfile_count;
	int shader_file_count;
	int shader_count;
	int total_file_count;
	int cacheable_file_count;
} fsc_stats_t;

#define FSC_CUSTOM_SOURCETYPE_COUNT 2
typedef struct fsc_filesystem_s {
	// Support
	fsc_stack_t general_stack;
	fsc_hashtable_t string_repository;

	// Main Filesystem
	fsc_hashtable_t files;
	int refresh_count;

	// Iteration
	fsc_hashtable_t directories;

	// Shaders
	fsc_hashtable_t shaders;

	// Crosshairs
	fsc_hashtable_t crosshairs;

	// PK3 Hash Lookup - Useful to determine files needed to download
	fsc_hashtable_t pk3_hash_lookup;

	// Custom Sourcetypes - Can be used for special applications
	fsc_sourcetype_t custom_sourcetypes[FSC_CUSTOM_SOURCETYPE_COUNT];

	// Stats
	fsc_stats_t total_stats;
	fsc_stats_t active_stats;
	fsc_stats_t new_stats;
} fsc_filesystem_t;

// ***** Functions *****

int fsc_extract_file(const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh);
char *fsc_extract_file_allocated(fsc_filesystem_t *index, fsc_file_t *file, fsc_errorhandler_t *eh);

int fsc_is_file_enabled(const fsc_file_t *file, const fsc_filesystem_t *fs);
const char *fsc_get_mod_dir(const fsc_file_t *file, const fsc_filesystem_t *fs);
int fsc_file_hash(const fsc_file_t *file, const fsc_filesystem_t *fs);
void fsc_file_to_stream(const fsc_file_t *file, fsc_stream_t *stream, const fsc_filesystem_t *fs,
			int include_mod, int include_pk3_origin);

int fsc_filename_contents(const char *qp_dir, const char *qp_name, const char *qp_ext);

void fsc_filesystem_initialize(fsc_filesystem_t *fs);
void fsc_filesystem_free(fsc_filesystem_t *fs);
void fsc_filesystem_reset(fsc_filesystem_t *fs);
void fsc_load_directory(fsc_filesystem_t *fs, void *os_path, int source_dir_id, fsc_errorhandler_t *eh);

/* ******************************************************************************** */
// PK3 Handling (fsc_pk3.c)
/* ******************************************************************************** */

// receive_hash_data is used for standalone hash calculation operations,
// and should be nulled during normal filesystem loading
void fsc_load_pk3(void *os_path, fsc_filesystem_t *fs, fsc_stackptr_t sourcefile_ptr, fsc_errorhandler_t *eh,
				void (receive_hash_data)(void *context, char *data, int size), void *receive_hash_data_context );
void register_pk3_hash_lookup_entry(fsc_stackptr_t pk3_file_ptr, fsc_hashtable_t *pk3_hash_lookup, fsc_stack_t *stack);
void *fsc_pk3_handle_open(const fsc_file_frompk3_t *file, int input_buffer_size, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh);
void fsc_pk3_handle_close(void *handle);
unsigned int fsc_pk3_handle_read(void *handle, char *buffer, unsigned int length);
extern fsc_sourcetype_t pk3_sourcetype;

/* ******************************************************************************** */
// Shader Lookup (fsc_shader.c)
/* ******************************************************************************** */

typedef struct fsc_shader_s {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	fsc_stackptr_t shader_name_ptr;

	fsc_stackptr_t source_file_ptr;
	unsigned int start_position;
	unsigned int end_position;
} fsc_shader_t;

int index_shader_file(fsc_filesystem_t *fs, fsc_stackptr_t source_file_ptr, fsc_errorhandler_t *eh);
int is_shader_enabled(fsc_filesystem_t *fs, fsc_shader_t *shader);

/* ******************************************************************************** */
// Crosshair Lookup (fsc_crosshair.c)
/* ******************************************************************************** */

typedef struct {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	unsigned int hash;
	fsc_stackptr_t source_file_ptr;
} fsc_crosshair_t;

int index_crosshair(fsc_filesystem_t *fs, fsc_stackptr_t source_file_ptr, fsc_errorhandler_t *eh);
int is_crosshair_enabled(fsc_filesystem_t *fs, fsc_crosshair_t *crosshair);

/* ******************************************************************************** */
// Iteration (fsc_iteration.c)
/* ******************************************************************************** */

typedef struct {
	// Hash table compliance
	fsc_hashtable_entry_t hte;

	fsc_stackptr_t qp_dir_ptr;	// string repository
	fsc_stackptr_t peer_directory;
	fsc_stackptr_t sub_file;
	fsc_stackptr_t sub_directory;
} fsc_directory_t;

void fsc_iteration_register_file(fsc_stackptr_t file_ptr, fsc_hashtable_t *directories, fsc_hashtable_t *string_repository, fsc_stack_t *stack);

/* ******************************************************************************** */
// Index Cache (fsc_index.c)
/* ******************************************************************************** */

int fsc_cache_export_file(fsc_filesystem_t *source_fs, void *os_path, fsc_errorhandler_t *eh);
int fsc_cache_import_file(void *os_path, fsc_filesystem_t *target_fs, fsc_errorhandler_t *eh);
