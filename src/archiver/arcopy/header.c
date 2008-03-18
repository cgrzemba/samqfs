/*
 * header.c - Build tar header record.
 *
 */

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

#pragma ident "$Revision: 1.27 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include <pub/stat.h>
#include <sam/types.h>
#include <sam/lib.h>
#include <aml/tar.h>
#include <aml/tar_hdr.h>

/* Local headers. */
#include "arcopy.h"
#include "sparse.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private functions. */
static void writeHeader(char *name, struct sam_stat *st, char type);
static char *getgname(int gid);
static char *getuname(int uid);
static void l2oct(uint32_t value, char *dest, int width);

/* Private data. */
static struct sam_stat sta;

/* Public data. */
char linkname[MAXPATHLEN + 1];
int link_l;


/*
 * Build tar header record.
 */
void
BuildHeader(
	char *name,		/* File name. */
	int name_l,		/* Length of file name. */
	struct sam_stat *st)	/* File status. */
{
	if (name_l >= NAMSIZ) {
		sta.st_size = (u_longlong_t)name_l;
		writeHeader("././@LongLink", &sta, LF_LONGNAME);
		WriteData(name, name_l);
		RoundBuffer(TAR_RECORDSIZE);
	}
	if (S_ISLNK(st->st_mode)) {
		snprintf(ScrPath, sizeof (ScrPath), "%s/%s", MntPoint, name);
		if ((link_l =
		    readlink(ScrPath, linkname, sizeof (linkname)-1)) < 0) {
			Trace(TR_DEBUGERR, "readlink(%s)", ScrPath);
			link_l = 0;
		}
		linkname[link_l] = '\0';
		if (link_l >= NAMSIZ) {
			sta.st_size = link_l;
			writeHeader("././@LongLink", &sta, LF_LONGLINK);
			WriteData(linkname, link_l);
			RoundBuffer(TAR_RECORDSIZE);
		}
		st->st_size = link_l;
		writeHeader(name, st, LF_SYMLINK);
	} else if (S_ISDIR(st->st_mode)) {
		writeHeader(name, st, LF_DIRHDR);
	} else {
		writeHeader(name, st, IsSparse() ? LF_SPARSE : LF_NORMAL);
	}
}



/* Private functions. */

/*
 * Write a tar header record.
 */
