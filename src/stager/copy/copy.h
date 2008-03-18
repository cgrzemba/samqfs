/*
 * copy.h -
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

#if !defined(COPY_H)
#define	COPY_H

#pragma ident "$Revision: 1.24 $"

#include "aml/sam_rft.h"

#define	SetErrno (*(___errno()))

/*
 * Define prototypes in copy.c
 */
CopyInstance_t *FindCopyProc(int lib, media_t media);

/*
 * Define prototypes in copyfile.c
 */
void CopyFiles();
void InvalidateBuffers();
int SetPosition(int from_pos, int to_pos);

/*
 * Define prototypes in stage.c
 */
void InitStage(char *new_vsn);
void EndStage();
int LoadVolume();
int NextArchiveFile();
void EndArchiveFile();
int GetBlockSize();
u_longlong_t GetPosition();
int SeekVolume(int to_pos);
void UnloadVolume();
offset_t GetBufferSize(int block_size);
equ_t GetDriveNumber();

/*
 * Define prototypes in rmstage.c
 */
void InitRmStage();
int LoadRmVolume();
int GetRmBlockSize();
u_longlong_t GetRmPosition();
int SeekRmVolume(int to_pos);
void UnloadRmVolume();
offset_t GetRmBufferSize(int block_size);
equ_t GetRmDriveNumber();

/*
 * Define prototypes in dkstage.c
 */
void InitDkStage();
int LoadDkVolume();
int NextDkArchiveFile();
void EndDkArchiveFile();
int SeekDkVolume(int to_pos);
u_longlong_t GetDkPosition();
void UnloadDkVolume();

/*
 * Define prototypes in hcstage.c
 */
void HcStageInit();
int HcStageLoadVolume();
int HcStageNextArchiveFile();
void HcStageEndArchiveFile();
int HcStageReadArchiveFile(void *rft, void *buf, size_t nbytes);
int HcStageSeekVolume(int to_pos);
u_longlong_t HcStageGetPosition();
void HcStageUnloadVolume();

/*
 * Define prototypes in checksum.c
 */
void InitChecksum(FileInfo_t *file, boolean_t init);
void SetChecksumDone();
void Checksum(char *data, int num_bytes);
void WaitForChecksum();
int ChecksumCompare(int fd, sam_id_t *id);
csum_t GetChecksumVal();
void SetChecksumVal(csum_t val);

#endif
