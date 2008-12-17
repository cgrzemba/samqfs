/*
 * archiver.h - archiver definitions.
 *
 * Public (and private) definitions for the archiver.
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

#ifndef _AML_ARCHIVER_H
#define	_AML_ARCHIVER_H

#pragma ident "$Revision: 1.49 $"

/* SAM-FS headers. */
#include "sam/defaults.h"
#include "sam/names.h"
#include "sam/param.h"
#include "sam/fs/arfind.h"

/* Macros. */

/* Names of directories and files. */
#define	ARCHIVER_CMD SAM_CONFIG_PATH"/archiver.cmd" /* Archiver command file */
#define	NOTIFY "archiver.sh"	/* Notify script file */

#define	AA_PROG "archiver"	/* Archive program */
#define	AC_PROG "sam-arcopy"	/* Archive copy program */
#define	AF_PROG "sam-arfind"	/* Archive find files program */
#define	ARCHIVER_DIR SAM_VARIABLE_PATH"/archiver" /* Archiver base directory */

/* Relative to ARCHIVER_DIR/ */
#define	ARCHIVE_SETS "ArchiveSets"	/* Archive sets file */
#define	ARCHIVER_STATE "state"		/* archiver state file */
#define	RESERVED_VSNS "ReservedVSNs"	/* Reserved VSNs file */

/* Relative to ARCHIVER_DIR/filesystem/ */
#define	ARCHREQ_DIR "ArchReq"	/* Archive requests directory */
#define	ARFIND_STATE "state"	/* arfind state file */
#define	EXAMLIST "Examlist"	/* Examine inode list file name */
#define	FILE_PROPS "FileProps"	/* arfind file properties files */
#define	SCANLIST "Scanlist"	/* Scanlist file name */

/* Relative to SAM-FS mount point */
#define	RM_FILES_DIR ".archive"	/* Removable media files directory */
#define	RM_FILES_NAME "rm"

/* Parameters. */
#define	ALARM_TIME (30)		/* Used for alarm() in sam-archiverd */
#define	SCAN_INTERVAL_MAX 100000000 /* Maximum scan interval */
#define	SPACE_FACTOR 50		/* Space factor for fitting on a VSN */
#define	SPACE_DISCOUNT 0.90	/* Space multiplier to account for IRG etc. */
#define	TAPE_SPACE_FACTOR (.001) /* Factor of capacity for fitting on a VSN */
#define	UMOUNT_DELAY (1 * 60)	/* Delay after umount before restarting */

/* Default values. */
#define	ARCH_AGE (4 * 60)		/* Archive age */
#define	BACKGROUND_SCAN_INTERVAL (24 * 60 * 60)	/* Background scan interval */
#define	BACKGROUND_SCAN_TIME (0)	/* Background scan time (hhmm) */
#define	CBARCHMAX (0)			/* Honeycomb archive file size */
#define	DIR_CACHE_SIZE (64 * 1024 * 1024) /* Directory cache size 64M */
#define	DKARCHMAX (1024 * 1024 * 1024)	/* Disk archive file size 1G */
#define	ODARCHMAX (1024 * 1024 * 1024)	/* Optical archive file size 1G */
#define	READ_TIMEOUT (1 * 60)	/* Timeout for reading file being archived */
#define	REQUEST_TIMEOUT (15 * 60)	/* Timeout for media request */
#define	SCAN_INTERVAL (4 * 60)		/* Scan interval */
#define	STAGE_TIMEOUT (0)
			/* Timeout for staging file being archived, 0=off */
#define	STARTAGE (120 * 60)
				/* Start age (2 hours) for examine=noscan */
#define	STARTCOUNT (500000)		/* Start count for examine=noscan */
#define	WRITE_TIMEOUT (15 * 60)
				/* Timeout for writing file being archived */

#define	TIARCHMAX ((fsize_t)22 * 1024 * 1024 * 1024)
					/* ti (STK Titanium) archmax = 22G */
