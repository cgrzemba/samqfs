/*
 * sam_malloc.c - SAM memory allocation functions.
 *
 * The SAM memory allocation functions are cover functions for the
 * standard memory allocation functions.  Argument validation is performed
 * and out of memory error conditions are diagnosed.
 *
 * The arguments are the same as the standard functions.  The functions
 * will never return NULL.  Instead, the program is aborted if an out
 * of memory condition occurs.
 *
 * If the symbol DEBUG is defined, memory allocated will be filled with
 * a pattern.  Memory to be freed will be filled with different pattern.
 *
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

#pragma ident "$Revision: 1.18 $"

/* Feature test switches. */
/* DEBUG  If defined, allocated memory will be filled with a pattern. */

/* ANSI C headers. */
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"

/* Macros. */

#define	FREE_FILL   0x3a
#define	MALLOC_FILL 0xa3

/* Private data. */

#if defined(DEBUG)
static pthread_mutex_t RefMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Private functions. */
#if defined(DEBUG)
static void AssertAlloc(char *SrcFile, int SrcLine, char *fmt, ...);
static void AddRef(char *ObjectName, void *Object, size_t size);
static void ChgRef(char *ObjectName, void *OldObject, void *Object,
	size_t size);
static void FreeRef(char *SrcFile, int SrcLine, void *Object);
#endif /* defined(DEBUG) */


/*
 * Duplicate a string.
 */
void
_SamStrdup(
	char *SrcFile,		/* Caller's source file */
	int SrcLine,		/* Caller's source line */
	char **Object,		/* New string object */
	char *str,		/* String to duplicate */
	char *ObjectName)	/* Name of new string object */
{
	size_t	size;

	if (str == NULL) {
#if defined(DEBUG)
		AssertAlloc(SrcFile, SrcLine,
		    "SamStrDup(%s): NULL source string", ObjectName);
#endif /* defined(DEBUG) */
		return;
	}
	size = strlen(str) + 1;
	*Object = malloc(size);
	if (NULL == *Object) {
		_Trace(TR_err, SrcFile, SrcLine, "SamStrdup(%s, %s, %d) failed",
		    ObjectName, str, size);
		Nomem("SamStrdup", ObjectName, size);
		/* No Return */
	}
	strcpy(*Object, str);
#if defined(DEBUG)
	AddRef(ObjectName, *Object, size);
#endif /* defined(DEBUG) */
	_Trace(TR_alloc, SrcFile, SrcLine, "SamStrdup(%s, %s, %d)",
	    ObjectName, str, size);
}


/*
 * Free memory block.
 */
void
_SamFree(
	char *SrcFile,		/* Caller's source file */
	int SrcLine,		/* Caller's source line */
	void *Object,		/* Object to free */
	char *ObjectName)	/* Name of object being freed */
{
	if (Object == NULL) {
#if defined(DEBUG)
		_Trace(TR_debug, SrcFile, SrcLine, "SamFree(%s): NULL pointer",
		    ObjectName);
#endif /* defined(DEBUG) */
		return;
	}

#if defined(DEBUG)
	memset(Object, FREE_FILL, 1);
	FreeRef(SrcFile, SrcLine, Object);
#endif /* defined(DEBUG) */
	free(Object);
	_Trace(TR_alloc, SrcFile, SrcLine, "SamFree(%s)", ObjectName);
}


/*
 * Assign new memory block.
 */
void
_SamMalloc(
	char *SrcFile,		/* Caller's source file */
	int SrcLine,		/* Caller's source line */
	void **Object,		/* Object to be allocated */
	size_t size,		/* Size of block */
	char *ObjectName,	/* Name of object being allocated */
	int PageAlign)		/* If set, page allign object */
{
	if (size == 0 || size >= SIZE_MAX) {
#if defined(DEBUG)
		AssertAlloc(SrcFile, SrcLine,
		    "SamMalloc(%s, %u): invalid size", ObjectName, size);
#endif /* defined(DEBUG) */
		_Trace(TR_err, SrcFile, SrcLine,
		    "SamMalloc(%s, %u) failed, invalid size",
		    ObjectName, size);
		*Object = NULL;
		return;
	}

	if (PageAlign != 0) {
		*Object = valloc(size);
	} else {
		*Object = malloc(size);
	}
	if (NULL == *Object) {
		_Trace(TR_err, SrcFile, SrcLine, "SamMalloc(%s, %u) failed",
		    ObjectName, size);
		Nomem("SamMalloc", ObjectName, size);
		/* No Return */
	}

	_Trace(TR_alloc, SrcFile, SrcLine, "SamMalloc(%s, %u)",
	    ObjectName, size);
#if defined(DEBUG)
	memset(*Object, MALLOC_FILL, size);
	AddRef(ObjectName, *Object, size);
#endif /* defined(DEBUG) */
}


/*
 * Resize memory block.
 */
