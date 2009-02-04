/*
 *	samfsck.c  - Check a SAM-FS file system.
 *
 *	samfsck verifies the consistency of the blocks.
 *
 *	Check for duplicate file blocks or blocks also free in bit maps.
 *
 *	-F Repair inodes - if inode bad, mark offline; duplicated blocks
 *	   in inodes, mark offline. Repair bit maps - build bit maps from
 *	   the inodes and write maps.  Orphans are moved to lost+found.
 *	   Build and write out small block file. Write superblock with
 *	   updated space.
 *
 *	-G Generate directory hash - When used in conjunction with -F,
 *	   generate appropriate directory entry hash values. When -F is
 *	   omitted, a hash is generated and compared to the directory
 *	   entry hash, but no corrections are made.
 *
 *	-O Allow offlines - This option will prevent orphaning of files
 *	   and subdirectories of offline directories or orphaning of
 *	   segment file inodes when their parent index is offline.  A
 *	   warning message is generated when such offline components
 *	   are encountered to indicate that possible orphans may be
 *	   created since linkages between directories / segment indicies
 *	   and their subcomponents can not be verified.  Orphan inode
 *	   candidates will not be sent to lost+found when their parent
 *	   inode is offline.
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

#pragma ident "$Revision: 1.67 $"


/* ----- Includes */

#define	__QUOTA_DEFS

#include <stdio.h>
#include <stddef.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/mman.h>
#include <sys/sysmacros.h>

/* Solaris headers. */
#include <syslog.h>

#include <sam/types.h>
#include "sam/defaults.h"
#include <sam/custmsg.h>
#include <sam/param.h>
#include <pub/devstat.h>
#include <sam/lib.h>
#include <sam/mount.h>
#include <sam/quota.h>
#include <sam/nl_samfs.h>

#include "ino.h"
#include "macros.h"
#include "quota.h"
#include "sblk.h"
#include "validation.h"
#include "ino_ext.h"
#include "inode.h"
#include "mount.h"
#include "dirent.h"
#include "samhost.h"
#define	MAIN
#include "utility.h"
#include "pub/rminfo.h"
#include "qfs_log.h"

/*
 * Exit status:
 * 0	= filesystem is consistent.
 * 2	= filesystem is byte-reversed (use native host to fsck).
 * 4	= Non fatal: superblock counts can be reconciled.
 * 5	= Non fatal: blocks can be reclaimed.
 * 10   = Non fatal: orphans can be moved to lost+found.
 * 14	= Non fatal: bad info found in data segment inodes but can be repaired.
 * 15	= Non fatal: bad info found in extension inodes but can be repaired.
 * 20   = fatal: invalid directory blocks, duplicate blocks exist. Files/
 *			directories will be marked damaged/offline.
 * 30   = fatal: I/O Errors, but kept going. Filesystem is not consistent.
 * 35   = fatal: Argument errors.
 * 36   = fatal: Malloc errors.
 * 37   = fatal: Device errors.
 * 40   = fatal: Filesystem superblock is invalid.
 * 41   = fatal: Filesystem option mask has non-backwards compatible options.
 * 45   = fatal: Filesystem .inodes file is invalid.
 * 46	= fatal: Filesystem log contains transactions that should be replayed
 * 50   = Fatal I/O errors terminated processing.
 * 55   = -p (preen) specified; filesystem has 'fsck requested' bit set
 */
#define	ES_ok		0
#define	ES_byterev	2
#define	ES_sblock	4
#define	ES_blocks	5
#define	ES_orphan	10
#define	ES_ext		15
#define	ES_alert	20
#define	ES_error	30
#define	ES_args		35
#define	ES_malloc	36
#define	ES_device	37
#define	ES_sblk		40
#define	ES_opt_mask	41
#define	ES_inodes	45
#define	ES_log		46
#define	ES_io		50
#define	ES_preen	55

int exit_status = ES_ok;	/* Exit status */

int worm_convert_failed = 0, worm_converted = 0;
static int worm_conv_once = 0;

#define	SETEXIT(s) \
	exit_status = ((s) > exit_status ? (s) : exit_status)



/* Must do something here. All directories are 4096 now */

/* ----- Global tables & pointers */

#define	FIRST_PASS 0
#define	SECOND_PASS 1
#define	THIRD_PASS 2
int pass = FIRST_PASS;

/*
 * Names and defines to go with special files in root.
 * search.  The #defines match their index in the array.
 */
#define		ROOT_QUOTA_A		0
#define		ROOT_QUOTA_G		1
#define		ROOT_QUOTA_U		2
#define		ROOT_LOSTFOUND		3
char *rootfiles[] =				/* File name table */
	{ ".quota_a", ".quota_g", ".quota_u", "lost+found" };
int rootfilecount = sizeof (rootfiles)/sizeof (char *);


int repair_files = 0;		/* -F Repair files & directories */
int hash_dirs = 0;		/* -G Generate hash for directories */
int rename_fs = 0;		/* -R Change fs name in the super block */
int preen_fs = 0;		/* -p Return 0 if FS has not requested fsck */
int cvt_to_shared = 0;		/* -S Convert FS to a shared FS */
int cvt_to_nonshared = 0;	/* -U Convert FS to an unshared FS */
char *scratch_dir = NULL;	/* -s Scratch directory name */
#ifdef OFFDIRS
int clear_offlines = 0;		/* -O Clear offline directories */
#endif /* OFFDIRS */
#ifdef DAMFSCK
int damage_files = 0;		/* -d Use damage files & directories */
#endif /* DAMFSCK */
int verbose_print = 0;		/* -V verbose output */
int debug_print = 0;		/* -D debug output */
int fsck_wrapper = 0;		/* TRUE == Called from fsck wrapper */

char fsname[MAXNAMELEN];

int sm_blocks = 0;		/* Number of small blocks free & allocated */
int lg_blocks = 0;		/* Number of large blocks free & allocated */
int mm_blocks = 0;		/* Number of meta blocks free & allocated */

struct sam_sblk nblock;		/* New Superblock buffer */
int sblk_version;		/* Superblock version number */

long num_2hash;				/* Num hashable dir ents */
struct sam_perm_inode *inode_ino;	/* .inodes (ino = 1) pointer */
struct sam_perm_inode *block_ino;	/* .blocks (ino = 3) pointer */

struct sam_perm_inode *orphan_ino = NULL;	/* lost+found inode pointer */
struct sam_perm_inode  orphan_inode;		/* lost+found inode */
int orphan_inumber = 0;
int orphan_full = 0;				/* lost+found full flag */

sam_mount_info_t mnt_info;	/* Mount info from mcf file */
int fs_count = -1;		/* Number of partition in the filesystem */


#define	SM_INOCOUNT	3
#define	DUP_END	0x8fffffffffffffff
struct dup_inoblk {
	sam_daddr_t bn;		/* Large block address of first small block */
	uchar_t	ord;		/* Partition ordinal */
	uchar_t	dtype;		/* DD (data) or MM (meta) disk type */
	uchar_t	btype;		/* SM (small) or LG (large)  block type */
	uchar_t	count;		/* Count of duplicates */
	ushort_t  fill2;	/* reserved */
	ushort_t  free;		/* Bit mask for free blocks */
	ino_t	  ino[SM_INOCOUNT];
};
char dup_name[sizeof (uname_t) + 24]; /* Temp duplicate inodes file name */
int dup_fd = -1;		/* File descriptor for small block file */
int ext_bshift;			/* Extent shift for this file system */
char *dup_mm = NULL;
struct dup_inoblk *dup_last = NULL;
int dup_length = 512;
int cur_length = 0;

/* Hard link parent list */
#define	HLP_COUNT 4
struct hlp_list {
	int	count;			/* # of allocated ids[] */
	uint_t	n_ids;			/* Count of hard link parents */
	sam_id_t ids[HLP_COUNT];	/* start small */
};

/* Orphan status type defs */
#define	ORPHAN 0
#define	NOT_ORPHAN 1

/* Inode type defs */
#define	REG_FILE 1
#define	INO_EXTEN 2
#define	DIRECTORY 3
#define	SEG_INDEX 4
#define	SEG_INO 5
#define	INO_OBJECT 6

/* Problem type defs */
#define	OKAY			 0
#define	INVALID_EXT		10
#define	IO_ERROR		20
#define	INVALID_DIR		30
#define	INVALID_SEG		40
#define	INVALID_BLK		50
#define	DUPLICATE_BLK		60
#define	INVALID_INO		70
#define	BAD_BLKCNT		80

/* Log status type defs */
#define	LOG_NONE	0	/* Logbno in superblock 0 */
#define	LOG_EMPTY	10	/* Log ok and empty */
#define	LOG_NOT_EMPTY	20	/* Log ok and not empty */
#define	LOG_CORRUPT	30	/* Log is corrupt */

/*
 * Using this inode number to id block references from the log.
 * Should inode 42 ever be used, this number would have to change.
 */
#define	SAM_LOG_INO		42

/* Archive status type defs */
#define	COPIES 1
#define	NOCOPY 2
#define	FREED 3

/* Message class defs */
#define	MSG_CLASS_ALERT		"ALERT"
#define	MSG_CLASS_NOTICE	"NOTICE"
#define	MSG_CLASS_DEBUG		"DEBUG"

struct ino_list {
	sam_id_t	id;		/* Inode id */
	sam_id_t	parent_id;	/* Parent id */
	uchar_t		orphan;		/* Orphan status */
	uchar_t		type;		/* Type of inode */
	uchar_t		prob;		/* Type of probl - inval or dup blk */
	uchar_t		arch;		/* Type of sol'n - offline or damaged */
	uchar_t		seg_prob;	/* Segment inode children problems */
	uchar_t		seg_arch;	/* Segment inode archive copy status */
	ushort_t	fmt;		/* Mode of directory entry */
	uint_t		seg_size;	/* Segment index seg size in Mbytes */
	uint_t		seg_lim;	/* Segment index ordinal count */
	uint_t		nblock;		/* Inode block count */
	uint_t		block_cnt;	/* Running count of blocks */
	uint_t		nlink;		/* Inode link count */
	uint_t		link_cnt;	/* Running count of links */
	struct hlp_list *hlp;		/* Alternate parent list */
} *ino_mm;
int ino_count;

char tmp_name[sizeof (uname_t) + 24];
int tmp_fd = -1;		/* File desc for temp bit map mmap file */

time_t start_time;		/* time this fsck started */

int smb_stop = 0;		/* Stop writing to .blocks file */
off_t smb_off = 0;		/* Free small blocks offset in large block */
off_t smb_offset = 0;		/* Free small blocks file offset */
char *smbbuf;
struct sam_inoblk *smbptr;

ino_t freeino = 0;		/* Last free inode */
ino_t min_usr_inum;		/* Minimum user inode number */
int sord = -1;			/* Last ordinal for get_bn */
sam_daddr_t sbn = 0;		/* Last block for get_bn */

int bio_buf_ord = -1;		/* Last ordinal read/written for bio_buffer */
sam_daddr_t bio_buf_bn = 0;	/* Last block read/written for bio_buffer */
int bio_buf_mod = 0;		/* Current block in bio_buffer is modified */
offset_t bio_buf_off = -1;	/* Last file offset read/written, bio_buffer */

offset_t scount[SAM_MAX_DD];	/* Count of free blocks in bit map */
offset_t ncount[SAM_MAX_DD];	/* Count of free blocks in new bit map */

int dir_blk_ord = -1;		/* Last ord read/written for dir_blk buffer */
sam_daddr_t dir_blk_bn = 0;	/* Last blk read/written for dir_blk buffer */

int offline_dirs = 0;		/* Num offline dirs.  Can't fix nlinks if >0. */

/*
 * Quota data structures
 */
ino_t	quota_file_ino[SAM_QUOTA_DEFD];		/* .quota_[agu] file inodes */

#define		Q_HASHSIZE		1000
#define		Q_HASH(x)		(((~0UL >> 1) & (int)(x))%(Q_HASHSIZE))

/*
 * A hash table entry for a quota record.  Quota
 * type (admin, group, user) is implicit.
 */
struct quota_ent {
	int qe_index;
	int qe_flags;
	struct quota_ent *qe_next;
	struct sam_dquot qe_quota;
};

struct quota_tab {
	struct quota_ent **qt_tab;
};

static	char	msgbuf[MAX_MSGBUF_SIZE];	/* sysevent message buffer */

int process_args(int argc, char **argv);
static int devinfo_init();
void check_fs(void);
void fix_system(void);
void build_devices(void);
int process_inodes();
int check_inode(ino_t ino, struct sam_perm_inode *dp);
void count_inode_blocks(struct sam_perm_inode *dp);
int count_indirect_blocks(struct sam_perm_inode *dp, sam_daddr_t bn,
	int ord, int level, sam_daddr_t lastbn, int lastord, int *lastflag,
	int *excess_msg);
int count_block(ino_t ino, int dt, int bt, sam_daddr_t bn, int ord);
int check_bn(ino_t ino, sam_daddr_t bn, int ord);
int get_bn(struct sam_perm_inode *dp, offset_t offset, sam_daddr_t *bn,
	int *ord, int correct);
int check_duplicate(ino_t ino, int dt, int bt, sam_daddr_t bn, int ord);
void init_dup(void);
void extend_dup(void);
void write_map(int ord);
void update_block_counts_object(int ord);
void build_sm_block(sam_daddr_t bn, int ord, int mask);
void write_sm_block();
void read_sys_inodes();
int check_sys_inode_blocks(struct sam_perm_inode *dp);
int check_sys_indirect_blocks(struct sam_perm_inode *dp, sam_daddr_t bn,
	int ord, int level, sam_daddr_t lastbn, int lastord, int *lastflag,
	uint_t *block_cnt);
int verify_indirect_validation(struct sam_perm_inode *dp, sam_daddr_t bn,
	int ord, int level, sam_indirect_extent_t *iep);
int verify_inode_block(sam_daddr_t bn, int ord, ino_t ino);
void note_block(int ord, daddr32_t blk);
void lqfs_log_validate(int32_t logbno, int logord, void (*cb)(int, daddr32_t));
void lqfs_log_dispatch(int status);
int log_checksum(int32_t *sp, int32_t *lp, int nb);
void log_setsum(int32_t *sp, int32_t *lp, int nb);
void write_sys_inodes();
void check_free_blocks(int ord);
void check_sm_free_blocks();
void print_duplicates(void);
void print_inode_prob(struct ino_list *inop);
void make_orphan(ino_t ino, struct sam_perm_inode *dp);
int add_orphan(struct sam_perm_inode *dp);
int	change_dotdot(struct sam_perm_inode *dp);
int verify_dir_validation(struct sam_perm_inode *dp, struct sam_dirval *dvp,
	offset_t offset);
int verify_dot_dotdot(struct sam_perm_inode *dp, struct sam_empty_dir *dirp);
void mark_inode(ino_t ino, int prob);
void offline_inode(ino_t ino, struct sam_perm_inode *dp);
void free_inode(ino_t ino, struct sam_perm_inode *dp);
int check_reg_file(struct sam_perm_inode *dp);
int check_inode_exts(struct sam_perm_inode *dp);
int verify_inode_ext(struct sam_perm_inode *dp, struct sam_inode_ext *ep,
	sam_id_t eid);
int check_multivolume_inode_exts(sam_perm_inode_v1_t *dp, int copy);
int check_dir(struct sam_perm_inode *dp);
int check_seg_inode(struct sam_perm_inode *sp);
int check_seg_index(struct sam_perm_inode *dp);
int verify_hdr_validation(struct sam_perm_inode *dp, sam_val_t *dvp,
	offset_t offset);
int get_dir_blk(int type, struct sam_perm_inode *dp, offset_t offset);
int put_dir_blk(int type, struct sam_perm_inode *dp);
int get_inode(ino_t ino, struct sam_perm_inode *dp);
void save_hard_link_parent(struct sam_perm_inode *dp, struct ino_list *inop);
void update_hard_link_parent(struct sam_perm_inode *dp, struct hlp_list *hlp);
void put_inode(ino_t ino, struct sam_perm_inode *dp);
void sync_inodes(void);
void debug_print_blocks(int ord);
void debug_print_sm_blocks();
int debug_count_free_blocks(int ord);
int find_sm_free_block(int input_ord, sam_daddr_t input_bn);
int isrootfile(char *name, uint_t len);
void shared_fs_convert(struct sam_sblk *);
#ifdef DAMFSCK
int damage_fs(void);
#endif /* DAMFSCK */

void quota_tabinit(void);
void quota_count_file(struct sam_perm_inode *);
void quota_uncount_file(struct sam_perm_inode *);
void quota_uncount_blocks(struct sam_perm_inode *);
void quota_count_blocks(struct sam_perm_inode *);
void quota_block_adj(struct sam_perm_inode *dp, uint_t old, uint_t new);
void check_quota(int fix);
void verify_quota_file(struct sam_perm_inode *qip, int qt, int fix);
void verify_quota_indirect_block(struct sam_perm_inode *qip,
	sam_daddr_t bn, int ord, int level,
	int qt, offset_t *foffset, int fix);
void verify_quota_block(struct sam_perm_inode *qip,
	sam_daddr_t bn, int ord, int dt, int bt,
	int qt, offset_t *foffset, int fix);
int quota_compare_entry(struct sam_dquot *, struct sam_dquot *, int, int);

extern int fstab_fsname_get(char *mntpt, char *fsname);
extern ushort_t sam_dir_gennamehash(int nl, char *np);
extern int getHostName(char *host, int len, char *fs);
extern void conv_to_v2inode(sam_perm_inode_t *dp);
static int shared_server(struct devlist *dp, offset_t hblk, upath_t server);

static struct sigaction sig_action;		/* signal actions */
static void catch_signals(int sig);

static void usage_exit(int excode);
static void clean_exit(int excode);

char opt_usage[255];
char opt_string[64];
static void opt_init();

/*
 * ----- main
 * Consistency check a samfs filesystem.
 */

void
main(int argc, char **argv)
{
	int err;

	CustmsgInit(0, NULL);
	program_name = basename(argv[0]);

	/* Process the supplied arguments. */
	if ((err = process_args(argc, argv)) != ES_ok) {
		usage_exit(err);
	}

	/* Configure file system if neeeded. */
	ChkFs();
	time(&fstime);		/* File system init time */

	/* Set-up signal handling. */
	sig_action.sa_handler = catch_signals;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	(void) sigaction(SIGHUP,  &sig_action, NULL);
	(void) sigaction(SIGINT,  &sig_action, NULL);
	(void) sigaction(SIGTERM, &sig_action, NULL);

	/* Initialize device info. */
	if ((err = devinfo_init()) != ES_ok) {
		clean_exit(err);
	}

	/* Check the file system. (Sets 'exit_status'.) */
	check_fs();

	/* Clean-up and exit. */
	clean_exit(exit_status);
}


/*
 * ----- catch_signals
 */
static void
catch_signals(int sig)
{
	switch (sig) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
			error(0, 0, catgets(catfd, SET, 13968, "Stopped."));
			clean_exit(0);
	default:
			break;
	}
}

/*
 * ----- opt_init - Initialize supported command options.
 */
static void
opt_init()
{
	strcpy(opt_usage,
	    " [-s scratch_dir] [-D] [-F] [-V] [-G] [-R] [-p] [-S] [-U] ");
	strcpy(opt_string, "s:FDVGRpSU");
#ifdef OFFDIRS
	strcat(opt_usage, "[-O] ");
	strcat(opt_string, "O");
#endif /* OFFDIRS */
#ifdef DAMFSCK
	strcat(opt_usage, "[-d] ");
	strcat(opt_string, "d");
#endif /* DAMFSCK */

	if (strcmp(program_name, "fsck") == 0) {
		fsck_wrapper = TRUE;
		strcat(opt_usage,
		    "[-n | -N | -y | -Y] [-o FSType-specific-options] ");
		strcat(opt_string, "nNyYo:");
	}
}


/*
 * ----- usage - Construct and display usage message.
 */
static void
usage_exit(int excode)
{
	fprintf(stderr, "usage: %s %sfs_name\n",
	    program_name, opt_usage);

	clean_exit(excode);
}


/*
 * ----- clean_exit - Clean up samfsck leftovers and exit with the
 * specified exit code (excode).
 */
static void
clean_exit(int excode)
{
	/* Clean-up dup blk file */
	if (dup_fd >= 0) {
		close(dup_fd);
		unlink(dup_name);
	}

	/* Clean-up bit map file */
	if (tmp_fd >= 0) {
		close(tmp_fd);
		unlink(tmp_name);
	}

	/* Clean-up device info */
	close_devices(&mnt_info);

	exit(excode);
}


/*
 * -----  process_args
 * Process supplied command options.
 */
int					/* ERRNO if error, 0 if successful */
process_args(int argc, char **argv)
{
	char *sub_options = NULL;
	char *name;
	int c;
	int respond_no = 0;	/* -n/-N Respond 'no' to all questions */
	int respond_yes = 0;	/* -y/-Y Respond 'yes' to all questions */

	/*
	 * The supported options depend on how we were invoked (fsck vs.
	 * samfsck). Initialize the supported options list.
	 */
	opt_init();

	while ((c = getopt(argc, argv, opt_string)) != EOF) {
		switch (c) {
		case 's':		/* Specify scratch directory */
			scratch_dir = optarg;
			break;
#ifdef OFFDIRS
		case 'O':		/* Clear off-lines */
			clear_offlines = TRUE;
			break;
#endif /* OFFDIRS */
#ifdef DAMFSCK
		case 'd':		/* Use damage files */
			damage_files = TRUE;
			break;
#endif /* DAMFSCK */
		case 'F':		/* Repair files */
			repair_files = TRUE;
			break;
		case 'D':		/* Debug output */
			debug_print = TRUE;
			break;
		case 'V':		/* Verbose output */
			verbose_print = TRUE;
			break;
		case 'G':		/* Generate hash for dirs */
			hash_dirs = TRUE;
			break;
		case 'R':		/* Rename filesystem in superblock */
			rename_fs = TRUE;
			break;
		case 'p':		/* Ret 0 if FS has not requested fsck */
			preen_fs = TRUE;
			break;
		case 'S':		/* Convert v2 FS to shared FS */
			cvt_to_shared = TRUE;
			break;
		case 'U':		/* Convert shared to unshared v2 FS */
			cvt_to_nonshared = TRUE;
			break;
		case 'n':		/* Respond 'no' to all questions */
		case 'N':
			respond_no = TRUE;
			break;
		case 'y':		/* Respond 'yes' to all questions */
		case 'Y':
			respond_yes = TRUE;
			break;
		case 'o':		/* List of sub-opts (processed later) */
			sub_options = optarg;
			break;
		default:
			return (ES_args);
		}
	}

	/* Validate -o suboptions, if any */
	if (sub_options != NULL) {
		while (sub_options) {
			switch (*sub_options) {
#ifdef OFFDIRS
			case 'O':
				clear_offlines = TRUE;
				break;
#endif /* OFFDIRS */
#ifdef DAMFSCK
			case 'd':
				damage_files = TRUE;
				break;
#endif /* DAMFSCK */
			case 'D':
				debug_print = TRUE;
				break;
			case 'G':
				hash_dirs = TRUE;
				break;
			case 'n':
			case 'N':
				respond_no = TRUE;
				break;
			case 'p':
				preen_fs = TRUE;
				break;
			case 'R':
				rename_fs = TRUE;
				break;
			case 'V':
				verbose_print = TRUE;
				break;
			case 'y':
			case 'Y':
				respond_yes = TRUE;
				break;
			default:
				error(0, 0, catgets(catfd, SET, 13962,
				    "illegal -o option flag -- %c"),
				    *sub_options);
				return (ES_args);
			}
			if (*(++sub_options) == '\0') {
				/* End of list */
				break;
			} else if (*sub_options != ',') {
				/*
				 * Since we don't support any name-value
				 * pairs, must be a ','
				 */
				error(0, 0, catgets(catfd, SET, 13963,
				    "-o option flags must be separated "
				    "by a comma."));
				return (ES_args);
			} else {
				sub_options++;  /* Skip comma */
			}
		}
	}

	/* Make sure we have a mount point or fset name argument */
	if (argc == optind) {
		error(0, 0, catgets(catfd, SET, 13961,
		    "Missing mount point or family set name argument."));
		return (ES_args);
	}
	name = argv[optind++];

	/* Anything after mount point or fset name argument is too much */
	if (argc != optind) {
		error(0, 0, catgets(catfd, SET, 13964,
		    "Too many arguments."));
		return (ES_args);
	}

	/* -n/-N and -y/-Y options are mutually exclusive */
	if (respond_no && respond_yes) {
		error(0, 0, catgets(catfd, SET, 13965,
		    "The -n/-N and -y/-Y options are mutually exclusive."));
		return (ES_args);
	}

	/* The -n/-N and -F options are mutually exclusive */
	if (respond_no && repair_files) {
		error(0, 0, catgets(catfd, SET, 13966,
		    "The -n/-N and -F options are mutually exclusive."));
		return (ES_args);
	}

	/*
	 * Repair files if invoked via fsck wrapper and the user didn't
	 * specify -n/-N.
	 */
	if (fsck_wrapper && !respond_no) {
		repair_files = TRUE;
	}

	/* The -S/-U options require the -F (or equivalent) option */
	if ((cvt_to_shared || cvt_to_nonshared) && !repair_files) {
		error(0, 0, catgets(catfd, SET, 13967,
		    "The -S/-U options require the -F option."));
		return (ES_args);
	}

	/* If scratch directory not specified, use default */
	if (scratch_dir == NULL) {
		scratch_dir = "/tmp";
	}

	/* Determine the family set name to use */
	if (strrchr(name, '/') == NULL) {
		/* The supplied name is not a path name.  Use it. */
		strcpy(fsname, name);
	} else {
		int err;

		/*
		 * If the supplied name starts with '/', it is an absolute
		 * mount point path, so lookup the corresponding family set
		 * name in /etc/vfstab.  Otherwise, as with traditional
		 * fsck, a relative mount point path is not allowed.
		 */
		if ((err = fstab_fsname_get(name, fsname)) != 0) {
			if (err == EINVAL) {
				error(0, 0, catgets(catfd, SET, 13969,
				    "\"%s\" is not an absolute path name."),
				    name);
			} else {
				error(0, 0, catgets(catfd, SET, 13970,
				    "Can't find filesystem name for mount "
				    "point \"%s\"."),
				    name);
			}
			clean_exit(ES_args);
		}
	}

	if (check_mnttab(fsname)) {
		if (!repair_files) {
			error(0, 0,
			    catgets(catfd, SET, 13325,
			"Results are NOT accurate -- the filesystem %s is "
			"mounted."),
			    fsname);
		} else {
			error(0, 0, catgets(catfd, SET, 13421,
			    "filesystem %s is mounted."), fsname);
			clean_exit(1);
		}
	}

	/* All options have been set appropriately */
	return (ES_ok);
}

static int
devinfo_init()
{
	int err = 0;

	if ((err = chk_devices(fsname,
	    repair_files ? O_RDWR : O_RDONLY, &mnt_info)) == 0) {
		d_cache_init(16);
		fs_count = mnt_info.params.fs_count;
		mm_count = mnt_info.params.mm_count;
		mm_ord = 0;
		sprintf(tmp_name, "%s/%d.samfsck", scratch_dir, (int)getpid());
		if ((tmp_fd =
		    open(tmp_name, O_CREAT|O_TRUNC|O_RDWR, 0600)) < 0) {
			err = TRUE;
			error(0, errno, "%s", tmp_name);
			error(0, 0, catgets(catfd, SET, 613,
			    "Cannot open %s"), tmp_name);
		}
	}

	if (err) {
		error(0, 0, catgets(catfd, SET, 722,
		    "Configuration error"));
		return (ES_device);
	}

	return (ES_ok);
}


/*
 * ----- check_fs - Check an existing filesystem.
 * Read file system labels and build the geometry for the disk
 * filesystem in the mount entry.
 */

void			/* ERRNO if error */
check_fs(void)
{
	char	timebuf[40];
	int i;
	int ord;
	struct devlist *devlp;
	uint_t *smbp;
	off_t length;
	int dt;
	int d_dau;
	int needfsck = FALSE;
	char ver_str[20];

	init_dup();		/* Initialize the duplicate file */
	build_devices();	/* Build the devices array in ordinal order */
	quota_tabinit();	/* initialize quota accumulation structures */

	/* Read old superblock in sblock. Copy in nblock for new superblock. */
	devlp = &devp->device[mm_ord];
	i = howmany(L_SBINFO + (fs_count * L_SBORD), SAM_DEV_BSIZE);

	/* Read both superblocks and pick last updated one */
	if (d_read(devlp, (char *)&sblock, i, SUPERBLK)) {
		error(0, 0,
		    catgets(catfd, SET, 2439,
		    "Superblock read failed on eq %d"),
		    devlp->eq);
		clean_exit(ES_io);
	}
	if (((sblock.info.sb.magic != SAM_MAGIC_V1) &&
	    (sblock.info.sb.magic != SAM_MAGIC_V2) &&
	    (sblock.info.sb.magic != SAM_MAGIC_V2A)) ||
	    (strncmp(sblock.info.sb.name, "SBLK", 4) != 0)) {
		error(0, 0,
		    catgets(catfd, SET, 13316,
		    "Superblock on eq %d at block %d missing magic "
		    "and/or name"),
		    devlp->eq, SUPERBLK);
		clean_exit(ES_sblk);
	}

	if (d_read(devlp, (char *)&nblock, i, sblock.info.sb.offset[1]) == 0) {
		if (((nblock.info.sb.magic != SAM_MAGIC_V1) &&
		    (nblock.info.sb.magic != SAM_MAGIC_V2) &&
		    (nblock.info.sb.magic != SAM_MAGIC_V2A)) ||
		    (strncmp(nblock.info.sb.name, "SBLK", 4) != 0)) {
				error(0, 0,
				    catgets(catfd, SET, 13316,
				"Superblock on eq %d at block %d missing "
				"magic and/or name"),
				    devlp->eq, sblock.info.sb.offset[1]);
				clean_exit(ES_sblk);
		}
		if (sblock.info.sb.time == nblock.info.sb.time) {
			memcpy((char *)&sblock, (char *)&nblock,
			    sizeof (sblock));
		}
	}

	/*
	 * Check superblock options mask.  If the option mask version
	 * has advanced beyond this version of samfsck's capability or
	 * the option mask has new bits set, fail the samfsck.
	 */
	if (sblock.info.sb.opt_mask_ver > 0) {	/* Versioning in use */
		if (sblock.info.sb.opt_mask_ver == SBLK_OPT_VER1) {
			if (sblock.info.sb.opt_mask & ~SBLK_ALL_OPTV1) {
				/*
				 * This version of samfsck only understands
				 * SBLK_ALL_OPTV1 options in option mask
				 * version 1.
				 */
				error(0, 0, catgets(catfd, SET, 13299,
				    "ALERT:  Option version or mask mismatch "
				    "(vers %d, mask %x), cannot "
				    "check file system.\n"),
				    sblock.info.sb.opt_mask_ver,
				    sblock.info.sb.opt_mask);
				clean_exit(ES_opt_mask);
			}
		} else {
			/*
			 * This version of samfsck does not
			 * understand option mask versions
			 * greater than SBLK_OPT_VER1.
			 */
			error(0, 0, catgets(catfd, SET, 13299,
			    "ALERT:  Option version or mask mismatch "
			    "(vers %d, mask %x), cannot check file system.\n"),
			    sblock.info.sb.opt_mask_ver,
			    sblock.info.sb.opt_mask);
			clean_exit(ES_opt_mask);
		}
	}

	/*
	 * Now that we know which super block we are using, rename the
	 * the filesystem if it was properly requested and there is a
	 * new family set name in the mcf.
	 */
	if (strncmp(sblock.info.sb.fs_name, fsname, sizeof (uname_t))) {
		if (rename_fs && repair_files) {
			strncpy(sblock.info.sb.fs_name, fsname,
			    sizeof (uname_t));
		} else {

			/* Send sysevent to generate SNMP trap */
			snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(1661),
			    devlp->eq, fsname);
			PostEvent(FS_CLASS, "MismatchEq", 1661, LOG_ERR,
			    msgbuf,
			    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
			error(0, 0, "%s", msgbuf);
			clean_exit(ES_sblk);
		}
	}

	if (sblock.info.sb.magic == SAM_MAGIC_V1) {
		sblk_version = SAMFS_SBLKV1;
		sprintf(ver_str, "%s", "1");
		update_sblk_to_40(&sblock, fs_count, mm_count);
	} else if (sblock.info.sb.magic == SAM_MAGIC_V2) {
		sblk_version = SAMFS_SBLKV2;
		sprintf(ver_str, "%s", "2");
	} else {
		sblk_version = SAMFS_SBLKV2;
		sprintf(ver_str, "%s", "2A");
	}

	if (cvt_to_shared || cvt_to_nonshared) {
		if (sblk_version == SAMFS_SBLKV1) {
			error(0, 0, catgets(catfd, SET, 13925,
			    "-S/-U options not supported for Version 1 FS"));
			clean_exit(ES_args);
		}
		if (cvt_to_shared && sblock.info.sb.hosts != 0) {
			error(0, 0, catgets(catfd, SET, 13931,
			    "shared_fs_convert:  FS already shared"));
			clean_exit(ES_error);
		}
		if (cvt_to_nonshared && sblock.info.sb.hosts == 0) {
			error(0, 0, catgets(catfd, SET, 13932,
			    "shared_fs_convert:  FS already non-shared"));
			clean_exit(ES_error);
		}
	}

	/*
	 * Note any (slice or superblock) bits that indicate an
	 * fsck request, and set needfsck.  Print out info on
	 * slice-specific bits if verbose is set.  Exit with
	 * appropriate code if the 'preen' option (-p) was set.
	 */
	for (i = 0; i < sblock.info.sb.fs_count; i++) {
		char *bits;

		if (sblock.eq[i].fs.fsck_stat & SB_FSCK_ALL) {
			needfsck = TRUE;
		}

		switch (sblock.eq[i].fs.fsck_stat & SB_FSCK_ALL) {
		case SB_FSCK_GEN:
			bits = "gen";
			break;

		case SB_FSCK_SP:
			bits = "sp";
			break;

		case (SB_FSCK_SP | SB_FSCK_GEN):
			bits = "sp+gen";
			break;

		default:
			bits = NULL;
		}
		if (bits && (verbose_print || debug_print)) {
			error(0, 0, catgets(catfd, SET, 13909,
			    "Filesystem %s slice %d/eq %d:  fsck bits = %s"),
			    fsname, i, sblock.eq[i].fs.eq, bits);
		}
	}
	if (verbose_print || debug_print) {
		char *bits;

		switch (sblock.info.sb.state & SB_FSCK_ALL) {
		case SB_FSCK_GEN:
			bits = "gen";
			break;

		case SB_FSCK_SP:
			bits = "sp";
			break;

		case (SB_FSCK_SP | SB_FSCK_GEN):
			bits = "sp+gen";
			break;

		default:
			bits = "none";
		}
		if (bits) {
			error(0, 0, catgets(catfd, SET, 13910,
			    "Filesystem %s superblock:  fsck bits = %s"),
			    fsname, bits);
		}
	}
	if (sblock.info.sb.state & SB_FSCK_ALL) {
		needfsck = TRUE;
	}
	if (needfsck) {
		error(0, 0, catgets(catfd, SET, 13911,
		    "NOTICE: Filesystem %s requires fsck"), fsname);
	}
	if (preen_fs) {
		clean_exit(needfsck ? ES_preen : ES_ok);
	}

	ext_bshift = sblock.info.sb.ext_bshift - SAM_DEV_BSHIFT;
	min_usr_inum = MAX((SAM_LOSTFOUND_INO + 1),
	    sblock.info.sb.min_usr_inum);

	sam_set_dau(&mp->mi.m_dau[DD], sblock.info.sb.dau_blks[SM],
	    sblock.info.sb.dau_blks[LG]);
	sam_set_dau(&mp->mi.m_dau[MM], sblock.info.sb.mm_blks[SM],
	    sblock.info.sb.mm_blks[LG]);
	if (sblock.info.sb.fs_count != fs_count) {

		/* Send sysevent to generate SNMP trap */
		snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(2250),
		    sblock.info.sb.fs_count, fs_count, fsname);
		PostEvent(FS_CLASS, "MismatchSblk", 2250, LOG_ERR, msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		error(0, 0, "%s", msgbuf);
		clean_exit(ES_sblk);

	}

	printf(catgets(catfd, SET, 13466,
	    "name:     %s       version:     %s%10s"),
	    fsname, ver_str, sblock.info.sb.hosts == 0 ? "" :
	    catgets(catfd, SET, 13467, "    shared"));
	printf("\n");

	if (!SBLK_MAPS_ALIGNED(&sblock.info.sb)) {
		/* May have to fix sblk system value for old releases. */
		fix_system();
	}

	memcpy((char *)&nblock, (char *)&sblock, sizeof (sblock));
	nblock.info.sb.space = sblock.info.sb.capacity;
	nblock.info.sb.mm_space = sblock.info.sb.mm_capacity;

	time(&start_time);
	if ((verbose_print || debug_print) && nblock.info.sb.repaired) {
		time_t last_time = nblock.info.sb.repaired;
		strftime(timebuf, sizeof (timebuf)-1, "%c\n",
		    localtime(&last_time));
		printf(catgets(catfd, SET, 13942,
		    "INFO:  FS %s last repaired: %s"),
		    fsname, timebuf);
	}

	get_mem(ES_malloc);

	if ((smbbuf = (char *)malloc(LG_BLK(mp, MM))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 1606,
		    "malloc: %s."),
		    "smbbuf");
		clean_exit(ES_malloc);
	}
	smbptr = (struct sam_inoblk *)smbbuf;
	smbp = (uint_t *)smbbuf;
	*smbp++ = 0xffffffff;
	*smbp = 0;


	/*
	 * Map enough space for bit maps --
	 * (# large blocks times * small blocks in large block)
	 */
	length = 0;
	for (ord = 0, devlp = (struct devlist *)devp; ord < fs_count;
	    ord++, devlp++) {
		sblock.eq[ord].fs.ord = ord;
		sblock.eq[ord].fs.eq = devlp->eq;
		sblock.eq[ord].fs.type = devlp->type;
		nblock.eq[ord].fs.space = sblock.eq[ord].fs.capacity;
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* If NOT first group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				devlp->off = 0;
				devlp->length = 0;
				continue;
			}
		}
		if (sblock.eq[ord].fs.type == DT_META) {
			dt = MM;
		} else {
			dt = DD;
		}
		d_dau = SAM_DEV_BSIZE * SM_BLKCNT(mp, dt);
		devlp->off = (off_t)length;
		devlp->length = (sblock.eq[ord].fs.l_allocmap * d_dau);
		length += ((devlp->length + PAGEOFFSET) & PAGEMASK);
	}
	if (ftruncate(tmp_fd, length) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 645,
		    "Cannot truncate %s: length %d"),
		    tmp_name, length);
		clean_exit(ES_device);
	}
	for (ord = 0, devlp = (struct devlist *)devp; ord < fs_count;
	    ord++, devlp++) {
		if (devlp->state == DEV_OFF || devlp->state == DEV_DOWN) {
			devlp->mm = NULL;
			continue;
		}
		if (is_osd_group(devlp->type)) {
			devlp->mm = NULL;
			continue;
		}
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* If NOT first group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				continue;
			}
		}
		if ((devlp->mm = mmap((caddr_t)NULL, devlp->length,
		    (PROT_WRITE | PROT_READ), MAP_SHARED, tmp_fd,
		    (off_t)devlp->off)) == (char *)MAP_FAILED) {
			error(0, errno,
			    catgets(catfd, SET, 610,
			    "Cannot mmap %s: length %d"),
			    tmp_name, length);
			clean_exit(ES_device);
		}
	}
	/*
	 * Only let server repair a file system for the shared file system.
	 */
	if (repair_files && sblock.info.sb.hosts) {
		upath_t srvr;
		ushort_t hosts_ord = sblock.info.sb.hosts_ord;

		if (!shared_server(&devp->device[hosts_ord],
		    sblock.info.sb.hosts, srvr)) {
			error(0, 0, catgets(catfd, SET, 13326,
			    "Must run samfsck from the metadata server %s"),
			    srvr);
			clean_exit(ES_args);
		}
	}

	/*
	 * Build free bit map.
	 */
	for (ord = 0, devlp = (struct devlist *)devp; ord < fs_count;
	    ord++, devlp++) {
		int mord;
		int err;

		if (devlp->state == DEV_OFF || devlp->state == DEV_DOWN) {
			continue;
		}
		if (is_osd_group(devlp->type)) {
			continue;
		}
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* If NOT first group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				continue;
			}
		}
		if (sblock.eq[ord].fs.type == DT_META) {
			dt = MM;
		} else {
			dt = DD;
		}
		mord = sblock.eq[ord].fs.mm_ord;
		err = sam_bfmap(SAMFSCK_CALLER, &sblock, ord,
		    &devp->device[mord],
		    dcp, devlp->mm, SM_BLKCNT(mp, dt));
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 801,
			    "Dau map write failed on eq %2d, (%s)"),
			    devp->device[mord].eq, devp->device[mord].eq_name);
			clean_exit(ES_error);
		}
	}

	/*
	 * Clear allocated blocks.
	 */
	for (ord = 0, devlp = (struct devlist *)devp; ord < fs_count;
	    ord++, devlp++) {
		int err;
		int len;

		if (devlp->state == DEV_OFF || devlp->state == DEV_DOWN) {
			continue;
		}
		if (is_osd_group(devlp->type)) {
			continue;
		}
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* If NOT first group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				continue;
			}
		}
		if (sblock.eq[ord].fs.type == DT_META) {
			dt = MM;
		} else {
			dt = DD;
		}
		err = sam_cablk(SAMFSCK_CALLER, &sblock, NULL, ord,
		    SM_BLKCNT(mp, dt),
		    SM_BLKCNT(mp, MM), LG_DEV_BLOCK(mp, DD),
		    LG_DEV_BLOCK(mp, MM),
		    &len);
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 13488,
			    "Error %d clearing blocks in bitmap. "
			    "eq %d system len %x computed len %x\n"),
			    err, devlp->eq, sblock.eq[ord].fs.system, len);
			clean_exit(ES_error);
		}
	}

	/*
	 * If present, validate the log, and determine if there
	 * are transactions to replay.
	 */
	lqfs_log_validate(nblock.info.sb.logbno, nblock.info.sb.logord,
	    &note_block);

	printf(catgets(catfd, SET, 13311, "First pass\n"));

	/* Read and validate special inodes for .inodes and .blocks files */
	read_sys_inodes();