#define	LIARCHMAX ((fsize_t)22 * 1024 * 1024 * 1024)
					/* li (LTO) archmax = 22G */
#define	SFARCHMAX ((fsize_t)11 * 1024 * 1024 * 1024)
					/* sf (STK 9940) archmax = 11G */
#define	M2ARCHMAX ((fsize_t)22 * 1024 * 1024 * 1024)
					/* m2 (IBM 3592) archmax = 22G */
#define	SGARCHMAX ((fsize_t)4 * 1024 * 1024 * 1024)
					/* sg (STK 9840) archmax = 4G */
#define	LTARCHMAX ((fsize_t)11 * 1024 * 1024 * 1024)
					/* lt (linear tape) archmax = 11G */
#define	TPARCHMAX ((fsize_t)8 * 1024 * 1024 * 1024)
					/* default tape archmax 8G */

/* Others. */
#define	OPRMSG_SIZE 80

/* Archiver daemon state file. */

#define	AD_MAGIC 0104232405
#define	AD_VERSION 60409	/* Archiver daemon state file version (YMMDD) */

/* Execution state. */
/* These are ordered for numerical testing. */
typedef enum { ES_none,
	ES_init,		/* Initialization in process */
	ES_cmd_errors,		/* Errors in archiver command file */
	ES_idle,		/* Idle archiving */
	ES_wait,		/* 'wait' directive, wait for operator start */
	ES_fs_wait,		/* File system 'wait' wait for operator start */
	ES_run,			/* Archive files */
	ES_umount,		/* Filesystem unmounting */
	ES_term,		/* Terminate archiving */
	ES_rerun,		/* Soft restart */
	ES_restart,		/* Restart archiver */
	ES_max
} ExecState_t;

struct ArchiverdState {
	MappedFile_t Ad;
	int	AdVersion;	/* File version */

	int	AdCount;	/* Arcopy count */
	ExecState_t AdExec;	/* Execution state */
	int	AdInterval;	/* Global interval */
	time_t	AdLastAlarm;	/* Time at which last SIGALRM was received */
	time_t	AdCmdfile;	/* Time at which archiver.cmd was processed */
	upath_t	AdNotifyFile;	/* Notify file name */
	char	AdOprMsg[OPRMSG_SIZE];	/* Operator message */
	char	AdOprMsg2[OPRMSG_SIZE];	/* Second operator message */
	pid_t	AdPid;			/* archiverd's pid - 0 if not active */

	upath_t	AdArchReq[1];	/* AdCount ArchReq-s assigned to sam-arcopy-s */
};


/* Arfind daemon state file. */

#define	AF_MAGIC 0106232405
#define	AF_VERSION 81215	/* Arfind daemon state file version (YMMDD) */

/* File system examination method. */
typedef enum { EM_none,
	EM_scan,		/* Traditional scan  */
	EM_scandirs,		/* Scan only directories */
	EM_scaninodes,		/* Scan only inodes */
	EM_noscan,		/* Continuous archive */
	EM_noscan_obsolete,	/* Obsolete 4.x continuous archive */
	EM_max
} ExamMethod_t;

#if defined(NEED_EXAM_METHOD_NAMES)
static char *ExamMethodNames[] = {
	"??", "scan", "scandirs", "scaninodes", "noscan", "noscan_obsolete"
};
#endif /* defined(NEED_EXAM_METHOD_NAMES) */

struct stats {		/* statistics */
	int numof;
	offset_t size;
};

/* Filesystem statistics */
struct FsStats {
	struct stats total;	/* All files */
	struct stats regular;	/* Regular files */
	struct stats offline;	/* Files offline */
	struct stats archdone;	/* Files with all archive action completed */
	struct stats copies[MAX_ARCHIVE];	/* Archived copies */
	struct stats dirs;			/* Directories */
};

/* sam-arfind state file. */
struct ArfindState {
	MappedFile_t Af;
	int	AfVersion;		/* File version */

