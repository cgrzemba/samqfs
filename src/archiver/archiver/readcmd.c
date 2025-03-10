/*
 * readcmd.c - Read commands file.
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.117 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

/* Solaris headers. */
#include <libgen.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/sam_trace.h"

/* Local headers. */
#define	NEED_ARCHSET_NAMES
#define	NEED_EXAM_METHOD_NAMES
#define	NEED_FILEPROPS_NAMES
#include "archiver.h"
#include "archset.h"
#include "device.h"
#include "fileprops.h"
#include "volume.h"

/* Directive functions. */
static void dirArchmax(void);
static void dirArchmeta(void);
static void dirBackGndInterval(void);
static void dirBackGndTime(void);
static void dirBufsize(void);
static void dirDirCacheSize(void);
static void dirDrives(void);
static void dirEndparams(void);
static void dirEndvsnpools(void);
static void dirEndvsns(void);
static void dirExamine(void);
static void dirFs(void);
static void dirInterval(void);
static void dirLogfile(void);
static void dirNoBegin(void);
static void dirNotify(void);
static void dirOvflmin(void);
static void dirParams(void);
static void dirScanlist(void);
static void dirSetarchdone(void);
static void dirTimeout(void);
static void dirVsnpools(void);
static void dirVsns(void);
static void dirWait(void);
static void copyNorelease(void);
static void copyRelease(void);
static void notDirective(void);
static void notGlobalDirective(void);
static void paramsDiskArchive(void);
static void paramsInvalid(void);
static void paramsDeprecated(void);
static void paramsSetfield(void);
static void paramsSetFillvsns(void);
static void procParams(void);
static void procVsnpools(void);
static void procVsns(void);
static void propRelease(void);
static void propSetfield(void);
static void propStage(void);

/* Private data. */

/* Global only directives table */
static DirProc_t globalDirectives[] = {
	{ "archmax",	dirArchmax,	DP_value,	4410 },
	{ "bufsize",	dirBufsize,	DP_value,	4410 },
	{ "drives",	dirDrives,	DP_value,	4411 },
	{ "notify",	dirNotify,	DP_value,	4415 },
	{ "ovflmin",	dirOvflmin,	DP_value,	4410 },
	{ "timeout",	dirTimeout,	DP_value,	4410 },
	{ NULL,	notGlobalDirective, DP_other }
};

/* Directives table */
static DirProc_t directives[] = {
	{ "archivemeta",	dirArchmeta,	DP_value,	4542 },
	{ "background_interval", dirBackGndInterval, DP_value,	4548 },
	{ "background_time",	dirBackGndTime,	DP_value,	4549 },
	{ "dircache_size",	dirDirCacheSize, DP_value,	4551 },
	{ "endparams",		dirNoBegin,	DP_set   },
	{ "endvsnpools",	dirNoBegin,	DP_set   },
	{ "endvsns",		dirNoBegin,	DP_set   },
	{ "examine",		dirExamine,	DP_value,	4537 },
	{ "fs",			dirFs,		DP_value,	4412 },
	{ "interval",		dirInterval,	DP_value,	4413 },
	{ "logfile",		dirLogfile,	DP_value,	4414 },
	{ "params",		dirParams,	DP_set   },
	{ "scanlist_squash",	dirScanlist,	DP_value,	4542 },
	{ "setarchdone",	dirSetarchdone,	DP_value,	4542 },
	{ "vsnpools",		dirVsnpools,	DP_set   },
	{ "vsns",		dirVsns,	DP_set   },
	{ "wait",		dirWait,	DP_set   },
	{ NULL,	notDirective, DP_other }
};

static char dirName[TOKEN_SIZE];
static char errMsg[256];
static char token[TOKEN_SIZE];
static boolean_t peekedToken = FALSE; /* Whether next token looked at. */

static struct ArchSet *as;		/* Current archive set */
static set_name_t asname;		/* Current archive set name */
static boolean_t fileRequired = FALSE;
static boolean_t noCmdFile = FALSE;
static boolean_t noDefine = FALSE;	/* Don't define an archive set */
static boolean_t noGlobal = FALSE;	/* Don't allow a global directive */
static char *cmdFname = ARCHIVER_CMD;
static char *noEnd = NULL;		/* Which end statement missing */
static int *vsnDescs;			/* Array of VSN descriptors */
static int vsnDescsNumof;
static int copy;			/* Current copy */
static int errors;
static long linenum;

static struct FileProps *globalProps;	/* For the global values */
static struct FileProps *fpt;		/* For the current file system */
static int fsn;				/* Current file system (-1 = global) */
static struct FilePropsEntry *fpDef;	/* File properties defining */

/* Private functions. */
static void addFileProps(void);
static void addGlobalProps(void);
static void asmAge(uint_t *age, char *name, int max);
static struct MediaParamsEntry *asmMedia(void);
static int asmSetName(void);
static void asmSize(fsize_t *size, char *name);
static int asmVsnExp(char *string);
static void assignFileProps(void);
static void checkRange(char *name, int64_t value, int64_t min, int64_t max);
static void checkVolumes(void);
static void copyAsParams(struct ArchSet *to, struct ArchSet *from);
static void defineCopy(void);
static void defineArchset(char *name);
static int findset(char *name);
static struct VsnPool *findVsnPool(char *poolName);
static void makeDefaultsArchSet(void);
static void msgFunc(char *msg, int lineno, char *line);
static void msgFuncSetField(int code, char *msg);
static char *normalizePath(char *path);
static void packVsnListTable(void);
static boolean_t propsMatch(struct FilePropsEntry *fp1,
		struct FilePropsEntry *fp2);
static void setAsn(void);
static void setDefaultParams(int which);
static void setDefaultVsns(void);
static void setFsFlag(uint_t flag);
static void setDiskArchives(void);
static boolean_t verifyFile(char *file);

static int paramsPrioritySet(void *v, char *value, char *buf, int bufsize);
#define	paramsPriorityTostr NULL
static int paramsReserveSet(void *v, char *value, char *buf, int bufsize);
#define	paramsReserveTostr NULL
#include "aml/archset.hc"

static int propAfterSet(void *v, char *value, char *buf, int bufsize);
#define	propAfterTostr NULL
static int propGroupSet(void *v, char *value, char *buf, int bufsize);
#define	propGroupTostr NULL
static int propNameSet(void *v, char *value, char *buf, int bufsize);
#define	propNameTostr NULL
static int propUserSet(void *v, char *value, char *buf, int bufsize);
#define	propUserTostr NULL
#include "fileprops.hc"


/*
 * Read commands file.
 */
void
ReadCmds(
	char *fname)	/* Command file name.  NULL if using default. */
{
	size_t	size;

	if (fname != NULL) {
		cmdFname = fname;
		fileRequired = TRUE;
	}

	makeDefaultsArchSet();

	/*
	 * Initialize global file properties and file properties for
	 * each filesystem.
	 */
	size = STRUCT_RND(sizeof (struct FileProps));
	SamMalloc(globalProps, size);
	memset(globalProps, 0, size);

	/*
	 * Initialize file properties for each filesystem.
	 * Define the metadata archive set for each filesystem.
	 */
	SamMalloc(FilesysProps,
	    FileSysTable->count * sizeof (struct FileProps *));
	for (fsn = 0; fsn < FileSysTable->count; fsn++) {
		struct FileSysEntry *fs;
		set_name_t name;
		int	i;

		fs = &FileSysTable->entry[fsn];
		SamMalloc(FilesysProps[fsn], size);
		fpt = FilesysProps[fsn];
		memset(fpt, 0, size);
		strncpy(fpt->FpFsname, fs->FsName, sizeof (fpt->FpFsname));
		fpDef = &fpt->FpEntry[0];
		fpDef->FpBaseAsn = findset(fs->FsName);
		for (i = 0; i < MAX_ARCHIVE; i++) {
			fpDef->FpArchAge[i] = ARCH_AGE;
		}
		addFileProps();
		fpDef->FpFlags |= FP_metadata;
		snprintf(name, sizeof (name), "%s.1", fs->FsName);
		(void) findset(name);
	}
	(void) asmVsnExp(".*");	/* Initialize VsnExpTable */

	/*
	 * Begin with global definitions.
	 */
	fpt = globalProps;
	fsn = -1;
	fpDef = NULL;

	/*
	 * Read the command file.
	 */
	errors = ReadCfg(cmdFname, directives, dirName, token, msgFunc);
	if (errors == -1) {
		if (fileRequired) {
			exit(EXIT_ERRORS);
		}
		noCmdFile = TRUE;
		dirWait(); /* If noCmdFile default to wait directive */
		errors = 0;
	}

	assignFileProps();
	setDefaultParams(1);
	setDefaultParams(0);
	setDefaultVsns();
	setDiskArchives();
	checkVolumes();
	if (errors != 0) {
		if (Daemon) {
			exit(EXIT_ERRORS);
		}
		exit(EXIT_FAILURE);
	}
	setAsn();
}


/*
 * Not a directive.
 */
static void
notDirective(void)
{
	if (noGlobal) {
		int	i;

		/*
		 * Check for global directive.
		 */
		for (i = 0; globalDirectives[i].DpName != NULL; i++) {
			if (strcmp(dirName, globalDirectives[i].DpName) == 0) {
				/* '%s' must precede any 'fs =' command */
				ReadCfgError(CustMsg(4439), dirName);
			}
		}
		notGlobalDirective();
	} else {
		ReadCfgLookupDirname(dirName, globalDirectives);
	}
}


