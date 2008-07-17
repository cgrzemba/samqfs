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
#pragma ident   "$Revision: 1.23 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */


#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <dirent.h>
#include <procfs.h>
#include <strings.h>
#include <ctype.h>
#include <fcntl.h>
#include "sam/sam_trace.h"
#include "aml/archiver.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/util.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/error.h"

int create_process_job(psinfo_t *info, char **output);

boolean_t job_type_matches(char *type_str, char *str);

int get_activity_type(char *str, char **type_str);


boolean_t psinfo_matches_filter(psinfo_t info, char *filter);


static int parse_job_filter(char *filter, pid_t *pid,
	pid_t *ppid, char *argstr, char *type, char *fname);

static int parse_str(char *in, char *out, int bufsize);

static int parse_pid(char *in, pid_t *out);



/* process names indexed by proc_job_type */
static char proc_type_names[][25]  = { AC_PROG, AF_PROG, "tplabel", "samfsck",
	"emax", "__procjobbegin__", "umount", "mount", "sammkfs", "archiver",
	"sam-sharefsd", "__procjobend__", "other"};


/*
 * define sam names that are currently not in public headers.
 */
#define	SAM_ARFIND_NM		"sam-arfind"
#define	SAM_RAPIDCHNGD_NM	"sam-rapidchgd"
#define	SAM_GENERICD_NM		"sam-genericd"
#define	SAM_STKD_NM		"sam-stkd"
#define	SAM_IBM3494D_NM		"sam-ibm3494d"
#define	SAM_SONYD_NM		"sam-sonyd"
#define	SAM_STKSSI_NM		"ssi.sh"   /* Needed if stk's defined */
#define	SAM_RMT_SRVD_NM		"sam-serverd"
#define	SAM_RMT_CLID_NM		"sam-clientd"
#define	SAM_SCANNERD_NM		"sam-scannerd"
#define	SAM_ROBOTSD_NM		"sam-robotsd"
#define	SAM_SAMRPCD_NM		"sam-rpcd"
#define	SAM_CATSERVER_CMD	"sam-catserverd"


/*
 * Names are used to map between processes and activity types. These should not
 * be confused with the activity type strings.
 *
 */

static char *activity_names[activitytypemax] = {
	"samrnone",		/* SAMRTHR_E */
	"samrdecompress",	/* SAMRTHR_D */
	"samrsearch",		/* SAMRTHR_S */
	"samrrestore",		/* SAMRTHR_R */
	"samrcrontab",		/* SAMRTHR_C */
	"samrdump",		/* SAMRTHR_U */

	SAM_FSD,		/* SAMD_FSD */
	SAM_ARCHIVER,		/* SAMD_ARCHIVERD */
	SAM_ARFIND_NM,		/* SAMD_ARFIND */
	SAM_STAGER,		/* SAMD_STAGERD */
	SAM_STAGEALL,		/* SAMD_STAGEALLD */
	SAM_AMLD,		/* SAMD_AMLD */
	SAM_SCANNERD_NM,	/* SAMD_SCANNERD */
	SAM_CATSERVER,		/* SAMD_CATSERVERD */
	"fsmgmtd",		/* SAMD_MGMTD */
	SAM_ROBOTSD_NM,		/* SAMD_ROBOTSD */
	SAM_GENERICD_NM,	/* SAMD_GENERICD */
	SAM_STKD_NM,		/* SAMD_STKD */
	SAM_IBM3494D_NM,	/* SAMD_IBM3494D */
	SAM_SONYD_NM,		/* SAMD_SONYD */

	SAM_SHAREFSD,		/* SAMD_SHAREFSD */
	SAM_RMTSERVER,		/* SAMD_RMTSERVERD */
	SAM_RMTCLIENT,		/* SAMD_RMTCLIENTD */
	SAM_RFT,		/* SAMD_RFTD */
	SAM_RELEASER,		/* SAMD_RELEASERD */
	SAM_RECYCLER,		/* SAMD_RECYCLERD */

	"samfsck",		/* SAMP_FSCK */
	"tplabel",		/* SAMP_TPLABEL */
	"mount",		/* SAMP_MOUNT */
	"umount",		/* SAMP_UMOUNT */
	"sammkfs",		/* SAMP_SAMMKFS */
	"archiver",		/* SAMP_ARCHIVERCLI */

	"releasefiles",		/* SAMA_RELEASEFILES */
	"archivefiles",		/* SAMA_ARCHIVEFILES */
	"runexplorer",		/* SAMA_RUNEXPLORER */
	"stagefiles",		/* SAMA_STAGEFILES */
	"samadispatchjob"	/* SAMADISPATCHJOB */
};

