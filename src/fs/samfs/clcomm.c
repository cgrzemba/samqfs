/*
 * ----- clcomm.c - Communication Interface for the shared file system.
 *
 *	This is the interface module that handles the client socket requests.
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

#ifdef sun
#pragma ident "$Revision: 1.120 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */
#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/kmem.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <netinet/in.h>
#include <sys/strsubr.h>
#include <sys/callb.h>
#include <sys/policy.h>
#endif /* sun */

#ifdef linux
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#endif /* linux */

/*
 * ----- SAMFS Includes
 */

#include <sam/types.h>
#include <sam/syscall.h>

#include "bswap.h"
#include "scd.h"
#include "inode.h"
#include "mount.h"
#include "global.h"
#ifdef sun
#include "macros_solaris.h"
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "macros_linux.h"
#include "clextern.h"
#endif /* linux */
#include "debug.h"
#include "trace.h"
#ifdef	sun
#include "kstats.h"
#endif	/* sun */
#ifdef linux
#include "kstats_linux.h"
#endif /* linux */
#include "sam_interrno.h"
#include "sam_list.h"

/*
 * These defines control some of the behavior of the dynamic sharefs
 * thread management.  Each incoming message from a client is assigned
 * a thread from a per-FS common pool of worker threads.  The size of
 * the pool may occasionally be adjusted according to demand.
 * ~_MAX is the maximum number of active sharefs threads.
 * ~_INCR is the number of new threads to spawn when the workload
 * indicates more threads are needed.
 * The minimum number is set via the 'min_pool' mount parameter;
 * the 'nstreams' mount parameter is now obsolete.
 */
#define	SAM_SHAREFS_THREADS_MAX	4096
#define	SAM_SHAREFS_THREADS_INCR	4

#ifdef sun
static int sam_get_sock_msg(vnode_t *vp, char *msg, enum uio_seg segflg,
		int size, cred_t *credp);
#endif /* sun */

#ifdef linux
#define	freemsg(a)	kfree_skb(a)
static int sam_get_sock_msg(struct socket *sock, char *msg,
		enum uio_seg segflg, int size, cred_t *credp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 15))
struct sk_buff *rfs_alloc_skb(unsigned int size, int priority);
#else
struct sk_buff *rfs_alloc_skb(unsigned int size, gfp_t priority);
#endif
#endif /* linux */


/*
 * ----- sam_client_rdsock - Process the client read socket command.
 *	Called when the sam-sharefsd client thread issues a syscall,
 *  SC_client_rdsock.
 *  NOTE. This thread reads from the socket in the file system.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_rdsock(
	void *arg,		/* Pointer to arguments. */
	int size,		/* Size of the arguments */
	cred_t *credp)		/* Pointer to the credentials */
{
	sam_mount_t *mp;
	struct sam_syscall_rdsock rdsock;
#ifdef sun
	vnode_t *vp;
#endif /* sun */
#ifdef linux
	struct socket *sock;
#endif /* linux */
	char *rdmsg;
	int error;

	if (size != sizeof (rdsock) ||
	    copyin(arg, (caddr_t)&rdsock, sizeof (rdsock))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(rdsock.fs_name)) == NULL) {
		return (ENOENT);
	}

	if ((error = secpolicy_fs_config(credp, mp->mi.m_vfsp)) != 0) {
		/*
		 * Decrement syscall count which was incremented
		 * in sam_find_filesystem() and return.
		 */
		SAM_SYSCALL_DEC(mp, 0);
		return (error);
	}
#ifdef sun
	rdmsg = (char *)(long)rdsock.msg.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		rdmsg = (char *)rdsock.msg.p64;
	}
#endif /* sun */
#ifdef linux
	/*
	 * Set to NULL so a kernel buffer
	 * is allocated later.
	 */
	rdmsg = NULL;
#endif /* linux */

	SAM_SHARE_DAEMON_HOLD_FS(mp);

	if (rdsock.flags & SOCK_BYTE_SWAP) {
		/*
		 * The server has reverse byte ordering
		 */
		mp->mt.fi_status |= FS_SRVR_BYTEREV;
	} else {
		/*
		 * The server does not have reverse byte ordering
		 */
		mp->mt.fi_status &= ~FS_SRVR_BYTEREV;
	}

	/*
	 * Decrement syscall count which was incremented
	 * in sam_find_filesystem().
	 */
	SAM_SYSCALL_DEC(mp, 0);

	if (!SAM_IS_SHARED_FS(mp)) {
		error = ENOTTY;
		goto out;
	}

	strncpy(mp->ms.m_cl_hname, rdsock.hname, sizeof (mp->ms.m_cl_hname));

	/*
	 * Client daemon restarting, verify there is not a previous connection.
	 * Clear any previous sockfs socket.
	 * Get and hold the current socket opened by this sam-sharefsd.
	 */

	mutex_enter(&mp->ms.m_cl_wrmutex);
	if (mp->ms.m_cl_thread) {
		mutex_exit(&mp->ms.m_cl_wrmutex);
		error = EIO;
		goto out;
	}
	mp->ms.m_cl_thread = curthread;

	mp->ms.m_cl_sock_flags = rdsock.flags;
	sam_clear_sock_fp(&mp->ms.m_cl_sh);

	if ((error = sam_set_sock_fp(rdsock.sockfd, &mp->ms.m_cl_sh)) == 0) {
#ifdef sun
		vp = mp->ms.m_cl_sh.sh_fp->f_vnode;
		mutex_exit(&mp->ms.m_cl_wrmutex);

		/*
		 * Get responses or callouts from the shared server.
		 */
		error = sam_read_sock(TRUE, -1, mp, vp, rdmsg, credp);
		mutex_enter(&mp->ms.m_cl_wrmutex);
#endif /* sun */
#ifdef linux
		sock = mp->ms.m_cl_sh.sh_fp;
		mutex_exit(&mp->ms.m_cl_wrmutex);
		if (sock == NULL) {
			error = EIO;
			goto out;
		}

		/*
		 * Get responses or callouts from the shared server.
		 */
		error = sam_read_sock(TRUE, -1, mp, sock, rdmsg, credp);
		mutex_enter(&mp->ms.m_cl_wrmutex);
#endif /* linux */
	}

	sam_clear_sock_fp(&mp->ms.m_cl_sh);
	mp->ms.m_cl_thread = NULL;
	mutex_exit(&mp->ms.m_cl_wrmutex);

