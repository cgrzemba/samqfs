/*
 * stager.h - Stager definitions
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

#ifndef _AML_STAGER_H
#define	_AML_STAGER_H

#include "sam/param.h"
#include "sam/types.h"

/*
 * Stager's uds configuration.
 */
#define	STAGER_SERVER_NAME	"Stager"
#define	STAGER_SERVER_MAGIC	05541536

enum StagerSrvrReq {
	SSR_fs_mount,			/* filesystem mount */
	SSR_fs_umount,			/* filesystem umount */
	SSR_stop_rm,			/* stop removable media activity */
	SSR_log_event,			/* staging event for log file */
	SSR_control,			/* control staging activity */
	SSR_MAX
};

struct StagerFsname {
	uname_t	fsname;
};

struct StagerLogEvent {
	int type;			/* event type */
	int id;				/* file identifier */
};

struct StagerControl {
	char ident[64];			/* control variable identifier */
	char value[64];			/* value to set */
};

struct StagerGeneralRsp {
	int status;
	int error;
};

struct StagerControlRsp {
	char msg[128];			/* error message */
};


/*
 * Stager's directory in SAM_VARIABLE_PATH.
 */
#define	STAGER_DIRNAME	"stager"

/*
 * Stager's state information.  Name of memory mapped file
 * containing current staging state information.  File is located
 * in directory SAM_VARIABLE_PATH.  State data is defined by
 * struct StagerStateInfo.
 */
#define	STAGER_STATE_FILENAME	"state"

/*
 * Default number of active stages per GB of memory.
 */
#define	STAGER_MAX_ACTIVE_PER_GB	5000

/*
 * Currently only used by libfsmgmt to display default value.
 * TODO: Remove once libfsmgmt uses STAGER_MAX_ACTIVE_PER_GB
 * instead (CR6707401).
 */
#define	STAGER_DEFAULT_MAX_ACTIVE 	5000

/*
 * Maximum number of active stage requests
 */
#define	STAGER_MAX_ACTIVE		500000

/*
 * Default value for maximum number of retries.
 */
#define	STAGER_DEFAULT_MAX_RETRIES	3

/*
 * Default is directio rather than paged io
 */
#define	STAGER_DEFAULT_DIRECTIO		1

/*
 * Default is 32MB for minimum directio stage size
 */
#define	STAGER_DEFAULT_DIRECTIO_MIN	32

/*
 * Maximum path length.
 */
#define	STAGER_MAX_PATHLEN	sizeof (upath_t)

/*
 * Default trace mask.
 */
#define	STAGER_DEFAULT_TRACE_MASK	TR_err | TR_cust

/*
 * Default media characteristics buffer size.
 */
#define	STAGER_DEFAULT_MC_BUFSIZE	16

/*
 * Log file enumerations and default logging value.
 */
enum {
	STAGER_LOGFILE_START	=	1 << 0,
	STAGER_LOGFILE_FINISH	=	1 << 1,
	STAGER_LOGFILE_CANCEL	=	1 << 2,
	STAGER_LOGFILE_ERROR	=	1 << 3
};

#define	STAGER_LOGFILE_DEFAULT	STAGER_LOGFILE_FINISH | \
	STAGER_LOGFILE_CANCEL | \
	STAGER_LOGFILE_ERROR


/*
 * Stager's bufsize configuration options.
 */
typedef struct sam_stager_bufsize {
	media_t		media;			/* media type */
	int		size;			/* buffer size */
	boolean_t	lockbuf;		/* lock buffer */
} sam_stager_bufsize_t;

/*
 * Stager's removable drives configuration options.
 */
typedef struct sam_stager_drives {
	uname_t		robot;			/* robot name */
	int		count;			/* number of drives to use */
} sam_stager_drives_t;

/*
 * Stager's stream parameters configuration options.
 */
typedef struct sam_stager_streams {
	media_t		ss_media;	/* media type */
	ushort_t	ss_flags;
	int		ss_drives;	/* number of streams to use */
	fsize_t		ss_maxSize;	/* max size of stream to create */
	int		ss_maxCount;	/* max number of files in stream */
} sam_stager_streams_t;

/*
 * Stager's log file configuration options.
 */
