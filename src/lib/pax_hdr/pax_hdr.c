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
 * or https://illumos.org/license/CDDL.
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


static char *_SrcFile = __FILE__;

#include <stdio.h>
#include <string.h>

#include "pax_hdr/pax_err.h"
#include "pax_hdr/pax_hdr.h"
#include "pax_hdr/pax_pair.h"
#include "pax_hdr/pax_util.h"
#include "pax_hdr/pax_xhdr.h"
#include "sam/sam_malloc.h"

/* local includes */
#include "pax_macros.h"

/* ****************** static function prototypes ***************** */

/*
 * Notes on the internal set functions:
 * These all remove existing values, whether they're in the extended data
 * or the header block.  This is done as follows:
 * 1) Remove existing pairs from the extended header data
 * 2) If there's a buffer supplied, and the value is "appropriate" (1) try to
 *    put it in
 * 3) If it doesn't fit due to length limits, zero the buffer, and try to put
 *    the value in as a pair in the extended header (if a keyword is supplied).
 *
 * (1) Appropriate generally means non-negative, and in the case of times, not
 *     having a fractional part.
 *
 * _ph_set_string is slightly special, because it zeroes the buffer before it
 * tries to fill it.  This is because it is not guaranteed to have a full
 * buffer worth of data to copy in, potentially leaving junk after the
 * terminating \0.  All of the other functions are written to fill the entirety
 * of the buffer when they try to format a number into it, so they don't risk
 * leaving trailing junk.
 *
 * Generally, these functions do not do much unneccessary error checking, as
 * they are strictly for internal use.	Notably, passing a null pax_hdr pointer
 * is unchecked.  The exceptions are _ph_set_unsigned and _ph_set_signed,
 * which allows this for
 * setting finalized values of checksum and size into a ustar header block
 * as it's being written.
 */

static int _ph_set_string(
	pax_hdr_t *pax_hdr,
	char *dst,
	int dlen,
	char *src,
	char *xhdr_key);

static int _ph_set_unsigned(
	pax_hdr_t *pax_hdr,
	char *dst,
	size_t dlen,
	uint64_t src,
	char *xhdr_key);

static int _ph_set_signed(
	pax_hdr_t *pax_hdr,
	char *dst,
	size_t dlen,
	int64_t src,
	char *xhdr_key);

static int _ph_set_time(
	pax_hdr_t *pax_hdr,
	char *dst,
	size_t dlen,
	pax_time_t src,
	char *xhdr_key);

/* ********************* static get functions ********************** */

static int
_ph_get_string(
	pax_hdr_t *hdr,
	const char **result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key);

static int
_ph_get_unsigned(
	pax_hdr_t *hdr,
	uint64_t *result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key);

static int
_ph_get_signed(
	pax_hdr_t *hdr,
	int64_t *result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key);

static int
_ph_get_time(
	pax_hdr_t *hdr,
	pax_time_t *result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key);

/* ******************* static utility functions ******************** */

static int
write_merged_ustar_blk(
	pax_hdr_blk_t *blk,
	pax_hdr_t *hdr,
	pax_hdr_t *defaults,
	int xhdr_blk,
	pax_filename_callback_t filename_callback);

static int
checksum(
	pax_hdr_blk_t *blk);
/* ********************* exported function definitions ****************** */

pax_hdr_t *
ph_create_hdr()
{
	pax_hdr_t *hdr = NULL;
	SamMalloc(hdr, sizeof (pax_hdr_t));

	memset(hdr, 0, sizeof (pax_hdr_t));

	/* no overrun risk, it's a compile time constant */
	strcpy(hdr->hdr_blk.hdr.magic.magic, PAX_USTAR_MAGIC);

	/* version is not NULL terminated */
	memcpy(hdr->hdr_blk.hdr.magic.version, PAX_USTAR_VERSION, 2);
	return (hdr);
}

void
ph_destroy_hdr(
	pax_hdr_t *hdr)
{
	if (!hdr) {
		return;
	}

	(void) pxp_destroy_pairs(hdr->xhdr_list);

	SamFree(hdr);
}

