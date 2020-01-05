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

#ifdef NEW_FILESYSTEM
#include "fscore.h"

#define STACKPTR(pointer) (FSC_STACK_RETRIEVE(&fs->general_stack, pointer, 0))  // non-null
#define STACKPTRN(pointer) (FSC_STACK_RETRIEVE(&fs->general_stack, pointer, 1))  // null allowed

/* ******************************************************************************** */
// Direct Sourcetype Operations
/* ******************************************************************************** */

static int fsc_direct_is_file_active(const fsc_file_t *file, const fsc_filesystem_t *fs)
{
    return ((fsc_file_direct_t *)file)->refresh_count == fs->refresh_count;
}

static const char *fsc_direct_get_mod_dir(const fsc_file_t *file, const fsc_filesystem_t *fs)
{
    return (const char *)STACKPTR(((fsc_file_direct_t *)file)->qp_mod_ptr);
}

static int fsc_direct_extract_data(
    const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh)
{
    void *fp;
    unsigned int result;

    // Open the file
    fp = fsc_open_file(STACKPTR(((fsc_file_direct_t *)file)->os_path_ptr), "rb");
    if (!fp)
    {
        fsc_report_error(eh, FSC_ERROR_EXTRACT, "failed to open file", 0);
        return 1;
    }

    result = fsc_fread(buffer, file->filesize, fp);
    if (result != file->filesize)
    {
        fsc_report_error(eh, FSC_ERROR_EXTRACT, "failed to read all data from file", 0);
        fsc_fclose(fp);
        return 1;
    }

    fsc_fclose(fp);
    return 0;
}

static fsc_sourcetype_t direct_sourcetype = {
    FSC_SOURCETYPE_DIRECT, fsc_direct_is_file_active, fsc_direct_get_mod_dir, fsc_direct_extract_data};

/* ******************************************************************************** */
// Common file operations
/* ******************************************************************************** */

static const fsc_sourcetype_t *fsc_get_sourcetype(const fsc_file_t *file, const fsc_filesystem_t *fs)
{
    int i;

    // Check built in sourcetypes
    if (file->sourcetype == FSC_SOURCETYPE_DIRECT)
        return &direct_sourcetype;
    if (file->sourcetype == FSC_SOURCETYPE_PK3)
        return &pk3_sourcetype;

    // Check custom sourcetypes
    for (i = 0; i < FSC_CUSTOM_SOURCETYPE_COUNT; ++i)
    {
        if (file->sourcetype == fs->custom_sourcetypes[i].sourcetype_id)
            return &fs->custom_sourcetypes[i];
    }

    return 0;
}

const fsc_file_direct_t *fsc_get_base_file(const fsc_file_t *file, const fsc_filesystem_t *fs)
{
    // Returns the source pk3 if file is from a pk3, the file itself if file is on disk,
    // and null if the file is from a custom sourcetype
    FSC_ASSERT(file);
    if (file->sourcetype == FSC_SOURCETYPE_DIRECT)
    {
        return (const fsc_file_direct_t *)file;
    }
    if (file->sourcetype == FSC_SOURCETYPE_PK3)
    {
        return (fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3);
    }
    return 0;
}

int fsc_extract_file(const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh)
{
    // Provided buffer should be size file->filesize
    // Returns 0 on success, 1 on error
    const fsc_sourcetype_t *sourcetype;
    if (file->contents_cache)
    {
        fsc_memcpy(buffer, STACKPTR(file->contents_cache), file->filesize);
        return 0;
    }
    sourcetype = fsc_get_sourcetype(file, fs);
    FSC_ASSERT(sourcetype && sourcetype->extract_data);
    return sourcetype->extract_data(file, buffer, fs, eh);
}

int fsc_is_file_enabled(const fsc_file_t *file, const fsc_filesystem_t *fs)
{
    // Returns 1 if file enabled, 0 otherwise
    const fsc_sourcetype_t *sourcetype = fsc_get_sourcetype(file, fs);
    FSC_ASSERT(sourcetype && sourcetype->is_file_active);
    return sourcetype->is_file_active(file, fs);
}

