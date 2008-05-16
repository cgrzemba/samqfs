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
#pragma ident	"$Revision: 1.17 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <errno.h>

#include "aml/archiver.h"
#include "pub/mgmt/error.h"
#include "src/utility/samu/samu.h"
#include "pub/mgmt/types.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "pub/mgmt/filesystem.h"

#define	RUN_CTL		"run"
#define	RESTART_CTL	"restart"
#define	IDLE_CTL	"idle"
#define	STOP_CTL	"stop"
#define	RERUN_CTL	"rerun"


/*
 * Causes the archiver to run for the named file system. This function
 * overrides any wait commands that have previously been applied to the
 * filesystem through the archiver.cmd file.
 *
 * start archiving for fs fs_name
 */
int
ar_run(
ctx_t *ctx		/* ARGSUSED */,
uname_t fs_name)	/* name of fs for which to run archiving */
{

	char msg[80];

	Trace(TR_MISC, "arrun %s", fs_name);

	if (ISNULL(fs_name)) {
		Trace(TR_ERR, "arrun %s failed: %s", fs_name, samerrmsg);
		return (-1);
	}

	/* copy the command into msg */
	snprintf(msg, sizeof (msg), "exec.%s.%s", "fs", fs_name);

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(msg, RUN_CTL, msg, sizeof (msg));
	if (*msg != '\0') {

		set_samdaemon_err(errno, SE_AR_RUN_FAILED, msg);

		Trace(TR_ERR, "arrun %s failed: %s msg %s", fs_name,
		    samerrmsg, msg);
		return (-1);
	}

	Trace(TR_MISC, "arrun %s completed", fs_name);
	return (0);

}


/*
 * Causes a soft restart of the archiver.
 */
int
ar_rerun_all(
ctx_t *ctx /* ARGSUSED */)
{

	char msg[80];

	Trace(TR_MISC, "ar_rerun_all");

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(RERUN_CTL, NULL, msg, sizeof (msg));
	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_RERUN_FAILED, msg);
		Trace(TR_ERR, "ar_rerun_all failed: %s msg %s",
		    samerrmsg, msg);
		return (-1);
	}

	Trace(TR_MISC, "ar_rerun_all completed");
	return (0);

}


/*
 * Causes the archiver to run for all file systems. This function
 * overrides any wait commands that have been previously applied
 * through the archiver.cmd file.
 */
int
ar_run_all(
ctx_t *ctx /* ARGSUSED */)
{
	char msg[80];

	Trace(TR_MISC, "arrun all");

	/* copy the command into msg */
	snprintf(msg, sizeof (msg), "exec");

	/* pass msg as the command and the return buffer */
	ArchiverControl(msg, "run", msg, sizeof (msg));

	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_RUN_FAILED, msg);
		Trace(TR_ERR, "arrun all failed: %s\tmsg:%s", samerrmsg,
		    msg);
		return (-1);
	}

	Trace(TR_MISC, "arrun all completed");
	return (0);
}


/*
 * idle archiving for fs. This function stops archiving for the named file
 * system at the next convienent point. e.g. at the end of the current tar file
 */
int ar_idle(
ctx_t *ctx		/* ARGSUSED */,
uname_t fs_name)	/* The fs for which to idle archiving. */
{

	char msg[80];

	Trace(TR_MISC, "aridle %s", Str(fs_name));

	if (ISNULL(fs_name)) {
		Trace(TR_ERR, "aridle %s failed: %s", fs_name, samerrmsg);
		return (-1);
	}

	/* copy the command into msg */
	snprintf(msg, sizeof (msg), "exec.%s.%s", "fs", fs_name);

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(msg, IDLE_CTL, msg, sizeof (msg));

	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_IDLE_FAILED, msg);
		Trace(TR_ERR, "aridle %s failed: %s",
		    fs_name, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "aridle %s completed", fs_name);
	return (0);

}


/*
 * idle all archiving. This function stops all archiving at the next convienent
 * point. e.g. at the end of the current tar files
 */
int
ar_idle_all(
ctx_t *ctx /* ARGSUSED */)
{

	char msg[80];
	Trace(TR_MISC, "aridle all");

	/* copy the command into msg */
	snprintf(msg, sizeof (msg), "exec");

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(msg, IDLE_CTL, msg, sizeof (msg));
	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_IDLE_FAILED, msg);
		Trace(TR_ERR, "aridle all failed: %s msg:%s",
		    samerrmsg, msg);
		return (-1);
	}

	Trace(TR_MISC, "aridle all");
	return (0);
}

