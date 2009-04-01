/*
 *	macros.h - SAM-QFS macros.
 *
 *	Description:
 *		define SAM-QFS macros.
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

#ifndef	_SAM_MACROS_H
#define	_SAM_MACROS_H

#ifdef sun
#pragma ident "$Revision: 1.65 $"
#endif

#include "sam/osversion.h"

#ifdef sun
#include "macros_solaris.h"
#endif /* sun */
#ifdef linux
#include "macros_linux.h"
#endif /* linux */


/*
 * ----- Common macros
 */

#define	SAM_TIMESPEC32_TO_TIMESPEC(ts, ts32)	{	\
	(ts)->tv_sec = (time_t)(ts32)->tv_sec;		\
	(ts)->tv_nsec = (ts32)->tv_nsec;			\
}

#define	SAM_TIMESPEC_TO_TIMESPEC32(ts32, ts)	{	\
	(ts32)->tv_sec = (time32_t)(ts)->tv_sec;	\
	(ts32)->tv_nsec = (ts)->tv_nsec;			\
}

/*
 *  ----- Given logical disk offset in the .inodes file, return ino.
 */

#define	SAM_DTOI(x)	(uint32_t) \
		((x >> SAM_ISHIFT) + 1)

/*
 * ----- Given inode number, return logical disk offset in the .inodes file.
 */

#define	SAM_ITOD(x)		((offset_t)(x - 1) << SAM_ISHIFT)

/*
 * ----- Given inode number and buffer pointer, return the logical offset
 * ----- (pointer) to the given inode in memory.
 */

#define	SAM_ITOO(x, size, bp)	(struct sam_disk_inode *) \
		(void *)((SAM_ITOD(x) & (size-1)) + bp->b_un.b_addr)

/*
 * ----- File buffer size for offset
 */

#define	SAM_FBSIZE(offset)	\
	(PAGESIZE - (offset & (PAGESIZE - 1)))

/*
 * ----- Test if special vnode type.
 */

#define	SAM_ISVDEV(t) \
	(((t) == VFIFO) || ((t) == VCHR) || ((t) == VBLK))

/*
 * ----- Given segment vnode, decrement activity count and clear busy flag.
 */

#define	SAM_SEGRELE(vp) { \
	mutex_enter(&(vp)->v_lock); \
	(vp)->v_count--; \
	mutex_exit(&(vp)->v_lock); \
}

/*
 * ----- Given segment size in megabytes, return byte size.
 */

#define	SAM_SEGSIZE(t) \
	(offset_t)(((offset_t)t) << SAM_SEGMENT_SHIFT)

/*
 * ----- Allocate and zero-out "number" structures each of type "structure"
 *		in kernel memory.
 */

#define	ALLOCSTRUCT(structure, number)   \
	((structure *) kmem_zalloc(\
	(uint_t)(sizeof (structure) * (number)), KM_NOSLEEP))


/*
 * ----- Set a bit in an integer given the int x and bit position p (rightmost
 *      bit is 0).
 */

#define		sam_setbit(x, p) \
		(unsigned)((1 << p) | x)


/*
 * ----- Set bits in an integer given the int x, the bit position p (rightmost
 *      bit is 0), and number of bits n.
 */

#define		sam_setbits(x, p, n) \
		(unsigned)((~((~0 >> n) << n) << (p+1-n)) | x)


/*
 * ----- Clear a bit in an integer given the int x and bit position p (rightmost
 *      bit is 0).
 */

#define		sam_clrbit(x, p) \
		(unsigned)(~(~((~0 >> 1) << 1) << p) & x)


/*
 * ----- Clear bits in an integer given the int x, the bit position p (rightmost
 *      bit is 0), and number of bits n.
 */

#define		sam_clrbits(x, p, n) \
		(unsigned)(~(~((~0 >> n) << n) << (p+1-n)) & x)


/*
 * ----- Get a bit from an integer given the int x, the bit position p
 *	(rightmost bit is 0).
 */

#define		sam_getbit(x, p) \
	(unsigned)((x >> p) & ~(~0 << 1))


/*
 * ----- General test if samfs is standalone.
 * Standalone meaning just QFS.
 */

#define		SAM_IS_STANDALONE(mp) \
	(!((mp)->mt.fi_config & MT_SAM_ENABLED))


/*
 * ----- General test for samfs is a shared reader.
 */

#define	SAM_IS_SHARED_READER(mp) \
	((mp)->mt.fi_config & MT_SHARED_READER)


/*
 * ----- General test for samfs is a shared writer.
 */

#define	SAM_IS_SHARED_WRITER(mp) \
	((mp)->mt.fi_config & MT_SHARED_WRITER)


/*
 * ----- General test for samfs is a shared client or shared reader.
 */

