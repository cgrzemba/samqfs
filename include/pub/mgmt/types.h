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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef _FSMTYPES_H_
#define	_FSMTYPES_H_

#pragma ident   "$Revision: 1.16 $"

/*
 * types.h is a collection of common macros and type definitions.
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "sam/types.h"

/*
 * values to use to reset default values.
 */
#define	int_reset		-1
#define	long_reset		-1L
#define	float_reset		-1.0f
#define	double_reset		-1.0
#define	enum_reset		-1
#define	flag_reset		B_FALSE
#define	char_array_reset	'\0'
#define	fsize_reset		0xffffffffffffffff
#define	long_long_reset		-1L
#define	uint16_reset		(uint16_t)0xffff
#define	int16_reset		(int16_t)-1
#define	uint_reset		((unsigned int) ~(unsigned int) 0)


#define	MAX_PATH_LENGTH		128	/* path name max len */
#define	MAX_NAME_LENGTH		32	/* device/fs name max len */
#define	MAX_MTYPE_LENGTH	3	/* media type max len */
#define	MAX_DEVTYPE_LENGTH	5	/* device type max length */
#define	MAX_VSN_LENGTH		32	/* volume serial number max len */
#define	MAX_EQU			65534	/* Maximum value for equip ordinal */
#define	MAX_VSN_INFO_LENGTH	127	/* maximum length of vsn info */
#define	MAX_COPY		4	/* maximum copy id */
#define	MAX_OPRMSG_SIZE		80	/* size of operator message buffer */
#define	NODEV_STR		"nodev"
#define	GLOBAL			"global properties"

#define	MAX_VENDORID_LENGTH	8	/* vendor id length */
#define	MAX_PRODUCTID_LENGTH	16	/* product id length */
#define	MAX_REVISION_LENGTH	4	/* revision length */

#ifndef _NETDB_H
#define	MAXHOSTNAMELEN	256
#endif /* _NETDB_H */

typedef char devtype_t[MAX_DEVTYPE_LENGTH]; /* device type */
typedef char umsg_t[MAX_OPRMSG_SIZE]; /* operator message */


/*
 * this structure is only used for the RPC clients of the API
 */
typedef struct samrpc_client {
#ifdef _SAMMGMT_H_RPCGEN
	CLIENT	*clnt;
#else
	void*	clnt;
#endif /* _SAMMGMT_H_RPCGEN */
	char *svr_name;	/* 4.4 used to save server architecture */
	int timestamp;
} samrpc_client_t;

/*
 * all API functions take this as the first argument (may be NULL).
 */
typedef struct ctx {
	upath_t dump_path;	/* dump configuration to this directory. */
				/* used only by 'write'-type functions */

	upath_t read_location;	/* location from which to read the config */

	samrpc_client_t
	    *handle;		/* used by RPC clients to specify which */
				/* connection should be used. */
				/* see sammgmt_rpc.h for details */

	uname_t user_id;	/* User id to be used for logging purposes */
} ctx_t;

/*
 * convert from a string to an fsize.  The string should be of the form
 * 10G or 10k or ...
 */
int str_to_fsize(char *size, fsize_t *fsize);

/*
 * convert from an fsize to a string.
 * The buffer passed in should be FSIZE_STR_LEN characters long.
 */
#define	FSIZE_STR_LEN 24
char *fsize_to_str(fsize_t f, char *buf, int bufsz);

/*
 * match_fsize()
 *
 * A more flexible version of fsize_to_string().  Allows the caller
 * to get an inexact string returned (i.e., cannot be reverse-matched
 * back to the original fsize).  Useful for printing reports and/or
 * error messages.  Precision is also variable.
 *
 */
char *match_fsize(fsize_t v, int prec, boolean_t exact, char *buf,
		size_t buflen);

/*  interval_to_str() and str_to_interval() are only used by forms.c */
/*
 * convert a uint_t which represents a time in seconds to a string
 * with a unit.
 */
char *interval_to_str(uint_t interval);

/*
 * convert from a string representing a time to the number of seconds the
 * string represents.
 */
int str_to_interval(char *str, uint_t *interval);


#ifdef __cplusplus
}
#endif


#endif	/* _FSMTYPES_H_ */
