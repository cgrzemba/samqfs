/*
 * copy.h - stager copy process definitions
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

#if !defined(COPY_H)
#define	COPY_H

#pragma ident "$Revision: 1.26 $"

#include "aml/sam_rft.h"

#define	SetErrno (*(___errno()))

/*
 * Used for number of entries in the copy instance list.
 * Only the data section of the copy instance list is memory mapped.
 */
#define	CopyInstanceListCount SharedInfo->si_numCopyInstanceInfos

/*
 * Define prototypes in archive_read.c
 */
void *ArchiveRead(void *arg);
int SetPosition(int from_pos, int to_pos);

/*
 * Define prototypes in copy.c
 */
CopyInstanceInfo_t *FindCopyInstanceInfo(int lib, media_t media);

/*
 * Define prototypes in copyfile.c
 */
void CopyFiles();
void ResetBuffers();
void SetFileError(FileInfo_t *file, int fd, offset_t write_off,
	int error);

/*
 * Define prototypes in disk_cache.c
 */
int DiskCacheOpen(FileInfo_t *file);
int DiskCacheWrite(FileInfo_t *file, int fd);

/*
 * Define prototypes in double_buffer.c
 */
void *DoubleBuffer(void *arg);

/*
 * Define prototypes in filesys.c
 */
void MapInFileSystem();
char *GetMountPointName(equ_t fseq);
char *GetFsName(equ_t fseq);

/*
 * Define prototypes in stage.c
 */
void StageInit(char *new_vsn);
void StageEnd();
int LoadVolume();
int NextArchiveFile();
void EndArchiveFile();
int GetBlockSize();
u_longlong_t GetPosition();
int SeekVolume(int to_pos);
void UnloadVolume();
equ_t GetDriveNumber();

/*
 * Define prototypes in rmstage.c
 */
void RmInit();
int RmLoadVolume();
int RmGetBlockSize();
u_longlong_t RmGetPosition();
int RmSeekVolume(int to_pos);
void RmUnloadVolume();
offset_t RmGetBufferSize(int block_size);
equ_t RmGetDriveNumber();

/*
 * Define prototypes in dkstage.c
 */
void DkInit();
int DkLoadVolume();
int DkNextArchiveFile();
void DkEndArchiveFile();
int DkSeekVolume(int to_pos);
u_longlong_t DkGetPosition();
void DkUnloadVolume();

/*
 * Define prototypes in hcstage.c
 */
void HcInit();
int HcLoadVolume();
int HcNextArchiveFile();
void HcEndArchiveFile();
int HcReadArchiveFile(void *rft, void *buf, size_t nbytes);
int HcSeekVolume(int to_pos);
u_longlong_t HcGetPosition();
void HcUnloadVolume();

/*
 * Define prototypes in checksum.c
 */
void ChecksumInit(FileInfo_t *file, boolean_t init);
void Checksum(char *data, int num_bytes);
void ChecksumWait();
int ChecksumCompare(int fd, sam_id_t *id);
csum_t ChecksumGetVal();
void ChecksumSetVal(csum_t val);

/*
 * Define prototypes in error_retry.c
 */
boolean_t IfRetry(FileInfo_t *file, int errorNum);
int RetryOptical(char *buf, longlong_t total_read, int file_position);
void SendErrorResponse(FileInfo_t *file);
void ErrorFile(FileInfo_t *file);

/*
 * Define prototypes in log.c
 */
void LogIt(LogType_t type, FileInfo_t *file);

/*
 * Define prototypes in stage_reqs.c
 */
FileInfo_t *GetFile(int id);
void SetStageDone(FileInfo_t *file);
void MapInRequestList();
void UnMapRequestList();

#endif