#ifdef	DAMFSCK
	if (damage_files) {
		damage_fs();
	}
#endif /* DAMFSCK */

	/*
	 * Allocate internal inode table based on size field in .inodes
	 * inode
	 */
	ino_count = inode_ino->di.rm.size >> SAM_ISHIFT;
	if ((ino_mm = (struct ino_list *)malloc
	    (((ino_count+INO_IN_BLK)*sizeof (struct ino_list)))) == NULL) {
		error(0, 0,
			catgets(catfd, SET, 602,
				"Cannot malloc ino array"));
		clean_exit(ES_malloc);
	}
	memset((char *)ino_mm, 0,
	    (((ino_count+INO_IN_BLK)*sizeof (struct ino_list))));

	pass = FIRST_PASS;
	process_inodes();

	printf(catgets(catfd, SET, 2265, "Second pass\n"));
	pass = SECOND_PASS;
	process_inodes();
	print_duplicates();

	printf(catgets(catfd, SET, 13341, "Third pass\n"));
	pass = THIRD_PASS;
	process_inodes();

	printf("\n");
	printf(catgets(catfd, SET, 1380, "Inodes processed: %d\n"), ino_count);

	/*
	 * If debug print option, print maps and .blocks file
	 */
	if (debug_print) {
		for (ord = 0; ord < fs_count; ord++) {
			if ((sblock.eq[ord].fs.state == DEV_OFF) ||
			    (sblock.eq[ord].fs.state == DEV_DOWN)) {
				continue;
			}
			if (is_osd_group(sblock.eq[ord].fs.type)) {
				continue;
			}
			if (is_stripe_group(sblock.eq[ord].fs.type)) {
				/* If NOT first group member */
				if (sblock.eq[ord].fs.num_group == 0) {
					continue;
				}
			}
			debug_print_blocks(ord);
		}
		debug_print_sm_blocks();
	}

	/*
	 * Build new bit map and update new superblock
	 */
	for (ord = 0; ord < fs_count; ord++) {
		if ((sblock.eq[ord].fs.state == DEV_OFF) ||
		    (sblock.eq[ord].fs.state == DEV_DOWN)) {
			continue;
		}
		if (is_osd_group(sblock.eq[ord].fs.type)) {
			update_block_counts_object(ord);
			continue;
		}
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			/* If NOT first group member */
			if (sblock.eq[ord].fs.num_group == 0) {
				continue;
			}
		}
		write_map(ord);
	}
	if (smb_stop == 0) {
		smb_stop = 1;		/* Done writing to .blocks file */
		write_sm_block();	/* Terminate .blocks */
	}

	if (repair_files) {
		time_t finish_time;
		write_sys_inodes();
		sync_inodes();

		/* clear fsck request bits */
		nblock.info.sb.state &= ~SB_FSCK_ALL;
		for (ord = 0; ord < fs_count; ord++) {
			nblock.eq[ord].fs.fsck_stat &= ~SB_FSCK_ALL;
		}

		if (cvt_to_shared || cvt_to_nonshared) {
			shared_fs_convert(&nblock);
		}

		time(&finish_time);
		nblock.info.sb.repaired = (uint32_t)finish_time;

		if ((sblock.info.sb.opt_mask & SBLK_OPTV1_CONV_WORMV2) != 0) {
			nblock.info.sb.opt_mask |= SBLK_OPTV1_CONV_WORMV2;
		}

		if (write_sblk(&nblock, devp)) {
			clean_exit(ES_io);
		}

		if (verbose_print || debug_print) {
			char start[40], finish[40];

			strftime(start, sizeof (start)-1, "%c\n",
			    localtime(&start_time));
			strftime(finish, sizeof (finish)-1, "%c\n",
			    localtime(&finish_time));

			printf(catgets(catfd, SET, 13943,
			    "INFO:  FS %s repaired:\n\tstart:  %s\t"
			    "finish: %s"),
			    fsname, start, finish);
		}

		if (debug_print) {
			d_cache_printstats();
		}

		{
			sam_defaults_t *defaults = GetDefaults();

			openlog("samfsck", LOG_NOWAIT, defaults->log_facility);
			syslog(LOG_NOTICE, "FS %s: samfsck repair completed.",
			    fsname);
			closelog();
		}

		if ((lg_blocks =
		    (nblock.info.sb.space - sblock.info.sb.space)) > 0) {
			offset_t lg_bytes = ((offset_t)lg_blocks *
			    SAM_DEV_BSIZE);

			printf(catgets(catfd, SET, 1824,
			    "NOTICE: Reclaimed %lld bytes\n"),
			    lg_bytes);
		}
		if (mm_count && (mm_blocks =
		    (nblock.info.sb.mm_space - sblock.info.sb.mm_space)) > 0) {
			offset_t mm_bytes = ((offset_t)mm_blocks *
			    SAM_DEV_BSIZE);

			printf(catgets(catfd, SET, 13312,
			    "NOTICE: Reclaimed %lld meta bytes\n"),
			    mm_bytes);
		}
		printf("\n");

	} else {
		int sblock_counts_wrong = 0;

		for (ord = 0; ord < fs_count; ord++) {
			if ((sblock.eq[ord].fs.state == DEV_OFF) ||
			    (sblock.eq[ord].fs.state == DEV_DOWN)) {
				continue;
			}
			if (is_osd_group(sblock.eq[ord].fs.type)) {
				continue;
			}
			if (is_stripe_group(sblock.eq[ord].fs.type)) {
				/* If NOT first group member */
				if (sblock.eq[ord].fs.num_group == 0) {
					continue;
				}
			}
			(void) check_free_blocks(ord);
		}

		check_sm_free_blocks();

		/*
		 * If debug print option, count bits and print extra
		 * information
		 */
		if (debug_print) {
			for (ord = 0; ord < fs_count; ord++) {
				if ((sblock.eq[ord].fs.state == DEV_OFF) ||
				    (sblock.eq[ord].fs.state == DEV_DOWN)) {
					continue;
				}
				if (is_osd_group(sblock.eq[ord].fs.type)) {
					continue;
				}
				if (is_stripe_group(sblock.eq[ord].fs.type)) {
					/* If NOT first group member */
					if (sblock.eq[ord].fs.num_group == 0) {
						continue;
					}
				}
				(void) debug_count_free_blocks(ord);
			}
			printf("DEBUG: data: capacity  = 0x%llx\t%lld\n",
			    nblock.info.sb.capacity, nblock.info.sb.capacity);
			printf("DEBUG: data: space     = 0x%llx\t%lld\n",
			    sblock.info.sb.space, sblock.info.sb.space);
			printf("DEBUG: data: cal_space = 0x%llx\t%lld\n",
			    nblock.info.sb.space, nblock.info.sb.space);
			printf("DEBUG: data: maps      = 0x%llx\t%lld\n",
			    scount[DD], scount[DD]);
			printf("DEBUG: data: cal_maps  = 0x%llx\t%lld\n",
			    ncount[DD], ncount[DD]);
			if (mm_count) {
				printf("DEBUG: meta: capacity  = "
				    "0x%llx\t%lld\n",
				    nblock.info.sb.mm_capacity,
				    nblock.info.sb.mm_capacity);
				printf("DEBUG: meta: space     = "
				    "0x%llx\t%lld\n",
				    sblock.info.sb.mm_space,
				    sblock.info.sb.mm_space);
				printf("DEBUG: meta: cal_space = "
				    "0x%llx\t%lld\n",
				    nblock.info.sb.mm_space,
				    nblock.info.sb.mm_space);
				printf("DEBUG: meta: maps      = "
				    "0x%llx\t%lld\n",
				    scount[MM], scount[MM]);
				printf("DEBUG: meta: cal_maps  = "
				    "0x%llx\t%lld\n",
				    ncount[MM], ncount[MM]);
			}
			printf("\n");
		}
		if (lg_blocks) {
			printf(catgets(catfd, SET, 13318,
			    "ALERT:  %d large blocks are free (can be "
			    "allocated), "
			    "but also allocated to inodes.\n"),
			    lg_blocks);
		}
		if (sm_blocks) {
			printf(catgets(catfd, SET, 13319,
			    "ALERT:  %d small blocks are free (can be "
			    "allocated), "
			    "but also allocated to inodes.\n"),
			    sm_blocks);
		}
		if (mm_count && mm_blocks) {
			printf(catgets(catfd, SET, 13320,
			    "ALERT:  %d meta blocks are free (can be "
			    "allocated), "
			    "but also allocated to inodes.\n"),
			    mm_blocks);
		}
		if ((lg_blocks = (nblock.info.sb.space -
		    sblock.info.sb.space)) != 0) {
			if (lg_blocks > 0) {
				offset_t blocks = ((offset_t)lg_blocks *
				    SAM_DEV_BSIZE);

				printf(catgets(catfd, SET, 1823,
				    "NOTICE: %lld bytes can be reclaimed\n"),
				    blocks);
				SETEXIT(ES_blocks);
			} else {
				sblock_counts_wrong = 1;
			}
		}
		if (mm_count && (mm_blocks =
		    (nblock.info.sb.mm_space - sblock.info.sb.mm_space)) !=
		    0) {
			if (mm_blocks > 0) {
				offset_t mm_bytes = ((offset_t)mm_blocks *
				    SAM_DEV_BSIZE);

				printf(catgets(catfd, SET, 13313,
				    "NOTICE: %lld meta bytes can be "
				    "reclaimed\n"),
				    mm_bytes);
				SETEXIT(ES_blocks);
			} else {
				sblock_counts_wrong = 1;
			}
		}
		if (sblock_counts_wrong) {
			printf(catgets(catfd, SET, 13328,
			    "NOTICE: Superblock space counts are incorrect, "
			    "\t-F will correct.\n"));
			SETEXIT(ES_sblock);
		}
	}
	if (num_2hash) {
		if (repair_files) {
			printf(catgets(catfd, SET, 13331,
			    "NOTICE: %d directory entries hashed."),
			    num_2hash);
		} else {
			printf(catgets(catfd, SET, 13332,
			    "NOTICE: %d directory entries missing hash. "
			    "Both -F and -G needed to correct.\n"),
			    num_2hash);
		}
	}
}


/*
 * ----- fix_system - build partition array for devices.
 * The number of system blocks may need to be adjusted.  This can occur
 * in filesystems mkfs'ed with an inadequate number of system blocks
 * reserved in the super block due to a rounding problem in mkfs.
 */

void
fix_system(void)
{
	struct devlist *devlp;
	int sblk_size;
	int ord;

	sblk_size = sizeof (struct sam_sblk) >> SAM_DEV_BSHIFT;
	for (ord = 0, devlp = (struct devlist *)devp; ord < fs_count;
	    ord++, devlp++) {
		int len, minlen, sblk_len;

		if (devlp->state == DEV_OFF || devlp->state == DEV_DOWN) {
			continue;
		}
		if (devlp->type == DT_DATA) {
			/* No meta device, bit maps on each disk */
			if (mm_count == 0) {
				/* Zero blocks + super blocks */
				len = SUPERBLK + LG_DEV_BLOCK(mp, DD);
				len += sblock.eq[ord].fs.l_allocmap;
			} else {
				/* Zero blocks + super blocks */
				len = SUPERBLK + sblk_size;
			}
			if (ord == 0) {
				/* Last super block */
				len += LG_DEV_BLOCK(mp, DD);
			}
			minlen = len;
			len = roundup(len, LG_DEV_BLOCK(mp, DD));

		} else if ((devlp->type == DT_META) && (ord == 0)) {
			len = sblock.info.sb.offset[1]; /* Last super block */
			len += LG_DEV_BLOCK(mp, MM);
			minlen = len;
			len = roundup(len, LG_DEV_BLOCK(mp, MM));

		} else if ((devlp->type == DT_META) && (ord != 0)) {
			continue;

		} else {
			len = SUPERBLK + sblk_size;
			minlen = len;
			len = roundup(len, LG_DEV_BLOCK(mp, DD));
		}

		sblk_len = sblock.eq[ord].fs.system;

		if (sblk_len == 0) {	/* correct 3.3.0 stripe groups */
			if (is_stripe_group(devlp->type) && ord != 0) {
				sblock.eq[ord].fs.system = sblk_len =
				    sblock.eq[ord - 1].fs.system;
			}
		}

		if (sblk_len != len) {		/* Sanity check */
			if (sblk_len < minlen) {
				if (sblk_len != OLD_SBSIZE ||
				    LG_DEV_BLOCK(mp, DD) != OLD_SBSIZE) {
					error(0, 0,
					    catgets(catfd, SET, 2252,
					    "Superblock for eq %d: system "
					    "%d != computed len %d"),
					    devlp->eq, sblk_len, len);
					clean_exit(ES_sblk);
				}
			} else {
				/* Reserved for system */
				sblock.eq[ord].fs.system = len;
			}
		}
	}
}


/*
 * ----- build_devices - build partition array for devices.
 * Build devices array in ordinal order. Verify all partitions are present.
 */

void
build_devices(void)
{
	int	ord;
	int sblk_ord;
	int old_count;
	time_t time;
	struct devlist *devlp;
	struct d_list *ndevp;
	struct sam_sblk *sblk;
	int sblk_meta_on = 0;
	int sblk_data_on = 0;
	int meta_on = 0;
	int data_on = 0;


	if ((ndevp = (struct d_list *)malloc(sizeof (struct devlist) *
	    fs_count)) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 604,
		    "Cannot malloc ndevp"));
		clean_exit(ES_malloc);
	}
	memcpy((char *)ndevp, (char *)devp,
	    (sizeof (struct devlist) * fs_count));
	memset((char *)devp, 0, (sizeof (struct devlist) * fs_count));

	/*
	 * Find ordinal 0 for existing filesystem fsname.
	 * Set fs_count and mm_count from superblock.
	 */
	old_count = 0;
	sblk = (struct sam_sblk *)malloc(sizeof (struct sam_sblk));
	if (sblk == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 1606,
		    "malloc: %s\n"), "superblock");
		clean_exit(ES_malloc);
	}
	for (ord = 0, devlp = (struct devlist *)ndevp; ord < fs_count;
	    ord++, devlp++) {
		if (is_osd_group(devlp->type)) {
			continue;
		}
		if (d_read(devlp, (char *)sblk,
		    (sizeof (sam_sblk_t) / SAM_DEV_BSIZE), SUPERBLK)) {
			error(0, 0,
			    catgets(catfd, SET, 2439,
			    "Superblock read failed on eq %d"),
			    devlp->eq);
			clean_exit(ES_io);
		}
		if (((strncmp(sblk->info.sb.fs_name, fsname,
		    sizeof (uname_t)) == 0) ||
		    (rename_fs && repair_files)) &&
		    (sblk->info.sb.ord == 0)) {
			int j;

			old_count = sblk->info.sb.fs_count;
			fs_count = sblk->info.sb.fs_count;
			mm_count = sblk->info.sb.mm_count;
			time = sblk->info.sb.init;
			for (j = 0; j < fs_count; j++) {
				if (sblk->eq[j].fs.state == DEV_ON ||
				    sblk->eq[j].fs.state == DEV_NOALLOC ||
				    sblk->eq[j].fs.state == DEV_UNAVAIL) {
					if (sblk->eq[j].fs.type == DT_META) {
						sblk_meta_on++;
					} else {
						sblk_data_on++;
					}
				}
			}
			break;
		}
	}

	if (old_count == 0) {
		error(0, 0,
		    catgets(catfd, SET, 583,
		    "Cannot find ordinal 0 for filesystem %s"),
		    fsname);
		clean_exit(ES_sblk);
	}
	for (ord = 0, devlp = (struct devlist *)ndevp; ord < fs_count;
	    ord++, devlp++) {
		if (sblk->eq[ord].fs.state == DEV_OFF ||
		    sblk->eq[ord].fs.state == DEV_DOWN) {
			devlp->state = sblk->eq[ord].fs.state;
			continue;
		}
		if (is_osd_group(devlp->type)) {
			if ((read_object(fsname, devlp->oh, ord,
			    SAM_OBJ_SBLK_INO, (char *)&sblock, 0,
			    SAM_DEV_BSIZE))) {
				error(0, 0,
				    catgets(catfd, SET, 2439,
				    "Superblock read failed on eq %d"),
				    devlp->eq);
				clean_exit(ES_io);
			}
		} else {
			if (d_read(devlp, (char *)&sblock, 1, SUPERBLK)) {
				error(0, 0,
				    catgets(catfd, SET, 2439,
				    "Superblock read failed on eq %d"),
				    devlp->eq);
				clean_exit(ES_io);
			}
		}

		if (sblock.info.sb.magic == SAM_MAGIC_V1_RE ||
		    sblock.info.sb.magic == SAM_MAGIC_V2_RE ||
		    sblock.info.sb.magic == SAM_MAGIC_V2A_RE) {
			/*
			 * FS built on host with reversed byte order.
			 */
			snprintf(msgbuf, sizeof (msgbuf), GetCustMsg(13472),
			    fsname);
			error(0, 0, "%s", msgbuf);
			clean_exit(ES_byterev);
		}
		/* Validate label is same for all members & not duplicated. */
		if (((sblock.info.sb.magic != SAM_MAGIC_V1) &&
		    (sblock.info.sb.magic != SAM_MAGIC_V2) &&
		    (sblock.info.sb.magic != SAM_MAGIC_V2A)) ||
		    (strncmp(sblock.info.sb.name, "SBLK", 4) != 0) ||
		    (old_count != sblock.info.sb.fs_count) ||
		    ((strncmp(sblock.info.sb.fs_name, fsname,
		    sizeof (uname_t)) != 0) &&
		    !(rename_fs && repair_files)) ||
		    (time != sblock.info.sb.init)) {
			/* Extra partitions for filesystem fsname */

			devlp->state = DEV_OFF;
			sblk_ord = sblock.info.sb.ord;
			if (sblk->eq[sblk_ord].fs.state == DEV_ON ||
			    sblk->eq[sblk_ord].fs.state == DEV_NOALLOC ||
			    sblk->eq[sblk_ord].fs.state == DEV_UNAVAIL) {
				/* Send sysevent to generate SNMP trap */
				snprintf(msgbuf, sizeof (msgbuf),
				    GetCustMsg(1661), devlp->eq, fsname);
				PostEvent(FS_CLASS, "MismatchEq", 1661, LOG_ERR,
				    msgbuf,
				    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
				error(0, 0, "%s", msgbuf);
				clean_exit(ES_sblk);
			}
		} else {
			/* Old partitions for filesystem fsname */
			if (devp->device[sblock.info.sb.ord].eq != 0) {
				error(0, 0,
				    catgets(catfd, SET, 1627,
				    "mcf eq %d has duplicate ordinal %d in "
				    "filesystem %s"),
				    devlp->eq, sblock.info.sb.ord, fsname);
				clean_exit(ES_sblk);
			}
			memcpy((char *)&devp->device[sblock.info.sb.ord],
			    (char *)devlp,
			    sizeof (struct devlist));
			if (devlp->type == DT_META) {
				meta_on++;
			} else {
				data_on++;
			}
		}
	}

	/*
	 * Check for the correct number of data and metadata devices.
	 * Verify the dev is set.
	 */
	if (data_on != sblk_data_on) {
		error(0, 0, catgets(catfd, SET, 13274,
		    "%s: mcf has missing data devices (%d/%d)"),
		    fsname, data_on, sblk_data_on);
		clean_exit(ES_sblk);
	}
	if (meta_on != sblk_meta_on) {
		error(0, 0, catgets(catfd, SET, 13275,
		    "%s: mcf has missing metadata devices (%d/%d)"),
		    fsname, meta_on, sblk_meta_on);
		clean_exit(ES_sblk);
	}

	/*
	 * Copy the OFF/DOWN devices into the zero entries. Set the state
	 * from the superblock.
	 */
	for (ord = 0, devlp = (struct devlist *)ndevp; ord < fs_count;
	    ord++, devlp++) {
		if (devlp->state == DEV_OFF || devlp->state == DEV_DOWN) {
			int j;

			for (j = 0; j < fs_count; j++) {
				if (devp->device[j].eq == 0) {
					memcpy((char *)&devp->device[j],
					    (char *)devlp,
					    sizeof (struct devlist));
					devp->device[j].state =
					    sblk->eq[ord].fs.state;
					break;
				}
			}
		}
	}
	free((void *)sblk);
	free((void *)ndevp);

	/*
	 * Make sure all members of the storage set are present.
	 */
	for (ord = 0, devlp = (struct devlist *)devp; ord < fs_count;
	    ord++, devlp++) {
		if (devlp->state == DEV_OFF || devlp->state == DEV_DOWN) {
			continue;
		}
		if (devlp->eq == 0) {
			error(0, 0,
			    catgets(catfd, SET, 1628,
			    "mcf eq %d, ordinal %d, not present in "
			    "filesystem %s"),
			    devlp->eq, ord, fsname);
			clean_exit(ES_sblk);
		}
	}
}


/*
 * ----- process inodes - process all inodes
 * Scan through all the inodes and process them depending on pass number.
 * Initialize entries in memory table for valid inodes on first pass.
 * Check directory contents and segment file index and inode linkages on
 * second pass.  Count data blocks on first pass.  Check for duplicate
 * blocks on first and second passes.
 */

int			/* ERRNO if error, 0 if successful */
process_inodes()
{
	char *ip = NULL;
	struct ino_list *inop, *dinop;
	struct sam_perm_inode *dp;
	struct sam_inode_ext *ep;
	ino_t ino;

	if ((ip = (char *)malloc(sizeof (struct sam_perm_inode))) == NULL) {
		error(0, 0, catgets(catfd, SET, 13338,
		    "Cannot malloc inode"));
		clean_exit(ES_malloc);
	}
	bzero(ip, sizeof (struct sam_perm_inode));
	dp = (struct sam_perm_inode *)ip;
	for (ino = 1; ino <= ino_count; ino++) {
		inop = &ino_mm[ino - 1];

		if (pass == FIRST_PASS) {
			if (get_inode(ino, dp) == 1) {		/* EOF */
				error(0, 0, catgets(catfd, SET, 13365,
				    "ALERT:  Zero block number in .inodes "
				    "file at ino %d"),
				    ino);
				clean_exit(ES_inodes);
			}
			if (check_inode(ino, dp)) {
				continue;		/* free inode */
			}

			inop->id = dp->di.id;
			inop->parent_id = dp->di.parent_id;
			if (inop->id.ino == SAM_ROOT_INO) {
				inop->orphan = NOT_ORPHAN;
			} else {
				inop->orphan = ORPHAN;	/* until found */
			}
			if (S_ISDIR(dp->di.mode)) {
				inop->type = DIRECTORY;
			} else if (S_ISEXT(dp->di.mode)) {
				ep = (struct sam_inode_ext *)dp;
				inop->id = ep->hdr.id;
				inop->parent_id = ep->hdr.file_id;
				inop->type = INO_EXTEN;
			} else if (S_ISSEGS(&dp->di)) {
				inop->type = SEG_INO;
			} else if (S_ISSEGI(&dp->di)) {
				inop->type = SEG_INDEX;
				inop->seg_size = dp->di.rm.info.dk.seg_size;
				if (inop->seg_size) {
					inop->seg_lim = (dp->di.rm.size +
					    SAM_SEGSIZE(
					    dp->di.rm.info.dk.seg_size) -
					    1) /
					    SAM_SEGSIZE(
					    dp->di.rm.info.dk.seg_size);
				} else {
					inop->seg_lim = 0;
				}
			} else if (dp->di.rm.ui.flags & RM_OBJECT_FILE) {
				inop->type = INO_OBJECT;
			} else {
				inop->type = REG_FILE;
			}
			inop->prob = OKAY;
			if (dp->di.arch_status) {
				inop->arch = COPIES;
			} else {
				inop->arch = NOCOPY;
			}
			inop->seg_prob = OKAY;
			inop->seg_arch = 0;
			inop->fmt = dp->di.mode & S_IFMT;
			if (S_ISREQ(inop->fmt)) {
				inop->fmt = S_IFREG;
			}
			inop->nblock = dp->di.blocks;
			inop->block_cnt = 0;
			inop->nlink = dp->di.nlink;
			inop->link_cnt = 0;
			inop->hlp = (struct hlp_list *)NULL;

			if (inop->type != INO_OBJECT) {
				count_inode_blocks(dp);
				quota_count_file(dp);
			}

		} else if (pass == SECOND_PASS) {
			if (inop->id.ino != ino) {
				continue;
			}

			if (get_inode(ino, dp) == 1) {		/* EOF */
				error(0, 0, catgets(catfd, SET, 13365,
				    "ALERT:  Zero block number in .inodes "
				    "file at ino %d"),
				    ino);
				clean_exit(ES_inodes);
			}
			if (inop->type == REG_FILE ||
			    inop->type == INO_OBJECT) {
				(void) check_reg_file(dp);
			} else if (inop->type == DIRECTORY) {
				(void) check_dir(dp);
			} else if (inop->type == INO_EXTEN) {
				/*
				 * Skip until we find something useful to do
				 *
				 * ep = (struct sam_inode_ext *)dp;
				 * (void) check_extension_inode(ep);
				 */
				continue;
			} else if (inop->type == SEG_INO) {
				(void) check_reg_file(dp);
				(void) check_seg_inode(dp);
			} else if (inop->type == SEG_INDEX) {
				(void) check_reg_file(dp);
				(void) check_seg_index(dp);
			}
			if (inop->type != INO_OBJECT) {
				count_inode_blocks(dp);
			}

			/*
			 * If the inode has the WORM flag set,
			 * convert it to version 2 if we haven't
			 * performed this yet. Take note if the
			 * conversion is successful as we'll need
			 * to update the Superblock.
			 */
			if (dp->di.status.b.worm_rdonly &&
			    (S_ISREG(dp->di.mode) ||
			    S_ISDIR(dp->di.mode)) &&
			    (dp->di.version == SAM_INODE_VERS_2)) {
				if ((dp->di2.p2flags & P2FLAGS_WORM_V2) == 0) {
					if (repair_files) {
						conv_to_v2inode(dp);
						if ((dp->di2.p2flags &
						    P2FLAGS_WORM_V2) == 0) {
							worm_convert_failed = 1;
							printf(catgets(catfd,
							    SET, 13340,
							    "NOTICE:\tino "
							    "%d.%d: failed to "
							    "convert to "
							    "version 2 "
							    "WORM format\n"),
							    (int)inop->id.ino,
							    inop->id.gen);
						} else {
							put_inode(dp->di.id.ino,
							    dp);
							worm_converted = 1;
						}
					} else if (worm_conv_once == 0) {
						printf(catgets(catfd, SET,
						    13339,
						    "NOTICE:\t-F "
						    "will convert WORM inodes "
						    "to version 2 format\n"));
						worm_conv_once = 1;
					}
				}
			}

		} else if (pass == THIRD_PASS) {

			if (inop->id.ino != ino) {
				continue;
			}

			if (inop->type == REG_FILE ||
			    inop->type == DIRECTORY) {
				if (inop->prob == INVALID_DIR ||
				    inop->prob == INVALID_BLK ||
				    inop->prob == DUPLICATE_BLK) {
					(void) get_inode(inop->id.ino, dp);
					offline_inode(inop->id.ino, dp);
				}

			} else if (inop->type == SEG_INO) {
				if (inop->prob == INVALID_BLK ||
				    inop->prob == DUPLICATE_BLK) {
					(void) get_inode(inop->id.ino, dp);
					offline_inode(inop->id.ino, dp);
				}

				if (inop->parent_id.ino >= min_usr_inum &&
				    inop->parent_id.ino <= ino_count) {
					/*
					 * Segment inode status dependent on
					 * index and siblings
					 */
					dinop = &ino_mm[inop->parent_id.ino -
					    1];

					/*
					 * Check segment index and siblings
					 * for fatal error
					 */
					if (dinop->prob == INVALID_INO ||
					    dinop->seg_prob == INVALID_INO) {
						inop->prob = INVALID_INO;
					}
				}

			} else if (inop->type == SEG_INDEX) {
				if (inop->prob == INVALID_SEG ||
				    inop->prob == INVALID_BLK ||
				    inop->prob == DUPLICATE_BLK) {
					(void) get_inode(inop->id.ino, dp);
					offline_inode(inop->id.ino, dp);
				}

				/*
				 * Segment index status is also dependent
				 * on segment inodes
				 */
				if (inop->seg_prob == INVALID_INO) {
					inop->prob = INVALID_INO;
				}

			} else if (inop->type == INO_EXTEN) {
				if (inop->orphan == ORPHAN) {
					inop->prob = INVALID_INO;
				}

				if (inop->parent_id.ino >= min_usr_inum &&
				    inop->parent_id.ino <= ino_count) {
					/*
					 * Extension inode status also
					 * dependent on base inode
					 */
					dinop = &ino_mm[inop->parent_id.ino -
					    1];

					if (dinop->prob == INVALID_INO) {
						inop->prob = INVALID_INO;
					}
				}
			}

			if (inop->nblock != inop->block_cnt &&
			    inop->prob == OKAY) {
				inop->prob = BAD_BLKCNT;
			}

			if (inop->prob == INVALID_INO) {
				if (repair_files) {
					(void) get_inode(inop->id.ino, dp);
					if (inop->type != INO_OBJECT) {
						quota_uncount_file(dp);
						/* uncount the blocks */
						count_inode_blocks(dp);
					}
					free_inode(inop->id.ino, dp);
				}
				print_inode_prob(inop);
				continue;	/* can't be an orphan */
			}

			/* bad HLP extension inode */
			if (inop->prob == INVALID_EXT) {
				/*
				 * Call update_hard_link_parent() with id
				 * of base ino.
				 */
				if (repair_files) {
					struct ino_list *pinop =
					    &ino_mm[inop->parent_id.ino - 1];

					(void) get_inode(pinop->id.ino, dp);
					update_hard_link_parent(dp,
					    pinop->hlp);
				}
				continue;
			}

			print_inode_prob(inop);

			if ((inop->type == REG_FILE) ||
			    (inop->type == DIRECTORY) ||
			    (inop->type == SEG_INO)) {

				/*
				 * Rewrite inode to correct invalid block
				 * count
				 */
				if (inop->block_cnt != inop->nblock) {
					if (repair_files) {
						printf(catgets(catfd, SET,
						    13366,
						    "NOTICE: ino %d.%d,\t"
						    "Repaired block count "
						    "from %d to %d\n"),
						    (int)inop->id.ino,
						    inop->id.gen,
						    inop->nblock,
						    inop->block_cnt);
						(void) get_inode(inop->id.ino,
						    dp);
						dp->di.blocks =
						    inop->block_cnt;
						put_inode(inop->id.ino, dp);
						quota_block_adj(dp,
						    inop->nblock,
						    inop->block_cnt);
					} else {
						printf(catgets(catfd, SET,
						    13367,
						    "NOTICE: ino %d.%d,\t-F "
						    "will repair block "
						    "count from %d to %d\n"),
						    (int)inop->id.ino,
						    inop->id.gen,
						    inop->nblock,
						    inop->block_cnt);
					}
				}
			}



	/* N.B. Bad indentation here to meet cstyle requirements */
	if (offline_dirs == 0) {
		if ((inop->type == REG_FILE) ||
		    (inop->type == DIRECTORY) ||
		    (inop->type == SEG_INDEX)) {

			/*
			 * Update stale parent id with
			 * alternate hard link parent
			 */
			if (inop->orphan == ORPHAN) {
				if (inop->hlp && inop->hlp->n_ids) {
					struct hlp_list *hlp = inop->hlp;

					/*
					 * Promote a hard link parent to be
					 * new parent id
					 */
					hlp->n_ids--;
					inop->parent_id = hlp->ids[hlp->n_ids];
					hlp->ids[hlp->n_ids].ino =
					    hlp->ids[hlp->n_ids].gen = 0;

					if (repair_files) {
						printf(catgets(catfd, SET,
						    13363,
						    "NOTICE: ino %d.%d,"
						    "\tRepaired parent id "
						    "to %d.%d\n"),
						    (int)inop->id.ino,
						    inop->id.gen,
						    (int)inop->parent_id.ino,
						    inop->parent_id.gen);
						(void) get_inode(inop->id.ino,
						    dp);
						dp->di.parent_id =
						    inop->parent_id;
						put_inode(inop->id.ino, dp);
					} else {
						printf(catgets(catfd, SET,
						    13364,
						    "NOTICE: ino %d.%d,\t"
						    "-F will repair parent "
						    "id to %d.%d\n"),
						    (int)inop->id.ino,
						    inop->id.gen,
						    (int)inop->parent_id.ino,
						    inop->parent_id.gen);
					}
					inop->orphan = NOT_ORPHAN;
				}
			}

			/*
			 * Rewrite inode to correct invalid
			 * link counts
			 */
			if (inop->link_cnt != inop->nlink) {
				/* assume fix needed */
				int fix_link = TRUE;

				/*
				 * .blocks and .hosts (v 2)are hidden files
				 * (no name).
				 * .shlock (v2 +) is a hidden file (no name).
				 */
				if (ino == SAM_BLK_INO) {
					fix_link = FALSE;
				} else if ((sblk_version >= SAMFS_SBLKV2) &&
				    (ino == SAM_HOST_INO)) {
					fix_link = FALSE;
				} else if ((sblk_version >= SAMFS_SBLKV2) &&
				    (nblock.info.sb.min_usr_inum ==
				    SAM_MIN_USER_INO) &&
				    (ino == SAM_SHFLOCK_INO)) {
					fix_link = FALSE;

				/*
				 * Don't set link count to 0 for orphans.
				 * make_orphan() would remove them below.
				 */
				} else if ((inop->orphan == ORPHAN) &&
				    (inop->link_cnt == 0)) {
					fix_link = FALSE;

				/*
				 * Skip fix up if this directory will be
				 * orphaned
				 * below and link count is off by 1, since
				 * add_orphan()
				 * will inc nlink for the directory by 1.
				 */
				} else if ((inop->orphan == ORPHAN) &&
				    (inop->type == DIRECTORY) &&
				    (inop->nlink == (inop->link_cnt + 1))) {
					fix_link = FALSE;
				}
				if (fix_link) {
					if (repair_files) {
						printf(catgets(catfd, SET,
						    13361,
						    "NOTICE: ino %d.%d,"
						    "\tRepaired link count "
						    "from %d to %d\n"),
						    (int)inop->id.ino,
						    inop->id.gen,
						    inop->nlink,
						    inop->link_cnt);
						(void) get_inode(inop->id.ino,
						    dp);
						dp->di.nlink = inop->link_cnt;
						put_inode(inop->id.ino, dp);
					} else {
						printf(catgets(catfd, SET,
						    13362,
						    "NOTICE: ino %d.%d,"
						    "\t-F will repair link "
						    "count from %d to %d\n"),
						    (int)inop->id.ino,
						    inop->id.gen,
						    inop->nlink,
						    inop->link_cnt);
					}
				}
			}
		}
	}

			/*
			 * Make inode an orphan if valid with no parent or
			 * parent didn't know inode. Skip orphan processing
			 * for the "mat" file system.
			 */
			if ((!orphan_full) && (inop->id.ino >= min_usr_inum)) {
				if ((mnt_info.params.fi_type !=
				    DT_META_OBJ_TGT_SET) &&
				    (inop->orphan == ORPHAN)) {
					(void) get_inode(inop->id.ino, dp);
					make_orphan(inop->id.ino, dp);
				}
			}
		}
	}
	if (pass == THIRD_PASS) {
	/*
	 * If we've encountered and successfully
	 * converted all the WORM inodes
	 * then update the superblock so
	 * we can see this on subsequent mounts.
	 */
		if (worm_converted && (worm_convert_failed == 0) &&
		    ((sblock.info.sb.opt_mask &
		    SBLK_OPTV1_CONV_WORMV2) == 0)) {
			sblock.info.sb.opt_mask |=
			    SBLK_OPTV1_CONV_WORMV2;
		}

	/*
	 * repair quota files if any.
	 */
		check_quota(repair_files);
	}
	free((void *)ip);
	return (0);
}


