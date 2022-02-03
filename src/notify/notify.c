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

#pragma ident   "$Revision: 1.31 $"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "sam/types.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/names.h"

/* sammgmt headers */
#include "pub/mgmt/error.h"
#include "pub/mgmt/faults.h"
#include "pub/mgmt/notify_summary.h"
#include "pub/mgmt/mgmt.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"

/*
 * notify.c
 *
 * The sysevent information received from syseventd is handled here
 * Depending on the action_flag (arg[8]), this event is persisted,
 * sent as an email message and/or send as a SNMP trap
 *
 * the fault_log.c is replaced with this file with additional support
 * to send emails
 *
 * The information provided by syseventd includes a keyword identifying the
 * category of the alert, subcategory or specific type of alert, error type
 * identifying the severity and syslog level of the event, the message number
 * as found in the message catalog, the system identifier i.e. the hostname
 * upon which the fault originated, the text of the message string,and date
 * time when the event occured. The type of action to be taken by this handler
 * is also included
 *
 * Positional command line arguments are expected.
 *
 * Error handling: All the errors are logged using sam_syslog
 * sam-log must be configured.
 *
 */

#define	NUMARGS		9
#define	SENDTRAP	"/etc/opt/SUNWsamfs/scripts/sendtrap"

static fault_attr_t *format_input(char **argv);
static void persist_fault(fault_attr_t *fault);
static void print_fault_attributes(fault_attr_t *);
static void strip_quotes(char *str);
static void email(char *class, char *subclass,
	int priority, char *from, char *msg, char *ts);
static int record_hwm(char *hwm_exceed_msg, char *time_str);
static int if_flooding_mbox(
	char *recipient, char *msg, time_t timestamp, int window);

void
fault_history(char **argv)
{
	fault_attr_t *fault_data = NULL;

	/* format the input from sysevent into a fault and then persist */
	if ((fault_data = format_input(argv)) != NULL) {

		persist_fault(fault_data);

		if (fault_data != NULL)
			free(fault_data);
	}
}


void
email(char *class, char *subclass, int priority,
    char *from, char *msg, char *ts)
{
	sqm_lst_t *lst = NULL;
	node_t *node = NULL;
	char eventtype[64] = {0};
	char subject[128] = {0};
	int key = SE_ALERT_TITLE;

	struct tm	tm; /* structure to hold broken-down time */
	time_t timestamp;
	char 	errormsg[MAX_MSG_LEN] = {0};

	if (ISNULL(class, subclass, from, msg, ts)) {
		return;
	}
	/*
	 * Get a localized string for the log severity
	 * The string values for the log severities are mapped from
	 * resource key SE_LOG_EMERG in error.h
	 *
	 * get a localized string for the class of Alert
	 * This is used to generate the subject of the email
	 */
	if (strncmp(class, DUMP_CLASS, strlen (class)) == 0) {
		key = SE_DUMP_TITLE;
	} else if (strncmp(class, ACSLS_CLASS, strlen (class)) == 0) {
		key = SE_ACSLS_TITLE;
	} else if (strncmp(class, FS_CLASS, strlen (class)) == 0) {
		key = SE_FS_TITLE;
	}
	snprintf(subject, sizeof (subject), "%s %s %s",
	    from, GetCustMsg(key), GetCustMsg(priority + SE_LOG_EMERG));

	/* get the recepients */
	if (strncmp(class, FS_CLASS, strlen (class)) == 0) {
		/* only the subclass is stored along with the email addrs */
		snprintf(eventtype, sizeof (eventtype), "%s", subclass);
	} else {
		snprintf(eventtype, sizeof (eventtype), "%s%s",
		    class, subclass);
	}
	if (get_addr(eventtype, &lst) != 0) {
		return;
	}
	node = lst->head;
	while (node != NULL) {

		char *addr = (char *)node->data;
		if (ISNULL(addr)) {
			sam_syslog(LOG_ERR, errormsg);
			return;
		}
		/* convert to time_t using strptime() */
		if (strptime(ts, "%a %b %d %H:%M:%S %Y", &tm) == NULL) {
			snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(32163), ts);
			sam_syslog(LOG_ERR, errormsg);
			return;
		}
		tm.tm_isdst = -1;
		timestamp = mktime(&tm);

		/*
		 * Do not flood the inbox with messages, In 4.6, the
		 * flood window is set to 2 hours, i.e. the same email
		 * message is not to be sent to the same recipient for
		 * atleast two hours.
		 */
		if (if_flooding_mbox(
		    addr, msg, timestamp, 7200) == 1) {
			node = node->next;
			continue;
		}

		/*
		 * append message header
		 *
		 * SYSTEM NOTIFICATION
		 *
		 * System  : Sun Microsystems, Inc StorEdge SAM-FS/QFS software
		 * Host    : ns-east-113
		 * Severity: Error
		 * Time Stamp : Thu Aug 11  8:41:09 2005
		 *
		 *
		 * Problem:
		 */
		char mail_str[2048] = {0};
		char *hostname = NULL;

		get_host_name(NULL, &hostname);

		snprintf(mail_str, sizeof (mail_str) - 1,
		    GetCustMsg(SE_MAIL_HEADER),
		    hostname != NULL ? hostname : "Unknown",
		    GetCustMsg(priority + SE_LOG_EMERG), ts);

		strncat(mail_str, msg, sizeof (mail_str) - 1);

		if (send_mail(addr, subject, mail_str) != 0) {
			sam_syslog(LOG_ERR, "Could not send email");
		}
		node = node->next;
	}

}


