/*
 * ----- srmisc.c - Miscellaneous interfaces for the server.
 *
 *	Miscellaneous interface modules for the server shared file system.
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

#pragma ident "$Revision: 1.86 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/dnlc.h>
#include <sys/file.h>
#include <sys/flock.h>
#include <sys/mount.h>
#include <nfs/lm.h>
#include <sys/policy.h>
#include <sys/mntent.h>

/* ----- SAMFS Includes */

#include <sam/types.h>
#include <sam/syscall.h>
#include <sam/shareops.h>

#include "inode.h"
#include "mount.h"
#include "scd.h"
#include "global.h"
#include "extern.h"
#include "rwio.h"
#include "samhost.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"


static int sam_check_new_server(sam_mount_t *mp, int new_ord, char *new_server);
static void sam_start_failover(sam_mount_t *mp, boolean_t new_server,
	char *server);
static void sam_failover_new_server(sam_mount_t *mp, cred_t *credp);
extern void sam_clear_cmd(enum SCD_daemons scdi);
static void sam_onoff_client_delay(sam_schedule_entry_t *entry);

extern	kmutex_t	qfs_scan_lock;
extern	int		lqfs_free(sam_mount_t *mp);


/*
 * ----- sam_voluntary_failover - Process the privileged failover system call.
 *
 * This is issued by the sam-sharefsd daemon to the current metadata
 * server. This begins voluntary failover on the old metadata server
 * to a new metadata server.
 */

int					/* ERRNO if error, 0 if successful. */
sam_voluntary_failover(
	void *arg,			/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_set_host args;
	sam_mount_t *mp;
	int error = 0;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}
	/*
	 * Switch hold on FS from the syscall count to a daemon (socket)
	 * hold. This avoids a short wait in the unmount routine waiting
	 * for the syscall count to drop to zero.  Since we've been called
	 * by sam-sharefsd, we use the socket hold mechanism.  It's a cheat,
	 * but not a bad one -- it can't exit until this call returns
	 * anyway.
	 */
	SAM_SHARE_DAEMON_HOLD_FS(mp);
	SAM_SYSCALL_DEC(mp, 0);

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		goto out;
	}

	if (!SAM_IS_SHARED_FS(mp)) {
		error = ENOTTY;
		goto out;
	}

	/*
	 * The daemon is shutting down. Note, this SC_failover system
	 * call is only sent from the sam-sharefsd daemon who is
	 * the current server and who is becoming a client.
	 * This call initially starts the voluntary failover sequence.
	 * NOTE:  An involuntary failover does not execute this call.
	 */
	TRACES(T_SAM_VOLFAILOVER, mp, args.server);

	if ((error = sam_check_new_server(mp, -1, args.server))) {
		goto out;
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);
	strncpy(mp->mt.fi_server, args.server, sizeof (mp->mt.fi_server));

	TRACE(T_SAM_VOLFAIL1, mp, mp->mt.fi_status, mp->ms.m_server_ord,
	    mp->ms.m_client_ord);
	/*
	 * If server is becoming a client, then initiate the failover
	 * if not already failing over or not mounted.
	 */
	if (!(mp->mt.fi_status & FS_MOUNTED)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		error = EXDEV;
		cmn_err(CE_NOTE,
	"SAM-QFS: %s: Voluntary failover ignored - "
	"file system is not mounted (%x)",
		    mp->mt.fi_name, mp->mt.fi_status);
	} else if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		error = EBUSY;
		cmn_err(CE_NOTE,
		    "SAM-QFS: %s: Failover ignored - failover in progress (%x)",
		    mp->mt.fi_name, mp->mt.fi_status);
	} else {
		mp->mt.fi_status |= (FS_FREEZING | FS_LOCK_HARD);
		mutex_exit(&mp->ms.m_waitwr_mutex);
		sam_failover_old_server(mp, args.server, credp);
	}

	TRACE(T_SAM_VOLFAIL2, mp, mp->mt.fi_status, mp->ms.m_server_ord,
	    mp->ms.m_client_ord);

out:
	SAM_SHARE_DAEMON_RELE_FS(mp);
	return (error);
}