/*
 * Not a global directive.
 * Must be an archive set definition, or a copy definition.
 */
static void
notGlobalDirective(void)
{
	if (isalpha(*dirName)) {
		if (ReadCfgGetToken() == 0 || strcmp(token, "=") == 0) {
			/* '%s' is not a valid archiver directive */
			ReadCfgError(CustMsg(4418), dirName);
		}
		/*
		 * An archive set definition.
		 */
		defineArchset(dirName);
	} else if (isdigit(*dirName)) {
		/*
		 * A copy definition.
		 */
		defineCopy();
	} else {
		/* '%s' is not a valid archiver directive */
		ReadCfgError(CustMsg(4418), dirName);
	}
}


/*
 * archmax = media value.
 */
static void
dirArchmax(void)
{
	struct MediaParamsEntry *mp;

	mp = asmMedia();
	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	} else if (mp->MpType == DT_STK5800) {
		/* Invalid option for STK 5800 disk archive */
		ReadCfgError(CustMsg(4547), dirName);
	}
	asmSize(&mp->MpArchmax, dirName);
}


/*
 * archivemeta = [on | off ]
 */
static void
dirArchmeta(void)
{
	setFsFlag(FS_archivemeta);
}


/*
 * background_interval = interval
 */
static void
dirBackGndInterval(void)
{
	uint_t	v;

	asmAge(&v, "background_interval", SCAN_INTERVAL_MAX);
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			FileSysTable->entry[i].FsBackGndInterval = v;
		}
	} else {
		FileSysTable->entry[fsn].FsBackGndInterval = v;
	}
}


/*
 * background_time = hhmm
 */
static void
dirBackGndTime(void)
{
	int	error;
	int	i;
	int	tm[4];

	error = strlen(token) != 4;
	for (i = 0; i < 4; i++) {
		error += !isdigit(token[i]);
		tm[i] = token[i] - '0';
	}
	if (error != 0) {
		/* Invalid background time */
		ReadCfgError(CustMsg(4550));
	}
	tm[0] = 10 * tm[0] + tm[1];
	tm[2] = 10 * tm[2] + tm[3];
	if (tm[2] > 59 || tm[0] > 23) {
		/* Invalid background time */
		ReadCfgError(CustMsg(4550));
	}
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			FileSysTable->entry[i].FsBackGndTime =
			    (tm[0] << 8) | tm[2];
		}
	} else {
		FileSysTable->entry[fsn].FsBackGndTime =
		    (tm[0] << 8) | tm[2];
	}
}


/*
 * bufsize = buffer size value.
 */
static void
dirBufsize(void)
{
	struct MediaParamsEntry *mp;
	char	*p;
	int	val;

	mp = asmMedia();
	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}

	/*
	 * Set bufsize.
	 */
	errno = 0;
	val = strtoll(token, &p, 0);
	if (errno != 0 || *p != '\0') {
		/* Invalid '%s' value '%s' */
		ReadCfgError(CustMsg(14101), dirName, token);
	}
	checkRange(dirName, val, 2, 8192);
	mp->MpBufsize = val;
	if (ReadCfgGetToken() != 0) {
		if (strcmp(token, "lock") == 0) {
			mp->MpFlags |= MP_lockbuf;
		} else {
			/* bufsize option must be 'lock' */
			ReadCfgError(CustMsg(4521));
		}
	}
}

/*
 * dircache_size = directory cache maximum size
 */
static void
dirDirCacheSize(void)
{
	fsize_t val;

	/* Get cache size */
	asmSize(&val, dirName);

	/* Between 8M and 512M */
	checkRange(dirName, val, (8<<20), (512<<20));
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			FileSysTable->entry[i].FsDirCacheSize = (int)val;
		}
	} else {
		FileSysTable->entry[fsn].FsDirCacheSize = (int)val;
	}
}


/*
 * drives = library_name count
 */
static void
dirDrives(void)
{
	struct ArchLibEntry *al = NULL;
	char	*p;
	int	drives;
	int	i;

	/*
	 * Look up library name.
	 */
	for (i = 0; i < ArchLibTable->count; i++) {
		al = &ArchLibTable->entry[i];
		if (strcmp(token, al->AlName) == 0) {
			break;
		}
	}
	if (i >= ArchLibTable->count) {
		ReadCfgError(CustMsg(4444), token);
	}
	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}

	/*
	 * Set drive count.
	 * Must be less than count in the library.
	 */
	p = token;
	errno = 0;
	drives = strtoll(token, &p, 0);
	if (errno != 0 || drives < 0 || p == token) {
		ReadCfgError(CustMsg(4445));
	}
	if (drives > al->AlDrivesNumof &&
	    !(al->AlFlags & (AL_disk | AL_honeycomb))) {
		ReadCfgError(CustMsg(4446), al->AlDrivesNumof);
	}
	al->AlDrivesAllow = drives;
	al->AlFlags |= AL_allow;
}


/*
 * examine = method
 */
static void
dirExamine(void)
{
	ExamMethod_t v;

	for (v = EM_scan; strcmp(token, ExamMethodNames[v]) != 0; v++) {
		if (v == EM_max) {
			/* Invalid %s value ('%s') */
			ReadCfgError(CustMsg(4538), "examine", token);
		}
	}
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			FileSysTable->entry[i].FsExamine = v;
		}
	} else {
		FileSysTable->entry[fsn].FsExamine = v;
	}
}


/*
 * fs = file_system
 */
static void
dirFs(void)
{
	int	n;

	/*
	 * Look up file system.
	 */
	for (n = 0; strcmp(token, FileSysTable->entry[n].FsName) != 0; n++) {
		if (n >= FileSysTable->count) {
			/* Unknown file system */
			ReadCfgError(CustMsg(4449));
		}
	}

	/*
	 * Set new filesystem to process.
	 */
	fsn = n;
	fpt = FilesysProps[fsn];
	fpDef = &fpt->FpEntry[0];
	strncpy(asname, token, sizeof (asname)-1);
	copy = 0;
	noGlobal = TRUE;
}


/*
 * interval = time
 */
static void
dirInterval(void)
{
	uint_t	v;

	asmAge(&v, "interval", SCAN_INTERVAL_MAX);
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			FileSysTable->entry[i].FsInterval = v;
		}
		AdState->AdInterval = v;
	} else {
		FileSysTable->entry[fsn].FsInterval = v;
		AdState->AdInterval = min(AdState->AdInterval, v);
	}
}


/*
 * logfile = file_name
 */
static void
dirLogfile(void)
{
	if (!verifyFile(token)) {
		/* Cannot access %s file %s */
		ReadCfgError(CustMsg(4497), "log", token);
	}

	if (strlen(token) > sizeof (upath_t)-1) {
		/* Logfile name longer than %d characters */
		ReadCfgError(CustMsg(4536), sizeof (upath_t)-1);
	}
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			strncpy(FileSysTable->entry[i].FsLogFile, token,
			    sizeof (FileSysTable->entry[i].FsLogFile)-1);
		}
	} else {
		strncpy(FileSysTable->entry[fsn].FsLogFile, token,
		    sizeof (FileSysTable->entry[fsn].FsLogFile)-1);
	}
}


/*
 * No beginning statement.
 * 'params', 'vsnpools', 'vsns'.
 */
static void
dirNoBegin(void)
{
	/* No preceding '%s' statement */
	ReadCfgError(CustMsg(4447), dirName + 3);
}


/*
 * notify = file_name
 */
static void
dirNotify(void)
{
	strncpy(AdState->AdNotifyFile, token, sizeof (AdState->AdNotifyFile));
}


/*
 * ovflmin = media value.
 */
static void
dirOvflmin(void)
{
	struct MediaParamsEntry *mp;

	mp = asmMedia();
	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}
	asmSize(&mp->MpOvflmin, "ovflmin");
	mp->MpFlags |= MP_volovfl;
}


/*
 * timeout = spec value.
 */
static void
dirTimeout(void)
{
	int	i;
	int	*timeout;
	int	value;
	int	writeTimeout;

	timeout = NULL;
	for (i = 0; Timeouts[i].EeName != NULL; i++) {
		if (strcmp(Timeouts[i].EeName, token) == 0) {
			switch (Timeouts[i].EeValue) {
			case TO_read:
				timeout = &MediaParams->MpReadTimeout;
				break;
			case TO_request:
				timeout = &MediaParams->MpRequestTimeout;
				break;
			case TO_stage:
				timeout = &MediaParams->MpStageTimeout;
				break;
			case TO_write:
				timeout = &writeTimeout;
				break;
			default:
				/* Missing '%s' value */
				ReadCfgError(CustMsg(14008), dirName);
				break;
			}
			break;
		}
	}
	if (timeout == NULL) {
		struct MediaParamsEntry *mp;

		mp = asmMedia();
		timeout = &mp->MpTimeout;
	}
	if (ReadCfgGetToken() == 0) {
		/* Missing '%s' value */
		ReadCfgError(CustMsg(14008), dirName);
	}
	if (StrToInterval(token, &value) == -1) {
		if (errno == ERANGE) {
			/* '%s' value must be less than or equal to %lld */
			ReadCfgError(CustMsg(14104), dirname,
			    (longlong_t)(INT_MAX-1));
		}
		/* Invalid '%s' value ('%s') */
		ReadCfgError(CustMsg(14101), dirname, token);
	}
	if (timeout != &writeTimeout) {
		*timeout = value;
	} else {
		for (i = 0; i < MediaParams->MpCount; i++) {
			MediaParams->MpEntry[i].MpTimeout = value;
		}
	}
}


