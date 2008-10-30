/*
 *  utility.c - common functions for sammkfs and samfsck.
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

#pragma ident "$Revision: 1.69 $"

#include "sam/osversion.h"

/* ----- Include Files ---- */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <sys/vfstab.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/resource.h>

#include <sam/types.h>
#include <sam/mount.h>
#include <sam/lib.h>
#include <sam/param.h>
#include <sam/syscall.h>
#include "sam/nl_samfs.h"

#include "macros.h"
#include "sblk.h"
#include "ino.h"
#include "inode.h"
#include "block.h"
#include "utility.h"
#include "indirect.h"

typedef struct d_cache_entry {
	offset_t byte_addr;
	offset_t byte_len;
	struct devlist *dp;
	char *data;
} d_cache_entry;

static d_cache_entry *d_cache = NULL;
static int d_cache_count = 0;
static struct {
	int reads;
	int writes;
	int hits;
	int kills;
	int moves;
} d_cache_stats = {0, 0, 0, 0, 0};

extern char *getfullrawname();

static int write_obj_sblk(struct sam_sblk *sbp, struct devlist *dp, int ord);
static int write_blk_sblk(struct sam_sblk *sbp, struct devlist *dp, int ord);
static int check_osd_daus(char *fs_name, struct sam_mount_info *mp,
    int fs_count);

/*
 * -----  chk_devices
 *
 *	Check filesystem devices. Open devices and get size.
 *	FIXME: State of devices is not checked.
 */

int
chk_devices(
	char *fs_name,
	int oflags,
	struct sam_mount_info *mp)
{
	struct sam_fs_part *fsp;
	struct devlist *dp;
	int i;
	int fs_count, da_count, mm_count;
	struct rlimit rlimit;

	/*
	 * Get the mount parameters and devices from the filesystem.
	 * The filesystem daemon fsd must have already executed and
	 * set the mount parameters (SC_setmount).
	 */
	memset(mp, 0, sizeof (struct sam_mount_info));
	if (GetFsMountDefs(fs_name, mp) < 0) {
		error(0, 0, catgets(catfd, SET, 620,
		    "Filesystem \"%s\" not found."),
		    fs_name);
		return (1);
	}
	fs_count = mp->params.fs_count;
	da_count = fs_count - mp->params.mm_count;
	mm_count = 0;
	if (fs_count <= 0 || da_count <= 0) {
		error(0, 0, catgets(catfd, SET, 1771,
		    "No members in filesystem %s."),
		    fs_name);
		return (1);
	}
	if (getrlimit(RLIMIT_NOFILE, &rlimit) < 0) {
		error(0, 0, catgets(catfd, SET, 13407, "Cannot getrlimit."));
		return (1);
	}
	if ((rlimit.rlim_cur - 10) < fs_count) {
		rlimit.rlim_cur = fs_count + 10;
		if (rlimit.rlim_cur > rlimit.rlim_max) {
			error(0, 0,
			    catgets(catfd, SET, 13408,
			    "Cannot open more than %d files."),
			    rlimit.rlim_max);
			return (1);
		}
		if (setrlimit(RLIMIT_NOFILE, &rlimit) < 0) {
			error(0, 0,
			    catgets(catfd, SET, 13409,
			    "Cannot setrlimit:RLIMIT_NOFILE."));
			return (1);
		}
	}
	if ((devp = (struct d_list *)malloc(sizeof (struct devlist) *
	    fs_count)) == NULL) {
		error(0, 0, catgets(catfd, SET, 599, "Cannot malloc devp."));
		return (1);
	}
	fsp = &mp->part[0];
	dp = (struct devlist *)devp;
	for (i = 0; i < fs_count; i++, dp++, fsp++) {
		(void) memset(dp, 0, sizeof (struct devlist));
		strncpy(dp->eq_name, fsp->pt_name, sizeof (upath_t));
		dp->eq = fsp->pt_eq;
		dp->type = fsp->pt_type;
		dp->size = 0;
		dp->state = fsp->pt_state;
		dp->filemode = oflags;

		/*
		 * Open device to get size.
		 */
		if (is_osd_group(dp->type)) {
			dp->num_group = 0;
			if (check_mnttab(fsp->pt_name)) {
				error(0, 0, catgets(catfd, SET, 13422,
				    "device %s is mounted."), fsp->pt_name);
				return (1);
			}
			if (open_obj_device(fsp->pt_name, oflags,
			    &dp->oh) < 0) {
				error(0, 0, catgets(catfd, SET, 17263,
				    "Cannot open object device %s"),
				    fsp->pt_name);
				return (1);
			}
			if (get_object_fs_attributes(fs_name, dp->oh, i,
			    (char *)fsp, sizeof (fsp)) < 0) {
				error(0, 0, catgets(catfd, SET, 17264,
				    "Cannot get object device attributes %s"),
				    fsp->pt_name);
				return (1);
			}
		} else {
			dp->num_group = 1;
			if ((dp->fd = get_blk_device(fsp, oflags)) < 0) {
				return (1);
			}
		}
		dp->size = fsp->pt_size;

		if (dp->type == DT_META) {
			if (mm_count == 0) {
				if (i != 0) {
					error(0, 0, catgets(catfd, SET, 13404,
					    "Meta device must be "
					    "ordinal 0 on filesystem %s."),
					    fs_name);
					return (1);
				}
			}
			mm_count++;
		}
#ifdef	DEBUG
		if (verbose) {
			if (is_osd_group(dp->type)) {
				fprintf(stderr, "%s: size = %llx "
				    "512-byte blocks\n",
				    dp->eq_name, dp->size);
				fprintf(stderr, "\tsmall dau %lld, "
				    "large dau %lld\n", fsp->pt_sm_dau,
				    fsp->pt_lg_dau);
			} else {
				char *devrname;

				if ((devrname = getfullrawname(fsp->pt_name))) {
					fprintf(stderr,
					    "%s: raw %s, size = %llx "
					    "512-byte blocks\n",
					    dp->eq_name, devrname, dp->size);
					free(devrname);
				}
			}
		}
#endif
	}

