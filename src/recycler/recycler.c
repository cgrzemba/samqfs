/*
 * recycler - recycle media.
 *
 * recycler examines the file system and determines what volumes need
 * to be recycled. Recycle candidates are marked read only and active
 * files on recycled media are rearchived.
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
 *
 */

#pragma ident "$Revision: 1.84 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
char	*ctime_r(const time_t *clock, char *buf, int buflen);

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

/* Solaris headers. */
#include <syslog.h>
#include <signal.h>
#include <assert.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/lib.h"
#include "sam/resource.h"
#include "sam/uioctl.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "sam/defaults.h"
#include "sam/sam_trace.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/odlabels.h"
#include "aml/tapes.h"
#include "aml/samapi.h"
#define	DEC_INIT	/* Recycler uses sam_chmed_value api function */
#include "aml/remote.h"	/* which checks for sam remote, so recycler must */
#undef  DEC_INIT	/* have the sam remote functions declared. */
#include "sam/lint.h"
#include "aml/diskvols.h"

#define	MAIN
#include "sam/nl_samfs.h"

/* Local headers. */
#include "recycler.h"

/* global data */

int VSNs_in_robot;	/* flag indicating if we are scanning a catalog(1), */
			/* or a filesystem's .inodes file(0), so that we */
			/* can flag the VSN_table entry */

/* Private data */

/* Temp buffers used in select_candidates for messages */
static char fu_buf[S2ABUFSIZE];		/* temp buffer for free up message */
static char qu_buf[S2ABUFSIZE];		/* temp buffer for quantity message */
static char su_buf[S2ABUFSIZE];		/* */

/* getopt() globals  */
extern char *optarg;
extern int optind, opterr, optopt;

static union sam_di_ino
	inodes[(INO_BLK_FACTOR * INO_BLK_SIZE / sizeof (union sam_di_ino)) + 2];

static int SamFd;		/* fd open on filesystem's mount point */
static char datentime[512];	/* date and time at start */

/* Command line options */
static boolean_t show_extrapolated_capacity;	/* -c */
static boolean_t suppress_vsn_listing;		/* -V */
static boolean_t suppress_empty_vsn;		/* -E */
static boolean_t ignore_all;			/* -n */

/*
 * Variables which communicate between the message reader thread and
 * the main thread
 */

/* Linked list of regexps describing no_recycle VSN/media types */
struct no_recycle *no_recycle = NULL;

/* Private functions. */
static void accumulate_filesystem(struct sam_fs_info *);
static void accumulate_inode(char *, union sam_di_ino *, int,
	long long, VSN_TABLE *);
static void assign_vsns(void);
static void check_inode(char *, union sam_di_ino *, int,
	long long, VSN_TABLE *);
static void check_relabel(void);
static void detect_another_recycler(void);
static void display_rm_vsns(void);
static void display_dk_vsns(void);
static char *family_type(ROBOT_TABLE *robot);
static void flag_recycle(VSN_TABLE *vsn);
static void log_header(void);
static void log_trailer(void);
static void mark_filesystem(struct sam_fs_info *);
static void process_desc(vsndesc_t, struct ArchSet *, int);
static void process_dk_desc(vsndesc_t, int, int);
static void process_pool(vsndesc_t, struct ArchSet *, int);
static void process_dk_pool(struct VsnPool *, int, int);
static void recycle_copy(char *, union sam_di_ino *, int);
static void readArchset(void);
static void select_rm_candidates(void);
static void select_dk_candidates(void);
static void select_this_candidate(VSN_TABLE *, ROBOT_TABLE *, FILE *);
static void sort_vsns(void);
static void strFromFreeup(long long value);
static void strFromQuantity(long long value, boolean_t limit);
static void strFromVsncount(int value, boolean_t limit);
static void summarize_vsns(void);
static void summarize_dk_vsn(VSN_TABLE *vsn);
static void sync_and_sleep(void);
static int  getMultiVsnInfo(char *, union sam_di_ino *, int,
	struct sam_section *, size_t);
static int  getMinGain(ROBOT_TABLE *robot, VSN_TABLE *vsn);

void
main(
	int argc,	/* Number of arguments */
	char **argv)	/* Argument pointer list */
{
	struct sam_fs_info *fsp;
	int i;
	int robot_i;
	char c;
	time_t clock = time(NULL);

	program_name = "recycler";
	cannot_recycle = FALSE;

	/*
	 * Set command-line options to default values.
	 */

	log = NULL;
	display_selection_logic = FALSE;
	check_expired = TRUE;
	display_draining_vsns = FALSE;
	ignore_all = FALSE;
	show_extrapolated_capacity = FALSE;
	suppress_catalog = FALSE;
	suppress_vsn_listing = FALSE;
	suppress_empty_vsn = FALSE;
	catalog_summary_only = FALSE;

	/*
	 * Set date and time string for postmortem files
	 */

	(void) cftime(datentime, "%m%d.%H%M", &clock);

	/*
	 * Open message catalog file.
	 */

	CustmsgInit(0, NULL);

	/*
	 * Make sure there's no other instance of recycler at the moment.
	 */

	detect_another_recycler();

	/*
	 * Command-line option processing.
	 */

	while ((c = getopt(argc, argv, "cCdEnsvVxXS:")) != EOF) {
		switch (c) {
		case 'c':
			show_extrapolated_capacity = TRUE;
			break;
		case 'C':
			suppress_catalog = TRUE;
			break;
		case 'd':
			display_selection_logic = TRUE;
			break;
		case 'E':
			suppress_empty_vsn = TRUE;
			break;
		case 'n':
			ignore_all = TRUE;
			break;
		case 's':
			catalog_summary_only = TRUE;
			break;
		case 'v':
			display_draining_vsns = TRUE;
			break;
		case 'V':
			suppress_vsn_listing = TRUE;
			break;
		case 'x':
			/* for compatibility - default is now -x */
			check_expired = TRUE;
			break;
		case 'X':
			/* ... and you need to use -X to disable */
			check_expired = FALSE;
			break;
		case '?':
			cannot_recycle = TRUE;
			break;
		}
	}

	TraceInit("recycler", TI_recycler);
	Trace(TR_MISC, "Recycler daemon started");
	Init_shm();   /* sets log_facility, fifo_path, etc_fs_samfs globals */

	/*
	 * Allocate the initial VSN araries.
	 */
	vsn_table = (struct VSN_TABLE *)
	    malloc(VSN_INCREMENT * sizeof (struct VSN_TABLE));
	table_used = 0;
	table_avail = VSN_INCREMENT;
	memset(vsn_table, 0, VSN_INCREMENT * sizeof (struct VSN_TABLE));
	vsn_permute = (int *)malloc(VSN_INCREMENT * sizeof (int));
	/*
	 * Initialize the vsn_permute arrary to the identity permutation.
	 * (Later, sort_vsns() will modify it so that referencing
	 * vsn_table[vsn_permute[i]], varying i from 0 to vsn_count, will
	 * give you the VSNs in order of best-to-recycle to worst-to-recycle.
	 */
	for (i = 0; i < table_avail; i++) {
		vsn_permute[i] = i;
	}

	/*
	 * Get filesystem configuration information.  All filesystems
	 * that are writable or for which we're the metadata server
	 * must be mounted to allow the .inodes file to be read and
	 * thus allow recycling.  Init_fs() returns only filesystems that
	 * are writable or for which we're the server.
	 */
	Init_fs();
	if (NULL == first_fs) {
		emit(TO_ALL, LOG_ERR, 20296);
		emit(TO_ALL, LOG_ERR, 2054);
		exit(1);
	}

	readArchset();			/* read archset table from archiver */
	if (cannot_recycle) {
		emit(TO_ALL, LOG_ERR, 2054);
		exit(1);
	}

	MapCatalogs();			/* map in catalogs */
	if (cannot_recycle) {
		emit(TO_ALL, LOG_ERR, 2054);
		exit(1);
	}

	/*
	 * Configure disk volumes.
	 */
	(void) DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0);

	VSNs_in_robot = TRUE;

	Readcmd();		/* read command file */

	log_header();		/* print banner to log file */
	(void) atexit(&log_trailer);


	ScanCatalogs();	/* get vsn information into VSN table from catalogs */

	/*
	 * All inode-writable filesystems must be mounted to allow the
	 * .inodes file to be read and thus allow recycling.  Check
	 * if any of the SAM-FS file systems are not mounted.
	 */
	for (i = 0, fsp = first_fs; i < num_fs; i++, fsp++) {
		if ((fsp->fi_status & FS_MOUNTED) == FALSE) {
			emit(TO_ALL, LOG_ERR, 1180, fsp->fi_name);
			cannot_recycle = TRUE;
		}
	}

	/*
	 * If an error, not all SAM-FS file systems are mounted,
	 * has occurred which will inhibit recycling, stop now.
	 */
	if (cannot_recycle) {
		emit(TO_ALL, LOG_ERR, 2054);
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		exit(1);
	}

	/*
	 * Assign vsns to archive sets as needed.
	 * If disk archiving, we need the max seqnum before the
	 * file system scan.  The max seqnum is found during
	 * this assign pass.
	 */
	assign_vsns();

	/* scan each filesystem's .inode file */

	VSNs_in_robot = FALSE;
	for (i = 0, fsp = first_fs; i < num_fs; i++, fsp++) {
		accumulate_filesystem(fsp);
	}

	/* process vsn table */
	summarize_vsns();

	/*
	 * if an non-option argument were specified, it's the name of the
	 * single robot or archive set which is to be recycled.  Set all the
	 * others to "ignore", regardless of the recycler.cmd file specs.
	 */
	if (optind < argc) {
		int any = FALSE;

		for (robot_i = 0; robot_i < ROBOT_count; robot_i++) {
			ROBOT_TABLE *Robot = &ROBOT_table[robot_i];
			if (strcmp(family_name(Robot), argv[optind]) == 0) {
				Robot->ignore = FALSE;
				any = TRUE;
			} else {
				Robot->ignore = TRUE;
			}
		}
		if (!any) {
			emit(TO_ALL, LOG_ERR, 1785, argv[optind]);
			cannot_recycle = TRUE;
		}
	}

	/*
	 * If -n was specified on the command line, set all robots to be
	 * ignored we will still print the details of what would have happened.
	 */
	if (ignore_all) {
		for (robot_i = 0; robot_i < ROBOT_count; robot_i++) {
			ROBOT_TABLE *Robot = &ROBOT_table[robot_i];
			Robot->ignore = TRUE;
		}
	}

	/*
	 * Sort the vsn_permute table in decending order of gains
	 * from recycling
	 */
	sort_vsns();

	/*
	 * If an error has occured up to this point which will inhibit
	 * recycling, stop now.
	 */

	if (cannot_recycle) {
		emit(TO_ALL, LOG_ERR, 2054);
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		exit(1);
	}

	PrintCatalogs();
	PrintDiskArchsets();

	/* determine which VSNs will be recycled */
	select_rm_candidates();
	select_dk_candidates();

	/* display what we know about the VSNs */
	if (!suppress_vsn_listing) {
		display_rm_vsns();
		display_dk_vsns();
	}

	/*
	 * Process each filesystem again, marking the copies on candidate media
	 * "to be recycled"
	 */

	for (i = 0, fsp = first_fs; i < num_fs; i++, fsp++) {
		char buff[MAXPATHLEN];
		struct sam_stat sb;

		SamFd = open(fsp->fi_mnt_point, O_RDONLY);
		if (SamFd >= 0) {
			sprintf(buff, "%s/.", fsp->fi_mnt_point);
			if ((sam_stat(buff, &sb, sizeof (sb)) < 0) ||
			    (0 == (sb.attr & SS_SAMFS))) {
					emit(TO_ALL, LOG_ERR, 20297, buff);
					cannot_recycle = TRUE;
			} else {
				mark_filesystem(fsp);
			}
			(void) close(SamFd);
		} else {
			emit(TO_ALL, LOG_ERR, 20262, fsp->fi_name, errtext);
			cannot_recycle = TRUE;
		}
	}

	/* find VSNs which are now 100% junk+free */

	check_relabel();

	/*
	 * Recycle disk archives.
	 */
	RecycleDiskArchives();
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);

	/* final check for errors */

	if (cannot_recycle) {
		emit(TO_ALL, LOG_ERR, 2055);
		(void) PostEvent(RY_CLASS, "Incomplete", 2055, LOG_ERR,
		    GetCustMsg(2055), NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		exit(1);
	}

	emit(TO_FILE, 0, 2053);
	dump_hash_stats();
	exit(0);
}


