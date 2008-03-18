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
#pragma ident   "$Revision: 1.67 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * various utility functions used by the SAM-FS/QFS management library
 * (libsammgmt)
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/mnttab.h>
#include <sys/vfstab.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <procfs.h>
#include <dirent.h>
#include <signal.h>

#include "mgmt/util.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/file_util.h"
#include "mgmt/config/common.h"

#include "sam/sam_trace.h"

#include <pthread.h>

#define	GET_DAEMONS_CMD "/usr/proc/bin/ptree `/bin/pgrep sam-fsd`"
#define	MAIL_CMD "/usr/bin/mailx"

#define	BUFFSIZ MAXPATHLEN

static int samd_stop(ctx_t *);
static int samd_config(ctx_t *);
static int samd_start(ctx_t *);

extern char *getfullrawname();


/* forward declarations */
static int write_buf(int fd, void* buffer, int len);

/*
 * Set up thread-specific data key
 * The key value is malloc()ed, so ensure free() is called
 * when the thread exits.
 */
static pthread_key_t  global_data_key;
static pthread_once_t tsd_once = PTHREAD_ONCE_INIT;
static void tsd_init(void);
static void tsd_destroy(void* data);

static void
tsd_init(void)
{
	Trace(TR_DEBUG, "tsd_init creating key for thread %d, pid %ld",
	    pthread_self(), getpid());
	pthread_key_create(&global_data_key, tsd_destroy);
}

static void
tsd_destroy(void* data)
{
	Trace(TR_DEBUG, "tsd_destroy removing data for thread %d, pid %ld",
	    pthread_self(), getpid());
	if (data == NULL) {
		Trace(TR_DEBUG, "tsd_destroy data is NULL");
	} else {
		free(data);
	}
}

/*
 * Function to return the thread specific data for
 * error message handling.
 *
 * Values for "which":
 *	1		returns the error number (int)
 * 	2		returns the error message buffer (char*)
 *			Buffer size is MAX_MSG_LEN.
 *
 * Return values
 * This function is intended to be called by the macros
 * samerrno and samerrmsg, but could used directly.
 *
 */
void*
fsm_err_tsd(int which)
{
	err_struct_t   *err_tsd = NULL;

	/* initialize the key, if it hasn't been already */
	pthread_once(&tsd_once, tsd_init);

	err_tsd = pthread_getspecific(global_data_key);
	if (err_tsd == NULL) {		/* not yet set */
		/* allocate an extra byte so message is always truncated */
		Trace(TR_DEBUG, "fsm_err_tsd alloc buf for thread %d pid %ld",
		    pthread_self(), getpid());
		err_tsd = calloc(1, sizeof (err_struct_t) + 1);
		pthread_setspecific(global_data_key, err_tsd);
	}

	/* something very bad must have happened... */
	if (err_tsd == NULL) {
		abort();
	}

	switch (which) {
		case 1:
			return (&err_tsd->errcode);
			break;		/* NOTREACHED */
		case 2:
			return (err_tsd->errmsg);
			break;		/* NOTREACHED */
	}
	return (NULL);
}

/*
 * return 1 if an NULL argument is found in the list, 0 otherwise
 * set samerrno and samerrmsg accordingly.
 */
int
isnull(char *argnames, ...)
{

	va_list ap;
	char delim[] = ", ";
	char *crtname;
	char *names;
	char *rest;

	names = strdup(argnames);
	crtname = strtok_r(names, delim, &rest);
	va_start(ap, argnames);
	while (crtname) {
		if (NULL == va_arg(ap, void *)) {
			samerrno = SE_NULL_PARAMETER;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NULL_PARAMETER), crtname);
			va_end(ap);
			free(names);
			return (1);
		}
		crtname = strtok_r(NULL, delim, &rest);
	}
	free(names);
	va_end(ap);
	return (0);
}


/* malloc + error checking */
void *
mallocer(size_t size)
{

	void *m = calloc(1, size);

	if (NULL == m)
		setsamerr(SE_NO_MEM);
	return (m);
}


/*
 * Similar to popen() but returns pointers to both stdout and stderr streams.
 * return child's PID or -1 if cmdstr is null or an error is encountered
 * exec-ing child.
 */
