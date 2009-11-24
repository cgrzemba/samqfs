/*
 * -----	sam/server.c - Process socket messages from the clients.
 *
 *	sam_client_cmd - Called when a socket message is read over the wire
 *		by sam_get_sock_msg. sam_write_to_client is called to return the
 *		response back to the client.
 *
 *	sam_proc_callout - Called when the metadata server wants to callout
 *		to the clients.
 *
 *	sam_proc_notify - Called when the metadata server wants to notify
 *		clients.
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

#pragma ident "$Revision: 1.309 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/flock.h>
#include <sys/flock_impl.h>
#include <sys/fbuf.h>
#include <sys/fs_subr.h>
#include <vm/seg_map.h>
#include <nfs/lm.h>
#include <sys/param.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/syscall.h"
#include "sam/samevent.h"

#include "samfs.h"
#include "ino.h"
#include "inode.h"
#include "mount.h"
#include "quota.h"
#include "arfind.h"
#include "extern.h"
#include "trace.h"
#include "debug.h"
#include "kstats.h"
#include "sam_interrno.h"
#include "sam_list.h"
#include "qfs_log.h"
#include "behavior_tags.h"

static void sam_process_message(sam_mount_t *mp, sam_san_message_t *msg,
	sam_id_t *id);
static int sam_get_mount_request(sam_mount_t *mp, sam_san_message_t *msg);
static int sam_get_lease_request(sam_mount_t *mp, sam_san_message_t *msg);
static int sam_process_frlock_request(sam_node_t *ip, int client_ord,
	sam_san_lease2_t *l2p, boolean_t is_get);
static int sam_blkd_frlock_start(sam_node_t *ip, int client_ord,
	int filemode, int64_t offset, flock64_t *flock);
static void sam_blkd_frlock_task(void *arg);
static callb_cpr_t *sam_blkd_frlock_callback(flk_cb_when_t when, void *arg);
static void sam_blkd_frlock_cancel(void *arg);
static sam_blkd_frlock_t *sam_blkd_frlock_new(sam_node_t *ip, int client_ord,
	int filemode, int64_t offset, flock64_t *flock);
static void sam_blkd_frlock_free(sam_blkd_frlock_t *blkd_frlock);
static sam_blkd_frlock_t *sam_blkd_frlock_find(sam_blkd_frlock_t *blkd_frlock);
static void sam_blkd_frlock_insert(sam_blkd_frlock_t *blkd_frlock);
static void sam_blkd_frlock_remove(sam_blkd_frlock_t *blkd_frlock);
static int sam_process_get_lease(sam_node_t *ip, sam_san_message_t *msg);
static int sam_process_rm_lease(sam_node_t *ip, sam_san_message_t *msg);
static int sam_process_reset_lease(sam_node_t *ip, sam_san_message_t *msg);
static int sam_process_relinquish_lease(sam_node_t *ip, sam_san_message_t *msg);
static int sam_process_extend_lease(sam_node_t *ip, sam_san_message_t *msg);
static int sam_record_lease(sam_node_t *ip, int client_ord,
	enum LEASE_type ltype, sam_san_lease2_t *l2p, boolean_t new_generation,
	boolean_t is_get);
static int sam_new_ino_lease(sam_node_t *ip, int client_ord,
	enum LEASE_type ltype, uint32_t interval, sam_san_lease2_t *l2p,
	sam_client_lease_t **out_clp, boolean_t mutex_held);
static int sam_add_ino_lease(sam_node_t *ip, int client_ord,
	enum LEASE_type ltype, uint32_t interval, sam_san_lease2_t *l2p,
	sam_client_lease_t **out_clp, boolean_t mutex_held);
static void sam_callout_actions(sam_node_t *ip, int client_ord,
	ushort_t actions, sam_lease_ino_t *llp);
static boolean_t sam_callout_directio(sam_node_t *ip, int client_ord,
	sam_lease_ino_t *llp);
static int sam_remove_lease(sam_node_t *ip, int client_ord, sam_sr_attr_t *ap,
	int lease_mask, uint32_t *gen_table);
static int sam_extend_lease(sam_node_t *ip, int client_ord, int lease_mask,
	sam_san_lease2_t *l2p);
static int sam_client_allocate_append(sam_node_t *ip, sam_san_message_t *msg);
static int sam_get_name_request(sam_mount_t *mp, sam_san_message_t *msg,
	sam_id_t *id);
static int sam_process_name_request(sam_node_t *pip, sam_san_message_t *msg);
static int sam_get_inode_request(sam_mount_t *mp, sam_san_message_t *msg);
static int sam_server_truncate(sam_node_t *ip, sam_san_lease2_t *l2p,
	cred_t *credp);
static void sam_v32_to_v(sam_vattr_t *va32p, vattr_t *vap);
static int sam_get_block_request(sam_mount_t *mp, sam_san_message_t *msg);
static int sam_update_inode_buffer_rw(sam_node_t *ip);
static void sam_set_sr_ino(sam_node_t *ip, sam_id_t *id,
	sam_ino_record_t *irec);
static void sam_set_sr_attr(sam_node_t *ip, ushort_t actions,
	sam_ino_record_t *irec);
static void sam_update_cl_attr(sam_node_t *ip, int client_ord,
	sam_cl_attr_t *attr);
static cred_t *sam_getcred(sam_cred_t *sam_credp);
static sam_msg_array_t *sam_get_msg_entry(sam_mount_t *mp, client_entry_t *clp);
static void sam_free_msg_entry(sam_mount_t *mp, client_entry_t *clp,
	sam_msg_array_t *mep);

extern struct vnodeops samfs_vnodeops;

/*
 * Tunable variables.
 *
 *   sam_lease_server_mul and sam_lease_server_add are used to adjust the
 *   time that the server will wait for a client to expire its lease.  The
 *   formula used is:
 *
 *   server time = (client time) * sam_lease_server_mul + sam_lease_server_add
 *
 *   The adjustments are bounded by sam_lease_server_min and
 *   sam_lease_server_max, if set, to lie within the interval:
 *
 *   client time + sam_lease_server_min .. client time + sam_lease_server_max
 *
 *   All of the above (except mul) are currently given in seconds.
 *
 *   A client lease will expire in lease time plus 10 minutes on the server.
 *   e.g. A 30 second client lease will expire in 630 seconds on the server.
 *
 *   sam_client_lease_increment controls how many extra lease entries will be
 *   allocated to a file when its lease table needs to be expanded.  We always
 *   allocate 1 initially, since files are infrequently shared.  We grow the
 *   table as needed.  A larger increment could improve performance if many
 *   files are shared among many clients, at the expense of memory utilization
 *   for files which are shared by fewer clients.
 */

uint16_t sam_lease_server_mul = 2;
uint16_t sam_lease_server_add = 0;
uint16_t sam_lease_server_min = 600;
uint16_t sam_lease_server_max = USHRT_MAX;
uint16_t sam_client_lease_increment = 1;

/*
 * ----- sam_calculate_lease_timeout - Calculate the amount of time before a
 * lease is considered to have timed out without a client expiring it.  See
 * the description of tunables above.
 */

int
sam_calculate_lease_timeout(int client_interval)
{
	int server_interval, extra;

	server_interval = (client_interval * sam_lease_server_mul) +
	    sam_lease_server_add;

	extra = server_interval - client_interval;

	if (extra > sam_lease_server_max) {
		extra = sam_lease_server_max;
	}

	if (extra < sam_lease_server_min) {
		extra = sam_lease_server_min;
	}

	return (client_interval + extra);
}


/*
 * ----- sam_server_cmd - Process the server command.
 *	Called when the fs server reads a command over the socket
 *	from the client.
 */

void
sam_server_cmd(sam_mount_t *mp, mblk_t *mbp)
{
	sam_san_message_t *msg;
	client_entry_t *clp = NULL;
	sam_msg_array_t *mep = NULL;
	sam_msg_array_t *tmep;
	sam_msg_array_t *pmep;
	uint32_t ack;
	boolean_t new_ack;

	msg = (sam_san_message_t *)(void *)mbp->b_rptr;

	/*
	 * Process server daemon commands. Check for valid client ordinal.
	 * If not the server or frozen, throw message away.
	 * Check for mounted & unchanged for all commands except SAM_CMD_MOUNT.
	 */
	if (msg->hdr.client_ord <= 0 || msg->hdr.client_ord > mp->ms.m_maxord) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: server recv invalid ord %d ([1..%d])"
		    " cmd=%x seqno=%d ack=%d",
		    mp->mt.fi_name, msg->hdr.client_ord, mp->ms.m_maxord,
		    (msg->hdr.command<<16)|msg->hdr.operation,
		    msg->hdr.seqno, msg->hdr.ack);
		goto fini;		/* Client ordinal out of range */
	}
	if (!SAM_IS_SHARED_SERVER(mp)) {
		goto fini;	/* If not server, client must resend */
	}
	if (mp->mt.fi_status & FS_FROZEN) {
		goto fini; /* If failing over & frozen, client must resend */
	}
	ASSERT(msg->hdr.command & 0xff);
	if (msg->hdr.command != SAM_CMD_MOUNT) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if ((!(mp->mt.fi_status & FS_MOUNTED)) ||
		    (mp->mt.fi_status &
		    (FS_MOUNTING | FS_UMOUNT_IN_PROGRESS))) {
			msg->hdr.error = EXDEV;
			mutex_exit(&mp->ms.m_waitwr_mutex);
			goto done;
		} else if (!((msg->hdr.command == SAM_CMD_BLOCK) &&
		    (msg->call.block.id.ino == 0)) &&
		    (mp->mi.m_sblk_fsid != msg->hdr.fsid)) {
			msg->hdr.error = EBADR;
			mutex_exit(&mp->ms.m_waitwr_mutex);
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Stale file system"
			    " SRV INIT=%x CLI INIT=%x, cmd=%x, seqno=%x",
			    mp->mt.fi_name, mp->mi.m_sbp->info.sb.init,
			    msg->hdr.fsid,
			    (msg->hdr.command<<16)|msg->hdr.operation,
			    msg->hdr.seqno);
			goto done;
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
	}

	/*
	 * Resync state allows clients to connect and reestablish leases.  While
	 * resyncing, let client MOUNT, BLOCK with no inode, and LEASE_reset
	 * messages through.
	 */
	if (mp->mt.fi_status & FS_RESYNCING) {
		if ((msg->hdr.command != SAM_CMD_MOUNT) &&
		    !((msg->hdr.command == SAM_CMD_BLOCK) &&
		    (msg->call.block.id.ino == 0)) &&
		    !((msg->hdr.command == SAM_CMD_LEASE) &&
		    (msg->hdr.operation == LEASE_reset))) {
			goto fini;
		}
	}

	/*
	 * Validate client and check to see if it is marked down.
	 * Verify fs generation number is correct if client
	 * was previously not responding.
	 */
	clp = sam_get_client_entry(mp, msg->hdr.client_ord, 0);
	if (clp == NULL) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Message received from unknown "
		    "client ord %d", mp->mt.fi_name, msg->hdr.client_ord);
		goto fini;
	}
	if (clp->cl_flags & SAM_CLIENT_OFF) {
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Message received from down "
		    "client ord %d", mp->mt.fi_name, msg->hdr.client_ord);
		/* Drop message */
		goto fini;
	}
	if ((msg->hdr.command != SAM_CMD_MOUNT) &&
	    !((msg->hdr.command == SAM_CMD_BLOCK) &&
	    (msg->call.block.id.ino == 0))) {
		if ((clp->cl_flags & SAM_CLIENT_NOT_RESP) &&
		    (mp->mi.m_sblk_fsgen != msg->hdr.fsgen)) {
			msg->hdr.error = EBADR;
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Stale file system"
			    " SRV GEN=%x CLI GEN=%x, cmd=%x, seqno=%x",
			    mp->mt.fi_name, mp->mi.m_sbp->info.sb.fsgen,
			    msg->hdr.fsgen,
			    (msg->hdr.command<<16)|msg->hdr.operation,
			    msg->hdr.seqno);
			goto done;
		}
		clp->cl_flags &= ~SAM_CLIENT_NOT_RESP;
	}

	/*
	 * Check if the client has already processed this message. The
	 * returned ack tells us the client has received & processed
	 * all the messages < ack. If so, remove the processed messages
	 * out of the outstanding message array for this client.
	 * Note, a client may send an ACK of 0 to indicate it is not
	 * acknowledging any messages with this transmission.
	 *
	 * Set clp->cl_rcv_time only for LEASE_ commands.  We don't
	 * care about these except when we're resyncing.  At that
	 * time, if a client is sending BLOCK_ commands, we DON'T
	 * want the cl_rcv_time updated, because we'll assume that
	 * the client is busy resyncing.
	 *
	 * XXX This logic should be fixed up so that we don't hold
	 * XXX up for a client that is mounting rather than failing
	 * XXX over.
	 */
	mutex_enter(&clp->cl_msgmutex);
	clp->cl_msg_time = lbolt;
	clp->cl_lastmsg = SAM_SECOND();
	if (clp->cl_flags & SAM_CLIENT_SOCK_BLOCKED) {
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Message received from blocked "
		    "client %s, resume sending to this client",
		    mp->mt.fi_name, clp->hname);
		clp->cl_flags &= ~SAM_CLIENT_SOCK_BLOCKED;
	}
	if (msg->hdr.command == SAM_CMD_LEASE) {
		clp->cl_rcv_time = lbolt;
	}
	ack = msg->hdr.ack;

	/*
	 * The client sets a header flag on MOUNT_init and MOUNT_failinit
	 * messages to notify the server to reset its sequence number for
	 * this file system. All messages in the message array which have
	 * been processed are freed because this mount point is reset.
	 */
	new_ack = FALSE;
	if (msg->hdr.reset_seqno) {
		mutex_enter(&mp->ms.m_cl_wrmutex);
		clp->cl_min_seqno = ack;
		new_ack = TRUE;
		for (tmep = list_head(&clp->queue.list); tmep != NULL;
		    tmep = pmep) {
			pmep = list_next(&clp->queue.list, tmep);
			if (tmep->active == SAM_WAITFOR_ACK) {
				sam_free_msg_entry(mp, clp, tmep);
			}
		}
		mutex_exit(&mp->ms.m_cl_wrmutex);

	} else if ((ack != 0) && (SAM_SEQUENCE_LATER(ack, clp->cl_min_seqno))) {
		clp->cl_min_seqno = ack;
		new_ack = TRUE;
	}

	mutex_enter(&mp->ms.m_cl_wrmutex);
	for (tmep = list_head(&clp->queue.list); tmep != NULL; tmep = pmep) {
		pmep = list_next(&clp->queue.list, tmep);
		if (msg->hdr.seqno == tmep->seqno) {
			/* If this msg is a duplicate */
			mep = tmep;
			SAM_COUNT64(shared_server, msg_dup);
			continue;
		}
		/*
		 * For a non-idempotent command which has been processed, check
		 * if this message is acknowledging it. If so, remove it from
		 * list of unacknowledged messages.
		 */
		if (new_ack && tmep->active == SAM_WAITFOR_ACK) {
			if (SAM_SEQUENCE_LATER(ack, tmep->seqno)) {
				sam_free_msg_entry(mp, clp, tmep);
			}
		}
	}
	mutex_exit(&mp->ms.m_cl_wrmutex);

	/*
	 * The client can time out and resend the same message. The
	 * server must detect that the same message has been sent and
	 * has possibly already been processed.
	 *
	 * If the message has already been acknowledged, the server can
	 * just ignore it.  cl_min_seqno is the minimum sequence number
	 * that a new message must have; all previous message responses
	 * have already been acknowledged by the client.
	 *
	 * However, messages with wait_flag > SHARE_wait do not participate
	 * in the ACK protocol so we need to be careful not to drop them in
	 * case sequence numbers arrive out of order.
	 */
	if (msg->hdr.wait_flag <= SHARE_wait) {
		if (SAM_SEQUENCE_LATER(clp->cl_min_seqno, msg->hdr.seqno)) {
			mutex_exit(&clp->cl_msgmutex);
			goto fini;
		}
	}

	/*
	 * The server maintains a list of all outstanding messages and
	 * all processed non-idempotent messages.  If the incoming message is a
	 * duplicate and it has already been processed, return completed
	 * status. If it is currently active throw the incoming message
	 * away to avoid duplicate message processing.
	 */
	if (mep) {			/* If this message is in the queue */
		mutex_enter(&mp->ms.m_cl_wrmutex);

		if (mep->active == SAM_WAITFOR_ACK) {
			/* If already processed, resend reply */
			mep->active = SAM_PROCESSING;
			msg->hdr.error = mep->error;
			ASSERT((msg->hdr.command == SAM_CMD_NAME) ||
			    (msg->hdr.command == SAM_CMD_LEASE));
			if (msg->hdr.command == SAM_CMD_NAME) {
				msg->call.name2.new_id = mep->id;
				if (mep->id.ino) {
					msg->call.name2.nrec = mep->irec;
				}
				mutex_exit(&mp->ms.m_cl_wrmutex);
				mutex_exit(&clp->cl_msgmutex);
				if (msg->hdr.error == 0) {
					/*
					 * Process message only to get parent
					 * info. Note, the new id info was saved
					 * from the previous execution.
					 */
					sam_process_message(mp, msg,
					    &msg->call.name2.new_id);
				}
			} else {
				int i;

				msg->call.lease2.irec = mep->irec;
				msg->call.lease2.granted_mask =
				    mep->granted_mask;
				for (i = 0; i < SAM_MAX_LTYPE; i++) {
					msg->call.lease.gen[i] = mep->gen[i];
				}
				/*
				 * Return changed fields for the lease command
				 * which was executed previously.
				 */
				mutex_exit(&mp->ms.m_cl_wrmutex);
				mutex_exit(&clp->cl_msgmutex);
			}
			goto done;

		} else {	/* If currently active, ignore this message */
			mutex_exit(&mp->ms.m_cl_wrmutex);
			mutex_exit(&clp->cl_msgmutex);
			goto fini;
		}
	}

	mutex_enter(&mp->ms.m_cl_wrmutex);

	mep = sam_get_msg_entry(mp, clp);
	if (mep == NULL) {
		mutex_exit(&mp->ms.m_cl_wrmutex);
		mutex_exit(&clp->cl_msgmutex);
		goto fini;
	}
	mep->active = SAM_PROCESSING;
	mep->command = msg->hdr.command;
	mep->operation = msg->hdr.operation;
	mep->seqno = msg->hdr.seqno;
	mep->client_ord = msg->hdr.client_ord;
	mep->error = 0;
	mep->id.ino = 0;
	mep->id.gen = 0;
	mutex_exit(&mp->ms.m_cl_wrmutex);

	mutex_exit(&clp->cl_msgmutex);

	sam_process_message(mp, msg, NULL);

	/*
	 * Write message response back to the client. Throw away all idempotent
	 * messages in the active message list.  We must keep non-idempotent
	 * messages (mostly name messages, but also leases due to the generation
	 * number increment) to avoid executing the command twice.
	 *
	 * imsg is not set if the message has not been added to the message
	 * array.  This occurs in some early-exit error conditions.
	 */
