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

#ifdef USE_LOCAL_HEADERS
#include "../../zlib/zlib.h"
#else
#include <zlib.h>
#endif

#define STACKPTR(pointer) ( fsc_stack_retrieve(&fs->general_stack, pointer) )
#define STACKPTRL(pointer) ( fsc_stack_retrieve(stack, pointer) )	// stack is a local parameter

// Somewhat arbitrary limit to avoid overflow issues
#define FSC_MAX_PK3_SIZE 4240000000u

/* ******************************************************************************** */
// PK3 File Processing
/* ******************************************************************************** */

typedef struct {
	char *data;
	int cd_length;
	unsigned int zip_offset;
	int entry_count;
} central_directory_t;

static int fsc_endian_check(void) {
	// Returns 1 on little endian system, 0 on big endian system
	// WARNING: Currently untested on big endian system
	static volatile int test = 1;
	if (*(char *)&test) return 1;
	return 0; }

static unsigned int fsc_endian_convert_int(unsigned int value) {
	if(fsc_endian_check()) return value;
    value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
    return (value << 16) | (value >> 16); }

static unsigned short fsc_endian_convert_short(unsigned short value) {
	if(fsc_endian_check()) return value;
	return (value << 8) | (value >> 8); }

static int get_pk3_central_directory_fp(void *fp, unsigned int file_length, central_directory_t *output) {
	// Returns 1 on error, 0 on success
	char buffer[66000];
	int buffer_read_size = 0;	// Offset from end of file/buffer of data that has been successfully read to buffer
	int eocd_position = 0;	// Offset from end of file/buffer of the EOCD record
	unsigned int cd_position;	// Offset from beginning of file of central directory

	// End Of Central Directory Record (EOCD) can start anywhere in the last 65KB or so of the zip file,
	// determined by the presence of a magic number. For performance purposes first scan the last 4KB of the file,
	// and if the magic number isn't found do a second pass for the whole 65KB.
	int pass;
	for(pass=0; pass<2; ++pass) {
		// Get buffer_read_target, which is the offset from end of file/buffer that we are tring to read to buffer
		int i;
		int buffer_read_target = pass == 0 ? 4096 : sizeof(buffer);
		if((unsigned int)buffer_read_target > file_length) buffer_read_target = file_length;
		if(buffer_read_target <= buffer_read_size) return 1;

		// Read the data
		fsc_fseek_set(fp, file_length - buffer_read_target);
		fsc_fread(buffer + sizeof(buffer) - buffer_read_target, buffer_read_target - buffer_read_size, fp);

		// Search for magic number
		for(i=22; i<buffer_read_target; ++i) {	// EOCD cannot start less than 22 bytes from end of file, because it is 22 bytes long
			char *string = buffer + sizeof(buffer) - i;
			if(string[0] == 0x50 && string[1] == 0x4b && string[2] == 0x05 && string[3] == 0x06) {
				eocd_position = i;
				break; } }

		buffer_read_size = buffer_read_target;
		if(eocd_position) break; }

	if(!eocd_position) return 1;

	#define EOCD_SHORT(offset) fsc_endian_convert_short(*(unsigned short *)(buffer + sizeof(buffer) - eocd_position + offset))
	#define EOCD_INT(offset) fsc_endian_convert_int(*(unsigned int *)(buffer + sizeof(buffer) - eocd_position + offset))

	output->entry_count = EOCD_SHORT(8);
	output->cd_length = EOCD_INT(12);

	// No reason for central directory to be over 100 MB
	if(output->cd_length < 0 || output->cd_length > 100<<20) return 1;

	// Must be space for central directory between the beginning of the file and the EOCD start
	if((unsigned int)output->cd_length > file_length - eocd_position) return 1;

	// EOCD sanity checks that were in the original code: something to do with ensuring this is not a spanned archive
	if(EOCD_INT(4) != 0) return 1;	// "this file's disk number" and "disk number containing central directory" should both be 0
	if(EOCD_SHORT(8) != EOCD_SHORT(8)) return 1;	// "cd entries on this disk" and "cd entries total" should be equal

	// Determine real central directory position
	cd_position = file_length - eocd_position - output->cd_length;

	// Determine zip offset from error in reported cd position - all file offsets need to be adjusted by this value
	{	unsigned int cd_position_reported = EOCD_INT(16);
		if(cd_position_reported > cd_position) return 1;	// cd_position is already the maximum valid position
		output->zip_offset = cd_position - cd_position_reported; }

	output->data = (char *)fsc_malloc(output->cd_length);

	{	unsigned int buffer_file_position = file_length - buffer_read_size;		// Position in file where buffered data begins
		unsigned int unbuffered_read_length = 0;	// Amount of non-buffered data read
		if(cd_position < buffer_file_position) {
			// Since central directory starts before buffer, read unbuffered part of central directory into output
			unbuffered_read_length = buffer_file_position - cd_position;
			if(unbuffered_read_length > (unsigned int)output->cd_length) unbuffered_read_length = output->cd_length;
			fsc_fseek_set(fp, cd_position);
			fsc_fread(output->data, unbuffered_read_length, fp); }
		if(unbuffered_read_length < (unsigned int)output->cd_length) {
			// Read remaining data from buffer into output
			char *buffer_data = buffer + sizeof(buffer) - buffer_read_size;
			if(cd_position > buffer_file_position) buffer_data += cd_position - buffer_file_position;
			fsc_memcpy(output->data + unbuffered_read_length, buffer_data, output->cd_length - unbuffered_read_length); } }

	return 0; }

