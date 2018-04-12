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

#define ADD_STRING(string) fsc_stream_append_string(stream, string)
#define ADD_STRINGL(string) fsc_stream_append_string(&stream, string)

typedef struct {
	// File lookup
	const char *qp_name;	// null to disable file lookup
	const char *qp_dir;
	const char **qp_exts;
	int extension_count;

	// Shader lookup
	const char *shader_name;	// null to disable shader lookup

	// Lookup flags
	int lookup_flags;

	// Specialized query configurations
	qboolean dll_query;
	fs_config_type_t config_query;
} lookup_query_t;

#define RESFLAG_IN_DOWNLOAD_PK3 1
#define RESFLAG_IN_CURRENT_MAP_PAK 2
#define RESFLAG_FROM_DLL_QUERY 4
#define RESFLAG_CASE_MISMATCH 8

typedef struct {
	// This should contain only static data, i.e. no pointers that expire when the query
	// finishes, since it gets saved for debug queries
	const fsc_file_t *file;
	const fsc_shader_t *shader;
	int system_pak_priority;
	int server_pak_position;
	int extension_position;
	int mod_dir_match;	// 3 = current mod dir, 2 = 'basemod' dir, 1 = base dir, 0 = no match
	int flags;

	// Can be set to an error explanation to disable the resource during selection but still
	// have it show up in the precedence debug listings.
	const char *disabled;
} lookup_resource_t;

typedef struct {
	lookup_resource_t *resources;
	int resource_count;
	int resource_allocation;
} selection_output_t;

typedef struct {
	const fsc_file_t *file;
	const fsc_shader_t *shader;
} query_result_t;

/* ******************************************************************************** */
// Resource construction - Converts file or shader to lookup resource
/* ******************************************************************************** */

static void configure_lookup_resource(const lookup_query_t *query, lookup_resource_t *resource) {
	// Determine mod dir match level
	const char *resource_mod_dir = fsc_get_mod_dir(resource->file, &fs);
	resource->mod_dir_match = fs_get_mod_dir_state(resource_mod_dir);

	// Configure pk3-specific properties
	if(resource->file->sourcetype == FSC_SOURCETYPE_PK3) {
		const fsc_file_direct_t *source_pk3 =
				(const fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)(resource->file))->source_pk3);
		if(!(query->lookup_flags & LOOKUPFLAG_IGNORE_PURE_LIST))
			resource->server_pak_position = pk3_list_lookup(&connected_server_pk3_list, source_pk3->pk3_hash, qfalse);
		if(source_pk3->f.flags & FSC_FILEFLAG_DLPK3) resource->flags |= RESFLAG_IN_DOWNLOAD_PK3;

		if(resource->mod_dir_match <= 1) {
			// Don't sort system paks or the current map pak specially if they are part of a mod
			resource->system_pak_priority = system_pk3_position(source_pk3->pk3_hash);
			if(!(query->lookup_flags & LOOKUPFLAG_IGNORE_CURRENT_MAP) && source_pk3 == current_map_pk3)
				resource->flags |= RESFLAG_IN_CURRENT_MAP_PAK; } }

	// Check mod dir for case mismatched current or basegame directory
	if((!Q_stricmp(resource_mod_dir, FS_GetCurrentGameDir()) && strcmp(resource_mod_dir, FS_GetCurrentGameDir()))
			|| (!Q_stricmp(resource_mod_dir, com_basegame->string) && strcmp(resource_mod_dir, com_basegame->string))) {
		resource->flags |= RESFLAG_CASE_MISMATCH; }

	// Handle settings (e.g. q3config.cfg or autoexec.cfg) query
	if(query->config_query == FS_CONFIGTYPE_SETTINGS) {
		if(resource->file->sourcetype == FSC_SOURCETYPE_PK3) {
			resource->disabled = "settings config file can't be loaded from pk3"; }
		if(fs_mod_settings->integer && resource->mod_dir_match != 1 && resource->mod_dir_match != 3) {
			resource->disabled = "settings config file can only be loaded from com_basegame or current mod dir"; }
		if(!fs_mod_settings->integer && resource->mod_dir_match != 1) {
			resource->disabled = "settings config file can only be loaded from com_basegame dir"; } }

	// Dll query handling
	if(query->dll_query) {
		if(resource->file->sourcetype != FSC_SOURCETYPE_DIRECT) {
			resource->disabled = "dll files can only be loaded directly from disk"; }
		resource->flags |= RESFLAG_FROM_DLL_QUERY; }

	// Disable files according to lookupflag sourcetype restrictions
	if((query->lookup_flags & LOOKUPFLAG_DIRECT_SOURCE_ONLY) && resource->file->sourcetype != FSC_SOURCETYPE_DIRECT) {
		resource->disabled = "blocking file because query requested files directly on disk only"; }
	if((query->lookup_flags & LOOKUPFLAG_PK3_SOURCE_ONLY) && resource->file->sourcetype != FSC_SOURCETYPE_PK3) {
		resource->disabled = "blocking file because query requested files inside pk3s only"; }

	// Disable config files from download folder (qvm file restrictions are handled in perform_lookup)
	if(fs_restrict_dlfolder->integer && (resource->flags & RESFLAG_IN_DOWNLOAD_PK3) && (query->config_query)) {
		resource->disabled = "blocking config file in downloaded pk3 due to fs_restrict_dlfolder setting"; }

	// Disable files not on server pak list if connected to a pure server
	if(!resource->server_pak_position && fs_connected_server_pure_state() == 1 &&
			!(query->lookup_flags & LOOKUPFLAG_IGNORE_PURE_LIST) &&
			!((query->lookup_flags & LOOKUPFLAG_DIRECT_SOURCE_ALLOW_UNPURE) && resource->file->sourcetype == FSC_SOURCETYPE_DIRECT)) {
		resource->disabled = "connected to pure server and file is not on server pak list"; }

	// Run general file disabled check
	switch(fs_file_disabled(resource->file, 0)) {
		case FD_RESULT_FILE_ENABLED:
			break;
		case FD_RESULT_INACTIVE_MOD_BLOCKED:
			resource->disabled = "blocking file from inactive mod dir due to fs_search_inactive_mods setting";
			break;
		default:
			// Shouldn't happen
			resource->disabled = "blocking file due to unexpected fs_file_disabled result"; } }

