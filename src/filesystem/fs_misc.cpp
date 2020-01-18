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
// Indented Debug Print Support
/* ******************************************************************************** */

// This section is used to support indented prints for the cvar-enabled debug logging options
//    to make the output more readable, especially if there are nested calls to functions that
//    produce cluster-type prints
// Theoretically the level could be messed up by Com_Error, but since it's a pretty obscure
//    scenario and this is ONLY used for cvar-enabled debug prints I'm not bothering with it

static int fs_debug_indent_level = 0;

void fs_debug_indent_start(void) { ++fs_debug_indent_level; }

void fs_debug_indent_stop(void)
{
    --fs_debug_indent_level;
    if (fs_debug_indent_level < 0)
    {
        Com_Printf("WARNING: Negative filesystem debug increment\n");
        fs_debug_indent_level = 0;
    }
}

void QDECL FS_DPrintf(const char *fmt, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    unsigned int indent = (unsigned int)fs_debug_indent_level;
    char spaces[16] = "               ";

    va_start(argptr, fmt);
    Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    if (indent > 4)
        indent = 4;
    spaces[indent * 2] = 0;
    Com_Printf("%s%s", spaces, msg);
}

/* ******************************************************************************** */
// Hash Table
/* ******************************************************************************** */

void fs_hashtable_initialize(fs_hashtable_t *hashtable, int bucket_count)
{
    // Valid for an uninitialized hash table
    FSC_ASSERT(hashtable);
    FSC_ASSERT(bucket_count > 0);
    hashtable->bucket_count = bucket_count;
    hashtable->buckets = (fs_hashtable_entry_t **)Z_Malloc(sizeof(fs_hashtable_entry_t *) * bucket_count);
    hashtable->element_count = 0;
}

void fs_hashtable_insert(fs_hashtable_t *hashtable, fs_hashtable_entry_t *entry, unsigned int hash)
{
    // Valid for an initialized hash table
    int index;
    FSC_ASSERT(hashtable);
    FSC_ASSERT(hashtable->bucket_count > 0);
    FSC_ASSERT(entry);
    index = hash % hashtable->bucket_count;
    entry->next = hashtable->buckets[index];
    hashtable->buckets[index] = entry;
    ++hashtable->element_count;
}

fs_hashtable_iterator_t fs_hashtable_iterate(fs_hashtable_t *hashtable, unsigned int hash, qboolean iterate_all)
{
    // Valid for an initialized or uninitialized (zeroed) hashtable
    fs_hashtable_iterator_t iterator;
    FSC_ASSERT(hashtable);
    iterator.ht = hashtable;
    if (!hashtable->bucket_count || iterate_all)
    {
        iterator.current_bucket = 0;
        iterator.bucket_limit = hashtable->bucket_count;
    }
    else
    {
        iterator.current_bucket = hash % hashtable->bucket_count;
        iterator.bucket_limit = iterator.current_bucket + 1;
    }
    iterator.current_entry = 0;
    return iterator;
}

void *fs_hashtable_next(fs_hashtable_iterator_t *iterator)
{
    fs_hashtable_entry_t *entry = iterator->current_entry;
    while (!entry)
    {
        if (iterator->current_bucket >= iterator->bucket_limit)
            return 0;
        entry = iterator->ht->buckets[iterator->current_bucket++];
    }
    iterator->current_entry = entry->next;
    return entry;
}

static void fs_hashtable_free_entries(fs_hashtable_t *hashtable, void (*free_entry)(fs_hashtable_entry_t *entry))
{
    // Valid for an initialized or uninitialized (zeroed) hashtable
    fs_hashtable_iterator_t it = fs_hashtable_iterate(hashtable, 0, qtrue);
    fs_hashtable_entry_t *entry;
    if (free_entry)
        while ((entry = (fs_hashtable_entry_t *)fs_hashtable_next(&it)))
            free_entry(entry);
    else
        while ((entry = (fs_hashtable_entry_t *)fs_hashtable_next(&it)))
            Z_Free(entry);
}

void fs_hashtable_free(fs_hashtable_t *hashtable, void (*free_entry)(fs_hashtable_entry_t *entry))
{
    // Valid for an initialized or uninitialized (zeroed) hashtable
    FSC_ASSERT(hashtable);
    fs_hashtable_free_entries(hashtable, free_entry);
    if (hashtable->buckets)
        Z_Free(hashtable->buckets);
    Com_Memset(hashtable, 0, sizeof(*hashtable));
}

void fs_hashtable_reset(fs_hashtable_t *hashtable, void (*free_entry)(fs_hashtable_entry_t *entry))
{
    // Valid for an initialized hash table
    FSC_ASSERT(hashtable);
    fs_hashtable_free_entries(hashtable, free_entry);
    Com_Memset(hashtable->buckets, 0, sizeof(*hashtable->buckets) * hashtable->bucket_count);
    hashtable->element_count = 0;
}

/* ******************************************************************************** */
// Pk3 List
/* ******************************************************************************** */