const char *fsc_get_mod_dir(const fsc_file_t *file, const fsc_filesystem_t *fs)
{
    // Should not return null; may return empty string to represent omitted/invalid mod directory
    const fsc_sourcetype_t *sourcetype = fsc_get_sourcetype(file, fs);
    const char *mod_directory;
    FSC_ASSERT(sourcetype && sourcetype->get_mod_dir);
    mod_directory = sourcetype->get_mod_dir(file, fs);
    FSC_ASSERT(mod_directory);
    return mod_directory;
}

char *fsc_extract_file_allocated(fsc_filesystem_t *fs, fsc_file_t *file, fsc_errorhandler_t *eh)
{
    // Returns data on success, 0 on failure
    // Caller is responsible for calling fsc_free on extracted data
    char *data;
    if (file->filesize + 1 < file->filesize)
        return 0;
    data = (char *)fsc_malloc(file->filesize + 1);
    if (fsc_extract_file(file, data, fs, eh))
    {
        fsc_free(data);
        return 0;
    }
    data[file->filesize] = 0;
    return data;
}

/* ******************************************************************************** */
// File to printable string functions
/* ******************************************************************************** */

#define FSC_ADD_STRING(string) fsc_stream_append_string(stream, string)

void fsc_file_to_stream(
    const fsc_file_t *file, fsc_stream_t *stream, const fsc_filesystem_t *fs, int include_mod, int include_pk3_origin)
{
    if (include_mod)
    {
        const char *mod_dir = fsc_get_mod_dir(file, fs);
        if (!*mod_dir)
            mod_dir = "<no-mod-dir>";
        FSC_ADD_STRING(mod_dir);
        FSC_ADD_STRING("/");
    }

    if (include_pk3_origin)
    {
        if (file->sourcetype == FSC_SOURCETYPE_DIRECT && ((fsc_file_direct_t *)file)->pk3dir_ptr)
        {
            FSC_ADD_STRING((const char *)STACKPTR(((fsc_file_direct_t *)file)->pk3dir_ptr));
            FSC_ADD_STRING(".pk3dir->");
        }
        else if (file->sourcetype == FSC_SOURCETYPE_PK3)
        {
            fsc_file_to_stream((const fsc_file_t *)fsc_get_base_file(file, fs), stream, fs, 0, 0);
            FSC_ADD_STRING("->");
        }
    }

    if (file->qp_dir_ptr)
    {
        FSC_ADD_STRING((const char *)STACKPTR(file->qp_dir_ptr));
        FSC_ADD_STRING("/");
    }
    FSC_ADD_STRING((const char *)STACKPTR(file->qp_name_ptr));
    if (file->qp_ext_ptr)
    {
        FSC_ADD_STRING(".");
        FSC_ADD_STRING((const char *)STACKPTR(file->qp_ext_ptr));
    }
}

/* ******************************************************************************** */
// File Indexing
/* ******************************************************************************** */

static void fsc_merge_stats(const fsc_stats_t *source, fsc_stats_t *target)
{
    target->valid_pk3_count += source->valid_pk3_count;
    target->pk3_subfile_count += source->pk3_subfile_count;
    target->shader_file_count += source->shader_file_count;
    target->shader_count += source->shader_count;
    target->total_file_count += source->total_file_count;
    target->cacheable_file_count += source->cacheable_file_count;
}