out:
	SAM_SHARE_DAEMON_RELE_FS(mp);
	return (error);
}


/*
 * Start up sharefs service threads (if not already started)
 */
static int
sam_start_sharefs_threads(sam_mount_t *mp)
{
	int error = 0;

	/*
	 * Start up fi_min_pool threads to process received packets.
	 * Note:  there is a race condition when the client and server
	 * both try to create the threads at the same time.  Protect
	 * this with the m_cl_init mutex.
	 */
	mutex_enter(&mp->ms.m_cl_init);
	mutex_enter(&mp->ms.m_sharefs.mutex);
	if (mp->ms.m_sharefs.no_threads == 0) {
		int i;
#ifdef sun
		for (i = 0; i < mp->mt.fi_min_pool; i++) {
			error = sam_init_thread(&mp->ms.m_sharefs,
			    sam_sharefs_thread, mp);
			if (error) {
				break;
			}
		}
#endif /* sun */
#ifdef linux
		for (i = 0; i < mp->mt.fi_min_pool; i++) {
			pid_t pid;

			pid = sam_init_thread(
			    &mp->ms.m_sharefs, sam_sharefs_thread, (void *)mp);
			if (pid <= 0) {
				error = pid;
				break;
			}
		}
#endif /* linux */
	}
	mutex_exit(&mp->ms.m_sharefs.mutex);
	mutex_exit(&mp->ms.m_cl_init);

	return (error);
}


/*
 * If client, wait for all sharefs service threads to idle, then reap them.
 * If server, just wait for no busy sharefs threads on this server socket.
 */
static void
sam_kill_sharefs_threads(
	sam_mount_t *mp,
	boolean_t client,
#ifdef METADATA_SERVER
	struct client_entry *clp)
#else
	void *clp)
#endif
{
	mutex_enter(&mp->ms.m_sharefs.mutex);
	while (mp->ms.m_sharefs.busy) {
#ifdef METADATA_SERVER
		if (!client && clp && (clp->cl_busy_threads == 0)) {
			break;
		}
#endif
		cv_wait(&mp->ms.m_sharefs.get_cv, &mp->ms.m_sharefs.mutex);
	}
	mutex_exit(&mp->ms.m_sharefs.mutex);

	if (!client) {
		return;
	}
	mutex_enter(&mp->ms.m_cl_init);
	mutex_enter(&mp->ms.m_sharefs.mutex);
	if (mp->ms.m_sharefs.no_threads) {
		sam_kill_threads(&mp->ms.m_sharefs);
	}
	mutex_exit(&mp->ms.m_sharefs.mutex);
	mutex_exit(&mp->ms.m_cl_init);
}


/*
 * ----- sam_read_sock - Process the sam-sharefsd read socket command.
 *  NOTE. This thread reads from the socket in the file system.
 */
int				/* ERRNO if error, 0 if successful. */
sam_read_sock(
	boolean_t client,
	int client_ord,		/* Client ordinal if server, -1 if client */
	sam_mount_t *mp,
#ifdef sun
	vnode_t *vp,
#endif /* sun */
#ifdef linux
	struct socket *sock,
#endif /* linux */
	char *rdmsg,		/* NULL or ptr to msg buffer in user space */
	cred_t *credp)
{
	sam_san_header_t hdr;
	sam_san_message_t *msg;
	m_sharefs_item_t *item;
	m_sharefs_item_t *head_item;
#ifdef sun
	mblk_t *mbp;
	clock_t thread_spawn_time;
#endif /* sun */
#ifdef linux
	struct sk_buff *mbp = NULL;
	char *mptr;
	uint64_t thread_spawn_time;
#endif /* linux */
	int body_size;
	int buf_size;
	int segflg = UIO_USERSPACE;
	int error = 0;
	int bswap;
	int i;

	/*
	 * If rdmsg is NULL, then the calling daemon did not allocate
	 * a user buffer.  Allocate a kernel buffer.
	 */
	if (rdmsg == NULL) {
		rdmsg = kmem_alloc(2*sizeof (sam_san_max_message_t),
		    KM_NOSLEEP);
		if (rdmsg != NULL) {
			segflg = UIO_SYSSPACE;
		} else {
			error = ENOMEM;
		}
	}

	if (!error) {
		error = sam_start_sharefs_threads(mp);
	}

	/* initialize dynamic thread time limit */
	thread_spawn_time = ddi_get_lbolt() + hz;

	/*
	 * Read the header first, then read the body.
	 */
	while (!error) {
		bswap = 0;

#ifdef sun
		error = sam_get_sock_msg(vp, rdmsg, segflg,
		    SAM_HDR_LENGTH, credp);
#endif /* sun */
#ifdef linux
		error = sam_get_sock_msg(sock, rdmsg, segflg,
		    SAM_HDR_LENGTH, credp);
#endif /* linux */
		if (error) {
#ifdef linux
			if (error == ERESTARTSYS) {
				error = EINTR;
			}
#endif /* linux */
			break;
		}
		if (segflg == UIO_SYSSPACE) {
			bcopy(rdmsg, (char *)&hdr, SAM_HDR_LENGTH);
		} else {
#ifdef sun
			if (copyin(rdmsg, (char *)&hdr, SAM_HDR_LENGTH)) {
				error = EFAULT;
				break;
			}
#endif /* sun */
#ifdef linux
			error = ENOSYS;
			break;
#endif /* linux */
		}
		/*
		 * Have header.  Byte-swap it to the local format
		 * now if that needs to be done.
		 */
		if (hdr.magic != SAM_SOCKET_MAGIC) {
			int r;

			r = sam_byte_swap_header(&hdr);
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_byte_swap_header "
				    "returned %d; cmd=%d, op=%d, "
				    "ord=%d",
				    mp->mt.fi_name, r, (int)hdr.command,
				    (int)hdr.operation,
				    client_ord);
				error = ENOTTY;
				break;
			}
			bswap = 1;
		}
		/*
		 * Now we can use the fields provided.
		 */
		if (hdr.magic != SAM_SOCKET_MAGIC) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: "
			    "hdr.magic (%llx) != SAM_SOCKET_MAGIC (%llx),"
			    " ord=%d",
			    mp->mt.fi_name,
			    (unsigned long long)hdr.magic,
			    (unsigned long long)SAM_SOCKET_MAGIC,
			    client_ord);
			error = ENOTTY;
			break;
		}
		if (hdr.length > sizeof (sam_san_max_message_t) ||
		    hdr.out_length > sizeof (sam_san_max_message_t)) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: header length (%x) or "
			    "out_length (%x) is too big, ord=%d",
			    mp->mt.fi_name, (int)hdr.length,
			    (int)hdr.out_length,
			    client_ord);
			error = ENOTTY;
			break;
		}

		hdr.error = solaris_to_local_errno(hdr.error);

		body_size = STRUCT_RND64(hdr.length);