/*
 * ----- check_inode - check inode
 * Check inode content and put invalid or empty inodes on the free list.
 */

int			/* -1 if error, 0 success, 1 if inode free/invalid */
check_inode(
	ino_t ino,			/* Inode number */
	struct sam_perm_inode *dp)	/* Inode entry */
{
	int err = 0;
	char *s1 = NULL, *s2 = NULL;

	/*
	 * Inodes in file system type "mat" with parent of SAM_OBJ_ORPHANS_INO
	 * are freed.
	 */
	if (mnt_info.params.fi_type == DT_META_OBJ_TGT_SET) {
		if (dp->di.parent_id.ino == SAM_OBJ_ORPHANS_INO) {
			err++;
			goto out;
		}
	}

	/*
	 * Regular files on an object file system should all be of type
	 * object (unless di.status.b.meta is set).  Likewise,
	 * regular files on a block file system should all be non-object.
	 */
	if (S_ISREG(dp->di.mode)) {
		if (mnt_info.params.fi_type == DT_META_OBJECT_SET) {
			/* Object (mb) file system */
			if (dp->di.status.b.meta) {
				/* Extended attribute file */
				if (dp->di.rm.ui.flags & RM_OBJECT_FILE) {
					/* Object flag set, should be clear */
					err++;
					s1 = "set";
					s2 = "clear";
				}
			} else {
				/* Object file */
				if ((dp->di.rm.ui.flags & RM_OBJECT_FILE)
				    == 0) {
					/* Object flag clear, should be set */
					err++;
					s1 = "clear";
					s2 = "set";
				}
			}
		} else {
			/* Block (non-mb) file system */
			if (dp->di.rm.ui.flags & RM_OBJECT_FILE) {
				/* Object flag set, should be clear */
				err++;
				s1 = "set";
				s2 = "clear";
			}
		}
	}
	if (err) {
		printf(catgets(catfd, SET, 13958,
		    "ALERT:  ino %d.%d, Object flag %s, should be %s%s, "
		    "meta_flag %d, size %lld\n"), (int)dp->di.id.ino,
		    dp->di.id.gen, s1, s2,
		    repair_files ? "" : " (-F to repair)",
		    dp->di.status.b.meta, dp->di.rm.size);
		goto out;
	}

	/* Process free inodes. Rebuild ino free link list */
	if (dp->di.mode == 0 || dp->di.id.ino == 0) {
		err++;
		goto out;
	}

	/* Check validity of inode */
	if ((dp->di.id.ino != ino) || (dp->di.id.ino > ino_count)) {
		err++;
		goto out;
	}

	if ((dp->di.id.ino != SAM_ROOT_INO) &&
	    (dp->di.id.ino == dp->di.parent_id.ino) &&
	    (dp->di.blocks == 0)) {
		err++;
		goto out;
	}