done:
	msg->hdr.length = msg->hdr.out_length;
	clp = sam_get_client_entry(mp, msg->hdr.client_ord, 0);
	ASSERT(clp != NULL);
	mutex_enter(&clp->cl_msgmutex);

	if (mep) {
		boolean_t keep = FALSE;
		int i;

		mutex_enter(&mp->ms.m_cl_wrmutex);

		ASSERT(msg->hdr.seqno == mep->seqno);
		if ((msg->hdr.command == SAM_CMD_LEASE) &&
		    (msg->hdr.wait_flag == SHARE_wait) &&
		    ((msg->hdr.operation == LEASE_get) ||
		    (msg->hdr.operation == LEASE_relinquish))) {
			mep->id = msg->call.lease2.inp.id;
			mep->irec = msg->call.lease2.irec;
			mep->granted_mask = msg->call.lease2.granted_mask;
			for (i = 0; i < SAM_MAX_LTYPE; i++) {
				mep->gen[i] = msg->call.lease.gen[i];
			}
			keep = TRUE;
		} else if ((msg->hdr.command == SAM_CMD_NAME) &&
		    (msg->hdr.operation < NAME_lookup)) {
			mep->id = msg->call.name2.new_id;
			if (mep->id.ino) {
				mep->irec = msg->call.name2.nrec;
			}
			keep = TRUE;
		}
		if (keep) {	/* non-idempotent, must save until ACKed */
			mep->active = SAM_WAITFOR_ACK;
			mep->error = msg->hdr.error;
		} else {
			sam_free_msg_entry(mp, clp, mep);
		}
		mutex_exit(&mp->ms.m_cl_wrmutex);
	}
	mutex_exit(&clp->cl_msgmutex);

	/*
	 * If no client waiting on this message, throw it away.
	 * If failing over, ignore allocate ahead. Causes deadlock
	 * on client getting inode rwlock.
	 */
	if (msg->hdr.wait_flag == SHARE_nowait) {
		goto fini;
	}
	if (mp->mt.fi_status & FS_FAILOVER) {
		if (msg->hdr.wait_flag == SHARE_nothr) {
			goto fini; /* If failing over, ignore advise messages */
		}
	}

	if (sam_write_to_client(mp, msg)) {
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: server cannot write to client "
		    "ord %d seqno=%d ack=%d",
		    mp->mt.fi_name, msg->hdr.client_ord,
		    msg->hdr.seqno, msg->hdr.ack);
	}
fini:
	return;
	/* out */
}

/*
 * -----    sam_get_msg_entry - make a message entry if the client hasn't
 *                              already exceeded allotment
 */

static sam_msg_array_t *
sam_get_msg_entry(sam_mount_t *mp, client_entry_t *clp)
{
	sam_msg_array_t *mep;

	ASSERT(MUTEX_HELD(&mp->ms.m_cl_wrmutex));

	mep = kmem_cache_alloc(samgt.msg_array_cache, KM_SLEEP);
	if (mep != NULL) {
		list_enqueue(&clp->queue.list, mep);
		clp->cl_nomsg++;
	} else {
		cmn_err(CE_WARN, "SAM-QFS: Server out of memory, "
		    "couldn't allocate message array entry on %s for client %s",
		    mp->mt.fi_name, clp->hname);
	}
	return (mep);
}

/*
 * -----	sam_free_msg_entry - free a message entry.
 */

static void
sam_free_msg_entry(
	sam_mount_t *mp,	/* Pointer to mount table */
	client_entry_t *clp,	/* Pointer to client entry */
	sam_msg_array_t *mep)	/* Pointer to the message entry */
{
	ASSERT(MUTEX_HELD(&mp->ms.m_cl_wrmutex));

	list_remove(&clp->queue.list, mep);
	kmem_cache_free(samgt.msg_array_cache, mep);
	clp->cl_nomsg--;
}


/*
 * ----- sam_process_message - return info in completed msg.
 *
 *	Called when the shared client issues a msg that has already
 *  processed by the server.
 */

static void
sam_process_message(sam_mount_t *mp, sam_san_message_t *msg, sam_id_t *id)
{
	switch (msg->hdr.command) {

	case SAM_CMD_MOUNT:		/* Add client to list and return ord */
		msg->hdr.error = sam_get_mount_request(mp, msg);
		break;

	case SAM_CMD_LEASE:		/* Return lease for the client inode */
		msg->hdr.error = sam_get_lease_request(mp, msg);
		break;

	case SAM_CMD_NAME:	/* Process name operation for the client */
		msg->hdr.error = sam_get_name_request(mp, msg, id);
		break;

	case SAM_CMD_INODE:	/* Process inode operation for the client */
		msg->hdr.error = sam_get_inode_request(mp, msg);
		break;

	case SAM_CMD_BLOCK:	/* Process block operation for the client */
		msg->hdr.error = sam_get_block_request(mp, msg);
		break;

	default:
		msg->hdr.error = EPROTO;
		break;
	}
}


/*
 * ----- sam_get_mount_request - Add shared client to list.
 *
 * Called when the shared server receives a mount operation.
 * Set the client ordinal in the mount table.
 */

static int				/* Error */
sam_get_mount_request(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_mount_t *cp = &msg->call.mount;
	int ord;
	client_entry_t *clp;
	int error;

	/*
	 * MOUNT_config -- wake up sam-sharefsd daemon.
	 */
	if (msg->hdr.operation == MOUNT_config) {
		sam_wake_sharedaemon(mp, EINTR);
		return (0);
	}

	/*
	 * Client should be set by SC_server_rdsock system call.
	 */
	error = EISAM_MOUNT_OUTSYNC;
	mutex_enter(&mp->ms.m_cl_wrmutex);
	for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
		clp = sam_get_client_entry(mp, ord, 0);
		if (clp == NULL) {
			continue;
		}
		if (strncasecmp(clp->hname, cp->hname,
		    sizeof (cp->hname)) == 0) {
			msg->hdr.client_ord = ord;
			clp->cl_status = cp->status;
			clp->cl_config = cp->config;
			clp->cl_config1 = cp->config1;
			cp->config = mp->mt.fi_config;
			cp->config1 = mp->mt.fi_config1;
			cp->status = mp->mt.fi_status;
			clp->hostid = msg->hdr.hostid;
			clp->cl_resync_time = 0;

			switch (msg->hdr.operation) {
			case MOUNT_init:
				/* Clear SC_DOWN (can't be if we're here) */
				if (clp->cl_flags & SAM_CLIENT_SC_DOWN) {
					clp->cl_flags &= ~SAM_CLIENT_SC_DOWN;
					TRACE(T_SAM_CLNT_SC_DOWN, NULL,
					    ord, 0, clp->cl_flags);
				}
				/* Clear any leases from a previous mount */
				sam_clear_server_leases(mp, ord);
				break;
			case MOUNT_resync:
			case MOUNT_status:
				/*
				 * The client sends the MOUNT_resync to signal
				 * the end of the client thawing phase.
				 */
				clp->cl_resync_time = lbolt;
				break;
			}

			error = 0;
			break;
		}
	}
	mutex_exit(&mp->ms.m_cl_wrmutex);
	if (error == EISAM_MOUNT_OUTSYNC) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: server mount request: Client %s not found",
		    mp->mt.fi_name, cp->hname);
	}
	return (error);
}


/*
 * ----- sam_get_lease_request - syscall server lease command.
 *
 *	Called when the shared client issues a LEASE command.
 *  Note, file size is only updated for LTYPE_append/LTYPE_truncate.
 *  hold_blocks flag is cleared when file at sync so excess allocated
 *  blocks can be freed.
 */

static int		/* ERRNO if an error occurred, 0 if successful. */
sam_get_lease_request(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	sam_node_t *ip;
	int error = 0;
	uint32_t lease_mask;

	if ((error = sam_get_ino(mp->mi.m_vfsp,
	    IG_EXISTS, &lp->id, &ip)) == 0) {

		/*
		 * ltype is actually a lease type for LEASE_get.
		 */
		if (msg->hdr.operation == LEASE_get) {
			lease_mask = 1 << lp->data.ltype;
		} else {
			lease_mask = lp->data.ltype;
		}
		TRACE(T_SAM_SR_LEASE, SAM_ITOV(ip), msg->hdr.client_ord,
		    lease_mask, ip->di.rm.size);

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_update_cl_attr(ip, msg->hdr.client_ord, &lp->cl_attr);

		/*
		 * Process lease message.
		 */
		error = sam_process_lease_request(ip, msg);

		/*
		 * Set all server attributes except the leases.
		 */
		sam_set_sr_attr(ip, l2p->irec.sr_attr.actions, &l2p->irec);
		(void) sam_update_inode_buffer_rw(ip);
		TRACE(T_SAM_SR_LEASE_RET, SAM_ITOV(ip),
		    l2p->irec.sr_attr.current_size,
		    l2p->irec.sr_attr.actions, error);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		VN_RELE(SAM_ITOV(ip));
	}
	return (error);
}


/*
 * ----- sam_process_lease_request - process the lease operations.
 *
 *  The server host directly calls this function.
 *
 *  Note: inode_rwl lock set on entry and exit (as reader or writer).
 *		Must be WRITER lock for all leases other than a read lease.
 */

int			/* ERRNO if an error occurred, 0 if successful. */
sam_process_lease_request(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease2_t *l2p = &msg->call.lease2;
	ushort_t op;
	int error = 0;

	/*
	 * Clear actions and stale indirect offset.
	 */
	l2p->irec.sr_attr.actions = 0;
	l2p->irec.sr_attr.offset = 0;

	op = msg->hdr.operation;
	switch (op) {

	case LEASE_get:
		error = sam_process_get_lease(ip, msg);
		break;

	case LEASE_remove:
		error = sam_process_rm_lease(ip, msg);
		break;

	case LEASE_reset:
		error = sam_process_reset_lease(ip, msg);
		break;

	case LEASE_relinquish:
		if (ip->mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
			error = EBUSY;
		} else {
			error = sam_process_relinquish_lease(ip, msg);
		}
		break;

	case LEASE_extend:
		if (ip->mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
			error = EBUSY;
		} else {
			error = sam_process_extend_lease(ip, msg);
		}
		break;

	default:
		error = EPROTO;
		break;
	}
	return (error);
}

/*
 * ----- sam_process_get_lease - process the lease get operation.
 *
 *	Called when the shared server receives a LEASE get operation or
 *  when the server host directly calls this function.
 *
 *  Note: inode_rwl lock set on entry and exit (as reader or writer).
 *		Must be WRITER lock for all leases other than a read lease.
 */

static int			/* ERRNO, else 0 if successful. */
sam_process_get_lease(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	cred_t *credp = NULL;
	int client_ord;
	enum LEASE_type ltype;
	int error;

	client_ord = msg->hdr.client_ord;

	/*
	 * ltype is actually a lease type for LEASE_get.
	 */
	ltype = lp->data.ltype;
	if ((ltype != LTYPE_read) && (ltype != LTYPE_mmap)) {
		ASSERT(rw_write_held(&ip->inode_rwl) != 0);
	}

	/*
	 * Mandatory lock is not currently supported.
	 * The file/record lock is processed by standard Solaris.
	 */
	if (MANDLOCK(SAM_ITOV(ip), ip->di.mode)) {
		return (EACCES);
	}

	/*
	 * Process file/record lock fcntl call.
	 */
	if (ltype == LTYPE_frlock) {
		error = sam_process_frlock_request(ip, client_ord, l2p, TRUE);
		return (error);
	}

	credp = sam_getcred(&lp->cred);
	/*
	 * Attempt to get the requested lease.
	 */
	if ((error = sam_record_lease(ip, client_ord, ltype,
	    l2p, (msg->hdr.wait_flag <= SHARE_wait), TRUE)) == 0) {

		/*
		 * If lease acquired (not waiting), do server-side processing
		 * as necessary.
		 */
		if (!(l2p->irec.sr_attr.actions & SR_WAIT_LEASE)) {
			switch (ltype) {

			case LTYPE_write:
				if ((ip->mp->mt.fi_config & MT_MH_WRITE) == 0) {
					ip->write_owner = client_ord;
				}
				if (lp->data.sparse == SPARSE_noalloc) {
					/*
					 * Don't allocate any space even if the
					 * region is sparse. The client is
					 * zeroing sparse regions during
					 * directio and needs to know when
					 * sparse regions are written to. A
					 * later call to sam_map_block on the
					 * client will tell him that.
					 */
					ASSERT(lp->data.shflags.b.directio);
					break;
				}
				if ((lp->data.sparse == SPARSE_zeroall) ||
				    (lp->data.sparse == SPARSE_directio)) {
					sam_ioblk_t ioblk;

					/*
					 * Allocate DAUs and return a bit map of
					 * allocated blocks. The client will
					 * clear the allocated blocks.
					 */
					error = sam_map_block(ip,
					    lp->data.offset,
					    lp->data.resid, SAM_ALLOC_BITMAP,
					    &ioblk, credp);
					lp->data.no_daus = ioblk.no_daus;
					lp->data.zerodau[0] = ioblk.zerodau[0];
					lp->data.zerodau[1] = ioblk.zerodau[1];

				} else if ((lp->data.sparse == SPARSE_nozero) ||
				    lp->data.shflags.b.directio) {
					error = sam_map_block(ip,
					    lp->data.offset,
					    lp->data.resid, SAM_ALLOC_BLOCK,
					    NULL, credp);
					lp->data.no_daus = 0;

				} else {
					break;
				}
				if (error == 0) {
					l2p->irec.sr_attr.actions |=
					    (SR_STALE_INDIRECT|SR_INVAL_PAGES);
					l2p->irec.sr_attr.offset =
					    lp->data.offset;
					ip->flags.b.updated = 1;
					error = sam_update_inode(ip,
					    SAM_SYNC_ONE, FALSE);
				}
				break;

			case LTYPE_append:
				if (ip->size_owner != client_ord) {
					/*
					 * Force size from the server if this is
					 * the first append. Note, the client
					 * does not know the true size if this
					 * is the first append.
					 */
					l2p->irec.sr_attr.actions |=
					    SR_FORCE_SIZE;
				}
				/*
				 * If the requester is already the size owner
				 * he knows what the correct size is so assume
				 * he has sent the correct offset for FAPPEND.
				 * Otherwise set the offset to the current size.
				 */
				if ((lp->data.filemode & FAPPEND) &&
				    (ip->size_owner != client_ord)) {
					lp->data.offset = ip->di.rm.size;
				}
				ip->size_owner = client_ord;

				/*
				 * Don't do the allocate append if it's
				 * an object file, pre-allocated file
				 * (direct_map), or if it's the server
				 * doing the request.
				 */
				if (SAM_IS_OBJECT_FILE(ip)) {
					ip->cl_allocsz = ip->di.rm.size +
					    ip->mp->mt.fi_maxallocsz;
					break;
				}
				if (ip->di.status.b.direct_map ||
				    (client_ord == ip->mp->ms.m_client_ord)) {
					break;
				}

				/*
				 * If the offset is less than the current file
				 * size, issue a SAM_ALLOC_ZERO so an
				 * allocated block is cleared. This handles
				 * the case where a client begins writing in
				 * the middle of an unallocated DAU
				 * (a hole) prior to the current end of file.
				 */
				if (lp->data.offset < ip->di.rm.size) {
					error = sam_map_block(ip,
					    lp->data.offset,
					    (ip->di.rm.size -
					    lp->data.offset), SAM_ALLOC_ZERO,
					    NULL, credp);
					sam_flush_pages(ip, B_INVAL);
					if (error) {
						break;
					}
				}

				/* LINTED [fallthrough on case statement] */
			case LTYPE_stage:
				/*
				 * Don't do the allocate append if it's
				 * an object file.
				 */
				if (SAM_IS_OBJECT_FILE(ip)) {
					break;
				}
				error = sam_client_allocate_append(ip, msg);
				break;

			case LTYPE_truncate: {
				int rmv_err;

				error = sam_server_truncate(ip, l2p, credp);

				/*
				 * Truncate is immediate, remove the lease now
				 */
				rmv_err = sam_remove_lease(ip, client_ord,
				    &l2p->irec.sr_attr,	CL_TRUNCATE, NULL);

				if (error == 0) {
					error = rmv_err;
				}
				sam_flush_pages(ip, B_INVAL);
				}
				break;

			case LTYPE_mmap:
				if (l2p->inp.data.filemode & FREAD) {
					break;
				}
				if (lp->data.sparse) {

					/*
					 * If server, Allocate DAUs and return a
					 * bit map of allocated blocks.  The
					 * client will clear the allocated
					 * blocks.
					 */
					if (client_ord ==
					    ip->mp->ms.m_client_ord) {
						error = sam_map_block(ip,
						    lp->data.offset,
						    lp->data.resid,
						    SAM_WRITE_SPARSE, NULL,
						    credp);
						lp->data.no_daus = 0;
					} else {
						sam_ioblk_t ioblk, *iop;

						iop = &ioblk;
						error = sam_map_block(ip,
						    lp->data.offset,
						    lp->data.resid,
						    SAM_ALLOC_BITMAP, iop,
						    credp);
						if (error == 0) {
							lp->data.no_daus =
							    ioblk.no_daus;
							lp->data.zerodau[0] =
							    ioblk.zerodau[0];
							lp->data.zerodau[1] =
							    ioblk.zerodau[1];
						}
					}
					if (error == 0) {
						l2p->irec.sr_attr.actions |=
						    SR_STALE_INDIRECT;
						l2p->irec.sr_attr.offset =
						    lp->data.offset;
						ip->flags.b.updated = 1;
						error = sam_update_inode(ip,
						    SAM_SYNC_ONE, FALSE);
					}
				}
				break;

			case LTYPE_exclusive:
				break;

			default:
				break;
			}
			if (error == EDQUOT) {
				/*
				 * Remove lease on the server since it will
				 * never be granted to the client.
				 */
				sam_remove_lease(ip, client_ord,
				    &l2p->irec.sr_attr, (1<<ltype), NULL);
			}
		}
	}
	crfree(credp);
	return (error);
}


/*
 * ----- sam_process_frlock_request - process the frlock request.
 *
 *	Called to process the frlock request.
 */

