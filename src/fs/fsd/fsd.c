/*
 *	fsd.c - filesystem daemon.
 *
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

#pragma ident "$Revision: 1.158 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#ifdef sun
#include <limits.h>
#endif /* sun */
#ifdef linux
#include <linux/limits.h>
#endif /* linux */
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef sun
#include <sys/systeminfo.h>
#include <sys/mount.h>
#endif /* sun */

/* Solaris headers. */
#include <syslog.h>
#include <sys/sysmacros.h>
#ifdef linux
#include <linux/utsname.h>
#endif /* linux */

/* SAM-FS headers. */
#include "pub/version.h"
#include "sam/types.h"
#if !defined(QFS)
#include "aml/archiver.h"
#include "aml/device.h"
#include "aml/stager.h"
#endif /* !defined(QFS) */
#include "sam/custmsg.h"
#include "pub/devstat.h"
#include "sam/exit.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#define	TRACE_CONTROL
#include "sam/sam_trace.h"
#include "sam/syscall.h"
#include "fsdaemon.h"
#include "syslogerr.h"
#include "sam/spm.h"
#ifdef sun
#include "sam/fioctl.h"
#endif /* sun */

/* Local headers. */
#define	DEC_INIT
#include "fsd.h"
#undef DEC_INIT
#include "utility.h"

/* Private data. */

/*
 * Child status queue.
 *
 * A circular queue filled by the signal handler for each SIGCHLD received.
 * Emptied by checkChildren().
 * This prevents race conditions during fork().
 */
static struct ChildStatus {
	pid_t	CsPid;
	int		CsStat;
} *childStatusQueue = NULL;
static ATOM_INT_T   csIn = 0;
static ATOM_INT_T   csOut = 0;
static ATOM_INT_T   csLimit = 0;

static struct ChildProc {
	int		count;
	struct ChildProcEntry {
		uname_t CpName;			/* process name */
		upath_t CpFsname;		/* Assoc file system name */
		char	CpParam1[16];		/* First parameter */
		char	CpParam2[16];		/* Second parameter */
		int	CpArgc;			/* Number of arguments */
		time_t	CpRestartTime;
		uint32_t CpFlags;
		pid_t	CpPid;
		int		CpTid;		/* trace id for TRACE_ROTATE */
	} entry[1];
} *childProcTable = NULL, *deadPool = NULL;

#ifdef	sun
static char *hsmDaemons[] = { SAM_ARCHIVER, SAM_STAGEALL, SAM_STAGER, NULL };
#endif
static struct TraceCtlBin *tb = NULL;

static boolean_t fileRequired = FALSE;	/* Set if config file is required */
static boolean_t moreInfo = FALSE;	/* Set for config file reading */
static boolean_t reconfig = FALSE;	/* Set to cause reconfiguration */
static boolean_t startAmld = FALSE;	/* Set to start sam-amld  */
static boolean_t stopAmld = FALSE;	/* Set to stop sam-amld  */
static boolean_t stopHsm = FALSE;	/* Set to stop hsm  */
static boolean_t ready = FALSE;		/* ready to execute */

static pid_t Pid = 0;
static int stopSignal = 0;		/* Signal to stop all processes */
static int nextWakeup;			/* secs until next awakening */
static int mountedFs;			/* number of fs mounted */
static int mountedHsmFs;		/* number of archiving fs mounted */
static int hsmsRunning = 0;		/* stager/archiver/... running? */

static sigset_t	sig_hold;		/* signal hold mask */
static struct sigaction sig_action;	/* signal actions */

static jmp_buf errReturn;		/* Error message return */

/* Private functions. */
static void catchSignals(int sig);
static void nullCatcher(int sig);
static void processSignals(int allow);
static void daemonize(void);
static void checkChildren(void);
static void checkRelease(void);
static void checkTraceFiles(void);
static void clearReleaser(char *fsname);
static void configure(char *defaults_name, char *diskvols_name,
		char *fscfg_name);
static void issueResellersMessage(void);
static void runManual(int argc, char *argv[]);
static void sendSyslogMsg(struct sam_fsd_syslog *args);
static void signalProc(int sig, char *pname, char *fsname);
static void sendFsMount(char *fsname);
static void sendFsUmount(char *fsname);
static void setInittab(void);
static void sigInit(void (*handler)(int));
static void sigReset(void);
#ifdef sun
static void countMountedFS(void);
static void startStopHsm(int cmd, char *fs);
static void startSamamld(int fsMounted);
static void startSamrftd(void);

/* LQFS: Logging control functions */
static void disable_logging(char *mp, char *special);
static void enable_logging(char *mp, char *special);
static int checkislog(char *mp);
static void initLogging(char *fsname);
#endif /* sun */