pid_t
exec_get_output(
const char *cmdstr,
FILE **stdout_stream, // if NULL then stdout ignored
FILE **stderr_stream) // if NULL then stderr ignored
{

	pid_t pid;		// for the shell command
	int fdo[2] = {-1, -1};	// pipe used for reading command's stdout
	int fde[2] = {-1, -1};	// pipe used for reading command's stderr

	Trace(TR_DEBUG, "will fork-exec new command");

	if (ISNULL(cmdstr)) {
		return (-1);
	}

	if (stdout_stream != NULL) {
		if (pipe(fdo) < 0) {
			Trace(TR_ERR, "stdout pipe creation failed:%s",
				Str(strerror(errno)));
			samerrno = SE_CANNOT_CREATE_PIPE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), "");
			strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
			return (-1);
		}
	}

	if (stderr_stream != NULL) {
		if (pipe(fde) < 0) {
			Trace(TR_ERR, "stderr pipe creation failed:%s",
				Str(strerror(errno)));
			samerrno = SE_CANNOT_CREATE_PIPE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), "");
			strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

			if (fdo[0] != -1) {
				close(fdo[0]);
			}
			if (fdo[1] != -1) {
				close(fdo[1]);
			}

			return (-1);
		}
	}

	if ((pid = fork()) < 0) {
		Trace(TR_ERR, "fork failed:%s", Str(strerror(errno)));
		samerrno = SE_FORK_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		if (fdo[0] != -1) {
			close(fdo[0]);
		}
		if (fdo[1] != -1) {
			close(fdo[1]);
		}
		if (fde[0] != -1) {
			close(fde[0]);
		}
		if (fde[1] != -1) {
			close(fde[1]);
		}
		return (-1);
	}

	if (pid == 0) {
		/* child */

		/*
		 * Redirect stdout and stderr and close the reading end of
		 * the pipes since child does not need them.
		 *
		 * if either stdout_stream or stderr_stream are NULL,
		 * redirect output to /dev/null (i.e., suppress it).
		 */
		if (stdout_stream == NULL) {
			fdo[1] = open("/dev/null", O_WRONLY);
		}

		if (stderr_stream == NULL) {
			fde[1] = open("/dev/null", O_WRONLY);
		}

		if ((fdo[1] == -1) || (fde[1] == -1)) {
			Trace(TR_DEBUG,
				"exec_get_output: pipes not open");
			exit(1);
		}

		dup2(fdo[1], STDOUT_FILENO);
		dup2(fde[1], STDERR_FILENO);

		/* close the ends of the pipes we're not using */
		if (fdo[0] != -1) {
			close(fdo[0]);
		}
		if (fde[0] != -1) {
			close(fde[0]);
		}

		// exec & send the output to filedescriptor fd[1]

		Trace(TR_DEBUG, "exec-ing sh -c \"%s\"", cmdstr);

		/* Close stdin to prevent child processes from inheriting it. */
		close(STDIN_FILENO);

		/* close open fds > stderr */
		closefrom(3);

		if (-1 == execl("/usr/bin/sh", "sh", "-c", cmdstr, NULL)) {
			exit(1);
		}
	}

	/*
	 * parent
	 * close the writing ends of the pipes.
	 */
	if (stdout_stream != NULL) {
		close(fdo[1]);
		*stdout_stream = fdopen(fdo[0], "r");
	}

	if (stderr_stream != NULL) {
		close(fde[1]);
		*stderr_stream = fdopen(fde[0], "r");
	}

	Trace(TR_PROC, "cmd exec-ed by process %ld", pid);
	return (pid);
}


/*
 * From a path and a file name get the full path to the file.
 * This function adds a slash at the end of the path if it is needed.
 * The result is written into a static array.
 * If time_ext is true the current time will be appended as a file extension.
 */
char *
assemble_full_path(
char *path,		/* The path to the file */
char *file,		/* The file name */
boolean_t time_ext,	/* if true .current_time is appended to file name */
upath_t res_path)	/* The result */
{
	char *result;
	static char full_path[160];
	time_t the_time;
	char time_str[32] = "";

	if (res_path == NULL) {
		Trace(TR_OPRMSG, "setting result to full_path");
		result = full_path;
	} else {
		Trace(TR_OPRMSG, "setting result to input ");
		result = res_path;
	}
	if (time_ext) {
		the_time = time(0);
		snprintf(time_str, sizeof (time_str), ".%ld", the_time);
	}

	if (ENDSWITHSLASH(path)) {
		snprintf(result, sizeof (full_path), "%s%s%s", path, file,
		    time_ext ? time_str : "");
	} else {

		Trace(TR_OPRMSG, "%s%s%s%s", path, "/", file,
		    time_ext ? time_str : "");

		snprintf(result, sizeof (full_path), "%s%s%s%s", path, "/",
		    file, time_ext ? time_str : "");
	}
	return (result);
}


/*
 * init_config()
 * This function will call samd config to initialize the system
 * to see changes to the config files. It then calls samd start to ensure
 * that all daemons are running.
 */
int
init_config(
ctx_t *ctx	/* ARGSUSED */)
{
	Trace(TR_DEBUG, "initializing configuration");

	if (samd_config(ctx) != 0) {
		Trace(TR_ERR, "initializing configuration failed: %d, %s",
		    samerrno, samerrmsg);
		return (-1);

	}

	Trace(TR_MISC, "called samd config");

	if (samd_start(ctx) != 0) {
		Trace(TR_ERR, "initializing configuration failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "initialized configuration");
	return (0);
}


/*
 * init_library_config()
 * This function will call samd stop and samd start to restart the library
 * and drive configuration after MCF has been changed.
 */
int
init_library_config(
ctx_t *ctx	/* ARGSUSED */)
{
	Trace(TR_DEBUG, "initializing mcf configuration");
	if (samd_stop(ctx) != 0) {
		Trace(TR_ERR,
		    "initializing mcf configuration: %d, %s",
		    samerrno, samerrmsg);
		return (-1);

	}

	sleep(2);
	if (samd_config(ctx) != 0) {
		Trace(TR_ERR,
		    "initializing mcf configuration failed: %d, %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	sleep(1);
	if (samd_start(ctx) != 0) {
		Trace(TR_ERR,
		    "initializing mcf configuration failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "initialized mcf configuration");
	return (0);
}


int
samd_stop(
ctx_t *ctx	/* ARGSUSED */)
{

	pid_t pid;
	char *samd_stop = SBIN_DIR"/samd stop";
	int status;

	pid = exec_get_output(samd_stop, NULL, NULL);


	if (pid < 0) {
		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FORK_EXEC_FAILED),
		    samd_stop);

		Trace(TR_ERR, "calling samd stop: %s", samerrmsg);
		return (-1);
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "calling samd stop failed (waitpid failed)");
	} else {
		Trace(TR_ERR, "calling samd stop status: pid %ld %d\n",
		    pid, status);
	}


	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_SAMD_STOP_FAILED;
			Trace(TR_ERR, "calling samd stop exit code: %d\n",
			    WEXITSTATUS(status));

		} else {
			Trace(TR_MISC, "calling samd stop done");
			return (0);
		}
	} else {
		samerrno = SE_SAMD_STOP_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_SAMD_STOP_FAILED),
		    "abnormally terminated/stopped");
		return (-1);
	}
	return (0);
}


