/*
 * samgetvol.c - dump fs ("volume") information
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


#pragma ident "$Revision: 1.16 $"

#include <stdio.h>
#include <stdlib.h>
#include "sys/types.h"
#include "samsanergy/fsmdc.h"
#include "samsanergy/fsmdcsam.h"

/* #define	SAN_DEBUG	1 */

static char *fstype[] = {
	"NT",
	"SPARC UFS",
	"OTHER",
	"SPARC SAM-FS"
};

static char *glomtype[] = {
	"SIMPLE",
	"SPAN",
	"NT_RAID0",
	"SAM_RAID0"
};

static char *flagbits[] = {
	"STRIPE_SUB",
	"META",
	"UNKNOWN FLAG [bit 2]",
	"UNKNOWN FLAG [bit 3]",
	"UNKNOWN FLAG [bit 4]",
	"UNKNOWN FLAG [bit 5]",
	"UNKNOWN FLAG [bit 6]",
	"UNKNOWN FLAG [bit 7]",
	"UNKNOWN FLAG [bit 8]",
	"UNKNOWN FLAG [bit 9]",
	"UNKNOWN FLAG [bit 10]",
	"UNKNOWN FLAG [bit 11]",
	"UNKNOWN FLAG [bit 12]",
	"UNKNOWN FLAG [bit 13]",
	"UNKNOWN FLAG [bit 14]",
	"UNKNOWN FLAG [bit 15]"
};

#ifdef SAN_DEBUG
static char *usage = "Usage:\n\tsamgetvol [-h] [-r] [-v] [-w] mntpoint\n";
#else
static char *usage = "Usage:\n\tsamgetvol [-h] [-r] [-w] mntpoint\n";
#endif

static char *
uni2str(FSUNI *up, char *buf)
{
	register char *r = buf;

	while ((*r++ = *up++) != 0)
		;

	return (buf);
}

static char *
mapretcode(int n, char *buf)
{
	switch (n) {
	case FS_E_SUCCESS:		/* Successful normal completion */
		return ("FS_E_SUCCESS (OK)");

	case FS_E_MISC_ERROR:		/* Vendor-specific error */
		return ("FS_E_MISC_ERROR (GENERIC ERROR)");

	case FS_E_PARTIAL:		/* Success, but partial information */
		return ("FS_E_PARTIAL (OK; BUT BUFFER TOO SMALL)");

	case FS_E_WRONG_FS:		/* Not owned by this FSMDC */
		return ("FS_E_WRONG_FS (NOT SAM-FS/QFS FILE)");

	case FS_E_BAD_PATH:		/* Illegal or bad path or share name */
		return ("FS_E_BAD_PATH (NO FILE)");

	case FS_E_BAD_COOKIE:		/* Bad volume cookie */
		return ("FS_E_BAD_COOKIE (BAD OR STALE COOKIE)");

	case FS_E_OUT_OF_MEMORY:	/* Out of memory */
		return ("FS_E_OUT_OF_MEMORY (CAN'T ALLOCATE MEMORY)");

	case FS_E_INVALID_MSG_LENGTH:	/* Bad message length */
		return ("FS_E_INVALID_MSG_LENGTH (?)");

	case FS_E_INVALID_ENDIAN:	/* Unrecognized "endian"ness? */
		return ("FS_E_INVALID_ENDIAN (?)");

	case FS_E_UNKNOWN_FUNCTION:	/* Unrecognized function? */
		return ("FS_UNKNOWN_FUNCTION (?)");

	case FS_E_UNKNOWN_SHARE:	/* ? */
		return ("FS_E_UNKNOWN_SHARE (?)");

	case FS_E_UNKNOWN_FILE:		/* stale or bad cookie */
		return ("FS_E_UNKNOWN_FILE (BAD OR STALE COOKIE #2)");

	case FS_E_UNKNOWN_STREAM:	/* unknown file stream (NT) */
		return ("FS_E_UNKNOWN_STREAM (UNKNOWN FILE STREAM -- "
		    "NT ERROR?)");

	case FS_E_NO_DIRECT:		/* valid file, but no direct access */
		return ("FS_E_NO_DIRECT (NO DIRECT ACCESS TO FILE "
		    "(SPECIAL? DIR?))");

	case FS_E_PRIVILEGE:		/* caller has insufficient privilege */
		return ("FS_E_PRIVILEGE (INSUFFICIENT PRIVILEGE)");

	case FS_E_BAD_FILE:		/* Bad file or volume cookie */
		return ("FS_E_BAD_FILE (BAD OR STALE COOKIE #3)");

	case FS_E_SUBMAP:		/* Success, but submap returned */
		return ("FS_E_SUBMAP (FILE NOT FULLY RESIDENT?)");

	case FS_E_IVPARAM:		/* Start value not aligned */
		return ("FS_E_IVPARAM (FILE START OFFSET NOT ALIGNED?)");

	case FS_E_NOT_LOCKED:		/* File wasn't locked to begin with */
		return ("FS_E_NOT_LOCKED (UNLOCK OF UNLOCKED FILE)");

	case FS_E_SHORT_ALLOC:		/* Success, but not fully allocated */
		return ("FS_E_SHORT_ALLOC (FILE ALLOCATION PARTIALLY "
		    "SUCCESSFUL)");

	case FS_E_LICENSE_ERR:		/* License problem */
		return ("FS_E_LICENSE_ERR (?)");

	case FS_E_LICENSE_DUP:		/* duplicated license */
		return ("FS_E_LICENSE_DUP (?)");

	case FS_E_LICENSE_INV:		/* invalid license */
		return ("FS_E_LICENSE_INV (?)");

	}
	sprintf(buf, "UNKNOWN ERROR (%#x)", n);
	return (buf);
}

