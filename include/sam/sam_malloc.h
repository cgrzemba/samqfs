/*
 * sam_malloc.h - SAM memory allocation function definitions.
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

#ifndef SAM_MALLOC_H
#define	SAM_MALLOC_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif

/* Memory allocation macros to pass source line information. */
#define	SamStrdup(o, s) _SamStrdup(_SrcFile, __LINE__, &o, s, #o)
#define	SamFree(o) _SamFree(_SrcFile, __LINE__, o, #o)
#define	SamMalloc(o, s) _SamMalloc(_SrcFile, __LINE__, (void **)&o, s, #o, 0)
#define	SamRealloc(o, s) _SamRealloc(_SrcFile, __LINE__, (void **)&o, s, #o)
#define	SamValloc(o, s) _SamMalloc(_SrcFile, __LINE__, (void **)&o, s, #o, 1)

/* Functions. */
void _SamStrdup(char *SrcFile, int SrcLine, char **Object, char *str,
	char *ObjectName);
void _SamFree(char *SrcFile, int SrcLine, void *Object, char *ObjectName);
void _SamMalloc(char *SrcFile, int SrcLine, void **Object, size_t size,
	char *ObjectName, int PageAlign);
void _SamRealloc(char *SrcFile, int SrcLine, void **Object, size_t size,
	char *ObjectName);

#if defined(DEBUG)
void TraceRefs(void);
#endif /* defined(DEBUG) */

#endif /* SAM_MALLOC_H */
