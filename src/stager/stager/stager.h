/*
 * stager.h - Stager definitions.
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

#if !defined(STAGER_H)
#define	STAGER_H

#pragma ident "$Revision: 1.11 $"

/*
 * Stager start mode.
 */
enum StartMode {
	SM_cold,	/* Start after clean shutdown */
	SM_failover,	/* Start after failover */
	SM_restart	/* Start after abnormal termination */
};

/*
 * Define prototypes in control.c
 */
void ReconfigLock();
void ReconfigUnlock();

/*
 * Define prototypes in compose.c
 */
void AddCompose(int id);
void Compose();
int GetNumComposeEntries();

/*
 * Define prototypes in control.c
 */
char *Control(char *ident, char *value);

/*
 * Define prototypes in device.c
 */
int InitDevices();
dev_ent_t *GetDevices(boolean_t check);

/*
 * Define prototypes in error_retry.c
 */
boolean_t DamageArcopy(FileInfo_t *file);

/*
 * Define prototypes in filesys.c
 */
int InitFilesys();
void CreateRmFiles();
char *GetMountPointName(equ_t fseq);
void RemoveFileSystemMapFile();
void MountFileSystem(char *name);
void UmountFileSystem(char *name);
boolean_t IsFileSystemMounted(equ_t fseq);
int InitMessages();

/*
 * Define prototypes in log.c
 */
void OpenLogFile(char *name);
void CheckLogFile(char *name);
void LogIt(LogType_t type, FileInfo_t *file);
void LogStageStart(FileInfo_t *file);

/*
 * Define prototypes in readcmd.c
 */
void ReadCmds();
char *GetCfgLogFile();
int GetCfgLogEvents();
char *GetCfgTraceFile();
uint32_t GetCfgTraceMask();
boolean_t IsTraceEnabled(char *keys);
long GetCfgMaxActive();
long GetCfgMaxRetries();
char *GetCfgProbeDir();
size_t GetCfgProbeBufsize();
int GetCfgNumDrives();
int GetCfgDirectio();
sam_stager_drives_t *GetCfgDrives();

#endif /* STAGER_H */
