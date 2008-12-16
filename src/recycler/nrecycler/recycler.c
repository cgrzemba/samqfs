/*
 * recycler.c - recycler's main program.
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
#pragma ident "$Revision: 1.14 $"

static char *_SrcFile = __FILE__;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/acl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/bitmap.h>

#include "recycler_c.h"
#include "recycler_threads.h"

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>

#include "sam/types.h"
#include "aml/shm.h"
#include "sam/mount.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/catlib.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "aml/diskvols.h"
#include "aml/sam_rft.h"
#include "aml/id_to_path.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "aml/archiver.h"
#include "aml/samapi.h"
#include "aml/remote.h"	/* Recycler uses sam_chmed_value api function */
			/* which check for sam remote, so recycler must */
			/* have the sam remote functions declared. */
#include "../../../src/fs/cmd/dump-restore/csd.h"

#include "recycler.h"

char *program_name = "sam-nrecycler";

shm_alloc_t master_shm;
shm_ptr_tbl_t *shm_ptr_tbl = NULL;
boolean_t CannotRecycle = B_FALSE;
boolean_t RegenDatfiles = B_FALSE;
boolean_t InfiniteLoop = B_TRUE;
MediaTable_t ArchMedia = { NULL, 0, 0, 0, NULL, NULL, 0, 0, 0, 0 };

static boolean_t ignoreRecycle = B_FALSE;
/* LINTED static unused */
static boolean_t excludedFilesystems = B_FALSE;

static CsdTable_t csdList = { 0, NULL };

static void initScan(int pass, DiskVolumeSeqnum_t min);

static void selectCandidates(MediaTable_t *table);
static void selectDkCandidate(MediaTable_t *table, MediaEntry_t *vsn);
static void selectRmCandidate(MediaEntry_t *vsn);

static long long unlinkFile(char *host, char *name);
static void setRecycleFlag(MediaEntry_t *vsn);
static void startRecyclerScript(MediaEntry_t *vsn);

static void detectAnotherRecycler();
static void usage();

