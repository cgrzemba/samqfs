/*
 * ----- client.c - Process socket messages from the server.
 *
 *	sam_client_cmd - Called when a socket message is read over the wire
 *		by sam_get_sock_msg. There is no response back to the caller.
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

#ifdef sun
#pragma ident "$Revision: 1.178 $"
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
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/flock.h>
#include <sys/dnlc.h>
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

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 5)
#include <linux/namei.h>
#endif
#endif /* linux */


/*
 * ----- SAMFS Includes
 */

#include "sam/types.h"
#include "sam/syscall.h"

#include "ino.h"
#include "inode.h"
#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */
#include "trace.h"
#include "debug.h"
#ifdef sun
#include "kstats.h"
#endif /* sun */
#ifdef linux
#include "kstats_linux.h"
#endif /* linux */


static void sam_set_mount_response(sam_mount_t *mp, sam_san_message_t *msg);
static void sam_set_block_response(sam_mount_t *mp, sam_san_message_t *msg);
static void sam_process_notify_request(sam_mount_t *mp,
					sam_san_message_t *msg);
static void sam_set_lease_response(sam_mount_t *mp, sam_san_message_t *msg);
static void sam_complete_msg(sam_mount_t *mp, sam_san_message_t *msg);

#if defined(SAM_REVOKE_LEASE)
static void sam_revoke_acquired_lease(sam_mount_t *mp, sam_san_message_t *msg);
static void sam_start_frlock_task(sam_node_t *ip, sam_san_message_t *msg);
#endif /* SAM_REVOKE_LEASE */

static int sam_process_callout_request(sam_mount_t *mp,
					sam_san_message_t *msg);
static void sam_process_notify_request(sam_mount_t *mp,
					sam_san_message_t *msg);

void samqfs_notify_dnlc_remove(sam_node_t *pip, char *comp);


/*
 * ----- sam_client_cmd - Process the server response or callout.
 *	Called when the fs client reads a command over the socket.
 *  These commands are the server response to the client initiated commands
 *  or a server callout command.
 *  NOTE. Nothing is done in this path to cause the client daemon
 *  thread to hang. The flush, stale, etc. is done by the calling thread.
 */

void
sam_client_cmd(sam_mount_t *mp, sam_san_message_t *msg)
{
	/*
	 * Process server response to client command or server callout command.
	 * Check for mounted & unchanged for all commands except SAM_CMD_MOUNT.
	 */
	ASSERT(msg->hdr.command & 0xff);
	if (msg->hdr.command != SAM_CMD_MOUNT) {
		if (!(mp->mt.fi_status & FS_MOUNTED)) {
			if ((msg->hdr.command != SAM_CMD_BLOCK) ||
			    ((msg->hdr.command == SAM_CMD_BLOCK) &&
			    !(mp->mt.fi_status & FS_MOUNTING))) {
				goto done;
			}
		} else if ((msg->hdr.command != SAM_CMD_BLOCK) &&
		    (mp->mi.m_sblk_fsid != msg->hdr.fsid)) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Stale file system"
			    " CLI Id=%x SRV Id=%x, cmd=%x, seqno=%x",
			    mp->mt.fi_name, mp->mi.m_sbp->info.sb.init,
			    msg->hdr.fsid,
			    (msg->hdr.command<<16)|msg->hdr.operation,
			    msg->hdr.seqno);
			goto done;
		}
	}

	switch (msg->hdr.command) {

	case SAM_CMD_MOUNT:	/* Record this client ord in the mount tbl. */
		sam_set_mount_response(mp, msg);
		break;

	case SAM_CMD_LEASE:		/* Set lease for the client inode */
		sam_set_lease_response(mp, msg);
		break;

	case SAM_CMD_NAME:		/* Set name operation for the client */
		sam_complete_msg(mp, msg);
		break;

	case SAM_CMD_INODE:		/* Set inode op for the client */
		sam_complete_msg(mp, msg);
		break;

	case SAM_CMD_BLOCK:		/* Set block op for the client */
		sam_set_block_response(mp, msg);
		break;

	case SAM_CMD_CALLOUT:	/* Process callout operation from the server */
		sam_process_callout_request(mp, msg);
		break;

	case SAM_CMD_NOTIFY:	/* Process notify operation from the server */
		sam_process_notify_request(mp, msg);
		break;

	default:
		msg->hdr.error = EPROTO;
		break;
	}

done:
	/*
	 * No response back to the client daemon.
	 */
	return;
	/* out */
}


/*
 * -----	sam_set_mount_response -
 *	Set mount response and wake up waiting thread (mount command)
 *	in sam_shared_mount.
 */

