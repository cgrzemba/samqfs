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


#pragma ident "$Revision: 1.22 $"


#include <errno.h>
#include <sam/types.h>
#include <pub/rminfo.h>
#include <sam/fs/ino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include <sam/fs/dirent.h>
#include <sam/nl_samfs.h>
#include <sam/fs/bswap.h>
#include <sys/acl.h>

#include "mgmt/cmn_csd.h"
#include "src/fs/cmd/dump-restore/csd_defs.h"
#include "src/fs/cmd/dump-restore/old_resource.h"
#include "aml/tar_hdr.h"

/* function prototypes */
static longlong_t oct2ll(char *, int);
static longlong_t str2ll(char *, int);
static void common_read_and_decode_tar_header(gzFile gzf, csd_tar_t *hdr_info);

static char  tar_name[MAXPATHLEN+1];	/* used to read, not referenced */

/*
 * Error handling function.  Only causes a program exit on error if
 * CSD_FATAL_ERR is defined.
 */
void
csd_error(
	int status,			/* ARGSUSED */
	int errnum,			/* ARGSUSED */
	char *message,			/* ARGSUSED */
	...)
{
#ifdef CSD_FATAL_ERR
	va_list	args;

	va_start(args);
	error(status, errnum, message, args);
	va_end(args);
#endif

}

int
common_get_csd_header(
	gzFile		gzf,			/* IN  */
	boolean_t	*swapped,		/* OUT */
	boolean_t	*data_possible,		/* OUT */
	csd_hdrx_t	*csd_header		/* OUT */
)
{

	int		rval = 0;
	int		io_sz;
	int		read_size;
	csd_hdr_t	*csd_hdr;

	memset(csd_header, 0, sizeof (csd_hdrx_t));

	io_sz = gzread(gzf, csd_header, sizeof (csd_hdr_t));

	if (io_sz != sizeof (csd_hdr_t)) {
		return (-1);
	}

	csd_hdr = &csd_header->csd_header;

	if (csd_hdr->magic != CSD_MAGIC) {
		if (csd_hdr->magic != CSD_MAGIC_RE) {
			return (-1);
		} else {
			*swapped = TRUE;
			sam_bswap4(&csd_hdr->magic, 1);
			sam_bswap4(&csd_hdr->version, 1);
			sam_bswap4(&csd_hdr->time, 1);
		}
	}

	switch (csd_hdr->version) {

		case CSD_VERS_5:
			/* read remainder of extended header */
			io_sz = sizeof (csd_hdrx_t) - sizeof (csd_hdr_t);

			read_size = gzread(gzf,
			    (char *)((char *)(csd_header) +
			    sizeof (csd_hdr_t)), io_sz);

			if (*swapped) {
				sam_bswap4(&csd_header->csd_header_flags, 1);
				sam_bswap4(&csd_header->csd_header_magic, 1);
			}
			if ((read_size != io_sz) ||
			    csd_header->csd_header_magic != CSD_MAGIC) {
				return (-1);
			}
			if (csd_header->csd_header_flags & CSD_H_FILEDATA) {
				*data_possible = TRUE;
			} else {
				*data_possible = FALSE;
			}
			break;

		case CSD_VERS:
		case CSD_VERS_3:
		case CSD_VERS_2:
			*data_possible = FALSE;
			break;

		default:
			/* bad version */
			rval = -1;
			break;
	}

	return (rval);
}

/*
 * We read an int giving the number of characters in the pathname first;
 * then the pathname; then the base inode.
 */
int
common_csd_read(
	gzFile			gzf,
	char 			*name,
	int			namelen,
	boolean_t		swapped,
	struct sam_perm_inode	*perm_inode)
{
	size_t			num;

	if (namelen <= 0 || namelen > MAXPATHLEN) {
		csd_error(1, 0, catgets(catfd, SET, 737,
		    "Corrupt samfsdump file.  name length %d."), namelen);
	}
	num = gzread(gzf, name, namelen);
	if (num != namelen) {
		return (1);
	}
	name[namelen] = '\0';
	num = gzread(gzf, perm_inode, sizeof (struct sam_perm_inode));
	if (num != sizeof (struct sam_perm_inode)) {
		return (1);
	}

