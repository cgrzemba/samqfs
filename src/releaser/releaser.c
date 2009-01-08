/*
 * releaser.c - release disk space to low water mark
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

/*
 *  The purpose of the releaser is to find the "best" on-line files
 *  and release the cache-disk space they occupy in an attempt to
 *  bring the free space in the cache down to or just below the
 *  configured low-water mark.  The program may be started from a
 *  command line, but is most likely started by sam-fsd when the
 *  filesystem tells sam-fsd that it's above the configured high-water
 *  mark.
 *
 *  After processing command-line options, the releaser reads the
 *  entire .inodes file from the filesystem.  As it's reading the inodes,
 *  it discards any which are disqualified for releasing, for such
 *  reasons as "file is marked release -n", or "file is already offline",
 *  etc.  (These are conditions checked for in acceptable_candidate()).
 *
 *  As acceptable candidates for releasing are found, a priority is
 *  calculated.  The priority is based on a weighted sum of the
 *  size and age of the file.  The candidates are added to a priority-ordered
 *  list of candidates.  The list is only list_size (10,000 default) elements
 *  long, so after that many entries are added, we begin throwing away
 *  candidates which are of worse priority than the worst entry in the
 *  list.
 *
 *  Once all inodes have been read, the releaser begins to release files.
 *  Beginning with the best-priority file, a system call is issued which
 *  releases the file and returns the number of blocks now free in the
 *  filesystem.  If that number is below the configured low-water mark,
 *  the releaser terminates, having done its job.  Otherwise, the
 *  next file is released, etc.
 *
 *  If all list_size files are released, and we still are above lwm, we
 *  compute the ratio of (size left to do)/(size released);  if this number
 *  is greater than 1, we make list_size bigger by this factor.  Then we
 *  loop to repeat the entire process.
 *
 */

#pragma ident "$Revision: 1.60 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#include <time.h>
#include <signal.h>
#include <grp.h>
#include <strings.h>


#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "sam/sam_trace.h"
#include "sam/syscall.h"
#include "sam/fs/sblk.h"
#include "pub/stat.h"
#include <aml/id_to_path.h>

#include "releaser.h"
#include "plist.h"
#include <sam/lint.h>

#define	max(a, b) (((a) > (b)) ? (a) : (b))
#define	min(a, b) (((a) < (b)) ? (a) : (b))

#ifndef INO_BLK_FACTOR
#define	INO_BLK_FACTOR 32
#endif

#define	CFG_FILE	SAM_CONFIG_PATH"/releaser.cmd"


#define	INODE_SIZE_THRSHLD (1000000 * SAM_ISIZE) /* 1M inodes (bytes) */
#define	SML_DEF_SIZE 30000
#define	LRG_DEF_SIZE 100000

/* Buffer for inodes read from disk */
static union sam_di_ino
	inodes[(INO_BLK_FACTOR * INO_BLK_SIZE) / sizeof (union sam_di_ino) + 2];

/* Statistics buffer */
static struct {
	clock_t seconds;
	int	already_offline;
	int	archnodrop;
	int	damaged;
	int	extension_inode;
	int	negative_age;
	int	nodrop;
	int	not_regular;
	int	number_in_list;
	int	rearch;
	int	released_files;
	int	too_new_residence_time;
	int	too_small;
	int	total_candidates;
	int	total_inodes;
	int	wrong_inode_number;
	int	zero_arch_status;
	int	zero_inode_number;
	int	zero_mode;
	int	files_to_verify;
	time_t begin;
} stats;

/*  prototypes */
char	*ctime_r(const time_t *clock, char *buf, int buflen);
static	char *format_time(clock_t);
static	void find_fs(char *mnt_point);
static	int acceptable_candidate(struct sam_perm_inode *inode,
		float *priority, struct data *data);
static	int big_enough(struct sam_perm_inode *inode);
static	long long release_a_file(struct sam_fs_info *mp, sam_id_t id);
static	long long where_is_fs(char *path);
static	long long where_is_lwm(char *path, int low);
static	void open_log_file(void);
static	void process_cmd_line(int argc, char **argv);
static	void releaser_header(void);
static	void releaser_trailer(void);
static	void show_stats_then_zero(void);
static	void sigalrm_handler(int ignored);
static	void sigint_handler(int ignored);
static  void set_default_listsize(void);

/*  The current time, and how often (in seconds) it should be refreshed */
static time_t now;
#define	ALARM_INTERVAL 15

/*  my name */
char *program_name = SAM_RELEASER;

/*
 *  command-line arguments, command-file arguments, or things derived
 *  therefrom
 */
char log_pathname[MAXPATHLEN];
char *fs_name;						/* File system name */
float weight_age_access = 1.0, weight_age_modify = 1.0;
float weight_age_residence = 1.0;
float weight_size = 1.0, weight_age = 1.0;
int debug_partial = FALSE;
int display_all_candidates = FALSE;
int list_size = 0;		/* size of candidate list */
int min_residence_age = 600;	/* minimum online age in seconds = 10 min. */
int release = TRUE;
int rearch_release = TRUE;
int use_one_age_weight = FALSE;
int use_three_age_weights = FALSE;
static char *mnt_point;
static char inode_pathname[MAXPATHLEN];
static int inode_fd;
static int low_water;
static struct sam_fs_info *mp;
static int block_size[2];
static int kblock_size[2];
static int block_shift[2];