// The pk3 list structure maps pk3 hashes to index value
// First pk3 inserted has index 1, second pk3 has index 2, etc.
// If same hash is inserted multiple times, the first index will be used

void pk3_list_initialize(pk3_list_t *pk3_list, unsigned int bucket_count)
{
    FSC_ASSERT(pk3_list);
    fs_hashtable_initialize(&pk3_list->ht, bucket_count);
}

int pk3_list_lookup(const pk3_list_t *pk3_list, unsigned int hash)
{
    fs_hashtable_iterator_t it;
    pk3_list_entry_t *entry;
    FSC_ASSERT(pk3_list);
    it = fs_hashtable_iterate((fs_hashtable_t *)&pk3_list->ht, hash, qfalse);
    while ((entry = (pk3_list_entry_t *)fs_hashtable_next(&it)))
    {
        if (entry->hash == hash)
        {
            return entry->position;
        }
    }
    return 0;
}

void pk3_list_insert(pk3_list_t *pk3_list, unsigned int hash)
{
    pk3_list_entry_t *new_entry;
    FSC_ASSERT(pk3_list);
    if (pk3_list_lookup(pk3_list, hash))
        return;
    new_entry = (pk3_list_entry_t *)S_Malloc(sizeof(pk3_list_entry_t));
    fs_hashtable_insert(&pk3_list->ht, &new_entry->hte, hash);
    new_entry->hash = hash;
    new_entry->position = pk3_list->ht.element_count;
}

void pk3_list_free(pk3_list_t *pk3_list)
{
    FSC_ASSERT(pk3_list);
    fs_hashtable_free(&pk3_list->ht, 0);
}

/* ******************************************************************************** */
// File helper functions
/* ******************************************************************************** */

const char *fs_file_extension(const fsc_file_t *file)
{
    // Returns null for no extension
    FSC_ASSERT(file);
    return (const char *)STACKPTRN(file->qp_ext_ptr);
}

qboolean fs_files_from_same_pk3(const fsc_file_t *file1, const fsc_file_t *file2)
{
    // Returns qtrue if both files are located in the same pk3, qfalse otherwise
    // Used by renderer for md3 lod handling
    if (!file1 || !file2 || file1->sourcetype != FSC_SOURCETYPE_PK3 || file2->sourcetype != FSC_SOURCETYPE_PK3 ||
        ((fsc_file_frompk3_t *)file1)->source_pk3 != ((fsc_file_frompk3_t *)file2)->source_pk3)
        return qfalse;
    return qtrue;
}

int fs_get_source_dir_id(const fsc_file_t *file)
{
    const fsc_file_direct_t *base_file;
    FSC_ASSERT(file);
    base_file = fsc_get_base_file(file, &fs);
    if (base_file)
        return base_file->source_dir_id;
    return -1;
}

const char *fs_get_source_dir_string(const fsc_file_t *file)
{
    int id = fs_get_source_dir_id(file);
    if (id >= 0 && id < FS_MAX_SOURCEDIRS && fs_sourcedirs[id].active)
        return fs_sourcedirs[id].name;
    return "unknown";
}

void fs_file_to_stream(const fsc_file_t *file, fsc_stream_t *stream, qboolean include_source_dir, qboolean include_mod,
    qboolean include_pk3_origin, qboolean include_size)
{
    FSC_ASSERT(file && stream);
    if (include_source_dir)
    {
        fsc_stream_append_string(stream, fs_get_source_dir_string(file));
        fsc_stream_append_string(stream, "->");
    }
    fsc_file_to_stream(file, stream, &fs, include_mod, include_pk3_origin);

    if (include_size)
    {
        char buffer[24];
        Com_sprintf(buffer, sizeof(buffer), " (%i bytes)", file->filesize);
        fsc_stream_append_string(stream, buffer);
    }
}

void fs_file_to_buffer(const fsc_file_t *file, char *buffer, unsigned int buffer_size, qboolean include_source_dir,
    qboolean include_mod, qboolean include_pk3_origin, qboolean include_size)
{
    fsc_stream_t stream = {buffer, 0, buffer_size};
    FSC_ASSERT(file && buffer);
    fs_file_to_stream(file, &stream, include_source_dir, include_mod, include_pk3_origin, include_size);
}

void fs_print_file_location(const fsc_file_t *file)
{
    char name_buffer[FS_FILE_BUFFER_SIZE];
    char source_buffer[FS_FILE_BUFFER_SIZE];
    FSC_ASSERT(file);
    fs_file_to_buffer(file, name_buffer, sizeof(name_buffer), qfalse, qfalse, qfalse, qfalse);
    if (file->sourcetype == FSC_SOURCETYPE_PK3)
    {
        fs_file_to_buffer((const fsc_file_t *)fsc_get_base_file(file, &fs), source_buffer, sizeof(source_buffer), qtrue,
            qtrue, qfalse, qfalse);
        Com_Printf("File %s found in %s\n", name_buffer, source_buffer);
    }
    else if (file->sourcetype == FSC_SOURCETYPE_DIRECT)
    {
        fs_file_to_buffer(file, source_buffer, sizeof(source_buffer), qtrue, qtrue, qfalse, qfalse);
        Com_Printf("File %s found at %s\n", name_buffer, source_buffer);
    }
    else
        Com_Printf("File %s has unknown sourcetype\n", name_buffer);
}

