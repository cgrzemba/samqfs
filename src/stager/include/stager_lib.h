/*
 * stager_lib.h - Stager Library DefInitions
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

#if !defined(STAGER_LIB_H)
#define	STAGER_LIB_H

#pragma ident "$Revision: 1.27 $"

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

/* Log type definitions. */
typedef enum {
    LOG_STAGE_START,
    LOG_STAGE_DONE,
    LOG_STAGE_CANCEL,
    LOG_STAGE_ERROR
} LogType_t;

extern boolean_t traceOn;

/*
 * Define prototypes in utility.c
 */
void FatalSyscallError(const int exitStatus, const char *srcFile,
	const int srcLine, const char *funcName, const char *funcArg);
void WarnSyscallError(const char *srcFile,
	const int srcLine, const char *funcName, const char *funcArg);
void FatalInternalError(const char *srcFile, const int srcLine,
    const char *msg);
void WarnInternalError(const char *srcFile, const int srcLine,
    const char *msg);

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
