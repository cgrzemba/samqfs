/*
 * defaults.c - read site defaults file.
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

#pragma ident "$Revision: 1.54 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef sun
#include <strings.h>
#endif
#ifdef linux
#include <string.h>
#endif

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <grp.h>
#include <unistd.h>

/* Solaris headers. */
#include "syslog.h"

/* SAM-FS headers. */
#include "sam/types.h"
#define	DEVICE_DEFAULTS
#include "sam/defaults.h"
#include "sam/dpparams.h"
#include "aml/device.h"
#define	NEED_DL_NAMES
#include "aml/dev_log.h"
#include "sam/devnm.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#ifdef sun
#include "aml/robots.h"
#endif /* sun */
#include "sam/sam_malloc.h"
#ifdef linux
#include "sam/names.h"
#endif /* linux */
#define	TRACE_CONTROL
#include "sam/sam_trace.h"
#ifdef sun
#include "sam/syscall.h"
#endif /* sun */

/* Local headers. */
#include "fsd.h"

/* Directive functions. */
static void dirDebug(void);
static void dirDevlog(void);
static void dirTapeAlert(void);
static void dirSAMStorade(void);
static void dirSef(void);
static void dirTapeclean(void);
static void dirDevParams(void);
static void dirEndtrace(void);
static void dirLabels(void);
static void dirLog(void);
static void dirNoBegin(void);
static void dirObsolete(void);
static void dirOperPriv(void);
static void dirOperator(void);
static void dirTrace(void);
static void dirValue(void);
static void procTrace(void);
static void dirTarFormat(void);
static void dirDeprecated(void);

/* Private data. */

/* Directive table. */
static DirProc_t dirProcTable[] = {
	{ "debug",			dirDebug,		DP_value },
	{ "devlog",			dirDevlog,		DP_value },
	{ "tapealert",			dirTapeAlert,		DP_value },
	{ "samstorade",			dirSAMStorade,		DP_value },
	{ "sef",			dirSef,			DP_value },
	{ "tapeclean",			dirTapeclean,		DP_value },
	{ "endtrace",			dirNoBegin,		DP_set   },
	{ "inodes",			dirObsolete,		DP_value },
	{ "labels",			dirLabels,		DP_value },
	{ "log",			dirLog,			DP_value },
	{ "oper_privileges",		dirOperPriv,		DP_value },
	{ "operator",			dirOperator,		DP_value },
	{ "stage_retries",		dirObsolete,		DP_value },
	{ "trace",			dirTrace,		DP_set   },
	{ "tar_format",			dirTarFormat,		DP_value },
	{ "dio_min_file_size",		dirDeprecated,		DP_other },
	{ NULL,				dirValue,		DP_setfield }
};

static struct DeviceParams *deviceParams;
static struct sam_defaults defaults;
static struct TraceCtlBin traceCtlBin;

static char errMsg[256];
static char dirName[TOKEN_SIZE];
static char token[TOKEN_SIZE];

#ifdef linux
#define	INITIAL_MOUNT_TIME	30
#endif /* linux */

#include "sam/defaults.hc"
#include "sam/dpparams.hc"

/* Private functions. */
static void makedeviceParams(void);
static void msgFunc(int code, char *msg);
static void setDevlogDefaults(void);
static void setTapeAlertDefaults(void);
static void setStoradeDefaults(void);
static void setSefDefaults(void);
static void setTapecleanDefaults(void);
static void setTraceDefaults(void);

/*
 * Read defaults.conf.
 */
