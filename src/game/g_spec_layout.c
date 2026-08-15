/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
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

===========================================================================
*/

/*
=======================================================================

SPECTATOR BUILDER LAYOUT SYSTEM - STUB

The full implementation of the spectator builder layout system is not
complete yet (the UI .menu files and end-to-end testing are still
pending). This stub provides empty definitions for every function
declared in g_local.h and referenced from the command table in
g_cmds.c so the game module links and the commands fail gracefully.

The complete implementation is preserved on the development branch
(build-system-modernization-backup:src/game/g_spec_layout.c) and will
replace this stub once the feature is finished.

=======================================================================
*/

#include "g_local.h"

/*
=================
SpecLayout_StubMsg

Notify the player that the feature is not available yet.
=================
*/
static void SpecLayout_StubMsg(gentity_t *ent)
{
    if (ent && ent->client)
    {
        trap_SendServerCommand(ent - g_entities, "print \"The spectator layout system is not yet available.\n\"");
    }
}

/*
=================
Spectator layout command stubs
=================
*/
void Cmd_SpecLayoutList_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutSave_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutLoad_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutDelete_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutEnable_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutRename_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutRate_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutClear_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutSort_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutVote_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecLayoutVoteList_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

void Cmd_SpecBuilderTeam_f(gentity_t *ent) { SpecLayout_StubMsg(ent); }

/*
=================
G_SpecLayout_ReadMetadata

Stub: report that no metadata is available.
=================
*/
qboolean G_SpecLayout_ReadMetadata(const char *map, const char *layoutName, char *creator, int creatorLen,
    char *createdDate, int dateLen, int *buildingCount, int *team)
{
    if (creator && creatorLen > 0)
        creator[0] = '\0';

    if (createdDate && dateLen > 0)
        createdDate[0] = '\0';

    if (buildingCount)
        *buildingCount = 0;

    if (team)
        *team = TEAM_NONE;

    return qfalse;
}

/*
=================
G_SpecLayout_ReadRatings

Stub: report that no ratings exist.
=================
*/
void G_SpecLayout_ReadRatings(const char *map, const char *layoutName, float *average, int *count)
{
    if (average)
        *average = 0.0f;

    if (count)
        *count = 0;
}