	if (dp->di.version != sblk_version) {
		if (sblk_version == SAMFS_SBLKV2 &&
		    (dp->di.version != sblk_version &&
		    dp->di.version != (sblk_version-1))) {
			if (repair_files) {
				printf(catgets(catfd, SET, 13900,
				    "NOTICE:  ino %d.%d,\tRepaired inode"
				    " version from %d to %d\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    dp->di.version, sblk_version);
				dp->di.version = sblk_version;
				put_inode(ino, dp);
			} else {
				printf(catgets(catfd, SET, 13901,
				    "NOTICE:  ino %d.%d,\t-F will repair inode "
				    "version from %d to %d\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    dp->di.version, sblk_version);
			}
		}
	}

	if (dp->di.rm.size < 0) {
		if (ino < min_usr_inum) {
			error(0, 0, catgets(catfd, SET, 13917,
			    "ALERT:  Invalid system inode %d with size %lld"),
			    ino, dp->di.rm.size);
			clean_exit(ES_inodes);
		}
		if (repair_files) {
			printf(catgets(catfd, SET, 13918,
			    "ALERT:  ino %d.%d,\tFreed inode with "
			    "size %lld\n"),
			    (int)dp->di.id.ino, dp->di.id.gen,
			    dp->di.rm.size);
		} else {
			printf(catgets(catfd, SET, 13919,
			    "ALERT:  ino %d.%d,\t-F will free inode with "
			    "size %lld\n"),
			    (int)dp->di.id.ino, dp->di.id.gen,
			    dp->di.rm.size);
		}
		err++;
		goto out;
	}

	/*
	 * If directory is damaged or zero size, free it.
	 * If segment index is damaged or zero size, free it.
	 */
	if (S_ISDIR(dp->di.mode) || S_ISSEGI(&dp->di)) {
		if (dp->di.status.b.damaged) {
			if (S_ISDIR(dp->di.mode)) {
				printf(catgets(catfd, SET, 13355,
				    "NOTICE:  ino %d.%d,\tDirectory is "
				    "damaged\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
			} else {
				printf(catgets(catfd, SET, 13356,
				    "NOTICE:  ino %d.%d,\tSegment index is "
				    "damaged\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
			}
			SETEXIT(ES_alert);
			err++;
			goto out;
		}
		if (S_ISDIR(dp->di.mode) && (dp->di.rm.size == 0)) {
			printf(catgets(catfd, SET, 13357,
			    "NOTICE:  ino %d.%d,\tDirectory size is 0\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			err++;
			goto out;
		} else if (S_ISSEGI(&dp->di) &&
		    (dp->di.rm.info.dk.seg.fsize == 0)) {
			printf(catgets(catfd, SET, 13358,
			    "NOTICE:  ino %d.%d,\tSegment index size is 0\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			err++;
			goto out;
		}
		if (dp->di.status.b.offline) {
#ifdef OFFDIRS
			if (clear_offlines) {
				if (repair_files) {
					if (S_ISDIR(dp->di.mode)) {
						printf(catgets(catfd, SET,
						    13333,
						"NOTICE: Offline directory: "
						"ino %d has been removed\n"),
						    (int)dp->di.id.ino);
					} else {
						printf(catgets(catfd, SET,
						    13335,
						    "NOTICE: Offline segment "
						    "index: ino %d has been "
						    "removed\n"),
						    (int)dp->di.id.ino);
					}
				} else {
					if (S_ISDIR(dp->di.mode)) {
						printf(catgets(catfd, SET,
						    13334,
						    "NOTICE: Offline "
						    "directory: -F will "
						    "remove ino %d\n"),
						    (int)dp->di.id.ino);
					} else {
						printf(catgets(catfd, SET,
						    13336,
						    "NOTICE: Offline "
						    "segment index: -F will "
						    "remove ino %d\n"),
						    (int)dp->di.id.ino);
					}
				}
				err++;
				goto out;
			} else
#else /* !OFFDIRS */
			if (S_ISDIR(dp->di.mode)) {
				printf(catgets(catfd, SET, 13905,
				    "NOTICE: Offline directory: ino "
				    "%d.%d detected\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
			} else {
				printf(catgets(catfd, SET, 13906,
				    "NOTICE: Offline segment index: "
				    "ino %d.%d detected\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
			}
#endif /* OFFDIRS */
			if (!orphan_full) {
				orphan_full = 1;
				if (S_ISDIR(dp->di.mode)) {
					printf(catgets(catfd, SET, 13310,
					    "NOTICE: Orphan processing "
					    "stopped due to offline "
					    "directory. \n"
					    "        Mount filesystem and "
					    "stage all directories.\n"));
				} else {
					printf(catgets(catfd, SET, 13337,
					    "NOTICE: Orphan processing "
					    "stopped due to offline segment "
					    "index. \n"
					    "        Mount filesystem and "
					    "stage all segment indices.\n"));
				}
				SETEXIT(ES_orphan);
			}
		}
	}

out:
	if (err) {
		free_inode(ino, dp);
	}
	return (err);
}


/*
 * ----- count_inode_blocks - count inode blocks
 * First pass, count and validate the blocks, start duplicate lists.
 * Second pass, check for duplicates and add to lists.
 * Third pass, uncount blocks and remove inode num from duplicate lists.
 */

void
count_inode_blocks(struct sam_perm_inode *dp)	/* Inode entry */
{
	ino_t ino = dp->di.id.ino;
	sam_daddr_t bn;
	int ord;
	int ii;
	int dt;
	int err = 0;
	int excess_blks = 0;
	int excess_msg = 0;
	int inval_blks = 0;
	int ioerr_blks = 0;

	/* Don't check inodes that don't have blocks */
	if ((nblock.info.sb.magic == SAM_MAGIC_V2) ||
	    (nblock.info.sb.magic == SAM_MAGIC_V2A)) {
		if (S_ISLNK(dp->di.mode)) {
			return;
		}
		if (S_ISREQ(dp->di.mode)) {
			return;
		}
		if (S_ISREG(dp->di.mode) && (dp->di.status.b.meta == 0) &&
		    (dp->di.rm.ui.flags & RM_OBJECT_FILE)) {
			return;		/* No blocks for object files */
		}
	}
	if (S_ISEXT(dp->di.mode)) {
		return;		/* No blocks for extensions */
	}

	if (pass == FIRST_PASS) {
		offset_t size;
		sam_daddr_t lastbn = 0;
		int lastord = 0;
		int lastflag = 0;
		struct ino_list *inop;

		if (S_ISREQ(dp->di.mode)) {
			size = dp->di.psize.rmfile;
		} else if (S_ISSEGI(&dp->di)) {
			size = dp->di.rm.info.dk.seg.fsize;
		} else {
			size = dp->di.rm.size;
		}

		/* Check that there are no extent blocks past EOF */
		if (size == 0) {
			lastflag++;
		} else {
			if (get_bn(dp, (size-1), &lastbn, &lastord, 0) < 0) {
				ioerr_blks++;
			}
		}

		/*
		 * Count allocated blocks and save in ino table entry
		 * for pass three
		 */
		inop = &ino_mm[ino - 1];



	/* N.B. Bad indentation here to meet cstyle requirements */
	if (dp->di.status.b.direct_map) {
		sam_daddr_t bn0;

		bn   = dp->di.extent[0];
		bn <<= ext_bshift;
		if (bn != 0) {
			bn0 = bn;
			ord = dp->di.extent_ord[0];
			dt = dp->di.status.b.meta;
			while (dp->di.extent[1] > (bn - bn0)) {
				/*
				 * Count all blocks for a direct_map
				 * file
				 * regardless of rm.size.
				 */
				if (count_block(ino, dt, LG, bn,
				    ord) == 0) {
					inop->block_cnt +=
					    (mp->mi.m_dau[dt].blocks[LG] *
					    devp->device[ord].num_group);
				} else {
					inval_blks++;
				}
				bn += LG_DEV_BLOCK(mp, dt);
			}
		}
	} else {

		for (ii = 0; ii < NOEXT; ii++) {
			bn   = dp->di.extent[ii];
			bn <<= ext_bshift;
			if (bn == 0) {
				continue;
			}
			ord = dp->di.extent_ord[ii];
			if (ii < NDEXT) {
				int bt;

				dt = dp->di.status.b.meta;
				bt = LG;
				if (ii < NSDEXT) {
					bt = dp->di.status.b.on_large ?
					    LG : SM;
				}
				if (lastflag) {
					/* ALERT on first msg only */
					if (excess_msg == 0) {
						if (repair_files) {
							printf(catgets(catfd,
							    SET, 13368,
							    "NOTICE:  ino "
							    "%d.%d,\t"
							    "Released excess "
							    "data block "
							    "0x%llx eq %d\n"),
							    (int)dp->di.id.ino,
							    dp->di.id.gen,
							    (sam_offset_t)bn,
							    devp->device[
							    ord].eq);
						} else {
							printf(catgets(catfd,
							    SET, 13369,
							    "NOTICE:  ino "
							    "%d.%d,\t-F "
							    "will release "
							    "excess data "
							    "block 0x%llx "
							    "eq %d\n"),
							    (int)dp->di.id.ino,
							    dp->di.id.gen,
							    (sam_offset_t)bn,
							    devp->device[
							    ord].eq);
						}
						excess_msg++;
					} else {
						if (verbose_print ||
						    debug_print) {
							printf("DEBUG:  "
							    "\t . Excess "
							    "data block "
							    "0x%llx eq %d\n",
							    (sam_offset_t)bn,
							    devp->device[
							    ord].eq);
						}
					}

					/* Clear excess data block */
					dp->di.extent[ii] =
					    dp->di.extent_ord[ii] = 0;
					excess_blks++;
				} else {
					if (count_block(ino, dt, bt, bn,
					    ord) == 0) {
						inop->block_cnt +=
						    (mp->mi.m_dau[
						    dt].blocks[bt] *
						    devp->device[
						    ord].num_group);
					} else {
						inval_blks++;
					}
				}
				if (lastbn &&
				    (bn == lastbn && ord == lastord)) {
					lastflag++;
				}
			} else {
				/* Clear indirect block */
				if (lastflag) {
					dp->di.extent[ii] =
					    dp->di.extent_ord[ii] = 0;
					excess_blks++;
				}
				err = count_indirect_blocks(dp, bn,
				    ord, (ii - NDEXT),
				    lastbn, lastord, &lastflag,
				    &excess_msg);
				if (err > 0) {
					inval_blks++;
				}
				if (err < 0) {
					ioerr_blks++;
				}
			}
		}
	}



	} else if (pass == SECOND_PASS) {

		if (dp->di.status.b.direct_map) {
			sam_daddr_t bn0;

			bn   = dp->di.extent[0];
			bn <<= ext_bshift;
			if (bn != 0) {
				bn0 = bn;
				ord = dp->di.extent_ord[0];
				dt = dp->di.status.b.meta;
				while (dp->di.extent[1] > (bn - bn0)) {
					if (check_duplicate(ino, dt, LG,
					    bn, ord) > 0) {
						inval_blks++;
					}
					bn += LG_DEV_BLOCK(mp, dt);
				}
			}
		} else {
			for (ii = 0; ii < NOEXT; ii++) {
				int bt;

				bn   = dp->di.extent[ii];
				bn <<= ext_bshift;
				if (bn == 0) {
					continue;
				}
				ord = dp->di.extent_ord[ii];
				if (ii < NDEXT) {
					dt = dp->di.status.b.meta;
					bt = LG;
					if (ii < NSDEXT) {
						bt = dp->di.status.b.on_large ?
						    LG : SM;
					}
					if (check_duplicate(ino, dt, bt,
					    bn, ord) > 0) {
						inval_blks++;
					}
				} else {
					err = count_indirect_blocks(dp, bn,
					    ord, (ii - NDEXT),
					    0, 0, NULL, NULL);
					if (err > 0) {
						inval_blks++;
					}
					if (err < 0) {
						ioerr_blks++;
					}
				}
			}
		}

	} else if (pass == THIRD_PASS) {

		if (dp->di.status.b.direct_map) {
			sam_daddr_t bn0;

			bn   = dp->di.extent[0];
			bn <<= ext_bshift;
			if (bn != 0) {
				bn0 = bn;
				ord = dp->di.extent_ord[0];
				dt = dp->di.status.b.meta;
				while (dp->di.extent[1] > (bn - bn0)) {
					(void) count_block(ino, dt, LG, bn,
					    ord);
					bn += LG_DEV_BLOCK(mp, dt);
				}
			}
		} else {
			for (ii = 0; ii < NOEXT; ii++) {
				int bt;

				bn   = dp->di.extent[ii];
				bn <<= ext_bshift;
				if (bn == 0) {
					continue;
				}
				ord = dp->di.extent_ord[ii];
				if (ii < NDEXT) {
					dt = dp->di.status.b.meta;
					bt = LG;
					if (ii < NSDEXT) {
						bt = dp->di.status.b.on_large ?
						    LG : SM;
					}
					(void) count_block(ino, dt, bt, bn,
					    ord);
				} else {
					(void) count_indirect_blocks(dp, bn,
					    ord, (ii - NDEXT),
					    0, 0, NULL, NULL);
				}
			}
		}
	}

	if (excess_blks) {
		if (repair_files) {
			put_inode(ino, dp);
		}
	}
	if (inval_blks) {
		mark_inode(ino, INVALID_BLK);
	} else if (ioerr_blks) {
		mark_inode(ino, IO_ERROR);
	}
}


/*
 * ----- count_indirect_blocks - count indirect blocks
 * First pass, count and validate the blocks, start duplicate lists.
 * Second pass, check for duplicates and add to lists.
 * Third pass, uncount blocks and remove inode num from duplicate lists.
 */

int				/* -1 if error, 0 if okay, 1 if invalid blk */
count_indirect_blocks(
	struct sam_perm_inode *dp,	/* Inode entry */
	sam_daddr_t bn,			/* mass storage extent block number */
	int ord,			/* mass storage extent ordinal */
	int level,			/* level of indirection */
	sam_daddr_t lastbn,		/* block number for EOF check */
	int lastord,			/* ordinal of block for EOF check */
	int *lastflag,			/* indicates that last block found */
	int *excess_msg)		/* ind first excess msg displayed */
{
	char *ibuf = NULL;
	sam_indirect_extent_t *iep;
	int dt;
	int ii;
	sam_daddr_t ibn;
	int iord;
	int err = 0;
	int excess_blks = 0;
	int inval_blks = 0;
	int ioerr_blks = 0;

	if (check_bn(dp->di.id.ino, bn, ord)) {
		return (1);
	}

	if ((ibuf = (char *)malloc(LG_BLK(mp, MM))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 601,
		    "Cannot malloc indirect block"));
		clean_exit(ES_malloc);
	}

	if (d_read(&devp->device[ord], (char *)ibuf,
	    LG_DEV_BLOCK(mp, MM), bn)) {
		printf(catgets(catfd, SET, 13388,
		    "ALERT:  ino %d,\tError reading indirect block "
		    "0x%llx on eq %d\n"),
		    (int)dp->di.id.ino, (sam_offset_t)bn,
		    devp->device[ord].eq);
		SETEXIT(ES_error);
		free((void *)ibuf);
		return (-1);
	}

	if (mm_count) {
		dt = MM;
	} else {
		dt = DD;
	}
	iep = (sam_indirect_extent_t *)ibuf;

	if (pass == FIRST_PASS) {
		struct ino_list *inop;

		if (verify_indirect_validation(dp, bn, ord, level, iep)) {
			free((void *)ibuf);
			return (1);
		}

		/* Clear the indirect block if beyond EOF */
		if (*lastflag) {
			if (repair_files) {
				printf(catgets(catfd, SET, 13370,
				    "NOTICE:  ino %d.%d,\tReleased excess "
				    "indirect block 0x%llx eq %d\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    (sam_offset_t)bn, devp->device[ord].eq);
			} else {
				printf(catgets(catfd, SET, 13371,
				    "NOTICE:  ino %d.%d,\t-F will release "
				    "excess indirect block 0x%llx eq %d\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    (sam_offset_t)bn, devp->device[ord].eq);
			}
		} else {
			if (count_block(dp->di.id.ino, dt, LG, bn, ord) > 0) {
				inval_blks++;
			}
		}

		/*
		 * Count allocated blocks and save in ino table entry
		 * for pass three
		 */
		inop = &ino_mm[dp->di.id.ino - 1];

		for (ii = 0; ii < DEXT; ii++) {
			ibn   = iep->extent[ii];
			ibn <<= ext_bshift;
			if (ibn == 0) {
				continue;
			}
			iord = (int)iep->extent_ord[ii];


			if (level) {
				if (*lastflag) { /* Clear indirect block */
					iep->extent[ii] =
					    iep->extent_ord[ii] = 0;
					excess_blks++;
				}
				err = count_indirect_blocks(dp, ibn, iord,
				    (level - 1),
				    lastbn, lastord, lastflag, excess_msg);
				if (err > 0) {
					inval_blks++;
				}
				if (err < 0) {
					ioerr_blks++;
				}
			} else {


	/* N.B. Bad indentation here to meet cstyle requirements */
	if (*lastflag) {
		/* ALERT on first msg only */
		if (*excess_msg == 0) {
			if (repair_files) {
				printf(catgets(catfd,
				    SET, 13368,
				    "NOTICE:  ino %d.%d,\tReleased excess "
				    "data block 0x%llx eq %d\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    (sam_offset_t)ibn, devp->device[iord].eq);
			} else {
				printf(catgets(catfd, SET, 13369,
				    "NOTICE:  ino %d.%d,\t-F will release "
				    "excess "
				    "data block 0x%llx eq %d\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    (sam_offset_t)ibn, devp->device[iord].eq);
			}
			(*excess_msg)++;
		} else {
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t. Excess data block "
				    "0x%llx eq %d\n",
				    (sam_offset_t)ibn, devp->device[iord].eq);
			}
		}

		/* Clear excess data block */
		iep->extent[ii] = iep->extent_ord[ii] = 0;
		excess_blks++;
	} else {
		dt = dp->di.status.b.meta;
		if (count_block(dp->di.id.ino, dt,
		    LG, ibn, iord) == 0) {
			inop->block_cnt += (mp->mi.m_dau[dt].blocks[LG] *
			    devp->device[iord].num_group);
		} else {
			inval_blks++;
		}
	}



				if (lastbn &&
				    (ibn == lastbn && iord == lastord)) {
					(*lastflag)++;
				}
			}
		}

	} else if (pass == SECOND_PASS) {

		/* Check for duplicate indirect block */
		if (check_duplicate(dp->di.id.ino, dt, LG, bn, ord) > 0) {
			inval_blks++;
		}

		for (ii = 0; ii < DEXT; ii++) {
			ibn   = iep->extent[ii];
			ibn <<= ext_bshift;
			if (ibn == 0) {
				continue;
			}
			iord = (int)iep->extent_ord[ii];
			if (level) {
				err = count_indirect_blocks(dp, ibn, iord,
				    (level - 1),
				    0, 0, NULL, NULL);
				if (err > 0) {
					inval_blks++;
				}
				if (err < 0) {
					ioerr_blks++;
				}
			} else {
				dt = dp->di.status.b.meta;
				if (check_duplicate(dp->di.id.ino, dt, LG,
				    ibn, iord) > 0) {
					inval_blks++;
				}
			}
		}

	} else if (pass == THIRD_PASS) {

		/* Uncount the indirect block */
		(void) count_block(dp->di.id.ino, dt, LG, bn, ord);

		for (ii = 0; ii < DEXT; ii++) {
			ibn   = iep->extent[ii];
			ibn <<= ext_bshift;
			if (ibn == 0) {
				continue;
			}
			iord = (int)iep->extent_ord[ii];
			if (level) {
				(void) count_indirect_blocks(dp, ibn, iord,
				    (level - 1),
				    0, 0, NULL, NULL);
			} else {
				dt = dp->di.status.b.meta;
				(void) count_block(dp->di.id.ino, dt, LG,
				    ibn, iord);
			}
		}
	}

	if (excess_blks) {
		if (repair_files) {
			if (d_write(&devp->device[ord], (char *)ibuf,
			    LG_DEV_BLOCK(mp, MM), bn)) {
				error(0, 0,
				    catgets(catfd, SET, 13398,
				    "Write failed on eq %d at block 0x%llx"),
				    devp->device[ord].eq, (sam_offset_t)bn);
				SETEXIT(ES_error);
			}
		}
	}
	free((void *)ibuf);

	if (inval_blks) {
		return (1);
	} else if (ioerr_blks) {
		return (-1);
	}

	return (0);
}


/*
 * ----- count_block - count block
 * Clear the corresponding bit for the block and count it.
 */

int			/* -1 if error, 0 if okay, 1 if invalid block */
count_block(
	ino_t ino,		/* I-number */
	int dt,			/* Data or meta device */
	int bt,			/* Small or large block */
	sam_daddr_t bn,		/* Block number */
	int ord)		/* Disk ordinal */
{
	struct devlist *devlp;
	char *cptr;
	sam_u_offset_t bit;
	sam_u_offset_t sbit;
	uint_t offset;
	uint_t *wptr;
	uint_t mask;

	if (check_bn(ino, bn, ord)) {
		return (1);
	}
	devlp = &devp->device[ord];
	/* ino dt (meta or data) must match device type */
	if (dt != (devlp->type == DT_META)) {
		if (pass == FIRST_PASS) {
			printf(catgets(catfd, SET, 13289,
			"ALERT:  ino %d,\tblock 0x%llx ord %d dt %d "
			"mismatch type %d\n"),
			    (int)ino, (sam_offset_t)bn, ord, dt, devlp->type);
		}
		return (1);
	}

	/*
	 * devlp->mm must not be NULL - bit map must be available.
	 * Internal error if NULL.  Most likely trying to count blocks
	 * on an object file.
	 */
	if (devlp->mm == NULL) {
		printf(catgets(catfd, SET, 13290,
		    "ALERT:  ino %d,\tblock 0x%llx ord %d dt %d "
		    "devlp->mm NULL\n"), (int)ino, (sam_offset_t)bn, ord, dt);
		return (-1);
	}

	cptr = devlp->mm;
	bit = bn;

	/*
	 * If small daus (SM_BLKCNT > 1) are supported for this device, the
	 * working bit maps (devlp->mm) are expanded to include SM_BLKCNT bits
	 * per large DAU.
	 */
	if (SM_BLKCNT(mp, dt) > 1) {	/* If this device has small daus */
		bit = bn >> DIF_SM_SHIFT(mp, dt);
		sbit = bit;
		bit = bit & ~(SM_DEV_BLOCK(mp, dt) - 1); /* large dau start */
	} else {
		if (mp->mi.m_dau[dt].dif_shift[bt]) {
			bit >>= mp->mi.m_dau[dt].dif_shift[bt];
		} else {
			bit /= mp->mi.m_dau[dt].kblocks[bt];
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
	if (pass == FIRST_PASS || pass == SECOND_PASS) {

		if ((*wptr & mask) == 0) {
			(void) check_duplicate(ino, dt, bt, bn, ord);
			return (0);
		}
		*wptr &= ~mask;

	} else if (pass == THIRD_PASS) {

		if ((*wptr & mask) == 0) {
			/* Others left */
			if (check_duplicate(ino, dt, bt, bn, ord) > 0) {
				return (0);
			}
		}
		*wptr |= mask;
	}

	return (0);
}


/*
 * ----- check_bn - check block
 *
 */

int					/* -1 if error, 0 if successful. */
check_bn(
	ino_t ino,			/* inode */
	sam_daddr_t bn,			/* block number */
	int ord)			/* ordinal */
{
	if ((ord < 0) || (ord >= fs_count)) {
		if (pass == FIRST_PASS) {
			printf(catgets(catfd, SET, 13392,
			"ALERT:  ino %d,\tblock 0x%llx ord %d exceeds "
			"max ordinal %d\n"),
			    (int)ino, (sam_offset_t)bn, ord, fs_count);
		}
		return (-1);
	}
	if (bn >= (sam_daddr_t)nblock.eq[ord].fs.capacity) {
		if (pass == FIRST_PASS) {
			printf(catgets(catfd, SET, 13386,
			    "ALERT:  ino %d,\tblock 0x%llx exceeds "
			    "capacity on eq %d\n"),
			    (int)ino, (sam_offset_t)bn, devp->device[ord].eq);
		}
		return (-1);
	}
	if (bn < (sam_daddr_t)nblock.eq[ord].fs.system) {
		if (pass == FIRST_PASS) {
			printf(catgets(catfd, SET, 13387,
			    "ALERT:  ino %d,\tblock 0x%llx in system area "
			    "on eq %d\n"),
			    (int)ino, (sam_offset_t)bn, devp->device[ord].eq);
		}
		return (-1);
	}
	return (0);
}


/*
 * ----- get_bn - get a block from a inode.
 * Get block, ord, given logical byte offset and inode pointer.
 */

int					/* -1 if error, 0 if successful */
get_bn(
	struct sam_perm_inode *dp,	/* inode pointer */
	offset_t offset,		/* Logical byte offset */
	sam_daddr_t *bn,		/* Block -- returned */
	int *ord,			/* Ordinal -- returned */
	int correct)			/* Apply correction 1=yes, 0=no */
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

	if (dp == NULL) {
		error(0, 0, catgets(catfd, SET, 319,
		    ".inodes pointer not set"));
		clean_exit(1);
	}
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
	if (sam_cmd_get_extent(ip, offset, sblk_version, &de, &dt, &bt,
	    kptr, &ileft)) {
		printf(catgets(catfd, SET, 13981,
		    "NOTICE:  ino %d.%d,\tFile size %lld exceeds "
		    "maximum allowed "
		    "for configured DAU size.\n"),
		    (int)dp->di.id.ino, dp->di.id.gen, offset+1);
		return (-1);
	}
	bnp = &dp->di.extent[de];
	eip = &dp->di.extent_ord[de];
	if (de >= NDEXT) {
		int ii, kk;
		sam_daddr_t tmp_sbn;

		ii = de - NDEXT;
		for (kk = 0; kk <= ii; kk++) {
			if (*bnp == 0) {
				break;
			}
			iep = (sam_indirect_extent_t *)ibufp;
			tmp_sbn   = *bnp;
			tmp_sbn <<= ext_bshift;
			if ((sbn != tmp_sbn) || (sord != (int)*eip)) {
				sbn = tmp_sbn;
				sord = (int)*eip;
				if (d_read(&devp->device[sord], (char *)ibufp,
				    LG_DEV_BLOCK(mp, MM), sbn)) {
					error(0, 0, catgets(catfd, SET, 1375,
					    "Ino %d read failed on eq %d"),
					    dp->di.id.ino,
					    devp->device[sord].eq);
					SETEXIT(ES_error);
					return (-1);
				}

				if (pass == FIRST_PASS) {
					if (verify_indirect_validation(dp,
					    sbn, sord,
					    (ii - kk), iep)) {
						return (-1);
					}
				}
			}
			bnp = &iep->extent[kptr[kk]];
			eip = &iep->extent_ord[kptr[kk]];
		}
	}
	*bn   = *bnp;
	*bn <<= ext_bshift;
	*ord  = *eip;
	if (*bn == 0) {
		return (-1);
	}
	if (correct && (bt != SM)) {
		offset_t off_corr;
		struct devlist *dip;

		off_corr = offset;

		/*
		 * of 16k, 32k and 64k daus, 64k needs a more detailed
		 * block number correction.
		 */
		if (mp->mi.m_dau[dt].size[bt] > mp->mi.m_dau[dt].sm_off) {
			if (!dp->di.status.b.on_large) {
				off_corr -= mp->mi.m_dau[dt].sm_off;
			}
		}

		/*
		 * May have to modify ordinal returned for stripe groups.
		 */
		dip = (struct devlist *)&devp->device[*ord];
		if (dip->num_group > 1) {
			*ord += (off_corr / mp->mi.m_dau[dt].size[bt]) %
			    dip->num_group;
		}

		if (off_corr % mp->mi.m_dau[dt].size[bt]) {
			*bn += (off_corr % mp->mi.m_dau[dt].size[bt]) >>
			    SAM_DEV_BSHIFT;
		}
	}
	return (0);
}


/*
 * ----- check_duplicate - check duplicate
 * Scan the existing duplicates and if a match on first or second pass,
 * add this ino, else make a new entry.  Remove the inode on third
 * pass and report the number of inodes left holding the block.
 */

int				/* -1 if error, 0 if okay, 1 if invalid blk */
check_duplicate(
	ino_t ino,		/* I-number */
	int dt,			/* Data or meta device */
	int bt,			/* Small or large block */
	sam_daddr_t bn,		/* Block number */
	int ord)		/* Disk ordinal */
{
	struct dup_inoblk *smp;
	int add;
	int i;
	int count;

	if (check_bn(ino, bn, ord)) {
		return (1);
	}
	add = 0;
	for (smp = (struct dup_inoblk *)dup_mm; smp <= dup_last; smp++) {

		if (pass == FIRST_PASS || pass == SECOND_PASS) {

			if (smp->bn == DUP_END) {	/* End of list */
				smp->count = 0;
				smp->free = 0;
				if (add) {
					break;
				}
				if (pass == FIRST_PASS) {
					break;
				}
				return (0);
			} else if ((smp->bn ==
			    (bn & ~(SM_DEV_BLOCK(mp, dt) - 1))) &&
			    (smp->ord == ord)) {
				if (bt == SM) {
					if (!(smp->free & (1 << ((bn  &
					    (SM_DEV_BLOCK(mp, dt) - 1)) >>
					    DIF_SM_SHIFT(mp, dt))))) {
						return (0);
					}
				}
				count = (int)smp->count;
				/* Ino already added */
				for (i = 0; i < count; i++) {
					if (smp->ino[i] == ino) {
						return (0);
					}
				}
				/* If room in entry */
				if (count < SM_INOCOUNT) {
					break;
				}
				/* Make new entry because cur entry is full */
				add = 1;

			}

		} else if (pass == THIRD_PASS) {

			if (smp->bn == DUP_END) {
				return (0);		/* End of list */
			}
			if ((smp->bn == (bn & ~(SM_DEV_BLOCK(mp, dt) - 1))) &&
			    (smp->ord == ord)) {
				if (bt == SM) {
					if (!(smp->free & (1 << ((bn  &
					    (SM_DEV_BLOCK(mp, dt) - 1)) >>
					    DIF_SM_SHIFT(mp, dt))))) {
						return (0);
					}
				}
				for (i = 0; i < (int)smp->count; i++) {
					/* Inode found */
					if (smp->ino[i] == ino) {
						smp->ino[i] = 0;
						/* last on the list */
						if ((int)--smp->count == 0) {
							smp->bn = 0;
							smp->ord = 0;
							smp->free = 0;
							smp->btype = 0;
							smp->dtype = 0;
						}
						/* Dup inodes left */
						return ((int)smp->count);
					}
				}
			}
		}
	}
	if (pass == THIRD_PASS) {
		return (0);				/* Not a dup block */
	}

	count = (int)smp->count;
#ifdef	ABORT
	if (count >= SM_INOCOUNT) {
		printf("smp=0x%x\n", (uint_t)smp);
		printf(catgets(catfd, SET, 1026,
		    "ALERT:  Error count>=%d:ino=%d, count=%d, free=0x%x, "
		    "dtype=%d, btype=%d, 0x%x.%d\n"),
		    SM_INOCOUNT, (int)ino, (int)count, (int)smp->free,
		    (int)smp->dtype, (int)smp->btype, bn, ord);
		abort();
	}
#endif	/* ABORT */
	smp->ino[count++] = ino;
	smp->count = count;
	if (count >= 2) {
		return (0);
	}
	smp->bn = bn;
	if (bt == SM) {
		smp->bn = bn & ~(SM_DEV_BLOCK(mp, dt) - 1);
	}
	smp->ord = ord;
	smp->btype = bt;
	smp->dtype = dt;
	if (bt == SM) {
		smp->free |= 1 << ((bn  & (SM_DEV_BLOCK(mp, dt) - 1)) >>
		    DIF_SM_SHIFT(mp, dt));
	} else {
		smp->free = SM_BITS(mp, dt);
	}
	cur_length += sizeof (struct dup_inoblk);
	if ((cur_length + sizeof (struct dup_inoblk)) >= dup_length) {
		extend_dup();
		smp = (struct dup_inoblk *)(dup_mm + cur_length);
	} else {
		smp++;
	}
	smp->bn = DUP_END;
	smp->ord = 0;
	smp->free = 0;
	smp->btype = 0;
	smp->dtype = 0;
	smp->count = 0;
	dup_last = smp;
	return (0);
}


/*
 * ----- init_dup - Initialize the duplicate block file.
 */

void
init_dup(void)
{
	struct dup_inoblk *smp;

	sprintf(dup_name, "%s/%d.dup_blks", scratch_dir, (int)getpid());
	if ((dup_fd = open(dup_name, O_CREAT|O_TRUNC|O_RDWR, 0600)) < 0) {
		error(0, errno, "%s", dup_name);
		error(0, 0,
		    catgets(catfd, SET, 613,
		    "Cannot open %s"),
		    dup_name);
		clean_exit(ES_device);
	}
	if (ftruncate(dup_fd, dup_length) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 645,
		    "Cannot truncate %s: length %d"),
		    dup_name, dup_length);
		clean_exit(ES_device);
	}
	if ((dup_mm = mmap((caddr_t)NULL, dup_length, (PROT_WRITE | PROT_READ),
	    MAP_SHARED, dup_fd, (off_t)0)) == (char *)MAP_FAILED) {
		error(0, errno,
		    catgets(catfd, SET, 610,
		    "Cannot mmap %s: length %d"),
		    dup_name, dup_length);
		clean_exit(ES_device);
	}
	smp = (struct dup_inoblk *)dup_mm;
	smp->bn = DUP_END;
	dup_last = (struct dup_inoblk *)dup_mm;
}


/*
 * ----- extend_dup - Extend duplicate block file.
 */

void
extend_dup(void)
{

	munmap(dup_mm, dup_length);
	dup_length = dup_length << 1;
	if (ftruncate(dup_fd, dup_length) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 645,
		    "Cannot truncate %s: length %d"),
		    dup_name, dup_length);
		clean_exit(ES_device);
	}
	if ((dup_mm = mmap((caddr_t)NULL, dup_length, (PROT_WRITE | PROT_READ),
	    MAP_SHARED, dup_fd, (off_t)0)) == (char *)MAP_FAILED) {
		error(0, errno,
		    catgets(catfd, SET, 610,
		    "Cannot mmap %s: length %d"),
		    dup_name, dup_length);
		clean_exit(ES_device);
	}
}


/*
 * ----- write_map
 * Build the new bit maps from the allocated blocks and existing inodes.
 */

void
write_map(int ord)	/* Disk ordinal */
{
	int dt;
	int ii;
	uint_t *optr;
	uint_t *iptr;
	int daul;
	int nbit;
	int bit;
	int kk;
	int ll;
	struct devlist *devlp;
	uint_t mask;
	int blocks;
	sam_daddr_t	bn;
	int mmord;
	offset_t allocmap;
	offset_t allocmap_min;

	devlp = &devp->device[ord];
	if (devlp->type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}
	mmord = sblock.eq[ord].fs.mm_ord;
	daul = sblock.eq[ord].fs.l_allocmap;	/* number of blocks */
	blocks = sblock.eq[ord].fs.dau_size;	/* no. of bits */
	bn = 0;
	iptr = (uint_t *)devlp->mm;

	/*
	 * Verify allocation bitmap disk offset.
	 */
	allocmap = sblock.eq[ord].fs.allocmap;
	if (sblock.info.sb.mm_count == 0) {
		allocmap_min = SUPERBLK + LG_DEV_BLOCK(mp, DD);
		if (SBLK_MAPS_ALIGNED(&sblock.info.sb)) {
			allocmap_min = roundup(allocmap_min,
			    LG_DEV_BLOCK(mp, DD));
		}
	} else {
		allocmap_min = SUPERBLK + (sizeof (sam_sblk_t) /
		    SAM_DEV_BSIZE);
		if (SBLK_MAPS_ALIGNED(&sblock.info.sb)) {
			allocmap_min = roundup(allocmap_min,
			    LG_DEV_BLOCK(mp, MM));
		}
	}
	if (allocmap < allocmap_min) {
		error(0, 0,
		    catgets(catfd, SET, 13487,
		    "ALERT:  Basic checks of allocation maps failed.\n"
		    "        File system may be critically damaged.\n"
		    "        Invalid allocation map eq %d disk offset %lldK "
		    "mimimum %lldK.\n"),
		    sblock.eq[ord].fs.eq, allocmap, allocmap_min);
		clean_exit(ES_error);
	}

	for (ii = 0; ii < daul; ii++) {
		optr = (uint_t *)dcp;
		for (ll = 0; ll < (SAM_DEV_BSIZE / NBPW); ll++) {
			*optr++ = (uint_t)0xffffffff;
		}
		optr = (uint_t *)dcp;
		nbit = 0;
		for (kk = 0; kk < (SAM_DEV_BSIZE / NBPW); kk++, optr++) {
			for (bit = 31; bit >= 0; bit--) {
				blocks--;
				mask = SM_BITS(mp, dt) << (31 - nbit -
				    (SM_BLKCNT(mp, dt) - 1));
				if ((*iptr & mask) != mask) {
					int mmm;
					*optr = sam_clrbit(*optr, bit);


		/* N.B. Bad indentation here to meet cstyle requirements */
		if (blocks >= 0) {
			if (dt == MM) {
				nblock.info.sb.mm_space -=
				    nblock.info.sb.mm_blks[LG];
				nblock.eq[ord].fs.space -=
				    nblock.info.sb.mm_blks[LG];
			} else {
				int space = nblock.info.sb.dau_blks[LG];
				if (sblock.eq[ord].fs.num_group > 1) {
					space *= sblock.eq[ord].fs.num_group;
				}
				nblock.info.sb.space -= space;
				nblock.eq[ord].fs.space -= space;
			}
			for (mmm = 0, ll = 0;
			    ll < SM_BLKCNT(mp, dt);
			    ll++) {
				if (*iptr & (1 << (31 - nbit - ll))) {
					mmm |= (1 << ll);
				}
			}
			if (mmm != 0) {
				build_sm_block(bn,
				    ord, mmm);
			}
		}


				}
				bn += LG_DEV_BLOCK(mp, dt);
				nbit += SM_BLKCNT(mp, dt);
				if (nbit == 32) {
					iptr++;
					nbit = 0;
				}
			}
		}
		if (repair_files) {
			if (d_write(&devp->device[mmord], (char *)dcp, 1,
			    (sblock.eq[ord].fs.allocmap + ii))) {
				error(0, 0,
				    catgets(catfd, SET, 800,
				    "Dau map write failed on eq %d"),
				    devp->device[ord].eq);
				clean_exit(ES_io);
			}
		}
	}
}


/*
 * ----- update_block_counts_object
 * Update superblock block counts from OSN counts.  Only for object.
 */

void
update_block_counts_object(int ord)	/* Disk ordinal */
{
	struct sam_mount_info *mp = &mnt_info;
	struct sam_fs_part *fsp = &mp->part[ord];
	offset_t used;

	/*
	 * space & capacity returned from OSN by get_object_fs_attributes()
	 * in chk_devices().
	 */
	used = fsp->pt_capacity - fsp->pt_space;
	nblock.info.sb.space -= used;
	nblock.eq[ord].fs.space -= used;
}


/*
 * ----- build_sm_block - Build .blocks file.
 * Write entry with free small blocks mask.
 */

void
build_sm_block(
	sam_daddr_t sbn,	/* Small Block */
	int sord,		/* Disk ordinal */
	int mask)		/* Mask of small blocks to mark free */
{
	if (smb_stop) {
		return;
	}
	smbptr->bn = sbn >> ext_bshift;
	smbptr->ord = sord;
	smbptr->free = mask;
	smbptr++;
	smb_off += sizeof (struct sam_inoblk);
	if (smb_off == LG_BLK(mp, MM)) {
		write_sm_block();
	}
	smbptr->bn = 0xffffffff;
}


/*
 * ----- write_sm_block - Write .blocks file.
 * Get block and write out .blocks data.
 */

void
write_sm_block()
{
	sam_daddr_t bn, nbn;
	int ord, nord;

	if (get_bn(block_ino, smb_offset, &bn, &ord, 1) ||
	    check_bn(block_ino->di.id.ino, bn, ord)) {
		smb_stop = 1;
		return;
	}
	smb_offset += LG_BLK(mp, MM);
	if (get_bn(block_ino, smb_offset, &nbn, &nord, 1) ||
	    check_bn(block_ino->di.id.ino, nbn, nord)) {
		if (smb_stop == 0) {
			smbptr--;
			smbptr->bn = 0xffffffff;
		}
		smb_stop = 1;
	}
	if (repair_files) {
		if (d_write(&devp->device[ord], (char *)smbbuf,
		    LG_DEV_BLOCK(mp, MM), bn)) {
			error(0, errno,
			    catgets(catfd, SET, 316,
			    ".blocks write failed on eq %d"),
			    devp->device[ord].eq);
			clean_exit(ES_io);
		}
	}
	smb_off = 0;
	memset(smbbuf, 0, LG_BLK(mp, MM));
	smbptr = (struct sam_inoblk *)smbbuf;
	smbptr->bn = 0xffffffff;
}


/*
 * ----- read_sys_inodes - read system inodes
 * Read and validate special inodes for .inodes and .blocks files.
 */

void
read_sys_inodes()
{
	sam_daddr_t bn;
	int ord;

	/* Save first block of system area so .inodes & .blocks can be read */
	bn = sblock.info.sb.inodes;	/* First block of .inodes file */
	ord = 0;			/* Ordinal of first block in .inodes */

	if (d_read(&devp->device[ord], (char *)first_sm_blk,
	    SM_DEV_BLOCK(mp, MM), bn)) {
		error(0, 0,
		    catgets(catfd, SET, 13390,
		    "Read failed in .inodes on eq %d at block 0x%llx"),
		    devp->device[ord].eq, (sam_offset_t)bn);
		clean_exit(ES_io);
	}
	inode_ino = (struct sam_perm_inode *)first_sm_blk;
	block_ino = (struct sam_perm_inode *)(first_sm_blk +
	    (sizeof (struct sam_perm_inode) * (SAM_BLK_INO - 1)));

	/* Sanity check the .inodes and .blocks inodes */
	if (inode_ino->di.id.ino != SAM_INO_INO ||
	    inode_ino->di.id.gen != SAM_INO_INO ||
	    block_ino->di.id.ino != SAM_BLK_INO ||
	    block_ino->di.id.gen != SAM_BLK_INO ||
	    !(SAM_CHECK_INODE_VERSION(inode_ino->di.version)) ||
	    !(SAM_CHECK_INODE_VERSION(block_ino->di.version)) ||
	    inode_ino->di.rm.size == 0 ||
	    block_ino->di.rm.size == 0) {
		error(0, 0,
		    catgets(catfd, SET, 13317,
		    "Basic checks of .inodes and/or .blocks failed"));
		clean_exit(ES_inodes);
	}

	/* Validate superblock version and inode version */
	if ((sblk_version <= SAM_INODE_VERS_2 &&
	    (inode_ino->di.version != sblk_version) &&
	    (block_ino->di.version != sblk_version)) ||
	    (sblk_version == SAM_INODE_VERSION &&
	    (inode_ino->di.version != sblk_version &&
	    inode_ino->di.version != (sblk_version-1)) ||
	    (block_ino->di.version != sblk_version &&
	    block_ino->di.version != (sblk_version-1)))) {
		error(0, 0,
		    catgets(catfd, SET, 13317,
		    "Basic checks of .inodes and/or .blocks failed"));
		clean_exit(ES_inodes);
	}

	/* Validate extents for .inodes file */
	if (check_sys_inode_blocks(inode_ino)) {
		error(0, 0,
		    catgets(catfd, SET, 13317,
		    "Basic checks of .inodes and/or .blocks failed"));
		clean_exit(ES_inodes);
	}
}


/*
 * ----- check_sys_inode_blocks - check system inode blocks
 *
 * Verify that size and count of blocks in extents are in agreement
 * for special system inodes.  This code is a simplified version
 * of pass one of the count_inode_blocks() routine.
 */

int						/* -1 if error, 0 if okay */
check_sys_inode_blocks(struct sam_perm_inode *dp)	/* Inode entry */
{
	uint_t block_cnt = 0;
	sam_daddr_t bn;
	int ord;
	int dt;
	ino_t first_ino;
	offset_t size;

	/* Count blocks contained in the extents */
	if (dp->di.status.b.direct_map) {
		sam_daddr_t bn0;

		bn   = dp->di.extent[0];
		bn <<= ext_bshift;
		if (bn != 0) {
			bn0 = bn;
			ord = dp->di.extent_ord[0];
			dt = dp->di.status.b.meta;
			while (dp->di.extent[1] > (bn - bn0)) {
				if (check_bn(dp->di.id.ino, bn, ord)) {
					return (-1);
				}

				/* verify contents of block */
				if (dp->di.id.ino == SAM_INO_INO) {
					first_ino = SAM_DTOI(
					    ((offset_t)block_cnt * SAM_BLK));
					if (verify_inode_block(bn, ord,
					    first_ino)) {
						break;
					}
				}

				block_cnt += (mp->mi.m_dau[dt].blocks[LG] *
				    devp->device[ord].num_group);
				bn += LG_DEV_BLOCK(mp, dt);
			}
		}
	} else {
		int lastflag = 0;
		sam_daddr_t lastbn = 0;
		int lastord = 0;
		int ii;

		/*
		 * Determine block number and ordinal of last block
		 * for EOF checks
		 */
		if (dp->di.rm.size <= 0) {
			lastflag++;
		} else {
			if (get_bn(dp, (dp->di.rm.size-1), &lastbn,
			    &lastord, 0)) {
				return (-1);
			}
		}

		for (ii = 0; ii < NOEXT; ii++) {
			bn   = dp->di.extent[ii];
			bn <<= ext_bshift;
			if (bn == 0) {
				continue;
			}
			ord = dp->di.extent_ord[ii];
			if (ii < NDEXT) {
				int bt;

				bt = LG;
				if (ii < NSDEXT) {
					bt = dp->di.status.b.on_large ?
					    LG : SM;
				}
				if (check_bn(dp->di.id.ino, bn, ord)) {
					return (-1);
				}
				if (lastflag) {

					/* verify contents of excess block */
					if (dp->di.id.ino == SAM_INO_INO) {
						first_ino = SAM_DTOI(
						    ((offset_t)block_cnt *
						    SAM_BLK));
						if (verify_inode_block(bn,
						    ord, first_ino)) {
							break;
						}
					}
				}
				dt = dp->di.status.b.meta;
				block_cnt += (mp->mi.m_dau[dt].blocks[bt] *
				    devp->device[ord].num_group);
				if (lastbn && (bn == lastbn &&
				    ord == lastord)) {
					lastflag++;
				}
			} else {
				if (check_sys_indirect_blocks(dp, bn, ord,
				    (ii - NDEXT),
				    lastbn, lastord, &lastflag,
				    &block_cnt)) {
					return (-1);
				}
			}
		}
	}

	/*
	 * Make sure special system file size matches block count size.
	 */
	size = (offset_t)block_cnt * SAM_BLK;
	if (dp->di.rm.size != size) {
		if (repair_files) {
			printf(catgets(catfd, SET, 13907,
			    "NOTICE: ino %d.%d,\tRepaired file size "
			    "from %lld to %lld\n"),
			    (int)dp->di.id.ino, dp->di.id.gen,
			    dp->di.rm.size, size);
		} else {
			printf(catgets(catfd, SET, 13908,
			    "NOTICE: ino %d.%d,\t-F will repair file "
			    "size from %lld to %lld\n"),
			    (int)dp->di.id.ino, dp->di.id.gen,
			    dp->di.rm.size, size);
		}
		dp->di.rm.size = size;
		dp->di.blocks = block_cnt;
	}
	return (0);
}


/*
 * ----- check_sys_inode_indirect_blocks - check system inode indirect blocks
 * Handle indirect blocks for special system inodes.  This code is
 * a simplified version of pass one of the count_indirect_blocks() routine.
 */

int					/* -1 if error, 0 if okay */
check_sys_indirect_blocks(
	struct sam_perm_inode *dp,	/* Inode entry */
	sam_daddr_t bn,			/* mass storage extent block number */
	int ord,			/* mass storage extent ordinal */
	int level,			/* level of indirection */
	sam_daddr_t lastbn,		/* block number for EOF check */
	int lastord,			/* ordinal of block for EOF check */
	int *lastflag,			/* indicates that last block found */
	uint_t *block_cnt)		/* count of blocks */
{
	char *ibuf = NULL;
	sam_indirect_extent_t *iep;
	int err = 0;
	int ii;
	sam_daddr_t ibn;
	int iord;
	ino_t first_ino;

	if (check_bn(dp->di.id.ino, bn, ord)) {
		return (-1);
	}

	if ((ibuf = (char *)malloc(LG_BLK(mp, MM))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 601,
		    "Cannot malloc indirect block"));
		clean_exit(ES_malloc);
	}

	if (d_read(&devp->device[ord], (char *)ibuf,
	    LG_DEV_BLOCK(mp, MM), bn)) {
		printf(catgets(catfd, SET, 13388,
		    "ALERT:  ino %d,\tError reading indirect block "
		    "0x%llx on eq %d\n"),
		    (int)dp->di.id.ino, (sam_offset_t)bn,
		    devp->device[ord].eq);
		free((void *)ibuf);
		return (-1);
	}
	iep = (sam_indirect_extent_t *)ibuf;

	/* Check indirect block validation header */
	if (verify_indirect_validation(dp, bn, ord, level, iep)) {
		free((void *)ibuf);
		return (1);
	}

	/* Count allocated blocks in indirect block */
	for (ii = 0; ii < DEXT; ii++) {
		ibn   = iep->extent[ii];
		ibn <<= ext_bshift;
		if (ibn == 0) {
			continue;
		}
		iord = (int)iep->extent_ord[ii];
		if (level) {
			err = check_sys_indirect_blocks(dp, ibn, iord,
			    (level - 1),
			    lastbn, lastord, lastflag,
			    block_cnt);
			if (err) {
				free((void *)ibuf);
				return (err);
			}
		} else {
			int dt;

			dt = dp->di.status.b.meta;
			if (check_bn(dp->di.id.ino, ibn, iord)) {
				free((void *)ibuf);
				return (-1);
			}
			if (*lastflag) {

				/* verify contents of excess block */
				if (dp->di.id.ino == SAM_INO_INO) {
					first_ino = SAM_DTOI(
					    ((offset_t)(*block_cnt) *
					    SAM_BLK));
					if (verify_inode_block(ibn, iord,
					    first_ino)) {
						break;
					}
				}
			}
			*block_cnt += (mp->mi.m_dau[dt].blocks[LG] *
			    devp->device[iord].num_group);
			if (lastbn && (ibn == lastbn && iord == lastord)) {
				(*lastflag)++;
			}
		}
	}

	free((void *)ibuf);
	return (0);
}


/*
 * ----- verify_indirect_validation - verify indirect validation
 * Check that validation header in indirect block belongs to inode
 * and that level is correct.
 */

int					/* -1 if error, 0 if okay */
verify_indirect_validation(
	struct sam_perm_inode *dp,	/* Inode entry */
	sam_daddr_t bn,			/* mass storage extent block number */
	int ord,			/* mass storage extent ordinal */
	int level,			/* level of indirection */
	sam_indirect_extent_t *iep)	/* indirect extent pointer */
{
	int err = 0;

	if (iep->ieno != level) {
		err = 1;				/* bad level */

	} else if (S_ISSEGS(&dp->di)) {			/* segment data */

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

	} else {					/* normal inode */
		if ((iep->id.ino != dp->di.id.ino) ||
		    (iep->id.gen != dp->di.id.gen)) {
			err = 1;
		}
	}
	if (err) {				/* invalid indirect block */
		printf(catgets(catfd, SET, 13389,
		    "ALERT:  ino %d,\tInvalid indirect block 0x%llx eq %d\n"),
		    dp->di.id.ino, (sam_offset_t)bn,
		    devp->device[ord].eq);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . mismatch level=%d, "
			    "expected=%d or\n",
			    iep->ieno, level);
			printf("DEBUG:  \t . mismatch id=%d.%d, "
			    "expected=%d.%d or\n",
			    (int)iep->id.ino, iep->id.gen,
			    (int)dp->di.id.ino, dp->di.id.gen);
		}
		return (-1);
	}
	return (0);
}


/*
 * ----- verify_inode_block - verify inode block
 *
 * Check that contents of block of .inodes file is actually inodes.
 */

int				/* -1 if error, 0 if okay */
verify_inode_block(
	sam_daddr_t bn,		/* mass storage extent block number */
	int ord,		/* mass storage extent ordinal */
	ino_t ino)		/* expected number of first inode in block */
{
	int err = 0;
	int dt;
	int i;
	union sam_di_ino *idp;
	struct sam_perm_inode *dp;

	dt = inode_ino->di.status.b.meta;
	if (d_read(&devp->device[ord], (char *)bio_buffer,
	    LG_DEV_BLOCK(mp, dt), bn)) {
		error(0, 0,
		    catgets(catfd, SET, 13390,
		    "Read failed in .inodes on eq %d at block 0x%llx"),
		    devp->device[ord].eq, (sam_offset_t)bn);
		clean_exit(ES_io);
	}
	idp = (union sam_di_ino *)bio_buffer;
	for (i = 0; i < INO_IN_BLK; i++, idp++, ino++) {
		dp = (struct sam_perm_inode *)idp;

		/*
		 * Check that inode contents are as expected.
		 *
		 * If the inode's ino # is 0, that's OK, but only
		 * if the rest of the inode is clear.  (Uninitialized
		 * (zeroed) inodes are OK.)
		 *
		 * If the inode's ino # is set, then it must match
		 * the inode's actual number, and if the mode is !0
		 * (inode in use), then the inode version must match
		 * the superblock version.
		 */
		if (dp->di.id.ino == 0) {
			if (dp->di.mode != 0) {
				err++;
			}
		} else {
			if (dp->di.id.ino != ino) {
				err++;
			}
			if (dp->di.mode == 0) {
				continue;
			}
			if (dp->di.version != sblk_version) {
				err++;
			}
		}

		if (err) {
			return (-1);
		}
	}

	return (0);
}

/*
 * ----- write_sys_inodes - write system inodes
 * Update any special inodes held in memory that may have changed.
 */

void		/* ERRNO if error */
write_sys_inodes()
{
	char *ip = NULL;
	struct sam_perm_inode *dp;

	if ((ip = (char *)malloc(sizeof (struct sam_perm_inode))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 13338,
		    "Cannot malloc inode"));
		clean_exit(ES_malloc);
	}
	dp = (struct sam_perm_inode *)ip;

	/* Free inode pointer in .inodes inode may have changed */
	(void) get_inode(SAM_INO_INO, dp);
	dp->di.free_ino = inode_ino->di.free_ino;

	/* Update last repair time in .inodes residence time */
	dp->di.residence_time = (uint32_t)fstime;
	put_inode(SAM_INO_INO, dp);

	/* lost+found directory inode may have changed */
	if (orphan_ino) {
		(void) get_inode(orphan_inumber, dp);
		dp->di.nlink = orphan_ino->di.nlink;
		put_inode(orphan_inumber, dp);
	}
	free((void *)ip);
}


/*
 * ----- check_free_blocks
 * Check bit maps for each actual free block in the specified tmp file.
 * Account for the free bits. Determine if any files are mapped to any
 * blocks that are free.
 */

void		/* ERRNO if error */
check_free_blocks(int ord)	/* Disk ordinal */
{
	int ii, dt;
	uint_t mask;
	int bit;
	int nbit;
	uint_t *optr;
	uint_t *iptr;
	char *cptr;
	int daul;
	int kk;
	struct devlist *devlp;
	sam_daddr_t bn = 0;
	int blocks;
	int mmord;

	/* compare large dau maps */

	devlp = &devp->device[ord];
	if (devlp->type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}
	mmord = sblock.eq[ord].fs.mm_ord;
	daul = sblock.eq[ord].fs.l_allocmap;	/* number of blocks */
	blocks = sblock.eq[ord].fs.dau_size;	/* no. of bits */
	cptr = devlp->mm;
	for (ii = 0; ii < daul;
	    ii++, cptr += (SAM_DEV_BSIZE * SM_BLKCNT(mp, dt))) {
		if (d_read(&devp->device[mmord], (char *)dcp, 1,
		    (sblock.eq[ord].fs.allocmap + ii))) {
			error(0, 0,
			    catgets(catfd, SET, 797,
			    "Dau map read failed on eq %d"),
			    devp->device[ord].eq);
			clean_exit(ES_io);
		}
		iptr = (uint_t *)cptr;
		optr = (uint_t *)dcp;
		nbit = 0;
		for (kk = 0; kk < (SAM_DEV_BSIZE / NBPW); kk++, optr++) {
			for (bit = 31; bit >= 0; bit--) {
				blocks--;
				if (blocks < 0) {
					break;
				}
				if (*optr & (1 << bit)) {	/* If free */
					mask = SM_BITS(mp, dt) <<
					    (31-nbit -
					    (SM_BLKCNT(mp, dt) - 1));
					if ((*iptr & mask) == 0) {
						if (bn <
						    sblock.eq[
						    mmord].fs.system) {
							printf(catgets(catfd,
							    SET, 13315,
							    "ALERT:  "
							    "Allocation block "
							    "free ord=%d "
							    "bn=%.8llx. Run "
							    "samfsck -F to "
							    "repair\n"),
							    ord,
							    (sam_offset_t)bn);
							SETEXIT(ES_alert);
						}
#ifdef	DEBUG
						printf("NOTICE: Already "
						    "free: ord=%d bn=%.8llx\n",
						    ord, (sam_offset_t)bn);
#endif
						if (dt == MM) {
							mm_blocks++;
						} else {
							lg_blocks++;
						}
					}
				}
				bn += LG_DEV_BLOCK(mp, dt);
				nbit += SM_BLKCNT(mp, dt);
				if (nbit == 32) {
					iptr++;
					nbit = 0;
				}
			}
		}
	}
}


/*
 * ----- check_sm_free_blocks
 * Check bit maps for each small block in the .blocks file.
 * Account for the small free bits if not repairing. Must determine if file
 * are mapped to any small blocks that are free.
 */

void		/* ERRNO if error */
check_sm_free_blocks()
{
	sam_daddr_t bn;
	int ord;
	offset_t offset;
	struct sam_inoblk *smp;
	struct devlist *devlp;
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
			error(0, errno,
			    catgets(catfd, SET, 315,
			    ".blocks read failed on eq %d"),
			    devp->device[ord].eq);
			clean_exit(ES_io);
		}
		smp = (struct sam_inoblk *)dcp;
		for (i = 0; i < SM_BLK(mp, MM);
			i += sizeof (struct sam_inoblk)) {
			if (smp->bn == 0xffffffff) {	/* End of list */
				return;
			} else if (smp->bn != 0) {	/* Full entry */
				devlp = &devp->device[smp->ord];
				cptr = devlp->mm;
				bn = smp->bn << ext_bshift;
				ord = smp->ord;
				if (check_bn(block_ino->di.id.ino, bn, ord)) {
					continue;
				}
				for (j = 0; j < SM_BLKCNT(mp, MM); j++) {
					if (smp->free & (1 << j)) {
						/* small dau */
						bit = bn >>
							DIF_SM_SHIFT(mp, MM);
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
						if ((*wptr & mask) == 0) {
#if DEBUG
							printf("NOTICE: "
								"Small block "
								"already "
								"free: "
								"ord=%d "
								"bn=%.8llx\n",
							    ord,
							    (sam_offset_t)bn);
							SETEXIT(ES_blocks);
#endif
							sm_blocks++;
						}
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
 * ----- print_duplicates
 */

void
print_duplicates(void)
{
	struct dup_inoblk *smp;
	struct ino_list *inop;
	int i;
	int not_dups;

	for (smp = (struct dup_inoblk *)dup_mm; smp <= dup_last; smp++) {
		if (smp->bn == DUP_END) {
			break;				/* End of list */
		}
		if ((int)smp->count) {
			not_dups = 0;
			for (i = 0; i < (int)smp->count; i++) {
				inop = &ino_mm[smp->ino[i] - 1];
				if (inop->type == DIRECTORY &&
				    inop->prob == OKAY) {
					not_dups++;
				}
				if (inop->type == SEG_INDEX &&
				    inop->prob == OKAY &&
				    inop->seg_prob == OKAY) {
					not_dups++;
				}
			}
			if (verbose_print || debug_print) {
				printf(
				"DEBUG: %d Duplicates: block = 0x%llx, "
				"ord=%d, bits=0x%x\n",
				    (int)smp->count, (offset_t)smp->bn,
				    (uint_t)smp->ord, (uint_t)smp->free);
			}
			for (i = 0; i < (int)smp->count; i++) {
				inop = &ino_mm[smp->ino[i] - 1];
				if (not_dups == 1 &&
				    inop->type == DIRECTORY &&
				    inop->prob == OKAY) {
					if (verbose_print || debug_print) {
						printf("\t %s %s i-number "
						    "= %d is valid directory "
						    "blk\n",
						    (smp->dtype == DD) ?
						    "DD" : "MM",
						    (smp->btype == SM) ?
						    "SM" : "LG",
						    (int)smp->ino[i]);
					}
				} else if (not_dups == 1 &&
				    inop->type == SEG_INDEX &&
				    inop->prob == OKAY &&
				    inop->seg_prob == OKAY) {
					if (verbose_print || debug_print) {
						printf("\t %s %s i-number = "
						    "%d is valid segment "
						    "index\n",
						    (smp->dtype == DD) ?
						    "DD" : "MM",
						    (smp->btype == SM) ?
						    "SM" : "LG",
						    (int)smp->ino[i]);
					}
				} else {
					if (verbose_print || debug_print) {
						printf("\t %s %s i-number "
						    "= %d\n",
						    (smp->dtype == DD) ?
						    "DD" : "MM",
						    (smp->btype == SM) ?
						    "SM" : "LG",
						    (int)smp->ino[i]);
					}
					mark_inode(smp->ino[i], DUPLICATE_BLK);
				}
			}
		}
	}
}


/*
 * ----- print_inode_prob
 */

void
print_inode_prob(struct ino_list *inop)
{
	char *inotype;
	char *inoarch;
	char *msg_class = MSG_CLASS_ALERT;
	int es_class = ES_alert;

	switch (inop->prob) {
	case OKAY:
	case IO_ERROR:
	case BAD_BLKCNT:
		return;
	case INVALID_EXT:
		inotype = "Invalid extension:    ";
		break;
	case INVALID_DIR:
		inotype = "Invalid directory:    ";
		break;
	case INVALID_SEG:
		inotype = "Invalid segment file: ";
		break;
	case INVALID_BLK:
		inotype = "Invalid block:        ";
		break;
	case DUPLICATE_BLK:
		inotype = "Duplicate block:      ";
		break;
	case INVALID_INO:
		inotype = "Invalid inode:        ";
		inop->arch = FREED;
		if ((inop->orphan == ORPHAN) && (inop->type == INO_EXTEN)) {
			msg_class = MSG_CLASS_NOTICE;
			es_class = ES_ext;
		}
		break;
	default:
		inotype = "Unknown error:        ";
	}
	if (inop->arch == COPIES) {
		inoarch = "offline";
	} else if (inop->arch == NOCOPY) {
		inoarch = "damaged";
	} else if (inop->arch == FREED) {
		inoarch = "free   ";
	} else {
		inoarch = "unknown";
	}
	if (repair_files) {
		printf(catgets(catfd, SET, 13971,
		    "%s:  %sino %d marked %s\n"),
		    msg_class, inotype, (int)inop->id.ino,
		    inoarch);
	} else {
		printf(catgets(catfd, SET, 13972,
		    "%s:  %s-F will mark ino %d %s\n"),
		    msg_class, inotype, (int)inop->id.ino,
		    inoarch);
		SETEXIT(es_class);
	}
}


/*
 * ----- make_orphan - make orphan inode
 * Inode has no parent or damaged parent, free inode if no blocks.
 * If blocks, move orphan to lost+found directory.
 */

void
make_orphan(
	ino_t ino,				/* Inode number */
	struct sam_perm_inode *dp)		/* Inode entry */
{
	int zero_size = 0;

	if (dp->di.mode == 0 || dp->di.id.ino != ino) {
		return;
	}
	if (!repair_files) {
		if (S_ISSEGI(&dp->di)) {
			if (dp->di.rm.info.dk.seg.fsize == 0) {
				zero_size++;
			}
		} else if (S_ISLNK(dp->di.mode) &&
		    (dp->di.ext_attrs & ext_sln)) {
			if (dp->di.psize.symlink == 0) {
				zero_size++;
			}
		} else if (S_ISREQ(dp->di.mode) &&
		    (dp->di.ext_attrs & ext_rfa)) {
			if (dp->di.psize.rmfile == 0) {
				zero_size++;
			}
		} else if (dp->di.rm.size == 0) {
			zero_size++;
		}
		if (zero_size) {
			printf(catgets(catfd, SET, 13321,
			    "NOTICE: Orphan ino:        -F will free "
			    "empty ino %d\n"),
			    ino);
		} else if (dp->di.nlink == 0) {
			printf(catgets(catfd, SET, 13350,
			    "NOTICE: Orphan ino:        -F will free "
			    "nlink 0 ino %d\n"),
			    ino);
		} else {
			printf(catgets(catfd, SET, 13301,
			    "NOTICE: Orphan ino:        -F will move "
			    "ino %d to lost+found\n"),
			    ino);
		}
		SETEXIT(ES_orphan);
		return;
	}
	if (S_ISSEGI(&dp->di)) {
		if (dp->di.rm.info.dk.seg.fsize == 0) {
			zero_size++;
		}
	} else if (S_ISLNK(dp->di.mode) && (dp->di.ext_attrs & ext_sln)) {
		if (dp->di.psize.symlink == 0) {
			zero_size++;
		}
	} else if (S_ISREQ(dp->di.mode) && (dp->di.ext_attrs & ext_rfa)) {
		if (dp->di.psize.rmfile == 0) {
			zero_size++;
		}
	} else if (dp->di.rm.size == 0) {
		zero_size++;
	}
	if (zero_size) {
		if (!(dp->di.rm.ui.flags & RM_OBJECT_FILE)) {
			quota_uncount_file(dp);
			count_inode_blocks(dp);		/* uncount the blocks */
		}
		free_inode(ino, dp);
		printf(catgets(catfd, SET, 13322,
		    "NOTICE: Orphan ino:        empty ino %d moved to "
		    "free list\n"),
		    ino);
		return;
	}
	if (dp->di.nlink == 0) {
		quota_uncount_file(dp);
		count_inode_blocks(dp);		/* uncount the blocks */
		free_inode(ino, dp);
		printf(catgets(catfd, SET, 13351,
		    "NOTICE: Orphan ino:        nlink 0 ino %d moved to "
		    "free list\n"),
		    ino);
		return;
	}

	/* Make seg inode a regular file */
	if (S_ISSEGS(&dp->di)) {
		dp->di.status.b.segment = 0;
		dp->di.status.b.seg_ino = 0;
		dp->di.rm.info.dk.seg_size = 0;
		dp->di.rm.info.dk.seg.fsize = 0;
		dp->di.rm.info.dk.seg.ord = 0;
	}


	/*
	 * The extended attribute directory will now be put into the namespace
	 * so we make it into a normal directory under lost+found.  Otherwise
	 * fsck would complain about link counts.
	 */
	if (S_ISATTRDIR(dp->di.mode)) {
		dp->di.mode &= ~S_IFATTRDIR;
	}

	if (orphan_inumber <= 0) {
		if (orphan_inumber == 0) {
			orphan_inumber = -1;
			printf(catgets(catfd, SET, 13302,
			    "NOTICE: Cannot process orphans. Please "
			    "create ./lost+found directory\n"));
		}
		printf(catgets(catfd, SET, 13314,
		    "NOTICE: Orphan ino:        ino %d cannot be moved "
		    "to missing lost+found directory.\n"), ino);
		SETEXIT(ES_orphan);
		return;
	}
	if (orphan_ino == NULL) {
		orphan_ino = &orphan_inode;
		(void) get_inode(orphan_inumber, orphan_ino);
	}
	if (add_orphan(dp) == 0) {
		if (repair_files) {
			put_inode(ino, dp);
		}
		printf(catgets(catfd, SET, 13303,
		    "NOTICE: Orphan ino:        ino %d moved to lost+found\n"),
		    ino);
	}
}


/*
 * ----- add_orphan - add orphan name to directory.
 */

int					/* -1 if error, 0 if successful */
add_orphan(struct sam_perm_inode *dp)	/* Pointer to directory inode */
{
	ino_t ino;	/* I-number */
	struct sam_dirval *dvp;
	offset_t offset;
	int in;
	int found = 0;
	int reclen;
	struct sam_dirent *dirp;
	char orphan[64];

	ino = dp->di.id.ino;
	sprintf(orphan, "%d", (int)ino);
	offset = 0;
	while (offset < orphan_ino->di.rm.size) {
		if (get_dir_blk(DIRECTORY, orphan_ino, offset)) {
			return (-1);
		}
		dvp = (struct sam_dirval *)(dir_blk + DIR_BLK -
		    sizeof (struct sam_dirval));
		if ((dvp->d_id.ino != orphan_ino->di.id.ino) ||
		    (dvp->d_id.gen != orphan_ino->di.id.gen) ||
		    (dvp->d_version != SAM_DIR_VERSION)) {
			printf(catgets(catfd, SET, 13914,
			    "ALERT:  ino %d.%d,\tInvalid lost+found "
			    "directory validation header "
			    "at block 0x%llx, eq %d, offset 0x%llx\n"),
			    (int)orphan_ino->di.id.ino, orphan_ino->di.id.gen,
			    (sam_offset_t)dir_blk_bn,
			    devp->device[dir_blk_ord].eq, offset);
			SETEXIT(ES_alert);
			return (-1);
		}
		in = 0;
		while (in < DIR_BLK) {
			dirp = (struct sam_dirent *)(dir_blk + in);
			reclen = dirp->d_reclen;
			if (dirp->d_fmt > SAM_DIR_VERSION) {
				if (SAM_DIRLEN(orphan) <=
				    (dirp->d_reclen-SAM_DIRSIZ(dirp))) {
					reclen -= SAM_DIRSIZ(dirp);
					dirp->d_reclen = SAM_DIRSIZ(dirp);
					dirp = (struct sam_dirent *)
					    ((char *)dirp+SAM_DIRSIZ(dirp));
					found = 1;
				}
			} else {		/* if empty entry */
				if (SAM_DIRLEN(orphan) <= dirp->d_reclen) {
					found = 1;
				}
			}
			if (found) {
				if (S_ISDIR(dp->di.mode)) {
					if (change_dotdot(dp)) {
						return (-1);
					}
				}
				dirp->d_id = dp->di.id;
				dirp->d_fmt = dp->di.mode & S_IFMT;
				dirp->d_namlen = strlen(orphan);
				dirp->d_reclen = reclen;
				strcpy((char *)dirp->d_name, orphan);
				if (S_ISDIR(dp->di.mode)) {
					orphan_ino->di.nlink++;
				}
				if (put_dir_blk(DIRECTORY, orphan_ino)) {
					return (-1);
				}
				dp->di.parent_id = orphan_ino->di.id;
				/* Entry successfully moved  to lost+found */
				return (0);
			}
			if (((int)dirp->d_reclen <= 0) ||
			    ((int)dirp->d_reclen > DIR_BLK) ||
			    ((int)dirp->d_reclen & (NBPW-1))) {
				printf(catgets(catfd, SET, 13915,
				    "ALERT:  ino %d.%d,\tInvalid "
				    "lost+found directory entry "
				    "at block 0x%llx, eq %d, offset 0x%llx\n"),
				    (int)orphan_ino->di.id.ino,
				    orphan_ino->di.id.gen,
				    (sam_offset_t)dir_blk_bn,
				    devp->device[dir_blk_ord].eq, offset);
				SETEXIT(ES_alert);
				return (-1);
			}
			in += dirp->d_reclen;
		}
		offset += DIR_BLK;
	}
	orphan_full = 1;
	printf(catgets(catfd, SET, 13306,
	    "ALERT:  Orphan processing stopped due to full lost+found. \n"
	    "        Increase lost+found and rerun.\n"));
	SETEXIT(ES_orphan);
	return (-1);
}


/*
 * ----- change_dotdot
 * Change the dotdot ino in the directory entry.
 */

int					/* -1 if error, 0 if successful */
change_dotdot(struct sam_perm_inode *dp)	/* directory inode */
{
	ino_t ino;	/* I-number */
	offset_t offset;
	sam_daddr_t bn;
	int ord;
	struct sam_empty_dir *dirp;
	struct sam_dirval *dvp;
	char odir[DIR_BLK];

	ino = dp->di.id.ino;
	offset = 0;
	if (get_bn(dp, offset, &bn, &ord, 1)) {
		return (-1);
	}
	if (check_bn(ino, bn, ord)) {
		return (-1);
	}
	if (d_read(&devp->device[ord], (char *)odir, DIR_LOG_BLOCK, bn)) {
		error(0, 0,
		    catgets(catfd, SET, 13391,
		    "Read failed on eq %d at block 0x%llx"),
		    devp->device[ord].eq, (sam_offset_t)bn);
		return (-1);
	}
	dvp = (struct sam_dirval *)(odir + DIR_BLK -
	    sizeof (struct sam_dirval));
	if (verify_dir_validation(dp, dvp, 0)) {
		offline_inode(dp->di.id.ino, dp);
		return (-1);
	}
	dirp = (struct sam_empty_dir *)odir;
	if (verify_dot_dotdot(dp, dirp)) {
		offline_inode(dp->di.id.ino, dp);
		return (-1);
	}
	dirp->dotdot.d_id = orphan_ino->di.id;
	if (d_write(&devp->device[ord], (char *)odir, DIR_LOG_BLOCK, bn)) {
		printf(catgets(catfd, SET, 13309,
		    "ALERT:  ino %d,\tdirectory write failed on eq %d\n"),
		    orphan_ino->di.id.ino, devp->device[ord].eq);
		SETEXIT(ES_alert);
		return (-1);
	}
	return (0);
}


/*
 * ----- verify_dir_validation
 * Verify validation at block offset.
 */

int					/* -1 if error, 0 if successful */
verify_dir_validation(
	struct sam_perm_inode *dp,	/* Pointer to directory inode */
	struct sam_dirval *dvp,
	offset_t offset)
{
	if ((dvp->d_id.ino != dp->di.id.ino) ||
	    (dvp->d_id.gen != dp->di.id.gen) ||
	    (dvp->d_version != SAM_DIR_VERSION)) {
		printf(catgets(catfd, SET, 13323,
		    "ALERT:  ino %d,\tInvalid directory, offset "
		    "0x%llx, size=0x%llx\n"),
		    (int)dp->di.id.ino, offset, dp->di.rm.size);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . ino=%d .ne. ino_in_parent=%d\n",
			    dvp->d_id.ino, dp->di.id.ino);
			printf("DEBUG:  \t . gen=%d .ne. gen_in_parent=%d\n",
			    dvp->d_id.gen, dp->di.id.gen);
			printf("DEBUG:  \t . version=%d .ne. 1\n",
			    dvp->d_version);
		}
		return (-1);
	}
	return (0);
}


/*
 * ----- verify_dot_dotdot
 * Verify . and .. entries.
 */

int					/* -1 if error, 0 if successful */
verify_dot_dotdot(
	struct sam_perm_inode *dp,	/* Pointer to directory inode */
	struct sam_empty_dir *dirp)
{
	if ((dp->di.id.ino != dirp->dot.d_id.ino) ||
	    (dp->di.id.gen != dirp->dot.d_id.gen) ||
	    (dirp->dot.d_namlen != 1) || (dirp->dot.d_name[0] != '.')) {
		printf(catgets(catfd, SET, 13329,
		    "ALERT:  ino %d,\tInvalid directory dot entry, "
		    "parent ino=%d\n"),
		    (int)dp->di.id.ino, dp->di.parent_id.ino);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . entry ino=%d .ne. ino=%d\n",
			    dirp->dot.d_id.ino, dp->di.id.ino);
			printf("DEBUG:  \t . entry gen=%d .ne. gen=%d\n",
			    dirp->dot.d_id.gen, dp->di.id.gen);
			printf("DEBUG:  \t . namelen=%d .ne. 1\n",
			    dirp->dot.d_namlen);
			printf("DEBUG:  \t . d_name=%s .ne. .\n",
			    dirp->dot.d_name);
		}
		return (-1);
	}
	if ((dp->di.parent_id.ino != dirp->dotdot.d_id.ino) ||
	    (dp->di.parent_id.gen != dirp->dotdot.d_id.gen) ||
	    (dirp->dotdot.d_namlen != 2) || (dirp->dotdot.d_name[0] != '.') ||
	    (dirp->dotdot.d_name[1] != '.')) {
		printf(catgets(catfd, SET, 13324,
		    "ALERT:  ino %d,\tInvalid directory dotdot entry, "
		    "parent ino=%d\n"),
		    (int)dp->di.id.ino, dp->di.parent_id.ino);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t .. entry parent ino=%d .ne. "
			    "parent ino=%d\n",
			    dirp->dotdot.d_id.ino, dp->di.parent_id.ino);
			printf("DEBUG:  \t .. entry parent gen=%d .ne. "
			    "parent gen=%d\n",
			    dirp->dotdot.d_id.gen, dp->di.parent_id.gen);
			printf("DEBUG:  \t .. namelen=%d .ne. 2\n",
			    dirp->dotdot.d_namlen);
			printf("DEBUG:  \t .. d_name=%s .ne. ..\n",
			    dirp->dotdot.d_name);
		}
		return (-1);
	}
	return (0);
}


/*
 * ----- mark_inode - mark inode
 * Save problem type in mem inode table entry.  Segment inodes
 * must propagate their errors to the segment index.
 */

void
mark_inode(
	ino_t ino,					/* Inode number */
	int prob)					/* Problem type */
{
	struct ino_list *inop, *dinop;

	inop = &ino_mm[ino - 1];

	if (inop->type == REG_FILE ||
	    inop->type == DIRECTORY ||
	    inop->type == SEG_INDEX ||
	    inop->type == INO_EXTEN) {
		if (prob > inop->prob) {
			inop->prob = prob;
		}
	} else if (inop->type == SEG_INO) {
		if (prob > inop->prob) {
			inop->prob = prob;
		}
		if (inop->parent_id.ino < min_usr_inum ||
		    inop->parent_id.ino > ino_count) {
			return;
		}
		dinop = &ino_mm[inop->parent_id.ino - 1];

		if ((dinop->type == SEG_INDEX) &&
		    (inop->orphan == NOT_ORPHAN)) {
			if (prob == INVALID_INO) {
				dinop->seg_prob = INVALID_INO;
				dinop->seg_arch = NOCOPY;
			} else if (prob > dinop->seg_prob) {
				dinop->seg_prob = prob;
				if (inop->arch > dinop->seg_arch) {
					dinop->seg_arch = inop->arch;
				}
			}
		}
	}
}


/*
 * ----- offline_inode - offline inode
 * If archived, mark offline. If not archived, mark damaged.
 */

void
offline_inode(
	ino_t ino,			/* Inode number */
	struct sam_perm_inode *dp)	/* Inode entry */
{
	int i;
	uint_t *arp;

	if (!repair_files) {
		return;
	}
	if ((ino == SAM_INO_INO) || (ino == SAM_BLK_INO)) {
		return;
	}
	if ((dp->di.version >= SAM_INODE_VERS_2) &&
	    (ino == SAM_SHFLOCK_INO)) {
		return;
	}

	/* Stale entries should not be rearchived */
	arp = (uint_t *)&dp->di.ar_flags[0];
	*arp &= ~((AR_rearch<<24)|(AR_rearch<<16)|(AR_rearch<<8)|AR_rearch);

	quota_uncount_blocks(dp);

	if (dp->di.arch_status) {
		dp->di.status.b.offline = 1;
	} else {
		dp->di.status.b.damaged = 1;
	}


	/* Finish clearing the inode */
	dp->di.status.b.pextents = 0;
	dp->di.status.b.direct_map = 0;
	dp->di.blocks = 0;
	for (i = 0; i < NOEXT; i++) {
		dp->di.extent[i] = 0;
		dp->di.extent_ord[i] = 0;
	}
	quota_count_blocks(dp);
	put_inode(dp->di.id.ino, dp);
}


/*
 * ----- free_inode - free_inode
 * Clear the inode and put it in the free list.
 */

void
free_inode(
	ino_t ino,			/* Inode number */
	struct sam_perm_inode *dp)	/* Inode entry */
{
	int gen;

	gen = dp->di.id.gen;
	memset((char *)dp, 0, sizeof (struct sam_perm_inode));
	dp->di.id.ino = ino;
	dp->di.id.gen = ++gen;
	if (ino >= min_usr_inum) {
		dp->di.free_ino = freeino;
		freeino = ino;
		inode_ino->di.free_ino = ino;
	}
	if (repair_files) {
		put_inode(ino, dp);
	}
}


/*
 * ----- check_reg_file - check regular file inode structures
 * Check the validity of the regular file inode.  Check for
 * multivolume archive inode extensions (version 1 inodes).
 * Check all inode extension types (version 2 inodes).
 */

int					/* -1 if inode error, 0 otherwise */
check_reg_file(struct sam_perm_inode *dp)		/* Inode entry */
{
	int copy;
	sam_id_t eid;
	int n_vsns;
	int err = 0;

#define	SAM_REG_EXT_MASK (ext_sln | ext_rfa | ext_hlp | ext_acl | ext_mva)

	if (dp->di.version >= SAM_INODE_VERS_2) {	/* Version 2 & 3 */

		/* Check for absence of inode extensions and validate */
		if ((dp->di.ext_attrs & SAM_REG_EXT_MASK) == 0) {

			if (dp->di.ext_id.ino != 0 || dp->di.ext_id.gen != 0) {
				printf(catgets(catfd, SET, 13352,
				    "ALERT:  ino %d.%d,\tInvalid base "
				    "inode\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext id=%d.%d "
					    "not expected\n",
					    (int)dp->di.ext_id.ino,
					    dp->di.ext_id.gen);
				}
				err++;
			}
			if (dp->di.status.b.acl || dp->di.status.b.dfacl) {
				printf("NOTICE: inode %d.%d acl/dfacl=%d/%d "
				    "has no ACL exts\n",
				    dp->di.id.ino, dp->di.id.gen,
				    dp->di.status.b.acl,
				    dp->di.status.b.dfacl);
				if (repair_files) {
					dp->di.status.b.acl = 0;
					dp->di.status.b.dfacl = 0;
					put_inode(dp->di.id.ino, dp);
				}
				SETEXIT(ES_ext);
			}

		/* Check for presence of inode extensions and validate */
		} else {
			/*
			 * Check that first inode extension in list is in
			 * range
			 */
			if ((dp->di.ext_id.ino < min_usr_inum) ||
			    (dp->di.ext_id.ino > ino_count)) {
				printf(catgets(catfd, SET, 13352,
				    "ALERT:  ino %d.%d,\tInvalid base "
				    "inode\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext id=%d.%d "
					    ".lt. min=%ld or\n",
					    (int)dp->di.ext_id.ino,
					    dp->di.ext_id.gen,
					    min_usr_inum);
					printf("DEBUG:  \t . ext id=%d.%d "
					    ".gt. max=%d\n",
					    (int)dp->di.ext_id.ino,
					    dp->di.ext_id.gen,
					    ino_count);
				}
				err++;

			/* Check all inode extensions in list */
			} else if (check_inode_exts(dp)) {
				err++;
			}
		}

		/* Check for object inode extensions and validate */
		if (dp->di.rm.ui.flags & RM_OBJECT_FILE) {
			sam_di_osd_t	*oip;
			sam_id_t ext_id;

			oip = (sam_di_osd_t *)(void *)&dp->di.extent[0];
			ext_id = oip->ext_id;
			if (ext_id.ino != 0 || ext_id.gen != 0) {
				if (check_inode_exts(dp)) {
					err++;
				}
			}
		}

		/* Check for absence of extended attribute dir inode */
		if (dp->di2.xattr_id.ino == 0) {
			/* ino is 0 but gen is not */
			if (dp->di2.xattr_id.gen != 0) {
				printf(catgets(catfd, SET, 13982,
				    "ALERT:  ino %d.%d,\tInvalid "
				    "extended attr directory "
				    "inode\n"), (int)dp->di.id.ino,
				    dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext attr "
					    "dir id=%d.%d not expected\n",
					    (int)dp->di2.xattr_id.ino,
					    dp->di2.xattr_id.gen);
				}
				err++;
			}

		/* Check for presence of extended attribute dir inode */
		} else {
			/*
			 * Check that extended attribute directory inode number
			 * is in range
			 */
			if ((dp->di2.xattr_id.ino < min_usr_inum) ||
			    (dp->di2.xattr_id.ino > ino_count)) {
				printf(catgets(catfd, SET, 13982,
				    "ALERT: ino %d.%d,\tInvalid extended "
				    "attr directory "
				    "inode\n"), (int)dp->di2.xattr_id.ino,
				    dp->di2.xattr_id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext attr "
					    "dir id=%d.%d .lt. min=%ld "
					    "or\n", (int)dp->di2.xattr_id.ino,
					    dp->di2.xattr_id.gen,
					    min_usr_inum);
					printf("DEBUG:  \t . ext attr "
					    "dir id=%d.%d .gt. max=%d\n",
					    (int)dp->di2.xattr_id.ino,
					    dp->di2.xattr_id.gen,
					    ino_count);
				}
				err++;

			} else {
				struct ino_list *inop;

				/*
				 * Mark extended attribute dir inode
				 * NOT_ORPHANED and fix link count
				 */
				inop = &ino_mm[dp->di2.xattr_id.ino - 1];

				if (inop->orphan == ORPHAN) {
					inop->orphan = NOT_ORPHAN;
					/*
					 * We count the base inode ref to the EA
					 * dir
					 */
					inop->link_cnt++;
				} else {
					printf(catgets(catfd, SET, 13982,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "extended attribute "
					    "directory inode\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d already "
						    "processed\n",
						    (int)dp->di2.xattr_id.ino,
						    dp->di2.xattr_id.gen);
					}
					err++;
				}
			}
		}

	} else if (dp->di.version == SAM_INODE_VERS_1) {
		/* Previous version */
		sam_perm_inode_v1_t *dp_v1 = (sam_perm_inode_v1_t *)dp;

		/* If archive copies and n_vsns > 1 on any copy */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {

			eid = dp_v1->aid[copy];
			if ((eid.ino != 0) &&
			    (dp->di.arch_status & (1<<copy))) {

				/*
				 * Check that first inode extension in
				 * list is in range
				 */
				if ((eid.ino < min_usr_inum) ||
				    (eid.ino > ino_count)) {
					printf(catgets(catfd, SET, 13352,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "base inode\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "copy=%d, ext id=%d.%d "
						    ".lt. min=%lu or\n",
						    copy, (int)eid.ino,
						    eid.gen, min_usr_inum);
						printf("DEBUG:  \t . ext "
						    "id=%d.%d .gt. max=%d\n",
						    (int)eid.ino, eid.gen,
						    ino_count);
					}
					err++;

				} else {

					/*
					 * Check that number of vsns
					 * warrants an inode extension
					 */
					n_vsns = dp->ar.image[copy].n_vsns;
					if ((n_vsns < 2) ||
					    (n_vsns > MAX_VOLUMES)) {
						printf(catgets(catfd, SET,
						    13352,
						    "ALERT:  ino %d.%d,\t"
						    "Invalid base inode\n"),
						    (int)dp->di.id.ino,
						    dp->di.id.gen);
						SETEXIT(ES_alert);
						if (verbose_print ||
						    debug_print) {
							printf("DEBUG:  "
							    "\t . copy="
							    "%d, #vsns=%d"
							    " is .lt. 2 "
							    "or .gt. "
							    "max=%d\n",
							    copy, n_vsns,
							    MAX_VOLUMES);
						}
						err++;

					} else {

						/*
						 * Check all inode extensions
						 * in multivolume list
						 */

		/* N.B. Bad indentation here to meet cstyle requirements */

			if (check_multivolume_inode_exts(dp_v1, copy)) {
				/*
				 * Clear link for
				 * this copy in base
				 * inode
				 */
				if (repair_files) {
					dp_v1->aid[copy].ino =
					    dp_v1->aid[copy].gen = 0;
					put_inode(dp->di.id.ino, dp);
				}
				err++;
			}


					}
				}
			}
			if (err) {
				break;
			}
		}

		/*
		 * Check for a condition that could occur in 3.3.1 where the
		 * pextents flag may be set without any data blocks.
		 */
		if (dp->di.status.b.bof_online &&
		    dp->di.status.b.pextents &&
		    (dp->di.blocks == 0) &&
		    (dp->di.rm.size > 0)) {
			if (repair_files) {
				dp->di.status.b.pextents = 0;
				put_inode(dp->di.id.ino, dp);
				printf(catgets(catfd, SET, 13912,
				    "NOTICE: ino %d.%d,\tflags repaired\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
			} else {
				printf(catgets(catfd, SET, 13913,
				    "NOTICE: ino %d.%d,\tflags incorrect, "
				    "-F will repair\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
			}
		}
	}

	if (err) {
		mark_inode(dp->di.id.ino, INVALID_INO);
		return (-1);
	}

	return (0);
}


/*
 * ----- check_inode_exts - check inode extensions
 * Check symlink, removable media, multivolume archive, and ACL inode
 * extensions attached to base inode.
 */

int					/* -1 if inode error, 0 otherwise */
check_inode_exts(struct sam_perm_inode *dp)		/* Base inode entry */
{
	sam_id_t eid;
	char *ip;
	struct sam_perm_inode *xp;
	struct sam_inode_ext *ep;
	sam_di_osd_t *oip;
	int f_ext = 0;				/* extension types found */
	int n_chars = 0;
	int rfa_t_vsns = 0;
	int rfa_n_vsns = 0;
	int n_acls = 0;
	int n_dfacls = 0;
	int n_parents = 0;
	int copy;
	int mva_t_vsns[MAX_ARCHIVE] = { 0, 0, 0, 0 };
	int mva_n_vsns[MAX_ARCHIVE] = { 0, 0, 0, 0 };


	/* Allocate space for reading inode extensions */
	if ((ip = (char *)malloc(sizeof (struct sam_perm_inode))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 13338,
		    "Cannot malloc inode"));
		clean_exit(ES_malloc);
	}
	xp = (struct sam_perm_inode *)ip;

	/* Validate each inode extension in list */
	if (dp->di.rm.ui.flags & RM_OBJECT_FILE) {
		oip = (sam_di_osd_t *)(void *)&dp->di.extent[0];
		eid = oip->ext_id;
		while ((eid.ino >= min_usr_inum) && (eid.ino <= ino_count)) {
			(void) get_inode(eid.ino, xp);
			ep = (struct sam_inode_ext *)xp;
			if (verify_inode_ext(dp, ep, eid) == -1) {
				goto fail;
			}
			eid = ep->hdr.next_id;

			/* Check that next inode extension number is in range */
			if ((eid.ino != 0) && ((eid.ino < min_usr_inum) ||
			    (eid.ino > ino_count))) {
				printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d next "
					    "id=%d.%d .lt. min=%ld or\n",
					    (int)ep->hdr.id.ino, ep->hdr.id.gen,
					    (int)eid.ino, eid.gen,
					    min_usr_inum);
					printf("DEBUG:  \t . ino=%d.%d next "
					    "id=%d.%d .gt. max=%d\n",
					    (int)ep->hdr.id.ino, ep->hdr.id.gen,
					    (int)eid.ino, eid.gen,
					    ino_count);
				}
				goto fail;
			}
		}
	}

	/* Validate each inode extension in list */
	eid = dp->di.ext_id;
	while ((eid.ino >= min_usr_inum) && (eid.ino <= ino_count)) {

		(void) get_inode(eid.ino, xp);
		ep = (struct sam_inode_ext *)xp;
		if (verify_inode_ext(dp, ep, eid) == -1) {
			goto fail;
		}

		if (S_ISSLN(ep->hdr.mode)) {
			/*
			 * symbolic link inode extension
			 */
			if (!S_ISLNK(dp->di.mode)) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "file mode=0x%x and attr=0x%x "
					    "not symlink\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    dp->di.mode,
					    (int)dp->di.ext_attrs);
				}
				goto fail;
			}

			/*
			 * Make sure that number of characters in name
			 * is in range
			 */
			if ((ep->ext.sln.n_chars == 0 &&
			    dp->di.psize.symlink != 0) ||
			    (ep->ext.sln.n_chars > MAX_SLN_CHARS_IN_INO)) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "#chars=%d is 0 or .gt. max=%d\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    ep->ext.sln.n_chars,
					    MAX_SLN_CHARS_IN_INO);
				}
				goto fail;
			}
			n_chars += ep->ext.sln.n_chars;
			f_ext |= ext_sln;

		} else if (S_ISRFA(ep->hdr.mode)) {
			/*
			 * removable media file attrs inode extension
			 */
			if (!S_ISREQ(dp->di.mode)) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid "
				    "inode extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "file mode=0x%x and attr=0x%x "
					    "not rmedia\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    dp->di.mode,
					    (int)dp->di.ext_attrs);
				}
				goto fail;
			}

			if (EXT_1ST_ORD(ep)) {

				/*
				 * Make sure that number of volumes is
				 * in range
				 */
				rfa_t_vsns = ep->ext.rfa.info.n_vsns;
				if ((rfa_t_vsns <= 0) ||
				    (rfa_t_vsns > MAX_VOLUMES)) {
					printf(catgets(catfd, SET, 13353,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "inode extension\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d #vsns=%d "
						    "is .le. 0 or .gt. "
						    "max=%d\n",
						    (int)ep->hdr.id.ino,
						    ep->hdr.id.gen,
						    rfa_t_vsns, MAX_VOLUMES);
					}
					goto fail;
				}
				rfa_n_vsns = 1;

			} else {

				/*
				 * Check that starting ordinal is expected
				 * value
				 */
				if (ep->ext.rfv.ord != rfa_n_vsns) {
					printf(catgets(catfd, SET, 13353,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "inode extension\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d ord=%d, "
						    "expected=%d\n",
						    (int)ep->hdr.id.ino,
						    ep->hdr.id.gen,
						    ep->ext.rfv.ord,
						    rfa_n_vsns);
					}
					goto fail;
				}

				/*
				 * Make sure that number of volumes is
				 * in range
				 */
				if ((ep->ext.rfv.n_vsns <= 0) ||
				    (ep->ext.rfv.n_vsns > MAX_VSNS_IN_INO)) {
					printf(catgets(catfd, SET, 13353,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "inode extension\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d #vsns=%d is "
						    ".le. 0 or .gt. max=%d\n",
						    (int)ep->hdr.id.ino,
						    ep->hdr.id.gen,
						    ep->ext.rfv.n_vsns,
						    MAX_VSNS_IN_INO);
					}
					goto fail;
				}
				rfa_n_vsns += ep->ext.rfv.n_vsns;

				/*
				 * Check running vsn total against
				 * base value in first ext
				 */
				if (rfa_n_vsns > rfa_t_vsns) {
					printf(catgets(catfd, SET, 13353,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "inode extension\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d #vsns=%d "
						    ".gt. n_vsns=%d\n",
						    (int)ep->hdr.id.ino,
						    ep->hdr.id.gen,
						    rfa_n_vsns, rfa_t_vsns);
					}
					goto fail;
				}
			}
			f_ext |= ext_rfa;

		} else if (S_ISACL(ep->hdr.mode)) {
			/*
			 * access control list inode extension
			 */
			/* Make sure that acl counts are in range */
			if ((ep->ext.acl.n_acls < 0) ||
			    (ep->ext.acl.n_dfacls < 0) ||
			    (ep->ext.acl.t_acls < 0) ||
			    (ep->ext.acl.t_dfacls < 0) ||
			    ((n_acls + ep->ext.acl.n_acls) >
			    ep->ext.acl.t_acls) ||
			    ((n_dfacls + ep->ext.acl.n_dfacls) >
			    ep->ext.acl.t_dfacls)) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "#acls=%d is <= 0 or .gt. "
					    "tot=%d or\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    ep->ext.acl.n_acls,
					    ep->ext.acl.t_acls);
					printf("DEBUG:  \t . ino=%d.%d "
					    "#dfacls=%d is <= 0 or .gt. "
					    "dftot=%d\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    ep->ext.acl.n_dfacls,
					    ep->ext.acl.t_dfacls);
				}
				goto fail;
			}
			n_acls += ep->ext.acl.n_acls;
			n_dfacls += ep->ext.acl.n_dfacls;
			f_ext |= ext_acl;

		/* Check for inode extension type of multivolume archive */
		} else if (S_ISMVA(ep->hdr.mode)) {

			/*
			 * Make sure that copy number in multivolume
			 * archive inode is okay
			 */
			copy = ep->ext.mva.copy;
			if ((copy < 0) || (copy >= MAX_ARCHIVE)) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "invalid copy number=%d\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    copy);
				}
				goto fail;
			}

			/*
			 * Make sure that total num of vsns in multivolume
			 * archive inode okay
			 */
			if (mva_t_vsns[copy] == 0) {
				if ((ep->ext.mva.t_vsns <= 0) ||
				    (ep->ext.mva.t_vsns > MAX_VOLUMES)) {
					printf(catgets(catfd, SET, 13353,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "inode extension\n"),
					    (int)dp->di.id.ino,
					    dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d #vsns=%d is "
						    ".le. 0 or .gt. max=%d\n",
						    (int)ep->hdr.id.ino,
						    ep->hdr.id.gen,
						    ep->ext.mva.t_vsns,
						    MAX_VOLUMES);
					}
					goto fail;
				}
				mva_t_vsns[copy] = ep->ext.mva.t_vsns;

			/*
			 * Make sure that total num of vsns is consistent
			 * per copy
			 */
			} else if (ep->ext.mva.t_vsns != mva_t_vsns[copy]) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "copy=%d #vsns=%d expected=%d\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    copy, ep->ext.mva.t_vsns,
					    mva_t_vsns[copy]);
				}
				goto fail;
			}

			/*
			 * Verify that num of vsns in multivolume archive
			 * inode is okay
			 */
			if ((ep->ext.mva.n_vsns <= 0) ||
			    (ep->ext.mva.n_vsns > MAX_VSNS_IN_INO)) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "#vsns=%d is .le. 0 or .gt. "
					    "max=%d\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    ep->ext.mva.n_vsns,
					    MAX_VSNS_IN_INO);
				}
				goto fail;
			}

			/*
			 * Verify that running count of vsns doesn't
			 * exceed total vsns.
			 */
			mva_n_vsns[copy] += ep->ext.mva.n_vsns;
			if (mva_n_vsns[copy] > mva_t_vsns[copy]) {
				printf(catgets(catfd, SET, 13353,
				    "ALERT:  ino %d.%d,\tInvalid inode "
				    "extension\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ino=%d.%d "
					    "#vsns=%d is .gt. total=%d\n",
					    (int)ep->hdr.id.ino,
					    ep->hdr.id.gen,
					    mva_n_vsns[copy],
					    mva_t_vsns[copy]);
				}
				goto fail;
			}
			f_ext |= ext_mva;
		} else if (S_ISHLP(ep->hdr.mode) &&
		    dp->di.version != SAM_INODE_VERS_1) {
			int i;

			if (ep->ext.hlp.n_ids < 0 ||
			    ep->ext.hlp.n_ids > MAX_HLP_IDS_IN_INO) {
				printf(catgets(catfd, SET, 13947,
				    "NOTICE:  inode %d.%d HLP extension "
				    "inode %d.%d "
				    "n_ids (%d) out-of-range [0..%d]\n"),
				    dp->di.id.ino, dp->di.id.gen,
				    ep->hdr.id.ino, ep->hdr.id.gen,
				    ep->ext.hlp.n_ids, MAX_HLP_IDS_IN_INO);
				mark_inode(ep->hdr.id.ino, INVALID_EXT);
				SETEXIT(ES_ext);
			}
			n_parents += ep->ext.hlp.n_ids;
			for (i = 0; i < ep->ext.hlp.n_ids; i++) {
				if (ep->ext.hlp.ids[i].ino == 0 ||
				    ep->ext.hlp.ids[i].gen == 0 ||
				    ep->ext.hlp.ids[i].ino > ino_count) {
					printf(catgets(catfd, SET, 13948,
					    "NOTICE:  inode %d.%d HLP "
					    "extension inode %d.%d "
					    "slot %d: bad entry (%d.%d)\n"),
					    dp->di.id.ino, dp->di.id.gen,
					    ep->hdr.id.ino, ep->hdr.id.gen,
					    i, ep->ext.hlp.ids[i].ino,
					    ep->ext.hlp.ids[i].gen);
					mark_inode(ep->hdr.id.ino,
					    INVALID_EXT);
					SETEXIT(ES_ext);
				} else if (S_ISDIR(dp->di.mode)) {
					printf(catgets(catfd, SET, 13949,
					    "ALERT:  Directory inode %d.%d "
					    "has HLP extension "
					    "inode attached (%d.%d)\n"),
					    dp->di.id.ino, dp->di.id.gen,
					    ep->hdr.id.ino, ep->hdr.id.gen);
					mark_inode(ep->hdr.id.ino,
					    INVALID_INO);
					SETEXIT(ES_alert);
				} else {
					struct ino_list *pinop;
					/*
					 * Called from second pass.  Should
					 * have all parent
					 * inodes in inop-> for type check.
					 */
					pinop = &ino_mm[
					    ep->ext.hlp.ids[i].ino - 1];
					if (!S_ISDIR(pinop->fmt)) {
						printf(catgets(catfd, SET,
						    13950,
						    "NOTICE:  inode %d.%d "
						    "HLP extension inode "
						    "%d.%d "
						    "slot %d points to "
						    "non-directory inode "
						    "%d.%d\n"),
						    dp->di.id.ino,
						    dp->di.id.gen,
						    ep->hdr.id.ino,
						    ep->hdr.id.gen, i,
						    ep->ext.hlp.ids[i].ino,
						    ep->ext.hlp.ids[i].gen);
						mark_inode(ep->hdr.id.ino,
						    INVALID_EXT);
						SETEXIT(ES_ext);
					}
				}
			}
			for (i = MAX(0, ep->ext.hlp.n_ids);
			    i < MAX_HLP_IDS_IN_INO; i++) {
				if (ep->ext.hlp.ids[i].ino != 0 ||
				    ep->ext.hlp.ids[i].gen != 0) {
					printf(catgets(catfd, SET, 13951,
					    "NOTICE:  inode %d.%d HLP "
					    "extension inode %d.%d "
					    "has entry %x.%x in slot >= "
					    "n_ids (%d/%d)\n"),
					    dp->di.id.ino, dp->di.id.gen,
					    ep->hdr.id.ino, ep->hdr.id.gen,
					    ep->ext.hlp.ids[i].ino,
					    ep->ext.hlp.ids[i].gen,
					    i, ep->ext.hlp.n_ids);
					mark_inode(ep->hdr.id.ino,
					    INVALID_EXT);
					SETEXIT(ES_ext);
				}
			}
			f_ext |= ext_hlp;
		} else {
			printf(catgets(catfd, SET, 13952,
			    "ALERT:  inode %d.%d: extension inode (%d.%d) "
			    "has unknown type: %x\n"),
			    dp->di.id.ino, dp->di.id.gen,
			    ep->hdr.id.ino, ep->hdr.id.gen,
			    (int)ep->hdr.mode);
			goto fail;
		}
		eid = ep->hdr.next_id;

		/* Check that next inode extension number is in range */
		if ((eid.ino != 0) &&
		    ((eid.ino < min_usr_inum) || (eid.ino > ino_count))) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d next "
				    "id=%d.%d .lt. min=%ld or\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen,
				    (int)eid.ino, eid.gen,
				    min_usr_inum);
				printf("DEBUG:  \t . ino=%d.%d next "
				    "id=%d.%d .gt. max=%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen,
				    (int)eid.ino, eid.gen,
				    ino_count);
			}
			goto fail;
		}
	}

	/* Check total size of symlink name against inode size */
	if (S_ISLNK(dp->di.mode) && (f_ext & ext_sln) &&
	    n_chars != dp->di.psize.symlink) {
		printf(catgets(catfd, SET, 13352,
		    "ALERT:  ino %d.%d,\tInvalid base inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . symlink chars=%d, expected=%d\n",
			    n_chars, dp->di.psize.symlink);
		}
		goto fail;
	}

	/* Check total size of removable media file attrs against inode size */
	if (S_ISREQ(dp->di.mode) && (f_ext & ext_rfa) &&
	    (SAM_RESOURCE_SIZE(rfa_t_vsns) != dp->di.psize.rmfile)) {
		printf(catgets(catfd, SET, 13352,
		    "ALERT:  ino %d.%d,\tInvalid base inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . rmedia size=%d, expected=%d\n",
			    SAM_RESOURCE_SIZE(rfa_t_vsns),
			    dp->di.psize.rmfile);
		}
		goto fail;
	}

	if (!S_ISDIR(dp->di.mode) && dp->di.nlink != n_parents + 1) {
		if (dp->di.ext_id.ino != 0) {
			printf(catgets(catfd, SET, 13953,
			    "NOTICE:  inode %d.%d shows %d links, "
			    "%d parents\n"),
			    dp->di.id.ino, dp->di.id.gen,
			    (int)dp->di.nlink, n_parents+1);
			mark_inode(dp->di.ext_id.ino, INVALID_EXT);
		}
	}

	if (f_ext & ext_acl) {
		if (n_acls && !dp->di.status.b.acl) {
			printf("NOTICE: inode %d.%d acl/dfacl=%d/%d has "
			    "no ACLs\n",
			    dp->di.id.ino, dp->di.id.gen,
			    dp->di.status.b.acl, dp->di.status.b.dfacl);
			if (repair_files) {
				dp->di.status.b.acl = 1;
				put_inode(dp->di.id.ino, dp);
			}
			SETEXIT(ES_ext);
		}
		if (n_dfacls && !dp->di.status.b.dfacl) {
			printf("NOTICE: inode %d.%d acl/dfacl=%d/%d has "
			    "no DFACLs\n",
			    dp->di.id.ino, dp->di.id.gen,
			    dp->di.status.b.acl, dp->di.status.b.dfacl);
			if (repair_files) {
				dp->di.status.b.dfacl = 1;
				put_inode(dp->di.id.ino, dp);
			}
			SETEXIT(ES_ext);
		}
	} else {
		if (dp->di.status.b.acl || dp->di.status.b.dfacl) {
			printf("NOTICE: inode %d.%d acl/dfacl=%d/%d "
			    "has no ACL exts\n",
			    dp->di.id.ino, dp->di.id.gen,
			    dp->di.status.b.acl, dp->di.status.b.dfacl);
			if (repair_files) {
				dp->di.status.b.acl = 0;
				dp->di.status.b.dfacl = 0;
				put_inode(dp->di.id.ino, dp);
			}
			SETEXIT(ES_ext);
		}
	}

	/* Verify ext flags against actual extensions */
	if (dp->di.ext_attrs ^ f_ext) {
		if (repair_files) {
			printf(catgets(catfd, SET, 13944,
			    "NOTICE:  Updating inode %d.%d ext attr "
			    "flags (%#x->%#x)\n"),
			    dp->di.id.ino, dp->di.id.gen,
			    dp->di.ext_attrs, f_ext);
			dp->di.ext_attrs = f_ext;
			put_inode(dp->di.id.ino, dp);
			SETEXIT(ES_ext);
		} else {
			printf(catgets(catfd, SET, 13945,
			    "NOTICE:  Would update inode %d.%d ext "
			    "attr flags (%#x->%#x)\n"),
			    dp->di.id.ino, dp->di.id.gen,
			    dp->di.ext_attrs, f_ext);
			SETEXIT(ES_ext);
		}
	}

	free((void *)ip);
	return (0);