#ifdef sun
		error = sam_get_sock_msg(vp, rdmsg, segflg, body_size, credp);
#endif /* sun */
#ifdef linux
		error = sam_get_sock_msg(sock, rdmsg, segflg, body_size,
		    credp);
#endif /* linux */
		if (error) {
			break;
		}

		if (client) {
			hdr.originator = SAM_SHARED_SERVER;
		} else {
			hdr.originator = SAM_SHARED_CLIENT;
		}

		buf_size = SAM_HDR_LENGTH +
		    MAX(STRUCT_RND64(hdr.length),
		    STRUCT_RND64(hdr.out_length));
		TRACE((client ? T_SAM_CL_RDCLNT : T_SAM_CL_RDSRVR), mp,
		    hdr.error, (hdr.command<<16)|hdr.operation, hdr.seqno);

#ifdef sun
		while ((mbp = allocb(buf_size, BPRI_LO)) == NULL) {
			if ((error = strwaitbuf((size_t)buf_size, BPRI_LO))) {
				goto out;
			}
		}

		bcopy((char *)&hdr, mbp->b_wptr, SAM_HDR_LENGTH);
		if (segflg == UIO_SYSSPACE) {
			bcopy(rdmsg, (mbp->b_wptr+SAM_HDR_LENGTH), body_size);
		} else {
			if (copyin(rdmsg, (mbp->b_wptr+SAM_HDR_LENGTH),
			    body_size)) {
				error = EFAULT;
				freemsg(mbp);
				break;
			}
		}

		msg = (sam_san_message_t *)(void *)mbp->b_wptr;
		mbp->b_wptr += buf_size;
#endif /* sun */
#ifdef linux
		mbp = rfs_alloc_skb(buf_size, GFP_KERNEL);
		if (mbp == NULL) {
			error = ENOMEM;
			goto out;
		}

		mptr = skb_put(mbp, SAM_HDR_LENGTH);
		bcopy((char *)&hdr, mptr, SAM_HDR_LENGTH);

		mptr = skb_put(mbp, body_size);
		if (segflg == UIO_SYSSPACE) {
			bcopy(rdmsg, mptr, body_size);
		} else {
			error = ENOSYS;
			freemsg(mbp);
			break;
		}

		msg = (sam_san_message_t *)mbp->data;
#endif /* linux */

		/*
		 * XXX It would be better to move this byte-swapping out of the
		 * XXX critical region, where it can be done at leisure.  But
		 * XXX we can't return an error message w/ a reversed header
		 * XXX and an unreversed body (below).  To do that, we'd need
		 * XXX to set a flag in the header indicating that byte-reversal
		 * XXX is needed in the body.  (Or in the client structure,
		 * XXX since individual clients should always be consistent.)
		 */
		if (bswap) {
			int r;

			r = sam_byte_swap_message(mbp);
			if (r == EPROTO) {
				freemsg(mbp);
				msg->hdr.error = EPROTO;
				msg->hdr.client_ord = client_ord;
				msg->hdr.wait_flag = SHARE_nothr;
#ifdef METADATA_SERVER
				if (!client) {
					error = sam_write_to_client(mp, msg);
					continue;
				}
#endif
				if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
					r = sam_byte_swap(
					    sam_san_header_swap_descriptor,
					    (void *)msg,
					    sizeof (sam_san_header_t));
					ASSERT(r == 0);
				}
				error = sam_write_to_server(mp, msg);
				continue;
			} else if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_byte_swap_message "
				    "returned %d;"
				    " cmd=%d, op=%d",
				    mp->mt.fi_name, r, (int)hdr.command,
				    (int)hdr.operation);
				freemsg(mbp);
				error = ENOTTY;
				break;
			}
		}


		/* allocate a new item to put on the list */
		item = kmem_cache_alloc(samgt.item_cache, KM_NOSLEEP);
		if (item == NULL) {
			error = ENOMEM;
			freemsg(mbp);
			break;
		}
		item->mbp = mbp;
		item->timestamp = ddi_get_lbolt();

		/*
		 * For client originator, verify client_ord from the client is
		 * still valid. If not, return EISAM_MOUNT_OUTSYNC, so client
		 * can relinquish leases and re-establish the connection.
		 */
#ifdef METADATA_SERVER
		if (!client) {
			client_entry_t *clp;

			clp = sam_get_client_entry(mp, client_ord, 0);
			ASSERT(clp != NULL);

			ASSERT(msg->hdr.command & 0xff);
			if (msg->hdr.command != SAM_CMD_MOUNT) {
				/*
				 * Verify client host is in the client table at
				 * client_ord.
				 */
				if ((msg->hdr.client_ord != client_ord) ||
				    (clp->hostid != msg->hdr.hostid)) {
					msg->hdr.client_ord = client_ord;
					msg->hdr.error = EISAM_MOUNT_OUTSYNC;
					error = sam_write_to_client(mp, msg);
					freemsg(mbp);
					kmem_cache_free(samgt.item_cache,
					    item);
					continue;
				}
			} else {
				/*
				 * Make sure message can be returned if error.
				 */
				msg->hdr.client_ord = client_ord;
			}
		}

		if (msg->hdr.originator == SAM_SHARED_CLIENT) {
			SAM_COUNT64(shared_server, msg_in);
		} else {
			SAM_COUNT64(shared_client, msg_in);
		}
#else
		SAM_COUNT64(shared_client, msg_in);
