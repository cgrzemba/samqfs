/*
 * trace.c - Trace file functions.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

/*
 * Provide event tracing facility for SAM processes.
 * Each daemon process composes trace messages in Trace().
 *
 * sam-fsd will periodically check the trace file.  If the trace file has
 * been removed, a new trace file will be created, and the TrChange field
 * incremented.
 *
 * Signals are traced with no interlock.  In this case, the trace message is
 * written to directly to the trace file.  Using the regular trace mechanism
 * may lead to a "deadly embrace" lockup on the trace mutex.
 */

#pragma ident "$Revision: 1.38 $"

/* ANSI C headers. */
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
struct tm *localtime_r(const time_t *clock, struct tm *res);
#include <unistd.h>

/* Solaris headers. */
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/names.h"
#define	TRACE_NAMES
#define	TRACE_CONTROL
#include "sam/sam_trace.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Macros. */
#if !defined(TEST)
#define	MAXLINE 1024+256
#else
#define	MAXLINE 100
#endif

/* Private data. */
static pthread_mutex_t traceMutex = PTHREAD_MUTEX_INITIALIZER;
static boolean_t mpLock;
static struct TraceCtlBin *traceCtlBin;
static struct TraceCtlEntry nullTraceCtl = { "", 0, 0, 0, 0, 0, 0, 0, 0 };
static struct TraceCtlEntry *traceCtl = &nullTraceCtl;
static FILE *traceSt = NULL;
static char traceBuf[MAXLINE];
static int traceChange = 0;
static int traceFd = -1;

/* For processing trace control settings. */
static jmp_buf errReturn;		/* Error message return */
static struct TraceCtlEntry defTrace = { /* Default trace controls */
	SAM_VARIABLE_PATH"/trace",	/* TrFname */
	TR_def_events,			/* TrFlags */
	0,				/* TrFsdPid */
	0,				/* TrChange */
	0,				/* TrAge */
	0,				/* TrSize */
	0,				/* TrRotTime */
	0,				/* TrRotSize */
	0				/* TrCurTime */
};

static char errMsg[256];

/* Public data. */
char *TraceName = "";
int TracePid;
uint32_t *TraceFlags = &nullTraceCtl.TrFlags;

/* Private functions. */
static void err(int msgNum, ...);
static void openTraceFile(void);
static void lockTraceFile(int lock);
static void setFile(char *fname, struct TraceCtlEntry *tc);
static void setOptions(char *optString, struct TraceCtlEntry *tc);
static void setSize(char *value, struct TraceCtlEntry *tc);
static void setAge(char *value, struct TraceCtlEntry *tc);


#if defined(DEBUG)
/*
 * Send assertion message.
 */
void
AssertMessage(
	char *SrcFile,	/* Caller's source file. */
	int SrcLine,	/* Caller's source line. */
	char *msg)
{
	defTrace.TrFlags |= (1 << TR_module) | (1 << TR_err);
	TraceFlags =  &defTrace.TrFlags;
	errno = 0;
	_Trace(TR_err, SrcFile, SrcLine, "%s", msg);
	sam_syslog(LOG_WARNING, traceBuf);
}


/*
 * Process failed assertion.
 * Write assertion failed message to trace file and syslog.
 * Provide a core file for analysis.
 */
void
_Assert(
	char *SrcFile,	/* Caller's source file. */
	int SrcLine)	/* Caller's source line. */
{
	AssertMessage(SrcFile, SrcLine, "Assertion failed");
	abort();
}
#endif /* defined(DEBUG) */


/*
 * Close the trace file.
 * Flush the buffer and unlock the trace file.
 */
void
TraceClose(
	int TrcLen)
{
	if (traceSt != NULL) {
		(void) fflush(traceSt);
	}
	if (traceSt != NULL && traceFd > STDERR_FILENO) {
		if (TrcLen < 0) {
			struct stat buf;

			if (fstat(traceFd, &buf) == 0) {
				traceCtl->TrCurSize = buf.st_size;
			}
		} else {
			traceCtl->TrCurSize += (fsize_t)TrcLen;
		}
		/*
		 * Check trace file size for the rotation.
		 */
		if ((traceCtl->TrSize > 0) &&
		    (traceCtl->TrCurSize > traceCtl->TrSize) &&
		    (traceCtl->TrFsdPid > 0) &&
		    (traceChange == traceCtl->TrChange)) {
			(void) kill(traceCtl->TrFsdPid, SIGALRM);
			traceCtl->TrFsdPid = 0;
		}
	}
	if (mpLock && traceFd != -1) {
		lockTraceFile(FALSE);
	}
	pthread_mutex_unlock(&traceMutex);
}


