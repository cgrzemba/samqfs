/*
 * custmsg.c - Process messages destined for customers.
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

#pragma ident "$Revision: 1.45 $"

/*
 * static char *_SrcFile = __FILE__;$* Using __FILE__ makes duplicate strings
 */

/* Feature test switches. */
/* TEST	If defined, compile main() for testing. */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* POSIX headers. */
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <locale.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <syslog.h>
#include <nl_types.h>

/* SAM-FS headers. */
#define DEC_INIT
#include "sam/types.h"
#undef DEC_INIT
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/param.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "sam/defaults.h"

/* Solaris sysevent (SNMP) headers */
#ifdef sun
#include <libsysevent.h>
#include <libnvpair.h>
#endif	/* sun */

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */


/* Macros. */
#if	!defined(TEST)
#define	MAXLINE 1024+256
#else
#define	MAXLINE 60
#endif
#define	NUMOF(a) (sizeof (a)/sizeof (*(a)))

/* sysevent attributes	*/
#define	SYSEVENT_PUBLISHER	"SUNWsamfs"
#define	SYSEVENT_MESSAGE	"MESSAGE"
#define	SYSEVENT_ERRORTYPE	"ERRORTYPE"
#define	SYSEVENT_CLASS		"COMPONENT"
#define	SYSEVENT_SUBCLASS	"SUBCOMPONENT"
#define	SYSEVENT_FAULTTIME	"FAULTTIME"
#define	SYSEVENT_SYSTEMID	"SYSTEMID"
#define	SYSEVENT_ERRORID	"ERRORID"
#define	SYSEVENT_ACTION		"ACTION"

/* Private data. */
static nl_catd ourCatFd = NULL;
static void (*notifyFunc)	/* Notify function */
	(int priority, int msgNum, char *msg) = NULL;
static int logMode = 0;		/* Demon process, send messages to syslog. */
/* Public data. */
int ErrorExitStatus = EXIT_FATAL;

/* Private functions. */
static char *getMsg(int msgNum);
static char *sendMsg(int msgNum, char *msg);
static void samCatopen(void);

#ifdef sun
static pthread_mutex_t sam_flood_events_mutex = PTHREAD_MUTEX_INITIALIZER;
static sam_defaults_t *defaults = NULL;	/* snmp support is on/off? */
/*
 * Some events are sent one per file and therefore these events can occur
 * once every second or multiple times in a fraction of a second
 * e.g. 4028, 19023 etc. are sent per file, depending
 * on the operation and the number of files being offline (4028), the list of
 * events sent is large enough to cause a flood. for these type of events, a
 * generic message should be sent and that should not be repeated for an
 * chosen optimum window
 *
 * Keep a table of the msgnum and the associated window within
 * which the event should be curbed. This is to avoid flooding the syseventd
 */
#define	NUM_SAM_FLOOD_EVENTS	(sizeof sam_flood_events /sizeof (struct event))
/* this table should be sorted by msgnum, this order is important */
static struct event {
	int msgnum;
	time_t last_sent;
	time_t window;
} sam_flood_events[] = {
	{ 4028, 0, 600 }, /* cannot archive, offline with all copies damaged */
	{ 19023, 0, 600 }, /* Unable to create removable media file */
	{ 20218, 0, 600 }, /* expired archive images */
	{ 31236, 0, 600 }, /* SE_RESTORE_FAILED */
	{ 31272, 0, 600 }, /* High water mark crossed */
	};


static int get_index(int msgnum, struct event e[]);

#endif	/* sun */

/*
 * Initialize syslog(), if running as daemon.
 */
void
CustmsgInit(
	int logMode_a,	/* Syslog message flag.  0 if not running as a demon. */
	void (*notifyFunc_a)	/* Notify function */
	(int priority, int msg_num, char *msg))
{
	notifyFunc = notifyFunc_a;
	samCatopen();
	logMode = logMode_a;
#ifdef sun
	defaults = (sam_defaults_t *)GetDefaults();
#endif	/* sun */
}


/*
 * Close all files for customer messages.
 */
void
CustmsgTerm(void)
{
	(void) catclose(ourCatFd);
	logMode = 0;
}


/*
 * Return message text for a customer message number.
 * Public version:  skips destination string.
 */
char *
GetCustMsg(
	int msgNum)
{
	char	*msg;

	msg = getMsg(msgNum);
	if (*msg == '-') {
		while (*msg != '\0' && !isspace(*msg)) {
			msg++;
		}
		while (*msg != '\0' && isspace(*msg)) {
			msg++;
		}
	}
	return (msg);
}