/*
 * ----- accumulate_inode - Accumulate into the VSN table a section of
 * an archive image of an inode.  Called by the handle_inode routine,
 * during the first pass of the .inode file.
 */
static void
accumulate_inode(
	char *fs_name,
	union sam_di_ino *inode,	/* inode read from .inodes file. */
	int copy,			/* archive copy number 0..3 */
	long long length,		/* number of bytes in this section */
	VSN_TABLE *vsn)			/* VSN on which the section resides */
{
	/*
	 * Accumulate inodes for disk archiving.
	 * Only accumulate inodes in the recycling seqnum range.
	 */
	if (IS_DISK_MEDIA(vsn->media)) {

		if (inode->inode.ar.image[copy].position <= vsn->maxSeqnum) {

			if (*TraceFlags & (1 << TR_debug)) {
				char *path;

				path = id_to_path(fs_name, inode->inode.di.id);

				Trace(TR_DEBUG,
				    "[%s] Accumulate file: '%s' "
				    "copy: %d size: %lld",
				    vsn->vsn, path, copy+1, length);
			}
			vsn->size += length;
			vsn->count++;
		}

	/*
	 * Note that we silently ignore files which are on media which
	 * has been relabeled, unless the check_expired flag is set
	 */
	} else if (vsn->label_time <=
	    inode->inode.ar.image[copy].creation_time) {

		vsn->size += length;
		(vsn->count)++;

		/*
		 * If file is marked "archive -n", then we cannot recycle
		 * the medium on which it's been archived.
		 */
		if (inode->inode.di.status.b.noarch &&
		    !S_ISDIR(inode->inode.di.mode)) {
			vsn->has_noarchive_files = TRUE;
		}

	} else if (check_expired) {
		time_t creation_time;

		char *pathname = id_to_path(fs_name, inode->inode.di.id);
		char archive_time[512], label_time[512], ctime_buf[512];
		char msgbuf[1024];

		creation_time =
		    (time_t)inode->inode.ar.image[copy].creation_time;
		strcpy(archive_time,
		    ctime_r(&creation_time, ctime_buf, sizeof (ctime_buf)));
		archive_time[strlen(archive_time)-1] = '\0';

		strcpy(label_time,
		    ctime_r(&(vsn->label_time), ctime_buf, sizeof (ctime_buf)));
		label_time[strlen(label_time)-1] = '\0';

		emit(TO_ALL, LOG_ERR, 20218, pathname);

		/* Send sysevent to generate SNMP trap */
		sprintf(msgbuf, GetCustMsg(20218), pathname);
		(void) PostEvent(RY_CLASS, "ExpiredArch", 20218, LOG_ERR,
		    msgbuf, NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);

		emit(TO_ALL, LOG_ERR, 20219, archive_time, copy+1);
		emit(TO_ALL, LOG_ERR, 20220, label_time,
		    sam_mediatoa(vsn->media), vsn->vsn);
		cannot_recycle = TRUE;

	}
#ifdef DEBUGxtra
	printf("Count now %d on %s, size %lld\n",
	    vsn->count, vsn->vsn, vsn->size);
#endif

}

/*
 * ----- handle_inode - Process file during recycler.
 * Process the given inode's archive copy sections.  For each section,
 * the VSN is looked up and passed to the process function.
 */

static void
handle_inode(
	char *fs_name,			/* name of filesystem */
	union sam_di_ino *inode,	/* inode read from .inodes file */
	void (*process)(char *, union sam_di_ino *, int,
	    long long, VSN_TABLE *))
{
	int media_type, copy, sec_i;
	VSN_TABLE *vsn;
	char *path;

	/* can we skip this inode? */
	if (inode->inode.di.mode == 0) {	/* inode unallocated */
		return;
	}

	if (inode->inode.di.arch_status == 0) {		/* Not archived */
		return;
	}
	if (S_ISEXT(inode->inode.di.mode)) {		/* Inode extension */
		return;
	}

	/* does the archive status flag look ok? */
	if ((inode->inode.di.arch_status & 0xf) !=
	    inode->inode.di.arch_status) {
		path = id_to_path(fs_name, inode->inode.di.id);
		emit(TO_ALL, LOG_ERR, 498, inode->inode.di.arch_status, path);
		cannot_recycle = TRUE;
		return;
	}

	/* process each archive copy */
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {

		/* if the archive copy isn't valid, skip it */
		if ((inode->inode.di.arch_status & (1 << copy)) == 0) {
			continue;
		}

		/* skip the archive copy if it's on a third-party medium. */
		if ((inode->inode.di.media[copy] & DT_CLASS_MASK) ==
		    DT_THIRD_PARTY) {
			continue;
		}

		/*
		 * check media type for validity.  Note that each copy only
		 * has one media type, even though it may have multiple VSNs.
		 * Each VSN will be on the same media type.
		 */
		for (media_type = 0; dev_nm2mt[media_type].nm != NULL;
		    media_type++) {
			if (inode->inode.di.media[copy] ==
			    dev_nm2mt[media_type].dt) {
				break;
			}
		}

		if (dev_nm2mt[media_type].nm == NULL) {
			path = id_to_path(fs_name, inode->inode.di.id);
			emit(TO_ALL, LOG_ERR, 2738, inode->inode.di.media[copy],
			    path, copy+1);
			cannot_recycle = TRUE;
			continue;	/* skip to next copy */
		}

	/*
	 * Check the n_vsns field.  There are three cases:
	 * 	<= 0:  Error
	 *	== 1:  Usual case where an archive copy is on only one VSN
	 *	>  1:  Need to call getMultiVsnInfo to get information
	 *			for the volume-overflow archive copy.
	 */

		if (inode->inode.ar.image[copy].n_vsns <= 0) {
			path = id_to_path(fs_name, inode->inode.di.id);
			emit(TO_ALL, LOG_ERR, 1421,
			    inode->inode.ar.image[copy].n_vsns, copy, path);
			cannot_recycle = TRUE;
			continue;	/* skip to next copy */

		} else if (inode->inode.ar.image[copy].n_vsns == 1) {

			/* Only one VSN - process it */
			vsn = Find_VSN(inode->inode.di.media[copy],
			    inode->inode.ar.image[copy].vsn);
			if (vsn == NULL) {
				path = id_to_path(fs_name, inode->inode.di.id);
				emit(TO_ALL, LOG_ERR, 20330, copy+1, path);
				cannot_recycle = TRUE;
				continue;
			}
			process(fs_name, inode, copy, inode->inode.di.rm.size,
			    vsn);

		} else if (inode->inode.ar.image[copy].n_vsns != 1) {
			struct sam_section *sections = NULL;
			struct sam_section *vsns;
			size_t size;
			int error;

			/* If multiple VSN's present, get list of sections */
			size = SAM_SECTION_SIZE(
			    inode->inode.ar.image[copy].n_vsns);
			SamMalloc(sections, size);
			vsns = sections;

			if ((error = getMultiVsnInfo(fs_name, inode,
			    copy, vsns, size)) < 0) {
				/*
				 * Issue error for all error conditions,
				 * except when the inode was deleted
				 * while it is being processed.
				 */
				if (error != -1 || errno != ENOENT) {
					emit(TO_ALL, LOG_ERR, 20326,
					    errtext,
					    inode->inode.di.id.ino,
					    inode->inode.di.id.gen,
					    copy+1);
					cannot_recycle = TRUE;
				}
				SamFree(sections);
				continue;
			}

			/* Process each section */
			for (sec_i = 0;
			    sec_i < inode->inode.ar.image[copy].n_vsns;
			    sec_i++, vsns++) {

				vsn = Find_VSN(inode->inode.di.media[copy],
				    vsns->vsn);
				if (vsn == NULL) {
					path = id_to_path(fs_name,
					    inode->inode.di.id);
					emit(TO_ALL, LOG_ERR, 20331, sec_i,
					    copy+1, path);
					cannot_recycle = TRUE;
					continue;
				}

				process(fs_name, inode, copy,
				    vsns->length, vsn);
			}  /* end for each section loop */
			SamFree(sections);
		}
	}  /* end for each copy loop */
}


/*
 * ----- accumulate_filesystem
 * Scan the indicated filesystem and accumulate VSN statistics.
 * First .inodes file pass.
 */

static void
accumulate_filesystem(
	struct sam_fs_info *fs_params)
{
	char buf[PATH_MAX];
	int fd, ngot, ninodes, inode_i;
	int expected_ino = 0;