void
sendtrap(char *class, char *subclass, int errortype, int errorid,
    char *systemid, char *msg, char *time)
{
	char errormsg[MAX_MSG_LEN] = {0};
	char *command = NULL;
	int len = 0;

	len = strlen(class) + strlen(subclass) + strlen(systemid) +
	    strlen(msg) + strlen(time) + 64; /* extra padding */
	command = (char *) malloc(sizeof (char) * len);
	memset(command, 0, len);

	snprintf(command, len, "%s %s %s %d %d %s \"%s\" \"%s\"", SENDTRAP,
	    class, subclass, errortype, errorid, systemid, msg, time);

	if (system(command) == -1) {
		snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(31001),
		    class, subclass, "");
		strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
		sam_syslog(LOG_ERR, errormsg);
	}

	if (command != NULL)
		free(command);


}


/*
 * format_input
 *
 * Fill up the faults structure with values which come in from the sysevent.
 * All such values are strings and must be converted as appropriate. All
 * arguments are positional and expected to be ordered, thus used in a
 * hard-coded manner.
 * This order is:
 * argv[1] -> component ID
 * argv[3] -> Error Type
 * argv[4] -> Error ID
 * argv[5] -> Host name
 * argv[6] -> Message
 * argv[7] -> Time
 * argv[8] -> Sequence number
 */

static fault_attr_t *
format_input(char **argv)
{

	fault_attr_t	*fault;		/* a fault attributes */
	int		errortype;	/* error type */
	struct tm	tm; /* structure to hold broken-down time */
	uint64_t	seq_id; /* local var to hold sequence # */

	/* Allocate a fault object */
	if ((fault = (fault_attr_t *)malloc(
	    sizeof (fault_attr_t))) == NULL) {
		return (NULL);
	}

	memset(fault, 0, sizeof (fault_attr_t));

	/* Copy in the class/Component ID */
	strncpy(fault->compID, argv[1], (sizeof (fault->compID) - 1));

	/*
	 * Copy the Error type
	 *
	 * Error types are defined as LOG_EMERG (0), LOG_ALERT (1),
	 * LOG_CRIT(2) etc. as in syslog
	 * The GUI has requirements to map these as CRITICAL, MAJOR and
	 * MINOR. So regroup the error types to meet GUI client's needs.
	 */
	errortype = strtol(argv[3], NULL, 0);

	if (errortype == 0 || errortype == 1) {
		fault->errorType = CRITICAL;
	} else if (errortype == 2 || errortype == 3) {
		fault->errorType = MAJOR;
	} else {
		fault->errorType = MINOR;
	}


	/* copy in the hostname */
	strncpy(fault->hostname, argv[5], (sizeof (fault->hostname) - 1));

	/* copy in the message */
	strncpy(fault->msg, argv[6], (sizeof (fault->msg) - 1));

	/*
	 * copy the fault timestamp
	 *
	 * The fault time is given as a string, convert the string to time
	 * using strptime().
	 * Tried using the StrToTime in SAM but that lacks a conversion for
	 * the format that this time string comes in.
	 */
	strptime(argv[7], "%a %b %d %H:%M:%S %Y", &tm);
	tm.tm_isdst = -1;
	fault->timestamp = mktime(&tm);

	/*
	 * Generate the error ID.
	 * Many options were considered. Of course,
	 * maintaining an ever-increasing ID starting
	 * with 0 would be ideal, but that would need persistence
	 * and re-reads upon crashes/reboots. So, time and
	 * sequence ID provided by syseventd were chosen.
	 */
	seq_id = strtoll(argv[8], NULL, 0);

	fault->errorID = fault->timestamp + seq_id; /* note the downcast */

	/* The default state */
	fault->state = UNRESOLVED;

	/*
	 * Library and eq are not required for these faults, these are only
	 * for TapeAlerts
	 */
	strcpy(fault->library, NO_LIBRARY);

	/* Set the eq to NO_EQ  */
	fault->eq = NO_EQ;

	return (fault);

}

