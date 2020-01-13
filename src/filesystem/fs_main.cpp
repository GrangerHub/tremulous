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
#include <initializer_list>
#include "fslocal.h"

/* ******************************************************************************** */
// Filesystem State
/* ******************************************************************************** */

// The convention is that everything here can be accessed from anywhere in the
// filesystem code, but should only be modified through this file

cvar_t *fs_dirs;
cvar_t *fs_mod_settings;
cvar_t *fs_index_cache;
cvar_t *fs_read_inactive_mods;
cvar_t *fs_list_inactive_mods;
cvar_t *fs_download_manifest;
cvar_t *fs_pure_manifest;
cvar_t *fs_redownload_across_mods;
cvar_t *fs_full_pure_validation;
cvar_t *fs_saveto_dlfolder;
cvar_t *fs_restrict_dlfolder;
cvar_t *fs_auto_refresh;

cvar_t *fs_debug_state;
cvar_t *fs_debug_refresh;
cvar_t *fs_debug_fileio;
cvar_t *fs_debug_lookup;
cvar_t *fs_debug_references;
cvar_t *fs_debug_filelist;

cvar_t *fs_basegame;
cvar_t *fs_force_profile;

fs_source_directory_t fs_sourcedirs[FS_MAX_SOURCEDIRS];
qboolean fs_read_only;  // If false, fs_sourcedirs[0] is write directory

fsc_filesystem_t fs;
static qboolean fs_initialized = qfalse;

cvar_t *fs_game;
char current_mod_dir[FSC_MAX_MODDIR];  // Matched to fs_game when fs_set_mod_dir is called
const fsc_file_direct_t *current_map_pk3;
int checksum_feed;

// Store connected server's sv_pure value here instead of relying on the cvar,
// because the cvar can be changed in console after connecting
int connected_server_sv_pure;
pk3_list_t connected_server_pure_list;

// Current version profile used to prioritize file lookups
FS_Profile fs_connected_server_profile;

/* ******************************************************************************** */
// Filesystem State Accessors
/* ******************************************************************************** */

const char *FS_GetCurrentGameDir(void)
{
    // Returns mod dir, but with empty mod dir replaced by basegame
    // Note: Potentially the usage in SV_RehashBans_f and SV_WriteBans could be changed
    //    to just use the non-SV file opening functions instead
    if (*current_mod_dir)
        return current_mod_dir;
    return fs_basegame->string;
}

const char *fs_pid_file_directory(void)
{
    if (fs_mod_settings->integer)
        return FS_GetCurrentGameDir();
    return fs_basegame->string;
}

qboolean FS_Initialized(void)
{
    // This might not be being used anywhere it could actually be false. Consider removing this
    //    function, or at least most of the calls to it, to reduce complexity.
    return fs_initialized;
}

int fs_connected_server_pure_state(void)
{
    // Returns 2 if semi-pure, 1 if pure, 0 if non-pure
    if (!connected_server_pure_list.ht.element_count)
        return 0;
    if (connected_server_sv_pure == 2)
        return 2;
    return 1;
}

FS_Profile fs_current_profile(void)
{
    if (*fs_force_profile->string)
    {
        if (!Q_stricmp(fs_force_profile->string, "13"))
            return FS_PROFILE_1_3;
        if (!Q_stricmp(fs_force_profile->string, "gpp"))
            return FS_PROFILE_GPP;
        if (!Q_stricmp(fs_force_profile->string, "11"))
            return FS_PROFILE_1_1;
    }
    if (fs_connected_server_profile != FS_PROFILE_UNSET)
        return fs_connected_server_profile;
    return FS_PROFILE_1_3;
}

/* ******************************************************************************** */
// Filesystem State Modifiers
/* ******************************************************************************** */

