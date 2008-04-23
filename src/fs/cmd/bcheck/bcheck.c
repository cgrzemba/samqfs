/*
 *  sambcheck.c - "check file system block usage" command
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

#pragma ident "$Revision: 1.38 $"

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Solaris headers. */
#include <syslog.h>

/* POSIX headers. */
#include <sys/types.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* SAM-FS headers. */
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/mount.h"
#include "pub/stat.h"
#include "sam/fs/macros.h"
#include "sam/fs/ino_ext.h"

/* Local headers. */
#include "inode.h"
#include "mount.h"
#define	MAIN
#include "utility.h"
#include "ino.h"


/*
 * Exit status:
 * 0	= no fatal problems encountered.
 * 20	= fatal: invalid indirect blocks.
 * 30	= fatal: I/O Errors, but kept going.
 * 34	= fatal: Permission errors.
 * 35	= fatal: Argument errors.
 * 36	= fatal: Malloc errors.
 * 37	= fatal: Device errors. Problem on mmap'ed tmp file.
 * 40	= fatal: Filesystem superblock is invalid.
 * 45	= fatal: Filesystem .inodes file is invalid.
 * 50	= fatal: I/O errors terminated processing.
 */
#define	ES_ok		0
#define	ES_alert	20
#define	ES_error	30
#define	ES_perm		34
#define	ES_args		35
#define	ES_malloc	36
#define	ES_device	37
#define	ES_sblk		40
#define	ES_inodes	45
#define	ES_io		50

int exit_status = ES_ok;			/* exit status */

#define	MAX_BLOCK_USAGE 10

typedef enum  {
	bu_unknown,		/* unknown block usage */
	bu_label,		/* file system label block */
	bu_pri_super,		/* file system primary super block */
	bu_alt_super,		/* file system alternate super block */
	bu_fsmap,		/* file system space map block */
	bu_fssys,		/* file system block */
	bu_free,		/* free data block */
	bu_inode,		/* data block for .inodes file */
	bu_data,		/* data block for other files */
	bu_dir,			/* data block for directory */
	bu_indirect,		/* indirect block for file/dir */
	bu_invalid_blk,		/* invalid block number */
	bu_invalid_ord		/* invalid ordinal number */
} bn_use_t;

sam_id_t	noid = {0, 0};			/* default inode id */

typedef struct block_use {
	bn_use_t	use;		/* block use */
	sam_id_t	id;		/* inode id, if data block */
	int		ino;		/* first inode num (.inodes block) */
} block_use_t;

typedef struct block_ord {
	ushort_t	bu_n;		/* count of uses on this ordinal */
	block_use_t	bu[MAX_BLOCK_USAGE];	/* block usage type(s) */
} block_ord_t;

typedef struct block_ent {
	block_ord_t	bo[1];		/* blk ord table (entry per ord) */
} block_ent_t;

typedef struct block_list {
	char		*arg;		/* ptr to original arg string */
	sam_daddr_t	bn;		/* block number */
	int		ord;		/* ordinal */
	block_ent_t	*bep;		/* array of block entries 1/ord */
} block_list_t;

block_list_t *blp;		/* ptr to global block list */
int nblks;			/* num of ents alloced for block list */
int ext_bshift;			/* Extent shift for this file system */
int sblk_version;		/* Superblock version */

sam_mount_info_t mnt_info;	/* mount info from mcf file */
int fs_count;			/* number of partitions in fs */

char tmp_name[sizeof (uname_t) + 24]; /* temp bit map mmap file */
int tmp_fd;

struct sam_perm_inode *inode_ino;   /* .inodes (ino = 1) pointer */
struct sam_perm_inode *block_ino;   /* .blocks (ino = 3) pointer */
int ino_count;

int sord = -1;					/* last ordinal for get_bn */
sam_daddr_t sbn = 0;				/* last block for get_bn */


sam_daddr_t bio_buf_bn = 0;	/* last block read/written for bio_buffer */
int bio_buf_ord = -1;		/* last ordinal r/w for bio_buffer */
offset_t bio_buf_off = -1;	/* last offset r/w for bio_buffer */
static char msgbuf[MAX_MSGBUF_SIZE];		/* sysevent message buffer */

int process_args(int argc, char **argv);
void build_devices(void);
void build_super_block(void);
int build_inodes(void);
void build_free_block_map(void);
void map_sm_free_blocks(void);

int check_fs_blocks(void);
int check_free_blocks(void);
int check_free_blk(sam_daddr_t bn, int ord);
int check_inode_blocks(void);
int check_ino_blk(struct sam_perm_inode *dp);
int check_ino_indirect_blk(struct sam_perm_inode *dp, sam_daddr_t bn, int ord,
					int level, uint_t *cnt);
int mark_block_use(int blk, int ord, bn_use_t use, sam_id_t id, int ino);

int check_bn(ino_t ino, sam_daddr_t bn, int ord);
int get_bn(struct sam_perm_inode *dp, offset_t offset,
				sam_daddr_t *bn, int *ord, int correct);
int get_inode(ino_t ino, struct sam_perm_inode *dp);
void print_block_list(void);


/*
 * ----------- main
 *
 * Find all uses of block(s) on requested mount point
 */
void
main(int argc, char **argv)
{
	int i;

	CustmsgInit(0, NULL);
	program_name = basename(argv[0]);
	if ((getuid() != 0) && (geteuid() != 0)) {
		/* You must be root to run %s\n */
		fprintf(stderr, GetCustMsg(13920), program_name);
		exit(ES_perm);
	}

	if (argc < 3) {
		/* Usage: %s fs-name block-number...\n */
		fprintf(stderr, GetCustMsg(13921), program_name);
		exit(ES_args);
	}

	if ((process_args(argc, argv))) {
		/* %s: Configuration error.\n */
		fprintf(stderr,  GetCustMsg(13922), program_name);
		exit(ES_args);
	}

	sync();
	sync();		/* make sys blocks and .inodes more current */

	build_devices();	/* build the devices array in ordinal order */
	build_super_block();	/* read super block info into global */
	build_inodes();		/* initialize .inodes and .blocks inodes */
	build_free_block_map();	/* construct in mem map of free blocks */

	check_fs_blocks();
	check_free_blocks();
	check_inode_blocks();

	print_block_list();

	unlink(tmp_name);

	for (i = 0; i < fs_count; i++) {
		close(devp->device[i].fd);
	}

	exit(exit_status);
}


/*
 * ----------- process_args
 *
 * Get filesystem devices. Open devices and get size.
 */