/*
 * params
 */
static void
dirParams(void)
{
	static DirProc_t table[] = {
		{ "endparams", dirEndparams, DP_set },
		{ "vsnpools", dirVsnpools, DP_other },
		{ "vsns", dirVsns, DP_other },
		{ NULL, procParams, DP_other }
	};
	char	*msg;

	assignFileProps();
	ReadCfgSetTable(table);
	msg = noEnd;
	noEnd = "endparams";
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 * end
 */
static void
dirEndparams(void)
{
	ReadCfgSetTable(directives);
	noEnd = NULL;
}


/*
 * Process params definitions.
 */
static void
procParams(void)
{
/* Archive set parameters table */
static DirProc_t table[] = {
	{ "-disk_archive",	paramsDiskArchive,	DP_param,	4522 },
#if !defined(DEBUG) | defined(lint)
	/* cstyle */ { "-simdelay", paramsInvalid,	DP_other },
	{ "-simeod",		paramsInvalid,		DP_other },
	{ "-simread",		paramsInvalid,		DP_other },
	{ "-tstovfl",		paramsInvalid,		DP_other },
#endif /* !defined(DEBUG) | defined(lint) */
	{ "-join",			paramsDeprecated,	DP_other },
	{ "-fillvsns",		paramsSetFillvsns,  DP_other },
	{ NULL,	paramsSetfield, DP_other }
};
	static boolean_t allsets = TRUE;
	static boolean_t allsetsCopy = TRUE;
	boolean_t paramsDefined;

	if (strcmp(dirName, ALL_SETS) == 0) {
		as = &ArchSetTable[0];
		if (!allsets || !allsetsCopy) {
			/* '%s' parameters must be defined first */
			ReadCfgError(CustMsg(4505), as->AsName);
		}
	} else {
		/* Assemble set name. */
		if (asmSetName() < ALLSETS_MAX) {
			/*
			 * allsets.copy
			 */
			if (!allsetsCopy) {
				/* '%s' parameters must be defined first */
				ReadCfgError(CustMsg(4505), as->AsName);
			}
			if (allsets) {
				allsets = FALSE;
				setDefaultParams(1);
			}
		} else {
			if (allsets || allsetsCopy) {
				allsets = FALSE;
				allsetsCopy = FALSE;
				setDefaultParams(1);
				setDefaultParams(0);
			}
		}
	}
	paramsDefined = FALSE;
	peekedToken = FALSE;
	while (peekedToken || ReadCfgGetToken() != 0) {
		if (peekedToken && token[0] == '\0') {
			break;
		}
		peekedToken = FALSE;
		strncpy(dirName, token, sizeof (dirName)-1);
		ReadCfgLookupDirname(dirName, table);
		if (!allsets && !allsetsCopy &&
		    (as->AsFlags & AS_diskArchSet)) {
			if (as->AsFlags & (AS_rmonly)) {
				ReadCfgError(CustMsg(4540), dirName);
			}
		}
		paramsDefined = TRUE;
	}
	if (!paramsDefined) {
		/* No archive set parameters were defined */
		ReadCfgError(CustMsg(4450));
	}
}


/*
 * scanlist_squash = [on | off ]
 */
static void
dirScanlist(void)
{
	setFsFlag(FS_scanlist);
}


/*
 * setarchdone = [on | off ]
 */
static void
dirSetarchdone(void)
{
	setFsFlag(FS_setarchdone);
}


/*
 * vsnpools
 */
static void
dirVsnpools(void)
{
	static DirProc_t table[] = {
		{ "endvsnpools", dirEndvsnpools, DP_set   },
		{ "params", dirParams, DP_other },
		{ "vsns", dirVsns, DP_other },
		{ NULL, procVsnpools, DP_other }
	};
	char	*msg;

	assignFileProps();
	ReadCfgSetTable(table);
	msg = noEnd;
	noEnd = "endvsnpools";
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 * endvsnpools
 */
static void
dirEndvsnpools(void)
{
	ReadCfgSetTable(directives);
	noEnd = NULL;
}


/*
 * Process vsnpools directives.
 */
static void
procVsnpools(void)
{
	struct VsnPool *vp, vpd;
	size_t	size;

	if (strlen(dirName) > sizeof (vpd.VpName) - 1) {
		/* VSN pool name is too long (max %d) */
		ReadCfgError(CustMsg(4488), sizeof (vpd.VpName) - 1);
	}
	if (findVsnPool(dirName) != NULL) {
		/* Duplicate VSN pool name %s */
		ReadCfgError(CustMsg(4489), dirName);
	}
	memset(&vpd, 0, sizeof (vpd));
	strncpy(vpd.VpName, token, sizeof (vpd.VpName)-1);

	if (ReadCfgGetToken() == 0) {
		/* Media specification missing */
		ReadCfgError(CustMsg(4410));
	}
	(void) asmMedia();	/* Validate media */
	strncpy(vpd.VpMtype, token, sizeof (vpd.VpMtype)-1);
	/* Assemble at least one VSN. */
	vpd.VpNumof = 0;
	while (ReadCfgGetToken() != 0) {
		if (vpd.VpNumof >= vsnDescsNumof) {
			vsnDescsNumof += 10;
			size = vsnDescsNumof * sizeof (int);
			SamRealloc(vsnDescs, size);
		}
		vsnDescs[vpd.VpNumof] = asmVsnExp(token);
		vpd.VpNumof++;
	}
	if (vpd.VpNumof == 0) {
		/* VSN specification missing */
		ReadCfgError(CustMsg(4453));
	}

	/*
	 * Add VSN pool entry to table.
	 */
	size = VsnPoolSize +
	    sizeof (struct VsnPool) + (vpd.VpNumof - 1) * sizeof (vpd.VpVsnExp);
	SamRealloc(VsnPoolTable, size);
	vp = (struct VsnPool *)(void *)((char *)VsnPoolTable + VsnPoolSize);
	memmove(vp, &vpd, sizeof (vpd));
	memmove(vp->VpVsnExp, vsnDescs, vpd.VpNumof * sizeof (int));
	VsnPoolSize = size;
}


/*
 * vsns
 */
static void
dirVsns(void)
{
	static DirProc_t table[] = {
		{ "endvsns", dirEndvsns, DP_set   },
		{ "params", dirParams, DP_other },
		{ "vsnpools", dirVsns, DP_other },
		{ NULL, procVsns, DP_other }
	};
	char	*msg;

	assignFileProps();
	ReadCfgSetTable(table);
	msg = noEnd;
	noEnd = "endvsns";
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}


/*
 * endvsns
 */
static void
dirEndvsns(void)
{
	packVsnListTable();
	ReadCfgSetTable(directives);
	noEnd = NULL;
}


/*
 * Process vsns.
 */
static void
procVsns(void)
{
	static boolean_t allsets = TRUE;
	static boolean_t allsetsCopy = TRUE;
	struct VsnList *vl;
	mtype_t	mtype;
	int	num_done;
	int	vdn;


	if (strcmp(dirName, ALL_SETS) == 0) {
		as = &ArchSetTable[0];
		if (!allsets || !allsetsCopy) {
			/* '%s' parameters must be defined first */
			ReadCfgError(CustMsg(4505), as->AsName);
		}
	} else {
		/* Assemble set name. */
		if (asmSetName() < ALLSETS_MAX) {
			/*
			 * allsets.copy
			 */
			if (!allsetsCopy) {
				/* '%s' parameters must be defined first */
				ReadCfgError(CustMsg(4505), as->AsName);
			}
			if (allsets) {
				allsets = FALSE;
			}
		} else {
			if (allsets || allsetsCopy) {
				allsets = FALSE;
				allsetsCopy = FALSE;
			}
		}
	}

	if (ReadCfgGetToken() == 0) {
		/* Media specification missing */
		ReadCfgError(CustMsg(4410));
	}
	(void) asmMedia();	/* Validate media */
	strncpy(mtype, token, sizeof (mtype));

	if (strcmp(mtype, "dk") == 0) {
		as->AsFlags |= AS_disk_archive;
	} else if (strcmp(mtype, "cb") == 0) {
		as->AsFlags |= AS_honeycomb;
	}

	if (as->AsVsnDesc != VD_none) {
		/*
		 * Previous VSNs command for this archive set.
		 */
		if (strcmp(as->AsMtype, mtype) != 0) {
			/* Different media previously specified */
			ReadCfgError(CustMsg(4493));
		}

		/*
		 * Start with previous definition.
		 * (Since the list was made before, vsnDescs can hold it).
		 */
		if ((VD_mask & as->AsVsnDesc) == VD_list) {
			vl = (struct VsnList *)(void *)((char *)VsnListTable +
			    (as->AsVsnDesc & ~VD_mask));
			vdn = vl->VlNumof;
			memmove(vsnDescs, vl->VlDesc, vdn * sizeof (vsndesc_t));
		} else {
			vdn = 1;
			vsnDescs[0] = as->AsVsnDesc;
		}
	} else {
		vdn = 0;
	}

	/* Assemble at least one VSN. */
	num_done = 0;
	while (ReadCfgGetToken() != 0) {
		if (vdn >= vsnDescsNumof) {
			vsnDescsNumof += 10;
			SamRealloc(vsnDescs, vsnDescsNumof * sizeof (int));
		}
		if (strcmp(token, "-pool") == 0) {
			struct VsnPool *vp;

			if (ReadCfgGetToken() == 0) {
				/* VSN pool name missing */
				ReadCfgError(CustMsg(4490));
			}
			if ((vp = findVsnPool(token)) == NULL) {
				/* VSN pool %s not defined */
				ReadCfgError(CustMsg(4491), token);
			}
			if (strcmp(vp->VpMtype, mtype) != 0) {
				/* Pool media does not match archive set */
				ReadCfgError(CustMsg(4492));
			}
			vsnDescs[vdn] = VD_pool | Ptrdiff(vp, VsnPoolTable);
		} else {
			vsnDescs[vdn] = VD_exp | asmVsnExp(token);
		}
		vdn++;
		num_done++;
	}
	if (num_done == 0) {
		/* VSN specification missing */
		ReadCfgError(CustMsg(4453));
	}
	if (vdn == 1) {
		/* Enter the single VSN descriptor. */
		as->AsVsnDesc = vsnDescs[0];
	} else {
		size_t	size;

		/*
		 * Add VSN descriptor list entry to table.
		 */
		size = VsnListSize +
		    sizeof (struct VsnList) + (vdn - 1) * sizeof (vsndesc_t);
		SamRealloc(VsnListTable, size);
		vl = (struct VsnList *)(void *)((char *)VsnListTable +
		    VsnListSize);
		vl->VlNumof = vdn;
		memmove(vl->VlDesc, vsnDescs, vdn * sizeof (vsndesc_t));
		as->AsVsnDesc = VD_list | VsnListSize;
		VsnListSize = size;
	}
	as->AsCflags |= AC_needVSNs;
	strncpy(as->AsMtype, mtype, sizeof (as->AsMtype)-1);
}


/*
 * wait
 */
static void
dirWait(void)
{
	if (FirstRead) {
		if (fsn == -1) {
			Wait = TRUE;
		} else {
			FileSysTable->entry[fsn].FsFlags |= FS_wait;
		}
	}
}


/*
 * Copy option -norelease.
 */
static void
copyNorelease(void)
{
	fpDef->FpCopiesNorel |= 1 << copy;
}


/*
 * Copy option -release.
 */
static void
copyRelease(void)
{
	fpDef->FpCopiesRel |= 1 << copy;
}


/*
 * Define an archive set.
 */
static void
defineArchset(
	char *name)
{
	/* File properties table */
	static DirProc_t table[] = {
		{ "-release",	propRelease,	DP_param,	4475 },
		{ "-stage",	propStage,	DP_param,	4476 },
		{ NULL,	propSetfield, DP_other }
	};
	char	*p;
	int	i;

	/*
	 * Validate archive set name.
	 */
	i = sizeof (as->AsName) - 4;
	if ((int)strlen(name) > i) {
		/* Archive set name is too long (max %d) */
		ReadCfgError(CustMsg(4454), i);
	}
	p = name;
	while (isalnum(*p) || *p == '_') {
		p++;
	}
	if (*p != '\0') {
		/* Invalid archive set name */
		ReadCfgError(CustMsg(4432));
	}
	strncpy(asname, name, sizeof (asname)-1);

	/*
	 * Set defaults in file properties.
	 */
	fpDef = &fpt->FpEntry[fpt->FpCount];
	memset(fpDef, 0, STRUCT_RND(sizeof (struct FilePropsEntry)));
	fpDef->FpBaseAsn = findset(asname);
	fpDef->FpLineno = (void *)linenum;
	if (strcmp(asname, NO_ARCHIVE) != 0) {
		for (i = 0; i < MAX_ARCHIVE; i++) {
			fpDef->FpArchAge[i] = ARCH_AGE;
		}
	} else {
		fpDef->FpFlags |= FP_noarch;
	}
	copy = 0;

	/*
	 * Assemble path.
	 */
	if (*token == '\0') {
		/* Path missing */
		ReadCfgError(CustMsg(4455));
	}
	if (*token == '/') {
		/* Path must be relative */
		ReadCfgError(CustMsg(4456));
	}
	if (normalizePath(token) == NULL) {
		/* Invalid path */
		ReadCfgError(CustMsg(4457));
	}
	strncpy(fpDef->FpPath, token, sizeof (fpDef->FpPath));
	while (ReadCfgGetToken() != 0) {
		strncpy(dirName, token, sizeof (dirName)-1);
		ReadCfgLookupDirname(dirName, table);
	}

	/*
	 * Assure that there is no duplicate definition.
	 * Skip the metadata entry.
	 */
	for (i = 1; i < fpt->FpCount; i++) {
		if (propsMatch(&fpt->FpEntry[i], fpDef)) {
			/* File properties duplicate those in line %d */
			ReadCfgError(CustMsg(4458),
			    (long)fpt->FpEntry[i].FpLineno);
		}
	}
	addFileProps();
}


/*
 * Define copy for the current archive set.
 */
static void
defineCopy(void)
{
	/* Copy options table */
	static DirProc_t table[] = {
		{ "-norelease",	copyNorelease,	DP_other },
		{ "-release",	copyRelease,	DP_other },
		{ NULL,	NULL, CustMsg(4474) }
	};
	set_name_t name;

	/*
	 * Check for file properties being defined.
	 * Not a "NoArchive" archive set.
	 * Number in range 1 - MAX_ARCHIVE.
	 * Copy not previously defined.
	 */
	if (fpDef == NULL) {
		/*  No file properties defined */
		ReadCfgError(CustMsg(4427));
	}
	if (fpDef->FpFlags & FP_noarch) {
		/* Copy not allowed for '%s' */
		ReadCfgError(CustMsg(4428), NO_ARCHIVE);
	}

	if ((int)strlen(dirName) > 1) {
		/* Invalid archive copy number */
		ReadCfgError(CustMsg(4434));
	}
	copy = *dirName - '1';
	if (copy < 0 || copy >= MAX_ARCHIVE) {
		/* Archive copy number must be 1 <= n <= %d */
		ReadCfgError(CustMsg(4435), MAX_ARCHIVE);
	}
	if (fpDef->FpCopiesReq & (1 << copy)) {
		/* Duplicate archive copy number */
		ReadCfgError(CustMsg(4429));
	}
	snprintf(name, sizeof (name), "%s.%d", asname, copy + 1);

	/*
	 * Define the copy.
	 */
	(void) findset(name);
	fpDef->FpCopiesReq |= 1 << copy;
	if (ReadCfgGetToken() == 0) {
		return;
	}

	/*
	 * Process copy options.
	 */
	while (*token == '-') {
		strncpy(dirName, token, sizeof (dirName)-1);
		ReadCfgLookupDirname(dirName, table);
		if (ReadCfgGetToken() == 0) {
			return;
		}
	}

	/* Archive and unarchive age. */
	asmAge(&fpDef->FpArchAge[copy], "archive age", INT_MAX);
	if (ReadCfgGetToken() == 0) {
		return;
	}
	asmAge(&fpDef->FpUnarchAge[copy], "unarchive age", INT_MAX);
	fpDef->FpCopiesUnarch |= 1 << copy;
	if (ReadCfgGetToken() == 0) {
		return;
	}
	/* Too many age parameters */
	ReadCfgError(CustMsg(4438));
}


/*
 * Process 'after' for SetFieldValue().
 */
static int
propAfterSet(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	struct tm tm;
	time_t	*tv = (time_t *)v;
	char	*p;

	p = strchr(value, 'T');
	if (p == NULL) {
		p = strptime(value, "%Y-%m-%d", &tm);
	} else {
		p = strptime(value, "%Y-%m-%dT%T", &tm);
	}
	if (*p != 0 && *p != 'Z') {
		/* Invalid %s value */
		snprintf(buf, bufsize, GetCustMsg(4903), "after");
		errno = EINVAL;
		return (-1);
	}
	tm.tm_isdst = -1;
	*tv = mktime(&tm);
	if (*p == 'Z') {
		extern time_t timezone, altzone;

		if (tm.tm_isdst == 0) {
			*tv += timezone;
		} else {
			*tv += altzone;
		}
	}
	return (0);
}


/*
 * Process 'group' for SetFieldValue().
 */
static int
propGroupSet(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	struct group *g;
	gid_t	*gid = (gid_t *)v;

	if ((g = getgrnam(value)) == NULL) {
		/* Group name not in group file */
		snprintf(buf, bufsize, GetCustMsg(4464));
		errno = EINVAL;
		return (-1);
	}
	*gid = g->gr_gid;
	return (0);
}


/*
 * Process 'name' for SetFieldValue().
 */
static int
propNameSet(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	char	*val = (char *)v;
	char	*regexp;
	int	l;

	if ((regexp = regcmp(value, NULL)) == NULL) {
		/* Incorrect file name expression */
		snprintf(buf, bufsize, GetCustMsg(4460));
		errno = EINVAL;
		return (-1);
	}
	free(regexp);

	/*
	 * Copy token string to result field.
	 */
	l = strlen(token) + 1;
	if (l > sizeof (fpDef->FpName)) {
		/* Name longer than %d characters */
		snprintf(buf, bufsize, GetCustMsg(4531),
		    sizeof (fpDef->FpName));
		errno = EINVAL;
		return (-1);
	}
	memmove(val, token, l);
	return (0);
}


/*
 * Set release attributes for a file.
 */
static void
propRelease(void)
{
	ino_st_t status;
	ino_st_t mask;
	char	*p;
	int	partial;

	if (fpDef->FpFlags & FP_noarch) {
		/* '%s' not allowed for '%s' */
		ReadCfgError(CustMsg(4440), "-release", NO_ARCHIVE);
	}
	status.bits = 0;
	mask.bits = 0;
	partial = fpDef->FpPartial;
	p = token;
	while (*p != 0) {
		switch (*p) {
		case 'd':
			mask.b.bof_online = 1;
			status.b.bof_online = 0;
			mask.b.release = 1;
			status.b.release = 0;
			mask.b.nodrop = 1;
			status.b.nodrop = 0;
			partial = 0;
			break;
		case 'a':
			mask.b.release = 1;
			status.b.release = 1;
			break;
		case 'n':
			mask.b.nodrop = 1;
			status.b.nodrop = 1;
			break;
		case 'p':
			mask.b.bof_online = 1;
			status.b.bof_online = 1;
			partial = 0;
			break;
		case 's':
			mask.b.bof_online = 1;
			status.b.bof_online = 1;
			partial = 0;
			while (isdigit(*(p+1))) {
				partial = (10 * partial) + *(p+1) - '0';
				p++;
			}
			checkRange("partial_size", partial,
			    SAM_MINPARTIAL, SAM_MAXPARTIAL);
			break;
		default:
			/* Invalid release attribute %c */
			ReadCfgError(CustMsg(4477), *p);
			break;
		}
		p++;
	}
	fpDef->FpMask |= mask.bits;
	fpDef->FpStatus |= status.bits;
	fpDef->FpPartial = partial;
}


/*
 * Set field for file properties.
 */
static void
propSetfield(void)
{
	if (dirName[0] != '-' || dirName[1] == '\0') {
		/* '%s' is not a valid file property */
		ReadCfgError(CustMsg(4426), dirName);
	}
	/*
	 * Try an empty value.
	 */
	*token = '\0';
	if (SetFieldValue(fpDef, FilePropsEntry, dirName+1, token,
	    msgFuncSetField) != 0) {
		if (errno == ENOENT) {
			/* '%s' is not a valid file property */
			ReadCfgError(CustMsg(4426), dirName);
		}
		if (errno != EINVAL) {
			ReadCfgError(0, errMsg);
		}
		/*
		 * Parameter requires a value.
		 */
		(void) ReadCfgGetToken();
		if (SetFieldValue(fpDef, FilePropsEntry, dirName+1, token,
		    msgFuncSetField) != 0) {
			ReadCfgError(0, errMsg);
		}
	}
}


/*
 * Set stage attributes for a file.
 */
static void
propStage(void)
{
	ino_st_t status;
	ino_st_t mask;
	char	*p;

	if (fpDef->FpFlags & FP_noarch) {
		/* '%s' not allowed for '%s' */
		ReadCfgError(CustMsg(4440), "-stage", NO_ARCHIVE);
	}
	status.bits = 0;
	mask.bits = 0;
	p = token;
	while (*p != 0) {
		switch (*p) {
		case 'd':
			mask.b.stage_all = 1;
			status.b.stage_all = 0;
			mask.b.direct = 1;
			status.b.direct = 0;
			break;
		case 'a':
			mask.b.stage_all = 1;
			status.b.stage_all = 1;
			break;
		case 'n':
			mask.b.direct = 1;
			status.b.direct = 1;
			break;
		default:
			/* Invalid stage attribute %c */
			ReadCfgError(CustMsg(4478), *p);
			break;
		}
		p++;
	}
	fpDef->FpMask |= mask.bits;
	fpDef->FpStatus |= status.bits;
}


/*
 * Process 'user' for SetFieldValue().
 */
static int
propUserSet(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	struct passwd *pw;
	uid_t	*uid = (uid_t *)v;

	if ((pw = getpwnam(value)) == NULL) {
		/* User name not in password file */
		snprintf(buf, bufsize, GetCustMsg(4465));
		errno = EINVAL;
		return (-1);
	}
	*uid = pw->pw_uid;
	return (0);
}


/*
 * Add a files properties entry.
 */
static void
addFileProps(void)
{
	size_t	size;

	fpt->FpCount++;
	size = STRUCT_RND(sizeof (struct FileProps)) +
	    fpt->FpCount * STRUCT_RND(sizeof (struct FilePropsEntry));
	if (fsn == -1) {
		SamRealloc(globalProps, size);
		fpt = globalProps;
	} else {
		SamRealloc(FilesysProps[fsn], size);
		fpt = FilesysProps[fsn];
	}
	fpDef = &fpt->FpEntry[fpt->FpCount - 1];
}


/*
 * Add global file properties to a filesystem.
 * Add a global file property to a filesystem's file properties table
 * if it does not match one.
 */
static void
addGlobalProps(void)
{
	int	i;

	for (i = 0; i < globalProps->FpCount; i++) {
		struct FilePropsEntry *fp;
		int	j;

		fp = &globalProps->FpEntry[i];
		for (j = 1; j < fpt->FpCount; j++) {
			if (propsMatch(&fpt->FpEntry[j], fp)) {
				break;
			}
		}
		if (j >= fpt->FpCount) {
			/*
			 * Does not match, add it.
			 */
			memmove(&fpt->FpEntry[fpt->FpCount], fp,
			    STRUCT_RND(sizeof (struct FilePropsEntry)));
			addFileProps();
		}
	}
}


/*
 * Assign file properties.
 * For each file system, add all the global file properties definitions
 * that apply to that file system.
 * This can only be done once.
 */
static void
assignFileProps(void)
{
	if (noDefine) {
		return;
	}

	/*
	 * Add a blank global file properties entry.
	 * This will be used to hold a copy of the meta data entry for default
	 * data file archiving.
	 */
	fpt = globalProps;
	fsn = -1;
	addFileProps();

	/*
	 * Copy global parameters, and all global properties that do not
	 * duplicate locals.
	 * Set default copy if no copies defined.
	 * Determine which Archive Sets will need volumes assigned.
	 */
	for (fsn = 0; fsn < FileSysTable->count; fsn++) {
		struct FilePropsEntry *fp;
		int	i;

		if (FileSysTable->entry[fsn].FsFlags & FS_noarchive) {
			continue;
		}
		fpt = FilesysProps[fsn];

		/*
		 * Fill in the default data entry.
		 * Add all the global properties created.
		 */
		fp = &globalProps->FpEntry[globalProps->FpCount-1];
		memmove(fp, &fpt->FpEntry[0],
		    STRUCT_RND(sizeof (struct FilePropsEntry)));
		fp->FpFlags &= ~FP_metadata;
		fp->FpFlags |= FP_default;
		addGlobalProps();

		/*
		 * Complete all archive sets required.
		 * Determine if VSNs are required.
		 */
		fp = &fpt->FpEntry[0];
		if (!(FileSysTable->entry[fsn].FsFlags & FS_archivemeta)) {
			fp->FpFlags |= FP_noarch;
		}
		for (i = 0; i < fpt->FpCount; i++, fp++) {
			char	*asname;
			int	copy;

			fp->FpPathSize = strlen(fp->FpPath);
			if (fp->FpFlags & FP_noarch) {
				continue;
			}
			if (fp->FpCopiesReq == 0) {
				/*
				 * Define the default copy 1.
				 */
				fp->FpCopiesReq = 1;
			}

			asname = ArchSetTable[fp->FpBaseAsn].AsName;
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if (fp->FpCopiesReq & (1 << copy)) {
					set_name_t name;

					snprintf(name, sizeof (name), "%s.%d",
					    asname, copy + 1);
					(void) findset(name);
					as->AsCflags |= AC_needVSNs;
				}
			}
		}
	}
	noDefine = TRUE;
}


/*
 * Assemble age.
 */
static void
asmAge(
	uint_t *age,
	char *name,
	int max)
{
	int	interval;

	if (StrToInterval(token, &interval) == -1) {
		if (errno == ERANGE) {
			/* '%s' value must be less than or equal to %lld */
			ReadCfgError(CustMsg(14104), name,
			    (longlong_t)(INT_MAX-1));
		}
		/* Invalid '%s' value ('%s') */
		ReadCfgError(CustMsg(14101), name, token);
	}
	checkRange(name, interval, 0, max);
	*age = (uint32_t)interval;
}


/*
 * Assemble media.
 * RETURN: Media parameters entry.
 */
static struct MediaParamsEntry *
asmMedia(void)
{
	struct MediaParamsEntry *mp;

	mp = MediaParamsGetEntry(token);
	if (mp == NULL) {
		/* Media not found. */
		ReadCfgError(CustMsg(4431));
	}
	mp->MpFlags |= MP_refer;
	return (mp);
}


/*
 * Assemble set name.
 */
static int
asmSetName(void)
{
	char	*p;
	int	asn;

	/*
	 * Set name must begin with letter.
	 */
	if (!isalpha(*dirName) || strlen(dirName) > (sizeof (set_name_t)-1)) {
		/* Invalid archive set name */
		ReadCfgError(CustMsg(4432));
	}

	/*
	 * Set copy.
	 */
	if ((p = strchr(dirName, '.')) == NULL) {
		/* Archive copy number missing */
		ReadCfgError(CustMsg(4433));
	}
	p++;
	if (!isdigit(*p)) {
		/* Invalid archive copy number */
		ReadCfgError(CustMsg(4434));
	}
	copy = *p++ - '1';
	if (copy < 0 || copy >= MAX_ARCHIVE) {
		/* Archive copy number must be 1 <= n <= %d */
		ReadCfgError(CustMsg(4435), MAX_ARCHIVE);
	}
	if (!(*p == '\0' || (*p == 'R' && *(p+1) == '\0'))) {
		/* Invalid archive set name */
		ReadCfgError(CustMsg(4432));
	}
	asn = findset(dirName);
	if (asn == -1) {
		struct ArchSet *asd;
		boolean_t noDefineSave;

		if (*p == '\0') {
			/* Archive set %s not defined */
			ReadCfgError(CustMsg(4436), dirName);
		}
		*p = '\0';
		asn = findset(dirName);
		if (asn == -1) {
			/* Archive set %s not defined */
			ReadCfgError(CustMsg(4436), dirName);
		}

		/*
		 * Define the rearch Archive Set.
		 */
		asd = as;
		noDefineSave = noDefine;
		noDefine = FALSE;
		*p = 'R';
		asn = findset(dirName);
		copyAsParams(as, asd);
		noDefine = noDefineSave;
		as->AsCflags |= AC_reArch | AC_needVSNs;
	}
	*(p - 2) = '\0';
	strncpy(asname, dirName, sizeof (asname)-1);
	*(p - 2) = '.';
	return (asn);
}


/*
 * Assemble size.
 * Size string in token.
 */
static void
asmSize(
	fsize_t *size,
	char *name)
{
	uint64_t value;
	int tok_len = strlen(token);

	if (isdigit(token[tok_len - 1])) {
		/* Warning: %s=%s has no fsize unit, defaulting to bytes */
		SendCustMsg(HERE, 4552, name, token);
	}
	if (StrToFsize(token, &value) == -1) {
		/* Invalid '%s' value ('%s') */
		ReadCfgError(CustMsg(14101), name, token);
	}
	*size = value;
}


/*
 * Assemble VSN.
 * token = VSN definition string.
 * RETURN: Offset to expression in VsnExpTable.
 */
static int
asmVsnExp(
	char *string)	/* VSN definition string. */
{
	int regexp_size;
	struct VsnExp *ve;
	size_t	size;
	size_t	sizeEntry;
	char	*RegExp;
	int	vsn_exp;

	RegExp = regcmp(string, NULL);
	if (RegExp == NULL) {
		/* Incorrect VSN expression */
		ReadCfgError(CustMsg(4442));
	}
	regexp_size = strlen(RegExp);

	/*
	 * Lookup expression.
	 * Return offset if found.
	 */
	for (vsn_exp = 0; vsn_exp < VsnExpSize;
		vsn_exp += STRUCT_RND(sizeof (struct VsnExp) + ve->VeExplen)) {
		ve = (struct VsnExp *)(void *)((char *)VsnExpTable + vsn_exp);
		if ((ve->VeExplen - 1) != regexp_size) {
			continue;
		}
		if (memcmp(ve->VeExpbuf, RegExp, regexp_size) == 0) {
			return (vsn_exp);
		}
	}

	/*
	 * Add new expression.
	 */
	vsn_exp = VsnExpSize;
	sizeEntry = STRUCT_RND(sizeof (struct VsnExp) + regexp_size + 1);
	size = VsnExpSize + sizeEntry;
	SamRealloc(VsnExpTable, size);
	ve = (struct VsnExp *)(void *)((char *)VsnExpTable + VsnExpSize);
	memset(ve, 0, sizeEntry);
	ve->VeExplen = regexp_size + 1;
	memmove(ve->VeExpbuf, RegExp, regexp_size);
	VsnExpSize = size;
	free(RegExp);
	return (vsn_exp);
}


/*
 * Check range of a value.
 */
static void
checkRange(
	char *name,
	int64_t value,
	int64_t min,
	int64_t max)
{
	if (value < min && value > max) {
		ReadCfgError(CustMsg(14102), name, min, max);
	}
	if (value < min) {
		ReadCfgError(CustMsg(14103), name, min);
	}
	if (value > max) {
		ReadCfgError(CustMsg(14104), name, max);
	}
}


/*
 * Check volumes.
 */
static void
checkVolumes(void)
{
	struct VolInfo vi;
	int	i;
	int	noVsnsDefined;

	/*
	 * No archiver command file, find first volume available.
	 */
	for (i = 0; /* Terminated inside */; i++) {
		if (GetVolInfo(i, &vi) == -1) {
			/* No media available for default assignment */
			fprintf(stderr, GetCustMsg(4532));
			fprintf(stderr, "\n");
			strncpy(vi.VfMtype, "??", sizeof (vi.VfMtype)-1);
			break;
		}
		if (!(vi.VfFlags & VF_unusable)) {
			break;
		}
	}
	noVsnsDefined = 0;
	for (i = ALLSETS_MAX; i < ArchSetNumof; i++) {
		as = &ArchSetTable[i];

		/*
		 * Check for VSNs defined.
		 * If the archive set is for a noarchive filesystem,
		 * allow no VSNs defined.
		 */
		if (!(as->AsFlags & AS_diskArchSet) &&
		    (as->AsCflags & AC_needVSNs) &&
		    as->AsVsnDesc == VD_none) {
			if (noCmdFile) {
				/*
				 * No archiver.cmd file.
				 * Make all volumes availble.
				 */
				as->AsCflags |= AC_defVSNs;
				strncpy(as->AsMtype, vi.VfMtype,
				    sizeof (vi.VfMtype)-1);
				as->AsVsnDesc = VD_exp | 0;
			} else if (as->AsCflags & AC_reArch) {
				set_name_t name;
				struct ArchSet *asp;

				/*
				 * Assign volumes from the non-rearchive
				 * Archive Set.
				 */
				asp = as;
				strncpy(name, as->AsName, sizeof (name));
				name[strlen(name)-1] = '\0';
				(void) findset(name);
				memmove(asp->AsMtype, as->AsMtype,
				    sizeof (asp->AsMtype));
				asp->AsVsnDesc = as->AsVsnDesc;
				asp->AsCflags |= AC_needVSNs;
			} else {
				/* %s has no volumes defined */
				fprintf(stderr, GetCustMsg(4529), as->AsName);
				fprintf(stderr, "\n");
				noVsnsDefined++;
			}
		}
	}
	if (errors == -1) {
		errors = 0;
	}
	if (noVsnsDefined != 0) {
		if (noVsnsDefined == 1) {
			/* 1 archive set has no volumes defined */
			SendCustMsg(HERE, 4470);
		} else {
			/* %d archive sets have no volumes defined */
			SendCustMsg(HERE, 4471, noVsnsDefined);
		}
		errors++;
	}
}


/*
 * Copy Archive Set parameters.
 */
static void
copyAsParams(
	struct ArchSet *to,
	struct ArchSet *from)
{
	struct ArchSet asSave;

	asSave = *to;
	*to = *from;

	/*
	 * Restore those fields that are not to be set to from's values.
	 */
	memmove(to, from, sizeof (struct ArchSet));
	memmove(to->AsName, asSave.AsName, sizeof (to->AsName));
	memmove(to->AsMtype, asSave.AsMtype, sizeof (to->AsMtype));
	to->AsFlags	= asSave.AsFlags;
	to->AsCflags = asSave.AsCflags;
	to->AsPrFlags = asSave.AsPrFlags;
	to->AsRyFlags = asSave.AsRyFlags;
	to->AsVsnDesc = asSave.AsVsnDesc;
	/*
	 * If honeycomb archive set restore archmax.
	 */
	if (to->AsFlags & AS_honeycomb) {
		to->AsArchmax = asSave.AsArchmax;
	}
	if ((from->AsFlags & AS_disk_archive) && to->AsVsnDesc == VD_none) {
		to->AsFlags |= AS_disk_archive;
		strncpy(to->AsMtype, "dk", sizeof (to->AsMtype)-1);
	}
	if ((from->AsFlags & AS_honeycomb) && to->AsVsnDesc == VD_none) {
		to->AsFlags |= AS_honeycomb;
		strncpy(to->AsMtype, "cb", sizeof (to->AsMtype)-1);
	}
}


/*
 * Find/enter an archive set.
 * Also sets as to set.
 * RETURN: Archive set number.
 */
static int
findset(
	char *name)	/* Archive set name */
{
	size_t	size;
	int	asn;

	for (asn = 0; asn < ArchSetNumof; asn++) {
		as = &ArchSetTable[asn];
		if (strcmp(name, as->AsName) == 0) {
			return (asn);
		}
	}
	if (noDefine) {
		return (-1);
	}

	/*
	 * Allocate new set.
	 */
	size = (ArchSetNumof + 1) * sizeof (struct ArchSet);
	SamRealloc(ArchSetTable, size);
	as = &ArchSetTable[ArchSetNumof];
	memset(as, 0, sizeof (struct ArchSet));
	strncpy(as->AsName, name, sizeof (as->AsName)-1);
	as->AsVsnDesc = VD_none;
	return (ArchSetNumof++);
}


/*
 * Find a VSN pool table entry.
 * RETURN: VSN pool table entry (NULL if not found).
 */
static struct VsnPool *
findVsnPool(
	char *poolName)
{
	struct VsnPool *vp;
	size_t	offset, size;

	for (offset = 0; offset < VsnPoolSize; offset += size) {
		vp = (struct VsnPool *)(void *)((char *)VsnPoolTable + offset);
		if (strcmp(poolName, vp->VpName) == 0) {
			return (vp);
		}
		size = sizeof (struct VsnPool) + ((vp->VpNumof - 1) *
		    sizeof (int));
	}
	return (NULL);
}


/*
 * Make the defaults Archive Set.
 * Check also that the tables are correct.
 */
static void
makeDefaultsArchSet(void)
{
	int	i;

	(void) findset(ALL_SETS);
	SetFieldDefaults(as, ArchSet);
	SetFieldDefaults(as, Priorities);

	/*
	 * Define allsets.1 - allsets.4.
	 */
	for (i = 1; i < ALLSETS_MAX; i++) {
		set_name_t name;

		if (i <= MAX_ARCHIVE) {
			snprintf(name, sizeof (name), ALL_SETS".%d", i);
		} else {
			snprintf(name, sizeof (name), ALL_SETS".%dR",
			    i - MAX_ARCHIVE);
		}
		(void) findset(name);
	}
}


/*
 * Process command file processing message.
 */
static void
msgFunc(
	char *msg,
	int lineno,
	char *line)
{
	linenum = lineno;
	if (line != NULL) {
		static boolean_t line_printed = FALSE;

		/*
		 * While reading the file.
		 */
		if (msg != NULL) {
			/*
			 * Error message.
			 */
			if (!line_printed) {
				printf("%d: %s\n", lineno, line);
			}
			fprintf(stderr, " *** %s\n", msg);
			/*
			 * If file properties being defined,
			 * clear copy information.
			 */
			if (fpDef != NULL) {
				fpDef->FpCopiesReq = 0;
			}
		} else if (ListOptions & LO_line) {
			/*
			 * Informational messages.
			 */
			if (lineno == 0) {
				printf("%s\n", line);
			} else {
				printf("%d: %s\n", lineno, line);
				line_printed = TRUE;
				return;
			}
		}
		line_printed = FALSE;
	} else if (lineno >= 0) {
		fprintf(stderr, "%s\n", msg);
	} else if (lineno < 0 && fileRequired) {
		char err_msg[STR_FROM_ERRNO_BUF_SIZE];

		/* Cannot open archiver command file */
		fprintf(stderr, "%s '%s': %s\n", GetCustMsg(4400), cmdFname,
		    StrFromErrno(errno, err_msg, sizeof (err_msg)));
	}
}


/*
 * Data field error message function.
 */
static void
msgFuncSetField(
/*LINTED argument unused in function */
	int code,
	char *msg)
{
	strncpy(errMsg, msg, sizeof (errMsg)-1);
}


/*
 * Normalize path.
 * Remove ./ ../ // sequences in a path.
 * Note: path array must be able to hold one more character.
 * RETURN: Start of path.  NULL if invalid, (too many ../'s).
 */
static char *
normalizePath(
	char *path)
{
	char *p, *ps, *q;

	ps = path;
	/* Preserve an absolute path. */
	if (*ps == '/') {
		ps++;
	}
	strncat(ps, "/", sizeof (token)-1);
	p = q = ps;
	while (*p != '\0') {
		char *q1;

		if (*p == '.') {
			if (p[1] == '/') {
				/*
				 * Skip "./".
				 */
				p++;
			} else if (p[1] == '.' && p[2] == '/') {
				/*
				 * "../" Back up over previous component.
				 */
				p += 2;
				if (q <= ps) {
					return (NULL);
				}
				q--;
				while (q > ps && q[-1] != '/') {
					q--;
				}
			}
		}
		/*
		 * Copy a component.
		 */
		q1 = q;
		while (*p != '/') {
			*q++ = *p++;
		}
		if (q1 != q) *q++ = *p;
		/*
		 * Skip successive '/'s.
		 */
		while (*p == '/') {
			p++;
		}
	}
	if (q > ps && q[-1] == '/') {
		q--;
	}
	*q = '\0';
	return (path);
}


/*
 * Pack the VSN List table.
 * VsnListTable may have holes because there was more than one VSN command
 * for an Archive Set.
 */
static void
packVsnListTable(void)
{
	struct VsnList *OldVsnListTable;
	int	asn;

	if (VsnListSize == 0) {
		return;
	}
	ASSERT(VsnListTable != NULL);
	OldVsnListTable = VsnListTable;
	VsnListTable = NULL;
	VsnListSize = 0;

	for (asn = 0; asn < ArchSetNumof; asn++) {
		struct VsnList *vl, *vl_old;
		size_t	size;
		int	vdn;

		as = &ArchSetTable[asn];
		if ((VD_mask & as->AsVsnDesc) != VD_list) {
			continue;
		}
		vl_old = (struct VsnList *)(void *)((char *)OldVsnListTable +
		    (as->AsVsnDesc & ~VD_mask));
		vdn = vl_old->VlNumof;
		size = VsnListSize +
		    sizeof (struct VsnList) + (vdn - 1) * sizeof (vsndesc_t);
		SamRealloc(VsnListTable, size);
		vl = (struct VsnList *)(void *)((char *)VsnListTable +
		    VsnListSize);
		vl->VlNumof = vdn;
		memmove(vl->VlDesc, vl_old->VlDesc, vdn * sizeof (vsndesc_t));
		as->AsVsnDesc = VD_list | VsnListSize;
		VsnListSize = size;
	}
	SamFree(OldVsnListTable);
}


/*
 * Set disk archive method for an archive set.
 */
static void
paramsDiskArchive(void)
{
	/* Fatal error */
	ReadCfgError(CustMsg(4541));
}


/*
 * Invalid parameter.
 */
static void
paramsInvalid(void)
{
	/* '%s' is not a valid archive set parameter */
	ReadCfgError(CustMsg(4420), dirName);
}

/*
 * Deprecated parameter, will be invalid in future release.
 */
static void
paramsDeprecated(void)
{
	/* '%s' is deprecated and will be removed in the next release */
	SendCustMsg(HERE, 4421, dirName);

	/*
	 * We need to see if the parameter has a value and ignore it.
	 * First see if \0 value is invalid, and if so ignore next value.
	 */
	*token = '\0';
	if (SetFieldValue(as, ArchSet, dirName+1, token,
	    msgFuncSetField) != 0) {
		if (errno != EINVAL) {
			ReadCfgError(0, errMsg);
		}
		/* Parameter requires a value, read it and ignore it */
		(void) ReadCfgGetToken();
	}
}

/*
 * Process 'priority' for SetFieldValue().
 */
/*ARGSUSED0*/
static int
paramsPrioritySet(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	strncpy(dirName, token, sizeof (dirName)-2);
	if (ReadCfgGetToken() == 0) {
		ReadCfgError(CustMsg(4508));
	}
	if (SetFieldValue(as, Priorities, dirName, token, msgFuncSetField) !=
	    0) {
		if (errno == ENOENT) {
			/* '%s' is not a valid priority name */
			ReadCfgError(CustMsg(4507), token);
		}
		ReadCfgError(0, errMsg);
	}
	return (0);
}


/*
 * Process 'reserve' for SetFieldValue().
 */
static int
paramsReserveSet(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	short	*val = (short *)v;
	int	i;
	int	reserve;

	for (i = 0; strcmp(value, Reserves[i].RsName) != 0; i++) {
		if (*Reserves[i].RsName == '\0') {
			/* Invalid %s value ('%s') */
			snprintf(buf, bufsize, GetCustMsg(4538),
			    "reserve", value);
			errno = EINVAL;
			return (-1);
		}
	}
	reserve = Reserves[i].RsValue;
	if (reserve & (RM_dir | RM_user | RM_group)) {
		reserve |= RM_owner;
	}
	*val |= reserve;
	return (0);
}


/*
 * Set field for an archive set.
 */
static void
paramsSetfield(void)
{
	if (dirName[0] != '-' || dirName[1] == '\0') {
		/* '%s' is not a valid archive set parameter */
		ReadCfgError(CustMsg(4420), dirName);
	}
	/*
	 * Try an empty value.
	 */
	*token = '\0';
	if (SetFieldValue(as, ArchSet, dirName+1, token, msgFuncSetField) !=
	    0) {
		if (errno == ENOENT) {
			/* '%s' is not a valid archive set parameter */
			ReadCfgError(CustMsg(4420), dirName);
		}
		if (errno != EINVAL) {
			ReadCfgError(0, errMsg);
		}
		/*
		 * Parameter requires a value.
		 */
		(void) ReadCfgGetToken();
		/* Check for ovflmin field and warn if no size unit is used. */
		if (strcmp(dirName+1, "ovflmin") == 0) {
			int len = strlen(token);
			if (isdigit(token[len-1])) {
				/*
				 * Warning: %s=%s has no fsize unit,
				 * defaulting to bytes
				 */
				SendCustMsg(HERE, 4552, dirName+1, token);
			}
		}
		if (SetFieldValue(as, ArchSet, dirName+1,
		    token, msgFuncSetField) != 0) {
			ReadCfgError(0, errMsg);
		}
	}
}

static void
paramsSetFillvsns(void)
{
	as->AsEflags |= AE_fillvsns; /* Set fillvsns execution flag. */

	/* Try to use the token value if it isn't a parameter */
	if (ReadCfgGetToken() != 0 && token[0] != '-') {
		Trace(TR_DEBUG, "Setting fillvsnsmin, token: %s", token);
		if (SetFieldValue(as, ArchSet, dirName+1,
		    token, msgFuncSetField) != 0) {
			ReadCfgError(0, errMsg);
		}
	} else {
		/* Tell procParams we peeked, but didn't use the token */
		peekedToken = TRUE;
	}
}


/*
 * Compare two file properties descriptions.
 */
static boolean_t		/* TRUE if file properties match */
propsMatch(
	struct FilePropsEntry *fp1,
	struct FilePropsEntry *fp2)
{
	/*
	 * Compare paths.
	 */
	if (strcmp(fp1->FpPath, fp2->FpPath) != 0) {
		return (FALSE);
	}

	/*
	 * Compare file properties.
	 */
	if (((fp1->FpFlags & FP_uid) != (fp2->FpFlags & FP_uid)) ||
	    fp1->FpUid != fp2->FpUid) {
		return (FALSE);
	}
	if (((fp1->FpFlags & FP_gid) != (fp2->FpFlags & FP_gid)) ||
	    fp1->FpGid != fp2->FpGid) {
		return (FALSE);
	}
	if (((fp1->FpFlags & FP_minsize) != (fp2->FpFlags & FP_minsize)) ||
	    fp1->FpMinsize != fp2->FpMinsize) {
		return (FALSE);
	}
	if (((fp1->FpFlags & FP_maxsize) != (fp2->FpFlags & FP_maxsize)) ||
	    fp1->FpMaxsize != fp2->FpMaxsize) {
		return (FALSE);
	}
	if (((fp1->FpFlags & FP_name) != (fp2->FpFlags & FP_name)) ||
	    strcmp(fp1->FpName, fp2->FpName) != 0) {
		return (FALSE);
	}
	if (((fp1->FpFlags & (FP_access | FP_nftv)) !=
	    (fp2->FpFlags & (FP_access | FP_nftv))) ||
	    fp1->FpAccess != fp2->FpAccess) {
		return (FALSE);
	}
	if (((fp1->FpFlags & FP_after) != (fp2->FpFlags & FP_after)) ||
	    fp1->FpAfter != fp2->FpAfter) {
		return (FALSE);
	}
	return (TRUE);
}


/*
 * Set Archive Set numbers.
 */
static void
setAsn(void)
{
	int	fsn;

	for (fsn = 0; fsn < FileSysTable->count; fsn++) {
		int	i;

		if (FileSysTable->entry[fsn].FsFlags & FS_noarchive) {
			continue;
		}
		fpt = FilesysProps[fsn];
		for (i = 0; i < fpt->FpCount; i++) {
			struct FilePropsEntry *fp;
			char	*asname;
			int	copy;

			fp = &fpt->FpEntry[i];
			if (fp->FpFlags & FP_noarch) {
				continue;
			}
			asname = ArchSetTable[fp->FpBaseAsn].AsName;
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				if (fp->FpCopiesReq & (1 << copy)) {
					set_name_t name;
					int	asn;

					snprintf(name, sizeof (name),
					    "%s.%d", asname, copy + 1);
					fp->FpAsn[copy] = findset(name);
					snprintf(name, sizeof (name),
					    "%s.%dR", asname, copy + 1);
					asn = findset(name);
					if (asn != -1) {
						fp->FpAsn[MAX_ARCHIVE + copy] =
						    asn;
					} else {
						fp->FpAsn[MAX_ARCHIVE + copy] =
						    fp->FpAsn[copy];
					}
				}
			}
		}
	}
}