static int				/* ERRNO, else 0 if successful. */
sam_process_frlock_request(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	sam_san_lease2_t *l2p,		/* Ptr to lease returned parameters */
	boolean_t is_get)		/* TRUE if LEASE_get (not a reset) */
{
	vnode_t *vp;
	sam_share_flock_t *flp = &l2p->inp.flock;
	struct flock64 flock;
	int cmd;
	int error;

	vp = SAM_ITOV(ip);
	cmd = l2p->inp.data.cmd;

	/* Construct the lock request. */

	bzero(&flock, sizeof (flock));
	flock.l_type = flp->l_type;
	flock.l_whence = flp->l_whence;
	flock.l_start = flp->l_start;
	flock.l_len = flp->l_end;
	flock.l_sysid = flp->l_sysid;
	flock.l_pid = flp->l_pid;

	l2p->granted_mask = 0; /* clear in case we return w/o recording lease */

	if (cmd == 0) {
		sam_blkd_frlock_t *cur;
		/* Cancel any blocked frlock requests */
		mutex_enter(&ip->blkd_frlock_mutex);
		cur = ip->blkd_frlock;
		while (cur) {
			if (flp->l_sysid == cur->flock.l_sysid &&
			    flp->l_pid == cur->flock.l_pid) {
				sam_blkd_frlock_cancel(cur);
			}
			cur = cur->fl_next; /* Okay, we still have mutex */
		}
		mutex_exit(&ip->blkd_frlock_mutex);

		cleanlocks(vp, flp->l_pid, flp->l_sysid);
		error = 0;
	} else if (cmd == F_HASREMOTELOCKS) {
		error = FS_FRLOCK_OS(vp, F_HASREMOTELOCKS, &flock, 0,
		    l2p->inp.data.filemode, NULL, kcred, NULL);
		/* LINTED [assignment causes implicit narrowing conversion] */
		flp->l_remote = l_has_rmt(&flock);
	} else {
		error = reclock(vp, &flock, (cmd == F_GETLK) ? 0 : SETFLCK,
		    l2p->inp.data.filemode, l2p->inp.data.offset, NULL);

		/* If  EAGAIN and a waiting lock, start blocking request */
		if (error == EAGAIN && (cmd == F_SETLKW || cmd == F_SETLKW64)) {
			error = sam_blkd_frlock_start(ip, client_ord,
			    l2p->inp.data.filemode, l2p->inp.data.offset,
			    &flock);
			/*
			 * EAGAIN is returned if the request blocked,
			 * translate this into a wait request.
			 */
			if (error == EAGAIN) {
				l2p->irec.sr_attr.actions |= (SR_WAIT_LEASE);
				error = 0;
			}
		}
	}

	if ((cmd == F_GETLK) || (cmd == F_HASREMOTELOCKS)) {
		flp->l_type = flock.l_type;
		flp->l_whence = flock.l_whence;
		flp->l_start = flock.l_start;
		flp->l_len = flock.l_len;
		flp->l_sysid = flock.l_sysid;
		flp->l_pid = flock.l_pid;
	} else {
		if (error == 0) {
			if ((cmd == 0) || (flp->l_type == F_UNLCK)) {
				error = sam_remove_lease(ip, client_ord,
				    &l2p->irec.sr_attr, CL_FRLOCK, NULL);
			} else {
				error = sam_record_lease(ip, client_ord,
				    LTYPE_frlock, l2p, FALSE, is_get);
			}
		}
	}
	return (error);
}

/*
 * ----- sam_blkd_frlock_start - dispatch a blocking frlock request
 *
 * Return value the result of reclock if task did not block.
 * Returns EAGAIN if existing blocked task is found, if new task
 * is blocked, or if we failed to create a new task.
 */

static int
sam_blkd_frlock_start(
	sam_node_t *ip,
	int client_ord,
	int filemode,
	int64_t offset,
	flock64_t *flock)
{
	sam_blkd_frlock_t *blkd_frlock;
	taskqid_t tid;
	int error;

	blkd_frlock = sam_blkd_frlock_new(ip, client_ord,
	    filemode, offset, flock);

	/*
	 * First try to find an existing blocked frlock that matches the one
	 * we are trying to start.  If this is the case then we do not want to
	 * start a new one.
	 */
	mutex_enter(&ip->blkd_frlock_mutex);
	if (ip->mp->mt.fi_status & (FS_FAILOVER|FS_UMOUNT_IN_PROGRESS) ||
	    sam_blkd_frlock_find(blkd_frlock)) {
		/* Found pending request or failover/umount in progress */
		error = EAGAIN;
		sam_blkd_frlock_free(blkd_frlock);
	} else {
		/*
		 * Since we didn't find an existing frlock, start it.  Dispatch
		 * task with NOQUEUE to start immediately or fail
		 */
		sam_blkd_frlock_insert(blkd_frlock);
		tid = taskq_dispatch(ip->mp->ms.m_frlock_taskq,
		    sam_blkd_frlock_task, blkd_frlock, TQ_NOQUEUE);

		if (tid == 0) {
			/*
			 * We failed to dispatch the task.  This is most likely
			 * because m_frlock_taskq is full and we specified to
			 * not queue tasks.  We will return EAGAIN and have
			 * the client wait to try again.
			 */
			blkd_frlock->status |= SAM_FRLOCK_DONE;
			blkd_frlock->rval = EAGAIN;
		} else {
			/* Wait for BLKD or DONE signal */
			cv_wait_sig(&blkd_frlock->fl_cv,
			    &ip->blkd_frlock_mutex);
		}

		/* Check if request already finished and return its status */
		if (blkd_frlock->status & SAM_FRLOCK_DONE) {
			error = blkd_frlock->rval;
			/* We are responsible for freeing non-blocked tasks */
			sam_blkd_frlock_remove(blkd_frlock);
			sam_blkd_frlock_free(blkd_frlock);
		} else {
			/* Request is now blocked */
			error = EAGAIN;
		}
	}

	mutex_exit(&ip->blkd_frlock_mutex);

	return (error);
}

/*
 * ----- sam_blkd_frlock_task - task to run blocking frlock
 *   arg: sam_blkd_frlock_t *blkd_frlock
 *
 * In the event the frlock request is blocked, SAM_FRLOCK_BLKD will
 * be set and blkd_frlock->fl_cv signaled before blocking.  It is the
 * responsibility of this task to remove blkd_frlock for blocked requests.
 *
 * If the request is not blocked SAM_FRLOCK_DONE will be set and
 * blkd_frlock->fl_cv will be signaled.  It is the responsibility of the
 * caller to remove the blkd_frlock after checking the return value of
 * reclock stored in blkd_frlock->rval.
 */

static void
sam_blkd_frlock_task(void *arg) {
	sam_blkd_frlock_t *blkd_frlock = (sam_blkd_frlock_t *)arg;
	flk_callback_t blkd_callback;
	timeout_id_t timeout_id;

	/*
	 * Create frlock callback which will be called in the event
	 * of the request blocking.  The callback is called twice, once
	 * immediately before blocking and once after.
	 */
	flk_add_callback(&blkd_callback, sam_blkd_frlock_callback,
	    blkd_frlock, NULL);

	mutex_enter(&blkd_frlock->ip->blkd_frlock_mutex);
	blkd_frlock->thread = curthread;

	/* It is possible we are already being destroyed */
	if (blkd_frlock->status & SAM_FRLOCK_DESTROY) {
		blkd_frlock->status |= SAM_FRLOCK_DONE;
		blkd_frlock->rval = EAGAIN;
	} else {
		/*
		 * Set timeout of half lease timeout.  This value ensures that
		 * proper deadlock detection will take place. If a deadlock
		 * situation exists we are always guaranteed to have waiting
		 * overlap with any other blocked locks.
		 */
		timeout_id = timeout(sam_blkd_frlock_cancel, blkd_frlock,
		    MAX_LEASE_TIME / 2 * hz);

		/* Attempt the lock request, if blocked callback will be used */
		blkd_frlock->rval = reclock(SAM_ITOV(blkd_frlock->ip),
		    &blkd_frlock->flock, (SETFLCK|SLPFLCK),
		    blkd_frlock->filemode, blkd_frlock->offset, &blkd_callback);

		/* Mark DONE and cancel timeout since we have returned */
		blkd_frlock->status |= SAM_FRLOCK_DONE;
		(void) untimeout(timeout_id);
	}

	/* Broadcast done (could have both caller and destroy waiting here) */
	cv_broadcast(&blkd_frlock->fl_cv);

	if (blkd_frlock->status & SAM_FRLOCK_BLKD) {
		/* Responsible for cleaning up blocked frlocks */
		sam_blkd_frlock_remove(blkd_frlock);
		mutex_exit(&blkd_frlock->ip->blkd_frlock_mutex);
		sam_blkd_frlock_free(blkd_frlock);
	} else {
		mutex_exit(&blkd_frlock->ip->blkd_frlock_mutex);
	}
}

/*
 * ----- sam_blkd_frlock_callback - Callback for handling blocked locks
 *
 * Callback used by reclock function just before and after blocking.  Parameter
 * "when" will equal FLK_BEFORE_SLEEP on call before blocking, and
 * FLK_AFTER_SLEEP after blocking.
 *
 * arg: sam_blkd_frlock_t *blkd_frlock
 */

static callb_cpr_t *
sam_blkd_frlock_callback(flk_cb_when_t when, void *arg) {
	sam_blkd_frlock_t *blkd_frlock = (sam_blkd_frlock_t *)arg;

	if (when == FLK_BEFORE_SLEEP) {
		blkd_frlock->status |= SAM_FRLOCK_BLKD;
		cv_signal(&blkd_frlock->fl_cv);
		/* Release mutex while sleeping */
		mutex_exit(&blkd_frlock->ip->blkd_frlock_mutex);
	} else {
		/* Acquire the mutex again */
		mutex_enter(&blkd_frlock->ip->blkd_frlock_mutex);
	}

	return (NULL);
}

/*
 * ----- sam_blkd_frlock_cancel - Cancels a blocked lock request
 * arg: sam_blkd_frlock_t *blkd_frlock
 *
 * A blocked reclock uses cv_wait_sig.  This function
 * signals the blocked thread to wake up and return EINTR.
 */

static void
sam_blkd_frlock_cancel(void *arg) {
	sam_blkd_frlock_t *blkd_frlock = (sam_blkd_frlock_t *)arg;

	/* Signal blocked task that is not done */
	if (blkd_frlock->thread &&
	    (blkd_frlock->status & SAM_FRLOCK_BLKD) &&
	    !(blkd_frlock->status & SAM_FRLOCK_DONE)) {
		sigtoproc(ttoproc(blkd_frlock->thread),
		    blkd_frlock->thread, SIGCONT);
	}
}

/*
 * ----- sam_blkd_frlock_destroy - Cancels all blocked locks on inode
 *
 * Sets destroy flag on each blocked frlock, cancels and waits for done.
 */

void
sam_blkd_frlock_destroy(sam_node_t *ip) {
	mutex_enter(&ip->blkd_frlock_mutex);
	while (ip->blkd_frlock) {
		ip->blkd_frlock->status |= SAM_FRLOCK_DESTROY;
		sam_blkd_frlock_cancel(ip->blkd_frlock);
		/* Wait for done */
		sam_cv_wait1sec_sig(&ip->blkd_frlock->fl_cv,
		    &ip->blkd_frlock_mutex);
	}
	mutex_exit(&ip->blkd_frlock_mutex);
}

/*
 * ----- sam_blkd_frlock_new - Allocate and initialize a new blocked frlock
 */

static sam_blkd_frlock_t *
sam_blkd_frlock_new(
	sam_node_t *ip,
	int client_ord,
	int filemode,
	int64_t offset,
	flock64_t *flock)
{
	sam_blkd_frlock_t *new_blkd;

	new_blkd = kmem_zalloc(sizeof (sam_blkd_frlock_t), KM_SLEEP);
	new_blkd->ip = ip;
	new_blkd->cl_ord = client_ord;
	new_blkd->filemode = filemode;
	new_blkd->offset = offset;
	new_blkd->flock = *flock;
	cv_init(&new_blkd->fl_cv, NULL, CV_DEFAULT, NULL);

	return (new_blkd);
}

/*
 * ----- sam_blkd_frlock_free - Free blocked frlock resources and memory
 */

static void
sam_blkd_frlock_free(sam_blkd_frlock_t *blkd_frlock)
{
	cv_destroy(&blkd_frlock->fl_cv);
	kmem_free(blkd_frlock, sizeof (sam_blkd_frlock_t));
}

/*
 * ----- sam_blkd_frlock_find - Find an existing frlock on inode
 *
 * Return an existing blkd_frlock that matches the provided data,
 * or null if no such entry exists.
 * ip->blkd_frlock_mutex must be held
 */

static sam_blkd_frlock_t *
sam_blkd_frlock_find(sam_blkd_frlock_t *blkd_frlock)
{
	sam_node_t *ip = blkd_frlock->ip;
	flock64_t *flock = &blkd_frlock->flock;
	sam_blkd_frlock_t *cur;

	ASSERT(ip);
	ASSERT(MUTEX_HELD(&ip->blkd_frlock_mutex));

	cur = ip->blkd_frlock;
	while (cur) {
		if (cur->cl_ord == blkd_frlock->cl_ord &&
		    cur->filemode == blkd_frlock->filemode &&
		    cur->offset == blkd_frlock->offset &&
		    cur->flock.l_sysid == flock->l_sysid &&
		    cur->flock.l_pid == flock->l_pid &&
		    cur->flock.l_type == flock->l_type &&
		    cur->flock.l_whence == flock->l_whence &&
		    cur->flock.l_start == flock->l_start &&
		    cur->flock.l_len == flock->l_len) {
			break; /* Found it! */
		}
		cur = cur->fl_next;
	}

	return (cur);
}

/*
 * ----- sam_blkd_frlock_insert - Insert new node at head of list
 *
 * Allocates a new sam_blkd_frlock structure and inserts it into
 * the provided inodes blocked frlock list.
 */

static void
sam_blkd_frlock_insert(sam_blkd_frlock_t *blkd_frlock)
{
	sam_node_t *ip = blkd_frlock->ip;
	ASSERT(ip);
	ASSERT(MUTEX_HELD(&ip->blkd_frlock_mutex));

	/* Insert new entry into inode list */
	if (ip->blkd_frlock) {
		ip->blkd_frlock->fl_prev = blkd_frlock;
	}
	blkd_frlock->fl_next = ip->blkd_frlock;
	ip->blkd_frlock = blkd_frlock;
}

/*
 * ----- sam_blkd_frlock_remove - Remove blkd_frlock from list
 *
 * Remove the provided entry from its list.
 */

static void
sam_blkd_frlock_remove(sam_blkd_frlock_t *blkd_frlock)
{
	sam_node_t *ip = blkd_frlock->ip;

	ASSERT(ip);
	ASSERT(MUTEX_HELD(&ip->blkd_frlock_mutex));

	/* Update prev entry */
	if (blkd_frlock->fl_prev == NULL) {
		ip->blkd_frlock = blkd_frlock->fl_next;
	} else {
		blkd_frlock->fl_prev->fl_next = blkd_frlock->fl_next;
	}

	/* Update next entry */
	if (blkd_frlock->fl_next) {
		blkd_frlock->fl_next->fl_prev = blkd_frlock->fl_prev;
	}
}

/*
 * ----- sam_process_rm_lease - process the lease remove operation.
 *
 *	Called when the shared server receives a LEASE remove operation or
 *  when the server host directly calls this function.
 *
 *  Note: inode_rwl lock set on entry and exit (as reader or writer).
 *		Must be WRITER lock for all leases other than a read lease.
 */

static int				/* ERRNO, else 0 if successful. */
sam_process_rm_lease(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	int client_ord;
	int error;
	uint32_t *gen_table = NULL;

	client_ord = msg->hdr.client_ord;
	if (lp->data.ltype & CL_OPEN) {
		/*
		 * Use the lease generation numbers
		 * if an open lease (plus probably others)
		 * is being removed so we don't
		 * remove more recently acquired
		 * leases.
		 *
		 * It is possible we should do this
		 * for all cases.
		 */
		gen_table = lp->gen;
	}
	error = sam_remove_lease(ip, client_ord,
	    &l2p->irec.sr_attr, lp->data.ltype, gen_table);
	return (error);
}


/*
 * ----- sam_process_reset_lease - process the lease reset operation.
 *
 *	Called when the shared server receives a LEASE_reset operation or
 *  when the server host directly calls this function.
 *
 *  Note: inode_rwl lock set on entry and exit (as reader or writer).
 *		Must be WRITER lock for all leases other than a read lease.
 */

static int				/* ERRNO, else 0 if successful. */
sam_process_reset_lease(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	int client_ord;
	enum LEASE_type ltype;
	int error = 0;

	client_ord = msg->hdr.client_ord;

	/*
	 * When resetting leases during failover, ltype is the lease mask
	 * for all leases except frlock. Frlock is issued with a separate
	 * LEASE_reset message.
	 */
	if (lp->data.ltype & CL_FRLOCK) {
		error = sam_process_frlock_request(ip, client_ord, l2p, FALSE);
	} else {
		uint32_t lease_stage;
		uint32_t lease_mask;
		int err;

		lease_stage = lp->data.ltype & CL_STAGE;
		lease_mask = lp->data.ltype & ~CL_STAGE;
		for (ltype = LTYPE_read; ltype < SAM_MAX_LTYPE; ltype++) {
			if (lease_mask & (1 << ltype)) {
				if ((err = sam_record_lease(ip, client_ord,
				    ltype, l2p, FALSE, FALSE))) {
					error = err;
					continue;
				}
				if (ltype == LTYPE_append) {
					ip->size_owner = client_ord;
				} else if (ltype == LTYPE_write) {
					if ((ip->mp->mt.fi_config &
					    MT_MH_WRITE) == 0) {
						ip->write_owner = client_ord;
					}
				}
			}
		}
		if (lease_stage) {
			ip->copy = (lp->data.copy == 0) ? 1 : lp->data.copy;
			ip->flags.b.stage_n = 0;
			error = sam_proc_offline(ip, (offset_t)0,
			    ip->di.rm.size, NULL, CRED(), NULL);
			if (!ip->flags.b.staging || !ip->di.status.b.offline) {
				ip->flags.b.stage_n = ip->di.status.b.direct;
				sam_notify_staging_clients(ip);
			}
		}
	}
	/*
	 * Lease_reset is issued during failover. Clear actions
	 * because this is simply a reestablishment of the existing
	 * lease. Actions (stale indirect, invalidate pages) performed
	 * on the client can cause a deadlock during failover.
	 */
	l2p->irec.sr_attr.actions = 0;
	ip->flags.b.changed = 1;
	ip->sr_sync = 1;
	return (error);
}


