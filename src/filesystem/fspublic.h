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

#include "../qcommon/q_shared.h"

/* ******************************************************************************** */
// Common Definitions
/* ******************************************************************************** */

#ifdef FSLOCAL
// Versioned pk3 search locations
#define BASEGAME_1_3 "trem13"
#define BASEGAME_GPP "gpp"
#define BASEGAME_1_1 "base"

// User mod directory; overrides the regular search locations
#define BASEGAME_OVERRIDE "basemod"

// Store location for saved demos, screenshots, configs, condumps, etc.
// Can be modified on command line via fs_basegame cvar
#define FS_BASEGAME_DEFAULT "trem13"

// Name of core game dll (full location is {BASEGAME_1_3}/{FS_CORE_GAME_DLL_NAME}.{DLL_EXT})
#define FS_CORE_GAME_DLL_NAME "core_game"
#endif

#define qboolean bool
typedef FS_Origin fsOrigin_t;
typedef FS_Mode fsMode_t;

#define DEF_PUBLIC(f) f;
#ifdef FSLOCAL
#define DEF_LOCAL(f) f;
#else
#define DEF_LOCAL(f)
#endif

#ifdef DEDICATED
#define Q3CONFIG_CFG "autogen_server.cfg"
#else
#define Q3CONFIG_CFG "autogen.cfg"
#endif

#ifdef WIN32
#define SYSTEM_NEWLINE "\r\n"
#else
#define SYSTEM_NEWLINE "\n"
#endif

typedef struct fsc_file_s fsc_file_t;
typedef struct fsc_shader_s fsc_shader_t;

typedef enum {
	FS_CONFIGTYPE_NONE,
	FS_CONFIGTYPE_DEFAULT,
	FS_CONFIGTYPE_SETTINGS
} fs_config_type_t;

#ifdef FSLOCAL
enum FS_Profile {
	FS_PROFILE_UNSET,
	FS_PROFILE_1_1,
	FS_PROFILE_GPP,
	FS_PROFILE_1_3
};

typedef struct {
	const char *name;
	const char *path;
	qboolean active;
	qboolean auxiliary;
} fs_source_directory_t;

typedef struct fs_hashtable_entry_s {
	struct fs_hashtable_entry_s *next;
} fs_hashtable_entry_t;

typedef struct {
	fs_hashtable_entry_t **buckets;
	int bucket_count;
	int element_count;
} fs_hashtable_t;

typedef struct {
	fs_hashtable_t *ht;
	int current_bucket;
	int bucket_limit;
	fs_hashtable_entry_t *current_entry;
} fs_hashtable_iterator_t;

typedef struct pk3_list_entry_s {
	fs_hashtable_entry_t hte;
	unsigned int hash;
	int position;
} pk3_list_entry_t;

typedef struct {
	fs_hashtable_t ht;
} pk3_list_t;

#define STACKPTR(pointer) ( FSC_STACK_RETRIEVE(&fs.general_stack, pointer, 0) )		// non-null
#define STACKPTRN(pointer) ( FSC_STACK_RETRIEVE(&fs.general_stack, pointer, 1) )	// null allowed
#endif

/* ******************************************************************************** */
// Main
/* ******************************************************************************** */

DEF_LOCAL( extern cvar_t *fs_dirs )
DEF_LOCAL( extern cvar_t *fs_mod_settings )
DEF_LOCAL( extern cvar_t *fs_index_cache )
DEF_LOCAL( extern cvar_t *fs_read_inactive_mods )
DEF_LOCAL( extern cvar_t *fs_list_inactive_mods )
DEF_LOCAL( extern cvar_t *fs_download_manifest )
DEF_LOCAL( extern cvar_t *fs_pure_manifest )
DEF_LOCAL( extern cvar_t *fs_redownload_across_mods )
DEF_LOCAL( extern cvar_t *fs_full_pure_validation )
DEF_LOCAL( extern cvar_t *fs_saveto_dlfolder )
DEF_LOCAL( extern cvar_t *fs_restrict_dlfolder )
DEF_LOCAL( extern cvar_t *fs_auto_refresh )

