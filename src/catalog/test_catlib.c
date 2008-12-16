/*
 * test_catlib.c - unit test for catlib.
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

#pragma ident "$Revision: 1.18 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "aml/trace.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "catlib_priv.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */


/*
 * Test functions.
 */
int
main(
	int argc,
	char **argv)
{
	struct CatalogEntry *ce;
	struct CatalogEntry ced;
	struct VolId vid;
	char	buf[256];
	char	*p;
	int	n;
	int	nc;
	int	status;

	status = CatalogInit("test");
	printf("CatalogInit() status = %d\n", status);
	if (status == -1) {
		LibFatal(CatalogInit, "");
	}

	if (argc < 2) {
		/*
		 * List equipment and catalogs.
		 */
		for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
			struct CatalogHdr *ch;
			int		ne;

			ch = Catalogs[nc].CmHdr;
			ch = CatalogGetHeader(ch->ChEq);
			if (ch == NULL) {
				printf("CatalogGetHeader() failed.\n");
				continue;
			}
			printf(" %d  %d %d %d  %s\n",
			    nc, ch->ChEq, ch->ChType,
			    ch->ChNumofEntries, ch->ChFname);
			for (ne = 0; ne < ch->ChNumofEntries; ne++) {
				struct CatalogEntry *ce;

				ce = &ch->ChTable[ne];
				printf("%s\n", CatalogStrFromEntry(ce,
				    buf, sizeof (buf)));
			}
		}
		return (EXIT_SUCCESS);
	}
	if (StrToVolId(argv[2], &vid) == -1) {
		if (*argv[1] != 's') {
			fprintf(stderr, "Vol spec error - %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
			exit(EXIT_FAILURE);
		}
	}
	printf(" %s\n", StrFromVolId(&vid, buf, sizeof (buf)));
	if ((ce = CatalogGetEntry(&vid, &ced)) == NULL) {
		if (*argv[1] != 's') {
			fprintf(stderr, "Vol spec error - %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
			exit(EXIT_FAILURE);
		}
	}

	if (ce != NULL) {
		printf(" %d %s\n",
		    ce->CeMid, CatalogStrFromEntry(ce, buf, sizeof (buf)));
	}

	switch (*argv[1]) {
	case 'n':
		break;

	case 's': {
		/*
		 * SlotInit
		 */
		char	*barcode = "";

		if (argc > 3)  barcode = argv[3];
		status = CatalogSlotInit(&vid, CES_inuse, 3, barcode, "");
		if (status == -1) {
			printf(" SlotInit failed: %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		}
	}
	break;

	case 'l': {
		/*
		 * l - VolumeLoaded
		 */
		struct CatalogEntry cea;

		memmove(&cea, ce, sizeof (cea));
		if (argc <= 3)  exit(EXIT_USAGE);
		if (StrToVolId(argv[3], &vid) == -1) {
			fprintf(stderr, "Vol spec error - %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
			exit(EXIT_FAILURE);
		}
		printf(" %s\n", StrFromVolId(&vid, buf, sizeof (buf)));
		if ((ce = CatalogGetEntry(&vid, &ced)) == NULL) {
			fprintf(stderr, "Vol spec error - %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		} else if (ce->CeStatus & CES_inuse) {
			fprintf(stderr, "%s in use.\n",
			    StrFromVolId(&vid, buf, sizeof (buf)));
			exit(EXIT_FAILURE);
		}

		cea.CeEq = vid.ViEq;
		cea.CeSlot = vid.ViSlot;
		cea.CePart = 0;
		status = CatalogVolumeLoaded(&cea);
		if (status == -1) {
			printf(" VolumeLoaded failed: %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		}
	}
	break;

	case 'm': {
		/*
		 * m - MoveSlot
		 */
		int		DestSlot;

		if (argc <= 3)  exit(EXIT_USAGE);
		DestSlot = strtol(argv[3], &p, 0);
		status = CatalogMoveSlot(&vid, DestSlot);
		if (status == -1) {
			printf(" VolumeMoveSlot failed: %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		}
	}
	break;

	case 'r':
		status = CatalogReserveVolume(&vid, 0, "as", "ow", "fs");
		if (status == -1) {
			printf(" Reserve volume failed: %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		}
		break;

#if (0)
	case 'u':
		status = CatalogUnReserveVolume(&vid);
		if (status == -1) {
			printf(" UnReserve volume failed: %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		}
		break;
#endif


	case 'x':
		memset(&vid, 0, sizeof (struct VolId));
		vid.ViEq = ce->CeEq;
		vid.ViSlot = ce->CeSlot;
		vid.ViPart = ce->CePart;
		vid.ViFlags = VI_cart;
		status = CatalogExport(&vid);
		if (status == -1) {
			printf(" Export failed: %s\n",
			    StrFromErrno(errno, buf, sizeof (buf)));
		}
		break;

	default:
		printf("??\n");
	}
	CatalogTerm();
	return (EXIT_SUCCESS);
}