static void			/* ERRNO, else 0 if successful. */
sam_set_mount_response(sam_mount_t *mp, sam_san_message_t *msg)
{
	switch (msg->hdr.operation) {

	/*
	 * The client sends a MOUNT_init to start the mount process.
	 * After the mount has completed and after an umount, the client
	 * sends a MOUNT_status.
	 * The client sends a MOUNT_failinit to start the failover process.
	 * The client sends a MOUNT_resync after the leases have been reset.
	 */
	case MOUNT_init:
	case MOUNT_status:
	case MOUNT_failinit:
	case MOUNT_resync:
		/*
		 * Wake up mount command waiting on SC_share_cmd system call.
		 * Set FS_SAM if SAM is running on the metadata server.
		 */
		if (msg->hdr.error == 0) {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->ms.m_client_ord = msg->hdr.client_ord;
			if (msg->call.mount.config & MT_SAM_ENABLED) {
				mp->mt.fi_status |= FS_SAM;
			} else {
				mp->mt.fi_status &= ~FS_SAM;
			}
			mutex_exit(&mp->ms.m_waitwr_mutex);
		}
		sam_complete_msg(mp, msg);
		break;


	case MOUNT_failover:
		/*
		 * The MOUNT_failover is received from the old server and starts
		 * the failover for the clients. Clear old failover flags
		 * because they can be left set for failed failovers.  Set
		 * freezing. Note the sam-sharefsd daemon will issue the
		 * SC_share_cmd call (SH_setclient) with the new server obtained
		 * from the updated label block.
		 */
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (mp->mt.fi_status & FS_MOUNTED) {
			mp->mt.fi_status &= ~(FS_RESYNCING | FS_THAWING);
			mp->mt.fi_status |= (FS_FREEZING | FS_LOCK_HARD);
#ifdef sun
			TRACE(T_SAM_FREEZING, mp, mp->mt.fi_status,
			    mp->ms.m_sharefs.busy,
			    mp->ms.m_cl_active_ops);
#elif defined(linux)
			TRACE(T_SAM_FREEZING, mp, mp->mt.fi_status,
			    mp->ms.m_sharefs.busy,
			    mp->ms.m_cl_active_ops.counter);
#endif
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: Received failover message "
			    "from old server %s:"
			    " freezing (%x)",
			    mp->mt.fi_name, mp->mt.fi_server,
			    mp->mt.fi_status);
			mutex_exit(&mp->ms.m_waitwr_mutex);
			sam_sighup_daemons(mp);
			msg->hdr.error = 0;
		} else {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Received failover message "
			    "from old server %s:"
			    " fs is not mounted",
			    mp->mt.fi_name, mp->mt.fi_server);
			mutex_exit(&mp->ms.m_waitwr_mutex);
			msg->hdr.error = EIO;
		}
		break;

	case MOUNT_faildone:
		/*
		 * The MOUNT_faildone is received from the new server and
		 * signals the failover has completed on the server. If this
		 * client just mounted, it will not be thawing. Just return.
		 */
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (!(mp->mt.fi_status & FS_THAWING)) {
			mutex_exit(&mp->ms.m_waitwr_mutex);
			break;
		}
		mp->mt.fi_status |= FS_SRVR_DONE;
		mutex_exit(&mp->ms.m_waitwr_mutex);

		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Received faildone message from "
		    "new server %s,"
		    " thawing (%x)",
		    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status);
		sam_failover_done(mp);
		break;

	default:
		break;
	}
}


/*
 * ----- sam_failover_done - finish failover.
 * Clear failover mount flags. Set m_sblk_failed so messages with the ETIME
 * error don't set server down. Wake up any threads which are frozen.
 * Then startup to sam and signal daemons.
 */

void
sam_failover_done(sam_mount_t *mp)
{
	mutex_enter(&mp->ms.m_waitwr_mutex);
	if ((mp->mt.fi_status & (FS_SRVR_DONE | FS_CLNT_DONE)) ==
	    (FS_SRVR_DONE | FS_CLNT_DONE)) {

		/*
		 * Do Post failover processing.
		 */
#ifdef sun
		(void) sam_flush_ino(mp->mi.m_vfsp,
		    SAM_FAILOVER_POST_PROCESSING, 0);
#endif /* sun */
		mp->mt.fi_status &= ~(FS_RESYNCING | FS_THAWING | FS_LOCK_HARD |
		    FS_SRVR_DONE | FS_CLNT_DONE);
		mp->ms.m_involuntary = 0;
		mp->ms.m_sblk_failed = ddi_get_lbolt();
		sam_start_stop_rmedia(mp, SAM_START_DAEMON);
		sam_sighup_daemons(mp);
		if (mp->mi.m_wait_frozen) {
			cv_broadcast(&mp->ms.m_waitwr_cv);
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Failed over to new server %s: (%x)",
		    mp->mt.fi_name, mp->mt.fi_server, mp->mt.fi_status);

	} else {
		mutex_exit(&mp->ms.m_waitwr_mutex);
	}
}


/*
 * ----- sam_sighup_daemons - Tell sam-fsd to send SIGHUP to all daemons.
 */

int		/* ERRNO if error, 0 if successful. */
sam_sighup_daemons(sam_mount_t *mp)
{
	struct sam_fsd_cmd cmd;

	bzero((char *)&cmd, sizeof (cmd));
	cmd.cmd = FSD_restart;
	bcopy((char *)&mp->mt.fi_name, (char *)&cmd.args.restart.fs_name,
	    sizeof (cmd.args.restart.fs_name));
	return (sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd)));
}


/*
 * ----- sam_wake_sharedaemon - Wake up the local sam-sharefsd
 */

void
sam_wake_sharedaemon(
	sam_mount_t *mp,
	int event)	/* should be non-zero (errno) */
{
	mutex_enter(&mp->ms.m_shared_lock);
	mp->ms.m_shared_event = event;
	cv_broadcast(&mp->ms.m_shared_cv);
	mutex_exit(&mp->ms.m_shared_lock);
	TRACE(T_SAM_DAE_WAKE, mp, mp->mt.fi_status, event, 0);
}


/*
 * ----- sam_set_lease_response - set acquired lease in client inode.
 */