int
ph_is_pax_hdr(
	char block[PAX_HDR_BLK_SIZE],
	size_t *additional_bytes)
{
	int status = PX_SUCCESS;
	pax_ustar_hdr_t *uh = (pax_ustar_hdr_t *)block;
	size_t xhdr_size;
	int result;

	if (additional_bytes) {
		*additional_bytes = 0;
	}

	if (strcmp(uh->magic.magic, PAX_USTAR_MAGIC)) {
		ABORT_ST(PX_ERROR_NOT_PAX_HEADER);
	}

	/*
	 * need to use strncmp because the version isn't
	 * NULL terminated.
	 */
	if (strncmp(uh->magic.version, PAX_USTAR_VERSION,
	    strlen(PAX_USTAR_VERSION))) {
		ABORT_ST(PX_ERROR_NOT_PAX_HEADER);
	}

	if (PAX_TYPE_FILE <= uh->type && uh->type <= PAX_TYPE_HI_PERF) {
		ABORT_ST(PX_SUCCESS_STD_HEADER);
	} else if (uh->type == PAX_TYPE_GLOBAL_HEADER) {
		result = PX_SUCCESS_GLOBAL_HEADER;
	} else if (uh->type == PAX_TYPE_EXTENDED_HEADER) {
		result = PX_SUCCESS_EXT_HEADER;
	} else {
		ABORT_ST(PX_ERROR_NOT_PAX_HEADER);
	}

	status = strtosize_t (&xhdr_size, uh->size, NULL, 8);
	TEST_AND_ABORT();

	xhdr_size = round_to_block(xhdr_size, PAX_HDR_BLK_SIZE);

	/*
	 * set additional_bytes, including the xhdr size and the following
	 * ustar header block if it's an 'x' header.
	 */
	if (additional_bytes) {
		*additional_bytes = xhdr_size;
		if (result == PX_SUCCESS_EXT_HEADER) {
			*additional_bytes += PAX_HDR_BLK_SIZE;
		}
	}

	status = result;
out:
	/* global extended headers aren't supported yet */
	if (status == PX_SUCCESS_GLOBAL_HEADER) {
		status = PX_ERROR_NOT_SUPPORTED;
	}

	return (status);
}

int
ph_has_ext_hdr(
	pax_hdr_t *hdr)
{
	if (!hdr) {
		return (-PX_ERROR_INVALID);
	}
	return (hdr->xhdr_list != NULL);
}

int
ph_get_header_size(
	pax_hdr_t *hdr,
	size_t *hdr_size,
	size_t *xhdr_exact_size,
	size_t *xhdr_blk_size)
{
	int status = PX_SUCCESS;
	int have_xhdr;
	size_t xhdr_exact_sz = 0;
	size_t xhdr_blk_sz = 0;

	if (!hdr || !hdr_size) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if ((have_xhdr = ph_has_ext_hdr(hdr)) < 0) {
		ABORT_ST(-have_xhdr);
	}

	if (!have_xhdr) {
		*hdr_size = PAX_HDR_BLK_SIZE;
		ABORT_ST(PX_SUCCESS);
	}

	status = pxh_get_xheader_size(hdr->xhdr_list, &xhdr_exact_sz,
	    &xhdr_blk_sz);
	TEST_AND_ABORT();

	/* two header blocks for a header with extended header data */
	*hdr_size = xhdr_blk_sz + 2 * PAX_HDR_BLK_SIZE;
out:
	if (xhdr_exact_size) {
		*xhdr_exact_size = xhdr_exact_sz;
	}
	if (xhdr_blk_size) {
		*xhdr_blk_size = xhdr_blk_sz;
	}

	return (status);
}

int ph_write_header(
	pax_hdr_t *hdr,
	char *buffer,
	size_t buffer_size,
	size_t *written,
	pax_hdr_t *defaults,
	pax_filename_callback_t filename_callback)
{
	int status = PX_SUCCESS;
	size_t min_buffer_size = PAX_HDR_BLK_SIZE;
	int def_has_xhdr;
	pax_hdr_blk_t *xhdr_ustar_blk = NULL;
	char *xhdr_data = NULL;
	size_t xhdr_exact_size;
	size_t xhdr_blk_size;
	pax_hdr_blk_t *file_ustar_blk = NULL;

	if (!hdr || !buffer) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	status = ph_get_header_size(hdr, &min_buffer_size, &xhdr_exact_size,
	    &xhdr_blk_size);

	if (buffer_size < min_buffer_size) {
		*written = min_buffer_size;
		ABORT_ST(PX_ERROR_WONT_FIT);
	}

	/* don't have to error check here, ph_get_header_size already has */
	if (ph_has_ext_hdr(hdr)) {
		if (!defaults) {
			ABORT_ST(PX_ERROR_REQUIRE_DEFAULTS);
		}

		if (def_has_xhdr = ph_has_ext_hdr(defaults)) {
			if (def_has_xhdr < 0) {
				ABORT_ST(-def_has_xhdr);
			}
			ABORT_ST(PX_ERROR_DEFAULTS_XHDR);
		}
		xhdr_ustar_blk = (pax_hdr_blk_t *)buffer;

		status = write_merged_ustar_blk(xhdr_ustar_blk, hdr,
		    defaults, 1, filename_callback);
		TEST_AND_ABORT();

		status = _ph_set_unsigned(NULL, xhdr_ustar_blk->hdr.size,
		    sizeof (xhdr_ustar_blk->hdr.size), xhdr_exact_size, NULL);

		TEST_AND_ABORT();

		status = checksum(xhdr_ustar_blk);

		xhdr_data = buffer + PAX_HDR_BLK_SIZE;

		status = pxh_write_xheader(hdr->xhdr_list, xhdr_data,
		    xhdr_blk_size);
		TEST_AND_ABORT();

		file_ustar_blk = (pax_hdr_blk_t *)(buffer + PAX_HDR_BLK_SIZE +
		    xhdr_blk_size);
	} else {
		file_ustar_blk = (pax_hdr_blk_t *)buffer;
	}

	status = write_merged_ustar_blk(file_ustar_blk, hdr,
	    defaults, 0, filename_callback);
	TEST_AND_ABORT();

	status = checksum(file_ustar_blk);
	if (written) {
		*written = min_buffer_size;
	}
out:
	return (status);
}