int
samd_config(
ctx_t *ctx	/* ARGSUSED */)
{

	pid_t pid;
	char *samd_config = SBIN_DIR"/samd config";
	int status;

	pid = exec_get_output(samd_config, NULL, NULL);


	if (pid < 0) {
		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FORK_EXEC_FAILED),
		    samd_config);

		Trace(TR_ERR, "calling samd config: %s", samerrmsg);
		return (-1);
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "calling samd config failed (waitpid failed)");
	} else {
		Trace(TR_ERR, "calling samd config status: pid %ld %d\n",
		    pid, status);
	}


	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_SAMD_CONFIG_FAILED;
			Trace(TR_ERR, "calling samd config exit code: %d\n",
			    WEXITSTATUS(status));

		} else {
			Trace(TR_MISC, "calling samd config done");
			return (0);
		}
	} else {
		samerrno = SE_SAMD_CONFIG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_SAMD_CONFIG_FAILED),
		    "abnormally terminated/stopped");
		return (-1);
	}

	return (0);
}

int
samd_start(
ctx_t *ctx	/* ARGSUSED */)
{
	pid_t pid;
	char *samd_start = SBIN_DIR"/samd start";
	int status;

	pid = exec_get_output(samd_start, NULL, NULL);

	if (pid < 0) {
		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FORK_EXEC_FAILED),
		    samd_start);

		Trace(TR_ERR, "calling samd start failed: %s", samerrmsg);
		return (-1);
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "waitpid failed for samd start");
	} else {
		Trace(TR_OPRMSG, "samd start status: pid %ld %d\n",
		    pid, status);
	}

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			char code[10];

			snprintf(code, sizeof (code), "%d",
			    WEXITSTATUS(status));

			samerrno = SE_SAMD_START_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_SAMD_START_FAILED), code);

			Trace(TR_ERR, "calling samd start failed: %s",
			    samerrmsg);

		} else {
			Trace(TR_MISC, "calling samd start done");
			return (0);
		}
	} else {
		samerrno = SE_SAMD_START_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_SAMD_START_FAILED),
		    "abnormally terminated/stopped");
		return (-1);
	}
	return (0);
}



int
cp_file(
	const char *old,
	const char *new)
{
	int		oldfd = -1;
	int		newfd = -1;
	struct stat	oldstatbuf;
	struct stat	newstatbuf;
	int		res;
	int		saverr = 0;
	struct timeval	oldtimes[2];
	char		buf[8192];
	int		wlen;
	int		oldlen;

	Trace(TR_DEBUG, "copying file %s to %s", old, new);

	/* make sure old exists */
	res = stat(old, &oldstatbuf);
	if (res != 0) {
		Trace(TR_DEBUG, "cp_file: old file %s does not exist", old);
		samerrno = SE_COPY_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_COPY_FAILED), old, "open failed");
		return (-1);
	}

	/* if the target exists, remove it */
	res = stat(new, &newstatbuf);
	if (res == 0) {
		Trace(TR_DEBUG, "cp_file: removing %s", new);
		unlink(new);
	}

	/* save the access & mod times so they can be reset */
	oldtimes[0].tv_sec = oldstatbuf.st_atim.tv_sec;
	oldtimes[0].tv_usec = oldstatbuf.st_atim.tv_nsec / 1000;
	oldtimes[1].tv_sec = oldstatbuf.st_mtim.tv_sec;
	oldtimes[1].tv_usec = oldstatbuf.st_mtim.tv_nsec / 1000;

	oldfd = open(old, O_RDONLY, oldstatbuf.st_mode);
	if (oldfd == -1) {
		Trace(TR_ERR, "cp_file: could not open %s", old);
		samerrno = SE_COPY_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_COPY_FAILED), old, "open failed");
		return (-1);
	}
	newfd = open(new, O_WRONLY|O_CREAT|O_EXCL, oldstatbuf.st_mode);
	if (newfd == -1) {
		Trace(TR_ERR, "cp_file: could not open %s", new);
		close(oldfd);
		samerrno = SE_COPY_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_COPY_FAILED), new, "open failed");
		return (-1);
	}

	/* finally, copy the file */
	res = 0;
	oldlen = oldstatbuf.st_size;

	while (oldlen > 0) {
		if (oldlen < 8192) {
			wlen = oldlen;
		} else {
			wlen = 8192;
		}

		res = readbuf(oldfd, buf, wlen);
		if (res == -1) {
			saverr = errno;
			Trace(TR_ERR, "cp_file: readbuf failed %d", saverr);
			samerrno = SE_COPY_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_COPY_FAILED), old, "readbuf failed");
			break;
		}

		res = write_buf(newfd, buf, wlen);
		if (res == -1) {
			saverr = errno;
			Trace(TR_ERR, "cp_file: write_buf failed %d", saverr);
			samerrno = SE_COPY_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_COPY_FAILED), old,
			    "write_buf failed");
			break;
		}

		oldlen -= wlen;
	}

	close(newfd);
	close(oldfd);

	/* set acccess & modify times to match original file */
	if (saverr == 0) {
		utimes(new, oldtimes);
		res = 0;
	} else {
		res = -1;
	}

	return (res);
}

int
get_running_sam_daemons(
sqm_lst_t **dnames)
{

	sqm_lst_t *daemons = *dnames = lst_create();
	int status;
	pid_t pid;
	char line[80];
	FILE *res_stream;

	pid = exec_get_output(GET_DAEMONS_CMD, &res_stream, NULL);
	if ((pid = waitpid(pid, &status, 0)) < 0) {
		return (-1);
	} else {
		// check child's exit code
		if (status > 1) {
			return (-1);
		}
	}
	while (NULL != fgets(line, 80, res_stream)) {
		size_t len = strlen(line);
		if (len) {
			line[len - 1] = '\0'; // remove '\n'
		}
		lst_append(daemons, (char *)strdup(line));
	}
	fclose(res_stream);
	return (0);
}