DEF_LOCAL( extern cvar_t *fs_debug_state )
DEF_LOCAL( extern cvar_t *fs_debug_refresh )
DEF_LOCAL( extern cvar_t *fs_debug_fileio )
DEF_LOCAL( extern cvar_t *fs_debug_lookup )
DEF_LOCAL( extern cvar_t *fs_debug_references )
DEF_LOCAL( extern cvar_t *fs_debug_filelist )

DEF_LOCAL( extern cvar_t *fs_basegame )
DEF_LOCAL( extern cvar_t *fs_force_profile )

#ifdef FSLOCAL
#define FS_MAX_SOURCEDIRS 16
#endif

DEF_LOCAL( extern fs_source_directory_t fs_sourcedirs[FS_MAX_SOURCEDIRS] )
DEF_LOCAL( extern qboolean fs_read_only )

DEF_LOCAL( extern fsc_filesystem_t fs )

DEF_LOCAL( extern cvar_t *fs_game )
DEF_LOCAL( extern char current_mod_dir[FSC_MAX_MODDIR] )
DEF_LOCAL( extern const fsc_file_direct_t *current_map_pk3 )
DEF_LOCAL( extern int checksum_feed )

DEF_LOCAL( extern int connected_server_sv_pure )
DEF_LOCAL( extern pk3_list_t connected_server_pure_list )
DEF_LOCAL( extern FS_Profile fs_connected_server_profile )

// State Accessors
DEF_PUBLIC( const char *FS_GetCurrentGameDir(void) )
DEF_PUBLIC( const char *fs_pid_file_directory(void) )
DEF_PUBLIC( qboolean FS_Initialized( void ) )
DEF_LOCAL( int fs_connected_server_pure_state(void) )
DEF_LOCAL( FS_Profile fs_current_profile(void) )

// State Modifiers
DEF_PUBLIC( void fs_register_current_map(const char *name) )
DEF_PUBLIC( void fs_set_connected_server_profile(int alternateProtocol) )
DEF_PUBLIC( void fs_set_connected_server_sv_pure_value(int sv_pure) )
DEF_PUBLIC( void FS_PureServerSetLoadedPaks(const char *hash_list, const char *name_list) )
DEF_PUBLIC( void fs_disconnect_cleanup(void) )
DEF_PUBLIC( void fs_set_mod_dir(const char *value, qboolean move_pid) )
DEF_PUBLIC( qboolean FS_ConditionalRestart(int checksumFeed, qboolean disconnect) )

// Filesystem Refresh
DEF_LOCAL( void fs_refresh(qboolean quiet) )
DEF_LOCAL( void fs_refresh_auto_ext(qboolean ignore_cvar, qboolean quiet) )
DEF_PUBLIC( void fs_refresh_auto(void) )

// Filesystem Initialization
DEF_LOCAL( void fs_indexcache_write(void) )
DEF_PUBLIC( void fs_startup(void) )

/* ******************************************************************************** */
// Lookup
/* ******************************************************************************** */

#define LOOKUPFLAG_ENABLE_DDS 1		// Enable dds format for image lookups. Must match value in tr_public.h!
#define LOOKUPFLAG_IGNORE_PURE_LIST 2	// Ignore pure list entirely (allow all files AND ignore ordering)
#define LOOKUPFLAG_PURE_ALLOW_DIRECT_SOURCE 4	// Allow files on disk when pure
#define LOOKUPFLAG_IGNORE_CURRENT_MAP 8		// Ignore current map sort criteria
#define LOOKUPFLAG_DIRECT_SOURCE_ONLY 16	// Only allow files on disk
#define LOOKUPFLAG_PK3_SOURCE_ONLY 32	// Only allow files in pk3s
#define LOOKUPFLAG_SETTINGS_FILE 64		// Apply fs_mod_settings for auto-executed settings files (e.g. autogen.cfg, autoexec.cfg)
#define LOOKUPFLAG_NO_DOWNLOAD_FOLDER 128	// Don't allow files from download folder
#define LOOKUPFLAG_PRIORITIZE_FS_BASEGAME 256	// (Tremulous) Prioritize fs_basegame over the other hardcoded base directories

