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

#pragma ident "$Revision: 1.31 $"

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

#include <pax_hdr/pax_hdr.h>

/* Local headers. */
#include "arcopy.h"
#include "sparse.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

#define	STANDARDIZED_MODE_BITS 07777
#define	SAM_ctime "SAM.ctime"

/* Private functions. */
static void buildPaxHeader(char *name, int name_l, struct sam_stat *st);
static pax_hdr_t *createDefaultHeader();
static void samTimeToPaxTime(time_t sam_time, pax_time_t *pax_time);
static void buildLegacyHeader(char *name, int name_l, struct sam_stat *st);
static void writeHeader(char *name, struct sam_stat *st, char type);
static char *getgname(int gid);
static char *getuname(int uid);
static void l2oct(uint32_t value, char *dest, int width);

/* Private data. */
static struct sam_stat sta;

/* Public data. */
char linkname[MAXPATHLEN + 1];
int link_l;

void
BuildHeader(
	char *name,		/* File name. */
	int name_l,		/* Length of file name. */
	struct sam_stat *st)	/* File status. */
{
	switch (ArchiveFormat) {
	case LEGACY_FORMAT:
		buildLegacyHeader(name, name_l, st);
		break;
	case PAX_FORMAT:
		buildPaxHeader(name, name_l, st);
		break;
	default:
		LibFatal(BuildHeader, "unknown archive format");
	}
}


/* Private functions. */

/*
 * Builds a pax header and writes it to the archive medium.
 */
static void
buildPaxHeader(
	char *name,		/* File name. */
	/* LINTED argument unused in function */
	int name_l,		/* Length of file name. */
	struct sam_stat *st)	/* File status. */
{
	static char *hdrBuffer = NULL;
	static int hdrBufLen = 0;

	int status;
	pax_hdr_t *hdr;
	int has_xhdr;
	size_t header_size;
	char *buffer;
	pax_pair_t *ctime_pair;
	pax_time_t mtime;
	pax_time_t atime;
	pax_time_t ctime;
	char *bufLast;

	status = PX_SUCCESS;
	hdr = ph_create_hdr();
	ctime_pair = NULL;

	if (!hdr) {
		LibFatal(ph_create_header, "failed to create a header");
	}

	/* Remove leading '/' from file name. */
	while (*name == '/') {
		name++;
	}

	status += ph_set_name(hdr, name);
	status += ph_set_mode(hdr, st->st_mode & STANDARDIZED_MODE_BITS);
	status += ph_set_uid(hdr, st->st_uid);
	status += ph_set_gid(hdr, st->st_gid);
	status += ph_set_uname(hdr, getuname(st->st_uid));
	status += ph_set_gname(hdr, getgname(st->st_gid));
	status += ph_set_size(hdr, st->st_size);

	samTimeToPaxTime(st->st_mtime, &mtime);
	samTimeToPaxTime(st->st_atime, &atime);
	samTimeToPaxTime(st->st_ctime, &ctime);

	status += ph_set_mtime(hdr, mtime);
	status += ph_set_atime(hdr, atime);

	ctime_pair = pxp_mkpair_time(SAM_ctime, ctime);
	status += pxh_put_pair(&hdr->xhdr_list, ctime_pair, 0);

	if (S_ISLNK(st->st_mode)) {
		snprintf(ScrPath, sizeof (ScrPath), "%s/%s", MntPoint, name);
		if ((link_l =
		    readlink(ScrPath, linkname, sizeof (linkname)-1)) < 0) {
			link_l = 0;
		}
		linkname[link_l] = '\0';
		status += ph_set_linkname(hdr, linkname);
		status += ph_set_type(hdr, PAX_TYPE_SYMLINK);
	} else if (S_ISDIR(st->st_mode)) {
		status += ph_set_type(hdr, PAX_TYPE_DIRECTORY);
	} else {
		status += ph_set_type(hdr, PAX_TYPE_FILE);
	}

	if (status) {
		LibFatal(buildPaxHeader, "failed setting a header field");
	}

	has_xhdr = ph_has_ext_hdr(hdr);

	if (has_xhdr && !DefaultHeader) {
		DefaultHeader = createDefaultHeader();
		if (!DefaultHeader) {
			LibFatal(createDefaultHeader,
			    "could not create default header");
		}
	}

	status = ph_get_header_size(hdr, &header_size, NULL, NULL);
	if (!PXSUCCESS(status)) {
		LibFatal(ph_get_header_size, "failed to get header size");
	}

	buffer = WaitRoom(header_size);

	/*
	 * Check if the tar header write needs to wrap around to top
	 * of the buffer.
	 */
	bufLast = GetBufLast();
	if (((long)buffer + header_size) > (long)bufLast) {
		long n;
		char *bufFirst;

		/* Allocate temp space to build tar header. */
		if (hdrBuffer == NULL) {
			SamMalloc(hdrBuffer, header_size);
		} else if (header_size > hdrBufLen) {
			SamRealloc(hdrBuffer, header_size);
		}
		hdrBufLen = header_size;

		/* Write pax header to temp buffer. */
		status = ph_write_header(hdr, hdrBuffer, header_size, NULL,
		    DefaultHeader, ph_basic_name_callback);

#if 0
		ASSERT_WAIT_FOR_DBX(B_FALSE);
#endif

		/* Copy pax header to circular i/o buffer. */
		if (PXSUCCESS(status)) {
			n = (long)bufLast - (long)buffer;

			memcpy(buffer, hdrBuffer, n);

			bufFirst = GetBufFirst();
			memcpy(bufFirst, hdrBuffer + n, header_size - n);
		}

	} else {
		/* Write pax header to circular i/o buffer. */
		status = ph_write_header(hdr, buffer, header_size, NULL,
		    DefaultHeader, ph_basic_name_callback);
	}

	if (!PXSUCCESS(status)) {
		LibFatal(ph_get_write_header, "failed to write header");
	}

	AdvanceIn(header_size);

	ph_destroy_hdr(hdr);
}

/*
 * Creates a default header for the pax library to use when actual header
 * data won't fit into the required ustar header blocks.
 */
static pax_hdr_t *
createDefaultHeader()
{
	int status = PX_SUCCESS;
	pax_hdr_t *hdr = NULL;
	pax_time_t now;

	hdr = ph_create_hdr();

	if (!hdr) {
		return (NULL);
	}

	now.sec = (int64_t)time(0);
	now.nsec = 0;

	status += ph_set_uname(hdr, "root");
	status += ph_set_gname(hdr, "root");
	status += ph_set_uid(hdr, 0);
	status += ph_set_gid(hdr, 0);
	status += ph_set_mtime(hdr, now);
	status += ph_set_type(hdr, '0');

	if (status) {
		ph_destroy_hdr(hdr);
		hdr = NULL;
	}

	return (hdr);
}

/*
 * Converts a sam time_t to a pax_time.
 */
static void
samTimeToPaxTime(
	time_t sam_time,
	pax_time_t *pax_time)
{
	pax_time->sec = sam_time;
	pax_time->nsec = 0;
}

/*
 * Build tar header record.
 */
static void
buildLegacyHeader(
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
	strncpy(hdr->magic, TMAGIC, sizeof (hdr->magic));
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
