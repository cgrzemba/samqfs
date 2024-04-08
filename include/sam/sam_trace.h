/*
 * trace.h - Sam trace and debug facility definitions.
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
 * or https://illumos.org/license/CDDL.
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

#ifndef SAM_TRACE_H
#define	SAM_TRACE_H

#ifdef sun
#pragma ident "$Revision: 1.39 $"
#endif

/* Debug Macros. */
#if defined(DEBUG)
#define	ASSERT(f) { if (!(f))  _Assert(_SrcFile, __LINE__, 0); }
#define	ASSERT_NOT_REACHED(f) assert(0);
#define	ASSERT_WAIT_FOR_DBX(f) { if (!(f))  _Assert(_SrcFile, __LINE__, 1); }
#else /* defined(DEBUG) */
#define	ASSERT(f) {}
#define	ASSERT_NOT_REACHED(f) {}
#define	ASSERT_WAIT_FOR_DBX(f) {}
#endif /* defined(DEBUG) */

/*
 * Types.
 *
 * The trace types are used in trace messages.
 * Bits are set in TraceFlags to turn on the message writing.
 */
typedef enum {
	/* Trace events */
	TR_cust,		/* Customer notification */
	TR_err,			/* Non-fatal program "errors" */
	TR_fatal,		/* Fatal notification */
	TR_ipc,			/* Inter process communication */
	TR_misc,		/* Miscellaneous */
	TR_proc,		/* Process initiation and completion */

	TR_all,			/* Include all events above */

	TR_rft,			/* File transfer events */
	TR_alloc,		/* Memory allocations */
	TR_files,		/* File actions */
	TR_oprmsg,		/* Operator message */
	TR_queue,		/* Archiver queue contents when changed */
	TR_dbfile,		/* Database file events */

	/* Available in DEBUG only */
	TR_debug,		/* Debug messages */
	TR_debugerr,		/* Debug messages with error number */
	TR_sig,			/* Signals received */

	/* Optional message elements */
	TR_date,
	TR_module,
	TR__type,

	TR_none,		/* Do not include any events */
	TR_MAX
} TR_type;

/* Events mask. */
#if defined(DEBUG)
#define	TR_events_allowed ((1 << TR_date) - 1)
#else /* defined(DEBUG) */
#define	TR_events_allowed ((1 << TR_debug) - 1)
#endif /* defined(DEBUG) */

/* The default events. */
#if defined(DEBUG)
#define	TR_debug_events | (1 << TR_debug) | (1 << TR_debugerr)
#else
#define	TR_debug_events
#endif /* defined(DEBUG) */
#define	TR_def_events ((1 << TR_cust) | (1 << TR_err) | (1 << TR_fatal) | \
	(1 << TR_misc) | (1 << TR_proc) | (1 << TR_date) TR_debug_events)
/* The events for "all". */
#define	TR_all_events (((1 << TR_all) - 1) TR_debug_events)

#define	STR_OPTIONS_BUF_SIZE 256
#if defined(TRACE_NAMES)
static char *TR_names[] = {
	"cust",
	"err",
	"fatal",
	"ipc",
	"misc",
	"proc",
	"all",

	"rft",
	"alloc",
	"files",
	"oprmsg",
	"queue",
	"dbfile",

#if defined(DEBUG)	/* Not available in release */
	"debug",
	"debugerr",
	"sig",
#else
	" ",		/* KEEP the same number as in #if for translation */
	" ",
	" ",
#endif /* defined(DEBUG) */

	"date",
	"module",
	"type",
	"none",
	""
};
#endif /* defined(TRACE_NAMES) */