/*
 * persist_fault
 *
 * Writes the fault to file
 */
void
persist_fault(fault_attr_t *fault)
{

	int		fd = -1;
	int		fsize;		/* file size */
	struct stat	st, buf2; /* buffers */
	char		cmd[CMD_ARG] = {0};
	char		errormsg[MAX_MSG_LEN] = {0};

	/* for event correlation */
	void *mp = NULL;
	fault_attr_t	*last_fault = NULL;
	int		num_faults = 1;

	/* Attempt to make the directory */
	if (mkdir(FAULTLOG_DIR, 0755) == -1) {
		/* If the failure is other than EEXIST, return */
		if (errno != EEXIST) {
			snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(30215),
			    FAULTLOG_DIR, "");
			strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
			sam_syslog(LOG_ERR, errormsg);
			return;
		}
	}

	/* Open the persistence file */
	if ((fd = open(FAULTLOG, O_RDWR | O_CREAT |O_APPEND, 0666)) < 0) {
		snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(30216),
		    FAULTLOG, "");
		strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
		sam_syslog(LOG_ERR, errormsg);
		return;
	}

	/*
	 * For this release, the log file is rotated after 700 faults
	 * the fault count in faultlog.bin will again start from 0 and
	 * increase till 700
	 * Log rotation if file size exceeds the MAX_SIZE
	 */
	fstat(fd, &st);
	fsize = st.st_size;
	num_faults = fsize / sizeof (fault_attr_t);

	if (fsize > MAX_FSIZE) {
		if (stat(SAM_EXECUTE_PATH"/"TRACE_ROTATE, &buf2) == 0) {


			sprintf(cmd, "%s %s", SAM_EXECUTE_PATH"/"TRACE_ROTATE,
			    FAULTLOG);
			if ((pclose(popen(cmd, "w"))) < 0) {

				snprintf(errormsg, MAX_MSG_LEN,
				    GetCustMsg(30213),
				    FAULTLOG, "");
				strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
				sam_syslog(LOG_ERR, errormsg);
				close(fd);
				return;
			}
		}
	} else if (fsize == 0) { /* file just created */

		/* Now write to the file */
		if ((write(fd, fault, sizeof (fault_attr_t))) !=
		    sizeof (fault_attr_t)) {

			snprintf(errormsg, MAX_MSG_LEN,
			    GetCustMsg(30214), FAULTLOG, "");
			strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
			sam_syslog(LOG_ERR, errormsg);
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
		upath_t stime; stime[0] = '\0';
		char format_str[MAXLINE]; format_str[0] = '\0';

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
			stime[0] = '\0';
			struct tm *tp = localtime(&last_fault->timestamp);

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
	if ((write(fd, fault, sizeof (fault_attr_t))) !=
	    sizeof (fault_attr_t)) {

		snprintf(errormsg, MAX_MSG_LEN,
		    GetCustMsg(30214), FAULTLOG, "");
		strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
		sam_syslog(LOG_ERR, errormsg);
		close(fd);
		return;
	}

	close(fd);

	return;

}

/*
 * record_hwm
 *
 * Parses the input hwm_exceed_msg for the fsname and writes/appends
 * the fsname and the time at which high water mark exceeded to file
 *
 * INPUT
 * char *hwm_exceed_msg - msg from catalog.msg
 * 	31272 The capacity of the file system %s has exceeded the
 *	configured high-water mark
 *
 * char *time - time is given as a string, convert to time_t
 */
static int
record_hwm(char *hwm_exceed_msg, char *time_str)
{
	char 	errormsg[MAX_MSG_LEN] = {0};
	char	fsname[32] = {0};
	FILE	*f = NULL;
	int	fd = -1;
	struct	tm	tm; /* structure to hold broken-down time */
	time_t	hwm_exceed_timestamp;

	if (ISNULL(hwm_exceed_msg, time_str)) {
		return (-1);
	}

	/* convert to time_t using strptime() */
	if (strptime(time_str, "%a %b %d %H:%M:%S %Y", &tm) == NULL) {
		snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(32163),
		    time_str);
		sam_syslog(LOG_ERR, errormsg);
		return (-1);
	}
	tm.tm_isdst = -1;
	hwm_exceed_timestamp = mktime(&tm);

	/* parse the (positional) fsname string from the msg */
	sscanf(hwm_exceed_msg, GetCustMsg(31272), fsname);

	/* write to file */
	/* Attempt to make the directory, parent directories must exist */
	if (mkdir(FAULTLOG_DIR, 0755) == -1) {
		/* If the failure is other than EEXIST, return */
		if (errno != EEXIST) {
			snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(30215),
			    FAULTLOG_DIR, "");
			strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
			sam_syslog(LOG_ERR, errormsg);
			return (-1);
		}
	}

	/* Open the persistence file */
	if ((fd = open(HWM_RECORDLOG, O_RDWR|O_CREAT|O_APPEND, 0666)) != -1) {
		f = fdopen(fd, "a");
	}
	if (f == NULL) {
		/* do not use GetCustMsg and strerror in the same call */
		snprintf(errormsg, MAX_MSG_LEN, GetCustMsg(30216),
		    HWM_RECORDLOG, "");
		strlcat(errormsg, strerror(errno), MAX_MSG_LEN);
		sam_syslog(LOG_ERR, errormsg);
		return (-1);
	}
	fprintf(f, "%s\t%ld\n", fsname, hwm_exceed_timestamp);
	fclose(f);

	return (0);
}