#if !defined(TEST)
/*  releaser */
int
main(
	int argc,
	char **argv
) {
	float priority;
	int expected_ino;
	int inode_i;
	int ngot;
	int ninodes;

	long long blocks_freed;		/* blocks freed this pass */
	long long blocks_now_free;
	long long lwm_blocks;
	int  release_ratio;

	struct data data;
	char msgbuf[MAX_MSGBUF_SIZE] = {0};

	log = NULL;
	(void) clock();    /* initialize clock() interface */
	stats.seconds = clock()/CLOCKS_PER_SEC;
	stats.begin = time((time_t *)NULL);

	/*
	 * We behave slightly differently depending on if we were started by
	 * sam-fsd or from a command line.  See which case has happened.
	 */
	Daemon = strcmp(GetParentName(), SAM_FSD) == 0;
	CustmsgInit(Daemon, NULL);
	if (Daemon) {
		/*
		 * sam-fsd starts us with one argument - the filesystem name.
		 * Send solaris event so it can be populated as email, trap etc.
		 */
		snprintf(msgbuf, sizeof (msgbuf),
			GetCustMsg(31272), argv[1]);
		(void) PostEvent(FS_CLASS, "HwmExceeded",  31272,
		    LOG_WARNING, msgbuf, NOTIFY_AS_FAULT | NOTIFY_AS_EMAIL);

		if (chdir(SAM_VARIABLE_PATH"/releaser") == -1) {
			LibFatal(chdir, SAM_VARIABLE_PATH"/releaser");
		}
		find_fs(argv[1]);
		if (read_cmd_file(CFG_FILE) > 0) {
			SendCustMsg(HERE, 8000, argv[0]);
			SendCustMsg(HERE, 8003, argv[0]);
			exit(EXIT_FAILURE);
		}
		open_log_file();
	} else {
		if (argc < 4 || argc > 5) {
			fprintf(stderr,
			    "Usage: %s file-system low-water weight-size "
			    "[ weight-age ]\n", program_name);
			exit(EXIT_USAGE);
		}

		/* Handle file system name */
		find_fs(argv[1]);

		/* Handle the command line arguments. */
		if (read_cmd_file(CFG_FILE) > 0) {
			/* An appropriate message has been written to stderr */
			exit(EXIT_FAILURE);
		}
		process_cmd_line(argc, argv);
		log = stdout;
	}

	/*
	 * If no age weights were specified, then use the traditional one
	 * weight.
	 */
	if (!use_one_age_weight && ! use_three_age_weights) {
		use_one_age_weight = TRUE;
	}

	/* If no list size was specified, set a default. */
	if (list_size == 0) {
		set_default_listsize();
	}

	/*
	 * set up signal handling.  SIGINT makes us exit; SIGALRM makes us
	 * update the global "now" variable.  sam-fsd will send us a SIGHUP
	 * on a configure; ignore it.
	 */
	now = time(NULL);
	if (signal(SIGALRM, &sigalrm_handler) == (void (*)(int))SIG_ERR) {
		SysError(HERE, "Cannot set handler for SIGALRM");
		exit(EXIT_FAILURE);
	}
	alarm(ALARM_INTERVAL);

	if (signal(SIGINT, &sigint_handler) == (void (*)(int))SIG_ERR) {
		SysError(HERE, "Cannot set handler for SIGINT");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGHUP, SIG_IGN) == (void (*)(int))SIG_ERR) {
		SysError(HERE, "Cannot set handler for SIGHUP");
		exit(EXIT_FAILURE);
	}

	/* Set up end-of-job actions, then perform beginning-of-job actions */
	atexit(releaser_trailer);
	releaser_header();

	/* Echo parameters.  Probably just for debugging */
	fprintf(log, "inode pathname          %s\n", inode_pathname);
	fprintf(log, "low-water mark          %d%%\n", low_water);
	fprintf(log, "list_size               %d\n", list_size);
	fprintf(log, "min_residence_age       %d sec.\n", min_residence_age);
	fprintf(log, "weight_size             %g\n", weight_size);
	if (use_three_age_weights) {
		fprintf(log, "weight_age_access       %g\n", weight_age_access);
		fprintf(log, "weight_age_modify       %g\n", weight_age_modify);
		fprintf(log, "weight_age_residence    %g\n",
				weight_age_residence);
	} else {
		fprintf(log, "weight_age              %g\n", weight_age);
	}
	fprintf(log, "fs equipment number     %d\n", mp->fi_eq);
	fprintf(log, "family-set name         %s\n", fs_name);