int
ph_basic_name_callback(
	char *name_buffer,
	size_t name_size,
	char *prefix_buffer,
	size_t prefix_size,
	const char *actual_name,
	int increment_serial,
	pax_gen_name_type_t name_type)
{
	int status = PX_SUCCESS;
	char *gen_name = NULL;
	char *name;
	char *prefix;

	if (!name_buffer || !prefix_buffer || !actual_name) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	/*
	 * There's no way the actual name is going to fit in the name
	 * buffer, create a name from whole cloth.  We'll use unix time in
	 * seconds from the epoch and a serial number that we reset every
	 * second.
	 */
	if (strlen(actual_name) >= name_size) {
		SamMalloc(gen_name, name_size);
		name = gen_name;
		memset(name, 0, name_size);

		status = ph_generate_short_name(name, name_size,
		    increment_serial);
		TEST_AND_ABORT();

		if (name_type == PAX_FILE_NAME) {
			prefix = PH_FILE_PREFIX;
		}

	} else {
		name = (char *)actual_name;
		if (name_type == PAX_FILE_NAME) {
			/* don't need to check size, it fits, tested above */
			strcpy(name_buffer, name);
			ABORT_ST(PX_SUCCESS);
		}
	}

	if (name_type == PAX_XHDR_NAME) {
		prefix = PH_XHDR_PREFIX;
	}

	/* length of prefix, slash, and name concatenated */
	if (strlen(prefix) + 1 + strlen(name) < name_size) {
		/* ignore the return, just checed that it fits */
		snprintf(name_buffer, name_size, "%s/%s", prefix, name);
	} else {
		/* no slash is output if prefix is used */
		if (strlen(name) < name_size && strlen(prefix) < prefix_size) {
			/* ditto */
			snprintf(name_buffer, name_size, "%s", name);
			snprintf(prefix_buffer, prefix_size, "%s", prefix);
		} else {
			/* oops */
			ABORT_ST(PX_ERROR_OVERFLOW);
		}
	}
out:
	if (gen_name != NULL) {
		SamFree(gen_name);
	}
	return (status);
}

int
ph_generate_short_name(
	char *buffer,
	size_t buffer_size,
	int increment_serial)
{
	int status = PX_SUCCESS;
	int written;
	static int id_num = 0;
	static time_t last_arch_time = 0;
	time_t arch_time;

	arch_time = time(0);
	if (increment_serial) {
		if (arch_time != last_arch_time) {
			id_num = 0;
			last_arch_time = arch_time;
		} else {
			++id_num;
		}
	}

	/*CONSTCOND*/
	if (sizeof (time_t) == sizeof (int64_t)) {
		written = snprintf(buffer, buffer_size, "%lld_%d",
		    last_arch_time, id_num);
	/*CONSTCOND*/
	} else if (sizeof (time_t) == sizeof (int32_t)) {
		written = snprintf(buffer, buffer_size, "%d_%d",
		    last_arch_time, id_num);
	} else {
		ABORT_ST(PX_ERROR_BAD_TYPE);
	}

	if (written >= buffer_size) {
		ABORT_ST(PX_ERROR_OVERFLOW);
	}
out:
	return (status);
}