int				/* ERRNO if error, 0 if successful */
process_args(int argc, char **argv)
{
	int err;
	int arg_index;
	upath_t mnt_point;
	uint64_t block_number;

	strcpy(mnt_point, argv[1]);

	/* Initialize samfs syscalls */
	LoadFS(NULL);
	if (err = chk_devices(mnt_point, O_RDONLY, &mnt_info)) {
		return (err);
	}
	fs_count = mnt_info.params.fs_count;
	strncpy(fs_name, mnt_info.params.fi_name, sizeof (uname_t));
	fs_name[sizeof (uname_t) - 1] = '\0';
	mm_count = mnt_info.params.mm_count;
	mm_ord = 0;

	if ((check_mnttab(fs_name))) {
		error(0, 0,
		    catgets(catfd, SET, 13325,
		"Results are NOT accurate -- the filesystem %s is mounted."),
		    fs_name);
	}

	/* Allocate table to hold requested block number/ordinal args. */
	if ((blp = (block_list_t *)malloc(sizeof (block_list_t) * argc))
	    == NULL) {
		error(ES_malloc, 0,
		    catgets(catfd, SET, 13220,
		    "Cannot malloc block list space"));
	}
	memset((char *)blp, 0, sizeof (block_list_t) * argc);

	/* Process args. */
	nblks = 0;
	for (arg_index = 2; arg_index < argc; arg_index++) {
		block_ent_t *bep;
		int nords;

		blp[nblks].arg = argv[arg_index];
		blp[nblks].bn = 0;
		blp[nblks].ord = -1;		/* default to all ords. */
		(void) sscanf(argv[arg_index], "%lli.%i",
		    &block_number, &blp[nblks].ord);
		blp[nblks].bn = block_number;

		/* Allocate a usage table for each ordinal requested. */
		if (blp[nblks].ord >= 0) {
			nords = 1;		/* scan bn on specific ord. */
		} else {
			nords = fs_count;	/* across all ords. */
		}
		if ((bep = (block_ent_t *)malloc(sizeof (block_ord_t) * nords))
		    == NULL) {
			error(ES_malloc, 0,
			    catgets(catfd, SET, 13220,
			    "Cannot malloc block list space"));
		}
		memset((char *)bep, 0, sizeof (block_ord_t) * nords);
		blp[nblks].bep = bep;

		/* Validate that ordinal is in range. */
		if (blp[nblks].ord >= fs_count) {
			mark_block_use(nblks, 0, bu_invalid_ord, noid, 0);
		}
		nblks++;
	}

	return (err);
}


/*
 * ----------- build_devices
 *
 * Build devices array in ordinal order. Verify all partitions are present.
 */
void
build_devices(void)
{
	struct d_list *ndevp;
	int old_count;
	int ord;
	time_t time;

	if ((ndevp = (struct d_list *)malloc(sizeof (struct devlist) *
	    fs_count)) ==
	    NULL) {
		error(ES_malloc, 0,
		    catgets(catfd, SET, 604,
		    "Cannot malloc ndevp"));
	}
	memcpy((char *)ndevp, (char *)devp, (sizeof (struct devlist) *
	    fs_count));
	memset((char *)devp, 0, (sizeof (struct devlist) * fs_count));

	/*
	 * Find ordinal 0 for existing filesystem fs_name.
	 */
	old_count = 0;
	for (ord = 0; ord < fs_count; ord++) {
		if (d_read(&ndevp->device[ord], (char *)&sblock, 1,
		    SUPERBLK)) {
			error(ES_io, 0,
			    catgets(catfd, SET, 2439,
			    "Superblock read failed on eq %d"),
			    ndevp->device[ord].eq);
		}
		if (strncmp(sblock.info.sb.fs_name, fs_name,
		    sizeof (uname_t)) == 0) {
			if (sblock.info.sb.ord == 0) {
				old_count = sblock.info.sb.fs_count;
				time = sblock.info.sb.init;
				break;
			}
		}
	}
	if (old_count == 0) {
		error(ES_sblk, 0,
		    catgets(catfd, SET, 583,
		    "Cannot find ordinal 0 for filesystem %s"),
		    fs_name);
	}

	for (ord = 0; ord < fs_count; ord++) {
		if (d_read(&ndevp->device[ord], (char *)&sblock, 1,
		    SUPERBLK)) {
			error(ES_io, 0,
			    catgets(catfd, SET, 2439,
			    "Superblock read failed on eq %d"),
			    ndevp->device[ord].eq);
		}
		/* Validate label is same for all members & not duplicated. */
		if (strncmp(sblock.info.sb.name, "SBLK", 4) != 0 ||
		    (sblock.info.sb.magic != SAM_MAGIC_V1 &&
		    sblock.info.sb.magic != SAM_MAGIC_V2 &&
		    sblock.info.sb.magic != SAM_MAGIC_V2A)) {
			/* Send sysevent to generate SNMP trap */
			snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(1660),
			    devp->device[mm_ord].eq, SUPERBLK);
			PostEvent(FS_CLASS, "MissingName", 1660, LOG_ERR,
			    msgbuf,
			    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
			error(ES_sblk, 0, "%s", msgbuf);

		} else if (strncmp(sblock.info.sb.fs_name, fs_name,
		    sizeof (uname_t)) ||
		    time != sblock.info.sb.init) {
			/* Send sysevent to generate SNMP trap */
			snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(1661),
			    ndevp->device[ord].eq, fs_name);
			PostEvent(FS_CLASS, "MismatchEq", 1661, LOG_ERR,
			    msgbuf,
			    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
			error(ES_sblk, 0, "%s", msgbuf);

		} else {
			/* Old partitions for filesystem fs_name */
			if (devp->device[sblock.info.sb.ord].eq != 0) {
				error(ES_sblk, 0,
				    catgets(catfd, SET, 1627,
				    "mcf eq %d has duplicate ordinal %d "
				    "in filesystem %s"),
				    ndevp->device[ord].eq,
				    sblock.info.sb.ord, fs_name);
			}
			memcpy((char *)&devp->device[sblock.info.sb.ord],
			    (char *)&ndevp->device[ord],
			    sizeof (struct devlist));
		}
	}

	/* Make sure all members of the storage set are present. */
	for (ord = 0; ord < fs_count; ord++) {
		if (devp->device[ord].eq == 0) {
			error(ES_sblk, 0,
			    catgets(catfd, SET, 1628,
			    "mcf eq %d, ordinal %d, not present "
			    "in filesystem %s"),
			    devp->device[ord].eq, ord, fs_name);
		}
	}

	free((void *)ndevp);
}


