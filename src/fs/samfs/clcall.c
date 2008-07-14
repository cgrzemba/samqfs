/*
 * ----- clcall.c - Interface for the client file system.
 *
 *	This is the interface module that handles the client requests
 *	for the server.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.251 $"
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
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/kmem.h>
#include <vm/pvn.h>
#include <nfs/nfs.h>
#include <sys/flock.h>
#include <sys/policy.h>
#endif /* sun */

#ifdef linux
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/autoconf.h>
#endif
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/skbuff.h>
#include <linux/ip.h>

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
#include "rwio.h"

#ifdef sun
#include "extern.h"
#endif /* sun */

#ifdef linux
#include "clextern.h"
#include "kstats_linux.h"
#endif /* linux */

#include "debug.h"
#include "behavior_tags.h"

#ifdef sun
#include "kstats.h"
#endif /* sun */

#include "trace.h"
#include "sam_interrno.h"


static int sam_shared_mount(sam_mount_t *mp, boolean_t background);
static void sam_build_mount(sam_mount_t *mp, sam_san_mount_msg_t *msg,
	ushort_t operation);
static int sam_send_to_server(sam_mount_t *mp, sam_node_t *ip,
	sam_san_message_t *msg);
static void sam_srvr_not_responding(sam_mount_t *mp, int errno);
static void sam_srvr_responding(sam_mount_t *mp);
static int sam_wait_server(sam_mount_t *mp, sam_node_t *ip,
	sam_client_msg_t **clpp, sam_san_message_t *msg, int wait_time);
static void sam_finish_message(sam_mount_t *mp, sam_node_t *ip,
	sam_client_msg_t **clpp, sam_san_message_t *msg);
static int sam_delay_client(sam_mount_t *mp, sam_node_t *ip,
	sam_san_message_t *msg, int wait_time);
static int sam_failover_freeze(sam_mount_t *mp, sam_node_t *ip,
	sam_san_message_t *msg);
static sam_client_msg_t *sam_add_message(sam_mount_t *mp,
	sam_san_message_t *msg);
static uint32_t sam_get_earliest_outstanding_msg(sam_mount_t *mp);
static void sam_remove_message(sam_mount_t *mp, sam_client_msg_t *clp);
#ifdef sun
void sam_set_cred(cred_t *credp, sam_cred_t *sam_credp);
#endif

void sam_get_superblock(sam_schedule_entry_t *entry);

#ifdef sun
#pragma rarely_called(sam_srvr_not_responding, sam_srvr_responding)
#pragma rarely_called(sam_delay_client, sam_failover_freeze)
#endif /* sun */

#ifdef linux
char *nfsd_thread_name = "nfsd";
#endif /* linux */


/*
 * ----- sam_share_mount - Processes the privileged mount system call.
 *	Processes the privileged mount request for the shared filesystem.
 */
int				/* ERRNO if error, 0 if successful. */
sam_share_mount(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_share_arg args;
	sam_mount_t *mp;
	int error = 0;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}

	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}

	/*
	 * For now, use secpolicy_fs_config() until the build server is
	 * updated.	 The real routine to use is secpolicy_fs_owner().
	 */
	if (secpolicy_fs_config(credp, NULL)) {
		error = EACCES;
	} else {
		if (SAM_IS_SHARED_FS(mp)) {
			struct sam_fsd_cmd cmd;

			/*
			 * Notify sam-fsd of impending mount.
			 */
			cmd.cmd = FSD_sharedmn;
			bcopy(mp->mt.fi_name, cmd.args.mount.fs_name,
			    sizeof (cmd.args.mount.fs_name));
			/* don't have init value yet */
			cmd.args.mount.init = 0;
			(void) sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd));

			/*
			 * If shared mount, wait mount until client is ready.
			 * Background and soft options from mount or
			 * /etc/vfstab.
			 */
			mp->mt.fi_config |= (args.config & MT_SHARED_BG);
			error = sam_shared_mount(mp, args.background ?
			    TRUE : FALSE);
		} else {
			error = ENOTTY;
		}
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_shared_mount - Process shared mount.
 *	For a shared filesystem send a shared MOUNT command to the
 *	server. The client sends the MOUNT command to the server.
 *	If server, return to caller (mount command) now.
 *	If not server, wait the caller (mount command) until the server
 *	has responded.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_shared_mount(
	sam_mount_t *mp,		/* Mount table pointer */
	boolean_t background)		/* Wait forever if background mount */
{
	sam_san_mount_msg_t *msg;
	int count;
	int error, send_error;

	/*
	 * Delay until the sam-sharefsd client issues SC_client_rdsock
	 * to set host name.
	 */
	msg = kmem_zalloc(sizeof (sam_san_mount_msg_t), KM_SLEEP);

	msg->hdr.magic = SAM_SOCKET_MAGIC;
	msg->hdr.command = SAM_CMD_MOUNT;
	count = 0;
	while (mp->ms.m_cl_hname[0] == '\0') {
		/*
		 * background will always be zero for the first mount attempt,
		 * and non-zero for the retry mount attempts.  The idea is to
		 * perform the first mount attempt in the foreground, and then
		 * perform the retries in the background for the bg mount
		 * option.
		 */
		if (!background && (mp->mt.fi_config & MT_SHARED_BG)) {
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (ETIME);
		}
		if ((error = sam_delay_client(mp, NULL,
		    (sam_san_message_t *)msg, 1)) == EINTR) {
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (EINTR);
		}
		/*
		 * If in the background, wait forever. Otherwise wait
		 * SAM_MOUNT_TIMEOUT seconds (30) for the daemon
		 * to establish the client socket.
		 */
		if (!background && (++count >= SAM_MOUNT_TIMEOUT)) {
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (ENOTCONN);
		}
	}

	count = 0;
newmsg:
	sam_build_mount(mp, msg, MOUNT_init);

	if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
		if (sam_byte_swap(sam_san_header_swap_descriptor, (void *)msg,
		    sizeof (sam_san_header_t))) {

			cmn_err(CE_WARN,
			    "SAM-QFS: %s: shared mount san_header byte "
			    "swap error",
			    mp->mt.fi_name);
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (EIO);
		}

		if (sam_byte_swap(sam_san_mount_swap_descriptor,
		    (void *)&msg->call.mount,
		    sizeof (sam_san_mount_t))) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: shared mount san_mount byte "
			    "swap error",
			    mp->mt.fi_name);
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (EIO);
		}
	}

	error = 0;
	while ((send_error = sam_send_to_server(mp, NULL,
	    (sam_san_message_t *)msg)) != 0) {
		/*
		 * Package protocol version mismatch.
		 * Return error for both foreground and background mounts.
		 */
		if (send_error == EPROTO) {
			cmn_err(CE_WARN,
			    "SAM-QFS: Package protocol version mismatch.");
			cmn_err(CE_WARN,
			    "SAM-QFS: Analyze /var/adm/messages on "
			    "metadata server.");
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (send_error);
		}
		if (!background && (mp->mt.fi_config & MT_SHARED_BG)) {
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (ETIME);
		}
		if ((error = sam_delay_client(mp, NULL,
		    (sam_san_message_t *)msg, 3)) == EINTR) {
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (EINTR);
		}
		/*
		 * If background, wait forever. Otherwise wait 30 seconds for
		 * daemon.
		 */
		if (!background && (++count >= 10)) {
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (ENOTCONN);
		}
		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			if (send_error == EISAM_MOUNT_OUTSYNC) {
				/*
				 * Need to create a new message before resending
				 * since msg contains a response from the server
				 * that is in local byte order.
				 */
				bzero(msg, sizeof (sam_san_mount_msg_t));
				goto newmsg;
			} else {
				/*
				 * Log any other send errors that are suspect.
				 * We probably need to create new messages for
				 * these too.
				 */
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: unexpected mount error "
				    "returned from server,"
				    " error=%d",
				    mp->mt.fi_name, send_error);
			}
		}
	}
	kmem_free(msg, sizeof (sam_san_mount_msg_t));
	return (error);
}


/*
 * ----- sam_build_mount - Build shared mount message.
 * The Mount init and Mount failover init messages reset sequence numbers
 * on the client. The reset_seqno header flag indicates the server should
 * also reset its sequence number. The sequence number is reset to the
 * earliest seqno in the client message array.
 */

static void			/* ERRNO if error, 0 if successful. */
sam_build_mount(
	sam_mount_t *mp,	/* Mount table pointer */
	sam_san_mount_msg_t *msg,
	ushort_t operation)
{
	bzero((void *)msg, sizeof (sam_san_mount_msg_t));
	sam_build_header(mp, &(msg->hdr), SAM_CMD_MOUNT, SHARE_wait, operation,
	    sizeof (sam_san_mount_t), sizeof (sam_san_mount_t));

	if ((operation == MOUNT_init) || (operation == MOUNT_failinit)) {
		msg->hdr.reset_seqno = 1;
	}

	strncpy(msg->call.mount.fs_name, mp->mt.fi_name,
	    sizeof (msg->call.mount.fs_name));
	strncpy(msg->call.mount.hname, mp->ms.m_cl_hname,
	    sizeof (msg->call.mount.hname));
	msg->call.mount.sock_flags = mp->ms.m_cl_sock_flags;
	msg->call.mount.status = mp->mt.fi_status;
	msg->call.mount.config = mp->mt.fi_config;
	msg->call.mount.config1 = mp->mt.fi_config1;
}


#ifdef linux
/*
 * ------ sam_send_shared_umount_blocking -
 *  if shared filesystem, send mount status until server responds
 */
void
sam_send_shared_mount_blocking(sam_mount_t *mp)
{
	if (SAM_IS_SHARED_FS(mp)) {
		sam_san_mount_msg_t *msg;
		int error;

		msg = kmem_zalloc(sizeof (sam_san_mount_msg_t), KM_SLEEP);

		while ((error = sam_send_mount_cmd(mp, msg, MOUNT_status,
		    SAM_MOUNT_TIMEOUT))) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: cannot send mount status to "
			    "server %s, error=%d",
			    mp->mt.fi_name, mp->mt.fi_server, error);
			delay(60*HZ); /* don't flood the situation */
		}
		kmem_free(msg, sizeof (sam_san_mount_msg_t));
	}
}
#endif

/*
 * ----- sam_send_shared_mount - If shared file system, send mount status.
 */
void
sam_send_shared_mount(sam_mount_t *mp)
{

	if (SAM_IS_SHARED_FS(mp)) {
		sam_san_mount_msg_t *msg;
		int error;

		msg = kmem_zalloc(sizeof (sam_san_mount_msg_t), KM_SLEEP);

		if ((error = sam_send_mount_cmd(mp, msg, MOUNT_status,
		    SAM_MOUNT_TIMEOUT))) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: cannot send mount status to "
			    "server %s, error=%d",
			    mp->mt.fi_name, mp->mt.fi_server, error);
		}
		kmem_free(msg, sizeof (sam_san_mount_msg_t));
	}
}


/*
 * ----- sam_send_mount_cmd - Send mount command to server.
 */

int					/* ERRNO if error, 0 if successful. */
sam_send_mount_cmd(
	sam_mount_t *mp,		/* Mount table pointer */
	sam_san_mount_msg_t *msg,
	ushort_t operation,		/* Mount Operation */
	int wait_time)			/* Backoff wait time in seconds */
{
	sam_client_msg_t *clp;
	int error;

	sam_build_mount(mp, msg, operation);
	if (wait_time == 0) {
		msg->hdr.wait_flag = SHARE_nowait;
	}

	if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
		if (sam_byte_swap(sam_san_header_swap_descriptor, (void *)msg,
		    sizeof (sam_san_header_t))) {

			cmn_err(CE_WARN,
			    "SAM-QFS: %s: send mount san_header byte swap "
			    "error",
			    mp->mt.fi_name);
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (EIO);
		}
		if (sam_byte_swap(sam_san_mount_swap_descriptor,
		    (void *)&msg->call.mount,
		    sizeof (sam_san_mount_t))) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: send mount san_mount byte swap "
			    "error",
			    mp->mt.fi_name);
			kmem_free(msg, sizeof (sam_san_mount_msg_t));
			return (EIO);
		}
	}

	clp = sam_add_message(mp, (sam_san_message_t *)msg);
	error = sam_write_to_server(mp,	(sam_san_message_t *)msg);
	if (clp) {
		mutex_exit(&clp->cl_msg_mutex);
	}
	if (error == 0) {
		if (msg->hdr.wait_flag <= SHARE_wait) {
			error = sam_wait_server(mp, NULL, &clp,
			    (sam_san_message_t *)msg, wait_time);
			if (clp) {
				sam_finish_message(mp, NULL, &clp,
				    (sam_san_message_t *)msg);
			}
		}
	} else {
		if (msg->hdr.wait_flag <= SHARE_wait) {
			mutex_enter(&mp->ms.m_cl_mutex);
			sam_remove_message(mp, clp);
			mutex_exit(&mp->ms.m_cl_mutex);
		}
	}
	return (error);
}


/*
 * ----- sam_proc_get_lease - get lease from the server.
 */