/* Generate trace message arguments. */
#define	Trace _Trace
#define	TR_ALLOC TR_alloc, _SrcFile, __LINE__
#define	TR_CUST TR_cust, _SrcFile, __LINE__
#define	TR_DBFILE TR_dbfile, _SrcFile, __LINE__
#define	TR_DEBUG TR_debug, _SrcFile, __LINE__
#define	TR_DEBUGERR TR_debugerr, _SrcFile, __LINE__
#define	TR_ERR TR_err, _SrcFile, __LINE__
#define	TR_FATAL TR_fatal, _SrcFile, __LINE__
#define	TR_FILES TR_files, _SrcFile, __LINE__
#define	TR_IPC TR_ipc, _SrcFile, __LINE__
#define	TR_MISC TR_misc, _SrcFile, __LINE__
#define	TR_OPRMSG TR_oprmsg, _SrcFile, __LINE__
#define	TR_PROC TR_proc, _SrcFile, __LINE__
#define	TR_QUEUE TR_queue, _SrcFile, __LINE__
#define	TR_RFT TR_rft, _SrcFile, __LINE__
#define	TR_SIG TR_sig, _SrcFile, __LINE__

/* Protect tracing of NULL strings */
#define	TrNullString(x) ((x) == NULL ? "NullStr" : (x))

/* Trace daemon identifiers. */
enum TraceId { TI_none = 0,	/* Used for stderr/stdout */
	TI_aml,
	TI_archiver,
	TI_catserver,
	TI_dbupd,
	TI_fsalogd,
	TI_fsd,
	TI_rft,
	TI_recycler,
	TI_nrecycler,
	TI_sharefs,
	TI_stager,
	TI_rmtserver,
	TI_rmtclient,
	TI_mgmtapi,
	TI_shrink,
	TI_MAX
};

#define	TR_MPLOCK	0x10000000	/* Use multi-process locking */
#define	TR_MPMASTER	0x20000000	/* Use multi-process master process */

#if defined(TRACE_CONTROL)
static char *traceIdNames[] = {
	"0",
	SAM_AMLD,
	SAM_ARCHIVER,
	SAM_CATSERVER,
	SAM_DBUPD,
	SAM_FSALOGD,
	SAM_FSD,
	SAM_RFT,
	SAM_RECYCLER,
	SAM_NRECYCLER,
	SAM_SHAREFSD,
	SAM_STAGER,
	SAM_RMTSERVER,
	SAM_RMTCLIENT,
	SAM_MGMTAPI,
	SAM_SHRINK,
	""
};

/* Trace control file definitions. */

#define	TRACECTL_MAGIC 024220103
#define	TRACECTL_VERSION 050412			/* version (YMMDD) */
#define	TRACECTL_BIN SAM_VARIABLE_PATH "/tracectl.bin"

struct TraceCtlBin {
	MappedFile_t	Tr;
	int		TrVersion;
	struct TraceCtlEntry {
		upath_t		TrFname;	/* Trace file name */
		uint32_t	TrFlags;	/* Flags */
		pid_t		TrFsdPid;	/* sam-fsd's pid, */
						/* 0 if rotating the trace */
		int		TrChange;	/* Change count */
		int		TrAge;		/* File age between rotates */
		fsize_t		TrSize;		/* Size to allow before file */
		time_t		TrRotTime;	/* Rotation time */
		fsize_t		TrRotSize;	/* Rotation size */
		fsize_t		TrCurSize;	/* Current size */
	} entry[TI_MAX];
};

#define	TRACE_ROTATE "trace_rotate"	/* Name of trace rotation script */

#endif /* defined(TRACE_CONTROL) */

/* Functions. */
#if defined(DEBUG)
void AssertMessage(char *SrcFile, int SrcLine, char *msg);
void _Assert(char *SrcFile, int SrcLine, int wait);
#endif /* defined(DEBUG) */
void TraceClose(int TrcLen);
void TraceInit(char *programName, int idmp);
FILE *TraceOpen(void);
void TraceReconfig(void);
void TraceSignal(int SigNum);
/* PRINTFLIKE4 */
void _Trace(const TR_type type, const char *SrcFile, const int SrcLine,
		const char *fmt, ...);
#if defined(TRACE_CONTROL)
char *TraceControl(char *identArg, char *value, struct TraceCtlBin *tb);
char *TraceGetOptions(struct TraceCtlEntry *tc, char *buf, size_t buf_size);
#endif /* defined(TRACE_CONTROL) */

/* Public data. */
extern char *TraceName;
extern int TracePid;
extern uint32_t *TraceFlags;	/* One bit per trace option */

#endif /* SAM_TRACE_H */
