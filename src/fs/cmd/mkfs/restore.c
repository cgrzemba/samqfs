/*
 *  restore.c - Restore a SAM-FS file system from a .inodes file.
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

#pragma ident "$Revision: 1.35 $"


/* ----- Include Files ---- */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/param.h>
#include <sys/buf.h>

#include <sam/types.h>
#include <sam/lib.h>
#include <sam/param.h>
#include <pub/devstat.h>
#include <sam/mount.h>
#include "sam/nl_samfs.h"

#include "sblk.h"
#include "ino.h"
#include "inode.h"
#include "mount.h"
#include "utility.h"


/* ----- Global tables & pointers */

int cnt_dir_offline = 0;	/* Count of directories set offline */
int cnt_fi_offline = 0;		/* Count of files set offline */
int cnt_dir_damaged = 0;	/* Count of directories set damaged */
int cnt_fi_damaged = 0;		/* Count of files set damaged */

static int process_inodes(int fd_inode, char *inode_file, FILE *log_st);
static void clear_inode(struct sam_disk_inode *inode_ino,
	struct sam_perm_inode *pid, FILE *log_st);
static void free_ino(struct sam_disk_inode *inode_ino,
	struct sam_perm_inode *pid);
static void print_log(FILE *logst, struct sam_perm_inode *pid, mode_t mode,
	char *status);


/*
 * ----- restore_cs - Restore control structures from .inodes file.
 *
 * Read .inodes and install this with all directories/files marked offline,
 * extents cleared. If no archive copy, mark damaged.
 */

void
restore_cs(int fd_inode, char *inode_file, FILE *log_st)
{
	if ((process_inodes(fd_inode, inode_file, log_st)) == 0) {
		printf(catgets(catfd, SET, 318,
		    ".inodes file (%s) restored\n"), inode_file);
		printf(catgets(catfd, SET, 910,
		    "  directories marked offline: %d\n"), cnt_dir_offline);
		printf(catgets(catfd, SET, 1176,
		    "        files marked offline: %d\n"), cnt_fi_offline);
		if (cnt_dir_damaged) {
			printf(catgets(catfd, SET, 909,
			    "  directories marked damaged: %d\n"),
			    cnt_dir_damaged);
		}
		if (cnt_fi_damaged) {
			printf(catgets(catfd, SET, 1175,
			    "        files marked damaged: %d\n"),
			    cnt_fi_damaged);
		}
	} else {
		error(1, 0, catgets(catfd, SET, 1070,
		    "Error restoring .inodes file"), 0);
	}
	if (log_st != NULL)  (void)fclose(log_st);
}

