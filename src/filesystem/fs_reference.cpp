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

#define MAX_REFERENCE_SET_ENTRIES 2048
#define MAX_PURE_CHECKSUM_CACHE 256

/* ******************************************************************************** */
// Referenced Pak Tracking
/* ******************************************************************************** */

// The "reference_tracker" set is filled by logging game references to pk3 files
// It currently serves two purposes:
// 1) To generate the pure validation string when fs_full_pure_validation is 1
//		(Although I have never seen any server that requires this to connect)
// 2) As a component of the download pak list creation
//		(Although in most cases it is redundant to other selector rules)
// Basically this section could probably be removed with no noticable effects in most
// situations, but is kept for now just to be on the safe side for compatibility purposes

typedef struct {
    fs_hashtable_entry_t hte;
    fsc_file_direct_t *pak;
} reference_tracker_entry_t;

static fs_hashtable_t reference_tracker;

static qboolean reference_tracker_add(fsc_file_direct_t *pak)
{
    // Returns qtrue on success, qfalse if already inserted or maximum hit
    fs_hashtable_iterator_t it = fs_hashtable_iterate(&reference_tracker, pak->pk3_hash, qfalse);
    reference_tracker_entry_t *entry;

    if (reference_tracker.element_count >= MAX_REFERENCE_SET_ENTRIES)
        return qfalse;

    while ((entry = (reference_tracker_entry_t *)fs_hashtable_next(&it)))
    {
        if (entry->pak->pk3_hash == pak->pk3_hash)
        {
            return qfalse;
        }
    }

    entry = (reference_tracker_entry_t *)S_Malloc(sizeof(reference_tracker_entry_t));
    entry->pak = pak;
    fs_hashtable_insert(&reference_tracker, (fs_hashtable_entry_t *)entry, pak->pk3_hash);
    return qtrue;
}