int
main(int argc, char *argv[])
{
	int i, errcode;
	int killsigs[] = { SIGEMT, SIGEMT, SIGKILL, SIGKILL, 0};
	char errbuf[SPM_ERRSTR_MAX];

	Pid = getpid();

#if defined(QFS)
	QfsOnly = TRUE;
#endif /* defined(QFS) */
	program_name = SAM_FSD;
	SamMalloc(childProcTable, sizeof (struct ChildProc));
	SamMalloc(deadPool, sizeof (struct ChildProc));
	memset(childProcTable, 0, sizeof (struct ChildProc));
	memset(deadPool, 0, sizeof (struct ChildProc));
	childProcTable->count = 1;
	deadPool->count = 1;

	CustmsgInit(1, NULL);	/* Assure message translation */
	if (argc == 2 && strcmp(argv[1], "-C") == 0) {
		struct sam_fs_status *fsarray;

		/*
		 * Filesystem is not configured and sam-fsd
		 * was executed for a filesystem program.
		 *
		 * Modload the samfs module and perform basic configuration.
		 */
		checkRelease();			/* verify Solaris version */
		TraceInit(program_name, TI_fsd);
		Trace(TR_MISC, "File system daemon started - config only");
		LoadFS((void *)FatalError);
		if (GetFsStatus(&fsarray) > 0) {
			/* File system already configured.\n */
			printf(GetCustMsg(17294));
			return (EXIT_SUCCESS);
		}

		FsCfgOnly = TRUE;
		daemonize();		/* fork -- only child returns */

		setInittab();
		free(fsarray);
#ifdef linux
		/* init was HUP'ed. It will respawn sam-fsd within 5 secs. */
		sleep(5);
#endif /* linux */
	} else if (getppid() != 1 || getpgid(0) != getpid()) {
		/*
		 * Not started by init.
		 * Process configuration files to report all errors.
		 */
		checkRelease();			/* verify Solaris version */
		TraceInit(NULL, TI_none);
		LoadFS((void *)FatalError);
		runManual(argc, argv);
		return (EXIT_SUCCESS);
	} else {

		/*
		 * Started by 'init', run as a daemon.
		 */
		Daemon = TRUE;

		/*
		 * Init (should have) started sam-fsd without open file
		 * descriptors.  In any case, before tracing initialization,
		 * grab the classic stdin, stdout, stderr file descriptors
		 * as /dev/null.
		 */
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

		MakeTraceCtl();
		checkRelease();		/* verify Solaris version */

		LoadFS((void *)FatalError);
	}

	/*
	 * Change to our working directory for any core files.
	 * Initialize message processing.
	 */
	MakeDir(SAM_VARIABLE_PATH"/fsd");
	if (chdir(SAM_VARIABLE_PATH"/fsd") == -1) {
		FatalError(608, SAM_VARIABLE_PATH"/fsd");
	}

	sigInit(catchSignals);

	/*
	 * Ensure that we're the only sam-fsd daemon running.  Two cases:
	 * (1) This proc wasn't started by init (exit if any others exist); or
	 * (2) This proc was started by init (kill off all others).
	 */
	if ((i = sam_lockout(SAM_FSD,
	    ".", SAM_FSD ".", FsCfgOnly ? NULL : killsigs)) != 0) {
		if (i < 0) {
			FatalError(17291, SAM_VARIABLE_PATH "/fsd");
		}
		if (FsCfgOnly || stopSignal) {
			exit(EXIT_SUCCESS);
		}
	}

	/*
	 * Read configuration files.
	 */
	moreInfo = TRUE;
	if (setjmp(errReturn) == 0) {
		reconfig = FALSE;
	}

	MakeDir(SAM_VARIABLE_PATH"/uds");
#ifdef METADATA_SERVER
	MakeDir(SAM_VARIABLE_PATH"/sharefsd");
	MakeDir(SAM_VARIABLE_PATH"/shrink");

	if (!QfsOnly) {
		struct stat lbuf;

		MakeDir(SAM_VARIABLE_PATH"/amld");
		MakeDir(SAM_VARIABLE_PATH"/archiver");
		MakeDir(SAM_VARIABLE_PATH"/releaser");
		/*
		 * HA-SAM requires stager to be linked to a directory.
		 * So check if stager is a link, if it is then don't
		 * blow away the link by making it a directory.
		 * This is applicable only on solaris.
		 */
		if (lstat(SAM_VARIABLE_PATH"/stager", &lbuf) != 0 ||
		    !S_ISLNK(lbuf.st_mode)) {
			MakeDir(SAM_VARIABLE_PATH"/stager");
		}
		MakeDir(SAM_VARIABLE_PATH"/rft");
	}

	/*
	 * Start up listener port for service registration.  This
	 * forks off a thread that listens in the background for
	 * service registrations and service requests.  Note that
	 * SPM requires the SAM_VARIABLE_PATH/uds to operate.
	 *
	 * If it doesn't work, we're pretty broken.
	 *
	 * We can come through here again via longjmp (to the setjmp
	 * above).  So we check the error return from spm_initialize
	 * to ensure that we aren't restarting.
	 */
	for (i = 0; spm_initialize(0, &errcode, &errbuf[0]) < 0; i++) {
		if (errcode == ESPM_AGAIN) {
			break;
		}
		if (i > 3) {
			FatalError(17290, errcode, &errbuf[0]);
		}
		sleep(2);
	}
#endif /* METADATA_SERVER */

	/*
	 * csLimit is the maximum number of outstanding children that
	 * sam-fsd can have.  This number is o(m * FileSysNumof + n * NDmn)
	 * where m ~= 2 (sharefsd, releaser), NDmn ~= 6 (archiverd,
	 * stagerd, stagealld, rftd, initd, and notify), n ~= 2 (the
	 * daemon itself, plus a proc to rotate its trace file).  Also,
	 * filesystems can be added on the fly.  Slots are cheap.
	 */
	if (childStatusQueue == NULL) {
		csLimit = (2 * FileSysNumof) + (2 * 6) + 128; /* 128 extras */
		SamMalloc(childStatusQueue, csLimit *
		    sizeof (struct ChildStatus));
	}

	if (!reconfig) {
		configure(NULL, NULL, NULL);
	}
	if (Daemon) {
		TraceInit(program_name, TI_fsd);
		Trace(TR_MISC, "File system daemon started");
	}

	/*
	 * Kill off waiting parent, so it returns to caller now that
	 * things are properly initialized.
	 */
	if (Pid > 1 && FsCfgOnly) {
		kill(Pid, SIGCLD);
	}

	/*
	 * Begin operation.
	 * Allocate child status queue.
	 */
	sam_syslog(LOG_INFO, "Starting %s version %s", SAM_NAME, SAM_VERSION);
	sam_syslog(LOG_INFO, COPYRIGHT);
	issueResellersMessage();
	/*
	 * SIGCHLD for death of child processes.
	 */
	(void) sigaction(SIGCHLD, &sig_action, NULL);
	ServerInit();

	ready = TRUE;

#ifdef sun
	startSamrftd();
#endif /* sun */

	if (setjmp(errReturn) != 0) {
		reconfig = TRUE;
	}

	for (;;) {
		int r, saveErrno;
		struct sam_fsd_cmd cmd;

		if (stopSignal) {
			signalProc(stopSignal, NULL, NULL);
			stopSignal = 0;
			break;			/* exit */
		}

		checkChildren();

		if (reconfig) {
			reconfig = FALSE;
			sam_syslog(LOG_INFO, "Rereading configuration files");
			configure(NULL, NULL, NULL);
			TraceReconfig();
#ifdef sun
			startSamrftd();
#endif /* sun */
		}

		if (startAmld) {
			startAmld = FALSE;
#ifdef sun
			sam_syslog(LOG_INFO, "Starting sam-amld");
			startSamamld(1);
#endif /* sun */

		}
		if (stopAmld) {
			stopAmld = FALSE;
#ifdef sun
			sam_syslog(LOG_INFO, "Stop sam-amld");
			startSamamld(0);
#endif /* sun */

		}
		if (stopHsm) {
			stopHsm = FALSE;
#ifdef sun
			if (hsmsRunning) {
				sam_syslog(LOG_INFO, "Stopping Hsm");
				startStopHsm(FSD_stop_sam, "");
			}
#endif /* sun */

		}
		checkTraceFiles();

		/*
		 * Enable signal processing only while we're not doing
		 * anything else.
		 *
		 * Enable signals before making the alarm call - they may
		 * change nextWakeup.  After signals are enabled, sleep
		 * until the next known pending event is due.
		 */
		processSignals(TRUE);

		/*
		 * There is a chance to receive a SIGCHLD while signals
		 * were blocked.  In such a case, blocked signals are
		 * processed right after signals were unblocked in
		 * processSignals(TRUE) above. We need to double
		 * check pending SIGCHLD here.
		 */
		if (csIn != csOut) {
			r = -1;
			saveErrno = EINTR;		/* presume SIGCHLD */
		} else {
			(void) alarm(nextWakeup);

			/*
			 * Tell the filesystem that we are ready for work.
			 */
			r = sam_syscall(SC_fsd, (void *)&cmd, sizeof (cmd));
			saveErrno = errno;
		}

		processSignals(FALSE);

		if (r < 0) {
			if (saveErrno == ENOPKG) {
#ifdef sun
				spm_shutdown(&errcode, &errbuf[0]);
#endif /* sun */
				FatalError(0, "sam_syscall(SC_fsd) failed.");
				/* NO RETURN */
			}
			if (saveErrno != ENOENT && saveErrno != EINTR) {
				sam_syslog(LOG_WARNING, "sam_syscall: %s",
				    strerror(saveErrno));
			}
			continue;
		}

		/*
		 * Process command from the filesystem.
		 */
		switch (cmd.cmd) {

		case (FSD_sharedmn): {
			struct sam_fsd_mount *args =
			    (struct sam_fsd_mount *)&cmd.args;

			StartShareDaemon(args->fs_name);
			}
			break;

		case (FSD_mount): {
			struct sam_fsd_mount *args =
			    (struct sam_fsd_mount *)&cmd.args;
			time_t	tv;
			char	time_str[64];

			Trace(TR_MISC, "mount(%s)", args->fs_name);
			tv = args->init;
			strftime(time_str, sizeof (time_str), "%Y-%m-%d %T",
			    localtime(&tv));
			sam_syslog(LOG_INFO, "%s built %s.", args->fs_name,
			    time_str);
#ifdef sun
			initLogging(args->fs_name);
#endif /* sun */
			signalProc(SIGHUP, NULL, args->fs_name);
			if (!QfsOnly) {
				sendFsMount(args->fs_name);
#ifdef sun
				startStopHsm(FSD_mount, args->fs_name);
#endif /* sun */
			}
			}
			break;

		case (FSD_releaser): {
#ifdef sun
			struct sam_fsd_releaser *args =
			    (struct sam_fsd_releaser *)&cmd.args;
			char	*argv[3];

			argv[0] = SAM_RELEASER;
			argv[1] = args->fs_name;
			argv[2] = NULL;
			StartProcess(2, argv, 0, 0);
#endif /* sun */
			}
			break;

		case (FSD_shrink): {
#ifdef sun
			struct sam_fsd_shrink *args =
			    (struct sam_fsd_shrink *)&cmd.args;
			char	ceq[16];
			char	*argv[5];

			argv[0] = SAM_SHRINK;
			argv[1] = args->fs_name;
			if (args->command == DK_CMD_remove) {
				argv[2] = "remove";
			} else {
				argv[2] = "release";
			}
			sprintf(ceq, "%d", args->eq);
			argv[3] = ceq;
			argv[4] = NULL;
			Trace(TR_MISC, "shrink(%s, %s, %s)", argv[1],
			    argv[2], argv[3]);
			StartProcess(4, argv, 0, 0);
#endif /* sun */
			}
			break;

		case (FSD_syslog): {
			struct sam_fsd_syslog *args =
			    (struct sam_fsd_syslog *)&cmd.args;

			sendSyslogMsg(args);
			}
			break;

		case (FSD_preumount): {
			struct sam_fsd_umount *args =
			    (struct sam_fsd_umount *)&cmd.args;
			struct sam_fs_info fi;

			Trace(TR_MISC, "preunmount(%s, %d)", args->fs_name,
			    args->umounted);
#ifdef	sun
			if (!QfsOnly) {
				if (GetFsInfo(args->fs_name, &fi) != -1) {
					if (fi.fi_status & FS_RELEASING) {
						signalProc(SIGINT,
						    SAM_RELEASER,
						    args->fs_name);
						Trace(TR_MISC,
						    "Stopped releaser "
						    "on %s to unmount",
						    args->fs_name);
					}
				}
			}
#endif
			}
			break;

		case (FSD_umount): {
			struct sam_fsd_umount *args =
			    (struct sam_fsd_umount *)&cmd.args;

			Trace(TR_MISC, "unmount(%s, %d)", args->fs_name,
			    args->umounted);
			if (!QfsOnly) {
				sendFsUmount(args->fs_name);
#ifdef sun
				startStopHsm(FSD_umount, args->fs_name);
#endif /* sun */
			}
			}
			break;

		case (FSD_stalefs): {
			/*
			 * The filesystem was busy, and forcibly unmounted.
			 * It needs a new share daemon; the old one may or
			 * may not exit shortly.  Stop the old daemon, and
			 * whether or not it actually stops, the next mount
			 * request will respin a new one.
			 */
			struct sam_fsd_umount *args =
			    (struct sam_fsd_umount *)&cmd.args;
			char	*argv[3];

			Trace(TR_MISC, "stalefs(%s, %d)", args->fs_name,
			    args->umounted);
			argv[0] = SAM_SHAREFSD;
			argv[1] = args->fs_name;
			argv[2] = NULL;
			StopProcess(argv, TRUE, SIGINT);
			}
			break;

		case (FSD_restart): {
			struct sam_fsd_restart *args =
			    (struct sam_fsd_restart *)&cmd.args;

			Trace(TR_MISC, "restart(%s)", args->fs_name);
			signalProc(SIGHUP, NULL, NULL);
			}
			break;

		default:
			sam_syslog(LOG_WARNING,
			    "sam_syscall: unknown command", cmd.cmd);
			break;
		}
	}

#ifdef sun
	spm_shutdown(&errcode, &errbuf[0]);
#endif /* sun */

	return (EXIT_SUCCESS);
}