int
ph_load_pax_hdr(
	pax_hdr_t *hdr,
	char *buffer,
	size_t buffer_len)
{
	int status = PX_SUCCESS;
	size_t addl_bytes;

	char *xhdr = NULL;
	size_t xhdr_len = 0;

	char *std_hdr = NULL;

	status = ph_is_pax_hdr(buffer, &addl_bytes);
	TEST_AND_ABORT();

	switch (status) {
	case PX_SUCCESS_EXT_HEADER:
		if (addl_bytes + PAX_HDR_BLK_SIZE != buffer_len) {
			ABORT_ST(PX_ERROR_INVALID_BUFFER);
		}

		xhdr = buffer + PAX_HDR_BLK_SIZE;
		xhdr_len = addl_bytes - PAX_HDR_BLK_SIZE;

		std_hdr = buffer + addl_bytes;

		/*
		 * don't care about additional bytes here, std_hdr
		 * had better point to a standard header block which has
		 * no additional header bytes.
		 */
		status = ph_is_pax_hdr(std_hdr, NULL);
		if (status != PX_SUCCESS_STD_HEADER) {
			ABORT_ST(PX_ERROR_INVALID_HEADER);
		}

		break;
	case PX_SUCCESS_STD_HEADER:
		std_hdr = buffer;
		break;
	case PX_SUCCESS_GLOBAL_HEADER:
	default:
		ABORT_ST(PX_ERROR_NOT_SUPPORTED);
	}

	memcpy(&hdr->hdr_blk, std_hdr, PAX_HDR_BLK_SIZE);

	if (xhdr && xhdr_len > 0) {
		status = pxh_mkpairs(&hdr->xhdr_list, xhdr, xhdr_len);
		TEST_AND_ABORT();
	}
out:
	return (status);
}

/*
 * Set functions for specific fields in the header.  A lot of grossness
 * follows because it's largely done with macros to keep from writing a lot
 * of cut/paste code
 */

#define	_PX_SET_STRING_FN(_field_, _alt_) \
int ph_set_##_field_(pax_hdr_t *hdr, char *_field_) \
{ \
	pax_ustar_hdr_t *uh = NULL; \
	 \
	if (!hdr) { \
		return (PX_ERROR_INVALID); \
	} \
	 \
	uh = &hdr->hdr_blk.hdr; \
	return (_ph_set_string(hdr, uh->_field_, sizeof (uh->_field_), \
	    _field_, _alt_)); \
}

_PX_SET_STRING_FN(name, PAX_STD_PATH_KEYWORD)
_PX_SET_STRING_FN(linkname, PAX_STD_LINKPATH_KEYWORD)
_PX_SET_STRING_FN(uname, PAX_STD_UNAME_KEYWORD)
_PX_SET_STRING_FN(gname, PAX_STD_GNAME_KEYWORD)

#undef _PX_SET_STRING_FN

#define	_PX_SET_UNDEF_SIGNED_ALT_FN(_type_, _field_, _alt_) \
int ph_set_##_field_(pax_hdr_t *hdr, _type_ _field_) \
{ \
	pax_ustar_hdr_t *uh = NULL; \
	 \
	if (!hdr) { \
		return (PX_ERROR_INVALID); \
	} \
	 \
	uh = &hdr->hdr_blk.hdr; \
	if ((_type_) -1 > 0 || _field_ >= 0) { \
		return (_ph_set_unsigned(hdr, uh->_field_, \
		    sizeof (uh->_field_), (uint64_t)_field_, _alt_)); \
	} else { \
		return (_ph_set_signed(hdr, uh->_field_, \
		    sizeof (uh->_field_), (int64_t)_field_, _alt_)); \
	} \
}

_PX_SET_UNDEF_SIGNED_ALT_FN(uid_t, uid, PAX_STD_UID_KEYWORD)
_PX_SET_UNDEF_SIGNED_ALT_FN(gid_t, gid, PAX_STD_GID_KEYWORD)

#undef _PX_SET_UNDEF_SIGNED_ALT_FN

int
ph_set_size(
	pax_hdr_t *hdr,
	offset_t size)
{
	pax_ustar_hdr_t *uh = NULL;

	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	uh = &hdr->hdr_blk.hdr;

	return (_ph_set_unsigned(hdr, uh->size, sizeof (uh->size),
	    (uint64_t)size, PAX_STD_SIZE_KEYWORD));
}

int
ph_set_mtime(
	pax_hdr_t *hdr,
	pax_time_t mtime)
{
	pax_ustar_hdr_t *uh = NULL;

	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	uh = &hdr->hdr_blk.hdr;

	return (_ph_set_time(hdr, uh->mtime, sizeof (uh->mtime), mtime,
	    PAX_STD_MTIME_KEYWORD));
}

int
ph_set_atime(
	pax_hdr_t *hdr,
	pax_time_t atime)
{
	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	return (_ph_set_time(hdr, NULL, 0, atime, PAX_STD_ATIME_KEYWORD));
}