	if (swapped) {
		if (sam_byte_swap(sam_perm_inode_swap_descriptor, perm_inode,
		    sizeof (*perm_inode))) {
			csd_error(0, 0, catgets(catfd, SET, 13532,
			    "%s: inode byteswap error - skipping."), name);
			return (1);
		}
	}
	if (!(SAM_CHECK_INODE_VERSION(perm_inode->di.version))) {
		csd_error(0, 0, catgets(catfd, SET, 739,
		    "%s: inode version incorrect - skipping"),
		    name);
		return (1);
	}
	return (0);
}


/*
 * If removable media file, read resource record. This record has changed
 * in each CSD version.
 * If symlink, read the link name;
 * Then, if archive copies overflowed, read vsn sections.
 * If access control list, return count of entries and list of entries.
 */
void
common_csd_read_next(
	gzFile			gzf,
	boolean_t		swap,
	int			csd_version,
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section	**vsnpp,
	char			*link,
	void			*data,
	int			*n_aclp,
	aclent_t		**aclpp)
{
	int linklen;
	size_t num;

	if (S_ISREQ(perm_inode->di.mode)) {
		struct sam_resource_file *resource =
		    (struct sam_resource_file *)data;

		if (csd_version == CSD_VERS_2) {
			num = gzread(gzf, resource,
			    sizeof (sam_old_resource_file_t));
			if (num != sizeof (sam_old_resource_file_t)) {
				return;
			}

			if (swap) {
				if (sam_byte_swap(
				    sam_old_resource_file_swap_descriptor,
				    resource,
				    sizeof (sam_old_resource_file_t))) {
					csd_error(0, 0, catgets(catfd, SET,
					    13533,
					    "Resource file v%d byte swap "
					    "error."), 2);
				}
			}
			resource->n_vsns = 1;
			resource->cur_ord = 0;
			resource->section[0].position =
			    resource->resource.archive.rm_info.position;
			resource->section[0].offset =
			    resource->resource.archive.rm_info.file_offset;
			resource->section[0].length = perm_inode->di.rm.size;
			resource->resource.revision = SAM_RESOURCE_REVISION;
			perm_inode->di.psize.rmfile =
			    sizeof (sam_resource_file_t);
		} else if (csd_version == CSD_VERS_3) {
			struct sam_old_rminfo rminfo;

			num = gzread(gzf, &rminfo,
			    sizeof (struct sam_old_rminfo));
			if (num != sizeof (struct sam_old_rminfo)) {
				return;
			}
			if (swap) {
				if (sam_byte_swap(
				    sam_old_rminfo_swap_descriptor,
				    &rminfo,
				    sizeof (struct sam_old_rminfo))) {
					csd_error(0, 0, catgets(catfd, SET,
					    13533,
					    "Resource file v%d byte swap "
					    "error."), 3);
				}
			}
			/*
			 * Don't support this csd version for removable
			 * media files.
			 */
			resource->resource.revision = -1;
		} else {
			num = gzread(gzf, resource,
			    perm_inode->di.psize.rmfile);
			if (num != perm_inode->di.psize.rmfile) {
				return;
			}
			if (swap) {
				if (sam_byte_swap(
				    sam_resource_file_swap_descriptor,
				    resource, perm_inode->di.psize.rmfile)) {
					csd_error(0, 0, catgets(catfd, SET,
					    13533,
					    "Resource file v%d byte swap "
					    "error."), 4);
				}
			}
		}
	}

	if (S_ISLNK(perm_inode->di.mode)) {
		num = gzread(gzf, &linklen, sizeof (int));
		if (num != sizeof (int)) {
			return;
		}
		if (swap) {
			sam_bswap4(&linklen, 1);
		}
		if (linklen < 0 || linklen > MAXPATHLEN) {
			csd_error(1, 0,
			    catgets(catfd, SET, 740, "Corrupt samfsdump file. "
			    "symlink name length %d"),
			    linklen);
		}
		num = gzread(gzf, link, linklen);
		if (num != linklen) {
			return;
		}
		link[linklen] = '\0';
	}

	if (!S_ISLNK(perm_inode->di.mode)) {
		*vsnpp = NULL;
		common_csd_read_mve(gzf, csd_version, perm_inode, vsnpp, swap);

		*aclpp = NULL;
		if (perm_inode->di.status.b.acl) {
			num = gzread(gzf, n_aclp, sizeof (int));
			if (num != sizeof (int)) {
				return;
			}
			if (swap) {
				sam_bswap4(n_aclp, 1);
			}
			if (*n_aclp) {
				int 	acllen = *n_aclp * sizeof (sam_acl_t);
				*aclpp = malloc(acllen);
				num = gzread(gzf, *aclpp, acllen);
				if (num != acllen) {
					return;
				}
				if (swap) {
					int i;

					for (i = 0; i < *n_aclp; i++) {
						if (sam_byte_swap(
						    sam_acl_swap_descriptor,
						    &((*aclpp)[i]),
						    sizeof (sam_acl_t))) {
							csd_error(0, 0,
							    catgets(catfd, SET,
							    3534,
						"ACL byte swap error."));
						}
					}
				}
			}
		}
	}
}