#define	yorn(x) ((x)?"yes":"no")
	fprintf(log, "started by sam-fsd?     %s\n", yorn(Daemon));
	fprintf(log, "release files?          %s\n", yorn(release));
	fprintf(log, "release rearch files?   %s\n", yorn(rearch_release));
	fprintf(log, "display_all_candidates? %s\n",
	    yorn(display_all_candidates));

	lwm_blocks = where_is_lwm(mnt_point, low_water);
	blocks_now_free = where_is_fs(mnt_point);

	fprintf(log, "---before scan---\n");
	fprintf(log, "blocks_now_free:     %lld\n", blocks_now_free);
	fprintf(log, "lwm_blocks:          %lld\n", lwm_blocks);
	(void) fflush(log);

	/* Loop while we have work to do */
	while (blocks_now_free < lwm_blocks) {
		int number_of_entries_in_list;

		fprintf(log, "---scanning---\n");

		/* initialize the priority list for this loop */
		init();
		remake_lists(list_size);
		expected_ino = 0;

		/* begin processing .inodes at its beginning */
		if (lseek(inode_fd, 0, SEEK_SET) != 0) {
			SysError(HERE, "Could not lseek() .inodes to BOF");
			exit(EXIT_FAILURE);
		}

		/*
		 * read the inodes file, build priority-ordered list
		 * of candidates
		 */
		while ((ngot = read(inode_fd, &inodes,
				INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {
			ninodes = ngot / sizeof (union sam_di_ino);

			for (inode_i = 0; inode_i < ninodes; inode_i++) {
				expected_ino ++;
				stats.total_inodes++;

				if ((inodes[inode_i].inode.di.id.ino == 0) ||
				    (inodes[inode_i].inode.di.id.gen == 0)) {
					stats.zero_inode_number++;
					continue;
				}

				if (inodes[inode_i].inode.di.id.ino !=
				    expected_ino) {
					stats.wrong_inode_number++;
					continue;
				}

				if (acceptable_candidate(&inodes[inode_i].inode,
						&priority, &data)) {
					stats.total_candidates++;
					add_entry(priority, &data);
				}
			}

		}

		/* finish up the priority-list processing */
		number_of_entries_in_list = stats.number_in_list = finish();

		blocks_freed = 0LL;

		if (number_of_entries_in_list != 0) {
			/*
			 * release candidates as long as there are any,
			 * and we're still above LWM
			 */
			while (remove_entry(&priority, &data) &&
					blocks_now_free < lwm_blocks) {
				char *formatted_time;
				char seg_num[16] = "S0";

				/* release_a_file() can return -1 on error. */
				blocks_now_free = release_a_file(mp, data.id);
				if (blocks_now_free < 0) continue;
				blocks_freed += data.info.size;
				stats.released_files ++;
				if (data.info.seg_num) {
					sprintf(seg_num, "S%d",
					    data.info.seg_num);
				}

#define	VERBOSE
#ifdef VERBOSE
				{
					char *pathname;
					pathname =
					    id_to_path(mnt_point, data.id);
					if (data.info.age_key != '3') {
						formatted_time =
						    format_time(data.info.time);
						fprintf(log,
						    "%g (%c: %s) %ld min, "
						    "%lld blks %s %s\n",
						    priority,
						    data.info.age_key,
						    formatted_time,
						    data.info.age,
						    data.info.size,
						    seg_num, pathname);
					} else {
						fprintf(log,
						    "%g %lld blks %s %s\n",
						    priority,
						    data.info.size,
						    seg_num, pathname);
					}

				}
#else
				if (data.info.age_key != '3') {
					formatted_time =
					    format_time(data.info.time);
					fprintf(log,
					    "%g (%c: %s) %d min, "
					    "%lld blks %s %u\n",
					    priority, data.info.age_key,
					    formatted_time, data.info.age,
					    data.info.size, seg_num,
					    data.id.ino);
				} else {
					fprintf(log,
					    "%g %lld blks %s %u\n",
					    priority, data.info.size,
					    seg_num, data.id.ino);
				}
#endif
			}
		} else {
			fprintf(log, "No releaser candidates were found.");
		}

		/*
		 * if the last call to release_a_file() failed,
		 * blocks_now_free is -1
		 */
		blocks_now_free = where_is_fs(mnt_point);

		fprintf(log, "---after scan---\n");
		fprintf(log, "blocks_now_free:       %lld\n", blocks_now_free);
		if (release) {
			fprintf(log,
			    "blocks_freed:          %lld\n", blocks_freed);
		} else {
			fprintf(log,
			    "blocks to free (no release): %lld\n",
			    blocks_freed);
		}
		fprintf(log, "lwm_blocks:            %lld\n", lwm_blocks);
		show_stats_then_zero();

		/*
		 * If no candidates or nothing accomplished, give up for now.
		 */

		if (!release || number_of_entries_in_list == 0 ||
		    blocks_freed == 0) {
			(void) fflush(log);
			break;
		}

		/*
		 * Adjust list_size based on previous pass results.
		 * If we filled the list with candidates and didn't
		 * get to LWM, increase the list size by the ratio
		 * (blocks left to free)/(blocks freed this pass)
		 * up to a maximum of LIST_MAX entries.
		 */
		if (blocks_freed > 0) {
			release_ratio =
				(lwm_blocks - blocks_now_free)/blocks_freed;
		} else {
			release_ratio = 1;
		}
		fprintf(log, "release_ratio:         %d\n", release_ratio);
		if ((release_ratio > 1) &&
		    (number_of_entries_in_list == list_size)) {
			int	new_size = list_size * release_ratio;

			list_size =
			    (new_size <= LIST_MAX) ? new_size : LIST_MAX;
			fprintf(log, "list_size adjusted to: %d\n", list_size);
		}
		(void) fflush(log);
	}

	return (EXIT_SUCCESS);
}

#endif

/*
 *	big_enough - Return true if blocks will be gained on a release.
 *	This function handles checking for a partial-release file
 *	which is already released.    Another case is if the file
 *	is zero size.
 */
static int
big_enough(struct sam_perm_inode *ino)
{
	long	byte_count, blocks;
	long	partial = 0;	/* bytes in the always-on-disk portion */
	int	pextents = 0;	/* are there partial extents?  That is, */
				/* extents which are always on disk.  */

	/*
	 * Check for "release -p" and partial extents.
	 */
	if (ino->di.status.b.bof_online) {
		partial = ino->di.psize.partial * SAM_DEV_BSIZE;
		pextents = ino->di.status.b.pextents;
		if (debug_partial) {
			fprintf(log,
			    "inode %#x is new-format. "
			    "partial %ld pextents %d\n",
			    ino->di.id.ino, partial, pextents);
		}
	}

	/*
	 * If the file is "release -p" and it has partial extents,
	 * we need to figure out if there are blocks past the partial size
	 */
	if (ino->di.status.b.bof_online && pextents) {
		int sm_off;

		sm_off = NSDEXT * block_size[SM];
		if (ino->di.status.b.on_large) {
			byte_count = ((partial + block_size[LG] - 1) >>
			    block_shift[LG]) * block_size[LG];
		} else {
			if (partial <= sm_off) {
				byte_count = ((partial + block_size[SM] - 1) >>
				    block_shift[SM]) * block_size[SM];
			} else {
				byte_count = sm_off +
				    ((((partial - sm_off) +
				    block_size[SM] - 1) >>
				    block_shift[SM]) * block_size[SM]);
			}
		}
		/* size in 4k blocks */
		blocks = ((byte_count + SAM_BLK - 1) / SAM_BLK);
		if (debug_partial) {
			fprintf(log,
			    "and has partial extents.  "
			    "byte_count %ld blocks %ld ino.blocks %d\n",
			    byte_count, blocks, ino->di.blocks);
		}
		if (ino->di.blocks <= blocks) {
			return (FALSE);
		}
		return (TRUE);
	} else {
		/*
		 * in the case where the file isn't release -p, or it
		 * doesn't have partial extents, then we just base the decision
		 * on if it's a zero-length file or not
		 */
		if (debug_partial) {
			fprintf(log,
			    "and has no partial extents.  ino.blocks %d\n",
			    ino->di.blocks);
		}
		if (ino->di.blocks) {
			return (TRUE);
		}
		return (FALSE);
	}

/*NOTREACHED*/
}

/*
 *  where_is_lwm - calculate how many blocks would be in the filesystem
 *  if it were at low-water-mark.  This is our target.  NOTE:  we are
 *  NOT calculating how many we need to free - we're calculating how
 *  many free blocks there'd be in the filesystem at LWM.
 */
static long long
where_is_lwm(char *path, int low)
{
	long long blocks;
	struct STATVFS stat_buf;

	if (STATVFS(path, &stat_buf) < 0) {
		SysError(HERE, "STATVFS(%s) call failed", path);
		exit(EXIT_FAILURE);
	}

	/* NOTE:  f_blocks is "total blocks on(sic) filesystem" */
	blocks =
	    (((long long)stat_buf.f_blocks) * (long long)(100 - low))/100LL;
	return (blocks);
}

/*
 * where_is_fs - how many blocks are currently free in the filesystem?
 */
static long long
where_is_fs(char *path)
{
	struct STATVFS stat_buf;

	if (STATVFS(path, &stat_buf) < 0) {
		SysError(HERE, "STATVFS(%s) call failed", path);
		exit(EXIT_FAILURE);
	}

	return ((long long)stat_buf.f_bavail);
}

/*
 * find_fs - find the filesystem's mount information.   We need this so
 * we can tell the filesystem the releaser's running for this eq, as
 * well as marking the "r" flag set in the samu "s" display.
 */
static void
find_fs(char *name)
{
	static struct sam_fs_info fi;
	sam_fssbinfo_arg_t sbinfo_arg;
	sam_sbinfo_t sb_info;
	int i;

	if (GetFsInfo(name, &fi) == -1) {
		/* Filesystem \"%s\" not found. */
		SendCustMsg(HERE, 620, name);
		exit(EXIT_FAILURE);
	}
	if (!(fi.fi_status & FS_MOUNTED)) {
		/* Filesystem \"%s\" not mounted. */
		SendCustMsg(HERE, 8001, name);
		exit(EXIT_FAILURE);
	}
	sbinfo_arg.sbinfo.ptr = &sb_info;
	sbinfo_arg.fseq = fi.fi_eq;
	if (sam_syscall(SC_fssbinfo, &sbinfo_arg,
	    sizeof (sam_fssbinfo_arg_t)) < 0) {
		LibFatal(SC_fssbinfo, name);
		/*NOTREACHED*/
	}
	for (i = 0; i < 2; i++) {
		int value;

		kblock_size[i] = sb_info.dau_blks[i];
		block_size[i] = sb_info.dau_blks[i] * SAM_DEV_BSIZE;
		block_shift[i] = SAM_DEV_BSHIFT;
		value = SAM_DEV_BSIZE;
		for (;;) {
			if (value == (kblock_size[i] * SAM_DEV_BSIZE))  break;
			if (value > (kblock_size[i] * SAM_DEV_BSIZE)) {
				block_shift[i] = 0;
				break;
			}
			block_shift[i]++;
			value <<= 1;
		}
	}
	low_water = fi.fi_low;
	fs_name = fi.fi_name;
	mnt_point = fi.fi_mnt_point;
	(void) sprintf(inode_pathname, "%s/.inodes", mnt_point);

	if ((inode_fd = OpenInodesFile(mnt_point)) < 0) {
		LibError(NULL, EXIT_FAILURE, 613, inode_pathname);
		/*NOTREACHED*/
	}
	mp = &fi;
}

/*
 * release_a_file - release the given file.  Returns the number of
 * blocks free in the filesystem after the file was released.
 * On error, print an info message and return -1.
 */
static long long
release_a_file(struct sam_fs_info *mp, sam_id_t id)
{
	sam_fsdropds_arg_t	dropds_arg;
	long long freeblocks;
	int ret;

	dropds_arg.fseq = mp->fi_eq;
	dropds_arg.id = id;
	dropds_arg.freeblocks = 0LL;
	dropds_arg.shrink = 0;

	freeblocks = 0LL;
	if (release) {
		ret =
		    sam_syscall(SC_fsdropds, &dropds_arg, sizeof (dropds_arg));
		if (ret == -1 && errno != EINTR) {
			SysError(HERE, "Can't release fseq %d inode %d",
			    dropds_arg.fseq, id.ino);
			freeblocks = (long long)ret;
		} else {
			freeblocks = dropds_arg.freeblocks;
		}
	}
	return (freeblocks);
}

/*
 * sigalrm_handler - handle a SIGALRM.  We want to keep the global
 * variable "now" reasonably close to the current time.  Each time
 * an alarm comes in, we grab the current time to "now", and reset
 * the alarm signal so it comes again in ALRM_INTERVAL seconds.
 */
static void
sigalrm_handler(
/*LINTED argument unused in function */
    int ignored)
{
	now = time(NULL);
	if (signal(SIGALRM, &sigalrm_handler) == (void (*)(int))SIG_ERR) {
		SysError(HERE, "Cannot set handler for SIGALRM");
		exit(EXIT_FAILURE);
	}
	alarm(ALARM_INTERVAL);
}

/*
 * sigint_handler - handle a SIGINT.  SIGINT causes an orderly shutdown.
 * Any clean-up tasks have been queued by calling atexit(), so all we
 * have to do is exit().
 */
static void
sigint_handler(
/*LINTED argument unused in function */
    int ignored)
{
	/* Releaser shutdown by SIGINT */
	SendCustMsg(HERE, 8002);
	exit(EXIT_FAILURE);
}

/*
 * process_cmd_line - process command line.  Plugs various global variables
 * with parsed command-line options
 */
static void
process_cmd_line(int argc, char **argv)
{
	char *endptr;


	/* Handle low-water mark */
	low_water = (int)strtol(argv[2], &endptr, 0);
	if (*endptr != '\0') {
		fprintf(stderr,
		    "%s: error converting low-water mark %s to an integer.\n",
		    program_name, argv[2]);
		exit(EXIT_USAGE);
	}

	/* handle weight-size */
	weight_size = (float)strtod(argv[3], &endptr);
	if (*endptr != '\0') {
		fprintf(stderr,
		    "%s: error converting weight-size %s to a float.\n",
		    program_name, argv[3]);
		exit(EXIT_USAGE);
	}

	/* handle weight-age */
	if (argc >= 5) {
		weight_age = (float)strtod(argv[4], &endptr);
		if (*endptr != '\0') {
			fprintf(stderr,
			    "%s: error converting weight-age %s to a float.\n",
			    program_name, argv[4]);
			exit(EXIT_USAGE);
		}
	} else {
		weight_age = 0.0;
	}

}

/*
 * acceptable_candidate - returns TRUE if the inode passed is a
 * candidate for release.  (In which case, priority and data are also
 * filled in.)  If the inode shouldn't be released, return FALSE.
 */
static int
acceptable_candidate(struct sam_perm_inode *inode, float *priority,
    struct data *data)
{
	clock_t age;
	long long blocks;
	clock_t time;
	int copy;

	if (inode->di.mode == 0) {
		stats.zero_mode++;
		return (FALSE);
	}

	if (!S_ISREG(inode->di.mode) || S_ISSEGI(&inode->di)) {
		stats.not_regular++;
		return (FALSE);
	}

	if (S_ISEXT(inode->di.mode)) {
		stats.extension_inode++;
		return (FALSE);
	}

	if (inode->di.arch_status == 0) {
		stats.zero_arch_status++;
		return (FALSE);
	}

	if (inode->di.status.b.offline) {
		/*
		 * If the file is marked offline it should have no blocks
		 * allocated, unless it has partial release set or there
		 * has been an error in the file system.  Try to clean up
		 * the latter.
		 */
		if (inode->di.blocks == 0) {
			stats.already_offline++;
			return (FALSE);
		}
	}

	if (inode->di.status.b.damaged) {
		stats.damaged++;
		return (FALSE);
	}

	if (inode->di.status.b.nodrop) {
		stats.nodrop++;
		return (FALSE);
	}

	if (inode->di.status.b.archnodrop) {
		stats.archnodrop++;
		return (FALSE);
	}

	if (inode->di.residence_time >
	    (now - min_residence_age)) {
		stats.too_new_residence_time++;
		return (FALSE);
	}

	if (!big_enough(inode)) {
		stats.too_small++;
		return (FALSE);
	}

	if (inode->di.rm.ui.flags & RM_DATA_VERIFY) {
		/*
		 * All archived copies must be verified before
		 * a data verification file can be released.
		 */
		int	num_c = 0;
		int	num_c_verified = 0;
		int mask;

		for (copy = 0, mask = 1; copy < MAX_ARCHIVE;
		    copy++, mask += mask) {
			if (inode->di.arch_status & mask) {
				num_c++;
			}
			if ((inode->di.ar_flags[copy] & AR_verified) &&
			    !(inode->di.ar_flags[copy] & AR_damaged)) {
				num_c_verified++;
			}
		}
		if (num_c_verified != num_c || num_c == 1 ||
		    !inode->di.status.b.archdone) {

			stats.files_to_verify++;
			return (FALSE);
		}
	}

	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (inode->di.ar_flags[copy] & AR_rearch) {
			stats.rearch++;
			if (rearch_release == FALSE) {
				return (FALSE);
			}
		}
	}

	/*
	 * Set "age" to the difference between "now" and the most
	 * recent of the access, modify and residence times.  Set the age_key
	 * to an abbreviation of whichever time was chosen.
	 */
	if (inode->di.access_time.tv_sec > inode->di.residence_time) {
		if (inode->di.modify_time.tv_sec >
		    inode->di.access_time.tv_sec) {
			time = inode->di.modify_time.tv_sec;
			data->info.age_key = 'M';
		} else {
			time = inode->di.access_time.tv_sec;
			data->info.age_key = 'A';
		}
	} else {
		if (inode->di.modify_time.tv_sec > inode->di.residence_time) {
			time = inode->di.modify_time.tv_sec;
			data->info.age_key = 'M';
		} else {
			time = inode->di.residence_time;
			data->info.age_key = 'R';
		}
	}

	age = (now - time) / 60;   /* Convert to minutes */

	if (age < 0) {
		stats.negative_age++;
		return (FALSE);
	}

	blocks = inode->di.blocks;
	data->info.size = blocks;
	if (S_ISSEGS(&inode->di)) {
		data->info.seg_num = inode->di.rm.info.dk.seg.ord + 1;
	} else {
		data->info.seg_num = 0;
	}

	/*
	 * Calculate the priority of the file for releaser.  There are two
	 * cases:  one or three weights can be specified for the age.
	 */
	if (use_three_age_weights) {
		clock_t age_access, age_modify, age_residence;

		/*
		 * We enforce the rule "The future is now."  That is, if the
		 * access, modify or residence time is in the future, pretend
		 * that it's the current time instead.  This prevents nasty
		 * negative ages.  Make 'em all into minutes.
		 */
		age_access = max((now - inode->di.access_time.tv_sec), 0) / 60;
		age_modify = max((now - inode->di.modify_time.tv_sec), 0) / 60;
		age_residence = max((now - inode->di.residence_time), 0) / 60;

		*priority = weight_age_access	* age_access	+
		    weight_age_modify	* age_modify	+
		    weight_age_residence * age_residence  +
		    weight_size			* blocks;

		data->info.age_key = '3';
		data->info.time = 0;

	} else {

		*priority = weight_age  * age +
		    weight_size * blocks;
		data->info.age = age;
		data->info.time = time;

	}

	data->id = inode->di.id;

	/* log if requested */
#ifdef VERBOSE
	if (display_all_candidates) {
		char *pathname = id_to_path(mnt_point, inode->di.id);
		char seg_num[16] = "S0";

		if (S_ISSEGS(&inode->di)) {
			sprintf(seg_num,
			    "S%d", inode->di.rm.info.dk.seg.ord + 1);
		}
		if (data->info.age_key != '3') {
			char *formatted_time = format_time(data->info.time);
			fprintf(log, "%g (%c: %s) %ld min, %lld blks %s %s\n",
			    *priority, data->info.age_key, formatted_time, age,
			    blocks, seg_num, pathname);
		} else {
			fprintf(log, "%g %lld blks %s %s\n",
			    *priority, blocks, seg_num, pathname);
		}
	}
#else
	if (display_all_candidates) {
		char seg_num[16] = "S0";

		if (S_ISSEGS(&inode->di)) {
			sprintf(seg_num,
			    "S%d", inode->di.rm.info.dk.seg.ord + 1);
		}
		if (data->info.age_key != '3') {
			char *formatted_time = format_time(data->info.time);
			fprintf(log, "%g (%c: %s) %ld min %lld blks %s %d\n",
			    *priority, data->info.age_key, formatted_time, age,
			    blocks, seg_num, inode->di.id.ino);
		} else {
			fprintf(log, "%g %lld blks %s %d\n",
			    *priority, blocks, seg_num, inode->di.id.ino);
		}
	}
#endif

	return (TRUE);
}

static void
show_stats_then_zero()
{
	time_t cpu;
	clock_t elapsed;

#define	display_and_zero(x) fprintf(log, #x ": %d\n", stats.x);\
		stats.x = 0
	display_and_zero(archnodrop);
	display_and_zero(already_offline);
	display_and_zero(damaged);
	display_and_zero(extension_inode);
	display_and_zero(negative_age);
	display_and_zero(nodrop);
	display_and_zero(not_regular);
	display_and_zero(number_in_list);
	display_and_zero(rearch);
	display_and_zero(released_files);
	display_and_zero(too_new_residence_time);
	display_and_zero(too_small);
	display_and_zero(files_to_verify);
	display_and_zero(total_candidates);
	display_and_zero(total_inodes);
	display_and_zero(wrong_inode_number);
	display_and_zero(zero_arch_status);
	display_and_zero(zero_inode_number);
	display_and_zero(zero_mode);

	cpu = clock()/CLOCKS_PER_SEC - stats.seconds;
	elapsed = time((time_t *)NULL) - stats.begin;
	fprintf(log, "CPU time: %ld seconds.\n", cpu);
	fprintf(log, "Elapsed time: %ld seconds.\n\n", elapsed);
	stats.seconds = clock()/CLOCKS_PER_SEC;
	stats.begin = time((time_t *)NULL);
}

static void
releaser_header()
{
	time_t  clock = time(NULL);
	char    *ascii;
	char    ctime_buf[512];

	ascii = ctime_r(&clock, ctime_buf, sizeof (ctime_buf));
	*(ascii+strlen(ascii)-1) = '\0';

	fprintf(log, "\n\nReleaser begins at %s\n", ascii);
}

static void
releaser_trailer()
{
	time_t  clock = time(NULL);
	char    *ascii;
	char    ctime_buf[512];

	ascii = ctime_r(&clock, ctime_buf, sizeof (ctime_buf));

	fprintf(log, "Releaser ends at %s\n", ascii);
}

static void
open_log_file()
{
	if (*log_pathname == '\0') {
		strcpy(log_pathname, "/dev/null");
	}
	if ((log = fopen64(log_pathname, "a")) == NULL) {
		SysError(HERE, "Cannot open %s", log_pathname);
		log = stdout;
	}
}

/*  Return a nicely formatted age. */
static char *
format_time(clock_t clock)
{
	static char buf[512];

	(void) cftime(buf, (char *)0, &clock);
	return (buf);
}

/*
 * Sets the list_size to a default
 * based on the length of the .inodes file.
 */
static void
set_default_listsize(void)
{
	struct stat64 buf;

	if (fstat64(inode_fd, &buf) != 0) {
		fprintf(log, "Error getting .inodes size: %s\n",
		    strerror(errno));
		fprintf(log, "Using list_size %d\n", SML_DEF_SIZE);
		list_size = SML_DEF_SIZE;
		return;
	}

	if (buf.st_size < INODE_SIZE_THRSHLD) {
		list_size = SML_DEF_SIZE;
	} else {
		list_size = LRG_DEF_SIZE;
	}
}

#if defined(TEST)

struct sam_perm_inode	ino;

struct testVectors {
	int blocks;		/* allocated blocks */
	int filesize;	/* actual file size */
	int bof_online;	/* old partial flag */
	int partial;	/* partial blocks - new style */
	int on_large;	/* starts with large DAU */
	int extents;	/* true if partial extents */
	int blockszl;	/* large DAU size */
	int blockszs;	/* small DAU size */
	int answer;		/* desired result from big_enough() */
};

struct testVectors tv[] = {

	/* blocks size bof_online partial on-large extents bs-lg bs-sm answer */

	/* non-partial SAM-FS -  small DAU */
	{  1,  4096, 0,  0, 0, 0, 16384, 4096, 1},
	{  2,  5000, 0,  0, 0, 0, 16384, 4096, 1},
	{  0, 10000, 0,  0, 0, 0, 16384, 4096, 0},
	{  4, 16384, 0,  0, 0, 0, 16384, 4096, 1},
	{  0, 22000, 0,  0, 0, 0, 16384, 4096, 0},

	/* non-partial SAM-FS -  large DAU */
	{  0, 16384, 0,  0, 1, 0, 16384, 4096, 0},
	{  0, 47000, 0,  0, 1, 0, 16384, 4096, 0},
	{  4, 16384, 0,  0, 1, 0, 16384, 4096, 1},
	{ 12, 47000, 0,  0, 1, 0, 16384, 4096, 1},

	/* partial SAM-FS - new style partial, small DAU */
	{  2,  5000, 1,  8, 0, 1, 16384, 4096, 0},
	{  3, 10000, 1,  8, 0, 1, 16384, 4096, 1},
	{  4, 16384, 1, 32, 0, 1, 16384, 4096, 0},
	{  6, 22000, 1, 32, 0, 1, 16384, 4096, 0},
	{  9, 35000, 1, 32, 0, 1, 16384, 4096, 1},
	{ 12, 47000, 1, 32, 0, 1, 16384, 4096, 1},

	{  4, 16384, 1,  8, 0, 1, 65536, 4096, 1},
	{  6, 24000, 1, 16, 0, 1, 65536, 4096, 1},
	{  8, 32000, 1, 32, 0, 1, 65536, 4096, 0},
	{ 16, 65535, 1, 32, 0, 1, 65536, 4096, 1},
	{ 17, 66000, 1, 32, 0, 1, 65536, 4096, 1},

	/* partial SAM-FS - new style partial, large DAU */
	{  4, 16384, 1,  8, 1, 1, 16384, 4096, 0},
	{  6, 24000, 1,  8, 1, 1, 16384, 4096, 1},
	{  4, 16384, 1, 32, 1, 1, 16384, 4096, 0},
	{  6, 24000, 1, 32, 1, 1, 16384, 4096, 0},
	{  8, 32000, 1, 32, 1, 1, 16384, 4096, 0},
	{ 12, 44000, 1, 32, 1, 1, 16384, 4096, 1},

	{  4, 16384, 1,  8, 1, 1, 65536, 4096, 0},
	{  6, 24000, 1, 16, 1, 1, 65536, 4096, 0},
	{  8, 32000, 1, 32, 1, 1, 65536, 4096, 0},
	{ 16, 65535, 1, 32, 1, 1, 65536, 4096, 0},
	{ 17, 66000, 1, 32, 1, 1, 65536, 4096, 1},

	/* partial SAM-QFS - new style partial, small DAU */
	{  2,  5000, 1,  8, 0, 1, 16384, 16384, 0},
	{  3, 10000, 1,  8, 0, 1, 16384, 16384, 0},
	{  4, 16384, 1, 16, 0, 1, 16384, 16384, 0},
	{  6, 22000, 1, 16, 0, 1, 16384, 16384, 1},
	{  9, 35000, 1, 32, 0, 1, 16384, 16384, 1},
	{ 12, 47000, 1, 32, 0, 1, 16384, 16384, 1},

	/* partial SAM-QFS - new style partial, large DAU */
	{  4, 16384, 1,  8, 1, 1, 16384, 16384, 0},
	{  6, 24000, 1,  8, 1, 1, 16384, 16384, 1},
	{  4, 16384, 1, 16, 1, 1, 16384, 16384, 0},
	{  6, 24000, 1, 16, 1, 1, 16384, 16384, 1},
	{  8, 32000, 1, 32, 1, 1, 16384, 16384, 0},
	{ 12, 44000, 1, 32, 1, 1, 16384, 16384, 1},

	{  4, 16384, 1,  8, 1, 1, 65536, 65536, 0},
	{  6, 24000, 1, 16, 1, 1, 65536, 65536, 0},
	{  8, 32000, 1, 32, 1, 1, 65536, 65536, 0},
	{ 16, 65535, 1, 32, 1, 1, 65536, 65536, 0},
	{ 17, 66000, 1, 32, 1, 1, 65536, 65536, 1},

	{  0,	0,	0,	0,	0, 0,	0,		0,	0}

};
#define	NTESTS ((sizeof (tv)/sizeof (struct testVectors))-1)

/*
 *  Compute log base 2 of size.
 *  If size is less than SAM_DEV_BSIZE (1024 bytes) return 0.
 */
int
logtwo(int size)
{
	int value;
	int result;

	value = SAM_DEV_BSIZE;
	result = SAM_DEV_BSHIFT;

	while (TRUE) {
		if (value == size) break;

		if (value > size) {
			result = 0;
			break;
		}
		result++;
		value <<= 1;
	}
	return (result);
}

int
main(int argc, char **argv)
{
	int i, r;
	char *res;

	printf("Running %d tests on releaser.\n", NTESTS);
	debug_partial = TRUE;
	log = stdout;
	memset(&ino, sizeof (struct sam_perm_inode), 0);
	ino.di.mode = 0x8000;

	for (i = 0; i < NTESTS; i++) {

		/* set up inode values */
		ino.di.blocks = tv[i].blocks;
		ino.di.status.b.bof_online = tv[i].bof_online;
		ino.di.psize.partial = tv[i].partial;
		ino.di.rm.size = tv[i].filesize;
		ino.di.extent[0] = tv[i].extents;
		ino.di.status.b.pextents = tv[i].extents;
		ino.di.status.b.on_large = tv[i].on_large;
		kblock_size[SM] = tv[i].blockszs / SAM_DEV_BSIZE;
		kblock_size[LG] = tv[i].blockszl / SAM_DEV_BSIZE;
		block_size[SM] = tv[i].blockszs;
		block_size[LG] = tv[i].blockszl;
		block_shift[SM] = logtwo(tv[i].blockszs);
		block_shift[LG] = logtwo(tv[i].blockszl);


		/* run the test */

		fprintf(log, "test %d\n", i);

		r = big_enough(&ino);
		res = (r == tv[i].answer) ? "OK" : "***** ERROR *****";
		fprintf(log, "test %d result should be %d was %d: %s\n",
		    i, tv[i].answer, r, res);
		fprintf(log,
		    "test %d blocks  %d bof_online %d partial %d extents %d\n",
		    i, ino.di.blocks, ino.di.status.b.bof_online,
		    ino.di.psize.partial, ino.di.extent[0]);
		fprintf(log,
		    "     lg blksz  %d  shift %d sz %d "
		    "sm blksz %d shift %d sz %d\n",
		    kblock_size[LG], block_shift[LG], block_size[LG],
		    kblock_size[SM], block_shift[SM], block_size[SM]);

	}
}
#endif