	ExamMethod_t	AfExamine;	/* File system examination method */
	ExecState_t	AfExec;		/* Execution state */
	uint32_t	AfFlags;
	int 		AfBackGndInterval; /* Background scan interval */
	int		AfBackGndTime;	/* Background time of day */
	int		AfInterval;	/* Scan interval */
	int		AfDirCacheSize;	/* Maximum size of directory cache */
	upath_t		AfLogFile;	/* Archiver log file name */
	boolean_t	AfNormalExit;	/* arfind had a normal exit */
	char		AfOprMsg[OPRMSG_SIZE]; /* Operator message */
	pid_t		AfPid;		/* arfind's pid - 0 if not active */
	sam_time_t	AfExitTime;	/* Time arfind exited normally */
	sam_time_t	AfSbInit;	/* Time super block initialized */
	sam_time_t	AfStartTime;	/* Time arfind started execution */
	int		AfSeqNum;	/* ArchReq sequence number */

	struct FsStats AfStats;
	struct FsStats AfStatsScan;

	/* Counters. */
	int		AfFsactBufs;	/* FS activity - buffers allocated */
	int		AfFsactCalls;	/* FS activity - syscalls made */
	int		AfFsactEvents;	/* FS activity - events processed */
	int		AfEvents[AE_MAX]; /* FS activity - each event */

	int		AfIdstat;	/* idstat calls */
	int		AfOpendir;	/* opendir calls */
	int		AfGetdents;	/* getdents calls */

	/* Id2path counters. */
	int		AfId2path;			/* number of calls */
	int		AfId2pathCached;	/* cache hits */
	int		AfId2pathIdstat;	/* idstat calls */
	int		AfId2pathReaddir;	/* directory reads */

	int		AfScanlist[2];	/* Scanlist entries - count, active */

	int		AfFilesCreate;	/* Files in create ArchReq */
	int		AfFilesSchedule; /* Files in schedule ArchReq */
	int		AfFilesArchive;	/* Files in archive ArchReq */
};

/* Definitions for flags */
#define	ASF_archivemeta	0x0001	/* Archive meta data */
#define	ASF_scanlist	0x0002	/* Perform scanlist consolidation */
#define	ASF_setarchdone	0x0004	/* Set archdone state while scanning files */


/* Public functions. */
/* The return is an error message.  An empty messsage means no error. */
#define	ArchiverControl(a, b, c, d)	_ArchiverControl(_SrcFile, __LINE__, \
	(a), (b), (c), (d))
char *_ArchiverControl(const char *SrcFile, const int SrcLine,
	char *ident, char *value, char *msg, int msgSize);
#define	ArfindControl(a, b, c, d, e) _ArfindControl(_SrcFile, __LINE__, \
	(a), (b), (c), (d), (e))
char *_ArfindControl(const char *SrcFile, const int SrcLine, char *fsname,
	char *ident, char *value, char *msg, int msgSize);

/* The return is -1 if an error occurred. */
#define	ArchiverCatalogChange() _ArchiverCatalogChange(_SrcFile, __LINE__)
int _ArchiverCatalogChange(const char *SrcFile, const int SrcLine);
#define	ArchiverDequeueArchReq(a, b) _ArchiverDequeueArchReq(_SrcFile, \
	__LINE__, (a), (b))
int _ArchiverDequeueArchReq(const char *SrcFile, const int SrcLine,
	char *fsname, char *arname);
#define	ArchiverFsMount(a) _ArchiverFsMount(_SrcFile, __LINE__, (a))
int _ArchiverFsMount(const char *SrcFile, const int SrcLine, char *fsname);
#define	ArchiverFsUmount(a) _ArchiverFsUmount(_SrcFile, __LINE__, (a))
int _ArchiverFsUmount(const char *SrcFile, const int SrcLine, char *fsname);
#define	ArchiverRmState(a) _ArchiverRmState(_SrcFile, __LINE__, (a))
int _ArchiverRmState(const char *SrcFile, const int SrcLine, int state);
#define	ArchiverVolStatus(a, b) _ArchiverVolStatus(_SrcFile, __LINE__, \
		(a), (b))