static void
writeHeader(
	char *name,		/* File name. */
	struct sam_stat *st,	/* File status. */
	char type)		/* Header type. */
{
	struct header *hdr;
	struct Extent *ep;
	int i, n, sum;
	uchar_t *p;

	hdr = (struct header *)WaitRoom(TAR_RECORDSIZE);
	memset(hdr, 0, TAR_RECORDSIZE);

	/* Remove leading '/' from file name. */
	while (*name == '/') {
		name++;
	}
	strncpy(hdr->arch_name, name, sizeof (hdr->arch_name)-1);
	l2oct(st->st_mode, hdr->mode, sizeof (hdr->mode));
	l2oct(st->st_uid, hdr->uid, sizeof (hdr->uid));
	l2oct(st->st_gid, hdr->gid, sizeof (hdr->gid));
	ll2str(type == LF_SPARSE ? Fsip->f_nbytes : st->st_size,
	    hdr->size, sizeof (hdr->size)+1);
	l2oct(st->st_mtime, hdr->mtime, sizeof (hdr->mtime)+1);
#ifdef GNUTAR
	l2oct(st->st_atime, hdr->atime, sizeof (hdr->atime)+1);
	l2oct(st->st_ctime, hdr->ctime, sizeof (hdr->ctime)+1);
#endif
	hdr->linkflag = type;
	if (type == LF_SYMLINK) {
		strncpy(hdr->arch_linkname, linkname,
		    sizeof (hdr->arch_linkname)-1);
	}
	/* Mark as Unix Standard. */
	strncpy(hdr->magic, TMAGIC, sizeof (hdr->magic)-1);
	strncpy(hdr->uname, getuname(st->st_uid), sizeof (hdr->uname)-1);
	strncpy(hdr->gname, getgname(st->st_gid), sizeof (hdr->gname)-1);
	if (type == LF_SPARSE) {
		ep = &Fsip->f_ex[0];
		for (n = 0; n < Fsip->f_nextents && n < SPARSE_IN_HDR;
		    n++, ep++) {
			ll2str(ep->start, hdr->sp[n].offset,
			    sizeof (hdr->sp[n].offset)+1);
			ll2str(ep->count, hdr->sp[n].numbytes,
			    sizeof (hdr->sp[n].numbytes)+1);
		}
		hdr->isextended = (n != Fsip->f_nextents);
	}

	ll2str(st->st_size, hdr->realsize, sizeof (hdr->realsize)+1);

	/*
	 * Generate header checksum.
	 * During the checksumming, the checksum field contains all spaces.
	 */
	memset(hdr->chksum, ' ', sizeof (hdr->chksum));
	sum = 0;
	for (p = (uchar_t *)hdr, i = sizeof (*hdr); i-- > 0; )
		sum += *p++;

	/*
	 * Fill in the checksum field.  It's formatted differently from
	 * the other fields:  it has digits, a null, then a space.
	 */
	l2oct(sum, hdr->chksum, sizeof (hdr->chksum));
	hdr->chksum[6] = '\0';
	AdvanceIn(TAR_RECORDSIZE);


	/*
	 * Generate any extra SPARSE headers required
	 */
	if (type == LF_SPARSE) {
		while (n < Fsip->f_nextents) {
			struct extended_header *ehdr;

			ehdr =
			    (struct extended_header *)WaitRoom(TAR_RECORDSIZE);
			memset(ehdr, 0, TAR_RECORDSIZE);
			for (i = 0; n < Fsip->f_nextents && i < SPARSE_EXT_HDR;
			    i++, n++, ep++) {
				ll2str(ep->start, ehdr->sp[i].offset,
				    sizeof (ehdr->sp[i].offset)+1);
				ll2str(ep->count, ehdr->sp[i].numbytes,
				    sizeof (ehdr->sp[i].numbytes)+1);
			}
			ehdr->isextended = (n != Fsip->f_nextents);
			AdvanceIn(TAR_RECORDSIZE);
		}
	}
}

/*
 * Get the group name given the group id.
 * Uses a single entry cache.
 */
static char *
getgname(
	int gid)	/* Group id. */
{
	static char gname[TGNMLEN] = "";
	static int sgid = -1;
	struct group *gr;

	if (gid != sgid) {
		sgid = gid;
		gname[0] = '\0';
		if ((gr = getgrgid(gid)) != NULL) {
			strncpy(gname, gr->gr_name, sizeof (gname)-1);
		}
	}
	return (gname);
}


/*
 * Get the user name given the uid.
 * Uses a single entry cache.
 */
static char *
getuname(
	int uid)	/* User id. */
{
	static char uname[TUNMLEN] = "";
	static int suid = -1;
	struct passwd *pw;

	if (uid != suid) {
		suid = uid;
		uname[0] = '\0';
		if ((pw = getpwuid(uid)) != NULL) {
			strncpy(uname, pw->pw_name, sizeof (uname)-1);
		}
	}
	return (uname);
}


/*
 * Convert a long value to octal.
 * Convert value to octal in a character field.
 * The field consists of leading spaces, octal digits, a single space,
 * and room for a terminating '\0'.  The '\0' is not stored.  The field
 * will not overflow to the left.
 * e.g.  A value of 077 and field width of 3 would produce: "7 ".
 */
static void
l2oct(
	uint32_t value,		/* Value to convert. */
	char *dest,		/* Character field. */
	int width)		/* Destination field width */
{
	width -= 2;
	dest += width;	/* Store space in last character position. */
	*dest-- = ' ';

	/* Convert digits. */
	while (width-- > 0) {
		*dest-- = (value & 7) + '0';
		value = value >> 3;
		if (value == 0) {
			break;
		}
	}

	/* Add leading spaces. */
	while (width-- > 0) {
		*dest-- = ' ';
	}
}