#endif	/* METADATA_SERVER */

		/* add item to message list */
		mutex_enter(&mp->ms.m_sharefs.mutex);
		item->count = mp->ms.m_sharefs.queue.next_item++;
		list_enqueue(&mp->ms.m_sharefs.queue.list, item);
		mp->ms.m_sharefs.queue.items++;
		if (mp->ms.m_sharefs.queue.wmark <
		    mp->ms.m_sharefs.queue.items) {
			mp->ms.m_sharefs.queue.wmark =
			    mp->ms.m_sharefs.queue.items;
		}
		/*
		 * Check to see if items are spending too much time in queue.
		 * If they are, spawn more sharefs threads to help deal with the
		 * workload.
		 */
		if (thread_spawn_time <= ddi_get_lbolt()) {
			head_item = (m_sharefs_item_t *)list_head(
			    &mp->ms.m_sharefs.queue.list);
			if (head_item && ((head_item->timestamp + (hz/4)) <=
			    item->timestamp)) {
				if ((mp->ms.m_sharefs.put_wait == 0) &&
				    (mp->ms.m_sharefs.no_threads <
				    SAM_SHAREFS_THREADS_MAX)) {
					for (i = 0;
					    i < SAM_SHAREFS_THREADS_INCR;
					    i++) {
						sam_init_thread(
						    &mp->ms.m_sharefs,
						    sam_sharefs_thread,
						    (void *)mp);
					}
#ifdef DEBUG
					cmn_err(CE_NOTE, "SAM-QFS: new "
					    "sharefs_threads, "
					    "total threads: %d",
					    mp->ms.m_sharefs.no_threads);
#endif
					thread_spawn_time = ddi_get_lbolt() + hz;
				}
			}
		}
		cv_signal(&mp->ms.m_sharefs.put_cv);
		mutex_exit(&mp->ms.m_sharefs.mutex);
	}
out:
	/*
	 * Shutting down, wait for sharefs threads to idle. Then
	 * kill threads. Make sure we are not killing and creating
	 * threads simultaneously by using the m_cl_init mutex.
	 */
	TRACE((client ? T_SAM_CL_RDCLNT_RET : T_SAM_CL_RDSRVR_RET), mp,
	    error, (hdr.command<<16)|hdr.operation, hdr.seqno);

#ifdef METADATA_SERVER
	sam_kill_sharefs_threads(mp, client, client ? NULL :
	    sam_get_client_entry(mp, client_ord, 0));
#else
	sam_kill_sharefs_threads(mp, client, NULL);
#endif

	/* destroy the remaining items in the msg list */
	mutex_enter(&mp->ms.m_sharefs.mutex);
	while ((item = list_dequeue(&mp->ms.m_sharefs.queue.list)) != NULL) {
		freemsg(item->mbp);
		kmem_cache_free(samgt.item_cache, item);
	}
	/* update current_item to reflect the dequeuing of messages */
	mp->ms.m_sharefs.queue.current_item = mp->ms.m_sharefs.queue.next_item;
	mutex_exit(&mp->ms.m_sharefs.mutex);

	if (segflg == UIO_SYSSPACE) {
		kmem_free(rdmsg, 2*sizeof (sam_san_max_message_t));
	}

	return (error);
}


/*
 * ----- sam_get_sock_msg - Get message directly from sockfs.
 *
 */
#ifdef sun
static int			/* ERRNO if error, 0 if successful. */
sam_get_sock_msg(
	vnode_t *vp,		/* Socket vnode pointer */
	char *msg,		/* Pointer to user space message */
	enum uio_seg segflg,	/* msg points to kernel (user) buffer */
	int size,		/* Requested size */
	cred_t *credp)
{
	struct sonode *so;
	struct iovec iov;
	struct uio uio;
	int cnt;
	int error;

	switch (vp->v_type) {
	case VSOCK:
		so = VTOSO(vp);

		switch (so->so_family) {
		case AF_INET:
		case AF_INET6:
			/*
			 * Check that socket is still connected.
			 */
			if (so->so_type != SOCK_STREAM) {
				error = EOPNOTSUPP;
				goto out;
			}

			if ((so->so_state & (SS_ISCONNECTED|SS_CANTRCVMORE)) !=
			    SS_ISCONNECTED) {
				error = ENOTCONN;
				goto out;
			}
			break;
		default:
			error = EAFNOSUPPORT;
			goto out;
		}
		break;

	default:
		error = EINVAL;
		goto out;
	}

	VOP_RWLOCK_OS(vp, 0, NULL);

	iov.iov_len = size;
	iov.iov_base = (caddr_t)msg;

	uio.uio_iov = &iov;
	uio.uio_loffset = 0;
	uio.uio_iovcnt = 1;
	uio.uio_resid = size;
	uio.uio_segflg = segflg;
	uio.uio_fmode = FREAD|FWRITE;
	uio.uio_llimit = size;

	/*
	 * Wait for a packet over the socket. Note, the packet may
	 * have more than 1 message. Also messages can span packets.
	 * Can't always use system buffer because UIO_SYSSPACE is
	 * not supported by all versions + releases.
	 */
	while (size > 0) {
		if ((error = VOP_READ_OS(vp, &uio, 0, credp, NULL))) {
			if (error != EINTR) {
				VOP_RWUNLOCK_OS(vp, 0, NULL);
				goto out;
			}
			error = 0;
		}
		if (uio.uio_resid == size) {
			error = EIO;
			VOP_RWUNLOCK_OS(vp, 0, NULL);
			goto out;
		}
		cnt = size - uio.uio_resid;
		size -= cnt;
	}
	VOP_RWUNLOCK_OS(vp, 0, NULL);

out:
	if (error) {
		return (error);
	}
	return (0);
}
#endif /* sun */

#ifdef linux
static int			/* ERRNO if error, 0 if successful. */
sam_get_sock_msg(
	struct socket *sock,	/* Socket pointer */
	char *rdmsg,		/* Pointer to message */
	enum uio_seg segflg,	/* msg points to kernel (user) buffer */
	int size,		/* Requested size */
	cred_t *credp)
{
	int len;
	int ret = 0;
	int flags = 0;
	struct msghdr mh;
	struct iovec iov;
	mm_segment_t save_state;

	if (sock->state != SS_CONNECTED) {
		return (ENOTCONN);
	}

	iov.iov_base = rdmsg;
	iov.iov_len = size;

	mh.msg_name = NULL;
	mh.msg_namelen = 0;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;

	save_state = get_fs();
	set_fs(KERNEL_DS);

	while (size > 0) {

		len = sock_recvmsg(sock, &mh, size, flags);

		if (len == 0) {
			ret = EPIPE;
			break;
		}
		if (len < 0) {
			ret = -len;
			break;
		}

		size -= len;
	}

	set_fs(save_state);

	return (ret);
}
#endif /* linux */