/*
 * Process library error message.
 * Compose a message from the supplied catalog message number.
 * Call the supplied function with the severity code and message.
 * If the supplied function is NULL,
 * Print the message to stderr.
 * If the exit_status is non_zero, exit(exit_status);
 * Otherwise, return.
 */
void
LibError(
	void (*MsgFunc)(int code, char *msg),	/* Callers function to */
						    /* handle message */
	int exit_status,			/* Exit status */
	int msgNum,				/* Message catalog number */
	...)					/* Message arguments */
{
	va_list	args;
	char *fmt;
	char msg_buf[MAXLINE];
	char *p, *pe;
	int saveErrno;

	saveErrno = errno;
	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}
	p = msg_buf;
	pe = p + sizeof (msg_buf) - 1;
	if (fmt != NULL) {
		vsnprintf(p, Ptrdiff(pe, p), fmt, args);
		p += strlen(p);
	}
	va_end(args);

	/* Error number */
	if (saveErrno != 0) {
		snprintf(p, Ptrdiff(pe, p), ": ");
		p += strlen(p);
		(void) StrFromErrno(saveErrno, p, Ptrdiff(pe, p));
		p += strlen(p);
	}
	*p = '\0';

	if (NULL != MsgFunc) {
		errno = saveErrno;
		MsgFunc(exit_status, msg_buf);
		errno = saveErrno;
		return;
	}
	fprintf(stderr, "%s\n", msg_buf);
	if (exit_status != 0) {
		exit(exit_status);
	}
}


/*
 * Out of memory error.
 * Write a message and terminate.
 */
void
Nomem(
	const char *functionName,
	char *objectName,
	size_t size)
{
	char msg_buf[MAXLINE];
	char *msg;

	snprintf(msg_buf, sizeof (msg_buf), "%s: %s(%s, %u)", GetCustMsg(14081),
	    functionName, objectName, size);
	msg = sendMsg(14081, msg_buf);
	_Trace(TR_err, NULL, 0, "%s", msg);
#if !defined(TEST)
	exit(EXIT_NOMEM);

#else /* !defined(TEST) */
	printf("*** Nomem() would exit(%d)\n", EXIT_NOMEM);
#endif /* !defined(TEST) */
}


/*
 * Send a customer message.
 */
void
SendCustMsg(
	const char *srcFile,	/* Caller's source file. */
	const int srcLine,	/* Caller's source line. */
	int msgNum,		/* Message catalog number. */
	...)
{
	va_list	args;
	char *fmt;
	char msg_buf[MAXLINE];
	char *msg;
	int saveErrno;

	saveErrno = errno;
	fmt = getMsg(msgNum);
	va_start(args, msgNum);
	vsnprintf(msg_buf, sizeof (msg_buf), fmt, args);
	va_end(args);
	msg = sendMsg(msgNum, msg_buf);
	if (TraceFlags != NULL) {
		_Trace(TR_cust, srcFile, srcLine, "Message %d: %s",
		    msgNum, msg);
	}
	errno = saveErrno;
}


/*
 * Fatal error related to a system call.
 * Write a message with the errno value to the logfile and terminate.
 */
void
_LibFatal(
	const char *srcFile,		/* Caller's source file. */
	const int srcLine,		/* Caller's source line. */
	const char *functionName,	/* Name of failing function. */
	const char *functionArg)	/* Argument to function */
{
	char msg_buf[MAXLINE];
	char *msg;
	char *p, *pe;
	int saveErrno;

	saveErrno = errno;

	/*
	 * Format message and add error number.
	 */
	p = msg_buf;
	pe = p + sizeof (msg_buf) - 1;
	snprintf(p, Ptrdiff(pe, p), "%s", getMsg(14080));
	p += strlen(p);
	if (functionArg == NULL) {
		functionArg = "NULL";
	}
	snprintf(p, Ptrdiff(pe, p), ": %s(%s) called from: %s:%d",
	    functionName, functionArg, srcFile, srcLine);
	p += strlen(p);
	if (saveErrno != 0) {
		snprintf(p, Ptrdiff(pe, p), ": ");
		p += strlen(p);
		(void) StrFromErrno(saveErrno, p, Ptrdiff(pe, p));
		p += strlen(p);
	}
	*p = '\0';

	msg = sendMsg(14080, msg_buf);
	errno = 0;	/* Already included error number */
	_Trace(TR_err, srcFile, srcLine, "%s", msg);
#if !defined(TEST)
	if (ErrorExitStatus == EXIT_FATAL) {
		exit(EXIT_FATAL);
	}
	_exit(ErrorExitStatus);

#else /* !defined(TEST) */
	printf("*** _LibFatal() would exit(%d)\n",
	    (ErrorExitStatus != 0) ? ErrorExitStatus : EXIT_FATAL);
#endif /* !defined(TEST) */
}