static int		/* ERRNO if error, 0 if successful */
process_inodes(int fd_inode, char *inode_file, FILE *log_st)
{
	int i;
	int bio_buffer_size;
	union sam_di_ino *idp;
	uint32_t *bnp, bn;
	uint32_t sbn;
	int ord;
	int sord;
	uchar_t *eip;
	sam_indirect_extent_t *iep;
	int de;
	int dt;
	int bt;
	int ileft;
	int kptr[3];
	offset_t offset;
	offset_t inode_size;
	int err;
	sam_bn_t first_bn;
	struct sam_disk_inode *dp;
	struct sam_disk_inode *inode_ino;
	sam_ino_t ino;


	dt = (mm_count != 0) ? MM : DD;
	bio_buffer_size = LG_BLK(mp, dt);
	offset = 0;
	memset((char *)ibufp, 0, bio_buffer_size);
	inode_ino = (struct sam_disk_inode *)first_sm_blk;
	bn = inode_ino->extent[0];		/* .inodes first extent */
	ord = inode_ino->extent_ord[0];	/* .inodes first ord */
	first_bn = 0;
	sbn = 0;
	sord = -1;
	ino = 0;
	while ((err = read(fd_inode, bio_buffer, bio_buffer_size)) > 0) {
		if ((err % INO_BLK_SIZE) != 0)
			break;			/* if ill-sized read */
		if (first_bn == 0) {	/* First block of .inodes file */
			union sam_di_ino *sdp;

			sdp = (union sam_di_ino *)first_sm_blk;
			idp = (union sam_di_ino *)bio_buffer;
			for (i = 1; i <= SAM_MAX_MKFS_INO; i++, sdp++, idp++) {
				/*
				 * Put initial settings for all inodes
				 * except root & lost+found.
				 */
				dp = (struct sam_disk_inode *)idp;
				if (SAM_PRIVILEGE_INO(dp->version, i)) {
					if (dp->id.ino != i ||
					    dp->id.gen != i) {
						error(1, 0,
						    catgets(catfd, SET, 96,
						    "%s is not a .inodes "
						    "file"),
						    inode_file);
					}
				}
				if (i == SAM_INO_INO) {
					inode_size = dp->rm.size;
					inode_ino->free_ino = dp->free_ino;
				}
				if (i == SAM_ROOT_INO ||
				    i == SAM_LOSTFOUND_INO)  {
					if (S_ISDIR(sdp->inode.di.mode)) {
						dp->extent[0] =
						    sdp->inode.di.extent[0];
						dp->extent_ord[0] =
						    sdp->inode.di.extent_ord[
						    0];
						dp->status.b.meta =
						    sdp->inode.di.status.b.meta;
						dp->blocks = LG_BLOCK(mp, dt);
					}
					if (i == SAM_ROOT_INO) {
						/*
						 * Verify that an offline
						 * copy exists
						 */
						if (sdp->inode.di.arch_status
						    == 0) {
							/*
							 * If root directory
							 * not archived
							 */
							error(1, 0,
							    catgets(catfd,
							    SET, 13442,
							"Root directory has "
							"no archive copy, "
							"%s -r fails"),
							    program_name);
						}
					}
				} else {
					memcpy((char *)idp, (char *)sdp,
					    sizeof (union sam_di_ino));
				}
			}
		}
		idp = (union sam_di_ino *)bio_buffer;
		for (i = 0; i < (LG_BLK(mp, dt) >> SAM_ISHIFT); i++, idp++) {
			ino++;
			clear_inode(inode_ino, (struct sam_perm_inode *)idp,
			    log_st);
		}
		if (d_write(&devp->device[ord], (char *)bio_buffer,
		    LG_DEV_BLOCK(mp, dt), bn)) {
			error(1, 0, catgets(catfd, SET, 109,
			    "%s write failed on (%s)"),
			    inode_file,
			    devp->device[ord].eq_name);
		}
		if (first_bn == 0) {
			sam_ino_t freeino = inode_ino->free_ino;

			/* save first block so .inodes can be restored */
			first_bn = bn;
			memcpy(first_sm_blk, bio_buffer, SM_BLK(mp, dt));
			inode_ino->free_ino = freeino;
		}
		offset += LG_BLK(mp, dt);
		if (offset >= inode_size) {
			err = 0;
			break;
		}
		sam_cmd_get_extent(inode_ino, offset, SAMFS_SBLKV1,
		    &de, &dt,  &bt, kptr, &ileft);
		bnp = &inode_ino->extent[de];
		eip = &inode_ino->extent_ord[de];
		if (de >= NDEXT) {
			int ii, kk;

			ii = de - NDEXT;
			for (kk = 0; kk <= ii; kk++) {
				if (*bnp == 0) {
					if ((bn = getdau(ord, 1, 0, 1)) ==
					    (sam_offset_t)-1) {
						error(1, 0,
						    catgets(catfd, SET, 89,
						    "%s allocation failed "
						    "on (%s)"),
						    inode_file,
						    devp->device[ord].eq_name);
					}
					*bnp = bn;
					*eip = ord;
					if (sbn) {
						if (d_write(&devp->device[
						    sord],
						    (char *)ibufp,
						    LG_DEV_BLOCK(mp, dt),
						    sbn)) {
							error(1, 0,
							    catgets(catfd,
							    SET, 109,
							    "%s write failed "
							    "on (%s)"),
							    inode_file,
							    devp->device[
							    sord].eq_name);
						}
					}
					sbn = bn;
					sord = ord;
					memset((char *)ibufp, 0,
					    LG_BLK(mp, dt));
					iep = (sam_indirect_extent_t *)ibufp;
					iep->id = inode_ino->id;
					iep->ieno = (ii - kk);
					iep->time = fstime;
				} else {
					if (sbn != *bnp) {
						if (d_write(&devp->device[
						    sord], (char *)ibufp,
						    LG_DEV_BLOCK(mp, dt),
						    sbn)) {
							error(1, 0,
							    catgets(catfd,
							    SET, 109,
							    "%s write failed "
							    "on (%s)"),
							    inode_file,
							    devp->device[
							    sord].eq_name);
						}
						sbn = *bnp;
						sord = (int)*eip;
						if (d_read(&devp->device[
						    sord], (char *)ibufp,
						    LG_DEV_BLOCK(mp, dt),
						    sbn)) {
							error(1, 0,
							    catgets(catfd,
							    SET, 103,
							    "%s read failed "
							    "on (%s)"),
							    inode_file,
							    devp->device[
							    ord].eq_name);
						}

					}
					iep = (sam_indirect_extent_t *)ibufp;
					if ((iep->id.ino !=
					    inode_ino->id.ino) ||
					    (iep->id.gen !=
					    inode_ino->id.gen) ||
					    (iep->ieno != (ii - kk))) {
						error(1, ENOCSI,
						    catgets(catfd, SET, 1039,
						    "Error in indirect "
						    "blocks of .inodes (%s) "
						    "file"),
						    inode_file);
					}
				}
				bnp = &iep->extent[kptr[kk]];
				eip = &iep->extent_ord[kptr[kk]];
			}
		}
		if (*bnp == 0) {
			if ((bn = getdau(ord, 1, 0, 1)) == (sam_offset_t)-1) {
				error(1, 0,
				    catgets(catfd, SET, 89,
				    "%s allocation failed on (%s)"),
				    inode_file,
				    devp->device[ord].eq_name);
			}
			*bnp = bn;
			*eip = ord;
			inode_ino->blocks += LG_BLOCK(mp, dt);
			inode_ino->rm.size += LG_BLK(mp, dt);
		} else if (de >= NDEXT) {
			error(1, EINVAL,
			    catgets(catfd, SET, 1040,
			    "Error in layout of .inodes (%s) file"),
			    inode_file);
		} else {
			bn = *bnp;
			ord = *eip;
		}
	}
	if (err != 0) {
		error(1, errno,
		    catgets(catfd, SET, 1064,
		    "Error reading .inodes (%s) file"),
		    inode_file);
	}
	if (sbn) {
		if (d_write(&devp->device[sord], (char *)ibufp,
		    LG_DEV_BLOCK(mp, dt), sbn)) {
			error(1, 0,
			    catgets(catfd, SET, 109,
			    "%s write failed on (%s)"),
			    inode_file,
			    devp->device[sord].eq_name);
		}
	}
	/* Write 1st .inodes large block in order to update SAM_INO_INO */
	memcpy((char *)dcp, (char *)first_sm_blk,
	    SAM_MAX_MKFS_INO << SAM_ISHIFT);
	if (d_write(&devp->device[ord], (char *)dcp, 1, first_bn)) {
		error(1, 0,
		    catgets(catfd, SET, 109, "%s write failed on (%s)"),
		    inode_file, devp->device[ord].eq_name);
	}
	return (0);
}