int					/* ERRNO if error, 0 if successful. */
sam_proc_get_lease(
	sam_node_t *ip,			/* Pointer to inode */
	sam_lease_data_t *dp,		/* Pointer to arguments */
	sam_share_flock_t *flp,		/* Pointer to flock parameters */
	void *lmfcb,
	cred_t *credp)			/* Pointer to credentials */
{
	enum LEASE_type ltype;
	ushort_t lease_mask;
	boolean_t check_lease_expiration;
	sam_san_lease_msg_t *msg = NULL;
	int error;
	krw_t rw_type;

	check_lease_expiration = TRUE;
	ltype = dp->ltype;
	lease_mask = 1 << ltype;

	/*
	 * For rd/wr/ap, check lease operation and get or extend the lease if:
	 * . Reading past EOF the first time.
	 * . Writing past allocated space.
	 * . Lease already acquired, but lease timer has expired.
	 */
	rw_type = RW_WRITER;

	switch (ltype) {
	case LTYPE_read:
		if ((dp->offset + dp->resid) > ip->size) {
			check_lease_expiration = FALSE;
		}
		rw_type = RW_READER;
		break;

	case LTYPE_write:
		/*
		 * If we don't have a write lease but do have an append lease,
		 * copy the append lease into the write lease.
		 */
		mutex_enter(&ip->ilease_mutex);
		if (!(ip->cl_leases & lease_mask)) {
			if ((ip->cl_leases & (1 << LTYPE_append))) {
				ip->cl_leases |= (1 << LTYPE_write);
				ip->cl_leasetime[LTYPE_write] =
				    ip->cl_leasetime[LTYPE_append];
			}
		}
		mutex_exit(&ip->ilease_mutex);
		/*
		 * If we need to allocate backing store for this write, force a
		 * new lease request to go to the server, even if we have a
		 * lease.
		 */
		if (dp->sparse) {
			check_lease_expiration = FALSE;
		}
		break;

	case LTYPE_append:
		/*
		 * Only allow one append lease outstanding at one time.
		 * Serialized with the apmutex. If more blocks are needed, skip
		 * the lease expiration check and get the blocks now.
		 */
		mutex_enter(&ip->cl_apmutex);
		if (!SAM_IS_SHARED_SERVER(ip->mp)) {
			if ((dp->offset + dp->resid) > ip->cl_allocsz) {
				check_lease_expiration = FALSE;
			}
		}
		break;

	case LTYPE_frlock:
		/*
		 * The file & record lock lease is acquired on the server.
		 */
		check_lease_expiration = FALSE;
		break;

	case LTYPE_stage:
		/*
		 * The stage lease is granted when any other lease is present.
		 */
		check_lease_expiration = FALSE;
		mutex_enter(&ip->cl_apmutex);
		break;

	case LTYPE_open:
		/*
		 * The open lease is acquired when a file is opened.
		 */
		check_lease_expiration = FALSE;
		break;

	case LTYPE_rmap:
		rw_type = RW_READER;
		break;

	case LTYPE_wmap:
		/*
		 * Write mmap needs to allocate and clear range.
		 */
		check_lease_expiration = FALSE;
		break;

	case LTYPE_exclusive:
		check_lease_expiration = FALSE;
		break;

	case LTYPE_truncate:
	default:
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: sam_proc_get_lease: ltype=%d",
		    ip->mp->mt.fi_name, ltype));
		return (EINVAL);
	}

	/*
	 * If check_lease_expiration, check to see if lease expired.
	 * If not expired & appending, allocate ahead if nearing allocsz.
	 * Otherwise, return.
	 */

	RW_LOCK_OS(&ip->inode_rwl, rw_type);

	if (check_lease_expiration) {
		if ((ip->cl_leases & lease_mask) &&
		    (ip->cl_leasetime[ltype] > lbolt)) {
			error = 0;
			if (!SAM_IS_SHARED_SERVER(ip->mp) &&
			    (ltype == LTYPE_append)) {
				if (!ip->di.status.b.direct_map &&
				    (((dp->offset + dp->resid) +
				    ip->cl_alloc_unit) >
				    ip->cl_allocsz) &&
				    (ip->cl_pend_allocsz == 0)) {
					/*
					 * Clients allocate ahead one additional
					 * alloc_unit.  Do not wait.  We only do
					 * this if we're not pre-allocated.
					 */
					ip->cl_pend_allocsz = ip->cl_allocsz;
					msg = kmem_zalloc(sizeof (*msg),
					    KM_SLEEP);
					sam_build_header(ip->mp, &msg->hdr,
					    SAM_CMD_LEASE,
					    SHARE_nothr, LEASE_get,
					    sizeof (sam_san_lease_t),
					    sizeof (sam_san_lease2_t));
					msg->call.lease.data = *dp;
					msg->call.lease.data.offset =
					    ip->cl_allocsz;
					msg->call.lease.data.resid = 0;
					sam_set_cred(credp,
					    &msg->call.lease.cred);
					error = sam_issue_lease_request(ip,
					    msg, SHARE_nothr, 0,
					    lmfcb);
				}

				/*
				 * Append lease has not expired.  Check for
				 * blocks under lock.
				 */
				if ((dp->offset + dp->resid) <=
				    ip->cl_allocsz) {
					mutex_exit(&ip->cl_apmutex);
					RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
					goto out;
				}
			} else {
				if (ltype == LTYPE_append) {
					mutex_exit(&ip->cl_apmutex);
				}
				RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
				return (0); /* msg has not been allocated */
			}
		}
	}

	/*
	 * Create or extend the lease. Set the lease begin time now. The lease
	 * will expire a little bit (travel time) on the client before expiring
	 * on the server. For append lease, double the allocation unit at this
	 * time if it is smaller than configured maximum.
	 */
	if (msg == NULL) {
		msg = kmem_alloc(sizeof (*msg), KM_SLEEP);
	}
	error = 0;

	bzero(msg, sizeof (*msg));
	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE, SHARE_wait,
	    LEASE_get,
	    sizeof (sam_san_lease_t), sizeof (sam_san_lease2_t));
	msg->call.lease.data = *dp;
	if (flp) {
		msg->call.lease.flock = *flp;
	}
	sam_set_cred(credp, &msg->call.lease.cred);

	for (;;) {
		error = sam_issue_lease_request(ip, msg, SHARE_wait, 0, lmfcb);
		if (ltype == LTYPE_open) {
			if (error == 0 &&
			    msg->call.lease2.inp.data.shflags.b.abr) {
				TRACE(T_SAM_ABR_INIT, SAM_ITOP(ip),
				    ip->di.id.ino,
				    ip->flags.b.abr,
				    msg->call.lease2.inp.data.shflags.b.abr);
				SAM_SET_ABR(ip);
			}
		} else if ((ltype == LTYPE_append) || (ltype == LTYPE_stage)) {
			if (error != 0 && IS_SAM_ENOSPC(error)) {

				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				error = sam_wait_space(ip, error);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

				if (error == 0) { /* Try again to get space */
					continue;
				}
			}
			if (!SAM_IS_SHARED_SERVER(ip->mp) && (error == 0)) {
				if ((ip->cl_alloc_unit * 2) <=
				    ip->mp->mt.fi_maxallocsz) {
					ip->cl_alloc_unit *= 2;
				}

				if (!SAM_THREAD_IS_NFS()) {
					ip->flags.b.ap_lease = 1;
				}

				if (dp->offset + dp->resid > ip->cl_allocsz) {
					if ((ip->mp->mt.fi_config &
					    MT_SAM_ENABLED) == 0) {
						error = ENOSPC;
					}
				}
			}
			mutex_exit(&ip->cl_apmutex);
		}
		break;
	}

	/*
	 * XXX - There is a window between when the server grants the lease
	 *		 and when we record it.  This allows a race between
	 *		 granting
	 *		 a lease and expiring it, for instance (if the
	 *		 server is very
	 *		 slow to respond).
	 *
	 * XXX - The only case where "granted_mask" can be zero without error
	 *		 is for certain frlock calls.  These aren't really
	 *		 leases and
	 *		 shouldn't be treated that way.
	 */

	if ((error == 0) && (msg->call.lease2.granted_mask == 0)) {
		ASSERT(ltype == LTYPE_frlock);
	}

	if ((error == 0) && (msg->call.lease2.granted_mask != 0)) {
		int duration;
		boolean_t new_lease;
		uint16_t implicit_mask;

		implicit_mask = msg->call.lease2.granted_mask;
		ASSERT(implicit_mask & (1 << ltype));
		implicit_mask &= ~(1 << ltype);

		if (ltype < MAX_EXPIRING_LEASES) {
			duration = ip->mp->mt.fi_lease[ltype];
		} else {
			duration = MAX_LEASE_TIME;
		}

		new_lease = sam_client_record_lease(ip, ltype, duration);

		if (new_lease || SAM_SEQUENCE_LATER(msg->call.lease.gen[ltype],
		    ip->cl_leasegen[ltype])) {
			ip->cl_leasegen[ltype] = msg->call.lease.gen[ltype];
		}

		/*
		 * Implicit leases may be granted by the server.  Add them too.
		 * Currently, only READ and WRITE can be implicitly granted
		 * Let the loop check all leases, though; short-circuiting will
		 * prevent this from costing us.
		 */

		if (implicit_mask) {
			enum LEASE_type ltype2;

			for (ltype2 = 0;
			    (implicit_mask != 0) && (ltype2 < LTYPE_max_op);
			    ltype2++) {
				if (implicit_mask & (1 << ltype2)) {
					implicit_mask &= ~(1 << ltype2);

					if (ltype2 < MAX_EXPIRING_LEASES) {
						duration =
						    ip->mp->mt.fi_lease[ltype2];
					} else {
						duration = MAX_LEASE_TIME;
					}

					new_lease = sam_client_record_lease(
					    ip, ltype2, duration);

					if (new_lease ||
					    SAM_SEQUENCE_LATER(
					    msg->call.lease.gen[ltype2],
					    ip->cl_leasegen[ltype2])) {
						ip->cl_leasegen[ltype2] =
						    msg->call.lease.gen[ltype2];
					}
				}
			}
		}
	}


	RW_UNLOCK_OS(&ip->inode_rwl, rw_type);

	if (flp) {
		*flp = msg->call.lease.flock;
	}
	if (dp->sparse) {
		dp->no_daus = msg->call.lease.data.no_daus;
		dp->zerodau[0] = msg->call.lease.data.zerodau[0];
		dp->zerodau[1] = msg->call.lease.data.zerodau[1];
	}

	TRACE(T_SAM_CL_LEASE, SAM_ITOP(ip), ip->cl_leases, ip->cl_allocsz,
	    error);

out:
	if (msg != NULL) {
		kmem_free(msg, sizeof (*msg));
	}
	return (error);
}


/*
 * ----- sam_client_record_lease - record a lease on the client.
 *  Returns TRUE if the lease was newly granted, FALSE if we already had it.
 */

boolean_t
sam_client_record_lease(
	sam_node_t *ip,			/* Inode */
	enum LEASE_type leasetype,	/* Type of lease to record */
	int duration)			/* Duration of lease, in seconds */
{
	sam_mount_t *mp;
	boolean_t new_lease;
	ushort_t leasemask;

	if (leasetype == LTYPE_truncate) {	/* transient lease */
		SAM_ASSERT_PANIC("Transient Lease");
	}

	mp = ip->mp;

	leasemask = 1 << leasetype;

	mutex_enter(&mp->mi.m_lease_mutex);
	mutex_enter(&ip->ilease_mutex);

	if (ip->cl_leases == 0) {
		VN_HOLD_OS(ip);	/* Hold inode while leases exist */

		/*
		 * Add new leases to the end of the chain.
		 */
		mp->mi.m_cl_lease_chain.back->forw = &ip->cl_lease_chain;
		ip->cl_lease_chain.forw = &mp->mi.m_cl_lease_chain;
		ip->cl_lease_chain.back = mp->mi.m_cl_lease_chain.back;
		mp->mi.m_cl_lease_chain.back = &ip->cl_lease_chain;

	}

	new_lease = ((ip->cl_leases & leasemask) == 0);
	ip->cl_leases |= leasemask;
	if (ip->cl_leases & CL_APPEND) {
		ip->cl_saved_leases = 0;
	}

	/*
	 * Set duration of lease.  cl_short_leases contains the list of leases
	 * for which an unexpected relinquish was received.  This usually
	 * indicates a race between a lease being issued and a relinquish
	 * request.  Honor these to prevent the relinquish from being lost, but
	 * always allow at least two clock ticks before the lease would expire.
	 * This reduces the chance that we'll schedule a wasteful run of the
	 * expiration thread before the I/O which needed this lease completes.
	 */
	if (ip->cl_short_leases & leasemask) {
		if (ip->cl_leasetime[leasetype] < lbolt + 2) {
			ip->cl_leasetime[leasetype] = lbolt + 2;
		}
	} else {
		ip->cl_leasetime[leasetype] = lbolt + (duration * hz);
	}

	mutex_exit(&ip->ilease_mutex);
	mutex_exit(&mp->mi.m_lease_mutex);

	/*
	 * Schedule client-lease reclamation for when this lease expires.
	 */

	sam_taskq_add(sam_expire_client_leases, mp, NULL, (duration * hz));

	return (new_lease);
}


/*
 * ----- sam_client_remove_leases - remove recorded leases from the client.
 */