void
main(
	int argc,
	char **argv)
{
	int rval;
	char c;
	int i;
	size_t size;
	boolean_t err;
	struct sam_fs_info *firstFs;
	int numFs;
	int numCsd;
	CsdDir_t *csdDir;
	Crew_t crew;
	int numWorkers;
	DiskVolumeSeqnum_t maxseqnum;
	DiskVolumeSeqnum_t seqnum;
	int pass;
	char *logfilePath;
	time_t clock;
	char *start;
	char ctime_buf[512];

	clock = time(NULL);

	err = 0;
	logfilePath = NULL;

	/*
	 * Open message catalog file.
	 */
	CustmsgInit(0, NULL);

	/*
	 * Make sure there's not another instance of the recycler
	 * running.
	 */
	detectAnotherRecycler();

	/*
	 * Command-line option processing.
	 */
	while ((c = getopt(argc, argv, "nl:RX:T:")) != EOF) {
		switch (c) {
		case 'n':	/* prevent any action from being taken */
			ignoreRecycle = B_TRUE;
			break;
		case 'R':	/* regenerate all nrecycler's dat files */
			RegenDatfiles = B_TRUE;
			break;
		case 'X':	/* excluded file systems */
		/*
		 * Exclude the file system names appearing in the
		 * comma-separated * list 'fslist' from the list of file
		 * systems to evaluate for recycling. This option allows
		 * recycling to occur even if file system are umounted.
		 * By default, all file systems must be mounted.
		 *
		 * man rcs(-elogins)
		 * If 'fslist' is omitted, all umounted file systems
		 * will be ignored. NO
		 */
			excludedFilesystems = B_TRUE;
			break;

		case 'T':
		/*
		 * sfind -xtime  - Retain archive copies for files whose
		 * last modification period was CCYYMMDDHHMM ago.
		 */

		/*
		 * sfind -rafter - Retain archive copies for files whose
		 * last modification period ends after the given date.
		 * The date is specified with traditional format CCYYMMDDHHMM.
		 * CC is the century, YY is the year, MM is the month,
		 * DD is the day, HH is the hour, and MM is minutes(s).
		 */
			break;

		case '?':
		default:
			err++;
			break;
		}
	}

#if 0
	if (optind == argc) {
		(void) fprintf(stderr,
		    "SAM-FS dump directory is a required parameter\n");
		err++;
	}

	for (i = optind; argv[i] != NULL; i++) {
		dirname = argv[i];
		if (dirname != NULL && dirname[0] != '/') {
			(void) fprintf(stderr,
			    "SAM-FS dump directory must be a fullpath name\n");
			err++;
		}
	}
#endif

	if (err != 0) {
		usage();
		exit(1);
	}

	TraceInit(program_name, TI_nrecycler);
	Trace(TR_MISC, "Recycler daemon started");

	/*
	 * Read recycler's command file.
	 */
	CmdReadfile();

	/*
	 * Open log file.  Must be done before changing to nrecycler
	 * home directory.
	 */
	logfilePath = CmdGetLogfilePath();
	if (logfilePath != NULL) {
		(void) LogOpen(logfilePath);
	}

	start = ctime_r(&clock, ctime_buf, sizeof (ctime_buf));
	*(start + strlen(start) - 1) = '\0';

	Log(20400, start);		/* Recycler begins */
	if (ignoreRecycle == B_TRUE) {
		Log(20401);		/* ignore option enabled */
	}

	MakeDir(RECYCLER_HOMEDIR);
	if (chdir(RECYCLER_HOMEDIR) == -1) {
		Trace(TR_MISC, "Error: chdir '%s' failed, errno: %d",
		    RECYCLER_HOMEDIR, errno);
		Log(20402);
		exit(EXIT_FATAL);
	}

	/*
	 * Initialize access to samfs devices.
	 */
	DeviceInit();

	/*
	 * Initialize archive media table.  The archive media table is
	 * accumulating media usage for all file systems and samfs dump
	 * files.
	 */
	if (MediaInit(&ArchMedia, "ARCHIVE") != 0) {
		Log(20402);
		exit(EXIT_FATAL);
	}

	/*
	 * Get file system configuration.  All file systems that are writable
	 * or for which we're the metadata server must be mounted to allow the
	 * .inodes file to be read.
	 */
	firstFs = FsInit(&numFs);
	if (firstFs == NULL) {
		Log(20402);
		exit(EXIT_FATAL);
	}

	/*
	 * Get file system dump information for all specified samfs dump
	 * files.  A quick pass to make sure all files in the specified
	 * dump directories are actually valid samfs dump files.
	 */
	for (i = 0; CmdGetDumpPath(i) != NULL; i++) {
		csdList.ct_count++;
	}

	numCsd = 0;

	if (csdList.ct_count > 0) {
		size = csdList.ct_count * sizeof (CsdDir_t);
		SamMalloc(csdList.ct_data, size);
		(void) memset(csdList.ct_data, 0, size);


		for (i = 0; CmdGetDumpPath(i) != NULL; i++) {
			csdDir = CsdInit(CmdGetDumpPath(i));
			if (csdDir == NULL) {
				Log(20405, CmdGetDumpPath(i));
				FATAL_EXIT();
			}
			/*
			 * Initialize dat file for each samfsdump file.
			 */
			DatInit(csdDir);
			csdList.ct_data[i] = csdDir;
			/*
			 * Accumulate total number of samfs dump files to
			 * process.
			 */
			numCsd += csdDir->cd_count;
		}
	}

	numWorkers = numCsd > numFs ? numCsd : numFs;
	rval = CrewCreate(&crew, numWorkers);
	if (rval != 0) {
		Trace(TR_MISC, "Error: CrewCreate failed");
		exit(EXIT_FAILURE);
	}

	pass = 1;			/* at least one scanning pass */
	seqnum = 0;
	maxseqnum = MediaGetSeqnum(&ArchMedia);

	while (pass == 1 || seqnum <= maxseqnum) {
		/*
		 * Initialize tables for next scan pass.
		 */
		initScan(pass, seqnum);

		Trace(TR_MISC, "Scanning pass: %d seqnum candidates: %lld-%lld",
		    pass, seqnum, seqnum + ArchMedia.mt_mapchunk - 1);

		/*
		 * Scan each file system's inode file.
		 */
		Trace(TR_MISC, "Begin scanning %d samfs inode files", numFs);
		FsScan(&crew, firstFs, numFs, pass, FsAccumulate);
		Trace(TR_MISC, "Done scanning samfs inode files");

		/*
		 * Scan each samfs dump file.
		 */
		Trace(TR_MISC, "Begin scanning %d samfs dump files", numCsd);
		if (numCsd > 0) {
			CsdScan(&crew, &csdList, numCsd, pass, CsdAccumulate);
		}
		Trace(TR_MISC, "Done scanning samfs dump files");

		if (CannotRecycle == B_TRUE) {
			Trace(TR_MISC, "Cannot recycle due to errors");
			break;
		}

		MediaDebug(&ArchMedia);
		selectCandidates(&ArchMedia);

		seqnum += ArchMedia.mt_mapchunk;
		pass++;

	}

	CrewCleanup(&crew);
	FsCleanup(firstFs, numFs);
	CsdCleanup(&csdList);

	(void) LogClose();

	exit(0);
}