/*
 * ----------- build_super_block
 *
 * Read super block info from disk into global var sblock.
 */
void
build_super_block(void)
{
	int sblk;
	int ord;

	/* Read superblock to locate system area. */
	sblk = howmany(L_SBINFO + (fs_count * L_SBORD), SAM_DEV_BSIZE);
	if (d_read(&devp->device[mm_ord], (char *)&sblock, sblk, SUPERBLK)) {
		error(ES_io, 0,
		    catgets(catfd, SET, 2439,
		    "Superblock read failed on eq %d"),
		    devp->device[mm_ord].eq);
	}
	if (strncmp(sblock.info.sb.name, "SBLK", 4) != 0 ||
	    (sblock.info.sb.magic != SAM_MAGIC_V1 &&
	    sblock.info.sb.magic != SAM_MAGIC_V2 &&
	    sblock.info.sb.magic != SAM_MAGIC_V2A)) {
		/* Send sysevent to generate SNMP trap */
		snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(1660),
		    devp->device[mm_ord].eq, SUPERBLK);
		PostEvent(FS_CLASS, "MissingName", 1660, LOG_ERR, msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		error(ES_sblk, 0, "%s", msgbuf);
	}

	/*
	 * Validate that superblock contains expected family set name.
	 */
	if (strncmp(sblock.info.sb.fs_name, fs_name, sizeof (uname_t))) {
		/* Send sysevent to generate SNMP trap */
		snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(1661),
		    devp->device[mm_ord].eq, fs_name);
		PostEvent(FS_CLASS, "MismatchEq", 1661, LOG_ERR, msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		error(ES_sblk, 0, "%s", msgbuf);
	}

	/*
	 * Backward compatibility prior to 3.6. Set meta blocks for LG.
	 */
	if (sblock.info.sb.mm_blks[LG] == 0) {
		if (sblock.info.sb.meta_blks) {
			sblock.info.sb.mm_blks[SM] = sblock.info.sb.meta_blks;
			sblock.info.sb.mm_blks[LG] = sblock.info.sb.meta_blks;
		} else {
			sblock.info.sb.mm_blks[SM] =
			    sblock.info.sb.dau_blks[SM];
			sblock.info.sb.mm_blks[LG] =
			    sblock.info.sb.dau_blks[LG];
		}
	}

	/*
	 * Backward compatibility prior to 3.3.
	 */
	if (mm_count == 0) {
		sblock.info.sb.mm_blks[SM] = sblock.info.sb.dau_blks[SM];
		sblock.info.sb.mm_blks[LG] = sblock.info.sb.dau_blks[LG];
		sblock.info.sb.meta_blks = sblock.info.sb.dau_blks[LG];
		sblock.info.sb.da_count = fs_count;
		for (ord = 0; ord < fs_count; ord++) {
			sblock.eq[ord].fs.mm_ord = ord;
			sblock.eq[ord].fs.system = sblock.eq[ord].fs.allocmap +
			    sblock.eq[ord].fs.l_allocmap;
			if (ord == 0) {
				sblock.eq[ord].fs.system +=
				    sblock.info.sb.dau_blks[LG];
			}
		}
	}
	if (sblock.info.sb.fs_count != fs_count) {
		/* Send sysevent to generate SNMP trap */
		snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(2250),
		    sblock.info.sb.fs_count, fs_count, fs_name);
		PostEvent(FS_CLASS, "MismatchSblk", 2250, LOG_ERR, msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		error(ES_sblk, 0, "%s", msgbuf);
	}
	ext_bshift = sblock.info.sb.ext_bshift - SAM_DEV_BSHIFT;
	switch (sblock.info.sb.magic) {
		case SAM_MAGIC_V1:
			sblk_version = SAMFS_SBLKV1;
			break;
		case SAM_MAGIC_V2:
		case SAM_MAGIC_V2A:
			sblk_version = SAMFS_SBLKV2;
			break;
	}

	sam_set_dau(&mp->mi.m_dau[DD], sblock.info.sb.dau_blks[SM],
	    sblock.info.sb.dau_blks[LG]);
	sam_set_dau(&mp->mi.m_dau[MM], sblock.info.sb.mm_blks[SM],
	    sblock.info.sb.mm_blks[LG]);
	get_mem(ES_malloc);
}


/*
 * ----------- build_inodes
 *
 * Initialize the special inodes for .inodes and .blocks files.
 */
int
build_inodes(void)
{
	sam_daddr_t bn;
	int ord;

	/* Save first block so .inodes & .blocks can be read */
	bn = sblock.info.sb.inodes;	/* First block of .inodes file */
	ord = 0;			/* Ordinal of first block in .inodes */
	if (d_read(&devp->device[ord], (char *)first_sm_blk,
	    SM_DEV_BLOCK(mp, MM), bn)) {
		error(ES_io, 0,
		    catgets(catfd, SET, 13390,
		    "Read failed in .inodes on eq %d at block 0x%llx"),
		    devp->device[ord].eq, bn);
	}
	inode_ino = (struct sam_perm_inode *)first_sm_blk;
	block_ino = (struct sam_perm_inode *)(first_sm_blk +
	    (sizeof (struct sam_perm_inode) * (SAM_BLK_INO - 1)));

	/* Sanity check the .inode and .blocks inodes */
	if (inode_ino->di.id.ino != SAM_INO_INO ||
	    inode_ino->di.id.gen != SAM_INO_INO ||
	    block_ino->di.id.ino != SAM_BLK_INO ||
	    block_ino->di.id.gen != SAM_BLK_INO) {
		error(ES_inodes, 0,
		    catgets(catfd, SET, 13317,
		    "Basic checks of .inodes and/or .blocks failed"));
	}
	ino_count = inode_ino->di.rm.size>>SAM_ISHIFT;

	return (0);
}


/*
 * ----------- build_free_block_map
 *
 * Build free block allocation map from ondisk map and small blocks file.
 */
