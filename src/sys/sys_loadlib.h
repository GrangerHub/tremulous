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
#ifndef _SYS_LOADLIB_H_
#define _SYS_LOADLIB_H_

#ifdef DEDICATED
#ifdef _WIN32
#include <windows.h>
#define Sys_LoadLibrary(f) (void*)LoadLibrary(f)
#define Sys_UnloadLibrary(h) FreeLibrary((HMODULE)h)
#define Sys_LoadFunction(h, fn) (void*)GetProcAddress((HMODULE)h, fn)
#define Sys_LibraryError() "unknown"
#else
#include <dlfcn.h>
#define Sys_LoadLibrary(f) dlopen(f, RTLD_NOW)
#define Sys_UnloadLibrary(h) dlclose(h)
#define Sys_LoadFunction(h, fn) dlsym(h, fn)
#define Sys_LibraryError() dlerror()
#endif
#else
#include <SDL3/SDL.h>
#include <SDL3/SDL_loadso.h>
// SDL3: SDL_LoadObject returns SDL_SharedObject*, SDL_LoadFunction returns SDL_FunctionPointer
// Cast to void* for compatibility with existing code that uses void* handles
#define Sys_LoadLibrary(f) ((void*)SDL_LoadObject(f))
#define Sys_UnloadLibrary(h) SDL_UnloadObject((SDL_SharedObject*)(h))
#define Sys_LoadFunction(h, fn) ((void*)SDL_LoadFunction((SDL_SharedObject*)(h), fn))
#define Sys_LibraryError() SDL_GetError()
#endif

void* QDECL Sys_LoadDll(const char* name, bool useSystemLib);

#endif
