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

#pragma ident	"$Revision: 1.24 $"

/*
 * recyc_sh_wrap.c.
 * A wrapper to get the
 * label or export information
 * from the recycler.sh or
 * add/delete/modify such action.
 */

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/stat.h>

/* SAM API headers. */
#include "pub/mgmt/recyc_sh_wrap.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"

/* SAM-FS headers. */
#include "sam/sam_trace.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

boolean_t	RC_EXPORT_ON = B_FALSE;
boolean_t	RC_LABEL_ON = B_FALSE;

static int add_recycle_sh_action(char *action);

/* Implementations */

/*
 * get_recycl_sh_action_status().
 * get what action (label/export)
 * is configured in the
 * recycler.sh.
 */

int
get_recycl_sh_action_status(
ctx_t	*ctx /* ARGSUSED */)
{

	FILE	*fp;
	char	line[1024]; /* line read in */
	uint32_t	stat_flag = 0; /* status flag */
	struct	stat64 buf;

	Trace(TR_MISC, "getting recycler action status");

	/*
	 * The recycler.sh script is optional, check if
	 * it exists, if the file does not exist, return 0
	 */
	if ((stat64(RECYC_SH, &buf)) != 0) {
		/* ENOENT */
		return (stat_flag);	/* file is optional */
	}

	/* file exists, open the recycler.sh, if open fails, return err */

	if ((fp = fopen(RECYC_SH, "r")) == NULL) {
		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_READ_OPEN_FAILED), RECYC_SH);
		Trace(TR_ERR, "gettin recycler action status failed: %s",
		    samerrmsg);
		return (-1);
	}

	while (fgets(line, sizeof (line), fp) != NULL) {

		/* line is NOT a comment and has 'label'cmd  in it */

		if ((strchr(line, '#') == NULL) &&
		    (strstr(line, LABEL) != NULL)) {

			stat_flag |= RC_label_on;
			RC_LABEL_ON = B_TRUE;
			Trace(TR_DEBUG, "Labeling is enabled");
			break;

		}

		/*
		 * Export is not indicated by the
		 * export command; it is merely
		 * a mail message sent indicating
		 * that 'someone' should physically
		 * export the media. Thus, this piece
		 * of code merely looks at a default
		 * mail-message sub-string and the
		 * comment character, '#', to
		 * determine if 'export' is "on".
		 */
		if ((strchr(line, '#') == NULL) &&
		    (strstr(line, EXPORT) != NULL)) {

			stat_flag |= RC_export_on;
			RC_EXPORT_ON = B_TRUE;
			Trace(TR_DEBUG, "Export is enabled");
			break;

		}
	}

	Trace(TR_DEBUG, "recycler action status flag is 0x%x",
	    stat_flag);

	fclose(fp);

	Trace(TR_MISC, "got recycler action status");

	return (stat_flag);
}


/*
 * add_recycle_sh_action(char *action).
 * Add the export/label action to the recycler.sh
 */
static int
add_recycle_sh_action(
char *action	/* label or export commands */
)
{

	int	fd;
	char	err_buf[80];
	struct stat64 buf;

	Trace(TR_MISC, "adding action to recycler.sh...");

	/*
	 * The recycler.sh script is optional, if it does not exist
	 * create the file
	 */
	if ((stat64(RECYC_SH, &buf)) == 0) {

		/* move  the original file */
		if (backup_cfg(RECYC_SH) != 0) {
			return (-1);
		}
		unlink(RECYC_SH);
	}

	/*
	 * create the file. give read permissions for others. This
	 * is required for automated testing, if required this can
	 * be rolled back to perms=750 during the 4.4 release
	 */

	if ((fd = open(RECYC_SH,  O_WRONLY | O_CREAT | O_TRUNC, 0754)) < 0) {
		samerrno = SE_FILE_CREATE_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_CREATE_OPEN_FAILED),
		    RECYC_SH);

		Trace(TR_ERR, "adding recycler action failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * Set the owner and group to root and bin, else
	 * retain the owner and group
	 * don't report it as an error ig chgrp fails
	 */
	fchown(fd, 0, 2);	/* root and bin */

	/*
	 * Write out the header: pls see the header file
	 * recyc_sh_wrap.h for a definition of this header.
	 */
	if ((write(fd, HEADER, sizeof (HEADER))) < 0) {

		samerrno = SE_WRITE_MODE_ERROR;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_WRITE_MODE_ERROR),
		    RECYC_SH, err_buf);

		Trace(TR_ERR, "adding recycler action failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* Now write out the action ON BLOCK */
	if ((write(fd, action, strlen(action))) < 0) {

		samerrno = SE_WRITE_MODE_ERROR;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_WRITE_MODE_ERROR),
		    RECYC_SH, err_buf);

		Trace(TR_ERR, "adding recycler action failed: %s",
		    samerrmsg);
		return (-1);
	}

	close(fd);

	Trace(TR_MISC, "added recycler action done");

	return (0);
}