void
ReadDefaults(char *defaults_name)
{
	char	*fname = SAM_CONFIG_PATH "/defaults.conf";
	int		errors;

	if (defaults_name != NULL) {
		fname = defaults_name;
	}
	makedeviceParams();
	SetFieldDefaults(&defaults, Defaults);
	setDevlogDefaults();
	setTapeAlertDefaults();
	setStoradeDefaults();
	setSefDefaults();
	setTapecleanDefaults();
	setTraceDefaults();

	errors = ReadCfg(fname, dirProcTable, dirName, token, ConfigFileMsg);
	if (errors != 0 && !(errors == -1 && defaults_name == NULL)) {
		/* Absence of the default defaults.conf file not an error. */
		if (errors > 0) {
			errno = 0;
		}
		/* Read defaults %s failed */
		FatalError(17224, fname);
	}
	if (Verbose) {
		int		i;

		SetFieldPrintValues("Defaults", Defaults, &defaults);

		printf("deviceParams:\n");
		for (i = 0; i < deviceParams->count; i++) {
			struct DpEntry *dp;

			dp = &deviceParams->DpTable[i];
			printf("%2d %s %04x %d %d %d %d\n", i, dp->DpName,
			    dp->DpType,
			    dp->DpBlock_size, dp->DpDelay_time,
			    dp->DpPosition_timeout,
			    dp->DpUnload_time);
		}
	}
	if (Daemon && defaults.operator.gid != 0) {
		if (setgid(defaults.operator.gid) == -1) {
			/* Cannot set group ID %d */
			FatalError(17225, defaults.operator.gid);
		}
		/*
		 * Change the group id of previously made sam-fsd files.
		 */
		if (chown(".", 0, defaults.operator.gid) == -1) {
			LibError(NULL, 0, 618, ".");
		}
		if (chown("trace", 0, defaults.operator.gid) == -1) {
			LibError(NULL, 0, 618, "trace");
		}
	}
}


/*
 * Make trace control binary file.
 */
void
MakeTraceCtl(void)
{
	struct TraceCtlBin *tb;
	char	*fname = TRACECTL_BIN;
	size_t	size;
	int		fd;

	/*
	 * Assure that the trace control binary file exists.
	 */
	if ((tb = MapFileAttach(fname, TRACECTL_MAGIC, O_RDWR)) != NULL) {
		if (tb->TrVersion == TRACECTL_VERSION) {
			(void) MapFileDetach(tb);
			return;
		}
		(void) MapFileDetach(tb);
		errno = 0;
	}

	/*
	 * Create it otherwise.
	 */
	size = sizeof (traceCtlBin);
	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		FatalError(0, "Cannot create %s", fname);
	}
	traceCtlBin.Tr.MfLen = size;
	traceCtlBin.Tr.MfMagic = TRACECTL_MAGIC;
	traceCtlBin.Tr.MfValid = 1;
	traceCtlBin.TrVersion = TRACECTL_VERSION;
	if (write(fd, &traceCtlBin, size) != size) {
		FatalError(0, "Cannot write %s %m", fname);
	}
	if (close(fd) == -1) {
		FatalError(0, "Cannot close %s %m", fname);
	}

	/*
	 * Assure that the library function will work.
	 */
	if ((tb = MapFileAttach(fname, TRACECTL_MAGIC, O_RDWR)) == NULL) {
		FatalError(0, "MapfileAttach(%s) failed.", fname);
	}
	(void) MapFileDetach(tb);
}


/*
 * Write defaults binary file.
 */
void
WriteDefaults(void)
{
	struct DefaultsBin *db;
	MappedFile_t mfh;
	size_t	size;
	char	*fname = DEFAULTS_BIN;
	int		fd;

	size = sizeof (struct DeviceParams) +
	    (deviceParams->count - 1) * sizeof (struct DpEntry);
	mfh.MfLen = sizeof (mfh) + sizeof (defaults) + size;

	db = MapFileAttach(fname, DEFAULTS_MAGIC, O_RDWR);
	if (db != NULL && db->Db.MfLen == mfh.MfLen) {
		boolean_t changed = FALSE;

		/*
		 * File exists and is the same size.
		 * Compare new data and present file.
		 */
		if (memcmp(&db->DbDefaults, &defaults, sizeof (defaults)) !=
		    0) {
			memmove(&db->DbDefaults, &defaults, sizeof (defaults));
			changed = TRUE;
		}
		if (memcmp(&db->DbDeviceParams, deviceParams, size) != 0) {
			memmove(&db->DbDeviceParams, deviceParams, size);
			changed = TRUE;
		}
		Trace(TR_MISC, "defaults.bin %schanged",
		    (changed) ? "" : "not ");
		(void) MapFileDetach(db);
		return;
	}

	/*
	 * Invalidate existing file.
	 */
	(void) unlink(fname);
	if (db != NULL) {
		db->Db.MfValid = 0;
		(void) MapFileDetach(db);
	}

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		FatalError(0, "Cannot create %s", fname);
	}
	mfh.MfMagic = DEFAULTS_MAGIC;
	mfh.MfValid = 1;
	if (write(fd, &mfh, sizeof (mfh)) != sizeof (mfh)) {
		FatalError(0, "Cannot write %s %m", fname);
	}

	if (write(fd, &defaults, sizeof (defaults)) != sizeof (defaults)) {
		FatalError(0, "Cannot write %s %m", fname);
	}
	if (write(fd, deviceParams, size) != size) {
		FatalError(0, "Cannot write %s %m", fname);
	}
	if (close(fd) == -1) {
		FatalError(0, "Cannot close %s %m", fname);
	}
	Trace(TR_MISC, "defaults.bin replaced");

	/*
	 * Assure that the library function will work.
	 */
	if (GetDefaultsRw() == NULL) {
		FatalError(0, "GetDefaults(%s) failed.", fname);
	}
}


