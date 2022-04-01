/*
 *	sam-fsalogd - File system activity event logging daemon.
 *
 *	sam-fsalogd is initiated by SAMFS at mounting time.  It uses
 *	the Solaris Door's API to read file system activity events from
 *	the file system.  These events are writen to log files residing
 *	in /var/opt/SUNWsamfs/fsalog.
 *
 *	Error messages are writen to the samlog and to the trace.
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

#pragma ident "$Revision: 1.3 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

			/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <inttypes.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <time.h>
#include <signal.h>
#include <strings.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>

/* Solaris includes. */
#include <door.h>

/* SAM-FS includes. */
#define DEC_INIT
#include "sam/types.h"
#include "sam/param.h"
#include "sam/defaults.h"
#include "sam/custmsg.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/signals.h"
#include "sam/syscall.h"
#include "sam/uioctl.h"
#include "sam/samevent.h"

/* Logs are rolled over every 8 hours */
#define	FSA_ROLLOVER_INTERVAL	28800
/* Logs expire after 48 hours */
#define	FSA_EXPIRE_INTERVAL	172800
#define	FSA_SYSLOG_NAME		"sam-fsalogd"
#define	FSA_CMD_FILE		SAM_CONFIG_PATH"/fsalogd.cmd"
#define	FSA_LOG_PATH		SAM_VARIABLE_PATH"/fsalogd/"

#define	EVENT_SYSCALL_RETRY	5
#define	EVENT_INTERVAL		10

#define	ALARM_TIME		120

/* Private data. */
static	struct sam_event_buffer	*eb = NULL;

/* Utlity functions.		*/
extern	int	read_cmd_file(char *FileName);

/* Private functions.		 */
static	void	Event_Processor(char *fs_name);
static	void	init_event(char *fs_name);
static	void	read_events(void *cookie, char *argp, size_t arg_size,
			door_desc_t *dp, uint_t n_desc);
static	void	fsalogdTraceInit(void);
static	void	fsalogdInit(void);
static	int	Open_FSA_log_file(char *);
static	int	Write_FSA_log_file(char *, sam_event_t *, int);
static	void	check_log_expire(void);
static	void	print_event(FILE *, sam_event_t *);

int	EventNotifies = 0;	/* Number of  notifies			*/
int	EventBufFull  = 0;	/* Number of event buffer fulls		*/
int	EventCount  = 0;	/* Number of event received		*/

typedef	struct	thd	{
	pthread_cond_t	thd_cv;
	pthread_mutex_t	thd_mutex;
	int		thd_waiting;
} thr_data_t;

int		Verbose		= 0;	/* Verbose flag			*/
int		Daemon		= 0;	/* Running as sam-fsd child	*/
char		*Logfile_Path	= NULL;	/* Logfile path			*/
char		*fs_name;		/* File system name		*/
int		Log_Rollover;		/* Log rollover interval	*/
int		Log_Expire;		/* Log modification expiration 	*/
int		Event_Interval;		/* Event interval time		*/
int		Event_Buffer_Size;	/* Event buffer size		*/
int		Event_Open_Retry;	/* Open syscall retry count	*/

static	thr_data_t	main_thr;
static	pthread_mutex_t	door_mutex;	/* Locked when door open'ed	*/
	pthread_t	signal_thread;	/* Signal watcher thread	*/

static	int	caught_ALRM	= 0;	/* number of SIGALRMs pending	*/
static	int	caught_HUP	= 0;	/* number of SIGHUGs pending	*/
static	int	caught_TERM	= 0;	/* number of SIGTERMs pending	*/

static	int	running		= 1;	/* Daemon running flag		*/
static	int	abort_daemon	= 0;	/* Abort daemon flag		*/
static	int	log_rollover	= 0;	/* Log file rollover flag	*/
static	int	check_expire	= 0;	/* Flag to check log expiration */
static	time_t	logtime;		/* Log open time		*/
static	int	fsa;			/* FSAlog file descriptor	*/

typedef	struct	{			/* FSA event name table entry	*/
	char	*label;			/* Event label string		*/
	short	pino;			/* Parent inode used flag	*/
	short	p;			/* Parameter field used flag	*/
}	fsa_event_t;

