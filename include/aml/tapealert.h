/*
 * tapealert.h - define macros for tapealert interface
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

#ifndef _TAPEALERT_H
#define	_TAPEALERT_H

#pragma ident "$Revision: 1.13 $"

/* probe for tapealert support then setup 0x1c mode page */
void get_supports_tapealert(dev_ent_t *un, int fd);

/* general tapealert request */
int tapealert(char *src_fn, int src_ln, int fd, dev_ent_t *un,
	uchar_t *logpage, int logpage_len);

/* unrecovered uscsi error */
int tapealert_skey(char *fn, int ln, int fd, dev_ent_t *un);

/* unrecovered mtio error */
int tapealert_mts(char *fn, int ln, int fd, dev_ent_t *un, short erreg);

/* macro for general tapealert request */
#define	TAPEALERT(_fd, _un)\
	(void) tapealert(_SrcFile, __LINE__, _fd, _un, NULL, 0)

/* macro for sef to tapealert request */
#define	TAPEALERT_PAGE(_fd, _un, _logpage, _pagelen)\
	(void) tapealert(_SrcFile, __LINE__, _fd, _un, _logpage, _pagelen)

/* macro for unrecovered uscsi error */
#define	TAPEALERT_SKEY(_fd, _un)\
	(void) tapealert_skey(_SrcFile, __LINE__, _fd, _un)

/* macro for unrecovered mtio error */
#define	TAPEALERT_MTS(_fd, _un, _erreg)\
	(void) tapealert_mts(_SrcFile, __LINE__, _fd, _un, _erreg)

#endif /* _TAPEALERT_H */
