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

#define SYSTEMINFO_RESERVED_SIZE 256

#define MAX_DOWNLOAD_LIST_STRING 2048
#define MAX_PURE_LIST_STRING BIG_INFO_STRING

#define REF_DPRINTF(...)                  \
    {                                     \
        if (fs_debug_references->integer) \
            FS_DPrintf(__VA_ARGS__);      \
    }

/* ******************************************************************************** */
// Referenced Pak Tracking
/* ******************************************************************************** */

// The "reference_tracker" set is filled by logging game references to pk3 files
// It currently serves two purposes:
// 1) To generate the pure validation string when fs_full_pure_validation is 1
//		(Although I have never seen any server that requires this to connect)
// 2) As a component of the download pak list creation via '*referenced_paks' rule
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
            REF_DPRINTF("recording reference: %s\n", temp);
        }
    }
}

void FS_ClearPakReferences(int flags)
{
    REF_DPRINTF("clearing referenced paks\n");
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
            Com_Error(ERR_FATAL, "generate_referenced_pak_list list overflowed");
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
            REF_DPRINTF("adding pak to pure validation list: %s\n", temp);
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
// Shared Reference Structures
/* ******************************************************************************** */

// The reference query represents the input to the reference set generation functions.
typedef struct {
    // Manifest string (from manifest cvars)
    const char *manifest;

    // Enable certain special handling if query is for download list
    qboolean download;
} reference_query_t;

// Each reference list entry corresponds to one hash+name pair in the output pure/download
//   list strings.
typedef struct {
    // Primary characteristics
    char mod_dir[FSC_MAX_MODDIR];
    char name[FSC_MAX_QPATH];
    unsigned int hash;
    const fsc_file_direct_t *pak_file;  // Optional (if null, represents hash-only entry)

    // For debug print purposes
    char command_name[64];  // Name of the selector command that created this entry
    int entry_id;  // Numerical value assigned to entry to identify it in debug prints

    // Don't write to final string output
    qboolean disabled;
} reference_list_entry_t;

// Current state of a reference set / list / strings structure
typedef enum { REFSTATE_UNINITIALIZED, REFSTATE_OVERFLOWED, REFSTATE_VALID } reference_state_t;

/* ******************************************************************************** */
// Reference Set Generation
/* ******************************************************************************** */

// This section is used to create a reference set from a reference query.

typedef struct {
    // REFSTATE_VALID: Hashtable will be initialized and iterable
    // REFSTATE_UNINITIALIZED / REFSTATE_OVERFLOWED: Hashtable not initialized
    reference_state_t state;
    fs_hashtable_t h;
} reference_set_t;

typedef struct {
    fs_hashtable_entry_t hte;
    reference_list_entry_t l;

    // Misc sorting characteristics
    int pak_file_name_match;
    unsigned int cluster;  // Indicates dash separated cluster (lower value is higher priority)

    // Sort key
    char sort_key[FSC_MAX_MODDIR + FSC_MAX_QPATH + 32];
    unsigned int sort_key_length;
} reference_set_entry_t;

typedef enum { RSPC_NONE, RSPC_CVAR_IMPORT } reference_set_pending_command_t;

typedef struct {
    // General state
    const reference_query_t *query;
    reference_set_t *reference_set;
    pk3_list_t block_set;
    int cluster;
    qboolean overflowed;

    // Current command
    reference_set_pending_command_t pending_command;
    qboolean block_mode;

    // For debug prints
    int entry_id_counter;
    char command_name[64];
} reference_set_work_t;

static void refset_generate_entry(reference_set_work_t *rsw, const char *mod_dir, const char *name, unsigned int hash,
    const fsc_file_direct_t *pak_file, reference_set_entry_t *target)
{
    // pak can be null; other parameters are required
    fsc_stream_t sort_stream = {target->sort_key, 0, sizeof(target->sort_key), 0};

    Com_Memset(target, 0, sizeof(*target));
    Q_strncpyz(target->l.mod_dir, mod_dir, sizeof(target->l.mod_dir));
    Q_strncpyz(target->l.name, name, sizeof(target->l.name));
    target->l.hash = hash;
    target->l.pak_file = pak_file;
    target->cluster = rsw->cluster;
    target->l.entry_id = rsw->entry_id_counter++;

    // Write command name debug string
    Q_strncpyz(target->l.command_name, rsw->command_name, sizeof(target->l.command_name));
    if (fsc_strlen(rsw->command_name) >= sizeof(target->l.command_name))
    {
        strcpy(target->l.command_name + sizeof(target->l.command_name) - 4, "...");
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
        if (!fsc_strcmp(mod_dir, pak_mod) && !fsc_strcmp(name, pak_name))
            target->pak_file_name_match = 3;
        else if (!Q_stricmp(mod_dir, pak_mod) && !Q_stricmp(name, pak_name))
            target->pak_file_name_match = 2;
        else
            target->pak_file_name_match = 1;
    }

    // Write sort key
    {
        FS_ModType mod_type = fs_get_mod_type(target->l.mod_dir);
        unsigned int core_pak_priority =
            mod_type < MODTYPE_OVERRIDE_DIRECTORY ? (unsigned int)core_pk3_position(hash) : 0;

        fs_write_sort_value(~target->cluster, &sort_stream);
        fs_write_sort_value(mod_type >= MODTYPE_OVERRIDE_DIRECTORY ? (unsigned int)mod_type : 0, &sort_stream);
        fs_write_sort_value(core_pak_priority, &sort_stream);
        fs_write_sort_value((unsigned int)mod_type, &sort_stream);
        fs_write_sort_string(target->l.mod_dir, &sort_stream, qfalse);
        fs_write_sort_string(target->l.name, &sort_stream, qfalse);
        fs_write_sort_value(target->pak_file_name_match, &sort_stream);
        target->sort_key_length = sort_stream.position;
    }
}

static int refset_compare_entry(const reference_set_entry_t *e1, const reference_set_entry_t *e2)
{
    // Returns < 0 if e1 is higher precedence, > 0 if e2 is higher precedence
    return -fsc_memcmp(e1->sort_key, e2->sort_key,
        e1->sort_key_length < e2->sort_key_length ? e1->sort_key_length : e2->sort_key_length);
}

static void refset_insert_entry(
    reference_set_work_t *rsw, const char *mod_dir, const char *name, unsigned int hash, const fsc_file_direct_t *pak)
{
    // Inserts or updates reference entry into output reference set
    reference_set_entry_t new_entry;
    fs_hashtable_iterator_t it;
    reference_set_entry_t *target_entry = 0;

    // Peform some mod dir patching for download list
    if (rsw->query->download)
    {
        // Replace basemod with fs_basegame since downloads aren't supposed
        // to go directly into basemod and clients may block it or have errors
        if (!Q_stricmp(mod_dir, "basemod"))
        {
            REF_DPRINTF("[manifest processing] Replacing download mod directory 'basemod' with fs_basegame\n");
            mod_dir = fs_basegame->string;
        }

        // Patch mod dir capitalization
        if (!Q_stricmp(mod_dir, fs_basegame->string))
            mod_dir = fs_basegame->string;
        if (!Q_stricmp(mod_dir, FS_GetCurrentGameDir()))
            mod_dir = FS_GetCurrentGameDir();
    }

    // Generate new entry
    refset_generate_entry(rsw, mod_dir, name, hash, pak, &new_entry);

    // Print entry contents
    if (fs_debug_references->integer)
    {
        REF_DPRINTF("[manifest processing] Reference set entry created\n");
        REF_DPRINTF("  entry id: %i\n", new_entry.l.entry_id);
        REF_DPRINTF("  source rule: %s\n", new_entry.l.command_name);
        REF_DPRINTF("  path: %s/%s\n", new_entry.l.mod_dir, new_entry.l.name);
        REF_DPRINTF("  hash: %i\n", (int)new_entry.l.hash);
        if (new_entry.l.pak_file)
        {
            char buffer[FS_FILE_BUFFER_SIZE];
            fs_file_to_buffer(
                (const fsc_file_t *)new_entry.l.pak_file, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
            REF_DPRINTF("  pak file: %s\n", buffer);
            REF_DPRINTF("  pak file name match: %u\n", new_entry.pak_file_name_match);
        }
        else
        {
            REF_DPRINTF("  pak file: <none>\n");
        }
        REF_DPRINTF("  cluster: %i\n", new_entry.cluster);
    }

#ifndef STANDALONE
    // Exclude paks that fail the ID pak check from download list because clients won't download
    // them anyway and may throw an error
    if (rsw->query->download)
    {
        char buffer[256];
        Com_sprintf(buffer, sizeof(buffer), "%s/%s", mod_dir, name);
        if (FS_idPak(buffer, BASEGAME, FS_NODOWNLOAD_PAKS) || FS_idPak(buffer, BASETA, FS_NODOWNLOAD_PAKS_TEAMARENA))
        {
            REF_DPRINTF("  result: Skipping download list entry due to ID pak name\n");
            return;
        }
    }
#endif

    // Process block command
    if (rsw->block_mode)
    {
        if (pk3_list_lookup(&rsw->block_set, hash))
        {
            REF_DPRINTF("  result: Hash already in block list\n");
        }
        else
        {
            REF_DPRINTF("  result: Hash added to block list\n");
            pk3_list_insert(&rsw->block_set, hash);
        }
        return;
    }

    // Check if hash is blocked
    if (pk3_list_lookup(&rsw->block_set, hash))
    {
        REF_DPRINTF("  result: Skipping entry due to hash in block list\n");
        return;
    }

    // Look for existing entry with same hash
    it = fs_hashtable_iterate(&rsw->reference_set->h, hash, qfalse);
    while ((target_entry = (reference_set_entry_t *)fs_hashtable_next(&it)))
    {
        if (new_entry.l.hash == target_entry->l.hash)
        {
            // Found entry; check if new entry is higher priority
            int compare_result = refset_compare_entry(&new_entry, target_entry);
            if (fs_debug_references->integer)
            {
                if (compare_result >= 0)
                {
                    REF_DPRINTF("  result: Duplicate hash - skipping entry due to existing %s precedence entry id %i\n",
                        compare_result > 0 ? "higher" : "equal", target_entry->l.entry_id);
                }
                else
                {
                    REF_DPRINTF("  result: Duplicate hash - overwriting existing lower precedence entry id %i\n",
                        target_entry->l.entry_id);
                }
            }
            if (compare_result < 0)
                *target_entry = new_entry;
            return;
        }
    }

    // Check for excess element count
    if (rsw->reference_set->h.element_count >= MAX_REFERENCE_SET_ENTRIES)
    {
        REF_DPRINTF("  result: Skipping entry due to MAX_REFERENCE_SET_ENTRIES hit\n");
        rsw->overflowed = qtrue;
        return;
    }

    // Save the entry
    REF_DPRINTF("  result: Added entry to reference set\n");
    target_entry = (reference_set_entry_t *)Z_Malloc(sizeof(*target_entry));
    *target_entry = new_entry;
    fs_hashtable_insert(&rsw->reference_set->h, (fs_hashtable_entry_t *)target_entry, target_entry->l.hash);
}

static void refset_insert_pak(reference_set_work_t *rsw, const fsc_file_direct_t *pak)
{
    // Add a particular pak file to the reference set
    refset_insert_entry(
        rsw, (const char *)STACKPTR(pak->qp_mod_ptr), (const char *)STACKPTR(pak->f.qp_name_ptr), pak->pk3_hash, pak);
}

static void refset_add_referenced_paks(reference_set_work_t *rsw)
{
    // Add all current referenced paks to the reference set
    fs_hashtable_iterator_t it = fs_hashtable_iterate(&reference_tracker, 0, qtrue);
    reference_tracker_entry_t *entry;
    while ((entry = (reference_tracker_entry_t *)fs_hashtable_next(&it)))
    {
        refset_insert_pak(rsw, entry->pak);
    }
}

static void refset_add_pak_containing_file(reference_set_work_t *rsw, const char *name)
{
    // Add the pak containing the specified file to the reference set
    const fsc_file_t *file =
        fs_general_lookup(name, LOOKUPFLAG_IGNORE_CURRENT_MAP | LOOKUPFLAG_PK3_SOURCE_ONLY, qfalse);
    if (!file || file->sourcetype != FSC_SOURCETYPE_PK3)
    {
        return;
    }
    refset_insert_pak(rsw, fsc_get_base_file(file, &fs));
}

typedef enum { PAKCATEGORY_ACTIVE_MOD, PAKCATEGORY_BASEGAME, PAKCATEGORY_INACTIVE_MOD } pakcategory_t;

static pakcategory_t refset_get_pak_category(const fsc_file_direct_t *pak)
{
    int mod_type = fs_get_mod_type(fsc_get_mod_dir((fsc_file_t *)pak, &fs));
    if (mod_type >= MODTYPE_CURRENT_MOD)
        return PAKCATEGORY_ACTIVE_MOD;
    if (mod_type > MODTYPE_INACTIVE)
        return PAKCATEGORY_BASEGAME;
    return PAKCATEGORY_INACTIVE_MOD;
}

static void refset_add_paks_by_category(reference_set_work_t *rsw, pakcategory_t category)
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
            if (refset_get_pak_category(pk3) != category)
                continue;
            refset_insert_pak(rsw, pk3);
        }
    }
}