static int get_pk3_central_directory_path(void *os_path, central_directory_t *output,
			fsc_file_direct_t *source_file, fsc_errorhandler_t *eh) {
	// Returns 1 on error, 0 on success
	void *fp = 0;
	unsigned int length;

	// Open file
	fp = fsc_open_file(os_path, "rb");
	if(!fp) {
		fsc_report_error(eh, FSC_ERROR_PK3FILE, "error opening pk3", source_file);
		return 1; }

	// Get size
	fsc_fseek(fp, 0, FSC_SEEK_END);
	length = fsc_ftell(fp);
	if(!length) {
		fsc_fclose(fp);
		fsc_report_error(eh, FSC_ERROR_PK3FILE, "zero size pk3", source_file);
		return 1; }
	if(length > FSC_MAX_PK3_SIZE) {
		fsc_fclose(fp);
		fsc_report_error(eh, FSC_ERROR_PK3FILE, "excessively large pk3", source_file);
		return 1; }

	// Get central directory
	if(get_pk3_central_directory_fp(fp, length, output)) {
		fsc_fclose(fp);
		fsc_report_error(eh, FSC_ERROR_PK3FILE, "error retrieving pk3 central directory", source_file);
		return 1; }
	fsc_fclose(fp);

	return 0; }

void register_pk3_hash_lookup_entry(fsc_stackptr_t pk3_file_ptr, fsc_hashtable_t *pk3_hash_lookup, fsc_stack_t *stack) {
	fsc_file_direct_t *pk3_file = (fsc_file_direct_t *)STACKPTRL(pk3_file_ptr);
	fsc_stackptr_t hash_map_entry_ptr = fsc_stack_allocate(stack, sizeof(fsc_pk3_hash_map_entry_t));
	fsc_pk3_hash_map_entry_t *hash_map_entry = (fsc_pk3_hash_map_entry_t *)STACKPTRL(hash_map_entry_ptr);
	hash_map_entry->pk3 = pk3_file_ptr;
	fsc_hashtable_insert(hash_map_entry_ptr, pk3_file->pk3_hash, pk3_hash_lookup); }