/*
 * Return option string.
 */
char *
TraceGetOptions(
	struct TraceCtlEntry *tc,
	char *buf,		/* Buffer for the options */
	size_t bufSize)		/* Size of the buffer */
{
	static char ourBuf[STR_OPTIONS_BUF_SIZE];
	char	*p, *pe;

	if (buf == NULL) {
		buf = ourBuf;
		bufSize = sizeof (ourBuf);
	}
	if (tc == NULL || bufSize < 1) {
		return ("");
	}
	p = buf;
	pe = p + bufSize - 1;
	if (tc->TrFlags != 0) {
		boolean_t first;
		int		i;

		first = TRUE;
		for (i = 0; i < TR_MAX; i++) {
			if (tc->TrFlags & (1 << i)) {
				if (first) {
					first = FALSE;
				} else {
					if (Ptrdiff(pe, p) < 1) {
						break;
					}
					*p++ = ' ';
				}
				strncpy(p, TR_names[i], Ptrdiff(pe, p)-1);
				p += strlen(p);
			}
		}
	}
	*p = '\0';
	return (buf);
}


/*
 * Initialize trace file processing.
 */
void
TraceInit(
	char *programName,	/* Name of calling program file */
	int idmp)		/* Trace id + multi-process lock flag */
{
	static boolean_t first = TRUE;
	int		id;

	if (!first) {
		/*
		 * Only allow one initialization.
		 */
		return;
	}
	first = FALSE;

	/*
	 * Select daemon to trace.
	 */
	id = idmp & 0xff;
	if (id > TI_none && id < TI_MAX) {
		traceCtlBin = MapFileAttach(TRACECTL_BIN,
		    TRACECTL_MAGIC, O_RDWR);
		if (traceCtlBin != NULL) {
			traceCtl = &traceCtlBin->entry[id];
			TraceFlags = &traceCtl->TrFlags;
			traceChange = -1;
		}
	} else {
		if (id > TI_MAX) {
			return;
		}
		/*
		 * id == TI_none - user requested trace to stderr.
		 * Use internal trace control.
		 */
		memset(&nullTraceCtl, 0, sizeof (nullTraceCtl));
		nullTraceCtl.TrFlags = TR_def_events;
		traceCtl = &nullTraceCtl;
		TraceFlags = &nullTraceCtl.TrFlags;
		traceChange = 0;
		traceSt = stderr;
		traceFd = STDERR_FILENO;
	}
	TraceName = programName;
	TracePid = getpid();
	mpLock = (idmp & TR_MPLOCK) ? TRUE : FALSE;
}


/*
 * Open the trace file and return the file stream.
 * If required, the trace file will be locked.
 * The trace file will be reopened if the TrChange indicates that the file
 * has been changed.
 * Returns trace file stream.  NULL if not open.
 */
FILE *
TraceOpen(void)
{
	pthread_mutex_lock(&traceMutex);

	/*
	 * Check trace file change.
	 */
	if (traceChange != traceCtl->TrChange) {
		traceChange = traceCtl->TrChange;
		openTraceFile();
	}
	if (traceSt == NULL) {
		/*
		 * Returning NULL indicates to caller that trace won't be done.
		 * Caller will not call TraceClose().
		 */
		pthread_mutex_unlock(&traceMutex);
	} else if (mpLock && traceFd != -1) {
		lockTraceFile(TRUE);
	}
	return (traceSt);
}


/*
 * Reconfigure tracing.
 */
void
TraceReconfig(void)
{
	pthread_mutex_lock(&traceMutex);
	openTraceFile();
	pthread_mutex_unlock(&traceMutex);
}


/*
 * Trace signal.
 * The signal trace message is written directly to the trace file via stdout.
 */
void
TraceSignal(
	int SigNum)
{
	if ((*TraceFlags & (1 << TR_sig)) && traceFd >= 0) {
		char	msg[128];
		char	*name = "";

		if (TraceName != NULL) {
			name = TraceName;
		}
		snprintf(msg, sizeof (msg),
		    "%s[%ld.%llu]: Received signal %d\n",
		    name, TracePid, (unsigned long long)pthread_self(),
		    SigNum);
		(void) write(traceFd, msg, strlen(msg));
	}
}