/*
 * Immediately stop archiving for fs. For a more graceful stop see ar_idle.
 */
int
ar_stop(
ctx_t *ctx		/* ARGSUSED */,
uname_t fs_name)	/* the name of the fs to stop archiving for */
{

	char msg[80];
	Trace(TR_MISC, "arstop %s", fs_name);

	/* copy the command into msg */
	snprintf(msg, sizeof (msg), "exec.%s.%s", "fs", fs_name);

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(msg, STOP_CTL, msg, sizeof (msg));
	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_STOP_FAILED, msg);
		Trace(TR_ERR, "arstop %s failed: %s msg: %s",
		    fs_name, samerrmsg, msg);
		return (-1);
	}

	Trace(TR_MISC, "arstop %s completed", fs_name);
	return (0);
}


/*
 * Immediately stop archiving for all filesystems. For a more graceful stop
 * see ar_idle_all.
 */
int
ar_stop_all(ctx_t *ctx /* ARGSUSED */)
{
	char msg[80];
	Trace(TR_MISC, "arstop all");

	/* copy the command into msg */
	snprintf(msg, sizeof (msg), "exec");

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(msg, STOP_CTL, msg, sizeof (msg));
	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_STOP_FAILED, msg);
		Trace(TR_ERR, "arstop all failed: %s msg:%s",
		    samerrmsg, msg);

		return (-1);
	}

	Trace(TR_MISC, "arstop all completed");
	return (0);
}


/*
 * Interupt the archiver and restart it for all filesystems.
 * This occurs immediately without regard to the state of the archiver.
 * It may terminate media copy operations and therefore waste space on media.
 * This should be done with caution. It is not possible to specify a particular
 * fs for restart only restart all is possible.
 */
int
ar_restart_all(ctx_t *ctx /* ARGSUSED */)
{
	char msg[80];
	Trace(TR_MISC, "arrestart all");

	/* pass msg as the command and the return buffer */
	(void) ArchiverControl(RESTART_CTL, NULL, msg, sizeof (msg));
	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_AR_RESTART_FAILED, msg);
		Trace(TR_ERR, "arrestart all failed: %s\tmsg:%s",
		    samerrmsg, msg);
		return (-1);
	}

	Trace(TR_MISC, "arrestart all completed");
	return (0);
}


/*
 * Function to set samerrno and samerrmsg for operations on the stager and
 * archiver daemons.
 *
 * The goal is to focus the error messages on what has occured and potentially
 * why and hopefully to avoid messages like:
 * "ar_restart failed: No such file or Directory"
 * which mean nothing to the user.
 *
 * Arguments:
 * errnum - the errno set by the daemon.
 * samerrnum - error number of an error that looks like:
 *	"<daemon> <operation> failed:%s"
 *	e.g.  "Archiver restart failed: %s"
 *
 * daemonmsg - string recieved from the daemon.
 */
void
set_samdaemon_err(int errnum, int samerrnum, char *daemonmsg) {
	char buf[MAX_MSG_LEN];
	char *msg;

	boolean_t mounted;

	/* check if any file systems are mounted */
	if (is_any_fs_mounted(NULL, &mounted) == -1) {
		/*
		 * We don't know what's up so set mounted to true to
		 * proceed with showing the msg from the daemon
		 */
		mounted = B_TRUE;
	}

	if (mounted == B_FALSE) {
		/*
		 * The likely cause is that no sam file systems are mounted
		 * and the daemon is not running. Reflect this in the
		 * additional message
		 */
		snprintf(buf, MAX_MSG_LEN, GetCustMsg(SE_NO_FS_ARE_MOUNTED));
		msg = buf;

	} else {
		/*
		 * We do not know what is wrong at this point. So include
		 * the message from the daemon itself.
		 */
		msg = daemonmsg;
	}


	/*
	 * We still must detect and pass along the ECONNREFUSED errno because
	 * it is used to make decisions up stack.
	 */
	if (errnum == ECONNREFUSED) {
		samerrno = SE_CONN_REFUSED;
	} else {
		samerrno = samerrnum;
	}

	/*
	 * Write the errmsg based on the input samerrnum even if we
	 * have set samerrno to SE_CONN_REFUSED.
	 */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrnum), Str(msg));

}