#define	UNKNOWN_ACTIVITY "unknown_activity"

/*
 * function to get information about a running processes that match the input
 * parameters.
 */
int
get_process_jobs(
ctx_t		*ctx		/* ARGSUSED */,
char		*filter,	/* see Filter Strings in process_job.h */
sqm_lst_t		**job_list)	/* list of proc_job_t */
{

	static char	*procdir = "/proc";	/* standard /proc directory */
	struct dirent64	*dent;
	struct dirent64	*dentp;
	DIR		*dirp;
	char		pname[MAXPATHLEN+1];

	Trace(TR_MISC, "getting process job with filter %s", Str(filter));

	if (ISNULL(job_list)) {
		Trace(TR_OPRMSG, "getting process jobs failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if ((dirp = opendir(procdir)) == NULL) {
		samerrno = SE_OPENDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), procdir, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "open dir %s failed", procdir);
		return (-1);
	}

	dent = mallocer(sizeof (struct dirent64) + MAXPATHLEN + 1);
	if (dent == NULL) {
		closedir(dirp);
		return (-1);
	}

	*job_list = lst_create();
	if (*job_list == NULL) {
		Trace(TR_ERR, "getting process jobs failed %d %s", samerrno,
		    samerrmsg);
		(void) closedir(dirp);
		free(dent);
		return (-1);
	}

	/* for each active process --- */
	while ((readdir64_r(dirp, dent, &dentp)) == 0) {
		int	procfd;	/* filedescriptor for /proc/nnnnn/psinfo */
		psinfo_t info;	/* process information from /proc */
		char *job_str;

		if (dentp == NULL) {
			break;
		}

		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;

		snprintf(pname, sizeof (pname), "%s/%s/psinfo",
		    procdir, dentp->d_name);

		if ((procfd = open(pname, O_RDONLY)) == -1) {
			continue;
		}

		/*
		 * Get the info structure for the process and close quickly.
		 */
		if (readbuf(procfd, (char *)&info, sizeof (info)) < 0) {
			continue;
		}
		(void) close(procfd);

		if (info.pr_lwp.pr_state == 0)		/* can't happen? */
			continue;

		/* Screen out processes that do not match our criteria. */
		/*
		 * make this a helper function that takes an info
		 * and a proc_job and returns true if match has been made.
		 * this implies a signature change for this function to take
		 * a job.
		 */
		if (psinfo_matches_filter(info, filter)) {
			if (create_process_job(&info, &job_str) != 0) {
				(void) closedir(dirp);
			}
			lst_append(*job_list, job_str);
		}

	}

	(void) closedir(dirp);
	free(dent);
	return (0);
}


/*
 * type_to_match can be a parital e.g. SAMD
 * Then if str contains SAMDFSD a match has been found.
 */
boolean_t
job_type_matches(
char *type_to_match,
char *str)
{

	char *nm;

	/* get the numeric type based on process name */
	if (get_activity_type(str, &nm) == -1) {
		return (B_FALSE);
	}
	if (strstr(nm, type_to_match) != NULL) {
		return (B_TRUE);
	}

	return (B_FALSE);
}


/*
 * Returns the activity type and sets activity_type_name to
 * point to the name of the type. If the type is unknown a -1
 * is returned and activity_type_name is set to NULL.
 *
 * Do NOT free the activity type name it is statically allocated
 */
int
get_activity_type(
char *process_name,
char **activity_type_name)
{

	int i;

	for (i = 0; i < activitytypemax; i++) {

		if (strstr(process_name, activity_names[i]) != NULL) {
			*activity_type_name = activitytypes[i];
			return (i);
		}
	}

	*activity_type_name = NULL;
	return (-1);
}


#define	PROC_JOB_FMT "activityid=%d,starttime=%ld,details=%s,type=%s," \
	"parentid=%d,description=%s"
#define	PROC_JOB_FMT_NO_DESC "activityid=%d,starttime=%ld,details=%s," \
	"type=%s,parentid=%d"