int
send_mail(
char *recipient,
char *subject,
char *msg)
{

	FILE *res_stream = NULL, *err_stream = NULL;
	char cmd[1024];
	pid_t pid = 0;
	int status = 0;

	snprintf(cmd, sizeof (cmd), "%s -s \"%s\" %s <<EOF\n%s\nEOF",
	    MAIL_CMD, subject, recipient, msg);

	pid = exec_get_output(cmd, &res_stream, &err_stream);
	if ((pid = waitpid(pid, &status, 0)) < 0) {
		return (-1);
	} else {
		if (status > 1) {
			return (-1);
		}
	}
	/* TBD: Parse err_stream */
	fclose(res_stream);
	fclose(err_stream);
	return (0);
}


/*
 * Check if there is a network problem between the client and the server
 * Use ping
 *
 * ping server timeout
 *
 * If network is reachable, ping returns 0
 * If network is unreachable, ping returns non-zero
 *
 * Return value:
 * for internal failure i.e. fork, popen etc.: return -1
 * ping to host fails: non-zero as returned by ping
 * ping to server succeeds: return 0
 *
 */
int
nw_down(char *svr)
{
	char cmd[BUFSIZ]	= {0};
	char buf[BUFSIZ]	= {0};
	FILE *ptr		= NULL;
	int exit_status		= -1;

	if (ISNULL(svr)) {
		return (-1);
	}
	snprintf(cmd, sizeof (cmd), "%s %s %d",
	    "/usr/sbin/ping", svr, 3);

	/*
	 * The following exit values are returned from ping:
	 *
	 * 0		Successful operation; the machine is alive.
	 * non-zero	An error has occurred. Either  a  malformed
	 *		argument  has been specified, or the machine
	 *		was not alive.
	 */

	if ((ptr = popen(cmd, "r")) != NULL) {
		while (fgets(buf, BUFSIZ, ptr) != NULL)
			Trace(TR_DEBUG, "%s", buf);
		/*
		 * pclose() returns the termination status of the cli
		 * as returned by waitpid, otherwise it returns -1
		 */
		exit_status = pclose(ptr);
		Trace(TR_DEBUG, "pclose return %d", exit_status);
	}
	return (exit_status);
}

/* function to copy a string and set samerrno on failure */
char *
copystr(char *ptr) {
	char *strp;
	int len;

	if (ISNULL(ptr)) {
		return (NULL);
	}
	len = strlen(ptr) + 1;

	strp = mallocer(len);
	if (strp != NULL)
		strlcpy(strp, ptr, len);
	return (strp);
}

/*
 * copyint - Given an integer, allocate storage and copy it.
 */
int *
copyint(int value) {
	int *intp;

	intp = mallocer(sizeof (int));
	if (intp != NULL)
		*intp = value;
	return (intp);
}

/*
 * samrerr -	Utility routine to set samerrmsg and samerrno.
 *		Presumes a single text string argument.
 */

int
samrerr(int errnum, char *text) {
	samerrno = errnum;
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(errnum), text);
	return (-1);		/* Generic "failure" */
}

/*
 * getfsmountpt - find current mount point for a particular filesystem
 *		  by searching through /etc/mnttab
 */

int
getfsmountpt(
	char 	*fsname,	/* IN, file system name */
	char 	*mountpt,	/* IN/OUT, buffer for mount point path */
	size_t	len)		/* IN, size of buffer */
{
	FILE *mnttab;
	struct mnttab mnt;
	struct mnttab inmnt;
	int ret;

	mnttab = fopen64(MNTTAB, "r");
	if (mnttab == NULL) {
		return (samrerr(SE_NOSUCHPATH, MNTTAB));
	}
	rewind(mnttab);		/* ensure set to top of file */

	/* set up the mnttab struct for the filesystem we're looking for */
	memset(&inmnt, 0, sizeof (struct mnttab));
	inmnt.mnt_special = fsname;

	ret = getmntany(mnttab, &mnt, &inmnt);

	fclose(mnttab);

	if (ret != 0) {		/* no match */
		return (samrerr(SE_NOSUCHFS, fsname));
	}

	strlcpy(mountpt, mnt.mnt_mountp, len);
	return (0);
}

/*
 * getvfsmountpt - find mount point for a filesystem that is not mounted
 *		   by searching through /etc/vfstab
 */

int
getvfsmountpt(
	char 	*fsname,	/* IN, file system name */
	char 	*mountpt,	/* IN/OUT, buffer for mount point path */
	size_t	len)		/* IN, size of buffer */
{
	FILE *fp;
	struct vfstab mnt;
	struct vfstab inmnt;
	int ret;

	fp = fopen64(VFSTAB, "r");
	if (fp == NULL) {
		return (samrerr(SE_NOSUCHPATH, VFSTAB));
	}
	rewind(fp);		/* ensure set to top of file */

	/* set up the vfstab struct for the filesystem we're looking for */
	memset(&inmnt, 0, sizeof (struct vfstab));
	inmnt.vfs_special = fsname;

	ret = getvfsany(fp, &mnt, &inmnt);

	fclose(fp);

	if (ret != 0) {		/* no match */
		return (samrerr(SE_NOSUCHFS, fsname));
	}

	strlcpy(mountpt, mnt.vfs_mountp, len);
	return (0);
}