static void file_to_lookup_resource(const lookup_query_t *query, const fsc_file_t *file,
			int extension_index, qboolean case_mismatch, lookup_resource_t *resource) {
	fsc_memset(resource, 0, sizeof(*resource));
	resource->file = file;
	resource->shader = 0;
	resource->extension_position = extension_index;
	if(case_mismatch) resource->flags |= RESFLAG_CASE_MISMATCH;
	configure_lookup_resource(query, resource); }

static void shader_to_lookup_resource(const lookup_query_t *query, fsc_shader_t *shader,
			lookup_resource_t *resource) {
	fsc_memset(resource, 0, sizeof(*resource));
	resource->file = (const fsc_file_t *)STACKPTR(shader->source_file_ptr);
	resource->shader = shader;
	configure_lookup_resource(query, resource); }

/* ******************************************************************************** */
// Selection - Generates set of lookup resources for given query
/* ******************************************************************************** */

/* *** Selection Output Structure *** */

static void initialize_selection_output(selection_output_t *output) {
	// Initialize output structure
	fsc_memset(output, 0, sizeof(*output));
	output->resources = (lookup_resource_t *)Z_Malloc(20 * sizeof(*output->resources));
	output->resource_count = 0;
	output->resource_allocation = 20; }

static void free_selection_output(selection_output_t *output) {
	Z_Free(output->resources); }

static lookup_resource_t *allocate_lookup_resource(selection_output_t *output) {
	if(output->resource_count >= output->resource_allocation) {
		// No more slots - double the allocation size
		lookup_resource_t *new_resources = (lookup_resource_t *)Z_Malloc(output->resource_count * 2 * sizeof(*output->resources));
		fsc_memcpy(new_resources, output->resources, output->resource_count * sizeof(*output->resources));
		Z_Free(output->resources);
		output->resources = new_resources;
		output->resource_allocation = output->resource_count * 2; }
	return &output->resources[output->resource_count++]; }

/* *** File Selection *** */

static qboolean string_match(const char *string1, const char *string2, qboolean *case_mismatch_out) {
	if(!string1 || !string2) return string1 == string2 ? qtrue : qfalse;
	if(!Q_stricmp(string1, string2)) {
		if(!*case_mismatch_out && strcmp(string1, string2)) *case_mismatch_out = qtrue;
		return qtrue; }
	return qfalse; }

static int is_file_selected(const fsc_file_t *file, const lookup_query_t *query, int *extension_index_out, qboolean *case_mismatch_out) {
	// Returns 1 if selected, 0 otherwise
	int i;
	if(!fsc_is_file_enabled(file, &fs)) return 0;
	if(!string_match(query->qp_name, (const char *)STACKPTR(file->qp_name_ptr), case_mismatch_out)) return 0;
	if(!string_match(query->qp_dir, (const char *)STACKPTR(file->qp_dir_ptr), case_mismatch_out)) return 0;

	if(!query->extension_count) return 1;
	for(i=0; i<query->extension_count; ++i) {
		if(string_match(query->qp_exts[i], (const char *)STACKPTR(file->qp_ext_ptr), case_mismatch_out)) {
			*extension_index_out = i + 1;
			return 1; } }
	return 0; }

static void perform_file_selection(const lookup_query_t *query, selection_output_t *output) {
	// Adds files matching criteria to output
	fsc_hashtable_iterator_t hti;
	fsc_stackptr_t file_ptr;

	fsc_hashtable_open(&fs.files, fsc_string_hash(query->qp_name, query->qp_dir), &hti);
	while((file_ptr = fsc_hashtable_next(&hti))) {
		const fsc_file_t *file = (const fsc_file_t *)STACKPTR(file_ptr);
		int extension_index = 0;
		qboolean case_mismatch = qfalse;
		if(is_file_selected(file, query, &extension_index, &case_mismatch)) {
			lookup_resource_t *resource = allocate_lookup_resource(output);
			file_to_lookup_resource(query, file, extension_index, case_mismatch, resource); } } }

/* *** Shader Selection *** */