	/*
	 * Set num_group for object pools and stripe groups.
	 * Verify sizes for striped groups are the same.
	 */
	dp = (struct devlist *)devp;
	for (i = 0; i < fs_count; i++) {
		if (is_osd_group(dp->type)) {
			int ii;
			struct devlist *ddp;

			ddp = (struct devlist *)devp;
			for (ii = 0; ii < fs_count; ii++) {
				if (is_osd_group(ddp->type) &&
				    (dp->type == ddp->type)) {
					dp->num_group++;
				}
				ddp++;
			}
		} else if (is_stripe_group(dp->type)) {
			int ii;
			struct devlist *ddp;

			if (dp->num_group == 0) {
				dp++;
				continue;
			}
			ddp = (struct devlist *)dp+1;
			for (ii = (i+1); ii < fs_count; ii++) {
				if (is_stripe_group(ddp->type) &&
				    (dp->type == ddp->type)) {
					if (dp->size != ddp->size) {
						error(0, 0, catgets(catfd,
						    SET, 13405,
"Striped group eq %d size differs from other members for filesystem %s."),
						    ddp->eq, fs_name);
						return (1);
					}
					dp->num_group++;
					ddp->num_group = 0;
				}
				ddp++;
			}
		}
		dp++;
	}
	if (mm_count != mp->params.mm_count) {
		error(0, 0, catgets(catfd, SET, 1771,
		    "No members in filesystem %s."),
		    fs_name);
		return (1);
	}

	/*
	 * Check that object pool DAU's are all the same.
	 */
	return (check_osd_daus(fs_name, mp, fs_count));
}


/*
 * -----  close_devices
 *
 *	Close file system devices.
 *	Nice for block devices.  Necessary for object devices.
 */

void
close_devices(
	struct sam_mount_info *mp)	/* Mount info struct */
{
	struct devlist *dp = (struct devlist *)devp;
	int i, r = 0, fs_count;

