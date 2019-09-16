/*
 * shrink.c - remove/release equipment (ordinal) from file system
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
 *  The purpose of the sam-shrink is to find all the files who have
 *  allocation on the specified equipment. Shrink reads the .inodes
 *  file to find the files. The program may be started from a
 *  command line, but is most likely started by sam-fsd when the
 *  filesystem tells sam-fsd that a remove or release command has
 *  been entered for a eq (ordinal).
 *
 *  sam-shrink fs_name -release | -remove eq
 *
 *  After processing command-line options, the shrink reads the
 *  entire .inodes file from the filesystem.  As it's reading the .inodes,
 *  it builds a list of files allocated on the desinated eq.
 *
 *  Once all inodes have been read, sam-shrink begins to release files
 *  for the -release option or copy files for the -remove option.
 *
 */

#pragma ident "$Revision: 1.9 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <inttypes.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#include <sys/mnttab.h>
#include <time.h>
#include <signal.h>
#include <strings.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>

#define	DEC_INIT
#define	MAIN

#include "sam/types.h"
#include "sam/param.h"
#include "sam/custmsg.h"
#include "sam/nl_samfs.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/defaults.h"
#include "sam/exit.h"
#include "sam/sam_trace.h"
#include "sam/uioctl.h"
#include "sam/syscall.h"
#include "sam/fioctl.h"

#include "pub/stat.h"
#include "pub/devstat.h"
#include "pub/lib.h"

/* SAM-FS headers. */
#include "sam/fs/ino.h"
#include "sam/fs/dirent.h"
#include "sam/fs/ino_ext.h"
#include "sam/fs/sblk.h"

#include "shrink.h"

#define	_POSIX_PTHREAD_SEMANTICS	1

/*
 * Global data
 */
char *program_name = SAM_SHRINK;
char *fs_name;			/* File system name */
boolean_t Daemon;		/* TRUE if started by sam-fsd */
int io_errors = 2;		/* I/O error threshold */
pthread_mutex_t log_lock;	/* Printing log lock */
control_t control;		/* Work queue ptrs & thread params */
work_t work[MAX_QUEUE];		/* Work queue */

/*
 * Prototypes
 */
static void sam_shrink_header(int msgno, char *cmd, char *eq);
static void sam_usage(char *name);
static FILE *sam_open_log_file();
static int sam_get_mountpoint(const char *fs_name);
static int sam_shrink_fs(char *fs_name, pblock_t *pp);
void sam_clean_thr(int);
static void sam_init_pblock(char *fs_name, pblock_t *pp);
static void sam_setup_file(pblock_t *pp);
static int sam_read_file(pblock_t *pp);
static int sam_add_work(work_t *wp, int queue_size);
static void *sam_worker_thread(void *c);
static void sam_check_inodes(work_t *wp);
static boolean_t sam_ord_found(sam_perm_inode_t *ip);
static int sam_ord_on_indirects(struct sam_fs_info *mp, sam_perm_inode_t *ip,
	ushort_t ord);
static int sam_move_file(struct sam_fs_info *mp, sam_perm_inode_t *ip);
static int sam_process_file(struct sam_fs_info *mp, sam_perm_inode_t *ip);
static void sam_write_log(sam_perm_inode_t *ip, boolean_t released,
	char *cmd, int error);

static int getfullpathname(char *fsname, sam_id_t id, char **fullpath);
extern int read_cmd_file(char *cfgFname);

/* Defines */
#define	DIRBUF_SIZE 10240

/* Private getfullpathname prototypes */
static int sam_id2path(int fs_fd, int *dir_fd, sam_id_t id, char **fullpath,
	offset_t *offset, int *n_valid, char *dirbuf);
static int sam_getdent(int dir_fd, char *dirbuf, struct sam_dirent **dirent,
	offset_t *offset, int *n_valid, struct sam_dirent **current);
static int sam_opendir(int *dir_fd, char *dir_name);
static void sam_closedir(int *dir_fd);
static int sam_readdir(int dir_fd, char *dirbuf, offset_t *offset,
	int dirent_size);

/*
 * Info about stripe groups if needed.
 */
typedef struct sginfo {
	int first_ord;
	int num_group;
	int free_space;
	int state;
} sginfo_t;

/*
 * Max stripe group value
 * Max 128 stripe groups numbered 0 - 127
 */
#define	MAX_STRIPE_GROUP	127

/*
 * main - sam-shrink - Process remove eq or release eq.
 */
