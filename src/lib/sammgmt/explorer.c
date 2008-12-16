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
#pragma ident   "$Revision: 1.19 $"


static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "mgmt/config/common.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/file_util.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/version.h"
#include "pub/mgmt/process_job.h"

#define	DEF_SAMEXP_LOCATION "/tmp/SAMreport"
#define	DEF_SAMEXP_NAME "SAMreport"
#define	SAM_EXPLORER_CMD "samexplorer"
#define	DEF_SAMEXP_LOG_LINES 1000


#define	LST_EXP_FMT "path=%s,name=%s,size=%lld,created=%lu,modified=%lu"

static int display_explorer_activity(samrthread_t *ptr, char **result);


/*
 * return a list of sam-explorer outputs found in the default
 * locations /tmp and /var/tmp directories
 * key=value pairs
 * "path=%s,name=%s,size=%lld,created=%lu,modified=%lu"
 */
int
list_explorer_outputs(
ctx_t *c /* ARGSUSED */,
sqm_lst_t **report_paths) {

	char restricts[] = "Filename=SAMreport*";
	char *search_paths[] = {"/tmp", "/var/tmp", ""};
	sqm_lst_t *l;
	node_t *n;
	int i;
	char buf[1024];
	struct stat64 st;

	if (ISNULL(report_paths)) {
		Trace(TR_ERR, "list explorer outputs failed:%d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	*report_paths = lst_create();
	if (*report_paths == NULL) {
		Trace(TR_ERR, "list explorer outputs failed:%d %s", samerrno,
		    samerrmsg);
		return (-1);
	}


	for (i = 0; *(search_paths[i]) != '\0'; i++) {
		/* Get SAMreports from each search path */
		if (list_dir(NULL, 100, search_paths[i], restricts, &l) != 0) {

			lst_free_deep(*report_paths);

			Trace(TR_ERR, "list explorer outputs failed: %s",
			    samerrmsg);
			return (-1);
		}

		/* Get file details and insert strings into the results list */
		if (l != NULL && l->length != 0) {
			for (n = l->head; n != NULL; n = n->next) {
				char *fname = (char *)n->data;
				char *res_str;


				snprintf(buf, sizeof (buf), "%s/%s",
				    search_paths[i], fname);

				if (stat64(buf, &st) != 0 ||
				    !S_ISREG(st.st_mode)) {

					Trace(TR_MISC, "not including  %s %s",
					    buf, "in explorer outputs");
					continue;
				} else {

					snprintf(buf, sizeof (buf),
					    LST_EXP_FMT, search_paths[i],
					    fname, st.st_size,
					    st.st_ctim.tv_sec,
					    st.st_mtim.tv_sec);
				}

				res_str = copystr(buf);
				if (res_str == NULL) {
					goto err;
				}
				if (lst_append(*report_paths, res_str) != 0) {
					goto err;
				}
			}
		}
		lst_free_deep(l);
	}

	Trace(TR_MISC, "got explorer outputs");
	return (0);

err:
	lst_free_deep(l);
	lst_free_deep(*report_paths);
	*report_paths = NULL;
	Trace(TR_ERR, "list explorer outputs failed: %s",
	    samerrmsg);
	return (-1);
}


/*
 * execute sam-explorer include line_cnt lines from each log
 * and trace file and save the output in the directory <location>
 */
int
run_sam_explorer(
ctx_t *ctx /* ARGSUSED */,
char *location,
int log_lines)
{

	pid_t pid;
	int status = 0;
	FILE *res_stream = NULL;
	FILE *err_stream = NULL;
	char cmd[MAXPATHLEN + 40];
	char output[MAXPATHLEN];
	time_t tm;
	struct tm now;
	size_t len;
	exec_cleanup_info_t *cl;
	char hostname[MAXHOSTNAMELEN];
	char *ch;
	char *job_id;
	argbuf_t *arg;
	int rval;

	if (location == NULL || *location == '\0') {
		/* set default location */
		snprintf(output, (size_t)MAXPATHLEN, "%s.",
		    DEF_SAMEXP_LOCATION);
	} else {
		snprintf(output, (size_t)MAXPATHLEN, "%s/%s.", location,
		    DEF_SAMEXP_NAME);
	}

	/* get the hostname and add it to the file name */
	if (gethostname(hostname, MAXHOSTNAMELEN) == 0) {
		strlcat(output, hostname, MAXPATHLEN);
		strlcat(output, ".", MAXPATHLEN);
	}

	/* add the sam version to the file name */
	strcpy(cmd, SAM_VERSION);

	/* In the version replace '.' with '_' */
	while ((ch = strchr(cmd, '.')) != NULL) {
		*ch = '_';
	}
	/* leave room for the date */
	strlcat(output, cmd, MAXPATHLEN);
	strlcat(output, ".", MAXPATHLEN);

	len = strlen(output);

	tm = time(0);
	localtime_r(&tm, &now);
	strftime(output+len, MAXPATHLEN-len, "%b%d%Y_%H%M", &now);

	if (log_lines == -1) {
		log_lines = DEF_SAMEXP_LOG_LINES;
	}

	snprintf(cmd, sizeof (cmd), "%s/%s %s %d", SBIN_DIR,
	    SAM_EXPLORER_CMD, output, log_lines);


	/* make the argument buffer for the activity */
	arg = (argbuf_t *)mallocer(sizeof (explorerbuf_t));
	if (arg == NULL) {
		return (-1);
	}
	strlcpy(arg->e.location, output, MAXPATHLEN);
	arg->e.log_lines = log_lines;

	/*
	 * create cleanup info struct prior to exec call so we can
	 * cleanly exit if it fails.
	 */
	cl = (exec_cleanup_info_t *)mallocer(sizeof (exec_cleanup_info_t));
	if (cl == NULL) {
		free_argbuf(SAMA_RUNEXPLORER, arg);
		Trace(TR_ERR, "running samexplorer failed, error:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/* create the activity */
	rval = start_activity(display_explorer_activity, kill_fork,
	    SAMA_RUNEXPLORER, arg, &job_id);
	if (rval != 0) {
		free(cl);
		free_argbuf(SAMA_RUNEXPLORER, arg);
		return (-1);
	}

	Trace(TR_MISC, "running samexplorer using command %s", cmd);

	/* exec the command and set its pid in the job */
	pid = exec_get_output(cmd, &res_stream, &err_stream);
	if (pid < 0) {
		free(cl);
		end_this_activity(job_id);
		Trace(TR_ERR, "running samexplorer failed, error:%d %s",
		    samerrno, samerrmsg);

		return (-1);
	}
	set_pid_or_tid(job_id, pid, 0);

	/* setup struct for call to cleanup */
	strlcpy(cl->func, cmd, sizeof (cl->func));
	cl->pid = pid;
	strlcpy(cl->job_id, job_id, MAXNAMELEN);
	cl->streams[0] = res_stream;
	cl->streams[1] = err_stream;


	/*
	 * wait a little while to verify the command has
	 * begun to execute cleanly. This also will give time
	 * for the results file to get created so it will show up
	 * in the list returned by list_explorer_outputs
	 */
	rval = bounded_activity_wait(&status, 10, job_id, pid, cl,
	    cleanup_after_exec_get_output);

	if (rval == -2) {
		/* went asynch setup async message */
		Trace(TR_MISC, "samexplorer going async. pid = %ld", pid);

		samerrno = SE_EXPLORER_ASYNC;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_EXPLORER_ASYNC));

		/* convert the job id into an int and return it */
		errno = 0;
		rval = atoi(job_id);
		if (errno != 0) {
			/*
			 * if translating the id failed
			 * simply return success.
			 */
			Trace(TR_ERR, "translating job id failed:%d", errno);
			rval = 0;
		}
		free(job_id);
		return (rval);

	} else if (rval < 0) {
		/* command returned an error */
		free(job_id);
		samerrno = SE_EXPLORER_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_EXPLORER_FAILED));

		Trace(TR_ERR, "samexplorer failed, exit code: %d",
		    WEXITSTATUS(status));

		return (-1);
	} else {
		free(job_id);
		/* succeeded */
		Trace(TR_MISC, "samexplorer done pid = %ld status: %d", pid,
		    status);
		return (0);
	}
}

int
display_explorer_activity(samrthread_t *ptr, char **result) {
	char buf[MAXPATHLEN];

	snprintf(buf, sizeof (buf), "activityid=%s,starttime=%ld"
	    ",details=\"%s\",type=%s,pid=%d",
	    ptr->jobid, ptr->start, ptr->args->e.location,
	    activitytypes[ptr->type], ptr->pid);
	*result = copystr(buf);
	return (0);
}
