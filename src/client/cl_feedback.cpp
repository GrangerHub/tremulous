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
  Cmd_AddCommand("hapticfeedback", IN_HapticFeedback_f);
}

void FB_Shutdown( void )
{
	Cmd_RemoveCommand("hapticfeedback");
}