int
parsekv_mallocstr(char *ptr, void *bufp) {
	char **result = (char **)bufp;

	if (ptr == NULL) {
		return (0);
	}
	if (result == NULL) {
		return (-1);
	}
	if (*ptr == '\0') {
		*result = NULL;
		return (0);
	}
	*result = strdup(ptr);
	if (*result == NULL) {
		return (-1);
	}
	return (0);
}



/*
 * Routine to parse comma-separated keyword=value pairs.
 */

int
parse_kv(char *str, parsekv_t *tokens, void *buf)
{
	char *ptr1, *ptr2, *ptr3, *ptr4;
	int rval;
	size_t blanks;
	char *buffer = NULL;
	parsekv_t *tokenptr;

	rval = 0;

	if ((str == NULL) || (strlen(str) == 0)) {
		return (0);	/* No string, done parsing */
	}

	/* copy the input string so we can hack it to pieces */
	buffer = copystr(str);
	if (buffer == NULL) {
		return (-1);
	}

	/*
	 * Parse keyword=value pairs, ignoring whitespace.
	 *
	 * ptr1 - start of keyword
	 * ptr2 - Next keyword (or null if end of string)
	 * ptr3 - start of value
	 * ptr4 - used to eliminate ending whitespace
	 */

	ptr1 = buffer;
	while (ptr1 != NULL) {
		blanks = strspn(ptr1, WHITESPACE);
		ptr1 += blanks;	/* Skip over whitespace */

		/* Find next keyword */
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
			ptr3++; /* Start at value */
			ptr3 += strspn(ptr3, WHITESPACE);
			ptr4 = strrspn(ptr3, WHITESPACE);
			if (ptr4 != NULL)
				*ptr4 = 0; /* Eliminate ending whitespace */
		} else {
			free(buffer);

			return (samrerr(SE_NOKEYVALUE, ptr1));
		}

		ptr4 = strrspn(ptr1, WHITESPACE); /* Ending whitespace */
		if (ptr4 != NULL)
			*ptr4 = 0;

		for (tokenptr = tokens; tokenptr->keyword[0] != 0; tokenptr++) {

			/* if it is not the right keyword continue */
			if (strcasecmp(ptr1, tokenptr->keyword) != 0) {
				continue;
			}
			if ((tokenptr->method) != parsekv_mallocstr) {
				void *bufptr =
				    (char *)buf + (tokenptr->offset);
				rval = (tokenptr->method)(ptr3, bufptr);
				break;
			} else {
				/*
				 * The location in the struct refers to
				 * a pointer. Passing in it's address
				 * allows setting the pointers value.
				 */
				void *ptr = (char *)buf + (tokenptr->offset);
				rval = (tokenptr->method)(ptr3, ptr);
			}
		}

		if (rval) {
			free(buffer);
			return (rval);
		}

		ptr1 = ptr2;    /* Step to next keyword */
	} /* End while */

	free(buffer);
	return (0);		/* Done, no error */
}

int
parsekv_int(char *ptr, void *bufp)
{
	int	*valuep = (int *)bufp;

	*valuep = atoi(ptr);

	return (0);
}

int
parsekv_ll(char *ptr, void *bufp)
{
	long long *valuep;

	valuep = (long long *)(bufp);
	sscanf(ptr, "%lld", valuep);

	return (0);
}

int
parsekv_llu(char *ptr, void *bufp)
{
	unsigned long long *valuep;

	valuep = (unsigned long long *)(bufp);
	sscanf(ptr, "%llu", valuep);

	return (0);
}

int
parsekv_time(char *ptr, void *bufp)
{
	time_t *valuep;
	long datum;

	valuep = (time_t *)(bufp);
	sscanf(ptr, "%ld", &datum);
	*valuep = datum;

	return (0);
}

int
parsekv_string_3(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 3);
	return (0);
}

int
parsekv_string_5(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 5);
	return (0);
}

int
parsekv_string_16(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 16);
	return (0);
}

int
parsekv_string_32(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 32);
	return (0);
}

int
parsekv_string_256(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 256);
	return (0);
}

int
parsekv_string_1024(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 1024);
	return (0);
}

int
parsekv_string_1072(char *ptr, void *bufp) {
	strlcpy(bufp, ptr, 1072);
	return (0);
}


int
parsekv_bool(char *ptr, void *bufp) {

	boolean_t *valuep = (boolean_t *)bufp;

	if (strcmp(ptr, "1") == 0) {
		*valuep = TRUE;
	} else {
		*valuep = FALSE;
	}

	return (0);
}

int
parsekv_bool_YN(char *ptr, void *bufp) {

	boolean_t *valuep = (boolean_t *)bufp;
	if ((*ptr == 'Y') || (*ptr == 'y')) {
		*valuep = TRUE;
	} else {
		*valuep = FALSE;
	}
	return (0);
}

/*
 *  Returns the user name equivalent of the UID.  If the username
 *  cannot be resolved, returns the UID in ascii form.
 */
int
get_user_name(uid_t uid, char *res, int reslen) {
	char uidbuff[BUFFSIZ];
	struct passwd pwd, *pwdp;

	setpwent();
#ifndef _POSIX_PTHREAD_SEMANTICS
	pwdp = getpwuid_r(uid, &pwd, uidbuff, BUFFSIZ);
#else
	getpwuid_r(uid, &pwd, uidbuff, BUFFSIZ, &pwdp);
#endif
	endpwent();

	if (pwdp != NULL) {
		strlcpy(res, pwd.pw_name, reslen);
	} else {
		snprintf(res, reslen, "%d", uid);
	}

	return (0);
}

/*
 *  Returns the group name equivalent of the GID.  If the name
 *  cannot be resolved, returns the GID in ascii form.
 */
