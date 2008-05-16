/*
 * dis_spm.c - Display single port multiplexer registered services.
 *
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

#pragma ident "$Revision: 1.10 $"


/* ANSI headers. */
#include <ctype.h>

/* Solaris headers. */
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>

/* SAM-FS headers. */
#include "sam/custmsg.h"
#include "sam/nl_samfs.h"
#include "sam/spm.h"

/* Local headers. */
#include "samu.h"

static int ServIndex;

void
DisServ(void)
{
	int error;
	char error_buffer[SPM_ERRSTR_MAX];
	int i;
	int ismore;
	char *server;
	struct spm_query_info *entry;
	struct spm_query_info *result;


	server = &hostname[0];
	if (spm_query_services(server, &result, &error, error_buffer)) {
		Mvprintw(ln++, 0, catgets(catfd, SET, 7388,
		    "spm_query_services failed: %d: %s"), error, error_buffer);
		return;
	}
	Mvprintw(ln++, 0, catgets(catfd, SET, 7389,
	    "Registered services for host '%s':"), server);
	ismore = 0;
	for (i = 0, entry = result; entry != NULL;
	    entry = entry->sqi_next, i++) {
		if (i >= ServIndex && ln <= LINES - 3) {
			Mvprintw(ln++, 0, "    %s", entry->sqi_service);
		}
		if (entry->sqi_next && ln > LINES - 3) {
			ismore = 1;
		}
	}
	if (ismore == 1) {
		Mvprintw(ln++, 0, catgets(catfd, SET, 7003,
		    "     more (ctrl-f)"));
	}
	Mvprintw(ln++, 0, catgets(catfd, SET, 7390,
	    "  %d service(s) registered."), i);
	spm_free_query_info(result);
}


boolean
InitServ(void)
{
	ServIndex = 0;
	return (TRUE);
}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)

boolean
KeyServ(char key)
{
	switch (key) {

	case KEY_full_fwd:
		ServIndex += TLINES;
		break;
	case KEY_full_bwd:
		if (ServIndex > 0) ServIndex -= TLINES;
		break;
	default:
		return (FALSE);
		/* NOTREACHED */
		break;
	}
	return (TRUE);
}
