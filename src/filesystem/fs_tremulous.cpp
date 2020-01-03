/*
   Copyright (C) 2016 Victor Roemer (wtfbbqhax), <victor@badsec.org>.
   Copyright (C) 2000-2013 Darklegion Development
   Copyright (C) 1999-2005 Id Software, Inc.
   Copyright (C) 2015-2019 GrangerHub

   This file is part of Tremulous.

   Tremulous is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3 of the License,
   or (at your option) any later version.

   Tremulous is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Tremulous; if not, see <https://www.gnu.org/licenses/>
*/

#ifdef NEW_FILESYSTEM
#include "fslocal.h"

// This file is used for some Tremulous-specific filesystem functions

/* ******************************************************************************** */
// System file manager functions
/* ******************************************************************************** */

/*
============
FS_OpenWithDefault

Wrapper for Sys_OpenWithDefault()
============
*/
static bool FS_OpenWithDefault( const char *path )
{
    if( Sys_OpenWithDefault( path ) )
    {
        // minimize the client's window
        Cmd_ExecuteString( "minimize" );
        return true;
    }

    return false;
}

/*
============
FS_BrowseHomepath

Opens the highest priority source directory (sourcedir 0) in default file manager
This directory will be the write directory (unless in read-only mode) and will contain any
   updated config files, log files, etc. It may or may not actually be the homepath depending
   on the configuration of the fs_dirs cvar.
============
*/
bool FS_BrowseHomepath( void )
{
    char path[FS_MAX_PATH];
    if(fs_generate_path_sourcedir(0, 0, 0, 0, 0, path, sizeof(path)))
    {
        if( FS_OpenWithDefault( path ) )
            return true;
    }

    Com_Printf( S_COLOR_RED "FS_BrowseHomepath: failed to open the homepath with the default file manager.\n" S_COLOR_WHITE );
    return false;
}

/*
============
FS_OpenBaseGamePath

Attempts to open path of form [sourcedir 0]/[fs_basegame]/[baseGamePath] in default file manager
============
*/
bool FS_OpenBaseGamePath( const char *baseGamePath )
{
    char path[FS_MAX_PATH];
    if(fs_generate_path_sourcedir(0, fs_basegame->string, baseGamePath, 0, 0, path, sizeof(path)))
    {
        if( FS_OpenWithDefault( path ) )
            return true;
    }

    Com_Printf( S_COLOR_RED "FS_OpenBaseGamePath: failed to open game path with the default file manager.\n" S_COLOR_WHITE );
    return false;
}

#endif  // NEW_FILESYSTEM
