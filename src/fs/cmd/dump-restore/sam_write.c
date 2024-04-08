/*
 *	sam_write.c - write a csd dump file.
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


#pragma ident "$Revision: 1.11 $"


/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* POSIX headers. */
#include <grp.h>
#include <unistd.h>

/* Solaris headers. */
#include <utility.h>
#include <sys/param.h>
#include <sys/acl.h>

/* SAM-FS headers. */
#include <sam/types.h>
#include <pub/rminfo.h>
#include <sam/fs/ino.h>
#include <sam/uioctl.h>
#include <sam/sam_malloc.h>
#include <sam/fs/validation.h>
#include <sam/fs/dirent.h>
#include <sam/lib.h>
#include "sam/nl_samfs.h"
#include <aml/tar_hdr.h>

/* Local headers. */
#include "csd_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private functions. */


static char *_SrcFile = __FILE__;

/*
 * ----- csd_write - write an entry to the csd file
 *
 *	In csd_version = CSD_VERS_4,
 *	we write an int giving the number of characters in the pathname first;
 *
 *	In csd_version >= CSD_VERS_5,
 *	we write a header consisting of three longs. First, a magic number
 *	which is "csdf". Next is a flags field. Lastly, is the name length.
 *
 *	In all versions,
 *	Then the pathname; then the base inode; then, if needed, the
 *	resource record; then, if needed, the link name; then, if needed, the
 *	vsn arrays. The vsn array for an archive copy is only written if
 *	one exists. The order is dependent on the existence of an aid
 *	in the permanent inode.
 *	Then access control list info is written if the permanent inode
 *	indicates an ACL is present.  The count of ACL entries is written,
 *	followed by the array of ACLs.
 */