/*
 * Run sam-fsd manually.
 */
static void
runManual(int argc, char *argv[])
{
	extern int optind;
	char	*defaults_name = NULL;
	char	*diskvols_name = NULL;
	char	*fscfg_name = NULL;
	int		c;
	int		errors;

	Daemon = FALSE;
	CustmsgInit(0, NULL);	/* Assure message translation */

	errors = 0;
	while ((c = getopt(argc, argv, "c:d:f:m:v")) != EOF) {
		switch (c) {
		case 'c':
			defaults_name = optarg;
			break;
		case 'd':
			diskvols_name = optarg;
			break;
		case 'f':
			fscfg_name = optarg;
			break;
		case 'm':
			McfName = optarg;
			break;
		case 'v':
			Verbose = TRUE;
			break;
		case '?':
		default:
			errors++;
			break;
		}
	}
	if (errors != 0 || (argc - optind) > 0) {
	/* usage: */
	fprintf(stderr,
	    "%s %s [-c defaults] [-d diskvols] [-f samfs] [-m mcf] [-v] \n",
	    GetCustMsg(4601), program_name);
		exit(EXIT_USAGE);
	}

	/*
	 * Read configuration files.
	 * Go through the motions of configuring the file systems.
	 */
	configure(defaults_name, diskvols_name, fscfg_name);
}


/*
 * daemonize()
 *
 * Fork off a child, and let it return to do initialization.
 * Parent hangs around in here until initialization is complete
 * or times out.
 *
 * The parent here should exit only when either:
 * (a) FS initialization has completed (GetFsStatus() > 0), or
 * (b) we've timed out.
 *
 */
static void
daemonize(void)
{
	pid_t child;
	time_t t;
	int r, status;
	struct sam_fs_status *fsarray;

	sigInit(nullCatcher);

	child = fork1();
	if (child == 0) {
		return;
	}

	/*
	 * parent or error - exit when done (no return)
	 */
	if (child < 0) {
		LibFatal(fork1, "configure");
		exit(EXIT_FAILURE);
	}

	processSignals(TRUE);		/* Enable signal reception */

#define		INIT_TIMEOUT	30	/* max time for sam-fsd to finish */
#define		WAIT_ALARMINT	3	/* check for config done period */

	t = time(NULL);
	do {
		int wt;

		/*
		 * Set a wakeup for the remaining INIT_TIMEOUT interval, but no
		 * more than WAIT_ALARMINT seconds and no less than one second.
		 */
		wt = (t + INIT_TIMEOUT) - time(NULL);
		wt = MIN(wt, WAIT_ALARMINT);
		wt = MAX(wt, 1);
		(void) alarm(wt);
		if (child) {
			r = waitpid(child, &status, 0);	/* await child */
			if (r >= 0) {
				child = 0;
			}
		} else {
			pause();
		}
	} while ((time(NULL) - t < INIT_TIMEOUT) &&
	    GetFsStatus(&fsarray) <= 0);
	_exit(EXIT_SUCCESS);
}


/*
 * Process fatal error.
 * Send error message to appropriate destination and error exit.
 */
void
FatalError(
	int msgNum,		/* Msg catalog num.  If 0, don't use catalog */
	...)			/* Msg args, If 0, first is fmt like printf */
{
	static char msg_buf[2048];
	va_list args;
	char	*fmt;
	char	*p, *pe;
	int		SaveErrno;

	SaveErrno = errno;
	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}
	p = msg_buf;
	pe = p + sizeof (msg_buf) - 1;
	if (fmt != NULL) {
		vsnprintf(p, Ptrdiff(pe, p), fmt, args);
		p += strlen(p);
	}
	va_end(args);

	/* Error number */
	if (SaveErrno != 0) {
		snprintf(p, Ptrdiff(pe, p), ": ");
		p += strlen(p);
		(void) StrFromErrno(SaveErrno, p, Ptrdiff(pe, p));
		p += strlen(p);
	}
	*p = '\0';

	if (!Daemon) {
		fprintf(stderr, "%s: %s\n", program_name, msg_buf);
		if (FsCfgOnly && Pid > 1 && Pid != getpid()) {
			sleep(5);
			kill(Pid, SIGKILL);
		}
		exit(EXIT_FATAL);
	}

	/*
	 * At this point, we have a serious configuration startup error.
	 * We don't even know how to access SAM's log file, so we use
	 * syslog directly.
	 */
	openlog("sam-fsd", LOG_NDELAY | LOG_CONS | LOG_PID, LOG_DAEMON);
	errno = 0;
	syslog(LOG_ALERT, "Fatal error - %s", msg_buf);
	if (moreInfo) {
		syslog(LOG_ALERT,
		    "  Run /usr/lib/fs/samfs/sam-fsd for details.");
	}
	syslog(LOG_ALERT,
	    "  You must run 'samd config' after the problem has been "
	    "corrected.");

	/*
	 * In case SAM's log file does exist.
	 */
	sam_syslog(LOG_ALERT, "Fatal error - %s", msg_buf);
	if (moreInfo) {
		sam_syslog(LOG_ALERT,
		    "  Run /usr/lib/fs/samfs/sam-fsd for details.");
	}
	sam_syslog(LOG_ALERT,
	    "  You must run 'samd config' after the problem has been "
	    "corrected.");

	if (reconfig) {
		longjmp(errReturn, 1);
	}

	/*
	 * Wait for restart.
	 * If we exit, 'init' will simply respawn us.
	 * Wait for someone else to kill us.
	 */
	for (;;) {
		/*
		 * Wait for signals
		 */
		processSignals(TRUE);
		sleep(5);
		processSignals(FALSE);

		SenseRestart();
	}
}


/*
 * Process configuration file processing message.
 */