int
get_group_name(gid_t uid, char *res, int reslen) {
	char gidbuff[BUFFSIZ];
	struct group gr, *grp;

	setgrent();
#ifndef _POSIX_PTHREAD_SEMANTICS
	grp = getgrgid_r(uid, &gr, gidbuff, BUFFSIZ);
#else
	getgrgid_r(uid, &gr, gidbuff, BUFFSIZ, &grp);
#endif
	endgrent();

	if (grp != NULL) {
		strlcpy(res, gr.gr_name, reslen);
	} else {
		snprintf(res, reslen, "%d", uid);
	}

	return (0);
}


/*
 * Parse text username into uid.
 */

int
parse_userid(char *ptr, void *bufp) {
	char uidbuff[BUFFSIZ];
	struct passwd pwd, *pwdp;
	uid_t *uidp = bufp;

	setpwent();
#ifndef _POSIX_PTHREAD_SEMANTICS
	pwdp = getpwnam_r(ptr, &pwd, uidbuff, BUFSIZ);
#else
	getpwnam_r(ptr, &pwd, uidbuff, BUFSIZ, &pwdp);
#endif
	endpwent();

	if (pwdp == NULL)
		return (samrerr(SE_NOSUCHUSER, ptr));

	*uidp = pwd.pw_uid;

	return (0);
}

/*
 * Parse text groupname into gid.
 */
int
parse_gid(char *ptr, void *bufp) {
	char gidbuff[BUFFSIZ];
	struct group grp, *grpp;
	gid_t *gidp = bufp;

	setgrent();
#ifndef _POSIX_PTHREAD_SEMANTICS
	grpp = getgrnam_r(ptr, &grp, gidbuff, BUFSIZ);
#else
	getgrnam_r(ptr, &grp, gidbuff, BUFSIZ, &grpp);
#endif
	endgrent();

	if (grpp == NULL)
		return (samrerr(SE_NOSUCHGROUP, ptr));

	*gidp = grp.gr_gid;
	return (0);
}

int
translate_period(char *input, char *allowed_units,
    uint32_t *value, char *unit) {

	char *end;


	if (ISNULL(input, allowed_units, value, unit)) {
		return (-1);
	}
	errno = 0;
	if ((*value = strtoul(input, &end, 10)) == 0) {
		if (errno != 0) {
			samerrno = SE_INVALID_PERIOD;
			snprintf(samerrmsg,  MAX_MSG_LEN, GetCustMsg(samerrno),
			    input);
			Trace(TR_ERR, "failing with %s\n", input);

			return (-1);
		}
	}
	if (end == input || end == NULL) {
		samerrno = SE_INVALID_PERIOD;
		snprintf(samerrmsg,  MAX_MSG_LEN, GetCustMsg(samerrno),
		    input);
		Trace(TR_ERR, "failing with %s\n", input);
		return (-1);
	}

	if (strspn(end, allowed_units) != 1) {
		samerrno = SE_INVALID_PERIOD;
		snprintf(samerrmsg,  MAX_MSG_LEN, GetCustMsg(samerrno),
			input);

		Trace(TR_ERR, "failing with %s\n", input);
		/* either too many of the units or not enough */
		return (-1);
	}
	*unit = *end;

	if (*(++end) != '\0') {
		/*
		 * the first char past digits was a valid unit
		 * but it was not the end of the string
		 */
		samerrno = SE_INVALID_PERIOD;
		snprintf(samerrmsg,  MAX_MSG_LEN, GetCustMsg(samerrno),
			input);
		Trace(TR_ERR, "failing with %s\n", input);
		return (-1);
	}
	return (0);
}

/*
 *  Takes a date string of the form YYYYMMDDhhmm and converts it
 *  to a struct tm (time structure).
 *
 *  Due to limitations with mktime(), this function will not work
 *  with dates >  03:14:07 UTC,  January 19, 2038.  Hopefully mktime()
 *  will get fixed before this becomes an issue for us.
 */
int
translate_date(char *indate, struct tm *outtime)
{
	char		*fmt = "%Y%m%d%H%M";
	time_t		tmpTime;

	if (ISNULL(indate, outtime)) {
		return (-1);
	}

	memset(outtime, 0, sizeof (struct tm));

	if ((strptime(indate, fmt, outtime)) == NULL) {
		samerrno = SE_INVALID_DATESPEC;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    indate);
		return (-1);
	} else {
		/* weekday and timezone not set with strptime */
		outtime->tm_isdst = -1;
		/* convert to seconds */
		tmpTime = mktime(outtime);
		/* convert back to struct tm */
		localtime_r(&tmpTime, outtime);

		return (0);
	}
}

/*
 * Turns a mode returned from the stat() family of functions
 * into a string.  Buffer "prots" must be at least 11 characters long.
 */