	fs_count = mp->params.fs_count;
	for (i = 0; i < fs_count; i++, dp++) {
		if (is_osd_group(dp->type) && (dp->oh != NULL)) {
			r = close_obj_device(dp->eq_name, dp->filemode, dp->oh);
		}
		if (!is_osd_group(dp->type) && (dp->fd > 0)) {
			r = close(dp->fd);
		}
		if (r < 0) {
			error(0, errno, catgets(catfd, SET, 17266,
			    "Cannot close device %s"), dp->eq_name);
		}
	}
}


/*
 * ----- fstab_fsname_get - Given an absolute mount point path name,
 * return the corresponding family set name from /etc/vfstab.
 */
int		/* return error code */
fstab_fsname_get(
	char *name,	/* search name */
	char *fsname)	/* returned family set name */
{
	FILE *fp;
	struct vfstab vbuf;

	if ((fsname == NULL) || (name == NULL) || (*name != '/')) {
		/* Path name must start with '/' */
		return (EINVAL);
	}

	*fsname = '\0';

	if ((fp = fopen(VFSTAB, "r")) == NULL) {
		return (errno);
	}

	while (getvfsent(fp, &vbuf) == 0) {
		if ((strcmp(vbuf.vfs_fstype, "samfs") == 0) &&
		    (strcmp(vbuf.vfs_mountp, name) == 0)) {
			/* Found mount point.  Return fset name. */
			strcpy(fsname, vbuf.vfs_special);
			break;
		}
	}

	fclose(fp);

	if (*fsname == '\0') {
		return (ENOENT);
	}

	return (0);
}


/*
 * ----- get_mem - Malloc memory.
 */

void
get_mem(int exit_status)
{
	int dt;
	int lg_blk;

	dt = DD;
	lg_blk = LG_BLK(mp, dt);
	if (lg_blk < (SAM_DEFAULT_META_DAU * SAM_DEV_BSIZE)) {
		lg_blk = (SAM_DEFAULT_META_DAU * SAM_DEV_BSIZE);
	}
	if ((dcp = (char *)malloc(lg_blk)) == NULL) {
		error(exit_status, 0, catgets(catfd, SET, 1606,
		    "malloc: %s."), "dcp");
	}
	if ((first_sm_blk = (char *)malloc(SM_BLK(mp, dt))) == NULL) {
		error(exit_status, 0,
		    catgets(catfd, SET, 1606, "malloc: %s."),
		    "first_sm_buf");
	}
	if ((dir_blk = (char *)malloc(DIR_BLK)) == NULL) {
		error(exit_status, 0, catgets(catfd, SET, 1606,
		    "malloc: %s."), "buf2");
	}
	if ((ibufp = (char *)malloc(lg_blk)) == NULL) {
		error(exit_status, 0, catgets(catfd, SET, 1606,
		    "malloc: %s."), "ibuf");
	}
	if ((daubuf = (char *)malloc(lg_blk)) == NULL) {
		error(exit_status, 0, catgets(catfd, SET, 1606,
		    "malloc: %s."), "daubuf");
	}
	if ((bio_buffer = (int *)malloc(lg_blk)) == NULL) {
		error(exit_status, 0, catgets(catfd, SET, 1606,
		    "malloc: %s."), "daubuf");
	}
}


/*
 * ---- cmd_clear_maps - Clear free disk blocks in maps.
 */

void
cmd_clear_maps(
	int caller,	/* Caller - SAMMKFS or SAMFSCK */
	int ord,	/* Disk ordinal to clear maps. */
	int len,	/* Number of allocation units */
	sam_daddr_t blk, /* Location of first block */
	int bits)	/* No of bits to clear -- mkfs=1, fsck=SM blocks */
{
	struct devlist *dp;
	uint_t *wptr;
	int nbits;
	int off;

	dp = &devp->device[ord];
	if (caller == SAMMKFS) {
		if (getdau(ord, len, blk, 1) < 0) {
			error(1, errno, catgets(catfd, SET, 13406,
			    "failed to get %d allocation units on eq %d"),
			    len, dp->eq);
		}
	} else {
		off = (blk * bits) >> NBBYSHIFT;
		off = off & SAM_LOG_WMASK;
		wptr = (uint_t *)(void *)(dp->mm + off);
		nbits = (blk * bits) & (32 - 1);
		len *= bits;
		for (blk = 0; blk < len; blk++) {
			*wptr = sam_clrbits(*wptr, (31 - nbits), 1);
			if (++nbits == 32) {
				nbits = 0;
				wptr++;
			}
		}
	}
}