static int is_shader_selected(fsc_stackptr_t shader_name_ptr, fsc_shader_t *shader) {
	if(shader->shader_name_ptr != shader_name_ptr) return 0;
	if(!fsc_is_file_enabled((const fsc_file_t *)STACKPTR(shader->source_file_ptr), &fs)) return 0;
	return 1; }

static void perform_shader_selection(const lookup_query_t *query, selection_output_t *output) {
	// Adds shaders matching criteria to output
	char shader_name_lower[FSC_MAX_SHADER_NAME];
	fsc_stackptr_t shader_name_ptr;
	fsc_hashtable_iterator_t hti;
	fsc_stackptr_t shader_ptr;
	fsc_shader_t *shader;
	lookup_resource_t *resource;

	// Get lowercase shader_name. If it's not in string repository, we know shader doesn't exist.
	fsc_strncpy_lower(shader_name_lower, query->shader_name, sizeof(shader_name_lower));
	shader_name_ptr = fsc_string_repository_getstring(shader_name_lower, 0, &fs.string_repository, &fs.general_stack);
	if(!shader_name_ptr) return;

	fsc_hashtable_open(&fs.shaders, fsc_string_hash(shader_name_lower, 0), &hti);
	while((shader_ptr = fsc_hashtable_next(&hti))) {
		shader = (fsc_shader_t *)STACKPTR(shader_ptr);
		if(is_shader_selected(shader_name_ptr, shader)) {
			resource = allocate_lookup_resource(output);
			shader_to_lookup_resource(query, shader, resource); } } }

/* *** Selection Main *** */

void perform_selection(const lookup_query_t *query, selection_output_t *output) {
	// Caller is responsible for calling initialize_selection_output beforehand,
	//   and free_selection_output afterwards.
	if(query->shader_name) perform_shader_selection(query, output);
	if(query->qp_name) perform_file_selection(query, output); }

/* ******************************************************************************** */
// Precedence - Used to select best lookup resource from set of lookup resources
/* ******************************************************************************** */

/* *** Support Functions *** */

static void resource_to_stream(const lookup_resource_t *resource, fsc_stream_t *stream) {
	fs_file_to_stream(resource->file, stream, qtrue, qtrue, qtrue, resource->shader ? qfalse : qtrue);

	if(resource->shader) {
		ADD_STRING("->");
		ADD_STRING((const char *)STACKPTR(resource->shader->shader_name_ptr)); } }

/* *** Precedence Rule Definitions *** */

#define PC_COMPARE(check_name) static int pc_cmp_ ## check_name(const lookup_resource_t *r1, const lookup_resource_t *r2)

#define PC_DEBUG(check_name) static void pc_dbg_ ## check_name(const lookup_resource_t *high, const lookup_resource_t *low, \
		int high_num, int low_num, fsc_stream_t *stream)

// Return -1 if r1 has higher precedence, 1 if r2 has higher precedence, 0 if neither

PC_COMPARE(resource_disabled) {
	if(r1->disabled && !r2->disabled) return 1;
	if(r2->disabled && !r1->disabled) return -1;
	return 0; }

PC_DEBUG(resource_disabled) {
	ADD_STRING(va("Resource %i was selected because resource %i is disabled: %s", high_num, low_num, low->disabled)); }

PC_COMPARE(special_shaders) {
	qboolean r1_special = (r1->shader &&
			(r1->mod_dir_match > 1 || r1->system_pak_priority || r1->server_pak_position)) ? qtrue : qfalse;
	qboolean r2_special = (r2->shader &&
			(r2->mod_dir_match > 1 || r2->system_pak_priority || r2->server_pak_position)) ? qtrue : qfalse;

	if(r1_special && !r2_special) return -1;
	if(r2_special && !r1_special) return 1;

	return 0; }

PC_DEBUG(special_shaders) {
	ADD_STRING(va("Resource %i was selected because it is classified as a special shader (from a system pak, the server pak list,"
		" the current mod dir, or the basemod dir) and resource %i is not.", high_num, low_num)); }

PC_COMPARE(server_pak_position) {
	if(r1->server_pak_position && !r2->server_pak_position) return -1;
	if(r2->server_pak_position && !r1->server_pak_position) return 1;

	if(r1->server_pak_position < r2->server_pak_position) return -1;
	if(r2->server_pak_position < r1->server_pak_position) return 1;
	return 0; }

PC_DEBUG(server_pak_position) {
	if(!low->server_pak_position) {
		ADD_STRING(va("Resource %i was selected because it is on the server pak list and resource %i is not.", high_num, low_num)); }
	else {
		ADD_STRING(va("Resource %i was selected because it has a lower server pak position (%i) than resource %i (%i).",
				high_num, high->server_pak_position, low_num, low->server_pak_position)); } }

PC_COMPARE(basemod_or_current_mod_dir) {
	if(r1->mod_dir_match >= 2 || r2->mod_dir_match >= 2) {
		if(r1->mod_dir_match > r2->mod_dir_match) return -1;
		if(r2->mod_dir_match > r1->mod_dir_match) return 1; }
	return 0; }

PC_DEBUG(basemod_or_current_mod_dir) {
	ADD_STRING(va("Resource %i was selected because it is from ", high_num));
	if(high->mod_dir_match == 3) {
		ADD_STRING(va("the current mod directory (%s)", fsc_get_mod_dir(high->file, &fs))); }
	else {
		ADD_STRING("the 'basemod' directory"); }
	ADD_STRING(va(" and resource %i is not. ", low_num)); }