void fsc_register_file(fsc_stackptr_t file_ptr, fsc_filesystem_t *fs, fsc_errorhandler_t *eh)
{
    // Registers file in index and loads secondary content such as shaders
    // Called for both files on disk and in pk3s
    fsc_file_t *file = (fsc_file_t *)STACKPTR(file_ptr);
    fsc_file_direct_t *base_file = (fsc_file_direct_t *)fsc_get_base_file(file, fs);
    const char *qp_dir = (const char *)STACKPTRN(file->qp_dir_ptr);
    const char *qp_name = (const char *)STACKPTR(file->qp_name_ptr);
    const char *qp_ext = (const char *)STACKPTRN(file->qp_ext_ptr);

    // Register file for main lookup and directory iteration
    fsc_hashtable_insert(file_ptr, fsc_string_hash(qp_name, qp_dir), &fs->files);
    fsc_iteration_register_file(file_ptr, &fs->directories, &fs->string_repository, &fs->general_stack);

    // Index shaders and update shader counter on base file
    if (qp_ext && qp_dir && !fsc_stricmp(qp_dir, "scripts") && !fsc_stricmp(qp_ext, "shader"))
    {
        int count = index_shader_file(fs, file_ptr, eh);
        if (base_file)
        {
            base_file->shader_file_count += 1;
            base_file->shader_count += count;
            base_file->f.flags |= FSC_FILEFLAG_LINKED_CONTENT;
        }
    }

    // Index crosshairs
    if (qp_dir && !fsc_stricmp(qp_dir, "gfx/2d"))
    {
        char buffer[10];
        fsc_strncpy(buffer, qp_name, sizeof(buffer));
        if (!fsc_stricmp(buffer, "crosshair"))
        {
            index_crosshair(fs, file_ptr, eh);
            if (base_file)
                base_file->f.flags |= FSC_FILEFLAG_LINKED_CONTENT;
        }
    }

    // Cache small arena and bot file contents
    if (file->filesize < 16384 && qp_dir && !fsc_stricmp(qp_dir, "scripts") && qp_ext &&
        (!fsc_stricmp(qp_ext, "arena") || !fsc_stricmp(qp_ext, "bot")))
    {
        char *source_data = fsc_extract_file_allocated(fs, file, eh);
        if (source_data)
        {
            fsc_stackptr_t target_ptr = fsc_stack_allocate(&fs->general_stack, file->filesize);
            char *target_data = (char *)STACKPTR(target_ptr);
            fsc_memcpy(target_data, source_data, file->filesize);
            file->contents_cache = target_ptr;
            fsc_free(source_data);
        }
    }
}

static int fsc_nstring_compare(const char *s1, const char *s2)
{
    // Compares two potentially null strings, returns 1 if matching, 0 otherwise
    if (!s1 && !s2)
        return 1;
    if (!s1 || !s2)
        return 0;
    return !fsc_strcmp(s1, s2);
}

