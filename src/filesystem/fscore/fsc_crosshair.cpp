/*
===========================================================================
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
#include "fscore.h"

// This section is to support future crosshair lookup features

#define STACKPTR(pointer) ( fsc_stack_retrieve(&fs->general_stack, pointer) )

/* ******************************************************************************** */
// Crosshair Indexing
/* ******************************************************************************** */

int index_crosshair(fsc_filesystem_t *fs, fsc_stackptr_t source_file_ptr, fsc_errorhandler_t *eh) {
	// Returns 1 on success, 0 otherwise.
	fsc_file_t *source_file = (fsc_file_t *)STACKPTR(source_file_ptr);
	unsigned int hash;

	char *data = fsc_extract_file_allocated(fs, source_file, 0);
	if(!data) {
		fsc_report_error(eh, FSC_ERROR_CROSSHAIRFILE, "failed to extract/open crosshair file", source_file);
		return 0; }

	hash = fsc_block_checksum(data, source_file->filesize);
	fsc_free(data);

 {	fsc_stackptr_t new_crosshair_ptr = fsc_stack_allocate(&fs->general_stack, sizeof(fsc_crosshair_t));
	fsc_crosshair_t *new_crosshair = (fsc_crosshair_t *)STACKPTR(new_crosshair_ptr);

	new_crosshair->hash = hash;
	new_crosshair->source_file_ptr = source_file_ptr;
	fsc_hashtable_insert(new_crosshair_ptr, hash, &fs->crosshairs); }

	return 1; }

/* ******************************************************************************** */
// Other
/* ******************************************************************************** */

int is_crosshair_enabled(fsc_filesystem_t *fs, fsc_crosshair_t *crosshair) {
	return fsc_is_file_enabled((const fsc_file_t *)STACKPTR(crosshair->source_file_ptr), fs); }

#endif	// NEW_FILESYSTEM