/*
 * construct a proc_job_t from a psinfo_t struct
 */
int
create_process_job(
psinfo_t *info,
char **output)
{

	char buf[1024];
	char *nm_after_slashes;
	char *desc = NULL;
	char *type_name;
	int num_type = -1;

	if ((nm_after_slashes = strrchr(info->pr_psargs, '/')) == NULL) {
		nm_after_slashes = info->pr_psargs;
	} else {
		nm_after_slashes++;
	}

	num_type = get_activity_type(info->pr_psargs, &type_name);
	if (num_type != -1) {
		desc = GetCustMsg(SE_ACTIVITY_DESCRIPTION_BEGIN +
		    num_type - SAMD_FSD);

		snprintf(buf, sizeof (buf), PROC_JOB_FMT, info->pr_pid,
		    info->pr_start.tv_sec, nm_after_slashes, type_name,
		    info->pr_ppid, desc);
	} else {
		snprintf(buf, sizeof (buf), PROC_JOB_FMT_NO_DESC, info->pr_pid,
		    info->pr_start.tv_sec, nm_after_slashes,
		    UNKNOWN_ACTIVITY, info->pr_ppid, desc);
	}

	*output = strdup(buf);
	return (0);
}


/*
 * Cancel a SAM-FS job by killing the process specified
 */
int
destroy_process(
ctx_t *ctx,	/* ARGSUSED */
pid_t pid,
proctype_t ptype)
{
	char *pname = NULL;

	Trace(TR_MISC, "destroying process with pid: %ld and type: %d",
	    pid, ptype);

	switch (ptype) {
		case PTYPE_PROC_JOBS_BEGIN:
			pname = NULL;
			break;
		case PTYPE_OTHER_JOB:
			pname = NULL;
			break;
		case PTYPE_PROC_JOB:
			pname = NULL;
			break;

		default:
			if (ptype >= PTYPE_PROC_JOB || ptype < 0) {
				pname = NULL;
			} else {
				pname = proc_type_names[ptype];
			}
			break;
	}

	if (pname == NULL) {
		samerrno = SE_INVALID_PROCESS_TYPE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), ptype);
		goto err;
	}

	/* now kill the process */
	if (signal_process(pid, pname, SIGKILL) != 0) {
		samerrno = SE_KILL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), pid, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		goto err;
	}

	Trace(TR_MISC, "destroyed process");
	return (0);

err:
	Trace(TR_ERR, "destroying process failed %s", samerrmsg);
	return (-1);
}


/* filter is broken currently its an or */
boolean_t
psinfo_matches_filter(
psinfo_t info,
char *filter) {

	pid_t 	pid;
	pid_t	ppid;
	char	argstr[PRARGSZ];
	char	type[20];
	char	fname[PRFNSZ];
	boolean_t matches = B_TRUE;


	if (filter && filter[0] != '\0') {
		parse_job_filter(filter, &pid, &ppid, argstr, type, fname);

		if (pid != -1 && info.pr_pid != pid) {
			matches = B_FALSE;
		} else if (ppid != -1 && info.pr_ppid != ppid) {
			matches = B_FALSE;
		} else if (*argstr != '\0') {
			info.pr_psargs[PRARGSZ-1] = '\0';
			if (strstr(info.pr_psargs, argstr) == NULL) {
				matches = B_FALSE;
			}
		} else if (*type != '\0') {
			if (!job_type_matches(type, info.pr_psargs)) {
				matches = B_FALSE;
			}
		} else if (*fname != '\0') {
			info.pr_fname[PRARGSZ-1] = '\0';
			if (strstr(info.pr_fname, fname) == NULL) {
				matches = B_FALSE;
			}
		}
		return (matches);
	} else {
		/* Do not return anything in the absence of a filter */
		return (B_FALSE);
	}
}


/*
 * This could be made into a very general table based mechanism for
 * parsing comma separated name value pairs into lists of nvpair_t
 * structs, but for now keep it light.
 *
 * All strings will come back lower case.
 */