PC_COMPARE(system_paks) {
	if(r1->system_pak_priority > r2->system_pak_priority) return -1;
	if(r2->system_pak_priority > r1->system_pak_priority) return 1;
	return 0; }

PC_DEBUG(system_paks) {
	ADD_STRING(va("Resource %i was selected because it has a higher system pak rank than resource %i.", high_num, low_num)); }

PC_COMPARE(current_map_pak) {
	if((r1->flags & RESFLAG_IN_CURRENT_MAP_PAK) && !(r2->flags & RESFLAG_IN_CURRENT_MAP_PAK)) return -1;
	if((r2->flags & RESFLAG_IN_CURRENT_MAP_PAK) && !(r1->flags & RESFLAG_IN_CURRENT_MAP_PAK)) return 1;
	return 0; }

PC_DEBUG(current_map_pak) {
	ADD_STRING(va("Resource %i was selected because it is from the same pk3 as the current map and %i is not.", high_num, low_num)); }

PC_COMPARE(inactive_mod_dir) {
	if(r1->mod_dir_match && !r2->mod_dir_match) return -1;
	if(r2->mod_dir_match && !r1->mod_dir_match) return 1;
	return 0; }

PC_DEBUG(inactive_mod_dir) {
	ADD_STRING(va("Resource %i was selected because resource %i is from an inactive mod directory "
			"(not basegame, basemod, or current mod).", high_num, low_num)); }

PC_COMPARE(downloads_folder) {
	if(!(r1->flags & RESFLAG_IN_DOWNLOAD_PK3) && (r2->flags & RESFLAG_IN_DOWNLOAD_PK3)) return -1;
	if(!(r2->flags & RESFLAG_IN_DOWNLOAD_PK3) && (r1->flags & RESFLAG_IN_DOWNLOAD_PK3)) return 1;
	return 0; }

PC_DEBUG(downloads_folder) {
	ADD_STRING(va("Resource %i was selected because resource %i is in the downloads folder and resource %i is not.",
			high_num, low_num, high_num)); }

PC_COMPARE(shader_over_image) {
	if(r1->shader && !r2->shader) return -1;
	if(r2->shader && !r1->shader) return 1;
	return 0; }

PC_DEBUG(shader_over_image) {
	ADD_STRING(va("Resource %i was selected because it is a shader and resource %i is not a shader.", high_num, low_num)); }

PC_COMPARE(dll_over_qvm) {
	#define QUALIFYING_DLL(resource) (resource->flags & RESFLAG_FROM_DLL_QUERY)
	if(QUALIFYING_DLL(r1) && !QUALIFYING_DLL(r2)) return -1;
	if(QUALIFYING_DLL(r2) && !QUALIFYING_DLL(r1)) return 1;
	return 0; }

PC_DEBUG(dll_over_qvm) {
	ADD_STRING(va("Resource %i was selected because it is a dll and resource %i is not a dll.", high_num, low_num)); }

PC_COMPARE(direct_over_pk3) {
	#define PK3_LIKE_FILE(file) (file->sourcetype == FSC_SOURCETYPE_PK3 || \
			(file->sourcetype == FSC_SOURCETYPE_DIRECT && ((fsc_file_direct_t *)file)->pk3dir_ptr))
	if(!PK3_LIKE_FILE(r1->file) && PK3_LIKE_FILE(r2->file)) return -1;
	if(!PK3_LIKE_FILE(r2->file) && PK3_LIKE_FILE(r1->file)) return 1;
	return 0; }

PC_DEBUG(direct_over_pk3) {
	ADD_STRING(va("Resource %i was selected because it is a file directly on the disk, while resource %i is inside a pk3.",
			high_num, low_num)); }

PC_COMPARE(pk3_name_precedence) {
	return fs_compare_pk3_source(r1->file, r2->file); }

PC_DEBUG(pk3_name_precedence) {
	ADD_STRING(va("Resource %i was selected because the pk3 containing it has lexicographically higher precedence "
		"than the pk3 containing resource %i.", high_num, low_num)); }

PC_COMPARE(extension_precedence) {
	if(r1->extension_position < r2->extension_position) return -1;
	if(r2->extension_position < r1->extension_position) return 1;
	return 0; }

PC_DEBUG(extension_precedence) {
	ADD_STRING(va("Resource %i was selected because its extension has a higher precedence than "
		"the extension of resource %i.", high_num, low_num)); }

PC_COMPARE(source_dir_precedence) {
	int r1_id = fs_get_source_dir_id(r1->file);
	int r2_id = fs_get_source_dir_id(r2->file);
	if(r1_id < r2_id) return -1;
	if(r2_id < r1_id) return 1;
	return 0; }

PC_DEBUG(source_dir_precedence) {
	ADD_STRING(va("Resource %i was selected because it is from a higher precedence source directory (%s) than resource %i (%s)",
			high_num, fs_get_source_dir_string(high->file), low_num, fs_get_source_dir_string(low->file))); }

