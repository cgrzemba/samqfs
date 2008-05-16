/*
 * control.c - Archiver daemon control module.
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

#pragma ident "$Revision: 1.42 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"

/* Local headers. */
#include "archiverd.h"
#include "device.h"
#include "volume.h"

/* Private data. */

/* Processing functions. */
static void dirRerun(void);
static void dirExec(void);
static void dirLibrary(void);
static void dirRestart(void);
static void dirRmarchreq(void);
static void dirScan(void);
static void dirTrace(void);

static struct Controls {
	char	*CtIdent;
	void	(*CtFunc)(void);
} controls[] = {
	{ "exec", dirExec },
	{ "library", dirLibrary },
	{ "rerun", dirRerun },
	{ "restart", dirRestart },
	{ "rmarchreq", dirRmarchreq },
	{ "scan", dirScan },
	{ "trace", dirTrace },
	{ NULL }
};
static char *dirName;		/* Directive name */
static char errMsg[CTMSG_SIZE];	/* Buffer for error message return */
static char ident[128];
static char *value;
static jmp_buf errReturn;	/* Error message return */

/* Private functions. */
static void changeArfindState(struct FileSysEntry *fs, int state);
static void err(int msgNum, ...);
static struct FileSysEntry *getFs(void);


/*
 * Archiver control.
 */
char *
Control(
	char *identArg,
	char *valueArg)
{
	struct Controls *table;

	/*
	 * Isolate first identifier and look it up.
	 */
	strncpy(ident, identArg, sizeof (ident));
	value = valueArg;
	dirName = strtok(ident, ".");
	for (table = controls; table->CtIdent != NULL; table++) {
		if (strcmp(dirName, table->CtIdent) == 0) {
			break;
		}
	}

	/*
	 * Process the identifier.
	 */
	if (setjmp(errReturn) == 0) {
		if (table->CtIdent == NULL) {
			/* Unknown directive '%s' */
			err(CustMsg(4900), dirName);
		} else {
			table->CtFunc();
			return ("");
		}
	}
	return (errMsg);
}


/* Control functions. */


/*
 * Set archiving execution control.
 */
static void
dirExec(void)
{
	static struct {
		char	*name;
		ExecControl_t ctrl;
		ExecState_t state;
	} stateValues[] = {
		{ "run", EC_run, ES_run },
		{ "idle", EC_idle, ES_wait },
		{ "stop", EC_stop, ES_wait },
		{ "", 0 }
	};
	ExecControl_t ctrl;
	ExecState_t state;
	char	*ident;
	int	i;

	/*
	 * Look up state value.
	 */
	if (*value == '\0') {
		/* %s value missing */
		err(CustMsg(4902), dirName);
	}
	for (i = 0; strcmp(stateValues[i].name, value) != 0; i++) {
		if (*stateValues[i].name == '\0') {
			/* Invalid %s value */
			err(CustMsg(4903), dirName);
		}
	}
	ctrl = stateValues[i].ctrl;
	state = stateValues[i].state;
	ident = strtok(NULL, ".");

	if (AdState->AdExec == ES_init) {
		/* Initializing */
		err(4327);
	}
	if (AdState->AdExec == ES_cmd_errors && state == ES_run) {
		/* Errors in archiver directives - no archiving will be done. */
		err(4023);
	}
	if (AdState->AdExec >= ES_term) {
		/* Restarting */
		err(4007);
	}

	if (ident == NULL) {
		int	i;

		/*
		 * Change all execution controls.
		 */
		if (state == ES_run) {
			if (!ChangeState(ES_run)) {
				return;
			}
		} else {
			/*
			 * Idle, stop.
			 */
			if (!ChangeState(state)) {
				return;
			}
		}
		ScheduleSetDkState(ctrl);
		ScheduleSetRmState(ctrl);
		for (i = 0; i < FileSysTable->count; i++) {
			struct FileSysEntry *fs;

			fs = &FileSysTable->entry[i];
			if (fs->FsAfState->AfExec != ES_fs_wait) {
				changeArfindState(fs, state);
				ScheduleSetFsState(fs, ctrl);
			}
		}
		if (state == ES_run) {
			ScheduleRun("dirExec");
		}
	} else if (strcmp(ident, "dk") == 0) {
		ScheduleSetDkState(ctrl);

	} else if (strcmp(ident, "fs") == 0) {
		struct FileSysEntry *fs;

		fs = getFs();
		if (state == ES_wait) {

			/*
			 * wait acts like wait for an individual file system.
			 */
			state = ES_fs_wait;
		}
		changeArfindState(fs, state);
		ScheduleSetFsState(fs, ctrl);

	} else if (strcmp(ident, "rm") == 0) {
		ScheduleSetRmState(ctrl);

	} else {
		/* Unknown '%s' identifier '%s' */
		err(CustMsg(4901), dirName, ident);
	}
	if (state == ES_run) {
		/*
		 * Any run must start sam-archiverd running.
		 */
		(void) ChangeState(ES_run);
	}
	ScheduleRun("dirExec");
}


/*
 * Set library values.
 */
