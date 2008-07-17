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

#ifndef _UTIL_H
#define	_UTIL_H

#pragma ident	"$Revision: 1.43 $"

#include <sys/types.h>
#include <stdio.h>
#include <limits.h>

#include "pub/mgmt/error.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

#include "sam/types.h"

#ifndef Str
#define	Str(s) (s ? (char *)s : "NULL")
#endif

/* Macro to do pointer comparisons in a lint-compatible manner */
#if !defined(lint)
#define	PTRDIFF(a, b)  ((intptr_t)a - (intptr_t)b)
#else /* !defined(lint) */
#define	PTRDIFF(a, b) (memcmp((a), (b), 1))
#endif /* !defined(lint) */

/* macro to set samerrno and samerrmsg; argument must be a sam_errno_t */
#define	setsamerr(err) { samerrno = err; \
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno)); }

/* used by the ISNULL macro below */
int isnull(char *argnames, ...);

/*
 * macro to check if API function arguments are NULL.
 * one or more pointers are expected as arguments to this macro.
 */
#define	ISNULL(...) isnull(#__VA_ARGS__, __VA_ARGS__)

/*
 * macro to check if a path has a trailing slash
 */
#define	ENDSWITHSLASH(input) input[strlen(input)-1] == '/'


/* wait this long after a SIGHUP before calling mkfs (seconds) */
#define	DELAY_FSD 1


/*
 * Note: STRLCATGROW can be used to concatenate b to
 * the end of a malloced buffer a. If a is not big enough
 * to hold the result it will automatically be grown.
 * Users can safely string together a number of STRLCATGROWS
 * and only check prior to using a that a != NULL.
 */
#define	STRLCATGROW(a, b, sz) \
	{ size_t _catted = 0; ptrdiff_t _remain = 0; \
		while (a != NULL && \
			(_catted = strlcat(a, b + _remain, sz)) >= sz) { \
			_remain = _catted - sz + 1; \
			_remain = strlen(b) - _remain; \
			sz = sz * (size_t)2; \
			a = realloc(a, sz); \
		} \
	}


/*
 * allocate memory and checks for errors.
 * if not enough memory then NULL is returned,
 * samerrno is set to SE_NO_MEM and samerrmsg is set accordingly.
 */
void *mallocer(size_t size);


/*
 * From a path and a file name get the full path to the file.
 * This function adds a slash at the end of the path if it is needed.
 * The result is written into a static array.
 * If time_ext is true the current time will be appended as a file extension.
 */
char *assemble_full_path(char *path, char *file, boolean_t time_ext,
	upath_t result);


/*
 * return child's PID
 */
pid_t
exec_get_output(const char *shcmd,	/* execute: sh -c shcmd */
    FILE **output_stream,		/* output is redirected here */
    FILE **stderr_stream);		/* stderr is redirected here */


int cp_file(const char *old, const char *new);
int backup_cfg(char *src_file);

/*
 * init_config()
 * This function will call samd config to initialize the system
 * to see changes to the config files. This function has been given a context
 * argument in case it becomes desireable to make it public.
 */
int init_config(ctx_t *);


/*
 * init_library_config()
 * This function will call samd stop and samd start to restart the library
 * and drive configuration after MCF has been changed.
 */
int init_library_config(ctx_t *);


/*
 * creates a list of all the instances of running SAM-FS daemons
 */
int get_running_sam_daemons(sqm_lst_t **daemons);


/*
 * Check if there is a network problem between the client and the server
 * Use ping
 *
 * ping server timeout
 * If network is reachable, we get the string 'host is alive'
 * If network is unreachable, we get the string 'no answer from host'
 *
 * Return value:
 * for internal failure while doing popen/pclose : return -1
 * ping to host fails: return 1
 * ping to server succeeds: return 0
 *
 */
int nw_down(char *svr);


/* SCSI stuff section */
#define	DISK_DEVTYPE 76
#define	MC_INQ_DEVTYPE 8		/* standard inquiry device type	*/
#define	DATAIN B_TRUE			/* SCSI read command		*/
#define	DATAOUT B_FALSE			/* SCSI write command		*/
#define	REQUIRED B_TRUE 		/* display ioctl failures	*/
#define	OPTIONAL B_FALSE

/*
 *	device data structure, used for SCSI commands
 */
typedef struct devdata
{
	char logdevpath[PATH_MAX];	/* logical device path		 */
	boolean_t verbose;		/* display debug info		 */
	int fd;				/* robot or tape file descriptor */
	int devtype;			/* standard inquiry device type	 */
} devdata_t;

int issue_cmd(devdata_t *devdata, char *cdb, int cdblen, char *data,
	int *datalen, boolean_t datain, boolean_t required);

/*
 * send email
 */
int send_mail(char *recepient, char *subject, char *msg);

/* function to copy a string and set samerrno on failure */
char *copystr(char *ptr);


/* copyint - Given an integer, allocate storage and copy it. */
int *
copyint(int value);

/*
 * samrerr -    Utility routine to set samerrmsg and samerrno.
 *		Presumes a single text string argument.
 */

int
samrerr(int errnum, char *text);