/*
 * ---- getdau - Get free disk blocks.
 *
 *	Allocate num contiguous DAUs.
 */

sam_offset_t		/* disk addr of the starting block, -1 if failed. */
getdau(
	int ord,	/* Disk ordinal to allocate from. */
	int num,	/* Number of allocation units */
	sam_daddr_t blk, /* Location of first block */
	int set)	/* 1=write the maps, 0=find contiguous blocks */
{
	int maplen;
	int bit;
	int wd;
	uint_t *wptr, mask;
	int mmord;
	int space, count;
	sam_offset_t offset, off, pg;
	sam_daddr_t mapaddr;
	sam_offset_t first_block;
	sam_daddr_t prev_blk;

	first_block = -1;

	mapaddr = sblock.eq[ord].fs.allocmap;

	offset = blk >> NBBYSHIFT;
	pg = offset >> SAM_DEV_BSHIFT;
	off = offset & SAM_LOG_WMASK;

	maplen = sblock.eq[ord].fs.l_allocmap;
	mmord = sblock.eq[ord].fs.mm_ord;
	count = num;

	for (; pg < maplen; pg++) {
		if (set) {
			if ((dausector != (mapaddr + pg)) || (dauord != ord)) {
				writedau();
				dausector = (mapaddr + pg);
				dauord = mmord;
				if (d_read(&devp->device[mmord], daubuf, 1,
				    (mapaddr + pg))) {
					error(1, 0,
					    catgets(catfd, SET, 91,
					    "%s dau map read "
					    "failed on (%s)"),
					    devp->device[mmord].eq_name);
				}
			}
		} else {
			if (d_read(&devp->device[mmord], daubuf, 1,
			    (mapaddr + pg))) {
				error(1, 0,
				    catgets(catfd, SET, 91,
				    "%s dau map read failed on (%s)"),
				    devp->device[mmord].eq_name);
			}
		}
		wptr = (uint_t *)(void *)(daubuf + off);
		for (wd = 0; wd < (SAM_DEV_BSIZE / NBPW); wd++, wptr++) {
			if (!*wptr)  continue;
			/* Find first bit set from left to right */
			for (bit = 0, mask = 0x80000000; bit < 32;
			    bit++, mask >>= 1) {
				if (*wptr & mask)  {
					*wptr &= ~mask;
					blk = bit + (wd << 5) +
					    (pg * SAM_DEV_BSIZE * NBBY);
					if (first_block < 0) {
						prev_blk = blk;
						if (sblock.eq[ord].fs.type ==
						    DT_META) {
							blk *= LG_DEV_BLOCK(
							    mp, MM);
						} else {
							blk *= LG_DEV_BLOCK(
							    mp, DD);
						}
						first_block = blk;
					} else {
						if (++prev_blk != blk) {
							return (-1);
						}
					}
					if (--count) {
						continue;
					}
					if (set == 0) {
						return (first_block);
					}

		/* N.B. Bad indentation here to meet cstyle requirements */
			if (sblock.eq[ord].fs.type == DT_META) {
				space = sblock.info.sb.mm_blks[LG] * num;
				sblock.info.sb.mm_space -= space;
				sblock.eq[ord].fs.space -= space;
			} else {
				space = sblock.info.sb.dau_blks[LG] * num;
				if (sblock.eq[ord].fs.num_group > 1) {
					space *= sblock.eq[ord].fs.num_group;
				}
				sblock.info.sb.space -= space;
				sblock.eq[ord].fs.space -= space;
			}


					return (first_block);
				}
			}
		}
	}
	return (-1);
}


