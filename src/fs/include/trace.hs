/*
 * ----- sam/fs/trace.h - SAM-FS file system trace definitions.
 *
 *	Defines the structure of the I/O entry from the map function.
 *	Used in read/write I/O.
 *
 *
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

/*
 * $Revision: 1.34 $
 */
#ifndef	_SAM_FS_TRACE_H
#define	_SAM_FS_TRACE_H

#ifdef	sun
#include <sys/types.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <sys/vfs.h>
#endif	/* sun */

#ifdef	linux
#ifdef	__KERNEL__
#include "linux/types.h"
#include "linux/time.h"
#include "linux/vfs.h"
#else	/* __KERNEL__ */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/vfs.h>
#endif	/* __KERNEL__ */
#endif	/* linux */

/*
 * ----- SAMFS Includes
 */

#include	<sam/types.h>

extern int sam_tracing;
extern int sam_trace_size;

#define	SAM_TRACE_TINY_ENTRIES	1000	/* Memory-constrained entries/cpu */
#define	SAM_TRACE_MIN_ENTRIES	10000	/* Min user-settable entries/cpu */
#define	SAM_TRACE_MAX_ENTRIES	200000	/* Max user-settable entries/cpu */
#define	SAM_TRACE_DEF_ENTRIES	20000	/* Entries/CPU if not set by user */

#define	SAM_TRACE_SOFT_LIMIT	2	/* Use <= 2% of memory by default */
#define	SAM_TRACE_HARD_LIMIT	20	/* Never allow use of > 20% of memory */

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/* ----	trace_ent - cpu trace entry. */

typedef	 struct	sam_trace_ent {
	hrtime_t		t_time;		/* Start time for this entry */
	sam_tr_t		t_p1;		/* Parameter one */
	sam_tr_t		t_p2;		/* Parameter two */
	sam_tr_t		t_p3;		/* Parameter three */
#ifdef sun
	kthread_id_t	t_thread;	/* Current thread */
#elif defined(linux)
	struct task_struct*	t_thread;	/* Current thread */
#endif
	void			*t_addr;	/* Vfsp or vnode pointer */
	ushort_t		t_event;	/* SAM event trace number */
	ushort_t		t_mount;	/* Current filesystem equipment */
	uint32_t		t_pad;		/* unused */
} sam_trace_ent_t;


/* ----	sam_trace_tbl - trace table (one per cpu). */

typedef struct	sam_trace_tbl {
	union {
		kmutex_t		stbl_t_lock;
		char			pad[64];
	} stbl;
	union {
		kcondvar_t		ttbl_t_cv;
		char			pad[64];
	} ttbl;
	int				t_in;
	int				t_limit;
#ifdef sun
	timestruc_t		t_basetime;
#elif defined(linux)
	struct timeval	t_basetime;
#endif
	hrtime_t		t_baseticks;
	int				t_cpu;
	int				t_wrap;				/* set when buffer wraps */
	sam_trace_ent_t	t_ent [1];
} sam_trace_tbl_t;

#define t_lock	stbl.stbl_t_lock
#define	t_cv	ttbl.ttbl_t_cv

/* -- sam_trace_info_t used for taking the info out of a running kernel for tracing -- */
typedef struct sam_trace_info {
	int cpus; /* same as samfs_trace_table_cpus */
	int bytes; /* same as samfs_trace_table_bytes */
	int entries; /* same as samfs_trace_table_entries */
	hrtime_t scale; /* same sas samfs_trace_table_scale */
} sam_trace_info_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#if SAM_TRACE

#if 0
/* trace with leading print to help track trace bugs */
#define	TRACE(ev, addr, p1, p2, p3)	printk(KERN_CRIT "trace at %s:%d\n", __FILE__, __LINE__); if (sam_tracing) \
					sam_trace(ev, addr, p1, p2, p3);
#define	TRACES(ev, addr, str)	printk(KERN_CRIT "traceS at %s:%d\n", __FILE__, __LINE__); if (sam_tracing) \
					sam_trace_name(ev, addr, str);
#else
#define	TRACE(ev, addr, p1, p2, p3)	if (sam_tracing) \
					sam_trace(ev, addr, p1, p2, p3);
#define	TRACES(ev, addr, str)	if (sam_tracing) \
					sam_trace_name(ev, addr, str);
#endif
#ifdef sun
#define	SYSCALL_TRACE(ev, addr, p1, p2, p3)	TRACE(ev, addr, p1, p2, p3)
#endif /* sun */
#ifdef linux
#define	SYSCALL_TRACE(ev, addr, p1, p2, p3)	TRACE(ev, addr, p1, p2, 0)
#endif /* linux */

#else

#define	TRACE(ev, addr, p1, p2, p3)
#define	TRACES(ev, addr, str)
#define	SYSCALL_TRACE(ev, addr, p1, p2, p3)

#endif	/* SAM_TRACE */

#if DEBUG

#define	DTRACE(ev, addr, p1, p2, p3) if (sam_tracing) \
					sam_trace(ev, addr, p1, p2, p3);
#define	DTRACES(ev, addr, str) if (sam_tracing) \
					sam_trace_name(ev, addr, str);

#else

#define	DTRACE(ev, addr, p1, p2, p3)
#define	DTRACES(ev, addr, str)

#endif	/* DEBUG */


/* TRACE numbers. */

enum sam_trace_id {
	T_NULL,
	T_SAM_MAX		/* Maximum trace number */
};

/*
 * ----- trace function prototypes.
 */

void sam_trace(enum sam_trace_id event, void *ptr, sam_tr_t p1, sam_tr_t p2,
	sam_tr_t p3);
void sam_trace_name(enum sam_trace_id event, void *ptr, const char *name);
void sam_trace_init(void);
void sam_trace_fini(void);

int sam_trace_info(sam_trace_info_t *in);
int sam_trace_addr_data(sam_trace_tbl_t **in);
int sam_trace_tbl_data(sam_trace_tbl_t *in);

#if defined(NEEDMSGS)

#define	SAMTRACED "%lld"
#define	SAMTRACEX "%llx"

char *sam_trace_msg[] = {
	"No message",

	"Unidentified trace"
};

#endif /* NEEDMSGS */

/*
 * What you need to know about trace message flag decoding:
 *
 * Messages which contain constructs which look like ^[...] are decoded by
 * samtrace if the -f flag is used.  These constructs may contain any text
 * to be added.  They are not very interesting unless coupled with a $x.
 * This indicates that the most recent parameter (or, if ^#[...] is used,
 * the parameter indexed by #) should be further decoded according to the
 * following table.
 *
 *   Flag     Meaning
 *   e        errno, if non-zero
 *   m        file mode
 *   M        file mode and type
 *   o        file open mode
 *   f        inode flags (in-core)
 *   s        inode status flags (on-disk)
 *   S        file system status (fi_status)
 *   l        lease type
 *   L        lease mask
 *   c        shared file system command
 *   a        action mask
 *   t        setattr flags
 *   T        setattr mask
 *   p        page flags (as in putpage)
 *   P        protection bits
 *   r        rwlock (read or write)
 *   R        seg_rw enum
 *   z        special handling for rename errors (errno + line number)
 *   q        sam_map_t enum (as in qkmap)
 */

#endif /* _SAM_FS_TRACE_H */
