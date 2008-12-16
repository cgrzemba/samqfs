/*
 * udscom.c - UDS communication functions.
 *
 * udscom provides a compact, general IPC message mechanism from SAM
 * processes to server daemons.  The message transmission is performed
 * using UNIX Domain Sockets(UDS) streams.  The sockets are located in
 * /var/opt/SUNWsamfs/uds.  Each server daemon has its named socket in this
 * directory
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

#pragma ident "$Revision: 1.19 $"

/* Define TEST to build a test jig. */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

/* Socket headers. */
#include <sys/socket.h>
#include <sys/un.h>

/* SAM-FS headers. */
#include "sam/names.h"
#include "sam/udscom.h"
#if defined(lint)
#include "sam/lint.h"
#undef unlink
#endif /* defined(lint) */

/* Private functions. */
static void catchSigpipe(int sig);
static void samlog(void (*LogFunc)(char *msg), struct UmNak *nak,
	struct UdsMsgHeader *hdr, const char *fmt, ...);
static void readClientMessage(int sfd, struct UdsServer *srvr, char *argbuf);
static ssize_t sockRead(int fildes, void *buf, size_t nbyte);
static ssize_t sockWrite(int fildes, void *buf, size_t nbyte);


/*
 * Send a message from a client to a server.
 */
int
UdsSendMsg(
	const char *srcFile,
	const int srcLine,
	struct UdsClient *clnt,	/* Client definitions */
	int srvrMsgType,	/* Server message type */
	void *arg,		/* Argument to message */
	int arg_size,		/* Size of argument to send */
	void *rsp,		/* Where to put response */
	int rsp_size)    	/* Size of response buffer */
{
	struct UdsMsgHeader hdr;
	struct UmNak nak;
	struct sockaddr_un name;
	int len;
	int n;
	int sock;

	/*
	 * Create the address of the server.
	 */
	memset(&name, 0, sizeof (name));
	name.sun_family = AF_UNIX;
	sprintf(name.sun_path, "%s/uds/%s", SAM_VARIABLE_PATH,
	    clnt->UcServerName);
	len = sizeof (name.sun_family) + strlen(name.sun_path);

	/*
	 * Create the socket.
	 */
retry:
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		if (errno == ENOMEM || errno == ENOBUFS) {
			sleep(1);
			goto retry;
		}
		return (-1);
	}

	/*
	 * Connect to the server.
	 */
	if (connect(sock, (struct sockaddr *)&name, len) < 0) {
		(void) close(sock);
		return (-1);
	}

	/*
	 * Send header.
	 */
	hdr.UhMagic   = clnt->UcMagic;
	hdr.UhType    = UM_max + srvrMsgType;
	strncpy(hdr.UhName, clnt->UcClientName, sizeof (hdr.UhName)-1);
	hdr.UhPid    = getpid();
	strncpy(hdr.UhSrcFile, srcFile, sizeof (hdr.UhSrcFile)-1);
	hdr.UhSrcLine = srcLine;
	hdr.UhArgSize = (arg != NULL) ? arg_size : 0;

#ifdef TEST
/*
 * This section generates pseudo random errors.
 */
	{
		int v;

		v = drand48() * 100000;
		if (20000 < v && v < 20010) {
			/*
			 * Short length header.
			 */
			if (write(sock, &hdr, sizeof (hdr)-1) == -1) {
				perror("SendToServer:");
				exit(EXIT_FAILURE);
			}
			(void) close(sock);
			return (1);
		}
		if (50000 < v && v < 50050) {
			/*
			 * Make a bad message.
			 */
			hdr.UhMagic = 020104;
		}
		if (60000 < v && v < 60050) {
			/*
			 * Make a bad message.
			 */
			hdr.UhType = 23;
		}
		if (70000 < v && v < 70050) {
			/*
			 * Make a short argument.
			 */
			arg_size = hdr.UhArgSize - 1;
		}
		if (80000 < v && v < 80050) {
			if (write(sock, &hdr, sizeof (hdr)) == -1) {
				(void) close(sock);
				return (-1);
			}
			(void) close(sock);
			return (0);
		}
	}