#define	SAM_IS_CLIENT_OR_READER(mp) \
	(SAM_IS_SHARED_CLIENT(mp) || ((mp)->mt.fi_config & MT_SHARED_READER))


/*
 * ----- General test for samfs is a shared metadata server or shared writer.
 */

#define	SAM_SYNC_META(mp) \
	((mp)->mt.fi_config & (MT_SYNC_META))


/*
 * ----- General test for samfs is a shared client (not metadata server).
 */

#define	SAM_IS_SHARED_CLIENT(mp) \
	((mp)->mt.fi_status & FS_CLIENT)


/*
 * ----- General test for samfs is a metadata server.
 */

#define	SAM_IS_SHARED_SERVER(mp) \
	((mp)->mt.fi_status & FS_SERVER)


/*
 * ----- General test for samfs is a potential metadata server.
 */

#define	SAM_IS_SHARED_SERVER_ALT(mp) \
	(((mp)->mt.fi_config1 & MC_SHARED_FS) && \
	!((mp)->mt.fi_status & FS_NODEVS))


/*
 * ----- General test for samfs is a shared file system with SAM on server.
 */

#define	SAM_IS_SHARED_SAM_FS(mp) \
	((mp)->mt.fi_status & FS_SAM)


/*
 * ----- General test for samfs is a shared server or shared client.
 */

#define	SAM_IS_SHARED_FS(mp) \
	((mp)->mt.fi_config1 & MC_SHARED_FS)


/*
 * ----- General test for samfs is an object-based file system
 */

#define	SAM_IS_OBJECT_FS(mp) \
	((mp)->mt.fi_config1 & MC_OBJECT_FS)

/*
 * ----- General test for inode is an object-based file
 */

#define	SAM_IS_OBJECT_FILE(ip) \
	(ip->di.rm.ui.flags & RM_OBJECT_FILE)


/*
 * ----- Return SAM ENOSPC error.
 */

#define	SAM_ENOSPC(ip) (int)sam_enospc((ip))


/*
 * ----- Check if SAM-QFS ENOSPC error.
 */

#define	SAM_META_ENOSPC_BIAS	0x10000

#define	IS_SAM_ENOSPC(error) \
	((error == ENOSPC) || (error == (ENOSPC + SAM_META_ENOSPC_BIAS)))


/*
 * ----- Check if SAM-QFS Metadata ENOSPC error.
 */

#define	IS_SAM_META_ENOSPC(error) \
	(error == (ENOSPC + SAM_META_ENOSPC_BIAS))


/*
 * ----- Return TRUE if sequence number a is later than b.
 */

#define	SAM_SEQUENCE_LATER(a, b) \
	(((a)-(b)) < ((b)-(a)))

/*
 * Decrement the FS syscall reference count.  These are incremented by
 * find_mount_point(int fseq) and sam_find_filesystem(uname_t fs_name).
 *
 * If the count drops to zero, then clear the deferred hold field.  If
 * it wasn't set, it was converted to a real hold; release it.
 */

#define	SAM_SYSCALL_DEC(mp, locked) {				\
	int mlocked = locked;					\
	boolean_t rele = FALSE;					\
	ASSERT((mp)->ms.m_syscall_cnt > 0);			\
	if (!(mlocked)) {					\
		mutex_enter(&(mp)->ms.m_waitwr_mutex);		\
	} else {						\
		ASSERT(MUTEX_HELD(&((mp)->ms.m_waitwr_mutex)));	\
	}							\
	if (--(mp)->ms.m_syscall_cnt == 0) {			\
		rele = !(mp)->ms.m_sysc_dfhold;			\
		(mp)->ms.m_sysc_dfhold = FALSE;			\
	}							\
	mutex_exit(&(mp)->ms.m_waitwr_mutex);			\
	if (rele) {						\
		SAM_VFS_RELE(mp);				\
	}							\
}

/*
 * Signal block thread to action.
 */

#define	SAM_KICK_BLOCK(mp) {				\
	mutex_enter(&((mp)->mi.m_block.put_mutex));	\
	if ((mp)->mi.m_block.put_wait) {		\
		cv_signal(&((mp)->mi.m_block.put_cv));	\
	}						\
	mutex_exit(&((mp)->mi.m_block.put_mutex));	\
}

/*
 * Decrement the leaseused for mmap files.
 */

#define	SAM_DECREMENT_LEASEUSED(ip, t) {				\
	mutex_enter(&ip->ilease_mutex);					\
	ip->cl_leaseused[t]--;						\
	if ((ip->cl_leaseused[t] == 0) &&				\
	    (ip->cl_leasetime[t] <= lbolt)) {				\
		sam_sched_expire_client_leases(ip->mp, 0, FALSE);	\
	}								\
	mutex_exit(&ip->ilease_mutex);					\
}

#endif /* _SAM_MACROS_H */
