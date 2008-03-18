/*
 * archiver.c - process archiver command file.
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

#pragma ident "$Revision: 1.105 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <synch.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/devnm.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"

/* Local headers. */
#define	DEC_INIT
#include "archiver.h"
#include "archset.h"
#include "device.h"
#include "fileprops.h"
#include "volume.h"
#include "common.h"
#undef	DEC_INIT

/* Private functions. */
static void checkVolAvail(void);
static void makeFileSys(void);
static void makeMediaParams(void);
static void setControls(void);
static void writeArchSetFile(void);
static void writeFileProps(void);

int
main(
	int argc,
	char *argv[])
{
	struct ArchiverdState *adState;
	extern int optind;
	char	*cmd_fname;
	char	*fsname;
	int	c;
	int	errflag;

	program_name = AA_PROG;

	/*
	 * Check initiator.
	 */
	if (strcmp(GetParentName(), SAM_ARCHIVER) != 0) {
		Daemon = FALSE;
	} else {
		if (argc > 1 && *argv[1] == '\0') {
			/*
			 * Not the first command file reading.
			 */
			FirstRead = FALSE;
			argc = 1;
			argv[1] = NULL;
		}
	}

	/*
	 * Translate options.
	 */
	errflag = 0;
	cmd_fname = NULL;
	fsname = NULL;
#if !defined(AR_DEBUG)
	while ((c = getopt(argc, argv, "Aac:fln:v")) != EOF) {
		switch (c) {
#else /* !defined(AR_DEBUG) */
	while ((c = getopt(argc, argv, "DAac:fln:v")) != EOF) {
		switch (c) {
		case 'D':
			Daemon = TRUE;
			ListOptions |= LO_line | LO_vsn;
			break;
#endif /* !defined(AR_DEBUG) */
		case 'A':
			ListOptions |= (LO_all & ~LO_arch);
			break;
		case 'c':
			cmd_fname = optarg;
			break;
		case 'a':
			ListOptions |= LO_arch;
			break;
		case 'f':
			ListOptions |= LO_fs;
			break;
		case 'l':
			ListOptions |= LO_conf | LO_line;
			break;
		case 'n':
			fsname = optarg;
			break;
		case 'v':
			ListOptions |= LO_conf | LO_vsn;
			break;
		case '?':
		default:
			errflag++;
			break;
		}
	}
	if ((argc - optind) >= 1) {
#if defined(AR_DEBUG)
		/*
		 * archiver control operations.
		 */
		if (optind == 1 && argc < 4) {
			char	msg[128];
			char	*value;

			/*
			 * No options, two arguments, assume an
			 * archiver directive.
			 */
			if (argc > 2) {
				value = argv[2];
			} else {
				value = "";
			}
			(void) ArchiverControl(argv[1], value, msg,
			    sizeof (msg));
			if (*msg != '\0') {
				fprintf(stderr, "%s\n", msg);
				exit(EXIT_FAILURE);
			} else {
				exit(EXIT_SUCCESS);
			}
		}
#endif /* defined(AR_DEBUG) */
		errflag++;
	}
	if (errflag != 0) {
		fprintf(stderr,
		    "%s  %s [-A] [-a] [-c archive_cmd] [-f] [-l]"
		    " [-nfilesystem] [-v] \n",
		    GetCustMsg(4601), program_name);
		exit(EXIT_USAGE);
	}
	if (ListOptions == LO_none) {
		if (fsname != NULL) {
			ListOptions = LO_fs;
		} else {
			ListOptions = LO_conf;
		}
	}

	/*
	 * Initialize our copy of the archiver daemon state file.
	 * The ArchReq array is not used by archiver.
	 */
	SamMalloc(AdState, sizeof (struct ArchiverdState));
	memset(AdState, 0, sizeof (struct ArchiverdState));
	strncpy(AdState->AdNotifyFile, SAM_SCRIPT_PATH"/"NOTIFY,
	    sizeof (AdState->AdNotifyFile)-1);
	AdState->AdInterval = SCAN_INTERVAL;
	if (Daemon) {
		adState = ArMapFileAttach(ARCHIVER_STATE, AD_MAGIC, O_RDWR);
		if (adState == NULL) {
			LibFatal(ArMapFileAttach, ARCHIVER_STATE);
		}
		TraceInit(program_name, TI_archiver | TR_MPLOCK);
	} else {
		TraceInit(NULL, TI_none);
		*TraceFlags = 0;
	}
#if defined(AR_DEBUG)
	*TraceFlags |= 1 << TR_ardebug;
#endif /* defined(AR_DEBUG) */
	makeFileSys();
	makeMediaParams();
	DeviceConfig(AP_archiver);
	VolumeConfig();

	/*
	 * Read the archiver command file.
	 */
	ReadCmds(cmd_fname);
	PrintInfo(fsname);
	if (!Daemon) {
		if (ListOptions == LO_conf) {
			checkVolAvail();
		}
		return (EXIT_SUCCESS);
	}

	/*
	 * Copy data from our copy of the archiver daemon state file.
	 */
	memmove(adState->AdNotifyFile, AdState->AdNotifyFile,
	    sizeof (adState->AdNotifyFile));
	adState->AdInterval = AdState->AdInterval;
	setControls();

	/*
	 * Write files we made.
	 */
	writeArchSetFile();
	writeFileProps();
	if (Wait) {
		return (EXIT_WAIT);
	}
	return (EXIT_SUCCESS);
}


/* Private functions. */


/*
 * Check volume availablity.
 */
static void
checkVolAvail(void)
{
	int	noVsnsFound;
	int	asn;

	noVsnsFound = 0;

	/*
	 * Check for volume availability.
	 */
	for (asn = ALLSETS_MAX; asn < ArchSetNumof; asn++) {
		struct ArchSet *as;
		struct VolInfo vi;

		as = &ArchSetTable[asn];
		if (!(as->AsCflags & AC_needVSNs) ||
		    (as->AsFlags & AS_diskArchSet) ||
		    as->AsVsnDesc == VD_none) {
			continue;
		}
		if (GetRmArchiveVol(as, 0, NULL, NULL, &vi) == -1) {
			noVsnsFound++;
		}
	}
	if (noVsnsFound != 0) {
		if (noVsnsFound == 1) {
			/* 1 archive set has no volumes available */
			SendCustMsg(HERE, 4472);
		} else {
			/* %d archive sets have no volumes available */
			SendCustMsg(HERE, 4473, noVsnsFound);
		}
	}
}


/*
 * Make file system table.
 */
static void
makeFileSys(void)
{
	struct sam_fs_status *fsarray;
	size_t	size;
	int	i;

	if ((i = GetFsStatus(&fsarray)) == -1) {
		if (!Daemon) {
			SendCustMsg(HERE, 4006);
			exit(EXIT_FAILURE);
		}
		LibFatal(GetFsStatus, "");
	}
	if (i == 0) {
		SendCustMsg(HERE, 4006);
		exit(EXIT_NORESTART);
	}
	size = sizeof (struct FileSys) + ((i - 1) *
	    sizeof (struct FileSysEntry));
	SamMalloc(FileSysTable, size);
	memset(FileSysTable, 0, size);
	FileSysTable->count = i;
	for (i = 0; i < FileSysTable->count; i++) {
		struct sam_fs_info fi;
		struct FileSysEntry *fs;

		fs = &FileSysTable->entry[i];
		strncpy(fs->FsName, fsarray[i].fs_name,
		    sizeof (fs->FsName)-1);
		fs->FsFlags = 0;
		if (GetFsInfo(fs->FsName, &fi) == -1) {
			LibFatal(GetFsInfo, fs->FsName);
		}
		if (!(fi.fi_config & MT_SAM_ENABLED)) {
			fs->FsFlags |= FS_noarchive;
		}
		if (!(fi.fi_config & MT_ARCHIVE_SCAN)) {
			fs->FsFlags |= FS_noarchive | FS_noarfind;
		}
		if (fi.fi_config & MT_SHARED_READER) {
			fs->FsFlags |= FS_noarchive | FS_share_reader;
		}
		if (fi.fi_status & FS_CLIENT) {
			fs->FsFlags |= FS_noarchive | FS_share_client;
		}

		fs->FsBackGndInterval = BACKGROUND_SCAN_INTERVAL;
		fs->FsBackGndTime = BACKGROUND_SCAN_TIME;
		fs->FsInterval = SCAN_INTERVAL;
		fs->FsExamine = EM_noscan;
		fs->FsMaxpartial = fi.fi_maxpartial;
	}
	free(fsarray);
}


/*
 * Make the media parameters table.
 */
static void
makeMediaParams(void)
{
	sam_defaults_t *defaults;
	struct MediaParamsEntry *mp;
	size_t	size;
	int	i;

	defaults = GetDefaults();

	/*
	 * Count entries in the device name table.
	 */
	for (i = 0; dev_nm2mt[i].nm != NULL; i++) {
		;
	}
	size = sizeof (struct MediaParams) + i *
	    sizeof (struct MediaParamsEntry);
	SamMalloc(MediaParams, size);
	memset(MediaParams, 0, size);
	MediaParams->MpReadTimeout = READ_TIMEOUT;
	MediaParams->MpRequestTimeout = REQUEST_TIMEOUT;
	MediaParams->MpStageTimeout = STAGE_TIMEOUT;
	MediaParams->MpCount = i + 1;

	/*
	 * Fill in each entry.
	 * Start with the entry for default media used for the case
	 * when no media is defined for an archive set.
	 * Continue with all the media entries in dev_nm2mt[];
	 */
	mp = &MediaParams->MpEntry[0];
	ASSERT(sizeof (mp->MpMtype) > strlen("??"));
	strncpy(mp->MpMtype, "??", sizeof (mp->MpMtype));
	for (i = 1; i < MediaParams->MpCount; i++) {
		dev_nm_t *dp;

		mp = &MediaParams->MpEntry[i];
		dp = &dev_nm2mt[i - 1];
		ASSERT(sizeof (mp->MpMtype) > strlen(dp->nm));
		strncpy(mp->MpMtype, dp->nm, sizeof (mp->MpMtype));
		mp->MpType = dp->dt;
		mp->MpBufsize = 4;
		mp->MpTimeout = WRITE_TIMEOUT;

		/*
		 * Set media for the site defaults.
		 */
		if (mp->MpType == DT_OPTICAL) {
			mp->MpType	= defaults->optical;
			mp->MpFlags	|= MP_default;
		} else if (mp->MpType == DT_TAPE) {
			mp->MpType	= defaults->tape;
			mp->MpFlags	|= MP_default;
		}

		/*
		 * Set default archmax based on media class.
		 */
		if (is_disk(mp->MpType)) {
			mp->MpArchmax = DKARCHMAX;
			if (is_stk5800(mp->MpType)) {
				mp->MpArchmax = CBARCHMAX;
			}
		} else if (is_tape(mp->MpType)) {

			switch (mp->MpType) {

			case DT_TITAN:
				mp->MpArchmax = (fsize_t)TIARCHMAX;
				break;
			case DT_IBM3580:
				mp->MpArchmax = (fsize_t)LIARCHMAX;
				break;
			case DT_3592:
				mp->MpArchmax = (fsize_t)M2ARCHMAX;
				break;
			case DT_9940:
				mp->MpArchmax = (fsize_t)SFARCHMAX;
				break;
			case DT_9840:
				mp->MpArchmax = (fsize_t)SGARCHMAX;
				break;
			case DT_LINEAR_TAPE:
				mp->MpArchmax = (fsize_t)LTARCHMAX;
				break;
			defaults:
				mp->MpArchmax = TPARCHMAX;

			}
		} else {
			mp->MpArchmax = ODARCHMAX;
		}
	}
}


/*
 * Set controls.
 */
static void
setControls(void)
{
	int	i;

	/*
	 * Set archive drives allowed in each library.
	 */
	for (i = 0; i < ArchLibTable->count; i++) {
		struct ArchLibEntry *al;
		char	*p;
		char	value[32];

		al = &ArchLibTable->entry[i];
		snprintf(ScrPath, sizeof (ScrPath),
		    "library.%s.drives", al->AlName);
		snprintf(value, sizeof (value), "%d", al->AlDrivesAllow);
		p = ArchiverControl(ScrPath, value, NULL, 0);
		if (*p != 0) {
			Trace(TR_ERR, p);
		}
	}
}


/*
 * Write archive set file.
 */
static void
writeArchSetFile(void)
{
	static char *fname = ARCHIVER_DIR"/"ARCHIVE_SETS;
	struct ArchSetFileHdr afh, *oldAfh;
	size_t	nextoffset;
	size_t	size;
	size_t	sizeMp;
	char	*fileMsg;
	int	fd;
	int	i;

	/*
	 * Include allsets flags in all Archive Sets.
	 */
	for (i = 1; i < ArchSetNumof; i++) {
		struct ArchSet *asd;
		struct ArchSet *as;

		as = &ArchSetTable[i];
		if (i < ALLSETS_MAX || strchr(as->AsName, '.') == NULL) {
			asd = &ArchSetTable[0];
		} else {
			asd = &ArchSetTable[1 + AS_COPY(as->AsName)];
		}
		as->AsFlags		|= asd->AsFlags;
		as->AsPrFlags	|= asd->AsPrFlags;
		as->AsRyFlags	|= asd->AsRyFlags;
	}
	/*
	 * Note where all tables are.
	 */
	nextoffset = STRUCT_RND(sizeof (afh));
	afh.ArchSetTable = nextoffset;
	afh.ArchSetNumof = ArchSetNumof;
	nextoffset += STRUCT_RND(ArchSetNumof * sizeof (struct ArchSet));

	afh.VsnExpTable = nextoffset;
	afh.VsnExpSize = VsnExpSize;
	nextoffset += STRUCT_RND(VsnExpSize);

	afh.VsnListTable = nextoffset;
	afh.VsnListSize = VsnListSize;
	nextoffset += STRUCT_RND(VsnListSize);

	afh.VsnPoolTable = nextoffset;
	afh.VsnPoolSize = VsnPoolSize;
	nextoffset += STRUCT_RND(VsnPoolSize);

	afh.AsMediaParams = nextoffset;
	sizeMp = sizeof (struct MediaParams) +
	    (MediaParams->MpCount - 1) * sizeof (struct MediaParamsEntry);
	nextoffset += sizeMp;

	afh.As.MfMagic	= ARCHSETS_MAGIC;
	afh.As.MfLen	= nextoffset;
	afh.As.MfValid	= 1;
	afh.AsVersion	= ARCHSETS_VERSION;

	/*
	 * Check existing file.
	 */
	oldAfh = ArMapFileAttach(fname, ARCHSETS_MAGIC, O_RDWR);
	fileMsg = "created";
	if (oldAfh != NULL) {
		char	*as, *ve, *vl, *vp, *mp;

		as = (char *)oldAfh + oldAfh->ArchSetTable;
		ve = (char *)oldAfh + oldAfh->VsnExpTable;
		vl = (char *)oldAfh + oldAfh->VsnListTable;
		vp = (char *)oldAfh + oldAfh->VsnPoolTable;
		mp = (char *)oldAfh + oldAfh->AsMediaParams;
		if (oldAfh->As.MfLen != afh.As.MfLen) {
			fileMsg = "header";
		} else if (oldAfh->ArchSetNumof != ArchSetNumof ||
		    memcmp(as, ArchSetTable, ArchSetNumof *
		    sizeof (struct ArchSet)) != 0) {
			fileMsg = "ArchSet";
		} else if (oldAfh->VsnExpSize != VsnExpSize ||
				memcmp(ve, VsnExpTable, VsnExpSize) != 0) {
			fileMsg = "VsnExp";
		} else if (oldAfh->VsnListSize != VsnListSize ||
		    memcmp(vl, VsnListTable, VsnListSize) != 0) {
			fileMsg = "VsnList";
		} else if (oldAfh->VsnPoolSize != VsnPoolSize ||
		    memcmp(vp, VsnPoolTable, VsnPoolSize) != 0) {
			fileMsg = "VsnPoolTable";
		} else if (memcmp(mp, MediaParams, sizeMp) != 0) {
			fileMsg = "MediaParams";
		} else {
			Trace(TR_ARDEBUG, ARCHIVE_SETS" unchanged");
			fileMsg = NULL;
		}
		if (fileMsg != NULL) {
			Trace(TR_ARDEBUG, "  %s", fileMsg);
			/*
			 * Invalidate existing file.
			 */
			(void) unlink(fname);
			oldAfh->As.MfValid = 0;
			fileMsg = "changed";
		}
		(void) ArMapFileDetach(oldAfh);
	}

	if (fileMsg != NULL) {
		/*
		 * Create the file.
		 */
		fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
		if (fd == -1) {
			LibFatal(open-CREAT, fname);
		}

		/*
		 * Write the header and the tables.
		 */
		size = STRUCT_RND(sizeof (afh));
		if (write(fd, &afh, size) != size) {
			LibFatal(write, ARCHIVE_SETS);
		}
		size = STRUCT_RND(ArchSetNumof * sizeof (struct ArchSet));
		if (write(fd, ArchSetTable, size) != size) {
			LibFatal(write, ARCHIVE_SETS);
		}
		if (VsnExpSize != 0) {
			size = STRUCT_RND(VsnExpSize);
			if (write(fd, VsnExpTable, size) != size) {
				LibFatal(write, ARCHIVE_SETS);
			}
		}
		if (VsnListSize != 0) {
			size = STRUCT_RND(VsnListSize);
			if (write(fd, VsnListTable, size) != size) {
				LibFatal(write, ARCHIVE_SETS);
			}
		}
		if (VsnPoolSize != 0) {
			size = STRUCT_RND(VsnPoolSize);
			if (write(fd, VsnPoolTable, size) != size) {
				LibFatal(write, ARCHIVE_SETS);
			}
		}
		if (write(fd, MediaParams, sizeMp) != sizeMp) {
			LibFatal(write, ARCHIVE_SETS);
		}

		if (close(fd) == -1) {
			LibFatal(close, ARCHIVE_SETS);
		}
		Trace(TR_MISC, ARCHIVE_SETS" %s", fileMsg);
	}

	/*
	 * Clear out the existing tables.
	 */
	SamFree(ArchSetTable);
	ArchSetTable = NULL;
	ArchSetNumof = 0;
	if (VsnExpTable != NULL) {
		SamFree(VsnExpTable);
	}
	VsnExpTable = NULL;
	VsnExpSize = 0;
	if (VsnListTable != NULL) {
		SamFree(VsnListTable);
	}
	VsnListTable = NULL;
	VsnListSize = 0;
	if (VsnPoolTable != NULL) {
		SamFree(VsnPoolTable);
	}
	VsnPoolTable = NULL;
	VsnPoolSize = 0;
	SamFree(MediaParams);
	if (ArchSetAttach(O_RDONLY) == NULL) {
		LibFatal(ArMapFileAttach,  ARCHIVER_DIR"/"ARCHIVE_SETS);
	}
}


/*
 * Write the file properties tables.
 * Enter 'wait' state, interval, logfile name and mode in state file.
 */
static void
writeFileProps(void)
{
	int	i;

	for (i = 0; i < FileSysTable->count; i++) {
		struct ArfindState *state;
		struct FileSysEntry *fs;
		struct FileProps *fp, *fpOld;
		boolean_t changed;
		uint_t	flags;

		fs = &FileSysTable->entry[i];
		if (fs->FsFlags & FS_noarchive) {
			continue;
		}

		/*
		 * Check the state file.
		 */
		snprintf(ScrPath, sizeof (ScrPath), "%s/"ARFIND_STATE,
		    fs->FsName);
		state = ArMapFileAttach(ScrPath, AF_MAGIC, O_RDWR);
		if (state == NULL) {
			LibFatal(ArMapFileAttach, ScrPath);
		}
		changed = FALSE;
		if (FirstRead) {
			if (Wait) {
				state->AfExec = ES_wait;
			} else {
				state->AfExec = ES_run;
			}
			if (fs->FsFlags & FS_wait) {
				state->AfExec = ES_fs_wait;
			}
		}
		if (state->AfBackGndInterval != fs->FsBackGndInterval) {
			changed = TRUE;
			state->AfBackGndInterval = fs->FsBackGndInterval;
		}
		if (state->AfBackGndTime != fs->FsBackGndTime) {
			changed = TRUE;
			state->AfBackGndTime = fs->FsBackGndTime;
		}
		if (state->AfInterval != fs->FsInterval) {
			changed = TRUE;
			state->AfInterval = fs->FsInterval;
		}
		if (strcmp(state->AfLogFile, fs->FsLogFile) != 0) {
			changed = TRUE;
			strncpy(state->AfLogFile, fs->FsLogFile,
			    sizeof (state->AfLogFile)-1);
		}
		if (state->AfExamine != fs->FsExamine) {
			changed = TRUE;
			state->AfExamine = fs->FsExamine;
		}
		flags = 0;
		if (fs->FsFlags & FS_archivemeta) {
			flags |= ASF_archivemeta;
		}
		if (fs->FsFlags & FS_scanlist) {
			flags |= ASF_scanlist;
		}
		if (fs->FsFlags & FS_setarchdone) {
			flags |= ASF_setarchdone;
		}
		if (state->AfExamine != EM_scandirs &&
		    state->AfExamine != EM_noscan) {
			flags |= ASF_setarchdone;
		}
		if (state->AfFlags != flags) {
			changed = TRUE;
			state->AfFlags = flags;
		}
		if (changed) {
			Trace(TR_ARDEBUG, "%s changed", ScrPath);
		}

		/*
		 * Check the previous file properties.
		 */
		snprintf(ScrPath, sizeof (ScrPath), "%s/"FILE_PROPS,
		    fs->FsName);
		fpOld = ArMapFileAttach(ScrPath, FP_MAGIC, O_RDWR);

		fp = FilesysProps[i];
		changed = FALSE;
		if (fpOld != NULL) {
			if (fpOld->FpVersion != FP_VERSION ||
			    fpOld->FpCount != fp->FpCount) {
				changed = TRUE;
			} else {
				size_t size;
				int	j;

				/*
				 * Check file properties entries.  Compare
				 * all but the part of the entry set by arfind.
				 */
				size = offsetof(struct FilePropsEntry,
				    FpArfind);
				for (j = 0; j < fp->FpCount; j++) {
					if (memcmp(&fpOld->FpEntry[j],
					    &fp->FpEntry[j], size) != 0) {
						changed = TRUE;
						break;
					}
				}
			}
			if (changed) {
				if (unlink(ScrPath) == -1) {
					LibFatal(unlink, ScrPath);
				}
				fpOld->Fp.MfValid = 0;
			}
			(void) ArMapFileDetach(fpOld);
		}
		if (changed || fpOld == NULL) {
			int	fd;

			/*
			 * Write new file.
			 */
			fp->Fp.MfMagic	= FP_MAGIC;
			fp->Fp.MfLen	=
			    STRUCT_RND(sizeof (struct FileProps)) +
			    (fp->FpCount - 1) *
			    STRUCT_RND(sizeof (struct FilePropsEntry));
			fp->Fp.MfValid	= 1;
			fp->FpVersion = FP_VERSION;
			fd = open(ScrPath, O_WRONLY | O_CREAT | O_TRUNC,
			    FILE_MODE);
			if (fd < 0) {
				LibFatal(open-CREAT, ScrPath);
			}
			if (write(fd, fp, fp->Fp.MfLen) != fp->Fp.MfLen) {
				LibFatal(write, ScrPath);
			}
			if (close(fd) == -1) {
				LibFatal(close, ScrPath);
			}
			if (changed) {
				Trace(TR_ARDEBUG, "%s changed", ScrPath);
			} else {
				Trace(TR_ARDEBUG, "%s created", ScrPath);
			}
		}
		(void) ArMapFileDetach(state);
	}
}
