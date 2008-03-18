/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.11 $"


#if !defined(_API_ERRORS_H)
#define	_API_ERRORS_H

typedef enum {
	API_ERR_OK = 0,
	API_ERR_TR,		/* Toss request with error */
	API_ERR_DM,		/* Toss request, 'down' media */
	API_ERR_DD,		/* Toss request, down drive */
	API_ERR_DL,		/* Toss request, down library */
	API_ERR_LIST_END	/* Invalid entry - used to terminate lists */
}		api_errs_t;

typedef struct {
	api_errs_t	degree;	/* What to return if retries fail */
	int		retry;	/* retry count (<0 = forever) */
	int		sleep;	/* how long to sleep between retries */
	int		code;	/* the code (should be the index */
	char	   *mess;	/* text message */
}		api_messages_t;

api_errs_t	api_return_degree(dtype_t type, int errorcode);
uint_t	   api_return_retry(dtype_t type, int errorcode);
char	   *api_return_message(dtype_t type, int errorcode);
uint_t	   api_return_sleep(dtype_t type, int errorcode);
int		api_valid_error(dtype_t type, int errorcode, dev_ent_t *un);

#endif				/* _API_ERRORS_H */
