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
#include <initializer_list>
#include <vector>
#include "fslocal.h"

// This file is used for some Tremulous-specific filesystem functions

/* ******************************************************************************** */
// Core Resource Precedence
/* ******************************************************************************** */

// This section is used to determine the priority of core resources such as pk3s that
// are included in game releases.

struct core_resource_s {
    unsigned int hash;
    const char *special_id;

    core_resource_s(int _hash, const char *_special_id = 0)
    {
        hash = (unsigned int)_hash;
        special_id = _special_id;
    }
};

#define CORE_RESOURCES_1_3 \
    {0, "core_game_dll"},   /* Core dll defined by FS_CORE_GAME_DLL_NAME */ \
    -498868165,             /* trem13/vms-gpp-v1.3.0-alpha.0.13-25-g55049001.pk3 */ \
    -754603633              /* trem13/data-v1.3.0-alpha.0.14.6.pk3 */ \

#define CORE_RESOURCES_GPP \
    -1154612609,            /* gpp/vms-gpp1.pk3 */ \
    -1688908842             /* gpp/data-gpp1.pk3 */ \

#define CORE_RESOURCES_1_1 \
    -1286840555,            /* base/vms-1.1.0.pk3 */ \
    1428306337,             /* base/data-1.1.0.pk3 */ \
    -537036296,             /* base/map-uncreation-1.1.0.pk3 */ \
    -1768007739,            /* base/map-tremor-1.1.0.pk3 */ \
    -2044057604,            /* base/map-transit-1.1.0.pk3 */ \
    2047591756,             /* base/map-niveus-1.1.0.pk3 */ \
    -2034645069,            /* base/map-nexus6-1.1.0.pk3 */ \
    1167370113,             /* base/map-karith-1.1.0.pk3 */ \
    -2177599,               /* base/map-atcs-1.1.0.pk3 */ \
    -1985258489             /* base/map-arachnid2-1.1.0.pk3 */ \

typedef std::vector<core_resource_s> core_resource_set_t;
static auto core_set_default = core_resource_set_t({CORE_RESOURCES_1_3, CORE_RESOURCES_GPP, CORE_RESOURCES_1_1});
static auto core_set_1_1 = core_resource_set_t({CORE_RESOURCES_1_1, CORE_RESOURCES_GPP, CORE_RESOURCES_1_3});
static auto core_set_gpp = core_resource_set_t({CORE_RESOURCES_GPP, CORE_RESOURCES_1_1, CORE_RESOURCES_1_3});

static core_resource_set_t &core_set_current()
{
    switch (fs_current_profile())
    {
        case FS_PROFILE_1_1:
            return core_set_1_1;
        case FS_PROFILE_GPP:
            return core_set_gpp;
        default:
            return core_set_default;
    }
}

int core_pk3_position(unsigned int hash)
{
    // Determines the precedence value of a core pk3 (higher value equals higher precedence)
    if (!hash)
        return 0;
    auto core_set = core_set_current();
    size_t length = core_set.size();
    for (int i = 0; i < length; ++i)
    {
        if (core_set[i].hash == hash)
            return length - i;
    }
    return 0;
}

int core_special_position(const char *special_id)
{
    // Determines the precedence value of a resource indicated by a special identifier string
    // This allows special content used by the engine such as game dlls to be included in the
    //   resource lists and get a precedence value relative to the core pk3s
    auto core_set = core_set_current();
    size_t length = core_set.size();
    for (int i = 0; i < length; ++i)
    {
        if (core_set[i].special_id && !Q_stricmp(special_id, core_set[i].special_id))
            return length - i;
    }
    return 0;
}

/* ******************************************************************************** */
// Base Directory & Mod Precedence
/* ******************************************************************************** */

FS_ModType fs_get_mod_type(const char *mod_dir, bool prioritize_fs_basegame)
{
    if (*current_mod_dir && !Q_stricmp(mod_dir, current_mod_dir))
        return MODTYPE_CURRENT_MOD;
    if (!Q_stricmp(mod_dir, BASEGAME_OVERRIDE))
        return MODTYPE_OVERRIDE_DIRECTORY;

    if (!Q_stricmp(mod_dir, fs_basegame->string))
    {
        if (prioritize_fs_basegame)
            return MODTYPE_FS_BASEGAME;

        // Always prioritize fs_basegame if it is different from any of the regular base directories
        if (Q_stricmp(fs_basegame->string, BASEGAME_1_3) && Q_stricmp(fs_basegame->string, BASEGAME_GPP) &&
            Q_stricmp(fs_basegame->string, BASEGAME_1_1))
            return MODTYPE_FS_BASEGAME;
    }

    switch (fs_current_profile())
    {
        case FS_PROFILE_1_1:
            if (!Q_stricmp(mod_dir, BASEGAME_1_1))
                return MODTYPE_BASE3;
            if (!Q_stricmp(mod_dir, BASEGAME_GPP))
                return MODTYPE_BASE2;
            if (!Q_stricmp(mod_dir, BASEGAME_1_3))
                return MODTYPE_BASE1;
            break;

        case FS_PROFILE_GPP:
            if (!Q_stricmp(mod_dir, BASEGAME_GPP))
                return MODTYPE_BASE3;
            if (!Q_stricmp(mod_dir, BASEGAME_1_1))
                return MODTYPE_BASE2;
            if (!Q_stricmp(mod_dir, BASEGAME_1_3))
                return MODTYPE_BASE1;
            break;

        default:
            if (!Q_stricmp(mod_dir, BASEGAME_1_3))
                return MODTYPE_BASE3;
            if (!Q_stricmp(mod_dir, BASEGAME_GPP))
                return MODTYPE_BASE2;
            if (!Q_stricmp(mod_dir, BASEGAME_1_1))
                return MODTYPE_BASE1;
            break;
    }

    return MODTYPE_INACTIVE;
}

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
    if(fs_generate_path_sourcedir(0, fs_basegame->string, baseGamePath, FS_CREATE_DIRECTORIES,
            FS_ALLOW_DIRECTORIES|FS_CREATE_DIRECTORIES, path, sizeof(path)))
    {
        if( FS_OpenWithDefault( path ) )
            return true;
    }

    Com_Printf( S_COLOR_RED "FS_OpenBaseGamePath: failed to open game path with the default file manager.\n" S_COLOR_WHITE );
    return false;
}

/*
============
FS_OpenModPath

Attempts to open path of form [sourcedir 0]/[modPath] in default file manager
============
*/
bool FS_OpenModPath( const char *modPath )
{
    char path[FS_MAX_PATH];
    if(fs_generate_path_sourcedir(0, modPath, 0, FS_CREATE_DIRECTORIES,
            FS_ALLOW_DIRECTORIES|FS_CREATE_DIRECTORIES, path, sizeof(path)))
    {
        if( FS_OpenWithDefault( path ) )
            return true;
    }

    Com_Printf( S_COLOR_RED "FS_OpenModPath: failed to open mod path %s with the default file manager.\n" S_COLOR_WHITE, modPath );
    return false;
}

#endif  // NEW_FILESYSTEM