void
sam_client_remove_leases(sam_node_t *ip, ushort_t lease_mask,
    int notify_expire)
{
	sam_mount_t *mp = ip->mp;
	boolean_t release = FALSE;

	mutex_enter(&ip->ilease_mutex);

	if (notify_expire) {
		ushort_t leaseused = 0;

		ASSERT((lease_mask & ~(CL_READ|CL_WRITE|CL_APPEND)) == 0);

		leaseused |= ip->cl_leaseused[LTYPE_read] ? CL_READ : 0;
		leaseused |= ip->cl_leaseused[LTYPE_write] ? CL_WRITE : 0;
		leaseused |= ip->cl_leaseused[LTYPE_append] ? CL_APPEND : 0;

		SAM_COUNT64(shared_client, notify_expire);
		if (leaseused & lease_mask) {
			SAM_COUNT64(shared_client, expired_inuse);
			TRACE(T_SAM_CL_EXP_INUSE, SAM_ITOP(ip), lease_mask,
			    leaseused, 0);
			cmn_err(CE_WARN, "SAM-QFS: %s: Server expired "
			    "in-use lease"
			    " ino=%llu.%llu",
			    mp->mt.fi_name,
			    (unsigned long long)ip->di.id.ino,
			    (unsigned long long)ip->di.id.gen);
		}
	}

	if (ip->cl_leases == 0) {
		mutex_exit(&ip->ilease_mutex);
		return;
	}

	ip->cl_leases &= ~lease_mask;
	ip->cl_short_leases &= ~lease_mask;

	if (!(ip->cl_leases & SAM_DATA_MODIFYING_LEASES)) {
		ip->flags.bits &= ~SAM_VALID_MTIME;
	}

	if (ip->cl_leases) {
		mutex_exit(&ip->ilease_mutex);
		return;
	}

	/*
	 * Possibly removing inode from lease chain.
	 * Need to get chain mutex.
	 */

	mutex_exit(&ip->ilease_mutex);

	mutex_enter(&mp->mi.m_lease_mutex);
	mutex_enter(&ip->ilease_mutex);

	if (ip->cl_leases == 0) {
		sam_client_remove_lease_chain(ip);
		release = TRUE;
	}

	mutex_exit(&ip->ilease_mutex);
	mutex_exit(&mp->mi.m_lease_mutex);

	if (release) {
		VN_RELE_OS(ip);
	}
}


/*
 * ----- sam_client_remove_all_leases - remove all leases from the client.
 */

void
sam_client_remove_all_leases(sam_node_t *ip)
{
	if (ip->cl_leases != 0) {
		sam_mount_t *mp = ip->mp;
		boolean_t release;

		release = FALSE;
		mutex_enter(&mp->mi.m_lease_mutex);
		mutex_enter(&ip->ilease_mutex);
		if (ip->cl_leases != 0) {
			ip->cl_leases = 0;
			sam_client_remove_lease_chain(ip);
			release = TRUE;
		}

		ip->cl_short_leases = 0;

		ip->flags.bits &= ~SAM_VALID_MTIME;

		mutex_exit(&ip->ilease_mutex);
		mutex_exit(&mp->mi.m_lease_mutex);
		if (release) {
			VN_RELE_OS(ip);
		}
	}
}


/*
 * ----- sam_client_remove_lease_chain - remove inode from the chain of
 * inodes with pending (client) leases.
 */

void
sam_client_remove_lease_chain(sam_node_t *ip)
{
	if (ip->cl_lease_chain.back) {
		ip->cl_lease_chain.back->forw = ip->cl_lease_chain.forw;
		ip->cl_lease_chain.forw->back = ip->cl_lease_chain.back;
		ip->cl_lease_chain.back = ip->cl_lease_chain.forw = NULL;
	}
}


/*
 * ---- sam_truncate_shared_ino - Truncate inode on the server.
 * The truncate lease is exclusive. It is acquired on the server
 * to truncate the file. If any other lease is outstanding,
 * the caller waits until all leases are removed or expired.
 */

int					/* ERRNO if error, 0 if successful. */
sam_truncate_shared_ino(
	sam_node_t *ip,			/* Pointer to inode */
	sam_lease_data_t *dp,		/* Pointer to arguments */
	cred_t *credp)			/* Pointer to credentials */
{
	sam_san_lease_msg_t *msg;
	int error;

#ifdef	sun
	{
		vnode_t *vp = SAM_ITOV(ip);

		/*
		 * Flush all pages prior to truncate. Errors are ignored.
		 */
		if (vn_has_cached_data(vp)) {
			offset_t addr;

			if (dp->resid < ip->size) {
				addr = (dp->resid + PAGESIZE - 1) & PAGEMASK;
				(void) pvn_vplist_dirty(vp,
				    (sam_u_offset_t)addr,
				    sam_putapage, (B_INVAL | B_TRUNC), credp);
			}
			(void) VOP_PUTPAGE_OS(vp, 0, 0, B_ASYNC, kcred, NULL);
			(void) sam_wait_async_io(ip, TRUE);
			SAM_COUNT64(shared_client, stale_indir);
			(void) sam_stale_indirect_blocks(ip, addr);
		}
	}
#endif	/* sun */

	/*
	 * Truncate is done on server. The server will resolve that there are no
	 * outstanding leases. The truncate lease is immediate and exclusive.
	 */

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE, SHARE_wait,
	    LEASE_get,
	    sizeof (sam_san_lease_t), sizeof (sam_san_lease2_t));
	msg->call.lease.data = *dp;
	sam_set_cred(credp, &msg->call.lease.cred);

	error = sam_issue_lease_request(ip, msg, SHARE_wait, 0, NULL);

	kmem_free(msg, sizeof (*msg));
	TRACE(T_SAM_CL_TRUNC, SAM_ITOP(ip), ip->cl_leases,
	    ip->cl_allocsz, error);
	return (error);
}


/*
 * ----- sam_proc_rm_lease - remove a lease on the server.
 * NOTE: inode_rwl lock set on entry and exit. rw_type is the type of lock.
 */

int				/* ERRNO if error, 0 if successful. */
sam_proc_rm_lease(
	sam_node_t *ip,		/* Pointer to inode */
	ushort_t lease_mask,	/* Mask of leases to be removed */
	krw_t rw_type)		/* Type of inode lock, READER or WRITER */
{
	sam_san_lease_msg_t *msg;
	ushort_t actions = 0;
	boolean_t close;
	int error;

	close = (lease_mask == CL_CLOSE);
	if (close) {
		if (ip->flags.b.ap_lease) {
			if (rw_type == RW_READER) {
				mutex_enter(&ip->fl_mutex);
			}
			ip->flags.b.ap_lease = 0;
			ip->flags.b.updated = 1;
			if (rw_type == RW_READER) {
				mutex_exit(&ip->fl_mutex);
			}
		}
		/*
		 * Force the size update on the server for a close with the
		 * append lease. Note, an outstanding lease relinquish clears
		 * the append lease, so check the saved_leases which is set
		 * across the round trip lease relinquish.  Note, fsflush can
		 * set the size and clear the inode updated flags. An out of
		 * order execution of the close before the fsflush on the server
		 * can cause zeroes at the end of the file. The server truncates
		 * the file and the fsflush resets the correct size.
		 */
		if ((ip->cl_leases | ip->cl_saved_leases) & CL_APPEND) {
			actions = SR_SET_SIZE;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
		sam_client_remove_all_leases(ip);
		RW_LOCK_OS(&ip->inode_rwl, rw_type);

	} else {
		if (lease_mask & CL_APPEND) {
			actions = SR_SET_SIZE;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
		sam_client_remove_leases(ip, lease_mask, 0);
		RW_LOCK_OS(&ip->inode_rwl, rw_type);
	}

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE, SHARE_wait,
	    LEASE_remove, sizeof (sam_san_lease_t), sizeof (sam_san_lease2_t));
	msg->call.lease.data.ltype = lease_mask;
	if (rw_type == RW_READER) {
		if (!RW_TRYUPGRADE_OS(&ip->inode_rwl)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}
	error = sam_issue_lease_request(ip, msg, SHARE_wait, actions, NULL);
	if (rw_type == RW_READER) {
		RW_DOWNGRADE_OS(&ip->inode_rwl);
	}
	kmem_free(msg, sizeof (*msg));
	return (error);
}


/*
 * ----- sam_proc_relinquish_lease - relinquish one or more leases.
 *      This does NOT remove the client-side lease, and should be
 *      paired with a call to sam_client_remove_leases in most cases.
 */

int				/* ERRNO if error, 0 if successful. */
sam_proc_relinquish_lease(
	sam_node_t *ip,		/* Pointer to inode */
	ushort_t lease_mask,	/* Leases to give up */
	boolean_t set_size,	/* Pass inode size to server? */
	uint32_t *leasegen)	/* Points to ary of lease generation numbers */
{
	sam_san_lease_msg_t *msg;
	int error;
	int i;
	ushort_t actions;
	enum SHARE_flag wait_flag;

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	if (set_size) {
		wait_flag = SHARE_wait;
		actions = SR_SET_SIZE;
	} else {
		wait_flag = SHARE_nowait;
		actions = 0;
	}
	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE, wait_flag,
	    LEASE_relinquish, sizeof (sam_san_lease_t),
	    sizeof (sam_san_lease2_t));
	msg->call.lease.data.ltype = lease_mask;

	for (i = 0; i < SAM_MAX_LTYPE; i++) {
		msg->call.lease.gen[i] = *leasegen++;
	}

	if (lease_mask & CL_APPEND) {
		mutex_enter(&ip->cl_apmutex);
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	error = sam_issue_lease_request(ip, msg, wait_flag, actions, NULL);

	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (lease_mask & CL_APPEND) {
		mutex_exit(&ip->cl_apmutex);
	}

	/*
	 * Clear saved leases after relinquish append lease message.
	 */
	if (set_size) {
		mutex_enter(&ip->ilease_mutex);
		if (error == 0) {
			ip->cl_saved_leases = 0;
		} else {
			if (ip->cl_leases & CL_APPEND) {
				ip->cl_saved_leases = 0;
			}
		}
		mutex_exit(&ip->ilease_mutex);
	}

	kmem_free(msg, sizeof (*msg));
	return (error);
}



/*
 * ----- sam_issue_lease_request - issue the lease request.
 * For server, issue directly. Client issues over the wire.
 * Inode READER lock held for a GET READ lease request.
 * Inode WRITER lock held for all other requests, including lease removal.
 */

int			/* ERRNO if error, 0 if successful. */
sam_issue_lease_request(sam_node_t *ip, sam_san_lease_msg_t *msg,
    enum SHARE_flag wait_flag, ushort_t actions, void *lmfcb)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	ushort_t op;
	enum LEASE_type ltype;
	boolean_t is_truncate;	/* Is this a truncate lease-get request? */
	int wait_time;
	int error = 0;
	int r;
	int i;
	krw_t rw_type;
#ifdef sun
	int callbackdone = 0;
#endif /* sun */
	sam_mount_t *mp = ip->mp;
	sam_san_wait_msg_t wait_msg;
	sam_client_msg_t *wait_clp = NULL;

	msg->hdr.wait_flag = (uchar_t)wait_flag;
	op = msg->hdr.operation;
	ltype = lp->data.ltype;
	is_truncate = ((op == LEASE_get) && (ltype == LTYPE_truncate));

	if ((op == LEASE_get) &&
	    ((ltype == LTYPE_read) || (ltype == LTYPE_rmap))) {
		rw_type = RW_READER;
	} else {
		rw_type = RW_WRITER;
	}

	/*
	 * Set requested lease time expiration from the client mount parameters.
	 */
	for (i = LTYPE_read; i < MAX_EXPIRING_LEASES; i++) {
		lp->data.interval[i] = ip->mp->mt.fi_lease[i];
	}

	/*
	 * Build a SAM_CMD_WAIT pseudo message that will be used to catch
	 * NOTIFY_lease messages from the server. Set the waiting inode
	 * so only the tasks waiting on this inode are notified.
	 */
	sam_build_header(ip->mp, &wait_msg.hdr, SAM_CMD_WAIT, SHARE_wait,
	    WAIT_lease, sizeof (sam_san_wait_t), sizeof (sam_san_wait_t));
	wait_msg.call.wait.id = ip->di.id;
	wait_msg.call.wait.ltype = (ushort_t)ltype;

	for (;;) {
		boolean_t on_server;

		lp->id = ip->di.id;

		sam_set_cl_attr(ip, actions, &lp->cl_attr,
		    msg->hdr.wait_flag <= SHARE_wait ? TRUE : FALSE,
		    (rw_type == RW_WRITER));

		if (op == LEASE_get) {
			/*
			 * Add the SAM_CMD_WAIT message early so we don't miss a
			 * NOTIFY_lease message that comes in very close to the
			 * lease response that tells us to sleep, waiting for
			 * the NOTIFY_lease.
			 */
			wait_clp = sam_add_message(ip->mp,
			    (sam_san_message_t *)&wait_msg);
			if (wait_clp) {
				wait_clp->cl_done = 0;
				wait_clp->cl_error = 0;
				mutex_exit(&wait_clp->cl_msg_mutex);
			}
		}

		on_server = SAM_IS_SHARED_SERVER(ip->mp);
		if (on_server) {
reissue:
#ifdef METADATA_SERVER
			error = sam_process_lease_request(ip,
			    (sam_san_message_t *)msg);
			if (error || wait_flag >= SHARE_nothr) {
				/*
				 * error non-zero, didn't get lease, or not
				 * waiting.
				 */
				if (wait_clp) {
					mutex_enter(&mp->ms.m_cl_mutex);
					sam_remove_message(mp, wait_clp);
					mutex_exit(&mp->ms.m_cl_mutex);
				}
				return (error);
			}

			if (l2p->irec.sr_attr.actions & SR_NOTIFY_FRLOCK) {
				sam_restart_waiter(ip->mp, &ip->di.id);
			}

			/*
			 * The only lease operation that can wait is LEASE_get.
			 * All other lease operations do not have SR_WAIT_LEASE
			 * set.
			 */
			if (!(l2p->irec.sr_attr.actions & SR_WAIT_LEASE)) {
				if (!is_truncate) {
					RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
				}
				break;		/* Got lease */
			}
			/*
			 * Inode lock can be safely lifted since the client is
			 * holding the inode lock.
			 */
			RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
#else	/* METADATA_SERVER */
			error = ENOTSUP;
#endif /* METADATA_SERVER */
		} else {
			if (!is_truncate) {
				RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
			}
			i = 0;
reissue2:
			ASSERT(msg->hdr.magic == SAM_SOCKET_MAGIC);
			if (ip->mp->mt.fi_status & FS_SRVR_BYTEREV) {
				/*
				 * Byte swap the message header
				 */
				r = sam_byte_swap(
				    sam_san_header_swap_descriptor,
				    (void *)msg, sizeof (sam_san_header_t));
				if (r) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: "
					    "sam_issue_lease_request "
					    "san_header "
					    "byte swap error",
					    ip->mp->mt.fi_name);
					error = EIO;
					break;
				}

				r = sam_byte_swap(
				    sam_san_lease_swap_descriptor,
				    (void *)&msg->call.lease,
				    sizeof (sam_san_lease_t));
				if (r) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: "
					    "sam_issue_lease_request "
					    "san_lease "
					    "byte swap error",
					    ip->mp->mt.fi_name);
					error = EIO;
					break;
				}
			}
			error = sam_send_to_server(ip->mp, ip,
			    (sam_san_message_t *)msg);
			if (error) {
				break;	/* Error - did not get lease */
			} else if (wait_flag >= SHARE_nothr) {
				break;	/* No thread waiting, don't wait */
			}

			/*
			 * If the lease was processed, but not processed by the
			 * current server, reissue this message. Note this can
			 * happen during failover. If the message is not
			 * reissued on the new server, then the client will have
			 * a different view of leases than the server. The
			 * server will not have the correct size_owner which can
			 * cause the size to NOT be updated.
			 */
			if (msg->hdr.server_ord &&
			    (msg->hdr.server_ord != ip->mp->ms.m_server_ord)) {
				on_server = SAM_IS_SHARED_SERVER(ip->mp);
				if (on_server) {
					/*
					 * The client became the new server
					 * during failover.
					 */
					if (!is_truncate) {
						RW_LOCK_OS(&ip->inode_rwl,
						    rw_type);
					}
					goto reissue;
				} else {
					if (++i < 3) {
					/*
					 * The message is now in local byte
					 * order and may need to be byte swapped
					 * before resending it to the correct
					 * server.
					 */
						goto reissue2;
					}
					error = ESRCH;
					break;
				}
			}

			/*
			 * Complete lease processing for LEASE_get and
			 * LEASE_remove.
			 */
			if ((op != LEASE_relinquish) && (op != LEASE_reset)) {
				sam_complete_lease(ip,
				    (sam_san_message_t *)msg);
			}

			/*
			 * The only lease operation that can wait is LEASE_get.
			 * All other lease operations do not have SR_WAIT_LEASE
			 * set.
			 */
			if (!(l2p->irec.sr_attr.actions & SR_WAIT_LEASE)) {
				break;	/* If not waiting for the lease */
			}
		}

		/*
		 * If another client has this lease, delay & then retry if
		 * time expired.  Should only happen when getting a new lease.
		 * If we see this on a lease reset, something is wrong; return
		 * an error to the caller but don't wait.
		 */
		if (op != LEASE_get) {
			SAM_ASSERT_PANIC("Not Requesting a Lease");

			/*
			 * EACCES is for internal use only
			 * The caller currently doesn't care what the
			 * non-zero return error code is.
			 */
			error = EACCES;
			break;
		}

		if (wait_clp) {
			mutex_enter(&mp->ms.m_cl_mutex);
			if (wait_clp->cl_done) {
				/*
				 * NOTIFY_lease arrived since we got the lease
				 * response that told us to sleep, reissue
				 * LEASE_get message to srvr.
				 */
				error = wait_clp->cl_error;
				sam_remove_message(mp, wait_clp);
				mutex_exit(&mp->ms.m_cl_mutex);
				wait_clp = NULL;
				goto reissue3;
			}
			mutex_exit(&mp->ms.m_cl_mutex);
		}