	sync_and_sleep();
	if ((fd = OpenInodesFile(fs_params->fi_mnt_point)) < 0) {
		emit(TO_ALL, LOG_ERR, 20262, fs_params->fi_name, errtext);
		cannot_recycle = TRUE;
		return;
	}

	Trace(TR_MISC, "Accumulate filesystem: %s", fs_params->fi_mnt_point);

	while ((ngot = read(fd, &inodes, INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {
		ninodes = ngot / sizeof (union sam_di_ino);
		for (inode_i = 0; inode_i < ninodes; inode_i++) {
			expected_ino ++;

			if (inodes[inode_i].inode.di.id.ino == expected_ino &&
			    inodes[inode_i].inode.di.mode != 0 &&
			    inodes[inode_i].inode.di.arch_status != 0 &&
			    ! (S_ISEXT(inodes[inode_i].inode.di.mode))) {

				/*
				 * If this is a "request" file, then flag all
				 * VSNs it references as not-recyclable.
				 */
				if (S_ISREQ(inodes[inode_i].inode.di.mode)) {
					int i;
					struct sam_rminfo resource;
					char *name;

/* FIXME handle_request_inode(fs_params, &inodes[inode_i]); */

					name = id_to_path(
					    fs_params->fi_mnt_point,
					    inodes[inode_i].inode.di.id);

					memset(&resource, 0,
					    sizeof (struct sam_rminfo));

					if (sam_readrminfo(name, &resource,
					    sizeof (resource)) < 0) {
						emit(TO_ALL, LOG_ERR, 1063,
						    name, errtext);
						continue;
					}

			/* N.B. Bad indentation to meet cstyle requirements. */
					for (i = 0; i < resource.n_vsns; i ++) {
					VSN_TABLE *vsn;

					Trace(TR_MISC, "Request file: %s "
					    "vsn: '%s'",
					    name,
					    resource.section[i].vsn);

					vsn = Find_VSN(
					    sam_atomedia(resource.media),
					    resource.section[i].vsn);

					if (vsn == NULL) {
						emit(TO_ALL, LOG_ERR, 20332,
						    i, name);
						continue;
					}
					vsn->has_request_files = TRUE;
					}
				} else {
					handle_inode(fs_params->fi_mnt_point,
					    &inodes[inode_i],
					    &accumulate_inode);
				}
			}
		}
	}

	if (ngot != 0) {
		sprintf(buf, "%s/.inodes", fs_params->fi_mnt_point);
		emit(TO_ALL, LOG_ERR, 1063, buf, errtext);
		cannot_recycle = TRUE;
	}

	(void) close(fd);
}


/*
 * ----- summarize_vsns
 * For simplicity of reference later on, calculate some statistics
 * for each vsn and store into the VSN_table.  Also set the
 * no_recycle flag for all vsns which match the no_recycle regular expressions.
 */
static void
summarize_vsns(void)
{
	int vsn_i, robot_i;
	struct no_recycle *p;

	Trace(TR_MISC, "Vsn table");

	for (robot_i = 0; robot_i < ROBOT_count; robot_i++) {
		ROBOT_table[robot_i].total_vsns = 0;
	}

	for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
		offset_t size;		/* amount of good data */
		offset_t capacity;	/* medium capacity */
		offset_t space;		/* medium space remaining */
		offset_t written;	/* amount of data written to medium */
		float alpha;		/* compression factor */
		int percent_good;	/* percent good data on medium */
		int percent_junk;	/* percent junk data on medium */
		int percent_free;	/* percent free space on medium */
		unsigned long long mask; /* hardware mask on space value */
		VSN_TABLE *VSN = &vsn_table[vsn_i];

		if (IS_DISK_MEDIA(VSN->media)) {
			summarize_dk_vsn(VSN);
			continue;
		}

		size = VSN->size;  /* byte size of found archive sections */

		if ((VSN->ce != NULL) || (IS_DISK_MEDIA(VSN->media))) {

			capacity = VSN->capacity;
			space =    VSN->space;

			if (IS_DISK_MEDIA(VSN->media)) {
				written = capacity - space;

			/*
			 * For mo, the amount which has been written is just
			 * cap-space. Tapes use ptoc_fwa as the number of
			 * blocks written, except * that some drives report -1
			 * as the number of blocks written when the medium is
			 * totally full.  Also, some drives use the high-order
			 * bits as flags or something, so those need to be
			 * masked off.
			 */

			} else if (
			    sam_atomedia(VSN->ce->CeMtype) == DT_ERASABLE ||
			    sam_atomedia(VSN->ce->CeMtype) == DT_PLASMON_UDO) {

				written = capacity - space;

			} else {

				switch (VSN->media) {
					case DT_9490:
					case DT_D3:
						mask = 0x003fffffLL;
						break;
					case DT_SQUARE_TAPE:
						mask = 0x00ffffffLL;
						break;
					case DT_9840:
					case DT_9940:
					case DT_TITAN:
					default:
						mask = 0xffffffffLL;
						break;
				}

				if (VSN->ce->m.CePtocFwa == mask) {
					/*
					 * If the tape says "I'm full",
					 * assume 2:1 compression, since we
					 * threw away the real number of
					 * blocks written, and so don't have
					 * any idea.
					 */
					written = 2L*VSN->capacity;
				} else {
					/*
					 * Otherwise, we can use (most of)
					 * ptoc_fwa as the number of blocks
					 * which were written to the tape
					 * already.
					 */
					written =
					    ((long long) VSN->ce->m.CePtocFwa &
					    mask) *
					    (long long) VSN->ce->CeBlockSize;
				}
			}

			/*
			 * alpha is used to factor in compression.  Note that a
			 * catalog entry * even before being archived to,
			 * ususally begins with some small non-zero ptoc value.
			 * We should take that into account as well.
			 */

			if ((written) && (capacity - space)) {
				alpha = ((float)capacity - (float)space) /
				    (float)written;
			} else {
				alpha = 1.0;
			}

			if (capacity) {

				if (size > written) {
					/*
					 * Amount of data for this vsn as
					 * accumulated from the inodes on all
					 * the filesystems is greater than the
					 * space written to the media.  Adjust
					 * size so we don't generate incorrect
					 * percentages (good, junk and free).
					 */
					Trace(TR_MISC,
					    "[%s] Size accumulated %lld > "
					    "space written %lld",
					    VSN->vsn, size, written);

					size = written;
				}

				percent_good = (int)((size*alpha) * 100LL /
				    capacity);
				percent_junk = (int)((capacity - space -
				    (size*alpha)) * 100LL / capacity);
				percent_free = 100LL - percent_good -
				    percent_junk;
			} else {
				/*
				 * We don't know much about medium if
				 * capacity is zero.
				 */
				percent_good = percent_junk = percent_free = 0;
			}

		} else {
			/* no catalog information available */

			capacity = space = written = 0LL;
			percent_good = percent_junk = percent_free = 0;
			alpha = 1.0;
			VSN->capacity = 0;
			VSN->space = 0;
		}

		VSN->free = percent_free;
		VSN->good = percent_good;
		VSN->junk = percent_junk;
		VSN->alpha = alpha;

		Trace(TR_MISC,
		    "[%s] free: %d%% good: %d%% junk: %d%% alpha: %f",
		    VSN->vsn, VSN->free, VSN->good, VSN->junk, VSN->alpha);

		for (p = no_recycle; p; p = p->next) {
			if ((regex(p->regexp, VSN->vsn, NULL) != NULL) &&
			    (p->medium == VSN->media)) {
				VSN->no_recycle = TRUE;
				Trace(TR_MISC, "[%s] no recycle vsn", VSN->vsn);
			}
		}
	}
}

static void
summarize_dk_vsn(
	VSN_TABLE *vsn)
{
	struct no_recycle *no;

	/* Amount of active data found on file systems for this disk volume */
	offset_t active_space;

	/* Amount of data written (archived) to this disk volume */
	offset_t written_space;

	int percent_good;	/* percent good data on disk volume */
	int percent_junk;	/* percent junk data on disk volume */

	active_space = vsn->size;
	written_space = vsn->written;

	Trace(TR_MISC, "[%s] active space: %lld (%s)", vsn->vsn,
	    active_space, StrFromFsize(active_space, 3, NULL, 0));

	Trace(TR_MISC, "[%s] written space: %lld (%s)", vsn->vsn,
	    written_space, StrFromFsize(written_space, 3, NULL, 0));

	if (active_space > written_space) {
		/*
		 * Amount of data for this vsn as accumulated from the
		 * inodes on all the filesystems is greater than the space
		 * written to the media.  Adjust size so we don't generate
		 * incorrect usage percentages (good, junk and free).
		 */
		Trace(TR_MISC,
		    "[%s] size accumulated %lld > space written %lld",
		    vsn->vsn, active_space, written_space);

		active_space = written_space;
	}

	if (written_space > 0) {
		percent_good = (active_space * 100LL) / written_space;
		percent_junk = 100LL - percent_good;
	} else {
		percent_good = 0;
		percent_junk = 0;
	}

	vsn->good = percent_good;
	vsn->junk = percent_junk;

	Trace(TR_MISC, "[%s] good: %d%% junk: %d%%", vsn->vsn,
	    vsn->good, vsn->junk);

	for (no = no_recycle; no; no = no->next) {
		if ((regex(no->regexp, vsn->vsn, NULL) != NULL) &&
		    (no->medium == vsn->media)) {
			vsn->no_recycle = TRUE;
			Trace(TR_MISC, "[%s] no recycle vsn", vsn->vsn);
		}
	}
}


/*
 * Sort VSN table by order that we'd like to recycle.
 *
 * First, all the in-robot VSNs come first,
 *
 * Then, group VSNs by robot.
 *
 * Then, the junkier VSNs should appear before the ones with less
 * junk space (because that's the payback we get for recycling that
 * VSN).  Examine number of bytes of archive copies if junk percentages
 * are equal.  Then, as a final tie-breaker, the VSNs with less good space
 * should appear before the ones with more good space (because such VSNs
 * involve less work to recycle).
 */

void
sort_vsns(void)
{
	int vsn_i, vsn_j;

	Trace(TR_MISC, "Sorted vsn table");

	for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
		for (vsn_j = 0; vsn_j < vsn_i; vsn_j++) {
			VSN_TABLE *VSN_i = &vsn_table[vsn_permute[vsn_i]];
			VSN_TABLE *VSN_j = &vsn_table[vsn_permute[vsn_j]];

			if ((VSN_i->in_robot == TRUE &&
			    VSN_j->in_robot == FALSE) ||
			    (VSN_i->dev < VSN_j->dev) ||
			    (VSN_i->dev == VSN_j->dev &&
			    (VSN_i->junk > VSN_j->junk ||
			    (VSN_i->junk == VSN_j->junk &&
			    (VSN_i->size < VSN_j->size ||
			    (VSN_i->size == VSN_j->size &&
			    (VSN_i->good < VSN_j->good))))))) {

				int vsn_entry;

				vsn_entry = vsn_permute[vsn_i];
				vsn_permute[vsn_i] = vsn_permute[vsn_j];
				vsn_permute[vsn_j] = vsn_entry;
			}
		}
	}

	for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
		VSN_TABLE *vsn;

		vsn = &vsn_table[vsn_permute[vsn_i]];

		Trace(TR_MISC, "[%s] good: %d%% junk: %d%%", vsn->vsn,
		    vsn->good, vsn->junk);
	}

}