/*
 * Write trace control binary file.
 */
void
WriteTraceCtl(void)
{
	struct TraceCtlBin *tb;
	int		tid;
	pid_t	dpid = getpid();

	if (Daemon) {
		if ((tb = MapFileAttach(TRACECTL_BIN,
		    TRACECTL_MAGIC, O_RDWR)) == NULL) {
			MakeTraceCtl();
		} else {
			(void) MapFileDetach(tb);
		}
		if ((tb = MapFileAttach(TRACECTL_BIN,
		    TRACECTL_MAGIC, O_RDWR)) == NULL) {
			FatalError(0, "MapfileAttach(%s) failed.",
			    TRACECTL_BIN);
		}
	}

	/* Trace file controls: */
	printf("%s\n", GetCustMsg(17283));
	for (tid = 1; tid < TI_MAX; tid++) {
		struct TraceCtlEntry *tc;

		tc = &traceCtlBin.entry[tid];
		printf("%-13s ", traceIdNames[tid]);
		if (*tc->TrFname != '\0') {
			printf("%s\n", tc->TrFname);
			printf("%13s %s\n", "", TraceGetOptions(tc, NULL, 0));
			printf("%13s size %s  age %d\n", "",
			    StrFromFsize(tc->TrSize, 0, NULL, 0),
			    tc->TrAge);
		} else {
			printf("off\n\n");
		}
		if (Daemon) {
			/*
			 * Copy the fields into the binary.
			 */
			struct TraceCtlEntry *tcb;

			tcb = &tb->entry[tid];
			tcb->TrFsdPid = dpid;
			strncpy(tcb->TrFname, tc->TrFname,
			    sizeof (tcb->TrFname));
			tcb->TrFlags = tc->TrFlags;
			tcb->TrAge = tc->TrAge;
			tcb->TrSize = tc->TrSize;
			tcb->TrRotSize = 0;
			tcb->TrChange++;
		}
	}
	if (Daemon) {
		(void) MapFileDetach(tb);
	}
}


/*
 * No beginning statement.
 * 'trace'.
 */
static void
dirNoBegin(void)
{
	ReadCfgError(17282, dirName + 3);
}


/*
 * trace
 */
static void
dirTrace(void)
{
	static DirProc_t table[] = {
		{ "endtrace", dirEndtrace, DP_set },
		{ NULL, procTrace, DP_value }
	};

	ReadCfgSetTable(table);
	memset(&traceCtlBin, 0, sizeof (struct TraceCtlBin));
}




/*
 * end
 */
static void
dirEndtrace(void)
{
	ReadCfgSetTable(dirProcTable);
}


/*
 * General value processor.
 */
static void
dirValue(void)
{
	if (SetFieldValue(&defaults, Defaults, dirName, token, msgFunc) != 0) {
		if (errno == ENOENT) {
			dirDevParams();
		} else {
			ReadCfgError(0, errMsg);
		}
	}
}


/*
 * debug = flags
 */
static void
dirDebug(void)
{
	sam_debug_t dbg;

	dbg = (sam_debug_t)SAM_DBG_DEFAULT;
	do {
		sam_dbg_strings_t *sp;
		char	*p;

		p = token;
		if (*token == '-') {
			p++;
		}
		for (sp = sam_dbg_strings; /* Terminated inside */; sp++) {
			if (sp->string == NULL) {
				ReadCfgError(17280, dirName, token);
			}
			if (strcmp(p, sp->string) == 0) {
				if (*token != '-') {
					dbg |=  sp->bits;
				} else {
					dbg &= ~sp->bits;
				}
				break;
			}
		}
	} while (ReadCfgGetToken() != 0);

	defaults.debug = dbg;
}