int
ph_set_type(
	pax_hdr_t *hdr,
	char type)
{
	pax_ustar_hdr_t *uh = NULL;

	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	uh = &hdr->hdr_blk.hdr;

	uh->type = type;
	return (PX_SUCCESS);
}

/*
 * The following set functions only set the value in the ustar header block
 * and return 0 if the required length exceeded that available in the header
 * block.  Otherwise they return non-zero.
 */

#define	_PX_SET_UNDEF_SIGNED_NO_ALT_FN(_type_, _field_) \
int ph_set_##_field_(pax_hdr_t *hdr, _type_ _field_) \
{ \
	pax_ustar_hdr_t *uh = NULL; \
	 \
	if (!hdr) { \
		return (PX_ERROR_INVALID); \
	} \
	 \
	uh = &hdr->hdr_blk.hdr; \
	if ((_type_) -1 > 0 || _field_ >= 0) { \
		return (_ph_set_unsigned(hdr, uh->_field_, \
		    sizeof (uh->_field_), (uint64_t)_field_, NULL)); \
	} else { \
		return (_ph_set_signed(hdr, uh->_field_, \
		    sizeof (uh->_field_), (int64_t)_field_, NULL)); \
	} \
}

/*LINTED suspicious comparison and logical expression always true*/
_PX_SET_UNDEF_SIGNED_NO_ALT_FN(mode_t, mode)

/*LINTED*/
_PX_SET_UNDEF_SIGNED_NO_ALT_FN(dev_t, devmajor)

/*LINTED*/
_PX_SET_UNDEF_SIGNED_NO_ALT_FN(dev_t, devminor)

#undef _PX_SET_UNDEF_SIGNED_NO_ALT_FN

/*
 * The get functions return the value of the requested value, whether it's in
 * the ustar header block or the extended data.  Strings returned are not
 * copied, and should not be modified.
 * Functions return zeroes or empty strings if the values have not been set.
 */

#define	_PX_GET_STRING_FN(_field_, _alt_) \
int ph_get_##_field_(pax_hdr_t *hdr, const char **_field_) \
{ \
	pax_ustar_hdr_t *uh = NULL; \
	 \
	if (!hdr) { \
		return (PX_ERROR_INVALID); \
	} \
	 \
	uh = &hdr->hdr_blk.hdr; \
	return (_ph_get_string(hdr, _field_, uh->_field_, \
	    sizeof (uh->_field_), _alt_)); \
}

_PX_GET_STRING_FN(name, PAX_STD_PATH_KEYWORD)
_PX_GET_STRING_FN(linkname, PAX_STD_LINKPATH_KEYWORD)
_PX_GET_STRING_FN(uname, PAX_STD_UNAME_KEYWORD)
_PX_GET_STRING_FN(gname, PAX_STD_GNAME_KEYWORD)

#undef _PX_GET_STRING_FN

#define	_PX_GET_UNDEF_SIGNED_ALT_FN(_type_, _field_, _alt_) \
int ph_get_##_field_(pax_hdr_t *hdr, _type_ *_field_) \
{ \
	int status = PX_SUCCESS; \
	int64_t i64val; \
	uint64_t u64val; \
	 \
	pax_ustar_hdr_t *uh = NULL; \
	 \
	if (!hdr) { \
		return (PX_ERROR_INVALID); \
	} \
	 \
	uh = &hdr->hdr_blk.hdr; \
	if ((_type_) -1 > 0) { \
		status = _ph_get_unsigned(hdr, &u64val, \
		    uh->_field_, sizeof (uh->_field_), _alt_); \
		*_field_ = (_type_) u64val; \
	} else { \
		status = _ph_get_signed(hdr, &i64val, \
		    uh->_field_, sizeof (uh->_field_), _alt_); \
		*_field_ = (_type_) i64val; \
	} \
	return (status); \
}

/*LINTED CONSTCOND in macro (CONSTCOND doesn't work here)*/
_PX_GET_UNDEF_SIGNED_ALT_FN(uid_t, uid, PAX_STD_UID_KEYWORD)

/*LINTED CONSTCOND in macro (CONSTCOND doesn't work here)*/
_PX_GET_UNDEF_SIGNED_ALT_FN(gid_t, gid, PAX_STD_GID_KEYWORD)

#undef _PX_SET_UNDEF_SIGNED_ALT_FN

int
ph_get_size(
	pax_hdr_t *hdr,
	offset_t *size)
{
	pax_ustar_hdr_t *uh = NULL;

	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	uh = &hdr->hdr_blk.hdr;

	return (_ph_get_unsigned(hdr, (uint64_t *)size, uh->size,
	    sizeof (uh->size), PAX_STD_SIZE_KEYWORD));
}