int
main(int ac, char *av[])
{
	unsigned long long buf[16384];
	char msgbuf[256];
	FSULONG buflen;
	FSVOLINFO *svp = (FSVOLINFO *)buf;
	int c, i, j;
	FSLONG r;
	FSLONG lr = 0, lw = 0;
	extern int errno;
#ifdef SAN_DEBUG
	extern int sam_verbose;
#endif

	while ((c = getopt(ac, av, "hrvw")) != EOF) {
		switch (c) {
		case 'r':		/* get max read lease */
			lr = 1;
			break;
		case 'w':
			lw = 1;		/* get max write lease */
			break;
#ifdef SAN_DEBUG
		case 'v':
			sam_verbose = 1;
			break;
#endif
		case 'h':
		default:
			printf(usage);
			exit(c != 'h');
		}
	}
	if (optind+1 != ac) {
		printf(usage);
		exit(1);
	}
	buflen = sizeof (buf);
	r = AFS_GetVol(av[optind], svp, &buflen);
	if (r || svp->vendorStatus) {
		printf("AFS_GetVol('%s') = %#lx (%s), vendorStatus=%d/%s\n",
		    av[optind],
		    (long)r, mapretcode(r, msgbuf),
		    (int)svp->vendorStatus,
		    uni2str(svp->vError, msgbuf));
		exit(1);
	}

	printf("msgLen       = %8ld [%#lx]\n",
	    (long)svp->msgLen, (long)svp->msgLen);
	printf("vendorStatus = %8ld [%#lx]\n",
	    (long)svp->vendorStatus, (long)svp->vendorStatus);
	printf("VolCookie =\n");
	for (i = 0; i < sizeof (svp->cookie); i++) {
		if ((i%8) == 0)
			printf("\t");
		printf(" %2x", svp->cookie.xprivate[i]&0xff);
		if ((i%16) == 15)
			printf("\n");
	}
	printf("fsType       = %8ld\t", (long)svp->fsType);
	if (svp->fsType < 4) {
		printf("(%s)\n", fstype[svp->fsType]);
	} else {
		printf("(INVALID FILESYSTEM TYPE)\n");
	}
	printf("system       = endian=%2d/cpu=%2d///os=%2d/fs=%2d//\n",
	    svp->system.endian, svp->system.cpu,
	    svp->system.os, svp->system.fs);
	printf("glomType     = %8ld\t", (long)svp->glomType);
	if (svp->glomType < 4) {
		printf("(%s)\n", glomtype[svp->glomType]);
	} else {
		printf("(INVALID GLOM TYPE)\n");
	}
	printf("glomInfo     = %8ld [%#lx]\n",
	    (long)svp->glomInfo, (long)svp->glomInfo);
	printf("nDisks       = %8ld [%#lx]\n",
	    (long)svp->nDisks, (long)svp->nDisks);
	printf("blockSize    = %8ld [%#lx]\n",
	    (long)svp->blockSize, (long)svp->blockSize);
	printf("\n");

	if (svp->nDisks > 0 && svp->nDisks < 200) {
		for (i = 0; i < svp->nDisks; i++) {
			printf("Disk[%d]\n", i);
			printf("\tidOffset    = %12lld [%#llx]\n",
			    (long long)svp->dxDisks[i].idOffset,
			    (long long)svp->dxDisks[i].idOffset);
			printf("\tblockOffset = %12lld [%#llx]\n",
			    (long long)svp->dxDisks[i].blockOffset,
			    (long long)svp->dxDisks[i].blockOffset);
			printf("\tidLength    = %12lld [%#llx]\n",
			    (long long)svp->dxDisks[i].idLength,
			    (long long)svp->dxDisks[i].idLength);
			printf("\tnBlocks     = %12lld [%#llx]\n",
			    (long long)svp->dxDisks[i].nBlocks,
			    (long long)svp->dxDisks[i].nBlocks);
			printf("\tflags       = %#x", svp->dxDisks[i].flags);
			if (svp->dxDisks[i].flags) {
				printf("\t\t( ");
				r = svp->dxDisks[i].flags;
				for (j = 0; j < 16; j++) {
					if (r&1) {
						printf("%s ", flagbits[j]);
					}
					r >>= 1;
				}
				printf(")");
			}
			printf("\n\tdiskID:\n");
			for (j = 0; j < sizeof (svp->dxDisks[i].diskID); j++) {
				if ((j%8) == 0)
					printf("\t\t");
				printf(" %2x",
				    svp->dxDisks[i].diskID.chars[j]&0xff);
				if ((j%8) == 7)
					printf("\n");
			}
			printf("\n");
		}
	}

	if (lr || lw) {
		errno = 0;
		r = FS_GetMaxLeases(&svp->cookie, lr ? &lr : NULL,
		    lw ? &lw : NULL);
		if (r < 0) {
			printf("FS_GetMaxLeases() failed; r = %d, "
			    "errno = %d\n", r, errno);
		} else {
			if (lr) {
				printf("   read lease = %lds", (long)lr);
			}
			if (lw) {
				printf("  write lease = %lds", (long)lw);
			}
			printf("\n");
		}
	}
	return (0);
}
