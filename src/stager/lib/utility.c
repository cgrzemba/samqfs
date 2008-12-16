/*
 * misc.c
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

#pragma ident "$Revision: 1.30 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
struct tm *localtime_r(const time_t *clock, struct tm *res);
#include <unistd.h>
#include <pthread.h>
#include <procfs.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sam/defaults.h"
#include "sam/uioctl.h"
#include "sam/exit.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "aml/shm.h"
#include "aml/stager_defs.h"
#include "aml/id_to_path.h"

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_shared.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

#define	MAXLINE 1024+256

extern char *GetMountPointName(equ_t fseq);

/* Private functions */
static void waitForDbx();

/*
 * Fatal system call error handler.  Unrecoverable system call
 * error found.  Set exit status and call LibFatal function.
 * The library will log and trace failing system call and errno.
 * Program will exit with specified status.
 */
void
FatalSyscallError(
	const int exitStatus,
	const char *srcFile,
	const int srcLine,
	const char *funcName,
	const char *funcArg)
{
	ErrorExitStatus = exitStatus;
	_LibFatal(srcFile, srcLine, funcName, funcArg);
}

/*
 * Warn system call error handler.  Recoverable system call
 * error found.  Log and trace failing system call and errno.
 */
void
WarnSyscallError(
	const char *srcFile,
	const int srcLine,
	const char *funcName,
	const char *funcArg)
{
	char trBuffer[MAXLINE];

	SendCustMsg(srcFile, srcLine, 19006, funcName, funcArg, errno);

	sprintf(trBuffer, GetCustMsg(19006), funcName, funcArg, errno);
	_Trace(TR_err, srcFile, srcLine, trBuffer);
}

/*
 * Fatal internal error handler.  Unrecoverable internal error was found.
 * Trace failing message and exit daemon process.
 */
void
FatalInternalError(
	const char *srcFile,
	const int srcLine,
	const char *msg)
{
	char trBuffer[MAXLINE];

	SendCustMsg(srcFile, srcLine, 19037, msg);

	sprintf(trBuffer, GetCustMsg(19037), msg);
	_Trace(TR_misc, srcFile, srcLine, trBuffer);

	waitForDbx();

	exit(EXIT_FATAL);
}


/*
 * Warn internal error handler.  Recoverable internal error was found.
 * Trace failing message and return to caller.
 */
void
WarnInternalError(
	const char *srcFile,
	const int srcLine,
	const char *msg)
{
	char trBuffer[MAXLINE];

	SendCustMsg(srcFile, srcLine, 19037, msg);

	sprintf(trBuffer, GetCustMsg(19037), msg);
	_Trace(TR_misc, srcFile, srcLine, trBuffer);

	waitForDbx();
}

char *
GetTime(void)
{
	struct tm tm;
	time_t now;
	static char buffer[80];
	static char *tdformat = "%m/%d %H:%M:%S";

	now = time(NULL);
	strftime(&buffer[0], sizeof (buffer)-1, tdformat,
	    localtime_r(&now, &tm));

	return (&buffer[0]);
}

size_t
GetMemUse(void)
{
	int fd;
	char buf[30];
	psinfo_t currproc;

	sprintf(buf, "/proc/%d/psinfo", (int)getpid());

	currproc.pr_size = 0;
	fd = open(buf, O_RDONLY);
	if (fd >= 0) {
		(void) read(fd, &currproc, sizeof (psinfo_t));
		(void) close(fd);
	}
	return ((currproc.pr_size * KILO)/getpagesize());
}


/*
 * Attach SAM-FS shared memory segment.
 */
void *
ShmatSamfs(
	int mode)
{
	extern shm_alloc_t master_shm;
	extern shm_ptr_tbl_t *shm_ptr_tbl;

	shm_ptr_tbl = NULL;
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) >= 0) {
		master_shm.shared_memory = shmat(master_shm.shmid, NULL, mode);
		if (master_shm.shared_memory != (void *)-1) {
			shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
			if (strcmp(shm_ptr_tbl->shm_block.segment_name,
			    SAM_SEGMENT_NAME) != 0) {
				errno = ENOCSI;
				shm_ptr_tbl = NULL;
			}
		}
	}
	return (shm_ptr_tbl);
}

/*
 * Dump raw data to trace file.
 */
void
TraceRawData(
	int flag,
	char *srcFile,
	int srcLine,
	char *fwa,
	int numBytes)
{
	static char hex[] = "0123456789abcdef";
	char str[80];
	char *s;
	int addr = 0;

	_Trace(flag, srcFile, srcLine, "Raw data 0x%x for %d bytes",
	    (int)fwa, numBytes);

	while (addr < numBytes) {
		int f, n;
		/* LINTED pointer cast may result in improper alignment */
		uint_t *a = (uint_t *)fwa;
		s = str;

		for (f = 1; f <= 4; f++) {
			uint_t v = *a++;

			for (n = 7; n >= 0; n--) {
				*(s + n) = hex[v & 0xf];
				v >>= 4;
			}
			s += 8;
			*s++ = ' ';
			if ((f & 3) == 0) {
				*s++ = ' ';
			}

		}

		for (n = 0; n < 16; n++) {
			char c;

			c = *fwa++ & 0x7F;
			*s++ = (isprint(c)) ? c : '.';
		}

		*s = '\0';
		_Trace(flag, srcFile, srcLine, "0x%08x  %s", addr, str);
		addr += 16;
	}
}

/*
 * Post an operator message.
 */