/*
 * ---- writedau - Write dau block
 *	Write dau block.
 */

void			/* The disk address of the starting block. */
writedau()
{
	if (dauord != -1) {
		if (d_write(&devp->device[dauord], daubuf, 1,
		    (int)dausector)) {
			error(0, 0,
			    catgets(catfd, SET, 799,
			    "Dau map write failed on (%s)"),
			    devp->device[dauord].eq_name);
			exit(1);
		}
	}
}


/*
 * ----- write_sblk - Write superblocks.
 */

int			/* 1 if error, 0 if successful */
write_sblk(
	struct sam_sblk *sbp,
	struct d_list *devp)
{
	int ord;
	struct devlist *dp;
	int err = 0;

	/* Write superblocks: */

	for (ord = 0, dp = (struct devlist *)devp; ord < sbp->info.sb.fs_count;
	    ord++, dp++) {
		sbp->info.sb.ord = ord;
		if (dp->state == DEV_OFF || dp->state == DEV_DOWN) {
			continue;
		}
		if (is_osd_group(dp->type)) {
			err = write_obj_sblk(sbp, dp, ord);
		} else {
			err = write_blk_sblk(sbp, dp, ord);
		}
		if (err) {
			error(0, 0, catgets(catfd, SET, 2440,
			    "Superblock write failed on %s on eq %d"),
			    sbp->info.sb.fs_name, dp->eq);
			return (1);
		}
	}
	printf(catgets(catfd, SET, 2526, "total %s kilobytes       = %lld\n"),
	    "data", sbp->info.sb.capacity);
	printf(catgets(catfd, SET, 2527, "total %s kilobytes free  = %lld\n"),
	    "data", sbp->info.sb.space);
	if (sbp->info.sb.mm_count) {
		printf(catgets(catfd, SET, 2526,
		    "total %s kilobytes       = %lld\n"),
		    "meta", sbp->info.sb.mm_capacity);
		printf(catgets(catfd, SET, 2527,
		    "total %s kilobytes free  = %lld\n"),
		    "meta", sbp->info.sb.mm_space);
	}
	return (0);
}


/*
 * ----- write_obj_sblk - Write object sblk
 */

static int			/* 1 if error, 0 if successful */
write_obj_sblk(
	struct sam_sblk *sbp,
	struct devlist *dp,
	int ord)
{
	int sblk_size;

	sblk_size = sizeof (struct sam_sblk);
	if ((create_object(sbp->info.sb.fs_name, dp->oh, ord,
	    SAM_OBJ_SBLK_INO))) {
		return (1);
	}

	if ((write_object(sbp->info.sb.fs_name, dp->oh, ord,
	    SAM_OBJ_SBLK_INO, (char *)sbp, 0, sblk_size))) {
		return (1);
	}
	return (0);
}


/*
 * ----- write_blk_sblk - Write block sblk
 */

static int			/* 1 if error, 0 if successful */
write_blk_sblk(
	struct sam_sblk *sbp,
	struct devlist *dp,
	int ord)
{
	int sblk_blocks;
	int ord_write_blks;

	sblk_blocks = (sizeof (struct sam_sblk)) / SAM_DEV_BSIZE;
	ord_write_blks = sblk_blocks;
	if (((int)sbp->info.sb.offset[0] + ord_write_blks) >
	    (sbp->eq[ord].fs.system)) {
		ord_write_blks = sbp->eq[ord].fs.system -
		    (int)sbp->info.sb.offset[0];
	}
	if (d_write(dp, (char *)sbp, ord_write_blks,
	    (int)sbp->info.sb.offset[0])) {
		return (1);
	}
	if (ord == 0) {
		if (d_write(dp, (char *)sbp, sblk_blocks,
		    (int)sbp->info.sb.offset[1])) {
			error(0, 0,
			    catgets(catfd, SET, 495,
			    "Backup superblock write failed "
			    "on %s"),
			    sbp->info.sb.fs_name);
			return (1);
		}
	}
	return (0);
}