/*
 * -----select_rm_candidates
 * Scan the robot table.  For each removable media robot which needs to be
 * recycled, find the best VSN.  Don't recycle VSNs with no junk space.
 * (If we recycle such VSNs, there is a decrease in robot
 * fragmentation, but that process is called "repacking" and we aren't
 * doing that here (yet!).)
 */

static void
select_rm_candidates(void)
{
	long long free_up;		/* number of blocks to be freed up */
	int robot_i, vsn_i;

	FILE *mail_file = NULL;

	for (robot_i = 0; robot_i < ROBOT_count; robot_i++) {
		unsigned long long capacity;
		unsigned long long space;
		long long quantity;
		int vsncount;
		boolean_t limit_quantity;
		boolean_t limit_vsncount;
		boolean_t any;

		ROBOT_TABLE *Robot = &ROBOT_table[robot_i];

		if (Robot->asflags & AS_diskArchSet) {
			continue;
		}

		if (Robot->needs_work == 0) {
			continue;
		}

		capacity = Robot->total_capacity;
		space = Robot->total_space;
		quantity = Robot->dataquantity;
		vsncount = Robot->vsncount;
		limit_quantity = Robot->limit_quantity;
		limit_vsncount = Robot->limit_vsncount;
		any = FALSE;

		if (capacity == 0LL) {
			continue;
		}

		/* create mail temporary file */
		if (Robot->mail) {
			mail_file = Mopen(Robot->mailaddress);
		}

		/*
		 * calculate the number of bytes which need to be freed
		 * up to bring the robot's utilization to just below the
		 * high-water mark.  Don't try to bring it below zero,
		 * though.
		 */
		free_up =
		    (((capacity - space) * 100LL / capacity) -
		    (long)Robot->high + 1) * capacity / 100LL;
		if (free_up > capacity) {
			free_up = capacity;
		}

		if (mail_file) {
			char cap_buf[S2ABUFSIZE], spa_buf[S2ABUFSIZE];
			char *fmt;

			fmt = catgets(catfd, SET, 20248, "Message 20248");
			strcpy(cap_buf, s2a(capacity));
			strcpy(spa_buf, s2a(space));
			fprintf(mail_file, fmt,
			    vendor_id(Robot), family_type(Robot),
			    family_name(Robot), cap_buf, spa_buf,
			    (capacity-space)*100/capacity,
			    Robot->high);
		}

		strFromFreeup(free_up);
		strFromQuantity(quantity, limit_quantity);
		strFromVsncount(vsncount, limit_vsncount);

		cemit(TO_FILE, 0, 20223, family_type(Robot),
		    family_name(Robot), fu_buf, qu_buf, su_buf);

		/*
		 * Total up the "good" space on each VSN marked
		 * "recycle" on * this robot/archive set.  This will be
		 * deducted from the quantity available to be
		 * rearchived.  Also total up the "obsolete" space on
		 * the same VSNs.  This space is deducted from the
		 * "free_up" requirement.  Of course, if a given VSN
		 * cannot be recycled, don't consider its contribution.
		 */

		for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
			VSN_TABLE *VSN;

			VSN = &vsn_table[vsn_permute[vsn_i]];
			if ((VSN->dev == Robot->dev) &&
			    (VSN->is_recycling)) {

				cemit(TO_FILE, 0, 20270, VSN->vsn);

				if ((!VSN->has_request_files) &&
				    (!VSN->no_recycle) &&
				    (!VSN->has_noarchive_files)) {

					cemit(TO_FILE, 0, 20222,
					    VSN->vsn);

					free_up -=
					    (VSN->junk*VSN->capacity)/
					    100L;
					quantity -=
					    (VSN->good*VSN->capacity)/
					    100L;

					vsncount--;
					any = TRUE;
				} else {
					cemit(TO_FILE, 0, 20271,
					    VSN->vsn);
				}
			}
		}

		/*
		 * If we don't have enough previously-recycling
		 * candidates, pick the best ones  (that is, the
		 * first ones) in the vsn list.
		 */
		if (free_up > 0 &&
		    ((!limit_quantity || (quantity > 0)) &&
		    (!limit_vsncount || (vsncount > 0)))) {

			for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
				VSN_TABLE *VSN;

				VSN = &vsn_table[vsn_permute[vsn_i]];

				strFromFreeup(free_up);
				strFromQuantity(quantity, limit_quantity);
				strFromVsncount(vsncount, limit_vsncount);

				cemit(TO_FILE, 0, 20224, VSN->vsn,
				    fu_buf, qu_buf, su_buf);

				/*
				 * Test each of the disqualifying
				 * conditions against this VSN
				 */

				/*
				 * Disqualify if VSN not in correct
				 * library.
				 */
				if (VSN->dev != Robot->dev) {
					/* VSN not in %s %s */
					cemit(TO_FILE, 0, 20232,
					    family_type(Robot),
					    family_name(Robot));
					continue;
				} else {
					/* VSN is in %s %s... good */
					cemit(TO_FILE, 0, 20227,
					    family_type(Robot),
					    family_name(Robot));
				}

				/*
				 * Disqualify if VSN already recycling.
				 */
				if (VSN->is_recycling) {
					cemit(TO_FILE, 0, 20286);
					continue;
				} else {
					cemit(TO_FILE, 0, 20287);
				}

				/*
				 * Disqualify if VSN contains
				 * request files.
				 */
				if (VSN->has_request_files) {
					cemit(TO_FILE, 0, 20225);
					continue;
				} else {
					cemit(TO_FILE, 0, 20226);
				}

				/*
				 * Disqualify if VSN contains
				 * 'archive -n' files.
				 */
				if (VSN->has_noarchive_files) {
					cemit(TO_FILE, 0, 20291);
					continue;
				} else {
					cemit(TO_FILE, 0, 20292);
				}

				/*
				 * Disqualify if 'no_recycle' set
				 * for VSN in recycler.cmd.
				 */
				if (VSN->no_recycle) {
					cemit(TO_FILE, 0, 20266);
					continue;
				} else {
					cemit(TO_FILE, 0, 20267);
				}

				/*
				 * Disqualify if VSN count limit
				 * exceeded.
				 */
				if (limit_vsncount && vsncount <= 0) {
					cemit(TO_FILE, 0, 20230, "VSN count");
					continue;
				}

				/*
				 * Disqualify if data quantity limit
				 * exceeded.
				 */
				if (limit_quantity &&
				    (VSN->good * VSN->capacity) / 100LL >
				    quantity) {

					cemit(TO_FILE, 0, 20230,
					    "data quantity");
					Trace(TR_MISC,
					    "'%s' data quantity limit "
					    "exceeded: %d%% %lld %lld",
					    VSN->vsn, VSN->good,
					    VSN->capacity, quantity);
					continue;
				}

				/*
				 * Disqualify if minimum gain
				 * requirement not met.
				 */
				if (VSN->junk <= getMinGain(Robot, VSN)) {
					cemit(TO_FILE, 0, 20231,
					    VSN->junk, getMinGain(Robot, VSN));
					Trace(TR_MISC,
					    "'%s' doesn't meet min-gain: "
					    "junk: %d%% min: %d%%",
					    VSN->vsn, VSN->junk,
					    getMinGain(Robot, VSN));
					continue;
				}

				/*
				 * Found recycling candidate.  If recycling is
				 * not ignored on this robot mark the VSN for
				 * recycling.
				 */
				Trace(TR_MISC, "'%s' needs recycling",
				    VSN->vsn);

				if (Robot->ignore == FALSE) {
					/*
					 * Adjust recycling parameters.
					 */
					free_up -=
					    (VSN->junk * VSN->capacity) / 100LL;
					quantity -=
					    (VSN->good * VSN->capacity) / 100LL;
					vsncount--;

					/* mark the candidate "recycling" */
					VSN->needs_recycling = TRUE;
					cemit(TO_FILE, 0, 20229);

					select_this_candidate(VSN, Robot,
					    mail_file);
					any = TRUE;

					/*
					 * if we've done enough (for now),
					 * stop selecting VSNs
					 */
					if (free_up < 0 ||
					    (limit_quantity &&
					    (quantity < 0)) ||
					    (limit_vsncount &&
					    (vsncount <= 0)))
						break;

				} else {
					/*
					 * Recycling is ignored on this library.
					 * VSN is not marked for recycling.
					 */
					cemit(TO_FILE, 0, 20233);
				}
			}
		} else {
			/*
			 * No need to select additional candidates.
			 */
			strFromFreeup(free_up);
			strFromQuantity(quantity, limit_quantity);
			strFromVsncount(vsncount, limit_vsncount);

			cemit(TO_FILE, 0, 20263, fu_buf, qu_buf, su_buf);
		}

		if (any == FALSE) {		/* no candidate found */
			cemit(TO_FILE, 0, 20234);
			if (mail_file) {
				char *fmt;
				fmt = catgets(catfd, SET, 20254,
				    "Message 20254");
				fprintf(mail_file, fmt, family_type(Robot));
			}
		}

		if (mail_file) {
			char buf[512];
			char *fmt;

			fmt = catgets(catfd, SET, 20255, "Message 20255");
			sprintf(buf, fmt, family_type(Robot),
			    family_name(Robot));
			Msend(&mail_file, Robot->mailaddress, buf);
		}
	}  /* end for loop over all robots */
}


/*
 * Scan the robot table for disk archive sets.  For each disk archive set
 * find the best VSN.
 */