PC_COMPARE(intra_pk3_position) {
	if(r1->file->sourcetype != FSC_SOURCETYPE_PK3 || r2->file->sourcetype != FSC_SOURCETYPE_PK3) return 0;
	if(((fsc_file_frompk3_t *)r1->file)->header_position > ((fsc_file_frompk3_t *)r2->file)->header_position) return -1;
	if(((fsc_file_frompk3_t *)r2->file)->header_position > ((fsc_file_frompk3_t *)r1->file)->header_position) return 1;
	return 0; }

PC_DEBUG(intra_pk3_position) {
	ADD_STRING(va("Resource %i was selected because it has a later position within the pk3 file than resource %i.",
			high_num, low_num)); }

PC_COMPARE(intra_shaderfile_position) {
	if(!r1->shader || !r2->shader) return 0;
	if(r1->shader->start_position < r2->shader->start_position) return -1;
	if(r2->shader->start_position < r1->shader->start_position) return 1;
	return 0; }

PC_DEBUG(intra_shaderfile_position) {
	ADD_STRING(va("Resource %i was selected because it has an earlier position within the shader file than resource %i.",
			high_num, low_num)); }

PC_COMPARE(case_match) {
	if(!(r1->flags & RESFLAG_CASE_MISMATCH) && (r2->flags & RESFLAG_CASE_MISMATCH)) return -1;
	if(!(r2->flags & RESFLAG_CASE_MISMATCH) && (r1->flags & RESFLAG_CASE_MISMATCH)) return 1;
	return 0; }

PC_DEBUG(case_match) {
	ADD_STRING(va("Resource %i was selected because resource %i has a case discrepancy from the query and resource %i does not.",
			high_num, low_num, high_num)); }

/* *** Precedence List & Sorting *** */

typedef struct {
	const char *identifier;
	int (*comparator)(const lookup_resource_t *r1, const lookup_resource_t *r2);
	void (*debugfunction)(const lookup_resource_t *high, const lookup_resource_t *low, int high_num, int low_num,
				fsc_stream_t *stream);
} precedence_check_t;

#define ADD_CHECK(check) { #check, pc_cmp_ ## check, pc_dbg_ ## check }
static const precedence_check_t precedence_checks[] = {
	ADD_CHECK(resource_disabled),
	ADD_CHECK(special_shaders),
	ADD_CHECK(server_pak_position),
	ADD_CHECK(basemod_or_current_mod_dir),
	ADD_CHECK(system_paks),
	ADD_CHECK(current_map_pak),
	ADD_CHECK(inactive_mod_dir),
	ADD_CHECK(downloads_folder),
	ADD_CHECK(shader_over_image),
	ADD_CHECK(dll_over_qvm),
	ADD_CHECK(direct_over_pk3),
	ADD_CHECK(pk3_name_precedence),
	ADD_CHECK(extension_precedence),
	ADD_CHECK(source_dir_precedence),
	ADD_CHECK(intra_pk3_position),
	ADD_CHECK(intra_shaderfile_position),
	ADD_CHECK(case_match) };

static int precedence_comparator(const lookup_resource_t *resource1, const lookup_resource_t *resource2) {
	int i;
	int result;
	for(i=0; i<ARRAY_LEN(precedence_checks); ++i) {
		result = precedence_checks[i].comparator(resource1, resource2);
		if(result) return result; }
	// Use memory address as comparison of last resort
	return resource1 < resource2 ? -1 : 1; }

static int precedence_comparator_qsort(const void *e1, const void *e2) {
	return precedence_comparator((const lookup_resource_t *)e1, (const lookup_resource_t *)e2); }

static void selection_sort(selection_output_t *selection_output) {
	qsort(selection_output->resources, selection_output->resource_count, sizeof(*selection_output->resources),
			precedence_comparator_qsort); }

/* ******************************************************************************** */
// Query processing - Runs selection/precedence oprations for a given lookup query
/* ******************************************************************************** */

/* *** Standard Lookup *** */