static void register_file_from_pk3(fsc_filesystem_t *fs, char *filename, int filename_length,
			fsc_stackptr_t sourcefile_ptr, unsigned int header_position, unsigned int compressed_size,
			unsigned int uncompressed_size, short compression_method, fsc_errorhandler_t *eh) {
	fsc_file_direct_t *sourcefile = (fsc_file_direct_t *)STACKPTR(sourcefile_ptr);
	fsc_stackptr_t file_ptr = fsc_stack_allocate(&fs->general_stack, sizeof(fsc_file_frompk3_t));
	fsc_file_frompk3_t *file = (fsc_file_frompk3_t *)STACKPTR(file_ptr);

	char buffer[FSC_MAX_QPATH];
	const char *qp_dir, *qp_name, *qp_ext;
	unsigned int fs_hash;
	int filename_contents;

	// Copy filename into null-terminated buffer for process_qpath
	// Also convert to lowercase to match behavior of old filesystem
	if(filename_length > FSC_MAX_QPATH - 1) filename_length = FSC_MAX_QPATH - 1;
	fsc_strncpy_lower(buffer, filename, filename_length+1);

	// Call process_qpath
	fsc_process_qpath(buffer, buffer, &qp_dir, &qp_name, &qp_ext);
	fs_hash = fsc_string_hash(qp_name, qp_dir);

	// Write qpaths to file structure
	file->f.qp_dir_ptr = qp_dir ? fsc_string_repository_getstring(qp_dir, 1, &fs->string_repository, &fs->general_stack) : 0;
	file->f.qp_name_ptr = fsc_string_repository_getstring(qp_name, 1, &fs->string_repository, &fs->general_stack);
	file->f.qp_ext_ptr = qp_ext ? fsc_string_repository_getstring(qp_ext, 1, &fs->string_repository, &fs->general_stack) : 0;

	// Load the rest of the fields
	file->f.sourcetype = FSC_SOURCETYPE_PK3;
	file->source_pk3 = sourcefile_ptr;
	file->header_position = header_position;
	file->compressed_size = compressed_size;
	file->compression_method = compression_method;
	file->f.filesize = uncompressed_size;

	// Register file and contents
	fsc_hashtable_insert(file_ptr, fs_hash, &fs->files);
	fsc_iteration_register_file(file_ptr, &fs->directories, &fs->string_repository, &fs->general_stack);
	++sourcefile->pk3_subfile_count;

	filename_contents = fsc_filename_contents(qp_dir, qp_name, qp_ext);
	if(filename_contents & FSC_CONTENTS_SHADER) {
		++sourcefile->shader_file_count;
		sourcefile->shader_count += index_shader_file(fs, file_ptr, eh); }
	if(filename_contents & FSC_CONTENTS_CROSSHAIR) {
		index_crosshair(fs, file_ptr, eh); } }

void fsc_load_pk3(void *os_path, fsc_filesystem_t *fs, fsc_stackptr_t sourcefile_ptr, fsc_errorhandler_t *eh,
				void (receive_hash_data)(void *context, char *data, int size), void *receive_hash_data_context ) {
	// In normal usage, this is used to load pk3 files into the index. It can also be called with
	//    receive_hash_data set to generate pk3 hash data without indexing anything.
	fsc_file_direct_t *sourcefile = (fsc_file_direct_t *)STACKPTR(sourcefile_ptr);
	central_directory_t cd;
	int entry_position = 0;		// Position of current entry relative to central directory data
	int entry_counter = 0;		// Number of current entry

	int filename_length;
	int entry_length;
	unsigned int uncompressed_size;
	unsigned int compressed_size;
	unsigned int header_position;

	int *crcs_for_hash;
	int crcs_for_hash_buffer[1024];
	int crcs_for_hash_count = 0;

	// Load central directory
	if(get_pk3_central_directory_path(os_path, &cd, sourcefile, eh)) return;

	// Try to use the stack buffer, but if it's not big enough resort to malloc
	if(cd.entry_count > sizeof(crcs_for_hash_buffer) / sizeof(*crcs_for_hash_buffer)) {
		crcs_for_hash = (int *)fsc_malloc((cd.entry_count + 1) * 4); }
	else crcs_for_hash = crcs_for_hash_buffer;

	// Process each file
	while(1) {
		// Make sure there is enough space to read the entry (minimum 47 bytes if filename is 1 byte)
		if(entry_position + 47 > cd.cd_length) {
			fsc_report_error(eh, FSC_ERROR_PK3FILE, "invalid file cd entry position", sourcefile);
			goto freemem; }

		// Verify magic number
		if(cd.data[entry_position] != 0x50 || cd.data[entry_position+1] != 0x4b
				|| cd.data[entry_position+2] != 0x01 || cd.data[entry_position+3] != 0x02) {
			fsc_report_error(eh, FSC_ERROR_PK3FILE, "file cd entry does not have correct signature", sourcefile);
			goto freemem; }

		#define CD_ENTRY_SHORT(offset) fsc_endian_convert_short(*(unsigned short *)(cd.data + entry_position + offset))
		#define CD_ENTRY_INT(offset) fsc_endian_convert_int(*(unsigned int *)(cd.data + entry_position + offset))
		#define CD_ENTRY_INT_LE(offset) (*(unsigned int *)(cd.data + entry_position + offset))

		// Get filename_length and entry_length
		filename_length = (int)CD_ENTRY_SHORT(28);
		{	int extrafield_length = (int)CD_ENTRY_SHORT(30);
			int comment_length = (int)CD_ENTRY_SHORT(32);
			entry_length = 46 + filename_length + extrafield_length + comment_length;
			if(entry_position + entry_length > cd.cd_length) {
				fsc_report_error(eh, FSC_ERROR_PK3FILE,
					"invalid file cd entry position 2", sourcefile);
				goto freemem; } }

		// Get compressed_size and uncompressed_size
		compressed_size = CD_ENTRY_INT(20);
		uncompressed_size = CD_ENTRY_INT(24);

		// Get local header_position (which is indicated by CD header, but needs to be modified by zip offset)
		header_position = CD_ENTRY_INT(42) + cd.zip_offset;

		// Sanity checks
		if(header_position + compressed_size < header_position) {
			fsc_report_error(eh, FSC_ERROR_PK3FILE, "invalid file local entry position 1", sourcefile); goto freemem; }
		if(header_position + compressed_size > FSC_MAX_PK3_SIZE) {
			fsc_report_error(eh, FSC_ERROR_PK3FILE, "invalid file local entry position 2", sourcefile); goto freemem; }

		if(uncompressed_size) {
			crcs_for_hash[crcs_for_hash_count++] = CD_ENTRY_INT_LE(16); }

		if(!(void *)receive_hash_data && !(!uncompressed_size && *(cd.data+entry_position+46+filename_length-1) == '/')) {
			// Not in hash mode and not a directory entry - load the file
			register_file_from_pk3(fs, cd.data+entry_position+46, filename_length, sourcefile_ptr,
					header_position, compressed_size, CD_ENTRY_INT(24), CD_ENTRY_SHORT(10), eh); }

		++entry_counter;
		entry_position += entry_length;
		if(entry_counter >= cd.entry_count) break; }		// Abort if that was the last file

	if((void *)receive_hash_data) {
		receive_hash_data(receive_hash_data_context, (char *)crcs_for_hash, crcs_for_hash_count * 4);
		goto freemem; }

	sourcefile->pk3_hash = fsc_block_checksum(crcs_for_hash, crcs_for_hash_count * 4);

	// Add the pk3 to the hash lookup table
	register_pk3_hash_lookup_entry(sourcefile_ptr, &fs->pk3_hash_lookup, &fs->general_stack);

	freemem:
	fsc_free(cd.data);
	if(crcs_for_hash != crcs_for_hash_buffer) fsc_free(crcs_for_hash); }