/*
 * ----- clear_inode - Clear the inode and mark it offline.
 */

static void
clear_inode(
	struct sam_disk_inode *inode_ino,	/* .inode Inode entry */
	struct sam_perm_inode *pid,		/* Perm Inode entry */
	FILE *log_st)
{
	int i, istart;
	sam_ino_t ino;
	uint_t *arp;

	ino = pid->di.id.ino;
	if (pid->di.mode == 0) {
		return;
	}
	istart = 0;
	if (pid->di.id.ino <= SAM_MAXIMUM_INO) {
		if (pid->di.id.ino == SAM_ROOT_INO ||
		    pid->di.id.ino == SAM_LOSTFOUND_INO)  {
			if (S_ISDIR(pid->di.mode)) {
				istart = 1;
			}
		} else {
			return;
		}
	} else {
		pid->di.blocks = 0;
	}
	for (i = istart; i < NOEXT; i++) {
		pid->di.extent[i] = 0;
		pid->di.extent_ord[i] = 0;
	}
	if (S_ISDIR(pid->di.mode) ||
	    S_ISLNK(pid->di.mode) ||
	    S_ISREQ(pid->di.mode) ||
	    S_ISSEGI(&pid->di)) {
		if (mm_count) {
			pid->di.status.b.meta = 1;
		} else {
			pid->di.status.b.meta = 0;
		}
	}
	/* Add check for inode version acceptable to superblock version. */
	if (!(S_ISREG(pid->di.mode) ||
	    S_ISDIR(pid->di.mode) ||
	    S_ISLNK(pid->di.mode) ||
	    S_ISREQ(pid->di.mode) || S_ISFIFO(pid->di.mode)) ||
	    (pid->di.id.ino != ino) ||
	    (!(SAM_CHECK_INODE_VERSION(pid->di.version)))) {
		return;
	}
	if (pid->di.parent_id.ino == SAM_ARCH_INO ||
	    pid->di.parent_id.ino == SAM_STAGE_INO) {
		free_ino(inode_ino, pid);
	}
	if (S_ISDIR(pid->di.mode)) {	/* If directory */
		int copy;

		/*
		 * Remove stale flag for directories and set archive flag.
		 */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if ((pid->di.ar_flags[copy] & AR_stale)) {
				pid->di.ar_flags[copy] &= ~(AR_stale);
				if (pid->ar.image[copy].vsn[0] != '\0') {
					pid->di.arch_status |= (1<<copy);
				}
			}
		}
		if (!pid->di.status.b.offline) { /* If not already offline */
			if (pid->di.arch_status) {	/* If archived */
				pid->di.status.b.offline = 1;
				pid->di.residence_time = fstime;
				/* Cnt of dir. set offline */
				cnt_dir_offline++;
				print_log(log_st, pid, pid->di.mode, "online");
			} else if (pid->di.rm.size) {
				/* No archive copies */
				pid->di.status.b.damaged = 1;
				/* Count of dir. set damaged */
				cnt_dir_damaged++;
			}
		}
	} else {
		if (!pid->di.status.b.offline) { /* If not already offline */
			if (pid->di.arch_status) {	/* If archived */
				pid->di.status.b.offline = 1;
				pid->di.residence_time = fstime;
				/* Count of files set offline */
				cnt_fi_offline++;
				print_log(log_st, pid, pid->di.mode, "online");
			} else if (pid->di.rm.size) {
				/* No archive copies */
				pid->di.status.b.damaged = 1;
				/* Count of files set damaged */
				cnt_fi_damaged++;
			}
		} else {
			if (pid->di.status.b.pextents) {
				print_log(log_st, pid, pid->di.mode,
				    "partial");
			}
		}
	}
	pid->di.status.b.pextents = 0;		/* No partial extents */
	/* If bigger than small extents, use only large extents. */
	if (S_ISREG(pid->di.mode) && pid->di.rm.size >= SM_OFF(mp, DD)) {
		pid->di.status.b.on_large = 1;
	}

	/* Remove rearch flag. These stale entries should not be rearchived */
	arp = (uint_t *)&pid->di.ar_flags[0];
	*arp &= ~((AR_rearch<<24)|(AR_rearch<<16)|(AR_rearch<<8)|AR_rearch);

	/* If inode has no archive copies, file is marked damaged */
	if (pid->di.arch_status == 0 && pid->di.rm.size) {
		/* No archive copies, file is damaged */
		pid->di.status.b.damaged = 1;
		/*
		 * Mark offline so if stale copy exists, the
		 * file/dir can be staged.
		 */
		pid->di.status.b.offline = 1;
	}
}


