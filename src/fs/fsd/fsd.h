/*
 * fsd.h - Filesystem daemon common definitions.
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

#ifndef FSD_H
#define	FSD_H

#pragma ident "$Revision: 1.61 $"

/* SAM-FS includes. */
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/sam_trace.h"

/* External functions */
int sam_syscall(int cmd, void *arg, int size);	/* lib/samconf/sam_syscall.c */

/* Macros. */
#define	DIR_MODE	0775	/* Create mode for directories */

#define	SCHEDQUANT	120	/* 2m default between scheduling wakeups */

/* Defines. */
#define	CP_respawn	0x01	/* Respawn process on exit */
#define	CP_norestart	0x02	/* proc requested no restart */
#define	CP_qstart	0x04	/* quick start, wait 10s from last start  */
#define	CP_nosignal	0x08	/* proc requested no signal */
#define	CP_adopted	0x10	/* Process found executing */
#define	CP_stopping	0x20	/* Process requested stop */

#ifdef linux
#define	GETFSIND			1
#define	GETFSTYP			2
#define	GETNFSTYP			3
#define	sysfs(i, fstype)		syscall(135, i, fstype)
#define	thr_sigsetmask(mask, ssp, p)	pthread_sigmask(mask, ssp, p)
#define	thr_exit(p)			pthread_exit(p)
#define	fork1()				fork()
#define	fopen64(name, flags)		fopen(name, flags)
#define	MIN_REL_MAJOR			2
#define	MIN_REL_MINOR			4
#ifdef x86_64
#define	MIN_REL_UPDATE			19
#else
#define	MIN_REL_UPDATE			18
#endif /* x86_64 */
#ifndef QFS
#define	QFS
#endif /* !defined(QFS) */
#ifdef	ASSERT
#undef	ASSERT
#ifdef DEBUG
#define	ASSERT(x)	if (!(x)) {		\
				abort();	\
			}
#else
#define	ASSERT(x)
#endif /* DEBUG */
#endif /* ASSERT */
#endif /* linux */


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

DCL char *McfName IVAL(SAM_CONFIG_PATH "/" CONFIG);

DCL boolean_t Daemon IVAL(FALSE);
DCL boolean_t FsCfgOnly IVAL(FALSE);
DCL boolean_t QfsOnly IVAL(FALSE);
DCL boolean_t Verbose IVAL(FALSE);

DCL int	ArchiveCount IVAL(0);
DCL dev_ent_t *DeviceTable IVAL(NULL);
DCL int DeviceNumof IVAL(0);
DCL int DiskVolCount IVAL(0);		/* Number of disk volumes */
DCL int MaxDevEq IVAL(0);
DCL int RmediaDeviceCount IVAL(0);	/* Number of removable media devices */
DCL int DiskVolClientCount IVAL(0);	/* Num of trusted disk arch clients */
DCL int SamRemoteServerCount IVAL(0);	/* Number of sam-remote server */

DCL struct sam_mount_info *FileSysTable IVAL(NULL);
DCL int FileSysNumof IVAL(0);


/* Public functions. */
void ConfigFileMsg(char *msg, int lineno, char *line);
void FatalError(int msgNum, ...);
#ifdef sun
void FsClient(uname_t fs_name);
#endif /* sun */
void FsConfig(char *fscfg_name);
void MakeTraceCtl(void);
void ProcessSignals(int block);
void ReadDefaults(char *defaults_name);
#ifdef sun
void ReadDiskVolumes(char *diskvols_name);
void ReadFsCmdFile(char *fscfg_name);
#endif /* sun */
void ReadMcf(char *mcf_name);
void WriteMcfbin(int DeviceNumof, dev_ent_t *DeviceTable);
void SenseRestart(void);
void ServerInit(void);
void StartProcess(int argc, char *argv[], int flags, int tid);
void StopProcess(char *argv[], boolean_t erase, int sig);
void WriteDefaults(void);
void StartShareDaemon(char *fs);
#ifdef sun
void WriteDiskVolumes(boolean_t reconfig);
#endif /* sun */
void WriteTraceCtl(void);

#endif /* FSD_H */