/* ******************************************************************************** */
// File disabled check
/* ******************************************************************************** */

static int get_pk3_list_position(const fsc_file_t *file)
{
    if (file->sourcetype != FSC_SOURCETYPE_PK3)
        return 0;
    return pk3_list_lookup(&connected_server_pure_list, fsc_get_base_file(file, &fs)->pk3_hash);
}

static qboolean inactive_mod_file_disabled(const fsc_file_t *file, int level)
{
    // Check if a file is disabled by inactive mod settings

    // Allow file if full inactive mod searching is enabled
    if (level >= 2)
        return qfalse;

    // Allow file if not in inactive mod directory
    if (fs_get_mod_type(fsc_get_mod_dir(file, &fs)) > MODTYPE_INACTIVE)
        return qfalse;

    // For setting 1, also allow files from core paks or on pure list
    if (level == 1)
    {
        const fsc_file_direct_t *base_file = fsc_get_base_file(file, &fs);
        if (base_file)
        {
            if (pk3_list_lookup(&connected_server_pure_list, base_file->pk3_hash))
                return qfalse;
            if (core_pk3_position(base_file->pk3_hash))
                return qfalse;
        }
    }

    return qtrue;
}

int fs_file_disabled(const fsc_file_t *file, int checks)
{
    // This function is used to perform various checks for whether a file should be used by the filesystem
    // Returns value of one of the triggering checks if file is disabled, null otherwise
    FSC_ASSERT(file);

    // File disabled check - blocks files disabled in the file index (they should never be accessed)
    if ((checks & FD_CHECK_FILE_ENABLED) && !fsc_is_file_enabled(file, &fs))
        return FD_CHECK_FILE_ENABLED;

    // Pure list check - blocks files disabled by pure settings of server we are connected to
    if ((checks & FD_CHECK_PURE_LIST) && fs_connected_server_pure_state() == 1)
    {
        if (!get_pk3_list_position(file))
            return FD_CHECK_PURE_LIST;
    }

    // Search inactive mods check - blocks files disabled by inactive mod settings for file reading
    if ((checks & FD_CHECK_READ_INACTIVE_MODS) && inactive_mod_file_disabled(file, fs_read_inactive_mods->integer))
    {
        return FD_CHECK_READ_INACTIVE_MODS;
    }

    // List inactive mods check - blocks files disabled by inactive mod settings for file listing
    if (checks & FD_CHECK_LIST_INACTIVE_MODS)
    {
        // Use read_inactive_mods setting if it is lower, because it doesn't make sense to list unreadable files
        int list_inactive_mods_level = fs_read_inactive_mods->integer < fs_list_inactive_mods->integer
                                           ? fs_read_inactive_mods->integer
                                           : fs_list_inactive_mods->integer;
        if (inactive_mod_file_disabled(file, list_inactive_mods_level))
            return FD_CHECK_LIST_INACTIVE_MODS;
    }

    // List auxiliary sourcedir check - blocks files from auxiliary source directories for file listing
    if (checks & FD_CHECK_LIST_AUXILIARY_SOURCEDIR)
    {
        const fsc_file_direct_t *base_file = fsc_get_base_file(file, &fs);
        if (base_file && fs_sourcedirs[base_file->source_dir_id].auxiliary && !get_pk3_list_position(file))
        {
            return FD_CHECK_LIST_AUXILIARY_SOURCEDIR;
        }
    }

    return 0;
}

/* ******************************************************************************** */
// File Sorting Functions
/* ******************************************************************************** */

// The lookup, file list, and reference modules have their own sorting systems due
//   to differences in requirements. Some sorting logic and functions that are shared
//   between multiple modules are included here.

static const unsigned char *get_string_sort_table(void)
{
    // The table maps characters to a precedence value
    // higher value = higher precedence
    qboolean initialized = qfalse;
    static unsigned char table[256];

    if (!initialized)
    {
        int i;
        unsigned char value = 250;
        for (i = 'z'; i >= 'a'; --i)
            table[i] = value--;
        value = 250;
        for (i = 'Z'; i >= 'A'; --i)
            table[i] = value--;
        for (i = '9'; i >= '0'; --i)
            table[i] = value--;
        for (i = 255; i >= 0; --i)
            if (!table[i])
                table[i] = value--;
        initialized = qtrue;
    }

    return table;
}