fail:

	/* Mark all inode extensions for this base inode as invalid */
	eid = dp->di.ext_id;
	while ((eid.ino >= min_usr_inum) && (eid.ino <= ino_count)) {

		(void) get_inode(eid.ino, xp);
		ep = (struct sam_inode_ext *)xp;

		/* Check that file id of inode extension is base inode */
		if ((ep->hdr.file_id.ino == dp->di.id.ino) &&
		    (ep->hdr.file_id.gen == dp->di.id.gen)) {
			mark_inode(ep->hdr.id.ino, INVALID_INO);
		} else {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d file id "
				    "mismatch=%d.%d expected=%d.%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen,
				    (int)ep->hdr.file_id.ino,
				    ep->hdr.file_id.gen,
				    (int)dp->di.id.ino, dp->di.id.gen);
			}
			break;
		}
		eid = ep->hdr.next_id;
	}

	free((void *)ip);
	return (-1);
}

int					/* -1 if inode error, 0 otherwise */
verify_inode_ext(
	struct sam_perm_inode *dp,	/* Base inode entry */
	struct sam_inode_ext *ep,	/* Extent inode entry */
	sam_id_t eid)				/* Inode.gen for extention */
{
	struct ino_list *inop;

	/* Check that file id of inode extension is base inode */
	if ((ep->hdr.file_id.ino != dp->di.id.ino) ||
	    (ep->hdr.file_id.gen != dp->di.id.gen)) {
		printf(catgets(catfd, SET, 13353,
		    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . ino=%d.%d file id "
			    "mismatch=%d.%d expected=%d.%d\n",
			    (int)ep->hdr.id.ino, ep->hdr.id.gen,
			    (int)ep->hdr.file_id.ino,
			    ep->hdr.file_id.gen,
			    (int)dp->di.id.ino, dp->di.id.gen);
		}
		return (-1);
	}

	/* Mark inode extension as recognized */
	inop = &ino_mm[eid.ino - 1];

	if (inop->orphan == ORPHAN) {
		inop->orphan = NOT_ORPHAN;
	} else {
		printf(catgets(catfd, SET, 13353,
		    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . ino=%d.%d already "
			    "processed\n",
			    (int)ep->hdr.id.ino, ep->hdr.id.gen);
		}
		return (-1);
	}

	/* Check that inode extension header version is okay */
	/* Current version */
	if ((ep->hdr.version == SAM_INODE_VERS_1) ||
	    (ep->hdr.version != dp->di.version)) {
		printf(catgets(catfd, SET, 13353,
		    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . ino=%d.%d vers=%d\n",
			    (int)ep->hdr.id.ino, ep->hdr.id.gen,
			    ep->hdr.version);
		}
		return (-1);
	}
	return (0);
}

