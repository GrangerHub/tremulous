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

/* ******************************************************************************** */
// Headers / Definitions
/* ******************************************************************************** */

#ifdef _WIN32
// Windows defines
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#ifdef UNICODE
#define WIN_UNICODE
#endif
#else
// Non-windows defines
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#endif
// Common defines
#include <stdio.h>

/* ******************************************************************************** */
// Misc Library Functions
/* ******************************************************************************** */

int fsc_rename_file(void *source_os_path, void *target_os_path) {
	// Returns 1 on error, 0 on success
#ifdef _WIN32
	if(!MoveFile((LPCTSTR)source_os_path, (LPCTSTR)target_os_path)) return 1;
#else
	if(rename((const char *)source_os_path, (const char *)target_os_path)) return 1;
#endif
	return 0; }

int fsc_delete_file(void *os_path) {
	// Returns 1 on error, 0 on success
	#ifdef _WIN32
		// Can we just use remove instead?
		if(!DeleteFile((LPCTSTR)os_path)) return 1;
	#else
		if(!remove((const char *)os_path)) return 1;
	#endif
	return 0; }

int fsc_mkdir(void *os_path) {
	// Returns 1 on error, 0 on success or directory already exists
	#ifdef _WIN32
		if(CreateDirectory((LPCTSTR)os_path, 0)) return 0;
		return GetLastError() != ERROR_ALREADY_EXISTS;
	#else
		if(mkdir((const char *)os_path, 0750)) return errno != EEXIST;
	#endif
	return 0; }

void *fsc_open_file(const void *os_path, const char *mode) {
	#ifdef WIN_UNICODE
		int i;
		wchar_t mode_wide[10];
		for(i=0; i<9; ++i) {
			mode_wide[i] = mode[i];
			if(!mode[i]) break; }
		mode_wide[9] = 0;
		return _wfopen((const wchar_t *)os_path, mode_wide);
	#else
		return fopen((const char *)os_path, mode);
	#endif
}

void fsc_fclose(void *fp) {
	fclose((FILE *)fp); }

unsigned int fsc_fread(void *dest, int size, void *fp) {
	return fread(dest, 1, size, (FILE *)fp); }

unsigned int fsc_fwrite(const void *src, int size, void *fp) {
	return fwrite(src, 1, size, (FILE *)fp); }

void fsc_fflush(void *fp) {
	fflush((FILE *)fp); }

int fsc_fseek(void *fp, int offset, fsc_seek_type_t type) {
	int os_type = SEEK_SET;
	if(type == FSC_SEEK_CUR) os_type = SEEK_CUR;
	if(type == FSC_SEEK_END) os_type = SEEK_END;
	return fseek((FILE *)fp, offset, os_type); }

int fsc_fseek_set(void *fp, unsigned int offset) {
	// Returns 1 on error, 0 otherwise
	fsc_seek_type_t type = FSC_SEEK_SET;
	do {
		unsigned int seek_amount = offset;
		if(seek_amount > 2000000000) seek_amount = 2000000000;
		if(fsc_fseek(fp, (int)seek_amount, type)) return 1;
		offset -= seek_amount;
		type = FSC_SEEK_CUR; }
	while(offset);
	return 0; }

unsigned int fsc_ftell(void *fp) {
	// Returns 4294967295 on error or overflow
	#ifdef _WIN32
		long long value = _ftelli64((FILE *)fp);
	#else
		off_t value = ftello((FILE *)fp);
	#endif
		if(value < 0 || value > 4294967295u) return 4294967295u;
		return (unsigned int)value; }

void fsc_memcpy(void *dst, const void *src, unsigned int size) {
	memcpy(dst, src, size); }

int fsc_memcmp(const void *str1, const void *str2, unsigned int size) {
	return memcmp(str1, str2, size); }

void fsc_memset(void *dst, int value, unsigned int size) {
	memset(dst, value, size); }

void fsc_strncpy(char *dst, const char *src, unsigned int size) {
	// Ensures null termination if size > 0
	strncpy(dst, src, size);
	if(size) dst[size-1] = 0; }

void fsc_strncpy_lower(char *dst, const char *src, unsigned int size) {
	if(size) {
		while(--size) *(dst++) = tolower(*(src++));
		*dst = 0; } }