#endif /* TEST */

	if (sockWrite(sock, &hdr, sizeof (hdr)) == -1) {
		(void) close(sock);
		return (-1);
	}

	/*
	 * Send message argument.
	 */
	if (hdr.UhArgSize != 0)  {
		if (sockWrite(sock, arg, arg_size) == -1) {
			(void) close(sock);
			return (-1);
		}
	}

	n = sockRead(sock, &hdr, sizeof (hdr));
	if (n == sizeof (hdr) && hdr.UhArgSize != 0) {
		char c;
		int rc;

		if (hdr.UhType == UM_nak) {
			rsp = &nak;
			rsp_size = sizeof (nak);
		}

		/*
		 * Read response.
		 */
		rc = 0;
		if (rsp != NULL && rsp_size > 0) {
			rc = (int)rsp_size;
			if (rc > hdr.UhArgSize) {
				rc = hdr.UhArgSize;
			}
			n = sockRead(sock, rsp, rc);
		}
		while (rc++ < hdr.UhArgSize) {
			(void) sockRead(sock, &c, 1);
		}
	}
	(void) close(sock);
	if (n < 0) {
		return (-1);
	}
	if (hdr.UhType != UM_nak) {
		return (0);
	}
	errno = nak.Errno;
	if (clnt->UcLog != NULL) {
		clnt->UcLog(nak.msg);
	}
	return (UM_nak);
}


/*
 * Receive Client message.
 */
int
UdsRecvMsg(
	struct UdsServer *srvr)			/* Server definitions */
{
	struct sigaction sig_action;
	struct sockaddr_un name;
	char *argbuf = NULL;
	int len;
	int sock = -1;
	int r = 0;

	argbuf = malloc(srvr->UsArgbufSize);
	if (argbuf == NULL) {
		return (-1);
	}

	/*
	 * Catch SIGPIPE.
	 */
	sig_action.sa_handler = catchSigpipe;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	(void) sigaction(SIGPIPE, &sig_action, NULL);

	/*
	 * Create the address of the server.
	 */
	memset(&name, 0, sizeof (struct sockaddr_un));
	name.sun_family = AF_UNIX;
	sprintf(name.sun_path, "%s/uds/%s", SAM_VARIABLE_PATH,
	    srvr->UsServerName);
	len = sizeof (name.sun_family) + strlen(name.sun_path);

	/*
	 * Remove any previous socket.
	 */
	if (unlink(name.sun_path) == -1) {
		if (errno != ENOENT) {
			r = -1;
			goto out;
		}
	}

	/*
	 * Create the socket.
	 */
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		r = -1;
		goto out;
	}

	/*
	 * Bind the socket to the address.
	 */
	if (bind(sock, (struct sockaddr *)&name, len) < 0) {
		r = -1;
		goto out;
	}

	/*
	 * Listen for connections.
	 */
	if (listen(sock, 5) < 0) {
		r = -1;
		goto out;
	}

	while (!srvr->UsStop) {
		int ns;
#if defined(SIM_ERROR)
		static int msgcount = 0;
#endif /* defined(SIM_ERROR) */

		/*
		 * Accept a connection.
		 * Read and process message.
		 */
		if ((ns = accept(sock, (struct sockaddr *)&name, &len)) < 0) {
			r = -1;
			goto out;
		}

#if defined(SIM_ERROR)
		fprintf(stderr, "\rMessage %d ", msgcount);
		msgcount++;
#endif /* defined(SIM_ERROR) */

		readClientMessage(ns, srvr, argbuf);
		(void) close(ns);
	}

out:
	if (sock >= 0) {
		(void) close(sock);
	}
	if (argbuf != NULL) {
		free(argbuf);
	}
	return (r);
}


/*
 * Catch SIGPIPE.
 */
static void
catchSigpipe(
/* LINTED argument unused in function */
	int sigNum)
{
}


/*
 * Log a communication error.
 */