/*
 * ----- check_multivolume_inode_exts - check multivolume inode extensions
 * Check multivolume archive inode extensions for specified copy.
 * This routine is used only for inode version 1 inode extensions.
 */

int					/* -1 if inode error, 0 otherwise */
check_multivolume_inode_exts(
	sam_perm_inode_v1_t *dp,	/* Base inode entry */
	int copy)			/* Archive copy number */
{
	sam_id_t eid;
	int n_vsns;
	char *ip;
	struct sam_perm_inode *xp;
	struct sam_inode_ext *ep;
	struct ino_list *inop;

	eid = dp->aid[copy];
	n_vsns = dp->ar.image[copy].n_vsns;

	/* Allocate space for reading multivolume inode extensions */
	if ((ip = (char *)malloc(sizeof (struct sam_perm_inode))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 13338,
		    "Cannot malloc inode"));
		clean_exit(ES_malloc);
	}
	xp = (struct sam_perm_inode *)ip;

	/* Validate each multivolume inode extension in list */
	while ((eid.ino >= min_usr_inum) && (eid.ino <= ino_count)) {

		(void) get_inode(eid.ino, xp);
		ep = (struct sam_inode_ext *)xp;

		/* Check that file id of inode extension is base inode */
		if ((ep->hdr.file_id.ino != dp->di.id.ino) ||
		    (ep->hdr.file_id.gen != dp->di.id.gen)) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy=%d, "
				    "file id mismatch=%d.%d expected=%d.%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen, copy,
				    (int)ep->hdr.file_id.ino,
				    ep->hdr.file_id.gen,
				    (int)dp->di.id.ino, dp->di.id.gen);
			}
			goto fail;
		}

		/* Mark inode extension as recognized */
		inop = &ino_mm[eid.ino - 1];

		if (inop->orphan == ORPHAN) {
			inop->orphan = NOT_ORPHAN;
		} else {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy=%d, "
				    "already processed\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen, copy);
			}
			goto fail;
		}

		/* Check that inode extension is a multivolume archive inode */
		if (!S_ISMVA(ep->hdr.mode) ||
		    (ep->hdr.version != SAM_INODE_VERS_1) ||
		    (ep->hdr.version != dp->di.version)) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy=%d, "
				    "mode=0x%x or vers=%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen, copy,
				    ep->hdr.mode, ep->hdr.version);
			}
			goto fail;
		}

		/*
		 * Make sure that copy number in multivolume archive
		 * inode is okay
		 */
		if (ep->ext.mv1.copy != copy) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy "
				    "mismatch=%d, expected=%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen,
				    ep->ext.mv1.copy, copy);
			}
			goto fail;
		}

		/*
		 * Make sure that num of vsns in multivolume archive
		 * inode is okay
		 */
		if ((ep->ext.mv1.n_vsns <= 0) ||
		    (ep->ext.mv1.n_vsns > MAX_VSNS_IN_INO)) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy=%d, "
				    "#vsns=%d is .le. 0 or .gt. max=%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen, copy,
				    ep->ext.mv1.n_vsns, MAX_VSNS_IN_INO);
			}
			goto fail;
		}
		n_vsns -= ep->ext.mv1.n_vsns;
		if (n_vsns < 0) {
			n_vsns = 0;
		}

		eid = ep->hdr.next_id;

		/* Check that next inode extension number is in range */
		if ((eid.ino != 0) &&
		    ((eid.ino < min_usr_inum) || (eid.ino > ino_count))) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy=%d, "
				    "next id=%d.%d .lt. min=%lu or\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen, copy,
				    (int)eid.ino, eid.gen,
				    min_usr_inum);
				printf("DEBUG:  \t . ino=%d.%d next "
				    "id=%d.%d .gt. max=%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen,
				    (int)eid.ino, eid.gen,
				    ino_count);
			}
			goto fail;
		}

		/* Check that next inode extension is set only when needed */
		if (((eid.ino == 0) && (n_vsns != 0)) ||
		    ((eid.ino != 0) && (n_vsns == 0))) {
			printf(catgets(catfd, SET, 13353,
			    "ALERT:  ino %d.%d,\tInvalid inode extension\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . ino=%d.%d copy=%d, "
				    "next id=%d.%d for #vsns=%d\n",
				    (int)ep->hdr.id.ino, ep->hdr.id.gen, copy,
				    (int)eid.ino, eid.gen,
				    n_vsns);
			}
			goto fail;
		}

	}

	free((void *)ip);
	return (0);

fail:

	/* Mark all multivolume inode extensions for this copy as invalid */
	eid = dp->aid[copy];
	while (eid.ino) {

		(void) get_inode(eid.ino, xp);
		ep = (struct sam_inode_ext *)xp;

		/* Check that file id of inode extension is base inode */
		if ((ep->hdr.file_id.ino == dp->di.id.ino) &&
		    (ep->hdr.file_id.gen == dp->di.id.gen)) {
			mark_inode(ep->hdr.id.ino, INVALID_INO);
		}
		eid = ep->hdr.next_id;
	}

	free((void *)ip);
	return (-1);
}


/*
 * ----- check_dir - check directory structure
 * Check the validity of the directory data and if bad, mark the
 * mem inode table to indicate error.  During final processing it
 * will be offlined if it has been archived, else damaged.
 */

int					/* -1 if data error, 0 otherwise */
check_dir(struct sam_perm_inode *dp)		/* Inode entry */
{
	offset_t offset;
	int in;
	uint_t *wptr;

	struct sam_dirval *dvp;
	int stale_dir = 0;
	int write_dir_blk;
	int first = 1;
	int err = 0;

#define	SAM_DIR_EXT_MASK (ext_acl | ext_mva)

	/* Treat offline as a transient error */
	if (dp->di.status.b.offline) {
		offline_dirs++;
		mark_inode(dp->di.id.ino, IO_ERROR);
		return (-1);
	}

	if (dp->di.version >= SAM_INODE_VERS_2) {	/* Version 2 & 3 */

		/* Check for absence of inode extensions and validate */
		if ((dp->di.ext_attrs & SAM_DIR_EXT_MASK) == 0) {

			if ((dp->di.ext_id.ino != 0) ||
			    (dp->di.ext_id.gen != 0)) {
				printf(catgets(catfd, SET, 13352,
				    "ALERT:  ino %d.%d,\tInvalid base "
				    "inode\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext id=%d.%d "
					    "not expected\n",
					    (int)dp->di.ext_id.ino,
					    dp->di.ext_id.gen);
				}
				err++;
			}

		/* Check for presence of inode extensions and validate */
		} else {
			/*
			 * Check that first inode extension in list is
			 * in range
			 */
			if ((dp->di.ext_id.ino < min_usr_inum) ||
			    (dp->di.ext_id.ino > ino_count)) {
				printf(catgets(catfd, SET, 13352,
				    "ALERT:  ino %d.%d,\tInvalid base "
				    "inode\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext id=%d.%d "
					    ".lt. min=%ld or\n",
					    (int)dp->di.ext_id.ino,
					    dp->di.ext_id.gen,
					    min_usr_inum);
					printf("DEBUG:  \t . ext id=%d.%d "
					    ".gt. max=%d\n",
					    (int)dp->di.ext_id.ino,
					    dp->di.ext_id.gen,
					    ino_count);
				}
				err++;

			/* Check all inode extensions in list */
			} else if (check_inode_exts(dp)) {
				err++;
			}
		}
		if (err) {			/* Clear link in base inode */
			if (repair_files) {
				dp->di.ext_id.ino = dp->di.ext_id.gen = 0;
				dp->di.ext_attrs = 0;

				/* Clear ACL status bits */
				dp->di.status.b.acl = 0;
				dp->di.status.b.dfacl = 0;

				put_inode(dp->di.id.ino, dp);
			}
			err = 0;
		}

		/* Check for absence of inode extended attr dir inode */
		if (dp->di2.xattr_id.ino == 0) {

			if (dp->di2.xattr_id.gen != 0) {
				printf(catgets(catfd, SET, 13982,
				    "ALERT:  ino %d.%d,\tInvalid "
				    "extended attr directory "
				    "inode\n"), (int)dp->di.id.ino,
				    dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext attr "
					    "dir id=%d.%d not expected\n",
					    (int)dp->di2.xattr_id.ino,
					    dp->di2.xattr_id.gen);
				}
				err++;
			}

		/* Check for presence of xattr dir and validate */
		} else {
			/* Check that xattr is in range */
			if ((dp->di2.xattr_id.ino < min_usr_inum) ||
			    (dp->di2.xattr_id.ino > ino_count)) {
				printf(catgets(catfd, SET, 13982,
				    "ALERT:  ino %d.%d,\tInvalid extended "
				    "attr directory "
				    "inode\n"), (int)dp->di2.xattr_id.ino,
				    dp->di2.xattr_id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . ext attr "
					    "dir id=%d.%d .lt. min=%ld"
					    " or\n", (int)dp->di2.xattr_id.ino,
					    dp->di2.xattr_id.gen,
					    min_usr_inum);
					printf("DEBUG:  \t . ext attr "
					    "dir id=%d.%d .gt. max=%d\n",
					    (int)dp->di2.xattr_id.ino,
					    dp->di2.xattr_id.gen,
					    ino_count);
				}
				err++;

			} else {
				struct ino_list *inop;

				/* Mark inode as recognized */
				inop = &ino_mm[dp->di2.xattr_id.ino - 1];

				if (inop->orphan == ORPHAN) {
					inop->orphan = NOT_ORPHAN;
					/*
					 * We count the base inode ref to the EA
					 * dir
					 */
					inop->link_cnt++;
				} else {
					printf(catgets(catfd, SET, 13982,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "inode extended "
					    "attribute\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "ino=%d.%d already "
						    "processed\n",
						    (int)dp->di2.xattr_id.ino,
						    dp->di2.xattr_id.gen);
					}
					err++;
				}
			}
		}
		if (err) {  /* Clear our reference to the extended attribute */
			if (repair_files) {
				dp->di2.xattr_id.ino = dp->di2.xattr_id.gen = 0;
				put_inode(dp->di.id.ino, dp);
			}
			err = 0;
		}
	}

	/* Verify good directory blocks */
	offset = 0;
	while (offset < dp->di.rm.size) {
		if (get_dir_blk(DIRECTORY, dp, offset)) {
			if (verbose_print || debug_print) {
				printf("DEBUG:  ino %d,\tcan't read dir "
				    "blk at off=0x%llx\n",
				    dp->di.id.ino, offset);
			}
			mark_inode(dp->di.id.ino, IO_ERROR);
			return (-1);
		}

		/*
		 * Truncate directory if last block (not first) is
		 * all zeros.
		 */
		if ((first == 0) && ((offset + DIR_BLK) >= dp->di.rm.size)) {
			in = 0;
			while (in < DIR_BLK) {
				wptr = (uint_t *)(dir_blk + in);
				if (*wptr) {
					break;
				}
				in += sizeof (uint_t);
			}
			if (in == DIR_BLK) {		/* All zeros */
				if (repair_files) {
					printf(catgets(catfd, SET, 13359,
					    "ALERT:  directory ino %d,"
					    "\tRepaired zero directory "
					    "block off=0x%llx\n"),
					    (int)dp->di.id.ino, offset);
					dp->di.rm.size -= DIR_BLK;
					put_inode(dp->di.id.ino, dp);
				} else {
					printf(catgets(catfd, SET, 13360,
					    "ALERT:  directory ino %d,"
					    "\t-F will repair zero "
					    "directory block off=0x%llx\n"),
					    (int)dp->di.id.ino, offset);
				}
				SETEXIT(ES_alert);
				break;
			}
		}

		dvp = (struct sam_dirval *)(dir_blk + DIR_BLK -
		    sizeof (struct sam_dirval));
		if (verify_dir_validation(dp, dvp, offset)) {
			err++;
			break;
		}
		if (first) {
			struct sam_empty_dir *dirp;

			dirp = (struct sam_empty_dir *)dir_blk;
			if (verify_dot_dotdot(dp, dirp)) {
				err++;
				break;
			}
		}

		/* Validate directory entries in this directory block. */

		in = 0;
		write_dir_blk = 0;
		while (in < DIR_BLK) {
			struct sam_dirent *dirp;
			ino_t ino;
			int gen;
			struct ino_list *inop;

			dirp = (struct sam_dirent *)(dir_blk + in);
			if (dirp->d_fmt > SAM_DIR_VERSION) {
				/* An in-use directory entry */
				ino = dirp->d_id.ino;
				gen = dirp->d_id.gen;
				if ((ino == 0) || (ino > ino_count) ||
				    (dirp->d_namlen == 0) ||
				    (dirp->d_namlen > dirp->d_reclen)) {
					printf(catgets(catfd, SET, 13374,
					    "ALERT:  directory ino %d.%d,"
					    "\tInvalid entry at in=0x%x, "
					    "off=0x%llx\n"),
					    (int)dp->di.id.ino,
					    dp->di.id.gen, in, offset);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . ino=%d "
						    ".eq. 0 or\n",
						    (int)ino);
						printf("DEBUG:  \t . ino=%d "
						    ".gt. max_ino=%d or\n",
						    (int)ino, ino_count);
						printf("DEBUG:  \t . "
						    "d_namlen=%d .eq. 0 or\n",
						    dirp->d_namlen);
						printf("DEBUG:  \t . "
						    "d_namlen=%d .gt. "
						    "d_reclen=%d\n",
						    dirp->d_namlen,
						    dirp->d_reclen);
					}
					SETEXIT(ES_alert);
					err++;
					break;
				}
				inop = &ino_mm[ino - 1];

				if (first && in <= 20) { /* Skip .  & .. */
					/*
					 * Update link count unless this is ..
					 * of an extended attribute directory.
					 * EAs are not in the namespace so the
					 * parent inode does not show a link
					 * count for the EA directory.
					 */
					if (!(S_ISATTRDIR(dp->di.mode) &&
					    (dirp->d_namlen == 2) &&
					    (dirp->d_name[0] == '.') &&
					    (dirp->d_name[1] == '.'))) {
						inop->link_cnt++;
					}
#if defined(STALEFMT)
				/*
				 * If directory entry format is invalid,
				 * repair it.
				 */
				} else if ((inop->id.ino > SAM_MAX_PRIV_INO) &&
				    (dirp->d_fmt != inop->fmt)) {
					if (repair_files) {
						printf(catgets(catfd, SET,
						    133XX,
						    "NOTICE: directory ino "
						    "%d,\tdirectory entry "
						    "format repaired %s\n"),
						    (int)dp->di.id.ino,
						    dirp->d_name);
						if (verbose_print ||
						    debug_print) {
							printf("DEBUG:  \t "
							    ". fmt "
							    "mismatch=0x%x "
							    "expected=0x%x\n",
							    dirp->d_fmt,
							    inop->fmt);
						}
						dirp->d_fmt = inop->fmt;
						stale_dir++;
					} else {
						printf(catgets(catfd, SET,
						    133XX,
						    "NOTICE: directory "
						    "ino %d,\t-F will "
						    "repair directory "
						    "entry format %s\n"),
						    (int)dp->di.id.ino,
						    dirp->d_name);
						if (verbose_print ||
						    debug_print) {
							printf("DEBUG:  "
							    "\t . fmt "
							    "mismatch=0x%x "
							    "expected=0x%x\n",
							    dirp->d_fmt,
							    inop->fmt);
						}
					}
#endif /* STALEFMT */
				/* If directory entry is stale, remove it. */
				} else if ((inop->id.ino > min_usr_inum) &&
				    (dirp->d_id.gen != inop->id.gen)) {
					int reclen;

					if (repair_files) {
						printf(catgets(catfd, SET,
						    13307,
						    "NOTICE: directory "
						    "ino %d,\tstale "
						    "directory entry %s "
						    "removed\n"),
						    (int)dp->di.id.ino,
						    dirp->d_name);
						if (verbose_print ||
						    debug_print) {
							printf("DEBUG:  "
							    "\t . id "
							    "mismatch=%d.%d "
							    "expected=%d.%d\n",
							    (int)inop->id.ino,
							    (int)inop->id.gen,
							    (int)ino, gen);
						}
						reclen = dirp->d_reclen;
						memset((caddr_t)dirp, 0,
						    reclen);
						dirp->d_fmt = 0;
						dirp->d_reclen = reclen;
						stale_dir++;
					} else {
						printf(catgets(catfd, SET,
						    13308,
						    "NOTICE: directory "
						    "ino %d,\t-F will "
						    "remove stale directory "
						    "entry %s\n"),
						    (int)dp->di.id.ino,
						    dirp->d_name);
						if (verbose_print ||
						    debug_print) {
							printf("DEBUG:  "
							    "\t . id "
							    "mismatch=%d.%d "
							    "expected=%d.%d\n",
							    (int)inop->id.ino,
							    (int)inop->id.gen,
							    (int)ino, gen);
						}
					}

				/*
				 * Check to see if parent id matches current
				 * directory.
				 * If not, save current directory as hard
				 * link parent.
				 */
				} else {
					if (inop->orphan == ORPHAN &&
					    inop->parent_id.ino ==
					    dp->di.id.ino &&
					    inop->parent_id.gen ==
					    dp->di.id.gen) {
						/* Have parent now */
						inop->orphan = NOT_ORPHAN;
					} else {	/* Another link ? */
						save_hard_link_parent(dp,
						    inop);
					}
					inop->link_cnt++;
				}

				if (dp->di.id.ino == SAM_ROOT_INO) {
					switch (isrootfile((char *)
					    dirp->d_name, dirp->d_namlen+1)) {
					case ROOT_LOSTFOUND:
						if (repair_files &&
						    S_ISDIR(inop->fmt)) {
							orphan_inumber =
							    dirp->d_id.ino;
						}
						break;
					case ROOT_QUOTA_A:
						quota_file_ino[
						    SAM_QUOTA_ADMIN] =
						    dirp->d_id.ino;
						break;
					case ROOT_QUOTA_G:
						quota_file_ino[
						    SAM_QUOTA_GROUP] =
						    dirp->d_id.ino;
						break;
					case ROOT_QUOTA_U:
						quota_file_ino[
						    SAM_QUOTA_USER] =
						    dirp->d_id.ino;
						break;
					default:
						break;
					}
				}
				if (hash_dirs && dirp->d_namlen > 2) {
					/* check for valid name hash entry */
					ushort_t namehash;

					namehash = sam_dir_gennamehash(
					    dirp->d_namlen,
					    (char *)dirp->d_name);
					if (namehash != 0 &&
					    namehash != dirp->d_namehash) {
						if (repair_files) {
							dirp->d_namehash =
							    namehash;
							write_dir_blk++;
						}
						if (ino > min_usr_inum) {
							num_2hash++;
						}
					}
				}
			}
			if (((int)dirp->d_reclen <= 0) ||
			    ((int)dirp->d_reclen > DIR_BLK) ||
			    ((int)dirp->d_reclen & (NBPW-1))) {

				/* last block ? */
				if (dp->di.rm.size == (offset + DIR_BLK)) {
					if (repair_files) {
						dp->di.rm.size -= DIR_BLK;
						printf(catgets(catfd, SET,
						    13372,
						    "ALERT:  directory "
						    "ino %d.%d,\tRepaired "
						    "invalid directory "
						    "block off=0x%llx\n"),
						    (int)dp->di.id.ino,
						    dp->di.id.gen, offset);
						put_inode(dp->di.id.ino, dp);
					} else {
						printf(catgets(catfd, SET,
						    13373,
						    "ALERT:  directory ino "
						    "%d.%d,\t-F will repair "
						    "invalid directory "
						    "block off=0x%llx\n"),
						    (int)dp->di.id.ino,
						    dp->di.id.gen, offset);
					}
				} else {
					printf(catgets(catfd, SET, 13374,
					    "ALERT:  directory ino %d.%d,"
					    "\tInvalid entry at in=0x%x, "
					    "off=0x%llx\n"),
					    (int)dp->di.id.ino,
					    dp->di.id.gen, in, offset);
					if (verbose_print || debug_print) {
						printf("DEBUG:  \t . "
						    "d_reclen=%d .le. 0 or\n",
						    dirp->d_reclen);
						printf("DEBUG:  \t . "
						    "d_reclen=%d .gt. %d or\n",
						    dirp->d_reclen, DIR_BLK);
						printf("DEBUG:  \t . "
						    "d_reclen=%d not word "
						    "aligned\n",
						    dirp->d_reclen);
					}
					err++;
				}
				SETEXIT(ES_alert);
				break;
			}
			in += dirp->d_reclen;
		}
		if (repair_files && (stale_dir || write_dir_blk)) {
			if (put_dir_blk(DIRECTORY, dp)) {
				if (verbose_print || debug_print) {
					printf("DEBUG:  ino %d.%d,\tcan't "
					    "write dir blk at off=0x%llx\n",
					    (int)dp->di.id.ino,
					    dp->di.id.gen, offset);
				}
				mark_inode(dp->di.id.ino, IO_ERROR);
				return (-1);
			}
		}
		offset += DIR_BLK;
		first = 0;
	}

	if (err) {
		mark_inode(dp->di.id.ino, INVALID_DIR);
		return (-1);
	}

	return (0);
}


/*
 * ----- check_seg_inode - check_seg_inode
 * Check the validity of the segmented file inode and if bad,
 * mark the mem inode table to indicate error.
 */

int						/* -1 if error, 0 otherwise */
check_seg_inode(struct sam_perm_inode *dp)	/* Inode entry */
{
	struct ino_list *inop;

	/* Check that segment attribute is set on segment inode */
	if (!dp->di.status.b.segment) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . segment attr is 0\n");
		}
		goto fail;
	}

	/* Find segment index inode as parent of segment inode */
	if ((dp->di.parent_id.ino < min_usr_inum) ||
	    (dp->di.parent_id.ino > ino_count)) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . parent id=%d.%d .lt. min=%lu "
			    "or\n",
			    (int)dp->di.parent_id.ino, dp->di.parent_id.gen,
			    min_usr_inum);
			printf("DEBUG:  \t . parent id=%d.%d .gt. max=%d\n",
			    (int)dp->di.parent_id.ino, dp->di.parent_id.gen,
			    ino_count);
		}
		goto fail;
	}

	inop = &ino_mm[dp->di.parent_id.ino - 1];

	/* Check that id of segment index is parent of segment inode */
	if ((inop->id.ino != dp->di.parent_id.ino) ||
	    (inop->id.gen != dp->di.parent_id.gen)) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . index id mismatch=%d.%d "
			    "expected=%d.%d\n",
			    (int)inop->id.ino, inop->id.gen,
			    (int)dp->di.parent_id.ino, dp->di.parent_id.gen);
		}
		goto fail;
	}

	/* Check that segment index inode was typed correctly in first pass */
	if (inop->type != SEG_INDEX || !S_ISREG(inop->fmt)) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . index ino=%d.%d has invalid "
			    "mode=0x%x\n",
			    (int)inop->id.ino, inop->id.gen,
			    inop->fmt);
		}
		goto fail;
	}

	/* Check that segment size in segment matches expected value */
	if ((dp->di.rm.info.dk.seg_size != inop->seg_size) ||
	    (dp->di.rm.info.dk.seg_size == 0)) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . segment size is 0 or "
			    "mismatch=%d expected=%d\n",
			    inop->seg_size, dp->di.rm.info.dk.seg_size);
		}
		goto fail;
	}

	/* Check that data in segment is <= a segment size */
	if (dp->di.rm.size > SAM_SEGSIZE(dp->di.rm.info.dk.seg_size)) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . segment data size=%lld .gt. "
			    "%lld\n",
			    dp->di.rm.size, SAM_SEGSIZE(inop->seg_size));
		}
		goto fail;
	}

	/* Check that ordinal in segment inode is in range */
	if (dp->di.rm.info.dk.seg.ord >= inop->seg_lim) {
		printf(catgets(catfd, SET, 13343,
		    "ALERT:  ino %d.%d,\tInvalid segment inode\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . invalid ord=%d .ge. limit=%d\n",
			    dp->di.rm.info.dk.seg.ord, inop->seg_lim);
		}
		goto fail;
	}

	return (0);

fail:

	/*
	 * If seg inode is still an orphan, check_seg_index (if there
	 * is an index) will pick up the error as a seg_prob.
	 */
	mark_inode(dp->di.id.ino, INVALID_INO);
	return (-1);
}


/*
 * ----- check_seg_index - check_seg_index
 * Check the validity of the segmented file index and if bad,
 * mark the mem inode table to indicate error.
 */

int						/* -1 if error, 0 otherwise */
check_seg_index(struct sam_perm_inode *dp)	/* Inode entry */
{
	offset_t offset;
	int in;

	int seg_ord;
	int seg_lim;
	int seg_err = 0;
	int seg_index_err = 0;
	sam_val_t *dvp;
	sam_id_t *id;
	struct ino_list *inop;

	/* Check that segment attribute is set on segment index inode */
	if (!dp->di.status.b.segment) {
		printf(catgets(catfd, SET, 13344,
		    "ALERT:  ino %d.%d,\tInvalid segment index\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . segment attr is 0\n");
		}
		goto fail;
	}

	/* Make sure that segment index thinks there are segments present */
	if (dp->di.rm.info.dk.seg_size == 0) {
		printf(catgets(catfd, SET, 13344,
		    "ALERT:  ino %d.%d,\tInvalid segment index\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:   \t . segment size is 0\n");
		}
		goto fail;
	}

	seg_ord = 0;
	seg_lim = howmany(dp->di.rm.size,
	    SAM_SEGSIZE(dp->di.rm.info.dk.seg_size));
	if (seg_lim == 0) {
		printf(catgets(catfd, SET, 13344,
		    "ALERT:  ino %d.%d,\tInvalid segment index\n"),
		    (int)dp->di.id.ino, dp->di.id.gen);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:   \t . number of segments is 0\n");
		}
		goto fail;
	}

	/* Treat offline as a transient error */
	if (dp->di.status.b.offline)  {
		mark_inode(dp->di.id.ino, IO_ERROR);
		return (-1);
	}

	/* Read and process the index block for the segmented file */
	for (offset = 0; offset < dp->di.rm.info.dk.seg.fsize;
	    offset += SAM_SEG_BLK) {

		if (get_dir_blk(SEG_INDEX, dp, offset)) {
			printf(catgets(catfd, SET, 13344,
			    "ALERT:  ino %d.%d,\tInvalid segment index\n"),
			    (int)dp->di.id.ino, dp->di.id.gen);
			SETEXIT(ES_alert);
			if (verbose_print || debug_print) {
				printf("DEBUG:  \t . can't read index "
				    "block at off=0x%llx\n",
				    offset);
			}
			mark_inode(dp->di.id.ino, IO_ERROR);
			return (-1);
		}

		/* Validate segment index header record */
		dvp = (sam_val_t *)(dir_blk+SAM_SEG_BLK - sizeof (sam_val_t));
		if (verify_hdr_validation(dp, dvp, offset)) {
			seg_err++;
			break;
		}

		/* Perform validity checks for each segment index entry */
		for (in = 0; in < (SAM_SEG_BLK-sizeof (sam_val_t));
		    in += sizeof (sam_id_t)) {
			id = (sam_id_t *)((char *)dir_blk + in);

			/* Check for excess seg inode refs in index */
			if (seg_ord >= seg_lim) {
				if (id->ino != 0 || id->gen != 0) {
					printf(catgets(catfd, SET, 13344,
					    "ALERT:  ino %d.%d,\tInvalid "
					    "segment index\n"),
					    (int)dp->di.id.ino, dp->di.id.gen);
					SETEXIT(ES_alert);
					if ((verbose_print || debug_print) &&
					    (seg_index_err == 0)) {
						printf("DEBUG:  \t . "
						    "invalid entry in=0x%x, "
						    "off=0x%llx\n",
						    in, offset);
						printf("DEBUG:  \t . ord=%d "
						    ".ge. limit=%d\n",
						    seg_ord, seg_lim);
						seg_index_err++;
					}
					seg_err++;
				}
				seg_ord++;
				continue;
			}

			/*
			 * Segment index for sparse file may have entries
			 * of 0
			 */
			if (id->ino == 0 && id->gen == 0) {
				seg_ord++;
				continue;
			}

			/* Validate index entry id range */
			if (id->ino < min_usr_inum || id->ino > ino_count) {
				printf(catgets(catfd, SET, 13344,
				    "ALERT:  ino %d.%d,\tInvalid segment "
				    "index\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . invalid entry "
					    "in=0x%x, off=0x%llx\n",
					    in, offset);
					printf("DEBUG:  \t . ino=%d .lt. "
					    "min=%lu or\n",
					    id->ino, min_usr_inum);
					printf("DEBUG:  \t . ino=%d .gt. "
					    "max=%d\n",
					    id->ino, ino_count);
				}
				seg_err++;
				seg_ord++;
				continue;
			}

			/*
			 * Check that parent of segment inode matches id
			 * of index
			 */
			inop = &ino_mm[id->ino - 1];

			if (inop->type != SEG_INO ||
			    inop->parent_id.ino != dp->di.id.ino ||
			    inop->parent_id.gen != dp->di.id.gen) {
				printf(catgets(catfd, SET, 13344,
				    "ALERT:  ino %d.%d,\tInvalid segment "
				    "index\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . invalid/stale "
					    "entry in=0x%x, off=0x%llx\n",
					    in, offset);
					printf("DEBUG:  \t . ino=%d type=%d "
					    "not SEG_INO or\n",
					    (int)id->ino, inop->type);
					printf("DEBUG:  \t . ino=%d "
					    "parent id mismatch=%d.%d "
					    "expected=%d.%d\n",
					    (int)id->ino,
					    (int)inop->parent_id.ino,
					    inop->parent_id.gen,
					    (int)dp->di.id.ino,
					    dp->di.id.gen);
				}
				seg_err++;
				seg_ord++;
				continue;
			}

			/* Mark segment inode as recognized */
			if (inop->orphan == ORPHAN) {
				inop->orphan = NOT_ORPHAN;

				/*
				 * Catch any segment inode errors not
				 * propagated to index
				 */
				if (inop->prob != OKAY &&
				    inop->prob != IO_ERROR) {
					mark_inode(id->ino, inop->prob);
				}
			} else {
				printf(catgets(catfd, SET, 13344,
				    "ALERT:  ino %d.%d,\tInvalid segment "
				    "index\n"),
				    (int)dp->di.id.ino, dp->di.id.gen);
				SETEXIT(ES_alert);
				if (verbose_print || debug_print) {
					printf("DEBUG:  \t . duplicate "
					    "entry in=0x%x, off=0x%llx\n",
					    in, offset);
					printf("DEBUG:  \t . ino=%d "
					    "already processed\n",
					    (int)id->ino);
				}
				seg_err++;
				seg_ord++;
				continue;
			}

			seg_ord++;
		}
	}

	/* Display count of total invalid indices found */
	if ((verbose_print || debug_print) && (seg_index_err > 1)) {
		printf("DEBUG:  \t . total of %d invalid entries\n",
		    seg_index_err);
	}

	/*
	 * Segment index data errors are potentially recoverable from
	 * offline copy
	 */
	if (seg_err) {
		mark_inode(dp->di.id.ino, INVALID_SEG);
		return (-1);
	}

	return (0);

