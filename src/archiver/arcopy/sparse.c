/*
 * sparse.c - Sparse file archiving routines.
 *
 * Sparse is responsible for determining the actual storage blocks
 * occupied by files, and generating lists of extents so that they
 * may be archived and eventually staged efficiently.
 *
 * Finally, these lists are used by the ReadSparse function,
 * which simulates the 'read' function, also indicating which
 * areas read are "real" (non-zero) data.
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

#pragma ident "$Revision: 1.20 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */


/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

/* POSIX headers. */
#include <unistd.h>

/* Solaris headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "sam/types.h"
#include "sam/sam_malloc.h"
#if !defined(_NoTIVOLI_)
#include "samsanergy/fsmdc.h"
#endif

/* Local headers. */
#include "arcopy.h"
#include "sparse.h"

#define	DEF_FILEMAPSIZE	(64*1024)

/* Public data. */
struct FileStoreInfo *Fsip = NULL;

/* Private data. */
static offset_t fileDataBytes = -1;

#if !defined(_NoTIVOLI_)

static FSMAPINFO *svmap;
static FS64LONG svlen;

/* Private functions. */
static int sparseArchSetUp(int);
static int sparseArchGetExtents(struct FileStoreInfo *);

/*
 * Build an extent list for the file whose 'fd' we're given.
 * First, allocate a full map for the file, and get all the
 * file's storage allocation info.  From that, compute the
 * size of the extent list, allocate space for it, and then
 * fill it in.
 */
void
GetFileStoreInfo(
	int fileFd)
{
	int	n;

	fileDataBytes = -1;
	if (Fsip != NULL) {
		SamFree(Fsip);
		Fsip = NULL;
	}

	n = sparseArchSetUp(fileFd);
	if (n < 0) {		/* not sparse */
		return;
	}

	SparseFile = TRUE;
	SamMalloc(Fsip, sizeof (*Fsip)+n*sizeof (Fsip->f_ex[0]));
	Fsip->f_nextents = n;
	if (sparseArchGetExtents(Fsip) < 0) {
		SamFree(Fsip);
		Fsip = NULL;
		return;
	}
	fileDataBytes = Fsip->f_nbytes;
}
#endif


/*
 * Return the number of data "payload" bytes that need to be written.
 * This function should only be called if IsSparse() returns true or
 * the caller is willing to accept -1 for non-sparse files.
 */
offset_t
GetFileSize(void)
{
	return (fileDataBytes);
}


/*
 * Return true if the file being is to be written using star "sparse"
 * format.
 */
int
IsSparse()
{
	return (fileDataBytes >= 0);
}


/*
 * This one's a little complicated.  It replaces read(2), and has
 * some twists.  First, it uses the extent list indicating what areas
 * of the file need to be read.  Second, it takes an argument indicating
 * whether zeroes need to be returned for the sparse areas.  (Checksum
 * values may be incorrect if the omitted zeroes are not returned.
 * So if checksumming is enabled, sparse archiving will slow as the
 * sparse areas are "read" and checksummed (tho' they won't be written
 * out to disk). That argument is returned as either a zero (indicating
 * that any data returned are sparse/zeroes), or a one (indicating that
 * the data returned should go to tape).
 *
 * Basically, if the data area to be read is all data, we issue the
 * normal read and return it.  If it runs from a data area over into
 * a sparse area, we return a short read (up to the sparse area).
 * If the data area to be read is all sparse, we return zeroes if *d
 * is set, otherwise we return data from the next extent.  If the
 * data area runs from a sparse area into a data area we return the
 * zeroes up to the data area (if *d is set), or the data from the
 * next data area (if it's not).
 */
int
ReadSparse(
	int fd,
	void *buf,
	size_t len,
	boolean_t *d)
{
	offset_t off;
	int	ext;
	int	r;

	if ((off = llseek(fd, 0LL, SEEK_CUR)) == -1LL) {
		Trace(TR_DEBUGERR, "Readsparse:  llseek failed.");
		return (-1);
	}

	/*
	 * Locate the proper extent for this read
	 *
	 * It might be desirable to rework this so that the 'current extent'
	 * is kept around so that we don't have to search for it each time.
	 */
	for (ext = 0; ext+1 < Fsip->f_nextents; ++ext) {
		if (Fsip->f_ex[ext+1].start >= off) {
			break;
		}
	}
	if (off >= Fsip->f_ex[ext].start + Fsip->f_ex[ext].count) {
		/*
		 * We're trying to read a sparse area after the extent.
		 * Change to read prior to next extent.
		 */
		if (ext+1 >= Fsip->f_nextents) {
			/*
			 * File ends in sparse region.  Rare but possible --
			 * truncating a sparse file can yield this situation.
			 */
			bzero(buf, len);
			*d = FALSE;
			if (llseek(fd, (long long)len, SEEK_CUR) == -1) {
				Trace(TR_DEBUGERR,
				    "llseek in unreal data failed");
				return (-1);
			}
			return (len);
		}
		++ext;
	}
	if (off < Fsip->f_ex[ext].start) {
		if (*d) {
			if (off+len > Fsip->f_ex[ext].start)
				len = Fsip->f_ex[ext].start - off;
			bzero(buf, len);
			*d = FALSE;
			if (llseek(fd, (long long)len, SEEK_CUR) == -1) {
				Trace(TR_DEBUGERR,
				    "llseek in unreal data failed");
				return (-1);
			}
			return (len);
		}
		/*
		 * reading sparse area, data not required:  seek to extent
		 */
		if ((off = llseek(fd, Fsip->f_ex[ext].start, SEEK_SET)) == -1) {
			Trace(TR_DEBUGERR, "llseek to real data failed");
			return (-1);
		}
	}
	if (off >= Fsip->f_ex[ext].start + Fsip->f_ex[ext].count) {
		Trace(TR_DEBUGERR, "SPARSEREAD:  Unexpected situation.");
		return (-1);
	}

	/*
	 * Read real data
	 */
	if (off < Fsip->f_ex[ext].start + Fsip->f_ex[ext].count) {
		if (off+len > Fsip->f_ex[ext].start+Fsip->f_ex[ext].count) {
			len =
			    Fsip->f_ex[ext].start + Fsip->f_ex[ext].count - off;
		}
		*d = TRUE;	/* real data */
		r = read(fd, buf, len);
		return (r);
	}

	Trace(TR_DEBUGERR, "NOTHING TO READ/%llx", off);
	return (-1);
}