/*
 * getfsmountpt - find current mount point for a particular filesystem
 *		  by searching through /etc/mnttab
 */

int
getfsmountpt(
	char *fsname,	/* Input, QFS file system name */
	char *mountpt,	/* Output, buffer for path to current mount point */
	size_t len	/* length of buffer */
);

/*
 * getvfsmountpt - find mount point for a filesystem that is not mounted
 *			by searching through /etc/vfstab
 */

int
getvfsmountpt(
	char	*fsname,	/* IN, file system name */
	char	*mountpt,	/* IN/OUT, buffer for mount point path */
	size_t	len		/* IN, size of buffer */
);

/*
 * given a uid get the associated user name
 */
int get_user_name(uid_t uid, char *res, int reslen);


/*
 * given a gid get the associated group name
 */
int get_group_name(gid_t uid, char *res, int reslen);


/*
 * Key Value Argument Handling
 */

/* Generic parsing constant */
#define	WHITESPACE " \11\12\15"
#define	CHAR_POUND	'#'
#define	COLON		":"


/* Parser-token definition */
typedef struct parsekv_s {
	char keyword[32];
	size_t offset;
	int (*method)(char *, void *);
} parsekv_t;


int parse_kv(char *str, parsekv_t *tokens, void *buf);

int parsekv_int(char *ptr, void * bufp);
int parsekv_ll(char *ptr, void * bufp);
int parsekv_llu(char *ptr, void * bufp);
int parsekv_time(char *ptr, void * bufp);
int parsekv_string_3(char *ptr, void * bufp);
int parsekv_string_5(char *ptr, void * bufp);
int parsekv_string_16(char *ptr, void * bufp);
int parsekv_string_32(char *ptr, void * bufp);
int parsekv_string_256(char *ptr, void * bufp);
int parsekv_string_1024(char *ptr, void * bufp);
int parsekv_string_1072(char *ptr, void * bufp);
int parsekv_bool(char *ptr, void * bufp);
int parsekv_bool_YN(char *ptr, void * bufp);
int parse_userid(char *ptr, void * bufp);
int parse_gid(char *ptr, void * bufp);

/*
 * parsekv_mallocstr will malloc a string and place a pointer
 * to it at bufptr. Bufp must be passed the address of a ptr
 */
int parsekv_mallocstr(char *ptr, void * bufp);

#define	BASIC_PERIOD_UNITS "smhdwy"
#define	EXTENDED_PERIOD_UNITS "smhdwyMDW"
/*
 * translates the input to an int and one of the valid units in the
 * units str. If the input does not represent a valid time period an
 * error will be returned.
 */
int translate_period(char *input, char *units, uint32_t *value, char *unit);

/*
 *  Takes a date string of the form YYYYMMDDhhmm and converts it
 *  to a struct tm (time structure).
 *
 *  Due to limitations with mktime(), this function will not work
 *  with dates >  03:14:07 UTC,  January 19, 2038.  Hopefully mktime()
 *  will get fixed before this becomes an issue for us.
 */
int
translate_date(char *indate, struct tm *outtime);

/*
 * Turns a mode returned from the stat() family of functions
 * into a string.  Buffer "prots" must be at least 11 characters long.
 */
int
modeToString(mode_t mode, char *prots);

/*
 *  Converts a block device path to a raw device path.
 *  If successful, *rdskpath is a malloc()ed string and will
 *  need to be freed by the caller.
 */
int
dsk2rdsk(
	const char *devpath,	/* IN  - block device */
	char ** rdskpath	/* OUT - raw device */
);

/*
 * function str_trim() deletes leading and trailing spaces.
 */
char *str_trim(char *s, char *out_str);

#define	PROCDIR "/proc"	/* standard procfs directory */

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
	sqm_lst_t ** procs);	/* OUT - list of psinfo_t structs */


/*
 * signal_process
 *
 * Function to send a signal to a specific process.
 *
 * If pid is 0, calls find_process() [emulates pkill]
 *
 */
int
signal_process(
	pid_t	pid,
	char 	*exename,
	int	signum);

/* helper function to use read() correctly */
int
readbuf(int fd, void* buffer, int len);

#define	default_tmpfile_dir VAR_DIR"/tmpfiles/fsmgmtd"

/*
 * mk_wc_path()
 *
 * Function to generate a path name for working copies of
 * files and creates the file.
 */
int
mk_wc_path(
	char *original,		/* IN - path to original file */
	char *tmppath,		/* IN/OUT - buffer to hold new file path */
	size_t buflen);		/* IN - length of buffer */


/*
 * make_working_copy()
 *
 * Copies a file to the default temporary location and returns
 * the pathname of the copy.
 *
 */
int
make_working_copy(char *path, char *wc_path, int pathlen);



/*
 * Extern utility declarations.  Eventually, all of these functions
 * should be replaced with local versions.
 */
extern char *StrFromErrno(int errno_arg, char *buf, int buf_size);

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
void set_samdaemon_err(int errnum, int samerrnum, char *daemonmsg);

void free_string_array(char **str_arr, int arr_len);

#endif /* _UTIL_H */