static void
samlog(
	void (*logFunc)(char *msg),	/* Server's logging function */
	struct UmNak *nak,		/* Nak message buffer */
	struct UdsMsgHeader *hdr,	/* Message header */
	const char *fmt,		/* printf() style format. */
	...)
{
	char *msg;
	int l;
	int n;

	msg = nak->msg;
	l = sizeof (nak->msg) - 1;
	if (hdr != NULL) {
		snprintf(msg, l, "From: %s[%ld] %s:%d, type %d ",
		    hdr->UhName, hdr->UhPid,
		    hdr->UhSrcFile, hdr->UhSrcLine, hdr->UhType);
		n = strlen(msg);
		l -= n;
		msg += n;
	}

	/* The message */
	if (fmt != NULL && l > 0) {
		va_list args;

		va_start(args, fmt);
		vsnprintf(msg, l, fmt, args);
		va_end(args);
		n = strlen(msg);
		l -= n;
		msg += n;
	}

	/* Error number */
	if (nak->Errno != 0 && l > 0) {
		char	*p;

		*msg++ = ':';
		*msg++ = ' ';
		l -= 2;
		p = strerror(nak->Errno);
		if (p != NULL && l > 0) {
			strncpy(msg, p, l - 1);
		} else {
			snprintf(msg, l, "error %d", nak->Errno);
		}
		n = strlen(msg);
		l -= n;
		msg += n;
	}
	*msg = '\0';
	if (logFunc != NULL) {
		logFunc(nak->msg);
	}
}


/*
 * Read message from client.
 */
static void
readClientMessage(
	int sfd,
	struct UdsServer *srvr,
	char *argbuf)
{
	struct UdsMsgHeader hdr;
	struct UmNak nak;
	void	*rsp;
	int		msgtype;
	int		n;

	/*
	 * Read message header.
	 */
	nak.Errno = 0;
	errno = 0;
	n = sockRead(sfd, &hdr, sizeof (hdr));
	if (n < 0) {
		nak.Errno = errno;
		samlog(srvr->UsLog, &nak, NULL, "Message read error");
		return;
	}

	/*
	 * Check header validity.
	 * Size read and magic correct.
	 */
	if (n != sizeof (hdr)) {
		samlog(srvr->UsLog, &nak, &hdr,
		    "read header: expected %d, got %d", sizeof (hdr), n);
		return;
	}
	if (hdr.UhMagic != srvr->UsMagic) {
		samlog(srvr->UsLog, &nak, &hdr, "Bad magic %o", hdr.UhMagic);
		goto nak;
	}

	if (hdr.UhType < UM_max) {
		/*
		 * Just idle chatter.
		 */
		if (hdr.UhArgSize > 0) {
			samlog(srvr->UsLog, &nak, &hdr,
			    "Invalid message type", hdr.UhType);
			goto nak;
		}
		goto ack;
	}

	/*
	 * Check for valid server message.
	 * Message type in range, and correct argument size.
	 */
	msgtype = hdr.UhType - UM_max;
	if (msgtype >= srvr->UsNumofTable) {
		samlog(srvr->UsLog, &nak, &hdr,
		    "Invalid server message", msgtype);
		goto nak;
	}
	if (hdr.UhArgSize != srvr->UsTable[msgtype].UmArgSize ||
	    hdr.UhArgSize > srvr->UsArgbufSize) {
		samlog(srvr->UsLog, &nak, &hdr,
		    "Invalid argument size for %d", msgtype);
		goto nak;
	}
	if (hdr.UhArgSize > 0) {
		/*
		 * Read message argument.
		 */
		n = sockRead(sfd, argbuf, hdr.UhArgSize);
		if (n < 0) {
			nak.Errno = errno;
			samlog(srvr->UsLog, &nak, &hdr,
			    "read argument: read error");
			goto nak;
		}
		if (n != hdr.UhArgSize) {
			nak.Errno = errno;
			samlog(srvr->UsLog, &nak, &hdr,
			    "read argument: expected %d, got %d",
			    hdr.UhArgSize, n);
			goto nak;
		}
	}
	hdr.UhArgSize = 0;

	/*
	 * Process message.
	 */
	rsp = srvr->UsTable[msgtype].UmFunc(argbuf, &hdr);
	if (rsp != NULL && srvr->UsTable[msgtype].UmRspSize != 0) {
		hdr.UhArgSize = srvr->UsTable[msgtype].UmRspSize;
	}

	/*
	 * Send response.
	 */
ack:
	hdr.UhType = UM_ack;
	if (sockWrite(sfd, &hdr, sizeof (hdr)) < 0) {
		nak.Errno = errno;
		samlog(srvr->UsLog, &nak, &hdr, "Response send error");
	}
	if (hdr.UhArgSize > 0) {
		if (sockWrite(sfd, rsp, hdr.UhArgSize) < 0) {
			nak.Errno = errno;
			samlog(srvr->UsLog, &nak, &hdr,
			    "Response argument send error");
		}
	}
	return;

nak:
	hdr.UhType = UM_nak;
	hdr.UhArgSize = sizeof (nak);
	if (sockWrite(sfd, &hdr, sizeof (hdr)) < 0) {
		samlog(srvr->UsLog, &nak, &hdr, "Response send error");
	}
	if (sockWrite(sfd, &nak, sizeof (nak)) < 0) {
		samlog(srvr->UsLog, &nak, &hdr, "Response send error");
	}
}