void
common_csd_read_mve(
	gzFile gzf,
	int csd_version,
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section  **vsnpp,
	boolean_t	swapped)
{
	if (csd_version >= CSD_VERS) {
		int n_vsns;
		int copy;

		n_vsns = 0;
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (perm_inode->di.version >= SAM_INODE_VERS_2) {
				if ((perm_inode->di.ext_attrs & ext_mva) &&
				    (perm_inode->ar.image[copy].n_vsns > 1)) {
					n_vsns +=
					    perm_inode->ar.image[copy].n_vsns;
				}
			} else if (perm_inode->di.version == SAM_INODE_VERS_1) {
				sam_perm_inode_v1_t *perm_inode_v1 =
				    (sam_perm_inode_v1_t *)perm_inode;

				if (perm_inode_v1->aid[copy].ino != 0) {
					if (perm_inode->ar.image[copy].n_vsns
					    > 1) {
						n_vsns +=
						    perm_inode->
						    ar.image[copy].n_vsns;
					}
				}
			}
		}
		if (n_vsns) {
			*vsnpp =
			    malloc(n_vsns * sizeof (struct sam_vsn_section));
			gzread(gzf, *vsnpp, sizeof (*vsnpp));
			if (swapped) {
				int i;

				for (i = 0; i < n_vsns; i++) {
					if (sam_byte_swap(
					    sam_vsn_section_swap_descriptor,
					    &((*vsnpp)[i]),
					    sizeof (struct sam_vsn_section))) {
						csd_error(0, 0, catgets(catfd,
						    SET, 13535,
						    "VSN byte swap error."));
					}
				}
			}
		}

	}
}


int
common_csd_read_header(
	gzFile gzf,
	int csd_version,
	boolean_t swapped,
	csd_fhdr_t *hdr)
{
	int corrupt_header = 0;

	if (csd_version < CSD_VERS_5) {
		int	namelen;

		if (gzread(gzf, &namelen, sizeof (int)) == sizeof (int)) {
			hdr->magic = CSD_FMAGIC;
			hdr->flags = 0;
			hdr->namelen = (long)namelen;
			if (swapped) {
				sam_bswap4(&hdr->namelen, 1);
			}
		} else {
			return (-1);
		}
	} else {
		if (gzread(gzf, hdr, sizeof (csd_fhdr_t))
		    != sizeof (csd_fhdr_t)) {
			return (-1);
		}
		if (swapped) {
			if (sam_byte_swap(csd_filehdr_swap_descriptor,
			    hdr, sizeof (csd_fhdr_t))) {
				csd_error(0, 0, catgets(catfd, SET, 13536,
				    "file header byte swap error."));
				corrupt_header++;
			}
		}
		/* some minor validation */
		if (hdr->magic != CSD_FMAGIC) {
			corrupt_header++;
		} else if (hdr->flags & CSD_FH_PAD) {
			if (hdr->namelen) {
				common_skip_pad_data(gzf, hdr->namelen);
			}
			return (0);
		} else if (hdr->namelen <= 0) {
			corrupt_header++;
		}

		if (corrupt_header) {
			csd_error(1, 0, catgets(catfd, SET, 13516,
		    "Corrupt samfsdump file. File header error <0x%lx/%ld>"),
			    hdr->magic, hdr->namelen);
			return (-1);
		}
	}
	return (1);
}