/*
 * ----- sam_cmd_get_extent - Get offset extent location.
 *
 *  Compute where the offset is located in the extent array.
 *  Called from the filesystem commands.
 */

int				/* ERRNO if error, 0 if successful. */
sam_cmd_get_extent(
	struct sam_disk_inode *dp,
	offset_t offset,	/* Logical byte offset in file (longlong_t) */
	int sblk_ver,		/* Super block version */
	int *de,		/* Direct extent-- returned */
	int *dtype,		/* Type of device - Data or Meta */
	int *btype,		/* Small/Large extent-- returned */
	int kptr[],		/* Indirect extent locations-- returned */
	int *ileft)		/* Number of extents left-- returned */
{
	ino_st_t status;
	int ord;
	struct devlist *dip;
	struct sam_get_extent_args arg;
	struct sam_get_extent_rval rval;

	status.bits = dp->status.bits;
	ord = dp->extent_ord[0];
	dip = (struct devlist *)&devp->device[ord];
	arg.dau = &mp->mi.m_dau[0];
	arg.num_group = dip->num_group;
	arg.sblk_vers = sblk_ver;
	arg.status = status;
	arg.offset = offset;
	rval.de = de;
	rval.dtype = dtype;
	rval.btype = btype;
	rval.ileft = ileft;
	return (sam_get_extent(&arg, kptr, &rval));
}


/*
 * ---- d_cache_init - Initialize disk cache.
 */

void
d_cache_init(int entries)
{
	if (d_cache != NULL) {		/* ignore redundant calls */
		return;
	}

	d_cache = malloc(entries * sizeof (d_cache_entry));
	if (d_cache != NULL) {
		bzero(d_cache, entries * sizeof (d_cache_entry));
		d_cache_count = entries;
	}
}


/*
 * ---- d_cache_stats - Print disk cache statistics.
 */

void
d_cache_printstats(void)
{
	printf("Cache usage: %d reads, %d writes, %d hits, %d kills, "
	    "%d moves\n",
	    d_cache_stats.reads, d_cache_stats.writes, d_cache_stats.hits,
	    d_cache_stats.kills, d_cache_stats.moves);
}

/*
 * ---- d_cache_find - Locate a buffer in the disk cache.
 *		If found, rotate it to the head of the cache.
 */

static char *
d_cache_find(
	struct devlist *dp,	/* Pointer to devlist for device */
	offset_t byte_addr,
	offset_t byte_len)
{
	int i;
	d_cache_entry *e;
	char *d;

	d_cache_stats.reads++;
	for (i = 0, e = d_cache; i < d_cache_count; i++, e++) {
		if ((e->byte_addr == byte_addr) && (e->byte_len == byte_len) &&
		    (e->dp == dp)) {
			d = e->data;
			if (i != 0) {
				d_cache_entry t;

				t = *e;
				bcopy(&d_cache[0], &d_cache[1],
				    i*sizeof (d_cache_entry));
				d_cache[0] = t;
				d_cache_stats.moves += i;
			}
			d_cache_stats.hits++;
			return (d);
		}
	}

	return (NULL);
}


/*
 * ---- d_cache_add - Add a buffer to the disk cache.
 */

static void
d_cache_add(
	struct devlist *dp,	/* Pointer to devlist for device */
	offset_t byte_addr,
	offset_t byte_len,
	char *data)
{
	int i;
	d_cache_entry *e;
	char *d;

	for (i = 0, e = d_cache; i < d_cache_count; i++, e++) {
		if (e->dp == NULL) {
			d = malloc(byte_len);
			if (d == NULL) {
				return;
			}
			bcopy(data, d, byte_len);
			e->dp = dp;
			e->byte_addr = byte_addr;
			e->byte_len = byte_len;
			e->data = d;
			return;
		}
	}

	e = &d_cache[d_cache_count-1];

	if (e->byte_len == byte_len) {
		d = e->data;
	} else {
		free(e->data);
		d = malloc(byte_len);
		if (d == NULL) {
			e->dp = NULL;
			e->data = NULL;
			return;
		}
	}

	bcopy(data, d, byte_len);

	bcopy(&d_cache[0], &d_cache[1],
	    (d_cache_count-1)*sizeof (d_cache_entry));
	d_cache_stats.moves += (d_cache_count-1);

	e = &d_cache[0];

	e->dp = dp;
	e->byte_addr = byte_addr;
	e->byte_len = byte_len;
	e->data = d;
}


