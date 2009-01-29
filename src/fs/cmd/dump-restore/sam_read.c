/*
 *	sam_read.c
 *
 *	Work routine for csd restore.
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


#pragma ident "$Revision: 1.13 $"


#include <sam/types.h>
#include <pub/rminfo.h>
#include <sam/fs/ino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sam/lib.h>
#include <sam/sam_malloc.h>
#include <sam/fs/dirent.h>
#include <sam/nl_samfs.h>
#include <sam/fs/bswap.h>
#include "csd_defs.h"
#include "old_resource.h"
#include <sys/acl.h>
#include <aml/tar_hdr.h>

static char	tar_name[MAXPATHLEN+1];	/* name from tar header (if any) */

static longlong_t oct2ll(char *, int);
static longlong_t str2ll(char *, int);
static void read_and_decode_tar_header(csd_tar_t *);

static char *_SrcFile = __FILE__;


/*
 * We read an int giving the number of characters in the pathname first;
 * then the pathname; then the base inode.
 */
void
csd_read(
	char	*name,
	int	namelen,
	struct sam_perm_inode *perm_inode)
{
	dont_process_this_entry = 0;
	if (namelen <= 0 || namelen > MAXPATHLEN) {
		error(1, 0, catgets(catfd, SET, 737,
		    "Corrupt samfsdump file.  name length %d."), namelen);
	}
	readcheck(name, namelen, 5001);
	name[namelen] = '\0';
	readcheck(perm_inode, sizeof (*perm_inode), 5002);
	if (swapped) {
		if (sam_byte_swap(sam_perm_inode_swap_descriptor,
		    perm_inode, sizeof (*perm_inode))) {
			error(0, 0, catgets(catfd, SET, 13532,
			    "%s: inode byteswap error - skipping."), name);
			dont_process_this_entry = 1;
		}
	}
	if (!(SAM_CHECK_INODE_VERSION(perm_inode->di.version))) {
		if (debugging) {
			fprintf(stderr,
			    "csd_read:  inode version %d\n",
			    perm_inode->di.version);
		}
		error(0, 0, catgets(catfd, SET, 739,
		    "%s: inode version incorrect - skipping"),
		    name);
		dont_process_this_entry = 1;
	}

	/*
	 * If it is not a WORM file, zero the di2 area of the disk inode.
	 * This is only needed for dumps taken in versions CSD_VERS_5 and
	 * lower.
	 */
	if ((dont_process_this_entry) || (csd_version >= CSD_VERS_6)) {
		return;
	}
	if (perm_inode->di.version >= SAM_INODE_VERS_2) {
		if (perm_inode->di.status.b.worm_rdonly) {
			return;
		}
		memset(&perm_inode->di2, 0, sizeof (sam_disk_inode_part2_t));
	}
}


/*
 * If removable media file, read resource record. This record has changed
 * in each CSD version.
 * If symlink, read the link name;
 * Then, if archive copies overflowed, read vsn sections.
 * If access control list, return count of entries and list of entries.
 */