/*
 * ----- sam_process_relinquish_lease - process the lease relinquish operation.
 *
 *	Called when the shared server receives a LEASE_relinquish operation or
 *  when the server host directly calls this function.
 *
 *  Note: inode_rwl WRITER set on entry and exit.
 */

static int				/* ERRNO, else 0 if successful. */
sam_process_relinquish_lease(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	int client_ord;
	uint32_t lease_mask;
	int error;

	client_ord = msg->hdr.client_ord;

	/*
	 * If lease_mask is all lease types, this is a close.
	 */
	lease_mask = lp->data.ltype;
	error = sam_remove_lease(ip, client_ord, &l2p->irec.sr_attr,
	    lease_mask, lp->gen);

	/*
	 * If a client is relinquishing a write, append, or stage lease,
	 * invalidate data pages on the server.
	 *
	 * XXX - Can't do this yet.  Multi-host write means that both the
	 * client and server may hold valid dirty pages.  How can we fix
	 * this without tracking who's modified which region???
	 */

#if 0
	if ((client_ord != ip->mp->ms.m_client_ord) &&
	    ((lease_mask & (CL_WRITE | CL_APPEND)) != 0)) {
		(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0, 0, B_INVAL, kcred, NULL);
	}
#endif
	return (error);
}


/*
 * ----- sam_process_extend_lease - process the lease extend operation.
 *
 *	Called when the shared server receives a LEASE_extend operation or
 *  when the server host directly calls this function.
 *
 *  Note: inode_rwl lock set on entry and exit (as reader or writer).
 *		Must be WRITER lock for all leases other than a read lease.
 */

static int				/* ERRNO, else 0 if successful. */
sam_process_extend_lease(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	int client_ord;
	int error;
	uint32_t lease_mask;

	client_ord = msg->hdr.client_ord;
	lease_mask = lp->data.ltype;

	error = sam_extend_lease(ip, client_ord, lease_mask, l2p);

	return (error);
}


/*
 * ----- sam_record_lease - record the lease in the linked list.
 *
 * Returns 1 if waiting on another lease, 0 if lease acquired.
 */

static int
sam_record_lease(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	enum LEASE_type ltype,		/* Lease type */
	sam_san_lease2_t *l2p,		/* lease returned parameters */
	boolean_t new_generation,	/* Set if not allocate-ahead etc. */
	boolean_t is_get)		/* TRUE if this is a LEASE_get */
{
	sam_mount_t *mp = ip->mp;
	sam_client_lease_t *clp;
	int error;
	boolean_t implicit_read;
	boolean_t implicit_write;
	boolean_t lease_granted;
	uint16_t granted_mask;
	uint32_t interval;

	TRACE(T_SAM_LEASE_ADD, SAM_ITOV(ip), ip->di.id.ino, client_ord,
	    1 << ltype);

	granted_mask = 0;

	if (ltype < MAX_EXPIRING_LEASES) {
		interval = l2p->inp.data.interval[ltype];
	} else {
		interval = MAX_LEASE_TIME;
	}

	/*
	 * Put this inode in the lease linked list. Hold inode while the lease
	 * is active. The reclaim thread expires leases and inactivates the
	 * inode.  Signal the reclaim thread if it is not already running.  If
	 * this inode does not have an existing lease entry, make a new one,
	 * Otherwise, add the lease request to the existing inode entry.
	 */
	mutex_enter(&ip->ilease_mutex);
	if (ip->sr_leases == NULL) {
		error = sam_new_ino_lease(ip, client_ord,
		    ltype, interval, l2p, &clp, FALSE);
	} else {
		error = sam_add_ino_lease(ip, client_ord,
		    ltype, interval, l2p, &clp, FALSE);
	}

	/*
	 * If we're opening a file for read-only access, grant a read lease.
	 * This saves a transaction in the common case.  This is only safe
	 * if no other clients have conflicting leases; a rough approximation
	 * (used here) is simply to check that this is the only client.
	 */

	lease_granted = ((error == 0) && (clp != NULL) &&
	    !(l2p->irec.sr_attr.actions & SR_WAIT_LEASE));

	if (lease_granted) {
		granted_mask = (1 << ltype);
	}

	implicit_read = FALSE;
	implicit_write = FALSE;

	if ((ltype == LTYPE_open) && is_get && lease_granted &&
	    ((l2p->inp.data.filemode & (FCREAT|FTRUNC|FWRITE)) == 0)) {
		if (ip->sr_leases->no_clients == 1) {
			clp->leases |= CL_READ;
			clp->time[LTYPE_read] = clp->time[LTYPE_open];
			implicit_read = TRUE;
			granted_mask |= (1 << LTYPE_read);
		}
	}

	/*
	 * Provide implicit read or write lease with the mmap lease.
	 */
	if ((ltype == LTYPE_mmap) && is_get && lease_granted) {
		if (l2p->inp.data.filemode & FREAD) {
			clp->leases |= CL_READ;
			clp->time[LTYPE_read] = clp->time[LTYPE_mmap];
			implicit_read = TRUE;
			granted_mask |= (1 << LTYPE_read);
		} else {
			clp->leases |= CL_WRITE;
			clp->time[LTYPE_write] = clp->time[LTYPE_mmap];
			implicit_write = TRUE;
			granted_mask |= (1 << LTYPE_write);
		}
	}

	if ((ltype == LTYPE_append) && is_get && lease_granted) {
		clp->leases |= CL_WRITE;
		clp->time[LTYPE_write] = clp->time[LTYPE_append];
		implicit_write = TRUE;
		granted_mask |= (1 << LTYPE_write);
	}

	if (new_generation && lease_granted) {
		l2p->inp.gen[ltype] = ++(clp->gen[ltype]);
		if (implicit_write) {
			l2p->inp.gen[LTYPE_write] = ++(clp->gen[LTYPE_write]);
		}
		if (implicit_read) {
			l2p->inp.gen[LTYPE_read] = ++(clp->gen[LTYPE_read]);
		}
	}

	mutex_exit(&ip->ilease_mutex);

	l2p->granted_mask = granted_mask;

	/*
	 * Schedule server-lease reclamation for when this lease expires.
	 */

	if (!(l2p->irec.sr_attr.actions & SR_WAIT_LEASE)) {
		sam_sched_expire_server_leases(mp,
		    sam_calculate_lease_timeout(interval) * hz, FALSE);
	}
	TRACE(T_SAM_LEASE_ADD_RET, SAM_ITOV(ip), ip->di.id.ino, client_ord,
	    granted_mask);

	return (error);
}


/*
 * -----	sam_new_ino_lease - create the lease entry for this inode.
 * This is the first lease entry for this inode.
 */

static int				/* 0 if lease set */
sam_new_ino_lease(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	enum LEASE_type ltype,		/* Lease type */
	uint32_t interval,		/* Lease expiration interval */
	sam_san_lease2_t *l2p,		/* the lease returned parameters */
	sam_client_lease_t **out_clp,	/* per-client lease info */
	boolean_t mutex_held)		/* Mount m_lease_mutex held */
{
	sam_mount_t *mp = ip->mp;
	struct sam_lease_ino *llp;
	uint16_t lease_mask;

	*out_clp = NULL;

	/*
	 * Allocate lease structure before grabbing global mutex to avoid
	 * holding the mutex so long.
	 */

	llp = (sam_lease_ino_t *)kmem_zalloc(sizeof (sam_lease_ino_t),
	    KM_SLEEP);

	/*
	 * Need to link this inode into the lease chain.  Drop the inode
	 * lease mutex so we can get the global mutex too.  We may be
	 * racing with someone else who wants to add a lease to this
	 * inode; after reacquiring mutexes, check for this case.
	 */

	if (!mutex_held) {
		mutex_exit(&ip->ilease_mutex);
		mutex_enter(&mp->mi.m_lease_mutex);
		mutex_enter(&ip->ilease_mutex);
		if (ip->sr_leases) {
			int error;

			kmem_free((char *)llp, sizeof (sam_lease_ino_t));

			error =
			    sam_add_ino_lease(ip, client_ord,
			    ltype, interval, l2p, out_clp,
			    TRUE);
			mutex_exit(&mp->mi.m_lease_mutex);
			return (error);
		}
	}

	ip->sr_leases = llp;

	llp->ip = ip; /* Must be set under protection of global lease mutex. */

	/*
	 * Add new leases to the end of the chain.
	 */
	mp->mi.m_sr_lease_chain.back->lease_chain.forw = llp;
	llp->lease_chain.forw =
	    (sam_lease_ino_t *)(void *)&mp->mi.m_sr_lease_chain;
	llp->lease_chain.back = mp->mi.m_sr_lease_chain.back;
	mp->mi.m_sr_lease_chain.back = llp;


	if (!mutex_held) {
		mutex_exit(&mp->mi.m_lease_mutex);
	}

	llp->max_clients = 1;
	llp->no_clients = 1;
	llp->notify_ord = 0;

	lease_mask = 1 << ltype;
	llp->lease[0].leases = lease_mask;
	llp->lease[0].actions = l2p->irec.sr_attr.actions;
	llp->lease[0].client_ord = client_ord;
	if (lease_mask & (CL_READ|CL_WRITE|CL_APPEND)) {
		llp->lease[0].shflags.b.directio =
		    l2p->inp.data.shflags.b.directio;
	}
	llp->lease[0].shflags.b.abr = ip->flags.b.abr;
	TRACE(T_SAM_ABR_LOPEN, SAM_ITOP(ip),
	    ip->di.id.ino, ip->flags.b.abr, llp->lease[0].shflags.bits);
	llp->lease[0].time[ltype] = lbolt +
	    (sam_calculate_lease_timeout(interval) * hz);

	/*
	 * Hold this inode while any lease is active. The reclaim
	 * thread inactivates the inode when the last lease is freed.
	 */
	VN_HOLD(SAM_ITOV(ip));	/* Hold this inode during the lease */

	*out_clp = &llp->lease[0];

	SAM_COUNT64(shared_server, lease_new);

	return (0);
}


/*
 * -----	sam_check_lease_timeo - relinquish the lease according to
 * the lease_timeo mount option.
 */

static void
sam_check_lease_timeo(sam_node_t *ip, uint16_t relinquish_lease_mask,
    struct sam_lease_ino *llp, int client_ord)
{
	sam_callout_arg_t arg;
	int i;

	if (!relinquish_lease_mask) {
		return;
	}
	if (ip->mp->mt.fi_lease_timeo < 0) {
		return;
	}

	arg.p.relinquish_lease.timeo = ip->mp->mt.fi_lease_timeo;
	for (i = 0; i < llp->no_clients; i++) {
		if (llp->lease[i].client_ord != client_ord) {
			if (llp->lease[i].leases & relinquish_lease_mask) {
				arg.p.relinquish_lease.lease_mask =
				    (relinquish_lease_mask &
				    llp->lease[i].leases);
				SAM_COUNT64(shared_server,
				    callout_relinquish_lease);
				sam_proc_callout(ip, CALLOUT_relinquish_lease,
				    0, llp->lease[i].client_ord, &arg);
			}
		}
	}
}


/*
 * -----	sam_add_ino_lease - record the lease in the linked list.
 * Add the lease info to the existing inode lease entry.
 */