/*
 * devlog =
 */
static void
dirDevlog(void)
{
	dev_ent_t *dev;
	uint_t	flags;
	int		i;

	if (strcmp(token, "all") == 0) {
		dev = NULL;
	} else {
		/*
		 * Find requested equipment.
		 */
		char	*p;
		long	eq;
		int		i;

		errno = 0;
		eq = strtoll(token, &p, 10);
		if (*p != '\0' || errno != 0) {
			/* Invalid equipment ordinal '%s' */
			ReadCfgError(17244, token);
		}
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			if (dev->eq == eq) {
				break;
			}
		}
		if (i >= DeviceNumof) {
			/* Invalid equipment ordinal '%s' */
			ReadCfgError(17244, token);
		}
	}

	/*
	 * Translate flags.
	 */
	if (ReadCfgGetToken() == 0) {
		ReadCfgError(14008, dirName);
	}
	flags = 0;
	while (*token != '\0') {
		char	*p;

		p = token;
		if (*token == '-') {
			p++;
		}
		for (i = 0; strcmp(p, DL_names[i]) != 0; i++) {
			if (i >= DL_MAX) {
				ReadCfgError(17280, dirName, token);
			}
		}
		if (i == DL_none) {
			flags = 0;
		} else {
			uint_t	flag;

			if (i == DL_all) {
				flag = DL_all_events;
			} else if (i == DL_default) {
				flag = DL_def_events;
			} else {
				flag = 1 << i;
			}
			if (*token != '-') {
				flags |= flag;
			} else {
				flags &= ~flag;
			}
		}
		(void) ReadCfgGetToken();
	}

	if (dev != NULL) {
		dev->log.flags = flags;
	} else {
		/*
		 * Set flags for all devices.
		 */
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			dev->log.flags = flags;
		}
	}
}


/*
 * tapealert =
 */
static void
dirTapeAlert(void)
{
	dev_ent_t	*dev;
	uchar_t		flags = 0;
	int		i;

	if (strcmp(token, "all") == 0) {
		dev = NULL;
	} else {
		/*
		 * Find requested equipment.
		 */
		char	*p;
		long	eq;
		int		i;

		errno = 0;
		eq = strtoll(token, &p, 10);
		if (*p != '\0' || errno != 0) {
			ReadCfgError(13110, token);
		}
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			if (dev->eq == eq) {
				break;
			}
		}
		if (i >= DeviceNumof) {
			/* Invalid equipment ordinal '%s' */
			ReadCfgError(13111, token);
		}
		if (IS_HISTORIAN(dev) || !(IS_TAPE(dev) || IS_ROBOT(dev))) {
			ReadCfgError(13112, eq);
		}
	}

	/*
	 * Translate flags.
	 */
	while (ReadCfgGetToken() != 0) {
		if (strcmp(token, "on") == 0) {
			flags |= TAPEALERT_ENABLED;
		} else if (strcmp(token, "off") == 0) {
			flags &= ~TAPEALERT_ENABLED;
		} else {
			ReadCfgError(13113, token);
		}
	}

	if (dev != NULL) {
		dev->tapealert &= ~TAPEALERT_ENABLED;
		dev->tapealert |= flags;
	} else {
		/*
		 * Set flags for all devices.
		 */
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			dev->tapealert &= ~TAPEALERT_ENABLED;
			dev->tapealert |= flags;
		}
	}
}

/*
 * StorADE API
 */
static void
dirSAMStorade(void)
{
	if (strcmp(token, "on") == 0) {
		defaults.samstorade = B_TRUE;
	} else if (strcmp(token, "off") == 0) {
		defaults.samstorade = B_FALSE;
	} else {
		ReadCfgError(13001, token);
	}
}

/*
 * sef =
 */
