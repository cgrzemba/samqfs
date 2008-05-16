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

#pragma ident   "$Revision: 1.26 $"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/names.h"
#include "tapealert_text.h"

#include "pub/mgmt/error.h"
#include "pub/mgmt/faults.h"

#define	COMPONENT	"TapeAlert"
#define	NUMARGS		15
#define	UNKNOWNHOST	"unknown host"

/*
 * tapealert_log.c
 *
 * The source file to persist tapealert faults by using the sysevent
 * framework.
 *
 * The sysevent information received from syseventd for tapealerts, contains
 * vendor, product, revision, faulttime, equipment ordinal, device name,
 * type, vsn, flags_len and flags(mapped to an appropriate user-friendly
 * message)
 *
 * This information populates a 'faults' structure and the 'fault' is
 * written to logfile (/var/opt/SUNWsamfs/faults/faultlog.bin)
 *
 * Thus, fault handling and persistence is done offline using the
 * syseventd mechanism.
 *
 * Input arguments to the program are:
 *
 * 1) vendor	- vendor (SCSI INQUIRY)
 * 2) product	- product id
 * 3) rev	- revision
 * 4) faulttime - time at which the event occured (in time_t format)
 * 5) eq	- equipment ordinal
 * 6) name	- tape/device path
 * 7) type	- the scsi inquiry peripheral device type
 * 8) vsn	- vsn
 * 9) flags_len - tapealert flags number
 * 10) flags	- tapealert flags 64-1
 * 11) set	- family set name of library
 * 12) fseq - Eq. Ord Number
 * 13) seq_id	- sysevent generated sequence number
 * 14) version  - standard inquiry version
 *
 * Error handling: Log all errors using sam_syslog and exit with 1
 */

static char *get_system_id();
static void strip_quotes(char *str);
#if 0
static void print_fault_attributes(fault_attr_t *);
#endif
static void persist_fault(fault_attr_t *);

int
main(int argc, char *argv[])
{

	fault_attr_t		*fault; /* a pointer to a fault */
	tapealert_text_t	text;
	int			scsi_version, scsi_type, flags_len;
	int			i, j;
	uint64_t		flags;
	int			seq_id = 0; /* sysevent generated sequence */

	if (argc == NUMARGS) {

		/* input from sysevent conf file contains quotes */
		for (j = 0; j < argc; j++) {
			strip_quotes(argv[j]);
		}

		/* Allocate a faults structure */
		if ((fault = (fault_attr_t *)malloc(sizeof (fault_attr_t)))
		    != NULL) {

			memset(fault, 0, sizeof (fault_attr_t));

			/* Fill in the component ID */
			strncpy(fault->compID, COMPONENT,
			    (sizeof (fault->compID) - 1));

			/* get system identifier */
			strncpy(fault->hostname, get_system_id(),
			    (sizeof (fault->hostname) - 1));

			/* The timestamp */
			fault->timestamp = strtol(argv[4], NULL, 0);

			/* The eq number */
			fault->eq = strtol(argv[5], NULL, 0);


			/* Copy in the library family setname */
			strncpy(fault->library, argv[11],
			    (sizeof (fault->library) - 1));

			/* The default state */
			fault->state = UNRESOLVED;

			scsi_version = strtol(argv[14], NULL, 0);
			scsi_type = strtol(argv[7], NULL, 0);
			flags_len = strtol(argv[9], NULL, 0);
			flags = (uint64_t)strtoll(argv[10], NULL, 16);

			seq_id = strtol(argv[13], NULL, 0);
			/*
			 * get the mapping between flags and application message
			 * in a tapealert_text_t structure
			 */
			tapealert_text(scsi_version, scsi_type, flags_len,
			    flags, &text);

			if (text.count == 0) {  /* no mapping of flags to txt */

				sam_syslog(LOG_ERR, GetCustMsg(31000),
				    flags, &text, scsi_type, flags_len);
			}

			/* how many alerts ? */
			for (i = 0; i < text.count; i++) {

				/*
				 * Each unique tapealert sysevent
				 * can have upto 64 flags set which
				 * can indicate unique alerts!! So
				 * there is need of uniquely identify-
				 * ing each of these alerts. They are
				 * each treated here as unique faults.
				 *
				 * Get the error ID.
				 * Ideally, a 64-bit quantity would express
				 * a unique ID the best...
				 */
				fault->errorID = fault->timestamp + seq_id + i;

				/* error type mapping */
				if (text.msg[i].scode == SEVERITY_RSVD ||
				    text.msg[i].scode == SEVERITY_INFO) {
					fault->errorType = MINOR;
				} else if (text.msg[i].scode == SEVERITY_WARN) {
					fault->errorType = MAJOR;
				} else if (text.msg[i].scode == SEVERITY_CRIT) {
					fault->errorType = CRITICAL;
				}

				/* Copy in the message */
				strncpy(fault->msg, text.msg[i].appmsg,
				    (sizeof (fault->msg) - 1));
				/* append a break */
				strncat(fault->msg, "\n",
				    (sizeof (fault->msg) -
				    strlen(fault->msg) - 1));
				strncat(fault->msg, text.msg[i].cause,
				    (sizeof (fault->msg) -
				    strlen(fault->msg) - 1));

				/* Now persist the fault */
				persist_fault(fault);

			}

			if (fault != NULL) /* free if only non-NULL */
				free(fault);
		}
	}

	return (0);
}