/*
 * ----- sam_write_to_server - Send client message to server.
 * Write directly to sockfs.
 */

int					/* ERRNO if error, 0 if successful. */
sam_write_to_server(sam_mount_t *mp, sam_san_message_t *msg)
{
#ifdef sun
	vnode_t *vp;
#endif /* sun */
#ifdef linux
	struct socket *sock;
#endif /* linux */
	int error;

	msg->hdr.originator = SAM_SHARED_CLIENT;
	mutex_enter(&mp->ms.m_cl_wrmutex);
#ifdef sun
	vp = mp->ms.m_cl_sh.sh_fp ?
	    mp->ms.m_cl_sh.sh_fp->f_vnode : NULL;
	if (vp != NULL) {
		VN_HOLD(vp);
#endif /* sun */
#ifdef linux
	sock = mp->ms.m_cl_sh.sh_fp;
	if (sock != NULL) {
#endif /* linux */
		SAM_SOCK_READER_LOCK(&mp->ms.m_cl_sh);
		mutex_exit(&mp->ms.m_cl_wrmutex);
		{
			int32_t  client_ord = msg->hdr.client_ord;
			ushort_t operation = msg->hdr.operation;
			ushort_t command = msg->hdr.command;
			uint32_t seqno = msg->hdr.seqno;

			if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
				sam_bswap2((void *)&operation, 1);
				sam_bswap2((void *)&command, 1);
				sam_bswap4((void *)&client_ord, 1);
				sam_bswap4((void *)&seqno, 1);
			}
			TRACE(T_SAM_CL_WRSRVR,
			    mp, client_ord, (command << 16)|operation, seqno);
		}
		SAM_COUNT64(shared_client, msg_out);
#ifdef sun
		error = sam_put_sock_msg(mp, vp, msg,
		    &mp->ms.m_cl_sh.sh_wmutex);
#endif /* sun */
#ifdef linux
		error = sam_put_sock_msg(mp, sock, msg,
		    &mp->ms.m_cl_sh.sh_wmutex);
#endif /* linux */
		SAM_SOCK_READER_UNLOCK(&mp->ms.m_cl_sh);
		if (error) {
			int32_t  client_ord = msg->hdr.client_ord;
			uint32_t seqno = msg->hdr.seqno;

			if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
				sam_bswap4((void *)&client_ord, 1);
				sam_bswap4((void *)&seqno, 1);
			}
			TRACE(T_SAM_CL_WRSRVR_ER1, mp, error, client_ord,
			    seqno);
			if (SOCKET_FATAL(error)) {
				mutex_enter(&mp->ms.m_cl_wrmutex);
				if (mp->ms.m_cl_thread) {
#ifdef	sun
					tsignal(mp->ms.m_cl_thread, SIGPIPE);
#endif
#ifdef	linux
					send_sig(SIGPIPE,
					    mp->ms.m_cl_thread, 0);
#endif
				}
				mutex_exit(&mp->ms.m_cl_wrmutex);
			}
		}
#ifdef	sun
		VN_RELE(vp);
#endif	/* sun */
	} else {
		int32_t  client_ord = msg->hdr.client_ord;
		ushort_t operation = msg->hdr.operation;
		ushort_t command = msg->hdr.command;
		uint32_t seqno = msg->hdr.seqno;

		mutex_exit(&mp->ms.m_cl_wrmutex);
		error = ENOTCONN;
		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			sam_bswap2((void *)&operation, 1);
			sam_bswap2((void *)&command, 1);
			sam_bswap4((void *)&client_ord, 1);
			sam_bswap4((void *)&seqno, 1);
		}
		TRACE(T_SAM_CL_WRSRVR_ER2,
		    mp, client_ord, (command << 16)|operation, seqno);
	}
	return (error);
}


#ifdef sun
/*
 * ----- sam_put_sock_msg - Send message directly to sockfs.
 *
 */

/* ARGSUSED3 */
int					/* ERRNO if error, 0 if successful. */
sam_put_sock_msg(
	sam_mount_t *mp,
	vnode_t *vp,			/* Socket vnode pointer */
	sam_san_message_t *msg,
	kmutex_t *wrmutex)
{
	mblk_t *mbp;
	struct sonode *so;
	int size;
	int error;
	ushort_t hlength;
	int flags;
	clock_t thread_spawn_time;

	switch (vp->v_type) {
	case VSOCK:
		so = VTOSO(vp);

		switch (so->so_family) {
		case AF_INET:
		case AF_INET6:
			if (vp->v_stream == NULL) {
				error = ENOTCONN;
				goto out;
			}

			/*
			 * Check that socket is still connected.
			 */
			if (so->so_state & SS_CANTSENDMORE) {
				error = EPIPE;
				goto out;
			}
			if (so->so_type != SOCK_STREAM) {
				error = EOPNOTSUPP;
				goto out;
			}

			if ((so->so_state & (SS_ISCONNECTED|SS_ISBOUND)) !=
			    (SS_ISCONNECTED|SS_ISBOUND)) {
				error = ENOTCONN;
				goto out;
			}
			break;

		default:
			error = EAFNOSUPPORT;
			goto out;
		}
		break;

	default:
		error = EINVAL;
		goto out;
	}

	/*
	 * Get a copy of the header length in native endianness.
	 */
	hlength = msg->hdr.length;

	if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
		sam_bswap2((void *)&hlength, 1);
	}

	ASSERT(hlength <= sizeof (sam_san_max_message_t));
	size = SAM_HDR_LENGTH + STRUCT_RND64(hlength);

	while ((mbp = allocb(size, BPRI_LO)) == NULL) {
		if ((error = strwaitbuf((size_t)size, BPRI_LO))) {
			goto out;
		}
	}

	bcopy(msg, mbp->b_wptr, SAM_HDR_LENGTH);
	bcopy(&msg->call, (mbp->b_wptr+SAM_HDR_LENGTH),
	    STRUCT_RND64(hlength));
	mbp->b_datap->db_type = M_DATA;
	mbp->b_wptr += size;

	/*
	 * Send data honoring flow control and errors.
	 */
	if (SAM_IS_SHARED_SERVER(mp) && IS_SHAREFS_THREAD_OS) {
		/*
		 * If this is the server sending to a client
		 * make it non-blocking. Prevents a broken client
		 * that is not reading from it's socket from
		 * hanging server threads.
		 */
		flags = FWRITE|FREAD|FNONBLOCK;
	} else {
		flags = FWRITE|FREAD;
	}

	/*
	 * The kstrputmsg socket function must be used because the VOP_WRITE()
	 * function sends a SIGPIPE to the user when an EPIPE error occurs.
	 */