static void perform_lookup(const lookup_query_t *queries, int query_count, qboolean protected_vm_lookup,
		query_result_t *output) {
	int i;
	selection_output_t selection_output;
	lookup_resource_t *best_resource = 0;

	// Start with empty output
	Com_Memset(output, 0, sizeof(*output));

	// Perform selection
	initialize_selection_output(&selection_output);
	for(i=0; i<query_count; ++i) {
		perform_selection(&queries[i], &selection_output); }

	// If selection didn't receive anything, leave the output file null to indicate failure
	if(!selection_output.resource_count) {
		free_selection_output(&selection_output);
		return; }

	if(protected_vm_lookup && fs_restrict_dlfolder->integer) {
		// Select the first resource that meets download folder restriction requirements
		selection_sort(&selection_output);
		for(i=0; i<selection_output.resource_count; ++i) {
			if(selection_output.resources[i].flags & RESFLAG_IN_DOWNLOAD_PK3) {
				char buffer[FS_FILE_BUFFER_SIZE];
				qboolean trusted_hash = fs_check_trusted_vm_file(selection_output.resources[i].file);
				if(!trusted_hash) {
					// Raise error if connected to pure server, otherwise just print a warning and skip
					fs_file_to_buffer(selection_output.resources[i].file, buffer, sizeof(buffer),
							qtrue, qtrue, qtrue, qfalse);
					if(!com_sv_running->integer && fs_connected_server_pure_state()) {
						free_selection_output(&selection_output);
						Com_Error(ERR_DROP, "QVM file %s has an untrusted hash and was blocked due to your"
								" fs_restrict_dlfolder setting. To remedy this, you may either:"
								"\n- Move the pk3 out of the downloads folder. If you don't trust the server"
								" you downloaded this file from, this may be a security risk."
								"\n- Set fs_restrict_dlfolder to 0 to allow running all QVMs from"
								" downloaded pk3s. This may lead to increased security risks.", buffer); }
					Com_Printf("^3WARNING: QVM file %s has an untrusted hash and was blocked due to your"
							" fs_restrict_dlfolder setting.\n", buffer);
					continue; }
				else if(fs_restrict_dlfolder->integer != 1) {
					// Raise error if connected to pure server, otherwise just print a warning and skip
					fs_file_to_buffer(selection_output.resources[i].file, buffer, sizeof(buffer),
							qtrue, qtrue, qtrue, qfalse);
					if(!com_sv_running->integer && fs_connected_server_pure_state()) {
						free_selection_output(&selection_output);
						Com_Error(ERR_DROP, "QVM file %s has a trusted hash but was blocked due to your"
								" fs_restrict_dlfolder setting. To remedy this, you may either:"
								"\n- Move the pk3 out of the downloads folder. If you don't trust the server"
								" you downloaded this file from, this may be a security risk."
								"\n- Set fs_restrict_dlfolder to 1 to allow running trusted QVMs from"
								" downloaded pk3s.", buffer); }
					Com_Printf("^3WARNING: QVM file %s has a trusted hash but was blocked due to your"
							" fs_restrict_dlfolder setting.\n", buffer);
					continue; } }

			// Have non-blocked file
			best_resource = &selection_output.resources[i];
			break; } }
	else {
		// Standard lookup; just pick the top resource
		best_resource = &selection_output.resources[0];
		for(i=1; i<selection_output.resource_count; ++i) {
			if(precedence_comparator(best_resource, &selection_output.resources[i]) > 0) {
				best_resource = &selection_output.resources[i]; } } }

	if(best_resource && !best_resource->disabled) {
		output->file = best_resource->file;
		output->shader = best_resource->shader; }

	free_selection_output(&selection_output); }

/* *** Debug Query Storage *** */

static qboolean have_debug_selection = qfalse;
static selection_output_t debug_selection;

/* *** Debug Lookup *** */

static void debug_lookup(const lookup_query_t *queries, int query_count, qboolean protected_vm_lookup) {
	int i;

	// Set global state
	if(have_debug_selection) free_selection_output(&debug_selection);
	initialize_selection_output(&debug_selection);

	// Perform selection
	for(i=0; i<query_count; ++i) {
		perform_selection(&queries[i], &debug_selection); }
	selection_sort(&debug_selection);
	have_debug_selection = qtrue;

	// Print element data
	for(i=0; i<debug_selection.resource_count; ++i) {
		char buffer[2048];
		fsc_stream_t stream = {buffer, 0, sizeof(buffer), 0};

		ADD_STRINGL(va("   Element %i: ", i+1));
		resource_to_stream(&debug_selection.resources[i], &stream);

		if(protected_vm_lookup) {
			// Print extra hash data
			unsigned char hash[32];
			calculate_file_sha256(debug_selection.resources[i].file, hash);
			ADD_STRINGL("\nhash: ");
			sha256_to_stream(hash, &stream);

			if(debug_selection.resources[i].file->qp_ext_ptr &&
					!Q_stricmp((const char *)STACKPTR(debug_selection.resources[i].file->qp_ext_ptr), "qvm")) {
				ADD_STRINGL(va("\ntrusted: %s", fs_check_trusted_vm_hash(hash) ? "yes" :
						"no; blocked in download folder if fs_restrict_dlfolder set")); } }

		Com_Printf("%s\n\n", buffer); }

	if(!debug_selection.resource_count) {
		Com_Printf("No matching resources found.\n"); }
	else if(debug_selection.resources[0].disabled) {
		Com_Printf("No resource was selected because element 1 is disabled: %s\n",
				debug_selection.resources[0].disabled); } }

/* *** Debug Comparison - Compares two resources, lists each test and result, and debug info for decisive test *** */

static void debug_resource_comparator(const lookup_resource_t *resource1, const lookup_resource_t *resource2,
			int resource1_num, int resource2_num, fsc_stream_t *stream) {
	int i;
	int result;
	int decisive_result = 0;
	int decisive_test = -1;
	int length;

	ADD_STRING("Check                           Result\n");
	ADD_STRING("------------------------------- ---------\n");

	for(i=0; i<ARRAY_LEN(precedence_checks); ++i) {
		// Write check name and spaces
		ADD_STRING(precedence_checks[i].identifier);
		length = strlen(precedence_checks[i].identifier);
		while(length++ < 32) ADD_STRING(" ");

		// Get result string
		result = precedence_checks[i].comparator(resource1, resource2);
		if(result && !decisive_result) {
			decisive_result = result;
			decisive_test = i; }

		// Write result identifier
		if(result < 0) ADD_STRING(va("resource %i", resource1_num));
		else if(result > 0) ADD_STRING(va("resource %i", resource2_num));
		else ADD_STRING("---");
		ADD_STRING("\n"); }

	if(decisive_result) {
		const lookup_resource_t *high_resource, *low_resource;
		int high_num, low_num;
		if(decisive_result < 0) {
			high_resource = resource1; low_resource = resource2;
			high_num = resource1_num; low_num = resource2_num; }
		else {
			high_resource = resource2; low_resource = resource1;
			high_num = resource2_num; low_num = resource1_num; }

		ADD_STRING("\n");
		precedence_checks[decisive_test].debugfunction(high_resource, low_resource, high_num, low_num, stream); } }

