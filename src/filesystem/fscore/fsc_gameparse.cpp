/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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
#include "fscore.h"

/* ******************************************************************************** */
// Support functions from qcommon.c
/* ******************************************************************************** */

#define qboolean int
#define qtrue 1
#define qfalse 0
#define NULL 0

#define FSC_EDIT

#ifdef FSC_EDIT
void fsc_SkipRestOfLine ( char **data ) {
#else
void SkipRestOfLine ( char **data ) {
#endif
	char	*p;
	int		c;

	p = *data;
	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
#ifndef FSC_EDIT
			com_lines++;
#endif
			break;
		}
	}

	*data = p;
}

#ifdef FSC_EDIT
static char *fsc_SkipWhitespace( char *data, qboolean *hasNewLines ) {
#else
static char *SkipWhitespace( char *data, qboolean *hasNewLines ) {
#endif
	int c;

	while( (c = *data) <= ' ') {
		if( !c ) {
			return NULL;
		}
		if( c == '\n' ) {
#ifndef FSC_EDIT
			com_lines++;
#endif
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

#ifdef FSC_EDIT
// com_token is a buffer of length MAX_TOKEN_CHARS that stores the token being returned.
char *fsc_COM_ParseExt( char *com_token, char **data_p, int allowLineBreaks )
#else
char *COM_ParseExt( char **data_p, qboolean allowLineBreaks )
#endif
{
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	while ( 1 )
	{
		// skip whitespace
		data = fsc_SkipWhitespace( data, &hasNewLines );
		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' )
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				data++;
			}
			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < FSC_MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < FSC_MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
#ifndef FSC_EDIT
		if ( c == '\n' )
			com_lines++;
#endif
	} while (c>32);

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

// Slightly modified from the original q_shared.c version
// depth parameter is initial depth (0 if expecting first brace)
int fsc_SkipBracedSection(char **program, int depth) {
	char token[FSC_MAX_TOKEN_CHARS];

	do {
		fsc_COM_ParseExt( token, program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );

	return depth;
}

#endif	// NEW_FILESYSTEM