static void
dirSef(void)
{
	dev_ent_t	*dev;
	uchar_t		flags = 0;
	int		i;
	long		interval = SEF_INTERVAL_ONCE;
	char		*endptr = NULL;

	if (strcmp(token, "all") == 0) {
		dev = NULL;
	} else {
		/*
		 * Find requested equipment.
		 */
		char	*p;
		long	eq;
		int		i;

		errno = 0;
		eq = strtoll(token, &p, 10);
		if (*p != '\0' || errno != 0) {
			ReadCfgError(13114, token);
		}
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			if (dev->eq == eq) {
				break;
			}
		}
		if (i >= DeviceNumof) {
			ReadCfgError(13102, token);
		}
		if (!IS_TAPE(dev)) {
			ReadCfgError(13103, eq);
		}
	}

	/*
	 * Translate flags.
	 */
	while (ReadCfgGetToken() != 0) {
		if (strcmp(token, "on") == 0) {
			flags |= SEF_ENABLED;
		} else if (strcmp(token, "off") == 0) {
			flags &= ~SEF_ENABLED;
		} else {
			ReadCfgError(13104, token);
		}

		if (ReadCfgGetToken() == 0) {
			break;
		}

		if (strcmp(token, "once") == 0) {
			interval = SEF_INTERVAL_ONCE;
		} else {
			interval = strtol(token, &endptr, 0);
			if (*endptr != '\0' ||
			    (interval == 0 && errno == EINVAL) ||
			    ((interval == LONG_MAX ||
			    interval == LONG_MIN) &&
			    errno == ERANGE) ||
			    interval > INT_MAX || interval < 300) {
				/* Invalid sef sample rate, use default. */
				printf("Invalid sef sample rate, reset to "
				    "default.\n");
				interval = SEF_INTERVAL_DEFAULT;
			}
		}
	}

	if (dev != NULL) {
		dev->sef_sample.state &= ~SEF_ENABLED;
		dev->sef_sample.state |= flags;
		dev->sef_sample.interval = (int)interval;
	} else {
		/*
		 * Set flags for all devices.
		 */
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			dev->sef_sample.state &= ~SEF_ENABLED;
			dev->sef_sample.state |= flags;
			dev->sef_sample.interval = (int)interval;
		}
	}
}

/*
 * Tapeclean
 */
static void
dirTapeclean(void)
{
	dev_ent_t	*dev;
	uchar_t		flags = 0;
	int		i;
	char		*endptr = NULL;
	int		user_set_logsense = 0;
	int		auto_clean_flag = 0;

	if (strcmp(token, "all") == 0) {
		dev = NULL;
	} else {
		/*
		 * Find requested equipment.
		 */
		char	*p;
		long	eq;
		int		i;

		errno = 0;
		eq = strtoll(token, &p, 10);
		if (*p != '\0' || errno != 0) {
			ReadCfgError(13106, token);
		}
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			if (dev->eq == eq) {
				break;
			}
		}
		if (i >= DeviceNumof) {
			ReadCfgError(13107, token);
		}
		if (!IS_TAPE(dev)) {
			ReadCfgError(13108, eq);
		}
	}

	/*
	 * Translate flags.
	 */
	while (ReadCfgGetToken() != 0) {
		if (strcmp(token, "logsense") == 0) {
			if (ReadCfgGetToken() == 0) {
				ReadCfgError(13109, token);
			}
			if (strcmp(token, "on") == 0) {
				flags |= TAPECLEAN_LOGSENSE;
			} else if (strcmp(token, "off") == 0) {
				flags &= ~TAPECLEAN_LOGSENSE;
			} else {
				ReadCfgError(13109, token);
			}
			/* user setting use of log sense cleaning flags */
			user_set_logsense = 1;
		} else if (strcmp(token, "autoclean") == 0) {
			if (ReadCfgGetToken() == 0) {
				ReadCfgError(13109, token);
			}
			if (strcmp(token, "on") == 0) {
				flags |= TAPECLEAN_AUTOCLEAN;
			} else if (strcmp(token, "off") == 0) {
				flags &= ~TAPECLEAN_AUTOCLEAN;
			} else {
				ReadCfgError(13109, token);
			}
			/* autoclean option set */
			auto_clean_flag = 1;
		} else {
			ReadCfgError(13109, token);
		}
	}

	/* user must set auto-clean flag, log sense flag is optional */
	if (auto_clean_flag == 0) {
		ReadCfgError(13115);
	}

	if (dev != NULL) {
		dev->tapeclean = flags;
		/* if log sense cleaning flag not set by user, then set it */
		if (user_set_logsense == 0) {
			dev->tapeclean |= TAPECLEAN_LOGSENSE;
		}
	} else {
		/*
		 * Set flags for all devices.
		 */
		for (i = 0; i < DeviceNumof; i++) {
			dev = &DeviceTable[i];
			dev->tapeclean = flags;
			/* if log sns cln flag not set by user, then set it */
			if (user_set_logsense == 0) {
				dev->tapeclean |= TAPECLEAN_LOGSENSE;
			}
		}
	}
}