static unsigned int server_pure_precedence(const fsc_file_t *file)
{
    if (file->sourcetype == FSC_SOURCETYPE_PK3)
    {
        // Pure list stores pk3s by position, with index 1 at highest priority,
        //   so index values need to be inverted to get precedence
        unsigned int index = pk3_list_lookup(&connected_server_pure_list, fsc_get_base_file(file, &fs)->pk3_hash);
        if (index)
            return ~index;
    }
    return 0;
}

static unsigned int mod_dir_precedence(FS_ModType mod_type)
{
    if (mod_type >= MODTYPE_OVERRIDE_DIRECTORY)
        return (unsigned int)mod_type;
    return 0;
}

static unsigned int core_pak_precedence(const fsc_file_t *file, FS_ModType mod_type)
{
    if (mod_type < MODTYPE_OVERRIDE_DIRECTORY)
    {
        const fsc_file_direct_t *base_file = fsc_get_base_file(file, &fs);
        if (base_file)
            return core_pk3_position(base_file->pk3_hash);
    }
    return 0;
}

static unsigned int basegame_dir_precedence(FS_ModType mod_type) { return (unsigned int)mod_type; }

void fs_write_sort_string(const char *string, fsc_stream_t *output, qboolean prioritize_shorter)
{
    // Set prioritize_shorter true to prioritize shorter strings (i.e. "abc" over "abcd")
    const unsigned char *sort_table = get_string_sort_table();
    while (*string && output->position < output->size)
    {
        output->data[output->position++] = (char)sort_table[*(unsigned char *)(string++)];
    }
    if (output->position < output->size)
        output->data[output->position++] = prioritize_shorter ? (char)(unsigned char)255 : 0;
}

void fs_write_sort_filename(const fsc_file_t *file, fsc_stream_t *output)
{
    // Write sort key of the file itself
    char buffer[FS_FILE_BUFFER_SIZE];
    fs_file_to_buffer(file, buffer, sizeof(buffer), qfalse, qfalse, qfalse, qfalse);
    fs_write_sort_string(buffer, output, qfalse);
}

static void write_sort_pk3_source_filename(const fsc_file_t *file, fsc_stream_t *output)
{
    // Write sort key of the pk3 file or pk3dir the file came from
    if (file->sourcetype == FSC_SOURCETYPE_DIRECT && ((fsc_file_direct_t *)file)->pk3dir_ptr)
    {
        fs_write_sort_string((const char *)STACKPTR(((fsc_file_direct_t *)file)->pk3dir_ptr), output, qfalse);
        fs_write_sort_value(1, output);
    }
    else if (file->sourcetype == FSC_SOURCETYPE_PK3)
    {
        const fsc_file_direct_t *source_pk3 = fsc_get_base_file(file, &fs);
        fs_write_sort_string((const char *)STACKPTR(source_pk3->f.qp_name_ptr), output, qfalse);
        fs_write_sort_value(0, output);
    }
}

void fs_write_sort_value(unsigned int value, fsc_stream_t *output)
{
    static volatile int test = 1;
    if (*(char *)&test)
    {
        value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
        value = (value << 16) | (value >> 16);
    }
    if (output->position + 3 < output->size)
    {
        *((unsigned int *)(output->data + output->position)) = value;
        output->position += 4;
    }
}

void fs_generate_core_sort_key(const fsc_file_t *file, fsc_stream_t *output, qboolean use_server_pure_list)
{
    // This is a rough version of the lookup precedence for reference and file listing purposes
    // This sorts the mod/pk3 origin of the file, but not the actual file name, or the source directory
    //    since the file list system handles file names separately and currently ignores source directory
    FS_ModType mod_type = fs_get_mod_type(fsc_get_mod_dir(file, &fs));
    if (use_server_pure_list)
        fs_write_sort_value(server_pure_precedence(file), output);
    fs_write_sort_value(mod_dir_precedence(mod_type), output);
    fs_write_sort_value(core_pak_precedence(file, mod_type), output);
    fs_write_sort_value(basegame_dir_precedence(mod_type), output);
    // Deprioritize download folder pk3s, whether the flag is set for this file or this file's source pk3
    fs_write_sort_value(
        (file->flags & FSC_FILEFLAG_DLPK3) ||
                (file->sourcetype == FSC_SOURCETYPE_PK3 && (fsc_get_base_file(file, &fs)->f.flags & FSC_FILEFLAG_DLPK3))
            ? 0
            : 1,
        output);
    if (file->sourcetype == FSC_SOURCETYPE_PK3 ||
        (file->sourcetype == FSC_SOURCETYPE_DIRECT && ((fsc_file_direct_t *)file)->pk3dir_ptr))
    {
        fs_write_sort_value(0, output);
        write_sort_pk3_source_filename(file, output);
        fs_write_sort_value(
            (file->sourcetype == FSC_SOURCETYPE_PK3) ? ~((fsc_file_frompk3_t *)file)->header_position : ~0u, output);
    }
    else
    {
        fs_write_sort_value(1, output);
    }
}