void
build_free_block_map(void)
{
	off_t length;
	int ord;
	int dt;
	int d_dau;

	sam_daddr_t bn = 0;
	int mmord;
	int daul;
	int blocks;
	char *cptr;
	int ii;
	uint_t *iptr;
	uint_t *optr;
	int obit;
	int kk;
	int ibit;
	uint_t mask;

	/*
	 * Allocate mmap'ed tmp file to hold bit maps.
	 */
	sprintf(tmp_name, "%s/%d.sambcheck", "/tmp\0", (int)getpid());
	if ((tmp_fd = open(tmp_name, (O_CREAT|O_TRUNC|O_RDWR), 0600)) < 0) {
		error(ES_device, errno,
		    catgets(catfd, SET, 613,
		    "Cannot open %s"),
		    tmp_name);
	}

	/*
	 * Map enough space for bit maps --
	 * (# large blocks times * small blocks in large block)
	 */
	length = 0;
	for (ord = 0; ord < fs_count; ord++) {
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* ! 1st group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				devp->device[ord].off = 0;
				devp->device[ord].length = 0;
				continue;
			}
		}
		if (sblock.eq[ord].fs.type == DT_META) {
			dt = MM;
		} else {
			dt = DD;
		}
		d_dau = SAM_DEV_BSIZE * SM_BLKCNT(mp, dt);
		devp->device[ord].off = length;
		devp->device[ord].length = (sblock.eq[ord].fs.l_allocmap *
		    d_dau);
		length += ((devp->device[ord].length + PAGEOFFSET) & PAGEMASK);
	}

	if (ftruncate(tmp_fd, length) < 0) {
		error(ES_device, errno,
		    catgets(catfd, SET, 645,
		    "Cannot truncate %s: length %d"),
		    tmp_name, length);
	}

	for (ord = 0; ord < fs_count; ord++) {
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* ! 1st group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				continue;
			}
		}
		if ((devp->device[ord].mm = mmap((caddr_t)NULL,
		    devp->device[ord].length,
		    (PROT_WRITE|PROT_READ),
		    MAP_SHARED,
		    tmp_fd,
		    (off_t)devp->device[ord].off)) == (char *)MAP_FAILED) {
			error(ES_device, errno,
			    catgets(catfd, SET, 610,
			    "Cannot mmap %s: length %d"),
			    tmp_name, length);
		}
		if (devp->device[ord].type == DT_META) {
			dt = MM;
		} else {
			dt = DD;
		}

		/* Update in-memory map from disk large dau map. */
		mmord = sblock.eq[ord].fs.mm_ord;
		daul = sblock.eq[ord].fs.l_allocmap;	/* number of blocks */
		blocks = sblock.eq[ord].fs.dau_size;	/* no. of bits */
		cptr = devp->device[ord].mm;
		for (ii = 0; ii < daul; ii++) {
			if (d_read(&devp->device[mmord], (char *)dcp, 1,
			    (sblock.eq[ord].fs.allocmap + ii))) {
				error(ES_io, 0,
				    catgets(catfd, SET, 797,
				    "Dau map read failed on eq %d"),
				    devp->device[ord].eq);
			}
			iptr = (uint_t *)dcp;
			optr = (uint_t *)cptr;
			obit = 0;
			for (kk = 0; kk < (SAM_DEV_BSIZE / NBPW); kk++) {
				for (ibit = 31; ibit >= 0; ibit--) {
					blocks--;
					if (blocks < 0)  break;
					if (obit == 0)  *optr = 0;
					/* If free */
					if (*iptr & (1 << ibit)) {
						mask = SM_BITS(mp, dt) <<
						    (31 - obit -
						    (SM_BLKCNT(mp, dt) - 1));
						*optr |= mask;
					}
					obit += SM_BLKCNT(mp, dt);
					if (obit == 32) {
						optr++;
						obit = 0;
					}
					bn += LG_DEV_BLOCK(mp, dt);
				}
				iptr++;
			}
			cptr += (SAM_DEV_BSIZE * SM_BLKCNT(mp, dt));
		}
	}

	map_sm_free_blocks();
}


/*
 * ----- map_sm_free_blocks
 *
 * Mark bit maps for each free small block in the .blocks file.
 */

void
map_sm_free_blocks(void)
{
	offset_t offset;
	sam_daddr_t bn;
	int ord;
	struct sam_inoblk *smp;
	int i, j;
	char *cptr;
	sam_u_offset_t bit;
	sam_u_offset_t sbit;
	uint_t off;
	uint_t *wptr;
	uint_t mask;

	offset = 0;
	while (offset < block_ino->di.rm.size) {
		if (get_bn(block_ino, offset, &bn, &ord, 1)) {
			return;
		}
		if (check_bn(block_ino->di.id.ino, bn, ord)) {
			return;
		}
		if (d_read(&devp->device[ord], (char *)dcp,
		    LG_DEV_BLOCK(mp, MM), bn)) {
			error(ES_io, errno,
			    catgets(catfd, SET, 315,
			    ".blocks read failed on eq %d"),
			    devp->device[ord].eq);
		}
		smp = (struct sam_inoblk *)dcp;
		for (i = 0; i < SM_BLK(mp, MM); i +=
			sizeof (struct sam_inoblk)) {
			if (smp->bn == 0xffffffff) {
				return;			/* End of list */
			}
			if (smp->bn != 0) {		/* Full entry */
				bn = smp->bn;
				bn <<= ext_bshift;
				ord = smp->ord;
				cptr = devp->device[ord].mm;
				if (check_bn(block_ino->di.id.ino, bn, ord)) {
					continue;
				}
				for (j = 0; j < SM_BLKCNT(mp, MM); j++) {
					if (smp->free & (1 << j)) {
						bit = bn >> DIF_SM_SHIFT(mp,
							MM);	/* small dau */
						sbit = bit;

						/* large dau start */
						bit = bit &
							~(SM_DEV_BLOCK(mp,
							MM) - 1);

						/* Word offset */
						off = (bit >> NBBYSHIFT) &
							0xfffffffc;
						wptr = (uint_t *)(cptr + off);
						bit = sbit & 0x1f;
						mask = 1 << (31 - bit);
						*wptr |= mask;
					}
					bn += SM_DEV_BLOCK(mp, MM);
				}
			}
			smp++;
		}
		offset += SM_BLK(mp, MM);
	}
}


/*
 * ----------- check_fs_blocks
 *
 * Check requested block list against fs system blocks.
 */