void
PostOprMsg(
	char *oprMsg,
	int msgNum,		/* Message catalog number. */
	...)
{
	va_list	args;
	char	msg_buf[132];
	char	*fmt;

	/*
	 * Look up message in message catalog.
	 */
	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}

	/*
	 * Do the message formatting.
	 */
	vsnprintf(msg_buf, sizeof (msg_buf), fmt, args);
	va_end(args);

	/*
	 * Copy the message to the operator message area.
	 */
	if (oprMsg != NULL) {
		int msgLength;

		msgLength = strlen(msg_buf);
		if (msgLength > STAGER_OPRMSG_SIZE-1) {
			char *dest;
			int tailLength;

			/*
			 * Message is too long.
			 * Most likely a file or directory name.
			 * Trim the last 'word' to fit by moving the part that
			 * fits within the message buffer.
			 * Preface the 'word' with "...".
			 */
			dest = strrchr(msg_buf, ' ');
			if (dest != NULL) {
				dest++;
				*dest++ = '.';
				*dest++ = '.';
				*dest++ = '.';
				tailLength = STAGER_OPRMSG_SIZE-1 -
				    Ptrdiff(dest, msg_buf);
				if (tailLength >= 8) {
					memmove(dest,
					    msg_buf + msgLength - tailLength,
					    tailLength);
				}
			}
		}
		strncpy(oprMsg, msg_buf, STAGER_OPRMSG_SIZE-1);
	}

	/*
	 * Trace the message if required.
	 */
	if (*TraceFlags & (1 << TR_oprmsg)) {
		_Trace(TR_oprmsg, NULL, 0, "OprMsg %d: %s", msgNum, msg_buf);
	}
}

/*
 * Clear an operator message.
 */
void
ClearOprMsg(
	char *oprMsg)
{
	if (oprMsg != NULL) {
		oprMsg[0] = '\0';
	}
}

/*
 * Returns TRUE if log event is enabled.
 */
boolean_t
IsLogEventEnabled(
	LogType_t type)
{
	boolean_t enabled = B_FALSE;
	int events;

	events = SharedInfo->si_logEvents;

	switch (type) {
		case LOG_STAGE_START:
			if (GET_FLAG(events, STAGER_LOGFILE_START)) {
				enabled = B_TRUE;
			}
			break;

		case LOG_STAGE_DONE:
			if (GET_FLAG(events, STAGER_LOGFILE_FINISH)) {
				enabled = B_TRUE;
			}
			break;

		case LOG_STAGE_CANCEL:
			if (GET_FLAG(events, STAGER_LOGFILE_CANCEL)) {
				enabled = B_TRUE;
			}
			break;

		case LOG_STAGE_ERROR:
			if (GET_FLAG(events, STAGER_LOGFILE_ERROR)) {
				enabled = B_TRUE;
			}
			break;

		default:
			enabled = B_FALSE;
	}

	return (enabled);
}

/*
 * Get full path name for file.
 */
void
GetFileName(
	FileInfo_t *file,
	char *fullpath,
	int fullpath_l,
	int *segment_ord)
{
	extern nl_catd catfd;
	static pthread_mutex_t idToPathMutex = PTHREAD_MUTEX_INITIALIZER;
	char *mount_name;
	char *file_name;
	int file_name_l;
	struct sam_ioctl_idstat idstat;
	int fs_fd;
	union sam_di_ino inode;

	mount_name = GetMountPointName(file->fseq);
	if (mount_name == NULL) {
		char *msg = catgets(catfd, SET, 19016, "Cannot find pathname.");
		strncpy(fullpath, msg, fullpath_l);
		return;
	}

	/*
	 * id_to_path library function is not thread-safe.
	 */
	(void) pthread_mutex_lock(&idToPathMutex);
	file_name = id_to_path(mount_name, file->id);
	(void) strncpy(fullpath, file_name, fullpath_l);
	(void) pthread_mutex_unlock(&idToPathMutex);

	file_name_l = strlen(file_name) + 1;

	/*
	 * Check if segmented file.
	 */
	if (segment_ord != NULL) {
		*segment_ord = 0;
	}
	fs_fd = open(mount_name, O_RDONLY);
	if (fs_fd >= 0) {
		idstat.id = file->id;
		idstat.size = sizeof (struct sam_perm_inode);
		idstat.dp.ptr = (void *)&inode;
		memset(idstat.dp.ptr, 0, sizeof (struct sam_perm_inode));
		if (ioctl(fs_fd, F_IDSTAT, &idstat) >= 0) {
			if (inode.inode.di.status.b.seg_ino) {
				sprintf(&fullpath[file_name_l - 1], "/%d",
				    inode.inode.di.rm.info.dk.seg.ord + 1);
				if (segment_ord != NULL) {
					*segment_ord =
					    inode.inode.di.rm.info.dk.seg.ord +
					    1;
				}
			}
		}
		(void) close(fs_fd);
	}
}

/*
 * Make a directory if it doesn't already exist.
 */
void
MakeDirectory(
	char *name)
{
	int status;
	struct stat sb;

	if (stat(name, &sb) != 0) {
		/*
		 *  Directory does not exist.
		 */
		status = mkdir(name, DIR_MODE);
		if (status != 0) {
			FatalSyscallError(EXIT_NORESTART, HERE, "mkdir", name);
		}
	}
}

#if defined(DEBUG)
static void
waitForDbx()
{
	static int attachDbx = 1;

	while (attachDbx == 1) {
		/* assign attachDbx = 0 */
		sleep(5);
	}
	attachDbx = 1;
}
#else
static void
waitForDbx()
{
}
#endif
