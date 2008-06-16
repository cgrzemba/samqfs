/*
 * sharefs.c - Shared file system client/server daemon.
 *
 *
 * sam-sharefsd - TCP/IP daemon which handles the file system to
 * file system connection.  sam-sharefsd is started by sam-fsd
 * if the file system is shared.  (Set in the mcf for the family
 * set.)
 *
 * sam-sharefsd is exec'd by sam-fsd when the mount parameters
 * are configured for the shared file system [SC_setmount].
 *
 * sam-sharefsd always has a main thread:
 *  . signalWaiter - on a server, it wakes every ten seconds to check
 *    for hosts file changes, and awaits signals.  A SIGHUP will cause
 *    it to check the hosts file immediately.  On a client, it just
 *    awaits signals.
 *
 * sam-sharefsd always has one client thread:
 *  . ClientRdSocket - this thread opens a TCP/IP connection to the
 *    sam-sharefsd daemon on the metadata server (using connect()),
 *    and holds this for the kernel.  This thread calls SetClientSocket()/
 *    SC_client_rdsock, where the kernel uses it to read data from
 *    the server.  (The kernel uses its own threads to write data.)
 *
 * sam-sharefsd on the metadata server has one server thread:
 *  . ServerListenerSocket - Waits in listen() for new client
 *    connections.  For each connection from a client sam-sharefsd,
 *    accept() is called, and a thread created.  It calls:
 *
 *  . serverRdSocket - Holds open a TCP/IP connection from a
 *    sam-sharefsd daemon on a client (via accept()), and hands
 *    this to the kernel.  This thread calls SetServerSocket()/
 *    SC_server_rdsock, where the kernel uses it to read data
 *    from the client.  (Again, the kernel uses its own threads
 *    to write data.)
 *
 *
 * So, on a client, there will be two threads:
 *  . the main thread (usu. in signalWaiter);
 *  . the ClientRdSocket thread (in SetClientSocket()/SC_client_rdsock);
 * The ClientRdSocket thread owns a TCP/IP connection endpoint,
 * connected to sam-sharefsd on the server.
 *
 * On a server, there will be a minimum of four threads:
 *  . the main thread (usu. in signalWaiter);
 *  . a ServerListenerSocket thread (in listen());
 *  . a serverRdSocket thread (in SetServerSocket()/SC_server_rdsock);
 *  . the ClientRdSocket thread (in SetClientSocket()/SC_client_rdsock);
 * and one TCP/IP connection connecting the endpoint of the serverRdSocket
 * thread to the ClientRdSocket thread endpoint.
 *
 * Plus, for each additional client, there is:
 *  . a serverRdSocket thread (in SetServerSocket()/SC_server_rdsock),
 *    that owns a TCP/IP connection endpoint to a client sam-sharefsd.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.68 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <syslog.h>
#include <unistd.h>
#ifdef linux
#include <string.h>
#endif

/* POSIX headers. */
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/syscall.h"
#include "sam/shareops.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "sam/exit.h"
#include "sam/spm.h"
#include "sam/quota.h"
#include "sblk.h"

#include "share.h"
#include "samhost.h"

/* Local headers. */
#define	DEC_INIT
#include "sharefs.h"
#ifdef METADATA_SERVER
#include "server.h"
#endif
#undef DEC_INIT

/*  External Functions */
extern int getHostName(char *host, int len, char *fs);
void * ClientRdSocket(void *host_data_t);
#ifdef METADATA_SERVER
void * ServerListenerSocket(void *s);
#endif

/*  Private Functions */
static void shutDown(void);
static void blockHangupSignals(char *);
static void blockSignalHandling(char *);
static void unBlockSignalHandling(char *);
static int signalWaiter(char *);
static void catchSig(int sig);
static void catchSigEMT(int sig);
static void ForceUmountDown(char *fs);


/*
 * ----- main
 * Get parameters passed by sam-fsd in the execl command:
 *  fs_name  - family set name
 *  CONFIG - configure only. Not started by persistent sam-fsd.
 */