static int				/* 0 if lease added */
sam_add_ino_lease(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	enum LEASE_type ltype,		/* Lease type */
	uint32_t interval,		/* Lease expiration interval */
	sam_san_lease2_t *l2p,		/* the lease returned parameters */
	sam_client_lease_t **out_clp,	/* per-client lease info */
	boolean_t mutex_held)		/* Mount m_lease_mutex held */
{
	struct sam_lease_ino *llp;	/* Ptr to lease entry for this inode */
	int index;
	uint32_t other_leases;
	uint32_t mmap_leases;
	uint32_t wt_leases;
	uint16_t relinquish_lease_mask = 0; /* Ask client to relinq lease(s) */
	uint16_t lease_mask;
	int i;
	boolean_t wait_lease = FALSE; /* Will this lease wait behind others? */
	boolean_t may_read = FALSE;	/* Does this lease allow reading? */
	boolean_t may_write = FALSE;	/* Does this lease allow writing? */

	*out_clp = NULL;
	llp = ip->sr_leases;

	/*
	 * Common case is that this client already holds a lease.  Check for
	 * that first.  Note that other_leases, mmap_leases, and wt_leases
	 * are only applicable to other clients.
	 */

	index = -1;
	other_leases = 0;
	mmap_leases = 0;
	wt_leases = 0;
	for (i = 0; i < llp->no_clients; i++) {
		if (llp->lease[i].client_ord == client_ord) {
			if (llp->lease[i].leases == 0 &&
			    llp->lease[i].wt_leases == 0) {
				/*
				 * Reusing an old lease entry for this
				 * client_ord, so clean it up. The stale
				 * attr_seqno value here can prevent valid size
				 * updates.
				 */
				bzero(&llp->lease[i],
				    sizeof (sam_client_lease_t));
				llp->lease[i].client_ord = client_ord;
			}
			index = i;
		} else {
			other_leases |= llp->lease[i].leases;
			wt_leases |= llp->lease[i].wt_leases;
			if ((llp->lease[i].leases & (CL_READ|CL_WRITE)) &&
			    (llp->lease[i].leases & CL_MMAP)) {
				mmap_leases |= llp->lease[i].leases;
			}
		}
	}

	/*
	 * Didn't find this client.  Check for an unused entry in the table.
	 * If there is one, use it.  If not, need to expand the table, which
	 * also requires acquiring the global lease lock.
	 */

	if (index < 0) {
		if (llp->no_clients < llp->max_clients) {
			index = llp->no_clients;
			llp->no_clients++;
			bzero(&llp->lease[index], sizeof (sam_client_lease_t));
			llp->lease[index].client_ord = client_ord;
		} else {
			struct sam_lease_ino *sllp;
			int cur_clients, new_clients, mp_max_clients;

			if (!mutex_held) {
				mutex_exit(&ip->ilease_mutex);
				mutex_enter(&ip->mp->mi.m_lease_mutex);
				mutex_enter(&ip->ilease_mutex);
				llp = ip->sr_leases;
				if (llp == NULL) {
					int error;

					error = sam_new_ino_lease(ip,
					    client_ord,
					    ltype, interval, l2p, out_clp,
					    TRUE);
					mutex_exit(&ip->mp->mi.m_lease_mutex);
					return (error);
				}
			}

			/*
			 * We dropped the lock; check to see if this client was
			 * added to the table by another thread.
			 */
			index = -1;
			other_leases = 0;
			mmap_leases = 0;
			wt_leases = 0;
			for (i = 0; i < llp->no_clients; i++) {
				if (llp->lease[i].client_ord == client_ord) {
					index = i;
				} else {
					other_leases |= llp->lease[i].leases;
					wt_leases |= llp->lease[i].wt_leases;
					if ((llp->lease[i].leases &
					    (CL_READ|CL_WRITE)) &&
					    (llp->lease[i].leases & CL_MMAP)) {
						mmap_leases |=
						    llp->lease[i].leases;
					}
				}
			}

			if (index < 0) {
				/*
				 * Still no entry for this client; grow the
				 * table.
				 */
				SAM_COUNT64(shared_server, lease_grow);

				/*
				 * Note that no_clients may no longer equal
				 * max_clients since we dropped the lock
				 * above.  Use
				 * max_clients to ensure we are using the
				 * correct sizes.
				 *
				 * Read ip->mp->ms.m_max_clients just once
				 * here to
				 * avoid possible race conditions with
				 * client hosts
				 * registering with the server.
				 */
				cur_clients = llp->max_clients;
				new_clients = cur_clients +
				    sam_client_lease_increment;
				mp_max_clients = ip->mp->ms.m_max_clients;
				if (new_clients > mp_max_clients) {
					ASSERT(cur_clients < mp_max_clients);
					new_clients = mp_max_clients;
				}

				sllp = llp;
				llp = (sam_lease_ino_t *)kmem_alloc(
				    sizeof (sam_lease_ino_t) +
				    sizeof (sam_client_lease_t) *
				    (new_clients - 1),
				    KM_SLEEP);
				bcopy((char *)sllp, (char *)llp,
				    sizeof (sam_lease_ino_t) +
				    (sizeof (sam_client_lease_t) *
				    (cur_clients - 1)));
				sllp->lease_chain.back->lease_chain.forw = llp;
				sllp->lease_chain.forw->lease_chain.back = llp;
				llp->max_clients = new_clients;
				kmem_free((char *)sllp,
				    (sizeof (sam_lease_ino_t) +
				    (sizeof (sam_client_lease_t) *
				    (cur_clients - 1))));
				ip->sr_leases = llp;
				if (!mutex_held) {
					mutex_exit(&ip->mp->mi.m_lease_mutex);
				}

				index = llp->no_clients;
				llp->no_clients++;
				bzero(&llp->lease[index],
				    sizeof (sam_client_lease_t));
				llp->lease[index].client_ord = client_ord;

				/*
				 * We dropped the lock above; need to recheck
				 * other leases.
				 */

				other_leases = 0;
				mmap_leases = 0;
				for (i = 0; i < llp->no_clients; i++) {
					if (llp->lease[i].client_ord ==
					    client_ord) {
						continue;
					}
					other_leases |= llp->lease[i].leases;
					if ((llp->lease[i].leases &
					    (CL_READ|CL_WRITE)) &&
					    (llp->lease[i].leases &
					    CL_MMAP)) {
						mmap_leases |=
						    llp->lease[i].leases;
					}
				}
			} else {
				if (!mutex_held) {
					mutex_exit(&ip->mp->mi.m_lease_mutex);
				}
			}
		}
	}

	llp->lease[index].actions |=
	    (l2p->irec.sr_attr.actions & ~SR_WAIT_LEASE);
	lease_mask = 1 << ltype;
	if (lease_mask & (CL_READ|CL_WRITE|CL_APPEND)) {
		llp->lease[index].shflags.b.directio =
		    l2p->inp.data.shflags.b.directio;
	}
	TRACE(T_SAM_ABR_LADD, SAM_ITOP(ip), ip->di.id.ino, lease_mask,
	    (ip->flags.b.abr << 2) |
	    (llp->lease[index].shflags.b.abr << 1) |
	    (int)l2p->inp.data.shflags.b.abr);
	if (ip->flags.b.abr) {
		if (!llp->lease[index].shflags.b.abr ||
		    !l2p->inp.data.shflags.b.abr) {
			llp->lease[index].actions |= SR_ABR_ON;
			llp->lease[index].shflags.b.abr = 1;
			l2p->inp.data.shflags.b.abr = 1;
		}
	}

	/*
	 * Check if this client must wait for the lease depending on
	 * other outstanding leases and the mh_write setting. Set
	 * server directed action for each client based on the other
	 * outstanding leases.
	 */
	switch (ltype) {

	case LTYPE_read:
		/*
		 * Allow multiple readers, but if there are any writers:
		 * Truncate and writable mmaps always block reads.
		 * No MH_WRITE - Wait reader until writer's lease
		 *		 expires or is relinquished.
		 * MH_WRITE - Allow multiple readers/writers but direct this
		 *		 client to stale and enable directio.
		 */
		if ((other_leases & (CL_TRUNCATE|CL_EXCLUSIVE)) ||
		    (mmap_leases & CL_WRITE)) {
			wait_lease = TRUE;
		} else if (other_leases & (CL_WRITE|CL_APPEND)) {
			if (!(ip->mp->mt.fi_config & MT_MH_WRITE)) {
				wait_lease = TRUE;
				relinquish_lease_mask =
				    other_leases & (CL_APPEND|CL_WRITE);
			} else {
				/*
				 * If lease owner is in paged mode, wait until
				 * owner has flushed pages. Lease owner will
				 * sync pages and send BLOCK_wakeup.
				 */
				llp->lease[index].actions |= SR_DIRECTIO_ON;
				wait_lease = sam_callout_directio(ip,
				    client_ord, llp);
			}
		}
		may_read = TRUE;
		break;

	case LTYPE_write:
		/*
		 * Allow only one writer at a time, but:
		 * Truncate and stage always block writes.
		 * No MH_WRITE - Wait writer until reader's/writer's lease
		 *		 expires or is relinquished.
		 * MH_WRITE - Readable/Writeable mmaps always block writes.
		 *	    - Other writes always block mmap writes.
		 *	    - Allow multiple readers/writers but direct this
		 *		 client to stale and enable directio.
		 */
		if (other_leases &
		    (CL_TRUNCATE|CL_STAGE|CL_EXCLUSIVE)) {
			wait_lease = TRUE;
		} else if (other_leases & ~(CL_OPEN|CL_FRLOCK|CL_MMAP)) {
			if (!(ip->mp->mt.fi_config & MT_MH_WRITE)) {
				wait_lease = TRUE;
				relinquish_lease_mask =
				    other_leases & (CL_APPEND|CL_WRITE|CL_READ);
			} else if ((mmap_leases & (CL_READ|CL_WRITE)) ||
			    ((llp->lease[index].leases & CL_MMAP) &&
			    (other_leases & (CL_APPEND|CL_WRITE)))) {
				wait_lease = TRUE;
			} else {
				/*
				 * If lease owner is in paged mode, wait until
				 * owner has flushed pages. Lease owner will
				 * sync pages and send BLOCK_wakeup.
				 */
				llp->lease[index].actions |= SR_DIRECTIO_ON;
				wait_lease = sam_callout_directio(ip,
				    client_ord, llp);
			}
		}
		may_read = TRUE;
		may_write = TRUE;
		break;

	case LTYPE_append:
		/*
		 * Only allow 1 append at one time. If MH_WRITE set on the
		 * server, allow readers and/or writers, but put them in
		 * directio.
		 */
		if (other_leases &
		    (CL_APPEND|CL_TRUNCATE|CL_STAGE|CL_EXCLUSIVE) ||
		    (mmap_leases & (CL_READ|CL_WRITE))) {
			wait_lease = TRUE;
			relinquish_lease_mask = other_leases & CL_APPEND;
		} else if (other_leases & ~(CL_OPEN|CL_FRLOCK)) {
			if (!(ip->mp->mt.fi_config & MT_MH_WRITE)) {
				wait_lease = TRUE;
				relinquish_lease_mask =
				    other_leases & (CL_APPEND|CL_WRITE|CL_READ);
			} else {
				/*
				 * If lease owner is in paged mode, wait until
				 * owner has flushed pages. Lease owner will
				 * sync pages and send BLOCK_wakeup.
				 */
				llp->lease[index].actions |= SR_DIRECTIO_ON;
				wait_lease = sam_callout_directio(ip,
				    client_ord, llp);
			}
		}
		may_read = TRUE;
		may_write = TRUE;
		break;

	case LTYPE_truncate:
		/*
		 * Only allow 1 truncate at one time. There can be no other
		 * outstanding leases except stage, open, or frlocks.
		 *
		 * If this is a SAM_RELEASE truncate, we are truncating due to
		 * a stage error or EOF.  We cannot wait for clients to expire
		 * their read or write leases because they may be waiting for
		 * a stage to complete.  For now, we will allow the truncation
		 * to proceed.  This is OK for writes because no writes could
		 * execute while staging is in progress.  For reads, this opens
		 * a very small window during which a read could proceed and
		 * access blocks which have been written by the stager but then
		 * truncated.  (The correct fix is probably to issue a
		 * notification
		 * to clients which will cause their leases to be cancelled.)
		 *
		 * Note: The truncate on the caller host locks out all other
		 * operations during the truncate.
		 */
		if (l2p->inp.data.lflag != SAM_RELEASE) {
			if (other_leases & ~(CL_STAGE|CL_OPEN|CL_FRLOCK)) {
				/*
				 * It is OK to truncate up the file if
				 * multi-host write set and there is no append
				 * lease. Otherwise, request a lease relinquish
				 * from the owner client(s). All leases must
				 * expire before we can truncate the file.
				 */
				wait_lease = TRUE;
				if (l2p->inp.data.resid >= ip->di.rm.size) {
					if ((ip->mp->mt.fi_config &
					    MT_MH_WRITE) &&
					    !(other_leases & CL_APPEND)) {
						wait_lease = FALSE;
					}

				} else {
					relinquish_lease_mask =
					    other_leases &
					    (CL_APPEND|CL_WRITE|CL_READ);
				}
			}
		}
		may_write = TRUE;
		break;

	case LTYPE_frlock:
		/*
		 * Frlock logic done by Solaris.
		 */
		if (l2p->irec.sr_attr.actions & SR_WAIT_LEASE) {
			wait_lease = TRUE;
		}
		break;

	case LTYPE_stage:
		break;

	case LTYPE_open:
		break;

	case LTYPE_mmap:
		/*
		 * We grant a read or write lease with the mmap lease so
		 * we do the same checks as LTYPE_read and LTYPE_write leases.
		 *
		 * In addition, we can't mix mmap'ed paged I/O with direct I/O
		 * (forced with MH_WRITE) so we use the mmap_leases bitmap
		 * to flag if others hold both a mmap lease AND either a
		 * read or write lease.
		 */
		if (l2p->inp.data.filemode & FWRITE) {
			if (other_leases &
			    (CL_TRUNCATE|CL_STAGE|CL_EXCLUSIVE)) {
				wait_lease = TRUE;
			} else if (other_leases &
			    ~(CL_OPEN|CL_FRLOCK|CL_MMAP)) {
				if (!(ip->mp->mt.fi_config & MT_MH_WRITE)) {
					wait_lease = TRUE;
					relinquish_lease_mask =
					    other_leases &
					    (CL_APPEND|CL_WRITE|CL_READ);
				} else if (mmap_leases & CL_READ|CL_WRITE) {
					wait_lease = TRUE;
				} else if (other_leases & CL_READ) {
					wait_lease = TRUE;
					relinquish_lease_mask =
					    other_leases &
					    (CL_APPEND|CL_WRITE|CL_READ);
				} else if (other_leases & CL_WRITE) {
					wait_lease = TRUE;
				}
			}
			may_write = TRUE;
		} else {
			if (other_leases & (CL_TRUNCATE|CL_EXCLUSIVE)) {
				wait_lease = TRUE;
			} else if (other_leases & (CL_WRITE|CL_APPEND)) {
				wait_lease = TRUE;
				if (!(ip->mp->mt.fi_config & MT_MH_WRITE)) {
					relinquish_lease_mask =
					    other_leases & (CL_APPEND|CL_WRITE);
				}
			}
		}
		may_read = TRUE;
		break;

	case LTYPE_exclusive:
		/*
		 * No other concurrent leases are allowed except CL_OPEN.
		 * Also someone waiting for an exclusive lease
		 * will block other lease requests.
		 */
		if (other_leases &
		    (CL_WRITE|CL_APPEND|CL_TRUNCATE|CL_READ|
		    CL_STAGE|CL_EXCLUSIVE)) {
			wait_lease = TRUE;
			relinquish_lease_mask =
			    other_leases & (CL_APPEND|CL_WRITE|CL_READ);
		}
		may_read = TRUE;
		may_write = TRUE;
		break;

	default:
		dcmn_err((CE_WARN, "SAM-QFS: %s: server bad lease type=%d",
		    ip->mp->mt.fi_name, ltype));
		break;
	}

	SAM_COUNT64(shared_server, lease_add);

	if (wait_lease) {
		llp->lease[index].wt_leases |= lease_mask;
		llp->lease[index].actions = SR_WAIT_LEASE;
		l2p->irec.sr_attr.actions = SR_WAIT_LEASE;
		SAM_COUNT64(shared_server, lease_wait);

		/*
		 * If lease_timeo set, request client to relinquish lease.
		 */
		sam_check_lease_timeo(ip, relinquish_lease_mask, llp,
		    client_ord);
	} else {
		llp->lease[index].leases |= lease_mask;
		llp->lease[index].time[ltype] = lbolt +
		    (sam_calculate_lease_timeout(interval) * hz);

		/*
		 * Except for FRLOCKs, granting a lease means we're no longer
		 * waiting for a lease of that type.
		 */
		if (ltype != LTYPE_frlock) {
			llp->lease[index].wt_leases &= ~lease_mask;
		}

		/*
		 * If this lease grants read permission, check for stale data.
		 */
		if (may_read) {
			if (llp->lease[index].wr_seq != ip->sr_write_seqno) {
				llp->lease[index].wr_seq = ip->sr_write_seqno;
				llp->lease[index].actions |=
				    (SR_STALE_INDIRECT|SR_INVAL_PAGES);
			}
		}

		/*
		 * If this lease grants write permission, record the fact so
		 * that later readers know to stale cached data.
		 */
		if (may_write) {
			ip->sr_write_seqno++;
			llp->lease[index].wr_seq = ip->sr_write_seqno;
		}

		/*
		 * Don't send SR_WAIT_LEASE when a lease
		 * is being granted, it can still be set if
		 * the client is still waiting for some other lease.
		 */
		l2p->irec.sr_attr.actions =
		    llp->lease[index].actions & ~SR_WAIT_LEASE;

		if (llp->lease[index].wt_leases == 0) {
			llp->lease[index].actions = 0;
		} else {
			/*
			 * Set this if leases are still being waited for.
			 */
			llp->lease[index].actions = SR_WAIT_LEASE;
		}
	}

	*out_clp = &llp->lease[index];

	return (0);
}


/*
 * -----	sam_callout_actions - callout for server directed actions.
 *
 * Depending on current leases, the client may need to
 * execute server directed actions.
 */

static void
sam_callout_actions(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	ushort_t actions,		/* Server directed action flags */
	sam_lease_ino_t *llp)		/* the lease entry for this inode */
{
	int nl;

	/*
	 * Refresh caller inode for all clients who have a lease(s) for this
	 * inode.  Notify clients to perform required actions: Sync/invalidate
	 * pages, Read/write with directio if not already notified, etc.
	 */
	for (nl = 0; nl < llp->no_clients; nl++) {
		if (llp->lease[nl].client_ord != client_ord) {
			SAM_COUNT64(shared_server, callout_action);
			sam_proc_callout(ip, CALLOUT_action, actions,
			    llp->lease[nl].client_ord, NULL);
		}
	}
}


/*
 * Notify all clients to set ABR flag
 */
void
sam_callout_abr(sam_node_t *ip, int client_ord)
{
	struct sam_lease_ino *llp;
	int nl;

	ASSERT(RW_READ_HELD(&ip->inode_rwl) || RW_WRITE_HELD(&ip->inode_rwl));
	/*
	 * Refresh abr flag for this file on all clients who have leases.
	 */
	mutex_enter(&ip->ilease_mutex);
	llp = ip->sr_leases;
	if (llp != NULL) {
		for (nl = 0; nl < llp->no_clients; nl++) {
			if (llp->lease[nl].client_ord != client_ord &&
			    !llp->lease[nl].shflags.b.abr) {
				SAM_COUNT64(shared_server, callout_abr);
				(void) sam_proc_callout(ip, CALLOUT_flags,
				    SR_ABR_ON,
				    llp->lease[nl].client_ord, NULL);
			}
		}
	}
	mutex_exit(&ip->ilease_mutex);
}


/*
 * ----- sam_callout_directio - callout for server directed actions.
 * If the current owner of the WRITE|APPEND lease is not in directio,
 * callout to the owner to flush pages and enter directio.  The
 * owner replies with BLOCK_wakeup to wake up clients waiting for
 * multi-host write.
 */

static boolean_t
sam_callout_directio(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	sam_lease_ino_t *llp)		/* the lease entry for this inode */
{
	boolean_t wait_lease = FALSE;
	int nl;

	/*
	 * If directio and abr have already been set do not repeat
	 * callout.
	 */
	for (nl = 0; nl < llp->no_clients; nl++) {
		if (llp->lease[nl].shflags.b.directio) {
			llp->lease[nl].actions &= ~SR_DIRECTIO_ON;
			continue;
		}
		if ((llp->lease[nl].client_ord != client_ord) &&
		    (llp->lease[nl].leases & (CL_READ|CL_WRITE|CL_APPEND))) {
			SAM_COUNT64(shared_server, callout_directio);
			llp->lease[nl].actions |= SR_DIRECTIO_ON;
			sam_proc_callout(ip, CALLOUT_flags, SR_DIRECTIO_ON,
			    llp->lease[nl].client_ord, NULL);
			llp->lease[nl].actions &= ~SR_DIRECTIO_ON;
			wait_lease = TRUE;
		}
	}
	return (wait_lease);
}


/*
 * ----- sam_remove_lease - remove the lease in the linked list.
 *
 * Close removes all leases for this file on this client (ord).
 * If any other clients are waiting, callout.
 * Note: inode_rwl lock set on entry and exit.
 */

static int			/* ERRNO, else 0 if successful. */
sam_remove_lease(
	sam_node_t *ip,		/* Pointer to inode */
	int client_ord,		/* Client ordinal */
	sam_sr_attr_t *ap,	/* server file attribute returned params */
	int lease_mask,		/* Lease masks(s) to be removed */
	uint32_t *gen_table)	/* Gen numbers of leases being removed, */
				/* or NULL. */
{
	sam_mount_t *mp = ip->mp;
	sam_lease_ino_t *llp;
	boolean_t write_lease_remains = FALSE;
	boolean_t append_lease_remains = FALSE;
	boolean_t removed_append_lease = FALSE;
	boolean_t removed_truncate_lease = FALSE;
	boolean_t removed_exclusive_lease = FALSE;
	boolean_t last_lease = TRUE;
	int no_clients;
	int nl;
	int append_client;
	int error = 0;
	const int SAM_EXPIRE_DELAY_SECS = 30;

	TRACE(T_SAM_LEASE_REMOVE, SAM_ITOV(ip), ip->di.id.ino,
	    client_ord, lease_mask);

	/*
	 * Clear the lease for this client ordinal.
	 * Notify clients who are waiting for a lease on this file.
	 * Note that notify_ord is used to round-robin the NOTIFY_lease.
	 * XXX - Could optimize the generation-mismatch case to avoid
	 * notifying other clients.
	 */
	mutex_enter(&ip->ilease_mutex);
	if (ip->sr_leases) {
		llp = ip->sr_leases;
		no_clients = llp->no_clients;
		nl = llp->notify_ord;
		if (nl >= no_clients) {
			nl = 0;
		}
		if (++llp->notify_ord >= no_clients) {
			llp->notify_ord = 0;
		}
		while (no_clients) {
			if (llp->lease[nl].client_ord == client_ord) {
				if (gen_table != NULL) {
					int i;

					for (i = 0; i < SAM_MAX_LTYPE; i++) {
						if (lease_mask & (1 << i)) {
							if (gen_table[i] !=
							    llp->lease[
							    nl].gen[i]) {
								lease_mask &=
								    ~(1 << i);
							}
						}
					}
				}

				/*
				 * For this client, remove WAIT actions.
				 * However, if removing a frlock, set
				 * SR_NOTIFY_FRLOCK to wake up any possible
				 * frlock waiters.
				 */
				ap->actions |= (llp->lease[nl].actions &
						~SR_WAIT_LEASE);
				if (lease_mask & CL_FRLOCK) {
					ap->actions |= SR_NOTIFY_FRLOCK;
				}
				removed_append_lease =
					((lease_mask & llp->lease[nl].leases &
					CL_APPEND) != 0);
				if ((lease_mask & llp->lease[nl].leases &
					CL_WRITE) != 0) {
					ap->actions |= SR_INVAL_PAGES;
				}
				removed_truncate_lease =
					((lease_mask & llp->lease[nl].leases &
					CL_TRUNCATE) != 0);
				removed_exclusive_lease =
				    ((lease_mask & llp->lease[nl].leases &
				    CL_EXCLUSIVE) != 0);

				llp->lease[nl].wt_leases &= ~lease_mask;
				llp->lease[nl].leases &= ~lease_mask;
			}

			/*
			 * XXX - We may issue NOTIFY_lease before the lease has
			 * been removed due to the client lease array order.
			 */
			if (llp->lease[nl].actions & SR_WAIT_LEASE) {
				llp->lease[nl].actions &= ~SR_WAIT_LEASE;
				(void) sam_proc_notify(ip,
					NOTIFY_lease,
					llp->lease[nl].client_ord, NULL, 0);
			}
			if (llp->lease[nl].leases != 0) {
				last_lease = FALSE;
				if (llp->lease[nl].leases & CL_APPEND) {
					append_lease_remains = TRUE;
					append_client = nl;
				}
				if (llp->lease[nl].leases & CL_WRITE) {
					write_lease_remains = TRUE;
				}
			}
			if (++nl >= llp->no_clients) {
				nl = 0;
			}
			no_clients--;
		}
	}
	mutex_exit(&ip->ilease_mutex);

	/*
	 * If we removed a write lease, clear write_owner.
	 */
	if (!write_lease_remains) {
		ip->write_owner = 0;
	}

	/*
	 * If we removed an append lease, truncate excess blocks down to file
	 * size.
	 */
	if (removed_append_lease) {
		if (!append_lease_remains) {
			ip->size_owner = 0;
			ap->actions |= (SR_STALE_INDIRECT|SR_INVAL_PAGES);
			if (!ip->flags.b.staging) {
				error = sam_trunc_excess_blocks(ip);
			}
		} else {
			/* XXX should we ASSERT that this doesn't happen? */
			cmn_err(CE_WARN,
				"SAM-QFS: %s: Multiple append leases "
				"detected on "
					"%d.%d (%d, %d)",
				ip->mp->mt.fi_name, ip->di.id.ino,
				ip->di.id.gen,
				client_ord, append_client);
		}
	}

	/*
	 * Stale indirect blocks and invalidate pages after removing a
	 * truncate, append, or exclusive lease. Also set sr_sync flag
	 * to write inode to disk. The server may panic and we would lose
	 * this inode update.
	 */
	if (removed_truncate_lease || removed_append_lease ||
	    removed_exclusive_lease) {
		mutex_enter(&ip->ilease_mutex);
		if (ip->sr_leases) {
			llp = ip->sr_leases;
			sam_callout_actions(ip, client_ord,
				(SR_STALE_INDIRECT|SR_INVAL_PAGES), llp);
		}
		mutex_exit(&ip->ilease_mutex);
		ip->flags.b.changed = 1;
		ip->sr_sync = 1;
	}

	/*
	 * If last lease & inode modified/changed, set sr_sync to sync inode to
	 * disk. Schedule a task to process leases if we might have removed the
	 * last lease on this file. If the file was removed, schedule the
	 * task now to clean up -- otherwise an orphan can result during
	 * involuntary failover. (If we've removed the last lease for
	 * this client, but other clients still have leases, don't bother
	 * scheduling a task; we'll clean up eventually.)
	 */
	if (last_lease) {
		boolean_t force;
		clock_t delay;

		TRACE(T_SAM_ABR_CLR, SAM_ITOV(ip), ip->di.id.ino,
			no_clients, nl);
		ip->flags.b.abr = 0;
		if (ip->flags.bits & (SAM_UPDATED | SAM_CHANGED)) {
			ip->flags.b.changed = 1;
			ip->sr_sync = 1;
		}
		if (ip->di.nlink > 0) {
			force = FALSE;
			delay = SAM_EXPIRE_DELAY_SECS * hz;
		} else {
			force = TRUE;
			delay = 0;
		}
		sam_sched_expire_server_leases(mp, delay, force);
	}
	return (error);
}