static unsigned int refset_string_to_hash(const char *string)
{
    // Converts a user-specified string (signed or unsigned) to hash value
    // Returns 0 on error, hash otherwise
    char test_buffer[16];
    if (*string == '-')
    {
        unsigned int hash = (unsigned int)atoi(string);
        Com_sprintf(test_buffer, sizeof(test_buffer), "%i", (int)hash);
        if (fsc_strcmp(string, test_buffer))
            return 0;
        return hash;
    }
    else
    {
        unsigned int hash = strtoul(string, 0, 10);
        Com_sprintf(test_buffer, sizeof(test_buffer), "%u", hash);
        if (fsc_strcmp(string, test_buffer))
            return 0;
        return hash;
    }
}

static void refset_add_pak_by_name(reference_set_work_t *rsw, const char *string)
{
    // Add any paks matching the specified filename to the reference set
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
        hash = refset_string_to_hash(hash_ptr);
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
            refset_insert_entry(rsw, mod_dir, name, hash, file);
            ++count;
        }

        // If no actual pak was found, create a hash-only entry
        if (!count)
        {
            refset_insert_entry(rsw, mod_dir, name, hash, 0);
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
            refset_insert_entry(rsw, mod_dir, name, file->pk3_hash, file);
            ++count;
        }

        if (count == 0)
            Com_Printf("WARNING: Command %s failed to match pk3.\n", rsw->command_name);
        if (count > 1)
            Com_Printf("WARNING: Command %s matched multiple pk3s.\n", rsw->command_name);
    }
}