/* ******************************************************************************** */
// PK3 Extraction Support
/* ******************************************************************************** */

typedef struct {
	void *input_handle;
	int compression_method;
	unsigned int input_remaining;	// Remaining to be read from input handle

	// For zlib streams only
	unsigned int input_buffer_size;
	char *input_buffer;
	z_stream zlib_stream;
} pk3_handle_t;

static int fsc_pk3_handle_open2(pk3_handle_t *handle, const fsc_file_frompk3_t *file, int input_buffer_size,
		const fsc_filesystem_t *fs, fsc_errorhandler_t *eh) {
	// Returns 1 on error, 0 otherwise
	const fsc_file_direct_t *source_pk3 = (const fsc_file_direct_t *)STACKPTR(file->source_pk3);
	char localheader[30];
	unsigned int data_position;

	// Open the file
	handle->input_handle = fsc_open_file(STACKPTR(source_pk3->os_path_ptr), "rb");
	if(!handle->input_handle) {
		fsc_report_error(eh, FSC_ERROR_EXTRACT, "pk3_handle_open - failed to open pk3 file", 0);
		return 1; }

	// Read the local header to get data position
	fsc_fseek_set(handle->input_handle, file->header_position);
	if(fsc_fread(localheader, 30, handle->input_handle) != 30) {
		fsc_report_error(eh, FSC_ERROR_EXTRACT, "pk3_handle_open - failed to read local header", 0);
		return 1; }
	if(localheader[0] != 0x50 || localheader[1] != 0x4b || localheader[2] != 0x03 || localheader[3] != 0x04) {
		fsc_report_error(eh, FSC_ERROR_EXTRACT, "pk3_handle_open - incorrect signature in local header", 0);
		return 1; }
	#define LH_SHORT(offset) fsc_endian_convert_short(*(unsigned short *)(localheader + offset))
	data_position = file->header_position + LH_SHORT(26) + LH_SHORT(28) + 30;

	// Seek to data start position
	fsc_fseek_set(handle->input_handle, data_position);

	// Configure the handle
	handle->input_remaining = file->compressed_size;
	if(file->compression_method == 8) {
		if(inflateInit2(&handle->zlib_stream, -MAX_WBITS) != Z_OK) {
			fsc_report_error(eh, FSC_ERROR_EXTRACT, "pk3_handle_open - zlib inflateInit failed", 0);
			return 1; }

		handle->compression_method = 8;
		handle->input_buffer_size = input_buffer_size;
		handle->input_buffer = (char *)fsc_malloc(input_buffer_size); }
	else if(file->compression_method != 0) {
		fsc_report_error(eh, FSC_ERROR_EXTRACT, "pk3_handle_open - unknown compression method", 0);
		return 1; }

	return 0; }

