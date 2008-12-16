/*
 *	pax_hdr.h - Provides the highest level interface to the pax library
 * including setting and getting standardized fields, as well as input/output
 * of the whole header.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */


#ifndef _PAX_HDR_H
#define	_PAX_HDR_H

#include <sys/types.h>

/* SAM-FS includes */
#include <pax_hdr/pax_err.h>
#include <pax_hdr/pax_pair.h>
#include <pax_hdr/pax_xhdr.h>
#include <sam/types.h>

#define	PAX_HDR_BLK_SIZE PAX_XHDR_BLK_SIZE
#define	PAX_USTAR_MAGIC "ustar"
#define	PAX_USTAR_VERSION "00"
#define	USTAR_NUMBER_BASE 8

#define	PAX_TYPE_FILE '0'
#define	PAX_TYPE_LINK '1'
#define	PAX_TYPE_SYMLINK '2'
#define	PAX_TYPE_CHAR_SPECIAL '3'
#define	PAX_TYPE_BLOCK_SPECIAL '4'
#define	PAX_TYPE_DIRECTORY '5'
#define	PAX_TYPE_FIFO '6'
#define	PAX_TYPE_HI_PERF '7'

#define	PAX_TYPE_EXTENDED_HEADER 'x'
#define	PAX_TYPE_GLOBAL_HEADER 'g'

typedef int
(*pax_time_conv_t)(
	void *system_time,
	pax_time_t *pax_time);

typedef struct pax_ustar_hdr {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char type;
	char linkname[100];
	struct magic {
		char magic[6];
		char version[2];
	} magic;
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix [155];
} pax_ustar_hdr_t;

typedef union pax_hdr_blk {
	char blk[PAX_HDR_BLK_SIZE];
	pax_ustar_hdr_t hdr;
} pax_hdr_blk_t;

typedef struct pax_hdr {
	/*
	 * standard ustar header block:
	 * always contains mode, devmajor, devminor (these have no standard
	 * pax keyword)
	 * Will contain fields that do not require a pax extended header as
	 * well.
	 */
	pax_hdr_blk_t hdr_blk;

	/*
	 * a list of pairs of data that will go in the extened header data
	 * because they don't fit in the fields in the standard block
	 */
	pax_pair_t *xhdr_list;
} pax_hdr_t;

/* see below */
typedef enum pax_gen_name_type {
	PAX_XHDR_NAME,
	PAX_FILE_NAME,
} pax_gen_name_type_t;

/*
 * Callback specified by the client of the library to provide a suitable
 * name to put into both the 'x' and file ustar header blocks when there is
 * extended header data.  The intention is to let the client specify how this
 * name is generated.
 *
 * For all files with extended header data, this callback will be called with
 * name_type = PAX_XHDR_NAME to generate a name for the extended header data
 * ustar block.
 *
 * For files with their names in the extended header data, this callback will
 * also be called with name_type = PAX_FILE_NAME to generate a name for the
 * file's ustar header block.
 *
 * The names returned for PAX_XHDR_NAME and PAX_FILE_NAME should be
 * correlated in some way such that when the archive is extracted with a
 * non-pax archiver, a sysadmin can go sort out the mess easily.
 *
 * If possible, it may be preferable to use only the name portion of the
 * header block for maximum portability with non-POSIX tar implemenations
 * (old GNU, for instance).  However, the prefix field is provided as a means
 * to allow a longer name than fits in the name field alone.  Clients are
 * guaranteed that the prefix field is not used by the library for other
 * purposes.  This was a neat side-effect of the original programmer's
 * laziness about worrying about whether or not you could split a filename
 * when you had a whole extended header region to use anyway ;-)
 *
 * Callers can expect that the sizes passed in name_size and prefix_size are
 * 100 and 155 bytes, respectively.  The strings written to those buffers
 * must be at most 99 and 154 bytes respectively.  Strings must be NULL
 * terminated and filled.
 *
 * increment_serial provides a mechanism for maintaining consistent generated
 * names between invocations.  The callback may be called twice for files with
 * extended headers, the first call will be made with a non-zero value in
 * increment_serial, the second will be made with a zero value.
 *
 * Generating a name must only be done for files where the actual name isn't
 * in the header block.  Otherwise, the generated name will over-write the
 * actual name in the archive, making the original name unrecoverable.
 * This may be tested by checking if strlen(actual_name) >= name_size.  If it
 * is, a name must be generated, and the actual name is in the extended header
 * data.  For file with extended header data but without their names in the
 * extended header, the actual name should be copied verbatim into the
 * name_buffer.
 */
typedef int (*pax_filename_callback_t)(
	char *name_buffer,
	size_t name_size,
	char *prefix_buffer,
	size_t prefix_size,
	const char *actual_name,
	int increment_serial,
	pax_gen_name_type_t name_type);

/*
 * used by the following implementation of a name callback to generate names
 * for the ustar header blocks.
 */
#define	PH_XHDR_PREFIX "pax__"
#define	PH_FILE_PREFIX "file_"

/*
 * provides a sample, simple possible implementation of a name callback
 */
int
ph_basic_name_callback(
	char *name_buffer,
	size_t name_size,
	char *prefix_buffer,
	size_t prefix_size,
	const char *actual_name,
	int increment_serial,
	pax_gen_name_type_t name_type);

int
ph_generate_short_name(
	char *buffer,
	size_t buffer_size,
	int increment_serial);

/*
 * Loads a buffer into a pax header, handling extended and standard headers
 * appropriately.  Verifies the header using ph_is_pax_hdr before loading.
 */