#if !defined(_NoTIVOLI_)
/*
 * Return the number of extents that the file
 * has.  This count is needed for the caller to
 * allocate a buffer to hold the extent info.
 * Return < 0 ==> error.
 */
static int
sparseArchSetUp(
	int fileFd)
{
extern int sam_fd_storage_ops(int fd, FS64LONG flags, offset_t flen,
/* cstyle */    offset_t start, offset_t len, FSMAPINFO *ubuf, FS64LONG ulen);

	FSLONG	r;
	int	extents;
	int	i;
	int	inextent;

	if (!svmap) {
		svlen = DEF_FILEMAPSIZE;
		SamMalloc(svmap, svlen);
	}

	r = sam_fd_storage_ops(fileFd, 0, 0, 0, -1, svmap, svlen);
	if (r < 0 && errno == E2BIG) {
		/*
		 * allocate a bigger buffer, and call again
		 */
		svlen = svmap->msgLen;	/* bytes needed */
		if (svlen < DEF_FILEMAPSIZE) {
			svlen = DEF_FILEMAPSIZE;
		}
		SamFree(svmap);
		SamMalloc(svmap, svlen);
		r = sam_fd_storage_ops(fileFd, 0, 0, 0, -1, svmap, svlen);
	}

	if (r < 0) {
		Trace(TR_DEBUGERR, "sam_fd_storage_ops() failed.");
		return (-1);
	}

	if (svmap->allocation >= svmap->fileSize) {
		/* not sparse */
		return (0);
	}

	if (svmap->extentType != FS_EXTENT_VERBOSE) {
		Trace(TR_DEBUGERR, "Unexpected map type (%d).",
		    (int)svmap->extentType);
		return (-1);
	}

	extents = 0;
	inextent = FALSE;
	for (i = 0; i < svmap->nExtents; i++) {
		switch (svmap->vExtent[i].validity) {
		case FS_EXT_DIRECT_ACCESS:
			if (!inextent) {
				inextent = TRUE;
				extents++;
			}
			break;
		case FS_EXT_HOLE:
			if (inextent) {
				inextent = FALSE;
			}
			break;
		default:
			Trace(TR_DEBUGERR, "Unexpected extent type (%d).",
			    svmap->vExtent[i].validity);
			return (-1);
		}
	}
	return (extents);
}


/*
 * When called, we have the file map saved in svmap, and space
 * for the file's extent list has been allocated to Fsip.  Run
 * through the file map and build the extent list.
 */
int
sparseArchGetExtents(
	struct FileStoreInfo *fp)
{
	struct Extent *ep = &fp->f_ex[0];
	fsize_t nbytes;
	int n = fp->f_nextents;
	int i, inextent, ex;

	if (svmap->extentType != FS_EXTENT_VERBOSE) {
		Trace(TR_DEBUGERR, "Non-verbose map (%d)",
		    (int)svmap->extentType);
		return (-1);
	}

	if (n == 0) {
		ep[0].start = 0;
		ep[0].count = 0;
		return (0);
	}

	inextent = FALSE;
	for (i = ex = 0; ex < n && i < svmap->nExtents; i++) {
		switch (svmap->vExtent[i].validity) {
		case FS_EXT_DIRECT_ACCESS:
			if (!inextent) {
				ep[ex].start =
				    svmap->vExtent[i].logicalByteOffset;
				ep[ex].count =
				    svmap->vExtent[i].nBytes;
				inextent = TRUE;
			} else {
				ep[ex].count += svmap->vExtent[i].nBytes;
			}
			break;

		case FS_EXT_HOLE:
			if (inextent) {
				ex++;
				inextent = FALSE;
			}
			break;

		default:
			Trace(TR_DEBUGERR, "Unexpected validity code (%d).",
			    (int)svmap->vExtent[i].validity);
			return (-1);
		}
	}
	if (n != ex+1) {
		Trace(TR_DEBUGERR, "Extent mismatch n(%d) != x+1(%d).",
		    n, ex+1);
		return (-1);
	}

	nbytes = 0LL;
	for (i = 0; i < n; i++) {
		if (fp->f_ex[i].start >= svmap->fileSize) {
			n = i;		/* Don't use this or extents beyond */
			break;
		}
		if (fp->f_ex[i].start + fp->f_ex[i].count > svmap->fileSize)
			fp->f_ex[i].count = svmap->fileSize - fp->f_ex[i].start;
		nbytes += fp->f_ex[i].count;
	}
	fp->f_size = svmap->fileSize;
	fp->f_nbytes = nbytes;
	return (0);
}
#endif
