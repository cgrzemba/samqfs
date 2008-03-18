/*
 * utility.c - Utility functions.
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

#pragma ident "$Revision: 1.49 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/param.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/defaults.h"
#include "sam/uioctl.h"
#include "aml/fsd.h"
#include "aml/shm.h"
#include "sam/sam_trace.h"

/* Local headers. */
#include "common.h"
#include "threads.h"
#include "utility.h"

#if defined(lint)	/* Suppress complaints about unused global functions. */
#include "archset.h"
#include "archreq.h"
#include "dir_inode.h"
#include "volume.h"
#include "device.h"
#endif /* defined(lint) */

/* Private Data. */
static char *oprMsg;	/* Where operator messages go. */
			/* Unique to instantiation of module. */
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * Clear operator message.
 */
void
ClearOprMsg(void)
{
	if (oprMsg != NULL) {
		*oprMsg = '\0';
#if defined(lint)	/* Suppress complaints about unused global functions. */
	} else {
struct sam_id id = { 0, 0 };

ArchReqFileName(NULL, ScrPath);
ArchReqMsg(NULL, NULL, NULL, 0);
(void) ArchReqName(NULL, NULL);
(void) ArchReqPrint(NULL, NULL, 0);
(void) ArchReqTrace(NULL, 0);
DeviceCheck();
DeviceConfig(AP_archiverd);
(void) GetPinode(id, NULL);
(void) VolumeCatalog();
VolumeConfig();
Daemon = FALSE;
MntPoint = NULL;

#endif /* defined(lint) */
	}
}


/*
 * Return pointer to comma separated count conversion.
 * RETURN: The start of the string.
 */
char *
CountToA(
	uint64_t v)	/* Value to convert. */
{
	static	char buf[32];
	char	*p;
	int	NumofDigits;

	p = buf + sizeof (buf) - 1;
	*p-- = '\0';
	NumofDigits = 0;
	/* Generate digits in reverse order. */
	do {
		if (NumofDigits++ >= 3) {
			*p-- = ',';
			NumofDigits = 1;
		}
		*p-- = v % 10 + '0';
	} while ((v /= 10) > 0);
	return (p+1);
}


/*
 * Return name of execution state.
 */
char *
ExecStateToStr(
	ExecState_t state)
{
	char *stNames[] = { "none", "init", "cmd_errors", "idle", "wait",
		"fs_wait", "run", "umount", "term", "rerun", "restart" };

	if (state < ES_none || state >= ES_max) {
		static char string[] = "00000000";

		snprintf(string, sizeof (string), "%d", state);
		return (string);
	}
	return (stNames[state]);
}


/*
 * Lock the log file.
 */
void
LogLock(
	FILE *st)
{
	flock64_t arg;
	int	fd;

	PthreadMutexLock(&logMutex);
	fd = fileno(st);
retry:
	memset(&arg, 0, sizeof (arg));
	arg.l_type = F_WRLCK;
	arg.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLKW64, &arg) == -1) {
		if (errno == EINTR || errno == EDEADLK) {
			goto retry;
		}
		LibFatal(SetLock, "log");
	}
}


/*
 * Unlock the log file.
 */
void
LogUnlock(
	FILE *st)
{
	flock64_t arg;
	int	fd;

	fd = fileno(st);
	memset(&arg, 0, sizeof (arg));
	arg.l_type = F_UNLCK;
	arg.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK64, &arg) == -1) {
		LibFatal(ClrLock, "log");
	}
	PthreadMutexUnlock(&logMutex);
}


/*
 * Call notify facility.
 */
void
NotifyRequest(
	int priority,
	int msgNum,
	char *msg)
{
	if (FsdNotify(AdState->AdNotifyFile, priority, msgNum, msg) == -1) {
		Trace(TR_ERR, "FsdNotify(%s) failed", msg);
	}
}


/*
 * Initialize operator message processing.
 * Set the operator message address.  This is different for each instance
 * of the archiver programs.
 */
void
OpenOprMsg(
	char *oprMsgLoc)
{
	oprMsg = oprMsgLoc;
}


/*
 * Post an operator message.
 * The message text is found in the message catalog.
 * Formatting like printf().
 * Edit message to fit the operator message areas.
 * OpenOprMsg() must have been called first.
 */
void
PostOprMsg(
	int msgNum,	/* Message catalog number. */
	...)
{
	va_list	args;
	char	msg_buf[MAXLINE];
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
		int	msgLength;

		msgLength = strlen(msg_buf);
		if (msgLength > OPRMSG_SIZE-1) {
			char	*dest;
			int	tailLength;

			/*
			 * Message is too long.
			 * Most likely a file or directory name.
			 * Trim the last 'word' to fit by using only the the
			 * last part of the 'word' that fits within the message
			 * buffer.  Preface the 'word' with "...".
			 */
			dest = strrchr(msg_buf, ' ');
			if (dest != NULL) {
				dest++;
				*dest++ = '.';
				*dest++ = '.';
				*dest++ = '.';
				tailLength =
				    OPRMSG_SIZE-1 - Ptrdiff(dest, msg_buf);
				if (tailLength >= 8) {
					memmove(dest,
					    msg_buf + msgLength - tailLength,
					    tailLength);
				}
			}
		}
		strncpy(oprMsg, msg_buf, OPRMSG_SIZE-1);
	}

	/*
	 * Trace the message if required.
	 */
	if (*TraceFlags & (1 << TR_oprmsg)) {
		msg_buf[OPRMSG_SIZE-1] = '\0';
		_Trace(TR_oprmsg, NULL, 0, "OprMsg %d: %s", msgNum, msg_buf);
	}
}


/*
 * Convert a scan path for a message.
 * Scan paths have several forms.
 */
char *
ScanPathToMsg(
	char *path)
{
	if (path == NULL || *path == '\0') {
		return (".");
	}
	if (*path == '/') {
		return (".inodes");
	}
	return (path);
}


/*
 * Attach SAM-FS shared memory segment.
 */
void *
ShmatSamfs(
	int	mode)	/* O_RDONLY = read only, read/write otherwise */
{
	/*
	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		return (NULL);
	}
	mode = (mode == O_RDONLY) ? SHM_RDONLY : 0;
	master_shm.shared_memory = shmat(master_shm.shmid, NULL, mode);
	if (master_shm.shared_memory == (void *)-1) {
		return (NULL);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
		errno = ENOCSI;
		return (NULL);
	}
	return (shm_ptr_tbl);
}


/*
 * Return name of signal.
 */
char *
StrSignal(
	int sig)
{
	char	*str;

	str = strsignal(sig);
	if (str == NULL) {
		static char string[] = "00000000";

		snprintf(string, sizeof (string), "%d", sig);
		str = string;
	}
	return (str);
}


/*
 * Convert time to a ISO string.
 */
char *
TimeToIsoStr(
	time_t tv_arg,
	char *buf)
{
	static char strBuf[ISO_STR_FROM_TIME_BUF_SIZE];
	struct tm tm;
	time_t	tv = tv_arg;

	if (buf == NULL) {
		buf = strBuf;
	}
	strftime(buf, sizeof (strBuf), "%Y-%m-%d %T", localtime_r(&tv, &tm));
	return (buf);
}