int
main(int argc, char *argv[])
{
	pblock_t	pblock;		/* Parameter block */
	struct sam_fs_info mnt_info;	/* File system mount table */
	struct sam_fs_part *part, *pt;	/* Array of partitions */
	int		command;
	int		size_part;
	equ_t		eq;
	int		ord;
	dtype_t	oldpt;
	int		error = 0;

	/*
	 * sam-shrink behaves slightly differently depending on if it
	 * was started by sam-fsd or from a command line. Initialize trace.
	 */
	Daemon = strcmp(GetParentName(), SAM_FSD) == 0;
	CustmsgInit(Daemon, NULL);
	TraceInit(SAM_SHRINK, TI_shrink);
#ifndef DEBUG
	if (!Daemon) {
		Trace(TR_ERR, "sam-shrink process not started by sam-fsd");
		exit(EXIT_FAILURE);
	}
#endif

	/*
	 * Parse arguments. sam-shrink is started with three arguments:
	 * filesystem_name [remove | release] eq
	 */
	if (argc == 4) {
		fs_name = argv[1];
		if (strcmp(argv[2], "remove") == 0) {
			command = DK_CMD_remove;
		} else if (strcmp(argv[2], "release") == 0) {
			command = DK_CMD_release;
		} else {
			sam_usage(argv[0]);
		}
		eq = atoi(argv[3]);
	} else {
		sam_usage(argv[0]);
	}
	Trace(TR_MISC, "sam-shrink process started, %s %s %s",
	    fs_name, argv[2], argv[3]);

	/*
	 * If Daemon, swtich into /var/opt/SUNWsamfs/shrink/fs_name.
	 * Read command file.
	 */
	control.log = NULL;
	control.streams = 0;
	if (Daemon) {
		char fileName[64];

		sprintf(fileName, "%s/%s/%s", SAM_VARIABLE_PATH, "shrink",
		    fs_name);
		MakeDir(fileName);
		if (chdir(fileName)) {
			/* Filesystem \"%s\" not found. */
			SendCustMsg(HERE, 3038, fileName);
			exit(EXIT_FAILURE);
		}
		if (read_cmd_file(SAM_CONFIG_PATH"/shrink.cmd") > 0) {
			/* Error in shrink command file %s */
			SendCustMsg(HERE, 25000, SAM_CONFIG_PATH"/shrink.cmd");
			exit(EXIT_FAILURE);
		}
		control.log = sam_open_log_file();
	} else {
		control.log = stdout;
	}

	/*
	 * Check if name is a mount point or family set name, and mounted.
	 */
	if ((GetFsInfo(fs_name, &mnt_info)) == -1) {
		/* Filesystem \"%s\" not found. */
		SendCustMsg(HERE, 620, fs_name);
		exit(EXIT_FAILURE);
	}
	if ((mnt_info.fi_status & FS_MOUNTED) == 0)  {
		/* Filesystem \"%s\" not mounted. */
		SendCustMsg(HERE, 13754, fs_name);
		exit(EXIT_FAILURE);
	}

	/*
	 * Check if eq is a member of the family set and its state is
	 * NOALLOC.
	 */
	size_part = mnt_info.fs_count * sizeof (struct sam_fs_part);
	part = (struct sam_fs_part *)malloc(size_part);
	if (part == NULL) {
		/* malloc: Eq table. */
		SendCustMsg(HERE, 1606, "Eq table");
		exit(EXIT_NOMEM);
	}
	if ((GetFsParts(fs_name, mnt_info.fs_count, part)) < 0) {
		/* Not a SAM-FS file system or not mounted (%s). */
		SendCustMsg(HERE, 7006, fs_name);
		exit(EXIT_FAILURE);
	}
	for (ord = 0, pt = part; ord < mnt_info.fs_count; ord++, pt++) {
		if (eq == pt->pt_eq) {
			if (pt->pt_state != DEV_NOALLOC) {
				/* Invalid device state '%s' */
				SendCustMsg(HERE, 17251,
				    dev_state[pt->pt_state]);
				exit(EXIT_FAILURE);
			}
			oldpt = pt->pt_type;
			break;
		}
	}
	if (ord >= mnt_info.fs_count) {
		/* %s is not a valid equipment ordinal. */
		SendCustMsg(HERE, 2350, argv[3]);
		exit(EXIT_FAILURE);
	}
	control.eq = eq;		/* Equipment */
	control.ord = (ushort_t)ord;	/* Equipment ordinal */
	control.ord2 = -1;
	control.command = command;	/* Release or Remove command */
	control.mp = &mnt_info;		/* Pointer to mount info */

	if ((command == DK_CMD_remove) && is_stripe_group(oldpt)) {
		int xord;
		sginfo_t *sginfop;
		int stripe_group;
		int onum_group;
		int tmp_ord2 = -1;
		int	curr_sg = -1;
		int last_sg = -1;
		fsize_t curr_space = 0;
		/*
		 * If removing a stripe group find a matching one.
		 */
		sginfop = (sginfo_t *)calloc(
		    MAX_STRIPE_GROUP+1 * sizeof (sginfo_t), sizeof (sginfo_t));

		if (sginfop == NULL) {
			SendCustMsg(HERE, 1606, "Stripe Group Info");
			exit(EXIT_NOMEM);
		}

		onum_group = 0;
		for (xord = 0, pt = part;
		    xord < mnt_info.fs_count; xord++, pt++) {
			/*
			 * Collect stripe group info so a matching
			 * stripe group can be found.
			 */
			if (is_stripe_group(pt->pt_type)) {
				if (pt->pt_type == oldpt) {
					onum_group++;
				}
				stripe_group =
				    pt->pt_type & ~DT_STRIPE_GROUP_MASK;
				if (stripe_group != curr_sg) {
					/*
					 * Starting a new stripe group.
					 */
					sginfop[stripe_group].first_ord = xord;
					sginfop[stripe_group].free_space =
					    pt->pt_space;
					sginfop[stripe_group].state =
					    pt->pt_state;
				}
				sginfop[stripe_group].num_group++;
				curr_sg = stripe_group;
				if (curr_sg > last_sg) {
					last_sg = curr_sg;
				}
			}
		}

		while (last_sg >= 0) {
			/*
			 * Find the matching stripe group
			 * with the most free space.
			 */
			if (sginfop[last_sg].num_group == onum_group &&
			    sginfop[last_sg].state == DEV_ON) {

				if (tmp_ord2 == -1 ||
				    sginfop[last_sg].free_space > curr_space) {
					tmp_ord2 = sginfop[last_sg].first_ord;
					curr_space =
					    sginfop[last_sg].free_space;
				}
			}
			last_sg--;
		}
		free(sginfop);
		if (tmp_ord2 < 0) {
			SendCustMsg(HERE, 7011, control.eq);
			exit(EXIT_FAILURE);
		}
		control.ord2 = tmp_ord2;
	}

	/*
	 * Set up parameter block (pblock).
	 * Queue size must be at the maximum size of the read threads.
	 */
	bzero(&pblock, sizeof (pblock));
	if (control.block_size == 0) {
		control.block_size = 1;	/* Default value */
	}
	pblock.block_size = (int64_t)control.block_size * 1048576;
	if (control.streams == 0) {
		control.streams = 8;	/* Default value */
	}
	pblock.read_threads = control.streams;
	pblock.queue_size = control.streams;

	/*
	 * Get the mount point.
	 */
	if ((sam_get_mountpoint(fs_name)) < 0) {
		/* Unable to find the mount point %s */
		SendCustMsg(HERE, 2596, fs_name);
		exit(EXIT_FAILURE);
	}
	Trace(TR_MISC, "sam-shrink command options: fs=%s mp=%s cmd=%d eq=%d "
	    "ord=%d ord2=%d do_not_execute=%d display_all_files=%d "
	    "stage_files=%d stage_partial=%d streams=%d block_size=%dMB log=%s",
	    fs_name, control.mountpoint, control.command, control.eq,
	    control.ord, control.ord2, control.do_not_execute,
	    control.display_all_files, control.stage_files,
	    control.stage_partial, control.streams, control.block_size,
	    control.log_pathname);

	/*
	 * Shrink the specified ordinal.
	 */
	sam_shrink_header(1, argv[2], argv[3]);
	if (sam_shrink_fs(fs_name, &pblock)) {
		Trace(TR_ERR, "sam-shrink process failed: %s %s %s",
		    fs_name, argv[2], argv[3]);
		sam_shrink_header(2, argv[2], argv[3]);
		return (0);
	}
	if ((SetFsPartCmd(fs_name, argv[3], DK_CMD_off)) < 0) {
		error = errno;
		if (error == EBUSY) {
			error = 0;
			sleep(1);
			if ((SetFsPartCmd(fs_name, argv[3],
			    DK_CMD_off)) < 0) {
				error = errno;
			}
		}
	}
	if (error == 0) {
		if (control.total_errors) {
			Trace(TR_MISC, "sam-shrink unsuccessful: busy files=%d "
			    "unarchived files=%d total errors=%d",
			    control.busy_files, control.unarchived_files,
			    control.total_errors);
			sam_shrink_header(4, argv[2], argv[3]);
		} else {
			Trace(TR_MISC, "sam-shrink completed successfully:"
			    " %s %s %s", fs_name, argv[2], argv[3]);
			sam_shrink_header(3, argv[2], argv[3]);
		}
	} else {
		Trace(TR_ERR, "sam-shrink could not change %d to OFF "
		    "for %s: %s", control.eq, fs_name, strerror(error));
		sam_shrink_header(2, argv[2], argv[3]);
	}
	return (0);
}