/*
 * Write trace message to trace file.
 */
void
_Trace(
	const TR_type type,	/* Type of trace message */
	const char *SrcFile,	/* Caller's source file. */
	const int SrcLine,	/* Caller's source line. */
	const char *fmt,	/* printf() style format. */
	...)
{
	struct tm tm;
	time_t	now;
	char *p, *pe;
	char *tdformat;
	int saveErrno;

	if (!(*TraceFlags & (1 << type) & TR_events_allowed)) {
		return;
	}

	saveErrno = errno;
	if (TraceOpen() == NULL) {
		errno = saveErrno;
		return;
	}

	/* Enter elements in message. */
	p = traceBuf;
	pe = p + sizeof (traceBuf) - 2;	/* Room for \n */

	/* Date and time. */
	if (*TraceFlags & (1 << TR_date)) {
		tdformat = "%Y-%m-%d %T ";
	} else {
		tdformat = "%H:%M:%S ";
	}
	now = time(NULL);
	strftime(p, sizeof (traceBuf)-1, tdformat, localtime_r(&now, &tm));
	p += strlen(p);

	/* Program name and pid */
	if (TraceName != NULL) {
		snprintf(p, Ptrdiff(pe, p), "%s[%ld:%llu]: ",
		    TraceName, TracePid,
		    (unsigned long long)pthread_self());
		p += strlen(p);
	}

	/* Source module */
	if ((*TraceFlags & (1 << TR_module)) && SrcFile != NULL) {
		snprintf(p, Ptrdiff(pe, p), "%s:%d ", SrcFile, SrcLine);
		p += strlen(p);
	}

	/* Trace type */
	if ((*TraceFlags & (1 << TR__type)) && 0 <= type && type < TR_MAX) {
		snprintf(p, Ptrdiff(pe, p), "%s ", TR_names[type]);
		p += strlen(p);
	}

	/* The message */
	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		vsnprintf(p, Ptrdiff(pe, p), fmt, args);
		va_end(args);
		p += strlen(p);
	}

	/* Error number */
	if ((type == TR_err || type == TR_debugerr) && saveErrno != 0) {
		snprintf(p, Ptrdiff(pe, p), ": ");
		p += strlen(p);
		(void) StrFromErrno(saveErrno, p, Ptrdiff(pe, p));
		p += strlen(p);
	}

	*p++ = '\n';
	*p = '\0';
	fputs(traceBuf, traceSt);
	TraceClose(Ptrdiff(p, traceBuf));
	errno = saveErrno;
}


/*
 * Set trace controls.
 */