static	fsa_event_t	fsa_event_names[] = {
			{"none",	0, 0},
			{"create",	1, 0},
			{"change",	1, 0},
			{"close",	1, 0},
			{"rename",	1, 1},
			{"remove",	1, 0},
			{"offline",	1, 0},
			{"online",	1, 0},
			{"archive",	0, 1},
			{"modify",	1, 0},
			{"archange",	0, 1},
			{"umount",	0, 1},
			{"unknown",	1, 1}
};

/* Signals control */

static void	sigCatcher(int sig);	/* Signal processor		*/
static void	sigFatal(int sig);	/* Fatal signal processor	*/

static struct SignalFunc	signalTable[] = {
			{ SIGALRM, sigCatcher },
			{ SIGHUP, sigCatcher },
			{ SIGINT, sigCatcher },
			{ SIGPIPE, NULL },
			{ SIGTERM, sigCatcher },
			{ 0 }
};

static struct SignalsArg signalsArg = {
			signalTable,	/* Signal function table	*/
			sigFatal,	/* Fatal signal processor	*/
			FSA_LOG_PATH	/* directory for dump/trace	*/
};

int	main(
	int		argc,		/* Argument count		*/
	char		**argv)		/* Argument vector		*/
{
	struct	sam_fs_info mnt_info;	/* File system mount table	*/

	Event_Interval = EVENT_INTERVAL;
	Event_Open_Retry = EVENT_SYSCALL_RETRY;
	Log_Rollover = FSA_ROLLOVER_INTERVAL;
	Log_Expire = FSA_EXPIRE_INTERVAL;

	Daemon = strcmp(GetParentName(), SAM_FSD) == 0;

	/*
	 * Argument processing only matters when started from command line.
	 */

	if (argc < 2) {
		fprintf(stderr,
		    "%s: usage: %s file_system\n", argv[0], argv[0]);
		exit(2);
	}

	while (--argc > 0) {
		if (**++argv == '-') {

			if (strcmp(*argv, "-v") == 0) {
				Verbose = 1;
				continue;
			}

			error(0, 0, "Unrecognized argument: %s\n", *argv);
		}
		break;
	}

	if (!Verbose) {			/* If running as daemon. */
		Daemon = TRUE;
	}

	fsalogdTraceInit();		/* Start trace facility	*/

	if (argc == 0) {
		sam_syslog(LOG_ERR, "filesystem not specified");
		Trace(TR_FATAL, "filesystem not specified");
		exit(EXIT_FAILURE);
	}

	fs_name = *argv;

	/*
	 * Check if name is a mount point or family set name, and mounted.
	 */

	if ((GetFsInfo(fs_name, &mnt_info)) == -1) {
		sam_syslog(LOG_ERR, "%s: filesystem not found", fs_name);
		Trace(TR_FATAL, "Filesystem %s not found", fs_name);
		exit(EXIT_FAILURE);
	}

	if ((mnt_info.fi_status & FS_MOUNTED) == 0) {
		sam_syslog(LOG_ERR, "%s: filesystem not mounted", fs_name);
		Trace(TR_FATAL, "Filesystem %s not mounted", fs_name);
		exit(EXIT_FAILURE);
	}

	fs_name = mnt_info.fi_name;

	if (read_cmd_file(FSA_CMD_FILE) > 0) {
		sam_syslog(LOG_ERR, "%s: error in fsalogd command file",
		    fs_name);
		Trace(TR_FATAL, "Filesystem %s error in fsalogd command file",
		    fs_name);
		exit(EXIT_FAILURE);
	}

	Trace(TR_MISC, "Parameters: log_path %s",
	    Logfile_Path != NULL ? Logfile_Path : "default");
	Trace(TR_MISC, "Parameters: rollover %d, event interval %d,"
	    " event size %d, open retry %d", Log_Rollover,
	    Event_Interval, Event_Buffer_Size, Event_Open_Retry);

	Trace(TR_MISC, "Logging started %s", fs_name);

	fsalogdInit();			/* Complete initialization	*/

	if (Logfile_Path == NULL) {
		SamMalloc(Logfile_Path, strlen(FSA_LOG_PATH) +
		    strlen(fs_name) + 2);
		strcpy(Logfile_Path, FSA_LOG_PATH);
		strcat(Logfile_Path, mnt_info.fi_name);
		strcat(Logfile_Path, "/");
	}

	strcat(signalsArg.SaCoreDir, "/");
	strcat(signalsArg.SaCoreDir, mnt_info.fi_name);

	fsa = Open_FSA_log_file(mnt_info.fi_name);

	if (fsa < 0) {
		exit(EXIT_FAILURE);
	}

	check_log_expire();
	Event_Processor(mnt_info.fi_name);
	exit(abort_daemon ? EXIT_FAILURE : EXIT_SUCCESS);
	/* return (0); */
}


