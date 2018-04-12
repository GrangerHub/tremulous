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

// This section is used to provide faster file iteration when starting at a certain
// directory, rather than iterating over the entire filesystem.

#define STACKPTR(pointer) ( fsc_stack_retrieve(&fs->general_stack, pointer) )
#define STACKPTRL(pointer) ( fsc_stack_retrieve(stack, pointer) )	// stack is a local parameter

/* ******************************************************************************** */
// File Registration
/* ******************************************************************************** */

static int get_parent_qp_dir(const char *qp_dir, char *target) {
	// Converts qp_dir string to parent dir
	// In other words, strip everything including and after the last slash
	// Target should be size FSC_MAX_QPATH
	// Returns 0 if no parent directory, 1 otherwise
	const char *current = qp_dir;
	const char *last_slash = 0;
	unsigned int length;

	while(*current) {
		if(*current == '/') last_slash = current;
		++current; }

	// If no slash is found, parent directory is root directory. Leave output null.
	if(!last_slash) return 0;

	// Copy in everything up to the slash
	length = last_slash - qp_dir;
	if(length > FSC_MAX_QPATH - 1) length = FSC_MAX_QPATH - 1;
	fsc_memcpy(target, qp_dir, length);
	target[length] = 0;
	return 1; }

static fsc_stackptr_t get_directory(const char *qp_dir, fsc_hashtable_t *directories,
		fsc_hashtable_t *string_repository, fsc_stack_t *stack) {
	// Null qp_dir represents the root directory
	unsigned int qp_dir_hash = qp_dir ? fsc_string_hash(qp_dir, 0) : 0;

	fsc_hashtable_iterator_t hti;
	fsc_stackptr_t directory_ptr;
	fsc_directory_t *directory;

	char parent_qp_dir[FSC_MAX_QPATH];
	int parent_path_found;
	fsc_stackptr_t parent_dir_ptr;
	fsc_directory_t *parent_dir;

	// Check if directory is already in the hash table
	fsc_hashtable_open(directories, qp_dir_hash, &hti);
	while((directory_ptr = fsc_hashtable_next(&hti))) {
		directory = (fsc_directory_t *)STACKPTRL(directory_ptr);
		if(!directory->qp_dir_ptr) {
			if(!qp_dir) return directory_ptr;
			continue; }
		if(!qp_dir) continue;
		if(!fsc_stricmp((const char *)STACKPTRL(directory->qp_dir_ptr), qp_dir)) return directory_ptr; }

	// It isn't, so create a new directory
	directory_ptr = fsc_stack_allocate(stack, sizeof(fsc_directory_t));
	directory = (fsc_directory_t *)STACKPTRL(directory_ptr);
	directory->qp_dir_ptr = qp_dir ? fsc_string_repository_getstring(qp_dir, 1, string_repository, stack) : 0;
	fsc_hashtable_insert(directory_ptr, qp_dir_hash, directories);

	// If we are at the root directory, don't proceed any further
	if(!qp_dir) return directory_ptr;

	// Get the parent directory
	parent_path_found = get_parent_qp_dir(qp_dir, parent_qp_dir);
	parent_dir_ptr = get_directory(parent_path_found ? parent_qp_dir : 0, directories, string_repository, stack);
	parent_dir = (fsc_directory_t *)STACKPTRL(parent_dir_ptr);

	// Add current directory to parent's sub_directory linked list
	directory->peer_directory = parent_dir->sub_directory;
	parent_dir->sub_directory = directory_ptr;

	return directory_ptr; }

void fsc_iteration_register_file(fsc_stackptr_t file_ptr, fsc_hashtable_t *directories,
		fsc_hashtable_t *string_repository, fsc_stack_t *stack) {
	fsc_file_t *file = (fsc_file_t *)STACKPTRL(file_ptr);

	// Get directory
	fsc_stackptr_t directory_ptr = get_directory((const char *)STACKPTRL(file->qp_dir_ptr),
			directories, string_repository, stack);
	fsc_directory_t *directory = (fsc_directory_t *)STACKPTRL(directory_ptr);

	// Add file to directory's sub_file linked list
	file->next_in_directory = directory->sub_file;
	directory->sub_file = file_ptr; }

#endif	// NEW_FILESYSTEM