#if (KERNEL_MINOR >= 4) /* >= Solaris 11.4 */
	thread_spawn_time = ddi_get_lbolt() + hz;
	error = kstrputmsg(vp, mbp, NULL, 0, 0, MSG_BAND | MSG_HOLDSIG, flags, thread_spawn_time);
#else
	error = kstrputmsg(vp, mbp, NULL, 0, 0, MSG_BAND | MSG_HOLDSIG, flags);
#endif
out:
	{
		int32_t  hdr_error = msg->hdr.error;
		ushort_t command = msg->hdr.command;
		ushort_t operation = msg->hdr.operation;
		uint32_t seqno = msg->hdr.seqno;

		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			sam_bswap4((void *)&hdr_error, 1);
			sam_bswap4((void *)&seqno, 1);
			sam_bswap2((void *)&command, 1);
			sam_bswap2((void *)&operation, 1);
		}

		if (error) {
			TRACE(T_SAM_CL_WRSOCK_ERR,
			    mp, error, (command<<16)|operation, seqno);
			return (error);
		}
		TRACE(T_SAM_CL_WRSOCK_RET,
		    mp, hdr_error, (command<<16)|operation, seqno);
	}
	return (0);
}


/*
 * ----- sam_set_sock_fp - Get the socket vnode and hold it.
 * NOTE: mp->ms.m_cl_wrmutex/clp->cl_wrmutex set on entry and exit.
 */

int				/* Error */
sam_set_sock_fp(
	int sockfd,		/* Socket file descriptor */
	sam_sock_handle_t *shp)	/* pointer to pointer to file (returned) */
{
	file_t *fp;

	/*
	 * Wait till all users of the current connection are
	 * done.
	 */
	SAM_SOCK_WRITER_LOCK(shp);

	shp->sh_fp = NULL;

	if ((fp = getf(sockfd)) == NULL) {
		SAM_SOCK_WRITER_UNLOCK(shp);
		return (EBADF);
	}
	VN_HOLD(fp->f_vnode);
	mutex_enter(&fp->f_tlock);
	fp->f_count++;
	mutex_exit(&fp->f_tlock);
	releasef(sockfd);

	shp->sh_fp = fp;
	SAM_SOCK_WRITER_UNLOCK(shp);

	return (0);
}


/*
 * ----- sam_clear_sock_fp - Clear socket fp.
 * Inactivate the sockfs vnode. This vnode is held after the
 * sam-sharefsd daemon has exited.
 * NOTE: mp->ms.m_cl_wrmutex/clp->cl_wrmutex set on entry and exit.
 */

void
sam_clear_sock_fp(sam_sock_handle_t *shp)
{
	file_t *fp;

	/*
	 * Wait till all users of the current connection are
	 * done.
	 */
	SAM_SOCK_WRITER_LOCK(shp);

	fp = shp->sh_fp;
	if (fp) {
		/*
		 * closef() does a VN_RELE() and decrements f_count.
		 */
		closef(fp);
		shp->sh_fp = NULL;
	}
	SAM_SOCK_WRITER_UNLOCK(shp);
}


/*
 * ----- sam_sharefs_thread - Async thread for the shared file system.
 * Processes the received messages for the server or client.
 */

void
sam_sharefs_thread(sam_mount_t *mp)
{
	callb_cpr_t cprinfo;
	sam_san_header_t *hdr;
	mblk_t *mbp;
	m_sharefs_item_t *item;
	static clock_t	reduction_time = 0;

	mutex_enter(&mp->ms.m_sharefs.mutex);
	/*
	 * Thread started, signal mount thread.
	 */
	mp->ms.m_sharefs.state = SAM_THR_EXIST;
	mp->ms.m_sharefs.no_threads++;
	cv_signal(&mp->ms.m_sharefs.get_cv);

	/*
	 * Setup the CPR callback for suspend/resume
	 */
	CALLB_CPR_INIT(&cprinfo, &mp->ms.m_sharefs.put_mutex,
	    callb_generic_cpr,
	    "sam_sharefs_thread");

	for (;;) {

		if (mp->ms.m_sharefs.flag == SAM_THR_UMOUNT) {
			mp->ms.m_sharefs.no_threads--;
			cv_broadcast(&mp->ms.m_sharefs.get_cv);
			mutex_exit(&mp->ms.m_sharefs.mutex);
			mutex_enter(&mp->ms.m_sharefs.put_mutex);
			CALLB_CPR_EXIT(&cprinfo);
			thread_exit();
			/* return; */
		}

		item = list_dequeue(&mp->ms.m_sharefs.queue.list);
		if (item == NULL) {
			if ((reduction_time <= ddi_get_lbolt()) &&
			    (mp->ms.m_sharefs.put_wait >=
			    mp->mt.fi_min_pool)) {
				/* don't reduce too quickly */
				reduction_time = ddi_get_lbolt() + (5*hz);
				mp->ms.m_sharefs.no_threads--;
				mutex_exit(&mp->ms.m_sharefs.mutex);
				mutex_enter(&mp->ms.m_sharefs.put_mutex);
				CALLB_CPR_EXIT(&cprinfo);
				thread_exit();
				/* return; */
			}
			mp->ms.m_sharefs.put_wait++;
			mutex_exit(&mp->ms.m_sharefs.mutex);
			mutex_enter(&mp->ms.m_sharefs.put_mutex);
			CALLB_CPR_SAFE_BEGIN(&cprinfo);
			cv_wait(&mp->ms.m_sharefs.put_cv,
			    &mp->ms.m_sharefs.put_mutex);
			CALLB_CPR_SAFE_END(&cprinfo,
			    &mp->ms.m_sharefs.put_mutex);
			mutex_exit(&mp->ms.m_sharefs.put_mutex);
			mutex_enter(&mp->ms.m_sharefs.mutex);
			mp->ms.m_sharefs.put_wait--;
			if (mp->ms.m_sharefs.put_wait < mp->mt.fi_min_pool) {
				reduction_time = ddi_get_lbolt() + (60*hz);
			}
			continue;
		}

		ASSERT(item->count == mp->ms.m_sharefs.queue.current_item);
		mp->ms.m_sharefs.queue.current_item++;
		mp->ms.m_sharefs.queue.items--;

		mp->ms.m_sharefs.busy++;
		if (mp->ms.m_sharefs.busy >
		    sam_thread_stats.max_share_threads.value.i64) {
				sam_thread_stats.max_share_threads.value.i64 =
				    mp->ms.m_sharefs.busy;
		}
		cv_signal(&mp->ms.m_sharefs.get_cv);
		mutex_exit(&mp->ms.m_sharefs.mutex);

		mbp = item->mbp;
		kmem_cache_free(samgt.item_cache, item);

		hdr = (sam_san_header_t *)(void *)mbp->b_rptr;

		if (hdr->originator == SAM_SHARED_CLIENT) {
#ifdef METADATA_SERVER
			client_entry_t *clp = NULL;
			int ord = hdr->client_ord;

			/*
			 * If server, increment busy threads for this client,
			 * ord is zero for initial MOUNT command.
			 */
			if (ord > 0) {
				if ((clp = sam_get_client_entry(mp, ord, 0))) {
					atomic_add_32((uint32_t *)
					    &clp->cl_busy_threads, 1);
				}
			}
			sam_server_cmd(mp, mbp);
			if (clp) {
				atomic_add_32((uint32_t *)
				    &clp->cl_busy_threads, -1);
			}
#else
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: invalid server originator",
			    mp->mt.fi_name);
#endif
		} else {
			sam_san_message_t *msg;

			msg = (sam_san_message_t *)(void *)mbp->b_rptr;
			sam_client_cmd(mp, msg);
		}
		freemsg(mbp);
		mutex_enter(&mp->ms.m_sharefs.mutex);
		mp->ms.m_sharefs.busy--;
		cv_signal(&mp->ms.m_sharefs.get_cv);
	}
}
#endif /* sun */