static void
select_dk_candidates(void)
{
	int robot_idx;
	int vsn_idx;
	ROBOT_TABLE *robot_entry;
	VSN_TABLE *vsn_entry;
	FILE *mail_file;


	for (robot_idx = 0; robot_idx < ROBOT_count; robot_idx++) {
		boolean_t any;

		robot_entry = &ROBOT_table[robot_idx];

		if ((robot_entry->asflags & AS_diskArchSet) == 0) {
			continue;
		}

		if (robot_entry->needs_work == 0) {
			continue;
		}

		any = B_FALSE;
		mail_file = NULL;

		if (robot_entry->mail) {
			mail_file = Mopen(robot_entry->mailaddress);
		}

		/* Need to select recycling candidate for %s %s. */
		cemit(TO_FILE, 0, 20328, family_type(robot_entry),
		    family_name(robot_entry));

		for (vsn_idx = 0; vsn_idx < table_used; vsn_idx++) {
			vsn_entry = &vsn_table[vsn_permute[vsn_idx]];

			/*
			 * Disqualify if VSN not in this archive set.
			 */
			if (vsn_entry->robot != robot_entry) {
				/* VSN not in %s %s. */
				continue;
			} else {
				/* Checking %s. */
				cemit(TO_FILE, 0, 20329, vsn_entry->vsn);
				/* VSN is in %s %s... good. */
			}

			/*
			 * Test disqualifying conditions against this VSN.
			 */

			/*
			 * Disqualify if VSN contains request files.
			 */
			if (vsn_entry->has_request_files) {
				/* VSN has request files... skipping. */
				cemit(TO_FILE, 0, 20225);
				continue;
			} else {
				/* VSN has no request files... good. */
				cemit(TO_FILE, 0, 20226);
			}

			/*
			 * Disqualify if VSN contains 'archive -n' files.
			 */
			if (vsn_entry->has_noarchive_files) {
				/* VSN has files marked 'archive -n'... skip */
				cemit(TO_FILE, 0, 20291);
				continue;
			} else {
				/* VSN has no 'archive -n' files... good. */
				cemit(TO_FILE, 0, 20292);
			}

			/*
			 * Disquality if 'no_recycle' set for VSN
			 * in recycler.cmd.
			 */
			if (vsn_entry->no_recycle) {
				/*
				 * VSN was specified as "no_recycle" in
				 * recycler.cmd file... skipping.
				 */
				cemit(TO_FILE, 0, 20266);
				continue;
			} else {
				/*
				 * VSN was not specified as "no_recycle" in
				 * recycler.cmd file... good.
				 */
				cemit(TO_FILE, 0, 20267);
			}

			/*
			 * Disqualify if minimum gain requirement not met.
			 */
			if (vsn_entry->junk <=
			    getMinGain(robot_entry, vsn_entry)) {
				/* VSN doesn't meet min-gain... skipping */
				cemit(TO_FILE, 0, 20231, vsn_entry->junk,
				    getMinGain(robot_entry, vsn_entry));
			} else {
				cemit(TO_FILE, 0, 20228, vsn_entry->junk,
				    getMinGain(robot_entry, vsn_entry));
			}

			/*
			 * Found recycling candidate.  If recycling is not
			 * ignored on this archive set mark the VSN for
			 * recycling.
			 */

			if (robot_entry->ignore == FALSE) {
				/*
				 * Mark VSN for recycling.
				 */
				vsn_entry->needs_recycling = TRUE;
				cemit(TO_FILE, 0, 20236);

				select_this_candidate(vsn_entry, robot_entry,
				    mail_file);
				any = TRUE;

			} else {
				/*
				 * Recycling is ignored on this archive set.
				 * VSN is not marked for recycling.  Set flag
				 * to indicate this VSN would be a candidate
				 * for recycling if ignore was not set on
				 * this archive set.
				 */
				cemit(TO_FILE, 0, 20338);
				vsn_entry->candidate = TRUE;
				/*
				 * Set flag so we log remove actions as if
				 * recycling is occurring.
				 */
				vsn_entry->log_action = TRUE;
			}
		}

		if (any == B_FALSE) {
			cemit(TO_FILE, 0, 20320);
			if (mail_file != NULL) {
				char *fmt;

				fmt = catgets(catfd, SET, 20254,
				    "Message 20254");
				fprintf(mail_file, fmt,
				    family_type(robot_entry));
			}
		}

		if (mail_file != NULL) {
			char buf[512];
			char *fmt;

			fmt = catgets(catfd, SET, 20255, "Message 20255");
			sprintf(buf, fmt, family_type(robot_entry),
			    family_name(robot_entry));
			Msend(&mail_file, robot_entry->mailaddress, buf);
		}

	}
}


/*
 * -----mark_filesystem
 * Scan the filesystem, checking all the inodes to see if they have
 * archive copies which reside VSNs which are to be recycled.
 * .inodes file second pass.
 */
void
mark_filesystem(
	struct sam_fs_info *fs_params)
{
	char buf[PATH_MAX];
	int fd, ngot, ninodes, inode_i;
	int expected_ino = 1;

	Trace(TR_MISC, "Mark filesystem: '%s'", fs_params->fi_mnt_point);

	sync_and_sleep();
	if ((fd = OpenInodesFile(fs_params->fi_mnt_point)) < 0) {
		emit(TO_ALL, LOG_ERR, 20262, fs_params->fi_name, errtext);
		cannot_recycle = TRUE;
		return;
	}

