/*
 * stream.h - stage stream defInitions
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
 * or https://illumos.org/license/CDDL.
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

#ifndef _STREAM_H
#define	_STREAM_H

#pragma ident "$Revision: 1.26 $"

#ifdef __cplusplus
extern "C" {
#endif

#define	EOS		-1			/* End of stream */

#define	IS_STREAM_EMPTY(x)	(x->first == EOS)

/*
 *	Argument to AddStream function.
 */
#define	ADD_STREAM_NOSORT	0
#define	ADD_STREAM_SORT		1

/* Functions */
StreamInfo_t *CreateStream(FileInfo_t *file);
void DeleteStream(StreamInfo_t *stream);
boolean_t AddStream(StreamInfo_t *stream, int fid, int sort);
void ErrorStream(StreamInfo_t *stream, int error);
boolean_t CancelStream(StreamInfo_t *stream, int id);
void ClearStream(StreamInfo_t *stream);

#ifdef __cplusplus
}
#endif

#endif /* _STREAM_H */