typedef struct sam_stager_logfile {
	upath_t		name;			/* path to log file */
	int		events;
} sam_stager_logfile_t;

/*
 * Stager configuration.
 */
typedef struct sam_stager_config {
	int			maxactive;	/* max active stages */
	int			maxretries;	/* max number of retries */
	sam_stager_logfile_t	logfile;	/* log file config options */
	sam_stager_bufsize_t	bufsize;	/* bufsize config options */
	int			num_drives;	/* number of drive configs */
	sam_stager_drives_t	*drives;	/* drives config options */
	int			directio;	/* directio = 1 pageio = 0 */
	int			dio_min_size;	/* min directio stage size */
	int			num_streams;	/* number of stream params */
	sam_stager_streams_t	*streams;	/* stream parameters */
} sam_stager_config_t;

#define	STAGER_DISPLAY_ACTIVE	20	/* number of active stages to display */
#define	STAGER_DISPLAY_STREAMS	10	/* number of streams to display */
#define	STAGER_OPRMSG_SIZE	80	/* size of operator message buffer */

/*
 * Staging state flags.
 */
typedef enum StagerStateFlags {
	STAGER_STATE_COPY 	= 1,	/* copying file */
	STAGER_STATE_LOADING,		/* loading vsn */
	STAGER_STATE_DONE,		/* unloading vsn */
	STAGER_STATE_POSITIONING,	/* positioning removable media */
	STAGER_STATE_WAIT,		/* waiting for media to be loaded or */
					/* imported */
	STAGER_STATE_NORESOURCES	/* no resources available */
} StagerStateFlags_t;

/*
 * Details of stage in progress.
 */
typedef struct StagerStateDetail {
	pid_t		pid;		/* copy proc's pid */
	sam_id_t	id;		/* file identification */
	equ_t		fseq;		/* family set equipment number */
	int		copy;		/* staging from this archive copy */
	u_longlong_t	position;	/* position of archived file */
	ulong_t		offset;		/* offset from beginning of ar file */
	u_longlong_t	len;		/* length of staging request */
	upath_t		name;		/* file name */
} StagerStateDetail_t;

typedef struct StagerStateInfo {
	pid_t		pid;		/* sam-stagerd's pid */
	long		reqEntries;	/* number of entries in use */
	long		reqAlloc;	/* size of allocated space */
	upath_t		logFile;	/* log file name */
	char		errmsg[STAGER_OPRMSG_SIZE];
	struct {
		StagerStateFlags_t 	flags;
		mtype_t			media;
		int			eq;
		vsn_t			vsn;
		char			oprmsg[STAGER_OPRMSG_SIZE];
		StagerStateDetail_t	detail;
	} active[STAGER_DISPLAY_ACTIVE];
	upath_t		streamsDir;	/* streams directory path */
	upath_t		stageReqsFile;	/* stage requests path */
	struct {
		boolean_t	active;
		media_t		media;
		int		seqnum;
		vsn_t		vsn;
		char		oprmsg[STAGER_OPRMSG_SIZE];
	} streams[STAGER_DISPLAY_STREAMS];
} StagerStateInfo_t;

#define	StagerFsMount(a) _StagerFsMount(_SrcFile, __LINE__, (a))
int _StagerFsMount(const char *SrcFile, const int SrcLine, char *fsname);

#define	StagerFsUmount(a) _StagerFsUmount(_SrcFile, __LINE__, (a))
int _StagerFsUmount(const char *SrcFile, const int SrcLine, char *fsname);

#define	StagerStopRm(a) _StagerStopRm(_SrcFile, __LINE__)
int _StagerStopRm(const char *SrcFile, const int SrcLine);

#define	StagerLogEvent(a, b) _StagerLogEvent(_SrcFile, __LINE__, (a), (b))
int _StagerLogEvent(const char *SrcFile, const int SrcLine, int type, int id);

#define	StagerControl(a, b, c, d) _StagerControl(_SrcFile, __LINE__, \
	(a), (b), (c), (d))
char *_StagerControl(const char *SrcFile, const int SrcLine, char *ident,
	char *value, char *msg, int msgsize);

int StagerStreamGetFiles(void *stream, void *stageReqs, void **files);

#endif /* _AML_STAGER_H */