/*
 * ----- sam_extend_lease - extend the lease in the linked list.
 *
 * Note: inode_rwl lock set on entry and exit.
 */

static int			/* ERRNO, else 0 if successful. */
sam_extend_lease(
	sam_node_t *ip,		/* Pointer to inode */
	int client_ord,		/* Client ordinal */
	int lease_mask,		/* Lease mask to be extended */
	sam_san_lease2_t *l2p)	/* Lease returned parameters */
{
	sam_mount_t *mp = ip->mp;
	sam_lease_ino_t *llp;
	uint32_t interval;
	int i, index;
	int extended_leases;
	int remaining_leases;
	int wt_leases;
	int error = 0;
	enum LEASE_type ltype;

	TRACE(T_SAM_LEASE_EXTEND, SAM_ITOV(ip), ip->di.id.ino, client_ord,
	    lease_mask);

	/*
	 * Currently only support extending a single lease in the mask.
	 */
	for (ltype = LTYPE_read; ltype < SAM_MAX_LTYPE; ltype++) {
		if (lease_mask & (1 << ltype)) {
			remaining_leases = (lease_mask & ~(1 << ltype));
			ASSERT(remaining_leases == 0);
			break;
		}
	}

	if (ltype < MAX_EXPIRING_LEASES) {
		interval = l2p->inp.data.interval[ltype];
	} else {
		interval = MAX_LEASE_TIME;
	}

	/*
	 * Extend the lease for this client ordinal if it still exists
	 * and no other client is waiting for a lease.  Note  that we
	 * do not perform lease relinquish for the case where a read
	 * lease is requested and a different client has an mmap write
	 * lease.
	 */
	index = -1;
	wt_leases = 0;
	extended_leases = 0;
	mutex_enter(&ip->ilease_mutex);
	if (ip->sr_leases) {
		llp = ip->sr_leases;
		for (i = 0; i < llp->no_clients; i++) {
			if (llp->lease[i].client_ord == client_ord) {
				index = i;
			} else {
				wt_leases |= llp->lease[i].wt_leases;
			}
		}
		if (index >= 0) {
			if (!wt_leases &&
			    (llp->lease[index].leases & lease_mask)) {
				llp->lease[index].time[ltype] = lbolt +
				    (sam_calculate_lease_timeout(interval) *
				    hz);
				extended_leases = lease_mask;
			}
		}
	}
	mutex_exit(&ip->ilease_mutex);

	/*
	 * Schedule server lease reclamation if extending a lease.
	 */
	if (extended_leases) {
		sam_sched_expire_server_leases(mp,
		    sam_calculate_lease_timeout(interval) * hz, FALSE);
	} else {
		error = ECANCELED;
	}
	TRACE(T_SAM_LEASE_EXTEND_RET, SAM_ITOV(ip), ip->di.id.ino, client_ord,
	    extended_leases);

	return (error);
}


#define	SAM_ALLOC_BLK_NUM	32	/* Allocate in 32 block segments */

/*
 * -----	sam_client_allocate_append - Allocate the file for append.
 * Set the flag cl_hold_blocks. This keeps the file from being truncated
 * to the file size until the client close.
 * Set SR_STALE_INDIRECT to stale the indirect blocks for this inode on
 * the client.
 */

static int				/* ERRNO, else 0 if successful. */
sam_client_allocate_append(sam_node_t *ip, sam_san_message_t *msg)
{
	sam_san_lease_t *lp = &msg->call.lease;
	sam_san_lease2_t *l2p = &msg->call.lease2;
	sam_lease_data_t *dp = &l2p->inp.data;
	offset_t offset, size, asize, allocsz;
	cred_t *credp;
	int error = 0;

	if (S_ISREG(ip->di.mode)) {
		/*
		 * If new file and request is >= one large DAU, set on
		 * large DAU flag
		 */
		if (!ip->di.status.b.on_large &&
		    (ip->di.blocks == 0) &&
		    (dp->offset == 0) &&
		    (dp->resid >= LG_BLK(ip->mp, ip->di.status.b.meta))) {
			ip->di.status.b.on_large = 1;
		}

		/*
		 * Page align buffer because of small allocation (4K) md
		 * devices.
		 */
		allocsz = (dp->alloc_unit + dp->resid + PAGESIZE - 1) &
		    PAGEMASK;
		offset = dp->offset & PAGEMASK;
		if ((offset + allocsz) > ip->cl_allocsz) {
			ip->cl_hold_blocks = 1;
			size = (LG_BLK(ip->mp, ip->di.status.b.meta)) *
			    SAM_ALLOC_BLK_NUM;
			if (size > allocsz) {
				size = allocsz;
			}
			credp = sam_getcred(&lp->cred);
			for (asize = 0; asize < allocsz; asize += size) {
				RW_UNLOCK_CURRENT_OS(&ip->inode_rwl);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (ip->cl_hold_blocks == 0) {
					break;
				}
				error = sam_map_block(ip, offset, size,
				    SAM_ALLOC_BLOCK, NULL, credp);
				if (error == EDQUOT) {
					/*
					 * Client may not have wanted
					 * this much.
					 * Try actual size of write.
					 */
					allocsz =
					    (dp->resid + PAGESIZE - 1) &
					    PAGEMASK;
					if (size <= allocsz) {
						break;
					}
					size = allocsz;
					if ((error = sam_map_block(ip, offset,
					    size, SAM_ALLOC_BLOCK,
					    NULL, credp)) != 0) {
						break;
					}
				} else if (error != 0) {
					break;
				}
				offset += size;
			}
			crfree(credp);
			l2p->irec.sr_attr.actions |= SR_STALE_INDIRECT;
			l2p->irec.sr_attr.offset = dp->offset;
			ip->flags.b.updated = 1;
			(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
		}
	}
	return (error);
}


/*
 * ----- sam_server_truncate -
 *	Allocate ahead if expanding past allocated size.
 *	Call truncate if expanding the file to clear pages.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_server_truncate(
	sam_node_t *ip,			/* inode pointer */
	sam_san_lease2_t *l2p,		/* the lease returned parameters */
	cred_t *credp)
{
	sam_lease_data_t *dp = &l2p->inp.data;
	int64_t length = dp->resid;
	int32_t tflag = dp->lflag;
	int error;

	error = 0;
	if (ip->di.status.b.offline) {
		error = sam_clear_ino(ip, length, MAKE_ONLINE, credp);
	}
	if (error == 0) {
		ip->cl_hold_blocks = 0;
		if (!(error = sam_proc_truncate(ip, length, tflag, credp))) {
			if (tflag != SAM_RELEASE) {
				ip->di.rm.size = ip->size;
				if (ip->di.rm.size == 0) {
					ip->di.status.b.offline = 0;
					ip->di.status.b.stage_failed = 0;
				}
				TRANS_INODE(ip->mp, ip);
				sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
			}
		}
		ip->flags.b.changed = 1;
		sam_flush_pages(ip, 0);
		l2p->irec.sr_attr.actions |= (SR_STALE_INDIRECT|SR_INVAL_PAGES);
		l2p->irec.sr_attr.offset = length;
	}
	return (error);
}


/*
 * -----	sam_get_name_request - system shared server name command.
 *	Called when the shared client issues a NAME command.
 */

static int				/* ERRNO, else 0 if successful. */
sam_get_name_request(
	sam_mount_t *mp,		/* Pointer to mount table */
	sam_san_message_t *msg,		/* Current message */
	sam_id_t *id)			/* New inode id */
{
	sam_san_name_t *np = &msg->call.name;
	sam_san_name2_t *n2p;
	sam_node_t *pip;
	int error = 0;

	n2p = (sam_san_name2_t *)np;
	if ((error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS, &np->parent_id,
	    &pip)) == 0) {
		TRACE(T_SAM_SR_NAME, SAM_ITOV(pip), msg->hdr.client_ord,
		    msg->hdr.operation, pip->di.id.ino);
		if (id == NULL) {
			error = sam_process_name_request(pip, msg);
		} else {
			if (id->ino != 0) {
				n2p->new_id = *id;
			} else {
				n2p->new_id.ino = 0;
				n2p->new_id.gen = 0;
			}
			if (error == 0) {
				error = msg->hdr.error;
			}
		}
		if (error == 0) {
			sam_set_sr_ino(pip, &n2p->parent_id, &n2p->prec);
		}
		VN_RELE(SAM_ITOV(pip));
	}
	return (error);
}


/*
 * -----	sam_process_name_request - system shared server name command.
 *	Called when the shared client issues NAME command. Note, the
 *	server host directly calls the vnops to process a NAME command.
 */

/*
 * The compiler will not overlap the various structures found in disjoint
 * blocks with optimization enabled.  To reduce stack usage,
 * the 'vattr', 'name', and 'vsecattr' variables have been combined into
 * the 'v' union.
 */