void
_SamRealloc(
	char *SrcFile,		/* Caller's source file */
	int SrcLine,		/* Caller's source line */
	void **Object,		/* Object being resized */
	size_t size,		/* Size of block */
	char *ObjectName)	/* Name of object being resized */
{
	void	*p;

	if (size == 0 || size >= SIZE_MAX) {
#if defined(DEBUG)
		AssertAlloc(SrcFile, SrcLine,
		    "SamMalloc(%s, %u): invalid size", ObjectName, size);
#endif /* defined(DEBUG) */
		return;
	}

	p = *Object;
	*Object = realloc(p, size);
#if defined(DEBUG)
	ChgRef(ObjectName, p, *Object, size);
#endif /* defined(DEBUG) */
	if (NULL == *Object) {
		_Trace(TR_err, SrcFile, SrcLine, "SamRealloc(%s, %u) failed",
		    ObjectName, size);
		Nomem("SamRealloc", ObjectName, size);
		/* No Return */
	}
	_Trace(TR_alloc, SrcFile, SrcLine, "SamRealloc(%s, %u)",
	    ObjectName, size);
}


#if defined(DEBUG)
/*
 * Process failed memory allocation assertion.
 */
static void
AssertAlloc(
	char *SrcFile,		/* Caller's source file */
	int SrcLine,		/* Caller's source line */
	char *fmt,
	...)
{
	va_list args;
	char msg_buf[256];
	char *msg;

	strcpy(msg_buf, "Memory allocation error: ");
	msg = msg_buf + strlen(msg_buf);
	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	AssertMessage(SrcFile, SrcLine, msg_buf);
	abort();
}


/*
 * Memory allocation tracking functions.
 * Each SamMalloc(), SamRealloc(), and SamFree() memory allocation are
 * kept in a linked list that records the Object name, Object and its
 * size.
 */

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

static struct RefInfo {
	struct RefInfo *next;
	char name[16];
	void *	object;
	size_t	size;
} * RefHead = NULL;

static size_t Alloc = 0;
static size_t MaxAlloc = 0;

/*
 * Add a memory reference.
 */
static void
AddRef(
	char *ObjectName,
	void *Object,
	size_t size)
{
	struct RefInfo *ri;

	/*
	 * Make the reference entry.
	 */
	ri = (struct RefInfo *)malloc(sizeof (struct RefInfo));
	ASSERT(ri != NULL);
	ri->object = Object;
	ri->size   = size;
	strncpy(ri->name, ObjectName, sizeof (ri->name)-1);

	/*
	 * Add it to the list.
	 */
	(void) pthread_mutex_lock(&RefMutex);
	ri->next = RefHead;
	RefHead = ri;

	/*
	 * Set allocation used.
	 */
	Alloc += size;
	if (Alloc > MaxAlloc)  MaxAlloc = Alloc;
	(void) pthread_mutex_unlock(&RefMutex);
}


/*
 * Change a memory reference.
 */
static void
ChgRef(
	char *ObjectName,
	void *OldObject,
	void *Object,
	size_t size)
{
	struct RefInfo *ri;

	/*
	 * Find the referenced object.
	 */
	for (ri = RefHead; ri != NULL; ri = ri->next) {
		if (ri->object == OldObject) {
			break;
		}
	}
	if (NULL == ri) {
		/*
		 * A new object - realloc(NULL);
		 */
		AddRef(ObjectName, Object, size);
	} else {

		/*
		 * Update allocation used.
		 */
		(void) pthread_mutex_lock(&RefMutex);
		Alloc -= ri->size;
		Alloc += size;
		if (Alloc > MaxAlloc)  MaxAlloc = Alloc;
		ri->object = Object;
		ri->size   = size;
		(void) pthread_mutex_unlock(&RefMutex);
	}
}


/*
 * Free a memory reference.
 */
static void
FreeRef(
	char *SrcFile,
	int SrcLine,
	void *Object)
{
	struct RefInfo *ri;
	struct RefInfo *ri_prev;

	/*
	 * Find and delink the object.
	 */
	(void) pthread_mutex_lock(&RefMutex);
	ri_prev = NULL;
	for (ri = RefHead; ri != NULL; ri = ri->next) {
		if (ri->object == Object) {
			if (NULL == ri_prev) {
				RefHead = ri->next;
			} else {
				ri_prev->next = ri->next;
			}
			break;
		}
		ri_prev = ri;
	}
	if (ri != NULL) {

		/*
		 * Update the allocation used.
		 */
		Alloc -= ri->size;

		/*
		 * Dispose of reference.
		 */
		memset(ri, MALLOC_FILL, sizeof (struct RefInfo));
		free(ri);
		(void) pthread_mutex_unlock(&RefMutex);
	} else {
		TraceRefs();
		errno = 0;
		SysError(SrcFile, SrcLine, "SamFree(): unallocated object");
		(void) pthread_mutex_unlock(&RefMutex);
		abort();
	}
}


/*
 * Trace memory references.
 * List out the memory allocation references.
 */
void
TraceRefs(void)
{
	struct RefInfo *ri;
	FILE *st;
	size_t	size;
	int n;

	Trace(TR_DEBUG, "Memory allocations");
	st = TraceOpen();
	if (NULL == st)
		return;

	fprintf(st, "  Alloc = %u, MaxAlloc = %u\n", Alloc, MaxAlloc);
	n = 0;
	size = 0;
	for (ri = RefHead; ri != NULL; ri = ri->next) {
		n++;
		fprintf(st, "  %d: %s %u %p\n",
		    n, ri->name, ri->size, ri->object);
		size += ri->size;
	}
	fprintf(st, " Total size = %u\n", size);
	TraceClose(-1);
	Alloc = size;
	MaxAlloc = size;
}

#endif /* defined(DEBUG) */