/*
 * ----- sam_check_new_server - Check that new server is connected & mounted.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_check_new_server(
	sam_mount_t *mp,	/* Pointer to mount table */
	int new_ord,		/* New server ord, if -1 use new_server */
	char *new_server)	/* New server name */
{
	client_entry_t *clp;
	int ord;
	int error;

	/*
	 * Verify new server is connected and mounted.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	error = ENOTACTIVE;
	for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
		clp = sam_get_client_entry(mp, ord, 0);
		if (clp == NULL || clp->cl_sh.sh_fp == NULL) {
			continue;
		}
		if (new_ord > 0) {
			if (new_ord != ord) {
				continue;
			}
		} else if (strncasecmp(clp->hname, new_server,
		    sizeof (clp->hname))) {
			continue;
		}
		if (clp->cl_sock_flags & SOCK_BYTE_SWAP) {
			error = ENOTSUP;
		} else if (clp->cl_status & FS_MOUNTED) {
			error = 0;
		}
		break;
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);
	return (error);
}


/*
 * ----- sam_set_server - Processes the privileged set server system call.
 *
 * Sets/clears the flag that indicates this host is the server.
 */

int				/* ERRNO if error, 0 if successful. */
sam_set_server(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_set_host args;
	sam_mount_t *mp;
	client_entry_t *clp;
	int error = 0;
	int ord;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		goto out;
	}

	if (!SAM_IS_SHARED_FS(mp) || !SAM_IS_SHARED_SERVER_ALT(mp)) {
		error = ENOTTY;
		goto out;
	}

	TRACES(T_SAM_SETSERVER, mp, args.server);
	TRACE(T_SAM_BEGSRVR, mp, mp->mt.fi_status, args.ord,
	    mp->ms.m_server_ord);

	/*
	 * Reset lists of known dead/inop clients.
	 */
	mutex_enter(&mp->ms.m_cl_wrmutex);
	if (mp->ms.m_clienti != NULL) {
		for (ord = 1; ord <= mp->ms.m_maxord; ord++) {
			clp = sam_get_client_entry(mp, ord, 0);
			if (clp == NULL) {
				continue;
			}
			clp->cl_flags &=
			    ~(SAM_CLIENT_INOP | SAM_CLIENT_DEAD |
			    SAM_CLIENT_SOCK_BLOCKED);
		}
	}
	mutex_exit(&mp->ms.m_cl_wrmutex);

	mutex_enter(&mp->ms.m_waitwr_mutex);
	strncpy(mp->mt.fi_server, args.server, sizeof (mp->mt.fi_server));
	mp->ms.m_prev_srvr_ord = mp->ms.m_server_ord;
	mp->ms.m_server_ord = args.ord;
	mp->ms.m_client_ord = args.ord;
	mp->ms.m_maxord = args.maxord;

	/*
	 * If mounted and client is becoming the server, failover.
	 * If mounting, return an error. Daemon will retry in 10 seconds.
	 */
	if (SAM_IS_SHARED_CLIENT(mp)) {
		if (mp->mt.fi_status & FS_MOUNTING) {
			mutex_exit(&mp->ms.m_waitwr_mutex);
			error = EINPROGRESS;
			goto out;
		}

		/*
		 * If the file system is mounted as a client and set server
		 * command is received, then this host is the new metadata
		 * server. We set freezing and turn off srvr_down for an
		 * involuntary failover.
		 */
		if (mp->mt.fi_status & FS_MOUNTED) {
			if (mp->mt.fi_status & FS_SRVR_DOWN) {
				mp->mt.fi_status &= ~FS_SRVR_DOWN;
				mp->mt.fi_status |=
				    (FS_FREEZING | FS_LOCK_HARD);
			}
			mp->mt.fi_status |= FS_RESYNCING;
			mutex_exit(&mp->ms.m_waitwr_mutex);

			cmn_err(CE_NOTE,
"SAM-QFS: %s: %s failover, Set new server %s, active ops = %d, freezing (%x)",
			    mp->mt.fi_name,
			    mp->ms.m_involuntary ? "Involuntary" : "Voluntary",
			    mp->mt.fi_server, mp->ms.m_cl_active_ops,
			    mp->mt.fi_status);

			sam_failover_new_server(mp, credp);

			mp->ms.m_resync_time = lbolt;
			sam_taskq_add(sam_resync_server, mp, NULL, 2 * hz);
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_status &= ~FS_FREEZING;
		}
	}
	mp->mt.fi_status |= FS_SERVER;
	mp->mt.fi_status &= ~FS_CLIENT;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	TRACE(T_SAM_ENDSRVR, mp, mp->mt.fi_status, args.ord,
	    mp->ms.m_server_ord);

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_sgethosts - set/get the /.hosts file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_sgethosts(
	void *arg,		/* Pointer to arguments. */
	int size,
	int write,
	cred_t *credp)
{
	sam_sgethosts_arg_t args;
	sam_host_table_t hdrhost;
	sam_mount_t *mp;
	sam_node_t *ip;
	sam_id_t fid;
	caddr_t ubuf;
	uio_t uioblk;
	iovec_t io_op;
	int error = 0;
	int rwsize;
	int issync;
	int trans_size;
	int terr = 0;

	if (size != sizeof (args) || copyin(arg, (caddr_t)&args, sizeof (args)))
		return (EFAULT);

	if ((mp = sam_find_filesystem(args.fsname)) == NULL) {
		return (ENOENT);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		goto out;
	}

	if (!(mp->mt.fi_status & FS_MOUNTED)) {
		error = EXDEV;
		goto out;
	}
	if (!SAM_IS_SHARED_FS(mp)) {
		error = ENOTTY;
		goto out;
	}

	/*
	 * Fail if this host can't access the metadata devices.
	 */
	if (!SAM_IS_SHARED_SERVER_ALT(mp)) {
		error = EREMOTE;
		goto out;
	}

	if (mp->mt.fi_status & (FS_FAILOVER | FS_RESYNCING)) {
		error = EBUSY;
		goto out;
	}

	ubuf = (caddr_t)args.hosts.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		ubuf = (caddr_t)args.hosts.p64;
	}
	if (copyin(ubuf, (caddr_t)&hdrhost, sizeof (hdrhost))) {
		error = EFAULT;
		goto out;
	}

	/*
	 * Fail if pending server is not connected or mounted.
	 */
	if (SAM_IS_SHARED_SERVER(mp)) {
		if (write && ((int)hdrhost.pendsrv >= 0)) {
			if ((error = sam_check_new_server(mp,
			    hdrhost.pendsrv+1, NULL))) {
				goto out;
			}
		}
	} else if (((int)hdrhost.pendsrv + 1) == mp->ms.m_client_ord) {
		if ((mp->mt.fi_status & FS_MOUNTED) == 0) {
			error = ENOTACTIVE;
			goto out;
		}
	}

	/*
	 * Found filesystem.  Know .hosts' inode number.  Grab inode.
	 */
	fid.ino = SAM_HOST_INO;
	fid.gen = SAM_HOST_INO;
	if ((error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &fid, &ip))) {
		goto out;
	}

	rwsize = args.size;

	if (write) {
		/*
		 * Write the host table.
		 */
		if (rwsize == SAM_LARGE_HOSTS_TABLE_SIZE &&
		    (ip->di.rm.size < SAM_LARGE_HOSTS_TABLE_SIZE)) {
			struct sam_sbinfo *sblk = &ip->mp->mi.m_sbp->info.sb;

			/*
			 * File system must be V2A or higher for large
			 * host table.
			 */
			if (!SAM_MAGIC_V2A_OR_HIGHER(sblk)) {
				error = ENOSYS;
				sam_rele_ino(ip);
				cmn_err(CE_WARN, "SAM-QFS: %s: File system "
				    "must be V2A to have large host table.",
				    mp->mt.fi_name);
				if (SAM_MAGIC_V2_OR_HIGHER(sblk)) {
					cmn_err(CE_WARN, "\tUpgrade file "
					    "system with samadm add-features "
					    "first.");
				}
				goto out;
			}

			/*
			 * TMP transaction until something more
			 * specific exists.
			 */
			trans_size = (int)TOP_SETATTR_SIZE(ip);
			TRANS_BEGIN_CSYNC(ip->mp, issync,
			    TOP_SETATTR, trans_size);

			/*
			 * See if there is enough space for
			 * a large host table, if not allocate a new one.
			 */
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if ((ip->di.blocks * SAM_BLK)
			    < SAM_LARGE_HOSTS_TABLE_SIZE) {
				sam_bn_t save_bn;
				uchar_t	save_ord;
				int32_t save_blocks;
				offset_t save_size;
				int fblk_to_extent;

				ASSERT(ip->di.status.b.direct_map == 0);
				ASSERT(ip->di.status.b.on_large == 1);
				ASSERT(ip->di.rm.size == SAM_HOSTS_TABLE_SIZE);

				/*
				 * Allocate space for a new large
				 * direct map host file.
				 */

				/*
				 * Save these old values in case of an error.
				 */
				save_size = ip->di.rm.size;
				save_bn = ip->di.extent[0];
				save_ord = ip->di.extent_ord[0];
				save_blocks = ip->di.blocks;

				ip->di.rm.size = 0;
				ip->size = 0;
				ip->di.extent[0] = 0;
				ip->di.extent_ord[0] = 0;
				ip->di.blocks = 0;

				/*
				 * The large host table must be a
				 * direct map file for samsharefs -R
				 */
				ip->di.status.b.direct_map = 1;

				error = sam_map_block(ip, (offset_t)0,
				    SAM_LARGE_HOSTS_TABLE_SIZE,
				    SAM_ALLOC_BLOCK, NULL, credp);

				if (error) {
					/*
					 * Restore the original info.
					 */
					ip->di.status.b.direct_map = 0;
					ip->size = save_size;
					ip->di.rm.size = save_size;
					ip->di.extent[0] = save_bn;
					ip->di.extent_ord[0] = save_ord;
					ip->di.blocks = save_blocks;

				} else {
					/*
					 * Free the old host table.
					 */
					sam_free_block(mp, LG,
					    save_bn, save_ord);

					/*
					 * Record the new hosts table location
					 * in the superblock.
					 */
					fblk_to_extent =
					    sblk->ext_bshift - SAM_DEV_BSHIFT;
					sblk->hosts =
					    ip->di.extent[0] << fblk_to_extent;
					sblk->hosts_ord = ip->di.extent_ord[0];
				}
			}

			if (error == 0) {
				/*
				 * Update the inode and the superblocks.
				 */
				ip->size = SAM_LARGE_HOSTS_TABLE_SIZE;
				ip->di.rm.size = SAM_LARGE_HOSTS_TABLE_SIZE;

				TRANS_INODE(ip->mp, ip);
				sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
				sam_update_inode(ip, SAM_SYNC_ONE, FALSE);

				/*
				 * Update options mask.
				 */
				mutex_enter(&mp->mi.m_sblk_mutex);
				sblk->opt_mask |= SBLK_OPTV1_LG_HOSTS;
				mutex_exit(&mp->mi.m_sblk_mutex);
				error = sam_update_the_sblks(mp);
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

			TRANS_END_CSYNC(ip->mp, terr, issync,
			    TOP_SETATTR, trans_size);

			if (error == 0) {
				error = terr;
			}
			if (error) {
				sam_rele_ino(ip);
				goto out;
			}
		}

	} else {
		/*
		 * Read the host table.
		 */
		if (rwsize == SAM_LARGE_HOSTS_TABLE_SIZE &&
		    ip->di.rm.size == SAM_HOSTS_TABLE_SIZE) {
			/*
			 * OK to read a small host table into
			 * a large buffer.
			 */
			rwsize = ip->di.rm.size;
		}
	}

	if (ip->di.rm.size != rwsize) {
		sam_rele_ino(ip);
		error = EMSGSIZE;
		goto out;
	}

	bzero((char *)&io_op, sizeof (io_op));
	io_op.iov_base = ubuf;
	io_op.iov_len  = rwsize;

	bzero((char *)&uioblk, sizeof (uioblk));
	uioblk.uio_iov		= &io_op;
	uioblk.uio_iovcnt	= 1;
	uioblk.uio_fmode	= write ? FWRITE : FREAD;
	uioblk.uio_segflg	= UIO_USERSPACE;
	uioblk.uio_loffset	= 0;
	uioblk.uio_resid	= rwsize;
	uioblk.uio_llimit	= SAM_MAXOFFSET_T;

	RW_LOCK_OS(&ip->inode_rwl, write ? RW_WRITER : RW_READER);
	if (write) {
		/*
		 * Push the pages out to disk, and stale the in-core pages.
		 */
		error = sam_write_io(SAM_ITOV(ip), &uioblk, 0, CRED());
		sam_flush_pages(ip, B_INVAL);
		if ((error == 0) && !SAM_IS_SHARED_SERVER(mp)) {
			sam_san_mount_msg_t msg;

			if (sam_send_mount_cmd(mp, &msg, MOUNT_config, 0)) {
				cmn_err(CE_WARN,
"SAM-QFS: %s: Cannot send MOUNT_config to server %s\nType samd config on %s",
				    mp->mt.fi_name, mp->mt.fi_server,
				    mp->mt.fi_server);
			}
		}
	} else {
		/*
		 * Get fresh from disk.
		 */
		sam_flush_pages(ip, B_TRUNC | B_INVAL);
		error = sam_read_io(SAM_ITOV(ip), &uioblk, 0);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, write ? RW_WRITER : RW_READER);

	sam_rele_ino(ip);

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_shared_failover - Process shared failover.
 *
 * Notify all clients including the new server that a failover
 * is in progress.
 */

int					/* ERRNO if error, 0 if successful. */
sam_shared_failover(
	sam_mount_t *mp,		/* Mount table pointer */
	enum MOUNT_operation cmd,
	int ord)			/* Ordinal of new server */
{
	sam_san_mount_msg_t msg;
	int error;

	bzero((void *)&msg, sizeof (sam_san_mount_t));
	sam_build_header(mp, &msg.hdr, SAM_CMD_MOUNT, SHARE_wait, cmd,
	    sizeof (sam_san_mount_t), sizeof (sam_san_mount_t));
	msg.hdr.client_ord = ord;
	error = sam_write_to_client(mp, (sam_san_message_t *)&msg);
	return (error);
}


/*
 * ---- sam_failover_old_server -
 *
 * Process current server failing over to become a client.
 */

void
sam_failover_old_server(
	sam_mount_t *mp,	/* The mount table pointer. */
	char *server,		/* New server name */
	cred_t *credp)		/* Credentials */
{
	sam_node_t *ip;
	client_entry_t *clp;
	vfs_t *vfsp;
	int ord;
	int no_ord;
	int error;
	int journal = 0;


	cmn_err(CE_NOTE,
	    "SAM-QFS: %s: Begin voluntary failover, "
	    "new server %s, active ops = %d, freezing (%x)",
	    mp->mt.fi_name, server, mp->ms.m_cl_active_ops, mp->mt.fi_status);

	/*
	 * Send failover message to new server and all the clients.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	no_ord = 0;
	for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
		clp = sam_get_client_entry(mp, ord, 0);
		if (clp == NULL || clp->cl_sh.sh_fp == NULL) {
			continue;
		}
		if (ord == mp->ms.m_client_ord) { /* This is the caller */
			no_ord++;
			(void) sam_sighup_daemons(mp);
			continue;
		}
		if ((error = sam_shared_failover(mp,
		    MOUNT_failover, ord)) == 0) {
			no_ord++;
		} else {
			cmn_err(CE_WARN,
"SAM-QFS: %s: Error %d sending failover message to client %s, freezing (%x)",
			    mp->mt.fi_name, error,
			    clp->hname, mp->mt.fi_status);
		}
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);
	cmn_err(CE_NOTE,
"SAM-QFS: %s: Failover msg sent to %d hosts (1..%d), freezing (%x)",
	    mp->mt.fi_name, no_ord, mp->ms.m_maxord, mp->mt.fi_status);

	sam_clear_cmd(SCD_stager);

	sam_start_stop_rmedia(mp, SAM_STOP_DAEMON);

	sam_start_failover(mp, FALSE, server);

	/*
	 * State is now FROZEN.
	 */
	ASSERT(mp->mt.fi_status & FS_FROZEN);

	vfsp = mp->mi.m_vfsp;
	vfs_lock_wait(vfsp);

	TRACE(T_SAM_FAIL_OLD1, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);

	/*
	 * Clean up all locks by sysid.
	 *
	 * NOTE: This should be replaced by a contract with Solaris similar
	 *		to that of cl_flk_delete_pxfs_locks.
	 *
	 * We can't use the existing routines to flush all locks for a
	 * given system id, because they're not specific to a given
	 * file system.
	 */

	{
		int	ihash;
		kmutex_t *ihp;
		sam_ihead_t	*hip;
		sam_node_t *ip;
		vnode_t *vp;

		hip = (sam_ihead_t *)&samgt.ihashhead[0];
		for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {
			ihp = &samgt.ihashlock[ihash];
			mutex_enter(ihp);
			for (ip = hip->forw;
			    (ip != (sam_node_t *)(void *)hip);
			    ip = ip->chain.hash.forw) {

				if ((ip->mp != mp)) {
					continue;
				}

				vp = SAM_ITOV(ip);
				if (flk_has_remote_locks(vp)) {
					for (ord = 1;
					    ord <= mp->ms.m_max_clients;
					    ord++) {
						cleanlocks(vp, IGN_PID, ord);
					}
				}
			}
			mutex_exit(ihp);
		}
	}

	(void) dnlc_purge_vfsp(vfsp, 0);

	TRACE(T_SAM_FAIL_OLD2, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);

	error = sam_unmount_fs(mp, MS_FORCE, SAM_FAILOVER_OLD_SRVR);
	TRACE(T_SAM_FAIL_OLD3, mp, mp->mt.fi_status,
	    mp->ms.m_syscall_cnt, error);

	ip = SAM_VTOI(mp->mi.m_vn_root);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), 0, 0, B_INVAL, credp, NULL);
	(void) sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	RW_LOCK_OS(&mp->mi.m_inoblk->inode_rwl, RW_WRITER);
	(void) sam_update_inode(mp->mi.m_inoblk, SAM_SYNC_ONE, FALSE);
	RW_UNLOCK_OS(&mp->mi.m_inoblk->inode_rwl, RW_WRITER);

	RW_LOCK_OS(&mp->mi.m_inodir->inode_rwl, RW_WRITER);
	(void) sam_update_inode(mp->mi.m_inodir, SAM_SYNC_ONE, FALSE);
	RW_UNLOCK_OS(&mp->mi.m_inodir->inode_rwl, RW_WRITER);

	/*
	 * Roll the journal, release journal accounting resources, and
	 * update this new client's visible mount state to 'nologging'.
	 */
	if ((LQFS_GET_LOGP(mp) != NULL) &&
	    (LQFS_GET_LOGBNO(VFS_FS_PTR(mp)) != 0)) {
		journal = 1;
		(void) lqfs_flush(mp);
		if (LQFS_GET_LOGP(mp)) {
			logmap_start_roll(LQFS_GET_LOGP(mp));
		}
		TRANS_MATA_UMOUNT(mp);
		LQFS_SET_DOMATAMAP(mp, 0);
		mutex_enter(&qfs_scan_lock);
		(void) lqfs_unsnarf(mp);
		mutex_exit(&qfs_scan_lock);
		vfs_setmntopt(mp->vfs_vfs, MNTOPT_NOLOGGING, NULL, 0);
	}

	sam_cleanup_mount(mp, NULL, credp);
	sam_set_mount(mp);

	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->ms.m_sr_ino_seqno = 1;
	mp->ms.m_sr_ino_gen = lbolt;
	mp->mt.fi_status &= ~FS_SERVER;
	mp->mt.fi_status |= FS_CLIENT;

	vfs_unlock(vfsp);

	/*
	 * Mark the journal as rolled, and free the space.
	 */
	if (journal) {
		LQFS_SET_FS_ROLLED(VFS_FS_PTR(mp), FS_ALL_ROLLED);
		LQFS_SET_NOLOG_SI(mp, 0);
		(void) lqfs_free(mp);
	}

	mp->mi.m_fs_syncing = 0;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	TRACE(T_SAM_FAIL_OLD4, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);
}