u_longlong_t
common_write_embedded_file_data(gzFile gzf, char *name)
{
	csd_tar_t	header;
	u_longlong_t bytes_written = 0;

	common_read_and_decode_tar_header(gzf, &header);
	if (header.csdt_bytes > 0) {
		bytes_written = common_copy_file_data_fr_dump(
		    gzf, header.csdt_bytes, name);
	}
	return (bytes_written);
}

void
common_skip_embedded_file_data(gzFile gzf)
{
	csd_tar_t	header;

	common_read_and_decode_tar_header(gzf, &header);
	if (header.csdt_bytes > 0) {
		common_skip_file_data(gzf, header.csdt_bytes);
	}
}


static void
common_read_and_decode_tar_header(gzFile gzf, csd_tar_t *hdr_info)
{
	union header_blk tarhdrblk;
	struct header *tarhdr = (struct header *)&tarhdrblk;
	longlong_t field_value;
	int		linktype;
	int		name_set = 0;
	int		namelen;

	hdr_info->csdt_bytes = 0;
	for (;;) {
		if (((gzread(gzf, &tarhdrblk, sizeof (tarhdrblk))) !=
		    TAR_RECORDSIZE) ||
		    (memcmp(tarhdr->magic, CSDTMAGIC,
		    sizeof (CSDTMAGIC)-1) != 0)) {
			goto header_err;
		}
		linktype = tarhdr->linkflag;

		switch (linktype) {
		case LF_LONGNAME:	/* looks like a file containing name */
			field_value = oct2ll(tarhdr->size, 12);
			if (field_value > MAXPATHLEN || name_set != 0) {
				goto header_err;
			}

			namelen = (int)field_value;
			gzread(gzf, tar_name, namelen);
			tar_name[namelen] = '\0';
			hdr_info->csdt_name = tar_name;
			name_set++;
			break;

		case LF_NORMAL:
		case LF_PARTIAL:
		case LF_OLDNORMAL:
			hdr_info->csdt_bytes = str2ll(tarhdr->size, 12);
			/* currently not dealing with GNU extra header */
			hdr_info->csdt_sparse = NULL;
			if (name_set == 0) {
				strlcpy(tar_name, tarhdr->arch_name, NAMSIZ);
				hdr_info->csdt_name = tar_name;
				name_set++;
			}
			return;

		default:
			goto header_err;
		}

	}

header_err:
	/*	Some error in the header has occurred. */
	csd_error(1, 0, catgets(catfd, SET, 13517,
	    "Corrupt samfsdump file. File tar header error."));
	/* NOTREACHED */
}


static longlong_t
oct2ll(char *cp, int numch)
{
	longlong_t	result;
	int			tmp;

	result = 0;
	while (numch > 0) {
		tmp = *cp;
		cp++; numch--;
		if (!isspace(tmp)) {
			tmp -= (int)'0';
			if (tmp >= 0 && tmp < 8) {
				result = (result * 8) + tmp;
			} else {
				return (result);
			}
		} else {
			if (result != 0) {
				return (result);
			}
		}
	}
	return (result);
}


static longlong_t
str2ll(char *cp, int numch)
{
	longlong_t	result;
	int			digit;
	int			tmp;
	int			base = 8;

	if (*cp == 'x') {
		base = 16;
		cp++;
		numch--;
	}
	result = 0;
	while (numch > 0) {
		tmp = *cp;
		cp++; numch--;
		if (!isspace(tmp)) {
			digit = tmp - (int)'0';
			if (digit > base && base > 10) {
				if (tmp >= (int)'a' && tmp <= (int)'z')
					tmp -= (int)'a' - (int)'A';
				digit = tmp + 10 - (int)'A';
			}

			if (digit >= 0 && digit < base) {
				result = (result * (longlong_t)base) +
				    (longlong_t)digit;
			} else {
				return (result);
			}
		} else {
			if (result != 0) {
				return (result);
			}
		}
	}
	return (result);
}

/*
 * common_skip_file_data - skip past dumped file data.
 */