void
ConfigFileMsg(
	char *msg,
	int lineno,
	char *line)
{
	if (line != NULL) {
		/*
		 * While reading the file.
		 */
		static boolean_t line_printed = FALSE;

		if (msg != NULL) {
			/*
			 * Error message.
			 */
			if (!line_printed) {
				printf("%d: %s\n", lineno, line);
			}
			fprintf(stderr, " *** %s\n", msg);
		} else if (Verbose) {
			/*
			 * Informational messages.
			 */
			if (lineno == 0) {
				printf("%s\n", line);
			} else {
				printf("%d: %s\n", lineno, line);
				line_printed = TRUE;
				return;
			}
		}
		line_printed = FALSE;
	} else if (lineno >= 0 || fileRequired) {
		fprintf(stderr, "%s\n", msg);
	}
}


/*
 * Block or unblock signals as requested.
 */
void
processSignals(int allow)
{
	if (allow) {
		if (sigprocmask(SIG_UNBLOCK, &sig_hold, NULL) != 0) {
			Trace(TR_MISC,
			    "sigprocmask(SIG_UNBLOCK) failed (errno = %d)",
			    errno);
		}
	} else {
		if (sigprocmask(SIG_BLOCK, &sig_hold, NULL) != 0) {
			Trace(TR_MISC,
			    "sigprocmask(SIG_BLOCK) failed (errno = %d)",
			    errno);
		}
	}
}


/*
 * Start a child process.
 * The process is entered into the child table.
 * Then started in checkChildren().
 */
void
StartProcess(
	int	argc,				/* Number of args */
	char *argv[],			/* Array of args */
	int flags,
	int tid)
{
	struct ChildProcEntry *cpNew;
	char	*arg[5];
	int		i;

	for (i = 1; i < argc; i++) {
		if (argv[i] != NULL) {
			arg[i] = argv[i];
		} else {
			arg[i] = "";
		}
	}
	if (!Daemon && !FsCfgOnly) {
		/* Would start %s(%s)\n */
		printf(GetCustMsg(17292), argv[0], arg[1]);
		return;
	}

	cpNew = NULL;
	for (i = 0; i < childProcTable->count; i++) {
		struct ChildProcEntry *cp;

		cp = &childProcTable->entry[i];
		if (*cp->CpName == '\0') {
			cpNew = cp;
		} else if (strcmp(cp->CpName, argv[0]) == 0 &&
		    strcmp(cp->CpFsname, arg[1]) == 0) {
			/*
			 * Already have one of these.
			 */
			cp->CpFlags = flags;
			if (cp->CpPid == 0 || cp->CpPid == (pid_t)-1) {
				/*
				 * Not running; restart
				 */
				cp->CpPid = 0;
				cp->CpRestartTime = time(NULL);
				checkChildren();
			}
			return;
		}
	}
	Trace(TR_MISC, "Adding process %s(%s)", argv[0], arg);
	if (cpNew == NULL) {	/* Increase childProc->entry */
		/*
		 * Small trick here - childProcTable is always 'one entry
		 * ahead'.
		 */
		SamRealloc(childProcTable, sizeof (struct ChildProc) +
		    childProcTable->count * sizeof (struct ChildProcEntry));
		SamRealloc(deadPool, sizeof (struct ChildProc) +
		    deadPool->count * sizeof (struct ChildProcEntry));
		cpNew = &childProcTable->entry[childProcTable->count];
		childProcTable->count++;
		memset(&deadPool->entry[deadPool->count], 0,
		    sizeof (struct ChildProcEntry));
		deadPool->count++;
	}
	memset(cpNew, 0, sizeof (struct ChildProcEntry));
	strncpy(cpNew->CpName, argv[0], sizeof (cpNew->CpName));
	strncpy(cpNew->CpFsname, arg[1], sizeof (cpNew->CpFsname));
	if (argc > 2) {
		strncpy(cpNew->CpParam1, arg[2], sizeof (cpNew->CpParam1));
	}
	if (argc > 3) {
		strncpy(cpNew->CpParam2, arg[3], sizeof (cpNew->CpParam2));
	}
	cpNew->CpArgc = argc;
	cpNew->CpFlags = flags;
	cpNew->CpTid = tid;
	if (ready || FsCfgOnly) {
		checkChildren();
	}
}


/*
 * Sense a restart.
 */
void
SenseRestart(void)
{
	if (stopSignal != 0) {
		exit(EXIT_SUCCESS);
	}
	if (reconfig) {
		longjmp(errReturn, 1);
	}
}


/*
 * Stop a child process.
 * The child process is terminated.
 * The process is removed from the child table.
 */
void
StopProcess(char *argv[], boolean_t erase, int sig)
{
	struct ChildProcEntry *cp;
	struct ChildProcEntry *dcp;
	char	*arg;
	int		n;

	if (argv[1] != NULL) {
		arg = argv[1];
	} else {
		arg = "";
	}
	if (!Daemon && !FsCfgOnly) {
		/* Would stop %s(%s)\n */
		printf(GetCustMsg(17293), argv[0], arg);
		return;
	}
	for (n = 0; n < childProcTable->count; n++) {
		cp = &childProcTable->entry[n];
		if (strcmp(cp->CpName, argv[0]) == 0 &&
		    strcmp(cp->CpFsname, arg) == 0) {
			if (cp->CpPid != 0 && cp->CpPid != (pid_t)-1) {
				kill(cp->CpPid, sig);
			}
			cp->CpFlags &= ~CP_respawn;
			Trace(TR_MISC, "Stopped %s[%d](%s) sig: %d",
			    cp->CpName, (int)cp->CpPid, cp->CpFsname, sig);
			/*
			 * Used to mark the process as dead when signals
			 * are off.
			 */
			if (erase == TRUE) {
				dcp = &deadPool->entry[n];
				strcpy(dcp->CpName, cp->CpName);
				strcpy(dcp->CpFsname, cp->CpFsname);
				dcp->CpRestartTime = cp->CpRestartTime;
				dcp->CpFlags = cp->CpFlags;
				dcp->CpPid = cp->CpPid;
				cp->CpPid = (pid_t)-1;
			}
		}
	}
}


/* * *			 Private functions.			* * */


/*
 * sigInit -- initialize signal handling
 */
static void
sigInit(void (*handler)(int))
{

	/*
	 * Set process signal handling.
	 * SIGHUP to re-configure.
	 * SIGINT to stop.
	 * SIGALRM to wake up periodically.
	 * SIGTERM to stop.
	 * SIGUSR1 to start robots.
	 * SIGUSR2 to stop hsm.
	 * SIGCHLD for death of child processes.
	 *
	 * Register signals of interest.
	 */
	sig_action.sa_handler = handler;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	(void) sigaction(SIGHUP,  &sig_action, NULL);
	(void) sigaction(SIGINT,  &sig_action, NULL);
	(void) sigaction(SIGEMT,  &sig_action, NULL);
	(void) sigaction(SIGALRM, &sig_action, NULL);
	(void) sigaction(SIGTERM, &sig_action, NULL);
	(void) sigaction(SIGUSR1, &sig_action, NULL);
	(void) sigaction(SIGUSR2, &sig_action, NULL);

	/*
	 * Block signals for now.  Enable receipt
	 * only when we're ready to handle them.
	 */
	sigemptyset(&sig_hold);
	sigaddset(&sig_hold, SIGHUP);
	sigaddset(&sig_hold, SIGINT);
	sigaddset(&sig_hold, SIGEMT);
	sigaddset(&sig_hold, SIGALRM);
	sigaddset(&sig_hold, SIGTERM);
	sigaddset(&sig_hold, SIGUSR1);
	sigaddset(&sig_hold, SIGUSR2);
	sigaddset(&sig_hold, SIGCHLD);
	processSignals(FALSE);
}


/*
 * sigReset -- reset signal defaults and enable delivery
 *
 * Called by child procs after fork(), so that they
 * have default signal reception characteristics.
 */