int
ph_get_mtime(
	pax_hdr_t *hdr,
	pax_time_t *mtime)
{
	pax_ustar_hdr_t *uh = NULL;

	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	uh = &hdr->hdr_blk.hdr;

	return (_ph_get_time(hdr, mtime, uh->mtime, sizeof (uh->size),
	    PAX_STD_MTIME_KEYWORD));
}

int
ph_get_atime(
	pax_hdr_t *hdr,
	pax_time_t *atime)
{
	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	return (_ph_get_time(hdr, atime, NULL, 0, PAX_STD_ATIME_KEYWORD));
}

int
ph_get_type(
	pax_hdr_t *hdr,
	char *type)
{
	pax_ustar_hdr_t *uh = NULL;

	if (!hdr) {
		return (PX_ERROR_INVALID);
	}

	uh = &hdr->hdr_blk.hdr;

	*type = uh->type;
	return (PX_SUCCESS);
}

/*
 * The following set functions only get the value from the ustar header block
 * because there are no standard keywords for them in the extended header.
 */

#define	_PX_GET_UNDEF_SIGNED_NO_ALT_FN(_type_, _field_) \
int ph_get_##_field_(pax_hdr_t *hdr, _type_ *_field_) \
{ \
	int status = PX_SUCCESS; \
	int64_t i64val; \
	uint64_t u64val; \
	 \
	pax_ustar_hdr_t *uh = NULL; \
	 \
	if (!hdr) { \
		return (PX_ERROR_INVALID); \
	} \
	 \
	uh = &hdr->hdr_blk.hdr; \
	if ((_type_) -1 > 0) { \
		status = _ph_get_unsigned(hdr, &u64val, \
		    uh->_field_, sizeof (uh->_field_), NULL); \
		*_field_ = (_type_) u64val; \
	} else { \
		status = _ph_get_signed(hdr, &i64val, \
		    uh->_field_, sizeof (uh->_field_), NULL); \
		*_field_ = (_type_) i64val; \
	} \
	return (status); \
}

/*LINTED CONSTCOND in macro (CONSTCOND doesn't work here)*/
_PX_GET_UNDEF_SIGNED_NO_ALT_FN(mode_t, mode)

/*LINTED CONSTCOND in macro (CONSTCOND doesn't work here)*/
_PX_GET_UNDEF_SIGNED_NO_ALT_FN(dev_t, devmajor)

/*LINTED CONSTCOND in macro (CONSTCOND doesn't work here)*/
_PX_GET_UNDEF_SIGNED_NO_ALT_FN(dev_t, devminor)

#undef _PX_GET_UNDEF_SIGNED_NO_ALT_FN

/* ******************* static function definitions ******************* */

static int
_ph_set_string(
	pax_hdr_t *hdr,
	char *dst,
	int dlen,
	char *src,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t *pair = NULL;
	pax_pair_t *old_pairs = NULL;
	int count;

	memset(dst, 0, dlen);
	old_pairs = pxh_remove_pair(&hdr->xhdr_list, xhdr_key, &count);
	(void) pxp_destroy_pairs(old_pairs);

	if (!dst || strlen(src) > dlen) {
		if (!xhdr_key) {
			ABORT_ST(PX_ERROR_WONT_FIT);
		}

		pair = pxp_mkpair_string(xhdr_key, src);
		status = pxh_put_pair(&hdr->xhdr_list, pair, 0);
		TEST_AND_ABORT()
	} else {
		strlcpy(dst, src, dlen);
	}
out:
	return (status);
}

static int
_ph_set_unsigned(
	pax_hdr_t *hdr,
	char *dst,
	size_t dlen,
	uint64_t src,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t *pair = NULL;
	pax_pair_t *old_pairs = NULL;
	int count;

	if (hdr) {
		old_pairs = pxh_remove_pair(&hdr->xhdr_list, xhdr_key, &count);
		(void) pxp_destroy_pairs(old_pairs);
	}

	if (!dst || snprintf(dst, dlen, "%.*llo", dlen - 1, src) >= dlen) {
		/* zero out the buffer, whatever fit went into it */
		memset(dst, 0, dlen);

		if (!xhdr_key || !hdr) {
			ABORT_ST(PX_ERROR_WONT_FIT);
		}

		pair = pxp_mkpair_u64(xhdr_key, src);
		status = pxh_put_pair(&hdr->xhdr_list, pair, 0);
		TEST_AND_ABORT()
	}
out:
	return (status);
}