/*
 * ---- sam_failover_new_server -
 *
 * Process client failing over to become the new server.
 */

static void
sam_failover_new_server(sam_mount_t *mp, cred_t *credp)
{
	sam_node_t *ip;
	vfs_t *vfsp;
	int ino;

	sam_start_failover(mp, TRUE, mp->mt.fi_server);

	/*
	 * State is now FROZEN.
	 */
	ASSERT(mp->mt.fi_status & FS_FROZEN);

	vfsp = mp->mi.m_vfsp;
	vfs_lock_wait(vfsp);

	TRACE(T_SAM_FAIL_NEW1, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);

	/*
	 * Stale root.
	 */
	ip = SAM_VTOI(mp->mi.m_vn_root);
	sam_directed_actions(ip, (SR_STALE_INDIRECT | SR_INVAL_PAGES),
	    0, credp);

	/*
	 * Cleanup mount while a client. Must stale buffer cache before becoming
	 * the server because outstanding I/O can attempt to allocate (uses
	 * buffer cache) while the server is invalidating the buffer cache.
	 */
	sam_cleanup_mount(mp, NULL, credp);

	/*
	 * Become server and get superblock from the disk.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status |= FS_SERVER;
	mp->mt.fi_status &= ~FS_CLIENT;
	mutex_exit(&mp->ms.m_waitwr_mutex);
	sam_set_mount(mp);
	TRACE(T_SAM_FAIL_NEW2, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);

	/*
	 * Update first 3 incore inodes with disk image.
	 */
	for (ino = SAM_INO_INO; ino <= SAM_BLK_INO; ino++) {
		buf_t *bp;
		struct sam_perm_inode *permip;
		int error;

		if ((error = sam_read_ino(mp, ino, &bp, &permip)) == 0) {

			switch (ino) {
			case SAM_INO_INO:
				ip = mp->mi.m_inodir;
				break;
			case SAM_ROOT_INO:
				ip = SAM_VTOI(mp->mi.m_vn_root);
				break;
			case SAM_BLK_INO:
				ip = mp->mi.m_inoblk;
				break;
			}
			ip->di = permip->di;
			ip->di2 = permip->di2;
			sam_set_size(ip);
			sam_clear_map_cache(ip);
			brelse(bp);
		} else {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Cannot read ino %d.%d during "
			    "failover error=%d",
			    mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
			    error);
		}
	}

	/*
	 * Update active hash chain with disk image.
	 * Refresh inodes only after becoming the server. Note, pages
	 * cannot be flushed before becoming the server.
	 */
	mp->ms.m_sr_ino_seqno = 1;
	mp->ms.m_sr_ino_gen = lbolt;
	(void) sam_flush_ino(vfsp, SAM_FAILOVER_NEW_SRVR, 0);

	TRACE(T_SAM_FAIL_NEW3, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);

	vfs_unlock(vfsp);

	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mi.m_fs_syncing = 0;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	if (sam_start_threads(mp) != 0) {
		cmn_err(CE_PANIC,
		    "SAM-QFS: %s: Cannot create threads during failover, panic",
		    mp->mt.fi_name);
	}

	(void) dnlc_purge_vfsp(vfsp, 0);

	TRACE(T_SAM_FAIL_NEW4, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);
}