DEF_LOCAL( void debug_resource_comparison(int resource1_position, int resource2_position) )
DEF_PUBLIC( const fsc_file_t *fs_general_lookup(const char *name, int lookup_flags, qboolean debug) )
DEF_PUBLIC( const fsc_shader_t *fs_shader_lookup(const char *name, int lookup_flags, qboolean debug) )
DEF_PUBLIC( const fsc_file_t *fs_image_lookup(const char *name, int lookup_flags, qboolean debug) )
DEF_PUBLIC( const fsc_file_t *fs_sound_lookup(const char *name, int lookup_flags, qboolean debug) )
DEF_PUBLIC( const fsc_file_t *fs_vm_lookup(const char *name, qboolean qvm_only, qboolean debug, qboolean *is_dll_out) )

/* ******************************************************************************** */
// File Listing
/* ******************************************************************************** */

#define FLISTFLAG_IGNORE_TAPAK0 1	// Ignore missionpak pak0.pk3 (to keep incompatible models out of model list)
#define FLISTFLAG_IGNORE_PURE_LIST 2	// Ignore pure list entirely (allow all files AND ignore ordering)
#define FLISTFLAG_PURE_ALLOW_DIRECT_SOURCE 4	// Allow files on disk when pure

DEF_PUBLIC( void FS_FreeFileList( char **list ) )
DEF_LOCAL( char **FS_FlagListFilteredFiles(const char *path, const char *extension, const char *filter,
		int *numfiles_out, int flags) )
DEF_PUBLIC( char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) )
DEF_PUBLIC( int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) )
DEF_PUBLIC( int	FS_GetFilteredFiles( const char *path, const char *extension, const char *filter, char *listbuf, int bufsize ) )

/* ******************************************************************************** */
// File IO
/* ******************************************************************************** */

// Path Generation Flags
#define FS_MAX_PATH 512
#define FS_NO_SANITIZE 1
#define FS_CREATE_DIRECTORIES 2
#define FS_CREATE_DIRECTORIES_FOR_FILE 4
#define FS_ALLOW_DIRECTORIES 8
#define FS_ALLOW_PK3 16
#define FS_ALLOW_DLL 32
#define FS_ALLOW_SPECIAL_CFG 64

// Path Generation Functions
DEF_LOCAL( unsigned int fs_generate_path_sourcedir(int source_dir_id, const char *path1, const char *path2,
		int path1_flags, int path2_flags, char *target, unsigned int target_size) )
DEF_PUBLIC( unsigned int fs_generate_path(const char *path1, const char *path2, const char *path3,
		int path1_flags, int path2_flags, int path3_flags,
		char *target, unsigned int target_size) )
DEF_PUBLIC( unsigned int fs_generate_path_writedir(const char *path1, const char *path2,
		int path1_flags, int path2_flags, char *target, unsigned int target_size) )

// Direct file access
DEF_LOCAL( void *fs_open_file(const char *path, const char *mode) )
DEF_LOCAL( void fs_rename_file(const char *source, const char *target) )
DEF_LOCAL( void fs_delete_file(const char *path) )
DEF_PUBLIC( void FS_HomeRemove( const char *homePath ) )
DEF_LOCAL( qboolean FS_FileInPathExists(const char *testpath) )
DEF_PUBLIC( qboolean FS_FileExists(const char *file) )
DEF_PUBLIC( qboolean FS_FileExistsInBaseGame(const char *file) )

// File read cache
DEF_PUBLIC( void fs_cache_initialize(void) )
DEF_PUBLIC( void fs_advance_cache_stage(void) )
DEF_LOCAL( void fs_readcache_debug(void) )

// Data reading
DEF_PUBLIC( char *fs_read_data(const fsc_file_t *file, const char *path, unsigned int *size_out, const char *calling_function) )
DEF_PUBLIC( void fs_free_data(char *data) )
DEF_PUBLIC( char *fs_read_shader(const fsc_shader_t *shader) )