static void refset_process_manifest(reference_set_work_t *rsw, const char *string, int recursion_count)
{
    while (1)
    {
        const char *token = COM_ParseExt((char **)&string, qfalse);
        if (!*token)
            break;

        // Process pending multi-token commands
        if (rsw->pending_command == RSPC_CVAR_IMPORT)
        {
            // Static buffer from COM_ParseExt will be overwritten, so copy out token
            char cvar_name[256];
            Q_strncpyz(cvar_name, token, sizeof(cvar_name));

            rsw->pending_command = RSPC_NONE;

            if (recursion_count >= 128)
            {
                Com_Error(ERR_DROP, "Recursive overflow processing pk3 manifest");
            }
            REF_DPRINTF("[manifest processing] Entering import cvar '%s'\n", cvar_name);
            refset_process_manifest(rsw, Cvar_VariableString(cvar_name), recursion_count + 1);
            REF_DPRINTF("[manifest processing] Leaving import cvar '%s'\n", cvar_name);
            continue;
        }

        // Process special commands
        if (!Q_stricmp(token, "&cvar_import"))
        {
            rsw->pending_command = RSPC_CVAR_IMPORT;
            continue;
        }
        else if (!Q_stricmp(token, "&block"))
        {
            rsw->block_mode = qtrue;
            continue;
        }
        else if (!Q_stricmp(token, "&block_reset"))
        {
            REF_DPRINTF("[manifest processing] Resetting blocked hash set.\n");
            pk3_list_free(&rsw->block_set);
            pk3_list_initialize(&rsw->block_set, 64);
            continue;
        }
        else if (!Q_stricmp(token, "-"))
        {
            ++rsw->cluster;
            continue;
        }

        // Save command_name for debug purposes
        Com_sprintf(rsw->command_name, sizeof(rsw->command_name), "%s%s", rsw->block_mode ? "&block " : "", token);

        // Process selector commands
        REF_DPRINTF("[manifest processing] Processing selector '%s'\n", rsw->command_name);
        if (!Q_stricmp(token, "*mod_paks"))
        {
            refset_add_paks_by_category(rsw, PAKCATEGORY_ACTIVE_MOD);
        }
        else if (!Q_stricmp(token, "*base_paks"))
        {
            refset_add_paks_by_category(rsw, PAKCATEGORY_BASEGAME);
        }
        else if (!Q_stricmp(token, "*inactivemod_paks"))
        {
            refset_add_paks_by_category(rsw, PAKCATEGORY_INACTIVE_MOD);
        }
        else if (!Q_stricmp(token, "*referenced_paks"))
        {
            refset_add_referenced_paks(rsw);
        }
        else if (!Q_stricmp(token, "*currentmap_pak"))
        {
            refset_add_pak_containing_file(rsw, va("maps/%s.bsp", Cvar_VariableString("mapname")));
        }
        else if (!Q_stricmp(token, "*cgame_pak"))
        {
            refset_add_pak_containing_file(rsw, "vm/cgame.qvm");
        }
        else if (!Q_stricmp(token, "*ui_pak"))
        {
            refset_add_pak_containing_file(rsw, "vm/ui.qvm");
        }
        else if (*token == '*' || *token == '&')
        {
            Com_Printf("WARNING: Unrecognized manifest selector '%s'\n", token);
        }
        else
        {
            refset_add_pak_by_name(rsw, token);
        }

        // Reset single-use modifiers
        rsw->block_mode = qfalse;
    }
}

