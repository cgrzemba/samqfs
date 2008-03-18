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
#pragma ident   "$Revision: 1.5 $"

#ifndef __SCRKD_CURL_H__
#define	__SCRKD_CURL_H__

#pragma ident	"@(#)scrkd_curl.h	1.1	06/09/25 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include "csn/scrkd.h"
#define	HTTP_CLIENT_ERR		-1
#define	CA_FILE			CNS_DATA_DIR"/cacert.pem"
#define	CNS_FILE		CNS_DATA_DIR"/cnscert.pem"

#define	USER_PASS_LEN	100
#define	HOST_PORT_LEN	264 /* MAXHOSTNAME + : + PORT */


/*
 * Send a request to the identified url.
 * Returns the response if the post was successful or NULL if anything
 * failed. If NULL is returned and err_code != NULL err_str points
 * to an error message.
 */
char *
curl_send_request(int *err_code, char **err_str, void *post_data,
		    const char *url, const char *content_type,
		    const proxy_cfg_t *opts);

#ifdef	__cplusplus
}
#endif

#endif /* __SCRKD_CURL_H__ */