int
check_fs_blocks(void)
{
	int ord;
	int i;

	/* Identify usage of each block on selected ordinal (or default all) */
	for (ord = 0; ord < fs_count; ord++) {
		for (i = 0; i < nblks; i++) {
			if ((blp[i].ord < 0) || (blp[i].ord == ord)) {

				/* Check easy ones first */
				if (blp[i].bn == LABELBLK) {
					mark_block_use(i, ord, bu_label,
					    noid, 0);

				/* Primary super block on all ordinals */
				} else if (blp[i].bn ==
				    sblock.info.sb.offset[0]) {
					mark_block_use(i, ord, bu_pri_super,
					    noid, 0);

				/* Alternate super block only on ord 0 */
				} else if ((blp[i].bn ==
				    sblock.info.sb.offset[1]) &&
				    (ord == 0)) {
					mark_block_use(i, ord, bu_alt_super,
					    noid, 0);

				/* Allocation map ?? */
				} else if ((blp[i].bn >=
				    sblock.eq[ord].fs.allocmap) &&
				    (blp[i].bn < (sblock.eq[ord].fs.allocmap +
				    sblock.eq[ord].fs.l_allocmap))) {
					mark_block_use(i, ord, bu_fsmap,
					    noid, 0);

				/* System block ?? */
				} else if (blp[i].bn <
				    sblock.eq[ord].fs.system) {
					mark_block_use(i, ord, bu_fssys,
					    noid, 0);

				/* Invalid block ?? */
				} else if (blp[i].bn >=
				    sblock.eq[ord].fs.capacity) {
					mark_block_use(i, ord,
					    bu_invalid_blk, noid, 0);
				}
			}
		}
	}
	return (0);
}


/*
 * ----------- check_free_blocks
 *
 * Build free block allocation map from ondisk map and small blocks file.
 * Check requested blocks against map.
 */
int
check_free_blocks(void)
{
	int ord;
	int i;

	for (ord = 0; ord < fs_count; ord++) {

		/*
		 * Check to see if each block is free on this ordinal,
		 * if requested.
		 */
		for (i = 0; i < nblks; i++) {

			/* this ord */
			if ((blp[i].ord < 0) || (blp[i].ord == ord)) {
				/* invalid ?? */
				if (blp[i].bn >= sblock.eq[ord].fs.capacity) {
					continue;
				}
				if (check_free_blk(blp[i].bn, ord) == 1) {
					mark_block_use(i, ord, bu_free, noid,
					    0);
				}
			}
		}
	}

	return (0);
}


/*
 * ----------- check_free_blk
 *
 * Check free map for requested block on requested ordinal.
 */
int				/* returns 1 if free, 0 allocated, -1 error */
check_free_blk(
	sam_daddr_t bn,		/* block number */
	int	ord)		/* ordinal to search */
{
	char *cptr;
	int dt;			/* data or meta device */
	int	bt;		/* small or large block */
	int bit;
	int sbit;
	int offset;
	uint_t *wptr;
	uint_t mask;

	cptr = devp->device[ord].mm;
	if (sblock.eq[ord].fs.type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}

	/*
	 * If small daus (SM_BLKCNT > 1) are supported for this device, the
	 * working bit maps (devp->device[ord].mm) have been expanded to
	 * include SM_BLKCNT bits per large DAU.
	 */
	if (SM_BLKCNT(mp, dt) > 1) {	/* If this device has small daus */
		bt = SM;
		bit = (int)(bn >> DIF_SM_SHIFT(mp, dt));
		sbit = bit;
		bit = bit & ~(SM_DEV_BLOCK(mp, dt) - 1); /* large dau start */
	} else {
		bt = LG;
		if (mp->mi.m_dau[dt].dif_shift[bt]) {
			bit = (int)(bn >> mp->mi.m_dau[dt].dif_shift[bt]);
		} else {
			bit = (int)(bn / mp->mi.m_dau[dt].kblocks[bt]);
		}
		sbit = bit;
	}

	offset = (bit >> NBBYSHIFT) & 0xfffffffc;	/* Word offset */
	wptr = (uint_t *)(cptr + offset);
	bit = sbit & 0x1f;
	if (bt == SM) {
		mask = 1 << (31 - bit);
	} else {
		mask = SM_BITS(mp, dt) << (31 - bit - (SM_BLKCNT(mp, dt) - 1));
	}
	if (*wptr & mask) {
		return (1);
	}
	return (0);
}


/*
 * ----------- check_inode_blocks
 *
 * Check each inode for request blocks
 */
int
check_inode_blocks(void)
{
	char *ip;
	struct sam_perm_inode *dp;
	ino_t ino;

	if ((ip = (char *)malloc(sizeof (struct sam_perm_inode))) == NULL) {
		error(ES_malloc, 0,
		    catgets(catfd, SET, 13338,
		    "Cannot malloc inode"));
	}
	dp = (struct sam_perm_inode *)ip;
	for (ino = 1; ino <= ino_count; ino++) {
		if (get_inode(ino, dp) == 1) {			/* EOF */
			error(ES_inodes, 0, catgets(catfd, SET, 13365,
			    "ALERT:  Zero block number in .inodes file "
			    "at ino %d"), ino);
		}

		/* Don't check free inodes */
		if (dp->di.mode == 0 || dp->di.id.ino == 0) {
			continue;
		}
		if ((dp->di.id.ino != SAM_ROOT_INO) &&
		    (dp->di.id.ino == dp->di.parent_id.ino) &&
		    (dp->di.blocks == 0)) {
			continue;
		}

		/* Don't check inodes that don't have blocks */
		if (S_ISLNK(dp->di.mode) && (dp->di.ext_attrs & ext_sln)) {
			continue;
		}
		if (S_ISREQ(dp->di.mode) && (dp->di.ext_attrs & ext_rfa)) {
			continue;
		}
		if (S_ISEXT(dp->di.mode)) {
			continue;
		}

		/* Check inodes that do have blocks */
		if (S_ISREG(dp->di.mode) || S_ISDIR(dp->di.mode)) {
			check_ino_blk(dp);
		}
	}

	free((void *)ip);
	return (0);
}


/*
 * ----------- check_ino_blk
 *
 * Check extent blocks for inode for match on request block(s) list.
 */