static reference_set_t refset_uninitialized(void)
{
    reference_set_t result = {REFSTATE_UNINITIALIZED};
    return result;
}

static reference_set_t refset_generate(const reference_query_t *query)
{
    // Generates reference set for given query
    // Result must be freed by refset_free
    reference_set_t output = refset_uninitialized();
    reference_set_work_t rsw;

    // Initialize output
    output.state = REFSTATE_VALID;
    fs_hashtable_initialize(&output.h, MAX_REFERENCE_SET_ENTRIES);

    // Initialize rsw
    Com_Memset(&rsw, 0, sizeof(rsw));
    rsw.query = query;
    rsw.reference_set = &output;
    pk3_list_initialize(&rsw.block_set, 64);

    // Invoke manifest processing
    refset_process_manifest(&rsw, query->manifest, 0);

    // Free rsw
    pk3_list_free(&rsw.block_set);

    if (rsw.overflowed)
    {
        // Clear structure in case of overflow
        fs_hashtable_free(&output.h, 0);
        output = refset_uninitialized();
        output.state = REFSTATE_OVERFLOWED;
    }

    return output;
}

static void refset_free(reference_set_t *reference_set)
{
    if (reference_set->state == REFSTATE_VALID)
    {
        fs_hashtable_free(&reference_set->h, 0);
    }
}