/*	Event_Processor - Process file system activity events.		 */

static void
Event_Processor(
	char	*fs_name)

{

	/* thread 0 sync */

	(void) pthread_mutex_init(&(main_thr.thd_mutex), NULL);
	(void) pthread_cond_init(&(main_thr.thd_cv), NULL);
	(void) pthread_mutex_init(&(door_mutex), NULL);

	/* Set up a separate thread for signal handling.  */

	signal_thread = Signals(&signalsArg);

	/*
	 * Set up door and notify SAM.
	 */
	init_event(fs_name);

	/*
	 * Here is the main body of the door daemon.  running == 0 means that
	 * after flushing out the queue, it is time to exit in response to
	 * SIGTERM.
	 */

	while (running) {

		/*
		 * thread_signal() signals main (this thread) when it has
		 * received a signal.
		 */
		(void) pthread_mutex_lock(&(main_thr.thd_mutex));

		if (!(caught_HUP || caught_TERM || caught_ALRM)) {
			alarm(ALARM_TIME);
			(void) pthread_cond_wait(&(main_thr.thd_cv),
			    &(main_thr.thd_mutex));
		}

		(void) pthread_mutex_unlock(&(main_thr.thd_mutex));

		/*
		 * Got here because a signal came in.
		 * Since we may have gotten more than one, we assume a
		 * priority scheme.
		 */

		if (abort_daemon || caught_TERM) {
			if (caught_TERM) {
				Trace(TR_MISC, "SIGTERM recieved");
			}
			caught_ALRM = 0;
			caught_HUP  = 0;
			caught_TERM = 0;

			/* wait for door to close */
			(void) pthread_mutex_lock(&(door_mutex));
			eb->eb_dstatus = FSA_STATUS_EXIT;
			running = 0;		/* shutdown */
			continue;
		}

		if (caught_ALRM) {
			caught_ALRM = 0;
		}

		if (caught_HUP) {	/* new logfile		*/
			Trace(TR_MISC, "SIGHUP recieved");
			caught_HUP   = 0;
			(void) pthread_mutex_lock(&(door_mutex));
			close(fsa);
			fsa = Open_FSA_log_file(fs_name);
			(void) pthread_mutex_unlock(&(door_mutex));
			if (fsa < 0) {
				Trace(TR_FATAL, "[%s] rollover failure, %s",
				    fs_name, strerror(errno));
				abort_daemon = 1;
				running = 0;
			}
		}

		if (check_expire) {
			check_expire = 0;
			check_log_expire();
		}
	}
}


/*
 * Initialize module.
 */
