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
// Lookup Test Commands
/* ******************************************************************************** */

static void cmd_find_file(void) {
	if(Cmd_Argc() != 2) {
		Com_Printf("Usage: find_file <path>\n");
		return; }

	fs_general_lookup(Cmd_Argv(1), 0, qtrue); }

static void cmd_find_shader(void) {
	if(Cmd_Argc() != 2) {
		Com_Printf("Usage: find_shader <shader/image name>\n");
		return; }

	fs_shader_lookup(Cmd_Argv(1), 1, qtrue); }

static void cmd_find_sound(void) {
	if(Cmd_Argc() != 2) {
		Com_Printf("Usage: find_sound <sound name>\n");
		return; }

	fs_sound_lookup(Cmd_Argv(1), qtrue); }

static void cmd_find_vm(void) {
	if(Cmd_Argc() != 2) {
		Com_Printf("Usage: find_vm <vm/dll name>\n");
		return; }

	fs_vm_lookup(Cmd_Argv(1), qfalse, qtrue, 0); }

static void cmd_compare(void) {
	if(Cmd_Argc() != 3) {
		Com_Printf("Usage: compare <resource #> <resource #>\n\nRun this command following a 'find_file', 'find_shader', "
				"'find_sound', or 'find_vm' command and specify the resource numbers you wish to compare.\n");
		return; }

	debug_resource_comparison(atoi(Cmd_Argv(1)), atoi(Cmd_Argv(2))); }

/* ******************************************************************************** */
// Other Commands
/* ******************************************************************************** */

static void cmd_readcache_debug(void) {
	fs_readcache_debug(); }

static void cmd_indexcache_write(void) {
	fs_indexcache_write(); }

static void FS_Dir_f( void ) {
	const char	*path;
	const char	*extension;
	char	**dirnames;
	int		ndirs;
	int		i;

	if ( Cmd_Argc() < 2 || Cmd_Argc() > 3 ) {
		Com_Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( Cmd_Argc() == 2 ) {
		path = Cmd_Argv( 1 );
		extension = "";
	} else {
		path = Cmd_Argv( 1 );
		extension = Cmd_Argv( 2 );
	}

	Com_Printf( "Directory of %s %s\n", path, extension );
	Com_Printf( "---------------\n" );

	dirnames = FS_ListFiles( path, extension, &ndirs );

	for ( i = 0; i < ndirs; i++ ) {
		Com_Printf( "%s\n", dirnames[i] );
	}
	FS_FreeFileList( dirnames );
}

static void FS_NewDir_f( void ) {
	const char	*filter;
	char	**dirnames;
	int		ndirs;
	int		i;

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "usage: fdir <filter>\n" );
		Com_Printf( "example: fdir *q3dm*.bsp\n");
		return;
	}

	filter = Cmd_Argv( 1 );

	Com_Printf( "---------------\n" );

	dirnames = FS_ListFilteredFiles( "", "", filter, &ndirs, qfalse );

	for ( i = 0; i < ndirs; i++ ) {
		Com_Printf( "%s\n", dirnames[i] );
	}
	Com_Printf( "%d files listed\n", ndirs );
	FS_FreeFileList( dirnames );
}

static void FS_Which_f( void ) {
	// The lookup functions are more powerful, but this is kept for
	// users who are familiar with it
	const char *filename = Cmd_Argv(1);
	const fsc_file_t *file;

	if ( !filename[0] ) {
		Com_Printf( "Usage: which <file>\n" );
		return;
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	file = fs_general_lookup(filename, 0, qfalse);
	if(file) fs_print_file_location(file);
	else Com_Printf("File not found: \"%s\"\n", filename);
}

static void FS_TouchFile_f( void ) {
	fileHandle_t	f;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: touchFile <file>\n" );
		return;
	}

	FS_FOpenFileRead( Cmd_Argv( 1 ), &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

static void FS_Path_f( void ) {
	// Quick implementation without sorting
	fsc_hashtable_iterator_t hti;
	fsc_pk3_hash_map_entry_t *hash_entry;
	int sourceid;
	int i;

	for(sourceid=0; sourceid<FS_MAX_SOURCEDIRS; ++sourceid) {
		if(!fs_sourcedirs[sourceid].active) continue;
		Com_Printf("Looking in %s (%s)\n", fs_sourcedirs[sourceid].name, fs_sourcedirs[sourceid].path_cvar->string);

		for(i=0; i<fs.pk3_hash_lookup.bucket_count; ++i) {
			fsc_hashtable_open(&fs.pk3_hash_lookup, i, &hti);
			while((hash_entry = (fsc_pk3_hash_map_entry_t *)STACKPTR(fsc_hashtable_next(&hti)))) {
				const fsc_file_direct_t *pak = (const fsc_file_direct_t *)STACKPTR(hash_entry->pk3);
				if(pak->source_dir_id == sourceid && !fs_file_disabled((fsc_file_t *)pak, 0)) {
					char buffer[FS_FILE_BUFFER_SIZE];
					fs_file_to_buffer((fsc_file_t *)pak, buffer, sizeof(buffer), qfalse, qtrue, qfalse, qfalse);
					Com_Printf("%s (%i files)\n", buffer, pak->pk3_subfile_count);
					if(fs_connected_server_pure_state()) Com_Printf("    %son the pure list\n",
							pk3_list_lookup(&connected_server_pk3_list, pak->pk3_hash, qfalse) ? "" : "not "); } } } }

	Com_Printf("\n");
	fs_print_handle_list(); }

/* ******************************************************************************** */
// Command Register Function
/* ******************************************************************************** */

void fs_register_commands(void) {
	Cmd_AddCommand("find_file", cmd_find_file);
	Cmd_AddCommand("find_shader", cmd_find_shader);
	Cmd_AddCommand("find_sound", cmd_find_sound);
	Cmd_AddCommand("find_vm", cmd_find_vm);
	Cmd_AddCommand("compare", cmd_compare);

	Cmd_AddCommand("readcache_debug", cmd_readcache_debug);
	Cmd_AddCommand("indexcache_write", cmd_indexcache_write);

	Cmd_AddCommand("dir", FS_Dir_f);
	Cmd_AddCommand("fdir", FS_NewDir_f);
	Cmd_AddCommand("which", FS_Which_f);
	Cmd_AddCommand("touchfile", FS_TouchFile_f);
	Cmd_AddCommand("path", FS_Path_f); }

#endif	// NEW_FILESYSTEM
