/*
 * catalog.c - catalog files to removable media.
 *
 * recycler examines the file system and determines what files are
 * active on what volumes.
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

#pragma ident "$Revision: 1.50 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <libgen.h>
#include <synch.h>
#include <thread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/syscall.h"
#include "sam/mount.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "aml/catlib.h"
#include "sam/sam_trace.h"
#include "sam/lint.h"

/* Local headers. */
#define	DEC_INIT
#include "recycler.h"


/* Private data. */
static int number_of_robots;

/* Shared memory segment pointers. */
static shm_alloc_t master_shm;
static shm_ptr_tbl_t *shm_ptr_tbl = NULL;

/* Private functions. */
static boolean_t insert_vsn(ROBOT_TABLE *robot, VSN_TABLE *vsn,
	struct CatalogEntry *ce);


void
Init_shm(void)
{
	char msgbuf[1024];

	/*
	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		/*
		 * If disk archive recycling the shared memory
		 * segment is not needed.
		 */
		return;
	}
	if ((master_shm.shared_memory =
	    shmat(master_shm.shmid, NULL, 0)) == (void *)-1) {
		emit(TO_SYS|TO_TTY, LOG_ERR, 20200, errtext);

		/* Send sysevent to generate SNMP trap */
		sprintf(msgbuf, GetCustMsg(20200), errtext);
		(void) PostEvent(RY_CLASS, "Attachshm", 20200, LOG_ERR, msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);

		exit(1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
		emit(TO_SYS|TO_TTY, LOG_ERR, 20261);

		/* Send sysevent to generate SNMP trap */
		(void) PostEvent(RY_CLASS, "Incorrectshm", 20261, LOG_ERR,
		    GetCustMsg(20261), NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);

		exit(1);
	}
	if (CatalogInit(program_name) == -1) {
		sam_syslog(LOG_ERR, "%s", catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(1);
	}
}


/*
 * Initialize file system information.  Really, we've just got a linked list
 * of the filesystems.
 */
void
Init_fs(void)
{
	struct sam_fs_status *fsarray, *fs;
	struct sam_fs_info *fsp;
	int i;
	int size;

	first_fs = NULL;
	if ((num_fs = GetFsStatus(&fsarray)) == -1) {
		emit(TO_SYS|TO_TTY, LOG_ERR, 4800);
		exit(1);
	}

	Trace(TR_FILES, "Total number of filesystems: %d", num_fs);

	/*
	 * Get the mount parameters and devices from the filesystem for
	 * each filesystem.
	 */
	size = num_fs * sizeof (struct sam_fs_info);
	first_fs = (struct sam_fs_info *)malloc(size);
	if (first_fs == NULL) {
		/* malloc() failed. */
		emit(TO_SYS|TO_TTY, LOG_ERR, 20322, size, errtext);
		exit(1);
	}
	memset(first_fs, 0, sizeof (struct sam_fs_info) * num_fs);
	for (i = 0, fsp = first_fs, fs = fsarray; i < num_fs; i++, fs++) {
		if (GetFsInfo(fs->fs_name, fsp) == -1) {
			emit(TO_SYS|TO_TTY, LOG_ERR, 4804, fs->fs_name);
			exit(1);
		}
		/*
		 * If this system is not the metadata server/writer
		 * for this filesystem, ignore the filesystem.
		 */
		if ((fsp->fi_config & MT_SHARED_READER) ||
		    (fsp->fi_status & FS_CLIENT)) {
			Trace(TR_FILES,
			    "Can't access inodes for filesystem %s, "
			    "ignoring", fsp->fi_name);
		} else {
			Trace(TR_FILES, "filesystem %s will be examined",
			    fsp->fi_name);
			fsp++;
		}
	}
	free(fsarray);
	num_fs = fsp-first_fs;
	if (num_fs == 0) {
		/*
		 * Table of info. about filesystems is empty. They were all
		 * client or shared reader.
		 */
		free(first_fs);
		first_fs = NULL;
		Trace(TR_FILES, "No acceptable filesystems were found.");
	}
	/*
	 * Note that the first_fs array may not be full (however, it is packed).
	 * It does not contain filesystems for which this system is currently
	 * a client or the filesystem is read-only.
	 */
}


/*
 * Count the number of physical robots.  Look in the archset table to
 * find the number of archive sets.  Allocate enough robot table entries
 * to hold both types of "robot".  For the physical robots, map in the
 * catalog files and attach them to the robot table entry.
 */
void
MapCatalogs(void)
{
	dev_ent_t *dev;
	int robot_i;
	int size;

	/*
	 * Count all devices.
	 * Make the device catalog array.
	 *
	 */

	number_of_robots = 0;

	if (shm_ptr_tbl != NULL) {
		for (dev = (dev_ent_t *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
		    dev != NULL; dev = (dev_ent_t *)SHM_REF_ADDR(dev->next)) {
			if (IS_ROBOT(dev) || IS_RSC(dev)) {
				number_of_robots++;
			}
		}
	}

	if (number_of_robots == 0 && afh->ArchSetNumof == 0) {
		emit(TO_SYS|TO_TTY, LOG_INFO, 3067);
		return;
	}

	/*
	 * Now that we've counted the robots, and read the archset table,
	 * we know how many robot table entries we need:  enough to hold
	 * the sum of the physical robots, plus the number of archive sets.
	 * Note that the historian is a "physical" robot.
	 */
	ROBOT_count = number_of_robots + afh->ArchSetNumof;
	size = ROBOT_count * sizeof (ROBOT_TABLE);
	if ((ROBOT_table = (ROBOT_TABLE *)malloc(size)) == NULL) {
		emit(TO_ALL, LOG_ERR, 3068, size, errtext);
		cannot_recycle = TRUE;
		return;
	}
	memset(ROBOT_table, 0, size);

	/*  Apply default parameters to each robot.  */
	for (robot_i = 0; robot_i < ROBOT_count; robot_i ++) {
		ROBOT_TABLE *ROBOT = &ROBOT_table[robot_i];

		ROBOT->high = DEFAULT_ROBOT_HWM;
		ROBOT->obs = DEFAULT_MIN_OBS;
		ROBOT->min = DEFAULT_MIN_GAIN;
		ROBOT->dataquantity = DEFAULT_DATAQUANTITY;
		ROBOT->vsncount = DEFAULT_VSNCOUNT;
		ROBOT->ignore = TRUE;
		ROBOT->limit_quantity = TRUE;
		ROBOT->limit_vsncount = TRUE;
		ROBOT->mail = FALSE;
		ROBOT->mailaddress = "root";
	}


	/*
	 * Map in robot catalogs.
	 *
	 */
	number_of_robots = 0;
	if (shm_ptr_tbl == NULL) {
		return;
	}
	for (dev = (dev_ent_t *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	    dev != NULL; dev = (dev_ent_t *)SHM_REF_ADDR(dev->next)) {

		if (IS_ROBOT(dev) || IS_RSC(dev)) {
			ROBOT_TABLE *ROBOT = &ROBOT_table[number_of_robots];

			ROBOT->chtable = CatalogGetEntriesByLibrary(dev->eq,
			    &ROBOT->chtable_numof);
			ROBOT->cat_file_size = ROBOT->chtable_numof *
			    sizeof (struct CatalogEntry);

			ROBOT->dev = dev;
			strcpy(ROBOT->name, dev->dt.rb.name);
			ROBOT->archset = B_FALSE;
			number_of_robots++;
		}
	}
}

/*
 * Print catalogs.  Also accumulate size and capacity from VSNs in each
 * robot's catalog into the robot table.
 * Determine if robot needs to be recycled.
 */
void
PrintCatalogs(void)
{
	int  robot_idx;
	int  total_vsns;
	char cap_buf[1024], spa_buf[1024];
	int ce_idx;
	int log_idx;
	ROBOT_TABLE	*ROBOT;
	int total_libraries;
	int total_archsets;

	total_libraries = 0;
	total_archsets = 0;

	for (robot_idx = 0; robot_idx < number_of_robots; robot_idx++) {
		ROBOT = &ROBOT_table[robot_idx];

		if (ROBOT->asflags & AS_diskArchSet) {
			continue;
		}

		if (ROBOT->archset == B_TRUE) {
			total_archsets++;
		} else {
			total_libraries++;
		}
	}

	Trace(TR_MISC, "Accumulate removable media libraries: %d "
	    "archive sets: %d", total_libraries, total_archsets);

	if (CATALOG_LIST_ON()) {
		/* Removable Media Libraries: %d Archive Sets: %d */
		emit(TO_FILE, 0, 20205, total_libraries, total_archsets);
	}

	if (total_libraries == 0 && total_archsets == 0) {
		if (CATALOG_LIST_ON()) {
			emit(TO_FILE, 0, 20259);
		}
		return;
	}

	/* Loop through all the robots.  Display the catalog for each.  */

	log_idx = 0;

	for (robot_idx = 0; robot_idx < number_of_robots; robot_idx++) {

		offset_t free = 0LL, total = 0LL;
		offset_t util;

		ROBOT = &ROBOT_table[robot_idx];
		if (ROBOT->asflags & AS_diskArchSet) {
			continue;
		}

		/*
		 * Display catalog header information:
		 * vendor, product, path, etc.
		 */
		if (CATALOG_LIST_ON()) {
			if (ROBOT->archset) {
				emit(TO_FILE, 0, 20207,
				    log_idx, ROBOT->name, ARCHIVER_CMD,
				    "SAM-FS", "Archive set");
			} else {
				emit(TO_FILE, 0, 20207,
				    robot_idx, family_name(ROBOT),
				    ROBOT->name,
				    vendor_id(ROBOT), product_id(ROBOT));
			}
			emit(TO_FILE, 0, 20208);
		}

		log_idx++;

		/* Display the entries in the robot's catalog */
		total_vsns = 0;

		for (ce_idx = 0; ce_idx < ROBOT->chtable_numof; ce_idx++) {
			media_t media;
			struct CatalogEntry *ce;

			ce = &ROBOT->chtable[ce_idx];
			if (!(ce->CeStatus & CES_inuse) ||
			    ce->CeVsn[0] == '\0') {
				continue;
			}

			total_vsns ++;

			if (!catalog_summary_only && CATALOG_LIST_ON()) {
				char *state;
				char *readonly;

				strcpy(cap_buf,
				    s2a((long long)ce->CeCapacity * 1024LL));
				strcpy(spa_buf,
				    s2a((long long)ce->CeSpace * 1024LL));

				media = sam_atomedia(ce->CeMtype);

				state = "         ";
				readonly = "    ";
				if (ce->CeStatus & CES_recycle) {
					state = "(recycle)";
				} else if (ce->CeStatus & CES_unavail) {
					state = "(unavail)";
				}
				if (ce->CeStatus & CES_read_only) {
					readonly = "(ro)";
				}

				emit(TO_FILE, 0, 20209,
				    ce->CeSlot, state, readonly,
				    device_to_nm(media),
				    cap_buf, spa_buf, ce->CeVsn);
			}

			free += (long long)ce->CeSpace * 1024LL;
			total += (long long)ce->CeCapacity * 1024LL;
		}

		if (!total_vsns && CATALOG_LIST_ON()) {
			emit(TO_FILE, 0, 20210);
		}

		util = total ? (total-free)*100LL/total : 0LL;

		/*
		 * Update the robot table with what we've just learned
		 * about the vsns in this robot.
		 */
		ROBOT->total_vsns = total_vsns;
		ROBOT->total_space = free;
		ROBOT->total_capacity = total;

		if (CATALOG_LIST_ON()) {
			char tcap_buf[1024], tspa_buf[1024];
			strcpy(tcap_buf, s2a(ROBOT->total_capacity));
			strcpy(tspa_buf, s2a(ROBOT->total_space));
			emit(TO_FILE, 0, 20211, tcap_buf, tspa_buf);
		}

		/*
		 * Note that the > is not >= , since we're using
		 * percentages, if the robot's utilization equals the high
		 * water mark, we'll try to free up 0 blocks on the media
		 * (because the difference between the utilization in percent
		 * and the configured * high water mark will be zero, and zero
		 * times the capacity of the robot is also zero.  So, the test
		 * is for utilization strictly greater than the configured HWM.
		 */
		Trace(TR_MISC, "'%s' util: %lld%% hwm %d%%",
		    family_name(ROBOT), util, ROBOT->high);
		if (util > ROBOT->high)  {
			ROBOT->needs_work = 1;
			if (CATALOG_LIST_ON()) {
				emit(TO_FILE, 0, 20213, util, ROBOT->high,
				    ROBOT->min);
			}
		} else if (CATALOG_LIST_ON()) {
			emit(TO_FILE, 0, 20212, util, ROBOT->high, ROBOT->min);
		}

		/* Inform user that default is being based on VSN capacity */
		if (ROBOT->min == DEFAULT_MIN_GAIN && CATALOG_LIST_ON()) {
			emit(TO_FILE, 0, 20356);
		}

		if (ROBOT->mail && CATALOG_LIST_ON()) {
			if (ROBOT->archset) {
				emit(TO_FILE, 0, 20278, ROBOT->mailaddress);
			} else {
				emit(TO_FILE, 0, 20214, ROBOT->mailaddress);
			}
		}

		if (ROBOT->ignore && CATALOG_LIST_ON()) {
			if (ROBOT->archset) {
				emit(TO_FILE, 0, 20277);
			} else {
				emit(TO_FILE, 0, 20215);
			}
		}
	}
}

/*
 * Print disk archive sets.  Also accumulate active and written space from
 * all VSNs in the archive set.
 * All disk archive sets are recycling candidates.
 */
void
PrintDiskArchsets()
{
	int  robot_idx;
	int vsn_idx;
	int log_idx;
	int i;
	ROBOT_TABLE *robot_entry;
	VSN_TABLE *vsn_entry;
	offset_t active_space;
	offset_t written_space;
	int  total_vsns;
	char cap_buf[1024], spa_buf[1024];
	int num_diskArchsets;
	int percent_good;
	int percent_junk;
	char *media_type;
	char *vsn_name;

	/*
	 * Total count of disk archive sets.
	 */
	num_diskArchsets = 0;
	for (robot_idx = 0; robot_idx < number_of_robots; robot_idx++) {
		robot_entry = &ROBOT_table[robot_idx];

		if (robot_entry->asflags & AS_diskArchSet) {
			num_diskArchsets++;
		}
	}

	Trace(TR_MISC, "Accumulate disk media archive sets: %d",
	    num_diskArchsets);

	if (CATALOG_LIST_ON()) {
		emit(TO_FILE, 0, 20206, num_diskArchsets);
	}

	if (num_diskArchsets == 0) {
		if (CATALOG_LIST_ON()) {
			emit(TO_FILE, 0, 20259);
		}
		return;
	}

	log_idx = 0;

	for (robot_idx = 0; robot_idx < number_of_robots; robot_idx++) {

		active_space = 0LL;
		written_space = 0LL;

		robot_entry = &ROBOT_table[robot_idx];
		if ((robot_entry->asflags & AS_diskArchSet) == 0) {
			continue;
		}

		media_type = "dk";
		if (robot_entry->asflags & AS_honeycomb) {
			media_type = "cb";
		}

		/*
		 * Log archive header information.
		 */
		if (CATALOG_LIST_ON()) {
			emit(TO_FILE, 0, 20334, log_idx, robot_entry->name);
			emit(TO_FILE, 0, 20335);
		}

		log_idx++;

		total_vsns = 0;

		for (i = 0; i < robot_entry->diskvols.entries; i++) {

			vsn_name = robot_entry->diskvols.key[i];

			for (vsn_idx = 0; vsn_idx < table_used; vsn_idx++) {
				vsn_entry = &vsn_table[vsn_idx];

				if (strcmp(vsn_entry->vsn, vsn_name) == 0) {

					active_space  += vsn_entry->size;
					written_space += vsn_entry->written;

					total_vsns++;

					if (catalog_summary_only == TRUE) {
						continue;
					}

					if (suppress_catalog == TRUE) {
						continue;
					}

					strcpy(cap_buf,
					    s2a((long long)vsn_entry->written));
					strcpy(spa_buf,
					    s2a((long long)vsn_entry->size));

					emit(TO_FILE, 0, 20336, vsn_idx,
					    "         ", "    ",
					    media_type, cap_buf, spa_buf,
					    vsn_entry->vsn);

					Trace(TR_MISC,
					    "[%s] '%s' active space: %lld "
					    "(%s)",
					    robot_entry->name, vsn_entry->vsn,
					    vsn_entry->size,
					    StrFromFsize(vsn_entry->size, 3,
					    NULL, 0));

					Trace(TR_MISC,
					    "[%s] '%s' written space: %lld "
					    "(%s)",
					    robot_entry->name, vsn_entry->vsn,
					    vsn_entry->written,
					    StrFromFsize(vsn_entry->written, 3,
					    NULL, 0));
				}
			}
		}

		if (active_space > written_space) {
			/*
			 * Amount of data for this ARCHIVE SET as
			 * accumulated from the inodes on all the file systems
			 * greater than the space written to the media.
			 * Adjust space so we don't generate negative
			 * usage percentages.
			 */
			Trace(TR_MISC, "[%s] active space > written",
			    robot_entry->name);

			written_space = active_space +
			    (offset_t)(active_space *.05);

			/* Make sure this archive set is a candidate. */
			robot_entry->needs_work = 1;
		}

		if (written_space > 0) {
			percent_good = (active_space * 100LL)/written_space;
			percent_junk = 100LL - percent_good;
		} else {
			percent_good = 0;
			percent_junk = 0;
		}

		/*
		 * Update the robot table with what we've just learned about
		 * the vsns in this archive set.
		 */
		robot_entry->total_vsns = total_vsns;
		robot_entry->total_written = written_space;
		robot_entry->total_active = active_space;

		if (suppress_catalog == FALSE) {
			strcpy(cap_buf, s2a(robot_entry->total_written));
			strcpy(spa_buf, s2a(robot_entry->total_active));
			emit(TO_FILE, 0, 20337, cap_buf, spa_buf);
		}

		Trace(TR_MISC, "[%s] good: %d%% junk %d%%",
		    family_name(robot_entry), percent_good, percent_junk);

		/*
		 * All (almost all) disk archive sets are recycling candidates.
		 */
		if (percent_junk != 0) {
			robot_entry->needs_work = 1;
		}

		if (suppress_catalog == FALSE) {
			if (robot_entry->needs_work) {
				emit(TO_FILE, 0, 20339,
				    percent_junk, percent_good);
			} else {
				emit(TO_FILE, 0, 20352,
				    percent_junk, percent_good);
			}

			if (robot_entry->mail) {
				emit(TO_FILE, 0, 20278,
				    robot_entry->mailaddress);
			}
			if (robot_entry->ignore) {
				emit(TO_FILE, 0, 20277);
			}
		}
	}
}

/*
 * Transfer the catalog vsn information into the vsn table.  This is done
 * for real robots only.  The archive set ones have not yet been created.
 */
void
ScanCatalogs(void)
{
	int idx;
	int catalog_index;
	struct VSN_TABLE *VSN;
	ROBOT_TABLE *ROBOT;
	vsn_t vsn;
	media_t media;
	char msgbuf[1024];

	for (catalog_index = 0; catalog_index < number_of_robots;
	    catalog_index++) {

		ROBOT = &ROBOT_table[catalog_index];

		for (idx = 0; idx < ROBOT->chtable_numof; idx++) {

			struct CatalogEntry *ce = &ROBOT->chtable[idx];

			if ((ce->CeStatus & CES_inuse) &&
			    (ce->CeStatus & CES_labeled) &&
			    (ce->CeVsn[0] != '\0')) {

				memmove(vsn, ce->CeVsn, sizeof (vsn_t));
				media = sam_atomedia(ce->CeMtype);

				/*
				 * This call will probably add a new vsn
				 * to the VSN_table.
				 */
				VSN = Find_VSN((int)media, (char *)vsn);
				if (VSN == NULL) {
					emit(TO_ALL, LOG_ERR, 20333,
					    ROBOT->name, ce->CeSlot);
					continue;
				}

				if (VSN->chtable != NULL) {
					emit(TO_ALL, LOG_ERR, 20216, vsn);

					/* Generate SNMP trap */
					sprintf(msgbuf, GetCustMsg(20216), vsn);
					(void) PostEvent(RY_CLASS,
					    "DuplicateVSN",
					    20216, LOG_ERR, msgbuf,
					    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);

					VSN->duplicate = 1;
					cannot_recycle = TRUE;
				}

				VSN->chtable = ROBOT->chtable;
				VSN->ce = ce;
				VSN->dev = ROBOT->dev;
				VSN->real_dev = ROBOT->dev;
				VSN->slot = ce->CeSlot;
				VSN->is_recycling =
				    (ce->CeStatus & CES_recycle) ? 1 : 0;
				VSN->is_read_only =
				    (ce->CeStatus & CES_read_only) ? 1 : 0;
				VSN->robot = ROBOT;
				VSN->blocksize = ce->CeBlockSize;
				VSN->label_time = ce->CeLabelTime;
				VSN->capacity = ce->CeCapacity * 1024LL;
				VSN->space = ce->CeSpace * 1024LL;
			}
		}
	}
}

/*
 * Create a archset robot.  A robot has a device table entry and a catalog
 * table.  Instead of getting the recycler parameters from the recycler's
 * command file, they come from the archiver's archset table.
 * Instead of having a device table entry, a archset robot has a small integer
 * index into the robot table.    We can tell a archset robot because it's
 * got the "archset" field set true.
 */

int
create_robot(
	int archive_set_index)
{
	struct ArchSet *archive_set = &ArchSetTable[archive_set_index];

	struct CatalogEntry *catalog;
	ROBOT_TABLE *ROBOT = &ROBOT_table[number_of_robots];
	int size;

	/* First, make sure this archive set hasn't already been "robotized" */
	int robot_i;

	for (robot_i = 0; robot_i < number_of_robots; robot_i++) {
		if (strcmp(ROBOT_table[robot_i].name,
		    archive_set->AsName) == 0) {
			return (robot_i);
		}
	}

	ROBOT->asflags = archive_set->AsFlags;
	if (ROBOT->asflags & AS_diskArchSet) {
		/*
		 * Create empty dictionary for this disk archset.
		 */
		ROBOT->diskvols.alloc = DICT_CHUNKSIZE;

		size = ROBOT->diskvols.alloc * sizeof (vsn_t);
		SamMalloc(ROBOT->diskvols.key, size);
		(void) memset(ROBOT->diskvols.key, 0, size);

		size = ROBOT->diskvols.alloc * sizeof (DiskVolumeInfo_t *);
		SamMalloc(ROBOT->diskvols.data, size);
		(void) memset(ROBOT->diskvols.data, 0, size);

	} else {
		/* Create empty "catalog" for this robot */
		size = sizeof (struct CatalogEntry) * CATALOG_CHUNKSIZE;
		catalog = malloc(size);
		if (catalog == NULL) {
			emit(TO_SYS|TO_TTY, LOG_ERR, 20282, size,
			    archive_set->AsName, errtext);
			exit(1);
		}

		memset(catalog, 0, size);

		ROBOT->chtable_numof = CATALOG_CHUNKSIZE;
		ROBOT->chtable = catalog;
	}

	ROBOT->dev = (dev_ent_t *)number_of_robots;
	strcpy(ROBOT->name, archive_set->AsName);

	/* Apply archive command file values, or else defaults */

	if (archive_set->AsRyFlags & ASRY_hwm) {
		ROBOT->high = archive_set->AsRyHwm;
	}

	if (archive_set->AsRyFlags & ASRY_mingain) {
		ROBOT->min = archive_set->AsRyMingain;
	}

	if (archive_set->AsRyFlags & ASRY_ignore) {
		ROBOT->ignore = TRUE;
	} else {
		/* default for archive sets is to not ignore */
		ROBOT->ignore = FALSE;
	}

	ROBOT->mail = FALSE;
	if (archive_set->AsRyFlags & ASRY_mailaddr) {
		ROBOT->mail = TRUE;
		ROBOT->mailaddress = archive_set->AsRyMailaddr;
	}

	/*
	 * Set dataquanity default based on archive media.
	 */
	if (ROBOT->asflags & AS_diskArchSet) {
		ROBOT->limit_quantity = FALSE;
	}
	if (archive_set->AsRyFlags & ASRY_dataquantity) {
		ROBOT->dataquantity = archive_set->AsRyDataquantity;
		ROBOT->limit_quantity = TRUE;
	}

	if (archive_set->AsRyFlags & ASRY_vsncount) {
		ROBOT->vsncount = archive_set->AsRyVsncount;
		ROBOT->limit_vsncount = TRUE;
	}

	if (archive_set->AsRyFlags & ASRY_minobs) {
		ROBOT->obs = archive_set->AsRyMinobs;
	}

	ROBOT->archset = B_TRUE;
	return (number_of_robots++);
}

/*
 * Assign a VSN to a archset robot.  Create a catalog entry for the
 * VSN in the archset robot's catalog.  We use the catalog entries to
 * accumulate the total space and capacity of the robot later on
 * in PrintCatalogs().
 */

void
assign_vsn(
	int robot_index,
	VSN_TABLE *vsn)
{

	int idx;
	media_t media;
	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	boolean_t already_in_catalog = B_FALSE;
	ROBOT_TABLE *robot = &ROBOT_table[robot_index];
	struct CatalogEntry *catalog = ROBOT_table[robot_index].chtable;

	ce = vsn->ce;
	if (ce == NULL) {
		ce = CatalogGetCeByMedia(sam_mediatoa(vsn->media),
		    vsn->vsn, &ced);
		/*
		 * Nothing to assign if the catalog entry is not found. VSN has
		 * been exported from the media library and historian.
		 */
		if (ce == NULL) {
			return;
		}
	}

	/* make sure the vsn isn't already in the robot's catalog.  */
	for (idx = 0; idx < robot->chtable_numof; idx++) {
		struct CatalogEntry *entry;

		entry = &catalog[idx];
		if (entry->CeStatus & CES_inuse) {
			media = sam_atomedia(entry->CeMtype);
			if (media == vsn->media &&
			    strcmp(entry->CeVsn, vsn->vsn) == 0) {
				already_in_catalog = B_TRUE;
				break;
			}
		}
	}

	if (already_in_catalog == B_FALSE) {
		/* insert the new catalog entry from the VSN's information */
		already_in_catalog = insert_vsn(robot, vsn, ce);
	}

	/*
	 * Did not find space for new catalog entry.  Need to expand it.
	 */
	if (already_in_catalog == B_FALSE) {
		size_t size;
		int old;
		struct CatalogEntry *entry;

		old = robot->chtable_numof;
		robot->chtable_numof += CATALOG_CHUNKSIZE;
		size = robot->chtable_numof * sizeof (struct CatalogEntry);

		catalog = realloc(catalog, size);
		ROBOT_table[robot_index].chtable = catalog;

		if (catalog != NULL) {
			for (idx = robot->chtable_numof-1; idx >= old; idx--) {
				entry = &catalog[idx];
				(void) memset(entry, 0,
				    sizeof (struct CatalogEntry));
			}
		}

		if (catalog == NULL || insert_vsn(robot, vsn, ce) == B_FALSE) {
			emit(TO_SYS|TO_TTY, LOG_ERR, 20283, size,
			    robot->name, errtext);
			exit(1);
		}
	}

	/* flag the VSN entry as having been reassigned */

	vsn->was_reassigned = 1;

	/*
	 * check to see this VSN is not already assigned to some other
	 * archive set.  Being assigned to a real robot is ok.
	 */
	if ((vsn->dev != vsn->real_dev) && (vsn->dev != (void *)robot_index)) {
		vsn->multiple_arsets = TRUE;
	}
	vsn->dev = (void *)robot_index;
}

static boolean_t
insert_vsn(
	ROBOT_TABLE	*robot,
	VSN_TABLE *vsn,
	struct CatalogEntry *ce)
{
	int idx;
	boolean_t inserted;
	struct CatalogEntry *catalog;

	inserted = B_FALSE;
	catalog = robot->chtable;

	for (idx = 0; idx < robot->chtable_numof; idx++) {
		struct CatalogEntry *entry;

		entry = &catalog[idx];
		if ((entry->CeStatus & CES_inuse) == B_FALSE) {
			memcpy(entry, ce, sizeof (struct CatalogEntry));
			vsn->ce = ce;
			inserted = B_TRUE;
			break;
		}
	}
	return (inserted);
}