/*
 * Error related to a system call.
 * Write a message with the errno value to the logfile.
 */
void
SysError(
	const char *srcFile,	/* Caller's source file. */
	const int srcLine,	/* Caller's source line. */
	const char *fmt,	/* printf() style format. */
	...)
{
	char msg_buf[MAXLINE];
	char *msg;
	char *p, *pe;
	int saveErrno;

	saveErrno = errno;

	/*
	 * Format message and add error number.
	 */
	p = msg_buf;
	pe = p + sizeof (msg_buf) - 1;

	/* Catalog message. */
	snprintf(p, Ptrdiff(pe, p), "%s", getMsg(14082));
	p += strlen(p);
	snprintf(p, Ptrdiff(pe, p), ": ");
	p += strlen(p);

	/* The message */
	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		vsnprintf(p, Ptrdiff(pe, p), fmt, args);
		va_end(args);
		p += strlen(p);
	}

	/* Error number */
	if (saveErrno != 0) {
		snprintf(p, Ptrdiff(pe, p), ": ");
		p += strlen(p);
		(void) StrFromErrno(saveErrno, p, Ptrdiff(pe, p));
		p += strlen(p);
	}
	*p = '\0';

	msg = sendMsg(14082, msg_buf);
	errno = 0;	/* Already included error number. */
	_Trace(TR_err, srcFile, srcLine, "%s", msg);
	errno = saveErrno;
}


/* Private functions. */


/*
 * Private version of GetCustMsg().
 */
static char *
getMsg(
	int msgNum)
{
	static char noMsg[64];
	char	*msg;

	if (ourCatFd == NULL) {
		samCatopen();
	}
	msg = catgets(ourCatFd, SET, msgNum, "");
	if (*msg != '\0') {
		return (msg);
	}
	snprintf(noMsg, sizeof (noMsg), "Message %d", msgNum);
	return (noMsg);
}

#ifdef	linux
#define	LOCALE	"C"
#else
#define	LOCALE	""
#endif	/* linux */

/*
 * Open the message catalog for the current locale or
 * /usr/lib/locale/C/LC_MESSAGES/SUNWsamfs in the C locale.
 */
static void
samCatopen(void)
{
	(void) setlocale(LC_ALL, LOCALE);
	ourCatFd = catopen(NL_CAT_NAME, NL_CAT_LOCALE);

	/*
	 * catopen() gives us a null catalog in the C locale;
	 * we need the real one.
	 */
#ifdef linux
	if (ourCatFd == (nl_catd)-1) {
		ourCatFd = catopen(DEFCAT NL_CAT_NAME, NL_CAT_LOCALE);
	}
#else
	if ((ourCatFd == (nl_catd)-1) || (ourCatFd->__content == NULL)) {
		catclose(ourCatFd);
		ourCatFd = catopen(DEFCAT NL_CAT_NAME, NL_CAT_LOCALE);
	}
#endif /* linux */

	catfd = ourCatFd;
}


/*
 * Private version of SendCustMsg().
 */