void fs_register_current_map(const char *name)
{
    const fsc_file_t *bsp_file = fs_general_lookup(name, LOOKUPFLAG_IGNORE_CURRENT_MAP, qfalse);
    if (!bsp_file || bsp_file->sourcetype != FSC_SOURCETYPE_PK3)
        current_map_pk3 = 0;
    else
        current_map_pk3 = fsc_get_base_file(bsp_file, &fs);

    if (fs_debug_state->integer)
    {
        char buffer[FS_FILE_BUFFER_SIZE];
        if (current_map_pk3)
            fs_file_to_buffer((fsc_file_t *)current_map_pk3, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
        else
            Q_strncpyz(buffer, "<none>", sizeof(buffer));
        Com_Printf("fs_state: current_map_pk3 set to '%s'\n", buffer);
    }
}

void fs_set_connected_server_sv_pure_value(int sv_pure)
{
    connected_server_sv_pure = sv_pure;
    if (fs_debug_state->integer)
    {
        Com_Printf("fs_state: connected_server_sv_pure set to %i\n", sv_pure);
    }
}

void fs_set_connected_server_profile(int alternateProtocol)
{
    if (alternateProtocol == 0)
        fs_connected_server_profile = FS_PROFILE_1_3;
    else if (alternateProtocol == 1)
        fs_connected_server_profile = FS_PROFILE_GPP;
    else
        fs_connected_server_profile = FS_PROFILE_1_1;
    if (fs_debug_state->integer)
    {
        Com_Printf("fs_state: connected_server_profile set to %s\n", fs_profile_to_string(fs_connected_server_profile));
    }
}

void FS_PureServerSetLoadedPaks(const char *hash_list, const char *name_list)
{
    int i;
    int count;

    pk3_list_free(&connected_server_pure_list);
    pk3_list_initialize(&connected_server_pure_list, 100);

    Cmd_TokenizeString(hash_list);
    count = Cmd_Argc();
    if (count > 4096)
        count = 4096;  // Sanity check

    for (i = 0; i < count; ++i)
    {
        pk3_list_insert(&connected_server_pure_list, atoi(Cmd_Argv(i)));
    }

    if (fs_debug_state->integer)
        Com_Printf("fs_state: connected_server_pure_list set to '%s'\n", hash_list);
}

void fs_disconnect_cleanup(void)
{
    current_map_pk3 = 0;
    connected_server_sv_pure = 0;
    fs_connected_server_profile = FS_PROFILE_UNSET;
    pk3_list_free(&connected_server_pure_list);
    if (fs_debug_state->integer)
        Com_Printf(
            "fs_state: disconnect cleanup\n"
            "   > current_map_pk3 cleared\n"
            "   > connected_server_sv_pure set to 0\n"
            "   > connected_server_pure_list cleared\n"
            "   > connected_server_profile cleared\n");
}

static void generate_current_mod_dir(const char *source, char *target)
{
    // Converts mod dir to format used in current_mod_dir, with base directories and basemod
    //   replaced by empty string
    // Target should be size FSC_MAX_MODDIR
    fs_sanitize_mod_dir(source, target);
    for (const char *name : {BASEGAME_1_1, BASEGAME_GPP, BASEGAME_1_3, BASEGAME_OVERRIDE})
    {
        if (!Q_stricmp(target, name))
            *target = 0;
    }
}

static qboolean matches_current_mod_dir(const char *mod_dir)
{
    char generated_mod_dir[FSC_MAX_MODDIR];
    generate_current_mod_dir(mod_dir, generated_mod_dir);
    return strcmp(current_mod_dir, generated_mod_dir) ? qfalse : qtrue;
}

#ifndef STANDALONE
void Com_AppendCDKey(const char *filename);
void Com_ReadCDKey(const char *filename);
#endif

void fs_set_mod_dir(const char *value, qboolean move_pid)
{
    char old_pid_dir[FSC_MAX_MODDIR];
    Q_strncpyz(old_pid_dir, fs_pid_file_directory(), sizeof(old_pid_dir));

    // Set current_mod_dir
    generate_current_mod_dir(value, current_mod_dir);

    /*
    // Move pid file to new mod dir if necessary
    if (move_pid && strcmp(old_pid_dir, fs_pid_file_directory()))
    {
        Sys_RemovePIDFile(old_pid_dir);
        Sys_InitPIDFile(fs_pid_file_directory());
    }
    */

    // Read CD keys
#ifndef STANDALONE
    if (!com_standalone->integer)
    {
        Com_ReadCDKey(BASEGAME);
        if (strcmp(FS_GetCurrentGameDir(), BASEGAME))
            Com_AppendCDKey(FS_GetCurrentGameDir());
    }
#endif

    Cvar_Set("fs_game", current_mod_dir);
    if (fs_debug_state->integer)
        Com_Printf("fs_state: current_mod_dir set to %s\n", *current_mod_dir ? current_mod_dir : "<none>");
}

qboolean FS_ConditionalRestart(int checksumFeed, qboolean disconnect)
{
    // This is used to activate a new mod dir, using a game restart if necessary to load
    // new settings. It also sets the checksum feed (used for pure validation) and clears
    // references from previous maps.
    // Returns qtrue if restarting due to changed mod dir, qfalse otherwise
    if (fs_debug_state->integer)
        Com_Printf("fs_state: FS_ConditionalRestart invoked\n");
    FS_ClearPakReferences(0);
    checksum_feed = checksumFeed;

    // Check for default.cfg here and attempt an ERR_DROP if it isn't found, to avoid getting
    // an ERR_FATAL later due to broken pure list
    if (!fs_general_lookup("default.cfg", LOOKUPFLAG_PURE_ALLOW_DIRECT_SOURCE, qfalse))
    {
        Com_Error(ERR_DROP, "Failed to find default.cfg, assuming invalid configuration");
    }

    // Check if we need to do a restart to load new config files
    if (fs_mod_settings->integer && !matches_current_mod_dir(fs_game->string))
    {
        Com_GameRestart(checksumFeed, disconnect);
        return qtrue;
    }

    // Just update the mod dir
    fs_set_mod_dir(fs_game->string, qtrue);
    return qfalse;
}

/* ******************************************************************************** */
// Source Directory Determination
/* ******************************************************************************** */

static qboolean prepare_writable_directory(const char *directory)
{
    // Attempts to create directory and tests writability
    // Returns qtrue if test passed, qfalse otherwise
    char path[FS_MAX_PATH];
    void *fp;

    if (!fs_generate_path(
            directory, "writetest.dat", 0, FS_CREATE_DIRECTORIES | FS_NO_SANITIZE, 0, 0, path, sizeof(path)))
        return qfalse;
    fp = fs_open_file(path, "wb");
    if (!fp)
        return qfalse;
    fsc_fclose(fp);

    fs_delete_file(path);
    return qtrue;
}

typedef struct {
    fs_source_directory_t s;
    int fs_dirs_position;  // lower means higher priority
    qboolean write_dir;
} temp_source_directory_t;

static int compare_temp_source_dirs(const temp_source_directory_t *dir1, const temp_source_directory_t *dir2)
{
    // Put write dir first, and sort remaining directories according to position in fs_dirs cvar
    if (dir1->s.active && !dir2->s.active)
        return -1;
    if (dir2->s.active && !dir1->s.active)
        return 1;
    if (dir1->write_dir && !dir2->write_dir)
        return -1;
    if (dir2->write_dir && !dir1->write_dir)
        return 1;
    if (dir1->fs_dirs_position < dir2->fs_dirs_position)
        return -1;
    return 1;
}

static int compare_temp_source_dirs_qsort(const void *dir1, const void *dir2)
{
    return compare_temp_source_dirs((const temp_source_directory_t *)dir1, (const temp_source_directory_t *)dir2);
}

void fs_initialize_sourcedirs(void)
{
    int i;
    qboolean have_write_dir = qfalse;
    char *fs_dirs_ptr;
    temp_source_directory_t temp_dirs[FS_MAX_SOURCEDIRS];
    const char *homepath = Sys_DefaultHomePath();

    // Initialize default path cvars
    Cvar_Get("fs_homepath", homepath ? homepath : "", CVAR_INIT | CVAR_PROTECTED);
    Cvar_Get("fs_basepath", Sys_DefaultInstallPath(), CVAR_INIT | CVAR_PROTECTED);
#ifdef __APPLE__
    Cvar_Get("fs_apppath", Sys_DefaultAppPath(), CVAR_INIT | CVAR_PROTECTED);
#endif

    // Generate temp_dirs based on fs_dirs entries
    Com_Memset(temp_dirs, 0, sizeof(temp_dirs));
    fs_dirs_ptr = fs_dirs->string;
    while (1)
    {
        qboolean write_flag = qfalse;
        qboolean write_dir = qfalse;
        qboolean auxiliary_dir = qfalse;
        const char *token;
        const char *path;

        // Get next token (source dir name) from fs_dirs
        token = COM_ParseExt(&fs_dirs_ptr, qfalse);
        if (!*token)
            break;

        // Process prefixes
        while (*token == '*' || *token == '#')
        {
            if (*token == '*')
                write_flag = qtrue;
            if (*token == '#')
                auxiliary_dir = qtrue;
            ++token;
        }
        if (!*token)
            continue;

        // Determine path from cvar
        path = Cvar_VariableString(token);
        if (!*path)
            continue;

        // Look for duplicate entry or next empty slot
        for (i = 0; i < FS_MAX_SOURCEDIRS; ++i)
        {
            if (!temp_dirs[i].s.active)
                break;
            if (!Q_stricmp(token, temp_dirs[i].s.name))
                break;
        }
        if (i >= FS_MAX_SOURCEDIRS)
        {
            Com_Printf("WARNING: FS_MAX_SOURCEDIRS exceeded parsing fs_dirs\n");
            break;
        }
        if (temp_dirs[i].s.active)
        {
            Com_Printf("WARNING: Duplicate entry '%s' parsing fs_dirs\n", token);
            continue;
        }

        // If write flag is set and no write directory yet, test writability
        if (write_flag && !have_write_dir)
        {
            Com_Printf("Checking if %s is writable...\n", token);
            if (prepare_writable_directory(path))
            {
                Com_Printf("Confirmed writable.\n");
                write_dir = qtrue;
                have_write_dir = qtrue;
            }
            else
            {
                Com_Printf("Not writable due to failed write test.\n");
            }
        }

        // Create entry in available slot
        temp_dirs[i] =
            (temp_source_directory_t){{CopyString(token), CopyString(path), qtrue, auxiliary_dir}, i, write_dir};
    }

    // Sort temp_dirs
    qsort(temp_dirs, ARRAY_LEN(temp_dirs), sizeof(*temp_dirs), compare_temp_source_dirs_qsort);

    // Check for read-only mode
    if (temp_dirs[0].write_dir)
    {
        fs_read_only = qfalse;
        Com_Printf("Write directory: %s (%s)\n", temp_dirs[0].s.name, temp_dirs[0].s.path);
    }
    else
    {
        fs_read_only = qtrue;
        Com_Printf("WARNING: No write directory selected. Filesystem in read-only mode.\n");
    }

    // Transfer entries from temp_dirs to fs_sourcedirs
    for (i = 0; i < FS_MAX_SOURCEDIRS; ++i)
    {
        fs_sourcedirs[i] = temp_dirs[i].s;
        if (fs_sourcedirs[i].active)
            Com_Printf("Source directory %i%s: %s (%s)\n", i + 1, fs_sourcedirs[i].auxiliary ? " [AUX]" : "",
                fs_sourcedirs[i].name, fs_sourcedirs[i].path);
    }
}

/* ******************************************************************************** */
// Filesystem Refresh
/* ******************************************************************************** */

static void refresh_errorhandler(int id, const char *msg, void *current_element, void *context)
{
    if (fs_debug_refresh->integer)
    {
        const char *type = "general";
        if (id == FSC_ERROR_PK3FILE)
            type = "pk3";
        if (id == FSC_ERROR_SHADERFILE)
            type = "shader";
        Com_Printf("********** refresh %s error **********\n", type);

        if (current_element)
        {
            char buffer[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer((fsc_file_t *)current_element, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
            Com_Printf("file: %s\n", buffer);
        }
        Com_Printf("message: %s\n", msg);
    }
}

#define NON_PK3_FILES(stats) (stats.total_file_count - stats.pk3_subfile_count - stats.valid_pk3_count)

static void index_directory(const char *directory, int dir_id, qboolean quiet)
{
    fsc_errorhandler_t errorhandler = {refresh_errorhandler, 0};
    fsc_stats_t old_active_stats = fs.active_stats;
    fsc_stats_t old_total_stats = fs.total_stats;
    void *os_path = fsc_string_to_os_path(directory);

    fsc_load_directory(&fs, os_path, dir_id, &errorhandler);
    fsc_free(os_path);

    if (!quiet)
    {
        Com_Printf("Indexed %i files in %i pk3s, %i other files, and %i shaders.\n",
            fs.active_stats.pk3_subfile_count - old_active_stats.pk3_subfile_count,
            fs.active_stats.valid_pk3_count - old_active_stats.valid_pk3_count,
            NON_PK3_FILES(fs.active_stats) - NON_PK3_FILES(old_active_stats),
            fs.active_stats.shader_count - old_active_stats.shader_count);

        Com_Printf("%i files in %i pk3s and %i shaders had not been previously indexed.\n",
            fs.total_stats.pk3_subfile_count - old_total_stats.pk3_subfile_count,
            fs.total_stats.valid_pk3_count - old_total_stats.valid_pk3_count,
            fs.total_stats.shader_count - old_total_stats.shader_count);
    }
}

extern int com_frameNumber;
static int fs_refresh_frame = 0;

void fs_refresh(qboolean quiet)
{
    int i;
    if (fs_debug_refresh->integer)
        quiet = qfalse;
    if (!quiet)
        Com_Printf("----- fs_refresh -----\n");

    fsc_filesystem_reset(&fs);

    for (i = 0; i < FS_MAX_SOURCEDIRS; ++i)
    {
        if (!fs_sourcedirs[i].active)
            continue;
        if (!quiet)
            Com_Printf("Indexing %s...\n", fs_sourcedirs[i].name);
        index_directory(fs_sourcedirs[i].path, i, quiet);
    }

    if (!quiet)
        Com_Printf("Index memory usage at %iMB.\n", fsc_fs_size_estimate(&fs) / 1048576 + 1);

    fs_refresh_frame = com_frameNumber;
    fs_readback_tracker_reset();
}

void fs_refresh_auto_ext(qboolean ignore_cvar, qboolean quiet)
{
    // Calls fs_refresh if enabled by settings, but maximum once per several frames
    //    to avoid redundant refreshes during load operations
    // This can be called in any situation where there is a potential that files were
    //    added or changed on disk
    // The ignore_cvar option is used for the fs_refresh console command, which should
    //    work for manual refreshes even if fs_auto_refresh setting is disabled
    int frames_elapsed = com_frameNumber - fs_refresh_frame;
    if (!ignore_cvar && !fs_auto_refresh->integer)
        return;
    if (frames_elapsed >= 0 && frames_elapsed < 5)
    {
        if (fs_debug_refresh->integer)
            Com_Printf("Skipping redundant filesystem auto refresh.\n");
        return;
    }
    fs_refresh(quiet);
}

void fs_refresh_auto(void) { fs_refresh_auto_ext(qfalse, qtrue); }

/* ******************************************************************************** */
// Filesystem Initialization
/* ******************************************************************************** */

static void *get_fscache_path(void)
{
    char path[FS_MAX_PATH];
    if (!fs_generate_path_sourcedir(0, "fscache.dat", 0, 0, 0, path, sizeof(path)))
        return 0;
    return fsc_string_to_os_path(path);
}

void fs_indexcache_write(void)
{
    void *ospath = get_fscache_path();
    if (!ospath)
        return;
    fsc_cache_export_file(&fs, ospath, 0);
    fsc_free(ospath);
}

static qboolean fs_filesystem_refresh_tracked(void)
{
    // Calls fs_refresh, returns qtrue if enough changed to justify rewriting fscache.dat,
    // returns qfalse otherwise
    fsc_stats_t old_total_stats = fs.total_stats;
    fs_refresh(qfalse);
    if (fs.total_stats.valid_pk3_count - old_total_stats.valid_pk3_count > 20 ||
        fs.total_stats.pk3_subfile_count - old_total_stats.pk3_subfile_count > 5000 ||
        fs.total_stats.shader_file_count - old_total_stats.shader_file_count > 100 ||
        fs.total_stats.shader_count - old_total_stats.shader_count > 5000)
    {
        return qtrue;
    }
    return qfalse;
}

static void fs_initialize_index(void)
{
    // Initialize the fs structure, using cache file if possible
    qboolean cache_loaded = qfalse;

    if (fs_index_cache->integer)
    {
        void *path = get_fscache_path();
        Com_Printf("Loading fscache.dat...\n");
        if (path)
        {
            if (!fsc_cache_import_file(path, &fs, 0))
                cache_loaded = qtrue;
            fsc_free(path);
        }

        if (!cache_loaded)
            Com_Printf("Failed to load fscache.dat.\n");
    }

    if (cache_loaded)
    {
        Com_Printf("Index data loaded for %i files, %i pk3s, and %i shaders.\n",
            fs.files.utilization - fs.pk3_hash_lookup.utilization, fs.pk3_hash_lookup.utilization,
            fs.shaders.utilization);

        if (fs_debug_refresh->integer)
        {
            Com_Printf(
                "WARNING: Using index cache may prevent fs_debug_refresh error messages from being logged."
                " For full debug info consider setting fs_index_cache to 0 or temporarily removing fscache.dat.\n");
        }
    }
    else
    {
        fsc_filesystem_initialize(&fs);
    }
}

static void fs_fatal_error_handler(const char *msg) { Com_Error(ERR_FATAL, "filesystem error: %s", msg); }

void fs_startup(void)
{
    // Initial startup, should only be called once
    Com_Printf("\n----- fs_startup -----\n");

    fsc_register_fatal_error_handler(fs_fatal_error_handler);

#ifdef __APPLE__
    fs_dirs = Cvar_Get("fs_dirs", "*fs_homepath fs_basepath fs_apppath", CVAR_INIT | CVAR_PROTECTED);
#else
    fs_dirs = Cvar_Get("fs_dirs", "*fs_homepath fs_basepath", CVAR_INIT | CVAR_PROTECTED);
#endif
    fs_mod_settings = Cvar_Get("fs_mod_settings", "0", CVAR_INIT);
    fs_index_cache = Cvar_Get("fs_index_cache", "1", CVAR_INIT);
    fs_read_inactive_mods = Cvar_Get("fs_read_inactive_mods", "1", CVAR_ARCHIVE);
    fs_list_inactive_mods = Cvar_Get("fs_list_inactive_mods", "1", CVAR_ARCHIVE);
    fs_download_manifest =
        Cvar_Get("fs_download_manifest", "*mod_paks *cgame_pak *ui_pak *currentmap_pak *referenced_paks", CVAR_ARCHIVE);
    fs_pure_manifest = Cvar_Get("fs_pure_manifest", "*mod_paks *base_paks *inactivemod_paks", CVAR_ARCHIVE);
    fs_redownload_across_mods = Cvar_Get("fs_redownload_across_mods", "1", CVAR_ARCHIVE);
    fs_full_pure_validation = Cvar_Get("fs_full_pure_validation", "0", CVAR_ARCHIVE);
    fs_saveto_dlfolder = Cvar_Get("fs_saveto_dlfolder", "0", CVAR_ARCHIVE);
    fs_restrict_dlfolder = Cvar_Get("fs_restrict_dlfolder", "0", CVAR_ARCHIVE);
    fs_auto_refresh = Cvar_Get("fs_auto_refresh", "1", 0);

    fs_debug_state = Cvar_Get("fs_debug_state", "0", 0);
    fs_debug_refresh = Cvar_Get("fs_debug_refresh", "0", 0);
    fs_debug_fileio = Cvar_Get("fs_debug_fileio", "0", 0);
    fs_debug_lookup = Cvar_Get("fs_debug_lookup", "0", 0);
    fs_debug_references = Cvar_Get("fs_debug_references", "0", 0);
    fs_debug_filelist = Cvar_Get("fs_debug_filelist", "0", 0);

    fs_basegame = Cvar_Get("fs_basegame", FS_BASEGAME_DEFAULT, CVAR_INIT);
    fs_force_profile = Cvar_Get("fs_force_profile", "", 0);

    Cvar_Get("new_filesystem", "1", CVAR_ROM);  // Enables new filesystem calls in renderer

    fs_initialize_sourcedirs();
    fs_initialize_index();

    fs_game = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
    fs_set_mod_dir(fs_game->string, qfalse);

    Com_Printf("\n");
    if (fs_filesystem_refresh_tracked() && fs_index_cache->integer && !fs_read_only)
    {
        Com_Printf("Writing fscache.dat due to updated files...\n");
        fs_indexcache_write();
    }
    Com_Printf("\n");

    fs_register_commands();
    fs_initialized = qtrue;

#ifndef STANDALONE
    fs_check_core_paks();
#endif
}

#endif  // NEW_FILESYSTEM