static void
dirLibrary(void)
{
	struct ArchLibEntry *al;
	char	*name;
	char	*p;
	int	drives;
	int	i;

	/*
	 * Get the library name and find it.
	 */
	name = strtok(NULL, ".");
	if (name == NULL) {
		/* Library name missing */
		err(CustMsg(4411));
	}
	al = &ArchLibTable->entry[0];
	for (i = 0; strcmp(name, al->AlName) != 0; al++, i++) {
		if (i >= ArchLibTable->count) {
			/* Unknown library */
			err(CustMsg(4444), name);
		}
	}

	/*
	 * Check data identifier.
	 */
	name = strtok(NULL, ".");
	if (name == NULL) {
		/* Missing identifier name in '%s' */
		err(CustMsg(4901), ident);
	}
	if (strcmp(name, "drives") != 0) {
		/* Unknown identifier name '%s' */
		err(CustMsg(4900), name);
	}

	/*
	 * Assemble drive count.
	 */
	p = value;
	if (*p == '\0') {
		/* Drive count missing */
		err(CustMsg(4419));
	}
	errno = 0;
	drives = strtoll(value, &p, 0);
	if (errno != 0 || p == value || drives < 0) {
		/* Invalid drive count */
		err(CustMsg(4445));
	}
	if (al->AlFlags & AL_disk || al->AlFlags & AL_honeycomb) {
		al->AlDrivesNumof = al->AlDrivesAvail = drives;
	} else if (drives > al->AlDrivesNumof) {
		/* Too many drives requested. Maximum is %d */
		err(CustMsg(4446), al->AlDrivesNumof);
	}
	al->AlDrivesAllow = drives;
}


/*
 * Soft restart archiver.
 */
static void
dirRerun(void)
{
	(void) ChangeState(ES_rerun);
}


/*
 * Restart archiver.
 */
static void
dirRestart(void)
{
	(void) ChangeState(ES_restart);
}


/*
 * Remove an ArchReq.
 */
static void
dirRmarchreq(void)
{
	struct FileSysEntry *fs;
	char	*arname;

	fs = getFs();
	arname = strtok(NULL, "\0");	/* Get archreq */
	if (arname == NULL) {
		err(CustMsg(4337));
	}
	if (fs->FsAfState->AfPid != 0) {
		char	*msg;

		msg = ArfindControl(fs->FsName, ident, arname, NULL, 0);
		if (*msg != '\0') {
			err(0, msg);
		}
	} else {
		/* Not active */
		err(CustMsg(4337));
	}
}



/*
 * Scan a filesystem.
 */
static void
dirScan(void)
{
	struct FileSysEntry *fs;
	upath_t	newIdent;
	char	*p;

	fs = getFs();
	p = strtok(NULL, "\0");	/* Get remainder of ident */
	if (p != NULL) {
		snprintf(newIdent, sizeof (newIdent), "scan.%s", p);
	} else {
		snprintf(newIdent, sizeof (newIdent), "scan");
	}
	if (fs->FsAfState->AfPid != 0) {
		char	*msg;

		msg = ArfindControl(fs->FsName, newIdent, value, NULL, 0);
		if (*msg != '\0') {
			err(0, msg);
		}
	} else {
		/* Not active */
		err(CustMsg(4337));
	}
}


/*
 * Trace archiver tables.
 */
static void
dirTrace(void)
{
	char	*ident;

	ident = strtok(value, ".");
	if (ident == NULL || strcmp(ident, "all") == 0) {
		ScheduleTrace();
		ComposeTrace();
		QueueTrace(HERE, NULL);
		ChildTrace();
#if defined(DEBUG)
		MapFileTrace();
		TraceRefs();
#endif /* defined(DEBUG) */
		if (ident != NULL) {
			int	i;

			/*
			 * Trace each file system.
			 */
			for (i = 0; i < FileSysTable->count; i++) {
				struct FileSysEntry *fs;

				fs = &FileSysTable->entry[i];
				if (fs->FsAfState->AfPid != 0) {
					(void) ArfindControl(fs->FsName,
					    "trace", "", NULL, 0);
				}
			}
		}
	} else if (strcmp(ident, "fs") == 0) {
		struct FileSysEntry *fs;

		/*
		 * Trace a single file system.
		 */
		fs = getFs();
		if (fs->FsAfState->AfPid != 0) {
			char	*msg;

			msg = ArfindControl(fs->FsName, "trace", "", NULL, 0);
			if (*msg != '\0') {
				err(0, msg);
			}
		} else {
			/* Not active */
			err(CustMsg(4337));
		}

	} else {
		/* Unknown '%s' identifier '%s' */
		err(CustMsg(4901), dirName, ident);
	}
}


/* Private functions. */


/*
 * Change arfind state.
 */
static void
changeArfindState(
	struct FileSysEntry *fs,
	int state)					/* New state */
{
	if (fs->FsAfState->AfPid != 0) {
		if (ArfindChangeState(fs->FsName, state) == 0) {
			return;
		}
	}
	fs->FsAfState->AfExec = state;
}


/*
 * Error return.
 * Compose message in response buffer.
 */
static void
err(
	int msgNum,
	...)
{
	va_list args;
	char *fmt;

	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}
	vsnprintf(errMsg, sizeof (errMsg), fmt, args);
	va_end(args);
	longjmp(errReturn, 1);
}


/*
 * Get FileSysEntry.
 */
static struct FileSysEntry *
getFs(void)
{
	struct FileSysEntry *fs;
	char	*fsname;
	int	i;

	fsname = strtok(NULL, ".");
	if (fsname == NULL) {
		/* Filesystem name missing */
		err(CustMsg(4412));
	}

	for (i = 0; i < FileSysTable->count; i++) {
		fs = &FileSysTable->entry[i];
		if (strcmp(fs->FsName, fsname) == 0) {
			break;
		}
	}
	if (i >= FileSysTable->count) {
		/* Unknown file system */
		err(CustMsg(4449));
	}
	return (fs);
}