/*
 * Initialize media tables for a scanning pass.  In pass 1, removable media
 * and a disk seqnum number range is scanned for.  During subsequent passes
 * only disk seqnum number ranges are scanned for.
 */
static void
initScan(
	int pass,
	DiskVolumeSeqnum_t min)
{
	boolean_t diskArchive;

	/*
	 * Initialize media tables for next scan pass.
	 */
	diskArchive = B_FALSE;
	if (pass > 1) {
		diskArchive = B_TRUE;
	}
	MediaSetSeqnum(&ArchMedia, min, diskArchive);
	CsdSetSeqnum(&csdList, min, diskArchive);
}

static void
selectCandidates(
	MediaTable_t *table)
{
	int i;
	MediaEntry_t *vsn;
	boolean_t diskArchive;
	boolean_t noexist;

	Trace(TR_MISC, "[%s] Select results, seqnum candidates: %lld-%lld",
	    table->mt_name, table->mt_mapmin,
	    table->mt_mapmin + table->mt_mapchunk - 1);

	diskArchive = table->mt_diskArchive;
	if (diskArchive == B_TRUE) {
		Trace(TR_MISC, "  disk archive accumulation only");
	}

	for (i = 0; i < table->mt_tableUsed; i++) {
		vsn = &table->mt_data[i];

		/*
		 * Disk archive pass only so gnore removable media.
		 */
		if ((diskArchive == B_TRUE) && (vsn->me_type != DT_DISK)) {
			continue;
		}

		/*
		 * VSN does not exist in current catalog/dictionary.
		 * This media will not be recycled.
		 */
		noexist = GET_FLAG(vsn->me_flags, ME_noexist);
		if (noexist == B_TRUE) {
			continue;
		}

		if (vsn->me_type == DT_DISK) {
			selectDkCandidate(table, vsn);
		} else {
			selectRmCandidate(vsn);
		}
	}
	Trace(TR_MISC, "[%s] End select results", table->mt_name);
}


static void
selectDkCandidate(
	MediaTable_t *table,
	MediaEntry_t *vsn)
{
	int rval;
	DiskVolumeSeqnum_t seqnum;
	DiskVolsDictionary_t *diskvols;
	DiskVolumeInfo_t *dv;
	int idx;
	upath_t fullpath;
	char path[128];
	char *host;
	char *name;
	char *media;
	long long recycledSpace;

	if (vsn->me_bitmap == NULL) {
		return;
	}

	name = vsn->me_name;
	media = sam_mediatoa(vsn->me_type);

	rval = DiskVolsInit(&diskvols, DISKVOLS_VSN_DICT, program_name);
	if (rval != 0 || diskvols == NULL) {
		Trace(TR_MISC, "Error: disk volume dictionary init failed");
		return;
	}

	recycledSpace = 0;
	rval = diskvols->Open(diskvols, DISKVOLS_RDONLY);
	if (rval != 0) {
		Trace(TR_MISC, "Error: disk volume dictionary open failed");
		(void) DiskVolsDestroy(diskvols);
		return;
	}
	(void) diskvols->Get(diskvols, name, &dv);

	for (idx = 0; idx <= table->mt_mapchunk; idx++) {
		if (BT_TEST(vsn->me_bitmap, idx) == 0) {
			seqnum = table->mt_mapmin + idx;
			if (seqnum <= vsn->me_maxseqnum) {
				(void) DiskVolsGenFileName(seqnum, fullpath,
				    sizeof (fullpath));
				CsdAssembleName((char *)dv->DvPath, fullpath,
				    path, sizeof (path));
				host = DiskVolsGetHostname(dv);
				if (host != NULL) {
					Trace(TR_MISC, "[%s.%s] unlink %s:%s",
					    media, name, host, path);
				} else {
					Trace(TR_MISC, "[%s.%s] unlink %s",
					    media, name, path);
				}
				recycledSpace += unlinkFile(host, path);
			}
		}
	}

	(void) diskvols->Close(diskvols);
	(void) DiskVolsDestroy(diskvols);

	Trace(TR_MISC, "[%s.%s] Recycled space during remove: %s",
	    media, name, StrFromFsize(recycledSpace, 1, NULL, 0));
}

static void
selectRmCandidate(
	MediaEntry_t *vsn)
{
	if (vsn->me_files == 0 && GET_FLAG(vsn->me_flags, ME_ignore) == 0) {
		if (GET_FLAG(vsn->me_flags, ME_recycle) == B_FALSE) {
			setRecycleFlag(vsn);
		} else {
			startRecyclerScript(vsn);
		}
	}
}