int fs_compare_pk3_source(const fsc_file_t *file1, const fsc_file_t *file2)
{
    char buffer1[1024];
    char buffer2[1024];
    fsc_stream_t stream1 = {buffer1, 0, sizeof(buffer1), qfalse};
    fsc_stream_t stream2 = {buffer2, 0, sizeof(buffer2), qfalse};
    write_sort_pk3_source_filename(file1, &stream1);
    write_sort_pk3_source_filename(file2, &stream2);
    return fsc_memcmp(
        stream2.data, stream1.data, stream1.position < stream2.position ? stream1.position : stream2.position);
}

/* ******************************************************************************** */
// Misc Functions
/* ******************************************************************************** */

void fs_execute_config_file(const char *name, fs_config_type_t config_type, cbufExec_t exec_type, qboolean quiet)
{
    char *data;
    unsigned int size = 0;

    if (com_journalDataFile && com_journal->integer == 2)
    {
        // In journal playback mode, try to load config files from journal data file
        Com_Printf("execing %s from journal data file\n", name);
        data = fs_read_journal_data();
        if (!data)
        {
            Com_Printf("couldn't exec %s - not present in journal\n", name);
            return;
        }
    }
    else
    {
        const fsc_file_t *file;
        int lookup_flags =
            LOOKUPFLAG_PURE_ALLOW_DIRECT_SOURCE | LOOKUPFLAG_IGNORE_CURRENT_MAP | LOOKUPFLAG_PRIORITIZE_FS_BASEGAME;
        if (fs_restrict_dlfolder->integer)
        {
            // Don't allow config files from restricted download folder pk3s, because they could disable the download
            // folder restrictions to unrestrict themselves
            lookup_flags |= LOOKUPFLAG_NO_DOWNLOAD_FOLDER;
        }
        if (config_type == FS_CONFIGTYPE_SETTINGS)
        {
            // For autogen.cfg and autoexec.cfg - only load files on disk and from appropriate fs_mod_settings
            // locations
            lookup_flags |= (LOOKUPFLAG_SETTINGS_FILE | LOOKUPFLAG_DIRECT_SOURCE_ONLY);
        }

        // NOTE: For now, just use the standard precedence for default.cfg. Since it gets overridden by the settings
        // config files executed afterwards it usually shouldn't cause any problems even if it doesn't follow the
        // fs_mod_settings policy.

        if (!quiet)
            Com_Printf("execing %s\n", name);

        // Locate file
        fs_refresh_auto();
        file = fs_general_lookup(name, lookup_flags, qfalse);
        if (!file)
        {
            Com_Printf("couldn't exec %s - file not found\n", name);
            fs_write_journal_data(0, 0);
            return;
        }

        // Load data
        data = fs_read_data(file, 0, &size, "fs_execute_config_file");
        if (!data)
        {
            Com_Printf("couldn't exec %s - failed to read data\n", name);
            fs_write_journal_data(0, 0);
            return;
        }
    }

    fs_write_journal_data(data, size);

    Cbuf_ExecuteText(exec_type, data);
    if (exec_type == EXEC_APPEND)
        Cbuf_ExecuteText(EXEC_APPEND, "\n");
    fs_free_data(data);
}

void *fs_load_game_dll(
    const fsc_file_t *dll_file, intptr_t(QDECL **entryPoint)(int, ...), intptr_t(QDECL *systemcalls)(intptr_t, ...))
{
    // Used by vm.c
    // Returns dll handle, or null on error
    char dll_info_string[FS_FILE_BUFFER_SIZE];
    const void *dll_path;
    char *dll_path_string;
    void *dll_handle;
    FSC_ASSERT(dll_file);

    // Print the info message
    fs_file_to_buffer(dll_file, dll_info_string, sizeof(dll_info_string), qtrue, qtrue, qtrue, qfalse);
    Com_Printf("Attempting to load dll file at %s\n", dll_info_string);

    // Get dll path
    if (dll_file->sourcetype != FSC_SOURCETYPE_DIRECT)
    {
        // Shouldn't happen
        Com_Printf("Error: selected dll is not direct sourcetype\n");
        return 0;
    }
    dll_path = STACKPTR(((fsc_file_direct_t *)dll_file)->os_path_ptr);
    dll_path_string = fsc_os_path_to_string(dll_path);
    if (!dll_path_string)
    {
        // Generally shouldn't happen
        Com_Printf("Error: failed to convert dll path\n");
        return 0;
    }

    // Attemt to open the dll
    dll_handle = Sys_LoadGameDll(dll_path_string, entryPoint, systemcalls);
    if (!dll_handle)
    {
        Com_Printf("Error: failed to load game dll\n");
    }
    fsc_free(dll_path_string);
    return dll_handle;
}