/*
 * Process device parameters.
 * xx_blksize, xx_delay, xx_position_timeout, xx_unload
 */
static void
dirDevParams(void)
{
	struct DpEntry *dp;
	char	*p;
	int		n;

	/*
	 * Break dirName into device type and parameter name.
	 */
	p = strchr(dirName, '_');
	if (p == NULL || p == dirName || (
	    strcmp(p + 1, "blksize") != 0 &&
	    strcmp(p + 1, "unload") != 0 &&
	    strcmp(p + 1, "position_timeout") != 0 &&
	    strcmp(p + 1, "delay") != 0)) {
		ReadCfgError(0, errMsg);
	}
	*p = '\0';

	/*
	 * Look up device type name.
	 */
	for (n = 0; n < deviceParams->count; n++) {
		dp = &deviceParams->DpTable[n];
		if (strcmp(dp->DpName, dirName) == 0) {
			break;
		}
	}
	if (n >= deviceParams->count) {
		/* Invalid device type '%s' */
		ReadCfgError(17248, dirName);
	}

	/*
	 * Set value in the device type entry.
	 */
	if (SetFieldValue(dp, DpParams, p + 1, token, msgFunc) != 0) {
		ReadCfgError(0, errMsg);
	}
}


/*
 * labels = barcodes | barcodes_low | read
 */
static void
dirLabels(void)
{
	if (strcmp(token, "barcodes") == 0) {
		(void) SetFieldValue(&defaults, Defaults, "label_barcode",
		    "TRUE", msgFunc);
		(void) SetFieldValue(&defaults, Defaults, "barcode_low",
		    "FALSE", msgFunc);
	} else if (strcmp(token, "barcodes_low") == 0) {
		(void) SetFieldValue(&defaults, Defaults, "label_barcode",
		    "TRUE", msgFunc);
		(void) SetFieldValue(&defaults, Defaults, "barcode_low",
		    "TRUE", msgFunc);
	} else if (strcmp(token, "read") == 0) {
		(void) SetFieldValue(&defaults, Defaults, "label_barcode",
		    "FALSE", msgFunc);
	} else {
		ReadCfgError(14101, dirName, token);
	}
}

/*
 * tar_format = legacy | pax
 */
static void
dirTarFormat(void)
{
#if DEBUG
	if (strcmp(token, "legacy") == 0) {
		(void) SetFieldValue(&defaults, Defaults, "legacy_arch_format",
		    "TRUE", msgFunc);
		(void) SetFieldValue(&defaults, Defaults, "pax_arch_format",
		    "FALSE", msgFunc);
	} else if (strcmp(token, "pax") == 0) {
		(void) SetFieldValue(&defaults, Defaults, "legacy_arch_format",
		    "FALSE", msgFunc);
		(void) SetFieldValue(&defaults, Defaults, "pax_arch_format",
		    "TRUE", msgFunc);
	} else {
		ReadCfgError(14101, dirName, token);
	}
#else /* DEBUG */
	/* redundant, these values are set by default */
	(void) SetFieldValue(&defaults, Defaults, "legacy_arch_format",
	    "TRUE", msgFunc);
	(void) SetFieldValue(&defaults, Defaults, "pax_arch_format",
	    "FALSE", msgFunc);
#endif /* DEBUG */
}

/*
 * Deprecated parameter, will be invalid in future release.
 */
static void
dirDeprecated(void)
{
	SendCustMsg(HERE, 4421, token);

}

/*
 * log = facility
 */