/*
 * ---- sam_start_failover -
 *
 * Initiate the failover for the old server or new server.
 * new_server is set for new server, FALSE for old server.
 */
/* ARGSUSED1 */
static void
sam_start_failover(
	sam_mount_t *mp,	/* The mount table pointer. */
	boolean_t new_server,	/* TRUE if new server, FALSE if old server */
	char *server)		/* New Server */
{
	int count;
	int maxlease;
	int i;

	TRACE(T_SAM_FAILOVER1, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);
	SAM_COUNT64(shared_server, failovers);

	/*
	 * Wake up and freeze threads waiting on space.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->mi.m_wait_write) {
		cv_broadcast(&mp->ms.m_waitwr_cv);
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);

	maxlease = mp->mt.fi_lease[0];
	for (i = 1; i < MAX_EXPIRING_LEASES; i++) {
		maxlease = MAX(maxlease, mp->mt.fi_lease[i]);
	}

	/*
	 * Do not proceed until the file system is not syncing.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	count = 0;
	while (mp->mi.m_fs_syncing) {
		sam_cv_wait1sec_sig(&mp->ms.m_waitwr_cv,
		    &mp->ms.m_waitwr_mutex);
		if (++count > maxlease) {
			count = 0;
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: Failing over to new "
			    "server %s: waiting for sync",
			    mp->mt.fi_name, server);
		}
	}
	mp->mi.m_fs_syncing = 1;

	/*
	 * Wait until all client operations are done or frozen.
	 * This applies to both the old and the new server.
	 */
	count = 0;
	while (mp->ms.m_cl_active_ops) {
		sam_cv_wait1sec_sig(&mp->ms.m_waitwr_cv,
		    &mp->ms.m_waitwr_mutex);
		if (++count > maxlease) {
			count = 0;
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: Failing over to new server %s: "
			    "active operations = %d",
			    mp->mt.fi_name, server,
			    mp->ms.m_cl_active_ops);
		}
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);

	/*
	 * Wait until all client traffic has completed.
	 */
	TRACE(T_SAM_FAILOVER2, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);
	mutex_enter(&mp->ms.m_sharefs.mutex);
	count = 0;
	while (mp->ms.m_sharefs.busy) {
		sam_cv_wait1sec_sig(&mp->ms.m_sharefs.get_cv,
		    &mp->ms.m_sharefs.mutex);
		if (++count > maxlease) {
			count = 0;
			cmn_err(CE_NOTE,
			    "SAM-QFS: %s: Failing over to new "
			    "server %s: busy threads = %d",
			    mp->mt.fi_name, server, mp->ms.m_sharefs.busy);
		}
	}
	mutex_exit(&mp->ms.m_sharefs.mutex);

	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status |= (FS_FROZEN | FS_LOCK_HARD);
	mp->mt.fi_status &= ~FS_FREEZING;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	cmn_err(CE_NOTE,
	    "SAM-QFS: %s: Failing over to new server %s, frozen (%x)",
	    mp->mt.fi_name, server, mp->mt.fi_status);

	TRACE(T_SAM_FAILOVER3, mp, mp->mt.fi_status, mp->ms.m_sharefs.busy,
	    mp->ms.m_cl_active_ops);
}