/*
 * ----- free_ino - free inode
 *
 *  Clear the inode and put it in the free list.
 */

static void
free_ino(
	struct sam_disk_inode *inode_ino,	/* .inode Inode entry */
	struct sam_perm_inode *pid)		/* Perm Inode entry */
{
	sam_ino_t freeino = inode_ino->free_ino;
	sam_ino_t ino = pid->di.id.ino;
	int gen = pid->di.id.gen;

	memset((char *)pid, 0, sizeof (union sam_di_ino));
	pid->di.id.ino = ino;
	pid->di.id.gen = ++gen;
	pid->di.free_ino = freeino;
	inode_ino->free_ino = ino;
}


/*
 * ----- print_log -
 *
 *  Print line for online directory/file.
 */

static void
print_log(FILE *log_st, struct sam_perm_inode *pid, mode_t mode,
	char *status)
{
	int copy;
	char type;

	if (log_st == NULL) {
		return;
	}
	if (S_ISDIR(pid->di.mode))  type = 'd';
	else if (S_ISSEGI(&pid->di))	 type = 'I';
	else if (S_ISSEGS(&pid->di))	 type = 'S';
	else if (S_ISREQ(pid->di.mode))  type = 'R';
	else if (S_ISLNK(pid->di.mode))  type = 'l';
	else if (S_ISREG(pid->di.mode))  type = 'f';
	else if (S_ISBLK(pid->di.mode))  type = 'b';
	else type = '?';
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		uint_t offset;

		if (pid->ar.image[copy].vsn[0] != '\0') {
			offset = pid->ar.image[copy].file_offset;
			if ((pid->ar.image[copy].arch_flags &
			    SAR_size_block) == 0) {
				offset >>= 9;
			}
			fprintf(log_st, "%c %s.%s %x.%x %s %d\n",
			    type,
			    sam_mediatoa(pid->di.media[copy]),
			    pid->ar.image[copy].vsn,
			    pid->ar.image[copy].position,
			    offset,
			    status,
			    pid->di.id.ino);
			break;
		}
	}
}