	while ((ngot = read(fd, &inodes, INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {
		ninodes = ngot / sizeof (union sam_di_ino);
		for (inode_i = 0; inode_i < ninodes; inode_i++) {
			if (inodes[inode_i].inode.di.id.ino == expected_ino) {
				handle_inode(fs_params->fi_mnt_point,
				    &inodes[inode_i], &check_inode);
			}
			expected_ino ++;
		}
	}
	if (ngot != 0) {
		sprintf(buf, "%s/.inodes", fs_params->fi_mnt_point);
		emit(TO_ALL, LOG_ERR, 1063, buf, errtext);
		cannot_recycle = TRUE;
	}
	(void) close(fd);
}


/*
 * -----check_inode - check inode for recycling archive copies.
 * Given a VSN for a section of an archive copy of an inode, if the
 * VSN is recycling, mark the copy as needing to be recycled.
 * Done during the second scan of the .inodes file.
 */
static void
check_inode(
	char *fs_name,
	union sam_di_ino *inode,
	int copy,
	/* LINTED argument unused in function */
	long long length,
	VSN_TABLE *vsn)
{
	if ((IS_DISK_MEDIA(vsn->media) == FALSE) &&
	    vsn->label_time <= inode->inode.ar.image[copy].creation_time) {

		vsn->has_active_files = TRUE;
		vsn->active_files++;	/* count VSNs active files */

		if (vsn->needs_recycling ||
		    vsn->is_recycling) {  /* if VSN is recycling, mark copy */
			recycle_copy(fs_name, inode, copy);

			if (display_draining_vsns) {
				char *pathname;

				pathname = id_to_path(fs_name,
				    inode->inode.di.id);
				emit(TO_FILE, 0, 20235, copy+1, pathname,
				    vsn->vsn);
			}
		}

	}
}


/*
 * -----recycle_copy
 * recycle a copy.  Make an ioctl to mark the copy as
 * needing to be recycled.  The archiver will see this flag and
 * rearchive the copy.
 */
static void
recycle_copy(
	char *fs_name,
	union sam_di_ino *inode,
	int c)
{
	struct sam_ioctl_idscf arg;

	if (cannot_recycle) {
		return;
	}

	arg.id = inode->inode.di.id;
	arg.copy = c;
	arg.c_flags = AR_rearch;
	arg.flags = AR_rearch;

	/*
	 * Issue F_IDSCF (Set copy flags by ID) ioctl call
	 */

	if (ioctl(SamFd, F_IDSCF, &arg) < 0) {
		int savee = errno;
		char *pathname;

		pathname = id_to_path(fs_name, inode->inode.di.id);
		errno = savee;
		emit(TO_ALL, LOG_ERR, 594, pathname, errtext);
	}
}


/*
 * -----check_relabel
 * Find any VSNs which are marked for recycling and have no active
 * files on them.  These may be post-processed.  This function
 * also prints out the "action log."
 */
static void
check_relabel(void)
{
	int vsn_i, stat;
	char *recycler_script;
	FILE *mail_file = NULL;
	FILE *fptr;

	if (cannot_recycle) {
		return;
	}

	emit(TO_FILE, 0, 20259);

	for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
		VSN_TABLE *VSN = &vsn_table[vsn_i];
		/* if no action on the VSN, skip it */
		if (VSN->needs_recycling == FALSE &&
		    VSN->is_recycling == FALSE) {
			continue;
		}

		/* if it's third-party media, skip it */
		if ((VSN->media & DT_CLASS_MASK) == DT_THIRD_PARTY) {
			continue;
		}

		/* if it's disk media, skip it */
		if (IS_DISK_MEDIA(VSN->media)) {
			continue;
		}

		/* check for any active files left */
		if (VSN->has_active_files == FALSE) {
			char medium[512];
			char buf[512];
			vsn_t vsn;
			char *modifier;

			strcpy(medium, "od");
			if ((VSN->media & DT_CLASS_MASK) == DT_TAPE) {
				strcpy(medium, "tp");
			}
			memcpy(vsn, VSN->vsn, sizeof (vsn_t));
			if (is_optical(VSN->media)) {
				if (VSN->ce->CePart == 1) {
					modifier = "1";
				} else {
					modifier = "2";
				}
			} else {
				modifier = "0";
			}

			recycler_script = GetScript();
			ASSERT(recycler_script != NULL);

			sprintf(buf, "%s %s %s %d %d %s %s %s",
			    recycler_script, medium, vsn,
			    VSN->ce->CeSlot, VSN->real_dev->eq,
			    device_to_nm(VSN->media),
			    family_name(VSN->robot), modifier);

			(void) fflush(NULL);    /* flush all writable FILES */

			stat = -1;
			if (((fptr = popen(buf, "w")) != NULL) &&
			    ((stat = pclose(fptr)) == 0)) {
				emit(TO_ALL, LOG_INFO, 1460, buf);
			} else {
				emit(TO_ALL, LOG_ERR, 20246, buf, errtext);
			}

			if (VSN->robot && VSN->robot->mail) {
				char *fmt;
				char *sub;
				char *mailaddress;

				mailaddress = VSN->robot->mailaddress;
				sub = catgets(catfd, SET, 20257,
				    "Message 20257");
				if ((mail_file = Mopen(mailaddress)) != NULL) {
					if (stat != 0) {
						fmt = catgets(catfd, SET, 20298,
						    "Message 20298");
						fprintf(mail_file, fmt, buf);
					} else {
						char lbuff[24];
						fmt = catgets(catfd, SET, 20256,
						    "Message 20256");
						if (is_optical(VSN->media)) {
							sprintf(lbuff, "%d:%s",
							    VSN->ce->CeSlot,
							    modifier);
						} else {
							sprintf(lbuff, "%d",
							    VSN->ce->CeSlot);
						}
						fprintf(mail_file, fmt,
						    VSN->vsn, lbuff,
						    family_name(VSN->robot));
					}
					Msend(&mail_file, mailaddress, sub);
				}
			}

			/*
			 * Labeling media clears "recycle" flag so that VSN can
			 * be reused
			 */

		} else {    /* waiting for VSN to drain */

			emit(TO_ALL, LOG_INFO, 20280,
			    device_to_nm(VSN->media), VSN->vsn,
			    VSN->active_files);

		}  /* if active files left on VSN */
	}	   /* for each VSN */
}

/*
 * Display the removable media in VSN table.  This routine assumes that
 * the VSN table is sorted by the VSN->dev field.
 */
static void
display_rm_vsns(void)
{
	int i;
	dev_ent_t *olddev = (dev_ent_t *)-1;
	int total_vsns;

	total_vsns = 0;
	for (i = 0; i < table_used; i++) {
		VSN_TABLE *VSN = &vsn_table[vsn_permute[i]];

		if (IS_DISK_MEDIA(VSN->media) == TRUE) {
			continue;
		}
		total_vsns++;
	}

	emit(TO_FILE, 0, 20237, total_vsns);

	if (suppress_empty_vsn) {
		emit(TO_FILE, 0, 20293);
	}

	for (i = 0; i < table_used; i++) {
		VSN_TABLE *VSN = &vsn_table[vsn_permute[i]];
		char countbuf[S2ABUFSIZE];
		char sizebuf[S2ABUFSIZE];
		char *tag1;

		if (IS_DISK_MEDIA(VSN->media) == TRUE) {
			continue;
		}

		if (suppress_empty_vsn && (VSN->count == 0) &&
		    (VSN->size == 0) && (VSN->good == 0) && (VSN->junk == 0)) {
			continue;
		}

		strcpy(countbuf, s2a(VSN->count));
		strcpy(sizebuf, s2a(VSN->size));

		if (VSN->is_recycling)
			tag1 = "old candidate   ";
		else if (VSN->needs_recycling)
			tag1 = "new candidate   ";
		else if (VSN->no_recycle)
			tag1 = "no_recycle VSN  ";
		else if (VSN->size == 0 && VSN->count == 0) {
			if (VSN->junk == 0)
				tag1 = "empty VSN       ";
			else
				tag1 = "no-data VSN     ";
		} else if (VSN->free != 0)
			tag1 = "partially full  ";
		else if (VSN->dev == 0)
			tag1 = "shelved VSN     ";
		else if (VSN->free == 0)
			tag1 = "full VSN        ";

		if (VSN->multiple_arsets)
			tag1 = "in multiple sets";
		if (VSN->has_noarchive_files)
			tag1 = "archive -n files";
		if (VSN->has_request_files)
			tag1 = "request files   ";

		/*
		 * If the VSN->dev field of the current VSN is different than
		 * the previous vsn, then we need to emit a new header.
		 */
		if (olddev != VSN->dev) {
			char *archive_set_name;

			if (VSN->in_robot) {
				if (VSN->was_reassigned) {
					archive_set_name =
					    ROBOT_table[(int)(VSN->dev)].name;
				} else {
					archive_set_name =
					    family_name(VSN->robot);
				}
			} else {
				archive_set_name = "";
			}

			emit(TO_FILE, 0, 20238, archive_set_name);
			emit(TO_FILE, 0, 20239);
			olddev = VSN->dev;
		}

		/*
		 * Display the VSN information.  We use different formats if
		 * it's in a robot or not - we don't have the catalog info such
		 * as capacity and space if it's not in a robot.
		 */
		if (VSN->dev || IS_DISK_MEDIA(VSN->media)) {
			char *fname;

			/*
			 * In a robot or a disk archive set.
			 */
			if (IS_DISK_MEDIA(VSN->media)) {
				fname = catgets(catfd, SET, 20300,
				    "Message 20300");
			} else {
				fname = family_name(VSN->robot);
			}

			emit(TO_FILE, 0, 20240, tag1, countbuf, sizebuf,
			    VSN->good, VSN->junk, VSN->free,
			    fname, device_to_nm(VSN->media), VSN->vsn);
			if (show_extrapolated_capacity) {
				emit(TO_FILE, 0, 20241,
				    VSN->capacity / VSN->alpha);
			}
		} else {		/* else it's not in any robot */
			emit(TO_FILE, 0, 20242, tag1, countbuf, sizebuf,
			    device_to_nm(VSN->media), VSN->vsn);
		}
	}
}

/*
 * Display the disk media in VSN table.
 */
static void
display_dk_vsns(void)
{
	int i;
	char *tag;
	VSN_TABLE *vsn_entry;
	char countbuf[S2ABUFSIZE];
	char sizebuf[S2ABUFSIZE];
	char *archive_set_name;
	int total_vsns;

	total_vsns = 0;
	for (i = 0; i < table_used; i++) {
		VSN_TABLE *vsn_entry = &vsn_table[vsn_permute[i]];

		if (IS_DISK_MEDIA(vsn_entry->media) == FALSE) {
			continue;
		}
		total_vsns++;
	}

	emit(TO_FILE, 0, 20348, total_vsns);

	if (suppress_empty_vsn) {
		emit(TO_FILE, 0, 20293);
	}

	emit(TO_FILE, 0, 20238, "Disk Volumes");
	emit(TO_FILE, 0, 20350);

	for (i = 0; i < table_used; i++) {
		vsn_entry = &vsn_table[vsn_permute[i]];

		if (IS_DISK_MEDIA(vsn_entry->media) == FALSE) {
			continue;
		}

		if (suppress_empty_vsn) {
			if ((vsn_entry->count == 0) &&
			    (vsn_entry->size == 0) &&
			    (vsn_entry->good  == 0) &&
			    (vsn_entry->junk == 0)) {
				continue;
			}
		}

		strcpy(countbuf, s2a(vsn_entry->count));
		strcpy(sizebuf,  s2a(vsn_entry->size));

		if (vsn_entry->needs_recycling) {
			tag = catgets(catfd, SET, 20340, "recycle candidate");

		} else if (vsn_entry->no_recycle) {
			tag = catgets(catfd, SET, 20341, "no recycle");

		} else if (vsn_entry->size == 0 && vsn_entry->count == 0) {

			if (vsn_entry->junk == 0) {
				tag = catgets(catfd, SET, 20342, "empty VSN");
			} else {
				tag = catgets(catfd, SET, 20343, "no-data VSN");
			}

		} else if (vsn_entry->candidate) {
			tag = catgets(catfd, SET, 20347, "ignored candidate");

		} else {
			tag = catgets(catfd, SET, 20349, "not a candidate");
		}


		if (vsn_entry->has_noarchive_files) {
			tag = catgets(catfd, SET, 20345, "archive -n files");
		}

		if (vsn_entry->has_request_files) {
			tag = catgets(catfd, SET, 20346, "request files");
		}

		if (vsn_entry->disk_archsets > 1) {
			archive_set_name = catgets(catfd, SET, 20344,
			    "in multiple sets");
		} else {
			archive_set_name = family_name(vsn_entry->robot);
		}

		emit(TO_FILE, 0, 20351, tag, countbuf, sizebuf,
		    vsn_entry->good, vsn_entry->junk,
		    vsn_entry->vsn, archive_set_name);

	}
}


/*
 *  Emit a header to the log indicating the time the recycler begins
 *  running.
 */
static void
log_header(void)
{
	time_t clock;
	char *ascii;
	char ctime_buf[512];

	clock = time(NULL);
	if (log == NULL)
		return;

	ascii = ctime_r(&clock, ctime_buf, sizeof (ctime_buf));
	*(ascii+strlen(ascii)-1) = '\0';

	fprintf(log, "\n========== Recycler begins at %s ===========\n", ascii);
}

/*
 * Emit a trailer to the log indicating the time the recycler quits
 * running.
 */
static void
log_trailer(void)
{
	time_t clock;
	char *ascii;
	char ctime_buf[512];

	clock = time(NULL);
	if (log == NULL)
		return;

	ascii = ctime_r(&clock, ctime_buf, sizeof (ctime_buf));
	*(ascii+strlen(ascii)-1) = '\0';

	fprintf(log, "\n========== Recycler ends at %s ===========\n\n", ascii);
}

/*  issue an API call to set the indicated vsn's recycling bit to "flag" */
static void
flag_recycle(
	VSN_TABLE *table_entry)
{
	char *media_type_ascii;

	media_type_ascii = device_to_nm(table_entry->media);

	if (IS_DISK_MEDIA(table_entry->media) == FALSE) {
		(void) sam_chmed(table_entry->real_dev->eq,
		    table_entry->ce->CeSlot, table_entry->ce->CePart,
		    media_type_ascii, table_entry->vsn, CMD_CATALOG_RECYCLE,
		    1, 0);
	}
}

static char *mail_name;  /* name of file into which mail is being placed */

/*
 * Mopen - open a file which will be mailed to a user
 * destination - the mail address string.  Used here only to report failures.
 */

FILE *
Mopen(char *destination)
{
	FILE *mail_file;

	char *fmt;
	time_t clock = time(NULL);
	char current_time[512];
	char ctime_buf[512];

	fmt = catgets(catfd, SET, 20290, "Message 20290");
	mail_name = tmpnam(NULL);  /* tmpnam returns static area */
	if ((mail_file = fopen(mail_name, "w+")) == NULL) {
		emit(TO_ALL, LOG_INFO, 20245, mail_name, destination, errtext);
	}
	strcpy(current_time, ctime_r(&clock, ctime_buf, sizeof (ctime_buf)));
	current_time[strlen(current_time)-1] = '\0';
	fprintf(mail_file, fmt, current_time);

	return (mail_file);
}

/*
 * Msend - close mail file, send mail
 *
 * mail_file - pointer to FILE * where the mail file is opened (as returned
 *     by Mopen)
 * destination - mail address string
 * subject - mail subject string
 */
void
Msend(FILE **mail_file, char *destination, char *subject)
{
	char buf[512];
	FILE *fptr;

	(void) fclose(*mail_file);
	*mail_file = NULL;

	sprintf(buf, "/bin/mailx -s \"%s\" %s < %s\n",
	    subject, destination, mail_name);

	if (((fptr = popen(buf, "w")) == NULL) || (pclose(fptr) != 0)) {
		emit(TO_ALL, LOG_ERR, 20246, buf, errtext);
	}

	(void) unlink(mail_name);
}

/*
 *  Kludge function to force the .inodes file to be a little more
 *  up-to-date when we start looking at it.  Not 100% effective, but a
 *  little better than doing nothing at all.
 */
void
sync_and_sleep()
{
#ifdef DEBUGxtra
	printf("About to sync() and sleep(5);\n");
#endif
	sync();
	sleep(5);
}

/*
 * ----detect_another_recycler
 *  Detect if another recycler is running and exit if it is.  We cannot
 *  have two recyclers running for a number of reasons, one of which
 *  is that there's only one recycler <-> archiver communication path.
 */
void
detect_another_recycler(void)
{
	char pid_string[MAXPATHLEN];
	int fd;
	sam_defaults_t *defaults;

	defaults = GetDefaults();
	if (defaults == NULL || (defaults->flags & DF_NEW_RECYCLER) != 0) {
		emit(TO_ALL, LOG_ERR, 20353);
		exit(1);
	}

	if ((fd = open(RECYCLER_LOCK_FILE, O_CREAT|O_RDWR, 0644)) < 0) {
		return; /* Cannot create lock file - assume all's well */
	}

	if (lockf(fd, F_TLOCK, 0)) {
		if (errno == EAGAIN) {
			(void) close(fd);
#ifdef DEBUG
			emit(TO_ALL, LOG_ERR, 20265);
#endif
			/* File already locked - assume another's running */
			exit(1);
		}
		return;    /* Some obscure error - assume all's well */
	}

	(void) ftruncate(fd, (off_t)0);
	sprintf(pid_string, "%ld\n", getpid());
	(void) write(fd, pid_string, strlen(pid_string));
	/* We've got the file locked - we KNOW all's well */
}

/*
 *  Functions to get device table fields with some error checking and
 *  special-casing for historian, and "archset" robots.
 */
char *
family_name(
	ROBOT_TABLE *robot)
{
	char *notFound = "(NONE)";

	if (robot == NULL) {
		return (notFound);
	}
	if (robot->archset) {
		return (robot->name);
	}
	if (robot->dev) {
		if ((robot->dev->set != 0) && *(robot->dev->set) != 0) {
			return (robot->dev->set);
		}
		if (robot->dev->equ_type == DT_HISTORIAN) {
			return ("hy");
		}
	}
	return (notFound);
}

static char *
family_type(
	ROBOT_TABLE *robot)
{
	char *type = "NONE";
	if (robot) {
		if (robot->archset) {
			type = "Archive Set";
		} else {
			type = "Library";
		}
	}
	return (type);
}

char *
vendor_id(
	ROBOT_TABLE *robot)
{
	if (!robot) {
		return ("(NULL)");
	}
	if (robot->archset) {
		return ("SAM-FS");
	}
	if ((robot->dev->vendor_id != 0) && *(robot->dev->vendor_id) != 0) {
		/* vendor_id is unsigned! */
		return ((char *)robot->dev->vendor_id);
	}
	if (robot->dev->equ_type == DT_HISTORIAN) {
		return ("SAM-FS");
	}
	return ("(NULL)");
}

char *
product_id(
	ROBOT_TABLE *robot)
{
	if (!robot) {
		return ("(NULL)");
	}
	if (robot->archset) {
		return ("SAM-FS");
	}
	if ((robot->dev->product_id != 0) && *(robot->dev->product_id) != 0) {
		/* product_id is unsigned! */
		return ((char *)robot->dev->product_id);
	}
	if (robot->dev->equ_type == DT_HISTORIAN) {
		return ("Historian");
	}
	return ("(NULL)");
}

char *
catalog_name(
	dev_ent_t *dev)
{
	if (!dev) {
		return ("(No catalog)");
	}

	if ((dev->dt.rb.name != 0) && *dev->dt.rb.name != 0)
		return ((char *)dev->dt.rb.name);

	return ("(NULL)");
}


/*
 * For each archive set, process the archive set's regular-expression
 * selected VSNs.  For each archive set which has recycling selected,
 * inspect the set's VSN descriptor.  This descriptor has several variants.
 * The only ones in which we are interested are the expression descriptor
 * and the list descriptor.  The list descriptor is used if there is more
 * than one regular expression associated with the set.  The list
 * descriptor contains a variable number of VSN descriptors.  The
 * expression descriptor gives an offset into the VsnExprList.  The
 * VsnExprList contains the information needed to match the
 * corresponding regular expression.
 *
 * So, what we've got is either a simple regular expression or a list of
 * regular exporessions for each archive set. Against each of these regular
 * expressions, the entire set of VSNs is matched.  Any VSNs which match
 * the regular expression will be assigned to the archive set's catalog.
 *
 * Note that we've already read the reserved VSNs from the archiver's Reserved
 * file, so that any VSNs which are assigned to archive sets in this way have
 * already been handled.
 */
static void
assign_vsns(void)
{
	int archive_set;

	for (archive_set = 0; archive_set < afh->ArchSetNumof; archive_set++) {
		struct ArchSet *as = &ArchSetTable[archive_set];
		int archset_robot_index;

		if (as->AsRyFlags == 0) {
			continue;  /* skip set if not recycling */
		}

		archset_robot_index = create_robot(archive_set);

		switch (as->AsVsnDesc & VD_mask) {
		case VD_none:
			goto next_ars;

		case VD_list: {
			struct VsnList *vl;
			vsndesc_t *vd;
			int vdnumof;

			vl = (struct VsnList *)(void *)((char *)VsnListTable
			    + (as->AsVsnDesc & ~VD_mask));

			vd = (vsndesc_t *)&(vl->VlDesc[0]);
			vdnumof	= vl->VlNumof;

			while (vdnumof > 0) {
				switch ((*vd) & VD_mask) {
				case VD_none:
					goto next_ars;
				case VD_list:
					goto next_ars;
				case VD_exp:
					process_desc((*vd), as,
					    archset_robot_index);
					break;
				case VD_pool:
					process_pool((*vd), as,
					    archset_robot_index);
					break;
				}
				vdnumof--;
				vd++;
			}

			break;
			}

		case VD_exp:
			process_desc(as->AsVsnDesc, as, archset_robot_index);
			break;

		case VD_pool:
			process_pool(as->AsVsnDesc, as, archset_robot_index);
			break;

		}

next_ars:;   /* come here to process the next archive set */
	}
}



/*
 * Return pointer to 3.1m size conversion.
 * Returns start of the string.
 */
char *
s2a(
	unsigned long long v)	/* Value to convert. */
{
#define	EXA  (1024LL * 1024 * 1024 * 1024 * 1024 * 1024)
#define	PETA (1024LL * 1024 * 1024 * 1024 * 1024)
#define	TERA (1024LL * 1024 * 1024 * 1024)
#define	GIGA (1024LL * 1024 * 1024)
#define	MEGA (1024LL * 1024)
#define	KILO (1024LL)

	static char buf[32];
	char *p;
	int NumofDigits;

	p = buf + sizeof (buf) - 1;
	*p-- = '\0';
	NumofDigits = 0;
	if (v >= EXA) {
		*p-- = 'E';
		v = (v + (5 * EXA/100)) / (EXA / 10);
	} else if (v >= PETA) {
		*p-- = 'P';
		v = (v + (5 * PETA/100)) / (PETA / 10);
	} else if (v >= TERA) {
		*p-- = 'T';
		v = (v + (5 * TERA/100)) / (TERA / 10);
	} else if (v >= GIGA) {
		*p-- = 'G';
		v = (v + (5 * GIGA/100)) / (GIGA / 10);
	} else if (v >= MEGA) {
		*p-- = 'M';
		v = (v + (5 * MEGA/100)) / (MEGA / 10);
	} else if (v >= KILO) {
		*p-- = 'k';
		v = (v + (5 * KILO/100)) / (KILO / 10);
	} else {
		*p-- = ' ';
		*p-- = ' ';
		*p-- = ' ';
		NumofDigits = 2;
	}

	/* Generate digits in reverse order. */
	do {
		if (NumofDigits++ == 1) {
			*p-- = '.';
		}
		*p-- = v % 10 + '0';
	} while ((v /= 10) > 0);
	return (p+1);
}

/*
 * Handle the vsn descriptor "vd", assigning any matching VSNs to the
 * given archset_robot.
 */
static void
process_desc(
	vsndesc_t vd,
	struct ArchSet *as,
	int archset_robot_index)
{
	int vsn_i;

	if (as->AsFlags & AS_diskArchSet) {
		process_dk_desc(vd, sam_atomedia(as->AsMtype),
		    archset_robot_index);

		} else {

		if ((vd & ~VD_mask) == 0) {
			for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
				VSN_TABLE *VSN = &vsn_table[vsn_i];

				if (VSN->media == sam_atomedia(as->AsMtype)) {
					assign_vsn(archset_robot_index, VSN);
				}
			}
		} else {
			for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
				VSN_TABLE *VSN;
				struct VsnExp *ve;

				VSN = &vsn_table[vsn_i];
				ve = (struct VsnExp *)(void *)
				    ((char *)VsnExpTable + (vd & ~VD_mask));

				if (VSN->media == sam_atomedia(as->AsMtype)) {
					if (regex(ve->VeExpbuf,
					    VSN->vsn, NULL)) {
						assign_vsn(archset_robot_index,
						    VSN);
					}
				}
			}
		}
	}
}