void FS_GetModDescription(const char *modDir, char *description, int descriptionLen)
{
    const char *descPath = va("%s/description.txt", modDir);
    fileHandle_t descHandle;
    int descLen = FS_SV_FOpenFileRead(descPath, &descHandle);
    if (descLen > 0 && descHandle)
    {
        descLen = FS_Read(description, descriptionLen - 1, descHandle);
        description[descLen] = 0;
    }
    if (descHandle)
        fs_handle_close(descHandle);
    if (descLen <= 0)
    {
        // Just use the mod name as the description
        Q_strncpyz(description, modDir, descriptionLen);
    }
}

void FS_FilenameCompletion(const char *dir, const char *ext, qboolean stripExt, void (*callback)(const char *s),
    qboolean allowNonPureFilesOnDisk)
{
    char **filenames;
    int nfiles;
    int i;
    char filename[MAX_STRING_CHARS];

    // Currently using the less restrictive FLISTFLAG_IGNORE_PURE_LIST when allowNonPureFilesOnDisk is
    //    false, since that's what's used for map completion, and we want to ignore the pure list there
    filenames = FS_FlagListFilteredFiles(dir, ext, NULL, &nfiles,
        allowNonPureFilesOnDisk ? FLISTFLAG_PURE_ALLOW_DIRECT_SOURCE : FLISTFLAG_IGNORE_PURE_LIST);

    for (i = 0; i < nfiles; i++)
    {
        Q_strncpyz(filename, filenames[i], MAX_STRING_CHARS);

        if (stripExt)
        {
            COM_StripExtension(filename, filename, sizeof(filename));
        }

        callback(filename);
    }
    FS_FreeFileList(filenames);
}

// From old filesystem. Used in a couple of places.
qboolean FS_FilenameCompare(const char *s1, const char *s2)
{
    int c1, c2;

    do
    {
        c1 = *s1++;
        c2 = *s2++;

        if (c1 >= 'a' && c1 <= 'z')
        {
            c1 -= ('a' - 'A');
        }
        if (c2 >= 'a' && c2 <= 'z')
        {
            c2 -= ('a' - 'A');
        }

        if (c1 == '\\' || c1 == ':')
        {
            c1 = '/';
        }
        if (c2 == '\\' || c2 == ':')
        {
            c2 = '/';
        }

        if (c1 != c2)
        {
            return qtrue;  // strings not equal
        }
    } while (c1);

    return qfalse;  // strings are equal
}

void QDECL FS_Printf(fileHandle_t h, const char *fmt, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    FS_Write(msg, strlen(msg), h);
}

void fs_comma_separated_list(const char **strings, int count, fsc_stream_t *output)
{
    // Writes array of strings to stream separated by comma (useful for debug print purposes)
    // Ignores strings that are null or empty
    // Writes "<none>" if nothing was written
    int i;
    qboolean have_item = qfalse;
    FSC_ASSERT(strings);
    FSC_ASSERT(output);
    fsc_stream_append_string(output, "");
    for (i = 0; i < count; ++i)
    {
        if (strings[i] && *strings[i])
        {
            if (have_item)
                fsc_stream_append_string(output, ", ");
            fsc_stream_append_string(output, strings[i]);
            have_item = qtrue;
        }
    }
    if (!have_item)
        fsc_stream_append_string(output, "<none>");
}

qboolean FS_idPak(const char *pak, const char *base, int numPaks)
{
    int i;

    for (i = 0; i < numPaks; i++)
    {
        if (!FS_FilenameCompare(pak, va("%s/pak%d", base, i)))
        {
            break;
        }
    }
    if (i < numPaks)
    {
        return qtrue;
    }
    return qfalse;
}

void fs_sanitize_mod_dir(const char *source, char *target)
{
    // Sanitizes mod dir string. If mod dir is invalid it will be replaced with empty string.
    // Target should be size FSC_MAX_MODDIR
    char buffer[FSC_MAX_MODDIR];

    // Copy to buffer before calling fs_generate_path, to allow overly long mod names to be truncated
    //   instead of the normal fs_generate_path behavior of generating an empty string on overflow
    Q_strncpyz(buffer, source, sizeof(buffer));
    if (!fs_generate_path(buffer, 0, 0, 0, 0, 0, target, FSC_MAX_MODDIR))
        *target = 0;
}

const char *fs_profile_to_string(FS_Profile profile)
{
    switch (profile)
    {
        case FS_PROFILE_1_1:
            return "1.1";
        case FS_PROFILE_GPP:
            return "GPP";
        case FS_PROFILE_1_3:
            return "1.3";
        case FS_PROFILE_UNSET:
            return "unset";
    }
    return "invalid";
}

/* ******************************************************************************** */
// VM Hash Verification
/* ******************************************************************************** */

qboolean calculate_file_sha256(const fsc_file_t *file, unsigned char *output)
{
    // Returns qtrue on success, qfalse otherwise
    unsigned int size = 0;
    char *data = fs_read_data(file, 0, &size, "calculate_file_sha256");
    if (!data)
    {
        Com_Memset(output, 0, 32);
        return qfalse;
    }
    fsc_calculate_sha256(data, size, output);
    fs_free_data(data);
    return qtrue;
}