int _ArchiverVolStatus(const char *SrcFile, const int SrcLine, char *mtype,
	char *vsn);

struct ArfindState *ArfindAttach(char *fsname, int mode);

#if defined(ARCHIVER_PRIVATE)

/* Archiver daemon private functions. */
#define	ArchiverQueueArchReq(a, b) _ArchiverQueueArchReq(_SrcFile, __LINE__, \
	(a), (b))
int _ArchiverQueueArchReq(const char *SrcFile, const int SrcLine,
	char *fsname, char *arname);
#define	ArchiverRequestVolume(a, b) _ArchiverRequestVolume(_SrcFile, __LINE__, \
	(a), (b))
int _ArchiverRequestVolume(const char *SrcFile, const int SrcLine,
	char *ariname, fsize_t fileSize);
#define	ArfindArchreqDone(a, b) _ArfindArchreqDone(_SrcFile, __LINE__, (a), (b))
int _ArfindArchreqDone(const char *SrcFile, const int SrcLine, char *fsname,
	char *arname);
#define	ArfindChangeState(a, b) _ArfindChangeState(_SrcFile, __LINE__, (a), (b))
int _ArfindChangeState(const char *SrcFile, const int SrcLine, char *fsname,
	int state);


#define	SERVER_NAME "Archiver"
#define	SERVER_MAGIC 001220310

#define	CTMSG_SIZE 128

enum ArchSrvrReq {
	ASR_control = 0,	/* Control archiver/arfind (Must be first) */
	ASR_catalog_change,	/* Catalog change */
	ASR_dequeueArchreq,	/* Dequeue archreq */
	ASR_fs_mount,		/* Filesystem mounted */
	ASR_fs_umount,		/* Filesystem unmount requested */
	ASR_queueArchreq,	/* Queue archreq */
	ASR_request_volume,	/* Request a volume for an archreq */
	ASR_rm_state,		/* Removable media stop */
	ASR_vol_status,		/* Request volume status */
	ASR_MAX
};


/* Arguments for requests. */

struct AsrControl {
	char	CtIdent[64];		/* Identifier of control variable */
	char	CtValue[64];		/* Value to set */
};

struct AsrControlRsp {
	char	CtMsg[CTMSG_SIZE];	/* Error message. */
					/* Empty string if no error */
};

struct AsrFsname {
	uname_t	ArFsname;		/* Filesystem name */
};

struct AsrArchReq {
	uname_t	ArFsname;		/* Filesystem name */
	upath_t	ArArname;		/* ArchReq */
};

struct AsrRmState {
	int		state;		/* 1 = on, 0 = off */
};

struct AsrRequestVolume {
	upath_t	AsAriname;		/* ArchReq instance */
	fsize_t	AsFileSize;		/* Size of file being archived */
};

struct AsrVolStatus {
	mtype_t	mtype;			/* Media to check */
	vsn_t	vsn;			/* VSN to check */
};

/* General response. */
struct AsrGeneralRsp {
	int	GrStatus;
	int	GrErrno;
};

#endif /* defined ARCHIVER_PRIVATE */

#if defined(ARFIND_PRIVATE)

#define	AFSERVER_NAME "Arfind"
#define	AFSERVER_MAGIC 001220310

enum ArfindSrvrReq {
	AFR_archreqDone = 1,		/* Archreq finished */
	AFR_changeState,		/* Change state */
	AFR_MAX
};

/* Arguments for requests. */

struct AfrPath {
	char	AfPath[MAXPATHLEN];	/* Path name */
};

struct AfrState {
	int	AfState;	/* New state */
};

/* General response. */
struct AfrGeneralRsp {
	int	GrStatus;
	int	GrErrno;
};

#endif /* defined ARFIND_PRIVATE */

#endif /* _AML_ARCHIVER_H */
