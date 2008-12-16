/*
 * ----- utility/samstorade.c - StorADE shared memory service program.
 *
 * Send and receive SAM-FS XML messages.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sam/lib.h>
#include <sam/nl_samfs.h>
#include <sam/custmsg.h>
#include "samfm.h"

#define	FIVE_SEC_TIMEOUT 5000
#define	DEVENT "<message id=\"devent\" version=\"1.0\"/>"

/*
 * ----- Query SAM-FS for device and health attributes.
 */
int /* Non-zero if error, 0 if successful. */
main(int argc, char **argv)
{
	char *host = "localhost";
	int timeout = FIVE_SEC_TIMEOUT;
	char *send = DEVENT;
	char *recv = NULL;
	int c, rtn, custom = 0;
	char *p1, *p2;

	CustmsgInit(0, NULL);

	while ((c = getopt(argc, argv, "hr:t:s:d")) != EOF) {
		switch (c) {
		case 'r':
			host = optarg;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 's':
			send = optarg;
			custom = 1;
			break;
		case 'd':
			send = DEVENT;
			break;
		case 'h':
		default:
			fprintf(stderr, "%s samstorade [-r hostname]"
			    " [-t timeout] [-s xml_message] [-d]\n",
			    catgets(catfd, SET, 4601, "usage:")),
			    exit(2);
		}
	}

	if ((rtn = samfm(host, timeout, send, &recv)) != 0) {
		return (rtn);
	}

	if (custom) {
		printf("%s", recv);
		free(recv);
		return (0);
	}

	p1 = strstr(recv, "<samfm");
	p2 = strstr(recv, "</samfm>");
	if (p1 && p2) {
		p2[8] = '\0';
		printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
		printf("<!DOCTYPE samfm SYSTEM \""
		    "/opt/SUNWsamfs/doc/samfm.dtd\">\n\n");
		printf("%s\n", p1);
		free(recv);
		return (0);
	}

	free(recv);
	return (1);
}