/*
 * add_recycle_sh_action_label().
 * Allow labeling to occur in the
 * recycler.sh. This means, just
 * uncomment the default label
 * block.
 */
int
add_recycle_sh_action_label(
ctx_t 	*ctx /* ARGSUSED */)
{

	/* write out the LABEL ON BLOCK */
	if ((add_recycle_sh_action(LABEL_ON_BLOCK)) < 0) {

		Trace(TR_ERR, "adding recycler action failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "added recycler action label");

	return (0);
}


/*
 * add_recycle_sh_action_export().
 */
int
add_recycle_sh_action_export(
ctx_t	*ctx, /* ARGSUSED */
uname_t	emailaddr) /* the emailaddr to send */
{
	char str[1024];

	/* Write out the export ON block */
	sprintf(str, EXPORT_ON_BLOCK(emailaddr), emailaddr);

	add_recycle_sh_action(str);

	Trace(TR_MISC, "Added action label export");

	return (0);
}


/*
 * del_recycle_sh_action().
 * Comment out the directive section.
 * Since label and export are mutually
 * exclusive, only ONE common function.
 */
int
del_recycle_sh_action(ctx_t *ctx /* ARGSUSED */)
{
	char	cmd1[MAXPATHLEN+1];
	int	ret;
	FILE	*ptr;
	char	tmpfil[MAXPATHLEN+1];
	char 	err_buf[80];
	struct stat64 buf;

	Trace(TR_MISC, "deleting recycler.sh action...");

	/* check the file exists */
	if ((stat64(RECYC_SH, &buf)) != 0) {

		samerrno = SE_FILE_NOT_PRESENT;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_NOT_PRESENT),
		    RECYC_SH, err_buf);

		Trace(TR_ERR, "del. recycler.sh action failed: %s",
		    samerrmsg);
		return (-1);
	}


	/* backup the original file */
	ret = backup_cfg(RECYC_SH);
	if (ret != 0) {
		/* samerrno & samerrmsg set in backup_cfg() */
		return (-1);
	}

	/* comment out lines */
	if (mk_wc_path(RECYC_SH, tmpfil, sizeof (tmpfil)) != 0) {
		return (-1);
	}

	snprintf(cmd1, sizeof (cmd1), "%s %s %s %s %s",
	    "/usr/bin/sed", "'s/^\\([^#].*\\)/#\\1/'",
	    RECYC_SH, ">", tmpfil);

	Trace(TR_DEBUG, "del_recycle_sh_action(): the 'sed' cmd is [%s]\n",
	    cmd1);


	if ((ptr = popen(cmd1, "w")) == NULL) {
		setsamerr(SE_POPEN_WRITE_OPEN_FAILED);
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POPEN_WRITE_OPEN_FAILED),
		    cmd1);

		Trace(TR_MISC, "del. recycler.sh action failed: %s",
		    samerrmsg);
		unlink(tmpfil);
		return (-1);
	}

	pclose(ptr);

	memset(cmd1, 0, sizeof (cmd1));

	/* Now move the redirected file back */
	if ((cp_file(tmpfil, RECYC_SH)) < 0) {

		samerrno = SE_RENAME_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_RENAME_FAILED),
		    tmpfil, RECYC_SH);
		unlink(tmpfil);

		Trace(TR_ERR,
		    "adding recycler action failed: %s",
		    samerrmsg);
		return (-1);
	}

	unlink(tmpfil);
	Trace(TR_MISC, "recycler.sh action deleted...");

	return (0);
}
