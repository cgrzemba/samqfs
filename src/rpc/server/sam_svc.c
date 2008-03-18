/*
 * sam_svc.c
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

#pragma ident "$Revision: 1.19 $"

#include <stdio.h>
#include <stdlib.h>		/* getenv, exit */
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory.h>
#include <stropts.h>
#include <netconfig.h>
#include <sys/resource.h>	/* rlimit */
#include <syslog.h>
#include <string.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#ifdef DEBUG
#define	RPC_SVC_FG
#endif

#define	_RPCSVC_CLOSEDOWN 120
#include <sys/types.h>
#include <sys/time.h>
#include <rpc/rpcent.h>

#include "pub/stat.h"
#include "pub/samrpc.h"
#include "sam/lib.h"

static int _rpcpmstart;		/* Started by a port monitor ? */

/* States a server can be in wrt request */

#define	_IDLE 0
#define	_SERVED 1

static int _rpcsvcstate = _IDLE;	/* Set when a request is serviced */
static int _rpcsvccount = 0;		/* Number of requests being serviced */

static
void
_msgout(
	char *msg)
{
#ifdef RPC_SVC_FG
	if (_rpcpmstart)
		sam_syslog(LOG_ERR, msg);
	else
		(void) fprintf(stderr, "%s\n", msg);
#else
	sam_syslog(LOG_ERR, msg);
#endif
}

static void
closedown(
	/* LINTED argument unused in function */
	int sig)
{
	exit(1);
}

static void
samfs_1(
	struct svc_req *rqstp,
	SVCXPRT * transp)
{
	union {
		statcmd	samstat_1_arg;
		statcmd	samlstat_1_arg;
		filecmd	samarchive_1_arg;
		filecmd	samrelease_1_arg;
		filecmd	samstage_1_arg;
		filecmd	samsetfa_1_arg;
		filecmd	samsegment_1_arg;
	} argument;
	char *result;
	char *pname = "nobody";
	uid_t request_uid, request_gid, my_euid, my_egid;
	struct passwd *pwentry;
	bool_t(*xdr_argument)(), (*xdr_result)();
	char  *(*local)();

	_rpcsvccount++;
	my_euid = geteuid();
	my_egid = getegid();
	if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
		struct authunix_parms *unix_cred =
		    (struct authunix_parms *)rqstp->rq_clntcred;
		if (unix_cred->aup_uid == 0) {
			pwentry = getpwnam(pname);
			request_uid = pwentry->pw_uid;
			request_gid = pwentry->pw_gid;
		} else {
			request_uid = unix_cred->aup_uid;
			request_gid = unix_cred->aup_gid;
		}
		setegid(request_gid);
		seteuid(request_uid);
	} else {
		svcerr_weakauth(transp);
		_rpcsvccount--;
		_rpcsvcstate = _SERVED;
		return;
	}
	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void, (char *)NULL);
		_rpcsvccount--;
		_rpcsvcstate = _SERVED;
		setegid(my_egid);
		seteuid(my_euid);
		return;

	case samstat:
		xdr_argument = xdr_statcmd;
		xdr_result = xdr_sam_st;
		local = (char *(*) ()) samstat_1;
		break;

	case samlstat:
		xdr_argument = xdr_statcmd;
		xdr_result = xdr_sam_st;
		local = (char *(*) ()) samlstat_1;
		break;

	case samarchive:
		xdr_argument = xdr_filecmd;
		xdr_result = xdr_int;
		local = (char *(*) ()) samarchive_1;
		break;

	case samrelease:
		xdr_argument = xdr_filecmd;
		xdr_result = xdr_int;
		local = (char *(*) ()) samrelease_1;
		break;

	case samstage:
		xdr_argument = xdr_filecmd;
		xdr_result = xdr_int;
		local = (char *(*) ()) samstage_1;
		break;

	case samsetfa:
		xdr_argument = xdr_filecmd;
		xdr_result = xdr_int;
		local = (char *(*) ()) samsetfa_1;
		break;

	case samsegment:
		xdr_argument = xdr_filecmd;
		xdr_result = xdr_int;
		local = (char *(*) ()) samsegment_1;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvccount--;
		_rpcsvcstate = _SERVED;
		setegid(my_egid);
		seteuid(my_euid);
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (char *)&argument)) {
		svcerr_decode(transp);
		_rpcsvccount--;
		_rpcsvcstate = _SERVED;
		setegid(my_egid);
		seteuid(my_euid);
		return;
	}
	result = (*local) (&argument, rqstp);
	setegid(my_egid);
	seteuid(my_euid);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (char *)&argument)) {
		_msgout("unable to free arguments");
		exit(1);
	}
	_rpcsvccount--;
	_rpcsvcstate = _SERVED;
}