static int		/* ERRNO if an error occurred, 0 if successful. */
sam_process_name_request(
	sam_node_t *pip,		/* Pointer to parent inode */
	sam_san_message_t *msg)
{
	sam_san_name_t *np = &msg->call.name;
	sam_san_name2_t *n2p;
	sam_node_t *ip;
	int client_ord;
	cred_t *cred;
	int error = 0;

	union {				/* See comment re compiler bug above. */
		vattr_t vattr;
		struct sam_name name;
		vsecattr_t vsecattr;
	} v;

	client_ord = msg->hdr.client_ord;
	n2p = (sam_san_name2_t *)np;
	cred = sam_getcred(&np->cred);
	switch (msg->hdr.operation) {

	case NAME_create: {
		vnode_t *vp;

		sam_v32_to_v(&np->arg.p.create.vattr, &v.vattr);
		/*
		 * LQFS transaction block is in sam_create_ino()
		 */
		if ((error = sam_create_ino(pip, np->component, &v.vattr,
		    (vcexcl_t)np->arg.p.create.ex, np->arg.p.create.mode, &vp,
		    cred, np->arg.p.create.flag)) == 0) {
			ip = SAM_VTOI(vp);
			sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
			VN_RELE(SAM_ITOV(ip));	/* Inactivate the new inode. */
		}
		}
		break;

	case NAME_remove: {
		vnode_t *rmvp = NULL;
		vnode_t *pvp = SAM_ITOV(pip);
		int issync;
		int trans_size;

		trans_size = (int)TOP_REMOVE_SIZE(pip);
		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_REMOVE, trans_size);
		RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
		v.name.operation = SAM_REMOVE;
		v.name.client_ord = client_ord;
		TRACES(T_SAM_REMOVE, pvp, np->component);
		if ((error = sam_lookup_name(pip, np->component, &ip, &v.name,
		    cred)) == 0) {
			/*
			 * Entry exists, remove it.
			 */
			error = sam_remove_name(pip, np->component, ip,
			    &v.name, cred);
			sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
			rmvp = SAM_ITOV(ip);
		}
		RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
		TRANS_END_CSYNC(pip->mp, error, issync, TOP_REMOVE, trans_size);
		if (rmvp != NULL) {
			if (error == 0) {
				VNEVENT_REMOVE_OS(rmvp, pvp, np->component,
				    NULL);
			}
			VN_RELE(rmvp);
		}
		}
		break;

	case NAME_link:
		if ((error = sam_get_ino(pip->mp->mi.m_vfsp, IG_EXISTS,
		    &np->arg.p.link.id, &ip)) == 0) {
			/*
			 * LQFS transaction block is in sam_link_vn()
			 */
#if defined(SOL_511_ABOVE)
			error = sam_link_vn(SAM_ITOV(pip), SAM_ITOV(ip),
			    np->component, cred, NULL, 0);
#else
			error = sam_link_vn(SAM_ITOV(pip), SAM_ITOV(ip),
			    np->component, cred);
#endif
			if (error == 0) {
				sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
			}
			VN_RELE(SAM_ITOV(ip));	/* Inactivate the new inode. */
		}
		break;

	case NAME_mkdir: {
		vnode_t *vp;

		sam_v32_to_v(&np->arg.p.mkdir.vattr, &v.vattr);
		/*
		 * LQFS transaction block is in sam_mkdir_vn()
		 */
#if defined(SOL_511_ABOVE)
		if ((error = sam_mkdir_vn(SAM_ITOV(pip), np->component,
		    &v.vattr, &vp, cred, NULL, 0, NULL)) == 0) {
#else
		if ((error = sam_mkdir_vn(SAM_ITOV(pip), np->component,
		    &v.vattr, &vp, cred)) == 0) {
#endif
			ip = SAM_VTOI(vp);
			sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
			VN_RELE(vp);		/* Inactivate the new inode. */
		}
		}
		break;

	case NAME_rmdir: {
		vnode_t *rmvp = NULL;
		vnode_t *pvp = SAM_ITOV(pip);
		int issync;
		int terr = 0;

		TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_RMDIR, TOP_RMDIR_SIZE);
		RW_LOCK_OS(&pip->data_rwl, RW_WRITER);
		v.name.operation = SAM_RMDIR;
		TRACES(T_SAM_RMDIR, pvp, np->component);
		ip = NULL;
		if ((error = sam_lookup_name(pip, np->component, &ip, &v.name,
		    cred)) == 0) {
			/*
			 * Entry exists.  Check the locking order.
			 */
			if (pip->di.id.ino > ip->di.id.ino) {
				sam_node_t *ip2;

				RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
go_fish:
				sam_dlock_two(pip, ip, RW_WRITER);
				bzero(&v, sizeof (v));
				v.name.operation = SAM_RMDIR;
				if ((error = sam_lookup_name(pip, np->component,
				    &ip2, &v.name, cred))) {
					/*
					 * Entry was removed while the lock
					 * was released.
					 */
					goto lost_it;
				}
				/*
				 * If this name gives us a new inode then we
				 * have to start over.
				 */
				if ((ip->di.id.ino != ip2->di.id.ino) ||
				    (ip->di.id.gen != ip2->di.id.gen)) {
					sam_dunlock_two(pip, ip, RW_WRITER);
					VN_RELE(SAM_ITOV(ip));
					ip = ip2;
					goto go_fish;
				}
				VN_RELE(SAM_ITOV(ip2));
			} else {
				RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
			}

			v.name.client_ord = client_ord;
			error = sam_remove_name(pip, np->component,
			    ip, &v.name, cred);
			if (error == 0) {
				rmvp = SAM_ITOV(ip);
			}
			sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
		}
lost_it:
		sam_dunlock_two(pip, ip ? ip : pip, RW_WRITER);
		TRANS_END_CSYNC(pip->mp, terr, issync, TOP_RMDIR,
		    TOP_RMDIR_SIZE);
		if (rmvp) {
			VNEVENT_RMDIR_OS(rmvp, pvp, np->component, NULL);
		}
		if (ip) {
			VN_RELE(SAM_ITOV(ip));
		}
		if (error == 0) {
			error = terr;
		}
		}
		break;

	case NAME_rename: {
		vnode_t *realvp, *npvp;
		sam_node_t *npip;
		int pass = 0;

retry:
		if ((error = sam_get_ino(pip->mp->mi.m_vfsp, IG_EXISTS,
		    &np->arg.p.rename.new_parent_id, &npip)) == 0) {
			TRACES(T_SAM_RENAME1, SAM_ITOV(pip), np->component);
			npvp = SAM_ITOV(npip);
			if (VOP_REALVP_OS(npvp, &realvp, NULL) == 0) {
				npvp = realvp;
			}
			TRACES(T_SAM_RENAME2, npvp, &np->component[MAXNAMELEN]);
			/*
			 * LQFS transaction block is in sam_rename_ino()
			 */
			if ((error = sam_rename_inode(pip, np->component,
			    SAM_VTOI(npvp),
			    &np->component[MAXNAMELEN], &ip, cred)) == 0) {
				sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
				/*
				 * Inactivate the inode of the renamed file.
				 */
				VN_RELE(SAM_ITOV(ip));
			}
			/*
			 * Inactivate the new parent inode.
			 */
			VN_RELE(SAM_ITOV(npip));
			if (error == EDEADLK) {
				if (pass >= 10) {
					pass += 4;
				} else {
					pass++;
				}
				delay(pass);
				goto retry;
			}
		}
		}
		break;

	case NAME_symlink:
		sam_v32_to_v(&np->arg.p.symlink.vattr, &v.vattr);
		/*
		 * LQFS transaction block is in sam_symlink_vn()
		 */
#if defined(SOL_511_ABOVE)
		if ((error = sam_symlink_vn(SAM_ITOV(pip), np->component,
		    &v.vattr, &np->component[MAXNAMELEN], cred, NULL, 0))
		    == 0) {
#else
		if ((error = sam_symlink_vn(SAM_ITOV(pip), np->component,
		    &v.vattr, &np->component[MAXNAMELEN], cred)) == 0) {
#endif
			sam_node_t *ip;

			error = sam_lookup_name(pip, np->component,
			    &ip, NULL, cred);
			if (error == 0) {
				sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
				/* Inactivate the new inode. */
				VN_RELE(SAM_ITOV(ip));
			}
		}
		break;

	case NAME_acl: {
		aclent_t *entp;

		bzero(&v.vsecattr, sizeof (v.vsecattr));

		if (np->arg.p.acl.set) {
			/*
			 * Client requested set_vsecattr
			 */
			ASSERT(offsetof(sam_san_name_t, component) +
			    (np->arg.p.acl.aclcnt + np->arg.p.acl.dfaclcnt) *
			    sizeof (aclent_t) <= msg->hdr.length);
			v.vsecattr.vsa_mask = np->arg.p.acl.mask;
			entp = (aclent_t *)(void *)&np->component[0];
			if (np->arg.p.acl.aclcnt) {
				v.vsecattr.vsa_aclcnt = np->arg.p.acl.aclcnt;
				v.vsecattr.vsa_aclentp = entp;
			}
			if (np->arg.p.acl.dfaclcnt) {
				v.vsecattr.vsa_dfaclcnt =
				    np->arg.p.acl.dfaclcnt;
				v.vsecattr.vsa_dfaclentp =
				    entp + v.vsecattr.vsa_aclcnt;
			}
			/*
			 * LQFS transaction block is in sam_setsecattr_vn()
			 */
#if defined(SOL_511_ABOVE)
			if (error = sam_setsecattr_vn(SAM_ITOV(pip),
			    &v.vsecattr, 0, cred, NULL)) {
#else
			if (error = sam_setsecattr_vn(SAM_ITOV(pip),
			    &v.vsecattr, 0, cred)) {
#endif
				break;
			}
			sam_callout_acl(pip, client_ord);
		}
		/*
		 * return secattrs.
		 */
		bzero(&v.vsecattr, sizeof (v.vsecattr));
		v.vsecattr.vsa_mask = VSA_DFACLCNT | VSA_ACLCNT |
		    VSA_DFACL | VSA_ACL;
#if defined(SOL_511_ABOVE)
		if ((error = sam_getsecattr_vn(SAM_ITOV(pip),
		    &v.vsecattr, 0, cred, NULL)) != 0) {
#else
		if ((error = sam_getsecattr_vn(SAM_ITOV(pip),
		    &v.vsecattr, 0, cred)) != 0) {
#endif
			msg->hdr.out_length = offsetof(sam_san_name_t,
			    component[0]);
			break;
		}
		n2p->arg.p.acl.aclcnt = v.vsecattr.vsa_aclcnt;
		n2p->arg.p.acl.dfaclcnt = v.vsecattr.vsa_dfaclcnt;

		entp = (aclent_t *)(void *)&n2p->component[0];
		bcopy(v.vsecattr.vsa_aclentp, entp,
		    v.vsecattr.vsa_aclcnt * sizeof (aclent_t));
		bcopy(v.vsecattr.vsa_dfaclentp, entp + v.vsecattr.vsa_aclcnt,
		    v.vsecattr.vsa_dfaclcnt * sizeof (aclent_t));
		msg->hdr.out_length =
		    offsetof(sam_san_name2_t, component[0]) +
		    (v.vsecattr.vsa_aclcnt + v.vsecattr.vsa_dfaclcnt) *
		    sizeof (aclent_t);
		/*
		 * Free kmem_alloc'd acl/dfacl entries
		 */
		if (v.vsecattr.vsa_aclcnt) {
			kmem_free(v.vsecattr.vsa_aclentp,
			    v.vsecattr.vsa_aclcnt * sizeof (aclent_t));
		}
		if (v.vsecattr.vsa_dfaclcnt) {
			kmem_free(v.vsecattr.vsa_dfaclentp,
			    v.vsecattr.vsa_dfaclcnt * sizeof (aclent_t));
		}
		}
		break;

	case NAME_lookup: {
		int flags = np->arg.p.lookup.flags;
		int itrc;

		if ((flags & (LOOKUP_XATTR|CREATE_XATTR_DIR)) != 0) {
			vnode_t *vp;

			itrc = T_SAM_SR_LOOKX_RET;
			if ((error = sam_lookup_xattr(SAM_ITOV(pip), &vp, flags,
			    cred)) == 0) {
				ip = SAM_VTOI(vp);
			}
		} else  {
			itrc = T_SAM_SR_LOOK_RET;
			error = sam_lookup_name(pip, np->component,
			    &ip, NULL, cred);
		}
		if (error == 0) {
			sam_set_sr_ino(ip, &n2p->new_id, &n2p->nrec);
			TRACE(itrc, SAM_ITOV(ip), ip->di.id.ino, ip->di.id.gen,
			    ip->di.rm.size);
			VN_RELE(SAM_ITOV(ip));
		} else {
			TRACE(T_SAM_SR_LOOK_ERR, SAM_ITOV(pip), flags, error,
			    0);
		}
		}
		break;

	default:
		error = EPROTO;
		break;
	}
	crfree(cred);
	return (error);
}


/*
 * ----- sam_callout_acl
 *
 *	Called to send out a callout acl action.
 */

void
sam_callout_acl(sam_node_t *ip, int client_ord)
{
	int ord;
	client_entry_t *client;
	sam_mount_t *mp = ip->mp;

	if (mp->ms.m_clienti == NULL) {
		return;
	}

	/*
	 * Refresh acl for this file on all mounted clients.
	 */
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
		client = sam_get_client_entry(mp, ord, 0);
		if (client != NULL) {
			if ((client->cl_status & FS_MOUNTED) &&
			    (ord != client_ord)) {
				SAM_COUNT64(shared_server, callout_acl);
				(void) sam_proc_callout(ip, CALLOUT_acl, 0,
				    ord, NULL);
			}
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
}


/*
 * ----- sam_setup_stage
 *
 *	Called when the shared server must get a lease for an offline file.
 */

int					/* ERRNO, else 0 if successful. */
sam_setup_stage(sam_node_t *ip, cred_t *credp)
{
	sam_san_lease_msg_t *msg;
	sam_san_lease_t *lp;
	sam_san_lease2_t *l2p;
	int error = 0;

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	lp = &msg->call.lease;
	l2p = &msg->call.lease2;

	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_LEASE, SHARE_wait,
	    LEASE_get, sizeof (sam_san_lease_t), sizeof (sam_san_lease2_t));
	lp->id = ip->di.id;

	sam_set_cl_attr(ip, 0, &lp->cl_attr, TRUE, TRUE);

	bzero(&lp->data, sizeof (sam_lease_data_t));
	lp->data.ltype = LTYPE_stage;
	lp->data.resid = ip->di.rm.size;	/* Allocate file size */
	if (ip->di.blocks == 0) {
		if ((error = sam_set_unit(ip->mp, &(ip->di)))) {
			kmem_free(msg, sizeof (*msg));
			return (error);
		}
	}

	if ((error = sam_process_lease_request(ip,
	    (sam_san_message_t *)msg)) == 0) {
		sam_directed_actions(ip, l2p->irec.sr_attr.actions,
		    l2p->irec.sr_attr.offset, credp);
	}
	sam_set_size(ip);
	ip->flags.b.stage_directio = 1;	/* Shared always uses direct I/O */
	kmem_free(msg, sizeof (*msg));
	return (error);
}


/*
 * ----- sam_v32_to_v - convert 32 vattr to vattr.
 */

static void
sam_v32_to_v(sam_vattr_t *va32p, vattr_t *vap)
{
	bzero((void *)vap, sizeof (vattr_t));
	vap->va_mask = va32p->va_mask;		/* Bit-mask of attributes */
	vap->va_mode = va32p->va_mode;		/* File access mode */

	vap->va_uid  = va32p->va_uid;		/* User id */
	vap->va_gid  = va32p->va_gid;		/* Group id */
	vap->va_nlink = va32p->va_nlink;	/* Num references to file */
	vap->va_size = va32p->va_rsize;		/* File size in bytes */
	vap->va_nblocks = va32p->va_nblocks;	/* # of blocks allocated */

	SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_atime, &va32p->va_atime);
	SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_mtime, &va32p->va_mtime);
	SAM_TIMESPEC32_TO_TIMESPEC(&vap->va_ctime, &va32p->va_ctime);

	vap->va_type = (vtype_t)va32p->va_type;	/* vnode type (for create) */
	if (vap->va_type == VBLK || vap->va_type == VCHR) {
		vap->va_rdev = va32p->va_rdev;
	}
}


/*
 * -----	sam_get_inode_request - system shared server inode command.
 *	Called when the shared client issues a INODE command.
 */

static int				/* ERRNO, else 0 if successful. */
sam_get_inode_request(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_inode_t *inp = &msg->call.inode;
	sam_san_inode2_t *i2p = &msg->call.inode2;
	sam_node_t *ip;
	int error = 0;
	int issync;
	int trans_size;
	int dotrans = 0;

	if ((error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS,
	    &inp->id, &ip)) == 0) {
		TRACE(T_SAM_SR_INODE, SAM_ITOV(ip), msg->hdr.client_ord,
		    msg->hdr.operation, ip->di.rm.size);

		switch (msg->hdr.operation) {
		case INODE_setattr:
		case INODE_samaid:
			trans_size = (int)TOP_SETATTR_SIZE(ip);
			TRANS_BEGIN_CSYNC(ip->mp, issync, TOP_SETATTR,
			    trans_size);
			dotrans++;
			break;
		default:
			break;
		}

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_update_cl_attr(ip, msg->hdr.client_ord, &inp->cl_attr);

		/*
		 * Process lease message. Current leases set in
		 * sam_process_inode_request and returned in
		 * i2p->irec.sr_attr.leases.
		 */
		error = sam_process_inode_request(ip, msg->hdr.client_ord,
		    msg->hdr.operation, inp);

		/*
		 * Set all server attributes except the leases.
		 */
		sam_set_sr_attr(ip, i2p->irec.sr_attr.actions, &i2p->irec);
		(void) sam_update_inode_buffer_rw(ip);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		if (dotrans) {
			int terr = 0;

			TRANS_END_CSYNC(ip->mp, terr, issync, TOP_SETATTR,
			    trans_size);

			if (error == 0) {
				error = terr;
			}
		}

		TRACE(T_SAM_SR_INODE_RET, SAM_ITOV(ip),
		    i2p->irec.sr_attr.current_size,
		    i2p->irec.sr_attr.actions, error);

		VN_RELE(SAM_ITOV(ip));
	}
	return (error);
}


/*
 * -----	sam_process_inode_request - system shared server inode command.
 *	Called when the shared server receives a INODE operation or
 *  when the server host directly calls this function.
 */