void fsc_load_file(int source_dir_id, const void *os_path, const char *mod_dir, const char *pk3dir_name,
    const char *qp_dir, const char *qp_name, const char *qp_ext, unsigned int os_timestamp, unsigned int filesize,
    fsc_filesystem_t *fs, fsc_errorhandler_t *eh)
{
    fsc_stackptr_t file_ptr;
    fsc_file_direct_t *file;
    fsc_hashtable_iterator_t hti;
    unsigned int fs_hash = fsc_string_hash(qp_name, qp_dir);
    int unindexed_file = 0;  // File was not present in the index at all
    int new_file = 0;  // File was not present in last refresh, but may have been in the index

    FSC_ASSERT(os_path);
    FSC_ASSERT(qp_name);
    FSC_ASSERT(fs);

    // Search filesystem to see if a sufficiently equivalent entry already exists
    fsc_hashtable_open(&fs->files, fs_hash, &hti);
    while ((file_ptr = fsc_hashtable_next(&hti)))
    {
        file = (fsc_file_direct_t *)STACKPTR(file_ptr);
        if (file->f.sourcetype != FSC_SOURCETYPE_DIRECT)
            continue;
        if (!fsc_nstring_compare((char *)STACKPTR(file->f.qp_name_ptr), qp_name))
            continue;
        if (!fsc_nstring_compare((char *)STACKPTRN(file->f.qp_dir_ptr), qp_dir))
            continue;
        if (!fsc_nstring_compare((char *)STACKPTRN(file->f.qp_ext_ptr), qp_ext))
            continue;
        if (!fsc_nstring_compare((char *)STACKPTRN(file->qp_mod_ptr), mod_dir))
            continue;
        if (!fsc_nstring_compare((char *)STACKPTRN(file->pk3dir_ptr), pk3dir_name))
            continue;
        if (file->os_path_ptr && fsc_compare_os_path(STACKPTR(file->os_path_ptr), os_path))
            continue;
        if (file->f.filesize != filesize || file->os_timestamp != os_timestamp)
        {
            if (file->os_path_ptr && !(file->f.flags & FSC_FILEFLAG_LINKED_CONTENT) && !file->f.contents_cache)
            {
                // Reuse the same file object to save memory (this prevents files actively written
                // by the game such as logs generating a new file object every refresh)
                file->f.filesize = filesize;
                file->os_timestamp = os_timestamp;
                break;
            }
            else
            {
                // Otherwise treat the file as non-matching
                continue;
            }
        }
        break;
    }

    if (file_ptr)
    {
        // Have existing entry
        if (file->refresh_count == fs->refresh_count)
        {
            // Existing file already active. This can happen with if there are duplicate source directories
            // loaded in the same refresh cycle. Just leave the existing file unchanged.
            return;
        }

        // Activate the entry
        if (file->refresh_count != fs->refresh_count - 1)
            new_file = 1;
        file->refresh_count = fs->refresh_count;
    }

    else
    {
        // Create a new entry
        file_ptr = fsc_stack_allocate(&fs->general_stack, sizeof(*file));
        file = (fsc_file_direct_t *)STACKPTR(file_ptr);

        // Set up fields (other fields are zeroed by default due to stack allocation)
        file->f.sourcetype = FSC_SOURCETYPE_DIRECT;
        file->f.qp_dir_ptr =
            qp_dir ? fsc_string_repository_getstring(qp_dir, 1, &fs->string_repository, &fs->general_stack) : 0;
        file->f.qp_name_ptr = fsc_string_repository_getstring(qp_name, 1, &fs->string_repository, &fs->general_stack);
        file->f.qp_ext_ptr =
            qp_ext ? fsc_string_repository_getstring(qp_ext, 1, &fs->string_repository, &fs->general_stack) : 0;
        file->qp_mod_ptr =
            mod_dir ? fsc_string_repository_getstring(mod_dir, 1, &fs->string_repository, &fs->general_stack) : 0;
        file->pk3dir_ptr =
            pk3dir_name ? fsc_string_repository_getstring(pk3dir_name, 1, &fs->string_repository, &fs->general_stack)
                        : 0;
        file->f.filesize = filesize;
        file->os_timestamp = os_timestamp;
        file->refresh_count = fs->refresh_count;

        unindexed_file = new_file = 1;
    }

    // Update source dir and download folder flag
    file->source_dir_id = source_dir_id;
    file->f.flags &= ~FSC_FILEFLAG_DLPK3;
    if (qp_ext && !fsc_stricmp(qp_ext, "pk3") && qp_dir && !fsc_stricmp(qp_dir, "downloads"))
    {
        file->f.flags |= FSC_FILEFLAG_DLPK3;
    }

    // Save os path. This happens on loading a new file, and also when first activating an entry that was loaded from
    // cache.
    if (!file->os_path_ptr)
    {
        int os_path_size = fsc_os_path_size(os_path);
        file->os_path_ptr = fsc_stack_allocate(&fs->general_stack, os_path_size);
        fsc_memcpy(STACKPTR(file->os_path_ptr), os_path, os_path_size);
    }

    // Register file and load contents
    if (unindexed_file)
    {
        fsc_register_file(file_ptr, fs, eh);
        if (qp_ext && !fsc_stricmp(qp_ext, "pk3") && (!qp_dir || !fsc_stricmp(qp_dir, "downloads")))
        {
            fsc_load_pk3(STACKPTR(file->os_path_ptr), fs, file_ptr, eh, 0, 0);
            file->f.flags |= FSC_FILEFLAG_LINKED_CONTENT;
        }
    }

    // Update stats
    {
        fsc_stats_t stats;
        fsc_memset(&stats, 0, sizeof(stats));

        stats.total_file_count = 1 + file->pk3_subfile_count;

        stats.cacheable_file_count = file->pk3_subfile_count;
        if (file->shader_count || file->pk3_subfile_count)
            ++stats.cacheable_file_count;

        stats.pk3_subfile_count = file->pk3_subfile_count;

        // By design, this field records only *valid* pk3s with a nonzero hash.
        // Perhaps create another field that includes invalid pk3s?
        if (file->pk3_hash)
            stats.valid_pk3_count = 1;

        stats.shader_file_count = file->shader_file_count;
        stats.shader_count = file->shader_count;

        fsc_merge_stats(&stats, &fs->active_stats);
        if (unindexed_file)
            fsc_merge_stats(&stats, &fs->total_stats);
        if (new_file)
            fsc_merge_stats(&stats, &fs->new_stats);
    }
}

static int fsc_app_extension(const char *name)
{
    // Returns 1 if name matches mac app bundle extension, 0 otherwise
    int length = fsc_strlen(name);
    if (length < 4)
        return 0;
    if (!fsc_stricmp(name + (length - 4), ".app"))
        return 1;
    return 0;
}

