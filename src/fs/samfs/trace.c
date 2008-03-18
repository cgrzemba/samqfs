/*
 *  trace.c - Process the SAM-FS trace functions.
 *
 */

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

#ifdef sun
#pragma ident "$Revision: 1.45 $"
#endif

#include "sam/osversion.h"

/* ----- UNIX Includes */

#ifdef sun
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/cpuvar.h>
#include <sys/thread.h>
#include <sys/sysmacros.h>
#endif
#ifdef linux
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <asm/mman.h>
#include <linux/swap.h>
#include <linux/vmalloc.h>
#endif



/* ----- SAMFS Includes */

#include "sam/types.h"

#include "inode.h"
#include "mount.h"
#ifdef linux
#include "global.h"
#endif
#define	NEEDMSGS
#include "trace.h"

sam_trace_tbl_t	**samfs_trace = NULL;
int samfs_trace_table_cpus;
int samfs_trace_table_bytes;
int samfs_trace_table_entries;
hrtime_t samfs_trace_time_scale;
kmutex_t samfs_trace_global_lock;

#ifdef linux
extern struct proc_dir_entry *proc_fs_samqfs;
struct proc_dir_entry *proc_fs_trace;
int sam_dump_trace(char *buffer, char **start, off_t offset, int length,
	int *eof, void *data);
extern void *QFS_vmalloc(unsigned long size);

#include "clextern.h"
#endif

#if SAM_TRACE
static sam_trace_tbl_t *sam_trace_init_table(int cpu);
static void sam_get_fseq(int event, void *ptr, equ_t *eq);

#ifdef sun
#pragma rarely_called(sam_trace_init_table)
#endif

#ifdef linux
#ifndef _LP64
static inline hrtime_t gethrtime_unscaled(void) {
	struct timeval t;
	hrtime_t tval;

	do_gettimeofday(&t);
	tval = (hrtime_t)t.tv_sec * 1000000000 + (hrtime_t)t.tv_usec * 1000;
	return (tval);
}
#else
static inline hrtime_t gethrtime_unscaled(void) {
	hrtime_t tval;

	tval = (*QFS_jiffies) * (1000000000/HZ);
	return (tval);
}
#endif
#endif

/*
 * sam_trace - Record the trace entry in the circular trace buffer.
 */

void
sam_trace(
	enum sam_trace_id event,	/* the event to be traced. */
	void *ptr,			/* the vnode/vfs pointer. */
	sam_tr_t p1,			/* the first uint_t parameter. */
	sam_tr_t p2,			/* the second uint_t parameter. */
	sam_tr_t p3)			/* the third uint_t parameter. */
{
	sam_trace_ent_t	*tp;
	sam_trace_tbl_t *samfs_trace_local;
	int	in;
	int cpu;

#ifdef sun
	cpu = CPU->cpu_id;
#elif defined(linux)
	cpu = smp_processor_id();
#endif

	samfs_trace_local = samfs_trace[cpu];
	if (samfs_trace_local == NULL) {
		samfs_trace_local = sam_trace_init_table(cpu);
	}

	mutex_enter(&samfs_trace_local->t_lock);
	in = samfs_trace_local->t_in;
	tp = &samfs_trace_local->t_ent[in];
	if (++in >= samfs_trace_local->t_limit) {
		samfs_trace_local->t_wrap = 1;
		in = 0;
		/* resync clocks */
		SAM_HRESTIME(&samfs_trace_local->t_basetime);
		samfs_trace_local->t_baseticks = gethrtime_unscaled();
	}
	if (!samfs_trace_local->t_wrap && in == samfs_trace_local->t_limit/2) {
		cv_signal(&samfs_trace_local->t_cv);
	}
	samfs_trace_local->t_in = in;
	tp->t_time = gethrtime_unscaled();
	tp->t_p1 = p1;
	tp->t_p2 = p2;
	tp->t_p3 = p3;
	tp->t_thread = curthread;
	tp->t_addr = ptr;
	/*LINTED [assignment causes implicit narrowing conversion] */
	tp->t_event = event;
	sam_get_fseq(event, ptr, &tp->t_mount);

	mutex_exit(&samfs_trace_local->t_lock);
}


/*
 * sam_trace_name - Record trace entry with name in the circular trace buffer.
 */

