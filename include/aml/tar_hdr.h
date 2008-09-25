/*
 * Declarations for tar archives.
 *    Copyright (C) 1988, 1992, 1993 Free Software Foundation
 *
 * *** This file is copied from GNU Tar, and subsequently modified. ***
 *
 * GNU Tar is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU Tar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Tar; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * For the avoidance of doubt, except that if any license choice other than GPL
 * or LGPL is available it will apply instead, Sun elects to use only the
 * General Public License version 2 (GPLv2) at this time for any software where
 * a choice of GPL license versions is made available with the language
 * indicating that GPLv2 or any later version may be used, or where a choice of
 * which version of the GPL is applied is otherwise unspecified.
 */

#ifndef _AML_TAR_HDR_H
#define	_AML_TAR_HDR_H

#pragma ident "$Id: tar_hdr.h,v 1.4 2008/09/25 18:05:54 am143972 Exp $"

/*
 * Header block on tape.
 */

struct sparse {
	char offset[12];
	char numbytes[12];
};

struct header {
	char arch_name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char linkflag;
	char arch_linkname[100];
	char magic[8];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	/* these following fields were added for gnu */
	/* and are NOT standard */
	char atime[12];
	char ctime[12];
	char offset[12];
	char longnames[4];
	char pad;
	struct sparse sp[4];
	char isextended;
	char realsize[12];	/* true size of the sparse file */
};

union header_blk {
	char	i[512];
	struct header	hs;
};

struct extended_header {
	struct sparse sp[21];
	char isextended;
};

#endif /* _AML_TAR_HDR_H */