/*
 * Set default Archive Set parameter values.
 * allsets flags must be cleared to allow setting Archive Set
 * parameters.  The allsets flags will be set later.
 * This can only be done once.
 */
static void
setDefaultParams(
	int which)	/* 1 for allsets.copy, 0 for all others */
{
	int	end;
	int	i;
	int	start;

	if (which == 0) {
		static boolean_t first = TRUE;

		if (!first) {
			return;
		}
		start = ALLSETS_MAX;
		end = ArchSetNumof;
		first = FALSE;
	} else {
		static boolean_t first = TRUE;

		if (!first) {
			return;
		}
		start = 1;
		end = ALLSETS_MAX;
		first = FALSE;
	}
	for (i = start; i < end; i++) {
		struct ArchSet *asd;		/* Default Archive Set */
		struct ArchSet *asi;		/* Archive Set to change */

		asi = &ArchSetTable[i];
		if (which == 0 && strchr(asi->AsName, '.') != NULL) {
			asd = &ArchSetTable[1 + AS_COPY(asi->AsName)];
		} else {
			asd = &ArchSetTable[0];
		}
		copyAsParams(asi, asd);
	}
}


/*
 * Set default Archive Set VSNs.
 * If the Archive Set does not have VSNs defined, assign the VSN definition
 * from 'allsets.copy'.
 * This can only be done once.
 */