int
csd_write(
	char	*name,
	struct sam_perm_inode *perm_inode,
	int	n_vsns,
	struct sam_vsn_section *vsnp,
	char	*link,
	void	*data,
	int	n_acls,
	aclent_t *aclp,
	long	flags)
{
	csd_fhdr_t	hdr;
	int namelen = strlen(name);

	if (namelen > MAXPATHLEN) {
		BUMP_STAT(errors);
		error(0, 0,
		    catgets(catfd, SET, 268, "%s: Pathname too long"),
		    name);
		return (1);
	}

	if (verbose) {
		fprintf(stderr, "%s\n", name);
	}

	memset((char *)&hdr, '\0', sizeof (hdr));
	hdr.magic = CSD_FMAGIC;
	hdr.flags = flags;
	hdr.namelen = namelen;
	writecheck(&hdr, sizeof (hdr), 3091);
	writecheck(name, strlen(name), 3092);
	writecheck(perm_inode, sizeof (struct sam_perm_inode), 3093);
	if (S_ISREQ(perm_inode->di.mode)) {
		writecheck(data, perm_inode->di.psize.rmfile, 3094);
		return (0);
	}
	if (S_ISLNK(perm_inode->di.mode)) {
		int linklen = strlen(link);
		writecheck(&linklen, sizeof (int), 3095);
		writecheck(link, strlen(link), 3096);
		return (0);
	}
	if (n_vsns) {
		writecheck(vsnp, sizeof (struct sam_vsn_section) * n_vsns,
		    5008);
	}
	if (n_acls >= 0) {
		writecheck(&n_acls, sizeof (int), 5027);
		if (n_acls > 0) {
			writecheck(aclp, (sizeof (aclent_t) * n_acls), 5028);
		}
	}
	if (S_ISSEGI(&perm_inode->di)) {
		struct sam_ioctl_idstat idstat;
		struct sam_perm_inode seg_inode;
		struct sam_perm_inode_v1 *seg_inode_v1;
		int off, i, ord;
		int nstale, ndam, nseg, maxseg;
		offset_t seg_size;
		sam_id_t *sid;
		struct sam_vsn_section *seg_vsnp = NULL;
		int n_vsns;
		int copy;

		seg_size = (offset_t)perm_inode->di.rm.info.dk.seg_size *
		    SAM_MIN_SEGMENT_SIZE;
		maxseg = (perm_inode->di.rm.size + seg_size - 1) / seg_size;

		ord = 0;
		nstale = ndam = nseg = 0;
		for (off = 0; off < perm_inode->di.rm.info.dk.seg.fsize;
		    off += SAM_SEG_BLK, ord += SAM_MAX_SEG_ORD) {
			sid = (sam_id_t *)(void *)((char *)data + off);
			for (i = 0; i < SAM_MAX_SEG_ORD; i++, sid++) {
				/*
				 * Stat the seg_ino.  We know the "id" from
				 * the segment entry.
				 */
				idstat.id = *sid;
				if (idstat.id.ino == 0) {
					/*
					 * XXX - should not happen? may
					 * result in corrupt dump.
					 */
					continue;
				}
				idstat.size = sizeof (seg_inode);
				idstat.dp.ptr = (void *)&seg_inode;
				if (ioctl(SAM_fd, F_IDSTAT, &idstat) < 0) {
					BUMP_STAT(errors);
					error(0, errno,
					    catgets(catfd, SET, 5016,
					    "%s: cannot F_IDSTAT, %d.%d"),
					    name, sid->ino, sid->gen);
					return (1);
				}
				nseg++;
				if ((seg_inode.di.arch_status == 0) &&
				    (seg_inode.di.rm.size != 0) &&
				    !dumping_file_data) {
					if (seg_inode.di.ar_flags[0] &
					    stale_flags) {
						nstale++;
					} else {
						ndam++;
					}
				}
				writecheck(&seg_inode,
				    sizeof (struct sam_perm_inode), 13525);
				if (verbose) {
					fprintf(stderr, "%s %d.%d S%d\n", name,
					    seg_inode.di.id.ino,
					    seg_inode.di.id.gen,
					    seg_inode.di.rm.info.dk.seg.ord +
					    1);
				}
				n_vsns = 0;



	/* N.B. Bad indentation here to meet cstyle requirements */
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (seg_inode.di.version >= SAM_INODE_VERS_2) {
			/* Current version */
			if (seg_inode.ar.image[copy].n_vsns > 1) {
				n_vsns += seg_inode.ar.image[copy].n_vsns;
			}
		} else if (seg_inode.di.version == SAM_INODE_VERS_1) {
			seg_inode_v1 =
			    (struct sam_perm_inode_v1 *)&seg_inode;
			if (seg_inode_v1->aid[copy].ino != 0) {
				if (seg_inode.ar.image[copy].n_vsns > 1) {
					n_vsns +=
					    seg_inode.ar.image[copy].n_vsns;
				} else {
					seg_inode_v1->aid[copy].ino = 0;
					seg_inode_v1->aid[copy].gen = 0;
				}
			}
		}
	}
	/*
	 * If overflow vsns for any archive copy,
	 * get them.
	 */
	if (n_vsns) {
		struct sam_ioctl_idmva idmva;

		SamMalloc(seg_vsnp,
		    n_vsns * sizeof (struct sam_vsn_section));
		idmva.id = *sid;
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (seg_inode.di.version >= SAM_INODE_VERS_2) {
				idmva.aid[copy].ino = idmva.aid[copy].gen = 0;
			} else if (seg_inode.di.version == SAM_INODE_VERS_1) {
				seg_inode_v1 =
				    (struct sam_perm_inode_v1 *)&seg_inode;
				idmva.aid[copy] = seg_inode_v1->aid[copy];
			}
		}
		idmva.size = sizeof (struct sam_vsn_section) * n_vsns;
		idmva.buf.ptr = (void *)seg_vsnp;
		if (ioctl(SAM_fd, F_IDMVA, &idmva) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5024,
			    "%s: cannot F_IDMVA, %d.%d"),
			    name, seg_inode.di.id.ino, seg_inode.di.id.gen);
			if (seg_vsnp) {
				SamFree(seg_vsnp);
			}
			return (1);
		}
	}



				if (seg_vsnp) {
					writecheck(seg_vsnp,
					    sizeof (struct sam_vsn_section) *
					    n_vsns, 5008);
					SamFree(seg_vsnp);
					seg_vsnp = NULL;
				}
				if (nseg >= maxseg) {
					break;
				}
			}
			if (nseg >= maxseg) {
				break;
			}
		}
		if (nseg != maxseg) {
			BUMP_STAT(errors);
			error(0, 0, catgets(catfd, SET, 5035,
			    "%s: %d of %d segments are missing, cannot "
			    "dump file."),
			    name, maxseg - nseg, maxseg);
			/*
			 * Perhaps we should back up at this point, but the
			 * buffered write
			 * mechanism makes that difficult.  Instead, we pad
			 * the file with
			 * data that the restore can (more or less) skip over.
			 */
			memset(&seg_inode, 0, sizeof (seg_inode));
			for (i = nseg; i < maxseg; i++) {
				writecheck(&seg_inode,
				    sizeof (struct sam_perm_inode), 13525);
			}
		}
		/* the segment has damaged segments */
		if (!quiet && (nstale||ndam)) {
			if (nstale) {
				error(0, 0, catgets(catfd, SET, 13522,
				    "Segmented file has %d of %d stale "
				    "archive copies."),
				    nstale, nseg);
			} else {
				error(0, 0, catgets(catfd, SET, 13523,
				    "Segmented file has %d of %d without "
				    "archive copies."),
				    ndam, nseg);
			}
			error(0, 0, catgets(catfd, SET, 13515,
			    "%s: File data will not be recoverable "
			    "(file will be marked damaged)."),
			    name);
		}
	}
	return (0);
}