static void
sam_shrink_header(
	int msgno,
	char *cmd,
	char *eq)
{
	time_t	clock = time(NULL);
	char	*ascii;
	char	ctime_buf[512];
	char	msgbuf[512];

	switch (msgno) {
	case 1:
		sprintf(msgbuf,
		    "Shrink process started: %s %s eq %s", fs_name,
		    cmd, eq);
		break;
	case 2:
		sprintf(msgbuf,
		    "Shrink process failed for %s eq %s",
		    fs_name, eq);
		break;
	case 3:
		sprintf(msgbuf,
		    "Shrink process successful for %s eq %s",
		    fs_name, eq);
		break;
	case 4:
		sprintf(msgbuf,
		    "Shrink process unsuccessful for %s eq %s: "
		    "busy files=%d, unarchived files=%d, total_errors=%d",
		    fs_name, eq, control.busy_files, control.unarchived_files,
		    control.total_errors);
		break;
	default:
		break;
	}
	ascii = ctime_r(&clock, ctime_buf, sizeof (ctime_buf));
	*(ascii+strlen(ascii)-1) = '\0';

	pthread_mutex_lock(&log_lock);
	fprintf(control.log, "%s %s\n", ascii, msgbuf);
	pthread_mutex_unlock(&log_lock);
	fflush(control.log);
	control.log_time = time(NULL);
}


/*
 * sam_usage -- arguments illegal. Terminate.
 */
static void
sam_usage(char *name)
{
	char msg[1024];

	sprintf(msg, "Usage: %s fsname remove|release eq\n", name);
	if (Daemon) {
		sam_syslog(LOG_ERR, msg);
	} else {
		fprintf(stderr, msg);
	}
	exit(EXIT_USAGE);
}


/*
 * sam_open_log_file - Open log file
 */
static FILE *
sam_open_log_file()
{
	FILE *log;

	if (*control.log_pathname == '\0') {
		strcpy(control.log_pathname, "/dev/null");
	}
	if ((log = fopen64(control.log_pathname, "a")) == NULL) {
		SysError(HERE, "Cannot open %s", control.log_pathname);
		log = stdout;
	}
	return (log);
}


/*
 * sam_get_mountpoint - Get the mountpoint filename given the
 * mountpoint or file system name
 */
static int
sam_get_mountpoint(
	const char *fs_name)
{
	FILE *mnt_fp;
	struct mnttab mnt_entry;
	char *mountpoint;

	/*
	 * If given a SAM file system name, convert to a mount point.
	 */
	if (fs_name[0] != '/') {
		mnt_fp = fopen(MNTTAB, "r");
		if (mnt_fp == NULL) {
			return (-1);
		}
		mountpoint = NULL;
		while (getmntent(mnt_fp, &mnt_entry) == 0) {
			if (strcmp(mnt_entry.mnt_special, fs_name) == 0) {
				mountpoint = mnt_entry.mnt_mountp;
				break;
			}
		}
		(void) fclose(mnt_fp);
		if (mountpoint == NULL) {
			return (-1);
		}
	} else {
		mountpoint = (char *)fs_name;
	}
	sprintf(control.mountpoint, "%s", mountpoint);
	return (0);
}


/*
 * sam_shrink_fs  - reads the .inodes file based on the parameters
 * and copies those files (remove) or marks those files offline (release)
 */
static int
sam_shrink_fs(
	char *fs_name,
	pblock_t *pp)
{
	int i;
	int err;

	/*
	 * Initialize the control structure.
	 */
	pthread_mutex_init(&log_lock, NULL);
	pthread_mutex_init(&control.queue_lock, NULL);
	pthread_mutex_init(&control.queue_full_lock, NULL);
	pthread_cond_init(&control.queue_not_empty, NULL);
	pthread_cond_init(&control.queue_not_full, NULL);
	pthread_cond_init(&control.queue_empty, NULL);
	control.queue_size = 0;
	control.shutdown = 0;

	/*
	 * Set up signal handling. Exit on SIGINT, SIGSEGV, SIGBUS,
	 * and SIGTERM. sam-fsd send a SIGHUP on a configure -- ignore it.
	 */
	signal(SIGINT, sam_clean_thr);
	signal(SIGSEGV, sam_clean_thr);
	signal(SIGBUS, sam_clean_thr);
	signal(SIGTERM, sam_clean_thr);
	signal(SIGHUP, SIG_IGN);

	/*
	 * Open the .inodes file for all threads.
	 */
	sam_setup_file(pp);

	/*
	 * Create the threads and allocate the buffers.
	 */
	sam_init_pblock(fs_name, pp);
	Trace(TR_MISC, "sam-shrink threads=%d, blk_sz=0x%llx",
	    pp->read_threads, pp->block_size);

	/*
	 * Must set concurrency for all threads to be active
	 */
	if (pthread_setconcurrency(pp->read_threads) != 0) {
		/* %s: Error pthread_setconcurrency */
		SendCustMsg(HERE, 25001, fs_name);
		exit(EXIT_FAILURE);
	}

	/*
	 * Read from .inodes file
	 */
	err = sam_read_file(pp);

	/*
	 * Signal threads to terminate.
	 */
	pthread_mutex_lock(&control.queue_lock);
	control.shutdown = 1;
	pthread_cond_broadcast(&control.queue_not_empty);
	pthread_mutex_unlock(&control.queue_lock);

	/*
	 * Wait for those threads to terminate
	 */
	for (i = 0; pp->active_threads > 0; pp->active_threads--, i++) {
		if ((pthread_join(control.thr[i].tid, NULL)) != 0) {
			/* %s: Error pthread_join */
			SendCustMsg(HERE, 25003, fs_name);
			exit(EXIT_FAILURE);
		}
	}
	return (err);
}