// Direct read handle operations
DEF_PUBLIC( fileHandle_t fs_direct_read_handle_open(const fsc_file_t *file, const char *path, unsigned int *size_out) )

// Pipe files
DEF_PUBLIC( fileHandle_t FS_FCreateOpenPipeFile( const char *filename ) )

// Handle owners
typedef enum {
	FS_HANDLEOWNER_SYSTEM,
	FS_HANDLEOWNER_CGAME,
	FS_HANDLEOWNER_UI,
	FS_HANDLEOWNER_QAGAME
} fs_handle_owner_t;

// Common handle operations
DEF_PUBLIC( void fs_handle_close(fileHandle_t handle) )
DEF_PUBLIC( void fs_close_all_handles(void) )
DEF_PUBLIC( fs_handle_owner_t fs_handle_get_owner(fileHandle_t handle) )
DEF_LOCAL( void fs_print_handle_list(void) )
DEF_PUBLIC( void fs_close_owner_handles(fs_handle_owner_t owner) )

// Journal Data File Functions
DEF_LOCAL( void fs_write_journal_data(const char *data, unsigned int length) )
DEF_LOCAL( char *fs_read_journal_data(void) )

// Config file functions
DEF_PUBLIC( fileHandle_t fs_open_settings_file_write(const char *filename) )

// Data reading operations
DEF_PUBLIC( long FS_ReadFile(const char *qpath, void **buffer) )
DEF_PUBLIC( void FS_FreeFile(void *buffer) )

// "Read-back" tracking
DEF_LOCAL( void fs_readback_tracker_reset(void) )

// FS_FOpenFile functions
DEF_PUBLIC( long FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE) )
DEF_PUBLIC( long FS_FOpenFileReadParseScript(const char *filename, fileHandle_t *file, qboolean uniqueFILE) )
DEF_PUBLIC( fileHandle_t FS_FOpenFileWrite(const char *filename) )
DEF_PUBLIC( fileHandle_t FS_FOpenFileAppend(const char *filename) )
DEF_PUBLIC( int FS_FOpenFileByModeOwner(const char *qpath, fileHandle_t *f, fsMode_t mode, fs_handle_owner_t owner) )
DEF_PUBLIC( int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode) )

// Misc handle operations
DEF_PUBLIC( long FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp) )
DEF_PUBLIC( fileHandle_t FS_SV_FOpenFileWrite(const char *filename) )
DEF_PUBLIC( void FS_FCloseFile(fileHandle_t f) )
DEF_PUBLIC( int FS_Read(void *buffer, int len, fileHandle_t f) )
DEF_PUBLIC( int FS_Read2(void *buffer, int len, fileHandle_t f) )
DEF_PUBLIC( int FS_Write(const void *buffer, int len, fileHandle_t h) )
DEF_PUBLIC( int FS_Seek(fileHandle_t f, long offset, int origin) )
DEF_PUBLIC( int FS_FTell(fileHandle_t f) )
DEF_PUBLIC( void FS_Flush(fileHandle_t f) )
DEF_PUBLIC( void FS_ForceFlush(fileHandle_t f) )
DEF_PUBLIC( void FS_SV_Rename(const char *from, const char *to, qboolean safe) )
DEF_PUBLIC( void FS_WriteFile( const char *qpath, const void *buffer, int size ) )

/* ******************************************************************************** */
// Commands
/* ******************************************************************************** */

DEF_LOCAL( void fs_register_commands(void) )

/* ******************************************************************************** */
// Download
/* ******************************************************************************** */

// Download List Handling
DEF_PUBLIC( void fs_advance_download(void) )
DEF_PUBLIC( void fs_print_download_list(void) )
DEF_PUBLIC( void fs_register_download_list(const char *hash_list, const char *name_list) )

// Attempted Download Tracking
DEF_PUBLIC( void fs_register_current_download_attempt(qboolean http) )
DEF_PUBLIC( void fs_clear_attempted_downloads(void) )

// Download List Advancement
DEF_PUBLIC( void fs_advance_next_needed_download(qboolean curl_disconnected) )
DEF_PUBLIC( qboolean fs_get_current_download_info(char **local_name_out, char **remote_name_out,
		qboolean *curl_already_attempted_out) )