int
main(
	/* LINTED argument unused in function */
	int argc,
	/* LINTED argument unused in function */
	char **argv)
{
	char mname[FMNAMESZ + 1];
	struct rpcent *rpce;
	sigset_t block_set;

	/*
	 * Reset our signal mask to not block anything, ignore SIGPIPE.
	 */
	(void) sigemptyset(&block_set);
	(void) sigprocmask(SIG_SETMASK, &block_set, NULL);
	(void) sigset(SIGPIPE, SIG_IGN);

	if (!(rpce = getrpcbyname(PROGNAME))) {
		_msgout("getrpcbyname failed\n");
		exit(1);
	}
	if (setgid(0) == -1) {
		_msgout("setgid failed\n");
		exit(1);
	}
	if (!ioctl(0, I_LOOK, mname) &&
	    (strcmp(mname, "sockmod") == 0 ||
	    strcmp(mname, "timod") == 0)) {

		char *netid;
		struct netconfig *nconf = NULL;
		SVCXPRT *transp;
		int pmclose;

		_rpcpmstart = 1;

		if ((netid = getenv("NLSPROVIDER")) == NULL) {
			/* started from inetd */
			pmclose = 1;
		} else {
			if ((nconf = getnetconfigent(netid)) == NULL)
				_msgout("cannot get transport info");

			pmclose = (t_getstate(0) != T_DATAXFER);
		}
		if (strcmp(mname, "sockmod") == 0) {
			if (ioctl(0, I_POP, 0) || ioctl(0, I_PUSH, "timod")) {
				_msgout("could not get the right module");
				exit(1);
			}
		}
		if ((transp = svc_tli_create(0, nconf, NULL, 0, 0)) == NULL) {
			_msgout("cannot create server handle");
			exit(1);
		}
		if (nconf)
			freenetconfigent(nconf);
		/*
		 * if (!svc_reg(transp, SamFS, SAMVERS, samfs_1, 0))
		 */
		if (!svc_reg(transp, rpce->r_number, SAMVERS, samfs_1, 0)) {
			_msgout("unable to (SamFS, SAMVERS).");
			exit(1);
		}
		if (pmclose) {
			(void) signal(SIGALRM, (void (*) ()) closedown);
			(void) alarm(_RPCSVC_CLOSEDOWN / 2);
		}
		svc_run();
		exit(1);
		/* LINTED statement has no consequent */
	} else {
#ifndef RPC_SVC_FG
		int size;
		struct rlimit rl;
		int i;

		rl.rlim_max = 0;
		getrlimit(RLIMIT_NOFILE, &rl);
		if ((size = rl.rlim_max) == 0)
			exit(1);
		for (i = 0; i < size; i++)
			(void) close(i);
		i = open("/dev/console", 2);
		(void) dup2(i, 1);
		(void) dup2(i, 2);
		setsid();
#endif
	}
	(void) signal(SIGINT, (void (*) ()) closedown);
	/*
	 * if (!svc_create(samfs_1, SamFS, SAMVERS, "netpath"))
	 */
	if (!svc_create(samfs_1, rpce->r_number, SAMVERS, "netpath")) {
		_msgout("unable to create (SamFS, SAMVERS) for netpath.");
		exit(1);
	}
	svc_run();
	_msgout("svc_run returned");
	exit(1);
	/* LINTED Function has no return statement */
}