int
csd_write_csdheader(int fd, csd_hdrx_t *buf)
{
	int		io_sz;

	io_sz = sizeof (csd_hdrx_t);
	if (buffered_write(fd, (void *)buf, io_sz) != io_sz) {
		return (-1);
	} else {
		return (0);
	}
}

/* the following code was stolen from src/archiver/arcopy/header.c */
/*
 * header.c - Build tar header record.
 *
 */

/*
 * Write a tar header record.
 */
void
WriteHeader(
	char *name,		/* File name. */
	struct sam_stat *st,	/* File status. */
	char type)		/* Header type. */
{
	struct header *hdr;
	int n, sum;
	uchar_t *p;
	uint64_t dsize;

	SamMalloc(hdr, TAR_RECORDSIZE);
	memset(hdr, 0, TAR_RECORDSIZE);

	/* Remove leading '/' from file name. */
	while (*name == '/') {
		name++;
	}
	strncpy(hdr->arch_name, name, sizeof (hdr->arch_name)-1);
	ll2oct((u_longlong_t)st->st_mode, hdr->mode, sizeof (hdr->mode));
	ll2oct((u_longlong_t)st->st_uid, hdr->uid, sizeof (hdr->uid));
	ll2oct((u_longlong_t)st->st_gid, hdr->gid, sizeof (hdr->gid));
	if (type == LF_PARTIAL) {
		dsize = MIN(st->st_size, st->partial_size*SAM_DEV_BSIZE);
	} else {
		dsize = st->st_size;
	}
	ll2str(dsize, hdr->size, sizeof (hdr->size)+1);
	if (debugging) {
		fprintf(stderr, "Header file size %s <%lld>\n", hdr->size,
		    st->st_size);
	}
	ll2oct((u_longlong_t)st->st_mtime, hdr->mtime, sizeof (hdr->mtime)+1);
#ifdef GNUTAR
	ll2oct((u_longlong_t)st->st_atime, hdr->atime, sizeof (hdr->atime)+1);
	ll2oct((u_longlong_t)st->st_ctime, hdr->ctime, sizeof (hdr->ctime)+1);
#endif
	hdr->linkflag = type;
	strcpy(hdr->magic, TMAGIC);	/* Mark as Unix Std */
	/*
	 * old: char magic[6]; char version[2];
	 * aml: char magic[8];
	 */
#ifdef __FALSE__
	strncpy(hdr->version, TVERSION, TVERSLEN);
#else
	hdr->magic[6] = hdr->magic[7] = '0';
#endif
	strncpy(hdr->uname, getuser(st->st_uid), sizeof (hdr->uname));
	strncpy(hdr->gname, getgroup(st->st_gid), sizeof (hdr->gname));

	/*
	 * Generate header checksum.
	 * During the checksumming, the checksum field contains all spaces.
	 */
	memset(hdr->chksum, ' ', sizeof (hdr->chksum));
	sum = 0;
	for (p = (uchar_t *)hdr, n = sizeof (*hdr); n > 0; n--) {
		sum += *p++;
	}

	/*
	 * Fill in the checksum field. It's formatted differently from
	 * the other fields: it has digits, a null, then a space.
	 */
	ll2oct((u_longlong_t)sum, hdr->chksum, sizeof (hdr->chksum));
	hdr->chksum[6] = '\0';
	/* write the tar header */
	writecheck(hdr, TAR_RECORDSIZE, 66666);
	SamFree(hdr);
}