/* Helper function */
static void
print_fault_attributes(fault_attr_t *fault)
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
strip_quotes(
	char *str)	/* input str, also modified to hold unquoted str */
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

/*
 * check if mbox is being flooded with the same message sent
 * repeatedly to the recipient.
 *
 * returns 1 if last attempt with the same email contents
 * was made within the given flood window
 *
 * returns 0 if there is no record that this email was
 * sent in the last 'flood window' hours.
 *
 * returns -1 on error
 */
static int
if_flooding_mbox(
	char *recipient,	/* recipient of sysevent message */
	char *msg,		/* msg describing the event */
	time_t timestamp,	/* timestamp of event */
	int window)	/* window during which email should be suppressed */
{

	time_t last_attempt_at;
	struct stat statbuf;
	int fd = -1;
	FILE *infile = NULL, *tempfile = NULL;
	char tmpfilnam[MAXPATHLEN] = {0};
	char buf[BUFSIZ]	= {0};
	char *tbuf		= NULL;
	char *tokp		= NULL;
	char *addr		= NULL;
	char *timeStr		= NULL;
	boolean_t flood_flag	= B_FALSE;

	if (ISNULL(recipient, msg)) {
		return (-1);
	}

	/*
	 * The timetamp of the event, when the last email attempt was made
	 * is recorded in the EMAIL_RECORDLOG.
	 *
	 * If the EMAIL_RECORDLOG does not exist, create it and add an
	 * entry.
	 */
	if (stat(EMAIL_RECORDLOG, &statbuf) != 0) {
		/* create file for writing */
		if ((fd = open64(EMAIL_RECORDLOG, O_WRONLY | O_CREAT, 0644))
		    != -1) {
			infile = fdopen(fd, "w");
		}
		if (infile == NULL) {
			samerrno = SE_NOTAFILE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), EMAIL_RECORDLOG);
			sam_syslog(LOG_ERR, samerrmsg);
			if (fd != -1) {
				close(fd);
			}
			return (-1);
		}
		fprintf(infile, "%s:%ld:%s\n", recipient, timestamp, msg);
		fclose(infile);
		infile = NULL;
		return (0);
	}

	/* Create a temporary file */
	if (mk_wc_path(EMAIL_RECORDLOG, tmpfilnam, sizeof (tmpfilnam)) != 0) {
		sam_syslog(LOG_ERR, samerrmsg);
		return (-1);
	}
	if ((fd = open64(tmpfilnam,  O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		tempfile = fdopen(fd, "w+");
	}

	if (tempfile == NULL) {
		samerrno = SE_NOTAFILE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam);
		sam_syslog(LOG_ERR, samerrmsg);
		if (fd != -1) {
			close(fd);
		}
		unlink(tmpfilnam);
		return (-1);
	}

	/* file already exists, open it for reading */
	if ((infile = fopen(EMAIL_RECORDLOG, "r")) == NULL) {

		samerrno = SE_FILE_READ_FAILED;
		/* do not use GetCustMsg and strerror in the same call */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), EMAIL_RECORDLOG, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		sam_syslog(LOG_ERR, "%s", samerrmsg);

		fclose(tempfile);
		unlink(tmpfilnam);
		return (-1);
	}
	while (fgets(buf, BUFSIZ, infile) != NULL) {

		/* retain a copy of the inline for later use */
		tbuf = strdup(buf);
		if (tbuf == NULL) {
			fclose(tempfile);
			unlink(tmpfilnam);
			fclose(infile);
			return (-1);
		}
		/* tokenize addr:time:msg */
		tokp = strtok(tbuf, ":");
		if (tokp != NULL) {
			/* get the recipient's email address */
			addr = tokp;

			/* get the timestamp */
			tokp = strtok(NULL, ":");
			if (tokp != NULL) {
				last_attempt_at = strtol(tokp, &timeStr, 10);
				if (tokp == timeStr) {
					/* non-numeric, incorrect format */
					fclose(tempfile);
					unlink(tmpfilnam);
					fclose(infile);
					free(tbuf);
					return (-1);
				}
			}
			/* get the msg */
			tokp = strtok(NULL, ":");

			if (addr != NULL &&
			    tokp != NULL &&
			    (strncmp(recipient, addr,
			    strlen(recipient)) == 0) &&
			    (strncmp(msg, tokp, strlen(msg)) == 0)) {


				if (timestamp - last_attempt_at > window) {

					/* rewrite the timestamp */
					fprintf(tempfile, "%s:%ld:%s\n",
					    addr, timestamp, msg);
				} else {
					/* do not send email, (flooding) */
					flood_flag = B_TRUE;
					fprintf(tempfile, "%s:%ld:%s\n",
					    addr, last_attempt_at, msg);
				}
			} else {
				/* copy line to tempfile */
				fprintf(tempfile, "%s", buf);
			}
		}
		free(tbuf);
	}
	unlink(EMAIL_RECORDLOG);
	fclose(tempfile);

	if (rename(tmpfilnam, EMAIL_RECORDLOG) < 0) {
		samerrno = SE_RENAME_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam, EMAIL_RECORDLOG);

		sam_syslog(LOG_ERR, samerrmsg);
		return (-1);
	}

	return (flood_flag == B_TRUE ? 1 : 0);
}
/*
 * arguments are positional and expected to be ordered, thus used in a
 * hard-coded manner.
 * This order is:
 * argv[1] -> component ID
 * argv[3] -> Error Type
 * argv[4] -> Error ID
 * argv[5] -> Host name
 * argv[6] -> Message
 * argv[7] -> Time
 * argv[8] -> Sequence number
 * argv[9] -> Action flag	- optional
 */