#ifdef linux
/*
 * ----- sam_put_sock_msg - Send message directly to sockfs.
 *
 */

int					/* ERRNO if error, 0 if successful. */
sam_put_sock_msg(sam_mount_t *mp, struct socket *sock, sam_san_message_t *msg,
    kmutex_t *wmutex)
{
	struct msghdr mh;
	struct iovec iov;
	int error = 0;
	int wlen;
	mm_segment_t save_state;
	int size;
	ushort_t hlength;
	char *msgp;

	msgp = (char *)msg;

	/*
	 * Get a copy of the header length in native endianness.
	 */
	hlength = msg->hdr.length;

	if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
		sam_bswap2((void *)&hlength, 1);
	}

	ASSERT(hlength <= sizeof (sam_san_max_message_t));
	size = SAM_HDR_LENGTH + STRUCT_RND64(hlength);

	msg->hdr.error = local_to_solaris_errno(msg->hdr.error);

	if (sock->state != SS_CONNECTED) {
		return (ENOTCONN);
	}

	save_state = get_fs();
	set_fs(KERNEL_DS);

	mutex_enter(wmutex);

	iov.iov_len = size;
	iov.iov_base = (caddr_t)msgp;

	mh.msg_name = NULL;
	mh.msg_namelen = 0;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	mh.msg_flags = MSG_NOSIGNAL;

	while (size) {
		/*
		 * Send a packet over the socket. Hold the wrmutex to make
		 * sure message is not interleaved with other socket data.
		 */

		wlen = sock_sendmsg(sock, &mh, size);

		error = -wlen;
		if (wlen < 0) {
			if ((error == ERESTARTSYS) || (error == EINTR)) {
				continue;
			}
			if (error == EPIPE) {
				break;
			}
			cmn_err(CE_NOTE,
			    "SAM-QFS: sock_write error=%d", error);
			break;
		}

		if (wlen < size) {
#ifdef DEBUG
			error = sock_error(sock->sk);
			cmn_err(CE_NOTE,
			    "SAM-QFS: incomplete sock_write wlen=%d, "
			    "size=%d, error=%d",
			    wlen, size, error);
#endif
			size -= wlen;
			iov.iov_len = size;
			msgp += wlen;
		} else {
			size = 0;
		}
		error = 0;
	}
	mutex_exit(wmutex);

	set_fs(save_state);

	{
		int32_t  hdr_error = msg->hdr.error;
		ushort_t command = msg->hdr.command;
		ushort_t operation = msg->hdr.operation;
		uint32_t seqno = msg->hdr.seqno;

		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			sam_bswap4((void *)&hdr_error, 1);
			sam_bswap4((void *)&seqno, 1);
			sam_bswap2((void *)&command, 1);
			sam_bswap2((void *)&operation, 1);
		}

		hdr_error = solaris_to_local_errno(msg->hdr.error);

		if (error) {
			TRACE(T_SAM_CL_WRSOCK_ERR,
			    mp, error, (command<<16)|operation, seqno);
			return (error);
		}
		TRACE(T_SAM_CL_WRSOCK_RET,
		    mp, hdr_error, (command<<16)|operation, seqno);
	}
	return (0);
}


/*
 * -----	sam_set_sock_fp - Get the socket.
 * NOTE: mp->ms.m_cl_wrmutex/clp->cl_wrmutex set on entry and exit.
 */

int
sam_set_sock_fp(
	int sockfd,		/* Socket file descriptor */
	sam_sock_handle_t *shp)	/* pointer to pointer to file (returned) */
{
	struct socket *sock;
	int error;

	SAM_SOCK_WRITER_LOCK(shp);

	shp->sh_fp = NULL;

	sock = rfs_sockfd_lookup(sockfd, &error);

	if (sock == NULL) {
		SAM_SOCK_WRITER_UNLOCK(shp);
		return (error);
	}

	shp->sh_fp = sock;

	SAM_SOCK_WRITER_UNLOCK(shp);
	return (0);
}


/*
 * ----- sam_clear_sock_fp - Clear socket fp.
 * Inactivate the socket.
 * NOTE: mp->ms.m_cl_wrmutex/clp->cl_wrmutex set on entry and exit.
 */

void
sam_clear_sock_fp(sam_sock_handle_t *shp)
{
	struct socket *sock;

	SAM_SOCK_WRITER_LOCK(shp);

	sock = shp->sh_fp;
	shp->sh_fp = NULL;

	if (sock) {
		rfs_sockfd_put(sock);
	}

	SAM_SOCK_WRITER_UNLOCK(shp);
}