qboolean fs_check_trusted_vm_file(const fsc_file_t *file)
{
    // Returns qtrue if file is trusted, qfalse otherwise
    unsigned char sha[32];
    if (!calculate_file_sha256(file, sha))
        return qfalse;
    return fs_check_trusted_vm_hash(sha);
}

void sha256_to_stream(unsigned char *sha, fsc_stream_t *output)
{
    int i;
    char buffer[4];
    for (i = 0; i < 32; ++i)
    {
        Com_sprintf(buffer, sizeof(buffer), "%02x", sha[i]);
        fsc_stream_append_string(output, buffer);
    }
}

/* ******************************************************************************** */
// Core Pak Verification
/* ******************************************************************************** */

// This section is used to verify the core (ID) paks on startup, and produce
// appropriate warnings or errors if they are out of place

#ifndef STANDALONE
static const unsigned int core_hashes[] = {
    1566731103u, 298122907u, 412165236u, 2991495316u, 1197932710u, 4087071573u, 3709064859u, 908855077u, 977125798u};

static const unsigned int missionpack_hashes[] = {2430342401u, 511014160u, 2662638993u, 1438664554u};

static qboolean check_default_cfg_pk3(const char *mod, const char *filename, unsigned int hash)
{
    // Returns qtrue if there is a pk3 containing default.cfg with either the given name or hash
    fsc_hashtable_iterator_t hti;
    fsc_file_t *file;

    fsc_hashtable_open(&fs.files, fsc_string_hash("default", 0), &hti);
    while ((file = (fsc_file_t *)STACKPTRN(fsc_hashtable_next(&hti))))
    {
        const fsc_file_direct_t *source_pk3;
        if (fs_file_disabled(file, FD_CHECK_FILE_ENABLED | FD_CHECK_READ_INACTIVE_MODS))
            continue;
        if (file->sourcetype != FSC_SOURCETYPE_PK3)
            continue;
        if (Q_stricmp((const char *)STACKPTR(file->qp_name_ptr), "default"))
            continue;
        if (!file->qp_ext_ptr || Q_stricmp((const char *)STACKPTR(file->qp_ext_ptr), "cfg"))
            continue;
        if (file->qp_dir_ptr)
            continue;

        source_pk3 = fsc_get_base_file(file, &fs);
        if (source_pk3->pk3_hash == hash)
            return qtrue;
        if (mod && Q_stricmp(fsc_get_mod_dir((const fsc_file_t *)source_pk3, &fs), mod))
            continue;
        if (!Q_stricmp((const char *)STACKPTR(source_pk3->f.qp_name_ptr), filename))
            return qtrue;
    }

    return qfalse;
}

typedef struct {
    const fsc_file_direct_t *name_match;
    const fsc_file_direct_t *hash_match;
} core_pak_state_t;

static core_pak_state_t get_pak_state(const char *mod, const char *filename, unsigned int hash)
{
    // Locates name and hash matches for a given pak
    const fsc_file_direct_t *name_match = 0;
    fsc_hashtable_iterator_t hti;
    fsc_pk3_hash_map_entry_t *entry;
    const fsc_file_direct_t *file;

    fsc_hashtable_open(&fs.files, fsc_string_hash(filename, 0), &hti);
    while ((file = (const fsc_file_direct_t *)STACKPTRN(fsc_hashtable_next(&hti))))
    {
        if (fs_file_disabled((fsc_file_t *)file, FD_CHECK_FILE_ENABLED | FD_CHECK_READ_INACTIVE_MODS))
            continue;
        if (file->f.sourcetype != FSC_SOURCETYPE_DIRECT)
            continue;
        if (Q_stricmp((const char *)STACKPTR(file->f.qp_name_ptr), filename))
            continue;
        if (!file->f.qp_ext_ptr || Q_stricmp((const char *)STACKPTR(file->f.qp_ext_ptr), "pk3"))
            continue;
        if (file->f.qp_dir_ptr)
            continue;
        if (mod && Q_stricmp(fsc_get_mod_dir((fsc_file_t *)file, &fs), mod))
            continue;
        if (file->pk3_hash == hash)
        {
            core_pak_state_t result = {file, file};
            return result;
        }
        name_match = file;
    }

    fsc_hashtable_open(&fs.pk3_hash_lookup, hash, &hti);
    while ((entry = (fsc_pk3_hash_map_entry_t *)STACKPTRN(fsc_hashtable_next(&hti))))
    {
        file = (const fsc_file_direct_t *)STACKPTR(entry->pk3);
        if (fs_file_disabled((fsc_file_t *)file, FD_CHECK_FILE_ENABLED | FD_CHECK_READ_INACTIVE_MODS))
            continue;
        if ((file->pk3_hash == hash))
        {
            core_pak_state_t result = {name_match, file};
            return result;
        }
    }

    {
        core_pak_state_t result = {name_match, 0};
        return result;
    }
}