#ifdef sun
		if ((lmfcb != NULL) && (callbackdone == 0)) {
			/*
			 * Since this thread is about to sleep waiting for a
			 * lease to be granted invoke the provided callback
			 * function.
			 * The NLM provides this to notify NFS
			 * clients to wait for a blocking file lock to be
			 * granted.
			 */
			(void) flk_invoke_callbacks((flk_callback_t *)lmfcb,
			    FLK_BEFORE_SLEEP);
			callbackdone++;
		}
#endif /* sun */

		wait_time = lp->data.interval[ltype];
		if (ltype >= MAX_EXPIRING_LEASES) {
			wait_time = MAX_LEASE_TIME;
		}
		error = sam_wait_server(mp, ip,
		    &wait_clp, (sam_san_message_t *)&wait_msg, wait_time);
		if (wait_clp) {
			sam_finish_message(mp, ip,
			    &wait_clp, (sam_san_message_t *)&wait_msg);
		}
		if (error != ETIME) {
			if (on_server && is_truncate) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
			break;
		}
		/*
		 * Reissue lease messages.
		 */
reissue3:
		sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE, SHARE_wait,
		    LEASE_get, sizeof (sam_san_lease_t),
		    sizeof (sam_san_lease2_t));
		lp->cl_attr.actions  = 0;
		if (on_server || (!is_truncate)) {
			RW_LOCK_OS(&ip->inode_rwl, rw_type);
		}
	}

	if (wait_clp) {
		mutex_enter(&mp->ms.m_cl_mutex);
		sam_remove_message(mp, wait_clp);
		mutex_exit(&mp->ms.m_cl_mutex);
	}

	if (error == 0) {
		/*
		 * If LEASE_relinquish clear SR_INVAL_PAGES|SR_SYNC_PAGES.
		 * This prevents an inode/page deadlock.
		 * Still need SR_STALE_INDIRECT.
		 */
		if (op == LEASE_relinquish) {
			l2p->irec.sr_attr.actions &=
			    ~(SR_INVAL_PAGES|SR_SYNC_PAGES);
		}

		ASSERT(is_truncate ? RW_OWNER_OS(&ip->inode_rwl) ==
		    curthread : 1);
		if (msg->hdr.wait_flag <= SHARE_wait) {
			sam_directed_actions(ip, l2p->irec.sr_attr.actions,
			    l2p->irec.sr_attr.offset, CRED());
		}
	}
	if (!is_truncate) {
		RW_LOCK_OS(&ip->inode_rwl, rw_type);
	}
#ifdef sun
	if (callbackdone > 0) {
		/*
		 * Do the FLK_AFTER_SLEEP callback since
		 * we did the FLK_BEFORE_SLEEP callback.
		 */
		(void) flk_invoke_callbacks((flk_callback_t *)lmfcb,
		    FLK_AFTER_SLEEP);
	}
#endif /* sun */
	return (error);
}


/*
 * ----- sam_proc_name - process meta name operations.
 *	Send name request to server host.
 */