int
main(int argc, char *argv[])
{
	pthread_attr_t attr;
	char fileName[64];
	uname_t fs_name;
	char	*traceName;
	int err, ret;

	program_name  = "sam-sharefsd";
	if (argc == 3) {
		FsCfgOnly = TRUE;
	} else if (argc != 2) {
		sam_syslog(LOG_INFO, "argc %d not valid\n", argc);
		exit(EXIT_NORESTART);
	}
	strncpy((char *)&fs_name, argv[1], sizeof (uname_t));

	sprintf(fileName, "%s/%s/%s", SAM_VARIABLE_PATH, SHAREFS_DIRNAME,
	    fs_name);
	MakeDir(fileName);
	if (chdir(fileName)) {
		sam_syslog(LOG_INFO, "chdir %s failed for fs %s\n",
		    fileName, fs_name);
		exit(EXIT_NORESTART);
	}

	sprintf(fileName, "shf-%s", fs_name);
	SamStrdup(traceName, fileName);
	TraceInit(traceName, TI_sharefs);
	Trace(TR_MISC, "FS %s: Shared file system daemon started%s",
	    fs_name, FsCfgOnly ? " - config only" : "");

	memset(&Host, 0, sizeof (host_data_t));
	Host.sockfd = -1;
	if ((err = getHostName((char *)&Host.hname, sizeof (Host.hname),
	    fs_name)) != HOST_NAME_EOK) {
		char path[MAXPATHLEN];

		/*
		 * Trace detailed host name error info
		 */
		snprintf(path, sizeof (path), "%s/nodename.%s", SAM_CONFIG_PATH,
		    fs_name);
		switch (err) {
			case HOST_NAME_EFAIL:
				break;
			case HOST_NAME_ETOOSHORT:
				Trace(TR_MISC,
				    "FS %s: gethostname(): name too short\n",
				    fs_name);
				break;
			case HOST_NAME_ETOOLONG:
				Trace(TR_MISC,
				    "FS %s: gethostname(): name too long\n",
				    fs_name);
				break;
			case HOST_NAME_EBADCHAR:
				Trace(TR_MISC,
				    "FS %s: gethostname(): bad character\n",
				    fs_name);
				break;
			case HOST_FILE_EREAD:
				Trace(TR_MISC,
				    "FS %s: Read of %s failed\n",
				    fs_name, path);
				break;
			case HOST_FILE_ETOOSHORT:
				Trace(TR_MISC,
				    "FS %s: Bad hostname in %s: name too "
				    "short\n", fs_name, path);
				break;
			case HOST_FILE_ETOOLONG:
				Trace(TR_MISC,
				    "FS %s: Bad hostname in %s: name too "
				    "long\n", fs_name, path);
				break;
			case HOST_FILE_EBADCHAR:
				Trace(TR_MISC,
				    "FS %s: Hostname has bad character in %s\n",
				    fs_name, path);
				break;
			default:
				break;
		}
		Trace(TR_MISC, "FS %s: Gethostname failed (FATAL)\n", fs_name);
		exit(EXIT_NORESTART);
	}

	Trace(TR_MISC, "FS %s: Host %s", fs_name, Host.hname);

	CustmsgInit(1, NULL);	/* Assure message translation */

	if (!FsCfgOnly) {
		sam_syslog(LOG_INFO, "Host %s daemon started for shared "
		    "fs %s.\n",
		    Host.hname, fs_name);
	}

	/*
	 * Defer SIGHUPs until we're ready to deal with them.
	 */
	blockHangupSignals(fs_name);


	/*
	 * Initialize the thread id for this Host.
	 */
	Host.main_tid = pthread_self();

	/*
	 * Kill off any pre-existing sam-sharefsd daemons for this FS.
	 * (Note that we've already chdir'ed into a private directory
	 * for this FS.)
	 */
	{
		int pred;
		int killsigs[] = { SIGTERM, SIGTERM, SIGKILL, SIGKILL, 0 };

		if ((pred = sam_lockout(SAM_SHAREFSD,
		    ".", SAM_SHAREFSD ".", killsigs)) != 0) {
			if (pred < 0) {
				Trace(TR_MISC,
				    "FS %s: Error registering self and "
				    "killing predecessors",
				    fs_name);
				exit(EXIT_FAILURE);
			} else {
				Trace(TR_MISC, "FS %s: Couldn't kill "
				    "predecessors", fs_name);
			}
		}
	}

	/*
	 * Open root/label devs to lock out fsck.
	 * We need to do that until the label has been
	 * written (if we do that).
	 */
	OpenDevs(fs_name);

	/*
	 * Get mount parameters.
	 */
	if (!ValidateFs(fs_name, &Host.fs)) {
		exit(EXIT_FAILURE);
	}

	/*
	 * Verify this host is valid and return the server host.
	 */
	ServerHost = 0;
	switch (GetServerInfo(fs_name)) {
	case CFG_SERVER:
		Trace(TR_MISC, "FS %s: Local host is server (%s)",
		    Host.fs.fi_name, Host.server);
#ifdef METADATA_SERVER
		ServerHost = 1;
		break;
#else
		errno = 0;
		SysError(HERE, "FS %s: Fatal configuration error", fs_name);
		exit(EXIT_NORESTART);
		/*NOTREACHED*/
#endif

	case CFG_HOST:
		Trace(TR_MISC, "FS %s: Host %s; server = %s",
		    Host.fs.fi_name, Host.hname, Host.server);
		break;

	case CFG_CLIENT:
		if (strcasecmp(Host.hname, Host.server) == 0) {
			errno = 0;
			SysError(HERE, "FS %s: Server configuration error",
			    Host.fs.fi_name);
			exit(EXIT_NORESTART);
		}
		Trace(TR_MISC, "FS %s: Client %s; server = %s",
		    Host.fs.fi_name, Host.hname, Host.server);
		break;

	case CFG_FATAL:
		errno = 0;
		SysError(HERE, "FS %s: Fatal configuration error", fs_name);
		exit(EXIT_NORESTART);
		/*NOTREACHED*/

	case CFG_ERROR:
	default:
		errno = 0;
		SysError(HERE, "FS %s: Couldn't determine server", fs_name);
		exit(EXIT_FAILURE);
		/*NOTREACHED*/
	}

	CloseDevs();

	/*
	 * Block all signal handling by all the threads except SIGEMT.
	 */
	blockSignalHandling(Host.fs.fi_name);

	/*
	 * Initialize threads.
	 */
	bzero((char *)&attr, sizeof (attr));
	if ((errno = pthread_attr_init(&attr))) {
		LibFatal(pthread_attr_init, "");
	}
	if ((errno = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))) {
		LibFatal(pthread_attr_setscope, "");
	}