/*
 * Process the vsn descriptor "vd".  Assign matching disk volumes
 * to the archset_robot.
 */
static void
process_dk_desc(
	vsndesc_t vd,
	int media_type,
	int archset_robot_index)
{
	DiskVolsDictionary_t *diskvols;
	char *volName;
	DiskVolumeInfo_t *dv;
	VSN_TABLE *vsn;

	diskvols = DiskVolsGetHandle(DISKVOLS_VSN_DICT);
	if (diskvols == NULL) {
		return;
	}

	(void) diskvols->BeginIterator(diskvols);

	if ((vd & ~VD_mask) == 0) {

		while (diskvols->GetIterator(diskvols, &volName, &dv) == 0) {
			vsn = Find_VSN(media_type, volName);
			AssignDiskVol(archset_robot_index, vsn);
		}

	} else {

		while (diskvols->GetIterator(diskvols, &volName, &dv) == 0) {
			struct VsnExp *ve;

			ve = (struct VsnExp *)(void *)
			    ((char *)VsnExpTable + (vd & ~VD_mask));

			if (regex(ve->VeExpbuf, volName, NULL)) {
				vsn = Find_VSN(media_type, volName);
				AssignDiskVol(archset_robot_index, vsn);
			}
		}
	}

	(void) diskvols->EndIterator(diskvols);
}