/*
 * Read from socket.
 * Read data and handle an EINTR.
 */
static ssize_t
sockRead(
	int fildes,
	void *buf,
	size_t nbyte)
{
	char	*p;
	size_t nrem;

	p = (char *)buf;
	nrem = nbyte;
	while (nrem > 0) {
		ssize_t n;

		errno = 0;
		n = read(fildes, p, nrem);
		if (n == 0) {
			break;
		}
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return (n);
		}
		p += n;
		nrem -= n;
	}
	return (nbyte - nrem);
}


/*
 * Write to socket.
 * Write data and handle an EINTR.
 */
static ssize_t
sockWrite(
	int fildes,
	void *buf,
	size_t nbyte)
{
	char	*p;
	size_t nrem;

	p = (char *)buf;
	nrem = nbyte;
	while (nrem > 0) {
		ssize_t n;

		errno = 0;
		n = write(fildes, p, nrem);
		if (n == 0) {
			break;
		}
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return (n);
		}
		p += n;
		nrem -= n;
	}
	return (nbyte - nrem);
}


#if defined(TEST)
/*
 * Test section.
 * Compile with TEST defined.
 * cc -o test_udscom -g -DTEST udscom.c -lxnet
 *
 * Start server with:
 * ./test_udscom >s_out
 * Then start client with:
 * ./test_udscom some_arg >c_out
 *
 * Several error messages with be generated to stderr.
 *
 * Server may be stopped with ^C.
 */

#define	HERE _SrcFile, __LINE__

static int client_main(void);
static int srvr_main(void);

int
main(
	int argc)
{
	if (argc > 1) {
		fprintf(stderr, "Starting test client.\n");
		return (client_main());
	} else {
		fprintf(stderr, "Starting test server.\n");
		return (srvr_main());
	}
}



/* clisrvr.h - client/server definitions. */

#if !defined(CLISRVR_H)
#define	CLISRVR_H

#define	SERVER_NAME "UdsSrvr"
#define	SERVER_MAGIC 00122152307

enum UdsSrvrReq {
	USR_noarg,
	USR_setval32,
	USR_setval64,
	USR_setstring,
	USR_MAX
};


/* Arguments for requests. */
struct UsrSetval32 {
	int	val;
};

struct UsrSetval64 {
	long long	val;
};

struct UsrSetstring {
	char	string[80];
};

struct UsrResponse {
	char	rspmsg[32];
};

#endif /* !defined(CLISRVR_H) */

/*
 * udsclient.c - UNIX domain sockets server example.
 */

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>

/* Socket headers. */
#include <sys/socket.h>
#include <sys/un.h>

/* SAM-FS  headers. */
#include "sam/udscom.h"
/* #include "clisrvr.h" */

/* Private functions. */
static void ClientLogit(char *msg);

/* Private data. */
static struct UdsClient clnt = { SERVER_NAME, "Client", SERVER_MAGIC,
	ClientLogit };