#ifdef METADATA_SERVER
	if (ServerHost) {
		/*
		 * Notify file system that this host is the server.
		 */
		if (SetServer(Host.fs.fi_name,
		    Host.server, Host.serverord, Host.maxord)) {
			SysError(HERE, "FS %s: Setserver call returned error",
			    Host.fs.fi_name);
			exit(EXIT_FAILURE);
		}

		/*
		 * If this is the server in a clustered system, then let
		 * the kernel know about which hosts are required for early
		 * failover completion.
		 */
		SetClusterInfo(Host.fs.fi_name);

		/*
		 * Create the global server thread who listens for clients.
		 */
		if ((errno = pthread_create(&Host.listen_tid, &attr,
		    ServerListenerSocket, (void *)&Host)) != 0) {
			SysError(HERE,
			    "FS %s: pthread_create[ServerListenerSocket] "
			    "failed, server %s",
			    Host.fs.fi_name, Host.hname);
			exit(EXIT_FAILURE);
		}
	}
#endif

	/*
	 * Create ClientRdSocket thread. This thread is the LWP that reads
	 * packets over the socket in the file system from the shared server.
	 */
	if ((errno = pthread_create(&Host.sock_tid, &attr, ClientRdSocket,
	    (void *)&Host)) != 0) {
		SysError(HERE, "FS %s: pthread_create[ClientRdSocket] failed, "
		    "Host=%s",
		    Host.fs.fi_name, Host.hname);
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);

	/*
	 * This main thread unblocks signals and waits for signals.
	 * Every 10 seconds, sharefsd wakes up & checks for its parent.
	 * On return, it shuts down the two main threads.
	 */
	ret = signalWaiter(Host.fs.fi_name);
	shutDown();
	Trace(TR_MISC, "FS %s: Exiting, status = %d.", Host.fs.fi_name, ret);
	exit(ret);
}


/*
 * ----- shutDown
 * Shutdown this daemon.
 */