static int			/* ERRNO if error, 0 if successful. */
__sam_proc_name(
	sam_node_t *ip,
	enum NAME_operation op,	/* Name subcommand */
	void *ptr,		/* Pointer to subcommand specific arguments */
	int argsize,		/* Size of subcommand specific arguments */
	char *cp,		/* Pointer to first string */
	unsigned int cplen,	/* length of string */
	char *ncp,		/* Pointer to second string, if one */
	cred_t *credp,		/* Credentials */
	sam_ino_record_t *nrec)	/* out: new inode record information */
{
	sam_san_name_msg_t *msg;
	sam_san_name_t *np;
	int param_len = 0;
	int error;
	int r;

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	np = &msg->call.name;
	np->parent_id = ip->di.id;
	if (ptr) {
		bcopy(ptr, (char *)&np->arg, argsize);
	}
	sam_set_cred(credp, &np->cred);

	switch (op) {
	case NAME_create:
	case NAME_remove:
	case NAME_link:
	case NAME_mkdir:
	case NAME_rmdir:
	case NAME_lookup:
		ASSERT(offsetof(sam_san_name_t, component) + cplen + 1 <=
		    SAM_MAX_NAME_LEN);
		strncpy(np->component, cp, cplen);
		param_len = offsetof(sam_san_name_t, component) + cplen + 1;
		param_len = (param_len + NBPW) & ~(NBPW-1);
		ASSERT(param_len <= SAM_MAX_NAME_LEN);
		break;

	case NAME_rename:
	case NAME_symlink:
		ASSERT(cplen + 1 <= MAXNAMELEN);
		ASSERT(offsetof(sam_san_name_t, component) + MAXNAMELEN +
		    strlen(ncp) + 1 <= SAM_MAX_NAME_LEN);
		strncpy((char *)&np->component, cp, cplen);
		strcpy((char *)&np->component[MAXNAMELEN], ncp);
		param_len = offsetof(sam_san_name_t, component) +
		    MAXNAMELEN + strlen(ncp) + 1;
		param_len = (param_len + NBPW) & ~(NBPW-1);
		ASSERT(param_len <= SAM_MAX_NAME_LEN);
		break;

#ifdef sun
	case NAME_acl:
		if (cp) {
			bcopy(cp, (caddr_t)&np->component,
			    sizeof (aclent_t) * np->arg.p.acl.aclcnt);
		}
		if (ncp) {
			bcopy(ncp,
			    (caddr_t)&np->component[sizeof (aclent_t) *
			    np->arg.p.acl.aclcnt],
			    sizeof (aclent_t) * np->arg.p.acl.dfaclcnt);
		}
		/* space for info + regular ACLs */
		param_len = offsetof(sam_san_name2_t, component) +
		    MAX_ACL_ENTRIES * sizeof (aclent_t);
		if (S_ISDIR(ip->di.mode)) {
			/* space for default ACLs */
			param_len += MAX_ACL_ENTRIES * sizeof (aclent_t);
		}
		break;
#endif /* sun */
#ifdef linux
	case NAME_acl:
		error = ENOSYS;
		break;
#endif /* linux */

	default:
		error = EPROTO;
		goto out;
	}

	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_NAME, SHARE_wait, op,
	    param_len, sizeof (sam_san_name2_t));

	if (ip->mp->mt.fi_status & FS_SRVR_BYTEREV) {

		/*
		 * Byte swap the message header
		 */
		r = sam_byte_swap(sam_san_header_swap_descriptor,
		    (void *)msg, sizeof (sam_san_header_t));
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_name san_header "
			    "byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			goto out;
		}

		switch (op) {
		case NAME_create:
			/*
			 * Byte swap the create arg
			 */
			r = sam_byte_swap(sam_name_create_swap_descriptor,
			    (void *)&msg->call.name.arg.p.create,
			    sizeof (sam_name_create_t));

			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_create byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case NAME_remove:
			/*
			 * Byte swap the remove arg
			 */
			r = sam_byte_swap(sam_name_remove_swap_descriptor,
			    (void *)&msg->call.name.arg.p.remove,
			    sizeof (sam_name_remove_t));

			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_remove byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case NAME_rmdir:
			/*
			 * Byte swap the rmdir arg
			 */
			r = sam_byte_swap(sam_name_rmdir_swap_descriptor,
			    (void *)&msg->call.name.arg.p.rmdir,
			    sizeof (sam_name_rmdir_t));

			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_rmdir byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case NAME_symlink:
			/*
			 * Byte swap the symlink arg
			 */
			r = sam_byte_swap(sam_name_symlink_swap_descriptor,
			    (void *)&msg->call.name.arg.p.symlink,
			    sizeof (sam_name_symlink_t));

			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_symlink byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case NAME_link:
			/*
			 * Byte swap the link arg
			 */
			r = sam_byte_swap(sam_name_link_swap_descriptor,
			    (void *)&msg->call.name.arg.p.link,
			    sizeof (sam_name_link_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_link byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case NAME_rename:
			/*
			 * Byte swap the rename arg
			 */
			r = sam_byte_swap(sam_name_rename_swap_descriptor,
			    (void *)&msg->call.name.arg.p.rename,
			    sizeof (sam_name_rename_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_rename byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

#ifdef sun
		case NAME_acl: {
			sam_name_acl_t *aclp = (sam_name_acl_t *)ptr;

			r = sam_byte_swap(sam_name_acl_swap_descriptor,
			    (void *)&msg->call.name.arg.p.acl,
			    sizeof (sam_name_acl_t));
			/*
			 * If sending ACLs to update them on the server,
			 * byte swap them for the server's consumption.
			 */
			if (aclp->set) {
				int i, nacls, acl_offset;
				int paylen;		/* payload length */

				acl_offset = 0;
				nacls = aclp->aclcnt + aclp->dfaclcnt;
				paylen = param_len -
				    offsetof(sam_san_name_t, component);
				for (i = 0; i < nacls; i++) {
					if (paylen < sizeof (aclent_t)) {
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: name_acl "
						    "byte swap error, i=%d, "
						    "nacls=%d, paylen=%d, "
						    "param_len=%d, off=%d",
						    ip->mp->mt.fi_name,
						    i, nacls, paylen,
						    param_len,
						    (int)offsetof(
						    sam_san_name_t,
						    component));
						r = 1;
						break;
					}
					r = sam_byte_swap(
					    sam_acl_swap_descriptor,
					    &np->component[acl_offset],
					    sizeof (aclent_t));
					acl_offset += sizeof (aclent_t);
					paylen -= sizeof (aclent_t);
				}
			}
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_acl byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;
			}
#endif
#ifdef linux
		case NAME_acl:
			error = ENOSYS;
			goto out;
#endif

		case NAME_mkdir:
			/*
			 * Byte swap the mkdir arg
			 */
			r = sam_byte_swap(sam_name_mkdir_swap_descriptor,
			    (void *)&msg->call.name.arg.p.mkdir,
			    sizeof (sam_name_mkdir_t));

			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_name "
				    "name_mkdir byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case NAME_lookup:
			/*
			 * Nothing to do
			 */
			break;

		default:
			error = EIO;
			goto out;
		}

		/*
		 * This swaps the sam_id_t and the sam_cred_t
		 * but not the sam_name_arg_t
		 */
		r = sam_byte_swap(sam_san_name_swap_descriptor,
		    (void *)&msg->call.name,
		    sizeof (sam_san_name_t));
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_name "
			    "san_name byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			goto out;
		}
	}

	error = sam_send_to_server(ip->mp, ip, (sam_san_message_t *)msg);

	if (error == 0) {
		sam_san_name2_t *n2p;

		n2p = (sam_san_name2_t *)np;
		/*
		 * Reset the original (parent) inode.
		 */
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		(void) sam_reset_client_ino(ip, 0, &n2p->prec);
#ifdef sun
		if (op == NAME_acl) {
			/*
			 * If server sent back updated ACL list, then attach
			 * a copy to the inode.
			 */
			error = sam_reset_client_acl(ip,
			    n2p->arg.p.acl.aclcnt, n2p->arg.p.acl.dfaclcnt,
			    &n2p->component[0],
			    param_len - offsetof(sam_san_name2_t, component));
		}
#endif
		if (nrec != NULL) {
			/*
			 * New inode record for name operations.
			 */
			*nrec = *(&n2p->nrec);
		}
		sam_directed_actions(ip, n2p->prec.sr_attr.actions,
		    n2p->prec.sr_attr.offset, CRED());
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

out:
	kmem_free(msg, sizeof (*msg));
	TRACE(T_SAM_CL_NAME, SAM_ITOP(ip), ip->cl_leases,
	    ip->flags.bits, error);
	return (error);
}


#ifdef sun
int
sam_proc_name(sam_node_t *ip, enum NAME_operation op, void *ptr, int argsize,
    char *cp, char *ncp, cred_t *credp, sam_ino_record_t *nrec)
{
	return __sam_proc_name(ip, op, ptr, argsize, cp,
	    cp ? strlen(cp) : 0,
	    ncp, credp, nrec);
}
#endif /* sun */

#ifdef linux
int
sam_proc_name(sam_node_t *ip, enum NAME_operation op, void *ptr, int argsize,
    struct qstr *cp, char *ncp, cred_t *credp, sam_ino_record_t *nrec)
{
	return __sam_proc_name(ip, op, ptr, argsize, (char *)cp->name,
	    cp->len, ncp, credp, nrec);
}
#endif /* linux */

/*
 * ----- sam_proc_inode - process meta inode operations.
 * Send inode request to server host. The caller waits if SHARE_wait
 * is set; otherwise, the client caller does not wait for a response.
 * Depending on the operation, the caller requests a sync of the inode.
 */

int					/* ERRNO if error, 0 if successful. */
sam_proc_inode(
	sam_node_t *ip,
	enum INODE_operation op,
	void *ptr,			/* Pointer to arguments */
	cred_t *credp)
{
	sam_san_inode_msg_t *msg;
	sam_san_inode_t *inp;
	sam_san_inode2_t *i2p;
	enum SHARE_flag wait_flag;
	boolean_t force_sync;
	int error;
	int r;
	boolean_t grabbed_mutex = FALSE;
	int unset_nb = 0;

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	inp = &msg->call.inode;
	inp->id = ip->di.id;
	sam_set_cred(credp, &inp->cred);
	wait_flag = SHARE_wait;
	force_sync = TRUE;

	switch (op) {
	case INODE_getino:
		wait_flag = SHARE_wait_one;
		break;

	case INODE_fsync_wait:
		break;

	case INODE_fsync_nowait:
		wait_flag = SHARE_nowait;
		break;

	case INODE_setattr: {
		sam_inode_setattr_t *setattr;

		setattr = (sam_inode_setattr_t *)ptr;
		inp->arg.p.setattr = *setattr;
		}
		break;

	case INODE_stage: {
		sam_inode_stage_t *stage;

		wait_flag = SHARE_nowait;
		stage = (sam_inode_stage_t *)ptr;
		inp->arg.p.stage = *stage;
		force_sync = FALSE;
		}
		break;

	case INODE_cancel_stage:
		wait_flag = SHARE_wait_one;
		force_sync = FALSE;
		break;

	case INODE_samattr: {
		sam_inode_samattr_t *samattr;

		samattr = (sam_inode_samattr_t *)ptr;
		inp->arg.p.samattr = *samattr;
		}
		break;

	case INODE_samarch: {
		sam_inode_samarch_t *samarch;

		samarch = (sam_inode_samarch_t *)ptr;
		inp->arg.p.samarch = *samarch;
		}
		break;

	case INODE_samaid: {
		sam_inode_samaid_t *samaid;

		samaid = (sam_inode_samaid_t *)ptr;
		inp->arg.p.samaid = *samaid;
		}
		break;

	case INODE_putquota: {
		sam_inode_quota_t *quota;

		quota = (sam_inode_quota_t *)ptr;
		inp->arg.p.quota = *quota;
		force_sync = FALSE;
		}
		break;

	case INODE_setabr:
		TRACE(T_SAM_ABR_2SRVR, SAM_ITOP(ip), ip->di.id.ino,
		    ip->flags.b.abr, 0);
		break;

	default:
		error = ENOTTY;
		goto out;
	}

	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_INODE, wait_flag, op,
	    sizeof (sam_san_inode_t), sizeof (sam_san_inode2_t));

	if (ip->mp->mt.fi_status & FS_SRVR_BYTEREV) {

		/*
		 * Byte swap the message header
		 */
		r = sam_byte_swap(sam_san_header_swap_descriptor,
		    (void *)msg, sizeof (sam_san_header_t));
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_inode "
			    "san_header byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			goto out;
		}

		r = sam_byte_swap(sam_san_inode_swap_descriptor,
		    (void *)&msg->call.inode,
		    sizeof (sam_san_inode_t));
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_inode "
			    "san_inode byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			goto out;
		}

		switch (op) {
		case INODE_samarch:
			/*
			 * Byte swap the sam_inode_samarch_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_inode_samarch_swap_descriptor,
			    (void *)&msg->call.inode.arg.p.samarch,
			    sizeof (sam_inode_samarch_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_inode "
				    "inode_samarch byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case INODE_samattr:
			/*
			 * Byte swap the sam_inode_samattr_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_inode_samattr_swap_descriptor,
			    (void *)&msg->call.inode.arg.p.samattr,
			    sizeof (sam_inode_samattr_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_inode "
				    "inode_samattr byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case INODE_setattr:
			/*
			 * Byte swap the sam_inode_setattr_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_inode_setattr_swap_descriptor,
			    (void *)&msg->call.inode.arg.p.setattr,
			    sizeof (sam_inode_setattr_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_inode "
				    "inode_setattr byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case INODE_stage:
			/*
			 * Byte swap the sam_inode_stage_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_inode_stage_swap_descriptor,
			    (void *)&msg->call.inode.arg.p.stage,
			    sizeof (sam_inode_stage_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_inode "
				    "inode_stage byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case INODE_putquota:
			/*
			 * Byte swap the sam_inode_quota_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_inode_quota_swap_descriptor,
			    (void *)&msg->call.inode.arg.p.quota,
			    sizeof (sam_inode_quota_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_inode "
				    "inode_quota byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		case INODE_samaid:
			/*
			 * Byte swap the sam_inode_samaid_t portion
			 * of the message.
			 */
			r = sam_byte_swap(sam_inode_samaid_swap_descriptor,
			    (void *)&msg->call.inode.arg.p.samaid,
			    sizeof (sam_inode_samaid_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_inode "
				    "inode_samaid byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto out;
			}
			break;

		default:
			break;
		}
	}

	if ((ip->cl_leases & CL_APPEND) && force_sync && !sam_is_fsflush()) {
		/*
		 * Set this thread to non-blocking since
		 * it will be holding a mutex across a call to
		 * the server and INODE commands are not normally
		 * non-blocking. This will prevent this thread from
		 * being frozen during failover. Freezing a thread
		 * that's holding a mutex during failover can cause
		 * failover to fail to complete.
		 *
		 * This call will return 0 if the thread was
		 * already set non-blocking which means we don't
		 * want to unset it later.
		 */
		unset_nb = sam_set_operation_nb(ip->mp);

		mutex_enter(&ip->cl_apmutex);
		grabbed_mutex = TRUE;
	}
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	sam_set_cl_attr(ip, 0, &inp->cl_attr, force_sync, TRUE);

	if (ip->mp->mt.fi_status & FS_SRVR_BYTEREV) {
		r = sam_byte_swap(sam_cl_attr_swap_descriptor,
		    (void *)&msg->call.inode.cl_attr,
		    sizeof (sam_cl_attr_t));

		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_inode "
			    "cl_attr byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (grabbed_mutex) {
				mutex_exit(&ip->cl_apmutex);
			}
			goto out;
		}
	}

	i2p = (sam_san_inode2_t *)inp;
	if (SAM_IS_SHARED_SERVER(ip->mp)) {
#ifdef METADATA_SERVER
		error = sam_process_inode_request(ip,
		    ip->mp->ms.m_client_ord, op, inp);
#else
		error = ENOTSUP;
#endif /* METADATA_SERVER */
		if (error == 0) {
			sam_directed_actions(ip, i2p->irec.sr_attr.actions,
			    i2p->irec.sr_attr.offset, CRED());
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (grabbed_mutex) {
			mutex_exit(&ip->cl_apmutex);
		}
	} else {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if ((error = sam_send_to_server(ip->mp, ip,
		    (sam_san_message_t *)msg)) == 0) {

			if (grabbed_mutex) {
				mutex_exit(&ip->cl_apmutex);
			}
			if (msg->hdr.wait_flag <= SHARE_wait) {
				/*
				 * Reset inode for all inode commands with wait.
				 */
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				(void) sam_reset_client_ino(ip, 0, &i2p->irec);

				sam_directed_actions(ip,
				    i2p->irec.sr_attr.actions,
				    i2p->irec.sr_attr.offset, CRED());
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		} else if (grabbed_mutex) {
			mutex_exit(&ip->cl_apmutex);
		}
	}

out:
	if (unset_nb) {
		/*
		 * This thread should not continue
		 * non-blocking unless it was already
		 * non-blocking or it may cause failover
		 * to hang.
		 */
		sam_unset_operation_nb(ip->mp);
	}
	kmem_free(msg, sizeof (*msg));
	TRACE(T_SAM_CL_INODE, SAM_ITOP(ip), ip->cl_leases,
	    ip->flags.bits, error);
	return (error);
}


/*
 * ----- sam_proc_block - process block operations.
 *	Send block request to client daemon to forward to server host.
 */

int					/* ERRNO if error, 0 if successful. */
sam_proc_block(
	sam_mount_t *mp,
	sam_node_t *ip,
	enum BLOCK_operation op,
	enum SHARE_flag wait_flag,
	void *ptr)			/* Pointer to arguments */
{
	sam_san_block_msg_t *msg = NULL;
	sam_san_block_t *bnp = NULL;
	int length;
	int error, r;

	msg = kmem_zalloc(sizeof (sam_san_block_msg_t), KM_SLEEP);
	if (!msg) {
		return (ENOMEM);
	}
	bnp = &msg->call.block;

	switch (op) {
	case BLOCK_getbuf: {
		sam_block_getbuf_t *getbuf;

		getbuf = (sam_block_getbuf_t *)ptr;
		bnp->arg.p.getbuf = *getbuf;
		length = bnp->arg.p.getbuf.bsize;
		}
		break;

	case BLOCK_fgetbuf: {
		sam_block_fgetbuf_t *fgetbuf;

		fgetbuf = (sam_block_fgetbuf_t *)ptr;
		bnp->arg.p.fgetbuf = *fgetbuf;
		length = bnp->arg.p.fgetbuf.len;
		}
		break;

	case BLOCK_getino: {
		sam_block_getino_t *getino;

		getino = (sam_block_getino_t *)ptr;
		bnp->arg.p.getino = *getino;
		length = bnp->arg.p.getino.bsize;
		}
		break;

	case BLOCK_getsblk: {
		sam_block_sblk_t *sblk;

		sblk = (sam_block_sblk_t *)ptr;
		bnp->arg.p.sblk = *sblk;
		length = bnp->arg.p.sblk.bsize;
		}
		break;

	case BLOCK_vfsstat_v2: {
		struct sam_sblk *sblk;

		sblk = mp->mi.m_sbp;
		bnp->arg.p.vfsstat_v2.fs_count = sblk->info.sb.fs_count;
		bnp->arg.p.vfsstat_v2.mm_count = sblk->info.sb.mm_count;
		}

		/* LINTED [fallthrough on case statement] */
	case BLOCK_vfsstat:
		/*
		 * Don't update sblk if last update is within 1 second.
		 */
		if (((mp->ms.m_vfs_time + hz) > lbolt) &&
		    (mp->ms.m_vfs_time != 0)) {
			error = 0;
			goto done;
		}
		length = 4;
		break;

	case BLOCK_wakeup:
		length = 4;
		break;

	case BLOCK_panic:
		length = 4;
		break;

	case BLOCK_quota: {
		sam_block_quota_t *qblk;

		qblk = (sam_block_quota_t *)ptr;
		bnp->arg.p.samquota = *qblk;
		length = bnp->arg.p.samquota.len;
		}
		break;

	default:
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: sam_proc_block: invalid op, op=%d",
		    mp->mt.fi_name, op));
		error = EINVAL;
		goto done;
	}
	if (ip != NULL) {
		bnp->id = ip->di.id;
	} else {
		bnp->id.ino = 0;
		bnp->id.gen = 0;
	}

	sam_build_header(mp, &msg->hdr, SAM_CMD_BLOCK, wait_flag, op,
	    sizeof (sam_san_block_t), (sizeof (sam_san_block_t) + length - 4));

	if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
		/*
		 * Byte swap the message header
		 */
		r = sam_byte_swap(sam_san_header_swap_descriptor,
		    (void *)msg,
		    sizeof (sam_san_header_t));
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_block "
			    "san_header byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			goto done;
		}
		/*
		 * Byte swap the common sam_san_block_t portion
		 * of the message
		 */
		r = sam_byte_swap(sam_san_block_swap_descriptor,
		    (void *)&msg->call.block,
		    sizeof (sam_san_block_t));
		if (r) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: sam_proc_block "
			    "san_block byte swap error",
			    ip->mp->mt.fi_name);
			error = EIO;
			goto done;
		}

		switch (op) {
		case BLOCK_getbuf:
			/*
			 * Byte swap the sam_block_getbuf_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_block_getbuf_swap_descriptor,
			    (void *)&msg->call.block.arg.p.getbuf,
			    sizeof (sam_block_getbuf_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_block "
				    "block_getbuf byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto done;
			}
			break;

		case BLOCK_fgetbuf:
			/*
			 * Byte swap the sam_block_fgetbuf_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_block_fgetbuf_swap_descriptor,
			    (void *)&msg->call.block.arg.p.fgetbuf,
			    sizeof (sam_block_fgetbuf_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_block "
				    "block_fgetbuf byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto done;
			}
			break;

		case BLOCK_getsblk:
			/*
			 * Byte swap the sam_block_sblk_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_block_sblk_swap_descriptor,
			    (void *)&msg->call.block.arg.p.sblk,
			    sizeof (sam_block_sblk_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_block "
				    "block_sblk byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto done;
			}
			break;

		case BLOCK_getino:
			/*
			 * Byte swap the sam_block_getino_t portion
			 * of the message
			 */
			r = sam_byte_swap(sam_block_getino_swap_descriptor,
			    (void *)&msg->call.block.arg.p.getino,
			    sizeof (sam_block_getino_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_block "
				    "block_getino byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto done;
			}
			break;

		case BLOCK_vfsstat_v2:
			/*
			 * Byte swap the sam_block_vfsstat_v2 portion
			 * of the message
			 */
			r = sam_byte_swap(sam_block_vfsstat_v2_swap_descriptor,
			    (void *)&msg->call.block.arg.p.vfsstat_v2,
			    sizeof (sam_block_vfsstat_v2_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_block "
				    "block_vfsstat_v2 byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto done;
			}
			break;

		case BLOCK_vfsstat:
		case BLOCK_wakeup:
		case BLOCK_panic:
			/*
			 * Don't need to do anything else
			 */
			break;

		case BLOCK_quota:
			r = sam_byte_swap(sam_block_quota_swap_descriptor,
			    (void *)&msg->call.block.arg.p.samquota,
			    sizeof (sam_block_quota_t));
			if (r) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_proc_block "
				    "block_quota byte swap error",
				    ip->mp->mt.fi_name);
				error = EIO;
				goto done;
			}
			break;

		default:
			error = EINVAL;
			goto done;
		}
	}

	error = sam_send_to_server(mp, ip ? ip : mp->mi.m_inodir,
	    (sam_san_message_t *)msg);
