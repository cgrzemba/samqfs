/*
 * recycler_lib.h
 *
 * Declaration of functions which serve for work with disk archive
 * sequence numbers that are in use by currently running arcopy
 * processes.
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


#ifndef	_SEQNUMS_IN_USE_H
#define	_SEQNUMS_IN_USE_H

#pragma	ident	"$Revision$"

/*
 * Representation of sequence numbers in use by currently running arcopies.
 */
typedef struct SeqNumsInUse {
	int count; /* Sequence numbers array count */
	DiskVolumeSeqnum_t *seqnums; /* Array of sequence numbers */
} SeqNumsInUse_t;

SeqNumsInUse_t *GetSeqNumsInUse(char *vsn, char *fsname, SeqNumsInUse_t *inuse);

boolean_t IsSeqNumInUse(DiskVolumeSeqnum_t seqnum,
					    SeqNumsInUse_t *seqNumsInUse);

#endif /* _SEQNUMS_IN_USE_H */
