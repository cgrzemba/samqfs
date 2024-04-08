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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sam/spm.h>

#if defined(lint)
#include <sam/lint.h>
#endif

int
main(int argc, char **argv)
{
	int error;
	char error_buffer[SPM_ERRSTR_MAX];
	int i;
	char *server;
	struct spm_query_info *entry;
	struct spm_query_info *result;

	if (argc != 2) {
		fprintf(stderr, "USAGE: spmQuery host\n");
		exit(1);
	}
	server = argv[1];
	if (spm_query_services(server, &result, &error, error_buffer)) {
		fprintf(stderr, "spm_query_services failed: %d: %s\n",
		    error, error_buffer);
		exit(1);
	}

	printf("Registered services for host '%s':\n", server);
	for (i = 0, entry = result; entry; entry = entry->sqi_next, i++) {
		printf("    %s\n", entry->sqi_service);
	}
	printf("  %d service(s) registered.\n", i);
	spm_free_query_info(result);

	return (0);
}