char *
TraceControl(
	char *identArg,		/* Identifier daemon.item */
	char *value,		/* Value to set */
	struct TraceCtlBin *tb)	/* Trace control binary file. */
				/*    If NULL, attach it */
{
	static struct {
		char	*CtIdent;
		void	(*CtFunc)(char *value, struct TraceCtlEntry *tc);
	} *ct, controls[]  = {
		{ "file", setFile },
		{ "options", setOptions },
		{ "size", setSize },
		{ "age", setAge },
		{ NULL }
	};
	upath_t ident;
	char *item;
	struct TraceCtlEntry *tc;
	int tid;

	if (tb == NULL) {
		if (traceCtlBin == NULL) {
			traceCtlBin = MapFileAttach(TRACECTL_BIN,
			    TRACECTL_MAGIC, O_RDWR);
			if (traceCtlBin == NULL) {
				return (GetCustMsg(14200));
			}
		}
		tb = traceCtlBin;
	}

	/*
	 * Isolate first item - the daemon name - and look it up.
	 */
	strncpy(ident, identArg, sizeof (ident)-1);
	if (setjmp(errReturn) != 0) {
		return (errMsg);
	}
	item = strtok(ident, ".");
	if (strcmp(item, "all") == 0) {
		tid = 0;
		tc = &defTrace;
	} else {
		for (tid = 1; strcmp(item, traceIdNames[tid]) != 0; tid++) {
			if (tid >= TI_MAX) {
				/* Invalid daemon name '%s' */
				err(14201, item);
			}
		}
		tc = &tb->entry[tid];
	}

	/*
	 * Get the next item.
	 */
	item = strtok(NULL, ".");
	if (item == NULL) {
		/*
		 * For no data item, we can have 'on' or 'off'.
		 */
		if (strcmp(value, "on") == 0) {
			if (tid == 0) {
				/*
				 * On for all daemons.
				 */
				while (++tid < TI_MAX) {
					tc = &tb->entry[tid];
					snprintf(tc->TrFname,
					    sizeof (tc->TrFname), "%s/%s",
					    defTrace.TrFname,
					    traceIdNames[tid]);
					tc->TrFlags = defTrace.TrFlags;
					tc->TrAge = defTrace.TrAge;
					tc->TrSize = defTrace.TrSize;
				}
			} else {
				/*
				 * On for this daemon.
				 */
				snprintf(tc->TrFname, sizeof (tc->TrFname),
				    "%s/%s", defTrace.TrFname,
				    traceIdNames[tid]);
				tc->TrFlags = defTrace.TrFlags;
				tc->TrAge = defTrace.TrAge;
				tc->TrSize = defTrace.TrSize;
			}
		} else if (strcmp(value, "off") == 0) {
			if (tid == 0) {
				/*
				 * No tracing for all daemons.
				 */
				while (++tid < TI_MAX) {
					tc = &tb->entry[tid];
					*tc->TrFname = '\0';
				}
			} else {
				/*
				 * No tracing for this daemon.
				 */
				*tc->TrFname = '\0';
			}
		} else {
			/* Must have 'on' or 'off' */
			err(14202);
		}
		tc->TrChange++;
		return ("");
	}

	/*
	 * Look up the item and process it.
	 */
	for (ct = controls; ct->CtIdent != NULL; ct++) {
		if (strcmp(item, ct->CtIdent) == 0) {
			break;
		}
	}
	if (ct->CtIdent == NULL) {
		/* Unknown variable '%s' */
		err(14203, item);
	}
	ct->CtFunc(value, tc);
	if (tid == 0) {
		/*
		 * Set value for all daemons.
		 */
		while (++tid < TI_MAX) {
			tc = &tb->entry[tid];
			if (ct->CtFunc == setFile) {
				ct->CtFunc(traceIdNames[tid], tc);
			} else {
				ct->CtFunc(value, tc);
			}
		}
	}
	tc->TrChange++;
	return ("");
}


/* Private functions. */


/*
 * Error return.
 * Compose message in response buffer.
 */
static void
err(
	int msgNum,
	...)
{
	va_list args;
	char *fmt;

	va_start(args, msgNum);
	if (msgNum != 0) {
		fmt = GetCustMsg(msgNum);
	} else {
		fmt = va_arg(args, char *);
	}
	(void) vsnprintf(errMsg, sizeof (errMsg), fmt, args);
	longjmp(errReturn, 1);
}


/*
 * Open the trace file.
 */
static void
openTraceFile(void)
{
	if (traceSt != NULL && traceFd > STDERR_FILENO) {
		(void) fclose(traceSt);
	}
	if (*traceCtl->TrFname != '\0') {
		traceFd = open(traceCtl->TrFname,
		    O_WRONLY | O_CREAT | O_APPEND | SAM_O_LARGEFILE, 0666);
	} else {
		traceFd = -1;
	}
	if (traceFd < 0) {
		traceSt = NULL;
	} else {
		struct stat buf;

		traceSt = fdopen(traceFd, "w");
		if (mpLock) {
			lockTraceFile(TRUE);
		}
		if (fstat(traceFd, &buf) == 0) {
			traceCtl->TrCurSize = buf.st_size;
		}
		if (mpLock) {
			lockTraceFile(FALSE);
		}
	}
}

/*
 * Lock Trace File.
 */
void
lockTraceFile(
	int lock)
{
	flock_t arg;
	int cmd;

	memset(&arg, 0, sizeof (arg));
	arg.l_whence = SEEK_SET;
	if (lock) {
		arg.l_type = F_WRLCK;
		cmd = F_SETLKW;
	} else {
		arg.l_type = F_UNLCK;
		cmd = F_SETLK;
	}
	if (fcntl(traceFd, cmd, &arg) == -1) {
		sam_syslog(LOG_ERR, "Fatal OS call error: fcntl(%d) errno= %d",
		    arg.l_type, errno);
		abort();
	}
}

/*
 * Set trace file name.
 */