int
client_main(void)
{
	int msgnum;
	int n;

	msgnum = 0;

	for (n = 0; n < 50000; n++) {
		int		status;

		switch (msgnum) {

		case USR_setval32:
			status = UdsSendMsg(HERE, &clnt, USR_setval32,
			    &n, sizeof (n), NULL, 0);
			break;

		case USR_setval64: {
			long long v;

			v = (long long)n * 1000000000;
			status = UdsSendMsg(HERE, &clnt, USR_setval64,
			    &v, sizeof (v), NULL, 0);
		}
		break;

		case USR_setstring: {
			struct UsrSetstring r;
			struct UsrSetstring s;

			strcpy(s.string, "Just another message.");
			status = UdsSendMsg(HERE, &clnt, USR_setstring,
			    &s, sizeof (s), &r, sizeof (r));
			printf("Response to %d %s\n", n, r.string);
		}
		break;

		default:
			status = UdsSendMsg(HERE, &clnt, msgnum,
			    NULL, 0, NULL, 0);
			break;
		}

		if (status == -1) {
			perror("UdsSendMsg");
			exit(EXIT_FAILURE);
		}
		if (status != 0) {
			fprintf(stderr, "Message %d NAKed\n", n);
		}

		msgnum++;
		if (msgnum >= USR_MAX) {
			msgnum = 0;
		}
	}
	return (EXIT_SUCCESS);
}


/*
 * Log a communication error.
 */
static void
ClientLogit(
	char *msg)
{
	fprintf(stderr, "%s\n", msg);
}

/*
 * udssrvr.c - UNIX domain sockets prototype server.
 */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

/* SAM-FS  headers. */
#include "sam/udscom.h"
/* #include "clisrvr.h" */

/* Private functions. */
static void CatchSigint(int sig);
static void SrvrLogit(char *msg);
static void *Noarg(void *arg, struct UdsMsgHeader *hdr);
static void *SetVal32(void *arg, struct UdsMsgHeader *hdr);
static void *SetVal64(void *arg, struct UdsMsgHeader *hdr);
static void *SetString(void *arg, struct UdsMsgHeader *hdr);

/* Private data. */
static struct UdsMsgProcess Table[] = {
	{ Noarg, 0, 0 },
	{ SetVal32, sizeof (struct UsrSetval32), 0 },
	{ SetVal64, sizeof (struct UsrSetval64), 0 },
	{ SetString, sizeof (struct UsrSetstring),
		sizeof (struct UsrSetstring) }
};

/* Define the argument buffer to determine size required. */
union argbuf {
	struct UsrSetval32 a;
	struct UsrSetval64 b;
	struct UsrSetstring c;
	struct UsrResponse d;
};

static struct UdsServer srvr =
	{ SERVER_NAME, SERVER_MAGIC, SrvrLogit, 0, Table, USR_MAX,
	sizeof (union argbuf) };

static int msgs_processed = 0;
static int errors = 0;

int
srvr_main(void)
{
	struct sigaction sig_action;

	/*
	 * Catch SIGINT.
	 */
	sig_action.sa_handler = CatchSigint;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	(void) sigaction(SIGINT, &sig_action, NULL);

	/*
	 * Receive messages.
	 */
	if (UdsRecvMsg(&srvr) == -1) {
		if (errno != EINTR) {
			perror("UdsRecvMsg");
			exit(EXIT_FAILURE);
		}
	}
	fprintf(stderr, "\nMessages processed %d, errors %d\n",
	    msgs_processed, errors);
	return (EXIT_SUCCESS);
}


/*
 * Catch SIGPIPE.
 */
static void
CatchSigint(
/* LINTED argument unused in function */
	int SigNum)
{
	srvr.UsStop = 1;
}


/*
 * Log a communication error.
 */
static void
SrvrLogit(
	char *msg)
{
	errors++;
	fprintf(stderr, "%s\n", msg);
}


static void *
Noarg(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	msgs_processed++;
	arg = NULL;
	printf("Received Noarg\n");
	return (NULL);
}

static void *
SetVal32(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct UsrSetval32 *a = (struct UsrSetval32 *)arg;

	msgs_processed++;
	printf("Received value %d\n", a->val);
	return (NULL);
}


static void *
SetVal64(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct UsrSetval64 *a = (struct UsrSetval64 *)arg;

	msgs_processed++;
	printf("Received value %lld\n", a->val);
	return (NULL);
}


static void *
SetString(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	struct UsrSetstring *a = (struct UsrSetstring *)arg;
	static struct UsrSetstring rsp;

	msgs_processed++;
	strcpy(rsp.string, "Answer to Setstring");

	printf("Received string %s\n", a->string);
	return (&rsp);
}

#endif /* defined(TEST) */