fail:

	/* Segment index inode errors are not recoverable */
	mark_inode(dp->di.id.ino, INVALID_INO);
	return (-1);
}


/*
 * ----- verify_hdr_validation
 * Verify header validation at block offset.  Expects hdr version 2.
 */

int					/* -1 if error, 0 if successful */
verify_hdr_validation(
	struct sam_perm_inode *dp,	/* Ptr to inode */
	sam_val_t *dvp,			/* Ptr to validation record */
	offset_t offset)		/* Offset of block */
{
	offset_t size;
	uint_t blk_size;
	uint_t hdr_size;
	uint_t ent_size;

	if (S_ISSEGI(&dp->di)) {
		size = dp->di.rm.info.dk.seg.fsize;
		blk_size = SAM_SEG_BLK;
		hdr_size = sizeof (sam_val_t);
		ent_size = sizeof (sam_id_t);
	} else {
		return (0);
	}

	if (dvp->v_version != SAM_VAL_VERSION) {
		printf(catgets(catfd, SET, 13342,
		    "ALERT:  ino %d.%d,\tInvalid validation header, "
		    "offset=0x%llx, size=0x%llx\n"),
		    (int)dp->di.id.ino, dp->di.id.gen, offset, size);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . version=%d .ne. %d\n",
			    dvp->v_version, SAM_VAL_VERSION);
		}
		return (-1);
	}
	if ((dvp->v_id.ino != dp->di.id.ino) ||
	    (dvp->v_id.gen != dp->di.id.gen)) {
		printf(catgets(catfd, SET, 13342,
		    "ALERT:  ino %d.%d,\tInvalid validation header, "
		    "offset=0x%llx, size=0x%llx\n"),
		    (int)dp->di.id.ino, dp->di.id.gen, offset, size);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . ino=%d .ne. ino_in_parent=%d "
			"or\n",
			    (int)dvp->v_id.ino, (int)dp->di.id.ino);
			printf("DEBUG:  \t . gen=%d .ne. gen_in_parent=%d\n",
			    dvp->v_id.gen, dp->di.id.gen);
		}
		return (-1);
	}
	if (dvp->v_size != (offset + blk_size)) {
		printf(catgets(catfd, SET, 13342,
		    "ALERT:  ino %d.%d,\tInvalid validation header, "
		    "offset=0x%llx, size=0x%llx\n"),
		    (int)dp->di.id.ino, dp->di.id.gen, offset, size);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . hdr size=0x%llx .ne. "
			"offset=0x%llx\n",
			    dvp->v_size, (offset + blk_size));
		}
		return (-1);
	}
	if ((dvp->v_relsize % ent_size != 0) ||
	    (dvp->v_relsize > (blk_size - hdr_size))) {
		printf(catgets(catfd, SET, 13342,
		    "ALERT:  ino %d.%d,\tInvalid validation header, "
		    "offset=0x%llx, size=0x%llx\n"),
		    (int)dp->di.id.ino, dp->di.id.gen, offset, size);
		SETEXIT(ES_alert);
		if (verbose_print || debug_print) {
			printf("DEBUG:  \t . relsize=%d .mod. id size=%d "
			    "not 0 or\n",
			    dvp->v_relsize, ent_size);
			printf("DEBUG:  \t . relsize=%d .gt. max index "
			    "size=%d\n",
			    dvp->v_relsize, (blk_size - hdr_size));
		}
		return (-1);
	}
	if (dvp->v_time &&
	    ((dvp->v_time < nblock.info.sb.init) ||
	    (dvp->v_time > (uint32_t)fstime))) {
		/*
		 * Just warn for now
		 *
		 * printf(catgets(catfd, SET, 13342,
		 *	"ALERT:  ino %d.%d,\tInvalid validation header, "
		 *	"offset=0x%llx, size=0x%llx\n"),
		 *	(int)dp->di.id.ino, dp->di.id.gen, offset, size);
		 * SETEXIT(ES_alert);
		 */
		if (verbose_print || debug_print) {
			printf("DEBUG:  ino %d.%d,\tValidation header time "
			    "mismatch, offset=0x%llx, size=0x%llx\n",
			    (int)dp->di.id.ino, dp->di.id.gen, offset, size);
			printf("DEBUG:  \t . time=0x%x .lt. sblock=0x%x or\n",
			    dvp->v_time, sblock.info.sb.init);
			printf("DEBUG:  \t . time=0x%x .gt. now=0x%x\n",
			    dvp->v_time, (uint32_t)fstime);
		}
		/*
		 * Just warn for now
		 * return (-1);
		 */
	}

	return (0);
}


/*
 * ----- get_dir_blk - read directory or segmented file index
 * Read requested block from a directory or segmented index.
 */

int				/* -1 if read error, 1 EOF, 0 otherwise */
get_dir_blk(
	int type,			/* DIRECTORY, SEG_INDEX */
	struct sam_perm_inode *dp,	/* Ptr to inode */
	offset_t offset)		/* Byte offset to read */
{
	sam_daddr_t bn;
	int ord;

	if (get_bn(dp, offset, &bn, &ord, 1)) {
		return (-1);
	}
	if (bn == 0) {
		return (1);		/* EOF */
	}
	if ((bn != dir_blk_bn) || (ord != dir_blk_ord)) {
		if (check_bn(dp->di.id.ino, bn, ord)) {
			return (-1);
		}
		if (d_read(&devp->device[ord], (char *)dir_blk,
		    DIR_LOG_BLOCK, bn)) {
			if (type == DIRECTORY) {
				printf(catgets(catfd, SET, 380,
				    "ALERT:  ino %d,\tdirectory read "
				    "failed on eq %d\n"),
				    (int)dp->di.id.ino, devp->device[ord].eq);
			} else if (type == SEG_INDEX) {
				printf(catgets(catfd, SET, 13395,
				    "ALERT:  ino %d.%d,\tSegment index "
				    "read failed on "
				    "eq %d at block 0x%llx\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    devp->device[ord].eq,
				    (sam_offset_t)bn);
			}
			SETEXIT(ES_error);

			dir_blk_ord = -1;	/* clear for future use */
			dir_blk_bn = 0;
			return (-1);
		}
		dir_blk_ord = ord;		/* save for future refs */
		dir_blk_bn = bn;
	}
	return (0);
}


/*
 * ----- put_dir_blk - write directory or segment index
 * Write block in dir_blk buffer to a directory or segment index.
 */

int				/* -1 if read error, 1 EOF, 0 otherwise */
put_dir_blk(
	int type,			/* DIRECTORY, SEG_INDEX */
	struct sam_perm_inode *dp)	/* Ptr to inode */
{
	if (repair_files) {
		if (d_write(&devp->device[dir_blk_ord], (char *)dir_blk,
		    DIR_LOG_BLOCK, dir_blk_bn)) {
			if (type == DIRECTORY) {
				printf(catgets(catfd, SET, 13309,
				    "ALERT:  ino %d,\tdirectory write "
				    "failed on eq %d\n"),
				    (int)dp->di.id.ino,
				    devp->device[dir_blk_ord].eq);
			} else if (type == SEG_INDEX) {
				printf(catgets(catfd, SET, 13396,
				    "ALERT:  ino %d.%d,\tSegment index "
				    "write failed on "
				    "eq %d at block 0x%llx\n"),
				    (int)dp->di.id.ino, dp->di.id.gen,
				    devp->device[dir_blk_ord].eq,
				    (sam_offset_t)dir_blk_bn);
			}
			SETEXIT(ES_error);
			return (-1);
		}
	}
	return (0);
}


/*
 * ----- save_hard_link_parent -  save hard link parent id
 * Add parent id to hard link parent list.  The list will be used to
 * update the hard link parent inode extension in the third pass.
 */

void
save_hard_link_parent(
	struct sam_perm_inode *dp,		/* Ptr to directory inode */
	struct ino_list *inop)			/* Ptr to inode table entry */
{
	char *hp;
	struct hlp_list *hlp;

	if (inop->hlp == NULL) {
		if ((hp = (char *)malloc(sizeof (struct hlp_list))) == NULL) {
			error(0, 0,
			    catgets(catfd, SET, 602,
			    "Cannot malloc ino array"));
			clean_exit(ES_malloc);
		}
		bzero(hp, sizeof (struct hlp_list));
		hlp = (struct hlp_list *)hp;
		inop->hlp = hlp;
		hlp->n_ids = 0;
		hlp->count = HLP_COUNT;
	} else {
		hlp = inop->hlp;
		if (hlp->n_ids >= hlp->count) { /* enough space for ids ? */
			int count, size;

			count = hlp->count + MAX_HLP_IDS_IN_INO;
			size = offsetof(struct hlp_list, ids[count]);
			hp = realloc(hlp, size);
			if (hp == NULL) {
				error(0, 0, catgets(catfd, SET, 602,
				    "Cannot malloc ino array"));
				clean_exit(ES_malloc);
			}
			hlp = (struct hlp_list *)hp;
			inop->hlp = hlp;
			hlp->count = count;
		}
	}
	hlp->ids[hlp->n_ids] = dp->di.id;		/* Save directory id */
	hlp->n_ids++;
}


/*
 * ----- update_hard_link_parent -  update hard link parent inode table
 * Replace hard link parent inode extension contents with new list of
 * saved alternate parents.
 */

void
update_hard_link_parent(
	struct sam_perm_inode *dp,	/* Ptr to base inode */
	struct hlp_list *hlp)		/* Ptr to hard link parent list */
{
	char *ip;
	struct sam_inode_ext *ep;
	struct ino_list *inop, *hinop;
	sam_id_t eid;
	int n_ids, copied;
	struct hlp_list empty_hlp_list;

	if (hlp == NULL) {
		/*
		 * Inode hasn't got an hlp list; apparently it has 1 link
		 * and one parent, but an attached HLP extension.
		 */
		bzero(&empty_hlp_list, sizeof (empty_hlp_list));
		hlp = &empty_hlp_list;
	}

	/*
	 * If base inode has a bad parent ID, fix it up first.
	 */
	inop = &ino_mm[dp->di.id.ino - 1];
	if (inop->orphan == ORPHAN) {
		if (hlp->n_ids) {
			struct sam_perm_inode bdp;

			hlp->n_ids--;
			inop->parent_id = hlp->ids[hlp->n_ids];
			printf(catgets(catfd, SET, 13954,
			    "NOTICE:  inode %d.%d, setting parent to "
			    "ino %d.%d\n"),
			    dp->di.id.ino, dp->di.id.gen,
			    hlp->ids[hlp->n_ids].ino,
			    hlp->ids[hlp->n_ids].gen);
			hlp->ids[hlp->n_ids].ino =
			    hlp->ids[hlp->n_ids].gen = 0;
			get_inode(inop->id.ino, &bdp);
			bdp.di.parent_id = inop->parent_id;
			put_inode(inop->id.ino, &bdp);
			inop->orphan = NOT_ORPHAN;
		}
	}

	/* Allocate space for reading inode extensions */
	if ((ip = (char *)malloc(sizeof (struct sam_inode_ext))) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 13338,
		    "Cannot malloc inode"));
		clean_exit(ES_malloc);
	}
	ep = (struct sam_inode_ext *)ip;

	/* Find hard link parent inode extension in list */
	n_ids = hlp->n_ids;
	eid = dp->di.ext_id;
	copied = 0;
	/*
	 * Loop looking for HLP extensions.  Copy out as many as possible
	 * HLP entries into each one.  If there are any left over at the
	 * end, say something.
	 */
	while ((eid.ino >= min_usr_inum) && (eid.ino <= ino_count)) {

		(void) get_inode(eid.ino, (struct sam_perm_inode *)ep);

		hinop = &ino_mm[eid.ino - 1];
		/* Check that file id of inode extension is base inode */
		if (S_ISHLP(ep->hdr.mode) &&
		    ep->hdr.file_id.ino == dp->di.id.ino &&
		    ep->hdr.file_id.gen == dp->di.id.gen) {
			/*
			 * hard link parent extension: copy out n
			 * (<= MAX_HLP_IDS_IN_INO)
			 * parent links, and adjust appropriate counts.
			 */
			hinop->prob = OKAY;
			if (n_ids > copied) {
				int i, n = MIN(n_ids-copied,
				    MAX_HLP_IDS_IN_INO);

				printf(catgets(catfd, SET, 13955,
				    "NOTICE:  updating HLP extension "
				    "inode %d.%d; "
				    "copying %d parent IDs\n"),
				    ep->hdr.id.ino, ep->hdr.id.gen, n);
				bcopy(&hlp->ids[copied],
				    (char *)&ep->ext.hlp.ids[0],
				    n * sizeof (sam_id_t));
				ep->ext.hlp.n_ids = n;
				copied += n;
				for (i = n; i < MAX_HLP_IDS_IN_INO; i++) {
					ep->ext.hlp.ids[i].ino =
					    ep->ext.hlp.ids[i].gen = 0;
				}
			} else {
				/*
				 * Clear any extra HLP extension inodes
				 * hanging about
				 */
				printf(catgets(catfd, SET, 13956,
				    "NOTICE:  clearing HLP extension "
				    "inode %d.%d.\n"),
				    ep->hdr.id.ino, ep->hdr.id.gen);
				bzero((char *)&ep->ext.hlp.ids[0],
				    MAX_HLP_IDS_IN_INO * sizeof (sam_id_t));
				ep->ext.hlp.n_ids = 0;
			}
			put_inode(eid.ino, (struct sam_perm_inode *)ep);
		}
		eid = ep->hdr.next_id;
	}
	hlp->n_ids = n_ids;
	if (n_ids > copied) {
		printf(catgets(catfd, SET, 13957,
		    "ALERT:  inode %d.%d: %d parents unclaimed\n"),
		    dp->di.id.ino, dp->di.id.gen, n_ids - copied);
	}

	free((void *)ip);

	hlp->count = -hlp->count;		/* mark done */
}


/*
 * ----- get_inode - get inode
 * Read requested inode from the .inodes file using the bio_buffer.
 * Always read a full buffer of inodes aligned on buffer size boundary.
 */

int		/* Fatal if .inodes block read/write error, 1 EOF, else 0 */
get_inode(
	ino_t ino,			/* Inode number of requested inode */
	struct sam_perm_inode *dp)	/* Ptr to permanent inode (returned) */
{
	int dt;
	sam_daddr_t bn;
	int ord;
	offset_t offset;
	offset_t off;
	char *ip;

	offset = SAM_ITOD(ino);
	dt = inode_ino->di.status.b.meta;
	/*
	 * align boundary
	 * careful here! mask needs to be a long long
	 */
	off = offset & ~(offset_t)(mp->mi.m_dau[dt].size[LG] - 1);
	if (off != bio_buf_off) {
		if (get_bn(inode_ino, offset, &bn, &ord, 1)) {
			error(0, 0,
			    catgets(catfd, SET, 586,
			    "Cannot get .inodes block at offset 0x%llx"),
			    offset);
			clean_exit(ES_inodes);
		}
		if (bn == 0) {
			return (1);		/* EOF */
		}
		bn &= ~(LG_DEV_BLOCK(mp, dt) - 1);	/* align boundary */
		if (check_bn(ino, bn, ord)) {
			error(0, 0,
			    catgets(catfd, SET, 317,
			    ".inodes block at offset 0x%llx is in "
			    "system area"),
			    offset);
			clean_exit(ES_inodes);
		}
		sync_inodes();
		if (d_read(&devp->device[ord], (char *)bio_buffer,
		    LG_DEV_BLOCK(mp, dt), bn)) {
			error(0, 0,
			    catgets(catfd, SET, 13390,
			    "Read failed in .inodes on eq %d at block 0x%llx"),
			    devp->device[ord].eq, (sam_offset_t)bn);
			clean_exit(ES_io);
		}
		bio_buf_ord = ord;		/* save for future refs */
		bio_buf_bn = bn;
		bio_buf_off = off;
	}

	/* copy inode from buffer */
	ip = (char *)((char *)bio_buffer +
	    (offset & (mp->mi.m_dau[dt].size[LG]-1)));
	memcpy((char *)dp, ip, sizeof (struct sam_perm_inode));

	/*
	 * The size field in the .inodes inode may have been adjusted by
	 * the check_sys_inode_blocks() routine.  The new value needs to be
	 * reset each time .inodes inode is reread from disk so that the whole
	 * .inodes file will be processed.
	 */
	if (ino == SAM_INO_INO) {
		dp->di.rm.size = inode_ino->di.rm.size;
	}
	return (0);
}


/*
 * ----- put_inode - put inode
 * Update inode in .inodes file using the bio_buffer.
 * Always read a full buffer of inodes aligned on buffer size boundary.
 */

void				/* Fatal if error reading .inodes block */
put_inode(
	ino_t ino,			/* Inode number of requested inode */
	struct sam_perm_inode *dp)	/* Ptr to permanent inode */
{
	int dt;
	sam_daddr_t bn;
	int ord;
	offset_t offset;
	offset_t off;
	char *ip;

	offset = SAM_ITOD(ino);
	dt = inode_ino->di.status.b.meta;
	/*
	 * align boundary
	 * careful here! mask needs to be a long long
	 */
	off = offset & ~(offset_t)(mp->mi.m_dau[dt].size[LG] - 1);
	if (off != bio_buf_off) {
		if (get_bn(inode_ino, offset, &bn, &ord, 1)) {
			error(0, 0,
			    catgets(catfd, SET, 586,
			    "Cannot get .inodes block at offset 0x%llx"),
			    offset);
			clean_exit(ES_inodes);
		}
		bn &= ~(LG_DEV_BLOCK(mp, dt) - 1);	/* align boundary */
		if (check_bn(ino, bn, ord)) {
			error(0, 0,
			    catgets(catfd, SET, 317,
			    ".inodes block at offset 0x%llx is in "
			    "system area"),
			    offset);
			clean_exit(ES_inodes);
		}
		if (d_read(&devp->device[ord], (char *)bio_buffer,
		    LG_DEV_BLOCK(mp, dt), bn)) {
			error(0, 0,
			    catgets(catfd, SET, 13390,
			    "Read failed in .inodes on eq %d at block 0x%llx"),
			    devp->device[ord].eq, (sam_offset_t)bn);
			clean_exit(ES_io);
		}
		bio_buf_ord = ord;		/* save for future refs */
		bio_buf_bn = bn;
		bio_buf_off = off;
	}

	/* copy inode to buffer */
	ip = (char *)((char *)bio_buffer +
	    (offset & (mp->mi.m_dau[dt].size[LG]-1)));
	memcpy(ip, (char *)dp, sizeof (struct sam_perm_inode));

	bio_buf_mod = 1;			/* mark block modified */
}


/*
 * ----- sync_inodes - sync inodes
 * Write modified bio_buffer containing .inodes blocks to disk.
 * Always write a full buffer of inodes aligned on buffer size boundary.
 */

void				/* Fatal if error writing .inodes block */
sync_inodes(void)
{
	int dt;

	if (bio_buf_mod) {		/* Current block is modified */
		if (repair_files) {	/* Don't ever remove this line */
			dt = inode_ino->di.status.b.meta;
			if (d_write(&devp->device[bio_buf_ord],
			    (char *)bio_buffer,
			    LG_DEV_BLOCK(mp, dt), bio_buf_bn)) {
				error(0, 0,
				    catgets(catfd, SET, 13397,
				    "Write failed in .inodes on eq %d "
				    "at block 0x%llx"),
				    devp->device[bio_buf_ord].eq,
				    (sam_offset_t)bio_buf_bn);
				clean_exit(ES_io);
			}
		}
		bio_buf_mod = 0;
	}
}


/*
 * ----- debug_print_blocks
 * Print calculated bit maps.
 */

void
debug_print_blocks(int ord)
{
	struct devlist *devlp;
	uint_t *wptr;
	uint_t *ptr1, *ptr2, *ptr3, *ptr4;
	int i;
	int blocks;
	sam_bn_t bn = 0;
	int dt;

	printf("\nordinal = %d "
	    "----------------------------------------------\n",
	    ord);
	devlp = &devp->device[ord];
	if (devlp->type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}
	/* no. of bits */
	blocks = sblock.eq[ord].fs.dau_size * SM_BLKCNT(mp, dt);
	blocks = (blocks + 127) / 128;
	wptr = (uint_t *)devlp->mm;
	for (i = 0; i < blocks; i++) {
		ptr1 = wptr++;
		ptr2 = wptr++;
		ptr3 = wptr++;
		ptr4 = wptr++;
		printf("bn=%.8x   %.8x, %.8x, %.8x, %.8x\n",
		    bn, *ptr1, *ptr2, *ptr3, *ptr4);
		bn += ((32*4) / SM_BLKCNT(mp, dt)) * LG_DEV_BLOCK(mp, dt);
	}
}


/*
 * ----- debug_print_sm_blocks
 * Print bit maps for each small block in the .blocks file.
 */

void
debug_print_sm_blocks()
{
	sam_daddr_t bn;
	int ord;
	offset_t offset;
	struct sam_inoblk *smp;
	struct devlist *devlp;
	int i;

	printf("\nsmall blocks "
	    "----------------------------------------------\n");
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
			error(0, errno,
			    catgets(catfd, SET, 315,
			    ".blocks read failed on eq %d"),
			    devp->device[ord].eq);
			clean_exit(ES_io);
		}
		smp = (struct sam_inoblk *)dcp;
		for (i = 0; i < SM_BLK(mp, MM);
			i += sizeof (struct sam_inoblk)) {
			if (smp->bn == 0xffffffff) {
				return;	/* End of list */
			}
			if (smp->bn != 0) {		/* Full entry */
				devlp = &devp->device[smp->ord];
				bn = smp->bn;
				bn <<= ext_bshift;
				ord = smp->ord;
				printf("eq %d bn=%.8llx, %x\n",
				    devlp->eq, (sam_offset_t)bn, smp->free);
			}
			smp++;
		}
		offset += SM_BLK(mp, MM);
	}
}


/*
 * ----- debug_count_free_blocks
 * Compare the actual bit maps (optr) with the calculated bit maps (iptr).
 * The iptr is expanded to include small blocks.  f = 4 small blocks,
 * however in optr, 1 = 1 large block. Each small block in .blocks
 * must match a free large block.
 */

int
debug_count_free_blocks(int ord)	/* Disk ordinal */
{
	int ii;
	uint_t mask;
	int bit;
	int nbit;
	uint_t *optr;
	uint_t *iptr;
	char *cptr;
	int daul;
	int kk;
	struct devlist *devlp;
	sam_daddr_t bn = 0;
	int blocks;
	int dt;
	int mmord;

	/* compare large dau maps */

	printf("\nordinal = %d "
	    "----------------------------------------------\n",
	    ord);
	devlp = &devp->device[ord];
	if (devlp->type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}
	mmord = sblock.eq[ord].fs.mm_ord;
	daul = sblock.eq[ord].fs.l_allocmap;	/* number of blocks */
	blocks = sblock.eq[ord].fs.dau_size;	/* no. of bits */
	cptr = devlp->mm;
	iptr = (uint_t *)devlp->mm;
	for (ii = 0; ii < daul;
	    ii++, cptr += (SAM_DEV_BSIZE * SM_BLKCNT(mp, dt))) {
		if (d_read(&devp->device[mmord], (char *)dcp, 1,
		    (int)(sblock.eq[ord].fs.allocmap + ii))) {
			error(0, 0,
			    catgets(catfd, SET, 798,
			    "Dau map read failed on eq %d"),
			    devp->device[ord].eq);
			clean_exit(1);
		}
		iptr = (uint_t *)cptr;
		optr = (uint_t *)dcp;
		nbit = 0;
		for (kk = 0; kk < (SAM_DEV_BSIZE / NBPW); kk++, optr++) {
			for (bit = 31; bit >= 0; bit--) {
				blocks--;
				if (blocks < 0) {
					break;
				}
				mask = SM_BITS(mp, dt) <<
				    (31 - nbit - (SM_BLKCNT(mp, dt) - 1));
				if (*optr & (1 << bit)) {	/* If free */
					scount[dt] += LG_DEV_BLOCK(mp, dt);
					if ((*iptr & mask) == 0) {
						/* If allocated */
						printf(catgets(catfd, SET,
						    13399,
						"eq %d bn=%.8llx free in "
						"bit maps, but allocated\n"),
						    devlp->eq,
						    (sam_offset_t)bn);
					}
				} else {		/* If allocated */
					if (find_sm_free_block(ord, bn) == 0) {
						if (*iptr & mask) {
							/* If free */
							printf(catgets(catfd,
							    SET, 13300,
							"eq %d bn=%.8llx "
							"allocated in bit "
							"maps, but free\n"),
							    devlp->eq,
							    (sam_offset_t)bn);
						}
					}
				}
				if (*iptr & mask) {		/* If free */
					if ((*iptr & mask) == mask) {
						ncount[dt] +=
						    LG_DEV_BLOCK(mp, dt);
					} else {
						int ibit, inc;
						int cmask;

						cmask = 0;
						for (ibit = (31 - nbit),
						    inc = 0;
						    inc < SM_BLKCNT(mp, dt);
						    ibit--, inc++) {
							if (*iptr &
							    (1 << ibit)) {
								cmask |=
								    1 << inc;
							}
						}
						printf("eq %d bn=%.8llx, %x\n",
						    devlp->eq,
						    (sam_offset_t)bn, cmask);
					}
				}
				nbit += SM_BLKCNT(mp, dt);
				if (nbit == 32) {
					iptr++;
					nbit = 0;
				}
				bn += LG_DEV_BLOCK(mp, dt);
			}
		}
	}
	return (0);
}

/*
 * ----- find_sm_free_block
 * Print bit maps for each small block in the .blocks file.
 */

int
find_sm_free_block(int input_ord, sam_daddr_t input_bn)
{
	sam_daddr_t bn;
	int ord;
	offset_t offset;
	struct sam_inoblk *smp;
	int i;

	offset = 0;
	while (offset < block_ino->di.rm.size) {
		if (get_bn(block_ino, offset, &bn, &ord, 1)) {
			return (0);
		}
		if (check_bn(block_ino->di.id.ino, bn, ord)) {
			return (0);
		}
		if (ibufsector != bn && ibuford != ord) {
			if (d_read(&devp->device[ord], (char *)ibufp,
			    LG_DEV_BLOCK(mp, MM), bn)) {
				error(0, errno,
				    catgets(catfd, SET, 315,
				    ".blocks read failed on eq %d"),
				    devp->device[ord].eq);
				clean_exit(ES_io);
			}
			ibufsector = bn;
			ibuford = ord;
		}
		smp = (struct sam_inoblk *)ibufp;

		for (i = 0; i < SM_BLK(mp, MM);
			i += sizeof (struct sam_inoblk)) {
			sam_daddr_t bnx;

			bnx = smp->bn;
			bnx <<= ext_bshift;
			if (smp->bn == 0xffffffff) {
				return (0);	/* End of list */
			}
			if (smp->bn != 0) {	/* Full entry */
				if ((input_bn == bnx) &&
				    (input_ord == smp->ord)) {
					return (1);
				}
			}
			smp++;
		}
		offset += SM_BLK(mp, MM);
	}
	return (0);
}


#ifdef DAMFSCK
/*
 * ----- damage_fs - Write .blocks file.
 * Get block and write out .blocks data.
 */

int		/* ERRNO if error, 0 if successful */
damage_fs()
{
	sam_daddr_t bn;
	int ord;
	int ii;
	uint_t *optr;
	uint_t ooo;
	int bit;
	int offset;
	ino_t ino;
	struct sam_perm_inode *dp;

/* Damage small blocks (.blocks) file */
#if	1
	if (get_bn(block_ino, smb_offset, &bn, &ord, 1)) {
		return (1);
	}
	if (check_bn(block_ino->di.id.ino, bn, ord)) {
		return (1);
	}
	if (d_read(&devp->device[ord], (char *)smbbuf, mp->dau.lblocks[bt],
	    bn)) {
		error(0, errno,
		    catgets(catfd, SET, 315,
		    ".blocks read failed on eq %d"),
		    devp->device[ord].eq);
	}
	smbptr = (struct sam_inoblk *)smbbuf;
	smbptr->bn = 0xfa9f8;
	smbptr->free = 0x3;
	if (d_write(&devp->device[ord], (char *)smbbuf, mp->dau.lblocks[bt],
	    bn)) {
		error(0, errno,
		    catgets(catfd, SET, 316,
		    ".blocks write failed on eq %d"),
		    devp->device[ord].eq);
	}
#endif
/* Damage allocation blocks (.blocks) file */
#if	1
	ii = 0;
	ord = 0;
	if (d_read(&devp->device[ord], (char *)dcp, 1,
	    (int)(sblock.eq[ord].fs.allocmap + ii))) {
		error(0, 0,
		    catgets(catfd, SET, 798,
		    "Dau map read failed on eq %d"),
		    devp->device[ord].eq);
		clean_exit(1);
	}
	optr = (uint_t *)dcp;
	bit = 31;
	ooo = *optr;
	*optr = sam_setbit(*optr, bit);
	printf("DAMAGE:   %.8x, %.8x\n", ooo, *optr);
	if (d_write(&devp->device[ord], (char *)dcp, 1,
	    (sblock.eq[ord].fs.allocmap + ii))) {
		error(0, 0,
		    catgets(catfd, SET, 802,
		    "Dau map write failed on eq %d"),
		    devp->device[ord].eq);
		clean_exit(1);
	}
#endif
/* Damage file blocks */
#if	1
	ino = 11;
	offset = SAM_ITOD(ino);
	if (get_bn(inode_ino, offset, &bn, &ord, 1)) {
		return (1);
	}
	if (check_bn(ino, bn, ord)) {
		return (1);
	}
	if (d_read(&devp->device[ord], (char *)dcp, mp->dau.lblocks[bt], bn)) {
		error(0, 0,
		    catgets(catfd, SET, 13391,
		    "Read failed on eq %d at block 0x%llx"),
		    devp->device[ord].eq, (sam_offset_t)bn);
		return (1);
	}
	dp = (struct sam_perm_inode *)(((char *)dcp) +
	    (offset & (LG_BLK(mp)-1)));
	if (dp->di.id.ino == ino) {
		dp->di.extent[0] = 0x74;
		dp->di.extent[8] = 0x170;
		if (d_write(&devp->device[ord], (char *)dcp,
		    mp->dau.lblocks[bt], bn)) {
			error(0, 0,
			    catgets(catfd, SET, 13398,
			    "Write failed on eq %d at block 0x%llx"),
			    devp->device[ord].eq, (sam_offset_t)bn);
		}
	}
#endif
/* Damage base inode link to extended attribute */
#if	1
	ino = 1025;
	offset = SAM_ITOD(ino);
	dt = inode_ino->di.status.b.meta;
	if (get_bn(inode_ino, offset, &bn, &ord, 1)) {
		return (1);
	}
	bn &= ~(LG_DEV_BLOCK(mp, dt) - 1);	/* align boundary */
	if (check_bn(ino, bn, ord)) {
		return (1);
	}
	if (d_read(&devp->device[ord], (char *)dcp, LG_DEV_BLOCK(mp, dt),
	    bn)) {
		error(0, 0,
		    catgets(catfd, SET, 13391,
		    "Read failed on eq %d at block 0x%llx"),
		    devp->device[ord].eq, (sam_offset_t)bn);
		return (1);
	}
	dp = (struct sam_perm_inode *)(((char *)dcp) +
	    (offset & (mp->mi.m_dau[dt].size[LG]-1)));

	if (dp->di.id.ino == ino) {
		printf("extended attribute id %d.%d\n",
		    dp->xattr_id.ino, dp->xattr_id.gen);
		dp->xattr_id.ino = 0;
		dp->xattr_id.gen = 1;
		if (d_write(&devp->device[ord], (char *)dcp,
		    LG_DEV_BLOCK(mp, dt), bn)) {
			error(0, 0,
			    catgets(catfd, SET, 13398,
			    "Write failed on eq %d at block 0x%llx"),
			    devp->device[ord].eq, (sam_offset_t)bn);
		}
	}
	clean_exit(ES_error);
#endif
	return (0);
}
#endif /* DAMFSCK */


/*
 * Quota code.  We maintain three hash tables -- one for each
 * potential quota file (.quota_a, .quota_g, .quota_u).  Each is
 * hashed by its index (admin_id, group-id, or user-id).  Each
 * entry contains a quota entry in which we maintain the in-use
 * counts (files and blocks, online and total).
 *
 * While scanning .inodes, we update all these buckets, and we
 * save the inode # of the .quota_* files.  When all is done,
 * we read up the entries in the .quota_* files and compare
 * our saved entries with the on-disk entries, reporting (and
 * updating if required).
 */

struct quota_tab quota_tab[SAM_QUOTA_DEFD];

/*
 * Allocate the hash tables, and zero all the entries.
 */
void
quota_tabinit(void)
{
	int i;

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		quota_tab[i].qt_tab = (struct quota_ent **)malloc(Q_HASHSIZE *
		    sizeof (struct quota_ent *));
		if (!quota_tab[i].qt_tab) {
			error(0, 0, catgets(catfd, SET, 13376,
			    "Could not malloc %s quota table\n"),
			    quota_types[i]);
			return;
		}
		bzero((char *)quota_tab[i].qt_tab,
		    Q_HASHSIZE*sizeof (struct quota_ent *));
	}
}


/*
 * Search the specified (type) quota hash table for an entry
 * matched to the inode.  If none exists, create it.
 */
struct quota_ent *
quota_getent(struct sam_perm_inode *dp, int type)
{
	int index;
	struct quota_tab *tb;
	struct quota_ent **p2, *p1;

	switch (type) {
	case SAM_QUOTA_ADMIN:
		index = dp->di.admin_id;
		break;
	case SAM_QUOTA_GROUP:
		index = dp->di.gid;
		break;
	case SAM_QUOTA_USER:
		index = dp->di.uid;
		break;
	default:
		index = -1;
	}

	if (index < 0) {
		return (NULL);
	}

	tb = &quota_tab[type];
	for (p2 = &tb->qt_tab[Q_HASH(index)]; *p2 != NULL;
	    p2 = &(*p2)->qe_next) {
		if ((*p2)->qe_index == index) {
			return (*p2);
		}
	}

	p1 = (struct quota_ent *)malloc(sizeof (struct quota_ent));
	if (p1 == NULL) {
		error(0, 0, catgets(catfd, SET, 13377,
		    "quota malloc failed\n"));
		return (NULL);
	}
	bzero((char *)p1, sizeof (struct quota_ent));
	p1->qe_index = index;
	p1->qe_next = NULL;
	bzero(&p1->qe_quota, sizeof (struct sam_dquot));
	*p2 = p1;
	return (p1);
}


/*
 * Enter the file and its blocks into the appropriate hash tables.
 */