done:
	if (msg) {
		kmem_free(msg, sizeof (sam_san_block_msg_t));
	}
	return (error);
}


/*
 * ----- Return the BLOCK_vfsstat op based on the behavior tags
 */

int
sam_proc_block_vfsstat(sam_mount_t *mp, enum SHARE_flag wait_flag)

{
	enum BLOCK_operation op;
	int error;

	if (sam_server_has_tag(mp, QFS_TAG_VFSSTAT_V2)) {
		op = BLOCK_vfsstat_v2;
	} else {
		op = BLOCK_vfsstat;
	}

resend:
	error = sam_proc_block(mp, NULL, op, wait_flag, NULL);
	if (error == EPROTO && op == BLOCK_vfsstat_v2) {
		op = BLOCK_vfsstat;
		goto resend;
	}

	return (error);
}


/*
 * ----- sam_send_to_server - Send message to the server.
 * If the wait_flag is SHARE_wait, keep retrying to write to
 * the server forever. If the wait_flag is SHARE_wait_one,
 * only try for SAM_MIN_DELAY seconds to write to the server
 * and then return errno ETIME. NOTE, cannot hold up fsflush.
 * If SHARE_wait, retransmit forever, and delay for
 * SAM_MIN_DELAY * n up to SAM_MAX_DELAY where n is the
 * retransmit number.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_send_to_server(
	sam_mount_t *mp,		/* Mount table pointer */
	sam_node_t *ip,			/* Inode pointer, if not mounting */
	sam_san_message_t *msg)		/* Message pointer */
{
	sam_client_msg_t *clp;
	boolean_t mount_msg_sent = FALSE;
	boolean_t server_msg_sent = FALSE;
	int wait;
	int error;

	wait = SAM_MIN_DELAY;
	clp = NULL;
	for (;;) {
		if (clp == NULL) {
			clp = sam_add_message(mp, msg);
		}
		error = sam_write_to_server(mp, msg);
		if (clp) {
			mutex_exit(&clp->cl_msg_mutex);
		}
		if (error == 0) {
			if (msg->hdr.wait_flag <= SHARE_wait) {
				error = sam_wait_server(mp, ip, &clp, msg,
				    wait);
			}
		} else {
			if (msg->hdr.wait_flag <= SHARE_wait) {
				mutex_enter(&mp->ms.m_cl_mutex);
				sam_remove_message(mp, clp);
				mutex_exit(&mp->ms.m_cl_mutex);
				clp = NULL;
			}
		}

		if (error == EXDEV) {
			if (!(mp->mt.fi_status & FS_MOUNTED) &&
			    (mp->mt.fi_config & MT_SHARED_BG)) {
				return (ETIME);
			}
			if (!mount_msg_sent &&
			    (msg->hdr.wait_flag == SHARE_wait)) {
				mount_msg_sent = TRUE;
				uprintf("SAM-QFS: %s: Shared server "
				    "is not mounted\n",
				    mp->mt.fi_name);
			}
			/*
			 * Wait 15 seconds & retry if timeout -- error ETIME
			 * returned.
			 */
			error = sam_delay_client(mp, ip, msg, SAM_MIN_DELAY);

		} else if (error == ENOTCONN || error == EPIPE ||
		    error == ECONNRESET) {
			/*
			 * The message might have been byte swapped to the
			 * server's endianness.  If so, undo this so that the
			 * the fields are in the native endianness in order for
			 * the test below to work.
			 */
			ushort_t command, operation;
			command = msg->hdr.command;
			operation = msg->hdr.operation;

			sam_srvr_not_responding(mp, ENOTCONN);

			if (msg->hdr.magic != SAM_SOCKET_MAGIC) {
				sam_bswap2((void *)&command, 1);
				sam_bswap2((void *)&operation, 1);
			}
			ASSERT(command & 0xff);

			if (mp->mt.fi_status & (FS_FAILOVER|FS_LOCK_HARD)) {
				/*
				 * If failover and message is BLOCK_getbuf or
				 * BLOCK_getino, return so buffer can be freed.
				 * If BLOCK_fgetbuf, return to getpage so reader
				 * lock can be lifted. If BLOCK_vfsstat,
				 * BLOCK_vfsstat_v2, or BLOCK_getsblk,
				 * return so sync processing does not delay.
				 */
				if ((command == SAM_CMD_BLOCK) &&
				    ((operation == BLOCK_getbuf) ||
				    (operation == BLOCK_getino) ||
				    (operation == BLOCK_fgetbuf) ||
				    (operation == BLOCK_getsblk) ||
				    (operation == BLOCK_vfsstat) ||
				    (operation == BLOCK_vfsstat_v2))) {
					break;
				}

				/*
				 * No communication with the server
				 * so allow this thread to freeze.
				 */
				sam_unset_operation_nb(mp);

			} else {
				/*
				 * If not failover and this host is the new
				 * server, return for MOUNT and BLOCK_getbuf,
				 * BLOCK_getino, and BLOCK_fgetbuf commands. All
				 * other commands will be reissued thru the
				 * socket to this host, the new server.
				 */
				if (SAM_IS_SHARED_SERVER(mp)) {
					if (command == SAM_CMD_MOUNT) {
						break;
					}
					if ((command == SAM_CMD_BLOCK) &&
					    ((operation == BLOCK_getbuf) ||
					    (operation == BLOCK_getino) ||
					    (operation == BLOCK_fgetbuf))) {
						break;
					}
				}
			}

			if (!server_msg_sent &&
			    (msg->hdr.wait_flag == SHARE_wait)) {
				server_msg_sent = TRUE;
				uprintf("SAM-QFS: %s: Shared server "
				    "is not responding\n",
				    mp->mt.fi_name);
			}

			/*
			 * Wait 15 seconds & retry if no error.
			 */
			error = sam_delay_client(mp, ip, msg, SAM_MIN_DELAY);

		} else if (error == EISAM_MOUNT_OUTSYNC) {
			sam_san_mount_msg_t mount_msg;

			/*
			 * Resend the server mount information because this
			 * client is NOT in the server client table. This
			 * happens during failover.
			 */
			if ((error = sam_send_mount_cmd(mp, &mount_msg,
			    MOUNT_init,
			    SAM_MOUNT_TIMEOUT)) == 0) {
				msg->hdr.client_ord = mp->ms.m_client_ord;
				if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
					sam_bswap4(
					    (void *)&msg->hdr.client_ord, 1);
				}

				/*
				 * If the client is in a failover state, but the
				 * server is mounted and no longer in a failover
				 * state, set the FS_SRVR_DONE flag. This can
				 * happen if the server has been rebooted.
				 */
				if ((mount_msg.call.mount.status &
				    FS_MOUNTED) &&
				    ((mount_msg.call.mount.status &
				    (FS_RESYNCING|FS_FAILOVER)) == 0)) {
					if (mp->mt.fi_status &
					    (FS_FAILOVER|FS_LOCK_HARD)) {
						mutex_enter(
						    &mp->ms.m_waitwr_mutex);
						mp->mt.fi_status |=
						    FS_SRVR_DONE;
						mutex_exit(
						    &mp->ms.m_waitwr_mutex);
						sam_failover_done(mp);
					} else {
						sam_srvr_responding(mp);
					}
				}
				continue;
			}

		} else if ((error != 0) && (error != ETIME)) {
			/*
			 * The message might have been byte swapped to the
			 * server's endianness.  If so, undo this for the trace
			 * message.
			 */
			ushort_t command, operation;
			uint32_t seqno;
			command = msg->hdr.command;
			operation = msg->hdr.operation;
			seqno = msg->hdr.seqno;

			if (msg->hdr.magic != SAM_SOCKET_MAGIC) {
				sam_bswap2((void *)&command, 1);
				sam_bswap2((void *)&operation, 1);
				sam_bswap4((void *)&seqno, 1);
			}
			ASSERT(command & 0xff);
			TRACE(T_SAM_CL_SEND_ERR, (ip ? SAM_ITOP(ip) : NULL),
			    (command<<16)|operation, seqno, error);
		}

		if (error == ETIME || error == EIO) {
			/*
			 * The message might have been byte swapped to the
			 * server's endianness.  If so, undo this so that the
			 * the fields are in the native endianness in order for
			 * the test below to work.
			 */
			ushort_t command, operation;
			command = msg->hdr.command;
			operation = msg->hdr.operation;

			if (msg->hdr.magic != SAM_SOCKET_MAGIC) {
				sam_bswap2((void *)&command, 1);
				sam_bswap2((void *)&operation, 1);
			}
			ASSERT(command & 0xff);

			/*
			 * A message timed out. Determine if the server
			 * is not responding, or the messages just takes a
			 * long time.
			 */
			if (!SAM_IS_SHARED_SERVER(mp)) {
				int64_t down_time = lbolt - mp->ms.m_srvr_time;

				if (down_time >
				    (int64_t)(SAM_MIN_DELAY * hz)) {
					/*
					 * If we haven't seen this type of
					 * error, or the failure occurred during
					 * a "vfsstat" request.
					 */
					if ((mp->ms.m_sblk_failed == 0) ||
					    ((command == SAM_CMD_BLOCK) &&
					    (operation == BLOCK_vfsstat ||
					    operation == BLOCK_vfsstat_v2))) {
						sam_srvr_not_responding(mp,
						    ETIME);
					}
				}
			}

			/*
			 * Return so background mount can reissue mount command.
			 */
			if ((command == SAM_CMD_MOUNT) &&
			    (operation == MOUNT_init) &&
			    (mp->mt.fi_config & MT_SHARED_BG)) {
				if (clp) {
					sam_finish_message(mp, ip, &clp,
					    (sam_san_message_t *)msg);
				}
				return (error);
			}

			/*
			 * Retransmit all SHARE_wait messages.  SHARE_nothr
			 * messages are advise messages and can be returned
			 * without an error if the server is not responding.
			 * Return ETIME for SHARE_wait_one & SHARE_nowait
			 * messages.
			 */
			if (msg->hdr.wait_flag == SHARE_wait) {
				sam_san_message_t *m;
				m = (sam_san_message_t *)msg;

				if (mp->mt.fi_status &
				    (FS_FAILOVER|FS_LOCK_HARD)) {
					/*
					 * If failover and message is
					 * BLOCK_getbuf or BLOCK_getino, return
					 * so buffer can be freed. If
					 * BLOCK_fgetbuf, return to getpage so
					 * reader lock can be lifted.
					 */
					if ((command == SAM_CMD_BLOCK) &&
					    ((operation == BLOCK_getbuf) ||
					    (operation == BLOCK_getino) ||
					    (operation == BLOCK_fgetbuf))) {
						error = ENOTCONN;
						if (clp) {
							sam_finish_message(mp,
							    ip, &clp, m);
						}
						break;
					}
					wait = SAM_MIN_DELAY;

				} else {
					wait += SAM_MIN_DELAY;
					if (wait > SAM_MAX_DELAY) {
						wait = SAM_MAX_DELAY;
					}
				}

				/*
				 * Retransmit message to the server. Note, this
				 * message is still on the client outstanding
				 * message queue (clp != NULL).  Get the
				 * earliest outstanding message seqno in the
				 * message queue and transmit it to the server
				 * in ack.
				 */
				mutex_enter(&mp->ms.m_cl_mutex);
				/*
				 * XXX - clp is NULL if previous ENOTCONN or
				 * EPIPE
				 */
				if (clp) {
					mutex_enter(&clp->cl_msg_mutex);
					if (clp->cl_done) {
						/*
						 * Completion message was
						 * received after we timed out;
						 * don't retransmit, just clean
						 * up and return.
						 */
						error = clp->cl_error;
						mutex_exit(&clp->cl_msg_mutex);
						mutex_exit(&mp->ms.m_cl_mutex);
						sam_finish_message(mp, ip,
						    &clp,
						    (sam_san_message_t *)msg);
						goto leave;
					}
					/*
					 * Note, must hold cl_msg_mutex through
					 * retransmission at top of loop.
					 */
				}
				msg->hdr.ack =
				    sam_get_earliest_outstanding_msg(mp);

				if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
					sam_bswap4((void *)&msg->hdr.ack, 1);
				}

				mutex_exit(&mp->ms.m_cl_mutex);

				SAM_COUNT64(shared_client, retransmit_msg);
				continue;

			} else if (msg->hdr.wait_flag == SHARE_nothr) {
				error = 0;
			}
			if (clp) {
				sam_finish_message(mp, ip, &clp,
				    (sam_san_message_t *)msg);
			}
		}

