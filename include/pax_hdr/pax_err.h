/*
 *	pax_err.h - error definitions for pax library
 *
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


#ifndef	_PAX_ERR_H
#define	_PAX_ERR_H

/*
 * This is meant to provide a way to enable or disable run-time checking of
 * operations that can cause crashes or hangs.  This is intentionally NOT
 * tied to any other debug related #define, so that it can easily be left on
 * in production code.  Generally speaking, the more thorough checks come with
 * a cost of more than O(1), and it is up to the developer to decide whether or
 * not this is needed.  It also usually tests things that you absolutely
 * shouldn't ever do.  If you end up doing something that causes a problem
 * it's probably because there's a bug that needs to be worked out anyway.
 */
#define	PX_THOROUGH_CHECKS

/* successful operation */
#define	PX_SUCCESS 0
#define	PX_SUCCESS_NO_MORE_PAIRS 1
#define	PX_SUCCESS_STD_HEADER 2
#define	PX_SUCCESS_EXT_HEADER 3
#define	PX_SUCCESS_GLOBAL_HEADER 4

/* errors are defined to be less than zero */

/* generic errors */
#define	PX_ERROR_GENERIC (-100)
#define	PX_ERROR_INVALID (-101)
#define	PX_ERROR_PARSED (-102)
#define	PX_ERROR_NOT_PARSED (-103)
#define	PX_ERROR_INTERNAL (-104)
#define	PX_ERROR_NOT_PAX_HEADER (-105)
#define	PX_ERROR_INVALID_HEADER (-106)
#define	PX_ERROR_NOT_SUPPORTED (-107)
#define	PX_ERROR_REQUIRE_DEFAULTS (-108)
#define	PX_ERROR_DEFAULTS_XHDR (-109)
#define	PX_ERROR_INVALID_BUFFER (-110)

/* pax structural errors */
#define	PX_ERROR_NO_KEY (-200)
#define	PX_ERROR_MALFORMED (-201)
#define	PX_ERROR_DUPLICATE_INSERTION (-202)

/* type mismatch errors */
#define	PX_ERROR_BAD_TYPE (-210)
#define	PX_ERROR_I64_TYPE (-211)
#define	PX_ERROR_U64_TYPE (-212)
#define	PX_ERROR_TIME_TYPE (-213)
#define	PX_ERROR_STRING_TYPE (-214)

/* numeric errors */
#define	PX_ERROR_DOMAIN (-300)
#define	PX_ERROR_RANGE (-301)
#define	PX_ERROR_SIGN (-302)
#define	PX_ERROR_FPART_TOO_PRECISE (-303)
#define	PX_ERROR_DIV_ZERO (-304)
#define	PX_ERROR_IMPROPER_FRACTION (-305)

/* string errors */
#define	PX_ERROR_OVERFLOW (-400)
#define	PX_ERROR_LEADING_CHARS (-401)
#define	PX_ERROR_TRAILING_CHARS (-402)
#define	PX_ERROR_CANT_PARSE_TYPE (-403)
#define	PX_ERROR_END_OF_HEADER (-404)

/* set/get errors */
#define	PX_ERROR_WONT_FIT (-500)

#define	PXSUCCESS(_status_) (_status_ >= 0)
#endif /* _PAX_ERR_H */