void
quota_count_file(struct sam_perm_inode *dp)
{
	int i;
	struct quota_ent *qp;
	struct sam_dquot *qdp;

	if (S_ISSEGS(&dp->di)) {
		return;
	}
	if (S_ISEXT(dp->di.mode)) {
		return;
	}
	if (dp->di.id.ino < min_usr_inum) {
		return;
	}
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = quota_getent(dp, i);
		if (!qp) {
			continue;
		}
		qdp = &qp->qe_quota;
		qdp->dq_folused += 1;
		qdp->dq_ftotused += 1;
		if (S_ISLNK(dp->di.mode) && (dp->di.ext_attrs & ext_sln)) {
			continue;
		}
		if (S_ISREQ(dp->di.mode) && (dp->di.ext_attrs & ext_rfa)) {
			continue;
		}
		qdp->dq_bolused += D2QBLKS(dp->di.blocks);
		qdp->dq_btotused += TOTBLKS(dp);
	}
}


/*
 * Remove a file and its counts from the quota hash tables.
 */
void
quota_uncount_file(struct sam_perm_inode *dp)
{
	int i;
	struct quota_ent *qp;
	struct sam_dquot *qdp;

	if (S_ISSEGS(&dp->di)) {
		return;
	}
	if (S_ISEXT(dp->di.mode)) {
		return;
	}
	if (dp->di.id.ino < min_usr_inum) {
		return;
	}
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = quota_getent(dp, i);
		if (!qp) {
			continue;
		}
		qdp = &qp->qe_quota;
		qdp->dq_folused -= 1;
		qdp->dq_ftotused -= 1;
		if (S_ISLNK(dp->di.mode) && (dp->di.ext_attrs & ext_sln)) {
			continue;
		}
		if (S_ISREQ(dp->di.mode) && (dp->di.ext_attrs & ext_rfa)) {
			continue;
		}
		qdp->dq_bolused -= D2QBLKS(dp->di.blocks);
		qdp->dq_btotused -= TOTBLKS(dp);
	}
}


/*
 * Add only a file's block counts to the quota hash tables.
 */
void
quota_count_blocks(struct sam_perm_inode *dp)
{
	int i;
	struct quota_ent *qp;
	struct sam_dquot *qdp;

	if (S_ISSEGS(&dp->di)) {
		return;
	}
	if (S_ISEXT(dp->di.mode)) {
		return;
	}
	if (dp->di.id.ino < min_usr_inum) {
		return;
	}
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = quota_getent(dp, i);
		if (!qp) {
			continue;
		}
		qdp = &qp->qe_quota;
		if (S_ISLNK(dp->di.mode) && (dp->di.ext_attrs & ext_sln)) {
			continue;
		}
		if (S_ISREQ(dp->di.mode) && (dp->di.ext_attrs & ext_rfa)) {
			continue;
		}
		qdp->dq_bolused += D2QBLKS(dp->di.blocks);
		qdp->dq_btotused += TOTBLKS(dp);
	}
}


/*
 * Remove only a file's block counts from the quota hash tables.
 */
void
quota_uncount_blocks(struct sam_perm_inode *dp)
{
	int i;
	struct quota_ent *qp;
	struct sam_dquot *qdp;

	if (S_ISSEGS(&dp->di)) {
		return;
	}
	if (S_ISEXT(dp->di.mode)) {
		return;
	}
	if (dp->di.id.ino < min_usr_inum) {
		return;
	}
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = quota_getent(dp, i);
		if (!qp) {
			continue;
		}
		qdp = &qp->qe_quota;
		if (S_ISLNK(dp->di.mode) && (dp->di.ext_attrs & ext_sln)) {
			continue;
		}
		if (S_ISREQ(dp->di.mode) && (dp->di.ext_attrs & ext_rfa)) {
			continue;
		}
		qdp->dq_bolused -= D2QBLKS(dp->di.blocks);
		qdp->dq_btotused -= TOTBLKS(dp);
	}
}


/*
 * Add 'adj' disk blocks to the inode's block tally.
 * Convert to 512-byte quota blocks.
 *
 * Note our paranoia here:  we pass in the old and new counts,
 * and adjust with those to avoid potential problems with uints
 * being converted to 64 bits, and potential concomitant sign
 * problems.  We could have passed in a signed 64-bit quantity,
 * but that would require care in each calling.
 */
void
quota_block_adj(struct sam_perm_inode *dp, uint_t old, uint_t new)
{
	int i;
	struct quota_ent *qp;
	struct sam_dquot *qdp;

	if (S_ISEXT(dp->di.mode)) {
		return;
	}
	if (dp->di.id.ino < min_usr_inum) {
		return;
	}
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = quota_getent(dp, i);
		if (!qp) {
			continue;
		}
		qdp = &qp->qe_quota;
		qdp->dq_bolused -= D2QBLKS(old);
		qdp->dq_bolused += D2QBLKS(new);
		qdp->dq_btotused -= D2QBLKS(old);
		qdp->dq_btotused += D2QBLKS(new);
	}
}


/*
 * This routine is called for all filename entries in the root
 * directory, and returns the associated index in the "rootfiles[]"
 * array if the name matches.  We use this to note .quota_[agu],
 * as well as lost+found, .inodes, .ioctl, ...
 */
int
isrootfile(char *name, uint_t len)
{
	int i;

	for (i = 0; i < rootfilecount; i++) {
		if (strncmp(name, rootfiles[i], len) == 0) {
			return (i);
		}
	}
	return (-1);
}

#define		ROUNDDOWN(x, n) ((n) * ((x)/(n)))


/*
 * Validate quota files against the hashed quota entries.
 * Check all allocated quota file blocks, skipping any
 * sparse'd-out blocks (but verifying that any hashed entries
 * therein are zero).
 *
 * Handle direct-mapped quota files correctly, too.
 */
void
check_quota(int fix)
{
	int i;
	struct sam_perm_inode qi;

	/*
	 * Phase 0:
	 *   Don't count the quota files themselves against
	 * our totals.
	 */
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (!quota_file_ino[i]) {
			continue;
		}

		get_inode(quota_file_ino[i], &qi);
		quota_uncount_file(&qi); /* back out quota file's stats */
	}

	/*
	 * Phase I
	 * Verify that all on-disk quota entries match our
	 * accumulated totals.
	 */
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		int err = 0;

		if (!quota_file_ino[i]) {
			continue;
		}
		get_inode(quota_file_ino[i], &qi);
		if (!S_ISREG(qi.di.mode)) {
			error(0, 0,
			    catgets(catfd, SET, 13933,
			    "%s quota file is not a plain file"),
			    quota_types[i]);
			err = 1;
		}
		if (qi.di.status.b.segment) {
			error(0, 0,
			    catgets(catfd, SET, 13934,
			    "%s quota file is segmented"),
			    quota_types[i]);
			err = 1;
		}
		if (qi.di.status.b.offline) {
			error(0, 0,
			    catgets(catfd, SET, 13935,
			    "%s quota file is offline"),
			    quota_types[i]);
			err = 1;
		}
		if (err) {
			continue;
		}
		verify_quota_file(&qi, i, fix);
	}

	/*
	 * Phase II
	 * Check for hash entries that haven't been reconciled.
	 */
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		int j;

		if (!quota_file_ino[i]) {
			continue;
		}
		for (j = 0; j < Q_HASHSIZE; j++) {
			struct quota_ent *qp;

			for (qp = quota_tab[i].qt_tab[j]; qp;
			    qp = qp->qe_next) {
				if (qp->qe_flags == 0) {
					error(0, 0,
					    catgets(catfd, SET, 13941,
					    "%s:  no quota record for "
					    "index %d"),
					    quota_types[i], qp->qe_index);
				}
			}
		}
	}
}


/*
 * Verify quota file against hash buckets.
 *
 * Handle direct mapped files or ordinary files.
 */
void
verify_quota_file(
	struct sam_perm_inode *qip,		/* quota file inode */
	int qt,					/* a/g/u quota */
	int fix)				/* repair quota file? */
{
	if (qip->di.status.b.direct_map) {
		sam_daddr_t bn;

		bn = qip->di.extent[0] << ext_bshift;
		if (bn != 0) {
			int ord = qip->di.extent_ord[0];
			int dt = qip->di.status.b.meta;
			offset_t foffset = 0;
			sam_daddr_t bn0 = bn;

			while ((bn - bn0) < qip->di.extent[1]) {
				verify_quota_block(qip, bn, ord, dt, LG,
				    qt, &foffset, fix);
				bn += LG_DEV_BLOCK(mp, dt);
			}
		}
	} else {
		int i;
		offset_t foffset = 0;

		for (i = 0; i < NOEXT; i++) {
			int ord = qip->di.extent_ord[i];
			sam_daddr_t bn = qip->di.extent[i] << ext_bshift;
			int dt, bt;

			if (i < NDEXT) {
				dt = qip->di.status.b.meta;
				bt = LG;
				if (i < NSDEXT) {
					bt = qip->di.status.b.on_large ?
					    LG : SM;
				}
				verify_quota_block(qip, bn, ord, dt, bt,
				    qt, &foffset, fix);
			} else {
				verify_quota_indirect_block(qip, bn, ord,
				    (i - NDEXT),
				    qt, &foffset, fix);
			}
		}
	}
}


/*
 * Verify an indirect block belonging to a quota file.
 * Recursively calls itself (if double/triple indirect)
 * or verify_quota_block to verify data blocks.
 */
void
verify_quota_indirect_block(
	struct sam_perm_inode *qip,	/* quota file inode ptr */
	sam_daddr_t bn,			/* dev block # */
	int ord,			/* dev ordinal */
	int level,			/* level of indirection */
	int qt,				/* a/g/u quota */
	offset_t *foffset,		/* logical file offset of block bn */
	int fix)			/* repair? */
{
	sam_indirect_extent_t *iep;
	int nstripe = sblock.eq[qip->di.unit].fs.num_group;
	int blksz = LG_DEV_BLOCK(mp, MM);
	int nbytes = blksz * SAM_DEV_BSIZE;
	int dt, i;
	offset_t ofoffset;			/* output file offset */

	if (bn && debug_print) {
		printf("DEBUG: verify_quota_indirect_block"
		    "(qip=%p, bn=%lld, ord=%d, level=%d, "
		    "qt=%d, foffset=%lld, fix=%d)\n",
		    (void *)qip, (long long)bn, ord, level, qt, *foffset, fix);
	}

	ofoffset = DEXT;
	for (i = 0; i < level; i++) {
		ofoffset *= DEXT;
	}
	ofoffset *= LG_BLK(mp, DD) * nstripe;
	ofoffset += *foffset;

	if (bn == 0 || check_bn(qip->di.id.ino, bn, ord)) {
		*foffset = ofoffset;
		return;
	}

	if ((iep = (sam_indirect_extent_t *)malloc(nbytes)) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 601,
		    "Cannot malloc indirect block"));
		clean_exit(ES_malloc);
	}
	if (d_read(&devp->device[ord], (char *)iep, blksz, bn)) {
		printf(catgets(catfd, SET, 13388,
		    "ALERT:  ino %d,\tError reading indirect block "
		    "0x%llx on eq %d\n"),
		    (int)qip->di.id.ino, (sam_offset_t)bn,
		    devp->device[ord].eq);
		SETEXIT(ES_error);
		goto out;
	}

	dt = qip->di.status.b.meta;
	for (i = 0; i < DEXT; i++) {
		sam_daddr_t ibn = iep->extent[i] << ext_bshift;
		int iord = iep->extent_ord[i];

		if (level) {
			verify_quota_indirect_block(qip, ibn, iord, level-1,
			    qt, foffset, fix);
		} else {
			verify_quota_block(qip, ibn, iord, dt, LG, qt,
			    foffset, fix);
		}
	}

out:
	*foffset = ofoffset;
	free(iep);
}


/*
 * Verify data blocks belonging to a quota file.  Reconcile
 * the block's entries against appropriate entries from the
 * accumulated hash table entries.  We trust *foffset to
 * derive the associated quota indices.  (I.e., make sure
 * that *foffset is always updated correctly on return.)
 */
void
verify_quota_block(
	struct sam_perm_inode *qip,	/* quota file inode ptr */
	sam_daddr_t bn,			/* quota file data */
	int ord,			/* length (bytes) of quota file data */
	int dt,				/* meta? */
	int bt,				/* SM/LG */
	int qt,				/* a/g/u quota */
	offset_t *foffset,		/* logical file offset of block bn */
	int fix)			/* repair quota file? */
{
	struct quota_tab *tb = &quota_tab[qt];
	int nstripe = sblock.eq[qip->di.unit].fs.num_group;
	int blksz = mp->mi.m_dau[dt].kblocks[bt];
	int nbytes = blksz * SAM_DEV_BSIZE;
	int cbytes;
	int i, dirty;
	offset_t offset = *foffset, ofoffset;
	struct sam_dquot *dqp, zero_entry;
	struct quota_ent **p;
	char *buf;

	if (bn && debug_print) {
		printf("DEBUG: verify_quot_block"
		    "(qip=%p, bn=%lld, ord=%d, dt=%d, bt=%d, "
		    "qt=%d, foffset=%lld, fix=%d)\n",
		    (void *)qip, (long long)bn, ord, dt, bt, qt, *foffset,
		    fix);
	}

	blksz *= nstripe;
	nbytes *= nstripe;
	ofoffset = *foffset + (blksz * SAM_DEV_BSIZE);

	if (bn == 0 || check_bn(qip->di.id.ino, bn, ord)) {
		*foffset = ofoffset;
		return;
	}

	if (offset > qip->di.rm.size) {
		/*
		 * Should only happen w/o -F flag -- this would normally
		 * be cleaned up prior to this point.
		 */
		error(0, 0, catgets(catfd, SET, 13940,
	"Quota file %s DAU beyond EOF (offset=%lld, EOF=%lld, bn=%lld)"),
		    quota_types[qt], offset, qip->di.rm.size, bn);
		*foffset = ofoffset;
		return;
	}

	if ((buf = malloc(nbytes)) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 13936,
		    "Cannot malloc quota block buffer"));
		clean_exit(ES_malloc);
	}

	bzero(&zero_entry, sizeof (zero_entry));
	for (i = 0; i < nstripe; i++) {
		if (d_read(&devp->device[ord+i], &buf[i * (nbytes/nstripe)],
		    blksz/nstripe, bn)) {
			error(0, 0, catgets(catfd, SET, 13937,
			    "Quota file %s read failed on eq "
			    "%d, ord/block = %d/%lld len %d"),
			    quota_types[qt],
			    devp->device[ord+i].eq,
			    ord+i, bn, nbytes/nstripe);
			clean_exit(ES_io);
		}
	}

	cbytes = nbytes;
	if (offset + cbytes > qip->di.rm.size) {
		if (qip->di.rm.size % sizeof (struct sam_dquot)) {
			error(0, 0, catgets(catfd, SET, 13378,
			    "%s quota file has odd size; length %lld "
			    "should be %lld"),
			    quota_types[qt],
			    qip->di.rm.size,
			    ROUNDDOWN(qip->di.rm.size,
			    sizeof (struct sam_dquot)));
			qip->di.rm.size =
			    ROUNDDOWN(qip->di.rm.size,
			    sizeof (struct sam_dquot));
			if (fix) {
				put_inode(qip->di.id.ino, qip);
			}
		}
		cbytes = qip->di.rm.size - offset;
	}
	dirty = 0;
	for (i = 0; i < cbytes; i += sizeof (struct sam_dquot)) {
		int index = (offset + i)/sizeof (struct sam_dquot);

		dqp = (struct sam_dquot *)&buf[i];
		for (p = &tb->qt_tab[Q_HASH(index)]; *p != NULL;
		    p = &(*p)->qe_next) {
			if ((*p)->qe_index == index) {
				break;
			}
		}
		if (*p != NULL) {
			if (quota_compare_entry(dqp, &(*p)->qe_quota, qt,
			    index)) {
				dirty = 1;
			}
			(*p)->qe_flags = 1;
		} else {
			if (quota_compare_entry(dqp, &zero_entry, qt, index)) {
				dirty = 1;
			}
		}
	}

	if (dirty && debug_print) {
		printf("DEBUG: DIRTY: quota file %s offset %lld "
		    "(block=%lld)\n",
		    quota_types[qt], offset, bn);
	}

	if (dirty && fix) {
		error(0, 0, catgets(catfd, SET, 13938,
		    "Updating quota file %s, indices %ld - %ld"),
		    quota_types[qt],
		    (long)(*foffset/sizeof (struct sam_dquot)),
		    (long)(ofoffset/sizeof (struct sam_dquot) - 1));
		for (i = 0; i < nstripe; i++) {
			if (d_write(&devp->device[ord+i],
			    &buf[i * (nbytes/nstripe)],
			    blksz/nstripe, bn)) {
				error(0, 0, catgets(catfd, SET, 13939,
				    "Quota file %s write failed on eq %d, "
				    "ord/block %d/%lld len %d"),
				    quota_types[qt],
				    devp->device[ord+i].eq,
				    ord+i, bn, nbytes/nstripe);
			}
		}
	}

	*foffset = ofoffset;
	free(buf);
}


/*
 * Compare the in-use fields of two quota entries.
 * If they differ, update the differing fields in the "ondisk"
 * entry, and return "dirty" (!0).
 */
int
quota_compare_entry(struct sam_dquot *ondisk, struct sam_dquot *computed,
	int typ, int index)
{
	int dirty = 0;

	if (ondisk->dq_folused != computed->dq_folused) {
		error(0, 0,
		    catgets(catfd, SET, 13380,
		    "%s quota %d online files,  %lld should be %lld"),
		    quota_types[typ], index,
		    ondisk->dq_folused, computed->dq_folused);
		ondisk->dq_folused = computed->dq_folused;
		dirty++;
	}
	if (ondisk->dq_ftotused != computed->dq_ftotused) {
		error(0, 0,
		    catgets(catfd, SET, 13381,
		    "%s quota %d total files,   %lld should be %lld"),
		    quota_types[typ], index,
		    ondisk->dq_ftotused, computed->dq_ftotused);
		ondisk->dq_ftotused = computed->dq_ftotused;
		dirty++;
	}
	if (ondisk->dq_bolused != computed->dq_bolused) {
		error(0, 0,
		    catgets(catfd, SET, 13382,
		    "%s quota %d online blocks, %lld should be %lld"),
		    quota_types[typ], index,
		    ondisk->dq_bolused, computed->dq_bolused);
		ondisk->dq_bolused = computed->dq_bolused;
		dirty++;
	}
	if (ondisk->dq_btotused != computed->dq_btotused) {
		error(0, 0,
		    catgets(catfd, SET, 13383,
		    "%s quota %d total blocks,  %lld should be %lld"),
		    quota_types[typ], index,
		    ondisk->dq_btotused, computed->dq_btotused);
		ondisk->dq_btotused = computed->dq_btotused;
		dirty++;
	}
	return (dirty);
}


	/*
	 * XXX in "regular" fsck of shared FS, we ought to ensure that
	 * XXX sbp->info.sb.hosts matches inode[4].di.extent[0] << ext_bshift
	 * XXX and inode[4].di.extent_ord[0] == 0.
	 * XXX
	 * XXX What do we do if this is not the case?
	 * XXX if inode[4].di.extent_ord[0] = 0 && inode[4].di.extent[0] != 0,
	 * XXX then sbp->info.sb.hosts = inode[4].di.extent[0]
	 * XXX   sbp->info.sb.hosts = inode[4].di.extent[0];
	 * XXX else
	 * XXX   1) Make FS non-shared (set sbp->info.sb.hosts = 0)?
	 * XXX   2) set inode[4].di.extent_ord[0] = 0
	 * XXX		set inode[4].di.extent[0] = sbp->info.sb.hosts
	 * XXX		and re-fsck?
	 * XXX   3) punt?!
	 * XXX fi
	 */

/*
 * ----- shared_fs_convert
 *
 * Convert a shared filesystem to a non-shared filesystem
 * or vice versa.
 *
 * Things to do:
 *	) Get inode HOST_INO (the shared hosts file inode)
 *	) If converting to shared FS:
 *		) Ensure that the HOST_INO file is what we expect (IFREG,
 *		  correct length, LARGE, online, ...)
 *		) Ensure that the HOST_INO extent[0] isn't 0.
 *	) Ensure that the FS was shared if asking to uncovert or
 *	  non-shared if asking to convert.
 *	) If asking to convert, copying the shifted HOST_INO block
 *	  number from extent[0] into the superblock; or
 *	) if asking to unconvert, zeroing the host file offset in
 *	  the superblock.
 *
 *	If we return, the (updated) superblocks will be written out,
 *	and the conversion effected.
 */
void
shared_fs_convert(struct sam_sblk *sbp)
{
	ino_t ino;
	struct sam_perm_inode hostino;

	ino = SAM_HOST_INO;
	if (get_inode(ino, &hostino)) {
		error(0, 0, catgets(catfd, SET, 13927,
		    "shared_fs_convert: get_inode(HOST_INO) error"));
		clean_exit(ES_error);
	}
	if (cvt_to_shared) {	/* Unshared V2 -> Shared (V2) FS */
		if (hostino.di.id.ino != SAM_HOST_INO ||
		    hostino.di.id.gen != SAM_HOST_INO ||
		    !SAM_CHECK_INODE_VERSION(hostino.di.version) ||
		    hostino.di.version != sblk_version ||
		    !S_ISREG(hostino.di.mode) ||
		    (hostino.di.rm.size != SAM_HOSTS_TABLE_SIZE &&
		    hostino.di.rm.size != SAM_LARGE_HOSTS_TABLE_SIZE) ||
		    !hostino.di.status.b.on_large ||
		    hostino.di.status.b.offline) {
			error(0, 0, catgets(catfd, SET, 13928,
			    "shared_fs_convert: hosts file inode problem"));
			clean_exit(ES_error);
		}
		if (sbp->info.sb.magic == SAM_MAGIC_V2 &&
		    hostino.di.extent_ord[0] != 0) {
			error(0, 0, catgets(catfd, SET, 13929,
			    "shared_fs_convert: hosts file ordinal extent "
			    "err (%d != 0)"),
			    (int)hostino.di.extent_ord[0]);
			clean_exit(ES_error);
		}
		if (hostino.di.extent[0] == 0) {
			error(0, 0, catgets(catfd, SET, 13930,
			    "shared_fs_convert: hosts file extent err (%#x)"),
			    hostino.di.extent[0]);
			clean_exit(ES_error);
		}
		sbp->info.sb.hosts = hostino.di.extent[0] << ext_bshift;
		sbp->info.sb.hosts_ord = hostino.di.extent_ord[0];
	} else if (cvt_to_nonshared) {	/* Shared (V2) FS -> unshared V2 FS */
		sbp->info.sb.hosts = 0;
	}
}


/*
 * Return TRUE if we're the shared server.
 */
static int
shared_server(struct devlist *dp, offset_t hblk, upath_t server)
{
	struct sam_host_table_blk *hosts;
	struct sam_host_table *hp;
	upath_t hostname;
	int err;
	int htsize = SAM_HOSTS_TABLE_SIZE;

	hosts = (struct sam_host_table_blk *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
	if (hosts == NULL) {
		error(0, ENOMEM, catgets(catfd, SET, 13902,
		    "Read of shared fs hosts data failed"));
		return (TRUE);
	}
	hp = &hosts->info.ht;

	/*
	 * Return TRUE if the hosts file is so hosed that we can't
	 * determine who the server is.
	 */
again:
	if (d_read(dp, (char *)hosts, htsize/SAM_DEV_BSIZE, hblk)) {
		error(0, 0, catgets(catfd, SET, 13902,
		    "Read of shared fs hosts data failed"));
		free(hosts);
		return (TRUE);
	}
	if (hp->length > htsize) {
		/*
		 * Need to read a large hosts table.
		 */
		if (htsize == SAM_LARGE_HOSTS_TABLE_SIZE) {
			error(0, 0, catgets(catfd, SET, 13903,
			    "Bad hosts data -- see samsharefs(1M) to fix"));
			free(hosts);
			return (TRUE);
		}
		htsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		goto again;
	}
	if (hp->cookie == SAM_HOSTS_COOKIE) {
		if (hp->version != SAM_HOSTS_VERSION4) {
			error(0, 0, catgets(catfd, SET, 13903,
			    "Bad hosts data -- see samsharefs(1M) to fix"));
			free(hosts);
			return (TRUE);
		}
	} else if (hp->cookie == 0) {
		if (hp->version < SAM_HOSTS_VERSION_MIN ||
		    hp->version > SAM_HOSTS_VERSION_MAX) {
			error(0, 0, catgets(catfd, SET, 13903,
			    "Bad hosts data -- see samsharefs(1M) to fix"));
			free(hosts);
			return (TRUE);
		}
	} else {
		error(0, 0, catgets(catfd, SET, 13903,
		    "Bad hosts data -- see samsharefs(1M) to fix"));
		free(hosts);
		return (TRUE);
	}
	if (hp->length < sizeof (sam_host_table_t) ||
	    hp->length > SAM_LARGE_HOSTS_TABLE_SIZE) {
		error(0, 0, catgets(catfd, SET, 13903,
		    "Bad hosts data -- see samsharefs(1M) to fix"));
		free(hosts);
		return (TRUE);
	}
	if (!SamGetSharedHostName(hp, hp->server, server)) {
		error(0, 0, catgets(catfd, SET, 13903,
		    "Bad hosts data -- see samsharefs(1M) to fix"));
		free(hosts);
		return (TRUE);
	}
	if ((err = getHostName(hostname, sizeof (hostname), fsname)) !=
	    HOST_NAME_EOK) {
		char path[MAXPATHLEN];
		int syserr = errno;

		/*
		 * Output detailed host name error info
		 */
		snprintf(path, sizeof (path), "%s/nodename.%s", SAM_CONFIG_PATH,
		    fsname);
		switch (err) {
			case HOST_NAME_EFAIL:
				error(0, syserr, catgets(catfd, SET, 13973,
				    "%s: gethostname failed"), fsname);
				break;
			case HOST_NAME_ETOOSHORT:
				error(0, 0, catgets(catfd, SET, 13974,
				    "%s: gethostname(): name too short"),
				    fsname);
				break;
			case HOST_NAME_ETOOLONG:
				error(0, 0, catgets(catfd, SET, 13975,
				    "%s: gethostname(): name too long"),
				    fsname);
				break;
			case HOST_NAME_EBADCHAR:
				error(0, 0, catgets(catfd, SET, 13998,
				    "%s: gethostname(): bad character"),
				    fsname);
				break;
			case HOST_FILE_EREAD:
				error(0, syserr, catgets(catfd, SET, 13976,
				    "%s: Read of %s failed"), fsname, path);
				break;
			case HOST_FILE_ETOOSHORT:
				error(0, 0, catgets(catfd, SET, 13977,
				    "%s: Bad hostname in %s: name too short"),
				    fsname, path);
				break;
			case HOST_FILE_ETOOLONG:
				error(0, 0, catgets(catfd, SET, 13978,
				    "%s: Bad hostname in %s: name too long"),
				    fsname, path);
				break;
			case HOST_FILE_EBADCHAR:
				error(0, 0, catgets(catfd, SET, 13999,
				    "%s: Hostname has bad character in %s"),
				    fsname, path);
				break;
		}
		error(0, 0, catgets(catfd, SET, 13904,
		    "getHostName() failed -- can't get local hostname"));
		free(hosts);
		return (FALSE);
	}
	if (strncasecmp(server, hostname, sizeof (hostname)) == 0) {
		free(hosts);
		return (TRUE);
	}
	free(hosts);
	return (FALSE);
}


/*
 * Act on the state of the log:
 *
 * No log present		Continue.
 * Log ok and empty		Continue.
 * Log ok and not empty		Force a writeable mount. (exit).
 * Log corrupt			If -F was specified throw away log
 *				Else indicate that -F would have tossed log.
 */
void
lqfs_log_dispatch(int status)
{
	switch (status)  {
	/* Log not there, keep going. */
	case LOG_NONE:
#ifdef SAM_QFS_JOURNALING_SUPPORTED
		printf(catgets(catfd, SET, 13982, "No log present\n"));
#endif /* SAM_QFS_JOURNALING_SUPPORTED */
		break;

	/* Log empty, keep going. */
	case LOG_EMPTY:
		printf(catgets(catfd, SET, 13983, "Empty log found\n"));
		break;

	/* Transactions to be replayed. Force a writeable mount. */
	case LOG_NOT_EMPTY:
		error(0, 0,
		    catgets(catfd, SET, 13984,
		    "ALERT: Log contains transactions; mount filesystem "
		    "writeable to replay them\n"));
		clean_exit(ES_log);
		break;
	/* Corrupt log */
	case LOG_CORRUPT:
		/*
		 * Fix up superblock like mkfs does for new filesystems.
		 */
		if (repair_files) {
			error(0, 0, catgets(catfd, SET, 13985,
			    "NOTICE: Superblock points to a bad log; "
			    "freeing blocks\n"));
			nblock.info.sb.logbno = 0ULL;
			nblock.info.sb.qfs_rolled = 0;
			nblock.info.sb.logord = 0;
			nblock.info.sb.qfs_clean = FSCLEAN;
		/*
		 * Indicate that a corrupt log was found.
		 */
		} else {
			error(0, 0, catgets(catfd, SET, 13986,
			    "NOTICE: Superblock points to a bad log, "
			    "\t-F will correct\n"));
		}
		break;
	}
}


/*
 * Check that block number for the extent_block_t area is within range.
 * Verify that the extent_block_t checksum is correct.  Verify
 * that the block numbers referenced by the extent_block_t structure are
 * within range.  Read in enough of first block pointed to by extents
 * to verify checksum of header info for log.  Determine if log is empty.
 * Call lqfs_log_dispatch() to decide what to do with the log.
 */
void
lqfs_log_validate(int32_t logbno, int logord, void (*cb)(int, daddr32_t))
{
	int		status = 0;
	extent_t	*ep;
	extent_block_t	*ebp;
	int		i, j;
	struct devlist	*devlp = &devp->device[logord];
	uint32_t	*buf1 = NULL;
	uint32_t	*buf2 = NULL;
	int		log_dau_num_oneks = nblock.info.sb.dau_blks[SM];
	int		log_dau_size = log_dau_num_oneks * SAM_DEV_BSIZE;
	daddr_t		fno;
	uint32_t	nfno;
	ml_odunit_t	*od;

	/* If the log isn't there. */
	if (logbno == 0) {
		status = LOG_NONE;
		goto errout;
	}

	/* Is the block number in range? */
	if (check_bn(SAM_LOG_INO, logbno, logord)) {
		status = LOG_CORRUPT;
		if (verbose_print || debug_print) {
			error(0, 0,
			    catgets(catfd, SET, 13992,
			    "NOTICE: logbno in superblock out of range "
			    "%d %d\n"),
			    logbno, logord);
		}
		goto errout;
	}

	/* Malloc memory to hold extent_block_t */
	if ((buf1 = (uint32_t *)malloc(log_dau_size)) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 13987,
		    "ALERT: Cannot malloc log buffer\n"));
		clean_exit(ES_malloc);
	}

	/* Read in log extent_block_t structure off disk */
	if (d_read(devlp, (char *)buf1, log_dau_num_oneks, logbno)) {
		error(0, 0,
		    catgets(catfd, SET, 13988,
		    "ALERT: Log extent read failed on eq %d\n"),
		    devlp->eq);
		clean_exit(ES_io);
	}

	/* Verify checksum. */
	ebp = (extent_block_t *)buf1;
	if (!log_checksum(&ebp->chksum, (int32_t *)buf1, SAM_BLK)) {
		status = LOG_CORRUPT;
		if (verbose_print || debug_print) {
			error(0, 0,
			    catgets(catfd, SET, 13993,
			    "NOTICE: bad cksum for log extent_block_t "
			    "block %d %d\n"),
			    logbno, logord);
		}
		goto errout;
	}

	/* Verify that the extent block references are in range */
	for (i = 0, ep = &ebp->extents[0]; i < ebp->nextents; ++i, ++ep) {
		fno = ep->pbno;
		nfno = ep->nbno >> SAM2SUN_BSHIFT;

		if (debug_print) {
			error(0, 0,
			    catgets(catfd, SET, 13989,
			    "INFO: Log extent # %d - startbn %d %d "
			    "1K blocks\n"),
			    i, fno, nfno);
		}

		/* Is the first block number in range? */
		if (check_bn(SAM_LOG_INO, fno, ep->ord)) {
			status = LOG_CORRUPT;
			if (verbose_print || debug_print) {
				error(0, 0, catgets(catfd, SET, 13994,
				    "NOTICE: First block out of range in "
				    "log extent # %d - startbn %d %d "
				    "1K blocks\n"),
				    i, fno, nfno);
			}
			goto errout;
		}

		/* Is the last block number in range? */
		if (check_bn(SAM_LOG_INO, fno + (nfno - 1), ep->ord)) {
			status = LOG_CORRUPT;
			if (verbose_print || debug_print) {
				error(0, 0, catgets(catfd, SET, 13995,
				    "NOTICE: Last block out of range in "
				    "log extent # %d - lastbn %d \n"),
				    i, fno + (nfno - 1));
			}
			goto errout;
		}
	}

	/* Malloc memory to hold part of first extent */
	if ((buf2 = (uint32_t *)malloc(log_dau_size)) == NULL) {
		error(0, 0, catgets(catfd, SET, 13987,
		    "ALERT: Cannot malloc log buffer\n"));
		clean_exit(ES_malloc);
	}

	/* Read in head and tail pointers for log */
	if (d_read(devlp, (char *)buf2, log_dau_num_oneks,
	    ebp->extents[0].pbno)) {
		error(0, 0, catgets(catfd, SET, 13990,
		    "ALERT: Log head and tail read failed on eq %d\n"),
		    devlp->eq);
		clean_exit(ES_io);
	}

	/*
	 * Check checksum and version.
	 * Check head and tail pointers to determine if log is
	 * empty or not.
	 */
	od = (ml_odunit_t *)buf2;
	if ((od->od_chksum != od->od_head_ident + od->od_tail_ident) ||
	    (od->od_version != LQFS_VERSION_LATEST)) {
		if (verbose_print || debug_print) {
			error(0, 0, catgets(catfd, SET, 13996,
			    "INFO: Log header bad bn %d od_chksum %d "
			    "od_head_ident %d od_tail_ident %d\n"),
			    ebp->extents[0].pbno,
			    od->od_chksum, od->od_head_ident,
			    od->od_tail_ident);
		}
		status = LOG_CORRUPT;
		goto errout;
	}

	/*
	 * We have a good log, now decide if it contains transactions or not.
	 */
	if (nblock.info.sb.qfs_rolled == FS_ALL_ROLLED) {
		status = LOG_EMPTY;
	} else if (nblock.info.sb.qfs_rolled == FS_NEED_ROLL) {
		status = LOG_NOT_EMPTY;
	} else {
		if (verbose_print || debug_print) {
			error(0, 0, catgets(catfd, SET, 13997,
			    "ALERT: Unexpected state; valid log blocks "
			    "but qfs_rolled invalid %d\n"),
			    nblock.info.sb.qfs_rolled);
		}
		status = LOG_CORRUPT;
		goto errout;
	}

	/*
	 * If we get to this point, the log is not corrupt.
	 * The next block of code is for recording all the blocks
	 * used by the log so that they won't be freed later.
	 */

	/*
	 * Note block that holds extent_block_t.
	 */
	if (cb != NULL)
		(*cb)(logord, logbno);

	/*
	 * Walk the extents again to callback for each dau in use.
	 * This will account for all the blocks used by the log so
	 * that they won't be freed later by the block accounting
	 * logic.
	 */
	for (i = 0, ep = &ebp->extents[0]; cb && i < ebp->nextents;
	    ++i, ++ep) {
		fno = ep->pbno;
		nfno = ep->nbno >> SAM2SUN_BSHIFT;

		/* Make the call back for each small dau used by the log. */
		for (j = 0; j < nfno; j += log_dau_num_oneks,
		    fno += log_dau_num_oneks) {
			if (cb != NULL)
				(*cb)(ep->ord, fno);
		}
	}

errout:
	if (buf1) {
		free(buf1);
	}
	if (buf2) {
		free(buf2);
	}
	/* Act */
	lqfs_log_dispatch(status);
}


/*
 * Mark block as in use.
 */
void
note_block(int ord, daddr32_t blk)
{
	int status;
	struct devlist *devlp;
	int dt;

	devlp = &devp->device[ord];
	if (devlp->type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}
	if ((status = count_block(SAM_LOG_INO, dt, SM, blk, ord)) != 0) {
		error(0, 0, catgets(catfd, SET, 13991,
		    "ALERT: error %d counting block %d ord %d\n"),
		    status, blk, ord);
	}
}


/*
 * log_setsum() and log_checksum() are equivalent to lqfs.c:setsum()
 * and lqfs.c:checksum().
 */
void
log_setsum(int32_t *sp, int32_t *lp, int nb)
{
	int32_t csum = 0;

	*sp = 0;
	nb /= sizeof (int32_t);
	while (nb--)
		csum += *lp++;
	*sp = csum;
}


int
log_checksum(int32_t *sp, int32_t *lp, int nb)
{
	int32_t ssum = *sp;

	log_setsum(sp, lp, nb);
	if (ssum != *sp) {
		*sp = ssum;
		return (0);
	}
	return (1);
}