int					/* returns 0 success, -1 error */
check_ino_blk(struct sam_perm_inode *dp)
{
	int dt;
	sam_daddr_t bn;
	int cnt;
	int ord;
	int i;
	int ii;
	int use;
	int ino = 0;
	uint_t block_cnt = 0;


	dt = dp->di.status.b.meta;

	/* Check direct map to see if any block is on the req'ed block list. */
	if (dp->di.status.b.direct_map) {
		bn   = dp->di.extent[0];
		bn <<= ext_bshift;
		cnt = dp->di.extent[1];
		ord = dp->di.extent_ord[0];
		for (i = 0; i < nblks; i++) {
			if (bn &&
			    (blp[i].bn >= bn) &&
			    (blp[i].bn < (bn + cnt)) &&
			    ((blp[i].ord < 0) || (blp[i].ord == ord))) {
				if (S_ISDIR(dp->di.mode)) {
					use = bu_dir;
				} else if (dp->di.id.ino == SAM_INO_INO) {
					use = bu_inode;
					block_cnt = blp[i].bn - bn;
					/*
					 * align boundary
					 */
					block_cnt &= ~(LG_DEV_BLOCK(mp, dt) -
					    1);
					ino = SAM_DTOI(((offset_t)block_cnt *
					    SAM_DEV_BSIZE));
				} else {
					use = bu_data;
				}
				mark_block_use(i, ord, use, dp->di.id, ino);
				ino = 0;
			}
		}
		return (0);
	}

	/*
	 * Check data extents to see if any block is on the req'ed
	 * block list.
	 */
	for (ii = 0; ii < NOEXT; ii++) {
		bn   = dp->di.extent[ii];
		bn <<= ext_bshift;
		if (bn == 0) {
			continue;
		}
		ord = dp->di.extent_ord[ii];
		if (ii < NDEXT) {
			for (i = 0; i < nblks; i++) {
				if ((blp[i].bn == bn) &&
				    ((blp[i].ord < 0) ||
				    (blp[i].ord == ord))) {
					if (S_ISDIR(dp->di.mode)) {
						use = bu_dir;
					} else if (dp->di.id.ino ==
					    SAM_INO_INO) {
						use = bu_inode;
						block_cnt &=
						    ~(LG_DEV_BLOCK(mp, dt) -
						    1);	/* align */
						ino = SAM_DTOI((
						    (offset_t)block_cnt *
						    SAM_DEV_BSIZE));
					} else {
						use = bu_data;
					}
					mark_block_use(i, ord, use,
					    dp->di.id, ino);
					ino = 0;
				}
			}
			if (dp->di.id.ino == SAM_INO_INO) {
				block_cnt += LG_DEV_BLOCK(mp, dt);
			}
		} else {
			check_ino_indirect_blk(dp, bn, ord,
			    (ii - NDEXT), &block_cnt);
		}
	}

	return (0);
}


/*
 * ----------- check_ino_indirect_blk
 *
 * Check inode indirect block for match on request block(s) list.
 */
int			/* -1 if error, 0 if okay, 1 if invalid block */
check_ino_indirect_blk(
	struct sam_perm_inode *dp,	/* inode entry */
	sam_daddr_t bn,			/* mass storage extent block number */
	int ord,			/* mass storage extent ordinal */
	int level,			/* level of indirection */
	uint_t *cnt)			/* running blk count (.inodes only) */
{
	char *ibuf;
	sam_indirect_extent_t *iep;
	int err = 0;
	int ii, i;
	sam_daddr_t ibn;
	int iord;
	int dt;
	int use;
	int ino = 0;

	if ((ibuf = (char *)malloc(LG_BLK(mp, MM))) == NULL) {
		error(ES_malloc, 0,
		    catgets(catfd, SET, 601,
		    "Cannot malloc indirect block"));
	}

	if (check_bn(dp->di.id.ino, bn, ord)) {
		free((void *)ibuf);
		return (1);
	}
	if (d_read(&devp->device[ord], (char *)ibuf,
	    LG_DEV_BLOCK(mp, MM), bn)) {
		printf(catgets(catfd, SET, 13388,
		    "ALERT:  ino %d,\tError reading indirect block "
		    "0x%llx on eq %d\n"),
		    (int)dp->di.id.ino, (sam_offset_t)bn,
		    devp->device[ord].eq);
		if (exit_status < ES_error) exit_status = ES_error;
		free((void *)ibuf);
		return (-1);
	}

	/* Note: the following code is also present in get_bn() */
	iep = (sam_indirect_extent_t *)ibuf;

	/* check for invalid indirect block */
	if (iep->ieno != level) {
		err = 1;			/* bad level */

	} else if (S_ISSEGS(&dp->di)) {		/* segment data */

		/* seg inode 0 can match inode id or index id */
		if (dp->di.rm.info.dk.seg.ord == 0) {
			if (((iep->id.ino != dp->di.parent_id.ino) ||
			    (iep->id.gen != dp->di.parent_id.gen)) &&
			    ((iep->id.ino != dp->di.id.ino) ||
			    (iep->id.gen != dp->di.id.gen))) {
				err = 1;
			}

		/* all other seg inodes must match inode id */
		} else if ((iep->id.ino != dp->di.id.ino) ||
		    (iep->id.gen != dp->di.id.gen)) {
			err = 1;
		}
	} else {				/* normal inode */
		if ((iep->id.ino != dp->di.id.ino) ||
		    (iep->id.gen != dp->di.id.gen)) {
			err = 1;
		}
	}
	if (err) {
		free((void *)ibuf);
		return (1);
	}

	/* Check to see if the indirect block is on the req'ed block list. */
	dt = dp->di.status.b.meta;
	for (i = 0; i < nblks; i++) {
		if ((blp[i].bn == bn) &&
		    ((blp[i].ord < 0) || (blp[i].ord == ord))) {
			mark_block_use(i, ord, bu_indirect, dp->di.id, 0);
		}
	}

	/* Check each data block to see if it is on the req'ed block list. */
	for (ii = 0; ii < DEXT; ii++) {
		ibn   = iep->extent[ii];
		ibn <<= ext_bshift;
		if (ibn == 0) {
			continue;
		}
		iord = (int)iep->extent_ord[ii];
		if (level) {
			check_ino_indirect_blk(dp, ibn, iord, (level - 1),
			    cnt);
		} else {
			for (i = 0; i < nblks; i++) {
				if ((blp[i].bn == ibn) &&
				    ((blp[i].ord < 0) ||
				    (blp[i].ord == iord))) {
					if (S_ISDIR(dp->di.mode)) {
						use = bu_dir;
					} else if (dp->di.id.ino ==
					    SAM_INO_INO) {
						use = bu_inode;
						ino = SAM_DTOI((*cnt *
						    SAM_DEV_BSIZE));
					} else {
						use = bu_data;
					}
					mark_block_use(i, iord, use,
					    dp->di.id, ino);
					ino = 0;
				}
			}
			if (dp->di.id.ino == SAM_INO_INO) {
				*cnt += LG_DEV_BLOCK(mp, dt);
			}
		}
	}

	return (0);
}


/*
 * ----------- mark_block_use
 *
 * Set usage type for block/ordinal.
 */