void fsc_load_file_full_path(int source_dir_id, const void *os_path, const char *full_qpath, unsigned int os_timestamp,
    unsigned int filesize, fsc_filesystem_t *fs, fsc_errorhandler_t *eh)
{
    char qp_buffer[FSC_MAX_QPATH];
    char qp_mod[FSC_MAX_MODDIR];
    const char *qpath_start = 0;
    int file_in_pk3dir = 0;
    char pk3dir_buffer[FSC_MAX_QPATH];
    const char *pk3dir_remainder = 0;
    const char *qp_dir, *qp_name, *qp_ext;

    // Process mod directory prefix
    if (!fsc_get_leading_directory(full_qpath, qp_mod, sizeof(qp_mod), &qpath_start))
        return;
    if (!qpath_start)
        return;
    if (fsc_app_extension(qp_mod))
        return;  // Don't index mac app bundles as mods

    // Process pk3dir prefix
    if (fsc_get_leading_directory(qpath_start, pk3dir_buffer, sizeof(pk3dir_buffer), &pk3dir_remainder))
    {
        if (pk3dir_remainder)
        {
            int length = fsc_strlen(pk3dir_buffer);
            if (length >= 7 && !fsc_stricmp(pk3dir_buffer + length - 7, ".pk3dir"))
            {
                pk3dir_buffer[length - 7] = 0;
                file_in_pk3dir = 1;
                qpath_start = pk3dir_remainder;
            }
        }
    }

    // Process qpath
    if (!fsc_process_qpath(qpath_start, qp_buffer, &qp_dir, &qp_name, &qp_ext))
        return;

    // Load file
    fsc_load_file(source_dir_id, os_path, qp_mod, file_in_pk3dir ? pk3dir_buffer : 0, qp_dir, qp_name, qp_ext,
        os_timestamp, filesize, fs, eh);
}

typedef struct {
    int source_dir_id;
    fsc_filesystem_t *fs;
    fsc_errorhandler_t *eh;
} iterate_context_t;

static void load_file_from_iteration(iterate_data_t *file_data, void *iterate_context)
{
    iterate_context_t *iterate_context_typed = (iterate_context_t *)iterate_context;
    fsc_load_file_full_path(iterate_context_typed->source_dir_id, file_data->os_path, file_data->qpath_with_mod_dir,
        file_data->os_timestamp, file_data->filesize, iterate_context_typed->fs, iterate_context_typed->eh);
}

void fsc_filesystem_initialize(fsc_filesystem_t *fs)
{
    fsc_memset(fs, 0, sizeof(*fs));
    fsc_stack_initialize(&fs->general_stack);
    fsc_hashtable_initialize(&fs->files, &fs->general_stack, 65536);
    fsc_hashtable_initialize(&fs->string_repository, &fs->general_stack, 65536);
    fsc_hashtable_initialize(&fs->directories, &fs->general_stack, 16384);
    fsc_hashtable_initialize(&fs->shaders, &fs->general_stack, 65536);
    fsc_hashtable_initialize(&fs->crosshairs, &fs->general_stack, 1);
    fsc_hashtable_initialize(&fs->pk3_hash_lookup, &fs->general_stack, 4096);
}

void fsc_filesystem_free(fsc_filesystem_t *fs)
{
    // Can be called on a nulled, freed, initialized, or in some cases partially initialized filesystem
    fsc_stack_free(&fs->general_stack);
    fsc_hashtable_free(&fs->files);
    fsc_hashtable_free(&fs->string_repository);
    fsc_hashtable_free(&fs->directories);
    fsc_hashtable_free(&fs->shaders);
    fsc_hashtable_free(&fs->crosshairs);
    fsc_hashtable_free(&fs->pk3_hash_lookup);
}

void fsc_filesystem_reset(fsc_filesystem_t *fs)
{
    ++fs->refresh_count;
    fsc_memset(&fs->active_stats, 0, sizeof(fs->active_stats));
    fsc_memset(&fs->new_stats, 0, sizeof(fs->new_stats));
}

void fsc_load_directory(fsc_filesystem_t *fs, void *os_path, int source_dir_id, fsc_errorhandler_t *eh)
{
    iterate_context_t context;
    context.source_dir_id = source_dir_id;
    context.fs = fs;
    context.eh = eh;
    iterate_directory(os_path, load_file_from_iteration, &context);
}

#endif  // NEW_FILESYSTEM