leave:
		if (error == 0) {
			/*
			 * Send out server OK message.
			 */
			if (mount_msg_sent) {
				mount_msg_sent = FALSE;
				uprintf("SAM-QFS: %s: Shared server mounted\n",
				    mp->mt.fi_name);
			} else if (server_msg_sent) {
				server_msg_sent = FALSE;
				uprintf("SAM-QFS: %s: Shared server "
				    "responded\n",
				    mp->mt.fi_name);
			}
			if (msg->hdr.wait_flag <= SHARE_wait) {
				mp->ms.m_sblk_failed = 0;
				if ((mp->mt.fi_status & FS_SRVR_DOWN)) {
					sam_srvr_responding(mp);
				}
			}
		}
		break;
	}
	return (error);
}


/*
 * ----- sam_srvr_not_responding - Check server not responding
 * If failover in progress, ignore server not responding.
 * Wake up sam-sharefsd daemon after timeout expired.
 */

static void
sam_srvr_not_responding(sam_mount_t *mp, int errno)
{
	if (mp->mt.fi_status & (FS_FAILOVER | FS_RESYNCING)) {
		return;
	}

	/*
	 * If server is not connected or if server did not respond for
	 * over SAM_MIN_DELAY seconds, wake up sam-sharefsd to
	 * check for a configuration change.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->ms.m_sblk_failed = lbolt;

	/*
	 * Set FS_SRVR_DOWN and FS_LOCK_HARD. Note, FS_LOCK_HARD is
	 * required because all sam-sharefs threads need to freeze to
	 * enable the SC_client_rdsock to return to the sam-sharefsd
	 * daemon. Note, busy sam-sharefs threads keep the sam-sharefsd
	 * daemon from shutting down.
	 */
	if ((mp->mt.fi_status & (FS_SRVR_DOWN|FS_LOCK_HARD)) !=
	    (FS_SRVR_DOWN|FS_LOCK_HARD)) {
		mp->mt.fi_status |= FS_SRVR_DOWN|FS_LOCK_HARD;
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Server not responding, error=%d (%x)",
		    mp->mt.fi_name, errno, mp->mt.fi_status);
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);

	sam_wake_sharedaemon(mp, EINTR);
}


/*
 * ----- sam_srvr_responding - Check server responding
 * If failover in progress, ignore server responding.
 */

static void
sam_srvr_responding(sam_mount_t *mp)
{
	if (mp->mt.fi_status & (FS_FAILOVER | FS_RESYNCING)) {
		return;
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->mt.fi_status & FS_SRVR_DOWN) {
		mp->mt.fi_status &= ~FS_SRVR_DOWN;
		if (!(mp->mt.fi_status & (FS_FAILOVER | FS_RESYNCING))) {
			mp->mt.fi_status &= ~FS_LOCK_HARD;
			if (mp->mi.m_wait_frozen) {
				cv_broadcast(&mp->ms.m_waitwr_cv);
			}
		}
		cmn_err(CE_NOTE, "SAM-QFS: %s: Shared server responded (%x)",
		    mp->mt.fi_name, mp->mt.fi_status);
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);
}


/*
 * ----- sam_wait_server - Wait for response back from the server.
 * Note, if *clpp is set to NULL on return, the message has been
 * removed from the client message chain. Otherwise, the message
 * is still in the message chain.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_wait_server(
	sam_mount_t *mp,		/* Mount pointer */
	sam_node_t *ip,			/* Inode pointer */
	sam_client_msg_t **clpp,	/* Pointer pointer to request */
					/*   in inode client chain */
	sam_san_message_t *msg,		/* Message pointer */
	int wait_time)			/* Backoff wait max time */
{
	sam_client_msg_t *clp = *clpp;
#ifdef sun
	timespec_t start_time, end_time;
#endif /* sun */
#ifdef linux
	struct timeval start_time, end_time;
#endif /* linux */
	int ret;
	int error;
	ushort_t command;
	ushort_t operation;
	uint32_t seqno;

	/*
	 * Increment the number of threads (cl_server_wait) waiting on a
	 * response for this inode from the server.
	 */
	mutex_enter(&mp->ms.m_cl_mutex);
	SAM_HRESTIME(&start_time);
	mp->ms.m_cl_server_wait++;

	/*
	 * The message response might have already arrived due to a race,
	 * and would have been byte swapped to the client's endianess.
	 */
	command = msg->hdr.command;
	operation = msg->hdr.operation;
	seqno = msg->hdr.seqno;

	if (msg->hdr.magic != SAM_SOCKET_MAGIC) {
		sam_bswap2((void *)&command, 1);
		sam_bswap2((void *)&operation, 1);
		sam_bswap4((void *)&seqno, 1);
	}
	TRACE(T_SAM_SRVR_WAIT, (ip ? SAM_ITOP(ip) : NULL),
	    (command<<16)|operation, seqno, mp->ms.m_cl_server_wait);

	error = 0;
	while (!clp->cl_done && !error) {
		ret = sam_cv_wait1sec_sig(&mp->ms.m_cl_cv, &mp->ms.m_cl_mutex);

		if (ret == 0) {
			error = EINTR;	/* If signal received */
		} else {
			if (mp->mt.fi_status & (FS_FAILOVER|FS_LOCK_HARD)) {
				mutex_exit(&mp->ms.m_cl_mutex);
				error = sam_failover_freeze(mp, ip, msg);
				mutex_enter(&mp->ms.m_cl_mutex);
				/* Failover delay */
				wait_time = SAM_FAILOVER_DELAY;
			}
			SAM_HRESTIME(&end_time);
			if ((end_time.tv_sec - start_time.tv_sec) >=
			    wait_time) {
				error = ETIME;	/* Time expired */
			}
		}
	}

	/*
	 * Check for completion. Signal may arrive at time expiration.
	 */
	if (clp->cl_done) {
		error = 0;
	}

	/*
	 * If response was received from the server, set the returned
	 * error (cl_error). Remove the message from the client request chain.
	 */
	if (error == 0) {
		error = clp->cl_error;
	}
	if (error != ETIME) {
		ASSERT(mp->ms.m_cl_server_wait > 0);
		sam_remove_message(mp, clp);
		if (--mp->ms.m_cl_server_wait < 0) {
			mp->ms.m_cl_server_wait = 0;
		}
		mp->ms.m_srvr_time = lbolt;
		*clpp = NULL;
	}
	mutex_exit(&mp->ms.m_cl_mutex);

	TRACE(T_SAM_SRVR_GO, (ip ? SAM_ITOP(ip) : NULL),
	    (command<<16)|operation, seqno, error);
	return (error);
}


/*
 * ----- sam_finish_message - Complete message processing.
 * Remove message from client queue and wake up waiting thread.
 */

static void
sam_finish_message(
	sam_mount_t *mp,		/* Mount pointer */
	sam_node_t *ip,			/* Inode pointer */
	sam_client_msg_t **clpp,	/* Pointer pointer to request in */
					/*   inode client chain */
	sam_san_message_t *msg)		/* Message pointer */
{
	ushort_t command;
	ushort_t operation;
	uint32_t seqno;
	int32_t  error;

	mutex_enter(&mp->ms.m_cl_mutex);
	ASSERT(mp->ms.m_cl_server_wait > 0);
	sam_remove_message(mp, *clpp);
	if (--mp->ms.m_cl_server_wait < 0) {
		mp->ms.m_cl_server_wait = 0;
	}
	*clpp = NULL;
	mutex_exit(&mp->ms.m_cl_mutex);

	command = msg->hdr.command;
	operation = msg->hdr.operation;
	seqno = msg->hdr.seqno;
	error = msg->hdr.error;

	if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
		sam_bswap2((void *)&command, 1);
		sam_bswap2((void *)&operation, 1);
		sam_bswap4((void *)&seqno, 1);
		sam_bswap4((void *)&error, 1);
	}
	TRACE(T_SAM_FINISH_MSG, (ip ? SAM_ITOP(ip) : NULL),
	    (command<<16)|operation, seqno, error);
}


/*
 * ----- sam_delay_client - Delay for wait_time.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_delay_client(
	sam_mount_t *mp,	/* Mount pointer */
	sam_node_t *ip,		/* Inode pointer */
	sam_san_message_t *msg,	/* Message pointer */
	int wait_time)		/* Backoff wait time in seconds */
{
#ifdef sun
	timespec_t start_time, end_time;
#endif
#ifdef linux
	struct timeval start_time, end_time;
#endif /* linux */
	int ret;
	int error;

	error = 0;
	mutex_enter(&mp->ms.m_cl_mutex);
	SAM_HRESTIME(&start_time);
	TRACE(T_SAM_CLNT_DELAY, (ip ? SAM_ITOP(ip) : NULL),
	    (msg->hdr.command<<16)|msg->hdr.operation, msg->hdr.seqno,
	    mp->mt.fi_status);

	for (;;) {
		ret = sam_cv_wait1sec_sig(&mp->ms.m_cl_cv, &mp->ms.m_cl_mutex);
		if (ret == 0) {
			error = EINTR;
		} else {
			if (mp->mt.fi_status & (FS_FAILOVER|FS_LOCK_HARD)) {
				mutex_exit(&mp->ms.m_cl_mutex);
				error = sam_failover_freeze(mp, ip, msg);
				mutex_enter(&mp->ms.m_cl_mutex);
			}
			SAM_HRESTIME(&end_time);
			if ((end_time.tv_sec - start_time.tv_sec) >=
			    wait_time) {
				error = ETIME;
			}
		}
		if (error) {
			break;
		}
	}
	mutex_exit(&mp->ms.m_cl_mutex);
	TRACE(T_SAM_CLNT_GO, (ip ? SAM_ITOP(ip) : NULL),
	    (msg->hdr.command<<16)|msg->hdr.operation, msg->hdr.seqno, error);
	return (error);
}


/*
 * ----- sam_failover_freeze - Freeze the waiting job during
 * failover. Do not freeze block, mount or lease reset commands.
 */

static int
sam_failover_freeze(
	sam_mount_t *mp,		/* Mount pointer */
	sam_node_t *ip,			/* Inode pointer */
	sam_san_message_t *msg)		/* Message pointer */
{
	int error = 0;

	/*
	 * Due to the variety of calling sequences into this routine, not all
	 * messages requiring byte swapping have been byte swapped.  For those
	 * messages that have been byte swapped, we need to undo this so that
	 * the fields are in the native endianness in order for the test below
	 * to work.
	 */
	ushort_t command, operation;
	command = msg->hdr.command;
	operation = msg->hdr.operation;

	if (msg->hdr.magic != SAM_SOCKET_MAGIC) {
		sam_bswap2((void *)&command, 1);
		sam_bswap2((void *)&operation, 1);
	}
	ASSERT(command & 0xff);

	/*
	 * If BLOCK or MOUNT command, return to prevent deadlock.
	 * During failover, allow lease reset thru.
	 */
	if ((command == SAM_CMD_BLOCK) ||
	    (command == SAM_CMD_MOUNT) ||
	    ((command == SAM_CMD_LEASE) &&
	    (operation == LEASE_reset))) {
		return (0);
	}

	if (mp->mt.fi_status & (FS_FAILOVER|FS_LOCK_HARD)) {
		error = sam_stop_inode(ip);
	}
	return (error);
}