int					/* ERRNO, else 0 if successful. */
sam_process_inode_request(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal */
	ushort_t op,			/* Inode operation */
	sam_san_inode_t *inp)
{
	sam_inode_arg_t *arg = &inp->arg;
	sam_san_inode2_t *i2p;
	cred_t *cred;
	int error = 0;


	sam_open_operation_nb(ip->mp);

	i2p = (sam_san_inode2_t *)inp;
	cred = sam_getcred(&inp->cred);

	/*
	 * Clear actions and stale indirect offset.
	 */
	i2p->irec.sr_attr.actions = 0;
	i2p->irec.sr_attr.offset = 0;

	switch (op) {
	case INODE_getino:
		if ((ip->mp->mt.fi_config & MT_CONSISTENT_ATTR) &&
		    !S_ISDIR(ip->di.mode)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			error = sam_client_getino(ip, client_ord);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		break;

	case INODE_fsync_nowait:
		mutex_enter(&ip->ilease_mutex);
		ip->waiting_getino = FALSE;
		if (ip->getino_waiters) {
			cv_broadcast(&ip->ilease_cv);
		}
		mutex_exit(&ip->ilease_mutex);
		/* LINTED [fallthrough on case statement] */
	case INODE_fsync_wait:
		(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
		break;

	case INODE_setattr: {
		vattr_t vattr;

		sam_v32_to_v(&arg->p.setattr.vattr, &vattr);
		error = sam_setattr_ino(ip, &vattr,
		    (int)arg->p.setattr.flags, cred);
		}
		break;

	case INODE_stage:
		ip->copy = arg->p.stage.copy;
		ip->flags.b.stage_n = 0;
		error = sam_proc_offline(ip, (offset_t)0, arg->p.stage.len,
		    NULL,
		    cred, NULL);
		if (!ip->flags.b.staging || !ip->di.status.b.offline)  {
			ip->flags.b.stage_n = ip->di.status.b.direct;
			sam_notify_staging_clients(ip);
		}
		break;

	case INODE_cancel_stage:
		error = sam_remove_lease(ip, client_ord,
		    &i2p->irec.sr_attr, CL_STAGE,
		    NULL);
		(void) sam_cancel_stage(ip, EINTR, CRED());
		break;

	case INODE_samattr:
		error = sam_set_file_operations(ip, arg->p.samattr.cmd,
		    arg->p.samattr.ops, cred);
		break;

	case INODE_samarch: {
		struct sam_archive_copy_arg params;

		params.operation = arg->p.samarch.operation;
		params.flags = arg->p.samarch.flags;
		params.media = arg->p.samarch.media;
		params.copies = arg->p.samarch.copies;
		params.ncopies = arg->p.samarch.ncopies;
		params.dcopy = arg->p.samarch.dcopy;
		error = sam_proc_archive_copy(SAM_ITOV(ip), SC_archive_copy,
		    &params, cred);
		}
		break;

	case INODE_samaid:
		error = sam_set_aid(ip, &arg->p.samaid);
		break;

	case INODE_putquota:
		error = sam_quota_set_info(ip, &arg->p.quota.quota,
		    arg->p.quota.operation, arg->p.quota.type,
		    arg->p.quota.index);
		break;

	case INODE_setabr:
		TRACE(T_SAM_ABR_2SRVR, SAM_ITOP(ip), ip->di.id.ino,
		    0, ip->flags.b.abr);
		if (!SAM_IS_SHARED_SERVER(ip->mp) || ip->flags.b.abr) {
			break;
		}
		(void) sam_callout_abr(ip, client_ord);
		break;

	default:
		error = EPROTO;
		break;
	}

	SAM_CLOSE_OPERATION(ip->mp, error);

	crfree(cred);
	return (error);
}


/*
 * ----- sam_client_getino -
 *	Given inode, get the client size and mod times.
 */

int
sam_client_getino(sam_node_t *ip, int client_ord)
{
	int32_t client_owner = 0;
	int error = 0;

	/*
	 * If server or client is requesting inode info, get size from the
	 * append owner host if it is not the requesting client or if it is
	 * not the server. Likewise, get mod times from the write lease owner
	 * host if it is not the requesting client or if it is not the server.
	 *
	 * NOTIFY_getino is a callout with no wait to the client. The client
	 * responds with a no wait INODE_fsync_nowait. The caller waits at
	 * most DEF_META_TIMEO (3) seconds for a response from the client.
	 */
	mutex_enter(&ip->ilease_mutex);
	if (ip->size_owner && (ip->size_owner != client_ord) &&
	    (ip->size_owner != ip->mp->ms.m_client_ord)) {
		client_owner = ip->size_owner;
	} else if (ip->write_owner && (ip->write_owner != client_ord) &&
	    (ip->write_owner != ip->mp->ms.m_client_ord)) {
		client_owner = ip->write_owner;
	}
	if (client_owner) {
		ip->getino_waiters++;
		if (ip->getino_waiters == 1) {
			ip->waiting_getino = TRUE;
			mutex_exit(&ip->ilease_mutex);
			error = sam_proc_notify(ip, NOTIFY_getino,
			    client_owner, NULL, 0);
			mutex_enter(&ip->ilease_mutex);
		}
		if (ip->waiting_getino) {
			(void) cv_timedwait_sig(&ip->ilease_cv,
			    &ip->ilease_mutex,
			    (lbolt + (DEF_META_TIMEO * hz)));
		}
		ip->getino_waiters--;
	}
	mutex_exit(&ip->ilease_mutex);
	return (error);
}


/*
 * ----- sam_update_inode_buffer_rw -
 * For lease reset, leases removed, or size update, sync inode to disk
 * because the server may panic and this inode update would be lost.
 * Otherwise, update buffer cache. Inode WRITER rwlock set.
 */

static int
sam_update_inode_buffer_rw(sam_node_t *ip)
{
	buf_t *bp;
	struct sam_perm_inode *permip;
	int error = 0;

	if (ip->sr_sync) {
		if (ip->flags.b.changed) {
			error = sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
		}
		if (error == 0) {
			ip->sr_sync = 0;
		}
	} else {
		error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip);
		if (error == 0) {
			permip->di = ip->di;
			permip->di2 = ip->di2;
			if (TRANS_ISTRANS(ip->mp)) {
				TRANS_WRITE_DISK_INODE(ip->mp, bp, permip,
				    ip->di.id);
			} else {
				bdwrite(bp);
			}
		}
	}
	return (error);
}


/*
 * -----	sam_process_block_request - system shared server block command.
 *	Called when the shared server receives a BLOCK operation.
 */

static int				/* ERRNO, else 0 if successful. */
sam_get_block_request(sam_mount_t *mp, sam_san_message_t *msg)
{
	sam_san_block_t *bnp = &msg->call.block;
	sam_node_t *ip;
	vnode_t *vp;
	int error = 0;

	vp = NULL;
	if (bnp->id.ino == SAM_INO_INO) {
		ip = mp->mi.m_inodir;
		vp = SAM_ITOV(ip);
	} else if (bnp->id.ino != 0) {
		error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS, &bnp->id, &ip);
		if (error == 0) {
			vp = SAM_ITOV(ip);
		}
	}
	if (error == 0) {

		switch (msg->hdr.operation) {

		case BLOCK_getbuf: {
			sam_block_getbuf_t *blk = &bnp->arg.p.getbuf;
			buf_t *bp;

			TRACE(T_SAM_SR_BLOCK, vp, (uint_t)blk->blkno,
			    blk->ord, blk->bsize);
			error = SAM_BREAD(mp, mp->mi.m_fs[blk->ord].dev,
			    blk->blkno, blk->bsize, &bp);
			if (error == 0) {
				bcopy(bp->b_un.b_addr, &bnp->data, blk->bsize);
				brelse(bp);
			}
			}
			break;

		case BLOCK_fgetbuf: {
			sam_block_fgetbuf_t *fblk = &bnp->arg.p.fgetbuf;
			caddr_t base;
			int32_t len;
			int blk_off;

			len = fblk->len;
			TRACE(T_SAM_SR_FBLOCK, vp, fblk->offset, len,
			    fblk->ino);
			if ((fblk->offset + fblk->len) >
			    ((ip->di.rm.size + PAGEOFFSET) & PAGEMASK)) {
				/*
				 * Shared client has larger PAGESIZE than the
				 * server.  Adjust the length downward and bzero
				 * the difference.  e.g. SPARC client requesting
				 * AMD server to read 4k of actual data for an
				 * 8k PAGESIZE byte request.
				 */
				len = ((ip->di.rm.size + PAGEOFFSET) &
				    PAGEMASK) - fblk->offset;
				TRACE(T_SAM_SR_FBLOCK, vp, fblk->offset,
				    len, fblk->ino);
				ASSERT(len > 0);
				bzero(&bnp->data[len], fblk->len - len);
			}
			SAM_SET_LEASEFLG(ip);
			base = segmap_getmapflt(segkmap, vp, fblk->offset,
			    len, 0, S_READ);
			SAM_CLEAR_LEASEFLG(ip);

			/*
			 * Solaris (AMD and SPARC) clients always request
			 * PAGESIZE bytes.  If the client has a smaller page
			 * size, it might be asking for data in the middle of
			 * this page.
			 */
			blk_off = fblk->offset & MAXBOFFSET;

			bcopy(base + blk_off, &bnp->data, len);
			if (fblk->ino != 0) {
				struct sam_dirval *dvp;

				dvp = (struct sam_dirval *)(void *)
				    (base + DIR_BLK -
				    sizeof (struct sam_dirval));
				if (fblk->ino != dvp->d_id.ino) {
					cmn_err(CE_NOTE,
					    "SAM-QFS: %s: server.c "
					    "fbread ino=%d, %d.%d ",
					    mp->mt.fi_name, fblk->ino,
					    dvp->d_id.ino,
					    dvp->d_id.gen);
				}
			}
			segmap_release(segkmap, base, 0);
			}
			break;

		case BLOCK_getino: {
			sam_block_getino_t *getino = &bnp->arg.p.getino;
			buf_t *bp;
			struct sam_perm_inode *permip;

			if (getino->id.gen != 0 &&
			    (getino->bsize ==
					sizeof (struct sam_disk_inode))) {
				sam_node_t *ip;

				error = sam_check_cache(&getino->id, mp,
							IG_EXISTS, NULL, &ip);
				if (error == 0 && ip != NULL) {
					bcopy((char *)&ip->di,
						&bnp->data, getino->bsize);
					VN_RELE(SAM_ITOV(ip));
					break;
				}
			}
			error = sam_read_ino(mp, getino->id.ino, &bp, &permip);
			if (error == 0) {
				bcopy((char *)&permip->di, &bnp->data,
				    getino->bsize);
				brelse(bp);
			}
			TRACE(T_SAM_SR_INO, vp, getino->id.ino,
			    getino->id.gen, error);
			}
			break;

		case BLOCK_getsblk: {
			sam_block_sblk_t *sp = &bnp->arg.p.sblk;
			buf_t *bp;
			struct sam_sblk *blk;
			uint_t sz;

			error = 0;
			sz = sp->bsize;
			blk = mp->mi.m_sbp;
			if (blk) {
				bp = NULL;
				if (sz > mp->mi.m_sblk_size) {
					sz = mp->mi.m_sblk_size;
				}
			} else {
				/*
				 * XXX - Probably should sanity-check the
				 * requested size.
				 */
				error = SAM_BREAD(mp, mp->mi.m_fs[0].dev,
				    SUPERBLK, sp->bsize, &bp);
				if (error == 0) {
					blk = (struct sam_sblk *)
					    (void *)bp->b_un.b_addr;
				}
			}
			if (error == 0) {
				bcopy(blk, &bnp->data, sz);
				if (bp) {
					brelse(bp);
				}
			}
			}
			break;

		case BLOCK_vfsstat_v2: {
			sam_block_vfsstat_v2_t *sp2 = &bnp->arg.p.vfsstat_v2;
			client_entry_t *clp;

			clp = sam_get_client_entry(mp, msg->hdr.client_ord, 0);
			clp->fs_count = sp2->fs_count;
			clp->mm_count = sp2->mm_count;
			clp->cl_fsgen = sp2->fsgen;
			}

			/* LINTED [fallthrough on case statement] */
		case BLOCK_vfsstat: {
			sam_block_vfsstat_t *sp = &bnp->arg.p.vfsstat;
			struct sam_sblk *sblk = mp->mi.m_sbp;

			if (sblk != NULL) {
				int i;

				sp->capacity = sblk->info.sb.capacity;
				sp->space = sblk->info.sb.space;
				sp->mm_capacity = sblk->info.sb.mm_capacity;
				sp->mm_space = sblk->info.sb.mm_space;
				/*
				 * Add free blocks on the block list.
				 */
				for (i = 0; i < mp->mt.fs_count; i++) {
					struct samdent *dp;
					int bt;

					if (sblk->eq[i].fs.type == DT_META) {
						continue;
					}

					dp = &mp->mi.m_fs[i];
					if (dp->skip_ord) {
						continue;
					}
					for (bt = 0; bt < SAM_MAX_DAU; bt++) {
						int blocks;
						struct sam_block *block;

						if ((block = dp->block[bt]) ==
						    NULL) {
							continue;
						}
						blocks =
						    BLOCK_COUNT(block->fill,
						    block->out,
						    block->limit) *
						    sblk->info.sb.dau_blks[bt];
						if (sblk->eq[i].fs.num_group >
						    1) {
							blocks *=
							    sblk->eq[
							    i].fs.num_group;
						}
						sp->space += blocks;
					}
				}
				/*
				 * Add free small blocks on the release block
				 * list.
				 */
				if (!mp->mt.mm_count) {
					sp->space += mp->mi.m_inoblk_blocks;
				}
				/*
				 * Add reserved large block in .block file.
				 */
				if (mp->mi.m_blk_bn &&
				    (mp->mi.m_inoblk->di.status.b.meta == DD)) {
					sp->space += LG_DEV_BLOCK(mp, DD);
				}
			}
			}
			break;

		case BLOCK_wakeup:
			/*
			 * Current WRITE/APPEND inode owner has flushed pages
			 * and changed to directio. Wakeup all waiting clients.
			 */
			mutex_enter(&ip->ilease_mutex);
			if (ip->sr_leases != NULL) {
				struct sam_lease_ino *llp;
				int nl;

				llp = ip->sr_leases;
				for (nl = 0; nl < llp->no_clients; nl++) {
					if (llp->lease[nl].client_ord ==
					    msg->hdr.client_ord) {
					llp->lease[nl].shflags.b.directio = 1;
						continue;
					}
					if (llp->lease[nl].actions &
					    SR_WAIT_LEASE) {
						llp->lease[nl].actions &=
						    ~SR_WAIT_LEASE;
						(void) sam_proc_notify(ip,
						    NOTIFY_lease,
						    llp->lease[nl].client_ord,
						    NULL, 0);
					}
				}
			}
			mutex_exit(&ip->ilease_mutex);
			break;

#ifdef DEBUG
		case BLOCK_panic: {
			int prev_srvr = 0;

			/*
			 * Panic previous server and this host.
			 */
			prev_srvr = mp->ms.m_prev_srvr_ord;
			if (prev_srvr != 0) {
				(void) sam_proc_notify(ip, NOTIFY_panic,
				    prev_srvr, NULL, 0);
			}
			delay(hz);
			cmn_err(CE_PANIC,
			"SAM-QFS: %s: CLIENT %d panic, panic previous "
			"srvr %d, status=%x",
			    mp->mt.fi_name, msg->hdr.client_ord, prev_srvr,
			    mp->mt.fi_status);
			/*NOTREACHED*/
			/* NO DEPOSIT, NO RETURN -- DEBUG only? */
			}
			break;
#endif /* DEBUG */

		case BLOCK_quota: {
			sam_block_quota_t *qap = &bnp->arg.p.samquota;
			struct sam_quot *qp;

			qp = sam_quot_get(mp, NULL, qap->type, qap->index);
			if (qp) {
				bcopy((char *)&qp->qt_dent, &bnp->data,
				    sizeof (qp->qt_dent));
				sam_quot_rel(mp, qp);
			} else {
				bzero(&bnp->data, sizeof (qp->qt_dent));
				error = ENOENT;
			}
			}
			break;

		default:
			error = EPROTO;
			break;
		}
		if (vp && (ip->di.id.ino != SAM_INO_INO)) {
			mutex_enter(&vp->v_lock);
			vp->v_count--;
			mutex_exit(&vp->v_lock);
		}
	}
	return (error);
}


/*
 * ----- sam_proc_callout - process callout for the server.
 *	Send callout request OTW to the client specified by client_ord.
 *	If client_ord is zero, send to all clients. Do not wait for a response.
 *  A callout command causes a refresh of the inode on the client.
 *  NOTE. inode WRITERS lock held if write_lock TRUE; otherwise,
 *  READERS lock held.
 */

int					/* ERRNO if error, 0 if successful. */
sam_proc_callout(
	sam_node_t *ip,			/* Inode entry pointer */
	enum CALLOUT_operation op,
	ushort_t actions,		/* Server directed actions */
	int client_ord,			/* Client ordinal */
	sam_callout_arg_t *arg)		/* Pointer to callout args */
{
	sam_san_callout_msg_t *msg;
	sam_san_callout_t *cop;
	int error;

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	cop = &msg->call.callout;
	cop->id = ip->di.id;
	switch (op) {
		case CALLOUT_stage:
			cop->arg.p.stage = arg->p.stage;
			break;

		case CALLOUT_relinquish_lease:
			cop->arg.p.relinquish_lease = arg->p.relinquish_lease;
			break;

		default:
			ASSERT(arg == NULL);
			break;
	}

	/*
	 * Set server attributes.
	 */
	sam_set_sr_attr(ip, actions, &cop->irec);

	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_CALLOUT, SHARE_nowait, op,
	    sizeof (sam_san_callout_t), sizeof (sam_san_callout_t));
	msg->hdr.originator = SAM_SHARED_SERVER;
	msg->hdr.client_ord = client_ord;
	error = sam_write_to_client(ip->mp, (sam_san_message_t *)msg);
	kmem_free(msg, sizeof (*msg));
	return (error);
}


/*
 * ----- sam_proc_notify - process notify for the server.
 *	Send notify request OTW to the client specified by client_ord.
 *	If client_ord is zero, send to all clients. Do not wait for a response.
 *  Server notifies clients of an action.
 *   1. Reissue lease command.
 *   2. Stale the dnlc for a file.
 */

int
sam_proc_notify(
	sam_node_t *ip,			/* pointer to inode. */
	enum NOTIFY_operation op,	/* Lease operation */
	int client_ord,			/* Client ordinal */
	sam_notify_arg_t *arg,		/* Pointer to notify args */
	int skip_ord)			/* client ordinal to skip */
{
	sam_san_notify_msg_t *msg;
	sam_san_notify_t *nop;
	int error;
	sam_mount_t *mp = ip->mp;

	msg = kmem_zalloc(sizeof (*msg), KM_SLEEP);
	nop = &msg->call.notify;
	nop->id = ip->di.id;
	switch (op) {
		case NOTIFY_lease_expire:
			nop->arg.p.lease = arg->p.lease;
			break;

		case NOTIFY_dnlc:
			nop->arg.p.dnlc = arg->p.dnlc;
			break;

		case NOTIFY_getino:
			break;

		case NOTIFY_panic:
			break;

		default:
			ASSERT(arg == NULL);
			break;
	}
	sam_build_header(ip->mp, &msg->hdr, SAM_CMD_NOTIFY, SHARE_nowait,
	    op, sizeof (sam_san_notify_t), sizeof (sam_san_notify_t));
	msg->hdr.originator = SAM_SHARED_SERVER;

	if (client_ord == 0 && skip_ord > 0) {
		int ord;
		/*
		 * Send to all clients except skip_ord or this host.
		 */
		for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
			if (ord == skip_ord || ord == mp->ms.m_client_ord) {
				continue;
			}
			msg->hdr.client_ord = ord;
			error = sam_write_to_client(mp,
			    (sam_san_message_t *)msg);
		}

	} else {

		/* Send to this client ord, 0 if all */
		msg->hdr.client_ord = client_ord;
		error = sam_write_to_client(mp, (sam_san_message_t *)msg);

	}
	kmem_free(msg, sizeof (*msg));
	return (error);
}


/*
 * -----	sam_set_sr_ino - return server ino record.
 * Set READER lock and set server attributes.
 */

static void
sam_set_sr_ino(
	sam_node_t *ip,			/* Pointer to inode. */
	sam_id_t *id,			/* Id -- returned in message */
	sam_ino_record_t *irec)		/* inode record in returned message */
{
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	*id = ip->di.id;
	irec->sr_attr.offset = 0;
	sam_set_sr_attr(ip, 0, irec);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
}


/*
 * ----- sam_set_sr_attr - Set up the inode record.
 * This inode record is built on the server and processed
 * on the receiving client. Note, the client restores this info
 * only if the sequence if greater than the previous record.
 * Note, out of order messages can be received by the client.
 */

static void
sam_set_sr_attr(
	sam_node_t *ip,			/* Pointer to inode. */
	ushort_t actions,		/* Server directed actions */
	sam_ino_record_t *irec)		/* inode record in returned message */
{
	sam_mount_t *mp;
	/*
	 * Set Server shared file attributes - inode incore info.
	 */
	irec->sr_attr.actions = actions;
	irec->sr_attr.size_owner = ip->size_owner;
	irec->sr_attr.current_size = ip->size;
	irec->sr_attr.stage_size = ip->stage_size;
	irec->sr_attr.stage_err = ip->stage_err;
	irec->sr_attr.alloc_size = ip->cl_allocsz;

	/*
	 * Set permanent inode info.
	 */
	irec->di = ip->di;
	irec->di2 = ip->di2;

	/*
	 * Set inode instance. Client restores the attributes only if the
	 * sequence number is greater than the previously received sequence
	 * number. The inode server sequence number is persistent across the
	 * mount instance. It cannot be put in the incore inode because the
	 * server inode can be inactivated while the client inode exists.
	 * This is why we are using one server sequence number for all inodes.
	 */
	mp = ip->mp;
	irec->in.srvr_ord = mp->ms.m_server_ord;
	irec->in.ino_gen = mp->ms.m_sr_ino_gen;

	mutex_enter(&mp->ms.m_sr_ino_mutex);
	irec->in.seqno = mp->ms.m_sr_ino_seqno++;
	if (mp->ms.m_sr_ino_seqno == 0xffffffffffffffff) {
		mp->ms.m_sr_ino_seqno = 1;
	}
	mutex_exit(&mp->ms.m_sr_ino_mutex);
}


/*
 * -----	sam_update_cl_attr - update inode attributes.
 * Executed on the server due to a client lease or inode command.
 */

static void
sam_update_cl_attr(
	sam_node_t *ip,			/* Pointer to inode */
	int client_ord,			/* Client ordinal caller */
	sam_cl_attr_t *attr)		/* attributes in message. */
{
	/*
	 * Update file size only if client requested the update.
	 */
	if (attr->actions & SR_SET_SIZE) {
		boolean_t set_size;
		int srvr_seqno;

		/*
		 * Check for out of order file size update. Only reset the
		 * size if the client attr sequence number indicates this is
		 * the first time we are resetting the inode, or this is
		 * a newer copy of the attribute information.
		 */
		srvr_seqno = 0;
		set_size = TRUE;
		if (ip->sr_leases) {
			boolean_t lease_entry = FALSE;
			sam_lease_ino_t *llp;
			int nl;

			mutex_enter(&ip->ilease_mutex);
			llp = ip->sr_leases;
			if (llp) {
				for (nl = 0; nl < llp->no_clients; nl++) {
					if (llp->lease[nl].client_ord ==
					    client_ord) {
						srvr_seqno =
						    llp->lease[nl].attr_seqno;
						lease_entry = TRUE;
						break;
					}
				}
			}
			if ((srvr_seqno == 0) || (attr->seqno == 0) ||
			    (SAM_SEQUENCE_LATER(attr->seqno, srvr_seqno))) {
				if (lease_entry) {
					llp->lease[nl].attr_seqno = attr->seqno;
				}
			} else {
				set_size = FALSE;
			}
			mutex_exit(&ip->ilease_mutex);
		}
		if (set_size) {
			attr->actions &= ~SR_SET_SIZE;
			ip->di.rm.size = attr->real_size;
			if (SAM_IS_OBJECT_FILE(ip) &&
			    (client_ord != ip->mp->ms.m_client_ord)) {
				(void) sam_set_end_of_obj(ip, ip->di.rm.size,
				    0);
			}
			sam_set_size(ip);
			ip->flags.b.changed = 1;
			ip->sr_sync = 1;
			ip->updtime = SAM_SECOND();
		}
	}
	/*
	 * Update inode times only if client requested the update.
	 */
	if (attr->iflags & SAM_UPDATE_FLAGS) {
		ip->flags.bits |= (attr->iflags & SAM_UPDATE_FLAGS);
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, attr->iflags);
		ip->updtime = SAM_SECOND();
	}
	/*
	 * Execute directed actions only if client requested the actions.
	 */
	if (attr->actions) {
		sam_directed_actions(ip, attr->actions, 0, CRED());
	}
}


/*
 * ----- sam_getcred - get credentials from OTW message.
 */

static cred_t *
sam_getcred(sam_cred_t *sam_credp)
{
	cred_t *cr;

	if ((unsigned int)(sam_credp->cr_ruid) > MAXUID) {
		sam_credp->cr_ruid = UID_NOBODY;
	}
	if ((unsigned int)(sam_credp->cr_uid) > MAXUID) {
		sam_credp->cr_uid = UID_NOBODY;
	}
	if ((unsigned int)(sam_credp->cr_suid) > MAXUID) {
		sam_credp->cr_suid = UID_NOBODY;
	}
	if ((unsigned int)(sam_credp->cr_rgid) > MAXUID) {
		sam_credp->cr_rgid = GID_NOBODY;
	}
	if ((unsigned int)(sam_credp->cr_gid) > MAXUID) {
		sam_credp->cr_gid = GID_NOBODY;
	}
	if ((unsigned int)(sam_credp->cr_sgid) > MAXUID) {
		sam_credp->cr_sgid = GID_NOBODY;
	}
	cr = crget();
	crsetresuid(cr, sam_credp->cr_ruid, sam_credp->cr_uid,
	    sam_credp->cr_suid);
	crsetresgid(cr, sam_credp->cr_rgid, sam_credp->cr_gid,
	    sam_credp->cr_sgid);
	crsetgroups(cr, sam_credp->cr_ngroups, sam_credp->cr_groups);
	crsetzone(cr, global_zone);
	if (sam_credp->cr_projid == -1) {
		sam_credp->cr_projid = SAM_NOPROJECT;
	}
	crsetprojid(cr, sam_credp->cr_projid);

	return (cr);
}