/*
 * ----- sam_notify_staging_clients
 */

void
sam_notify_staging_clients(sam_node_t *ip)
{
	int nl;
	sam_lease_ino_t *llp;
	sam_callout_arg_t callout;

	callout.p.stage.copy = ip->copy;
	callout.p.stage.flags = ip->flags.bits;
	callout.p.stage.error = ip->stage_err;

	/*
	 * Update file size for clients with leases.
	 */
	mutex_enter(&ip->ilease_mutex);
	llp = ip->sr_leases;
	if (llp) {
		for (nl = 0; nl < llp->no_clients; nl++) {
			if (llp->lease[nl].leases &&
			    (llp->lease[nl].client_ord !=
			    ip->mp->ms.m_client_ord)) {
				SAM_COUNT64(shared_server, callout_stage);
				sam_proc_callout(ip, CALLOUT_stage,
				    SR_FORCE_SIZE,
				    llp->lease[nl].client_ord, &callout);
			}
		}
	}
	mutex_exit(&ip->ilease_mutex);
}


/* ARGSUSED */
void
qfs_checkclean(sam_mount_t *mp)
{
	/* LQFS_TODO */
}


int
lqfs_flush(sam_mount_t *mp)
{
	int error = 0;

	/*
	 * Flush any outstanding transactions and roll the log
	 * only if we are supposed to do, i.e. LDL_NOROLL not set.
	 * We can not simply check for fs_ronly here since fsck also may
	 * use this code to roll the log on a read-only filesystem, e.g.
	 * root during early stages of boot, if other then a sanity check is
	 * done, it will clear LDL_NOROLL before.
	 * In addition we assert that the deltamap does not contain any deltas
	 * in case LDL_NOROLL is set since this is not supposed to happen.
	 */
	if (TRANS_ISTRANS(mp)) {
		ml_unit_t	*ul	= LQFS_GET_LOGP(mp);
		mt_map_t	*mtm	= ul->un_deltamap;

		if (ul->un_flags & LDL_NOROLL) {
			ASSERT(mtm->mtm_nme == 0);
		} else {
			curthread->t_flag |= T_DONTBLOCK;
			TRANS_BEGIN_SYNC(mp, TOP_COMMIT_FLUSH, TOP_COMMIT_SIZE,
			    error);
			if (!error) {
				TRANS_END_SYNC(mp, error,
				    TOP_COMMIT_FLUSH, TOP_COMMIT_SIZE);
			}
			curthread->t_flag &= ~T_DONTBLOCK;
			logmap_roll_dev(LQFS_GET_LOGP(mp)); /* Roll the log */
		}
	}

	return (error);
}