static void
shutDown(void)
{
	int fd;
	void *statusp;

	Trace(TR_MISC, "FS %s: Initiating shutdown.", Host.fs.fi_name);

#ifdef METADATA_SERVER
	if (ServerHost) {
		extern int service_fd;

		CloseReaderSockets();

		if ((errno = pthread_kill(Host.listen_tid, SIGEMT))) {
			if (errno == ESRCH) {
				Trace(TR_MISC,
				    "FS %s: "
				    "pthread_kill[ServerListenerSocket]: "
				    "thread already exited",
				    Host.fs.fi_name);
			} else {
				SysError(HERE,
				    "FS %s: "
				    "pthread_kill[ServerListenerSocket] "
				    "failed",
				    Host.fs.fi_name);
			}
		}
		if ((errno = pthread_join(Host.listen_tid, &statusp))) {
			SysError(HERE, "FS %s: "
			    "pthread_join[ServerListenerSocket] failed",
			    Host.fs.fi_name);
		}
		if (service_fd >= 0) {
			spm_unregister_service(service_fd);
		}
	}
#endif	/* METADATA_SERVER */

	/*
	 * Close socket, then kill and join server socket thread.
	 */
	if ((fd = Host.sockfd) != -1) {
		Host.sockfd = -1;
		(void) close(fd);
	}
	if ((errno = pthread_kill(Host.sock_tid, SIGEMT))) {
		if (errno == ESRCH) {
			Trace(TR_MISC,
			    "FS %s: pthread_kill[ClientRdSocket]: "
			    "thread already exited",
			    Host.fs.fi_name);
		} else {
			SysError(HERE, "FS %s: pthread_kill[ClientRdSocket] "
			    "failed",
			    Host.fs.fi_name);
		}
	}
	if ((errno = pthread_join(Host.sock_tid, &statusp))) {
		SysError(HERE, "FS %s: pthread_join[ClientRdSocket] failed",
		    Host.fs.fi_name);
	}
}


/*
 * -----	blockSignalHandling.  Block all signals except SIGEMT.
 * All threads inherit this signal mask from their creator (this main thread).
 * All threads have all signals masked except SIGEMT.
 * Otherwise a signal that arrives while the signalWaiter is not blocked
 * in sigwait might be delivered to another thread.
 */
static void
blockSignalHandling(char *fs_name)
{
	struct sigaction act;

	bzero((char *)&act, sizeof (act));
	act.sa_handler = catchSigEMT;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGEMT, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGEMT");
	}

	sigfillset(&act.sa_mask);
	sigdelset(&act.sa_mask, SIGEMT);

	errno = pthread_sigmask(SIG_BLOCK, &act.sa_mask, NULL);
	if (errno != 0) {
		LibFatal(pthread_sigmask, fs_name);
	}
}


/*
 * -----	unblockSignalHandling.  Unblock all interesting signals
 * other than SIGEMT.  The main thread calls this routine to enable
 * reception of signals after the other threads are forked off.
 */