static void generate_pak_warnings(
    const char *mod, const char *filename, core_pak_state_t *state, fsc_stream_t *warning_popup_stream)
{
    // Prints console warning messages and appends warning popup string for a given pak
    if (state->hash_match)
    {
        if (!state->name_match)
        {
            char hash_match_buffer[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer((fsc_file_t *)state->hash_match, hash_match_buffer, sizeof(hash_match_buffer), qfalse,
                qtrue, qfalse, qfalse);
            Com_Printf("NOTE: %s/%s.pk3 is misnamed, found correct file at %s\n", mod, filename, hash_match_buffer);
        }
        else if (state->name_match != state->hash_match)
        {
            char hash_match_buffer[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer((fsc_file_t *)state->hash_match, hash_match_buffer, sizeof(hash_match_buffer), qfalse,
                qtrue, qfalse, qfalse);
            Com_Printf(
                "WARNING: %s/%s.pk3 has incorrect hash, found correct file at %s\n", mod, filename, hash_match_buffer);
        }
    }
    else
    {
        if (state->name_match)
        {
            Com_Printf("WARNING: %s/%s.pk3 has incorrect hash\n", mod, filename);
            fsc_stream_append_string(warning_popup_stream, va("%s/%s.pk3: incorrect hash\n", mod, filename));
        }
        else
        {
            Com_Printf("WARNING: %s/%s.pk3 not found\n", mod, filename);
            fsc_stream_append_string(warning_popup_stream, va("%s/%s.pk3: not found\n", mod, filename));
        }
    }
}

void fs_check_core_paks(void)
{
    int i;
    core_pak_state_t core_states[ARRAY_LEN(core_hashes)];
    core_pak_state_t missionpack_states[ARRAY_LEN(missionpack_hashes)];
    qboolean missionpack_installed = qfalse;  // Any missionpack paks detected
    char warning_popup_buffer[1024];
    fsc_stream_t warning_popup_stream = {warning_popup_buffer, 0, sizeof(warning_popup_buffer), qfalse};

    // Generate pak states
    for (i = 0; i < ARRAY_LEN(core_hashes); ++i)
    {
        core_states[i] = get_pak_state(BASEGAME, va("pak%i", i), core_hashes[i]);
    }
    for (i = 0; i < ARRAY_LEN(missionpack_hashes); ++i)
    {
        missionpack_states[i] = get_pak_state("missionpack", va("pak%i", i), missionpack_hashes[i]);
        if (missionpack_states[i].name_match || missionpack_states[i].hash_match)
            missionpack_installed = qtrue;
    }

    // Check for standalone mode
    if (Q_stricmp(com_basegame->string, BASEGAME))
    {
        qboolean have_id_pak = qfalse;
        for (i = 0; i < ARRAY_LEN(core_hashes); ++i)
            if (core_states[i].hash_match)
                have_id_pak = qtrue;
        for (i = 0; i < ARRAY_LEN(missionpack_hashes); ++i)
            if (missionpack_states[i].hash_match)
                have_id_pak = qtrue;
        if (!have_id_pak)
        {
            Com_Printf("Enabling standalone mode - no ID paks found\n");
            Cvar_Set("com_standalone", "1");
            return;
        }
    }

    // Print console warning messages and build warning popup string
    for (i = 0; i < ARRAY_LEN(core_hashes); ++i)
    {
        generate_pak_warnings(BASEGAME, va("pak%i", i), &core_states[i], &warning_popup_stream);
    }
    if (missionpack_installed)
        for (i = 0; i < ARRAY_LEN(missionpack_hashes); ++i)
        {
            generate_pak_warnings("missionpack", va("pak%i", i), &missionpack_states[i], &warning_popup_stream);
        }

    // Check for missing default.cfg
    if (!check_default_cfg_pk3(BASEGAME, "pak0", core_hashes[0]))
    {
        if (core_states[0].name_match || core_states[0].hash_match)
            Com_Error(ERR_FATAL, BASEGAME
                "/pak0.pk3 appears to be corrupt (missing default.cfg). Please recopy"
                " this file from your Quake 3 CD or reinstall Quake 3. You may also try"
                " deleting fscache.dat in case it has become corrupt");
        else
            Com_Error(ERR_FATAL, BASEGAME
                "/pak0.pk3 is missing. Please recopy"
                " this file from your Quake 3 CD or reinstall Quake 3");
    }

#ifndef DEDICATED
    // If warning popup info was generated, display warning popup
    if (warning_popup_stream.position)
    {
        dialogResult_t result = Sys_Dialog(DT_OK_CANCEL,
            va("The following game files appear"
               " to be missing or corrupt. You can try to run the game anyway, but you may"
               " experience errors or problems connecting to remote servers.\n\n%s\n"
               "You may need to reinstall Quake 3, the v1.32 patch, and/or team arena.",
                warning_popup_buffer),
            "File Warning");
        if (result == DR_CANCEL)
            Sys_Quit();
    }
#endif
}
#endif

#endif  // NEW_FILESYSTEM