/* ******************************************************************************** */
// Reference List Generation
/* ******************************************************************************** */

// This section is used to create a reference list from a reference set.

typedef struct {
    // REFSTATE_VALID: Entries will be valid pointer, entry count >= 0
    // REFSTATE_UNINITIALIZED / REFSTATE_OVERFLOWED: Entries not valid pointer, entry count == 0
    reference_state_t state;
    reference_list_entry_t *entries;
    int entry_count;
} reference_list_t;

static int reflist_sort_function(const void *e1, const void *e2)
{
    return refset_compare_entry(*(const reference_set_entry_t **)e1, *(const reference_set_entry_t **)e2);
}

static reference_list_t reflist_uninitialized(void)
{
    reference_list_t result = {REFSTATE_UNINITIALIZED};
    return result;
}

static reference_list_t reflist_generate(const reference_set_t *reference_set)
{
    // Converts reference set to reference list
    // Result must be freed by reflist_free
    reference_list_t reference_list = reflist_uninitialized();

    if (reference_set->state == REFSTATE_VALID)
    {
        // Generate temp entries
        int i;
        int count = 0;
        reference_set_entry_t *entry;
        fs_hashtable_iterator_t it = fs_hashtable_iterate((fs_hashtable_t *)&reference_set->h, 0, qtrue);
        reference_set_entry_t *temp_entries[MAX_REFERENCE_SET_ENTRIES];
        FSC_ASSERT(reference_set->h.element_count <= MAX_REFERENCE_SET_ENTRIES);
        while ((entry = (reference_set_entry_t *)fs_hashtable_next(&it)))
        {
            FSC_ASSERT(count < reference_set->h.element_count);
            temp_entries[count] = entry;
            ++count;
        }
        FSC_ASSERT(count == reference_set->h.element_count);

        // Sort temp entries
        qsort(temp_entries, count, sizeof(*temp_entries), reflist_sort_function);

        // Initialize reference list
        reference_list.state = REFSTATE_VALID;
        reference_list.entries = (reference_list_entry_t *)Z_Malloc(sizeof(*reference_list.entries) * count);
        reference_list.entry_count = count;

        // Copy reference list entries
        for (i = 0; i < count; ++i)
        {
            reference_list.entries[i] = temp_entries[i]->l;
        }
    }

    else
    {
        // Just leave the list uninitialized
        reference_list.state = reference_set->state;
    }

    return reference_list;
}

static void reflist_free(reference_list_t *reference_list)
{
    if (reference_list->state == REFSTATE_VALID)
    {
        Z_Free(reference_list->entries);
    }
}

/* ******************************************************************************** */
// Reference String Generation
/* ******************************************************************************** */