/*
 * ---- d_cache_kill - Invalidate buffer(s) in the disk cache.
 */

static void
d_cache_kill(
	struct devlist *dp,	/* Pointer to devlist for device */
	offset_t byte_addr,
	offset_t byte_len)
{
	int i;
	d_cache_entry *e;
	offset_t byte_end;

	d_cache_stats.writes++;

	byte_end = byte_addr + byte_len - 1;

	for (i = 0, e = d_cache; i < d_cache_count; i++, e++) {
		if (e->dp == dp) {
			offset_t e_addr, e_end;

			e_addr = e->byte_addr;
			e_end = e_addr + e->byte_len - 1;

			if ((e_addr > byte_end) || (e_end < byte_addr)) {
				continue;
			}

			free(e->data);

			e->dp = NULL;
			e->data = NULL;

			d_cache_stats.kills++;
		}
	}
}


/*
 * ---- d_read - Read disk.
 *
 *	Read from the specified disk.
 */

int				/* 1 if error, 0 if successful */
d_read(
	struct devlist *dp,	/* Pointer to devlist for device */
	char *buffer,		/* Address of buffer */
	int len,		/* Number of logical blocks */
	sam_daddr_t sector)	/* Logical sector address */
{
	int bytes;		/* Number of bytes transferred */
	offset_t byte_addr;
	char *cache;

	byte_addr = (offset_t)sector * SAM_DEV_BSIZE;
	bytes = len * SAM_DEV_BSIZE;

	if (d_cache) {
		cache = d_cache_find(dp, byte_addr, bytes);
		if (cache != NULL) {
			bcopy(cache, buffer, bytes);
			return (0);
		}
	}

	if (llseek(dp->fd, byte_addr, SEEK_SET) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 13028,
		    "Seek failed on eq %d at block 0x%llx"),
		    dp->eq, (sam_offset_t)sector);
		return (1);
	}
	if (read(dp->fd, buffer, bytes) != bytes) {
		error(0, errno,
		    catgets(catfd, SET, 13391,
		    "Read failed on eq %d at block 0x%llx, length = %d"),
		    dp->eq, (sam_offset_t)sector, len);
		return (1);
	}

	if (d_cache) {
		d_cache_add(dp, byte_addr, bytes, buffer);
	}

	return (0);
}

int				/* 1 if error, 0 if successful */
b_write(
	void *devp,		/* Pointer to devlist for device */
	char *buffer,		/* Address of buffer */
	int len,		/* Number of logical blocks */
	sam_daddr_t sector)	/* Logical sector addr, used only for msgs */
{
	struct devlist *devlp;	/* Pointer to devlist for device */

	devlp = (struct devlist *)devp;
	return (d_write(devlp, buffer, len, sector));
}


/*
 * ---- d_write - Write disk.
 *
 *	Write to the specified disk.
 */

int				/* 1 if error, 0 if successful */
d_write(
	struct devlist *dp,	/* Pointer to devlist for device */
	char *buffer,		/* Address of buffer */
	int len,		/* Number of logical blocks */
	sam_daddr_t sector)	/* Logical sector addr, used only for msgs */
{
	int bytes;		/* Number of bytes transferred */
	offset_t byte_addr;

	byte_addr = (offset_t)sector * SAM_DEV_BSIZE;
	bytes = len * SAM_DEV_BSIZE;

	if (d_cache) {
		d_cache_kill(dp, byte_addr, bytes);
	}

	if (llseek(dp->fd, byte_addr, SEEK_SET) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 13028,
		    "Seek failed on eq %d at block 0x%llx"),
		    dp->eq, (sam_offset_t)sector);
		return (1);
	}
	if (write(dp->fd, buffer, bytes) != bytes) {
		error(0, errno,
		    catgets(catfd, SET, 13029,
		    "Write failed on eq %d at block 0x%llx, length = %d"),
		    dp->eq, (sam_offset_t)sector, len);
		return (1);
	}
	return (0);
}


