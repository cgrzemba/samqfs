/*
 * stager_lib.h - Stager Library DefInitions
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

#if !defined(STAGER_LIB_H)
#define	STAGER_LIB_H

#pragma ident "$Revision: 1.25 $"

/* POSIX headers. */
#include <pthread.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/param.h"
#include "sam/sam_trace.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "stager_shared.h"
#include "log_defs.h"

/*
 * Macros for manipulating a bit mask used as flags.
 */
#define	GET_FLAG(flags, bit)	(flags & bit)
#define	SET_FLAG(flags, bit)	(flags |= bit)
#define	CLEAR_FLAG(flags, bit)	(flags &= ~bit)

#define	KILO 1024
#define	MEGA 1024 * 1024
#define	GIGA 1024 * 1024 * 1024

/*
 * Macro to determine if running with process shared attribute set
 * to PTHREAD_PROCESS_SHARED or if process shared attribute is
 * set to PTHREAD_PROCESS_PRIVATE.  See man pthread_mutexattr_setpshared.
 */
#define	IS_PROCESS_SHARED() \
	(SharedInfo->processSharedAttr == PTHREAD_PROCESS_SHARED)
#define	IS_PROCESS_PRIVATE() \
	(SharedInfo->processSharedAttr == PTHREAD_PROCESS_PRIVATE)

#define	PATHBUF_SIZE ((2*MAXPATHLEN)+4)		/* size of path buffer */

/*
 * Define macros to determine if filesystem is mounted or umounted.  A mounted
 * filesystem also implies that an unmount is not in progress.  An umounted
 * filesystem indicates an umount may be in progress.
 */
#define	IS_FILESYSTEM_MOUNTED(x) \
	((x & FS_MOUNTED) && !(x & FS_UMOUNT_IN_PROGRESS))
#define	IS_FILESYSTEM_UMOUNTED(x) \
	(!(x & FS_MOUNTED) || (x & FS_UMOUNT_IN_PROGRESS))

/*
 * Macros to determine if filesystem is shared and we are the shared client.
 */
#define	IS_SHARED_CLIENT(x)	((x) & FS_CLIENT)

/* Structures. */
typedef struct ThreadSema {
	pthread_mutex_t	mutex;		/* protect access to count */
	pthread_cond_t	cv;		/* signals change to count */
	int		count;		/* protected count */
} ThreadSema_t;

typedef struct ThreadState {
	pthread_mutex_t mutex;		/* protect access to state */
	pthread_cond_t	cv;		/* signals change to state */
	int		state;		/* protected state */
} ThreadState_t;

/*
 * Structure representing communication to another thread.
 */
typedef struct ThreadComm {
	pthread_mutex_t	mutex;		/* protect access to data */
	pthread_cond_t	avail;		/* data available */
	int		data_ready;	/* data present */
	void		*first;
	void		*last;
	pthread_t	tid;		/* thread id */
} ThreadComm_t;

#define	THREAD_INIT_LOCK(x, y)	(pthread_mutex_init(&(x->mutex), y));
#define	THREAD_LOCK(x)		(pthread_mutex_lock(&(x->mutex)));
#define	THREAD_UNLOCK(x)	(pthread_mutex_unlock(&(x->mutex)));

extern boolean_t traceOn;

/* Functions */
void ThreadSemaInit(ThreadSema_t *s, int value);
void ThreadSemaWait(ThreadSema_t *s);
void ThreadSemaPost(ThreadSema_t *s);

void ThreadStateInit(ThreadState_t *s);
void ThreadStateWait(ThreadState_t *s);
void ThreadStatePost(ThreadState_t *s);

/*
 * Define prototypes in utility.c
 */
void FatalSyscallError(const int exitStatus, const char *srcFile,
	const int srcLine, const char *funcName, const char *funcArg);
void WarnSyscallError(const char *srcFile,
	const int srcLine, const char *funcName, const char *funcArg);
char *GetTime();
size_t GetMemUse();
void *ShmatSamfs(int mode);
void FatalSignalHandler(int signum, char *core_dir);
void TraceRawData(int flag, char *srcFile, int srcLine,
	char *fwa, int numBytes);
void PostOprMsg(char *oprMsg, int msgNum, ...);
void ClearOprMsg(char *oprMsg);
void MakeDirectory(char *name);

void* MapInFile(char *file_name, int mode, size_t *len);
int WriteMapFile(char *file_name, void *mp, size_t len);
void UnMapFile(void *mp, size_t len);
void RemoveMapFile(char *file_name, void *mp, size_t len);
boolean_t IsLogEventEnabled(LogType_t type);
void GetFileName(FileInfo_t *file, char *fullpath, int fullpath_l,
	int *segment_ord);

#endif /* STAGER_LIB_H */