static void
unBlockSignalHandling(char *fs_name)
{
	struct sigaction act;

	bzero((char *)&act, sizeof (act));
	act.sa_handler = catchSig;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGHUP);
	sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGALRM);
	sigaddset(&act.sa_mask, SIGTERM);
	sigaddset(&act.sa_mask, SIGCHLD);
	if (sigaction(SIGHUP, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGHUP");
	}
	if (sigaction(SIGINT, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGINT");
	}
	if (sigaction(SIGALRM, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGALRM");
	}
	if (sigaction(SIGTERM, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGTERM");
	}
	if (sigaction(SIGCHLD, &act, NULL) == -1) {
		LibFatal(sigaction, "SIGCHLD");
	}

	errno = pthread_sigmask(SIG_UNBLOCK, &act.sa_mask, NULL);
	if (errno != 0) {
		LibFatal(pthread_sigmask, fs_name);
	}
}


/*
 * During initialization, we need to defer processing
 * of HUP signals.  We can't allow them to kill us (since
 * they are there to notify us that the underlying FS or
 * its status may have changed in some way (e.g., the FS
 * has been mounted), and we also can't allow them to get
 * by, since we should re-scan the FS to see if the FS
 * status has changed.
 */
static void
blockHangupSignals(char *fs_name)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGHUP);

	errno = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (errno != 0) {
		LibFatal(pthread_sigmask, fs_name);
	}
}


/*
 * -----	signalWaiter.
 * The main thread calls this and waits for signals.
 */
static int
signalWaiter(char *fs_name)
{
	int ret = EXIT_SUCCESS;

	/*
	 * The main program waits for signals, and notifies the other
	 * threads of outside conditions.
	 */
	while (!ShutdownDaemon) {
		int e, r;

		if (!Conn2Srvr) {
			alarm(10);
		}
		unBlockSignalHandling(fs_name);
		/*
		 * If a signal gets in now (likely, since they were blocked up
		 * to this point), it might be a shutdown request from sam-fsd
		 * trying to reconfigure this FS.  Don't wait on the FS.
		 */
		r = -1;
		e = 0;
		if (!ShutdownDaemon) {
			r = sam_shareops(fs_name, SHARE_OP_AWAIT_WAKEUP, 0);
			e = errno;
		}
		blockSignalHandling(fs_name);
		if (r >= 0) {
			Trace(TR_MISC, "FS %s: Wakened from AWAIT_WAKEUP",
			    fs_name);
		} else {
			if (e == EIO) {
				Trace(TR_MISC, "FS %s: forced unmount",
				    Host.fs.fi_name);
				ForceUmountDown(fs_name);
				ShutdownDaemon = TRUE;
				ret = EXIT_NORESTART;
				break;	/* skip DoUpdate() */
			} else if (e != EINTR) {
				Trace(TR_MISC, "FS %s: wakeup (%d)",
				    Host.fs.fi_name, e);
			}
		}
		if (!FsCfgOnly && getppid() == 1) {
			Trace(TR_MISC, "FS %s: Parent sam-fsd died",
			    Host.fs.fi_name);
			ShutdownDaemon = TRUE;
		}
		DoUpdate(fs_name);
	}
	return (ret);
}


/*
 * SIGHUP, SIGINT, and SIGTERM are delivered here.
 */
static void
catchSig(int sig)
{
#ifdef sun
	Trace(TR_MISC, "FS %s: Signal %d received: %s",
	    Host.fs.fi_name, sig, strsignal(sig));
#endif /* sun */
#ifdef linux
	/* strsignal() cores on Linux.  Don't use for now. */
	Trace(TR_MISC, "FS %s: Signal %d received", Host.fs.fi_name, sig);
#endif /* linux */

	if (!FsCfgOnly && getppid() == 1) {
		Trace(TR_MISC, "FS %s: Parent sam-fsd died", Host.fs.fi_name);
		ShutdownDaemon = TRUE;
	}

	switch (sig) {
	case SIGALRM:
		break;

	case SIGHUP:
		/*
		 * Restart the trace log.
		 */
		TraceReconfig();
		break;

	case SIGINT:
	case SIGTERM:
		ShutdownDaemon = TRUE;
		break;

	case SIGCLD:
		break;

	default:
#ifdef sun
		Trace(TR_MISC, "FS %s: Signal %d received: %s (ignored)",
		    Host.fs.fi_name, sig, strsignal(sig));
#endif /* sun */
#ifdef linux
		/* strsignal() cores on Linux.  Don't use for now. */
		Trace(TR_MISC, "FS %s: Signal %d received (ignored)",
		    Host.fs.fi_name, sig);
#endif /* linux */
		break;
	}
}


/*
 * Catch SIGEMT.
 */
static void
catchSigEMT(int sig)
{
	TraceSignal(sig);
}


/*
 * ForceUmountDown - unmount the file system because server told us to
 * via a NOTIFY that we are going down.  Try to unmount cleanly first,
 * but ultimately we are going down.
 */
#ifdef sun
#define	UMOUNT "/usr/sbin/umount"
#endif /* sun */
#ifdef linux
#define	UMOUNT "/bin/umount"
#endif /* linux */

static void
ForceUmountDown(char *fs)
{
	char p[sizeof (UMOUNT) + sizeof (" -f ") + sizeof (uname_t) + 2];
	int r;

	Trace(TR_MISC, "FS %s: Down request received, unmounting.", fs);
	sprintf(p, UMOUNT " %s", fs);
	r = system(p);
#ifdef sun
	if (r) {
		/* try umount -f if umount failed */
		sprintf(p, UMOUNT " -f %s", fs);
		system(p);
	}
#endif /* sun */
}