/*
 *	Handle the vsn pool "vd", assigning any matching VSNs to the
 *	given archset_robot.
 */
static void
process_pool(
	vsndesc_t vd,
	struct ArchSet *as,
	int archset_robot_index)
{
	int vpi;
	int vsn_i;
	struct VsnPool *vp;

	vp = (struct VsnPool *)(void *)((char *)VsnPoolTable + (vd & ~VD_mask));

	if (as->AsFlags & AS_disk_archive) {
		process_dk_pool(vp, sam_atomedia(as->AsMtype),
		    archset_robot_index);
	} else {

		for (vpi = 0; vpi < vp->VpNumof; vpi++) {
			struct VsnExp *ve;

			ve = (struct VsnExp *)(void *)
			    ((char *)VsnExpTable + vp->VpVsnExp[vpi]);

			for (vsn_i = 0; vsn_i < table_used; vsn_i++) {
				VSN_TABLE *VSN = &vsn_table[vsn_i];

				if (VSN->media == sam_atomedia(as->AsMtype)) {
					if (regex(ve->VeExpbuf,
					    VSN->vsn, NULL)) {
						assign_vsn(archset_robot_index,
						    VSN);
					}
				}
			}
		}
	}
}


/*
 * Process the vsn pool "vp".  Assign matching disk volumes
 * to the archset_robot.
 */
static void
process_dk_pool(
	struct VsnPool *vp,
	int media_type,
	int archset_robot_index)
{
	int i;
	DiskVolsDictionary_t *diskvols;

	diskvols = DiskVolsGetHandle(DISKVOLS_VSN_DICT);
	if (diskvols == NULL) {
		return;
	}

	for (i = 0; i < vp->VpNumof; i++) {
		struct VsnExp *ve;
		VSN_TABLE *vsn;
		char *volName;
		DiskVolumeInfo_t *dv;

		ve = (struct VsnExp *)(void *)
		    ((char *)VsnExpTable + vp->VpVsnExp[i]);
		(void) diskvols->BeginIterator(diskvols);

		while (diskvols->GetIterator(diskvols, &volName, &dv) == 0) {
			if (regex(ve->VeExpbuf, volName, NULL)) {
				vsn = Find_VSN(media_type, volName);
				AssignDiskVol(archset_robot_index, vsn);
			}
		}

		(void) diskvols->EndIterator(diskvols);
	}
}


/*
 * Read the archset table, which was written by the archiver last time
 * the archiver.cmd file changed.  This table may contain information about
 * recycling by archive sets.
 */
void
readArchset(void)
{
	static char *fname = ARCHIVER_DIR"/"ARCHIVE_SETS;
	void *mp;

	if ((mp = MapFileAttach(fname, ARCHSETS_MAGIC, O_RDONLY)) == NULL) {
		emit(TO_ALL, LOG_ERR, 20273, fname, "Missing");
		cannot_recycle = TRUE;
		return;
	}
	afh = (struct ArchSetFileHdr *)mp;
	if (afh->AsVersion != ARCHSETS_VERSION) {
		emit(TO_ALL, LOG_ERR, 20323, afh->AsVersion, ARCHSETS_VERSION);
		cannot_recycle = TRUE;
		return;
	}
	ArchSetTable = (struct ArchSet *)(void *)
	    ((char *)afh + afh->ArchSetTable);
	VsnExpTable = (struct VsnExp *)(void *)
	    ((char *)afh + afh->VsnExpTable);
	VsnListTable = (struct VsnList *)(void *)
	    ((char *)afh + afh->VsnListTable);
	VsnPoolTable = (struct VsnPool *)(void *)
	    ((char *)afh + afh->VsnPoolTable);
}


/*
 *  Mark the given VSN_candidate as being recycled, if the robot allows it.
 */
void
select_this_candidate(
	VSN_TABLE *VSN_candidate,
	ROBOT_TABLE *Robot,
	FILE *mail_file)
{
	if (Robot->ignore) {
		return;		/* should not happen */
	}

	if (IS_DISK_MEDIA(VSN_candidate->media) == FALSE) {
		int volStatus;

		volStatus = ArchiverVolStatus(
		    sam_mediatoa(VSN_candidate->media), VSN_candidate->vsn);
		Trace(TR_MISC, "ArchiverVolStatus(%s.%s) = %d",
		    sam_mediatoa(VSN_candidate->media), VSN_candidate->vsn,
		    volStatus);
		if (volStatus != 0) {
			/* Archiver says busy for %s.%s */
			cemit(TO_FILE, 0, 20319,
			    sam_mediatoa(VSN_candidate->media),
			    VSN_candidate->vsn);
			VSN_candidate->needs_recycling = FALSE;
			return;
		}
	}

	flag_recycle(VSN_candidate);
	if (mail_file) {
		char *tr_msg;
		char *fmt;
		if (VSN_candidate->is_recycling) {
			tr_msg = "Previously selected VSN %s";
			fmt = catgets(catfd, SET, 20250,
			    "Previously selected VSN %s is being processed.\n");
		} else {

			if (IS_DISK_MEDIA(VSN_candidate->media)) {
				tr_msg = "Will recycle VSN %s";
				fmt = catgets(catfd, SET, 20252,
				    "I will recycle %s.\n");
			} else {
				tr_msg = "Suggest recycle VSN %s";
				fmt = catgets(catfd, SET, 20253,
				    "I suggest you recycle VSN %s\n.");
			}
		}
		fprintf(mail_file, fmt, VSN_candidate->vsn);
		Trace(TR_MISC, tr_msg, VSN_candidate->vsn);
	}
}

static void
strFromFreeup(
	long long value)
{
	if (value > 0) {
		(void) StrFromFsize((fsize_t)value, 1, fu_buf, sizeof (fu_buf));
	} else {
		strcpy(fu_buf, "0");
	}
}

static void
strFromQuantity(
	long long value,
	boolean_t limit)
{
	if (limit == FALSE) {
		strcpy(qu_buf, catgets(catfd, SET, 20299, "unlimited"));
	} else {
		(void) StrFromFsize((fsize_t)value, 1, qu_buf, sizeof (qu_buf));
	}
}

static void
strFromVsncount(
	int value,
	boolean_t limit
)
{
	if (limit == FALSE) {
		strcpy(su_buf, catgets(catfd, SET, 20299, "unlimited"));
	} else {
		sprintf(su_buf, "%d", value);
	}
}

static int
getMultiVsnInfo(
	char *fs_name,
	union sam_di_ino *inode,
	int copy_to_get,
	struct sam_section *buf,
	size_t bufsize)
{
	int copy;
	int error = 0;
	int fs_fd = -1;
	int n_vsns = 0;
	int section_offset = 0;
	int nvsns[MAX_ARCHIVE];
	size_t size;
	struct sam_section *vsnp = NULL;
	struct sam_ioctl_idmva idmva;

	ASSERT(copy_to_get < MAX_ARCHIVE);
	ASSERT(buf != NULL);

	/*
	 * Use ioctl command IDMVA to get multivolume vsns
	 * information for all 4 possible copies first.
	 */
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		nvsns[copy] = inode->inode.ar.image[copy].n_vsns;
		if (inode->inode.di.version >= SAM_INODE_VERS_2) {
			/* Curr version */
			if (inode->inode.ar.image[copy].n_vsns > 1) {
				n_vsns += inode->inode.ar.image[copy].n_vsns;
			}
		} else if (inode->inode.di.version == SAM_INODE_VERS_1) {
			/* Prev version */
			if (inode->inode_v1.aid[copy].ino != 0) {
				if (inode->inode.ar.image[copy].n_vsns > 1) {
					n_vsns +=
					    inode->inode.ar.image[copy].n_vsns;
				} else {
					inode->inode_v1.aid[copy].ino = 0;
					inode->inode_v1.aid[copy].gen = 0;
				}
			}
		}
	}

	ASSERT(bufsize == SAM_SECTION_SIZE(nvsns[copy_to_get]));
	size = SAM_SECTION_SIZE(n_vsns);
	SamMalloc(vsnp, size);

	/*
	 * Setup argument structure for ioctl command IDMVA.
	 */
	idmva.id = inode->inode.di.id;
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (inode->inode.di.version >= SAM_INODE_VERS_2) {
			idmva.aid[copy].ino = idmva.aid[copy].gen = 0;
		} else if (inode->inode.di.version == SAM_INODE_VERS_1) {
			idmva.aid[copy] = inode->inode_v1.aid[copy];
		}
	}
	idmva.size = size;
	idmva.buf.ptr = (void *)vsnp;

	fs_fd = open(fs_name, O_RDONLY);
	if (fs_fd >= 0) {
		if (ioctl(fs_fd, F_IDMVA, &idmva) < 0) {
			error = errno;
			if (errno != ENOENT) {
				emit(TO_ALL, LOG_ERR, 5024,
				    fs_name, inode->inode.di.id.ino,
				    inode->inode.di.id.gen);
			}
			SamFree(vsnp);
			(void) close(fs_fd);
			errno = error;
			return (-1);
		}
	} else {
		emit(TO_ALL, LOG_ERR, 20262, fs_name, errtext);
		SamFree(vsnp);
		return (-2);
	}
	(void) close(fs_fd);

	/*
	 * Extract out multivolume vsns information for specified copy.
	 */
	for (copy = 0; copy < copy_to_get; copy++) {
		if (nvsns[copy] > 1) {
			section_offset += nvsns[copy];
		}
	}
	memcpy((void *)buf, (void *)(vsnp + section_offset), bufsize);
	SamFree(vsnp);

	return (0);
}

static int
getMinGain(ROBOT_TABLE *robot, VSN_TABLE *vsn)
{
	int mingain = robot->min;

	/* Check if the mingain is default, look at the vsn capacity */
	if (mingain == DEFAULT_MIN_GAIN) {
		if (vsn->capacity < MIN_GAIN_MEDIA_THRSH) {
			mingain = MIN_GAIN_SM_MEDIA;
		} else {
			mingain = MIN_GAIN_LG_MEDIA;
		}
	}

	return (mingain);
}