static int
_ph_set_signed(
	pax_hdr_t *hdr,
	char *dst,
	size_t dlen,
	int64_t src,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t *pair = NULL;
	pax_pair_t *old_pairs = NULL;
	int count;

	old_pairs = pxh_remove_pair(&hdr->xhdr_list, xhdr_key, &count);
	(void) pxp_destroy_pairs(old_pairs);

	if (!dst || src < 0 ||
	    snprintf(dst, dlen, "%.*llo", dlen - 1, src) >= dlen) {
		/* zero out the buffer, whatever fit went into it */
		memset(dst, 0, dlen);

		if (!xhdr_key) {
			ABORT_ST(PX_ERROR_WONT_FIT);
		}

		pair = pxp_mkpair_i64(xhdr_key, src);
		status = pxh_put_pair(&hdr->xhdr_list, pair, 0);
		TEST_AND_ABORT()
	}
out:
	return (status);
}

static int
_ph_set_time(
	pax_hdr_t *hdr,
	char *dst,
	size_t dlen,
	pax_time_t src,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t *pair = NULL;
	pax_pair_t *old_pairs = NULL;
	int count;

	old_pairs = pxh_remove_pair(&hdr->xhdr_list, xhdr_key, &count);
	(void) pxp_destroy_pairs(old_pairs);

	/*
	 * make an extended data entry for the time if any of the following
	 * 1) no buffer was supplied
	 * 2) non-zero nanoseconds
	 * 3) it didn't fit into the buffer
	 */
	if (!dst || src.nsec != 0 ||
	    snprintf(dst, dlen, "%.*llo", dlen - 1, src.sec) >= dlen) {
		/* zero out the buffer, whatever fit went into it */
		memset(dst, 0, dlen);

		if (!xhdr_key) {
			ABORT_ST(PX_ERROR_WONT_FIT);
		}

		pair = pxp_mkpair_time(xhdr_key, src);
		status = pxh_put_pair(&hdr->xhdr_list, pair, 0);
		TEST_AND_ABORT()
	}
out:
	return (status);
}

/*
 * What to do if a value in an xheader pair can't be converted to the
 * appropriate type is subject to interpretation.  The standard says that
 * values in the extended header data override values in the ustar block.  Of
 * course if the value can't be converted in a meaningful way, this leaves us
 * in a bind.
 *
 * The solution implemented here is to return an error, and let the caller deal
 * with the problem appropriately.  Odds are if we can't figure it out, it's
 * probably better to let the caller do what it wants rather than try to guess
 * what it intends.
 */

static int
_ph_get_string(
	pax_hdr_t *hdr,
	const char **result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t **pair = NULL;
	const char *value = NULL;

	if (!result) {
		return (PX_ERROR_INVALID);
	}

	*result = NULL;

	if (xhdr_key) {
		if (pair = pxh_get_pair(hdr->xhdr_list, xhdr_key, NULL)) {
			status = pxp_get_string(pair[0], &value);
			SamFree(pair);
			ABORT();
		}
	}

	if (block_field) {
		if (strlen(block_field) >= block_field_len) {
			ABORT_ST(PX_ERROR_MALFORMED);
		}
		if (strlen(block_field) == 0) {
			value = NULL;
		} else {
			value = (const char *)block_field;
		}
	}
out:
	if (PXSUCCESS(status)) {
		*result = value;
	}
	return (status);
}

static int
_ph_get_unsigned(
	pax_hdr_t *hdr,
	uint64_t *result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t **pair = NULL;
	uint64_t value = 0;
	char *end;

	if (!result) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if (xhdr_key) {
		if (pair = pxh_get_pair(hdr->xhdr_list, xhdr_key, NULL)) {
			status = pxp_get_u64(pair[0], &value);
			SamFree(pair);
			ABORT();
		}
	}
	if (block_field) {
		end = block_field + block_field_len;
		status = parse_u64(block_field, NULL, end, &value,
		    USTAR_NUMBER_BASE, ALLOW_LEADING_WS | ALLOW_TRAILING_CHARS);
	}
out:
	*result = value;
	return (status);
}

static int
_ph_get_signed(
	pax_hdr_t *hdr,
	int64_t *result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t **pair = NULL;
	int64_t value = 0;
	char *end;

	if (!result) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if (xhdr_key) {
		if (pair = pxh_get_pair(hdr->xhdr_list, xhdr_key, NULL)) {
			status = pxp_get_i64(pair[0], &value);
			SamFree(pair);
			ABORT();
		}
	}
	if (block_field) {
		end = block_field + block_field_len;
		status = parse_i64(block_field, NULL, end, &value,
		    USTAR_NUMBER_BASE, ALLOW_LEADING_WS | ALLOW_TRAILING_CHARS);
	}
out:
	*result = value;
	return (status);
}