int
ph_load_pax_hdr(pax_hdr_t *hdr, char *buffer, size_t buffer_len);

/*
 * Creates a new header.
 */
pax_hdr_t *
ph_create_hdr();

/*
 * Destroys an existing header, freeing the associated memory.
 */
void
ph_destroy_hdr(
	pax_hdr_t *hdr);

/*
 * Will determine if a 512 byte block contains a pax header, and if it does
 * how many more bytes make up the complete header data for the file.  512
 * bytes is the size of a ustar header block as specified in the pax standard.
 *
 * Tests block by comparing the magic in the block to the
 * correct magic for a ustar header block.  Then checks if type is either 'x'
 * or one of the other allowable ustar types.
 *
 * Returns:
 * PX_SUCCESS_STD_HEADER for pax headers without exteneded header data
 * PX_SUCCESS_EXT_HEADER for pax headers with extended header data
 * PX_SUCCESS_GLOBAL_HEADER for pax global headers -- NOT YET IMPLEMENTED
 *     (returns PX_ERROR_NOT_SUPPORTED currently)
 * PX_ERROR_NOT_PAX_HEADER if the block could not be identified as a pax header
 *
 * On successful return, additional bytes will be the number of additional
 * bytes that comprise the complete header.  For extended headers, this will
 * be the length specified in block, rounded up to a multiple of 512 (if
 * needed) + 512.  For standard headers, this will be zero.
 */
int ph_is_pax_hdr(
	char block[PAX_HDR_BLK_SIZE],
	size_t *additional_bytes);

/*
 * Determines whether or not a header has extened header data
 * Returns
 * 0 if not
 * > 1 if it does
 * < 1 if there was an error.  This value can be multiplied by -1 to get a
 *     pax error code
 */
int ph_has_ext_hdr(
	pax_hdr_t *hdr);

/*
 * Determines the size required to output the header, in bytes.  This includes
 * the ustar block for the extended header data, the exthended header data
 * and the ustar block for the file.  For files with no extended header data
 * this will be PAX_HDR_BLK_SIZE, for files with extended header data, this
 * will be a multiple of PAX_HDR_BLK_SIZE not less than 3 * PAX_HDR_BLK_SIZE.
 * xhdr_blk_size is optional, and will contain the size required for the
 * x header data on return if it is provided.
 */
int ph_get_header_size(
	pax_hdr_t *hdr,
	size_t *hdr_size,
	size_t *xhdr_exact_size,
	size_t *xhdr_blk_size);

int ph_write_header(
	pax_hdr_t *hdr,
	char *buffer,
	size_t buffer_size,
	size_t *written,
	pax_hdr_t *defaults,
	pax_filename_callback_t filename_callback);

/*
 * The following set functions will set the value in the pax header in either
 * the block or the extended header data, whichever is appropriate.  String
 * data is copied by functions that set string values.
 */
int ph_set_name(
	pax_hdr_t *hdr,
	char *name);

int ph_set_linkname(
	pax_hdr_t *hdr,
	char *linkname);

int ph_set_uname(
	pax_hdr_t *hdr,
	char *uname);

int ph_set_gname(
	pax_hdr_t *hdr,
	char *gname);

int ph_set_uid(
	pax_hdr_t *hdr,
	uid_t uid);

int ph_set_gid(
	pax_hdr_t *hdr,
	gid_t gid);

int ph_set_size(
	pax_hdr_t *hdr,
	offset_t size);

int ph_set_mtime(
	pax_hdr_t *hdr,
	pax_time_t mtime);

int ph_set_atime(
	pax_hdr_t *hdr,
	pax_time_t atime);

int ph_set_type(
	pax_hdr_t *hdr,
	char type);

/*
 * The following set functions only set the value in the ustar header block
 * and return a failure code if the required length exceeded that available
 * in the header block.  Otherwise they return PX_SUCCESS.
 */
int ph_set_mode(
	pax_hdr_t *hdr,
	mode_t mode);

int ph_set_devmajor(
	pax_hdr_t *hdr,
	dev_t devmajor);

int ph_set_devminor(
	pax_hdr_t *hdr,
	dev_t devminor);

/*
 * The get functions return the value of the requested value, whether it's in
 * the ustar header block or the extended data.
 * Functions return zeroes or NULL strings if the values have not been set.
 */
int ph_get_name(
	pax_hdr_t *hdr,
	const char **name);

int ph_get_uid(
	pax_hdr_t *hdr,
	uid_t *uid);

int ph_get_gid(
	pax_hdr_t *hdr,
	gid_t *gid);

int ph_get_size(
	pax_hdr_t *hdr,
	offset_t *size);

int ph_get_mtime(
	pax_hdr_t *hdr,
	pax_time_t *mtime);

int ph_get_atime(
	pax_hdr_t *hdr,
	pax_time_t *atime);

int ph_get_type(
	pax_hdr_t *hdr,
	char *type);

int ph_get_linkname(
	pax_hdr_t *hdr,
	const char **linkname);

int ph_get_uname(
	pax_hdr_t *hdr,
	const char **uname);

int ph_get_gname(
	pax_hdr_t *hdr,
	const char **gname);

int ph_get_mode(
	pax_hdr_t *hdr,
	mode_t *mode);

int ph_get_devmajor(
	pax_hdr_t *hdr,
	dev_t *devmajor);

int ph_get_devminor(
	pax_hdr_t *hdr,
	dev_t *devminor);

#endif /* _PAX_HDR_H */