// This section is used to create reference strings from a reference list.

typedef struct {
    // REFSTATE_VALID: String will be pointer in Z_Malloc, length >= 0
    // REFSTATE_UNINITIALIZED / REFSTATE_OVERFLOWED: String equals static "", length == 0
    reference_state_t state;
    char *string;
    unsigned int length;  // strlen(string)
} reference_substring_t;

typedef struct {
    reference_substring_t name;
    reference_substring_t hash;
} reference_strings_t;

static reference_substring_t refstrings_generate_substring(fsc_stream_t *source)
{
    // Convert source stream to reference string structure
    reference_substring_t output = {REFSTATE_UNINITIALIZED, (char *)"", 0};
    if (source->overflowed)
        output.state = REFSTATE_OVERFLOWED;
    if (source->position && !source->overflowed)
    {
        output.state = REFSTATE_VALID;
        output.length = source->position;
        output.string = (char *)Z_Malloc(output.length + 1);
        fsc_memcpy(output.string, source->data, output.length);
        output.string[output.length] = 0;
    }
    return output;
}

static void refstrings_free_substring(reference_substring_t *substring)
{
    if (substring->state == REFSTATE_VALID)
    {
        Z_Free(substring->string);
    }
}

static reference_strings_t refstrings_uninitialized(void)
{
    reference_strings_t result = {{REFSTATE_UNINITIALIZED, (char *)"", 0}, {REFSTATE_UNINITIALIZED, (char *)"", 0}};
    return result;
}

static reference_strings_t refstrings_generate(reference_list_t *reference_list, unsigned int max_length)
{
    // Result must be freed by refstrings_free
    reference_strings_t output = refstrings_uninitialized();

    if (reference_list->state != REFSTATE_VALID)
    {
        if (reference_list->state == REFSTATE_OVERFLOWED)
        {
            // Copy overflowed state to string outputs
            output.hash.state = output.name.state = REFSTATE_OVERFLOWED;
        }
    }

    else
    {
        int i;
        char buffer[512];
        fsc_stream_t name_stream = {(char *)Z_Malloc(max_length), 0, max_length, 0};
        fsc_stream_t hash_stream = {(char *)Z_Malloc(max_length), 0, max_length, 0};

        // Generate strings
        for (i = 0; i < reference_list->entry_count; ++i)
        {
            const reference_list_entry_t *entry = &reference_list->entries[i];
            if (entry->disabled)
                continue;

            Com_sprintf(buffer, sizeof(buffer), "%i", (int)entry->hash);
            if (hash_stream.position)
                fsc_stream_append_string(&hash_stream, " ");
            fsc_stream_append_string(&hash_stream, buffer);

            Com_sprintf(buffer, sizeof(buffer), "%s/%s", entry->mod_dir, entry->name);
            if (name_stream.position)
                fsc_stream_append_string(&name_stream, " ");
            fsc_stream_append_string(&name_stream, buffer);
        }

        // Transfer strings to output structure
        output.hash = refstrings_generate_substring(&hash_stream);
        Z_Free(hash_stream.data);
        output.name = refstrings_generate_substring(&name_stream);
        Z_Free(name_stream.data);
    }

    return output;
}