/*
 * ----- sam_onoff_client
 *
 * Process the system call to set/read the client on/off flag in the
 * client table.
 */
int				/* ERRNO if error, 0 if successful. */
sam_onoff_client(
	void *arg,		/* Pointer to arguments */
	int size,		/* Size of argument struct */
	cred_t *credp)		/* Credentials */
{
	sam_onoff_client_arg_t args;	/* Arguments into syscall */
	sam_mount_t *mp;
	client_entry_t *clp;
	int f;
	int error = 0;

	/*
	 * Copy in user arguments
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	/*
	 * Look up file system, check for permission, mounted, shared & server
	 */
	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		goto out;
	}
	if (!(mp->mt.fi_status & FS_MOUNTED)) {
		error = EXDEV;
		goto out;
	}
	if (!SAM_IS_SHARED_FS(mp) || !SAM_IS_SHARED_SERVER(mp)) {
		error = ENOTTY;
		goto out;
	}

	/*
	 * Look up client table entry
	 */
	if (args.clord < 1 || args.clord > mp->ms.m_maxord) {
		error = ENOENT;
		goto out;
	}
	clp = sam_get_client_entry(mp, args.clord, 1);
	if (clp == NULL) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Set on/off status in client table
	 */
	mutex_enter(&mp->ms.m_cl_wrmutex);
	f = clp->cl_flags & (SAM_CLIENT_OFF | SAM_CLIENT_OFF_PENDING);
	switch (args.command) {

	case SAM_ONOFF_CLIENT_OFF:
		if (f == 0) {
			clp->cl_flags |= SAM_CLIENT_OFF_PENDING;
			clp->cl_offtime = 0;
			sam_taskq_add(sam_onoff_client_delay, mp, NULL, hz / 2);
		}
		break;

	case SAM_ONOFF_CLIENT_ON:
		clp->cl_flags &= ~(SAM_CLIENT_OFF | SAM_CLIENT_OFF_PENDING);
		clp->cl_offtime = 0;
		break;

	case SAM_ONOFF_CLIENT_READ:
		break;

	default:
		error = EINVAL;
		mutex_exit(&mp->ms.m_cl_wrmutex);
		goto out;
	}
	mutex_exit(&mp->ms.m_cl_wrmutex);

	/*
	 * Send message to client
	 */
	if ((args.command == SAM_ONOFF_CLIENT_OFF) && (f == 0)) {
		/*
		 * Was up, going down.
		 * Use inode 1 (mp->mi.m_inodir) for sam_proc_notify
		 * to lookup mount point.
		 */
		sam_proc_notify(mp->mi.m_inodir, NOTIFY_hostoff, args.clord,
		    NULL, 0);
	}

	/*
	 * Return old flags to user
	 */
	args.ret = f;
	if (copyout((caddr_t)&args.ret,
	    (char *)arg + offsetof(sam_onoff_client_arg_t, ret),
	    sizeof (args.ret))) {
		error = EFAULT;
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_onoff_client_delay
 *
 * Process sam_onoff_client requests - move clients from SAM_CLIENT_OFF_PENDING
 * state to SAM_CLIENT_OFF state.
 */
static void
sam_onoff_client_delay(
	sam_schedule_entry_t *entry)	/* Task queue callback entry */
{
	sam_mount_t *mp = entry->mp;
	client_entry_t *clp;
	int ord;
	int off_pending = 0;

	mutex_enter(&samgt.schedule_mutex);
	sam_taskq_remove(entry);	/* releases above mutex */

	/*
	 * Loop over all clients for SAM_CLIENT_OFF_PENDING clients
	 */
	for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
		clp = sam_get_client_entry(mp, ord, 0);
		if ((clp->cl_flags & SAM_CLIENT_OFF_PENDING) == 0) {
			continue;
		}
		off_pending++;

		/*
		 * Wait for client to unmount for 20 sec., then off anyway.
		 */
		if ((clp->cl_offtime++ >= 40) ||
		    ((clp->cl_status & FS_MOUNTED) == 0)) {
			/*
			 * Move from SAM_CLIENT_OFF_PENDING to
			 * SAM_CLIENT_OFF state.
			 */
			mutex_enter(&mp->ms.m_cl_wrmutex);
			clp->cl_flags &= ~SAM_CLIENT_OFF_PENDING;
			clp->cl_flags |= SAM_CLIENT_OFF;
			mutex_exit(&mp->ms.m_cl_wrmutex);
			off_pending--;

			/*
			 * Signal server sam_read_sock thread (and only
			 * that thread) to exit.  sam-sharefsd ignores most
			 * signals except SIGEMT.  This will also indirectly
			 * decrement the mp->ms.m_no_clients count.
			 */
			if (clp->cl_thread != NULL) {
				tsignal(clp->cl_thread, SIGEMT);
				delay(hz / 2);
			}

			/*
			 * Mark as unmounted if not already unmounted.
			 */
			if (clp->cl_status & FS_MOUNTED) {
				mutex_enter(&mp->ms.m_cl_wrmutex);
				clp->cl_status &= ~FS_MOUNTED;
				mutex_exit(&mp->ms.m_cl_wrmutex);
			}
		}
	}
	if (off_pending > 0) {
		sam_taskq_add(sam_onoff_client_delay, mp, NULL, hz / 2);
	}
}