/*
 * ----- sam_stop_inode - Freeze the waiting job.
 * During failover, lift writers lock so failover can continue.  Then
 * freeze the caller.  Do not freeze fsflush or the sam_sharefs threads.
 */

int
sam_stop_inode(sam_node_t *ip)
{
	int error = 0;

	if (sam_is_fsflush()) {		/* Do not block fsflush */
		return (ETIME);
	}

	if (ip) {
		boolean_t lock_held;

		lock_held = FALSE;
		if (RW_OWNER_OS(&ip->inode_rwl) == curthread) {
			lock_held = TRUE;
		}
		if (lock_held) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		if (!IS_SHAREFS_THREAD_OS) {
			error = sam_idle_operation(ip);
		}
		if (lock_held) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}
	return (error);
}


/*
 * ----- sam_add_message - Add the message to the inode chain.
 * Only add this message if the sender is waiting for a reply.
 * Chain this message on the forward message queue.
 * Return with the message entry locked to avoid a race condition with
 * threads using this entry in the chain.
 */

static sam_client_msg_t *
sam_add_message(
	sam_mount_t *mp,		/* Mount pointer */
	sam_san_message_t *msg)		/* Message pointer */
{
	/*
	 * Include last received server message id number.
	 * When this ack is returned to the server, it means
	 * this client has processed this and previous messages.
	 */

	/*
	 * If this message has a client waiting thread, put
	 * this message into the forward mount chain.
	 */
	if (msg->hdr.wait_flag <= SHARE_wait) {
		sam_client_msg_t *clp, *lastclp;

		mutex_enter(&mp->ms.m_cl_mutex);
		if ((msg->hdr.seqno = ++mp->ms.m_clnt_seqno) == 0) {
			msg->hdr.seqno = mp->ms.m_clnt_seqno = 1;
		}
		msg->hdr.ack = sam_get_earliest_outstanding_msg(mp);
		if (msg->hdr.ack == 0) {
			msg->hdr.ack = msg->hdr.seqno;
		}

		ASSERT(!SAM_SEQUENCE_LATER(msg->hdr.ack, msg->hdr.seqno));

		clp = mp->ms.m_cl_head;
		lastclp = NULL; /* XXX - why not just add to front of list? */
		while (clp != NULL) {
			lastclp = clp;
			clp = clp->cl_next;
		}
		clp = kmem_cache_alloc(samgt.client_msg_cache, KM_SLEEP);
		bzero(clp, sizeof (*clp));

		sam_mutex_init(&clp->cl_msg_mutex, NULL, MUTEX_DEFAULT, NULL);
		mutex_enter(&clp->cl_msg_mutex);
		clp->cl_msg = msg;
		clp->cl_seqno = msg->hdr.seqno;

		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			sam_bswap4((void *)&msg->hdr.seqno, 1);
			sam_bswap4((void *)&msg->hdr.ack, 1);
		}

		clp->cl_command = msg->hdr.command;
		clp->cl_operation = msg->hdr.operation;
		clp->cl_done = FALSE;

		if (lastclp) {
			lastclp->cl_next = clp;
		} else {
			mp->ms.m_cl_head = clp;
		}
		mutex_exit(&mp->ms.m_cl_mutex);
		return (clp);
	} else {
		mutex_enter(&mp->ms.m_cl_mutex);
		if ((msg->hdr.seqno = ++mp->ms.m_clnt_seqno) == 0) {
			msg->hdr.seqno = mp->ms.m_clnt_seqno = 1;
		}
		mutex_exit(&mp->ms.m_cl_mutex);
		msg->hdr.ack = 0; /* Only acknowledge messages if waiting */

		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			sam_bswap4((void *)&msg->hdr.seqno, 1);
		}

	}
	return (NULL);
}


/*
 * -----	sam_get_earliest_outstanding_msg
 * The server maintains a list of all messages which may be retransmitted
 * from the client and cannot be re-executed (non-idempotent).  A message
 * may be removed from this list when it has been received by the client.
 * To simplify this acknowledgement, we simply track the earliest message
 * sent by the client for which a response has not yet been received from
 * the server.
 *
 * This routine determines that earliest message.  If there are no
 * outstanding messages, it returns 0, and its caller is responsible
 * for acknowledging all messages prior to the message being sent.
 * Note that this requires that this routine be called with the lock
 * protecting outstanding messages held, to avoid a case where two
 * threads are both racing to send a new message when there are no
 * outstanding messages.  We don't want one of those new messages to
 * inadvertently "acknowledge" the other.
 *
 */

static uint32_t			/* Earliest outstanding message seqno or 0 */
sam_get_earliest_outstanding_msg(sam_mount_t *mp)
{
	sam_client_msg_t *clp;
	uint32_t seqno;

	clp = mp->ms.m_cl_head;
	if (clp == NULL) {
		return (0);
	}

	seqno = 0;
	while (clp != NULL) {
		if (!clp->cl_done && (clp->cl_command != SAM_CMD_WAIT)) {
			if (seqno != 0) {
				if (SAM_SEQUENCE_LATER(seqno, clp->cl_seqno)) {
					seqno = clp->cl_seqno;
				}
			} else {
				seqno = clp->cl_seqno;
			}
		}
		clp = clp->cl_next;
	}
	return (seqno);
}


/*
 * ----- sam_remove_message - Remove the message from the inode chain.
 */

static void
sam_remove_message(sam_mount_t *mp, sam_client_msg_t *clp)
{
	sam_client_msg_t *thisclp, *lastclp;

	ASSERT(MUTEX_HELD(&mp->ms.m_cl_mutex));

	thisclp = mp->ms.m_cl_head;
	lastclp = NULL;
	while (thisclp != NULL) {
		if (thisclp == clp) {
			break;
		}
		lastclp = thisclp;
		thisclp = thisclp->cl_next;
	}

	if (thisclp == NULL) {
		cmn_err(SAMFS_DEBUG_PANIC,
		    "SAM-QFS: %s: sam_remove_message: clp=%p",
		    mp->mt.fi_name, (void *)clp);
		return;
	}
	mutex_enter(&clp->cl_msg_mutex);
	if (lastclp) {
		lastclp->cl_next = clp->cl_next;
	} else {
		mp->ms.m_cl_head = clp->cl_next;
	}
	mutex_exit(&clp->cl_msg_mutex);
	kmem_cache_free(samgt.client_msg_cache, clp);
}


/*
 * ----- sam_v_to_v32 - convert 64 vattr to vattr32.
 */

#ifdef	sun
int			/* ERRNO if error, 0 if successful. */
sam_v_to_v32(vattr_t *vap, sam_vattr_t *va32p)
{
	uint_t mask = vap->va_mask;

	bzero(va32p, sizeof (sam_vattr_t));
	va32p->va_mask = vap->va_mask;		/* Bit-mask of attributes */

	if (mask & AT_MODE) {
		va32p->va_mode = vap->va_mode;	/* File access mode */
	}
	if (mask & AT_UID) {
		va32p->va_uid  = vap->va_uid;	/* User id */
	}
	if (mask & AT_GID) {
		va32p->va_gid  = vap->va_gid;	/* Group id */
	}
	if (mask & AT_NLINK) {
		va32p->va_nlink = vap->va_nlink; /* Num references to file */
	}
	if (mask & AT_SIZE) {
		va32p->va_rsize = vap->va_size;	/* File size in bytes */
	}
	if (mask & AT_NBLOCKS) {
		va32p->va_nblocks = vap->va_nblocks; /* num blocks allocated */
	}
	if (mask & AT_ATIME) {
		SAM_TIMESPEC_TO_TIMESPEC32(&va32p->va_atime, &vap->va_atime);
	}
	if (mask & AT_MTIME) {
		SAM_TIMESPEC_TO_TIMESPEC32(&va32p->va_mtime, &vap->va_mtime);
	}
	if (mask & AT_CTIME) {
		SAM_TIMESPEC_TO_TIMESPEC32(&va32p->va_ctime, &vap->va_ctime);
	}
	if (mask & AT_TYPE) {
						/* Vnode type (for create) */
		va32p->va_type = (uint32_t)vap->va_type;
	}
	if (mask & AT_RDEV) {
		va32p->va_rdev = vap->va_rdev;
	}
	return (0);
}
#endif	/* sun */


#ifdef linux
int
qfs_iattr_to_v32(struct iattr *iap, sam_vattr_t *va32p)
{
	uint32_t ia_valid = iap->ia_valid;
	uint32_t va_mask = 0;
	int error = 0;

	bzero(va32p, sizeof (sam_vattr_t));

	if (ia_valid & ATTR_MODE) {
		va32p->va_mode = iap->ia_mode;	/* File access mode */
		va_mask |= QFS_AT_MODE;
	}
	if (ia_valid & ATTR_UID) {
		va32p->va_uid  = iap->ia_uid;	/* User id */
		va_mask |= QFS_AT_UID;
	}
	if (ia_valid & ATTR_GID) {
		va32p->va_gid  = iap->ia_gid;	/* Group id */
		va_mask |= QFS_AT_GID;
	}
	if (ia_valid & ATTR_SIZE) {
		va32p->va_rsize = iap->ia_size;	/* File size in bytes */
		va_mask |= QFS_AT_SIZE;
	}
	if (ia_valid & ATTR_ATIME) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		va32p->va_atime.tv_sec = iap->ia_atime.tv_sec;
#else
		va32p->va_atime.tv_sec = iap->ia_atime;
#endif
		va_mask |= QFS_AT_ATIME;
	}
	if (ia_valid & ATTR_MTIME) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		va32p->va_mtime.tv_sec = iap->ia_mtime.tv_sec;
#else
		va32p->va_mtime.tv_sec = iap->ia_mtime;
#endif
		va_mask |= QFS_AT_MTIME;
	}
	if (ia_valid & ATTR_CTIME) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		va32p->va_ctime.tv_sec = iap->ia_ctime.tv_sec;
#else
		va32p->va_ctime.tv_sec = iap->ia_ctime;
#endif
		va_mask |= QFS_AT_CTIME;
	}
	va32p->va_mask = va_mask;		/* Bit-mask of attributes */

	return (error);
}
#endif /* linux */


/*
 * ----- sam_set_cred - store credential in OTW message.
 */

void
sam_set_cred(cred_t *credp, sam_cred_t *sam_credp)
{
	int i;

#ifdef linux
	if (credp == NULL) {
		/*
		 * Get creds from the current task
		 */
		sam_credp->cr_ref = 1;
		sam_credp->cr_uid = current->fsuid;
		sam_credp->cr_gid = current->fsgid;
		sam_credp->cr_ruid = current->uid;
		sam_credp->cr_rgid = current->gid;
		sam_credp->cr_suid = current->suid;
		sam_credp->cr_sgid = current->sgid;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
		sam_credp->cr_ngroups = current->ngroups;
#else
		sam_credp->cr_ngroups = current->group_info->ngroups;
#endif
		if (sam_credp->cr_ngroups > NGROUPS_MAX_DEFAULT) {
			sam_credp->cr_ngroups = NGROUPS_MAX_DEFAULT;
		}
		for (i = 0; i < sam_credp->cr_ngroups; i++) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
			sam_credp->cr_groups[i] = current->groups[i];
#else
			sam_credp->cr_groups[i] =
			    GROUP_AT(current->group_info, i);
#endif
		}
	} else {
		/*
		 * Use the supplied creds
		 */
		sam_credp->cr_ref = credp->cr_ref;
		sam_credp->cr_uid = credp->cr_uid;
		sam_credp->cr_gid = credp->cr_gid;
		sam_credp->cr_ruid = credp->cr_ruid;
		sam_credp->cr_rgid = credp->cr_rgid;
		sam_credp->cr_suid = credp->cr_suid;
		sam_credp->cr_sgid = credp->cr_sgid;
		sam_credp->cr_ngroups = credp->cr_ngroups;
		if (sam_credp->cr_ngroups > NGROUPS_MAX_DEFAULT) {
			sam_credp->cr_ngroups = NGROUPS_MAX_DEFAULT;
		}
		for (i = 0; i < sam_credp->cr_ngroups; i++) {
			sam_credp->cr_groups[i] = credp->cr_groups[i];
		}
	}
	sam_credp->cr_projid = -1;
#endif /* linux */
#ifdef	sun
	sam_credp->cr_ref = crgetref(credp);
	sam_credp->cr_uid = crgetuid(credp);
	sam_credp->cr_gid = crgetgid(credp);
	sam_credp->cr_ruid = crgetruid(credp);
	sam_credp->cr_rgid = crgetrgid(credp);
	sam_credp->cr_suid = crgetsuid(credp);
	sam_credp->cr_sgid = crgetsgid(credp);
	sam_credp->cr_ngroups = crgetngroups(credp);
	for (i = 0; i < sam_credp->cr_ngroups; i++) {
		sam_credp->cr_groups[i] = crgetgroups(credp)[i];
	}
	sam_credp->cr_projid = crgetprojid(credp);
#endif	/* sun */
}


/*
 * ----- sam_server_has_tag - Return the server behavior tag bits
 */

uint64_t
sam_server_has_tag(sam_mount_t *mp, uint64_t tagsbit)
{
	return (mp->ms.m_server_tags & tagsbit);
}


/*
 * ----- sam_client_has_tag - Return the client behavior tag bits
 */

#ifdef METADATA_SERVER
uint64_t
sam_client_has_tag(client_entry_t *clp, uint64_t tagsbit)
{
	return (clp->cl_tags & tagsbit);
}
#endif /* METADATA_SERVER */