static void refstrings_free(reference_strings_t *reference_strings)
{
    refstrings_free_substring(&reference_strings->hash);
    refstrings_free_substring(&reference_strings->name);
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

static void dlmap_free_entry(fs_hashtable_entry_t *entry)
{
    Z_Free(((download_map_entry_t *)entry)->name);
    Z_Free(entry);
}

static void dlmap_add_entry(fs_download_map_t *dlmap, const char *path, const fsc_file_direct_t *pak)
{
    download_map_entry_t *entry = (download_map_entry_t *)Z_Malloc(sizeof(*entry));
    entry->name = CopyString(path);
    entry->pak = pak;
    fs_hashtable_insert(dlmap, (fs_hashtable_entry_t *)entry, fsc_string_hash(path, 0));
}

static void dlmap_free(fs_download_map_t *dlmap)
{
    fs_hashtable_free(dlmap, dlmap_free_entry);
    Z_Free(dlmap);
}

static fs_download_map_t *dlmap_generate(const reference_list_t *reference_list)
{
    int i;
    char buffer[512];
    fs_download_map_t *dlmap = (fs_download_map_t *)Z_Malloc(sizeof(*dlmap));
    fs_hashtable_initialize(dlmap, 16);
    for (i = 0; i < reference_list->entry_count; ++i)
    {
        const reference_list_entry_t *entry = &reference_list->entries[i];
        if (entry->disabled)
            continue;
        Com_sprintf(buffer, sizeof(buffer), "%s/%s.pk3", entry->mod_dir, entry->name);
        dlmap_add_entry(dlmap, buffer, entry->pak_file);
    }
    return dlmap;
}

static fileHandle_t dlmap_open_pak(fs_download_map_t *dlmap, const char *path, unsigned int *size_out)
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
// Download / Pure List Generation
/* ******************************************************************************** */

// Current download map
static fs_download_map_t *download_map;

static reference_list_t reference_list_from_query(const reference_query_t *query)
{
    reference_set_t reference_set = refset_generate(query);
    reference_list_t reference_list = reflist_generate(&reference_set);
    refset_free(&reference_set);
    return reference_list;
}

static qboolean hash_in_reference_list(reference_list_t *reference_list, unsigned int hash)
{
    int i;
    for (i = 0; i < reference_list->entry_count; ++i)
    {
        if (reference_list->entries[i].hash == hash)
            return qtrue;
    }
    return qfalse;
}

void fs_generate_reference_lists(void)
{
    // Generate download and pure lists for server and set appropriate cvars
    int i, j;
    reference_query_t download_query = {fs_download_manifest->string, qtrue};
    reference_query_t pure_query = {fs_pure_manifest->string, qfalse};
    reference_list_t download_list = reflist_uninitialized();
    reference_list_t pure_list = reflist_uninitialized();
    reference_strings_t download_strings = refstrings_uninitialized();
    reference_strings_t pure_strings = refstrings_uninitialized();
    qboolean download_valid = qtrue;
    qboolean pure_valid = qfalse;
    qboolean pure_names_valid = qfalse;

    // Need to clear cvars here for the systeminfo length checks to work properly
    Cvar_Set("sv_paks", "");
    Cvar_Set("sv_pakNames", "");
    Cvar_Set("sv_referencedPaks", "");
    Cvar_Set("sv_referencedPakNames", "");

    // Generate download list
    Com_Printf("Generating download list...\n");
    fs_debug_indent_start();
    download_list = reference_list_from_query(&download_query);
    fs_debug_indent_stop();
    Com_Printf("%i paks listed\n", download_list.entry_count);

    // Verify download list
    for (i = 0; i < download_list.entry_count; ++i)
    {
        int allowDownload = Cvar_VariableIntegerValue("sv_allowDownload");
        reference_list_entry_t *entry = &download_list.entries[i];

        // Check for entry with duplicate filename
        for (j = 0; j < i; ++j)
        {
            reference_list_entry_t *entry2 = &download_list.entries[j];
            if (entry2->disabled)
                continue;
            if (!Q_stricmp(entry->mod_dir, entry2->mod_dir) && !Q_stricmp(entry->name, entry2->name))
            {
                break;
            }
        }
        if (j < i)
        {
            Com_Printf(
                "WARNING: Skipping download list pak '%s/%s' with same filename but"
                " different hash as another entry.\n",
                entry->mod_dir, entry->name);
            entry->disabled = qtrue;
            continue;
        }

        // Print warning if file is physically unavailable
        if (!entry->pak_file && allowDownload && !(allowDownload & DLF_NO_UDP))
        {
            Com_Printf(
                "WARNING: Download list pak '%s/%s' from command '%s' was not found on the server."
                " Attempts to download this file via UDP will result in an error.\n",
                entry->mod_dir, entry->name, entry->command_name);
        }

        // Print warning if pak is from an inactive mod dir
        if (fs_get_mod_type(entry->mod_dir) <= MODTYPE_INACTIVE)
        {
            Com_Printf(
                "WARNING: Download list pak '%s/%s' from command '%s' is from an inactive mod dir."
                " This can cause problems for some clients. Consider moving this file or changing the"
                " active mod to include it.\n",
                entry->mod_dir, entry->name, entry->command_name);
        }
    }

    // Generate download strings
    download_strings = refstrings_generate(&download_list, MAX_DOWNLOAD_LIST_STRING);

    // Check for download list overflow
    if (download_strings.hash.state == REFSTATE_OVERFLOWED || download_strings.name.state == REFSTATE_OVERFLOWED)
    {
        Com_Printf("WARNING: Download list overflowed\n");
        download_valid = qfalse;
    }

    if (Cvar_VariableIntegerValue("sv_pure"))
    {
        int systeminfo_base_length = fsc_strlen(Cvar_InfoString_Big(CVAR_SYSTEMINFO));
        int download_base_length = download_valid ? download_strings.name.length + download_strings.hash.length : 0;
        pure_valid = pure_names_valid = qtrue;

        // Generate pure list
        Com_Printf("Generating pure list...\n");
        fs_debug_indent_start();
        pure_list = reference_list_from_query(&pure_query);
        fs_debug_indent_stop();
        Com_Printf("%i paks listed\n", pure_list.entry_count);

        // Generate pure strings
        pure_strings = refstrings_generate(&pure_list, MAX_PURE_LIST_STRING);

        // Check for pure list hash overflow
        if (pure_strings.hash.state == REFSTATE_OVERFLOWED)
        {
            Com_Printf(
                "WARNING: Setting sv_pure to 0 due to pure list overflow. Remove some"
                " paks from the server or adjust the pure manifest if you want to use sv_pure.\n");
            pure_valid = pure_names_valid = qfalse;
        }

        // Check for empty pure list
        if (pure_valid && pure_list.entry_count == 0)
        {
            Com_Printf("WARNING: Setting sv_pure to 0 due to empty pure list.\n");
            pure_valid = pure_names_valid = qfalse;
        }

        // Check for pure list hash systeminfo overflow
        if (pure_valid &&
            systeminfo_base_length + download_base_length + pure_strings.hash.length + SYSTEMINFO_RESERVED_SIZE >=
                BIG_INFO_STRING)
        {
            Com_Printf(
                "WARNING: Setting sv_pure to 0 due to systeminfo overflow. Remove some"
                " paks from the server or adjust the pure manifest if you want to use sv_pure.\n");
            pure_valid = pure_names_valid = qfalse;
        }

        // Check for pure list names output overflow
        if (pure_names_valid && pure_strings.name.state == REFSTATE_OVERFLOWED)
        {
            Com_Printf("NOTE: Not writing optional sv_pakNames value due to list overflow.\n");
            pure_names_valid = qfalse;
        }

        // Check for pure list names systeminfo overflow
        if (pure_names_valid && systeminfo_base_length + download_base_length + pure_strings.hash.length +
                                        pure_strings.name.length + SYSTEMINFO_RESERVED_SIZE >=
                                    BIG_INFO_STRING)
        {
            Com_Printf("NOTE: Not writing optional sv_pakNames value due to systeminfo overflow.\n");
            pure_names_valid = qfalse;
        }
    }

    if (download_valid && pure_valid)
    {
        // Check for download entries not on pure list
        for (i = 0; i < download_list.entry_count; ++i)
        {
            reference_list_entry_t *entry = &download_list.entries[i];
            if (!hash_in_reference_list(&pure_list, entry->hash))
            {
                Com_Printf(
                    "WARNING: Skipping download list pak '%s/%s' due to being missing"
                    " from pure list. Check to make sure all download manifest contents are also"
                    " included in the pure manifest.\n",
                    entry->mod_dir, entry->name);
                entry->disabled = qtrue;
            }
        }

        // Rebuild download strings to account for potential disabled entries from the previous check
        refstrings_free(&download_strings);
        download_strings = refstrings_generate(&download_list, MAX_DOWNLOAD_LIST_STRING);
    }

    // Write output cvars
    if (download_valid)
    {
        Cvar_Set("sv_referencedPaks", download_strings.hash.string);
        Cvar_Set("sv_referencedPakNames", download_strings.name.string);
    }
    if (pure_valid)
    {
        Cvar_Set("sv_paks", pure_strings.hash.string);
    }
    if (pure_names_valid)
    {
        Cvar_Set("sv_pakNames", pure_strings.name.string);
    }
    if (!pure_valid)
    {
        // This may not technically be necessary, since empty sv_paks should be sufficient
        // to make the server unpure, but set this as well for consistency
        Cvar_Set("sv_pure", "0");
    }

    // Update download map
    if (download_map)
        dlmap_free(download_map);
    download_map = 0;
    if (download_valid)
    {
        download_map = dlmap_generate(&download_list);
    }

    // Free temporary structures
    reflist_free(&download_list);
    reflist_free(&pure_list);
    refstrings_free(&download_strings);
    refstrings_free(&pure_strings);
}

/* ******************************************************************************** */
// Misc functions
/* ******************************************************************************** */

fileHandle_t fs_open_download_pak(const char *path, unsigned int *size_out)
{
    // Opens a pak on the server for a client UDP download
    if (download_map)
        return dlmap_open_pak(download_map, path, size_out);
    return 0;
}

#endif  // NEW_FILESYSTEM
