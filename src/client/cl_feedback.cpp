/*
===========================================================================
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

// cl.feedback.c  -- send any game feedback to the user (rumble, keyboard color, ....)

#include "client.h"

static void IN_HapticFeedback_f( void )
{
	int				argc;
	float			strength = 0.5;
	uint32_t	length = 25;

	argc = Cmd_Argc();
	if (argc > 3)
	{
		Com_Printf("usage: hapticfeedback [strength] [length]\n");
		return;
	}
	if (argc >= 2)
		strength = atof(Cmd_Argv(1));
	if (argc >= 3)
		length = atoi(Cmd_Argv(2));

  IN_HapticFeedback(strength, length);
}

void FB_Init( void )
{
  // initiate gamepad effects here

  Cmd_AddCommand("hapticfeedback", IN_HapticFeedback_f);
}

void FB_Update( void )
{
  // send feedback effects to the gamepad here
}

void FB_Shutdown( void )
{
  // free up gamepad effects here

	Cmd_RemoveCommand("hapticfeedback");
}