int
main(int argc, char *argv[])
{
	int		j;
	uint32_t action_flag = 0;

	/* Check the number of args passed in */
	if (argc < NUMARGS) {
		return (1);
	}

	/* the args come in as quoted strings... */
	for (j = 0; j < argc; j++) {
		strip_quotes(argv[j]);
	}

	/*
	 * The sysevent 'High water mark exceeded' from the FS_CLASS,
	 * Msg number 31272 has to be recorded in a flat file for further
	 * processing:- to calculate the number of times the high water
	 * mark exceeded in a given timeframe
	 */
	if ((strncmp(argv[1], FS_CLASS, strlen(argv[1])) == 0) &&
	    (strncmp(argv[2], "HwmExceeded", strlen(argv[2])) == 0)) {

		/* write fsname and timestamp to hwm_monitor file */
		(void) record_hwm(
		    argv[6] /* hwm_exceed_msg */,
		    argv[7] /* time */);
	}
	/* get the action flag */
	if (argc == 10) { /* action flag must be present */

		action_flag = (uint32_t)strtol(argv[9], NULL, 16);

		if ((action_flag & NOTIFY_AS_EMAIL) == NOTIFY_AS_EMAIL) {

			email(argv[1], argv[2], strtol(argv[3], NULL, 0),
			    argv[5], argv[6], argv[7]);
		}

		if ((action_flag & NOTIFY_AS_TRAP) == NOTIFY_AS_TRAP) {

			sendtrap(argv[1],
			    argv[2],
			    strtol(argv[3], NULL, 0),
			    strtol(argv[4], NULL, 0),
			    argv[5],
			    argv[6],
			    argv[7]);
		}

		if ((action_flag & NOTIFY_AS_FAULT) == NOTIFY_AS_FAULT) {
			fault_history(argv);
		}

	} else {

		/* default action: send trap and persist fault */
		sendtrap(argv[1],
		    argv[2],
		    strtol(argv[3], NULL, 0),
		    strtol(argv[4], NULL, 0),
		    argv[5],
		    argv[6],
		    argv[7]);

		fault_history(argv);

	}
	return (0);
}