int fsc_strcmp(const char *str1, const char *str2) {
	return strcmp(str1, str2); }

int fsc_stricmp(const char *str1, const char *str2) {
#ifdef _WIN32
	return _stricmp(str1, str2);
#else
	return strcasecmp(str1, str2);
#endif
}

int fsc_strlen(const char *str) {
	return strlen(str); }

void *fsc_malloc(unsigned int size) {
	return malloc(size); }

void *fsc_calloc(unsigned int size) {
	return calloc(size, 1); }

void fsc_free(void *allocation) {
	free(allocation); }

/* ******************************************************************************** */
// OS Path Handling
/* ******************************************************************************** */

#ifdef WIN_UNICODE
#define FSC_CHAR wchar_t
#define FSC_UCHAR unsigned short
#else
#define FSC_CHAR char
#define FSC_UCHAR unsigned char
#endif

void *fsc_string_to_os_path(const char *path) {
	// Converts UTF-8 string to OS path format
	// WARNING: Result must be freed by caller using fsc_free!!!
	FSC_CHAR *buffer;
	#ifdef WIN_UNICODE
		int length = MultiByteToWideChar(CP_UTF8, 0, path, -1, 0, 0);
		if(!length) return 0;

		buffer = (FSC_CHAR *)fsc_malloc(length * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, path, -1, buffer, length);
	#else
		int length = strlen(path);
		buffer = (char *)fsc_malloc(length+1);
		fsc_memcpy(buffer, path, length+1);
	#endif
	return buffer; }

char *fsc_os_path_to_string(const void *os_path) {
	// Converts OS path format to UTF-8 string
	// WARNING: Result must be freed by caller using fsc_free!!!
	char *buffer;
	#ifdef WIN_UNICODE
		int length = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)os_path, -1, 0, 0, 0, 0);
		if(!length) return 0;

		buffer = (char *)fsc_malloc(length);
		WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)os_path, -1, buffer, length, 0, 0);
	#else
		int length = strlen((const char *)os_path);
		buffer = (char *)fsc_malloc(length+1);
		fsc_memcpy(buffer, os_path, length+1);
	#endif
	return buffer; }

static char *fsc_os_path_to_qpath(void *os_path, char *output) {
	// Output buffer must be size FSC_MAX_QPATH
	// This converts special chars to underscores, but does not run the qpath
	// character convestion, so the output qpath is not fully sanitized yet
	FSC_UCHAR *os_path_typed = (FSC_UCHAR *)os_path;
	char *outpos = output;
	char *endpos = output + FSC_MAX_QPATH - 1;
	while(*os_path_typed && outpos < endpos) {
		if(*os_path_typed > 127) {
			// Write an underscore to represent special characters, but not on extended bytes
			#ifdef WIN_UNICODE
			if(!(*os_path_typed >= 0xdc00 && *os_path_typed <= 0xdfff)) *(outpos++) = '_';
			#else
			if(*os_path_typed >= 192) *(outpos++) = '_';
			#endif
		}

		else *(outpos++) = (char)*os_path_typed;
		++os_path_typed; }
	*outpos = 0;

	return output; }

int fsc_os_path_size(const void *os_path) {
	// Length in bytes
#ifdef WIN_UNICODE
	return 2 * wcslen((const wchar_t *)os_path) + 2;
#else
	return strlen((const char *)os_path) + 1;
#endif
}

int fsc_compare_os_path(const void *path1, const void *path2) {
	// Returns 0 if paths are equal
#ifdef WIN_UNICODE
	return wcscmp((const wchar_t *)path1, (const wchar_t *)path2);
#else
	return strcmp((const char *)path1, (const char *)path2);
#endif
}

/* ******************************************************************************** */
// Directory Iteration
/* ******************************************************************************** */

#define SEARCH_PATH_LIMIT 260

typedef struct {
	FSC_CHAR path[SEARCH_PATH_LIMIT];
	int path_position;
	int base_length;

	void (*operation)(iterate_data_t *file_data, void *iterate_context);
	char qpath_buffer[FSC_MAX_QPATH];
	iterate_data_t file_data;
	void *iterate_context;
} iterate_work_t;