void
csd_read_next(
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section	**vsnpp,
	char			*link,
	void			*data,
	int			*n_aclp,
	aclent_t		**aclpp)
{
	int linklen;

	if (S_ISREQ(perm_inode->di.mode)) {
		struct sam_resource_file *resource =
		    (struct sam_resource_file *)data;

		if (csd_version == CSD_VERS_2) {
			readcheck(resource, sizeof (sam_old_resource_file_t),
			    5003);
			if (swapped) {
				if (sam_byte_swap(
				    sam_old_resource_file_swap_descriptor,
				    resource,
				    sizeof (sam_old_resource_file_t))) {
					error(0, 0, catgets(catfd, SET, 13533,
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

			readcheck(&rminfo, sizeof (struct sam_old_rminfo),
			    5003);
			if (swapped) {
				if (sam_byte_swap(
				    sam_old_rminfo_swap_descriptor,
				    &rminfo,
				    sizeof (struct sam_old_rminfo))) {
					error(0, 0, catgets(catfd, SET, 13533,
						"Resource file v%d byte "
							    "swap error."), 3);
				}
			}
			/*
			 * Don't support this csd version for removable
			 * media files.
			 */
			resource->resource.revision = -1;
		} else {
			readcheck(resource, perm_inode->di.psize.rmfile, 5003);
			if (swapped) {
				if (sam_byte_swap(
				    sam_resource_file_swap_descriptor,
				    resource, perm_inode->di.psize.rmfile)) {
					error(0, 0, catgets(catfd, SET, 13533,
					    "Resource file v%d byte swap "
					    "error."), 4);
				}
			}
		}
	}

	if (S_ISLNK(perm_inode->di.mode)) {
		readcheck(&linklen, sizeof (int), 5004);
		if (swapped) {
			sam_bswap4(&linklen, 1);
		}
		if (linklen < 0 || linklen > MAXPATHLEN) {
			error(1, 0,
			    catgets(catfd, SET, 740,
			    "Corrupt samfsdump file.  symlink name length %d"),
			    linklen);
		}
		readcheck(link, linklen, 5005);
		link[linklen] = '\0';
	}

	if (!S_ISLNK(perm_inode->di.mode)) {
		*vsnpp = NULL;
		csd_read_mve(perm_inode, vsnpp);

		*aclpp = NULL;
		if (perm_inode->di.status.b.acl) {
			readcheck(n_aclp, sizeof (int), 5029);
			if (swapped) {
				sam_bswap4(n_aclp, 1);
			}
			if (*n_aclp) {
				SamMalloc(*aclpp, *n_aclp *
				    sizeof (sam_acl_t));
				readcheck(*aclpp, *n_aclp *
				    sizeof (sam_acl_t), 5030);
				if (swapped) {
					int i;

					for (i = 0; i < *n_aclp; i++) {
						if (sam_byte_swap(
						    sam_acl_swap_descriptor,
						    &((*aclpp)[i]),
						    sizeof (sam_acl_t))) {
							error(0, 0,
							    catgets(catfd,
							    SET, 13534,
							    "ACL byte swap "
							    "error."));
						}
					}
				}
			}
		}
	}
}

void
csd_read_mve(
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section  **vsnpp)
{
	int n_vsns;
	int copy;

	if (csd_version <= CSD_VERS_3) {
		return;
	}

	n_vsns = 0;
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (perm_inode->di.version >= SAM_INODE_VERS_2) {
			if (perm_inode->di.ext_attrs & ext_mva) {
				if (perm_inode->ar.image[copy].n_vsns > 1) {
					n_vsns +=
					    perm_inode->ar.image[copy].n_vsns;
					if (debugging) {
						printf(
						    "Read multi-volume "
						    "extension copy %d for "
						    "%d.%d\n",
						    (copy+1),
						    perm_inode->di.id.ino,
						    perm_inode->di.id.gen);
					}
				} else {
					if (debugging) {
						printf(
						    "Ignore multi-volume "
						    "extension copy %d for "
						    "inode %d.%d\n",
						    (copy+1),
						    perm_inode->di.id.ino,
						    perm_inode->di.id.gen);
					}
				}
			}
		} else if (perm_inode->di.version == SAM_INODE_VERS_1) {
			sam_perm_inode_v1_t *perm_inode_v1 =
			    (sam_perm_inode_v1_t *)perm_inode;

			if (perm_inode_v1->aid[copy].ino != 0) {
				if (perm_inode->ar.image[copy].n_vsns > 1) {
					n_vsns +=
					    perm_inode->ar.image[copy].n_vsns;
					if (debugging) {
						printf(
						    "Read multi-volume "
						    "extension copy %d "
						    "for %d.%d\n",
						    (copy+1),
						    perm_inode->di.id.ino,
						    perm_inode->di.id.gen);
					}
				} else {
				if (debugging) {
					printf(
					    "Ignore multi-volume extension "
					    "(%d.%d) copy %d for inode %d.%d\n",
					    perm_inode_v1->aid[copy].ino,
					    perm_inode_v1->aid[copy].gen,
					    (copy+1),
					    perm_inode->di.id.ino,
					    perm_inode->di.id.gen);
				}
				}
			}
		}
	}
	if (n_vsns) {
		SamMalloc(*vsnpp, n_vsns * sizeof (struct sam_vsn_section));
		readcheck(*vsnpp, n_vsns * sizeof (struct sam_vsn_section),
		    5006);
		if (swapped) {
			int i;

			for (i = 0; i < n_vsns; i++) {
				if (sam_byte_swap(
				    sam_vsn_section_swap_descriptor,
				    &((*vsnpp)[i]),
				    sizeof (struct sam_vsn_section))) {
					error(0, 0, catgets(catfd,
					    SET, 13535,
					    "VSN byte swap "
					    "error."));
				}
			}
		}
	}
}


int
csd_read_header(csd_fhdr_t *hdr)
{
	int corrupt_header = 0;

	if (csd_version <= CSD_VERS_4) {
		int	namelen;

		if (buffered_read(CSD_fd, &namelen, sizeof (int)) ==
		    sizeof (int)) {
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
		if (buffered_read(CSD_fd, hdr, sizeof (csd_fhdr_t)) !=
		    sizeof (csd_fhdr_t)) {
			return (-1);
		}
		if (swapped) {
			if (sam_byte_swap(csd_filehdr_swap_descriptor,
			    hdr, sizeof (csd_fhdr_t))) {
				error(0, 0, catgets(catfd, SET, 13536,
				    "file header byte swap error."));
				corrupt_header++;
			}
		}
		/* some minor validation */
		if (hdr->magic != CSD_FMAGIC) {
			corrupt_header++;
		} else if (hdr->flags & CSD_FH_PAD) {
			if (hdr->namelen) {
				skip_pad_data(hdr->namelen);
			}
			return (0);
		} else if (hdr->namelen <= 0) {
			corrupt_header++;
		}

		if (corrupt_header) {
			error(1, 0, catgets(catfd, SET, 13516,
			    "Corrupt samfsdump file. File header "
			    "error <0x%lx/%ld>"),
			    hdr->magic, hdr->namelen);
			return (-1);
		}
	}
	return (1);
}


u_longlong_t
write_embedded_file_data(int fd, char *name)
{
	csd_tar_t	header;
	u_longlong_t bytes_written = 0;

	read_and_decode_tar_header(&header);
	if (header.csdt_bytes > 0) {
		bytes_written = copy_file_data_fr_dump(fd,
		    header.csdt_bytes, name);
	}
	return (bytes_written);
}


void
skip_embedded_file_data()
{
	csd_tar_t	header;

	read_and_decode_tar_header(&header);
	if (header.csdt_bytes > 0) {
		skip_file_data(header.csdt_bytes);
	}
}


static void
read_and_decode_tar_header(csd_tar_t *hdr_info)
{
	union header_blk tarhdrblk;
	struct header *tarhdr = (struct header *)&tarhdrblk;
	longlong_t field_value;
	int		linktype;
	int		name_set = 0;
	int		namelen;

	hdr_info->csdt_bytes = 0;
	for (;;) {
		if (((buffered_read(CSD_fd, &tarhdrblk,
		    sizeof (tarhdrblk))) !=
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
			readcheck(tar_name, namelen, 5001);
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
				strncpy(tar_name, tarhdr->arch_name, NAMSIZ);
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
	error(1, 0, catgets(catfd, SET, 13517,
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
