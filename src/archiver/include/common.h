/*
 * common.h - Archiver common interface definitions.
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

#ifndef COMMON_H
#define	COMMON_H

#pragma ident "$Revision: 1.38 $"

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#define	ARCHIVER_PRIVATE
#include "aml/archiver.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/shm.h"

/* Macros. */
#define	EXIT_WAIT 0x10		/* archiver exit status - 'wait' directive */
#define	EXIT_ERRORS 0x20	/* archiver exit status - directive errors */
#define	DIR_MODE 0750		/* Create mode for archiver's directories */
#define	FILE_MODE 0640		/* Create mode for archiver's files */
#define	WAIT_TIME (30)		/* Used for timed waits */
#define	LONG_TIME (24*60*60)	/* Used for a long delay */
#define	MAXLINE 1024+256
#if defined(_LP64)		/* Maximum time value */
#define	TIME_MAX (9223372036854775807L)
#else /* defined(_LP64) */
#define	TIME_MAX (2147483647)
#endif /* defined(_LP64) */

/* Additional trace debug flags. */
#define	TR_ARDEBUG TR_ardebug, _SrcFile, __LINE__
#if defined(AR_DEBUG)
#define	TR_ardebug (TR_debug)
#define	TR_TEMP TR_debug, _SrcFile, __LINE__
#else /* defined(AR_DEBUG) */
#define	TR_ardebug (TR_MAX + 1)
#endif /* defined(AR_DEBUG) */

/* Function macros. */
#define	max(a, b) (((a) > (b)) ? (a) : (b))
#define	min(a, b) (((a) < (b)) ? (a) : (b))

/* Number of elements in array a. */
#define	NUMOF(a) (sizeof (a)/sizeof (*(a)))

/* Debugging archiver by itself. */
#if defined(AR_DEBUG)
#undef DBXWAIT
#define	DBXWAIT { static int w = 1; while (w); }
#define	KLUDGE 23
#endif /* defined(AR_DEBUG) */

/* Archiver program identifiers. */
typedef enum {
	AP_archiverd = 1,
	AP_archiver,
	AP_arcopy,
	AP_arfind
} ArchProg_t;

/* Public data declaration/initialization macros. */
#undef DCL
#undef IVAL
#if defined(DEC_INIT)
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */

/* Shared memory segment pointers. */
#if defined(SHM_MASTER_KEY)
DCL shm_alloc_t master_shm IVAL(/* MACRO for cstyle */ {NULL});
DCL shm_ptr_tbl_t *shm_ptr_tbl IVAL(NULL);
#endif /* defined(SHM_MASTER_KEY) */

DCL struct ArchiverdState *AdState IVAL(NULL);
DCL struct ArfindState *AfState IVAL(NULL);
DCL boolean_t Daemon IVAL(TRUE);
DCL char *MntPoint;		/* Filesystem mount point */
DCL int FsFd IVAL(-1);		/* File descriptor for ioctl-s */

#if defined(MAXPATHLEN)
DCL char ScrPath[2*(MAXPATHLEN+4)]; /* A place for assembling full path names */
#endif /* defined(MAXPATHLEN) */

/* Public functions. */
void *ArMapFileAttach(char *fileName, uint_t magic, int mode);
int ArMapFileDetach(void *mf_a);
void *MapFileCreate(char *fileName, uint_t magic, size_t size);
void *MapFileGrow(void *mf, size_t incr);
int MapFileRename(void *mf, char *newName);
void MapFileTrace(void);

#if defined(lint)
#include "sam/lint.h"
#define	memmove (void)memmove
#undef mutex_lock
#undef mutex_unlock
#undef unlink
#endif /* defined(lint) */

#endif /* COMMON_H */