int
modeToString(mode_t mode, char *prots)
{
	if (prots == NULL) {
		return (-1);
	}

	/* File type character */
	switch (mode & S_IFMT) {
		case S_IFDIR:	{ prots[0] = 'd'; break; } /* Directory */
		case S_IFDOOR:	{ prots[0] = 'D'; break; } /* door */
		case S_IFLNK:	{ prots[0] = 'l'; break; } /* softlink */
		case S_IFBLK:	{ prots[0] = 'b'; break; } /* block special */
		case S_IFCHR:	{ prots[0] = 'c'; break; } /* char special */
		case S_IFIFO:	{ prots[0] = 'p'; break; } /* FIFO */
#ifdef S_IFPORT
		/*
		 * Only available as of Solaris 10. ifdef can be removed
		 * when support for Solaris 9 is removed.
		 */
		case S_IFPORT:	{ prots[0] = 'P'; break; } /* event port */
#endif
		case S_IFSOCK:	{ prots[0] = 's'; break; } /* AF_UNIX socket */
		case S_IFREG:	{ prots[0] = '-'; break; } /* ordinary file */
		default:	{ prots[0] = '?'; break; } /* Dunno */
	}

	/* read-write-execute permissions */
	prots[1] =  (mode & S_IRUSR) ? 'r' : '-';
	prots[2] =  (mode & S_IWUSR) ? 'w' : '-';
	prots[3] =  (mode & S_IXUSR) ? 'x' : '-';
	prots[4] =  (mode & S_IRGRP) ? 'r' : '-';
	prots[5] =  (mode & S_IWGRP) ? 'w' : '-';
	prots[6] =  (mode & S_IXGRP) ? 'x' : '-';
	prots[7] =  (mode & S_IROTH) ? 'r' : '-';
	prots[8] =  (mode & S_IWOTH) ? 'w' : '-';
	prots[9] =  (mode & S_IXOTH) ? 'x' : '-';
	prots[10] = '\0';

	/* sticky bit and setuid/setgid */
	if (mode & S_ISUID) {
		prots[3] = (mode & S_IXUSR) ? 's' : 'S';
	}

	if (mode & S_ISGID) {
		prots[6] = (mode & S_IXGRP) ? 's' : 'S';
	}

	if (mode & S_ISVTX) {
		prots[9] = (mode & S_IXOTH) ? 't' : 'T';
	}

	return (0);
}

/*
 * Converts a block device path to a raw device path and returns a malloced
 * result. If devpath cannot be converted -1 is returned and *rdskpath is set
 * to null.
 */
int
dsk2rdsk(
	const char *devpath,	/* IN  - block device */
	char **rdskpath)	/* OUT - raw device */
{

	if (ISNULL(devpath, rdskpath)) {
		return (-1);
	}

	*rdskpath = getfullrawname(devpath);
	if (*rdskpath == NULL) {
                Trace(TR_ERR, "out of memory in getfullrawname");
                return (-1);
        }
	if (strcmp(*rdskpath, "\0") == 0) {
                Trace(TR_ERR, "Could not convert dsk to rawdsk");
		free(*rdskpath);
		*rdskpath = NULL;
                return (-1);
        }

	Trace(TR_DEBUG, "dsk2rdsk:  returned %s", *rdskpath);

	return (0);
}

/*
 * function str_trim() deletes leading and
 * trailing space.
 */
char *
str_trim(
char *s,	/* incoming string */
char *out_str)	/* output string */
{
	int l, r, c;

	if (ISNULL(s, out_str)) {
		return (NULL);
	}

	for (r = strlen(s)-1; r >= 0; r--) {
		if (!isspace(s[r])) {
			break;
		}
	}

	for (l = 0; s[l] != '\0'; l++) {
		if (!isspace(s[l])) {
			break;
		}
	}

	if (l > r) {
		out_str[0] = '\0';
	} else {
		for (c = l; c <= r; c++) {
			out_str[c-l] = s[c];
		}
		out_str[r-l+1] = '\0';
	}
	return (out_str);
}

/*
 * find_process
 *
 * Function to read through /proc and find all processes with
 * the specified executable name.
 *
 */
int
find_process(
	char *exename,		/* IN  - name of executable */
	sqm_lst_t **procs)		/* OUT - list of psinfo_t structs */
{
	DIR		*dirp;
	dirent64_t	*dent;
	dirent64_t	*dentp;
	char		pname[MAXPATHLEN];
	char		*ptr;
	int		procfd;	/* filedescriptor for /proc/nnnnn/psinfo */
	psinfo_t 	info;	/* process information from /proc */
	int		ret = 0;
	int		len = sizeof (info);

	if (ISNULL(exename, procs)) {
		return (-1);
	}

	Trace(TR_MISC, "finding all %s processes", exename);

	*procs = NULL;

	if ((dirp = opendir(PROCDIR)) == NULL) {
		samerrno = SE_OPENDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), PROCDIR, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "open dir %s failed", PROCDIR);
		return (-1);
	}

	*procs = lst_create();
	if (*procs == NULL) {
		closedir(dirp);
		Trace(TR_OPRMSG, "lst create failed");
		return (-1);
	}

	/* allocate the dirent structure */
	dent = malloc(MAXPATHLEN + sizeof (dirent64_t));
	if (dent == NULL) {
		closedir(dirp);
		lst_free(*procs);
		*procs = NULL;
		samerrno = SE_NO_MEM;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	/* find each active process --- */
	while ((ret = readdir64_r(dirp, dent, &dentp)) == 0) {

		if (dentp == NULL) {
			break;
		}

		/* skip . and .. */
		if (dentp->d_name[0] == '.') {
			continue;
		}

		snprintf(pname, MAXPATHLEN, "%s/%s/%s", PROCDIR,
		    dentp->d_name, "psinfo");

		procfd = open64(pname, O_RDONLY);
		if (procfd == -1) {
			/* process may have ended while we were processing */
			continue;
		}

		/*
		 * Get the info structure for the process and close quickly.
		 */
		ret = readbuf(procfd, &info, len);

		(void) close(procfd);

		if (ret == -1) {
			break;
		}

		if (info.pr_lwp.pr_state == 0)		/* can't happen? */
			continue;

		/* ensure cmd buffers properly terminated */
		info.pr_psargs[PRARGSZ-1] = '\0';
		info.pr_fname[PRFNSZ-1] = '\0';

		/* is it the proc we're looking for? */
		if (strcmp(info.pr_fname, exename) != 0) {
			continue;
		}

		ptr = mallocer(len);
		if (ptr == NULL) {
			ret = -1;
			break;
		}
		memcpy(ptr, &info, len);

		if (lst_append(*procs, ptr) == -1) {
			Trace(TR_OPRMSG, "lst append failed");
			ret = -1;
			break;
		}
	}

	closedir(dirp);
	free(dent);

	if (ret == 0) {
		Trace(TR_OPRMSG, "returned process info list of size [%d]",
		    (*procs)->length);
	} else {
		lst_free_deep(*procs);
		*procs = NULL;
	}

	return (ret);
}

