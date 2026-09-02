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

#ifdef _WIN32
#include <windows.h>
#endif

#include <SDL3/SDL.h>

#include "qcommon/cvar.h"
#include "qcommon/qcommon.h"
#include "renderercommon/tr_common.h"

extern SDL_Window *SDL_window;

/*
=================
GLimp_SetGamma

SDL3 removed SDL_SetWindowGammaRamp and SDL_SetWindowBrightness.
Gamma/brightness correction is now handled via shader-based post-processing
in the renderer. This function is kept as a no-op stub for API compatibility.

TODO: Once shader-based gamma post-processing is fully integrated into
both renderergl1 and renderergl2, this stub can be removed entirely
and the call sites in tr_image.cpp updated.
=================
*/
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
    // SDL3 removed hardware gamma ramp support.
    // Gamma is now handled by shader-based post-processing.
    // The texture-level gamma tables (s_gammatable) are still applied
    // via R_LightScaleTexture() in tr_image.cpp for texture brightness.
    (void)red;
    (void)green;
    (void)blue;
}
