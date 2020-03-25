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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#ifdef USE_LOCAL_HEADERS
# include "SDL.h"
#else
# include <SDL.h>
#endif

#include "client/client.h"
#include "game/tremulous.h"
#include "sys/sys_local.h"

static cvar_t *in_keyboardDebug     = NULL;

static SDL_GameController *gamepad	= NULL;
static SDL_Joystick *stick = NULL;
static SDL_Haptic *haptic = NULL;

static bool mouseAvailable = false;
static bool mouseActive = false;

static cvar_t *in_mouse             = NULL;
static cvar_t *in_nograb;

static cvar_t *in_joystick          = NULL;
static cvar_t *in_joystickNo        = NULL;
static cvar_t *in_joystickUseAnalog = NULL;
static cvar_t *in_joystickCount     = NULL;

static cvar_t *in_haptic          	= NULL;
static cvar_t *in_hapticNo       		= NULL;
static cvar_t *in_hapticIntensity 	= NULL;
static cvar_t *in_hapticCount 			= NULL;
static int hapticRumbleSupported 		= SDL_FALSE;

static int vidRestartTime = 0;
static int in_eventTime = 0;

static SDL_Window *SDL_window 			= NULL;

#define CTRL(a) ((a)-'a'+1)

/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey( const SDL_Keysym *keysym, keyNum_t key, bool down )
{
	if( down )
		Com_Printf( "+ " );
	else
		Com_Printf( "  " );

	Com_Printf( "Scancode: 0x%02x(%s) Sym: 0x%02x(%s)",
			keysym->scancode, SDL_GetScancodeName( keysym->scancode ),
			keysym->sym, SDL_GetKeyName( keysym->sym ) );

	if( keysym->mod & KMOD_LSHIFT )   Com_Printf( " KMOD_LSHIFT" );
	if( keysym->mod & KMOD_RSHIFT )   Com_Printf( " KMOD_RSHIFT" );
	if( keysym->mod & KMOD_LCTRL )    Com_Printf( " KMOD_LCTRL" );
	if( keysym->mod & KMOD_RCTRL )    Com_Printf( " KMOD_RCTRL" );
	if( keysym->mod & KMOD_LALT )     Com_Printf( " KMOD_LALT" );
	if( keysym->mod & KMOD_RALT )     Com_Printf( " KMOD_RALT" );
	if( keysym->mod & KMOD_LGUI )     Com_Printf( " KMOD_LGUI" );
	if( keysym->mod & KMOD_RGUI )     Com_Printf( " KMOD_RGUI" );
	if( keysym->mod & KMOD_NUM )      Com_Printf( " KMOD_NUM" );
	if( keysym->mod & KMOD_CAPS )     Com_Printf( " KMOD_CAPS" );
	if( keysym->mod & KMOD_MODE )     Com_Printf( " KMOD_MODE" );
	if( keysym->mod & KMOD_RESERVED ) Com_Printf( " KMOD_RESERVED" );

	Com_Printf( " Q:0x%02x(%s)\n", key, Key_KeynumToString( key ) );
}