// Returns 0 on overflow, 1 on success
static int iterate_append_path(iterate_work_t *iw, const FSC_CHAR *path) {
	while(iw->path_position < SEARCH_PATH_LIMIT) {
		iw->path[iw->path_position] = *path;
		if(!*(path++)) return 1;
		++iw->path_position; }

	iw->path_position = SEARCH_PATH_LIMIT-1;
	iw->path[iw->path_position] = 0;
	return 0; }

static void iterate_set_position(iterate_work_t *iw, int position) {
	iw->path_position = position;
	iw->path[position] = 0; }

#ifdef _WIN32
static void iterate_directory2(iterate_work_t *iw, int junction_allowed) {
	int old_position = iw->path_position;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	if(!iterate_append_path(iw, TEXT("/*"))) return;
	hFind = FindFirstFile(iw->path, &FindFileData);
	//hFind = FindFirstFileEx(search, FindExInfoBasic, &FindFileData, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
	if(hFind == INVALID_HANDLE_VALUE) return;

	do {
		// Prepare path
		iterate_set_position(iw, old_position);
		iterate_append_path(iw, TEXT("/"));
		if(!iterate_append_path(iw, FindFileData.cFileName)) continue;

		if(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			// Have directory - check validity
			if(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT && !junction_allowed) continue;
			if(FindFileData.cFileName[0] == '.' && (!FindFileData.cFileName[1] ||
					(FindFileData.cFileName[1] == '.' && !FindFileData.cFileName[2]))) continue;

			iterate_directory2(iw, junction_allowed); }
		else {
			// Skip files greater than 4GB; they are not currently supported
			if(FindFileData.nFileSizeHigh) continue;

			// Add filename to path, to generate complete os_path
			iw->file_data.os_path = iw->path;
			iw->file_data.qpath_with_mod_dir = fsc_os_path_to_qpath(iw->path + iw->base_length + 1, iw->qpath_buffer);
			iw->file_data.os_timestamp = FindFileData.ftLastWriteTime.dwLowDateTime;
			iw->file_data.filesize = FindFileData.nFileSizeLow;
			iw->operation(&iw->file_data, iw->iterate_context); }
	} while(FindNextFile(hFind, &FindFileData));

	FindClose(hFind); }
#else
static void iterate_directory2(iterate_work_t *iw, int junction_allowed) {
	int old_position = iw->path_position;
	DIR *dir = opendir(iw->path);
	if(!dir) return;

	while(1) {
		struct dirent *entry;
		struct stat st;

		// Get next entry
		entry = readdir(dir);
		if(!entry) break;

		// Get path and stat
		iterate_set_position(iw, old_position);
		iterate_append_path(iw, "/");
		if(!iterate_append_path(iw, entry->d_name)) continue;
		if(stat(iw->path, &st) == -1) continue;

		if(entry->d_type & DT_DIR) {
			// Have directory - check validity
			if(entry->d_name[0] == '.' && (!entry->d_name[1] ||
					(entry->d_name[1] == '.' && !entry->d_name[2]))) continue;

			iterate_directory2(iw, junction_allowed); }
		else {
			// Skip files greater than 4GB; they are not currently supported
			if(st.st_size > 4294967295u) continue;

			iw->file_data.os_path = iw->path;
			iw->file_data.qpath_with_mod_dir = fsc_os_path_to_qpath(iw->path + iw->base_length + 1, iw->qpath_buffer);
			iw->file_data.os_timestamp = (unsigned int)st.st_mtime;
			iw->file_data.filesize = (unsigned int)st.st_size;
			iw->operation(&iw->file_data, iw->iterate_context); } }

	closedir(dir); }
#endif

void iterate_directory(void *search_os_path, void (operation)(iterate_data_t *file_data,
			void *iterate_context), void *iterate_context) {
	iterate_work_t iw;
	iw.path_position = 0;
	iw.operation = operation;
	iw.iterate_context = iterate_context;

	iterate_append_path(&iw, (const FSC_CHAR *)search_os_path);
	iw.base_length = iw.path_position;

	iterate_directory2(&iw, 0); }

#endif	// NEW_FILESYSTEM
