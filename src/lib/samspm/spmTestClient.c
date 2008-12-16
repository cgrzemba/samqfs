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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sam/spm.h>


#if !defined(lint)
int
main(int argc, char **argv)
{
	int error;
	char error_buffer[SPM_ERRSTR_MAX];
	char *host;
	char *service;

	if (argc != 3) {
		fprintf(stderr, "USAGE: spmClient host service\n");
		exit(1);
	}
	host = argv[1];
	service = argv[2];

	if ((spm_connect_to_service(service, host, NULL, &error,
	    error_buffer)) == -1) {
		fprintf(stderr, "spm_connect_to_service failed: %d: %s\n",
		    error, error_buffer);
		exit(1);
	}
	printf("connection succeeded!\n");

	return (0);
}
#endif