void
common_skip_file_data(
	gzFile gzf,
	u_longlong_t nbytes)
{
	(void) common_copy_file_data_fr_dump(gzf, nbytes, NULL);
}


/*
 * common_skip_pad_data - skip past a dumped pad space.
 */
void
common_skip_pad_data(
	gzFile gzf,
	long nbytes)
{
	(void) gzseek(gzf, nbytes, SEEK_CUR);
}

/*
 * common_copy_file_data_fr_dump - Copy file data from a CSD dump file
 * from the open file. The file data has been filled
 * to the next TAR_RECORDSIZE (512) boundary, if needed.
 */
u_longlong_t
common_copy_file_data_fr_dump(
	gzFile		gzf,
	u_longlong_t	nbyte,
	char		*name)
{
	size_t		wr_size;
	ssize_t		wr_done;
	uint64_t	wr_total = 0;
	int64_t		to_block;
	int		pad_not_checked = 1;
	int		available;
	int		writing_data = 1;
	off64_t		curoff;
	int		wrfd = -1;

	int		bio_buffer_l = CSD_DEFAULT_BUFSZ;
	char		bio_buffer[CSD_DEFAULT_BUFSZ];
	int		bio_buffer_offset = bio_buffer_l;
	int		bio_buffer_end = bio_buffer_l;

	if (name == NULL) {
		writing_data = 0;
	} else {
		wrfd = open64(name, O_RDWR, 0600);
	}

	/* ensure we're working on 512 byte boundaries. */
	curoff = gztell(gzf);
	to_block = TAR_RECORDSIZE - (curoff % TAR_RECORDSIZE);

	for (;;) {
		available = bio_buffer_end - bio_buffer_offset;
		if (available == 0) {
			/* fill the buffer, so we have something */
			errno = 0;
			if (nbyte == 0) {
				break;
			}
			if (nbyte < bio_buffer_l) {
				bio_buffer_end = gzread(gzf, bio_buffer, nbyte);
			} else {
				bio_buffer_end = gzread(gzf, bio_buffer,
				    bio_buffer_l);
			}
			bio_buffer_offset = 0;
			if (bio_buffer_end <= 0) {
				csd_error(1, errno, catgets(catfd, SET, 13503,
				    "Error reading from samfsdump file %s"),
				    "during file data read");

				if (bio_buffer_end == 0) {
					return (0);
				} else {
					return (1);
				}
			}
			available = bio_buffer_end;
		}
		if (pad_not_checked) {
			if (nbyte <= (u_longlong_t)to_block) {
				/* data in space left */
				errno = 0;
				wr_size = (size_t)nbyte;
				if (writing_data) {
					if ((wr_done = write(wrfd,
					    &bio_buffer[bio_buffer_offset],
					    wr_size)) != wr_size) {
						csd_error(0, errno,
						    catgets(catfd, SET, 13505,
					    "Error writing to data file %s"),
						    name);
						return (wr_total);
					}
				} else {
					wr_done = wr_size;
				}
				nbyte = 0;
				wr_total = wr_done;
				to_block -= wr_done;
				bio_buffer_offset += wr_done;
			}
			/* skip pad to boundary */
			bio_buffer_offset += to_block;
			pad_not_checked = 0;
			continue;
		}
		wr_size = (size_t)(((u_longlong_t)available > nbyte) ?
		    nbyte : available);
		if (writing_data) {
			errno = 0;
			if ((wr_done = write(wrfd,
			    &bio_buffer[bio_buffer_offset], wr_size))
			    != wr_size) {

				csd_error(0, errno, catgets(catfd, SET, 13505,
				    "Error writing to data file %s"), name);
				return (wr_total);
			}
		} else {
			wr_done = wr_size;
		}
		nbyte -= wr_done;
		wr_total += wr_done;
		bio_buffer_offset += wr_done;

		/* sort out the trailing pad data */
		if (nbyte == 0) {
			curoff = gztell(gzf);
			to_block = TAR_RECORDSIZE - (curoff % TAR_RECORDSIZE);
			/*
			 * to_block can be exactly 512, in which case we
			 * don't need to read any further.
			 */
			if (to_block < 512) {
				common_skip_pad_data(gzf, to_block);
			}
			break;
		}
	}
	return (wr_total);
}