/*
 * sam_init_pblock - Create the threads and allocate the buffers.
 * Try to lock buffers if user is root.
 */
static void
sam_init_pblock(
	char *fs_name,
	pblock_t *pp)
{
	int i;
	int t;
	int64_t size;

	/*
	 * Create the read threads.
	 */
	for (t = 0; t < pp->read_threads; t++) {
		control.thr[t].num = t;
		if (pthread_create(&control.thr[t].tid,
		    NULL, sam_worker_thread, &control.thr[t])) {
			/* %s: Error pthread_create */
			SendCustMsg(HERE, 25004, fs_name);
			exit(EXIT_FAILURE);
		}
	}
	pp->active_threads = pp->read_threads;

	/*
	 * Allocate the page aligned buffers
	 */
	size = (int64_t)((pp->block_size + 7) / 8) * 8;
	for (i = 0; i < pp->queue_size; i++) {
		if ((work[i].buffer = (char *)valloc(size)) == NULL) {
			/* %s: Error valloc buffers */
			SendCustMsg(HERE, 25005, fs_name);
			exit(EXIT_NOMEM);
		}
		(void) memset(work[i].buffer, -1, pp->block_size);

		/*
		 * Lock buffers
		 */
		if (mlock(work[i].buffer, size) != 0) {
			/* %s: Error mlock buffers */
			SendCustMsg(HERE, 25006, fs_name);
			exit(EXIT_FAILURE);
		}
		pthread_mutex_init(&work[i].work_lock, NULL);
		work[i].num = i;	/* Work number */
		work[i].busy = 0;	/* Request is busy doing I/O */
		work[i].first = 1;
	}
}


/*
 * sam_setup_file - Set up to read .inodes file. Open the file for each
 * worker thread. Each worker thread opens the file because the file_offset
 * must be unique (out of order seeks can occur with threads).
 */