void fs_register_reference(const fsc_file_t *file)
{
    // Adds the source pk3 of the given file to the current referenced paks set
    if (file->sourcetype != FSC_SOURCETYPE_PK3)
        return;

    // Don't register references for certain extensions
    if (file->qp_ext_ptr)
    {
        int i;
        static const char *special_extensions[] = {"shader", "txt", "cfg", "config", "bot", "arena", "menu"};
        if (file->qp_ext_ptr)
        {
            const char *extension = (const char *)STACKPTR(file->qp_ext_ptr);
            for (i = 0; i < ARRAY_LEN(special_extensions); ++i)
            {
                if (!Q_stricmp(extension, special_extensions[i]))
                    return;
            }
        }
    }

    // Don't register reference in certain special cases
    if (!Q_stricmp((const char *)STACKPTR(file->qp_name_ptr), "qagame") && file->qp_ext_ptr &&
        !Q_stricmp((const char *)STACKPTR(file->qp_ext_ptr), "qvm") && file->qp_dir_ptr &&
        !Q_stricmp((const char *)STACKPTR(file->qp_dir_ptr), "vm"))
        return;
    if (file->qp_dir_ptr && !Q_stricmp((const char *)STACKPTR(file->qp_dir_ptr), "levelshots"))
        return;

    // Initialize reference_tracker if it isn't already
    if (!reference_tracker.bucket_count)
    {
        fs_hashtable_initialize(&reference_tracker, 32);
    }

    // Add the reference
    if (reference_tracker_add((fsc_file_direct_t *)fsc_get_base_file(file, &fs)))
    {
        if (fs_debug_references->integer)
        {
            char temp[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer(file, temp, sizeof(temp), qtrue, qtrue, qtrue, qfalse);
            Com_Printf("recording reference: %s\n", temp);
        }
    }
}

void FS_ClearPakReferences(int flags)
{
    if (fs_debug_references->integer)
        Com_Printf("clearing referenced paks\n");
    fs_hashtable_reset(&reference_tracker, 0);
}

static void reftracker_gen_sort_key(const fsc_file_t *file, fsc_stream_t *output)
{
    fs_generate_core_sort_key(file, output, qtrue);
    fs_write_sort_filename(file, output);
    fs_write_sort_value(fs_get_source_dir_id(file), output);
}

static int reftracker_compare_file(const fsc_file_t *file1, const fsc_file_t *file2)
{
    char buffer1[1024];
    char buffer2[1024];
    fsc_stream_t stream1 = {buffer1, 0, sizeof(buffer1), qfalse};
    fsc_stream_t stream2 = {buffer2, 0, sizeof(buffer2), qfalse};
    reftracker_gen_sort_key(file1, &stream1);
    reftracker_gen_sort_key(file2, &stream2);
    return fsc_memcmp(
        stream2.data, stream1.data, stream1.position < stream2.position ? stream1.position : stream2.position);
}

static int reftracker_qsort(const void *e1, const void *e2)
{
    return reftracker_compare_file(*(const fsc_file_t **)e1, *(const fsc_file_t **)e2);
}

static fsc_file_direct_t **generate_referenced_pak_list(int *count_out)
{
    // Result must be freed by Z_Free
    // Result will not be valid if reference_tracker is reset
    int count = 0;
    fs_hashtable_iterator_t it = fs_hashtable_iterate(&reference_tracker, 0, qtrue);
    const reference_tracker_entry_t *entry;
    fsc_file_direct_t **reference_list =
        (fsc_file_direct_t **)Z_Malloc(sizeof(*reference_list) * reference_tracker.element_count);

    // Generate reference list
    while ((entry = (reference_tracker_entry_t *)fs_hashtable_next(&it)))
    {
        if (count >= reference_tracker.element_count)
            Com_Error(ERR_FATAL, "generate_referenced_pak_list list overflow");
        reference_list[count] = entry->pak;
        ++count;
    }
    if (count != reference_tracker.element_count)
        Com_Error(ERR_FATAL, "generate_referenced_pak_list list underflow");

    // Sort reference list
    qsort(reference_list, count, sizeof(*reference_list), reftracker_qsort);

    if (count_out)
        *count_out = count;
    return reference_list;
}

const char *FS_ReferencedPakNames(void)
{
    // This is just used for a certain debug command now
    static char buffer[1000];
    fsc_stream_t stream = {buffer, 0, sizeof(buffer), 0};
    int i;
    int count = 0;
    fsc_file_direct_t **reference_list = generate_referenced_pak_list(&count);

    *buffer = 0;
    for (i = 0; i < count; ++i)
    {
        if (stream.position)
            fsc_stream_append_string(&stream, " ");
        fs_file_to_stream((const fsc_file_t *)reference_list[i], &stream, qfalse, qfalse, qfalse, qfalse);
    }

    Z_Free(reference_list);
    return buffer;
}

/* ******************************************************************************** */
// Pure Validation
/* ******************************************************************************** */

// This section is used to generate a pure validation string to pass the SV_VerifyPaks_f
// check when connecting to legacy pure servers.

typedef struct {
    const fsc_file_direct_t *pk3;
    int data_size;
    int pure_checksum;
    int checksum_feed;
} pure_checksum_entry_t;

static void get_pure_checksum_entry_callback(void *context, char *data, int size)
{
    pure_checksum_entry_t **entry = (pure_checksum_entry_t **)context;
    *entry = (pure_checksum_entry_t *)fsc_malloc(sizeof(pure_checksum_entry_t) + size);
    (*entry)->data_size = size;
    Com_Memcpy((char *)&(*entry)->checksum_feed + 4, data, size);
}

static pure_checksum_entry_t *get_pure_checksum_entry(const fsc_file_direct_t *pk3)
{
    pure_checksum_entry_t *entry = 0;
    fsc_load_pk3(STACKPTR(pk3->os_path_ptr), &fs, 0, 0, get_pure_checksum_entry_callback, &entry);
    if (entry)
        entry->pk3 = pk3;
    return entry;
}

static void update_pure_checksum_entry(pure_checksum_entry_t *entry, int checksum_feed)
{
    entry->checksum_feed = LittleLong(checksum_feed);
    entry->pure_checksum = fsc_block_checksum(&entry->checksum_feed, entry->data_size + 4);
}

// Pure checksum cache

typedef struct pure_checksum_node_s {
    struct pure_checksum_node_s *next;
    pure_checksum_entry_t *entry;
    int rank;
} pure_checksum_node_t;

static int pure_checksum_rank = 0;
static pure_checksum_node_t *pure_checksum_cache;

static int get_pure_checksum_for_pk3(const fsc_file_direct_t *pk3, int checksum_feed)
{
    int entry_count = 0;
    pure_checksum_node_t *node = pure_checksum_cache;
    pure_checksum_node_t *deletion_node = 0;

    while (node)
    {
        if (node->entry && node->entry->pk3 == pk3)
        {
            // Use existing entry
            if (node->entry->checksum_feed != checksum_feed)
            {
                update_pure_checksum_entry(node->entry, checksum_feed);
            }
            node->rank = ++pure_checksum_rank;
            return node->entry->pure_checksum;
        }

        if (!deletion_node || node->rank < deletion_node->rank)
            deletion_node = node;

        ++entry_count;
        node = node->next;
    }

    // Prepare new node
    if (entry_count >= MAX_PURE_CHECKSUM_CACHE)
    {
        node = deletion_node;
        if (node->entry)
            fsc_free(node->entry);
    }
    else
    {
        node = (pure_checksum_node_t *)S_Malloc(sizeof(*node));
        node->next = pure_checksum_cache;
        pure_checksum_cache = node;
    }

    // Create new entry
    node->entry = get_pure_checksum_entry(pk3);
    if (!node->entry)
        return 0;
    update_pure_checksum_entry(node->entry, checksum_feed);
    node->rank = ++pure_checksum_rank;
    return node->entry->pure_checksum;
}

static int get_pure_checksum_for_file(const fsc_file_t *file, int checksum_feed)
{
    if (!file)
        return 0;
    if (file->sourcetype != FSC_SOURCETYPE_PK3)
        return 0;
    return get_pure_checksum_for_pk3(fsc_get_base_file(file, &fs), checksum_feed);
}

static void add_referenced_pure_pk3s(fsc_stream_t *stream, fs_hashtable_t *reference_tracker)
{
    int i;
    int count = 0;
    fsc_file_direct_t **reference_list = generate_referenced_pak_list(&count);
    char buffer[20];
    int lump_checksum = 0;

    // Process entries
    for (i = 0; i < count; ++i)
    {
        int pure_checksum = get_pure_checksum_for_pk3(reference_list[i], checksum_feed);

        if (fs_debug_references->integer)
        {
            char temp[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer((fsc_file_t *)reference_list[i], temp, sizeof(temp), qtrue, qtrue, qtrue, qfalse);
            Com_Printf("adding pak to pure validation list: %s\n", temp);
        }

        lump_checksum ^= pure_checksum;
        Com_sprintf(buffer, sizeof(buffer), " %i", pure_checksum);
        fsc_stream_append_string(stream, buffer);
    }

    // Write final checksum
    Com_sprintf(buffer, sizeof(buffer), " %i ", checksum_feed ^ lump_checksum ^ count);
    fsc_stream_append_string(stream, buffer);

    Z_Free(reference_list);
}

static void build_pure_validation_string(char *output, unsigned int output_size, fs_hashtable_t *reference_tracker)
{
    fsc_stream_t stream = {output, 0, output_size, 0};
    char buffer[50];
    int cgame_checksum = get_pure_checksum_for_file(
        fs_general_lookup("vm/cgame.qvm", LOOKUPFLAG_IGNORE_CURRENT_MAP, qfalse), checksum_feed);
    int ui_checksum = get_pure_checksum_for_file(
        fs_general_lookup("vm/ui.qvm", LOOKUPFLAG_IGNORE_CURRENT_MAP, qfalse), checksum_feed);

    Com_sprintf(buffer, sizeof(buffer), "%i %i @", cgame_checksum, ui_checksum);
    fsc_stream_append_string(&stream, buffer);

    if (fs_full_pure_validation->integer && connected_server_sv_pure != 2)
    {
        add_referenced_pure_pk3s(&stream, reference_tracker);
    }
    else
    {
        Com_sprintf(buffer, sizeof(buffer), " %i %i ", cgame_checksum, checksum_feed ^ cgame_checksum ^ 1);
        fsc_stream_append_string(&stream, buffer);
    }
}

const char *FS_ReferencedPakPureChecksums(void)
{
    // Returns a space separated string containing the pure checksums of all referenced pk3 files.
    // Servers with sv_pure set will get this string back from clients for pure validation
    // The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
    static char buffer[1000];
    build_pure_validation_string(buffer, sizeof(buffer), &reference_tracker);
    return buffer;
}

/* ******************************************************************************** */
// Manifest processing
/* ******************************************************************************** */

// This section is used to convert a download/pure manifest into a reference set,
// which can be converted into download and pure lists.

typedef fs_hashtable_t reference_set_t;

typedef struct {
    fs_hashtable_entry_t hte;

    // Primary characteristics
    char mod_dir[FSC_MAX_MODDIR];
    char name[FSC_MAX_QPATH];
    unsigned int hash;
    const fsc_file_direct_t *pak_file;  // Optional (if null, represents hash-only entry)
    unsigned int pak_file_name_match;  // For sorting purposes

    // Command characteristics
    char command_name[64];  // Name of the selector command that created this entry, for debug prints
    unsigned int cluster;  // Indicates dash separated cluster (lower value is higher priority)

    // For debug print purposes
    int entry_id;

    // Sorting
    char sort_key[FSC_MAX_MODDIR + FSC_MAX_QPATH + 32];
    unsigned int sort_key_length;
} reference_set_entry_t;

typedef struct {
    // General state
    reference_set_t *reference_set;
    pk3_list_t exclude_set;
    int cluster;

    // For debug prints
    int entry_id_counter;

    // Current command
    char command_name[64];
    qboolean exclude_mode;
} reference_set_work_t;

static void generate_reference_set_entry(reference_set_work_t *rsw, const char *mod_dir, const char *name,
    unsigned int hash, const fsc_file_direct_t *pak_file, reference_set_entry_t *target)
{
    // pak can be null; other parameters are required
    fsc_stream_t sort_stream = {target->sort_key, 0, sizeof(target->sort_key), 0};

    Com_Memset(target, 0, sizeof(*target));
    Q_strncpyz(target->mod_dir, mod_dir, sizeof(target->mod_dir));
    Q_strncpyz(target->name, name, sizeof(target->name));
    target->hash = hash;
    target->pak_file = pak_file;
    target->cluster = rsw->cluster;
    target->entry_id = rsw->entry_id_counter++;

    // Write command name debug string
    Q_strncpyz(target->command_name, rsw->command_name, sizeof(target->command_name));
    if (strlen(rsw->command_name) >= sizeof(target->command_name))
    {
        strcpy(target->command_name + sizeof(target->command_name) - 4, "...");
    }

    // Determine pak_file_name_match, which is added to the sort key to handle special cases
    //   e.g. if a pk3 is specified in the download manifest with a specific hash, and multiple pk3s exist
    //   in the filesystem with that hash, this sort value attempts to prioritize the physical pk3 closer to
    //   the user-specified name to be used as the physical download source file
    // 0 = no pak, 1 = no name match, 2 = case insensitive match, 3 = case sensitive match
    if (pak_file)
    {
        const char *pak_mod = (const char *)STACKPTR(pak_file->qp_mod_ptr);
        const char *pak_name = (const char *)STACKPTR(pak_file->f.qp_name_ptr);
        if (!strcmp(mod_dir, pak_mod) && !strcmp(name, pak_name))
            target->pak_file_name_match = 3;
        else if (!Q_stricmp(mod_dir, pak_mod) && !Q_stricmp(name, pak_name))
            target->pak_file_name_match = 2;
        else
            target->pak_file_name_match = 1;
    }

    // Write sort key
    {
        FS_ModType mod_type = fs_get_mod_type(target->mod_dir);
        unsigned int core_pak_priority =
            mod_type < MODTYPE_OVERRIDE_DIRECTORY ? (unsigned int)core_pk3_position(hash) : 0;

        fs_write_sort_value(~target->cluster, &sort_stream);
        fs_write_sort_value(mod_type >= MODTYPE_OVERRIDE_DIRECTORY ? (unsigned int)mod_type : 0, &sort_stream);
        fs_write_sort_value(core_pak_priority, &sort_stream);
        fs_write_sort_value((unsigned int)mod_type, &sort_stream);
        fs_write_sort_string(target->mod_dir, &sort_stream, qfalse);
        fs_write_sort_string(target->name, &sort_stream, qfalse);
        fs_write_sort_value(target->pak_file_name_match, &sort_stream);
        target->sort_key_length = sort_stream.position;
    }
}

static int compare_reference_set_entry(const reference_set_entry_t *e1, const reference_set_entry_t *e2)
{
    // Returns < 0 if e1 is higher precedence, > 0 if e2 is higher precedence
    return fsc_memcmp(e1->sort_key, e2->sort_key,
        e1->sort_key_length < e2->sort_key_length ? e1->sort_key_length : e2->sort_key_length);
}

static void reference_set_insert_entry(
    reference_set_work_t *rsw, const char *mod_dir, const char *name, unsigned int hash, const fsc_file_direct_t *pak)
{
    // Inserts or updates reference entry into output reference set
    reference_set_entry_t new_entry;
    fs_hashtable_iterator_t it = fs_hashtable_iterate(rsw->reference_set, hash, qfalse);
    reference_set_entry_t *target_entry = 0;

    // Generate new entry
    generate_reference_set_entry(rsw, mod_dir, name, hash, pak, &new_entry);

    // Print entry contents
    if (fs_debug_references->integer)
    {
        Com_Printf("********** reference set entry **********\n");
        Com_Printf("entry id: %i\n", new_entry.entry_id);
        Com_Printf("source rule: %s\n", new_entry.command_name);
        Com_Printf("path: %s/%s\n", new_entry.mod_dir, new_entry.name);
        Com_Printf("hash: %i\n", (int)new_entry.hash);
        if (new_entry.pak_file)
        {
            char buffer[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer(
                (const fsc_file_t *)new_entry.pak_file, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
            Com_Printf("pak file: %s\n", buffer);
            Com_Printf("pak file name match: %u\n", new_entry.pak_file_name_match);
        }
        else
        {
            Com_Printf("pak file: <none>\n");
        }
        Com_Printf("cluster: %i\n", new_entry.cluster);
    }

    // Process exclude command
    if (rsw->exclude_mode)
    {
        if (pk3_list_lookup(&rsw->exclude_set, hash))
        {
            if (fs_debug_references->integer)
                Com_Printf("result: Hash already in exclude set\n");
        }
        else
        {
            if (fs_debug_references->integer)
                Com_Printf("result: Hash added to exclude set\n");
            pk3_list_insert(&rsw->exclude_set, hash);
        }
        return;
    }

    // Check if hash is excluded
    if (pk3_list_lookup(&rsw->exclude_set, hash))
    {
        if (fs_debug_references->integer)
            Com_Printf("result: Skipping entry due to hash in exclude set\n");
        return;
    }

    // Look for existing entry with same hash
    while ((target_entry = (reference_set_entry_t *)fs_hashtable_next(&it)))
    {
        if (new_entry.hash == target_entry->hash)
        {
            // Found entry; check if new entry is higher priority
            int compare_result = compare_reference_set_entry(&new_entry, target_entry);
            if (fs_debug_references->integer)
            {
                if (compare_result >= 0)
                    Com_Printf("result: Duplicate hash - skipping entry due to existing %s precedence entry id %i\n",
                        compare_result > 0 ? "higher" : "equal", target_entry->entry_id);
                else
                    Com_Printf("result: Duplicate hash - overwriting existing lower precedence entry id %i\n",
                        target_entry->entry_id);
            }
            if (compare_result < 0)
                *target_entry = new_entry;
            return;
        }
    }

    // Check for excess element count
    if (rsw->reference_set->element_count >= MAX_REFERENCE_SET_ENTRIES)
    {
        if (fs_debug_references->integer)
            Com_Printf("result: Skipping entry due to MAX_REFERENCE_SET_ENTRIES hit\n");
        return;
    }

    // Create new entry
    if (fs_debug_references->integer)
        Com_Printf("result: Added entry to reference set\n");
    target_entry = (reference_set_entry_t *)Z_Malloc(sizeof(*target_entry));
    *target_entry = new_entry;
    fs_hashtable_insert(rsw->reference_set, (fs_hashtable_entry_t *)target_entry, target_entry->hash);
}

static void reference_set_insert_pak(reference_set_work_t *rsw, const fsc_file_direct_t *pak)
{
    reference_set_insert_entry(
        rsw, (const char *)STACKPTR(pak->qp_mod_ptr), (const char *)STACKPTR(pak->f.qp_name_ptr), pak->pk3_hash, pak);
}

static void add_referenced_paks(reference_set_work_t *rsw)
{
    // Add all current referenced paks to the reference set
    fs_hashtable_iterator_t it = fs_hashtable_iterate(&reference_tracker, 0, qtrue);
    reference_tracker_entry_t *entry;
    while ((entry = (reference_tracker_entry_t *)fs_hashtable_next(&it)))
    {
        reference_set_insert_pak(rsw, entry->pak);
    }
}

static void add_pak_containing_file(reference_set_work_t *rsw, const char *name)
{
    // Add the pak containing the specified file to the reference set
    const fsc_file_t *file =
        fs_general_lookup(name, LOOKUPFLAG_IGNORE_CURRENT_MAP | LOOKUPFLAG_PK3_SOURCE_ONLY, qfalse);
    if (!file || file->sourcetype != FSC_SOURCETYPE_PK3)
    {
        return;
    }
    reference_set_insert_pak(rsw, fsc_get_base_file(file, &fs));
}

typedef enum { PAKCATEGORY_ACTIVE_MOD, PAKCATEGORY_BASEGAME, PAKCATEGORY_INACTIVE_MOD } pakcategory_t;

static pakcategory_t get_pak_category(const fsc_file_direct_t *pak)
{
    int mod_type = fs_get_mod_type(fsc_get_mod_dir((fsc_file_t *)pak, &fs));
    if (mod_type >= MODTYPE_CURRENT_MOD)
        return PAKCATEGORY_ACTIVE_MOD;
    if (mod_type > MODTYPE_INACTIVE)
        return PAKCATEGORY_BASEGAME;
    return PAKCATEGORY_INACTIVE_MOD;
}

static void add_paks_by_category(reference_set_work_t *rsw, pakcategory_t category)
{
    // Add all loaded paks in specified category to the pak set
    int i;
    fsc_hashtable_iterator_t hti;
    fsc_pk3_hash_map_entry_t *hash_entry;

    for (i = 0; i < fs.pk3_hash_lookup.bucket_count; ++i)
    {
        fsc_hashtable_open(&fs.pk3_hash_lookup, i, &hti);
        while ((hash_entry = (fsc_pk3_hash_map_entry_t *)STACKPTRN(fsc_hashtable_next(&hti))))
        {
            fsc_file_direct_t *pk3 = (fsc_file_direct_t *)STACKPTR(hash_entry->pk3);
            // The *inactivemod_paks rule explicitly follows the fs_read_inactive_mods setting in order for
            //    fs_read_inactive_mods to work in the expected way when using the default pure manifest
            // Note: Pure list from a previous client session should be cleared at this point in the map load process,
            //    so the potential pure list check in FD_CHECK_READ_INACTIVE_MODS should not be a factor here.
            if (fs_file_disabled((fsc_file_t *)pk3, FD_CHECK_FILE_ENABLED | FD_CHECK_READ_INACTIVE_MODS))
                continue;
            if (get_pak_category(pk3) != category)
                continue;
            reference_set_insert_pak(rsw, pk3);
        }
    }
}

static unsigned int string_to_hash(const char *string)
{
    // Converts a user-specified string (signed or unsigned) to hash value
    // Returns 0 on error, hash otherwise
    char test_buffer[16];
    if (*string == '-')
    {
        unsigned int hash = (unsigned int)atoi(string);
        Com_sprintf(test_buffer, sizeof(test_buffer), "%i", (int)hash);
        if (strcmp(string, test_buffer))
            return 0;
        return hash;
    }
    else
    {
        unsigned int hash = strtoul(string, 0, 10);
        Com_sprintf(test_buffer, sizeof(test_buffer), "%u", hash);
        if (strcmp(string, test_buffer))
            return 0;
        return hash;
    }
}

static void add_pak_by_name(reference_set_work_t *rsw, const char *string)
{
    // Add all paks matching the specified filename to the reference set
    char buffer[FSC_MAX_MODDIR + FSC_MAX_QPATH + 32];
    char *name_ptr = 0;
    char *hash_ptr = 0;
    char mod_dir[FSC_MAX_MODDIR];
    char name[FSC_MAX_QPATH];
    unsigned int hash = 0;
    int count = 0;

    // Determine mod_dir, name, and hash
    Q_strncpyz(buffer, string, sizeof(buffer));
    name_ptr = strchr(buffer, '/');
    if (name_ptr)
        *(name_ptr++) = 0;
    if (!name_ptr || !*name_ptr || strchr(name_ptr, '/'))
    {
        Com_Printf("WARNING: Error reading name for rule '%s'\n", rsw->command_name);
        return;
    }
    hash_ptr = strchr(name_ptr, ':');
    if (hash_ptr)
    {
        *(hash_ptr++) = 0;
        hash = string_to_hash(hash_ptr);
        if (!hash)
        {
            Com_Printf("WARNING: Error reading hash for rule '%s'\n", rsw->command_name);
            return;
        }
    }
    fs_generate_path(buffer, 0, 0, 0, 0, 0, mod_dir, sizeof(mod_dir));
    fs_generate_path(name_ptr, 0, 0, 0, 0, 0, name, sizeof(name));

    if (hash)
    {
        // Look for paks matching hash
        fsc_hashtable_iterator_t hti;
        fsc_pk3_hash_map_entry_t *entry;
        fsc_hashtable_open(&fs.pk3_hash_lookup, hash, &hti);
        while ((entry = (fsc_pk3_hash_map_entry_t *)STACKPTRN(fsc_hashtable_next(&hti))))
        {
            const fsc_file_direct_t *file = (const fsc_file_direct_t *)STACKPTR(entry->pk3);
            if (fs_file_disabled((fsc_file_t *)file, FD_CHECK_FILE_ENABLED))
                continue;
            if (file->pk3_hash != hash)
                continue;
            reference_set_insert_entry(rsw, mod_dir, name, hash, file);
            ++count;
        }

        // If no actual pak was found, create a hash-only entry
        if (!count)
        {
            reference_set_insert_entry(rsw, mod_dir, name, hash, 0);
            ++count;
        }
    }

    else
    {
        // No user-specified hash, so add paks matching name instead
        fsc_hashtable_iterator_t hti;
        const fsc_file_direct_t *file;
        fsc_hashtable_open(&fs.files, fsc_string_hash(name, 0), &hti);
        while ((file = (const fsc_file_direct_t *)STACKPTRN(fsc_hashtable_next(&hti))))
        {
            if (file->f.sourcetype != FSC_SOURCETYPE_DIRECT)
                continue;
            if (!file->pk3_hash)
                continue;
            if (fs_file_disabled((const fsc_file_t *)file, FD_CHECK_FILE_ENABLED))
                continue;
            if (Q_stricmp((const char *)STACKPTR(file->f.qp_name_ptr), name))
                continue;
            if (Q_stricmp(fsc_get_mod_dir((const fsc_file_t *)file, &fs), mod_dir))
                continue;
            reference_set_insert_entry(rsw, mod_dir, name, file->pk3_hash, file);
            ++count;
        }

        if (count == 0)
            Com_Printf("WARNING: Command %s failed to match pk3.\n", rsw->command_name);
        if (count > 1)
            Com_Printf("WARNING: Command %s matched multiple pk3s.\n", rsw->command_name);
    }
}

static void generate_reference_set(const char *manifest, fs_hashtable_t *output)
{
    // Provide initialized hashtable for output
    reference_set_work_t rsw;
    Com_Memset(&rsw, 0, sizeof(rsw));
    rsw.reference_set = output;
    pk3_list_initialize(&rsw.exclude_set, 64);

    while (1)
    {
        char *token = COM_ParseExt((char **)&manifest, qfalse);
        if (!*token)
            break;

        // Concatenate command name to include modifiers like "&exclude" in debug prints
        if (*rsw.command_name)
            Q_strcat(rsw.command_name, sizeof(rsw.command_name), " ");
        Q_strcat(rsw.command_name, sizeof(rsw.command_name), token);

        // Process command
        if (!Q_stricmp(token, "&exclude"))
        {
            rsw.exclude_mode = qtrue;
            continue;
        }
        if (!Q_stricmp(token, "&exclude_reset"))
        {
            if (fs_debug_references->integer)
                Com_Printf("Resetting excluded hash set.\n");
            pk3_list_free(&rsw.exclude_set);
            pk3_list_initialize(&rsw.exclude_set, 64);
        }
        else if (!Q_stricmp(token, "-"))
        {
            ++rsw.cluster;
        }
        else if (!Q_stricmp(token, "*mod_paks"))
        {
            add_paks_by_category(&rsw, PAKCATEGORY_ACTIVE_MOD);
        }
        else if (!Q_stricmp(token, "*base_paks"))
        {
            add_paks_by_category(&rsw, PAKCATEGORY_BASEGAME);
        }
        else if (!Q_stricmp(token, "*inactivemod_paks"))
        {
            add_paks_by_category(&rsw, PAKCATEGORY_INACTIVE_MOD);
        }
        else if (!Q_stricmp(token, "*referenced_paks"))
        {
            add_referenced_paks(&rsw);
        }
        else if (!Q_stricmp(token, "*currentmap_pak"))
        {
            add_pak_containing_file(&rsw, va("maps/%s.bsp", Cvar_VariableString("mapname")));
        }
        else if (!Q_stricmp(token, "*cgame_pak"))
        {
            add_pak_containing_file(&rsw, "vm/cgame.qvm");
        }
        else if (!Q_stricmp(token, "*ui_pak"))
        {
            add_pak_containing_file(&rsw, "vm/ui.qvm");
        }
        else
        {
            add_pak_by_name(&rsw, token);
        }

        // Reset modifiers
        *rsw.command_name = 0;
        rsw.exclude_mode = qfalse;
    }

    pk3_list_free(&rsw.exclude_set);
}

/* ******************************************************************************** */
// Download Map Handling
/* ******************************************************************************** */

// The download map is used to match client download requests to the actual file
// on the server, since the download list name may not match the server filename

typedef fs_hashtable_t fs_download_map_t;

typedef struct {
    fs_hashtable_entry_t hte;
    char *name;
    const fsc_file_direct_t *pak;
} download_map_entry_t;

static void free_download_map_entry(fs_hashtable_entry_t *entry)
{
    Z_Free(((download_map_entry_t *)entry)->name);
    Z_Free(entry);
}

static void fs_free_download_map(fs_download_map_t *dlmap)
{
    fs_hashtable_free(dlmap, free_download_map_entry);
    Z_Free(dlmap);
}

static fs_download_map_t *create_download_map(void)
{
    fs_download_map_t *dlmap = (fs_download_map_t *)Z_Malloc(sizeof(*dlmap));
    fs_hashtable_initialize(dlmap, 16);
    return dlmap;
}

static void add_download_map_entry(fs_download_map_t *dlmap, const char *path, const fsc_file_direct_t *pak)
{
    download_map_entry_t *entry = (download_map_entry_t *)Z_Malloc(sizeof(*entry));
    entry->name = CopyString(path);
    entry->pak = pak;
    fs_hashtable_insert(dlmap, (fs_hashtable_entry_t *)entry, fsc_string_hash(path, 0));
}

static fileHandle_t open_download_pak_by_map(fs_download_map_t *dlmap, const char *path, unsigned int *size_out)
{
    fs_hashtable_iterator_t it = fs_hashtable_iterate(dlmap, fsc_string_hash(path, 0), qfalse);
    download_map_entry_t *entry;

    while ((entry = (download_map_entry_t *)fs_hashtable_next(&it)))
    {
        if (!Q_stricmp(entry->name, path))
            return fs_direct_read_handle_open((fsc_file_t *)entry->pak, 0, size_out);
    }

    return 0;
}

/* ******************************************************************************** */
// Reference string generation
/* ******************************************************************************** */

// This section is used to generate the name/hash strings for the download and pure
// lists, as well as the download map corresponding to the download list

static int reference_set_entry_qsort(const void *e1, const void *e2)
{
    return -compare_reference_set_entry(*(const reference_set_entry_t **)e1, *(const reference_set_entry_t **)e2);
}

static void generate_reference_strings(const char *manifest, fsc_stream_t *hash_output, fsc_stream_t *name_output,
    fs_download_map_t *download_map_output, qboolean download)
{
    int i;
    char buffer[512];
    fs_hashtable_t reference_set;
    reference_set_entry_t **reference_list;
    int count = 0;
    int allowDownload = Cvar_VariableIntegerValue("sv_allowDownload");

    if (fs_debug_references->integer)
        Com_Printf("processing manifest: \"%s\"\n", manifest);

    // Make sure outputs are null terminated in case there are 0 references
    if (hash_output)
        *hash_output->data = 0;
    if (name_output)
        *name_output->data = 0;

    // Generate reference set
    fs_hashtable_initialize(&reference_set, MAX_REFERENCE_SET_ENTRIES);
    generate_reference_set(manifest, &reference_set);

    // Generate reference list
    {
        reference_set_entry_t *entry;
        fs_hashtable_iterator_t it = fs_hashtable_iterate(&reference_set, 0, qtrue);
        reference_list = (reference_set_entry_t **)Z_Malloc(sizeof(*reference_list) * reference_set.element_count);
        while ((entry = (reference_set_entry_t *)fs_hashtable_next(&it)))
        {
            if (count >= reference_set.element_count)
                Com_Error(ERR_FATAL, "generate_reference_strings list overflow");
            reference_list[count] = entry;
            ++count;
        }
        if (count != reference_set.element_count)
            Com_Error(ERR_FATAL, "generate_reference_strings list underflow");
    }

    // Sort reference list
    qsort(reference_list, count, sizeof(*reference_list), reference_set_entry_qsort);

    // Process reference list
    for (i = 0; i < count; ++i)
    {
        const reference_set_entry_t *entry = reference_list[i];
        const char *mod_dir = entry->mod_dir;
        const char *name = entry->name;

        if (fs_debug_references->integer)
        {
            Com_Printf("processing entry: entry(%i) hash(%i) path(%s/%s)\n", entry->entry_id, (int)entry->hash,
                entry->mod_dir, entry->name);
        }

        if (download)
        {
            // Replace basemod with fs_basegame since downloads aren't supposed
            // to go directly into basemod and clients may block it or have errors
            if (!Q_stricmp(mod_dir, "basemod"))
                mod_dir = fs_basegame->string;

            // Patch mod dir capitalization
            if (!Q_stricmp(mod_dir, fs_basegame->string))
                mod_dir = fs_basegame->string;
            if (!Q_stricmp(mod_dir, FS_GetCurrentGameDir()))
                mod_dir = FS_GetCurrentGameDir();

#ifndef STANDALONE
            // Don't put paks that fail the id pak check in download list because clients won't download
            // them anyway and may throw an error
            {
                Com_sprintf(buffer, sizeof(buffer), "%s/%s", mod_dir, name);
                if (FS_idPak(buffer, BASEGAME, FS_NODOWNLOAD_PAKS) ||
                    FS_idPak(buffer, BASETA, FS_NODOWNLOAD_PAKS_TEAMARENA))
                {
                    if (fs_debug_references->integer)
                    {
                        Com_Printf("NOTE: Skipping download list entry %i due to ID pak name\n", entry->entry_id);
                    }
                    continue;
                }
            }
#endif

            // Print warning if file is physically unavailable
            if (!entry->pak_file && allowDownload && !(allowDownload & DLF_NO_UDP))
            {
                Com_Printf(
                    "WARNING: Download list file %s/%s from command %s was not found on the server."
                    " Attempts to download this file via UDP will result in an error.\n",
                    mod_dir, name, entry->command_name);
            }

            // Print warning if pak is from an inactive mod dir
            if (fs_get_mod_type(mod_dir) <= MODTYPE_INACTIVE)
            {
                Com_Printf(
                    "WARNING: Download list file %s/%s from command %s is from an inactive mod dir."
                    " This can cause problems for some clients. Consider moving this file or changing the"
                    " active mod to include it.\n",
                    mod_dir, name, entry->command_name);
            }
        }

        if (hash_output)
        {
            Com_sprintf(buffer, sizeof(buffer), "%i", (int)entry->hash);
            if (hash_output->position)
                fsc_stream_append_string(hash_output, " ");
            fsc_stream_append_string(hash_output, buffer);
        }

        if (name_output)
        {
            Com_sprintf(buffer, sizeof(buffer), "%s/%s", mod_dir, name);
            if (name_output->position)
                fsc_stream_append_string(name_output, " ");
            fsc_stream_append_string(name_output, buffer);
        }

        if (download_map_output)
        {
            if (entry->pak_file)
            {
                Com_sprintf(buffer, sizeof(buffer), "%s/%s.pk3", mod_dir, name);
                add_download_map_entry(download_map_output, buffer, entry->pak_file);
            }
        }
    }

    fs_hashtable_free(&reference_set, 0);
    Z_Free(reference_list);

    Com_Printf("%i paks listed\n", count);
}

/* ******************************************************************************** */
// General functions
/* ******************************************************************************** */

static fs_download_map_t *download_map;

void fs_set_download_list(void)
{
    // Called by server to set "sv_referencedPaks" and "sv_referencedPakNames"
    // Also updates the download_map structure above which is used to match
    //    path string from the client to the actual pk3 path
    char hash_buffer[2048];
    fsc_stream_t hash_stream = {hash_buffer, 0, sizeof(hash_buffer), 0};
    char name_buffer[2048];
    fsc_stream_t name_stream = {name_buffer, 0, sizeof(name_buffer), 0};

    // Reset download map
    if (download_map)
        fs_free_download_map(download_map);
    download_map = create_download_map();

    // Generate strings and download map
    Com_Printf("Generating download list...\n");
    generate_reference_strings(fs_download_manifest->string, &hash_stream, &name_stream, download_map, qtrue);

    // Check for overflow
    if (hash_stream.overflowed || name_stream.overflowed)
    {
        Com_Printf("WARNING: Download list overflowed\n");
        Cvar_Set("sv_referencedPaks", "");
        Cvar_Set("sv_referencedPakNames", "");
        fs_free_download_map(download_map);
        download_map = 0;
    }

    Cvar_Set("sv_referencedPaks", hash_buffer);
    Cvar_Set("sv_referencedPakNames", name_buffer);
}

fileHandle_t fs_open_download_pak(const char *path, unsigned int *size_out)
{
    if (download_map)
        return open_download_pak_by_map(download_map, path, size_out);
    return 0;
}

#define SYSTEMINFO_RESERVED_SIZE 256

void fs_set_pure_list(void)
{
    Cvar_Set("sv_paks", "");
    Cvar_Set("sv_pakNames", "");

    if (Cvar_VariableIntegerValue("sv_pure"))
    {
        char hash_buffer[BIG_INFO_STRING];
        fsc_stream_t hash_stream = {hash_buffer, 0, sizeof(hash_buffer), 0};
        char name_buffer[BIG_INFO_STRING];
        fsc_stream_t name_stream = {name_buffer, 0, sizeof(name_buffer), 0};
        int systeminfo_base_length;

        // Generate strings
        Com_Printf("Generating pure list...\n");
        generate_reference_strings(fs_pure_manifest->string, &hash_stream, &name_stream, 0, qfalse);

        // Check for error conditions
        if (!*hash_buffer)
        {
            Com_Printf("WARNING: Setting sv_pure to 0 due to empty pure list.\n");
            Cvar_Set("sv_pure", "0");
            return;
        }

        if (hash_stream.overflowed)
        {
            Com_Printf(
                "WARNING: Setting sv_pure to 0 due to pure list overflow. Remove some"
                " paks from the server or adjust the pure manifest if you want to use sv_pure.\n");
            Cvar_Set("sv_pure", "0");
            return;
        }

        systeminfo_base_length = strlen(Cvar_InfoString_Big(CVAR_SYSTEMINFO));
        if (systeminfo_base_length + hash_stream.position + SYSTEMINFO_RESERVED_SIZE >= BIG_INFO_STRING)
        {
            Com_Printf(
                "WARNING: Setting sv_pure to 0 due to systeminfo overflow. Remove some"
                " paks from the server or adjust the pure manifest if you want to use sv_pure.\n");
            Cvar_Set("sv_pure", "0");
            return;
        }

        // Set main pure list value
        Cvar_Set("sv_paks", hash_buffer);

        // Only set names if it doesn't overflow
        // It is normally only used for informational purposes, so it's okay to leave it empty
        if (!name_stream.overflowed &&
            systeminfo_base_length + hash_stream.position + name_stream.position + SYSTEMINFO_RESERVED_SIZE <
                BIG_INFO_STRING)
        {
            Cvar_Set("sv_pakNames", name_buffer);
        }
    }
}

#endif  // NEW_FILESYSTEM