static void
setDefaultVsns(void)
{
	static boolean_t first = TRUE;
	int	i;

	if (!first) {
		return;
	}
	first = FALSE;
	for (i = 1; i < ArchSetNumof; i++) {
		struct ArchSet *asd;		/* Default Archive Set */
		struct ArchSet *asi;		/* Archive Set to change */

		asi = &ArchSetTable[i];
		if (i >= ALLSETS_MAX && strchr(asi->AsName, '.') != NULL) {
			asd = &ArchSetTable[1 + AS_COPY(asi->AsName)];
		} else {
			asd = &ArchSetTable[0];
		}
		if (asd->AsVsnDesc != VD_none && asi->AsVsnDesc == VD_none) {
			memmove(asi->AsMtype, asd->AsMtype,
			    sizeof (asi->AsMtype));
			asi->AsVsnDesc = asd->AsVsnDesc;
		}
	}
}


/*
 * Set disk archive drive count.
 */
static void
setDiskArchives(void)
{
	int	dkDrives;
	int	hcDrives;
	int	i;

	/*
	 * Count all Archive Set drives.
	 */
	dkDrives = 0;
	hcDrives = 0;
	for (i = ALLSETS_MAX; i < ArchSetNumof; i++) {
		struct ArchSet *asi;

		asi = &ArchSetTable[i];
		if (asi->AsFlags & AS_disk_archive) {
			dkDrives += asi->AsDrives;
		} else if (asi->AsFlags & AS_honeycomb) {
			hcDrives += asi->AsDrives;
			if (asi->AsArchmax != 0) {
				/* 'archmax' not a valid option */
				SendCustMsg(HERE, 4547);
				errors++;
			}
		}
	}
	if (dkDrives == 0 && hcDrives == 0) {
		return;
	}

	/*
	 * Find disk archive library and set its count.
	 */
        if (ArchLibTable == NULL) {
	        SendCustMsg(HERE, 4002);
                return;
        }
	for (i = 0; i < ArchLibTable->count; i++) {
		struct ArchLibEntry *al;

		al = &ArchLibTable->entry[i];
		if (!(al->AlFlags & (AL_disk | AL_honeycomb))) {
			continue;
		}
		if (!(al->AlFlags & AL_allow)) {
			if (al->AlFlags & AL_disk) {
				al->AlDrivesAllow = dkDrives;
			} else if (al->AlFlags & AL_honeycomb) {
				al->AlDrivesAllow = hcDrives;
				if (hcDrives > HONEYCOMB_MAX_OPEN_SESSIONS) {
					al->AlDrivesAllow =
					    HONEYCOMB_MAX_OPEN_SESSIONS;
				}
			}
		}
	}
}