int
mark_block_use(
	int blk,		/* block number index in global block_list */
	int ord,		/* block ordinal */
	bn_use_t use,		/* usage type */
	sam_id_t id,		/* inode id, if data block */
	int ino)		/* first inode if .inodes data block */
{
	int nord;
	int nuses;

	/* Determine ordinal index in block list. */
	if (blp[blk].ord < 0) {		/* all ordinals ?? */
		nord = ord;		/* each ord has own entry */
	} else {
		nord = 0;		/* specific ords are in [0] */
	}
	/* number/index of block usage(s) */
	nuses = blp[blk].bep->bo[nord].bu_n;
	if (nuses < MAX_BLOCK_USAGE) {
		blp[blk].bep->bo[nord].bu[nuses].use = use;
		blp[blk].bep->bo[nord].bu[nuses].id = id;
		blp[blk].bep->bo[nord].bu[nuses].ino = ino;
		blp[blk].bep->bo[nord].bu_n++;
	}
	return (0);
}


/*
 * ----- check_bn - check block number
 *
 * Check that block number is within requested partition and not
 * a system block.
 */
int					/* -1 if error, 0 if successful. */
check_bn(
	ino_t ino,			/* inode number */
	sam_daddr_t bn,			/* block number */
	int ord)			/* ordinal */
{
	if (bn >= sblock.eq[ord].fs.capacity) {
		printf(catgets(catfd, SET, 13386,
		    "ALERT:  ino %d,\tblock 0x%llx exceeds capacity "
		    "on eq %d\n"),
		    (int)ino, (sam_offset_t)bn, devp->device[ord].eq);
		return (-1);
	}
	if (bn < sblock.eq[ord].fs.system) {
		printf(catgets(catfd, SET, 13387,
		    "ALERT:  ino %d,\tblock 0x%llx in system area "
		    "on eq %d\n"),
		    (int)ino, (sam_offset_t)bn, devp->device[ord].eq);
		return (-1);
	}
	return (0);
}


/*
 * ----- get_bn - get a block number from an inode.
 *
 *	Get block, ord, given logical byte offset and inode pointer.
 */
int					/* -1 if error, 0 if successful */
get_bn(
	struct sam_perm_inode *dp,	/* inode pointer */
	offset_t offset,		/* logical byte offset */
	sam_daddr_t *bn,		/* block number -- returned */
	int *ord,			/* block ordinal -- returned */
	int correct)			/* apply correction 1=yes, 0=no */
{
	struct sam_disk_inode *ip;
	sam_bn_t *bnp;
	uchar_t *eip;
	sam_indirect_extent_t *iep;
	int de;
	int dt;
	int bt;
	int ileft;
	int kptr[3];
	int err;

	if (dp == NULL)
		error(1, 0,
		    catgets(catfd, SET, 319,
		    ".inodes pointer not set"));
	if (dp->di.status.b.direct_map) {
		if ((offset >> SAM_DEV_BSHIFT) >= dp->di.extent[1]) { /* EOF */
			*bn = 0;
		} else {
			*bn   = dp->di.extent[0];
			*bn <<= ext_bshift;
			*bn  += (offset >> SAM_DEV_BSHIFT);
		}
		*ord = dp->di.extent_ord[0];
		return (0);
	}
	ip = (struct sam_disk_inode *)&dp->di;
	sam_cmd_get_extent(ip, offset, sblk_version, &de, &dt, &bt, kptr,
	    &ileft);
	bnp = &dp->di.extent[de];
	eip = &dp->di.extent_ord[de];
	if (de >= NDEXT) {
		int ii, kk;
		sam_daddr_t tmp_sbn;

		ii = de - NDEXT;
		for (kk = 0; kk <= ii; kk++) {
			if (*bnp == 0)  break;
			iep = (sam_indirect_extent_t *)ibufp;
			tmp_sbn   = *bnp;
			tmp_sbn <<= ext_bshift;
			if ((sbn != tmp_sbn) || (sord != (int)*eip)) {
				sbn = tmp_sbn;
				sord = (int)*eip;
				if (d_read(&devp->device[sord], (char *)ibufp,
				    LG_DEV_BLOCK(mp, dt), sbn)) {
					error(0, 0,
					    catgets(catfd, SET, 1375,
					    "Ino %d read failed on eq %d"),
					    dp->di.id.ino,
					    devp->device[sord].eq);
					if (exit_status < ES_error)
						exit_status = ES_error;
					return (-1);
				}

				/*
				 * Note: the following code is also present
				 * in samfsck.c.
				 */
				err = 0;
				if (iep->ieno != (ii - kk)) {
					err = 1;	/* bad level */

				} else if (S_ISSEGS(&dp->di)) {
					/* segment data */
					/*
					 * seg inode 0 can match inode id
					 * or index id.
					 */
					if (dp->di.rm.info.dk.seg.ord == 0) {
						if (((iep->id.ino !=
						    dp->di.parent_id.ino) ||
						    (iep->id.gen !=
						    dp->di.parent_id.gen)) &&
						    ((iep->id.ino !=
						    dp->di.id.ino) ||
						    (iep->id.gen !=
						    dp->di.id.gen))) {
							err = 1;
						}

					/*
					 * all other seg inodes must match
					 * inode id.
					 */
					} else if ((iep->id.ino !=
					    dp->di.id.ino) ||
					    (iep->id.gen != dp->di.id.gen)) {
							err = 1;
					}

				} else {		/* normal inode */
					if ((iep->id.ino != dp->di.id.ino) ||
					    (iep->id.gen != dp->di.id.gen)) {
						err = 1;
					}
				}
				if (err) {

					/* Invalid indirect block */
					printf(catgets(catfd, SET, 13389,
					    "ALERT:  ino %d,"
					    "\tInvalid indirect "
					    "block 0x%llx eq %d\n"),
					    dp->di.id.ino,
					    (sam_offset_t)sbn,
					    devp->device[sord].eq);
					if (exit_status < ES_alert)
						exit_status = ES_alert;
					return (-1);
				}
			}
			bnp = &iep->extent[kptr[kk]];
			eip = &iep->extent_ord[kptr[kk]];
		}
	}
	*bn   = *bnp;
	*bn <<= ext_bshift;
	*ord  = *eip;
	if (correct && (bt != SM)) {
		offset_t off_corr;
		struct devlist *dip;

		off_corr = offset;

		/*
		 * of 16k, 32k and 64k daus, 64k needs a more detailed
		 * block number correction.
		 */
		if (mp->mi.m_dau[dt].size[bt] > mp->mi.m_dau[dt].sm_off) {
			if (!dp->di.status.b.on_large)
				off_corr -= mp->mi.m_dau[dt].sm_off;
		}

		/*
		 * May have to modify ordinal returned for stripe groups.
		 */
		dip = (struct devlist *)&devp->device[*ord];
		if (dip->num_group > 1) {
			*ord += (off_corr / mp->mi.m_dau[dt].size[bt]) %
			    dip->num_group;
		}

		if (off_corr & (mp->mi.m_dau[dt].size[bt] - 1)) {
			*bn += (off_corr & (mp->mi.m_dau[dt].size[bt] - 1)) >>
			    SAM_DEV_BSHIFT;
		}
	}
	return (0);
}