void
sigReset(void)
{
	struct sigaction sig_dfl;

	sig_dfl.sa_handler = SIG_DFL;
	sigemptyset(&sig_dfl.sa_mask);
	sig_dfl.sa_flags = 0;
	(void) sigaction(SIGHUP,  &sig_dfl, NULL);
	(void) sigaction(SIGINT,  &sig_dfl, NULL);
	(void) sigaction(SIGEMT,  &sig_dfl, NULL);
	(void) sigaction(SIGALRM, &sig_dfl, NULL);
	(void) sigaction(SIGTERM, &sig_dfl, NULL);
	(void) sigaction(SIGUSR1, &sig_dfl, NULL);
	(void) sigaction(SIGUSR2, &sig_dfl, NULL);
	(void) sigaction(SIGCHLD, &sig_dfl, NULL);

	processSignals(TRUE);
}


/*
 * nullCatcher -- do nothing
 */
/*ARGSUSED*/
static void
nullCatcher(int sig)
{
}


/*
 *	catchSignals.
 */
static void
catchSignals(int sig)
{
	switch (sig) {
	case SIGCHLD: {
		pid_t	pid;
		int		stat;

		/*
		 * Process all that haven't been wait()ed for.
		 */
		while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
			/*
			 * Enter child status circular queue.
			 */
			childStatusQueue[csIn].CsPid = pid;
			childStatusQueue[csIn].CsStat = stat;
			csIn++;
			if (csIn >= csLimit) {
				csIn = 0;
			}
		}
		}
		break;

	case SIGHUP:
		reconfig = TRUE;
		break;

	case SIGUSR1:
		startAmld = TRUE;
		break;

	case SIGUSR2:
		stopHsm = TRUE;
		break;

	case SIGINT:
	case SIGEMT:
	case SIGTERM:
		stopSignal = SIGTERM;
		break;

	default:
		break;
	}
}


/*
 * Check children.
 * Any child process that exited are processed here.
 * Any processes that need started are handled here.
 */
static void
checkChildren(void)
{
	struct ChildProcEntry *cp;
	int		cn;

	/*
	 * Process all queued SIGCHLDs.
	 */
	while (csOut != csIn) {
		pid_t	pid;
		int		stat;

		pid = childStatusQueue[csOut].CsPid;
		stat = childStatusQueue[csOut].CsStat;
		csOut++;
		if (csOut >= csLimit) {
			csOut = 0;
		}
		ASSERT(pid != 0);
		if (0 == pid) {
			continue;
		}

		/*
		 * Look up child.
		 */
		for (cn = 0; cn < childProcTable->count; cn++) {
			cp = &childProcTable->entry[cn];
			if (cp->CpPid == pid) {
				break;
			}
			cp = &deadPool->entry[cn];
			if (cp->CpPid == pid) {
				break;
			}
		}
		if (cn >= childProcTable->count) {
			/*
			 * This is not a known child.
			 */
			Trace(TR_MISC, "Unknown child - pid %d, stat %04x",
			    (int)pid, stat);
			continue;
		}


		/*
		 * Note if the completion status was not good.
		 */
		cp->CpPid = 0;
		if (stat != 0) {
			SendCustMsg(HERE, 4027, cp->CpName, pid,
			    stat & 0xffff);
		} else {
			Trace(TR_MISC, "%s[%d] exited", cp->CpName, (int)pid);
		}
		if (strcmp(cp->CpName, TRACE_ROTATE) == 0) {
			if (tb != NULL) {
				int tid = cp->CpTid;

				if ((tid >= 0) && (tid < TI_MAX)) {
					tb->entry[tid].TrFsdPid = Pid;
				}
			}
#ifdef	sun
		} else if (strcmp(cp->CpName, SAM_RELEASER) == 0) {
			/*
			 * Notify the file system that the releaser exited.
			 */
			clearReleaser(cp->CpFsname);
		} else if (strcmp(cp->CpName, SAM_STAGER) == 0) {
			if (!(cp->CpFlags & CP_respawn) &&
			    (hsmsRunning == 0)) {
				stopAmld = TRUE;
			}
#endif
		}

		/*
		 * Set up restart conditions.
		 *
		 * Note that cp can point into either the child table or the
		 * dead table.  Either way, things work out.  If we happen
		 * to be tweaking the deadPool entry below, nothing is hurt,
		 * but we won't respawn, either.  (The start code only looks
		 * through childProcTable.)
		 */
		if (!(cp->CpFlags & CP_respawn)) {
			memset(cp, 0, sizeof (*cp));
		} else {
			/*
			 * Restart conditions are based on exit status.
			 */
			switch (stat / 256) {
			case EXIT_USAGE:
			case EXIT_NORESTART:
				cp->CpFlags |= CP_norestart;
				SendCustMsg(HERE, 4049, cp->CpName, pid);
				break;

			case EXIT_NOMEM:
				Trace(TR_MISC,
				    "Restarting %s in 60 seconds.", cp->CpName);
				cp->CpRestartTime = time(NULL) + 60;
				break;

			case EXIT_FAILURE:
			case EXIT_FATAL:
			default: {
				time_t		now;
				int			delay;

				delay = ((stat & 0xff) == SIGHUP) ? 1 : 10;
				now = time(NULL);
				if (cp->CpFlags & CP_qstart) {
					cp->CpRestartTime =
					    MAX(now, cp->CpRestartTime + delay);
				} else {
					cp->CpRestartTime = now + delay;
				}
				Trace(TR_MISC, "Restarting %s in %ld seconds.",
				    cp->CpName, cp->CpRestartTime - now);
				break;
				}
			}
		}
	}

	/*
	 * Check status of known child processes.
	 * Restart any that need it.
	 */
	nextWakeup = SCHEDQUANT;
	for (cn = 0; cn < childProcTable->count; cn++) {
		cp = &childProcTable->entry[cn];
		if (cp->CpFlags & CP_norestart) {
			continue;
		}
		if (*cp->CpName != '\0' && cp->CpPid == 0) {
			if (time(NULL) >= cp->CpRestartTime) {
				static boolean_t exePathFirst = TRUE;
				struct stat sb;
				static upath_t path;
				char	*argv[6];
				int		i;

				/*
				 * Time to restart child.
				 *
				 * Assure that our executable directory is
				 * accessible.
				 */
				if (stat(SAM_EXECUTE_PATH, &sb) != 0) {
					if (exePathFirst) {
						exePathFirst = FALSE;
						Trace(TR_MISC,
						    "Path "SAM_EXECUTE_PATH
						    " not found: %s",
						    StrFromErrno(errno, NULL,
						    0));
					}
					break;
				}
				exePathFirst = TRUE;
				argv[0] = cp->CpName;
				argv[1] = cp->CpFsname;
				i = 2;
				if (cp->CpArgc > 2) {
					argv[i] = cp->CpParam1;
					i++;
				}
				if (cp->CpArgc > 3) {
					argv[i] = cp->CpParam2;
					i++;
				}
				if (FsCfgOnly) {
					argv[i] = CONFIG;
					argv[i+1] = NULL;
				} else {
					argv[i] = NULL;
				}

				/*
				 * Set non-standard files to close on exec.
				 */
				for (i = STDERR_FILENO + 1; i < OPEN_MAX; i++) {
					(void) fcntl(i, F_SETFD, FD_CLOEXEC);
				}
				snprintf(path, sizeof (path), "%s/%s",
				    SAM_EXECUTE_PATH, argv[0]);

				if ((cp->CpPid = fork1()) < 0) {
					LibFatal(fork1, argv[0]);
					/*NOTREACHED*/
				}

				if (cp->CpPid == 0) {	/* Child. */
					sigReset();
					execv(path, argv);
					TracePid = getpid();
					ErrorExitStatus = EXIT_NORESTART;
					LibFatal(exec, path);
					/*NOTREACHED*/
				}

				/* Parent. */
				Trace(TR_MISC, "Started %s[%d](%s%s)",
				    argv[0], (int)cp->CpPid,
				    argv[1], (argv[2] ? " mcf" : ""));
			} else {
				/*
				 * Not yet time to restart child.  Reset
				 * nextWakeup if its restart is due before
				 * the next expected wakeup.
				 */
				nextWakeup = MIN(nextWakeup,
				    cp->CpRestartTime - time(NULL));
				nextWakeup = MAX(nextWakeup, 1);
				Trace(TR_MISC,
				    "%d seconds to next restart (%s)",
				    nextWakeup, cp->CpName);
			}
		}
	}
}


#define	SYSCHK "/etc/opt/SUNWsamfs/startup/samsyschk"
/*
 * Check Solaris release.
 */