void *fsc_pk3_handle_open(const fsc_file_frompk3_t *file, int input_buffer_size, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh) {
	// Returns handle on success, null on error
	pk3_handle_t *handle = (pk3_handle_t *)fsc_calloc(sizeof(*handle));
	if(fsc_pk3_handle_open2(handle, file, input_buffer_size, fs, eh)) {
		if(handle->input_handle) fsc_fclose(handle->input_handle);
		fsc_free(handle);
		return 0; }
	return (void *)handle; }

void fsc_pk3_handle_close(void *handle) {
	pk3_handle_t *Handle = (pk3_handle_t *)handle;
	if(Handle->input_handle) fsc_fclose(Handle->input_handle);
	if(Handle->compression_method == 8) {
		fsc_free(Handle->input_buffer);
		inflateEnd(&Handle->zlib_stream); }
	fsc_free(handle); }

unsigned int fsc_pk3_handle_read(void *handle, char *buffer, unsigned int length) {
	// Returns number of bytes read
	pk3_handle_t *Handle = (pk3_handle_t *)handle;
	if(Handle->compression_method == 8) {
		Handle->zlib_stream.next_out = (Bytef *)buffer;
		Handle->zlib_stream.avail_out = length;

		while(Handle->zlib_stream.avail_out) {
			if(!Handle->zlib_stream.avail_in) {
				// Load new batch of data into input
				unsigned int feed_amount = Handle->input_remaining;
				if(feed_amount > Handle->input_buffer_size) feed_amount = Handle->input_buffer_size;
				if(!feed_amount) break;		// Ran out of input
				if(fsc_fread(Handle->input_buffer, (int)feed_amount, Handle->input_handle) != feed_amount) break;
				Handle->zlib_stream.avail_in += feed_amount;
				Handle->input_remaining -= feed_amount;
				Handle->zlib_stream.next_in = (Bytef *)Handle->input_buffer; }

			if(inflate(&Handle->zlib_stream, Z_SYNC_FLUSH) != Z_OK) break; }

		return length - Handle->zlib_stream.avail_out; }
	else {
		return fsc_fread(buffer, length, Handle->input_handle); } }

/* ******************************************************************************** */
// PK3 Sourcetype Operations
/* ******************************************************************************** */

static int fsc_pk3_is_file_active(const fsc_file_t *file, const fsc_filesystem_t *fs) {
	const fsc_file_direct_t *source_pk3 = (const fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3);
	return source_pk3->refresh_count == fs->refresh_count; }

static const char *fsc_pk3_get_mod_dir(const fsc_file_t *file, const fsc_filesystem_t *fs) {
	const fsc_file_direct_t *source_pk3 = (const fsc_file_direct_t *)STACKPTR(((fsc_file_frompk3_t *)file)->source_pk3);
	return (const char *)STACKPTR(source_pk3->qp_mod_ptr); }

static int fsc_pk3_extract_data(const fsc_file_t *file, char *buffer, const fsc_filesystem_t *fs, fsc_errorhandler_t *eh) {
	// Returns 0 on success, 1 on failure
	int result = 0;
	const fsc_file_frompk3_t *File = (fsc_file_frompk3_t *)file;
	void *handle = fsc_pk3_handle_open(File, File->compressed_size, fs, eh);
	if(!handle) return 1;
	if(fsc_pk3_handle_read(handle, buffer, file->filesize) != file->filesize) result = 1;
	fsc_pk3_handle_close(handle);
	return result; }

fsc_sourcetype_t pk3_sourcetype = {FSC_SOURCETYPE_PK3, fsc_pk3_is_file_active,
		fsc_pk3_get_mod_dir, fsc_pk3_extract_data};

#endif	// NEW_FILESYSTEM