static void				/* ERRNO, else 0 if successful. */
sam_set_lease_response(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_node_t *ip = NULL;
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	int error;

	if (msg->hdr.wait_flag <= SHARE_wait) {
		sam_complete_msg(mp, msg);

		/*
		 * Restart task waiting on this inode so the frlock can be
		 * reissued.
		 */
		if (l2p->irec.sr_attr.actions & SR_NOTIFY_FRLOCK) {
			sam_restart_waiter(mp, &lp->id);
		}
		return;
	}

	/*
	 * Throw advise messages away if failing over.
	 */
	if (mp->mt.fi_status & FS_FAILOVER) {
		return;
	}

	if (msg->hdr.error) {
		return;
	}
	error = sam_check_cache(&lp->id, mp, IG_EXISTS, NULL, &ip);
	if ((error == 0) && ip) {
		sam_complete_lease(ip, msg);
		sam_directed_actions(ip, l2p->irec.sr_attr.actions,
		    l2p->irec.sr_attr.offset, CRED());
		VN_RELE_OS(ip);
	}
}


void
sam_complete_lease(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p;
	enum LEASE_operation op;	/* Lease subcommand */
	boolean_t is_truncate;	/* Is this a truncate lease-get request? */
	enum LEASE_type ltype;


	l2p = (sam_san_lease2_t *)lp;
	ltype = msg->call.lease.data.ltype;
	op = msg->hdr.operation;

	is_truncate = ((op == LEASE_get) && (ltype == LTYPE_truncate));
	if (!is_truncate) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	} else {
		/*
		 * Force the reset of the size for a truncate. Note, it is
		 * possible for the size_owner to be set to this client because
		 * this client may issue a truncate while holding
		 * the append lease.
		 */
		l2p->irec.sr_attr.size_owner = 0;
	}

	/*
	 * Reset inode for all lease commands except
	 * LEASE_reset and LEASE_extend.
	 */
	if ((op != LEASE_reset) && (op != LEASE_extend)) {
		(void) sam_reset_client_ino(ip, SAM_CMD_LEASE, &l2p->irec);
	}

	if (!(l2p->irec.sr_attr.actions & SR_WAIT_LEASE)) {
		if ((op == LEASE_get) && (ltype == LTYPE_append)) {
			if (msg->hdr.wait_flag > SHARE_wait) {
				ip->cl_pend_allocsz = 0;
			}
		}
	}

	/*
	 * Truncate leases always set file size.
	 */
	if (is_truncate) {
		sam_set_size(ip);
	}

	if (!is_truncate) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
}


/*
 * ----- sam_set_block_response -
 * Wake up thread waiting on a block operation from the shared server.
 * Make sure the thread is still waiting. If not, buffer is not valid,
 * so throw the message away.
 */

static void
sam_set_block_response(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_block_t *mbp = &msg->call.block;
	sam_client_msg_t *clp = NULL;

	if (msg->hdr.error == 0) {
		if (msg->hdr.wait_flag <= SHARE_wait) {

			mutex_enter(&mp->ms.m_cl_mutex);
			clp = mp->ms.m_cl_head;

			/*
			 * Find this message entry in the client request chain.
			 */
			while (clp != NULL) {
				if (clp->cl_seqno == msg->hdr.seqno) {
					break;
				}
				clp = clp->cl_next;
			}

			if (clp == NULL) {		/* Thread has exited */
				mutex_exit(&mp->ms.m_cl_mutex);
				return;
			}
			/*
			 * Lock this entry during the copy operation so this
			 * thread does not terminate & return this message
			 * entry.
			 */
			mutex_enter(&clp->cl_msg_mutex);
			mutex_exit(&mp->ms.m_cl_mutex);
		}

		/*
		 * Copy the data into the buffer.
		 */
		switch (msg->hdr.operation) {

		case BLOCK_getbuf: {
			sam_block_getbuf_t *gp = &mbp->arg.p.getbuf;
#ifdef sun
			bcopy(mbp->data, (char *)gp->addr, gp->bsize);
#endif /* sun */
#ifdef linux
			int rsize;
			char *getbuf_addr;

			bcopy((char *)&gp->addr, (char *)&getbuf_addr,
			    sizeof (char *));
			rsize = (int)gp->bsize;
			bcopy(mbp->data, getbuf_addr, rsize);
#endif /* linux */
			}
			break;

		case BLOCK_fgetbuf: {
			sam_block_fgetbuf_t *fp = &mbp->arg.p.fgetbuf;
#ifdef sun
			struct sam_dirval *dvp;
			buf_t *bp;

			bp = (buf_t *)fp->bp;

			dvp = (struct sam_dirval *)(void *)
			    (mbp->data + DIR_BLK - sizeof (struct sam_dirval));
			if (fp->ino != dvp->d_id.ino) {
				cmn_err(CE_NOTE,
				    "SAM-QFS: client.c fbread ino=%d, %d.%d ",
				    fp->ino, dvp->d_id.ino, dvp->d_id.gen);
			}
			bcopy(mbp->data, bp->b_un.b_addr, fp->len);
#endif /* sun */
#ifdef linux
			char *bp;
			int bsize;

			bp = 0;
			memcpy((void *)&bp, (void *)&fp->bp, sizeof (char *));
			bsize = fp->len;

			bcopy(mbp->data, bp, bsize);
#endif /* linux */
			}
			break;

		case BLOCK_getino: {
			sam_block_getino_t *gino = &mbp->arg.p.getino;
#ifdef	sun
			bcopy(mbp->data, (char *)gino->addr, gino->bsize);
#endif /* sun */
#ifdef linux
			char *inoblk_addr;
			int rsize;

			memcpy((void *)&inoblk_addr,
			    (void *)&gino->addr, sizeof (char *));
			rsize = (int)gino->bsize;
			memcpy(inoblk_addr, mbp->data, rsize);
#endif /* linux */
			}
			break;

		case BLOCK_getsblk: {
			sam_block_sblk_t *sblk = &mbp->arg.p.sblk;
#ifdef linux
			char *sblk_addr;
			int rsize;
#endif /* linux */

			mutex_enter(&mp->ms.m_waitwr_mutex);
			if (SAM_IS_SHARED_CLIENT(mp)) {
#ifdef sun
				bcopy(mbp->data, (char *)sblk->addr,
				    sblk->bsize);
#endif /* sun */
#ifdef linux
				memcpy((void *)&sblk_addr,
				    (void *)&sblk->addr, sizeof (char *));

				rsize = (int)sblk->bsize;
				memcpy(sblk_addr, mbp->data, rsize);
#endif /* linux */
				mp->ms.m_cl_vfs_time = mp->ms.m_sblk_time =
				    ddi_get_lbolt();
			}
			mutex_exit(&mp->ms.m_waitwr_mutex);
			}
			break;

		case BLOCK_vfsstat_v2:
		case BLOCK_vfsstat: {
			sam_block_vfsstat_t *sp = &mbp->arg.p.vfsstat;
			struct sam_sblk *sblk = mp->mi.m_sbp;

			if (sblk) {
				sblk->info.sb.capacity = sp->capacity;
				sblk->info.sb.space = sp->space;
				sblk->info.sb.mm_capacity = sp->mm_capacity;
				sblk->info.sb.mm_space = sp->mm_space;
			}
			mp->ms.m_cl_vfs_time = ddi_get_lbolt();
			}
			break;

		case BLOCK_quota: {
			sam_block_quota_t *qargp = &mbp->arg.p.samquota;

			bcopy((char *)mbp->data, (char *)qargp->buf,
			    qargp->len);
			}
			break;

		default:
			break;
		}
	}

	/*
	 * If entry locked, unlock. Wake up the waiting inode.
	 */
	if (clp) {
		mutex_exit(&clp->cl_msg_mutex);
	}
	sam_complete_msg(mp, msg);
}