static void
checkRelease(void)
{
#ifdef sun
	char	*systemchk = SYSCHK;
#if defined(SOL5_11)
	char *rel = "5.11";
#elif defined(SOL5_10)
	char *rel = "5.10";
#else
	char *rel = "??";
#endif
	char buf[32];

	(void) sysinfo(SI_RELEASE, buf, sizeof (buf));
	if (strcmp(buf, rel) != 0) {
		/* Solaris release mismatch - is %s, should be %s */
		FatalError(17236, buf, rel);
	}
	/*
	 * The following call to the script samsyschk is done to validate
	 * the system setup prior to starting samfs.  If samfs was installed
	 * using an alternate root or from a remote server, many of the
	 * needed checks couldn't be done then.  The script determines
	 * if a non-live install was done, and if so, finishes the install
	 * by cleaning up, configuring, and starting the required items.
	 */
	if (open("/tmp/.samsyschk", O_RDONLY) == -1) {
		if (system(systemchk)) {
			LibFatal(samsyschk, "");
		}
		if (open("/tmp/.samsyschk", O_RDONLY | O_CREAT) == -1) {
			LibFatal(open, "/tmp/.samsyschk");
		}
	}
#endif /* sun */

#ifdef linux
	struct new_utsname up;
	char *rel;
	char *cp;
	int val;

	/* Get system info */
	if (uname(&up) != 0) {
		goto mismatch;
	}

	rel = up.release;

	/* Compare major release value. */
	if ((cp = strchr(rel, '.')) == NULL) {
		goto mismatch;
	}
	*cp = '\0';
	val = atoi(rel);
	*cp = '.';
	if (val > MIN_REL_MAJOR) {
		return;			/* Good */
	}
	if (val < MIN_REL_MAJOR) {
		goto mismatch;
	}
	rel = ++cp;

	/* Compare minor release value. */
	if ((cp = strchr(rel, '.')) == NULL) {
		goto mismatch;
	}
	*cp = '\0';
	val = atoi(rel);
	*cp = '.';
	if (val > MIN_REL_MINOR) {
		return;			/* Good */
	}
	if (val < MIN_REL_MINOR) {
		goto mismatch;
	}
	rel = ++cp;

	/* Compare update release value.  First non-digit terminates value. */
	while (*cp != '\0') {
		if ((*cp < '0') || (*cp > '9')) {
			*cp = '\0';
			break;
		}
		cp++;
	}
	val = atoi(rel);
	if (val >= MIN_REL_UPDATE) {	/* Good */
		return;
	}

mismatch:
	fprintf(stderr,
	    "SAM-QFS: sam-fsd requires minimum system release %d.%d.%d\n",
	    MIN_REL_MAJOR, MIN_REL_MINOR, MIN_REL_UPDATE);
	exit(EXIT_FATAL);
#endif /* linux */
}


/*
 * Check trace files.
 */
static void
checkTraceFiles(void)
{
	time_t	now;
	int		tid;

	if (!Daemon) {
		return;
	}
	while (tb == NULL) {
		if ((tb = MapFileAttach(TRACECTL_BIN, TRACECTL_MAGIC, O_RDWR))
		    == NULL) {
			MakeTraceCtl();
		}
	}

	/*
	 * Check each trace file.
	 */
	now = time(NULL);
	for (tid = 0; tid < TI_MAX; tid++) {
		struct TraceCtlEntry *tc;
		struct stat buf;

		tc = &tb->entry[tid];
		if (*tc->TrFname == '\0') {
			continue;
		}
		if (stat(tc->TrFname, &buf) == 0) {
			/*
			 * Trace file found.
			 * Check size and age.
			 */
			boolean_t rotate = FALSE;

			if (tc->TrRotTime == 0) {
				tc->TrRotTime = now;
			}
			if (tc->TrAge > 0 &&
			    (now - tc->TrRotTime) > tc->TrAge) {
				rotate = TRUE;
			}
			if (tc->TrSize > 0 &&
			    (buf.st_size - tc->TrRotSize) > tc->TrSize) {
				rotate = TRUE;
			}
			if (rotate) {
				/*
				 * Set these now in case TRACE_ROTATE fails
				 * to do anything.
				 * Otherwise, we'll just try to rotate
				 * every SCHEDQUANT seconds.
				 */
				tc->TrRotTime = now;
				tc->TrRotSize = buf.st_size;
			}
			if (rotate && stat(SAM_EXECUTE_PATH"/"TRACE_ROTATE,
			    &buf) == 0) {
				char	*argv[3];

				argv[0] = TRACE_ROTATE;
				argv[1] = tc->TrFname;
				argv[2] = NULL;
				StartProcess(2, argv, CP_nosignal, tid);
				Trace(TR_MISC, "Rotate %s", tc->TrFname);
			}
		} else if (errno == ENOENT || errno == ENOTDIR) {
			/*
			 * Trace file not found.
			 * Start a new one.
			 */
			static upath_t name;
			FILE	*st;
			char	*p;

			/*
			 * Assure that the path exists.
			 */
			strncpy(name, tc->TrFname, sizeof (name));
			if ((p = strrchr(name, '/')) != NULL) {
				*p = '\0';
				MakeDir(name);
			}
			if ((st = fopen64(tc->TrFname, "w")) != NULL) {
				struct tm tm;
				char	dttm[32];
				char	*tdformat;

				/* Date and time. */
				if (tc->TrFlags & (1 << TR_date)) {
					tdformat = "%Y-%m-%d %T";
				} else {
					tdformat = "%H:%M:%S";
				}
				strftime(dttm, sizeof (dttm)-1, tdformat,
				    localtime_r(&now, &tm));
				fprintf(st, "%s %s[%d] %s trace log begun.\n",
				    dttm, program_name,
				    (int)getpid(), traceIdNames[tid]);
				fclose(st);
				tc->TrChange++;
				tc->TrCurSize = 0;
				tc->TrRotTime = now;
				tc->TrRotSize = 0;
			}
		} else if (errno == EINTR) {
			Trace(TR_MISC, "EINTR trace stat %s", tc->TrFname);
		}
	}
}

/*
 * Clear releaser running status for this file system.
 */
static void
clearReleaser(char *fsname)
{
	/*
	 * Notify the file system that the releaser exited.
	 */
	struct sam_setARSstatus_arg args;

	strncpy(args.as_name, fsname, sizeof (args.as_name));
	args.as_status = ARS_clr_r;
	if (sam_syscall(SC_setARSstatus, (void *)&args, sizeof (args)) < 0) {
		Trace(TR_ERR, "SC_setARSstatus(%s) failed", fsname);
	}
}

/*
 * Configure (reconfigure).
 * Read configuration files, configure file systems.
 * Write tables.
 */
static void
configure(
	char *defaults_name,
	char *diskvols_name,
	char *fscfg_name)
{
	fileRequired = TRUE;
	ReadMcf(McfName);
	fileRequired = (defaults_name != NULL);
	ReadDefaults(defaults_name);
#ifdef sun
	fileRequired = (diskvols_name != NULL);
	ReadDiskVolumes(diskvols_name);
#endif /* sun */
	fileRequired = (fscfg_name != NULL);
	FsConfig(fscfg_name);
	if (FsCfgOnly) {
		return;
	}
	WriteTraceCtl();
	checkTraceFiles();

	if (Daemon) {
		WriteDefaults();
	}

	if (QfsOnly) {
		return;
	}

#ifdef sun
	/*
	 * Count mounted file systems.
	 */
	countMountedFS();
#endif /* sun */

	if (Daemon) {
		/*
		 * Write the configuration binary files.
		 * And notify all running processes of reconfiguration.
		 */
		WriteMcfbin(DeviceNumof, DeviceTable);
#ifdef sun
		if (mountedHsmFs > 0) {
			WriteDiskVolumes(reconfig);
		}
#endif /* sun */
		signalProc(SIGHUP, NULL, NULL);
	}
#ifdef sun
	startStopHsm(0, "");
#endif /* sun */
}


/*
 * issueResellersMessage - issue any message the reseller wants.
 */
