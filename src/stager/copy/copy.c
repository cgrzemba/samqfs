/*
 * copy.c - stager daemon copy's main program
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

#pragma ident "$Revision: 1.37 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

/* POSIX headers. */
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "sam/defaults.h"
#include "sam/names.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "aml/stager.h"
#include "sam/signals.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

#include "stager_config.h"
#include "stager_lib.h"
#include "stage_reqs.h"
#include "file_defs.h"
#include "copy_defs.h"
#include "filesys.h"
#include "copy.h"

/*
 * Public data.
 * Define or declare access to global variables which are visible
 * to all files in the program.
 */

/*
 * Program name.
 */
char *program_name;

/*
 * Data shared with copy procs.
 */
SharedInfo_t *SharedInfo = NULL;

/*
 * Stager's state information.
 */
StagerStateInfo_t *State = NULL;

/*
 * Copy process context.
 */
CopyInstance_t *Context = NULL;

/*
 * Set of signals to block.  Only the thread that calls
 * sigwait() will receive the signals.
 */
sigset_t signalSet;

/*
 * Number of open disk cache files in copy process.  An open
 * file can be maintained during error recovery and multivolume.
 * A count is used so process is not timed out while waiting for
 * another volume of media to arrive.
 */
int NumOpenFiles = 0;

/*
 * SAM-FS shared memory segment.
 */
shm_alloc_t   master_shm;
shm_ptr_tbl_t *shm_ptr_tbl;

/*
 * SAM-FS defaults shared memory segment.
 */
sam_defaults_t  *Defaults;

/*
 * Message catalog descriptor.
 */
nl_catd catfd = NULL;

/*
 * Private data.
 * Define static variables which are only visible in the
 * remainder of this source file.
 */

/*
 * Contexts for all procs.
 */
CopyProcList_t *copyprocList;

static upath_t	fullpath;

/* Private functions. */
static void initCopy();
static CopyInstance_t *getCopyProc(int rmfn);

static void fatalSignalCleanup(int signum);
static void sigHup(int signum);
static void sigTerm(int signum);
static void sigUsr1(int signum);

static struct SignalFunc signalFunc[] = {
	{ SIGHUP,   sigHup },
	{ SIGINT,   sigTerm },
	{ SIGTERM,  sigTerm },
	{ SIGUSR1,  sigUsr1 },
	{ 0 }
};

static struct SignalsArg signalsArg = {
	signalFunc, fatalSignalCleanup, "."
};

int
main(
	int argc,
	char *argv[])
{
	char *trace_name = NULL;
	int rmfn;

	if (argc != 2) {
		ASSERT_NOT_REACHED();
		return (EXIT_NORESTART);
	}
	(void) sscanf(argv[1], "%i", &rmfn);

	program_name = argv[0];
	CustmsgInit(1, NULL);

	/*
	 *  Set signal handling.
	 */
	snprintf(signalsArg.SaCoreDir, sizeof (signalsArg.SaCoreDir),
	    SAM_STAGERD_HOME"/%s", argv[1]);
	(void) Signals(&signalsArg);

	/*
	 * Map in stager's shared data.
	 */
	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, SHARED_FILENAME);
	SharedInfo = (SharedInfo_t *)MapInFile(fullpath, O_RDWR, NULL);
	if (SharedInfo == NULL) {
		FatalSyscallError(EXIT_NORESTART, HERE, "MapInFile", fullpath);
	}

	/*
	 * Map in stager's state information.
	 */
	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, STAGER_STATE_FILENAME);
	State = (StagerStateInfo_t *)MapInFile(fullpath, O_RDWR, NULL);

	/*
	 * Prepare tracing.
	 */
	sprintf(fullpath, "copy-rm%d", rmfn);
	SamStrdup(trace_name, fullpath);

	TraceInit(trace_name, TI_stager | TR_MPLOCK);

	/*
	 * Change directory.  This keeps the core file separate.
	 */
	sprintf(fullpath, "%s/%s/rm%d", SAM_VARIABLE_PATH,
	    STAGER_DIRNAME, rmfn);
	if (chdir(fullpath) == -1) {
		FatalSyscallError(EXIT_NORESTART, HERE, "chdir", fullpath);
	}

	Trace(TR_PROC, "Copy proc started");

	/*
	 * Initialize copy helper.
	 */
	initCopy();

	Context = getCopyProc(rmfn);
	ASSERT(Context != NULL);

	/*
	 * Enter loop to copy all files in stream.
	 */
	CopyFiles();

	sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, SHARED_FILENAME);
	UnMapFile((void *)SharedInfo, sizeof (SharedInfo_t));

	Trace(TR_PROC, "Copy process exiting");

	return (EXIT_SUCCESS);
}

/*
 * Find context for another copy proc.
 */
CopyInstance_t *
FindCopyProc(
	int lib,
	media_t media)
{
	int i;

	for (i = 0; i < SharedInfo->num_copyprocs; i++) {
		if (copyprocList->data[i].lib == lib) break;
	}
	ASSERT(i >= 0 && i < SharedInfo->num_copyprocs);

	if (copyprocList->data[i].created == B_FALSE) {
		copyprocList->data[i].media = media;
		copyprocList->data[i].num_buffers = 4;
	}
	return (&copyprocList->data[i]);
}

/*
 * Initialize copy process.
 */
static void
initCopy(void)
{
	/*
	 * Map in request list.
	 */
	MapInRequestList();

	/*
	 * Map in copy proc contexts.
	 */
	copyprocList = (CopyProcList_t *)MapInFile(
	    SharedInfo->copyprocsFile, O_RDWR, NULL);
	if (copyprocList == NULL) {
		FatalSyscallError(EXIT_NORESTART, HERE, "MapInFile",
		    SharedInfo->copyprocsFile);
	}

	/*
	 * Map in filesystem data.
	 */
	MapInFileSystem();
}

/*
 * Get context for copy proc.
 */
static CopyInstance_t *
getCopyProc(
	int rmfn)
{
	int i;

	for (i = 0; i < SharedInfo->num_copyprocs; i++) {
		if (copyprocList->data[i].rmfn == rmfn) break;
	}
	ASSERT(i >= 0 && i < SharedInfo->num_copyprocs);
	return (&copyprocList->data[i]);
}

/*
 * Fatal signal cleanup.
 */
static void
fatalSignalCleanup(
	/* LINTED argument unused in function */
	int signum)
{
}

static void
sigHup(
	/* LINTED argument unused in function */
	int signum)
{
	TraceReconfig();
}

static void
sigTerm(
	int signum)
{
	Trace(TR_MISC, "Shutdown by signal %d", signum);
	SET_FLAG(Context->flags, CI_shutdown);
	/*
	 *  Tell copyfile thread that a shutdown has been
	 *  requests.  This will unblock the thread waiting on
	 *  the condition variable.
	 */
	(void) pthread_mutex_lock(&Context->request_mutex);
	(void) pthread_cond_signal(&Context->request);
	(void) pthread_mutex_unlock(&Context->request_mutex);
}

static void
sigUsr1(
	int signum)
{
	Trace(TR_MISC, "Shutdown for voluntary failover by signal %d", signum);
	SET_FLAG(Context->flags, CI_failover);
	/*
	 *  Tell copyfile thread that a shutdown for a voluntary
	 *  failover has been requested. This will unblock the thread
	 *  waiting on the condition variable.
	 */
	(void) pthread_mutex_lock(&Context->request_mutex);
	(void) pthread_cond_signal(&Context->request);
	(void) pthread_mutex_unlock(&Context->request_mutex);
}