void debug_resource_comparison(int resource1_position, int resource2_position) {
	// Uses data from a previous lookup command (stored in debug_selection global variable)
	// Input is the index (resource #) of two resources from that lookup
	char data[65000];
	fsc_stream_t stream = {data, 0, sizeof(data), 0};

	if(!have_debug_selection) {
		Com_Printf("This command must be preceded by a 'find_file', 'find_shader', 'find_sound', or 'find_vm' command.\n");
		return; }

	if(resource1_position == resource2_position || resource1_position < 1 || resource2_position < 1 ||
			resource1_position > debug_selection.resource_count || resource2_position > debug_selection.resource_count) {
		Com_Printf("Resource numbers out of range.\n");
		return; }

	debug_resource_comparator(&debug_selection.resources[resource1_position-1], &debug_selection.resources[resource2_position-1],
			resource1_position, resource2_position, &stream);

	Com_Printf("%s\n", stream.data); }

/* ******************************************************************************** */
// Wrapper functions - Generates query and calls query handling functions
/* ******************************************************************************** */

static void lookup_print_debug_file(const fsc_file_t *file) {
	if(file) {
		char buffer[FS_FILE_BUFFER_SIZE];
		fs_file_to_buffer(file, buffer, sizeof(buffer), qtrue, qtrue, qtrue, qfalse);
		Com_Printf("result: %s\n", buffer); }
	else {
		Com_Printf("result: <not found>\n"); } }

const fsc_file_t *fs_general_lookup(const char *name, int lookup_flags, qboolean debug) {
	lookup_query_t query;
	char qpath_buffer[FSC_MAX_QPATH];
	const char *ext = 0;
	query_result_t lookup_result;

	Com_Memset(&query, 0, sizeof(query));
	query.lookup_flags = lookup_flags;

	// For compatibility purposes, support dropping one leading slash from qpath
	// as per FS_FOpenFileReadDir in original filesystem
	if(*name == '/' || *name == '\\') ++name;

	fsc_process_qpath(name, qpath_buffer, &query.qp_dir, &query.qp_name, &ext);
	query.qp_exts = &ext;
	query.extension_count = ext ? 1 : 0;

	if(debug) {
		debug_lookup(&query, 1, qfalse);
		return 0; }

	perform_lookup(&query, 1, qfalse, &lookup_result);
	if(fs_debug_lookup->integer) {
		Com_Printf("********** general lookup **********\n");
		Com_Printf("name: %s\nignore_pure_list: %s\nignore_current_map: %s\n", name,
			(lookup_flags & LOOKUPFLAG_IGNORE_PURE_LIST) ? "true" : "false",
			(lookup_flags & LOOKUPFLAG_IGNORE_CURRENT_MAP) ? "true" : "false");
		lookup_print_debug_file(lookup_result.file); }
	return lookup_result.file; }

static void shader_or_image_lookup(const char *name, qboolean image_only, int lookup_flags,
			query_result_t *output, qboolean debug) {
	// Input name should be extension-free (call COM_StripExtension first)
	lookup_query_t query;
	char qpath_buffer[FSC_MAX_QPATH];
	const char *exts[] = {"dds", "png", "tga", "jpg", "jpeg", "pcx", "bmp"};

	Com_Memset(&query, 0, sizeof(query));
	query.lookup_flags = lookup_flags;
	if(!image_only) query.shader_name = (char *)name;	// const cast!

	// For compatibility purposes, support dropping one leading slash from qpath
	// as per FS_FOpenFileReadDir in original filesystem
	if(*name == '/' || *name == '\\') ++name;

	fsc_process_qpath(name, qpath_buffer, &query.qp_dir, &query.qp_name, 0);
	query.qp_exts = (lookup_flags & LOOKUPFLAG_ENABLE_DDS) ? exts : exts + 1;
	query.extension_count = (lookup_flags & LOOKUPFLAG_ENABLE_DDS) ? ARRAY_LEN(exts) : ARRAY_LEN(exts) - 1;

	if(debug) debug_lookup(&query, 1, qfalse);
	else perform_lookup(&query, 1, qfalse, output); }

const fsc_shader_t *fs_shader_lookup(const char *name, int lookup_flags, qboolean debug) {
	// Input name should be extension-free (call COM_StripExtension first)
	// Returns null if shader not found or image took precedence
	query_result_t lookup_result;
	shader_or_image_lookup(name, qfalse, lookup_flags, &lookup_result, debug);
	if(debug) return 0;
	if(fs_debug_lookup->integer) {
		Com_Printf("********** shader lookup **********\n");
		Com_Printf("name: %s\n", name);
		lookup_print_debug_file(lookup_result.file); }
	return lookup_result.shader; }