static void
dirLog(void)
{
	static struct log_fac {
		char *name;
		int facility;
	} log_facility[] = {
	"LOG_KERN", LOG_KERN,
	"LOG_USER", LOG_USER,
	"LOG_MAIL", LOG_MAIL,
	"LOG_DAEMON", LOG_DAEMON,
	"LOG_AUTH", LOG_AUTH,
	"LOG_SYSLOG", LOG_SYSLOG,
	"LOG_LPR", LOG_LPR,
	"LOG_NEWS", LOG_NEWS,
	"LOG_UUCP", LOG_UUCP,
	"LOG_CRON", LOG_CRON,
	"LOG_LOCAL0", LOG_LOCAL0,
	"LOG_LOCAL1", LOG_LOCAL1,
	"LOG_LOCAL2", LOG_LOCAL2,
	"LOG_LOCAL3", LOG_LOCAL3,
	"LOG_LOCAL4", LOG_LOCAL4,
	"LOG_LOCAL5", LOG_LOCAL5,
	"LOG_LOCAL6", LOG_LOCAL6,
	"LOG_LOCAL7", LOG_LOCAL7,
	NULL, 0
	};
	int		n;

	for (n = 0; log_facility[n].name != NULL; n++) {
		if (strcmp(token, log_facility[n].name) == 0) {
			defaults.log_facility = log_facility[n].facility;
			return;
		}
	}
	ReadCfgError(14101, dirName, token);
}


/*
 * Obsolete commands.
 */
static void
dirObsolete(void)
{
	printf("Directive %s is obsolete.\n", dirName);
}

/*
 * operator_priv =
 */
static void
dirOperPriv(void)
{
	do {
		if (strcmp(token, "all") == 0) {
			defaults.operator.priv.label = 1;
			defaults.operator.priv.slot = 1;
			defaults.operator.priv.fullaudit = 1;
			defaults.operator.priv.state = 1;
			defaults.operator.priv.clear = 1;
			defaults.operator.priv.options = 1;
			defaults.operator.priv.refresh = 1;
		} else if (strcmp(token, "label") == 0) {
			defaults.operator.priv.label = 1;
		} else if (strcmp(token, "slot") == 0) {
			defaults.operator.priv.slot = 1;
		} else if (strcmp(token, "fullaudit") == 0) {
			defaults.operator.priv.fullaudit = 1;
		} else if (strcmp(token, "state") == 0) {
			defaults.operator.priv.state = 1;
		} else if (strcmp(token, "clear") == 0) {
			defaults.operator.priv.clear = 1;
		} else if (strcmp(token, "options") == 0) {
			defaults.operator.priv.options = 1;
		} else if (strcmp(token, "refresh") == 0) {
			defaults.operator.priv.refresh = 1;
		} else {
			ReadCfgError(0, "Invalid operator privilege '%s'",
			    token);
		}
	} while (ReadCfgGetToken() != 0);
}


/*
 * operator = groupname
 */
static void
dirOperator(void)
{
	struct group *grp;

	/*
	 * Get operator gid.
	 */
	if ((grp = getgrnam(token)) == NULL) {
		ReadCfgError(17281, token);
	}
	defaults.operator.gid = grp->gr_gid;
}


/*
 * Process trace definitions.
 */
static void
procTrace(void)
{
	static char *value = NULL;
	char	*msg;
	int		next;

	/*
	 * Collect tokens to make the value string.
	 * This is for the space separated options.
	 */
	next = 0;
	for (;;) {
		static size_t alloc = 0;
		size_t token_l;

		token_l = strlen(token);
		/* Room for token + separator */
		while (next + token_l + 1 >= alloc) {
			alloc += 64;
			SamRealloc(value, alloc);
		}
		memmove(value + next, token, token_l);
		next += token_l;
		if (ReadCfgGetToken() == 0) {
			break;
		}
		*(value + next++) = ' ';
	}
	*(value + next) = '\0';
	msg = TraceControl(dirName, value, &traceCtlBin);
	if (*msg != '\0') {
		ReadCfgError(0, msg);
	}
}


/*
 * Make the device parameters table.
 */