static int
_ph_get_time(
	pax_hdr_t *hdr,
	pax_time_t *result,
	char *block_field,
	size_t block_field_len,
	char *xhdr_key)
{
	int status = PX_SUCCESS;
	pax_pair_t **pair = NULL;
	pax_time_t value = {0, 0};
	char *end;

	if (!result) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	if (xhdr_key) {
		if (pair = pxh_get_pair(hdr->xhdr_list, xhdr_key, NULL)) {
			status = pxp_get_time(pair[0], &value);
			SamFree(pair);
			ABORT();
		}
	}
	if (block_field) {
		end = block_field + block_field_len;
		status = parse_i64(block_field, NULL, end, &value.sec,
		    USTAR_NUMBER_BASE, ALLOW_LEADING_WS | ALLOW_TRAILING_CHARS);
	}
out:
	*result = value;
	return (status);
}

#define	MERGE_FIELD(_field_) \
	if (strlen(uhdr->_field_)) { \
		memcpy(&ublk->_field_, &uhdr->_field_, \
		    sizeof (ublk->_field_)); \
	}


static int
write_merged_ustar_blk(
	pax_hdr_blk_t *blk,
	pax_hdr_t *hdr,
	pax_hdr_t *defaults,
	int xhdr_blk,
	pax_filename_callback_t filename_callback)
{
	int status = PX_SUCCESS;
	pax_ustar_hdr_t *ublk = &blk->hdr;
	pax_ustar_hdr_t *uhdr = &hdr->hdr_blk.hdr;
	pax_ustar_hdr_t *udefaults = &defaults->hdr_blk.hdr;
	int has_ext_hdr;
	const char *actual_name;

	memset(blk, 0, sizeof (*blk));
	memcpy(ublk, udefaults, sizeof (*ublk));

	MERGE_FIELD(mode);
	MERGE_FIELD(uid);
	MERGE_FIELD(gid);
	MERGE_FIELD(size);
	MERGE_FIELD(mtime);
	MERGE_FIELD(magic.magic);

	/* version is special because it's not NULL terminated */
	if (uhdr->magic.version[0] != '\0' && uhdr->magic.version[1] != '\0') {
		memcpy(&ublk->magic.version, &uhdr->magic.version,
		    sizeof (ublk->magic.version));
	}

	MERGE_FIELD(uname);
	MERGE_FIELD(gname);
	MERGE_FIELD(devmajor);
	MERGE_FIELD(devminor);

	if (!xhdr_blk) {
		ublk->type = uhdr->type;
	} else {
		ublk->type = 'x';
	}

	if ((has_ext_hdr = ph_has_ext_hdr(hdr))) {
		if (has_ext_hdr < 0) {
			ABORT_ST(-has_ext_hdr);
		}

		status = ph_get_name(hdr, &actual_name);
		TEST_AND_ABORT();

		if (xhdr_blk) {
			status = filename_callback(
			    ublk->name, sizeof (ublk->name),
			    ublk->prefix, sizeof (ublk->prefix),
			    actual_name, 1, PAX_XHDR_NAME);
			TEST_AND_ABORT();
		} else {
			status = filename_callback(
			    ublk->name, sizeof (ublk->name),
			    ublk->prefix, sizeof (ublk->prefix),
			    actual_name, 0, PAX_FILE_NAME);
			TEST_AND_ABORT();
		}
	} else {
		/*
		 * shouldn't be set if we're not writing a block for a header
		 * that doesn't have any extended header data.
		 */
		if (xhdr_blk) {
			ABORT_ST(PX_ERROR_INVALID);
		}

		MERGE_FIELD(name);
		MERGE_FIELD(linkname);
	}
out:
	return (status);
}

#undef	MERGE_FIELD

static int
checksum(
	pax_hdr_blk_t *blk)
{
	int status = PX_SUCCESS;
	int i;
	uint64_t chksum = 0;
	pax_ustar_hdr_t *hdr;

	if (!blk) {
		ABORT_ST(PX_ERROR_INVALID);
	}

	hdr = &blk->hdr;

	memset(hdr->chksum, ' ', sizeof (hdr->chksum));

	for (i = 0; i < sizeof (*hdr); ++i) {
		chksum += blk->blk[i];
	}

	status = _ph_set_unsigned(NULL, hdr->chksum, sizeof (hdr->chksum),
	    chksum, NULL);
	TEST_AND_ABORT();
out:
	return (status);
}