const fsc_file_t *fs_image_lookup(const char *name, int lookup_flags, qboolean debug) {
	// Input name should be extension-free (call COM_StripExtension first)
	query_result_t lookup_result;
	shader_or_image_lookup(name, qtrue, lookup_flags, &lookup_result, debug);
	if(debug) return 0;
	if(fs_debug_lookup->integer) {
		Com_Printf("********** image lookup **********\n");
		Com_Printf("name: %s\n", name);
		lookup_print_debug_file(lookup_result.file); }
	return lookup_result.file; }

const fsc_file_t *fs_sound_lookup(const char *name, qboolean debug) {
	// Input name should be extension-free (call COM_StripExtension first)
	lookup_query_t query;
	char qpath_buffer[FSC_MAX_QPATH];
	const char *exts[] = {
		"wav",
#ifdef USE_CODEC_VORBIS
		"ogg",
#endif
#ifdef USE_CODEC_OPUS
		"opus",
#endif
	};
	query_result_t lookup_result;

	// For compatibility purposes, support dropping one leading slash from qpath
	// as per FS_FOpenFileReadDir in original filesystem
	if(*name == '/' || *name == '\\') ++name;

	Com_Memset(&query, 0, sizeof(query));
	fsc_process_qpath(name, qpath_buffer, &query.qp_dir, &query.qp_name, 0);
	query.qp_exts = exts;
	query.extension_count = ARRAY_LEN(exts);

	if(debug) {
		debug_lookup(&query, 1, qfalse);
		return 0; }

	perform_lookup(&query, 1, qfalse, &lookup_result);
	if(fs_debug_lookup->integer) {
		Com_Printf("********** sound lookup **********\n");
		Com_Printf("name: %s\n", name);
		lookup_print_debug_file(lookup_result.file); }
	return lookup_result.file; }

const fsc_file_t *fs_vm_lookup(const char *name, qboolean qvm_only, qboolean debug, qboolean *is_dll_out) {
	// Returns a qvm or game dll file, or null if not found
	// May throw ERR_DROP due to fs_restrict_dlfolder checks
	const char *qvm_exts[] = {"qvm"};
	const char *dll_exts[] = {DLL_EXT+1};
	lookup_query_t queries[2];
	int query_count = qvm_only ? 1 : 2;
	char qpath_buffers[2][FSC_MAX_QPATH];
	query_result_t lookup_result;

	Com_Memset(queries, 0, sizeof(queries));
	queries[0].lookup_flags = LOOKUPFLAG_IGNORE_CURRENT_MAP;
	fsc_process_qpath(va("vm/%s", name), qpath_buffers[0], &queries[0].qp_dir, &queries[0].qp_name, 0);
	queries[0].qp_exts = qvm_exts;
	queries[0].extension_count = ARRAY_LEN(qvm_exts);

	if(!qvm_only) {
		queries[1].lookup_flags = LOOKUPFLAG_IGNORE_CURRENT_MAP;
		fsc_process_qpath(va("%s" ARCH_STRING, name), qpath_buffers[1], &queries[1].qp_dir, &queries[1].qp_name, 0);
		queries[1].qp_exts = dll_exts;
		queries[1].extension_count = ARRAY_LEN(dll_exts);
		queries[1].dll_query = qtrue; }

	if(debug) {
		debug_lookup(queries, query_count, qtrue);
		return 0; }

	perform_lookup(queries, query_count, qtrue, &lookup_result);
	if(fs_debug_lookup->integer) {
		Com_Printf("********** dll/qvm lookup **********\n");
		Com_Printf("name: %s\n", name);
		Com_Printf("qvm only: %s\n", qvm_only ? "yes" : "no");
		lookup_print_debug_file(lookup_result.file); }

	// Not elegant but should be adequate
	if(is_dll_out) *is_dll_out = lookup_result.file &&
			!Q_stricmp((const char *)STACKPTR(lookup_result.file->qp_ext_ptr), DLL_EXT+1) ? qtrue : qfalse;
	return lookup_result.file; }

const fsc_file_t *fs_config_lookup(const char *name, fs_config_type_t type, qboolean debug) {
	lookup_query_t query;
	char qpath_buffer[FSC_MAX_QPATH];
	const char *ext = 0;
	query_result_t lookup_result;

	Com_Memset(&query, 0, sizeof(query));
	query.lookup_flags = LOOKUPFLAG_IGNORE_CURRENT_MAP;
	if(type != FS_CONFIGTYPE_DEFAULT) query.lookup_flags |= LOOKUPFLAG_IGNORE_PURE_LIST;
	fsc_process_qpath(name, qpath_buffer, &query.qp_dir, &query.qp_name, &ext);
	query.qp_exts = &ext;
	query.extension_count = ext ? 1 : 0;
	query.config_query = type;

	if(debug) {
		debug_lookup(&query, 1, qfalse);
		return 0; }

	perform_lookup(&query, 1, qfalse, &lookup_result);
	if(fs_debug_lookup->integer) {
		Com_Printf("********** config lookup **********\n");
		Com_Printf("name: %s\n", name);
		lookup_print_debug_file(lookup_result.file); }
	return lookup_result.file; }

#endif	// NEW_FILESYSTEM