/*
 * ----- sam_reset_client_ino -
 * Refresh the inode data from OTW.
 * NOTE: requires inode write lock set (ip->inode_rwl).
 *
 * There are two different synchronization mechanisms used here.
 *
 * For general inode information, a sequence number is generated on the
 * server when a response is transmitted.  The client checks this and
 * does not update its copy if it receives an older sequence number.
 * This prevents out-of-order responses from causing problems.
 *
 * A client which is appending to a file may update the file size while
 * the server is processing requests.  Any updates to the size received
 * from the server must be handled with care.  In particular, it is
 * possible for the client to receive a response from the server which
 * contains an "out-of-date" size if the client -- AT THE TIME THE
 * RESPONSE WAS BEING COMPOSED -- was appending to the file.  It is not
 * sufficient for the client to check whether it is currently appending
 * to the file, because the response may be received after the client
 * has relinquished the append lease (which carries with it permission
 * to update the file size).  To handle this, the server records which
 * client was controlling the file size at the time it transmits inode
 * updates.  If this client was in control, the file size will not be
 * updated (since we were still the authority at the time).
 */

int
sam_reset_client_ino(
	sam_node_t *ip,			/* Pointer to inode table */
	int cmd,
	sam_ino_record_t *irec)		/* Inode instance info */
{
	offset_t real_size;
	sam_timestruc_t modify_time;
	sam_time_t residence_time;
	int error = 0;
#ifdef linux
	struct inode *li;
#endif /* linux */
#ifdef sun
	int was_rmchardev = 0;
	vnode_t *vp = SAM_ITOV(ip);
#endif

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	/*
	 * Check for out of order sequence inode information. Only reset the
	 * inode if the inode sequence number indicates this is the first time
	 * we are resetting the inode (ip->cl_ino_seqno = 0), the server changed
	 * (failover or remount), we are forcing the reset (ino_seqno = 0), or
	 * this is a newer copy of the inode information.
	 */
	if ((ip->cl_ino_seqno != 0) && (ip->cl_srvr_ord == irec->in.srvr_ord) &&
	    (ip->cl_ino_gen == irec->in.ino_gen) && (irec->in.seqno != 0)) {
		if (SAM_SEQUENCE_LATER(ip->cl_ino_seqno, irec->in.seqno)) {
			TRACE(T_SAM_CL_SEQNO, SAM_ITOP(ip), ip->cl_ino_seqno,
			    ip->cl_ino_gen, ip->cl_srvr_ord);
			TRACE(T_SAM_SR_SEQNO, SAM_ITOP(ip), irec->in.seqno,
			    irec->in.ino_gen, irec->in.srvr_ord);
			return (0);
		}
	}
	ip->cl_ino_seqno = irec->in.seqno;
	ip->cl_ino_gen = irec->in.ino_gen;
	ip->cl_srvr_ord = irec->in.srvr_ord;
	modify_time.tv_sec = ip->di.modify_time.tv_sec;
	modify_time.tv_nsec = ip->di.modify_time.tv_nsec;
	residence_time = ip->di.residence_time;
	real_size = ip->di.rm.size;
#ifdef linux
	if (S_ISLNK(ip->di.mode) && (ip->di.ext_attrs & ext_sln)) {
		real_size = ip->di.psize.symlink;
	}
#endif /* linux */

#ifdef sun
	if (ip->di.rm.ui.flags & RM_CHAR_DEV_FILE && (vp->v_type == VCHR)) {
		was_rmchardev = 1;
	}
#endif
	ASSERT(((ip->di.id.ino == irec->di.id.ino) &&
	    (ip->di.id.gen == irec->di.id.gen)) || (ip->di.mode == 0));
	ip->di = irec->di;	/* Move disk image to incore inode */
	ip->di2	= irec->di2;
	ip->updtime = SAM_SECOND();

	/*
	 * Update server returned attributes - incore inode information.
	 *
	 * Update file size only if we were not the "owner" of the size
	 * at the time it was transmitted by the server.  This avoids a
	 * race condition where we could get a stale size.  Note, for
	 * the first append, even though we are the owner, trust the size
	 * from the server. In all other cases where we are the owner, need
	 * to restore the on-disk size from the client's size.
	 */

#ifdef sun
	if ((irec->sr_attr.size_owner != ip->mp->ms.m_client_ord) ||
	    (irec->sr_attr.actions & SR_FORCE_SIZE)) {
		ip->size = irec->sr_attr.current_size;
#ifndef _NoOSD_
		if (SAM_IS_OBJECT_FILE(ip)) {
			/*
			 * When we move the disk image into the incore inode,
			 * this is the first time the object file bit is set.
			 * This is why the layout was not set in sam_get_ino.
			 * If this is a reset for a NAME_create, we need to
			 * create the object layout. Otherwise the object
			 * layout exists and we just need to set the end
			 * of object (eoo) for each stripe. Note, the end of
			 * object is set if we are creating the object layout.
			 */
			if (ip->olp == NULL) {
				sam_osd_create_obj_layout(ip);
			} else {
				(void) sam_set_end_of_obj(ip,
				    ip->di.rm.size, 1);
			}
		}
#endif
	} else {
		ip->di.rm.size = real_size;
	}

	if (was_rmchardev) {
		mutex_enter(&vp->v_lock);
		if (!(ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
		    (vp->v_type == VCHR) &&
		    (ip->no_opens == 0) &&
		    (cmd != SAM_CMD_LEASE)) {
			/*
			 * The RM_CHAR_DEV_FILE flag was removed. If it's not
			 * open locally and didn't just get a lease clean it up.
			 * If it's open locally unpredictable things may happen.
			 */
			sam_detach_aiofile(vp);
		}
		mutex_exit(&vp->v_lock);
	}
#endif /* sun */

#ifdef linux
	li = SAM_SITOLI(ip);

	if ((irec->sr_attr.size_owner != ip->mp->ms.m_client_ord) ||
	    (irec->sr_attr.actions & SR_FORCE_SIZE)) {
		ip->size = irec->sr_attr.current_size;
		if (li) {
			if (ip->di.status.b.offline) {
				rfs_i_size_write(li, real_size);
			} else {
				rfs_i_size_write(li, ip->size);
			}
		}
	} else {
		ip->di.rm.size = real_size;
	}
	if (li) {
		/*
		 * Update the Linux inode
		 */
		li->i_mode = ip->di.mode;
		li->i_nlink = ip->di.nlink;
		li->i_uid = ip->di.uid;
		li->i_gid = ip->di.gid;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		li->i_atime.tv_sec = ip->di.access_time.tv_sec;
		li->i_mtime.tv_sec = ip->di.modify_time.tv_sec;
		li->i_ctime.tv_sec = ip->di.change_time.tv_sec;
#else
		li->i_atime = ip->di.access_time.tv_sec;
		li->i_mtime = ip->di.modify_time.tv_sec;
		li->i_ctime = ip->di.change_time.tv_sec;
#endif
		li->i_blocks =
		    (u_longlong_t)ip->di.blocks *
		    (u_longlong_t)(SAM_BLK/DEV_BSIZE);
	}
#endif /* linux */

	/*
	 * Server controls stage size (amount of file online) and allocated
	 * size.  Always update these.
	 */
	ip->stage_size = irec->sr_attr.stage_size;
	ip->cl_allocsz = irec->sr_attr.alloc_size;
	if (ip->di.status.b.direct_map) {
		/*
		 * clear since we can't ever extend it
		 */
		ip->cl_alloc_unit = 0;
	}
	if (ip->di.status.b.offline) {
		ip->stage_err = irec->sr_attr.stage_err;
	} else {
		ip->stage_err = 0;
	}

	/*
	 * If file changed since we last updated this inode, and this is not a
	 * lease command and we don't hold any SAM_DATA_MODIFYING_LEASES, then
	 * stale indirect blocks & pages. The server controls this explicitly
	 * for lease commands.
	 */
	if ((cmd != SAM_CMD_LEASE) &&
	    !(ip->cl_leases & SAM_DATA_MODIFYING_LEASES)) {
		if ((ip->di.modify_time.tv_sec != modify_time.tv_sec) ||
		    (ip->di.modify_time.tv_nsec != modify_time.tv_nsec) ||
		    (residence_time != ip->di.residence_time) ||
		    (real_size != ip->di.rm.size)) {
			irec->sr_attr.actions |=
			    (SR_INVAL_PAGES | SR_STALE_INDIRECT);
			irec->sr_attr.offset = 0;
			if (irec->sr_attr.size_owner !=
			    ip->mp->ms.m_client_ord) {
				sam_set_size(ip);
			}
		}
	}
	sam_clear_map_cache(ip);

	return (error);
}


/*
 * ----- sam_complete_msg -
 *	The server has processed this msg. Wake up the thread waiting on
 *  this message. Flag the completed request so the appropriate thread can
 *  complete.
 */

void
sam_complete_msg(
	sam_mount_t *mp,		/* Pointer to mount table */
	sam_san_message_t *msg)		/* Pointer to message */
{
	sam_client_msg_t *clp;

	if (msg && msg->hdr.wait_flag > SHARE_wait) {
		/* Nobody should be waiting for this message. */
		return;
	}

	mutex_enter(&mp->ms.m_cl_mutex);
	clp = mp->ms.m_cl_head;

	/*
	 * Find this message entry in the client request chain.
	 */
	while (clp != NULL) {
		if (clp->cl_seqno == msg->hdr.seqno) {
			break;
		}
		clp = clp->cl_next;
	}
	if (clp) {
		mutex_enter(&clp->cl_msg_mutex);
		clp->cl_error = msg->hdr.error;
		/*
		 * For mount, lease, name, and inode commands copy the results
		 * back to the original message.
		 */
		ASSERT(msg->hdr.command & 0xff);
		if ((msg->hdr.command == SAM_CMD_MOUNT) ||
		    (msg->hdr.command == SAM_CMD_LEASE) ||
		    (msg->hdr.command == SAM_CMD_NAME) ||
		    (msg->hdr.command == SAM_CMD_INODE)) {
			bcopy(msg, clp->cl_msg,
			    SAM_HDR_LENGTH + msg->hdr.out_length);
		}
		clp->cl_done = TRUE;
		mutex_exit(&clp->cl_msg_mutex);
		if (mp->ms.m_cl_server_wait) {
			cv_broadcast(&mp->ms.m_cl_cv);
		}
		mutex_exit(&mp->ms.m_cl_mutex);
	} else {
		/*
		 * The waiting thread was interrupted. If a lease message
		 * completed successfully, we must remove it on the server.
		 */
		mutex_exit(&mp->ms.m_cl_mutex);

#if defined(SAM_REVOKE_LEASE)
/*
 * XXX - Can get here as a result of a retransmission, which means
 *		 revoking a lease is the wrong thing to do.
 *
 *		 This was for an NFS/Lockmanager problem where a process
 *		 blocked waiting for a file lock was interrupted.
 */
		if ((msg->hdr.command == SAM_CMD_LEASE) && !msg->hdr.error) {
			sam_revoke_acquired_lease(mp, msg);
		}
#endif /* SAM_REVOKE_LEASE */
	}
}


/*
 * ----- sam_restart_waiter -
 *	Wake up the thread waiting on a server notification for the given
 *	inode..
 */

void
sam_restart_waiter(
	sam_mount_t *mp,		/* Pointer to mount table */
	sam_id_t *id)			/* Pointer to id (ino.gen) */
{
	sam_client_msg_t *clp;
	boolean_t found_waiter = FALSE;

	mutex_enter(&mp->ms.m_cl_mutex);
	clp = mp->ms.m_cl_head;
	while (clp != NULL) {
		if (clp->cl_command == SAM_CMD_WAIT && !clp->cl_done) {
			sam_san_wait_msg_t *wait_msg;

			wait_msg = (sam_san_wait_msg_t *)clp->cl_msg;
			if ((wait_msg->call.wait.id.ino == id->ino) &&
			    (wait_msg->call.wait.id.gen == id->gen)) {
				/*
				 * Reissue lease command
				 */
				clp->cl_error = ETIME;
				clp->cl_done = TRUE;
				found_waiter = TRUE;
			}
		}
		clp = clp->cl_next;
	}
	if (found_waiter && mp->ms.m_cl_server_wait) {
		cv_broadcast(&mp->ms.m_cl_cv);
	}
	mutex_exit(&mp->ms.m_cl_mutex);
}


typedef struct sam_schedule_frlock_entry {
	struct sam_schedule_entry	schedule_entry;
	sam_san_lease_t				lease_message;
} sam_schedule_frlock_entry_t;

#if defined(SAM_REVOKE_LEASE)
/*
 * ----- sam_revoke_acquired_lease -
 *  A lease message completed successfully, however there is no waiting
 *  thread. This means the user interrupted the process. The completed
 *  lease must be removed on the server.
 */

static void
sam_revoke_acquired_lease(
	sam_mount_t *mp,		/* Pointer to mount table */
	sam_san_message_t *msg)		/* Pointer to message */
{
	sam_node_t *ip = NULL;
	sam_san_lease_t *lp = &msg->call.lease;
	ushort_t op = msg->hdr.operation;
	int error;

	if (op != LEASE_get) {
		return;
	}
	error = sam_check_cache(&lp->id, mp, IG_EXISTS, NULL, &ip);
	if ((error == 0) && ip) {
		/*
		 * Remove the lease on the server that was just acquired.
		 */
		if (lp->data.ltype == LTYPE_frlock) {
			sam_lease_data_t *dp = &lp->data;
			sam_share_flock_t *flp = &lp->flock;

			/*
			 * Release the lock that was just acquired.
			 * XXX - This is wrong in the case of a lock which was
			 * re-acquiring or upgrading an existing lock (e.g. a
			 * read lock was held on a byte range, and a new read
			 * lock or a write lock was acquired).  We will release
			 * the lock on the range instead of restoring it to its
			 * previous state.
			 */
			if ((dp->cmd == 0) || (dp->cmd == F_SETLK) ||
			    (dp->cmd == F_SETLKW)) {
				flp->l_type = F_UNLCK;
				sam_start_frlock_task(ip, msg);
			}
		} else {
			uint32_t lease_mask;

			lease_mask = (1 << lp->data.ltype);
			if (!(ip->cl_leases & lease_mask)) {
				sam_start_relinquish_task(ip, lease_mask,
				    lp->gen);
			}
		}
		VN_RELE_OS(ip);
	}
}


/*
 * ----- sam_start_frlock_task - start a task to remove frlocks.
 * The task waits for the response and then terminates.
 */

static void
sam_start_frlock_task(
	sam_node_t *ip,
	sam_san_message_t *msg)	/* Pointer to message */
{
	sam_mount_t *mp = ip->mp;
	sam_schedule_frlock_entry_t *ep;

	ep = (sam_schedule_frlock_entry_t *)kmem_alloc(
	    sizeof (sam_schedule_frlock_entry_t), KM_SLEEP);
	sam_init_schedule_entry(&ep->schedule_entry, mp, sam_remove_frlock,
	    (void *)ip);
	ep->lease_message = msg->call.lease;
	VN_HOLD_OS(ip);

	if (sam_taskq_dispatch((sam_schedule_entry_t *)ep) == 0) {
		VN_RELE_OS(ip);
		kmem_free(ep, sizeof (sam_schedule_frlock_entry_t));
		cmn_err(CE_WARN,
		"SAM-QFS: %s: can't start task to remove frlocks: "
		"ino=%d.%d size=%llx",
		    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
		    ip->di.rm.size);
	}
}
#endif /* SAM_REVOKE_LEASE */


/*
 * ----- sam_remove_frlock - remove just acquired frlock.
 */

static void
__sam_remove_frlock(sam_schedule_entry_t *entry)
{
	struct sam_schedule_frlock_entry *ep;
	sam_mount_t *mp;
	sam_node_t *ip;
	int error;

	mp = entry->mp;
	ip = (sam_node_t *)entry->arg;
	ep = (sam_schedule_frlock_entry_t *)entry;

	sam_open_operation_nb(mp);

	error = sam_proc_get_lease(ip, &ep->lease_message.data,
	    &ep->lease_message.flock, NULL, SHARE_wait, CRED());

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	if (error == 0) {
		sam_update_frlock(ip, ep->lease_message.data.cmd,
		    &ep->lease_message.flock, ep->lease_message.data.filemode,
		    ep->lease_message.data.offset);
		/*
		 * If there are no locks remaining, remove any potential
		 * FRLOCK lease.
		 *
		 * XXX - Eventually we should quit treating FRLOCK as a lease
		 * XXX - at all, and track locks independently of leases.
		 */
		if (ip->cl_locks == 0) {
			sam_client_remove_leases(ip, CL_FRLOCK, 0);
		}

	} else {
		dcmn_err((CE_NOTE,
		    "SAM-QFS: %s: remove frlock error=%d: ino=%d.%d "
		    "pid=%d, size=%llx",
		    mp->mt.fi_name, error, ip->di.id.ino, ip->di.id.gen,
		    ep->lease_message.flock.l_pid, ip->di.rm.size));
	}

	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	SAM_CLOSE_OPERATION(mp, error);
	VN_RELE_OS(ip);
	kmem_free(entry, sizeof (sam_schedule_frlock_entry_t));
	sam_taskq_uncount(mp);
}


#ifdef sun
void
sam_remove_frlock(sam_schedule_entry_t *entry)
{
	__sam_remove_frlock(entry);
}
#endif /* sun */

#ifdef linux
int
sam_remove_frlock(void *arg)
{
	sam_schedule_entry_t *entry;

	entry = task_begin(arg);
	__sam_remove_frlock(entry);
	return (0);
}
#endif /* linux */


/*
 * ----- sam_process_callout_request -
 * Process server callout request.
 */

static int				/* ERRNO, else 0 if successful. */
sam_process_callout_request(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_callout_t *cop = &msg->call.callout;
	sam_callout_arg_t *arg = &cop->arg;
	sam_node_t *ip = NULL;
	int error = 0;

	/*
	 * All callouts require an inode. Refresh the inode info.
	 */

	if (cop->id.ino == SAM_INO_INO) {
		ip = mp->mi.m_inodir;
	} else {
		error = sam_check_cache(&cop->id, mp, IG_EXISTS, NULL, &ip);
	}
	if (error == 0 && ip != NULL) {

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		/*
		 * Never reset inode info on server for a callout.
		 */
		if (!SAM_IS_SHARED_SERVER(ip->mp)) {
			error = sam_reset_client_ino(ip, 0, &cop->irec);
		}
		sam_directed_actions(ip, cop->irec.sr_attr.actions,
		    cop->irec.sr_attr.offset, CRED());

		switch (msg->hdr.operation) {

		case CALLOUT_action:
			break;

		case CALLOUT_stage:
			ip->copy = arg->p.stage.copy;
			ip->stage_err =
			    solaris_to_local_errno(arg->p.stage.error);
			if (!(arg->p.stage.flags & SAM_STAGING)) {
				ip->flags.b.staging = 0;
			}
			mutex_enter(&ip->rm_mutex);
			if (ip->rm_wait) {
				cv_broadcast(&ip->rm_cv);
			}
			mutex_exit(&ip->rm_mutex);
			break;

		case CALLOUT_acl:
#ifdef	sun
			if (ip->di.status.b.acl) {
				(void) sam_acl_inactive(ip);
			}
#endif	/* sun */
			break;

		case CALLOUT_flags:
			error = sam_proc_block(mp, ip,
			    BLOCK_wakeup, SHARE_nowait, NULL);
			break;

		case CALLOUT_relinquish_lease: {
			uint16_t lease_mask;
			int timeo;
			int ltype;

			lease_mask = arg->p.relinquish_lease.lease_mask;
			timeo = arg->p.relinquish_lease.timeo;
			mutex_enter(&ip->ilease_mutex);
			for (ltype = 0; ltype < MAX_EXPIRING_LEASES; ltype++) {
				int64_t new_leasetime;

				if (lease_mask & (1 << ltype)) {
					/*
					 * Calculate a new lease expiration
					 * time. If the new time is earlier
					 * than the current expiration
					 * set it in leasetime.
					 */
					new_leasetime = ddi_get_lbolt() + (hz * timeo);
					if (new_leasetime <
					    ip->cl_leasetime[ltype]) {
						ip->cl_leasetime[ltype] =
						    new_leasetime;
					}
				}
			}
			ip->cl_short_leases |= (lease_mask & ip->cl_leases);
			mutex_exit(&ip->ilease_mutex);
			sam_sched_expire_client_leases(mp, (hz * timeo), TRUE);
			}
			break;

		default:
			break;
		}

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (cop->id.ino != SAM_INO_INO) {
			VN_RELE_OS(ip);
		}
	}
	return (error);
}


/*
 * ----- sam_process_notify_request -
 * Process server notify request.
 */

static void
sam_process_notify_request(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_notify_t *nop = &msg->call.notify;
	sam_notify_arg_t *arg = &nop->arg;
	sam_node_t *ip = NULL;
	int error = 0;

	/*
	 * To avoid a deadlock when receiving a lease expiration notice
	 * and holding the inode lock across an OTW transaction (as is
	 * currently done for truncate), we don't update the inode when
	 * we receive these messages.
	 *
	 * In fact, lease availability callouts currently don't use the inode
	 * at all, though the server does set it.  This is important since we
	 * can't even locate an inode in the cache if the inode's rwlock
	 * is held.
	 */

	/*
	 * These notify operations don't require an inode.
	 */
	switch (msg->hdr.operation) {

	case NOTIFY_lease:
		sam_restart_waiter(mp, &nop->id);
		return;

	case NOTIFY_hostoff:
		/* Ask sharefsd to wake up, unmount, and quit */
		cmn_err(CE_WARN, "SAM-QFS: fs %s NOTIFY_hostoff received",
		    mp->mt.fi_name);
		sam_wake_sharedaemon(mp, EIO);
		return;
	}

	/*
	 * All other notify operations require an inode.
	 */
	if (nop->id.ino == SAM_INO_INO) {
		ip = mp->mi.m_inodir;
	} else {
		error = sam_check_cache(&nop->id, mp, IG_EXISTS, NULL, &ip);
		if ((error != 0) || (ip == NULL)) {
			SAM_COUNT64(shared_client, notify_ino);
			return;
		}
	}

	if (error == 0 && ip != NULL) {
		switch (msg->hdr.operation) {

		case NOTIFY_lease:
		case NOTIFY_hostoff:
			/* These cases handled above */
			break;

		case NOTIFY_lease_expire:
			sam_client_remove_leases(ip,
			    arg->p.lease.lease_mask, 1);
			break;

		case NOTIFY_dnlc:
			/*
			 * Remove from dnlc cache if there.
			 */
			samqfs_notify_dnlc_remove(ip, arg->p.dnlc.component);
			break;

		case NOTIFY_getino:
			(void) sam_update_shared_ino(ip, SAM_SYNC_ALL, FALSE);
			break;

#ifdef DEBUG
		case NOTIFY_panic:
			cmn_err(CE_PANIC,
			    "SAM-QFS: %s: CLIENT %d panic, status=%x",
			    mp->mt.fi_name, msg->hdr.client_ord,
			    mp->mt.fi_status);
			break;
#endif /* DEBUG */

		default:
			break;
		}
	}

	if (nop->id.ino != SAM_INO_INO) {
		VN_RELE_OS(ip);
	}
}


/*
 * ------ samqfs_notify_dnlc_remove -
 * Look for a dnlc/dcache entry of pip/comp
 * and delete it from the dnlc/dcache.
 */
void
samqfs_notify_dnlc_remove(
	sam_node_t *pip,	/* parent inode */
	char *comp)		/* name to be deleted from the dcache */
{
#ifdef sun
	(void) dnlc_remove(SAM_ITOV(pip), comp);
#endif /* sun */

#ifdef linux
	struct inode *pli;
	int length;
	struct dentry *dir, *de;
	struct qstr qstr;

	if (pip == NULL) {
		return;
	}
	if (comp == NULL) {
		return;
	}

	pli = SAM_SITOLI(pip);
	length = strlen(comp);
	if ((length == 0) || (length > NAME_MAX)) {
		return;
	}

	/*
	 * Find the dcache entry of the parent.
	 */
	dir = d_find_alias(pli);

	if (dir) {
		/*
		 * Find the dcache entry of the component.
		 */

		qstr.name = comp;
		qstr.len = length;
		qstr.hash = rfs_full_name_hash(comp, length);

		de = d_lookup(dir, &qstr);
		if (de) {
			rfs_d_drop(de);
			dput(de);
		}
		dput(dir);
	}
#endif /* linux */
}