static char *
sendMsg(
	int msgNum,	/* Message catalog number. */
	char *msg)
{
	static struct {
		char *name;
		int priority;
	} SyslogPriorities[] = {
		{ "info", LOG_INFO },
		{ "Emerg", LOG_EMERG },
		{ "alert", LOG_ALERT },
		{ "crit", LOG_CRIT },
		{ "err", LOG_ERR },
		{ "warning", LOG_WARNING },
		{ "notice", LOG_NOTICE },
		{ "debug", LOG_DEBUG },
	};
	boolean_t tonotifyFunc;
	boolean_t toSyslog;
	boolean_t toeventFunc;
	char *eventClass = (char *)NULL;
	char *eventSubclass = (char *)NULL;

	int priority;

	tonotifyFunc = FALSE;
	toSyslog = FALSE;
	toeventFunc = FALSE;
	priority = 0;

	/*
	 * Extract destination and priority.
	 */
	if (*msg == '-') {
		msg++;
		toSyslog = FALSE;
		tonotifyFunc = FALSE;
		toeventFunc = FALSE;
		while (*msg != '\0' && !(isspace(*msg))) {
			if (*msg == 'S') {
				toSyslog = TRUE;
			} else if (*msg == 'N') {
				tonotifyFunc = TRUE;
			} else if (*msg == 'V') {
				toeventFunc = TRUE;
			} else if (*msg == '[') {
				eventClass = ++msg;
				while ((*msg != ':') && (*msg != '\0')) {
					msg++;
				}
				if (*msg != '\0') {
					*msg = '\0';
					eventSubclass = ++msg;
					while ((*msg != ']') &&
					    (*msg != '\0')) {
						msg++;
					}
					if (*msg != '\0') {
						*msg = '\0';
					} else {
						continue;
					}
				} else {
					continue;
				}
			} else {
				int		i;

				for (i = 0; i < NUMOF(SyslogPriorities); i++) {
					if (*SyslogPriorities[i].name == *msg) {
						priority = i;
						break;
					}
				}
			}
			msg++;
		}
		while (isspace(*msg)) {
			msg++;
		}
	}
	if (toeventFunc) {
		(void) PostEvent(eventClass, eventSubclass, msgNum,
		    SyslogPriorities[priority].priority, msg,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
	}

	/*
	 * No logging setup.
	 */
	if (logMode == 0) {
		fprintf(stderr, "%s\n", msg);
		return (msg);
	}

	if (toSyslog == FALSE && tonotifyFunc == FALSE &&
	    toeventFunc == FALSE) {
		toSyslog = TRUE;
	}
#if defined(TEST)
	printf("*** %c%c%c %s %d %s %s %s\n", (toSyslog) ? 'S' : '-',
	    (tonotifyFunc) ? 'N' : '-', (toeventFunc) ? 'V' : '-',
	    SyslogPriorities[priority].name,
	    SyslogPriorities[priority].priority,
	    (eventClass) ? eventClass : "-",
	    (eventSubclass) ? eventSubclass : "-", msg);
#else	/* defined(TEST)	*/
	if (toSyslog) {
		sam_syslog(SyslogPriorities[priority].priority, "%s %s\n",
		    SyslogPriorities[priority].name, msg);
	}
#endif	/* defined(TEST)	*/


	if (tonotifyFunc && notifyFunc != NULL) {
		notifyFunc(SyslogPriorities[priority].priority,
		    msgNum, msg);
	}

	return (msg);
}


int
PostEvent(
	char *class,	/* pointer to a string defining the event class */
	char *subclass,	/* pointer to a string defining the event subclass */
	int msgnum,	/* error id (from ctalog.msg) */
	int errortype,	/* severity level indicator */
	char *msg,	/* error message (from catalog.msg) */
	uint32_t action_flag	/* send email, send trap or persist fault */
)
/*
 * PostEvent
 *
 * use an system event framework which would provide a common event generation
 * and delivery mechanism for kernel/user level event producers and  user level
 * event consumers.
 *
 * syseventd accepts delivery of these posted system events and propagates it
 * The system events can be received from syseventd via a configuration file
 * registration. In the configuration file the events are mapped to the action
 * to be taken in case of the event.
 * Subscribers to these events are snmptrap, mailx and persisting in a log.
 *
 * The error id (errorid and msg), component id (publisher and class),
 * system id (hostname), date/time and error type describe an error
 *
 * the class, subclass and msg are all required parameters and cannot be NULL.
 * If any of the args is NULL, the nv_list_add..() will fail and err is returned
 *
 * action_flag is used to determine what is to be done upon receiving the event
 * from syseventd, send a trap, send an email or log the event to a file for
 * event history
 */
{

#ifdef sun

	sysevent_id_t eid;	/* pointer to a system unique identifier */
	nvlist_t *attr_list;	/* name-value pair associated with event */
	upath_t	hostname;	/* system id */
	time_t	curtime	= time(NULL);	/* date-time */
	int err	= -1;			/* return value */

	if (gethostname(hostname, sizeof (hostname)) < 0) {
		sam_syslog(LOG_ERR, GetCustMsg(12526), strerror(errno));
		return (err);
	}
	/*
	 * If snmp support is turned ON, then post the sysevent
	 */
	if ((defaults != NULL) && (defaults->flags & DF_ALERTS)) {
		int index;

		/*
		 * check that the same event is not repeatedly sent
		 * in a short window
		 */
		pthread_mutex_lock(&sam_flood_events_mutex);
		index = -1;
		index = get_index(msgnum, sam_flood_events);
		/*
		 * Get the time when the event was last sent,
		 * get the window within which this event is not
		 * to be repeatedly sent
		 */
		if (index != -1) {
			if ((curtime - sam_flood_events[index].last_sent)
				< sam_flood_events[index].window) {
				pthread_mutex_unlock(&sam_flood_events_mutex);
				return (0); /* don't send */
			}
		}
		/* A different error message is to be sent for 4028, 19023 */
		switch (msgnum) {
			case 4028:
				msg[0] = '\0';
				/* Generic message instead of file specific */
				strcpy(msg, GetCustMsg(4054));
				break;
			case 19023:
				msg[0] = '\0';
				strcpy(msg, GetCustMsg(19035));
				break;
			case 20218:
				msg[0] = '\0';
				/* Send generic message */
				strcpy(msg, GetCustMsg(20327));
				break;
			case 31236:
				msg[0] = '\0';
				/* Send generic message */
				strcpy(msg, GetCustMsg(31243));
				break;
			/* default is to retain the msg */
		}
		/* send the event */
		if ((err = nvlist_alloc(&attr_list, 0, 0)) == 0) {

			err = nvlist_add_string(attr_list,
					SYSEVENT_CLASS, class);

			if (err == 0)
				err = nvlist_add_string(attr_list,
					SYSEVENT_SUBCLASS, subclass);

			if (err == 0)
				err = nvlist_add_string(attr_list,
					SYSEVENT_SYSTEMID, hostname);

			if (err == 0)
				err = nvlist_add_int16(attr_list,
					SYSEVENT_ERRORTYPE, errortype);

			if (err == 0)
				err = nvlist_add_int16(attr_list,
					SYSEVENT_ERRORID, msgnum);

			if (err == 0)
				err = nvlist_add_string(attr_list,
					SYSEVENT_FAULTTIME, ctime(&curtime));

			if (err == 0)
				err = nvlist_add_string(attr_list,
					SYSEVENT_MESSAGE, msg);

			if (err == 0)
				err = nvlist_add_uint32(attr_list,
					SYSEVENT_ACTION, action_flag);

			if (err == 0) {
				/*
				 * publisher is package/product name
				 * SUNWsamfs
				 */

				err = sysevent_post_event(class, subclass,
					SUNW_VENDOR, SYSEVENT_PUBLISHER,
					attr_list, &eid);
			}
			if (err != 0) {
				sam_syslog(LOG_ERR, GetCustMsg(14083),
					strerror(errno));
			} else {
				if (index != -1) {
					sam_flood_events[index].last_sent =
							curtime;
				}
			}
			pthread_mutex_unlock(&sam_flood_events_mutex);
			nvlist_free(attr_list);
		} else {
			sam_syslog(LOG_ERR, GetCustMsg(14084), strerror(errno));
		}
	} else {
		/*
		 * if snmpsupport is not enabled, then the return is 0
		 * as this is not an err
		 */
		err = 0;
	}
	return (err);

#else	/* sun */
	sam_syslog(errortype, "%s %s %d %s", class, subclass, msgnum, msg);
	/* sysevent support not available */
	return (0);
#endif	/* sun */

}


#ifdef sun
static int
get_index(int msgnum, struct event e[])
{
	int low, high, mid;
	low = 0; high = NUM_SAM_FLOOD_EVENTS;

	while (low <= high) {
		mid = (low + high)/2;
		if (msgnum < e[mid].msgnum)
			high = mid - 1;
		else if (msgnum > e[mid].msgnum)
			low = mid + 1;
		else
			return (mid);
	}
	return (-1);
}
#endif	/* sun */


#if defined(TEST)


static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

static void myNotifyFunc(int priority, int msgNum, char *msg);
char *program_name = "test";

/*
 * Test sending messages.
 */
int
main(void)
{
	static char fname[MAXLINE + 10];

	CustmsgInit(184, myNotifyFunc);
	printf("Catalog is %s; locale is %d\n", NL_CAT_NAME, NL_CAT_LOCALE);

	fname[0] = '\0';
	while (strlen(fname) < sizeof (fname)) {
		strncat(fname, "file0000xxx12345/", sizeof (fname));
	}
	errno = ENOENT;
	SysError(HERE, "Test long file name: %s", fname);
	LibFatal(foo, "bar");
	Nomem("SamMalloc", "VsnInfo", 4567);
	SendCustMsg(HERE, 4010, "set.3");
	SendCustMsg(HERE, 4011, "Set.1");
	SendCustMsg(HERE, 4012);
	SendCustMsg(HERE, 4020, "arfind", 2345);
	SendCustMsg(HERE, 4021, 44);
	SendCustMsg(HERE, 4022, "tracer");
	SendCustMsg(HERE, 4023);
	SendCustMsg(HERE, 8003, "arg.1");
	error(0, 23, "error() %s", "check");
	return (EXIT_SUCCESS);
}

static void myNotifyFunc(
	int priority,
	int msgNum,
	char *msg)
{
	printf("    myNotifyFunc(%d, %d, %s)\n", priority, msgNum, msg);
}

#endif /* defined(TEST) */
