/*
 * samgetmap.c - dump file allocation ("map") information
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


#pragma ident "$Revision: 1.22 $"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sys/types.h"
#include "samsanergy/fsmdc.h"
#include "samsanergy/fsmdcsam.h"

static char *extent_format[] = {
	"COMPACT",
	"VERBOSE",
	"SIMPLE",
	"CANONICAL"
};

static char *extent_validity[] = {
	"DIRECT",
	"HOLE",
	"RESERVED1",
	"RESERVED2",
	"NO ACCESS",
	"NOT PRESENT",
	"READ VALID",
	"READ ONLY"
};

static void
dumpmap(FSMAPINFO *mp)
{
	int i;

	printf("fileSize     = %lld\t[%#llx]\n", mp->fileSize, mp->fileSize);
	printf("allocation   = %lld\t[%#llx]\n",
	    mp->allocation, mp->allocation);
	printf("msgLen       = %ld\t[%#lx]\n",
	    (long)mp->msgLen, (long)mp->msgLen);
	printf("vendorStatus = %ld\t[%#lx]\n",
	    (long)mp->vendorStatus, (long)mp->vendorStatus);
	printf("nExtents     = %ld\t[%#lx]\n",
	    (long)mp->nExtents, (long)mp->nExtents);
	printf("extentType   = %ld", (long)mp->extentType);
	if (mp->extentType >= 0 && mp->extentType < 4) {
		printf("\t(%s)\n", extent_format[mp->extentType]);
	} else {
		printf("\t(INVALID EXTENT FORMAT)\n");
	}
	printf("fileOffset   = %lld\t[%#llx]\n",
	    (long long)mp->fileOffset,
	    (long long)mp->fileOffset);
	printf("fileBytes    = %lld\t[%#llx]\n",
	    (long long)mp->fileBytes, (long long)mp->fileBytes);
	printf("\n");

	switch (mp->extentType) {
	case FS_EXTENT_COMPACT:
		for (i = 0; i < mp->nExtents; i++) {
			printf("cExtent[%d]\n", i);
			printf("\tblockOffset = %llx\t",
			    (long long)mp->cExtent[i].blockOffset);
			printf("\tnBlocks     = %llx\n\n",
			    (long long)mp->cExtent[i].nBlocks);
		}
		break;
	case FS_EXTENT_SIMPLE:
		for (i = 0; i < mp->nExtents; i++) {
			printf("sExtent[%d]\n", i);
			printf("\tvolumeOrdinal = %lld\t[%#llx]\n",
			    (long long)mp->sExtent[i].volumeOrdinal,
			    (long long)mp->sExtent[i].volumeOrdinal);
			printf("\tblockOffset   = %lld\t[%#llx]\n",
			    (long long)mp->sExtent[i].blockOffset,
			    (long long)mp->sExtent[i].blockOffset);
			printf("\tnBlocks       = %lld\t[%#llx]\n\n",
			    (long long)mp->sExtent[i].nBlocks,
			    (long long)mp->sExtent[i].nBlocks);
		}
		break;
	case FS_EXTENT_VERBOSE:
	case FS_EXTENT_CANONICAL:
		for (i = 0; i < mp->nExtents; i++) {
			printf("vExtent[%d]\n", i);
			printf("\tvolumeOrdinal     = %ld\t[%#lx]\n",
			    (long)mp->vExtent[i].volumeOrdinal,
			    (long)mp->vExtent[i].volumeOrdinal);
			printf("\tlogicalByteOffset = %lld\t[%#llx]\n",
			    (long long)mp->vExtent[i].logicalByteOffset,
			    (long long)mp->vExtent[i].logicalByteOffset);
			printf("\tphysicalByteOffset= %lld\t[%#llx]\n",
			    (long long)mp->vExtent[i].physicalByteOffset,
			    (long long)mp->vExtent[i].physicalByteOffset);
			printf("\tnBytes            = %lld\t[%#llx]\n",
			    (long long)mp->vExtent[i].nBytes,
			    (long long)mp->vExtent[i].nBytes);
			printf("\tvalidity          = %ld\t[%#lx]",
			    (long)mp->vExtent[i].validity,
			    (long)mp->vExtent[i].validity);
			if (mp->vExtent[i].validity >= 0 &&
			    mp->vExtent[i].validity < 8) {
				printf("\t(%s)\n\n",
				    extent_validity[mp->vExtent[i].validity]);
			} else {
				printf("\t(INVALID EXTENT TYPE)\n\n");
			}
		}
		break;
	default:
		printf("BAD EXTENT FORMAT (%d).\n\n", mp->extentType);
	}
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
		return ("FS_E_UNKNOWN_FUNCTION (?)");

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


static char *usage =
	"Usage:\n\tsamgetmap\n"
	"\t	[-h]			-- print help message\n"
	"\t	[-a allocsize]		-- preallocate to allocsize\n"
	"\t	[-l setlen]		-- set file size to setlen\n"
	"\t	[-m minalloc]		-- allocate at least minalloc\n"
	"\t	[-c]			-- request canonical map\n"
	"\t	[-f]			-- fill allocation holes in map\n"
	"\t	[-u]			-- don't lock file\n"
	"\t	[-w]			-- wait (for staging?)\n"
	"\t	[-r]			-- retrieve (initiate stage)\n"
	"\t	[-s filestart]		-- start file map at filestart\n"
	"\t	[-n filebytes]		-- get map for at least filebytes\n"
	"\t	[-M]			-- don't get map\n"
	"\t	[-U]			-- don't unlock file\n"
	"\t	file\n\n";

int Mflag = 0;
int Uflag = 0;

int
main(int ac, char *av[])
{
	unsigned long long buf[32*1024];
	char msgbuf[256];
	FSULONG flags = 0, buflen = sizeof (buf);
	FS64LONG filesize = (FS64LONG)-1,
	    allocmin = (FS64LONG)-1,
	    allocsize = (FS64LONG)-1;
	FS64LONG fstart = 0, flen = (FS64LONG)-1;
	FSMAPINFO *smp = (FSMAPINFO *)buf;
	FSVOLCOOKIE vc;
	FSFILECOOKIE fc;
	int c, i;
	FSLONG r;
	extern int errno;

	while ((c = getopt(ac, av, "a:cfhl:m:Mn:rs:uUVw")) != EOF) {
		switch (c) {
		case 'a':		/* set allocated size */
			allocsize = atoll(optarg);
			break;
		case 'c':
			flags |= FS_M_FLAG_CANONICAL;
			break;
		case 'l':		/* set file len */
			filesize = atoll(optarg);
			break;
		case 'm':		/* min alloc size */
			allocmin = atoll(optarg);
			break;

		case 'f':		/* fill any allocation holes */
			flags |= FS_M_FLAG_NO_HOLE;
			break;
		case 'u':		/* don't lock file */
			flags |= FS_M_FLAG_UNLOCKED;
			break;
		case 'r':
			flags |= FS_M_FLAG_RETRIEVE;
			break;
		case 'w':		/* WAIT for entire submap */
			flags |= FS_M_FLAG_WAIT;
			break;

		case 'M':		/* don't get map */
			Mflag = 1;
			break;
		case 'U':		/* don't unlock file */
			Uflag = 1;
			break;

		case 's':		/* file map start offset */
			fstart = atoll(optarg);
			break;
		case 'n':		/* file map len */
			flen = atoll(optarg);
			break;

		case 'V':
			printf("Version (FS_GetVersion) = %ld\n",
			    (long)FS_GetVersion());
			break;

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
	errno = 0;
	r = AFS_GetCookies(av[optind], &vc, &fc);
	printf("AFS_GetCookies(\"%s\", &vc, &fc) = %#lx (%s)\n",
	    av[optind], (long)r, mapretcode(r, msgbuf));
	printf("\nVolcookie:\n");
	for (i = 0; i < sizeof (vc); i++) {
		if ((i%8) == 0)
			printf("\t");
		printf(" %2x", vc.xprivate[i]&0xff);
		if ((i%16) == 15)
			printf("\n");
	}
	printf("\nFilecookie:\n");
	for (i = 0; i < sizeof (fc); i++) {
		if ((i%8) == 0)
			printf("\t");
		printf(" %2x", fc.xprivate[i]&0xff);
		if ((i%16) == 15)
			printf("\n");
	}
	printf("\n");

	if (!Mflag) {
		r = FS_GetLockedMap(&vc, &fc,
		    fstart, flen,
		    flags,
		    smp, &buflen);
		printf("FS_GetLockedMap(&vc, &fc, %lld, %lld, %lx, %llx, %llx)"
		    "  = %#lx (%s)\n",
		    fstart, flen, (long)flags,
		    (long long)buf, (long long)&buflen,
		    (long)r, mapretcode(r, msgbuf));
		if (r != FS_E_SUCCESS) {
			sam_fsuni2mb(msgbuf, smp->vendorError,
			    sizeof (buf)-offsetof(FSMAPINFO, vendorError)),
			    printf("vendorStatus=%d, vendorError='%s', "
			    "errno='%s' (%d)\n",
			    smp->vendorStatus, msgbuf,
			    strerror(errno), (int)errno);
		}
		printf("returned buflen = %d\n", (int)buflen);
		dumpmap(smp);
	}

	r = FS_SetFileSizes(&vc, &fc, 0, filesize, allocsize, allocmin);
	printf("FS_SetFileSizes(&vc, &fc, 0, %llx, %llx, %llx)  = %#lx (%s)\n",
	    filesize, allocsize, allocmin,
	    (long)r, mapretcode(r, msgbuf));

	if (!Uflag) {
		r = FS_UnlockMap(&vc, &fc);
		printf("FS_UnlockMap(&vc, &fc) = %#lx (%s)\n",
		    (long)r, mapretcode(r, msgbuf));
	}

	return (0);
}