/*
 * ----- sam_sharefs_thread - Async thread for the shared file system.
 * Processes the received messages for the server or client.
 */

char *sharefs_thread_name = "sharefs_thread";

int
sam_sharefs_thread(void *arg)
{
	sam_san_header_t *hdr;
	struct sk_buff *mbp;
	m_sharefs_item_t *item;
	sam_mount_t *mp;
	static uint64_t	reduction_time = 0;

	SAM_DAEMONIZE(sharefs_thread_name);

	mp = (sam_mount_t *)arg;

	mutex_enter(&mp->ms.m_sharefs.mutex);
	/*
	 * Thread started, signal mount thread.
	 */
	mp->ms.m_sharefs.state = SAM_THR_EXIST;
	mp->ms.m_sharefs.no_threads++;
	cv_signal(&mp->ms.m_sharefs.get_cv);

	for (;;) {

		if (mp->ms.m_sharefs.flag == SAM_THR_UMOUNT) {
			mp->ms.m_sharefs.no_threads--;
			cv_broadcast(&mp->ms.m_sharefs.get_cv);
			mutex_exit(&mp->ms.m_sharefs.mutex);
			return (0);
		}

		item = list_dequeue(&mp->ms.m_sharefs.queue.list);
		if (item == NULL) {
			if ((reduction_time <= ddi_get_lbolt()) &&
			    (mp->ms.m_sharefs.put_wait >=
			    mp->mt.fi_min_pool)) {
				/* don't reduce too quickly */
				reduction_time = ddi_get_lbolt() + (5*hz);
				mp->ms.m_sharefs.no_threads--;
				mutex_exit(&mp->ms.m_sharefs.mutex);
				return (0);
			}
			mp->ms.m_sharefs.put_wait++;
			mutex_exit(&mp->ms.m_sharefs.mutex);
			mutex_enter(&mp->ms.m_sharefs.put_mutex);
			cv_wait_light(&mp->ms.m_sharefs.put_cv,
			    &mp->ms.m_sharefs.put_mutex);
			mutex_exit(&mp->ms.m_sharefs.put_mutex);
			mutex_enter(&mp->ms.m_sharefs.mutex);
			mp->ms.m_sharefs.put_wait--;
			if (mp->ms.m_sharefs.put_wait < mp->mt.fi_min_pool) {
				reduction_time = ddi_get_lbolt() + (60*hz);
			}
			continue;
		}

		ASSERT(item->count == mp->ms.m_sharefs.queue.current_item);
		mp->ms.m_sharefs.queue.current_item++;
		mp->ms.m_sharefs.queue.items--;

		mp->ms.m_sharefs.busy++;
		if (mp->ms.m_sharefs.busy >
		    sam_thread_stats.max_share_threads.value.ui64) {
				sam_thread_stats.max_share_threads.value.ui64 =
				    mp->ms.m_sharefs.busy;
		}
		mutex_exit(&mp->ms.m_sharefs.mutex);

		mbp = item->mbp;
		kmem_cache_free(samgt.item_cache, item);

		hdr = (sam_san_header_t *)mbp->data;
		if (hdr->originator == SAM_SHARED_CLIENT) {
			cmn_err(CE_NOTE,
			    "SAM-QFS: invalid server originator: fs %s",
			    mp->mt.fi_name);
		} else {
			sam_client_cmd(mp, (sam_san_message_t *)mbp->data);
		}
		freemsg(mbp);
		mutex_enter(&mp->ms.m_sharefs.mutex);
		mp->ms.m_sharefs.busy--;
		cv_signal(&mp->ms.m_sharefs.get_cv);
	}
}


/*
 * ----- sam_statfs_thread -
 *
 * Call sam_update_shared_filsys() every VFSSTAT_INTERVAL seconds.
 * Linux kupdated will not call the shared QFS file system, however
 * QFS depends on a periodic BLOCK_vfsstat for correct operation.
 */

char *statfs_thread_name = "statfs_thread";

/*
 * XXX This function is declared 'int', but doesn't have any
 * XXX return statement(s).  Should maybe be a void, but caller
 * XXX apparently expects an int.  Note also that 'error' in
 * XXX this routine is set but not used.  Needs fixing.
 */
int
sam_statfs_thread(void *arg)
{
	sam_mount_t *mp;
	int error;

	SAM_DAEMONIZE(statfs_thread_name);

	mp = (sam_mount_t *)arg;
	mutex_enter(&mp->mi.m_statfs.mutex);

	/*
	 * Thread started, signal mount thread.
	 */
	mp->mi.m_statfs.state = SAM_THR_EXIST;
	cv_signal(&mp->mi.m_statfs.get_cv);

	for (;;) {
		if (mp->mi.m_statfs.flag == SAM_THR_UMOUNT) {
			break;
		}
		mutex_enter(&mp->mi.m_statfs.put_mutex);
		mutex_exit(&mp->mi.m_statfs.mutex);
		/*
		 * Wait VFSSTAT_INTERVAL seconds or until a cv_signal
		 * is received.
		 */
		cv_timedwait_light(&mp->mi.m_statfs.put_cv,
		    &mp->mi.m_statfs.put_mutex,
		    ddi_get_lbolt() + (VFSSTAT_INTERVAL*HZ));

		mutex_exit(&mp->mi.m_statfs.put_mutex);

		mutex_enter(&mp->mi.m_statfs.mutex);
		if (mp->mi.m_statfs.flag == SAM_THR_UMOUNT) {
			break;
		}
		mutex_exit(&mp->mi.m_statfs.mutex);

		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (mp->mi.m_fs_syncing == 0) {
			mp->mi.m_fs_syncing = 1;
			mutex_exit(&mp->ms.m_waitwr_mutex);

			mutex_enter(&mp->ms.m_synclock);
			SAM_DEBUG_COUNT64(statfs_thread);
			error = sam_update_shared_filsys(mp,
			    SHARE_wait_one, 0);
			mutex_exit(&mp->ms.m_synclock);

			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mi.m_fs_syncing = 0;
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);

		mutex_enter(&mp->mi.m_statfs.mutex);
	}

	mp->mi.m_statfs.state = SAM_THR_DEAD;
	cv_broadcast(&mp->mi.m_statfs.get_cv);
	mutex_exit(&mp->mi.m_statfs.mutex);
	return (0);
}
#endif /* linux */