// Download Completion
DEF_PUBLIC( void fs_finalize_download(void) )

/* ******************************************************************************** */
// Referenced Pak Tracking
/* ******************************************************************************** */

DEF_LOCAL( void fs_register_reference(const fsc_file_t *file) )
DEF_PUBLIC( void FS_ClearPakReferences( int flags ) )
DEF_PUBLIC( const char *FS_ReferencedPakNames( void ) )
DEF_PUBLIC( const char *FS_ReferencedPakPureChecksums( void ) )
DEF_PUBLIC( void fs_set_download_list(void) )
DEF_PUBLIC( fileHandle_t fs_open_download_pak(const char *path, unsigned int *size_out) )
DEF_PUBLIC( void fs_set_pure_list(void) )

/* ******************************************************************************** */
// Misc
/* ******************************************************************************** */

// Indented debug prints
DEF_LOCAL( void fs_debug_indent_start(void) )
DEF_LOCAL( void fs_debug_indent_stop(void) )
DEF_LOCAL( void QDECL FS_DPrintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2))) )

// Hash Table
DEF_LOCAL( void fs_hashtable_initialize(fs_hashtable_t *hashtable, int bucket_count) )
DEF_LOCAL( void fs_hashtable_insert(fs_hashtable_t *hashtable, fs_hashtable_entry_t *entry, unsigned int hash) )
DEF_LOCAL( fs_hashtable_iterator_t fs_hashtable_iterate(fs_hashtable_t *hashtable, unsigned int hash, qboolean iterate_all) )
DEF_LOCAL( void *fs_hashtable_next(fs_hashtable_iterator_t *iterator) )
DEF_LOCAL( void fs_hashtable_free(fs_hashtable_t *hashtable, void (*free_entry)(fs_hashtable_entry_t *entry)) )
DEF_LOCAL( void fs_hashtable_reset(fs_hashtable_t *hashtable, void (*free_entry)(fs_hashtable_entry_t *entry)) )

// Pk3 List
DEF_LOCAL( void pk3_list_initialize(pk3_list_t *pk3_list, unsigned int bucket_count) )
DEF_LOCAL( int pk3_list_lookup(const pk3_list_t *pk3_list, unsigned int hash) )
DEF_LOCAL( void pk3_list_insert(pk3_list_t *pk3_list, unsigned int hash) )
DEF_LOCAL( void pk3_list_free(pk3_list_t *pk3_list) )

// File helper functions
#ifdef FSLOCAL
#define FS_FILE_BUFFER_SIZE 512
#endif

DEF_PUBLIC( const char *fs_file_extension(const fsc_file_t *file) )
DEF_PUBLIC( qboolean fs_files_from_same_pk3(const fsc_file_t *file1, const fsc_file_t *file2) )
DEF_LOCAL( int fs_get_source_dir_id(const fsc_file_t *file) )
DEF_LOCAL( const char *fs_get_source_dir_string(const fsc_file_t *file) )
DEF_LOCAL( void fs_file_to_stream(const fsc_file_t *file, fsc_stream_t *stream, qboolean include_source_dir,
			qboolean include_mod, qboolean include_pk3_origin, qboolean include_size) )
DEF_LOCAL( void fs_file_to_buffer(const fsc_file_t *file, char *buffer, unsigned int buffer_size, qboolean include_source_dir,
			qboolean include_mod, qboolean include_pk3_origin, qboolean include_size) )
DEF_PUBLIC( void fs_print_file_location(const fsc_file_t *file) )

// File disabled check function
#ifdef FSLOCAL
#define FD_CHECK_FILE_ENABLED 1		// Check if file is disabled in index
#define FD_CHECK_PURE_LIST 2		// Check if file is blocked by connected server pure configuration
#define FD_CHECK_READ_INACTIVE_MODS 4		// Check if file is blocked for file lookup by fs_read_inactive_mods setting
#define FD_CHECK_LIST_INACTIVE_MODS 8		// Check if file is blocked for file listing by fs_list_inactive_mods setting
#define FD_CHECK_LIST_AUXILIARY_SOURCEDIR 16	// Check if file is blocked for file listing due to auxiliary sourcedir
DEF_LOCAL( int fs_file_disabled(const fsc_file_t *file, int checks) )
#endif