#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
static bool IN_IsConsoleKey( keyNum_t key, int character )
{
	typedef struct consoleKey_s
	{
		enum
		{
			QUAKE_KEY,
			CHARACTER
		} type;

		union
		{
			keyNum_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	// Only parse the variable when it changes
	if( cl_consoleKeys->modified )
	{
		char *text_p, *token;

		cl_consoleKeys->modified = qfalse;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;

		while( numConsoleKeys < MAX_CONSOLE_KEYS )
		{
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int charCode = 0;

			token = COM_Parse( &text_p );
			if( !token[ 0 ] )
				break;

			if( strlen( token ) == 4 )
				charCode = Com_HexStrToInt( token );

			if( charCode > 0 )
			{
				c->type = consoleKey_s::CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type = consoleKey_s::QUAKE_KEY;
				c->u.key = (keyNum_t)Key_StringToKeynum( token );

				// 0 isn't a key
				if( c->u.key <= 0 )
					continue;
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if( key == character )
		key = (keyNum_t)0;

	for( i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch( c->type )
		{
            case consoleKey_s::QUAKE_KEY:
				if( key && c->u.key == key )
					return true;
				break;

            case consoleKey_s::CHARACTER:
				if( c->u.character == character )
					return true;
				break;
		}
	}

	return false;
}

/*
===============
IN_TranslateSDLToQ3Key
===============
*/
static keyNum_t IN_TranslateSDLToQ3Key( SDL_Keysym *keysym, bool down )
{
	keyNum_t key = (keyNum_t)0;

	if( keysym->scancode >= SDL_SCANCODE_1 && keysym->scancode <= SDL_SCANCODE_0 )
	{
		// Always map the number keys as such even if they actually map
		// to other characters (eg, "1" is "&" on an AZERTY keyboard).
		// This is required for SDL before 2.0.6, except on Windows
		// which already had this behavior.
		if( keysym->scancode == SDL_SCANCODE_0 )
			key = static_cast<keyNum_t>('0');
		else
			key = static_cast<keyNum_t>('1' + keysym->scancode - SDL_SCANCODE_1);
	}
	else if( keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE )
	{
		// These happen to match the ASCII chars
		key = (keyNum_t)keysym->sym;
	}
	else if (!(keysym->sym >= SDLK_KP_1 && keysym->sym <= SDLK_KP_9
						&& ((SDL_GetModState() & KMOD_NUM) == (KMOD_NUM))))
	{
		// Don't send K_KP_DOWNARROW and others if numpad is activated

		switch( keysym->sym )
		{
			case SDLK_PAGEUP:       key = K_PGUP;          break;
			case SDLK_KP_9:         key = K_KP_PGUP;       break;
			case SDLK_PAGEDOWN:     key = K_PGDN;          break;
			case SDLK_KP_3:         key = K_KP_PGDN;       break;
			case SDLK_KP_7:         key = K_KP_HOME;       break;
			case SDLK_HOME:         key = K_HOME;          break;
			case SDLK_KP_1:         key = K_KP_END;        break;
			case SDLK_END:          key = K_END;           break;
			case SDLK_KP_4:         key = K_KP_LEFTARROW;  break;
			case SDLK_LEFT:         key = K_LEFTARROW;     break;
			case SDLK_KP_6:         key = K_KP_RIGHTARROW; break;
			case SDLK_RIGHT:        key = K_RIGHTARROW;    break;
			case SDLK_KP_2:         key = K_KP_DOWNARROW;  break;
			case SDLK_DOWN:         key = K_DOWNARROW;     break;
			case SDLK_KP_8:         key = K_KP_UPARROW;    break;
			case SDLK_UP:           key = K_UPARROW;       break;
			case SDLK_ESCAPE:       key = K_ESCAPE;        break;
			case SDLK_KP_ENTER:     key = K_KP_ENTER;      break;
			case SDLK_RETURN:       key = K_ENTER;         break;
			case SDLK_TAB:          key = K_TAB;           break;
			case SDLK_F1:           key = K_F1;            break;
			case SDLK_F2:           key = K_F2;            break;
			case SDLK_F3:           key = K_F3;            break;
			case SDLK_F4:           key = K_F4;            break;
			case SDLK_F5:           key = K_F5;            break;
			case SDLK_F6:           key = K_F6;            break;
			case SDLK_F7:           key = K_F7;            break;
			case SDLK_F8:           key = K_F8;            break;
			case SDLK_F9:           key = K_F9;            break;
			case SDLK_F10:          key = K_F10;           break;
			case SDLK_F11:          key = K_F11;           break;
			case SDLK_F12:          key = K_F12;           break;
			case SDLK_F13:          key = K_F13;           break;
			case SDLK_F14:          key = K_F14;           break;
			case SDLK_F15:          key = K_F15;           break;

			case SDLK_BACKSPACE:    key = K_BACKSPACE;     break;
			case SDLK_KP_PERIOD:    key = K_KP_DEL;        break;
			case SDLK_DELETE:       key = K_DEL;           break;
			case SDLK_PAUSE:        key = K_PAUSE;         break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:       key = K_SHIFT;         break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:        key = K_CTRL;          break;

#ifdef __APPLE__
			case SDLK_RGUI:
			case SDLK_LGUI:         key = K_COMMAND;       break;
#else
			case SDLK_RGUI:
			case SDLK_LGUI:         key = K_SUPER;         break;
#endif

			case SDLK_RALT:
			case SDLK_LALT:         key = K_ALT;           break;

			case SDLK_KP_5:         key = K_KP_5;          break;
			case SDLK_INSERT:       key = K_INS;           break;
			case SDLK_KP_0:         key = K_KP_INS;        break;
			case SDLK_KP_MULTIPLY:  key = K_KP_STAR;       break;
			case SDLK_KP_PLUS:      key = K_KP_PLUS;       break;
			case SDLK_KP_MINUS:     key = K_KP_MINUS;      break;
			case SDLK_KP_DIVIDE:    key = K_KP_SLASH;      break;

			case SDLK_MODE:         key = K_MODE;          break;
			case SDLK_HELP:         key = K_HELP;          break;
			case SDLK_PRINTSCREEN:  key = K_PRINT;         break;
			case SDLK_SYSREQ:       key = K_SYSREQ;        break;
			case SDLK_MENU:         key = K_MENU;          break;
			case SDLK_APPLICATION:	key = K_MENU;          break;
			case SDLK_POWER:        key = K_POWER;         break;
			case SDLK_UNDO:         key = K_UNDO;          break;
			case SDLK_SCROLLLOCK:   key = K_SCROLLOCK;     break;
			case SDLK_NUMLOCKCLEAR: key = K_KP_NUMLOCK;    break;
			case SDLK_CAPSLOCK:     key = K_CAPSLOCK;      break;

			default:
                if( !( keysym->sym & SDLK_SCANCODE_MASK ) && keysym->scancode <= 95 )
                {
					// Map Unicode characters to 95 world keys using the key's scan code.
					// FIXME: There aren't enough world keys to cover all the scancodes.
					// Maybe create a map of scancode to quake key at start up and on
					// key map change; allocate world key numbers as needed similar
					// to SDL 1.2.
			        key = static_cast<keyNum_t>(K_WORLD_0 + (int)keysym->scancode);
                }
				break;
		}
	}

	if( in_keyboardDebug->integer )
		IN_PrintKey( keysym, key, down );

	if( IN_IsConsoleKey( key, 0 ) )
	{
		// Console keys can't be bound or generate characters
		key = K_CONSOLE;
	}

	return key;
}

/*
===============
IN_GobbleMotionEvents
===============
*/
static void IN_GobbleMotionEvents( void )
{
	SDL_Event dummy[ 1 ];
	int val = 0;

	// Gobble any mouse motion events
	SDL_PumpEvents( );
	while( ( val = SDL_PeepEvents( dummy, 1, SDL_GETEVENT,
		SDL_MOUSEMOTION, SDL_MOUSEMOTION ) ) > 0 ) { }

	if ( val < 0 )
		Com_Printf( "IN_GobbleMotionEvents failed: %s\n", SDL_GetError( ) );
}

/*
===============
IN_GetUIMousePosition
===============
*/
static void IN_GetUIMousePosition( int *x, int *y )
{
	if( cls.ui )
	{
		int pos = VM_Call( cls.ui, UI_MOUSE_POSITION );
		*x = pos & 0xFFFF;
		*y = ( pos >> 16 ) & 0xFFFF;

		*x = cls.glconfig.vidWidth * *x / 640;
		*y = cls.glconfig.vidHeight * *y / 480;
	}
	else
	{
		*x = cls.glconfig.vidWidth / 2;
		*y = cls.glconfig.vidHeight / 2;
	}
}

/*
===============
IN_SetUIMousePosition
===============
*/
static void IN_SetUIMousePosition( int x, int y )
{
	if( cls.ui )
	{
		x = x * 640 / cls.glconfig.vidWidth;
		y = y * 480 / cls.glconfig.vidHeight;
		VM_Call( cls.ui, UI_SET_MOUSE_POSITION, x, y );
	}
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if (!mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	if( !mouseActive )
	{
		SDL_SetRelativeMouseMode( SDL_TRUE );
		SDL_SetWindowGrab( SDL_window, SDL_TRUE );

		IN_GobbleMotionEvents( );
	}

	// in_nograb makes no sense in fullscreen mode
	if( !cls.glconfig.isFullscreen )
	{
		if( in_nograb->modified || !mouseActive )
		{
			if( in_nograb->integer )
            {
                SDL_SetRelativeMouseMode( SDL_FALSE  );
				SDL_SetWindowGrab( SDL_window, SDL_FALSE );
            }
			else
            {
                SDL_SetRelativeMouseMode( SDL_TRUE  );
                SDL_SetWindowGrab( SDL_window, SDL_TRUE );
            }
			in_nograb->modified = qfalse;
		}
	}

	mouseActive = true;
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( void )
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if( !cls.glconfig.isFullscreen )
	{
		if( ( Key_GetCatcher( ) == KEYCATCH_UI ) &&
		    SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_MOUSE_FOCUS )
			SDL_ShowCursor( 0 );
		else
			SDL_ShowCursor( 1 );
	}

	if( !mouseAvailable )
		return;

	if( mouseActive )
	{
		IN_GobbleMotionEvents( );

		SDL_SetWindowGrab( SDL_window, SDL_FALSE );
		SDL_SetRelativeMouseMode( SDL_FALSE );

		// Don't warp the mouse unless the cursor is within the window
		if( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_MOUSE_FOCUS && cls.uiInterface != 2 )
    {
			int x, y;
			IN_GetUIMousePosition( &x, &y );
			SDL_WarpMouseInWindow( SDL_window, x, y );
    }

		mouseActive = false;
	}
}

/*
===============
Joystick and Gamepad
===============
*/

// We translate axes movement into keypresses
static const int joyKeys[16] = {
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW, K_DOWNARROW,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28
};

#define joyKeysCount ARRAY_LEN(joyKeys)

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static const int hatKeys[16] = {
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};

#define hatKeysCount ARRAY_LEN(hatKeys)

// Mapping analogs input as keys, groupped by sign
static const int negKeys[SDL_CONTROLLER_AXIS_MAX] = {
  K_PAD0_LEFTSTICK_LEFT,
  K_PAD0_LEFTSTICK_UP,
  K_PAD0_RIGHTSTICK_LEFT,
  K_PAD0_RIGHTSTICK_UP
};
static const int posKeys[SDL_CONTROLLER_AXIS_MAX] = {
  K_PAD0_LEFTSTICK_RIGHT,
  K_PAD0_LEFTSTICK_DOWN,
  K_PAD0_RIGHTSTICK_RIGHT,
  K_PAD0_RIGHTSTICK_DOWN,
  K_PAD0_LEFTTRIGGER,
  K_PAD0_RIGHTTRIGGER
};


struct
{
	bool oldButtons[SDL_CONTROLLER_BUTTON_MAX + 1]; // +1 because old max was 16, current SDL_CONTROLLER_BUTTON_MAX is 15
	unsigned int oldAxes;
	int oldAxis[MAX_JOYSTICK_AXIS];
	int oldCroppedAxis[MAX_JOYSTICK_AXIS];
  int oldTranslatedAxis[MAX_JOYSTICK_AXIS];
	unsigned int oldHats;
} stick_state;

typedef struct
{
	int length;
	int attack_length;
	int attack_level;
	int period;
	float magnitude;
	float large_magnitude;
	float small_magnitude;
} effectgen_t;

static bool IN_GamepadMove( bool isCaught );
static void IN_JoyMove( bool isCaught );


/*
===============
IN_GetGamecontrollerAxis
===============
*/
static int IN_GetGamecontrollerAxis(int axis)
{
	int value;

	if (!gamepad)
		return 0;

	return SDL_GameControllerGetAxis(gamepad, (SDL_GameControllerAxis)(SDL_CONTROLLER_AXIS_LEFTX + axis));
}


/*
===============
IN_GetJoycontrollerAxis
===============
*/
static int IN_GetJoycontrollerAxis(int axis)
{
	int value;

	if (!stick)
		return 0;

	return SDL_JoystickGetAxis(stick, axis);
}


/*
===============
IN_ResetJoystickInputs

Reset any movements
Sometime the controller (ie PS3 Dualshock) are "not connected"
since their aren't powered on (the PS button had to be pressed)
while GetAttached return true
So we ignore the initial state of the gamepad and wait for any motion
===============
*/
static void IN_ResetJoystickInputs( void )
{
	int i;

	// So we begin by initializing "old values"
	if (!IN_GamepadMove( false ))
		IN_JoyMove( false );

	// And in all case, reset any keys or axis
	for (i = 0; i < hatKeysCount; i++)
		Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[i], false, 0, NULL );
	for (i = K_FIRST_GAMEPAD_KEY; i <= K_LAST_GAMEPAD_KEY; i++)
		Com_QueueEvent(in_eventTime, SE_KEY, i, false, 0, NULL );
	for (i = 0; i < MAX_JOYSTICK_AXIS; i++)
		Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, i, 0, 0, NULL);
}


/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick( void )
{
	int i = 0;
	int total = 0;
	char buf[2048] = "";

	if (gamepad)
		SDL_GameControllerClose(gamepad);

	if (stick)
		SDL_JoystickClose(stick);

	stick = NULL;
	gamepad = NULL;
	memset(&stick_state, '\0', sizeof (stick_state));

	if( !in_joystick->integer )
	{
		Com_DPrintf( "Joystick is disabled.\n" );
		return;
	}

	// SDL 2.0.4 requires SDL_INIT_JOYSTICK to be initialized separately from
	// SDL_INIT_GAMECONTROLLER for SDL_JoystickOpen() to work correctly,
	// despite https://wiki.libsdl.org/SDL_Init (retrieved 2016-08-16)
	// indicating SDL_INIT_JOYSTICK should be initialized automatically.
	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
		if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
		{
			Com_Printf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
	}

	if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_GAMECONTROLLER)...\n");
		if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
		{
			Com_Printf("SDL_Init(SDL_INIT_GAMECONTROLLER) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_GAMECONTROLLER) passed.\n");
	}

	total = SDL_NumJoysticks();
	Com_DPrintf("%d possible joysticks\n", total);

	// Print list and build cvar to allow ui to select joystick.
	for (i = 0; i < total; i++)
	{
		if (i)
			Q_strcat(buf, sizeof(buf), "\n");
		Q_strcat(buf, sizeof(buf), SDL_IsGameController(i) ? "[gamepad] " : "[joystick] ");
		Q_strcat(buf, sizeof(buf), SDL_JoystickNameForIndex(i));
	}

	Cvar_Set( "in_availableJoysticks", buf );
	Cvar_SetValue( "in_joystickCount", total );

	if ( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
		Cvar_Set( "in_joystickNo", "0" );


	if ( !(stick = SDL_JoystickOpen(in_joystickNo->integer)) )
	{
		Com_DPrintf( "No joystick opened: %s\n", SDL_GetError() );
		return;
	}

	if ( SDL_IsGameController(in_joystickNo->integer) )
		gamepad = SDL_GameControllerOpen(in_joystickNo->integer);

	Com_DPrintf( "Joystick %d opened\n", in_joystickNo->integer );
	Com_DPrintf( "Name:       %s\n", SDL_JoystickNameForIndex(in_joystickNo->integer) );
	Com_DPrintf( "Axes:       %d\n", SDL_JoystickNumAxes(stick) );
	Com_DPrintf( "Hats:       %d\n", SDL_JoystickNumHats(stick) );
	Com_DPrintf( "Buttons:    %d\n", SDL_JoystickNumButtons(stick) );
	Com_DPrintf( "Balls:      %d\n", SDL_JoystickNumBalls(stick) );
	Com_DPrintf( "Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No" );
	Com_DPrintf( "Is gamepad: %s\n", gamepad ? "Yes" : "No" );
	Com_DPrintf( "Is haptic: %s\n", SDL_JoystickIsHaptic(stick) ? "Yes" : "No" );

	SDL_JoystickEventState(SDL_QUERY);
	SDL_GameControllerEventState(SDL_QUERY);
	IN_ResetJoystickInputs( );
}


static int hapticEffectId[CG_FB_EFFECT_COUNT];
static SDL_HapticEffect hapticEffect[CG_FB_EFFECT_COUNT];

/*
===============
IN_HapticGenerateEffect
===============
*/
SDL_HapticEffect *IN_HapticGenerateEffect(effectgen_t *efxGen, SDL_HapticEffect *efx)
{
	unsigned int support = SDL_HapticQuery(haptic);
	memset(efx, 0, sizeof(efx));

	if (support & SDL_HAPTIC_SINE) {
		efx->type = SDL_HAPTIC_SINE;
		efx->periodic.length = efxGen->length;
		efx->periodic.attack_length = efxGen->attack_length;
		efx->periodic.attack_level = efxGen->attack_level;
		efx->periodic.direction.type = SDL_HAPTIC_CARTESIAN;
		efx->periodic.period = efxGen->period ? efxGen->period : 500;
		efx->periodic.magnitude = 32767.0f * efxGen->magnitude * in_hapticIntensity->value;
	} else if (support & SDL_HAPTIC_LEFTRIGHT) {
		efx->type = SDL_HAPTIC_LEFTRIGHT;
		efx->leftright.length = efxGen->length;
		efx->leftright.large_magnitude = 32767.0f * efxGen->large_magnitude * in_hapticIntensity->value;
		efx->leftright.small_magnitude = 32767.0f * efxGen->small_magnitude * in_hapticIntensity->value;
	}

	return efx;
}


/*
===============
IN_HapticInitEffects
===============
*/
void IN_HapticInitEffects( void )
{
	effectgen_t lcannonEfx;
	memset(&lcannonEfx, 0, sizeof(lcannonEfx));

	lcannonEfx.attack_length = lcannonEfx.length = LCANNON_CHARGE_TIME_MAX;
	lcannonEfx.magnitude = 0.5;
	lcannonEfx.small_magnitude = 0.25;

	hapticEffectId[CG_FB_EFFECT_LCANNON]
			= SDL_HapticNewEffect( haptic,
					IN_HapticGenerateEffect(&lcannonEfx, &hapticEffect[CG_FB_EFFECT_LCANNON]) );

	effectgen_t lcannonWarnEfx;
	memset(&lcannonWarnEfx, 0, sizeof(lcannonWarnEfx));

	lcannonWarnEfx.length = LCANNON_CHARGE_TIME_MAX - LCANNON_CHARGE_TIME_WARN;
	lcannonWarnEfx.magnitude = 0.8;
	lcannonWarnEfx.large_magnitude = 0.8;
	lcannonWarnEfx.small_magnitude = 0.8;

	hapticEffectId[CG_FB_EFFECT_LCANNONWARN]
			= SDL_HapticNewEffect( haptic,
					IN_HapticGenerateEffect(&lcannonWarnEfx, &hapticEffect[CG_FB_EFFECT_LCANNONWARN]) );
}


/*
===============
IN_HapticResetEffects
===============
*/
static void IN_HapticResetEffects( void )
{
	int i;

	for (i = 0; i < CG_FB_EFFECT_COUNT; i++)
		hapticEffectId[i] = -1;
}


/*
===============
IN_RunHapticEffect
===============
*/
void IN_RunHapticEffect(int id)
{
	if ( !in_haptic->integer || !haptic || id >= CG_FB_EFFECT_COUNT )
		return;

	SDL_HapticRunEffect( haptic, hapticEffectId[id], 1 );
}


/*
===============
IN_StopHapticEffect
===============
*/
void IN_StopHapticEffect(int id)
{
	if ( !haptic || id >= CG_FB_EFFECT_COUNT || !SDL_HapticGetEffectStatus(haptic, hapticEffectId[id]) )
		return;

	SDL_HapticStopEffect( haptic, hapticEffectId[id] );
}

/*
===============
IN_InitHaptic
===============
*/
static void IN_InitHaptic( void )
{
	int i = 0;
	int total = 0;
	char buf[2048] = "";

	hapticRumbleSupported = SDL_FALSE;

	if (haptic)
		SDL_HapticClose(haptic);

	IN_HapticResetEffects();
	haptic = NULL;

	if( !in_haptic->integer )
	{
		Com_DPrintf( "Haptic is disabled.\n" );
		return;
	}

	if (!SDL_WasInit(SDL_INIT_HAPTIC))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_HAPTIC)...\n");
		if (SDL_Init(SDL_INIT_HAPTIC) != 0)
		{
			Com_Printf("SDL_Init(SDL_INIT_HAPTIC) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_HAPTIC) passed.\n");
	}

	total = SDL_NumHaptics();
	Com_DPrintf("%d possible haptics\n", total);

	// Print list and build cvar to allow ui to select joystick.
	for (i = 0; i < total; i++)
	{
		if (i)
			Q_strcat(buf, sizeof(buf), "\n");
		Q_strcat(buf, sizeof(buf), SDL_HapticName(i));
	}

	Cvar_Set( "in_availableHaptics", buf );
	Cvar_SetValue( "in_hapticCount", total );

	if( in_hapticNo->integer < 0 || in_hapticNo->integer >= total )
		Cvar_Set( "in_hapticNo", "0" );

	if ( !(haptic = SDL_HapticOpen( in_hapticNo->integer )) )
	{
		Com_DPrintf( "No haptic opened: %s\n", SDL_GetError() );
		return;
	}

	if ( (hapticRumbleSupported = SDL_HapticRumbleSupported( haptic )) == SDL_TRUE )
	{
		if ( SDL_HapticRumbleInit( haptic ) != 0 )
		{
			Com_Printf( "Can't initialize haptic's rumble effect: %s\n", SDL_GetError() );
			SDL_HapticClose( haptic );
			haptic = NULL;
			hapticRumbleSupported = SDL_FALSE;
			return;
		}
	}
	else if (hapticRumbleSupported < 0)
	{
		Com_Printf( "Error when querying haptic's rumble effect: %s\n", SDL_GetError() );
		SDL_HapticClose( haptic );
		haptic = NULL;
		hapticRumbleSupported = SDL_FALSE;
		return;
	}

	IN_HapticInitEffects( );

	Com_DPrintf( "Haptic %d opened\n", in_hapticNo->integer );
	Com_DPrintf( "Rumble:     %s\n", hapticRumbleSupported == SDL_TRUE ? "Yes" : "No" );
	Com_DPrintf( "Axes:       %d\n", SDL_HapticNumAxes(haptic) );
	Com_DPrintf( "Effects:    %d\n", SDL_HapticNumEffects(haptic) );
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick( void )
{
	if ( !SDL_WasInit( SDL_INIT_GAMECONTROLLER ) )
		return;

	if ( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
		return;

	IN_ResetJoystickInputs();

	if (gamepad)
	{
		SDL_GameControllerClose(gamepad);
		gamepad = NULL;
	}

	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	IN_HapticResetEffects();
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/*
===============
IN_ShutdownHaptic
===============
*/
static void IN_ShutdownHaptic( void )
{
	if ( !SDL_WasInit( SDL_INIT_HAPTIC ) )
		return;

	if (haptic)
	{
		SDL_HapticStopAll(haptic);
		SDL_HapticClose(haptic);
		haptic = NULL;
		hapticRumbleSupported = SDL_FALSE;
	}

	SDL_QuitSubSystem(SDL_INIT_HAPTIC);
}

/*
===============
IN_HapticFeedback
===============
*/
void IN_HapticFeedback( float strength, uint32_t length )
{
	if ( strength > 1 )
		strength = 1;

	if ( in_haptic->integer && haptic && hapticRumbleSupported == SDL_TRUE && length )
		SDL_HapticRumblePlay( haptic, strength * in_hapticIntensity->value, length );
}


/*
===============
IN_KeyToDirection
===============
*/
static bool IN_KeyToDirection(int keynum, int *outAxis, int *outSign)
{
	if (!keynum)
		return false;

	const char* bind = Key_GetBinding(keynum);

	if (!bind || *bind != '+')
		return false;

	*outSign = 0;

	if (Q_stricmp(bind, "+forward") == 0)
	{
		*outAxis = j_forward_axis->integer;
		*outSign = 1;
	}
	else if (Q_stricmp(bind, "+back") == 0)
	{
		*outAxis = j_forward_axis->integer;
		*outSign = -1;
	}
	else if (Q_stricmp(bind, "+moveleft") == 0)
	{
		*outAxis = j_side_axis->integer;
		*outSign = -1;
	}
	else if (Q_stricmp(bind, "+moveright") == 0)
	{
		*outAxis = j_side_axis->integer;
		*outSign = 1;
	}
	else if (Q_stricmp(bind, "+lookup") == 0)
	{
		*outAxis = j_pitch_axis->integer;
		*outSign = j_pitch->value > 0.0f ? -1 : 1;
	}
	else if (Q_stricmp(bind, "+lookdown") == 0)
	{
		*outAxis = j_pitch_axis->integer;
		*outSign = j_pitch->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+left") == 0)
	{
		*outAxis = j_yaw_axis->integer;
		*outSign = j_yaw->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+right") == 0)
	{
		*outAxis = j_yaw_axis->integer;
		*outSign = j_yaw->value > 0.0f ? -1 : 1;
	}
	else if (Q_stricmp(bind, "+moveup") == 0)
	{
		*outAxis = j_up_axis->integer;
		*outSign = 1;
	}
	else if (Q_stricmp(bind, "+movedown") == 0)
	{
		*outAxis = j_up_axis->integer;
		*outSign = -1;
	}

	return *outSign != 0;
}

/*
===============
IN_SimpleJoystickDeadzone
===============
*/
static float IN_SimpleJoystickDeadzone(float input)
{
  float cropped;

  cropped = ((fabs(input) / 32767.0f) - j_threshold->value)
      / (1.0f - j_threshold->value);

  if (cropped < 0.0f)
    cropped = 0.0f;
  if (cropped > 1.0f)
    cropped = 1.0f;

  return (32767.0f * ((input < 0) ? -cropped : cropped));
}


/*
===============
IN_GamepadMove
===============
*/
static bool IN_GamepadMove( bool isCaught )
{
	int i;
	int translatedAxis[MAX_JOYSTICK_AXIS] = {};
	bool translatedAxisSet[MAX_JOYSTICK_AXIS] = {};
	bool posAnalog, negAnalog;
	int negKey, posKey;
	int posAxis, posSign, negAxis, negSign;
	int axisValue, oldAxisValue, axisCroppedValue, oldAxisCroppedValue;

	if (!gamepad || SDL_GameControllerGetAttached(gamepad) == SDL_FALSE)
		return false;

	SDL_GameControllerUpdate();

	// check buttons
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		bool pressed = SDL_GameControllerGetButton(gamepad, (SDL_GameControllerButton)(SDL_CONTROLLER_BUTTON_A + i));
		if (pressed != stick_state.oldButtons[i])
		{
			Com_QueueEvent(in_eventTime, SE_KEY, K_PAD0_A + i, pressed, 0, NULL);
			stick_state.oldButtons[i] = pressed;
		}
	}

	// check axes
	for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
	{
		posKey = posKeys[i];
		negKey = negKeys[i];
		axisValue = IN_GetGamecontrollerAxis(i);
		axisCroppedValue = IN_SimpleJoystickDeadzone(axisValue);
		oldAxisValue = stick_state.oldAxis[i];
		oldAxisCroppedValue = stick_state.oldCroppedAxis[i];

		if (in_joystickUseAnalog->integer && !isCaught)
		{
			posAxis = posSign = negAxis = negSign = 0;

			// get axis and axis signs for keys if available
			posAnalog = IN_KeyToDirection(posKey, &posAxis, &posSign);
			negAnalog = IN_KeyToDirection(negKey, &negAxis, &negSign);

			// positive to negative/neutral -> keyup if axis hasn't yet been set
			if (posAnalog && !translatedAxisSet[posAxis] && oldAxisValue > 0 && axisValue <= 0)
			{
				translatedAxis[posAxis] = 0;
				translatedAxisSet[posAxis] = true;
			}

			// negative to positive/neutral -> keyup if axis hasn't yet been set
			if (negAnalog && !translatedAxisSet[negAxis] && oldAxisValue < 0 && axisValue >= 0)
			{
				translatedAxis[negAxis] = 0;
				translatedAxisSet[negAxis] = true;
			}

			// negative/neutral to positive -> keydown
			if (posAnalog && axisValue > 0)
			{
				translatedAxis[posAxis] += axisValue * posSign;
				translatedAxisSet[posAxis] = true;
			}

			// positive/neutral to negative -> keydown
			if (negAnalog && axisValue < 0)
			{
				translatedAxis[negAxis] += -axisValue * negSign;
				translatedAxisSet[negAxis] = true;
			}
		}

		// If a direction is not used, use it as button
		// keyups first so they get overridden by keydowns later
		if (oldAxisCroppedValue != axisCroppedValue)
		{
			// positive to negative/neutral -> keyup
			if (!posAnalog && posKey && oldAxisCroppedValue > 0 && axisCroppedValue <= 0)
				Com_QueueEvent(in_eventTime, SE_KEY, posKey, false, 0, NULL);

			// negative to positive/neutral -> keyup
			if (!negAnalog && negKey && oldAxisCroppedValue < 0 && axisCroppedValue >= 0)
				Com_QueueEvent(in_eventTime, SE_KEY, negKey, false, 0, NULL);

			// negative/neutral to positive -> keydown
			if (!posAnalog && posKey && oldAxisCroppedValue <= 0 && axisCroppedValue > 0)
				Com_QueueEvent(in_eventTime, SE_KEY, posKey, true, 0, NULL);

			// positive/neutral to negative -> keydown
			if (!negAnalog && negKey && oldAxisCroppedValue >= 0 && axisCroppedValue < 0)
				Com_QueueEvent(in_eventTime, SE_KEY, negKey, true, 0, NULL);


			stick_state.oldAxis[i] = axisValue;
			stick_state.oldCroppedAxis[i] = axisCroppedValue;
		}
	}

	// set translated axis
	// Note : we send all raw axis data
	// We manage thresholds later in analog mode
	if (in_joystickUseAnalog->integer && !isCaught)
	{
		for (i = 0; i < MAX_JOYSTICK_AXIS; i++)
		{
			if (translatedAxisSet[i] && stick_state.oldTranslatedAxis[i] != translatedAxis[i])
			{
				Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, i, translatedAxis[i], 0, NULL);
				stick_state.oldTranslatedAxis[i] = translatedAxis[i];
			}
		}
	}

	return true;
}


/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove( bool isCaught )
{
	unsigned int axes = 0;
	unsigned int hats = 0;
	int total = 0;
	int i = 0;

	if (!stick || SDL_JoystickGetAttached(stick) == SDL_FALSE)
		return;

	SDL_JoystickUpdate();

	// update the ball state.
	total = SDL_JoystickNumBalls(stick);
	if (total > 0)
	{
		int balldx = 0;
		int balldy = 0;
		for (i = 0; i < total; i++)
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx += dx;
			balldy += dy;
		}
		if (balldx || balldy)
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if (abs(balldx) > 1)
				balldx *= 2;
			if (abs(balldy) > 1)
				balldy *= 2;
			Com_QueueEvent(in_eventTime, SE_MOUSE, balldx, balldy, 0, NULL);
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons(stick);
	if (total > 0)
	{
		if (total > ARRAY_LEN(stick_state.oldButtons))
			total = ARRAY_LEN(stick_state.oldButtons);
		for (i = 0; i < total; i++)
		{
			bool pressed = (SDL_JoystickGetButton(stick, i) != 0);
			if (pressed != stick_state.oldButtons[i])
			{
				Com_QueueEvent(in_eventTime, SE_KEY, K_JOY1 + i, pressed, 0, NULL );
				stick_state.oldButtons[i] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats(stick);
	if (total > 0)
	{
		if (total > 4) total = 4;
		for (i = 0; i < total; i++)
		{
			((Uint8 *)&hats)[i] = SDL_JoystickGetHat(stick, i);
		}
	}

	// update hat state
	if (hats != stick_state.oldHats)
	{
		for( i = 0; i < 4; i++ ) {
			if( ((Uint8 *)&hats)[i] != ((Uint8 *)&stick_state.oldHats)[i] ) {
				// release event
				switch( ((Uint8 *)&stick_state.oldHats)[i] ) {
					case SDL_HAT_UP:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 0], false, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 1], false, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 2], false, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 3], false, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 0], false, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 1], false, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 2], false, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 1], false, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 0], false, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 3], false, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 2], false, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 3], false, 0, NULL );
						break;
					default:
						break;
				}
				// press event
				switch( ((Uint8 *)&hats)[i] ) {
					case SDL_HAT_UP:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 0], true, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 1], true, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 2], true, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 3], true, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 0], true, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 1], true, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 2], true, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 1], true, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 0], true, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 3], true, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 2], true, 0, NULL );
						Com_QueueEvent(in_eventTime, SE_KEY, hatKeys[4*i + 3], true, 0, NULL );
						break;
					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldHats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes(stick);
	if (total > 0)
	{
		if (in_joystickUseAnalog->integer && !isCaught)
		{
			if (total > MAX_JOYSTICK_AXIS) total = MAX_JOYSTICK_AXIS;
			for (i = 0; i < total; i++)
			{
				int axisValue = IN_GetJoycontrollerAxis(i);

				if ( axisValue != stick_state.oldAxis[i] )
				{
					Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, i, axisValue, 0, NULL );
					stick_state.oldAxis[i] = axisValue;
				}
			}
		}
		else
		{
			if (total > 16) total = 16;
			for (i = 0; i < total; i++)
			{
				int axisValue = SDL_JoystickGetAxis(stick, i);
				float axisValueRatio = ( (float) axisValue ) / 32767.0f;
				if( axisValueRatio < -j_threshold->value ) {
					axes |= ( 1 << ( i * 2 ) );
				} else if( axisValueRatio > j_threshold->value ) {
					axes |= ( 1 << ( ( i * 2 ) + 1 ) );
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if (axes != stick_state.oldAxes)
	{
		for( i = 0; i < 16; i++ ) {
			if( ( axes & ( 1 << i ) ) && !( stick_state.oldAxes & ( 1 << i ) ) ) {
				Com_QueueEvent(in_eventTime, SE_KEY, joyKeys[i], true, 0, NULL );
			}

			if( !( axes & ( 1 << i ) ) && ( stick_state.oldAxes & ( 1 << i ) ) ) {
				Com_QueueEvent(in_eventTime, SE_KEY, joyKeys[i], false, 0, NULL );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldAxes = axes;
}

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents( void )
{
	SDL_Event e;
	keyNum_t key = (keyNum_t)0;
	static keyNum_t lastKeyDown = (keyNum_t)0;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
			return;

	while( SDL_PollEvent( &e ) )
	{
		switch( e.type )
		{
			case SDL_KEYDOWN:
				if ( e.key.repeat && Key_GetCatcher( ) == 0 )
					break;

				if( ( key = IN_TranslateSDLToQ3Key( &e.key.keysym, true ) ) )
					Com_QueueEvent(in_eventTime, SE_KEY, key, true, 0, NULL );

				if( key == K_BACKSPACE )
					Com_QueueEvent(in_eventTime, SE_CHAR, CTRL('h'), 0, 0, NULL );
				else if( keys[K_CTRL].down && key >= 'a' && key <= 'z' )
					Com_QueueEvent(in_eventTime, SE_CHAR, CTRL(key), 0, 0, NULL );

				lastKeyDown = key;
				break;

			case SDL_KEYUP:
				if( ( key = IN_TranslateSDLToQ3Key( &e.key.keysym, false ) ) )
					Com_QueueEvent(in_eventTime, SE_KEY, key, false, 0, NULL );

				lastKeyDown = (keyNum_t)0;
				break;

			case SDL_TEXTINPUT:
				if( lastKeyDown != K_CONSOLE )
				{
					char *c = e.text.text;

					// Quick and dirty UTF-8 to UTF-32 conversion
					while( *c )
					{
						int utf32 = 0;

						if( ( *c & 0x80 ) == 0 )
							utf32 = *c++;
						else if( ( *c & 0xE0 ) == 0xC0 ) // 110x xxxx
						{
							utf32 |= ( *c++ & 0x1F ) << 6;
							utf32 |= ( *c++ & 0x3F );
						}
						else if( ( *c & 0xF0 ) == 0xE0 ) // 1110 xxxx
						{
							utf32 |= ( *c++ & 0x0F ) << 12;
							utf32 |= ( *c++ & 0x3F ) << 6;
							utf32 |= ( *c++ & 0x3F );
						}
						else if( ( *c & 0xF8 ) == 0xF0 ) // 1111 0xxx
						{
							utf32 |= ( *c++ & 0x07 ) << 18;
							utf32 |= ( *c++ & 0x3F ) << 12;
							utf32 |= ( *c++ & 0x3F ) << 6;
							utf32 |= ( *c++ & 0x3F );
						}
						else
						{
							Com_Printf( "Unrecognised UTF-8 lead byte: 0x%x\n", (unsigned int)*c );
							c++;
						}

						if( utf32 != 0 )
						{
							if( IN_IsConsoleKey( (keyNum_t)0, utf32 ) )
							{
								Com_QueueEvent(in_eventTime, SE_KEY, K_CONSOLE, true, 0, NULL );
								Com_QueueEvent(in_eventTime, SE_KEY, K_CONSOLE, false, 0, NULL );
							}
							else
								Com_QueueEvent(in_eventTime, SE_CHAR, utf32, 0, 0, NULL );
						}
          }
        }
				break;

			case SDL_MOUSEMOTION:
				if( mouseActive )
				{
					if( !e.motion.xrel && !e.motion.yrel )
						break;
					Com_QueueEvent(in_eventTime, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL );
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					int b;
					switch( e.button.button )
					{
						case 1:   b = K_MOUSE1;     break;
						case 2:   b = K_MOUSE3;     break;
						case 3:   b = K_MOUSE2;     break;
						case 4:   b = K_MOUSE4;     break;
						case 5:   b = K_MOUSE5;     break;
						default:  b = K_AUX1 + ( e.button.button - 8 ) % 16; break;
					}
					Com_QueueEvent(in_eventTime, SE_KEY, b,
						( e.type == SDL_MOUSEBUTTONDOWN ? true : false ), 0, NULL );
				}
				break;

			case SDL_MOUSEWHEEL:
				if( e.wheel.y > 0 )
				{
					Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELUP, true, 0, NULL );
					Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELUP, false, 0, NULL );
				}
				else if( e.wheel.y < 0 )
				{
					Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELDOWN, true, 0, NULL );
					Com_QueueEvent(in_eventTime, SE_KEY, K_MWHEELDOWN, false, 0, NULL );
				}
				break;

			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
				if (in_joystick->integer)
					IN_InitJoystick();
				if (in_haptic->integer)
					IN_InitHaptic();
				break;

			case SDL_QUIT:
				Cbuf_ExecuteText(EXEC_NOW, "quit \"Closed window\"\n");
				break;

			case SDL_WINDOWEVENT:
				switch( e.window.event )
				{
					case SDL_WINDOWEVENT_RESIZED:
						{
							int width, height;

							width = e.window.data1;
							height = e.window.data2;

                            // ignore this event on fullscreen
                            if ( cls.glconfig.isFullscreen )
                                break;

							// check if size actually changed
							if( cls.glconfig.vidWidth == width && cls.glconfig.vidHeight == height )
							{
								break;
							}

							Cvar_SetValue( "r_width", width );
							Cvar_SetValue( "r_height", height );

							// Wait until user stops dragging for 1 second, so
							// we aren't constantly recreating the GL context while
							// he tries to drag...
							vidRestartTime = Sys_Milliseconds( ) + 1000;
						}
						break;

					case SDL_WINDOWEVENT_MINIMIZED:    Cvar_SetValue( "com_minimized", 1 ); break;
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MAXIMIZED:    Cvar_SetValue( "com_minimized", 0 ); break;
					case SDL_WINDOWEVENT_FOCUS_LOST:   Cvar_SetValue( "com_unfocused", 1 ); break;
					case SDL_WINDOWEVENT_FOCUS_GAINED: Cvar_SetValue( "com_unfocused", 0 ); break;
				}
				break;

			default:
				break;
		}
	}
}

/*
===============
IN_Frame
===============
*/
void IN_Frame( void )
{
	static bool beenCaught = false;
	bool isCaught = ( Key_GetCatcher( ) & KEYCATCH_UI );  // Let the UI access to analogs as keys
	bool loading;
	bool cursorShowing;
	int x, y;

	if (beenCaught != isCaught)
	{
		IN_ResetJoystickInputs( );
		beenCaught = isCaught;
	}

	// Don't allow joystick in the UI nor when using gamepad
	if (!IN_GamepadMove( isCaught ))
		IN_JoyMove( isCaught );


	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = ( clc.state != CA_DISCONNECTED && clc.state != CA_ACTIVE );
	cursorShowing = Key_GetCatcher( ) & KEYCATCH_UI;

	if( !cls.glconfig.isFullscreen && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !cls.glconfig.isFullscreen && loading )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !cls.glconfig.isFullscreen && cursorShowing && cls.uiInterface != 2 )
	{
		// Use WM cursor when not fullscreen
		IN_DeactivateMouse( );
	}
	else if( !( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_INPUT_FOCUS ) )
	{
		// Window not got focus
		IN_DeactivateMouse( );
	}
	else
		IN_ActivateMouse( );

	if( !mouseActive && cls.uiInterface != 2 )
	{
		SDL_GetMouseState( &x, &y );
		IN_SetUIMousePosition( x, y );
	}

	IN_ProcessEvents( );

    // Set event time for next frame to earliest possible time an event could happen
    in_eventTime = Sys_Milliseconds();

	// In case we had to delay actual restart of video system
	if( ( vidRestartTime != 0 ) && ( vidRestartTime < Sys_Milliseconds( ) ) )
	{
		vidRestartTime = 0;
		Cbuf_AddText( "vid_restart\n" );
	}
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	int appState;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Error( ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )" );
		return;
	}

	SDL_window = (SDL_Window *)windowData;

	Com_DPrintf( "\n------- Input Initialization -------\n" );

	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_ARCHIVE );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE );

	j_threshold = Cvar_Get( "j_threshold", "0.02", CVAR_ARCHIVE );
	j_outMovmentThreshold = Cvar_Get( "j_outMovmentThreshold", "0.02", CVAR_ARCHIVE );
	in_joystick = Cvar_Get( "in_joystick", "1", CVAR_ARCHIVE|CVAR_LATCH );
	in_joystickNo = Cvar_Get( "in_joystickNo", "0", CVAR_ARCHIVE );
	in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "1", CVAR_ARCHIVE );
	in_joystickCount = Cvar_Get( "in_joystickCount", "0", CVAR_ROM );
	in_haptic = Cvar_Get( "in_haptic", "1", CVAR_ARCHIVE|CVAR_LATCH );
	in_hapticNo = Cvar_Get( "in_hapticNo", "0", CVAR_ARCHIVE );
	in_hapticIntensity = Cvar_Get( "in_hapticIntensity", "0.5", CVAR_ARCHIVE );
	in_hapticCount = Cvar_Get( "in_hapticCount", "0", CVAR_ROM );

	SDL_StartTextInput( );

	mouseAvailable = ( in_mouse->value != 0 );
	IN_DeactivateMouse( );

	appState = SDL_GetWindowFlags( SDL_window );
	Cvar_SetValue( "com_unfocused",	!( appState & SDL_WINDOW_INPUT_FOCUS ) );
	Cvar_SetValue( "com_minimized", appState & SDL_WINDOW_MINIMIZED );

	IN_InitJoystick( );
	IN_InitHaptic( );
	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
	SDL_StopTextInput( );

	IN_DeactivateMouse( );
	mouseAvailable = false;

	IN_ShutdownHaptic( );
	IN_ShutdownJoystick( );

	SDL_window = NULL;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart( void )
{
	IN_ShutdownJoystick( );
	IN_ShutdownHaptic( );
	IN_Init( SDL_window );
}