static int
parse_job_filter(
char *filter,
pid_t *pid,
pid_t *ppid,
char *argstr,	/* ptr to buffer at least PRARGSZ */
char *type,	/* ptr to buffer at least 20 20  chars */
char *fname) /* ptr to buffer at least PRFNSZ */
{


	char *ptr1, *ptr2, *ptr3, *ptr4;
	size_t blanks;
	int rval;
	char buffer[MAXPATHLEN];

	/* Copy string to parse, so we can destroy it */
	strlcpy(buffer, filter, sizeof (buffer));
	ptr2 = ptr1 = buffer;		/* Pointers into string */

	/* Initialize return values, in case string doesn't refer to them */
	*pid = -1;
	*ppid = -1;
	*argstr = '\0';
	*type = '\0';
	*fname = '\0';

	/*
	 * Parse keyword=value pairs, ignoring whitespace.
	 *
	 * ptr1 - start of keyword
	 * ptr2 - Next keyword (or null if end of string)
	 * ptr3 - start of value
	 * ptr4 - used to eliminate ending whitespace
	 */

	while (ptr1 != NULL) {
		blanks = strspn(ptr1, WHITESPACE);
		ptr1 += blanks;		/* Skip over whitespace */

		/* Find next key value pair */
		ptr2 = strchr(ptr1, ',');
		if (ptr2 != NULL) {
			*ptr2 = 0; /* Terminate current keyword string */
			ptr2++;	/* Point to beginning of next keyword */
		}

		/*
		 * Find value within keword=value pair
		 */
		ptr3 = strchr(ptr1, '=');
		if (ptr3 != NULL) {
			*ptr3 = 0; /* Terminate keyword */
			ptr3++;	/* Start at value */
			ptr3 += strspn(ptr3, WHITESPACE);
			ptr4 = strrspn(ptr3, WHITESPACE);
			if (ptr4 != NULL)
				*ptr4 = 0; /* Eliminate ending whitespace */
		} else {
			samerrno = SE_NOKEYVALUE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NOKEYVALUE), ptr1);
			return (-1);
		}

		ptr4 = strrspn(ptr1, WHITESPACE); /* Ending whitespace */
		if (ptr4 != NULL)
			*ptr4 = 0;

		/*
		 * Dispatch to individual value parsing routines
		 * Don't bother with a keyword table, since we are also
		 * passing different variables to each subroutine.
		 */

		if (strcasecmp(ptr1, "pid") == 0) {
			rval = parse_pid(ptr3, pid);
		} else if (strcasecmp(ptr1, "ppid") == 0) {
			rval = parse_pid(ptr3, ppid);
		} else	if (strcasecmp(ptr1, "argstr") == 0) {
			rval = parse_str(ptr3, argstr, PRARGSZ);
		} else if (strcasecmp(ptr1, "type") == 0) {
			int i = 0;

			rval = parse_str(ptr3, type, 20);

			/* types are upper case, make the filter so */
			for (i = 0; i < strlen(type); i++) {
				type[i] = toupper(type[i]);
			}

		} else if (strcasecmp(ptr1, "fname") == 0) {
			rval = parse_str(ptr3, fname, PRFNSZ);
		} else {
			samerrno = SE_NOTKEYWORD;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NOTKEYWORD), ptr1);
			return (-1);
		}

		if (rval)
			return (rval); /* If parse error, stop now */

		ptr1 = ptr2;	/* Step to next keyword */
	} /* End while */

	return (0);		/* Done, no error */
}


int
parse_str(
char *in,
char *out,
int bufsize)
{
	strlcpy(out, in, bufsize);
	return (0);
}


int
parse_pid(
char *in,
pid_t *out)
{

	sscanf(in, "%ld", out);
	return (0);
}


int
wait_for_proc_start(
char		*filter,	/* same format as get_process_jobs filters */
int		max_wait,	/* Maximum amount of time to wait */
int		interval)	/* How often to check for process */
{

	sqm_lst_t *l;
	int time_waited = 0;

	while (time_waited < max_wait) {

		if (get_process_jobs(NULL, filter, &l) != 0) {
			return (-1);
		}

		if (l->length == 0) {
			time_waited += interval;
			sleep(interval);
		} else {

			/*
			 * This is a blunt instrument. We return because we
			 * found one or more jobs that match.
			 */
			Trace(TR_MISC, "found proc job = %s",
			    (char *)l->head->data);

			lst_free_deep(l);
			return (0);
		}
		lst_free_deep(l);
	}

	Trace(TR_MISC, "wait for process matching filter %s expired %d secs",
	    filter, max_wait);

	return (-1);
}