// File Sorting Functions
DEF_LOCAL( void fs_write_sort_string(const char *string, fsc_stream_t *output, qboolean prioritize_shorter) )
DEF_LOCAL( void fs_write_sort_filename(const fsc_file_t *file, fsc_stream_t *output) )
DEF_LOCAL( void fs_write_sort_value(unsigned int value, fsc_stream_t *output) )
DEF_LOCAL( void fs_generate_core_sort_key(const fsc_file_t *file, fsc_stream_t *output, qboolean use_server_pure_list) )
DEF_LOCAL( int fs_compare_file(const fsc_file_t *file1, const fsc_file_t *file2, qboolean use_server_pure_list) )
DEF_LOCAL( int fs_compare_pk3_source(const fsc_file_t *file1, const fsc_file_t *file2) )

// Misc Functions
DEF_PUBLIC( void fs_execute_config_file(const char *name, fs_config_type_t config_type, cbufExec_t exec_type, qboolean quiet) )
DEF_PUBLIC( void *fs_load_game_dll(const fsc_file_t *dll_file, intptr_t (QDECL **entryPoint)(int, ...),
			intptr_t (QDECL *systemcalls)(intptr_t, ...)) )
DEF_PUBLIC( void FS_GetModDescription(const char *modDir, char *description, int descriptionLen) )
DEF_PUBLIC( void FS_FilenameCompletion( const char *dir, const char *ext,
		qboolean stripExt, void(*callback)(const char *s), qboolean allowNonPureFilesOnDisk ) )
DEF_PUBLIC( qboolean FS_FilenameCompare( const char *s1, const char *s2 ) )
DEF_PUBLIC( void QDECL FS_Printf( fileHandle_t f, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3))) )
DEF_LOCAL( void fs_comma_separated_list(const char **strings, int count, fsc_stream_t *output) )
DEF_LOCAL( qboolean FS_idPak(const char *pak, const char *base, int numPaks) )
DEF_LOCAL( void fs_sanitize_mod_dir(const char *source, char *target) )
DEF_LOCAL( const char *fs_profile_to_string(FS_Profile profile) )

// QVM Hash Verification
DEF_LOCAL( qboolean calculate_file_sha256(const fsc_file_t *file, unsigned char *output) )
DEF_LOCAL( qboolean fs_check_trusted_vm_file(const fsc_file_t *file) )
DEF_LOCAL( void sha256_to_stream(unsigned char *sha, fsc_stream_t *output) )

// Core Pak Verification
DEF_LOCAL( void fs_check_core_paks(void) )

/* ******************************************************************************** */
// Trusted VMs
/* ******************************************************************************** */

DEF_LOCAL( qboolean fs_check_trusted_vm_hash(unsigned char *hash) )

/* ******************************************************************************** */
// Tremulous
/* ******************************************************************************** */

// Core Resource Precedence
DEF_LOCAL( int core_pk3_position(unsigned int hash) )
DEF_LOCAL( int core_special_position(const char *special_id) )

// Base Directory & Mod Precedence
#ifdef FSLOCAL
enum FS_ModType {
	MODTYPE_INACTIVE,
	MODTYPE_BASE1,
	MODTYPE_BASE2,
	MODTYPE_BASE3,
	MODTYPE_FS_BASEGAME,
	MODTYPE_OVERRIDE_DIRECTORY,
	MODTYPE_CURRENT_MOD
};
#endif
DEF_LOCAL( FS_ModType fs_get_mod_type(const char *mod_dir, bool prioritize_fs_basegame=false) )

// System file manager functions
DEF_PUBLIC( bool FS_BrowseHomepath( void ) )
DEF_PUBLIC( bool FS_OpenBaseGamePath( const char *baseGamePath ) )
DEF_PUBLIC( bool FS_OpenModPath( const char *modPath ) )

#undef qboolean