static void
init_event(char *fs_name)
{
	struct sam_event_open_arg arg;	/* SC_event_open syscall args	*/
	int		did;		/* Door descriptor		*/
	int		i;

	/*
	 * Create door
	 */
	if ((did = door_create(read_events, (void *)fs_name, 0)) < 0) {
		Trace(TR_FATAL, "[%s] door_create failed: %s",
		    fs_name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*
	 *	Connect to the file system event handler.
	 * 	If busy, try up to EVENT_SYSCALL_RETRY (5) times.
	 */
	memset(&arg, 0, sizeof (arg));
	strncpy(arg.eo_fsname, fs_name, sizeof (arg.eo_fsname));
	arg.eo_door_id	= did;
	arg.eo_interval	= Event_Interval;
	arg.eo_bufsize	= Event_Buffer_Size == 0 ? 0 :
	    sizeof (struct sam_event_buffer) + Event_Buffer_Size *
	    sizeof (sam_event_t);
	arg.eo_mask	= 0xffff;

	for (i = 0; i < Event_Open_Retry; i++) {
		if (sam_syscall(SC_event_open, &arg, sizeof (arg)) == 0) {
			break;
		}
		if (errno != EBUSY) {
			sam_syslog(LOG_ERR,
			    "%s: SAM system call event_open failed: %m",
			    fs_name);
			Trace(TR_FATAL, "[%s] SAM system call event_open "
			    "failed: %s", fs_name, strerror(errno));
			exit(EXIT_FAILURE);
		}
		sleep(1);
	}
	eb = arg.eo_buffer.ptr;
	if (eb == NULL) {
		sam_syslog(LOG_ERR,
		    "%s: SAM system call event_open: no buffer returned",
		    fs_name);
		Trace(TR_FATAL, "[%s] SAM system call event_open: no buffer"
		    " returned", fs_name);
		exit(EXIT_FAILURE);
	}

	/*
	 * Check for lost events.
	 */
	if (eb->eb_ev_lost[0] != eb->eb_ev_lost[1]) {
		i = eb->eb_ev_lost[0] - eb->eb_ev_lost[1];
		sam_syslog(LOG_NOTICE,
		    "%s: %d file system events losts", fs_name, i);
		Trace(TR_MISC, "[%s] %d file system events losts", fs_name, i);
	}

	eb->eb_dstatus = FSA_STATUS_RUNNING;
}


/*
 *	read_events - Event processor for door thread.
 *
 *	read_events is the door initiated service proceedure.  Its function
 *	is to read events from the file system and write them to the log
 *	file.
 *
 *	See door_create(3DOOR) for argument specification.
 */
/* ARGSUSED */
static void
read_events(
	void		*cookie,
	char		*argp,
	size_t		arg_size,
	door_desc_t	*dp,
	uint_t		n_desc)
{
	int		in;		/* Event buffer in pointer	*/
	int		n_ev;		/* Number of events in buffer	*/
	char		*fs_name = (char *)cookie;
	sam_event_t	null_event;	/* Used for closing event	*/
	int		wst;		/* Write status			*/

	(void) pthread_mutex_lock(&(door_mutex));

	if (log_rollover || time(0l) - logtime > Log_Rollover) {
		log_rollover = 0;
		close(fsa);
		fsa = Open_FSA_log_file(fs_name);
		if (fsa < 0) {
			Trace(TR_FATAL, "[%s] rollover failure, %s",
			    fs_name, strerror(errno));
			abort_daemon = 1;
			goto sortie;
		}
	}

	if (eb == NULL) {
		goto sortie;
	}

	if (eb->eb_buf_full[0] != eb->eb_buf_full[1]) {
		EventBufFull += eb->eb_buf_full[0] - eb->eb_buf_full[1];
		eb->eb_buf_full[1] = eb->eb_buf_full[0];
	}

	EventNotifies++;

	/*
	 * 	Process file system activity events:
	 *	    1. write buffer to log file.
	 *	    2. scan events for invalid events.
	 */
	while (eb->eb_out != eb->eb_in) {
		in = eb->eb_in;
		if (eb->eb_out > in)			/* buffer wrap	*/
		in = eb->eb_limit;
		n_ev = (in - eb->eb_out);
		wst  = Write_FSA_log_file(fs_name, &eb->eb_event[eb->eb_out],
		    n_ev);
		if (in == eb->eb_limit)
			in = 0;

		if (wst) {
			eb->eb_dstatus = FSA_STATUS_ABORT;
			abort_daemon   = 1;
		}

		while (eb->eb_out != in) {		/* Event scan	*/
			sam_event_t	*ev;
			int		event;

			ev = &eb->eb_event[eb->eb_out];
			if (eb->eb_out + 1 < eb->eb_limit) { /* advance out */
				eb->eb_out++;
			} else {
				eb->eb_out = 0;
			}
			EventCount++;
			event = ev->ev_num;

			if (Verbose) {
				print_event(stderr, ev);
			}

			if (event <= ev_none || event >= ev_max) {
				Trace(TR_ERR,
				    "[%s] invalid event %d", fs_name, event);
				abort_daemon = 1;
			}

			if (event == ev_umount) { /* watch for umount event */
				memset(&null_event, 0, sizeof (sam_event_t));
				null_event.ev_num   = ev_none;
				null_event.ev_time  = time(0l);
				null_event.ev_param = FSA_STATUS_EXIT;
				wst = Write_FSA_log_file(fs_name, &null_event,
				    1);
				eb->eb_dstatus = wst ?
				    FSA_STATUS_ABORT : FSA_STATUS_EXIT;
				close(fsa);
				Trace(TR_MISC,
				    "[%s] Unmount received", fs_name);
				exit(EXIT_SUCCESS);
			}
		}
	}

	if (eb->eb_umount) {
		memset(&null_event, 0, sizeof (sam_event_t));
		null_event.ev_num   = ev_none;
		null_event.ev_time  = time(0l);
		null_event.ev_param = FSA_STATUS_EXIT;
		wst = Write_FSA_log_file(fs_name, &null_event, 1);
		eb->eb_dstatus = wst ?
		    FSA_STATUS_ABORT : FSA_STATUS_EXIT;
		close(fsa);
		Trace(TR_MISC, "[%s] Umount received", fs_name);
		exit(EXIT_SUCCESS);
	}

sortie:
	(void) pthread_mutex_unlock(&(door_mutex));

	if (abort_daemon) {		/* If abort wake up main loop	*/
		(void) pthread_cond_signal(&(main_thr.thd_cv));
	}

	if (door_return(cookie, 0, NULL, 0) == -1) {
		sam_syslog(LOG_ERR, "%s: door_return failed: %m", fs_name);
		Trace(TR_FATAL, "[%s] door_return failed: %s",
		    fs_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	/* NOTREACHED */
}


/*
 *	fsalogdTraceInit - Initialize Trace Facility.
 *
 *	fsalogdTraceInit initializes trace and custom message facilities.
 *	The rest of initialization is finished later on.  See
 *	fsalogdInit().
 *
 */

static void
fsalogdTraceInit(void)
{
	sigset_t	set;

	program_name = SAM_FSALOGD;
	CustmsgInit(Daemon, NULL);
#if	0
	CustmsgInit(Daemon, NotifyRequest);
#endif

	/* Block all signals.  */

	if (sigfillset(&set) == -1) {
		LibFatal(sigfillset, "");
	}
	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
		LibFatal(sigprocmask, "block signals");
	}

	/* Set up tracing.  */

	if (Daemon) {
		/* Provide file descriptors for the standard files.  */

		(void) close(STDIN_FILENO);
		if (open("/dev/null", O_RDONLY) != STDIN_FILENO) {
			LibFatal(open, "stdin /dev/null");
		}
		(void) close(STDOUT_FILENO);
		if (open("/dev/null", O_RDWR) != STDOUT_FILENO) {
			LibFatal(open, "stdout /dev/null");
		}
		(void) close(STDERR_FILENO);
		if (open("/dev/null", O_RDWR) != STDERR_FILENO) {
			LibFatal(open, "stderr /dev/null");
		}

		TraceInit(program_name, TI_fsalogd | TR_MPLOCK | TR_MPMASTER);
	} else {
		/* Redirect trace messages to stdout/stderr.  */

		TraceInit(NULL, TI_none);
		*TraceFlags = (1 << TR_module) | ((1 << TR_date) - 1);
		*TraceFlags &= ~(1 << TR_alloc);
		*TraceFlags &= ~(1 << TR_dbfile);
		*TraceFlags &= ~(1 << TR_files);
		*TraceFlags &= ~(1 << TR_oprmsg);
		*TraceFlags &= ~(1 << TR_sig);
	}
}


/*
 *	fsalogdInit - Initialize Daemon.
 *
 *	1. Change working directory to daemon directory.
 *	2. Set GID.
 *	3. Set umask.
 */

static void
fsalogdInit(void)
{
	sam_defaults_t	*df;
	upath_t		runPath;

	/* If Daemon, switch into /var/opt/SUNWsamfs/fsalogd/fs_name.  */

	if (Daemon) {
		sprintf(runPath, "%s/%s/%s", SAM_VARIABLE_PATH, "fsalogd",
		    fs_name);
		MakeDir(runPath);
		if (chdir(runPath) == -1) {
			LibFatal(chdir, runPath);
		}
	}

	/*
	 * Set group id for all created directories and files.
	 */

	df = GetDefaults();
	if (setgid(df->operator.gid) == -1) {
		Trace(TR_ERR, "setgid(%ld)", df->operator.gid);
	}

	(void) umask(0027);	/* Allow only root writes */
}


/*
 *	sigCatcher - Signal Processor.
 *
 *	sigCatcher is called by signals() when a listed signal is received.
 *
 *	On Entry:
 *	    signal_caught	Signal number.
 *
 *	On Return:
 *	    Thread condition signal sent to main program loop.
 *
 */


static void
sigCatcher(int signal_caught)

{

	switch (signal_caught) {

	case    SIGALRM:
		caught_ALRM++;
		log_rollover |= (time(0l) - logtime > Log_Rollover);
		check_expire = log_rollover;
		break;

	case    SIGTERM:
	case    SIGINT:
		caught_TERM++;
		break;

	case    SIGHUP:
		caught_HUP++;
		log_rollover = 1;
		break;

	default:
		Trace(TR_ERR, "caught unexpected signal:  %d", signal_caught);
		break;
	}

	(void) pthread_cond_signal(&(main_thr.thd_cv));
}



/*
 *	Open_FSA_log_file - Open/Create File System Activity Log.
 *
 *	Creates log file in the directory /var/opt/SUNWsamfs/fsalog.
 *	Log file names are name.yymmddhhmm.log where name is the
 *	SAMFs family set name and yymmddhhmm is the date/time of creation.
 *
 *	At open, an initial null event is written to the log file.
 *
 *	On Entry:
 *	    fs_name = Family set name.
 *
 *	On Return:
 *	    Returns -1 if open failed, otherwise returns
 *	    file descriptor.
 */

static int
Open_FSA_log_file(char *fs_name)	/* SAMFS family set name */

{
	char		*fsa_path;	/* FSA log full path		*/
	char		file[40];	/* Log file name		*/
	mode_t		mode		/* Log file creation mode	*/
					= S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
	sam_event_t	null_event;	/* Time stamp event		*/
	int		l;

	l = strlen(Logfile_Path) + strlen(fs_name) + 42;
	SamMalloc(fsa_path, l);

	logtime = time(0l);
	strftime(file, 40, "%Y%m%d%H%M.log", localtime(&logtime));

	strcpy(fsa_path, Logfile_Path);
	strcat(fsa_path, fs_name);
	strcat(fsa_path, ".");
	strcat(fsa_path, file);

	fsa = open(fsa_path, O_WRONLY|O_APPEND|O_CREAT, mode);

	if (fsa == -1) {
		sam_syslog(LOG_ERR,
		    "%s: log file open(%s) failed: %m", fs_name, fsa_path);
		Trace(TR_ERR, "[%s] log file open(%s) failed: %s",
		    fs_name, fsa_path, strerror(errno));
		return (-1);
	} else {
		Trace(TR_MISC, "[%s] log file now: %s", fs_name, fsa_path);
	}

	memset(&null_event, 0, sizeof (sam_event_t));
	null_event.ev_num   = ev_none;
	null_event.ev_time  = logtime;
	null_event.ev_param = FSA_STATUS_RUNNING;

	if (Write_FSA_log_file(fs_name, &null_event, 1)) {
		close(fsa);
		fsa = -1;
	}

	SamFree(fsa_path);
	return (fsa);
}


/*
 *	Write_FSA_log_file - Write to the File System Activity Log.
 *
 *	Writes entries to the file system activity log.
 *
 *	On Entry:
 *	    fs_name  = Family set name (used for error messages).
 *	    ebuf     = Event buffer.
 *	    n_events = Number of events in buffer.
 *
 *	On Return:
 *	    Returns true if write failed.
 */

static int
Write_FSA_log_file(
	char		*fs_name,	/* SAMFS family set name	*/
	sam_event_t	*ebuf,		/* Event buffer			*/
	int		n_events)	/* Number of events in buffer	*/

{
	int		n;		/* Write(2) status		*/
	int		len;		/* Buffer length in bytes	*/

	len = n_events * sizeof (sam_event_t);
	n   = write(fsa, ebuf, len);

	if (n < 0) {
		sam_syslog(LOG_ERR, "%s: log file write error: %m", fs_name);
		Trace(TR_ERR, "[%s] log file write error: %s",
		    fs_name, strerror(errno));
	}
	if (n >= 0 && n != len) {
		sam_syslog(LOG_ERR, "%s: log file write truncated: %m",
		    fs_name);
		Trace(TR_ERR, "[%s] log file write truncated", fs_name);
	}

	return (n != len);
}

/*
 * Checks each logfile in 'Logfile_Path' directory.  If the
 * logfile timestamp is older than the calculated expired
 * time (current time - expire interval), then the log is
 * deleted.
 */
static void
check_log_expire(void)
{
	int logdir_fd = -1;
	DIR *logdir = NULL;
	dirent_t *ent = NULL;
	int fs_len;
	time_t expire_time;
	struct tm log_time;

	fs_len = strlen(fs_name);
	expire_time = time(0) - Log_Expire;

	if ((logdir_fd = open(Logfile_Path, O_RDONLY, 0)) < 0) {
		Trace(TR_ERR, "Error opening %s", Logfile_Path);
		goto out;
	}

	if ((logdir = fdopendir(logdir_fd)) == NULL) {
		Trace(TR_ERR, "Error opening %s from fd.",
		    Logfile_Path);
		goto out;
	}

	SamMalloc(ent, sizeof (dirent_t) +
	    fpathconf(logdir_fd, _PC_NAME_MAX));

	while ((ent = readdir(logdir)) != NULL) {
		int ent_len = strlen(ent->d_name);
		char *dot1 = strchr(ent->d_name, '.');
		char *dot2 = strrchr(ent->d_name, '.');

		if (ent_len < fs_len + 5) {
			/* length at least fs_name..log */
			continue;
		}

		if (strncmp(fs_name, ent->d_name, fs_len) != 0) {
			/* fs_name needs to be prefix */
			continue;
		}

		if (strncmp(".log", &ent->d_name[ent_len-4], 4) != 0) {
			/* .log needs to be suffix */
			continue;
		}

		if (dot1 == dot2) {
			/* must be at least two dots */
			continue;
		}

		/* Timestamp starts after dot1 and ends at dot2 */
		dot1 = strptime(dot1+1, "%Y%m%d%H%M", &log_time);

		if (dot1 != dot2) {
			/* didn't end up at dot2 */
			continue;
		}

		/* If log timestamp is expired, delete the file */
		if (mktime(&log_time) < expire_time) {
			unlinkat(logdir_fd, ent->d_name, 0);
		}
	}

	if (errno != 0) {
		if (errno != ENOENT) {
			Trace(TR_ERR, "Error reading log directory");
		}
		errno = 0;
	}

out:
	if (ent != NULL) {
		SamFree(ent);
	}

	if (logdir != NULL) {
		closedir(logdir);
	}

	if (logdir_fd >= 0) {
		close(logdir_fd);
	}
}


/*
 *	print_event - Print File System Activity Event.
 *
 *	On Entry:
 *	    F		File descriptor to write to.
 *	    ev		Event to print.
 */

void
print_event(

	FILE		*F,		/* File descriptor to print to	*/
	sam_event_t	*ev)		/* Event to print		*/

{
	int		event;

	event = ev->ev_num;

	if (event == ev_none)
		return;
	if (event >  ev_max)
		event = ev_max;

	fprintf(F, "%d.%d %s", ev->ev_id.ino, ev->ev_id.gen,
	    fsa_event_names[event].label);
	if (fsa_event_names[event].pino)
		fprintf(F, " pi=%d.%d", ev->ev_pid.ino, ev->ev_pid.gen);
	if (fsa_event_names[event].p)
		fprintf(F, " p=%d", ev->ev_param);
	fprintf(F, "\n");
}


/*
 *	sigFatal - Fatal Signal Processor.
 *
 *	sigFatal is called by signals() when a fatal signal is received.
 *
 *	On Entry:
 *	    sig		Signal number.
 *
 */
static void
/*ARGSUSED0*/
sigFatal(
	int sig)
{
#ifdef	DEBUG
	TraceRefs();
#endif
}