static void
sam_setup_file(pblock_t *pp)
{
	struct sam_ioctl_idopen arg;
	char		inode_name[MAXPATHLEN];
	struct stat64	statbuf;
	int		mountpoint_fd;
	int		nthreads, t;
	int		n;

	/*
	 * Get size of .inodes file
	 */
	sprintf(inode_name, "%s%s", control.mountpoint, "/.inodes");
	if (stat64(inode_name, &statbuf) != 0) {
		/* stat failed for file %s */
		SendCustMsg(HERE, 30217, inode_name);
		exit(EXIT_FAILURE);
	}
	pp->file_size = statbuf.st_size;

	/*
	 * Compute number of blocks based on the .inodes file size & block size.
	 */
	pp->block_count = (pp->file_size + pp->block_size - 1) / pp->block_size;
	n = (int)pp->block_count;
	if (n < pp->read_threads) {
		pp->read_threads = n;
		pp->queue_size = n;
	}

	/*
	 * Open the mountpoint and then open the .inodes file for each thread
	 * by using an ioctl on the mount point.
	 */
	if ((mountpoint_fd = open(control.mountpoint, O_RDONLY)) < 0) {
		/* %s: cannot open () */
		SendCustMsg(HERE, 213, control.mountpoint);
		exit(EXIT_FAILURE);
	}

	nthreads = pp->read_threads;
	for (t = 0; t < nthreads; t++) {
		bzero(&arg, sizeof (arg));
		arg.id.ino = SAM_INO_INO;
		control.thr[t].fd = ioctl(mountpoint_fd, F_IDOPEN, &arg);
		if (control.thr[t].fd < 0) {
			/* Cannot F_IDOPEN %s/.inodes: %s\n */
			SendCustMsg(HERE, 25002, control.mountpoint,
			    strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	(void) close(mountpoint_fd);
}


/*
 * sam_read_file -- loops, putting read requests in the I/O
 * request queue until the issue count is reached. Then it
 * waits until the request is done (not busy) before using the
 * work entry. Next it sets up the request parameters and adds the
 * entry to the work queue.
 */
static int
sam_read_file(pblock_t *pp)
{
	int	i, t;
	int	ique = 0;
	int64_t	issue_count = 0;
	int64_t	completion_count = 0;
	int64_t	byte_offset = 0;

	for (;;) {
		work_t *wp;

		wp = &work[ique];
		pthread_cond_init(&wp->cv_work_done, NULL);
		pthread_mutex_lock(&wp->work_lock);
		while (wp->busy && control.shutdown == 0) {
			pthread_cond_wait(&wp->cv_work_done, &wp->work_lock);
		}
		wp->busy = 1;
		wp->next = NULL;	/* Forward link */
		pthread_mutex_unlock(&wp->work_lock);

		if (wp->first) {
			wp->first = 0;
		} else {
			completion_count++;
		}
		wp->byte_offset = byte_offset;
		wp->size = pp->block_size;
		if (wp->byte_offset + wp->size > pp->file_size) {
			wp->size = pp->file_size - wp->byte_offset;
		}
		if (++ique >= pp->queue_size) {
			ique = 0;
		}
		if (completion_count >= pp->block_count || control.shutdown) {
			if (control.shutdown) {
				Trace(TR_MISC, "sam_read_file: work Q "
				    "shutdown, off %lld, filesize %lld",
				    byte_offset, pp->file_size);
			} else {
				Trace(TR_MISC, "sam_read_file:"
				    " completion cnt %lld >= blk count %lld",
				    completion_count, pp->block_count);
			}
			break;		/* Exit read loop */
		}

		/*
		 * Loop to compare last buffer
		 */
		if (issue_count >= pp->block_count) {
			Trace(TR_MISC,
			    "sam_read_file: issue_count %lld block_count %lld",
			    issue_count, pp->block_count);
			continue;
		}
		if (sam_add_work(wp, pp->queue_size)) {
			return (1);
		}
		issue_count++;
		byte_offset += (int64_t)pp->block_size;
	}

	/*
	 * Wait for queue to empty and I/O to complete.
	 */
	pthread_mutex_lock(&control.queue_lock);
	while (((control.queue_size != 0) || (control.io_outstanding != 0)) &&
	    (control.shutdown == 0)) {
		pthread_cond_wait(&control.queue_empty,
		    &control.queue_lock);
	}
	if (control.shutdown) {
		pthread_mutex_unlock(&control.queue_lock);
		return (1);
	}
	pthread_mutex_unlock(&control.queue_lock);

	for (t = 0; t < pp->read_threads; t++) {
		if (close(control.thr[t].fd) < 0) {
			/* error during close %#x */
			SendCustMsg(HERE, 1028, errno);
		}
	}

	for (i = 0; i < pp->queue_size; i++) {	/* Free non-null buffers */
		if (work[i].buffer != NULL) {
			(void) free(work[i].buffer);
			work[i].buffer = NULL;
		}
	}
	return (0);
}


/*
 * sam_add_work - If the I/O request work queue is full, the main shrink thread
 * waits on the queue_not_full condition. Then it adds a I/O request to the
 * tail of the I/O request work queue. Then it wakes up a sleeping
 * worker thread by signaling the queue_not_empty condition.
 */
static int
sam_add_work(work_t *wp, int queue_size)
{
	pthread_mutex_lock(&control.queue_lock);
	while ((control.queue_size == queue_size) && (control.shutdown == 0)) {
		pthread_mutex_unlock(&control.queue_lock);
		pthread_mutex_lock(&control.queue_full_lock);
		pthread_cond_wait(&control.queue_not_full,
		    &control.queue_full_lock);
		pthread_mutex_unlock(&control.queue_full_lock);
		pthread_mutex_lock(&control.queue_lock);
	}
	if (control.shutdown) {
		pthread_mutex_unlock(&control.queue_lock);
		return (1);
	}
	if (control.queue_size == 0) {
		control.queue_tail = control.queue_head = wp;
	} else {
		(control.queue_tail)->next = wp;
		control.queue_tail = wp;
	}
	control.queue_size++;
	pthread_cond_signal(&control.queue_not_empty);
	pthread_mutex_unlock(&control.queue_lock);
	return (0);
}


/*
 * sam_worker_thread - If the queue is empty, the worker thread sleeps on the
 * queue_not_empty condition condition. The worker thread removes the I/O
 * request from the head of the queue. After it removes the I/O request, it
 * signals the queue_not_full condition.
 */
static void *
sam_worker_thread(void *c)
{
	thr_t		*tp = (thr_t *)c;
	work_t		*wp;
	ssize_t		status = 0;
	int		errs;

	for (;;) {
		errs = 0;
		wp = NULL;
		pthread_mutex_lock(&control.queue_lock);

		/*
		 * Wait until there is something in the queue.
		 */
		while ((control.queue_size == 0) && (control.shutdown == 0)) {
			pthread_cond_wait(&control.queue_not_empty,
			    &control.queue_lock);
		}
		if (control.shutdown) {
			pthread_mutex_unlock(&control.queue_lock);
			pthread_exit(NULL);
		}

		/*
		 * Remove work from queue and then signal boss thread
		 * queue_not_full.
		 */
		wp = control.queue_head;
		control.queue_size--;
		if (control.queue_size == 0) {
			control.queue_head = control.queue_tail = NULL;
		} else {
			control.queue_head = wp->next;
		}
		control.io_outstanding++;
		pthread_mutex_unlock(&control.queue_lock);
		pthread_mutex_lock(&control.queue_full_lock);
		pthread_cond_signal(&control.queue_not_full);
		pthread_mutex_unlock(&control.queue_full_lock);

		while ((status = pread64(tp->fd, wp->buffer, wp->size,
		    wp->byte_offset)) != wp->size) {
			if (errs < io_errors) {
				Trace(TR_ERR, "Read error at %lld (%#llx): "
				    "%s: returned %d: retry %d",
				    wp->byte_offset, wp->byte_offset,
				    strerror(errno), status, errs);
				errs++;
			} else {
				/* Read failed: offset %lld (%#llx): %s */
				SendCustMsg(HERE, 25008, wp->byte_offset,
				    wp->byte_offset, strerror(errno));
				Trace(TR_ERR, "Read error at %lld (%#llx): "
				    "%s: returned %d: retry %d",
				    wp->byte_offset, wp->byte_offset,
				    strerror(errno), status, errs);
				wp->ret_errno = errno;
				goto fini;
			}
		}

		/*
		 * Check to see if there are inodes allocated on eq.
		 */
		sam_check_inodes(wp);

		pthread_mutex_lock(&wp->work_lock);
		wp->busy = 0;		/* I/O completed for this work entry */
		pthread_cond_signal(&wp->cv_work_done);
		pthread_mutex_unlock(&wp->work_lock);
		wp = NULL;

		/*
		 * Signal boss thread if queue_empty and no I/O is outstanding.
		 */
		pthread_mutex_lock(&control.queue_lock);
		control.io_outstanding--;
		if ((control.queue_size == 0) &&
		    (control.io_outstanding == 0)) {
			pthread_cond_signal(&control.queue_empty);
		}
		pthread_mutex_unlock(&control.queue_lock);
	}

fini:
	/* Shut down this worker thread. */
	pthread_mutex_lock(&control.queue_lock);
	control.shutdown = 1;
	control.io_outstanding--;
	pthread_cond_signal(&control.queue_empty);
	pthread_mutex_unlock(&control.queue_lock);
	pthread_mutex_lock(&control.queue_full_lock);
	pthread_cond_signal(&control.queue_not_full);
	pthread_mutex_unlock(&control.queue_full_lock);
	if (wp != NULL) {			/* move on */
		pthread_mutex_lock(&wp->work_lock);
		wp->busy = 0;		/* I/O completed for this work entry */
		pthread_cond_signal(&wp->cv_work_done);
		pthread_mutex_unlock(&wp->work_lock);
	}
	pthread_exit(NULL);
	/* return (0); */
}


/*
 * sam_check_inodes - Check inodes to build list to remove/release.
 */
static void
sam_check_inodes(work_t *wp)
{
	char *buf = wp->buffer;
	int64_t size = wp->size;
	int64_t ino_size = sizeof (sam_perm_inode_t);
	int64_t i;
	sam_perm_inode_t *ip;
	struct sam_fs_info *mp = control.mp;

	if (buf == NULL) {
		Trace(TR_ERR, "sam_check_inodes: Buffer NULL");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < size; i += ino_size, buf += ino_size) {
		ip = (sam_perm_inode_t *)(void *)buf;
		if (!S_ISREG(ip->di.mode)) {
			continue;
		}
		if (ip->di.blocks == 0) {
			continue;
		}
		if (sam_ord_found(ip)) {
			/*
			 * Files selected for releasing/removal may have been
			 * deleted before we start processing the file. Ignore
			 * ENOENT error for this case.
			 */
			if (sam_process_file(mp, ip) && (errno != ENOENT)) {
				pthread_mutex_lock(&log_lock);
				control.total_errors++;
				pthread_mutex_unlock(&log_lock);
			}
		}
	}
}


/*
 * sam_ord_found - Check inodes to build list to remove/release.
 */
static boolean_t		/* TRUE if file has ord, FALSE if not on ord */
sam_ord_found(sam_perm_inode_t *ip)
{
	int de;
	struct sam_fs_info *mp = control.mp;

	for (de = 0; de < (NDEXT + NIEXT); de++) {
		if (ip->di.extent[de] != 0 &&
		    ip->di.extent_ord[de] == control.ord) {
			return (TRUE);
		}
	}
	for (de = NDEXT; de < (NDEXT + NIEXT); de++) {
		if (ip->di.extent[de] == 0) {
			continue;
		}
		errno = 0;
		if ((sam_ord_on_indirects(mp, ip, control.ord))) {
			Trace(TR_MISC, "sam_ord_found: found indirects:"
			    "%d.%d ord=%d",
			    ip->di.id.ino, ip->di.id.gen, control.ord);
			return (TRUE);
		} else {
			if ((errno > 0) && (errno != ENOENT)) {
				Trace(TR_ERR, "sam_ord_found:"
				    "%d.%d ord=%d, ERROR: %s",
				    ip->di.id.ino, ip->di.id.gen,
				    control.ord, strerror(errno));
			}
			return (FALSE);
		}
	}
	return (FALSE);
}


/*
 * sam_ord_on_indirects - Check indirect blocks to see if file is on
 * this ordinal.
 */
static int			/* 0 if not on ord, !=0 if on ord */
sam_ord_on_indirects(
	struct sam_fs_info *mp,
	sam_perm_inode_t *ip,
	ushort_t ord)
{
	sam_fseq_arg_t	fseq_arg;
	int err;

	fseq_arg.cmd = SAM_FIND_ORD;
	fseq_arg.fseq = mp->fi_eq;
	fseq_arg.eq = control.eq;
	fseq_arg.ord = ord;
	fseq_arg.id = ip->di.id;
	fseq_arg.on_ord = 0;

	err = sam_syscall(SC_fseq_ord, &fseq_arg, sizeof (fseq_arg));
	if (err < 0) {
		fseq_arg.on_ord = 0;
	}
	return (fseq_arg.on_ord);
}


/*
 * sam_process_file - release/remove the given file.
 * The release only releases archive files. It is important to
 * release files before moving the disk blocks because the eq
 * may be totally down.
 */
static int
sam_process_file(
	struct sam_fs_info *mp,
	sam_perm_inode_t *ip)
{
	int error = 0;

	/*
	 * Check for do_not_execute parameter.
	 */
	if (control.do_not_execute) {
		sam_write_log(ip, FALSE, "--", 0);
		return (0);
	}

	if (control.command == DK_CMD_release) {
		/*
		 * Release file if release command.
		 * Release any partial blocks because we are draining the ord.
		 * If file has no archive copy, EBADE is returned. The
		 * release does not complete successfuly if there are file
		 * with no archive copy. You must type the remove command
		 * to move all the unarchived files to an alternate eq.
		 */
		sam_fsdropds_arg_t	dropds_arg;

		dropds_arg.fseq = mp->fi_eq;
		dropds_arg.id = ip->di.id;
		dropds_arg.freeblocks = 0;
		dropds_arg.shrink = 1;	/* Release any partial blocks */
		errno = 0;
		if (sam_syscall(SC_fsdropds, &dropds_arg,
		    sizeof (dropds_arg)) == 0) {
			if (control.display_all_files) {
				sam_write_log(ip, TRUE, "RE", 0);
			}
			return (0);	/* Successfully released file */
		}
		error = errno;
		if (error == EBUSY) {
			Trace(TR_ERR,
			    "Can't release inode %d.%d: staging or archiving",
			    ip->di.id.ino, ip->di.id.gen);
			pthread_mutex_lock(&log_lock);
			control.busy_files++;
			pthread_mutex_unlock(&log_lock);
		} else if (error == EBADE) {
			Trace(TR_ERR,
			    "Can't release inode %d.%d: No archive copy",
			    ip->di.id.ino, ip->di.id.gen);
			pthread_mutex_lock(&log_lock);
			control.unarchived_files++;
			pthread_mutex_unlock(&log_lock);
			if (control.display_all_files && (error != ENOENT)) {
				sam_write_log(ip, FALSE, "NA", 0);
			}
			return (error);
		} else if (error != ENOENT) {
			Trace(TR_ERR, "Can't release inode %d.%d: %s",
			    ip->di.id.ino, ip->di.id.gen, strerror(error));
		}
	} else if (control.command == DK_CMD_remove) {
		/*
		 * Move files if remove command. The remove will complete
		 * successfully if there are no errors moving the files.
		 */
		error = sam_move_file(mp, ip);
		if (error == 0) {
			if (control.display_all_files) {
				sam_write_log(ip, FALSE, "MV", 0);
			}
			return (0);	/* Successfully moved file */
		} else if (error < 0) {
			error = errno;
		}
		if (error != ENOENT) {
			Trace(TR_ERR, "Can't move inode %d.%d: %s",
			    ip->di.id.ino, ip->di.id.gen, strerror(error));
		}
	} else {
		error = ENOTTY;
		Trace(TR_ERR, "sam_process_file: invalid command: %d",
		    control.command);
	}
	if (control.display_all_files && (error != ENOENT)) {
		sam_write_log(ip, FALSE, "ER", error);
	}
	return (error);
}


/*
 * sam_write_log - Write entries to log.
 * cmd: RE = release; RM = remove; NA = Not archived; ER = error
 */
static void
sam_write_log(
	sam_perm_inode_t *ip,		/* Pointer to inode */
	boolean_t released,		/* TRUE if file released */
	char *cmd,			/* 2 char specifier */
	int error)			/* Errno */
{
	char pathname[MAXPATHLEN + 4];
	char *p = &pathname[0];
	char seg_num[16] = "S0";
	char rel_str[16] = "--";
	time_t log_time;

	if (S_ISSEGS(&ip->di)) {
		sprintf(seg_num, "S%d", ip->di.rm.info.dk.seg.ord + 1);
	}
	if (getfullpathname(control.mountpoint, ip->di.id, &p)) {
		if (error == 0) {
			error = errno;
		}
	}
	if (error) {
		sprintf(rel_str, "%d", error);
	} else if (released) {
		if (ip->di.status.b.pextents && control.stage_partial) {
			sprintf(rel_str, " P");
			(void) sam_stage(p, "isp");
		}
		if (ip->di.status.b.nodrop || control.stage_files) {
			sprintf(rel_str, " S");
			(void) sam_stage(p, "is");
		}
	}
	pthread_mutex_lock(&log_lock);
	fprintf(control.log, "%s %d.%d %s %s %s\n", cmd, ip->di.id.ino,
	    ip->di.id.gen, rel_str, seg_num, p);
	Trace(TR_MISC, "%s %d.%d %s %s %s", cmd, ip->di.id.ino,
	    ip->di.id.gen, rel_str, seg_num, p);
	log_time = time(NULL);
	if ((control.log_time + 60) < log_time) {
		fflush(control.log);
	}
	pthread_mutex_unlock(&log_lock);
}


/*
 * sam_clean_thr - Close thread file descriptors and free memory
 * after receiving signal.
 */
/* ARGSUSED */
void
sam_clean_thr(int sig)
{
	int i = 0;

	control.shutdown = 1;

	/*
	 * Kill active threads
	 */
	for (i = 0; i < MAX_THREADS; i++) {
		if (control.thr[i].tid > 0) {
			Trace(TR_MISC, "sam_clean_thr: killing thread %d\n",
			    control.thr[i].tid);
			pthread_kill(control.thr[i].tid, SIGKILL);
		}
	}

	/*
	 * Close file descriptors
	 */
	for (i = 0; i < MAX_THREADS; i++) {
		if (control.thr[i].fd > 0) {
			Trace(TR_MISC, "sam_clean_thr: closing "
			    "control.thr[%d].fd\n", i);
			(void) close(control.thr[i].fd);
		}

		if (control.thr[i].fdo > 0) {
			Trace(TR_MISC, "sam_clean_thr: closing "
			    "control.thr[%d].fdo\n", i);
			(void) close(control.thr[i].fdo);
		}
	}

	/*
	 * Free work buffers
	 */
	for (i = 0; i < MAX_QUEUE; i++) {
		if (work[i].buffer != NULL) {
			Trace(TR_MISC, "sam_clean_thr: freeing "
			    "work[%d].buffer\n", i);
			(void) free(work[i].buffer);
		}
	}
}


/*
 * getfullpathname - Given an inode id structure (inum,generation number), the
 * mount point, and return the full pathname to that inode. If an error occurs,
 * the "pathname" returned will be the string "Cannot find pathname for ..."
 * as shown below.
 */
static int			/* 0 if successful, -1 if cannot get pathname */
getfullpathname(
	char *fsname,		/* Mount point */
	sam_id_t id,		/* Inode.Generation number */
	char **fullpath)	/* Memory space MAXPATHLEN=4 chars allocated */
{
	offset_t offset;	/* Current offset in directory */
	int n_valid;		/* Dirbuf[0..n_valid] are valid info */
	int dir_fd;		/* FD on which the directory is open */
	char *dirbuf;		/* Directory buffer */
	int fs_fd;		/* FD for mount point */
	char *msg;


	n_valid = 0;
	offset = 0;
	dir_fd = -1;
	dirbuf = (char *)malloc(DIRBUF_SIZE);
	if (dirbuf == NULL) {
		if (errno) {
			Trace(TR_ERR, "getfullpathname malloc error: "
			    "inode %d.%d err=%d %s",
			    id.ino, id.gen, errno, strerror(errno));
		}
		return (-1);
	}

	/*
	 * The mount point name is given -- open it.
	 */
	strcpy(*fullpath, fsname);
	fs_fd = open(fsname, O_RDONLY);
	if (fs_fd >= 0) {
		if (sam_id2path(fs_fd, &dir_fd, id, fullpath, &offset,
		    &n_valid, dirbuf) > 0) {
			close(fs_fd);
			sam_closedir(&dir_fd);
			free(dirbuf);
			return (0);
		}
		close(fs_fd);
	}
	sam_closedir(&dir_fd);
	msg = catgets(catfd, SET, 584,
	    "Cannot find pathname for filesystem %s inum/gen %lu.%ld");
	sprintf(*fullpath, msg, fsname, id.ino, id.gen);
	free(dirbuf);
	return (-1);
}


/*
 * sam_id2path - Recursive helper routine for getfullpathname.
 */
static int 		/* return value = name buffer length, or zero */
sam_id2path(
	int fs_fd,
	int *dir_fd,
	sam_id_t id,
	char **fullpath,
	offset_t *offset,
	int *n_valid,
	char *dirbuf)
{
	union sam_di_ino inode;
	struct sam_ioctl_idstat idstat;
	struct sam_dirent *dirent;
	sam_id_t parent_id;
	int parent_strlen;
	struct sam_dirent *current = 0;

	/*
	 * Terminate recursion when we are asked to find inum == 2.
	 * This indicates the top of the sam-fs tree.
	 */
	if (id.ino == 2 || id.ino == 0) {
		return (strlen(*fullpath) - 1);
	}

	/*
	 * IDSTAT the id, getting the inode in question
	 */
	idstat.id = id;
	idstat.size = sizeof (struct sam_perm_inode);
	idstat.dp.ptr = (void *)&inode;
	if (ioctl(fs_fd, F_IDSTAT, &idstat) < 0) {
		if (errno) {
			Trace(TR_ERR, "sam_id2path F_IDSTAT: "
			    "%s inode %d.%d err=%d %s",
			    *fullpath, id.ino, id.gen, errno, strerror(errno));
		}
		return (0);
	}

	/*
	 * Jump back over the segment inode to the base segment inode
	 */
	if (inode.inode.di.status.b.seg_ino) {
		id = inode.inode.di.parent_id;
		idstat.id = id;
		idstat.size = sizeof (struct sam_perm_inode);
		idstat.dp.ptr = (void *)&inode;
		if (ioctl(fs_fd, F_IDSTAT, &idstat) < 0) {
			if (errno) {
				Trace(TR_ERR, "sam_id2path segment F_IDSTAT: "
				    "%s inode %d.%d err=%d %s",
				    *fullpath, id.ino, id.gen, errno,
				    strerror(errno));
			}
			return (0);
		}
	}

	/*
	 * next, get that parent's pathname (recursively)
	 */
	parent_id = inode.inode.di.parent_id;
	if ((parent_strlen = sam_id2path(fs_fd, dir_fd, parent_id, fullpath,
	    offset, n_valid, dirbuf)) == 0) {
		if (errno) {
			Trace(TR_ERR, "sam_id2path: %s inode %d.%d err=%d %s",
			    *fullpath, id.ino, id.gen, errno, strerror(errno));
		}
		return (0);
	}

	/*
	 * Open the parent(it should be a directory) and look for an
	 * entry which matches the id we're looking for.
	 */
	if (sam_opendir(dir_fd, *fullpath) < 0) {
		if (errno) {
			Trace(TR_ERR, "sam_opendir: %s inode %d.%d err=%d %s",
			    *fullpath, id.ino, id.gen, errno, strerror(errno));
		}
		return (0);
	}
	*offset = 0;		/* Flag sam_readdir to start at beginning */
	*n_valid = 0;		/* Flag sam_getdent to read up buffer */
	while (sam_getdent(*dir_fd, dirbuf, &dirent, offset, n_valid,
	    &current) > 0) {
		char *pathp = *fullpath;

		if (dirent->d_id.ino == id.ino && dirent->d_id.gen == id.gen) {
			pathp[parent_strlen + 1] = '/';
			strcpy(&pathp[parent_strlen + 2],
			    (const char *)dirent->d_name);
			return (parent_strlen +
			    strlen((const char *)dirent->d_name) + 1);
		}
	}
	return (0);
}


/*
 * sam_getdent - loop through the directory, returning when a valid entry
 * is found..
 */
static int
sam_getdent(
	int dir_fd,
	char *dirbuf,			/* Pointer to directory buffer */
	struct sam_dirent **dirent,	/* Ptr ptr to directory entry */
	offset_t *offset,		/* Offset into directory buffer */
	int *n_valid,			/* Offset into Directory buffer */
	struct sam_dirent **current)	/* Ptr ptr to next directory entry */
{
	int size;

	do {
		if (*n_valid == 0) {
			*n_valid = sam_readdir(dir_fd, dirbuf, offset,
			    DIRBUF_SIZE);
			if (*n_valid < 0) {
				return (-1);	/* Error */
			} else if (*n_valid == 0) {
				return (0);	/* EOF */
			}
			*current = (struct sam_dirent *)(void *)&dirbuf[0];
		}
		*dirent = *current;
		size = SAM_DIRSIZ(*dirent);
		*current = (struct sam_dirent *)(void *)
		    ((char *)*current + size);
		if ((char *)*current >= &dirbuf[*n_valid]) {
			*n_valid = 0;
		}
	} while ((*dirent)->d_fmt == 0);
	return (size);			/* valid */
}


/*
 * sam_readdir - Read the directory, select unformatted output.
 * Return length of directory returned.
 */
static int
sam_readdir(
	int dir_fd,		/* fd on which the directory is open */
	char *dirbuf,		/* returned directory entry */
	offset_t *offset,	/* current offset into directory */
	int dirbuf_size)	/* size of dir entry */
{
	sam_ioctl_getdents_t getdent;
	int ngot;

	(void) memset(dirbuf, 0, dirbuf_size);
	getdent.dir.ptr = (struct sam_dirent *)(void *)dirbuf;
	getdent.size = dirbuf_size;
	getdent.offset = *offset;

	ngot = ioctl(dir_fd, F_GETDENTS, &getdent);
	*offset = (offset_t)getdent.offset;
	return (ngot);
}


/*
 * sam_opendir - Open the directory.
 */
static int
sam_opendir(
	int *dir_fd,
	char *fullpath)
{
	sam_closedir(dir_fd);	/* in case it's still open */
	*dir_fd = open(fullpath, O_RDONLY);
	return (*dir_fd);
}


/*
 * sam_closedir - Close the directory.
 */
static void
sam_closedir(int *dir_fd)
{
	if (*dir_fd >= 0) {
		(void) close(*dir_fd);
		*dir_fd = -1;
		return;
	}
}

/*
 * sam_move_file - move any data for the specified file that
 * is on the specified ordinal to another device.
 */
static int
sam_move_file(
	struct sam_fs_info *mp,
	sam_perm_inode_t *ip)
{
	sam_fseq_arg_t	fseq_arg;
	int err;

	fseq_arg.cmd = SAM_MOVE_ORD;
	fseq_arg.fseq = mp->fi_eq;
	fseq_arg.eq = control.eq;
	fseq_arg.ord = control.ord;
	fseq_arg.id = ip->di.id;
	fseq_arg.on_ord = 0;
	/*
	 * Valid device ordinal to move data to
	 * otherwise -1.
	 */
	fseq_arg.new_ord = control.ord2;

	err = sam_syscall(SC_fseq_ord, &fseq_arg, sizeof (fseq_arg));
	return (err);
}