/*
 * signal_process
 *
 * Function to send a signal to a specific process.
 *
 * If pid is 0, calls find_process() [emulates pkill]
 *
 * If both pid and exename are specified, process that
 * matches pid is verified to have an fname of exename.
 *
 */
int
signal_process(
	pid_t		pid,
	char		*exename,
	int		signum)
{

	int		ret;
	sqm_lst_t		*procs = NULL;
	psinfo_t	*infop;
	node_t		*node;

	if (((pid == 0) && (exename == NULL)) || (signum < 1)) {
		return (-1);
	}

	/* if pid was provided without exename */
	if ((pid != 0) && (exename != NULL)) {
		sigsend(P_PID, pid, signum);

		return (0);
	}

	/* get the relevant processes */
	ret = find_process(exename, &procs);
	if (ret == -1) {
		return (-1);
	}

	node = procs->head;

	while (node != NULL) {
		infop = node->data;

		if (pid != 0) {
			if (pid != infop->pr_pid) {
				continue;
			}
		}

		Trace(TR_DEBUG, "signalling process %ld with signal %d",
		    infop->pr_pid, signum);

		sigsend(P_PID, infop->pr_pid, signum);

		node = node->next;
	}

	lst_free_deep(procs);

	return (0);
}

/* helper function to use read() correctly */
int
readbuf(int fd, void* buffer, int len)
{
	int	numread = 0;
	int	ret;
	char	*bufp;

	if ((buffer == NULL) || (len < 1) || (fd == -1)) {
		return (-1);
	}

	bufp = buffer;

	while (numread < len) {
		ret = read(fd, bufp, (len - numread));

		if (ret == -1) {
			if (errno == EAGAIN) {
				continue;
			}
			numread = -1;
			break;
		}

		numread += ret;
		bufp += ret;
	}

	return (numread);
}

/* helper function to use write() correctly */
static int
write_buf(int fd, void* buffer, int len)
{
	int	written = 0;
	int	ret;
	char	*bufp;

	if ((buffer == NULL) || (fd == -1)) {
		return (-1);
	}

	bufp = buffer;

	while (written < len) {
		ret = write(fd, bufp, (len - written));

		if (ret == -1) {
			if (errno == EAGAIN) {
				continue;
			}
			written = -1;
			break;
		}

		written += ret;
		bufp += written;
	}

	return (written);
}

/*
 * mk_wc_path()
 *
 * Function to generate a path name for working copies of
 * files and creates the file.
 */
int
mk_wc_path(
	char *original, 	/* IN - path to original file */
	char *tmppath, 		/* IN/OUT - buffer to hold new file path */
	size_t buflen)		/* IN - length of buffer */
{
	char		*copypath;
	char		template[MAXPATHLEN+1];
	char		buf[MAXPATHLEN+1];
	char		*fname;
	int		ret;
	struct stat64	statbuf;

	if (ISNULL(original, tmppath)) {
		return (-1);
	}

	/* make sure target directory exists */
	ret = create_dir(NULL, default_tmpfile_dir);
	if (ret != 0) {
		return (ret);
	}

	ret = stat64(original, &statbuf);

	/*
	 * not an error if the original doesn't exist.  In this
	 * case, dummy up a mode to be used for the later mknod.
	 */
	if (ret != 0) {
		statbuf.st_mode = S_IFREG;
		statbuf.st_mode |= S_IRWXU|S_IRGRP|S_IROTH;
	}

	/* create the template name */
	strlcpy(buf, original, MAXPATHLEN+1);
	fname = basename(buf);
	snprintf(template, MAXPATHLEN+1, "%s/%s_XXXXXX",
	    default_tmpfile_dir, fname);

	copypath = mktemp(template);

	if (copypath == NULL) {
		return (-1);
	} else {
		strlcpy(tmppath, copypath, buflen);
	}

	/* make sure an old version isn't hanging around */
	unlink(tmppath);

	/* create the target file */
	ret = mknod(tmppath, statbuf.st_mode, 0);

	return (ret);
}

/*
 * make_working_copy()
 *
 * Copies a file to the default temporary location and returns
 * the pathname of the copy.
 *
 */
int
make_working_copy(char *path, char *wc_path, int pathlen)
{
	int		ret;

	if (ISNULL(path, wc_path)) {
		return (-1);
	}

	ret = mk_wc_path(path, wc_path, pathlen);
	if (ret != 0) {
		return (ret);
	}

	ret = cp_file(path, wc_path);

	return (ret);
}

/* TEST FUNCTIONS */

#ifdef TEST
void
display_output(FILE *stream) {
	int MAXLINE = 400;
	char line[MAXLINE];
	while (NULL != fgets(line, MAXLINE, stream))
		printf("%s", line);
	fflush(NULL);
}

/*
 * sample use of exec_get_output()
 */
test(void) {
	pid_t pid;
	int status;
	FILE *res_stream = NULL, *err_stream = NULL;

	pid = exec_get_output(SBIN_DIR"/sammkfs sam4",
		&res_stream, &err_stream);
	if (pid < 0) {
		printf("error %d: %s\n", samerrno, samerrmsg);
		return;
	}
	printf("res_stream:\n");
	display_output(res_stream);
	printf("err_stream:\n");
	display_output(err_stream);
	printf("done\n"); fflush(NULL);
	if ((pid = waitpid(pid, &status, 0)) < 0)
		printf("waitpid failed");
	else
		printf("child's exit code: %d\n", status);
	fclose(res_stream); fclose(err_stream);
}

#endif /* TEST */