static void
issueResellersMessage(void)
{
#if 0
	char	**msg;
	void	*handle;

	if ((handle = dlopen(RESELL_LIB, RTLD_NOW | RTLD_GLOBAL)) == NULL) {
		Trace(TR_MISC, "No reseller library installed: %s.",
		    dlerror());
		return;
	}
	if ((msg = (char **)dlsym(handle, "resellers_message")) == NULL) {
		Trace(TR_MISC, "No reseller message: %s.", dlerror());
	} else {
		sam_syslog(LOG_INFO, "%s", *msg);
	}
	dlclose(handle);
#endif
}


/*
 * sendSyslogMsg - Log filesystem message in syslog.
 */
static void
sendSyslogMsg(struct sam_fsd_syslog *args)
{
	char	msgbuf[MAX_MSGBUF_SIZE];

	switch (args->slerror) {

	case E_NOSPACE: {
		char *space_reason;

		if (args->param >= E_WAITING && args->param <= E_NOSPC) {
			space_reason = syslog_nospace_reasons[args->param];
		} else  space_reason = "";
		sam_syslog(syslog_errors[args->slerror].priority,
		    syslog_errors[args->slerror].message,
		    args->mnt_point,
		    space_reason);

		/* Send sysevent to generate SNMP trap */
		sprintf(msgbuf, GetCustMsg(164), args->mnt_point,
		    space_reason);
		PostEvent(FS_CLASS, "ENospace", 164,
		    syslog_errors[args->slerror].priority, msgbuf,
		    NOTIFY_AS_EMAIL | NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		}
		break;


	default: {		/* Valid and out-of-range errors alike */
		static char errno_msg[STR_FROM_ERRNO_BUF_SIZE];

		if (args->error == 0) {
			*errno_msg = '\0';
		} else {
			(void) StrFromErrno(args->error, errno_msg,
			    sizeof (errno_msg));
		}
		sam_syslog(syslog_errors[args->slerror].priority,
		    syslog_errors[args->slerror].message,
		    args->mnt_point,
		    args->id.ino, args->id.gen, args->param,
		    errno_msg);
		}
		break;
	}
}


/*
 * Signal process.
 */
static void
signalProc(
	int sig,
	char *pname,	/* Process name.  If NULL, all processes. */
	char *fsname)	/* File system name.  If NULL, all file systems. */
{
	struct ChildProcEntry *cp;
	int		i;

	for (i = 0; i < childProcTable->count; i++) {
		cp = &childProcTable->entry[i];
		if (cp->CpPid != 0 && cp->CpPid != (pid_t)-1 &&
		    !(cp->CpFlags & CP_nosignal) &&
		    (fsname == NULL || strcmp(fsname, cp->CpFsname) == 0) &&
		    (pname == NULL || strcmp(pname, cp->CpName) == 0)) {
			kill(cp->CpPid, sig);
#ifdef linux
			/* strsignal() cores on Linux.  Don't use for now. */
			Trace(TR_MISC, "Signaled %s[%d](%s): %d", cp->CpName,
			    (int)cp->CpPid, cp->CpFsname, sig);
#endif /* linux */
#ifdef sun
			Trace(TR_MISC, "Signaled %s[%d](%s): %s", cp->CpName,
			    (int)cp->CpPid, cp->CpFsname, strsignal(sig));
#endif /* sun */
		}
		if (sig == SIGHUP) {
			/*
			 * Allow restart.
			 */
			cp->CpFlags &= ~CP_norestart;
		}
	}
}


#ifdef sun
/*
 * Report logging state change errors.
 */
static void
reportlogerror(int ret, char *mp, char *special, char *cmd, fiolog_t *flp)
{
	/* No error */
	if ((ret != -1) && (flp->error == FIOLOG_ENONE)) {
		return;
	}

	/* logging was not enabled/disabled */
	if (ret == -1 || flp->error != FIOLOG_ENONE) {
		Trace(TR_MISC, "Could not %s logging for %s on %s.\n",
		    cmd, special, mp);
	}

	/* ioctl returned error */
	if (ret == -1) {
		return;
	}

	/* Some more info */
	switch (flp->error) {
	case FIOLOG_ENONE :
		if (flp->nbytes_requested &&
		    (flp->nbytes_requested != flp->nbytes_actual)) {
			Trace(TR_MISC,
			    "The log has been resized from %d bytes "
			    "to %d bytes.\n",
			    flp->nbytes_requested,
			    flp->nbytes_actual);
		}
		return;
	case FIOLOG_ENOTSUP:
		Trace(TR_MISC,
		    "NOTE: File system version does not support logging.\n");
		break;
	case FIOLOG_ETRANS :
		Trace(TR_MISC,
		    "File system logging is already enabled.\n");
		break;
	case FIOLOG_EULOCK :
		Trace(TR_MISC, "File system is locked.\n");
		break;
	case FIOLOG_EWLOCK :
		Trace(TR_MISC, "The file system could not be write locked.\n");
		break;
	case FIOLOG_ECLEAN :
		Trace(TR_MISC, "The file system may not be stable.");
		Trace(TR_MISC, "Please see samfsck(1M).\n");
		break;
	case FIOLOG_ENOULOCK :
		Trace(TR_MISC, "The file system could not be unlocked.\n");
		break;
	default :
		Trace(TR_MISC, "Unrecognized error code (%d).\n", flp->error);
		break;
	}
}


/*
 * Check logging state - return non-zero if logging is enabled.
 */
static int
checkislog(char *mp)
{
	int fd;
	uint32_t islog;

	fd = open(mp, O_RDONLY);
	islog = 0;
	(void) ioctl(fd, _FIOISLOG, &islog);
	(void) close(fd);
	return ((int)islog);
}


/*
 * Enable logging.
 */
static void
enable_logging(char *mp, char *special)
{
	int fd, ret, islog;
	fiolog_t fl;
	uint_t debug_level = 0x2ff;

	fd = open(mp, O_RDONLY);
	if (fd == -1) {
		Trace(TR_MISC,
		    "Error while enabling logging for %s on %s (%s) - "
		    "Logging state is unknown.", special, mp, strerror(errno));
		return;
	}
	fl.nbytes_requested = 0;
	fl.nbytes_actual = 0;
	fl.error = FIOLOG_ENONE;
	ret = ioctl(fd, _FIOLOGENABLE, &fl);
	if (ret == -1) {
		Trace(TR_MISC, "Error while enabling logging for %s on %s (%s)",
		    special, mp, strerror(errno));
	}
	(void) close(fd);

	/* is logging enabled? */
	islog = checkislog(mp);

	/* report errors, if any */
	if (ret == -1 || !islog) {
		reportlogerror(ret, mp, special, "enable", &fl);
	}
out:
	Trace(TR_MISC, "Logging state is %s for %s on %s.",
	    islog ? "enabled" : "disabled", special, mp);
}


static void
disable_logging(char *mp, char *special)
{
	int fd, ret, islog;
	fiolog_t fl;

	fd = open(mp, O_RDONLY);
	if (fd == -1) {
		Trace(TR_MISC,
		    "Error while disabling logging for %s on %s (%s) - "
		    "Logging state is unknown.", special, mp, strerror(errno));
		return;
	}
	fl.error = FIOLOG_ENONE;
	ret = ioctl(fd, _FIOLOGDISABLE, &fl);
	if (ret == -1) {
		Trace(TR_MISC,
		    "Error while disabling logging for %s on %s (%s)",
		    special, mp, strerror(errno));
	}
	(void) close(fd);

	/* is logging enabled? */
	islog = checkislog(mp);

	/* report errors, if any */
	if (ret == -1 || islog) {
		reportlogerror(ret, mp, special, "disable", &fl);
	}
out:
	Trace(TR_MISC, "Logging state is %s for %s on %s",
	    islog ? "enabled" : "disabled", special, mp);
}

/*
 * Initialize logging (or not) for a newly-mounted filesystem.
 */
static void
initLogging(char *fsname)
{
	struct sam_fs_info fi;

	if (GetFsInfo(fsname, &fi) >= 0) {
		if ((fi.fi_status & FS_CLIENT) == 0) {
			Trace(TR_MISC,
			    "This host is the logging MDS for %s", fsname);
			if ((fi.fi_mflag & MS_RDONLY) == 0) {
				if (fi.fi_config1 & MC_LOGGING) {
					enable_logging(fi.fi_mnt_point,
					    fsname);
				} else {
					disable_logging(fi.fi_mnt_point,
					    fsname);
				}
			} else {
				Trace(TR_MISC,
				    "%s is mounted read-only.", fsname);
			}
		} else {
			Trace(TR_MISC,
			    "This host is not the logging MDS for %s", fsname);
		}
	}
}
#endif /* sun */


/*
 * Send filesystem mount message to interested daemons.
 */
static void
sendFsMount(char *fsname)
{
#if !defined(QFS)
	ArchiverFsMount(fsname);
	StagerFsMount(fsname);
#endif /* !defined(QFS) */
}


/*
 * Send filesystem umount message to interested daemons.
 */
static void
sendFsUmount(
char *fsname)
{
#if !defined(QFS)
	ArchiverFsUmount(fsname);
	StagerFsUmount(fsname);
#endif /* !defined(QFS) */
}


/*
 * Set /etc/inittab.
 */
void
setInittab(void)
{
	static char *samFsd = SAM_SAMFS_PATH"/"SAM_FSD;
	static char line[256];
	struct stat buf;
	FILE	*st;
	FILE	*stBak;
	upath_t	initBakName;
	char	*initName = "/etc/inittab";
	int		l = strlen(samFsd);

	/*
	 * Scan the "/etc/inittab" file to find our sam-fsd line.
	 */
	if ((st = fopen(initName, "r")) == NULL) {
		/* Cannot open %s */
		FatalError(613, initName);
	}

	while (fgets(line, sizeof (line)-1, st) != NULL) {
		char	*p;

		for (p = line; *p != '\0'; p++) {
			if (strncmp(p, samFsd, l) == 0) {
				/*
				 * daemon already in file.
				 */
				fclose(st);
				return;
			}
		}
	}
	fclose(st);

	/*
	 * Not in "/etc/inittab" - make backup copy, and add the
	 * line.
	 */
	snprintf(initBakName, sizeof (initBakName), "%s.SUNWsamfs", initName);
	if (stat(initBakName, &buf) == -1) {
		/*
		 * Backup does not exist.
		 */
		stBak = fopen(initBakName, "w");
		if (stBak == NULL) {
			/* Cannot create %s */
			FatalError(574, initBakName);
		}
	} else {
		/*
		 * Backup exists.
		 */
		stBak = NULL;
	}
	if ((st = fopen(initName, "r")) == NULL) {
		/* Cannot open %s */
		FatalError(613, initName);
	}
	while (fgets(line, sizeof (line)-1, st) != NULL) {
		if (stBak != NULL) {
			fputs(line, stBak);
		}
	}
	fsync(fileno(st));
	fclose(st);
	if (stBak != NULL) {
		Trace(TR_MISC, "%s backed up to %s", initName, initBakName);
		fclose(stBak);
	}

	if ((st = fopen(initName, "a")) == NULL) {
		/* Cannot open %s */
		FatalError(613, initName);
	}
	fprintf(st, "sf:023456:respawn:%s\n", samFsd);
	fsync(fileno(st));
	fclose(st);
	sync();

	Trace(TR_MISC, "sam-fsd respawn line added to %s", initName);
	kill(1, SIGHUP);
}


/*
 * ----	StartShareDaemon
 *
 * Kernel has called us to tell us that this file system is
 * about to be mounted.  A share daemon may or may not be
 * running already.
 */
void
StartShareDaemon(char *fs)
{
	char *argv[3] = { SAM_SHAREFSD, fs, NULL };

	StartProcess(2, argv, CP_respawn | CP_qstart, 0);
}


#ifdef sun

/*
 * Count mounted QFS and SAM-QFS file systems.
 */
static void
countMountedFS(void)
{
	int i;

	mountedFs = 0;
	mountedHsmFs = 0;
	for (i = 0; i < FileSysNumof; i++) {
		struct sam_fs_info fi;
		struct sam_mount_info *mi;

		mi = &FileSysTable[i];
		if (GetFsInfo(mi->params.fi_name, &fi) != -1) {
			if (fi.fi_status & FS_MOUNTED) {
				mountedFs++;
				if ((fi.fi_config & MT_SAM_ENABLED) &&
				    ((fi.fi_status & FS_CLIENT) == 0)) {
					mountedHsmFs++;
					/*
					 * In case a previous fsd died with
					 * a releaser running,
					 * clear the releaser running bit
					 * for this fs.
					 */
					if (!reconfig &&
					    fi.fi_status & FS_RELEASING) {
						clearReleaser(
						    mi->params.fi_name);
					}
				}
			}
		}
	}
}


/*
 * startStopHsm -- Start/stop HSM daemons.
 *
 * Includes: SAM_ARCHIVER, SAM_STAGEALL, SAM_STAGER, SAM_AMLD.
 * When we stop, we only stop SAM_ARCHIVER, SAM_STAGEALL, and SAM_STAGER.
 */
static void
startStopHsm(int cmd, char *fs)
{
	char	*argv[2];
	int		i, hsms;

	if (FsCfgOnly) {
		return;
	}

	/*
	 * Count mounted HSM file systems the local host must archive/stage.
	 * If we're being called for an unmount, don't count the named FS.
	 * (It's committed to the unmount, even tho' its status still includes
	 * FS_MOUNTED and FS_UMOUNT_IN_PROGESS).
	 */
	hsms = 0;
	for (i = 0; i < FileSysNumof; i++) {
		struct sam_fs_info fi;
		struct sam_mount_info *mi;

		mi = &FileSysTable[i];
		if ((cmd != FSD_umount && cmd != FSD_stop_sam) ||
		    strcmp(fs, mi->params.fi_name) != 0) {
			if (GetFsInfo(mi->params.fi_name, &fi) != -1) {
				if ((fi.fi_status &
				    (FS_MOUNTED | FS_CLIENT)) == FS_MOUNTED &&
				    (fi.fi_config & MT_SAM_ENABLED)) {
					hsms++;
				}
			}
		}
	}

	if (hsmsRunning && (hsms == 0 || cmd == FSD_stop_sam)) {
		if (Daemon) {
			Trace(TR_MISC,
			    "Stop archive/staging: %d remote servers, "
			    "%d mounted HSM FSes",
			    SamRemoteServerCount, hsms);
		}
		argv[1] = NULL;
		for (i = 0; hsmDaemons[i] != NULL; i++) {
			argv[0] = hsmDaemons[i];
			if (cmd == FSD_stop_sam &&
			    strcmp(argv[0], SAM_STAGER) == 0) {
				StopProcess(argv, FALSE, SIGUSR1);
			} else {
				StopProcess(argv, FALSE, SIGINT);
			}
		}
		hsmsRunning = 0;
	} else if (!hsmsRunning && hsms && ArchiveCount > 0 &&
	    (RmediaDeviceCount > 0 || DiskVolCount > 0)) {
		if (Daemon) {
			Trace(TR_MISC,
			    "Start archive/staging: %d remote servers, "
			    "%d mounted HSM FSes",
			    SamRemoteServerCount, hsms);

			/*
			 * Reread disk volume config to count diskVols.count.
			 */
			ReadDiskVolumes(NULL);
			WriteDiskVolumes(FALSE);
		}
		argv[1] = NULL;
		for (i = 0; hsmDaemons[i] != NULL; i++) {
			argv[0] = hsmDaemons[i];
			StartProcess(2, argv, CP_respawn, 0);
		}
		startSamamld(hsms + SamRemoteServerCount);
		hsmsRunning = 1;
	}
}


/*
 * Start/stop sam-amld.
 */
static void
startSamamld(int hsmfs)			/* Mounted HSM file systems */
{
	char	*argv[2];

	if (FsCfgOnly) {
		return;
	}
	argv[0] = SAM_AMLD;
	argv[1] = NULL;
	if (RmediaDeviceCount > 0 && hsmfs > 0) {
		StartProcess(2, argv, 0, 0);
	} else {
		StopProcess(argv, FALSE, SIGINT);
	}
}


/*
 * Start/stop remote file transfer daemon (old sam-ftpd)
 */
static void
startSamrftd(void)
{
	char	*argv[2];

	if (FsCfgOnly) {
		return;
	}
	argv[0] = SAM_RFT;
	argv[1] = NULL;
	if (SamRemoteServerCount > 0 || DiskVolClientCount > 0) {
		StartProcess(2, argv, CP_respawn, 0);
	} else {
		StopProcess(argv, FALSE, SIGINT);
	}
}

#endif /* sun */