/*
 * ---- update_sblk_to_40
 *
 * Update a pre-4.0 superblock to contain required 4.0 fields
 */

void
update_sblk_to_40(
	struct sam_sblk *sbp,		/* Pointer to on-disk Super block */
	int fs_count,			/* Number of total partitions */
	int mm_count)			/* Number of data partitions */
{
	int ord;

	/*
	 * Backward compatibility prior to 3.5.1. Set meta blocks for LG.
	 */
	if (sbp->info.sb.mm_blks[LG] == 0) {
		if (sbp->info.sb.meta_blks) {
			sbp->info.sb.mm_blks[SM] = sbp->info.sb.meta_blks;
			sbp->info.sb.mm_blks[LG] = sbp->info.sb.meta_blks;
		} else {
			sbp->info.sb.mm_blks[SM] = sbp->info.sb.dau_blks[SM];
			sbp->info.sb.mm_blks[LG] = sbp->info.sb.dau_blks[LG];
		}
	}
	if (sbp->info.sb.ext_bshift == 0)
		sbp->info.sb.ext_bshift = SAM_DEV_BSHIFT;

	/*
	 * Backward compatibility prior to 3.3
	 */
	if (mm_count == 0) {
		sbp->info.sb.mm_blks[SM] = sbp->info.sb.dau_blks[SM];
		sbp->info.sb.mm_blks[LG] = sbp->info.sb.dau_blks[LG];
		sbp->info.sb.meta_blks = sbp->info.sb.dau_blks[LG];
		sbp->info.sb.da_count = (short)fs_count;
		for (ord = 0; ord < fs_count; ord++) {
			sbp->eq[ord].fs.mm_ord = (uchar_t)ord;
			sbp->eq[ord].fs.system = sbp->eq[ord].fs.allocmap +
			    sbp->eq[ord].fs.l_allocmap;
			if (ord == 0) {
				sbp->eq[ord].fs.system +=
				    sbp->info.sb.dau_blks[LG];
			}
		}
	}
}


/*
 * ----- check_osd_daus() - Check to see if all members of an osd
 * object pool have the same DAU.
 */
static int				/* nonzero if error, 0 if ok */
check_osd_daus(
	char	*fs_name,		/* File system name */
	struct sam_mount_info *mp,	/* mount parameter pointer */
	int	fs_count)		/* devlist count */
{
	struct sam_fs_part *fsp, *fspp;
	int i, j;
	int r = 0;

	/*
	 * Scan over all object devices.
	 */
	for (i = 0; i < fs_count; i++) {
		fsp = &mp->part[i];
		if (!is_osd_group(fsp->pt_type)) {
			continue;
		}
		/*
		 * Check all following devices of same object pool for DAU match
		 */
		for (j = i+1; j < fs_count; j++) {
			fspp = &mp->part[j];
			if (fsp->pt_type != fspp->pt_type) {
				continue;
			}
			if (fsp->pt_sm_dau != fspp->pt_sm_dau) {
				error(0, 0, catgets(catfd, SET, 13036, "%s: "
				    "Error eq %d and eq %d, same object pool, "
				    "but %s DAUs mismatch (%lld vs. %lld)\n"),
				    fs_name, fsp->pt_eq, fspp->pt_eq, "small",
				    fsp->pt_sm_dau, fspp->pt_sm_dau);
				r++;
			}
			if (fsp->pt_lg_dau != fspp->pt_lg_dau) {
				error(0, 0, catgets(catfd, SET, 13036, "%s: "
				    "Error eq %d and eq %d, same object pool, "
				    "but %s DAUs mismatch (%lld vs. %lld)\n"),
				    fs_name, fsp->pt_eq, fspp->pt_eq, "large",
				    fsp->pt_lg_dau, fspp->pt_lg_dau);
				r++;
			}
		}
	}
	return (r);
}