static void
makedeviceParams(void)
{
	size_t	size;
	int		i;

	/*
	 * Count entries in the device name table.
	 */
	for (i = 0; dev_nm2dt[i].nm != NULL; i++) {
		;
	}
	size = sizeof (struct DeviceParams) + ((i - 1) *
	    sizeof (struct DpEntry));
	SamRealloc(deviceParams, size);
	memset(deviceParams, 0, size);
	deviceParams->count = i;

	/*
	 * Copy device names and types.
	 */
	for (i = 0; i < deviceParams->count; i++) {
		struct DpEntry *dp;

		dp = &deviceParams->DpTable[i];
		memmove(dp->DpName, dev_nm2dt[i].nm, sizeof (dp->DpName));
		dp->DpType = dev_nm2dt[i].dt;
		SetFieldDefaults(dp, DpParams);
	}

	/*
	 * Set individual device parameters from DeviceParamsVals table.
	 */
	for (i = 0; *DeviceParamsVals[i].device != '\0'; i++) {
		struct DpEntry *dp;
		char	*params, *tok_params;
		char	*field;
		int		j;

		/*
		 * Look up device type name.
		 */
		for (j = 0; j < deviceParams->count; j++) {
			dp = &deviceParams->DpTable[j];
			if (strcmp(dp->DpName, DeviceParamsVals[i].device) ==
			    0) {
				break;
			}
		}
		if (j >= deviceParams->count) {
			errno = 0;
			FatalError(0,
			    "Invalid device '%s' in DeviceParamsVals",
			    DeviceParamsVals[i].device);
		}

		/*
		 * Parse field/value pairs and set the fields.
		 */
		params = strdup(DeviceParamsVals[i].params);
		tok_params = params;
		while ((field = strtok(tok_params, " \t")) != NULL) {
			char	*value;

			tok_params = NULL;
			value = strtok(tok_params, ",");
			if (value == NULL) {
				errno = 0;
				FatalError(0,
				    "Value missing in DeviceParamsVals[%s]",
				    dp->DpName);
			}
			if (SetFieldValue(dp, DpParams, field, value,
			    msgFunc) != 0) {
				errno = 0;
				FatalError(0,
				    "Problem in DeviceParamsVals[%s]: %s",
				    dp->DpName, errMsg);
			}
		}
		free(params);
	}
}


/*
 * Set initial devlog defaults for all devices.
 */
static void
setDevlogDefaults(void)
{
	dev_ent_t *dev;
	int	i;

	/*
	 * Set flags for all devices.
	 */

	for (i = 0; i < DeviceNumof; i++) {
		dev = &DeviceTable[i];
		dev->log.flags = DL_def_events;
	}
}

/*
 * Set initial TapeAlert defaults for all devices.
 */
static void
setTapeAlertDefaults(void)
{
	dev_ent_t *dev;
	int	i;

	/*
	 * Set flags for all devices.
	 */

	for (i = 0; i < DeviceNumof; i++) {
		dev = &DeviceTable[i];
		dev->tapealert = TAPEALERT_ENABLED;
	}
}

/*
 * Set initial StorADE API.
 */
static void
setStoradeDefaults(void)
{
	/*
	 * Running the StorADE API is the default.
	 */
	defaults.samstorade = B_TRUE;
}

/*
 * This function sets up the default values for tracing. This is
 * done prior to actually parsing the defaults.conf file so any values
 * that are set for tracing in the defaults.conf will override these
 * default values
 *
 */
static void
setTraceDefaults(void)
{
	char    *msg;

	msg = TraceControl("all", "on", &traceCtlBin);
	if (*msg != '\0') {
		printf("Unable to initialize the trace control table to "
		    "defaults\n");
	}

	msg = TraceControl("all.size", "10M", &traceCtlBin);
	if (*msg != '\0') {
		printf("Unable to initialize the trace file rotation "
		    "defaults to 100M\n");
	}

}

/*
 * Set initial SEF.
 */
static void
setSefDefaults(void)
{
	dev_ent_t *dev;
	int	i;

	/*
	 * Sef flags for all devices.
	 */
	for (i = 0; i < DeviceNumof; i++) {
		dev = &DeviceTable[i];
		dev->sef_sample.state = SEF_ENABLED;
		dev->sef_sample.interval = SEF_INTERVAL_DEFAULT;
	}
}

/*
 * Set initial tapeclean.
 */
static void
setTapecleanDefaults(void)
{
	dev_ent_t *dev;
	int	i;

	/*
	 * Sef flags for all devices.
	 */
	for (i = 0; i < DeviceNumof; i++) {
		dev = &DeviceTable[i];
		dev->tapeclean = TAPECLEAN_LOGSENSE;
	}
}

/*
 * Data field error message function.
 */
static void
msgFunc(
/*LINTED argument unused in function */
	int code,
	char *msg)
{
	strncpy(errMsg, msg, sizeof (errMsg)-1);
}
