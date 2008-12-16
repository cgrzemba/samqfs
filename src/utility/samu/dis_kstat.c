/*
 * dis_kstat.c - Display kernel statistics.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.14 $"


/* ANSI headers. */
#include <ctype.h>

/* Solaris headers. */
#include <sys/ipc.h>
#include <kstat.h>
#include <sys/kstat.h>
#include <string.h>

/* SAM-FS headers. */
#include "sam/custmsg.h"
#include "sam/nl_samfs.h"
#include "sam/osversion.h"

/* Local headers. */
#include "samu.h"

static void DisKstatInfo(kstat_t *ksp);
static int KsIndex;

void
DisKstat(void)
{
	int i;
	kstat_ctl_t	*kc;
	kstat_t		*ksp;

	kc = kstat_open();
	if (kc == NULL) {
		Mvprintw(ln++, 0, catgets(catfd, SET, 7383,
		    "kernel statistics not available"));
		return;
	}
	for (ksp = kc->kc_chain, i = 0; ksp != NULL; ksp = ksp->ks_next) {
		if (strcmp(ksp->ks_module, "sam-qfs") == 0) {
			if (i == KsIndex || ScreenMode == FALSE) {
				kstat_read(kc, ksp, NULL);
				DisKstatInfo(ksp);
				if (ksp->ks_next && ScreenMode == TRUE) {
					Mvprintw(ln, 0, catgets(catfd, SET,
					    7003, "     more (ctrl-f)"));
				}
			}
			i++;
		}
	}
	kstat_close(kc);
}

static void
DisKstatInfo(kstat_t *ksp)
{
	int i;
	int col;
	kstat_named_t *ksnp;
#define	KS_STRMAX  50
	char kstring[KS_STRMAX + 1];

	Mvprintw(ln, 0, catgets(catfd, SET, 7384,
	    "module: %s  name: %s instance: %d class: %s"),
	    ksp->ks_module, ksp->ks_name, ksp->ks_instance, ksp->ks_class);
	if (ksp->ks_type == KSTAT_TYPE_NAMED) {
		ksnp = (kstat_named_t *)ksp->ks_data;
		col = 0;
		for (i = 0; i < ksp->ks_ndata; i++) {
			ln++;
			Mvprintw(ln, col, "%s", ksnp->name);
			switch (ksnp->data_type) {
			case KSTAT_DATA_CHAR:
				kstring[16] = '\0';
				strncpy(kstring, ksnp->value.c, 16);
				Mvprintw(ln, col+30, "%s", kstring);
				break;
			case KSTAT_DATA_INT32:
				Mvprintw(ln, col+30, "%d", ksnp->value.i32);
				break;
			case KSTAT_DATA_UINT32:
				Mvprintw(ln, col+30, "%u", ksnp->value.ui32);
				break;
			case KSTAT_DATA_INT64:
				Mvprintw(ln, col+30, "%lld", ksnp->value.i64);
				break;
			case KSTAT_DATA_UINT64:
				Mvprintw(ln, col+30, "%llu", ksnp->value.ui64);
				break;
			case KSTAT_DATA_STRING:
				kstring[KS_STRMAX] = '\0';
				strncpy(kstring, KSTAT_NAMED_STR_PTR(ksnp),
				    KS_STRMAX);
				Mvprintw(ln, col+30, "%s", kstring);
				break;
			default:
				Mvprintw(ln, col+30, catgets(catfd, SET, 7385,
				    "Unknown kstat data type %d"),
				    ksnp->data_type);
				break;
			}
			ksnp++;
		}
	} else {
		Mvprintw(ln++, 0, catgets(catfd, SET, 7386,
		    "Unknown kstat type %d"), ksp->ks_type);
	}
	ln += 2;
}

boolean
InitKstat(void)
{
	KsIndex = 0;
	return (TRUE);
}

boolean
KeyKstat(char key)
{
	switch (key) {

	case KEY_full_fwd:
		KsIndex++;
		break;
	case KEY_full_bwd:
		if (KsIndex > 0) KsIndex--;
		break;
	default:
		return (FALSE);
		/* NOTREACHED */
		break;
	}
	return (TRUE);
}