/*
 * Set file system flag.
 */
static void
setFsFlag(
	uint_t flag)
{
	int	flags = 0;

	if (strcmp(token, "on") == 0) {
		flags = flag;
	} else if (strcmp(token, "off") == 0) {
		flags = 0;
	} else {
		/* Invalid %s value ('%s') */
		ReadCfgError(CustMsg(4538), dirname, token);
	}
	if (fsn == -1) {
		int	i;

		for (i = 0; i < FileSysTable->count; i++) {
			FileSysTable->entry[i].FsFlags &= ~flag;
			FileSysTable->entry[i].FsFlags |= flags;
		}
	} else {
		FileSysTable->entry[fsn].FsFlags &= ~flag;
		FileSysTable->entry[fsn].FsFlags |= flags;
	}
}


/*
 * Verify that a file does or can exist.
 */
static boolean_t
verifyFile(
	char *file)
{
	int	stat_ret;
	char	*dup_file;
	char	*path;
	struct stat st;

	/* Must be a fully qualified path */
	if (*file != '/') {
		return (FALSE);
	}

	/* File already exists satisfies the condition */
	if (stat(file, &st) == 0) {
		return (TRUE);
	}

	/* A valid directory satisfies the condition */
	SamStrdup(dup_file, file);
	path = dirname(dup_file);
	stat_ret = stat(path, &st);
	SamFree(dup_file);
	if (stat_ret == 0) {
		return (TRUE);
	}

	/* Fell through, it doesn't exist and probably cannot be created */
	return (FALSE);
}