/*
 * Unlink file on local or remote host.
 */
static long long
unlinkFile(
	char *host,
	char *name)
{
	SamrftImpl_t *rft;
	SamrftStatInfo_t buf;
	int rval;

	rft = SamrftConnect(host);
	if (rft == NULL) {
		if (host != NULL) {
			Trace(TR_MISC,
			    "Error: remote connection to '%s' failed", host);
		} else {
			Trace(TR_MISC, "Error: local connection failed");
		}
		return (0);
	}

	if (SamrftStat(rft, name, &buf) < 0) {
		SamrftDisconnect(rft);
		return (0);
	}

	if (ignoreRecycle == B_FALSE) {
		rval = SamrftUnlink(rft, name);
		if (rval != 0) {
			Trace(TR_MISC, "Error: unlink failed %d", rval);
		}
	}
	SamrftDisconnect(rft);
	return (buf.size);
}

/*
 * Set flag in catalog that media is to be recycled.
 */
static void
setRecycleFlag(
	MediaEntry_t *vsn)
{
	int volStatus;

	volStatus = ArchiverVolStatus(sam_mediatoa(vsn->me_type), vsn->me_name);
	Trace(TR_MISC, "[%s.%s] archiver volume status %d",
	    sam_mediatoa(vsn->me_type), vsn->me_name, volStatus);
	if (volStatus != 0) {
		return;
	}

	Trace(TR_MISC, "[%s.%s] set recycling flag in catalog, eq %d",
	    sam_mediatoa(vsn->me_type), vsn->me_name, vsn->me_dev->eq);

	if (ignoreRecycle == B_TRUE) {
		return;
	}

	(void) sam_chmed(vsn->me_dev->eq, vsn->me_slot, vsn->me_part,
	    sam_mediatoa(vsn->me_type), vsn->me_name,
	    CMD_CATALOG_RECYCLE, 1, 0);
}

/*
 * Start recycler's post-processing script.
 */
static void
startRecyclerScript(
	MediaEntry_t *vsn)
{
	char *cmdpath;
	mtype_t mtype;
	vsn_t volname;
	char cmdline[512];
	char *modifier;
	FILE *fp;
	char *cmdstate;
	int rval;

	cmdpath = CmdGetScriptPath();

	if (cmdpath == NULL) {
		Trace(TR_MISC, "Error: recycler's script path is null");
		return;
	}

	Trace(TR_MISC, "nrecycler.sh %s.%s",
	    sam_mediatoa(vsn->me_type), vsn->me_name);

	if (ignoreRecycle == B_TRUE) {
		return;
	}

	(void) memcpy(volname, vsn->me_name, sizeof (vsn_t));

	DeviceGetMediaType(vsn->me_dev, &mtype);

	if (vsn->me_part == 1) {
		modifier = "1";
	} else {
		modifier = "2";
	}

	(void) snprintf(cmdline, sizeof (cmdline), "%s %s %s %d %d %s %s %s",
	    cmdpath, mtype, volname, vsn->me_slot,
	    DeviceGetEq(vsn->me_dev), DeviceGetTypeMnemonic(vsn->me_dev),
	    DeviceGetFamilyName(vsn->me_dev), modifier);

	(void) fflush(NULL);		/* flush all writable files */

	cmdstate = "failed";
	fp = popen(cmdline, "w");
	if (fp != NULL) {
		rval = pclose(fp);
		if (rval == 0) {
			cmdstate = "issued";
		}
	}
	Trace(TR_MISC, "Command '%s %s'", cmdstate, cmdline);
}


/*
 * Detect if another recycler is running and exit if there is.
 */
static void
detectAnotherRecycler(void)
{
	char pidString[MAXPATHLEN];
	int fd;
	sam_defaults_t *defaults;

	defaults = GetDefaults();
	if (defaults == NULL || ((defaults->flags & DF_NEW_RECYCLER) == 0)) {
		SendCustMsg(HERE, 20413);
		exit(1);
	}

	fd = open(RECYCLER_LOCK_PATH, O_CREAT|O_RDWR, 0644);
	/*
	 * If cannot create lock file, assume its okay.
	 */
	if (fd < 0) {
		return;
	}

	if (lockf(fd, F_TLOCK, 0)) {
		if (errno == EAGAIN) {
			(void) close(fd);
			SendCustMsg(HERE, 20265);
			exit(1);
		}
		return;
	}

	(void) ftruncate(fd, (off_t)0);
	(void) sprintf(pidString, "%ld\n", getpid());
	(void) write(fd, pidString, strlen(pidString));
}

static void
usage(void)
{
	(void) fprintf(stderr, "usage: %s [-n] path ...\n",
	    program_name);
}