/*
 * ----- get_inode - get inode
 *
 *  Read requested inode from the .inodes file using the bio_buffer.
 *	Always read a full buffer of inodes aligned on buffer size boundary.
 */

int		/* fatal if .inodes blk read/write error, 1 EOF, 0 otherwise */
get_inode(
	ino_t ino,			/* inode number of requested inode */
	struct sam_perm_inode *dp)	/* ptr to permanent inode (returned) */
{
	int dt = inode_ino->di.status.b.meta;
	sam_daddr_t bn;
	int ord;
	offset_t offset;
	offset_t off;
	char *ip;

	offset = SAM_ITOD(ino);
	off = offset & ~(mp->mi.m_dau[dt].size[LG] - 1); /* align boundary */
	if (off != bio_buf_off) {
		if (get_bn(inode_ino, offset, &bn, &ord, 1)) {
			error(ES_inodes, 0,
			    catgets(catfd, SET, 586,
			    "Cannot get .inodes block at offset 0x%llx"),
			    offset);
		}
		if (bn == 0) {
			return (1);			/* EOF */
		}
		bn &= ~(LG_DEV_BLOCK(mp, dt) - 1);	/* align boundary */
		if (check_bn(ino, bn, ord)) {
			error(ES_inodes, 0,
			    catgets(catfd, SET, 317,
			    ".inodes block at offset 0x%llx is in "
			    "system area"),
			    offset);
		}
		if (d_read(&devp->device[ord], (char *)bio_buffer,
		    LG_DEV_BLOCK(mp, dt), bn)) {
			error(ES_io, 0,
			    catgets(catfd, SET, 13390,
			    "Read failed in .inodes on eq %d at block 0x%llx"),
			    devp->device[ord].eq, (sam_offset_t)bn);
		}
		bio_buf_bn = bn;		/* save for future refs */
		bio_buf_ord = ord;
		bio_buf_off = off;
	}

	/* copy inode from buffer */
	ip = (char *)((char *)bio_buffer +
	    (offset&(mp->mi.m_dau[dt].size[LG]-1)));
	memcpy((char *)dp, ip, sizeof (struct sam_perm_inode));

	return (0);
}


/*
 * Print final usage contents of block list
 */
void
print_block_list(void)
{
	int i, j, k;
	int nords;
	int nuses;

	/* Display results of block number search on requested ordinals */
	for (i = 0; i < nblks; i++) {

		nords = fs_count;			/* report all ords */
		if (blp[i].ord >= 0)  nords = 1;	/* report one ord */
		for (j = 0; j < nords; j++) {

			if (is_stripe_group(sblock.eq[j].fs.type)) {
				if (sblock.eq[j].fs.num_group == 0) {
					/* ! 1st group member */
					if (blp[i].ord < 0) {
						continue;
					}
				}
			}
			nuses = blp[i].bep->bo[j].bu_n;
			if (nuses == 0)  nuses++; /* default usage applies */
			for (k = 0; k < nuses; k++) {

				switch (blp[i].bep->bo[j].bu[k].use) {
				case bu_label:
					/*
					 * block %s (%lld.%d) is a file
					 * system label block.
					 */
					printf(GetCustMsg(13276), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_pri_super:
					/*
					 * block %s (%lld.%d) is a file
					 * system primary super block.
					 */
					printf(GetCustMsg(13277), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_alt_super:
					/*
					 * block %s (%lld.%d) is a file
					 * system alternate super block.
					 */
					printf(GetCustMsg(13278), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_fsmap:
					/*
					 * block %s (%lld.%d) is a file
					 * system allocation map block
					 */
					printf(GetCustMsg(13279), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_fssys:
					/*
					 * block %s (%lld.%d) is a file
					 * system block.
					 */
					printf(GetCustMsg(13280), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_free:
					/*
					 * block %s (%lld.%d) is a free data
					 * block.
					 */
					printf(GetCustMsg(13281), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_inode:
					/*
					 * block %s (%lld.%d) is a data
					 * block for .inodes
					 * containing %d - %d.
					 */
					printf(GetCustMsg(13282), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j),
					    blp[i].bep->bo[j].bu[k].ino,
					    (blp[i].bep->bo[j].bu[k].ino +
					    INO_IN_BLK - 1));
					break;
				case bu_data:
					/*
					 * block %s (%lld.%d) is a data
					 * block for inode %d.%d.
					 */
					printf(GetCustMsg(13283), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j),
					    blp[i].bep->bo[j].bu[k].id.ino,
					    blp[i].bep->bo[j].bu[k].id.gen);
					break;
				case bu_dir:
					/*
					 * block %s (%lld.%d) is a data
					 * block for directory
					 * inode %d.%d.
					 */
					printf(GetCustMsg(13284), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j),
					    blp[i].bep->bo[j].bu[k].id.ino,
					    blp[i].bep->bo[j].bu[k].id.gen);
					break;
				case bu_indirect:
					/*
					 * block %s (%lld.%d) is an indirect
					 * block for inode %d.%d.
					 */
					printf(GetCustMsg(13285), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j),
					    blp[i].bep->bo[j].bu[k].id.ino,
					    blp[i].bep->bo[j].bu[k].id.gen);
					break;
				case bu_invalid_blk:
					/*
					 * block %s (%lld.%d) is an invalid
					 * block number
					 */
					printf(GetCustMsg(13286), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				case bu_invalid_ord:
					/*
					 * block %s (%lld.%d) is an invalid
					 * ordinal
					 */
					printf(GetCustMsg(13287), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				default:
					/*
					 * block %s (%lld.%d) usage is
					 * unknown
					 */
					printf(GetCustMsg(13288), blp[i].arg,
					    (sam_offset_t)blp[i].bn,
					    (blp[i].ord >= 0 ?  blp[i].ord :
					    j));
					break;
				}
				printf("\n");
			}
		}
	}
}