/*
 * persist_fault
 *
 * Writes the fault to file
 */
void
persist_fault(fault_attr_t *fault)
{

	int		fd;
	int		fsize;	/* file size */
	struct stat	st, buf2; /* buffers */
	char		cmd[CMD_ARG];

	/* for event correlation */
	void *mp = NULL;
	fault_attr_t	*last_fault = NULL;
	int		num_faults = 1;

	/* Attempt to make the directory */
	if (mkdir(FAULTLOG_DIR, 0755) == -1) {
		/* If the failure is other than EEXIST, return */
		if (errno != EEXIST) {
			sam_syslog(LOG_ERR, GetCustMsg(30215),
			    FAULTLOG_DIR, strerror(errno));
				return;
		}
	}

	/* Open the persistence file */
	if ((fd = open(FAULTLOG, O_RDWR |O_CREAT |O_APPEND, 0666)) < 0) {
		sam_syslog(LOG_ERR, GetCustMsg(30216),
		    FAULTLOG, strerror(errno));
		return;
	}

	/*
	 * Log rotation if file size exceeds the MAX_SIZE.
	 * Log rotation is only based on size right now.
	 * A time-based rotation was investigated but that
	 * would require either some daemon-based control
	 * [crond/logadm etc] or comparison of timestamps
	 * of faults to determine the aging [which could be a little
	 * expensive].
	 * Also a circular buffer implementation was implemented
	 * but due to flurry of tapealerts/sysevents, we run into
	 * contention while accessing the files, deleting data etc.
	 * Such a solution was deferred for now.
	 */
	fstat(fd, &st);
	fsize = st.st_size;

	if (fsize > MAX_FSIZE) {
		if (stat(SAM_EXECUTE_PATH"/"TRACE_ROTATE, &buf2) == 0) {


			sprintf(cmd, "%s %s", SAM_EXECUTE_PATH"/"TRACE_ROTATE,
			    FAULTLOG);
			if ((pclose(popen(cmd, "w"))) < 0) {

				sam_syslog(LOG_ERR, GetCustMsg(30213),
				    FAULTLOG, strerror(errno));
				close(fd);
				return;
			}
		}
	} else if (fsize == 0) { /* file just created */

		/* Now write to the file */
		if ((write(fd, fault, sizeof (fault_attr_t))) !=
		    sizeof (fault_attr_t)) {

			sam_syslog(LOG_ERR, GetCustMsg(30214),
			    FAULTLOG, strerror(errno));
			close(fd);
			return;
		}

		close(fd);

		return;
	}

	/*
	 * Fault log has some entries, try and correlate the entries. Check if
	 * this fault is same as the last fault in the log. If repetition,
	 * update the timestamp and add the repeat count to the message
	 *
	 * Memory map the last entry in the file
	 * lseek(fd, pos, 0), read(fd, last_fault, len)
	 *
	 * offset should be multiple of pagesize if only a portion is to be mmap
	 * so map the entire file
	 */
	mp = mmap(NULL, fsize, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
	if (mp == MAP_FAILED) {
		sam_syslog(LOG_ERR, GetCustMsg(SE_MMAP_FAILED), FAULTLOG);
		close(fd);
		return;
	}
	/* get the last entry */
	last_fault = (fault_attr_t *)mp;
	/* there is atleast one fault if the program is executing this */
	last_fault = last_fault + (num_faults - 1);

	if (strncmp(fault->msg, last_fault->msg, strlen(fault->msg)) == 0) {

		int repeat = 0;
		upath_t stime;
		char format_str[MAXLINE];

		stime[0] = '\0';
		format_str[0] = '\0';

		/*
		 * First repetition, get stime from last_fault->timestamp
		 * Subsequent repetitions, get stime from last_fault->msg
		 * Time display format: April 4, 2005 5:01:00 PM
		 *
		 * It would be easier to just store the repeat count and stime
		 * in the fault_attr_t but this would not allow for backward
		 * compatible release.
		 */
		strcpy(format_str, fault->msg);
		strcat(format_str, "<br>");
		/*
		 * 30601: "Repeated %d time(s) from %32[aA-zZ0-9:, ]"
		 *
		 * Note: The format arguments are positional. If the order
		 * of arguments is changed during localization, sscanf will fail
		 *
		 */
		strcat(format_str, GetCustMsg(SE_EVENT_REPEAT_MSG_FORMAT));

		sscanf(last_fault->msg, format_str, &repeat, stime);
		if (repeat == 0) {
			struct tm *tp = localtime(&last_fault->timestamp);
			stime[0] = '\0';

			strftime(stime, sizeof (stime) - 1,
			    "%B %d, %Y %I:%M:%S %p %Z", tp);
		}

		sprintf(format_str, "%s<br>%s", fault->msg,
		    GetCustMsg(SE_EVENT_REPEAT));
		sprintf(last_fault->msg, format_str, ++repeat, stime);

		last_fault->timestamp = fault->timestamp;

		munmap(mp, fsize);
		close(fd);
		return;
	}
	munmap(mp, fsize);

	/* Now write to the file */
	if ((write(fd, fault, sizeof (fault_attr_t)))
	    != sizeof (fault_attr_t)) {

		sam_syslog(LOG_ERR, GetCustMsg(30214),
		    FAULTLOG, strerror(errno));
		close(fd);
		return;
	}

	close(fd);

	return;

}


/*
 * get_system_id
 *
 * This always returns a string, never NULL. If the hostname cannot be
 * obtained, then it returns UNKNOWNHOST
 */
char *
get_system_id()
{
	char hostname[MAXHOSTNAMELEN];	/* system id */

	if (gethostname(hostname, sizeof (hostname)) < 0) {
		sam_syslog(LOG_ERR, GetCustMsg(12526), strerror(errno));
		strncpy(hostname, UNKNOWNHOST, (sizeof (hostname) - 1));
	}

	return (strdup(hostname));
}


/*
 * strip_quotes
 *
 * Inputs from script sometimes have quotation marks in them
 * strip_quotes removes the quotes, the input argument is modified
 *
 *
 * If str is NULL, then nothing is done.
 */
void
strip_quotes(char *str)	/* input str, also modified to hold unquoted str */
{
	char *quote;

	if (str != NULL) {
		quote = str;

		while (*quote) {
			if (*quote != '"') {
				*str = *quote;
				str++;
			}
			quote++;
		}
		*str = '\0';
	}
}

#if 0
void
print_fault_attributes(
fault_attr_t *fault
)
{
	sam_syslog(LOG_ERR, "errorID\t=\t%ld\n", fault->errorID);
	sam_syslog(LOG_ERR, "compID\t=\t%s\n", fault->compID);
	sam_syslog(LOG_ERR, "errorType\t=\t%d\n", fault->errorType);
	sam_syslog(LOG_ERR, "timestamp\t=\t%s\n", ctime(&fault->timestamp));
	sam_syslog(LOG_ERR, "hostname\t=\t%s\n", fault->hostname);
	sam_syslog(LOG_ERR, "msg\t=\t%s\n", fault->msg);
	sam_syslog(LOG_ERR, "state\t=\t%d\n", fault->state);
	sam_syslog(LOG_ERR, "library\t=\t%s\n", fault->library);
	sam_syslog(LOG_ERR, "eq\t=\t%d\n", fault->eq);
}
#endif