void
sam_trace_name(
	enum sam_trace_id event,	/* the event to be traced. */
	void *ptr,			/* the vfsp or vnode pointer. */
	const char *name)		/* the string. */
{
	sam_trace_ent_t	*tp;
	sam_trace_tbl_t *samfs_trace_local;
	int in;
	int cpu;

#ifdef sun
	cpu = CPU->cpu_id;
#elif defined(linux)
	cpu = smp_processor_id();
#endif

	samfs_trace_local = samfs_trace[cpu];
	if (samfs_trace_local == NULL) {
		samfs_trace_local = sam_trace_init_table(cpu);
	}

	mutex_enter(&samfs_trace_local->t_lock);
	in = samfs_trace_local->t_in;
	tp = &samfs_trace_local->t_ent[in];
	if (++in >= samfs_trace_local->t_limit) {
		in = 0;
		/* resync clocks */
		SAM_HRESTIME(&samfs_trace_local->t_basetime);
		samfs_trace_local->t_baseticks = gethrtime_unscaled();
	}
	if (!samfs_trace_local->t_wrap && in == samfs_trace_local->t_limit/2) {
		cv_signal(&samfs_trace_local->t_cv);
	}
	samfs_trace_local->t_in = in;
	tp->t_time = gethrtime_unscaled();


	/*
	 * If string crosses a kernel page boundary, use byte-by-byte copy
	 * to avoid accessing unmapped memory; otherwise, copy word-by-word.
	 */

#ifdef sun
	if ((((uint64_t)name & (PAGESIZE-1)) + 23) >= PAGESIZE ||
	    ((uint64_t)name & 0x3) != 0) {
#else
	if ((((uint64_t)name & (PAGE_SIZE-1)) + 23) >= PAGE_SIZE ||
	    ((uint64_t)name & 0x3) != 0) {
#endif
		strncpy((char *)&tp->t_p1, name, 3 * sizeof (sam_tr_t));
	} else {
		if ((((uint64_t)name) & 0x7) == 0) {
		/*LINTED [pointer cast may result in improper alignment]*/
			uint64_t *p = (uint64_t *)name;

			tp->t_p1 = p[0];
			tp->t_p2 = p[1];
			tp->t_p3 = p[2];
		} else {
			int i;
		/*LINTED [pointer cast may result in improper alignment]*/
			uint32_t *p = (uint32_t *)name;
			uint32_t *d = (uint32_t *)&tp->t_p1;

			for (i = 0; i < 6; i++) {
				d[i] = p[i];
			}
		}
	}

	tp->t_thread = curthread;
	tp->t_addr = (char *)ptr;
	/*LINTED [assignment causes implicit narrowing conversion] */
	tp->t_event = event;
	sam_get_fseq(event, ptr, &tp->t_mount);

	mutex_exit(&samfs_trace_local->t_lock);
}


/*
 * sam_get_fseq - Given the event and vfs/vnode pointer, return the fseq.
 */

static void
sam_get_fseq(
	int event,		/* the event to be traced. */
	void *ptr,		/* the vfsp or vnode pointer. */
	equ_t *eq)		/* pointer to returned equipment number */
{
	equ_t fseq = 0;

	if (ptr != NULL) {
		if (event > T_SAM_MAX_VFS) {
#ifdef sun
			fseq = SAM_VTOI((vnode_t *)ptr)->mp->mt.fi_eq;
#elif defined(linux)
			fseq = SAM_LITOSI((struct inode *)ptr)->mp->mt.fi_eq;
#endif
		} else if (event > T_SAM_MAX_MP) {
#ifdef sun
			vfs_t *vfsp;
			sam_mount_t *mp;

			vfsp = (vfs_t *)ptr;	/* vfs pointer. */
			mp = (sam_mount_t *)(void *)vfsp->vfs_data;
#elif defined(linux)
			struct super_block *vfsp;
			sam_mount_t *mp;

			vfsp = (struct super_block *)ptr;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			mp = (sam_mount_t *)(void *)vfsp->s_fs_info;
#else
			mp = (sam_mount_t *)(void *)vfsp->u.generic_sbp;
#endif
#endif
			fseq = mp->mt.fi_eq;
		} else {
			sam_mount_t *mp;

			mp = (sam_mount_t *)ptr;	/* mount pointer. */
			fseq = mp->mt.fi_eq;
		}
	}
	*eq = fseq;
}


/*
 * sam_trace_init - initialize the trace table.
 */

#ifdef linux
#define	btopr(x)	(((x) >> PAGE_SHIFT)+1)
#define	btop(x)		((x) >> PAGE_SHIFT)
#define	ptob(x)		((x) << PAGE_SHIFT)
#endif

extern int QFS_smp_num_cpus;

void
sam_trace_init(void)
{
	int entries, bytes;
	boolean_t user_specified_size;
#ifdef linux
	int i;
	sam_trace_tbl_t *tbl;
#endif

	/*
	 * We allocate a trace table for each CPU.  For the very fastest access,
	 * these are indexed by CPU id.  This wastes a little space, but it's
	 * not too much.
	 *
	 * We don't, however, preallocate the table for each CPU id.  The space
	 * of CPU ID numbers is sparse; e.g. on the 6800, the maximum CPU id is
	 * 553, though only 24 CPUs are supported.
	 *
	 * Not preallocating the table costs one test (for pointer = NULL)
	 * in the hot trace path.
	 */

	user_specified_size = (sam_trace_size > 0);

	entries = (user_specified_size) ?
	    sam_trace_size : SAM_TRACE_DEF_ENTRIES;
	if (entries < SAM_TRACE_MIN_ENTRIES) {
		entries = SAM_TRACE_MIN_ENTRIES;
	} else if (entries > SAM_TRACE_MAX_ENTRIES) {
		entries = SAM_TRACE_MAX_ENTRIES;
	}

	/*
	 * Limit the amount of memory consumed by trace tables.  Without this, a
	 * system without sufficient memory can hang at or shortly after mount.
	 * Note, use the maximum number of CPUs possible for this chassis.
	 */

	bytes = sizeof (sam_trace_tbl_t) + entries * sizeof (sam_trace_ent_t);

	{
		int total_pages, usable_pages, usable_entries;
		int allowed_percentage;

#ifdef sun
		total_pages = max_ncpus * btopr(bytes);
#elif defined(linux)
		total_pages = QFS_smp_num_cpus * btopr(bytes);
#endif
		allowed_percentage = (user_specified_size) ?
		    SAM_TRACE_HARD_LIMIT : SAM_TRACE_SOFT_LIMIT;
#ifdef sun
		usable_pages = (availrmem * allowed_percentage) / 100;
#elif defined(linux)
		usable_pages = (nr_free_pages() * allowed_percentage) / 100;
#endif
		if (total_pages > usable_pages) {
#ifdef sun
			usable_entries = ptob(usable_pages / max_ncpus);
#elif defined(linux)
			usable_entries = ptob(usable_pages / QFS_smp_num_cpus);
#endif
			usable_entries -= sizeof (sam_trace_tbl_t);
			usable_entries /= sizeof (sam_trace_ent_t);
			entries = (usable_entries < SAM_TRACE_TINY_ENTRIES) ?
			    SAM_TRACE_TINY_ENTRIES : usable_entries;
			bytes = sizeof (sam_trace_tbl_t) +
			    entries * sizeof (sam_trace_ent_t);
		}
	}

	samfs_trace_table_entries = entries;
	samfs_trace_table_bytes = bytes;
#ifdef sun
	samfs_trace_table_cpus = max_cpuid + 1;
#elif defined(linux)
	samfs_trace_table_cpus = QFS_smp_num_cpus + 1;
#endif

	samfs_trace = (sam_trace_tbl_t **)
	    kmem_zalloc(samfs_trace_table_cpus * sizeof (sam_trace_tbl_t *),
	    KM_SLEEP);

#ifdef linux
	if (samfs_trace == NULL) {
		printk(KERN_CRIT "memory alloc for samfs_trace "
		    "failed (size %lld)\n",
		    (long long)(samfs_trace_table_cpus *
		    sizeof (sam_trace_tbl_t *)));
	}
#endif

	/*
	 * Record the time scaling factor, used by samtrace.
	 */
	samfs_trace_time_scale = 1000000000;
#ifdef sun
	scalehrtime(&samfs_trace_time_scale);
#endif

	sam_mutex_init(&samfs_trace_global_lock, NULL, MUTEX_DEFAULT, NULL);

	if (user_specified_size && (entries != sam_trace_size)) {
		cmn_err(CE_WARN, "SAM-QFS: Trace buffer size set to %d entries",
		    entries);
	}

#ifdef linux
	/* printk(KERN_CRIT "samfs_..._bytes = %d\n", bytes); */
	/* pre-initialize the cpu entries since they are sequential and */
	/* we know how many there are */
	for (i = 0; i < QFS_smp_num_cpus; i++) {
		tbl = sam_trace_init_table(i);
	}
#endif

}


/*
 * sam_trace_init_table - initialize the trace table for given cpu.
 */
static sam_trace_tbl_t *
sam_trace_init_table(int cpu)
{
	sam_trace_tbl_t *tbl;

	mutex_enter(&samfs_trace_global_lock);

	if (samfs_trace[cpu] != NULL) {
		mutex_exit(&samfs_trace_global_lock);
		return (samfs_trace[cpu]);
	}

#ifdef sun
	tbl = kmem_zalloc(samfs_trace_table_bytes, KM_SLEEP);
#elif defined(linux)
	tbl = QFS_vmalloc(samfs_trace_table_bytes);
	if (tbl == NULL) {
		mutex_exit(&samfs_trace_global_lock);
		printk(KERN_CRIT "memory alloc for tbl failed (size = %d)\n",
		    samfs_trace_table_bytes);
		return (NULL);
	}
	memset(tbl, 0, samfs_trace_table_bytes);
#endif

	tbl->t_in = 0;
	tbl->t_limit = samfs_trace_table_entries;

	sam_mutex_init(&tbl->t_lock, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tbl->t_cv, NULL, CV_DEFAULT, NULL);


	SAM_HRESTIME(&tbl->t_basetime);	/* start clocks */
	tbl->t_baseticks = gethrtime_unscaled();

	tbl->t_cpu = cpu;
	samfs_trace[cpu] = tbl;

	mutex_exit(&samfs_trace_global_lock);

	return (tbl);
}

/*
 * sam_trace_fini - remove all trace tables.
 */

void
sam_trace_fini(void)
{
	if (samfs_trace != NULL) {
		int i;

		for (i = 0; i < samfs_trace_table_cpus; i++) {
			if (samfs_trace[i] != NULL) {
				mutex_destroy(&samfs_trace[i]->t_lock);
				cv_destroy(&samfs_trace[i]->t_cv);
#ifdef sun
				kmem_free((void *)samfs_trace[i],
				    samfs_trace_table_bytes);
#elif defined(linux)
				vfree((void *)samfs_trace[i]);
#endif
			}
		}
		mutex_destroy(&samfs_trace_global_lock);
		kmem_free(samfs_trace,
		    samfs_trace_table_cpus * sizeof (sam_trace_tbl_t *));
		samfs_trace = NULL;
	}
}


/*
 * ---- sam_await_trace_tbl_data
 *
 * Return a CPU's trace buffer to the caller.  CPU is specified
 * in the user's table header in memory before the call.  Copy
 * out all the entries in the (locked) table, in order, and return
 * after either the specified timeout, or as soon as the trace
 * table is half full.  Reset the trace pointers so that the
 * tracer never reads the same entry twice.
 */
int
sam_await_trace_tbl_data(sam_trace_tbl_t *ubuf)
{
	sam_trace_tbl_t tmp, *tp;
	sam_trace_ent_t *tep;
	int error = 0;

	if (copyin(ubuf, &tmp, sizeof (tmp))) {
		return (EFAULT);
	}
	if (tmp.t_cpu < 0 || tmp.t_cpu >= samfs_trace_table_cpus) {
		return (EINVAL);
	}
	if ((tp = samfs_trace[tmp.t_cpu]) == NULL) {
		return (EINVAL);
	}

	mutex_enter(&tp->t_lock);
	if (!tp->t_wrap && tmp.t_baseticks >= 0 && tp->t_in < tp->t_limit/2) {
		clock_t when = lbolt;

		/*
		 * If the buffer is less than half filled, wait up to
		 * a second for it to get half full.  (The trace code
		 * signals if the buffer gets more than half-full.)
		 * If the caller set t_baseticks, use that value instead.
		 */
		if (tmp.t_baseticks == 0) {
			when += hz;
		} else {
			when += tmp.t_baseticks;
		}
		(void) cv_timedwait_sig(&tp->t_cv, &tp->t_lock, when);
	}
	/*
	 * Add trace entry for buffer extraction
	 */
	tep = &tp->t_ent[tp->t_in++];
	bzero(tep, sizeof (*tep));
	tep->t_time = gethrtime_unscaled();
	tep->t_p1 = tp->t_wrap ? tp->t_limit : tp->t_in; /* # trace entries */
#ifdef sun
	tep->t_p2 = CPU->cpu_id;
#elif defined(linux)
	tep->t_p2 = smp_processor_id();
#endif
	tep->t_p3 = tp->t_wrap ? EOVERFLOW : 0;
	tep->t_thread = curthread;
	tep->t_event = T_SAM_TRACE_READ;

	tmp = *tp;
	tmp.t_limit = tp->t_wrap ? tp->t_limit : tp->t_in;	/* user count */
	/*
	 * Copy out base structure
	 */
	if (copyout((char *)&tmp, (char *)ubuf, sizeof (tmp))) {
		error = EFAULT;
		goto out;
	}

	/*
	 * If the buffer wrapped, copy out entries from [t_in .. limit).
	 */
	if (tp->t_wrap && copyout(&tep[1], (char *)&ubuf->t_ent[0],
	    (tp->t_limit - tp->t_in) * sizeof (*tep))) {
		error = EFAULT;
		goto out;
	}

	/*
	 * Copy out entries from [0 .. t_in).
	 */
	if (copyout(&tp->t_ent[0],
	    &ubuf->t_ent[tp->t_wrap ? tp->t_limit - tp->t_in : 0],
	    tp->t_in * sizeof (*tep))) {
		error = EFAULT;
		goto out;
	}

	/*
	 * Reset t_in, so that subsequent caller(s) don't see these entries
	 * again.
	 */
	tp->t_in = 0;
	if (tp->t_wrap) {
		error = EOVERFLOW;
		tp->t_wrap = 0;
	}

out:
	if (tp->t_in >= tp->t_limit) {			/* error paths only */
		tp->t_in = 0;
		tp->t_wrap = 1;
	}
	mutex_exit(&tp->t_lock);

	return (error);
}

#ifdef linux
int
sam_trace_info(sam_trace_info_t *in)
{
	sam_trace_info_t out;

	if (copyin(in, &out, sizeof (out))) {
		return (EFAULT);
	}
	out.cpus = samfs_trace_table_cpus;
	out.bytes = samfs_trace_table_bytes;
	out.entries = samfs_trace_table_entries;
	out.scale = samfs_trace_time_scale;
	if (copyout(&out, in, sizeof (out))) {
		return (EFAULT);
	}
	return (0);
}


/*
 * ---- sam_trace_addr_data
 *
 * Return info about the number of CPUs and their individual
 * trace buffers.  Buffers not returned...
 */
int
sam_trace_addr_data(sam_trace_tbl_t **in)
{
	if (copyout(samfs_trace, in,
	    sizeof (sam_trace_tbl_t *)*samfs_trace_table_cpus)) {
		return (EFAULT);
	}
	return (0);
}


/*
 * ---- sam_trace_tbl_data
 *
 * Return a CPU's trace buffer to the caller.  CPU is specified
 * in the user's table header in memory before the call.
 */
int
sam_trace_tbl_data(sam_trace_tbl_t *in)
{
	sam_trace_tbl_t tmp;

	if (copyin(in, &tmp, sizeof (tmp))) {
		return (EFAULT);
	}
	if ((tmp.t_cpu < 0) || (tmp.t_cpu >= samfs_trace_table_cpus)) {
		return (EINVAL);
	}
	if (samfs_trace[tmp.t_cpu] == NULL) {
		return (EINVAL);
	}
	if (copyout(samfs_trace[tmp.t_cpu], in, samfs_trace_table_bytes)) {
		return (EFAULT);
	}
	return (0);
}


static trace_global_tbl_t tgt;

int
sam_trace_global(trace_global_tbl_t *in)
{
	tgt.mp_list = samgt.mp_list;
	tgt.num_fs_configured = samgt.num_fs_configured;
	tgt.num_fs_mounting = samgt.num_fs_mounting;
	tgt.num_fs_mounted = samgt.num_fs_mounted;
	tgt.num_fs_syncing = samgt.num_fs_syncing;
	tgt.inocount = samgt.inocount;
	tgt.inofree = samgt.inofree;
	tgt.fstype = samgt.fstype;
	tgt.dio_stage_file_size = samgt.dio_stage_file_size;
	tgt.buf_wait = samgt.buf_wait;
	tgt.amld_pid = samgt.amld_pid;
	tgt.meta_minor = samgt.meta_minor;
	tgt.samioc_major = samgt.samioc_major;
	tgt.inode_cache = samgt.inode_cache;
	tgt.schedule_count = samgt.schedule_count;
	tgt.schedule_flags = samgt.schedule_flags;

	if (copyout(&tgt, in, sizeof (trace_global_tbl_t))) {
		return (EFAULT);
	}
	return (0);
}
#endif
#endif	/* SAM_TRACE */