static void
setFile(
	char *fname,
	struct TraceCtlEntry *tc)
{
	if (tc == &defTrace) {
		/*
		 * For "all", set trace base.
		 */
		if (*fname != '/') {
			/* Default trace path must be absolute */
			err(14203);
		}
		strncpy(defTrace.TrFname, fname, sizeof (defTrace.TrFname)-1);
	} else {
		if (*fname == '/') {
			/*
			 * Absolute file name.
			 */
			strncpy(tc->TrFname, fname, sizeof (tc->TrFname)-1);
		} else {
			/*
			 * Use trace base.
			 */
			snprintf(tc->TrFname, sizeof (tc->TrFname), "%s/%s",
			    defTrace.TrFname, fname);
		}
	}
}


/*
 * Set age.
 */
static void
setAge(
	char *value,
	struct TraceCtlEntry *tc)
{
	int		age;

	if (StrToInterval(value, &age) != 0) {
		/* Invalid age */
		err(14204);
	}
	tc->TrAge = (uint32_t)age;
}


/*
 * Set trace options.
 */
static void
setOptions(
	char *optString,
	struct TraceCtlEntry *tc)
{
	char *opt;
	char *prevOpt;
	uint32_t flags;

	/*
	 * Start with the existing flags.
	 * Look up each trace option and add it to the flags.
	 */
	flags = tc->TrFlags;
	opt = strtok(optString, " ");
	prevOpt = NULL;
	while (opt != NULL) {
		TR_type	n;
		char	*p;

		/*
		 * Restore input string.
		 */
		if (prevOpt != NULL) {
			prevOpt[strlen(prevOpt)] = ' ';
		}
		prevOpt = opt;
		p = opt;
		if (*opt == '-') {
			p++;
		}

		for (n = 0; strcmp(p, TR_names[n]) != 0; n++) {
			if (n >= TR_MAX) {
				/* Unknown trace option: %s */
				err(14205, opt);
			}
		}
		if (n == TR_none) {
			flags = 0;
		} else if (n == TR_all) {
			flags &= ~((1 << TR_date) - 1);
			flags |= TR_all_events;
		} else if (*opt != '-') {
			flags |= 1 << n;
		} else {
			flags &= ~(1 << n);
		}
		opt = strtok(NULL, " ");
	}
	tc->TrFlags = flags;
}


/*
 * Set rotation size.
 */
static void
setSize(
	char *size,
	struct TraceCtlEntry *tc)
{
	fsize_t value;

	if (StrToFsize(size, &value) == -1) {
		/* Invalid size */
		err(14206);
	}
	tc->TrSize = value;
}


#if defined(TEST)

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


int
main(
	int argc,
	char *argv[])
{
	static char fname[MAXLINE + 10];

	fname[0] = '\0';
	while (strlen(fname) < sizeof (fname)) {
		strncat(fname, "file0000xxx12345/", sizeof (fname));
	}

	Trace(TR_MISC, "Default behavior - before initialization.");

	if (argc > 1) {
		TraceName = argv[1];
	}
	TraceInit("trace_test", TI_none);
	Trace(TR_MISC, "First test message");
	fprintf(stdout, "Output to stdout\n");
	(void) fflush(stdout);
	fprintf(stderr, "Output to stderr\n");
	(void) fflush(stderr);
	*TraceFlags |= 1 << TR_module;
	Trace(TR_MISC, "module added");
	*TraceFlags |= 1 << TR_date;
	Trace(TR_MISC, "date added");
	*TraceFlags |= 1 << TR__type;
	Trace(TR_MISC, "type added");
	printf("Trace options: '%s'\n", TraceGetOptions(traceCtl, NULL, 0));
	Trace(TR_CUST, " - cust type");
	Trace(TR_ERR, " - err type");
	Trace(TR_FATAL, " - fatal type");
	Trace(TR_IPC, " - ipc type");
	Trace(TR_MISC, " - misc type");
	Trace(TR_PROC, " - proc type");
	Trace(TR_QUEUE, " - queue type");
	Trace(TR_ALLOC, " - alloc type");
	Trace(TR_FILES, " - files type");
	Trace(TR_OPRMSG, " - oprmsg type");
	Trace(TR_DEBUG, " - debug type");
	Trace(TR_SIG, " - sig type");
	Trace(TR_ERR, "Long name %s", fname);

	TraceSignal(SIGPIPE);

	return (EXIT_SUCCESS);
}

#endif /* defined(TEST) */