/*
 * ----- sam_change_features
 *
 * Change/add features to SAM-QFS file system.
 */
int				/* ERRNO if error, 0 if successful. */
sam_change_features(
	void *arg,		/* Pointer to arguments */
	int size,		/* Size of argument struct */
	cred_t *credp)		/* Credentials */
{
	sam_change_features_arg_t args;	/* Arguments into syscall */
	sam_mount_t *mp;
	sam_sbinfo_t *sblk;	/* Pointer to superblock */
	int f;
	int error = 0;

	/*
	 * Copy in user arguments
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * Look up file system, check for permission, mounted, shared & server
	 */
	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EACCES;
		goto out;
	}
	if (!(mp->mt.fi_status & FS_MOUNTED)) {
		error = EXDEV;
		goto out;
	}
	if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER(mp)) {
		error = ENOTTY;
		goto out;
	}

	sblk = &mp->mi.m_sbp->info.sb;
	switch (args.command) {

	case SAM_CHANGE_FEATURES_ADD_V2A:
		if (!SAM_MAGIC_V2_OR_HIGHER(sblk)) {
			/* Cannot upgrade from V1 */
			error = ENXIO;
			goto out;
		}
		if (SAM_MAGIC_V2A_OR_HIGHER(sblk)) {
			error = EEXIST;
			goto out;
		}
		cmn_err(CE_NOTE, "SAM-QFS: %s: sam_change_features: "
		    "Upgrading from V2 to V2A file system.", mp->mt.fi_name);
		mutex_enter(&mp->mi.m_sblk_mutex);
		sblk->magic = SAM_MAGIC_V2A;
		mutex_exit(&mp->mi.m_sblk_mutex);
		error = sam_update_the_sblks(mp);
		break;

	default:
		cmn_err(CE_WARN, "SAM-QFS: %s: Bad sam_change_features "
		    "request %d", mp->mt.fi_name, args.command);
		error = EINVAL;
		goto out;
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}
