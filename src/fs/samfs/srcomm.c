/*
 * ----- srcomm.c - Communication Interface for the shared file system.
 *
 *	This is the interface module that handles the server socket requests.
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

#pragma ident "$Revision: 1.38 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

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
#include <sys/un.h>
#include <sys/strsubr.h>
#include <sys/callb.h>
#include <sys/t_kuser.h>
#include <sys/tihdr.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include <sam/types.h>
#include <sam/syscall.h>

#include "scd.h"
#include "inode.h"
#include "mount.h"
#include "global.h"
#include "extern.h"
#include "rwio.h"
#include "debug.h"
#include "trace.h"
#include "kstats.h"
#include "samhost.h"

/*
 * Maximum number of times the server tries to send
 * a message to a client when the send returns EAGAIN.
 */
#define	SAM_MAX_SEND_TRIES	6
/*
 * Number of send tries to a unresponsive client
 * that has been flagged with SAM_CLIENT_SOCK_BLOCKED.
 */
#define	SAM_MIN_SEND_TRIES	3

static int sam_add_client(sam_mount_t *mp, char *hname, int client_ord,
			uint64_t);
static void sam_clear_client(sam_mount_t *mp, client_entry_t *clp);

extern int sam_max_shared_hosts;

/*
 * ----- sam_server_rdsock - Process the server read socket command.
 *
 *	Called when the sam-sharefsd server thread issues a syscall,
 *  SC_server_rdsock.
 *  NOTE. This thread reads from the socket in the file system.
 */

int				/* ERRNO if error, 0 if successful. */
sam_server_rdsock(
	void *arg,		/* Pointer to arguments. */
	int size,		/* Size of the arguments */
	cred_t *credp)		/* Pointer to the credentials */
{
	sam_mount_t *mp;
	struct sam_syscall_rdsock rdsock;
	int client_ord;
	client_entry_t *clp;
	vnode_t *vp;
	char *rdmsg;
	int error;


	if (size != sizeof (rdsock) ||
	    copyin(arg, (caddr_t)&rdsock, sizeof (rdsock))) {
		return (EFAULT);
	}
	rdmsg = (char *)rdsock.msg.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		rdmsg = (char *)rdsock.msg.p64;
	}

	/*
	 * Find mount point. Then decrement syscall count which was
	 * incremented in sam_find_filesystem.
	 */
	if ((mp = sam_find_filesystem(rdsock.fs_name)) == NULL) {
		return (ENOENT);
	}
	SAM_SHARE_DAEMON_HOLD_FS(mp);
	SAM_SYSCALL_DEC(mp, 0);

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EPERM;
		goto out;
	}

	if (!SAM_IS_SHARED_FS(mp)) {
		error = ENOTTY;
		goto out;
	}

	/*
	 * Server daemon restarting, verify there is not a previous connection.
	 * Get and hold current socket vnode.
	 */
	error = sam_add_client(mp, rdsock.hname, rdsock.hostord, rdsock.tags);
	if (error != 0) {
		goto out;
	}
	client_ord = rdsock.hostord;
	clp = sam_get_client_entry(mp, client_ord, 0);
	ASSERT(clp != NULL);
	mutex_enter(&clp->cl_wrmutex);
	if (clp->cl_sh.sh_fp) {
		cmn_err(CE_NOTE,
		    "SAM-FS: server already connected to %s: fs %s",
		    clp->hname, mp->mt.fi_name);
		mutex_exit(&clp->cl_wrmutex);
		error = EISCONN;
		goto out;
	}
	clp->cl_thread = curthread;
	if ((error = sam_set_sock_fp(rdsock.sockfd, &clp->cl_sh)) == 0) {
		vp = clp->cl_sh.sh_fp->f_vnode;
		if (clp->cl_flags & SAM_CLIENT_SOCK_BLOCKED) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Blocked client %s reconnected",
			    mp->mt.fi_name, clp->hname);
			clp->cl_flags &= ~SAM_CLIENT_SOCK_BLOCKED;
		}
		mutex_exit(&clp->cl_wrmutex);

		/*
		 * Get requests from the shared client(s).
		 */
		error = sam_read_sock(FALSE, client_ord, mp, vp, rdmsg, credp);
		sam_clear_client(mp, clp);
		mutex_enter(&clp->cl_wrmutex);
	}
	clp->cl_thread = NULL;
	mutex_exit(&clp->cl_wrmutex);

out:
	SAM_SHARE_DAEMON_RELE_FS(mp);
	return (error);
}


/*
 * ----- sam_write_to_client - Send server message to client.
 *
 * Write directly to sockfs.
 *
 */

int					/* ERRNO if error, 0 if successful. */
sam_write_to_client(sam_mount_t *mp, sam_san_message_t *msg)
{
	vnode_t *vp;
	client_entry_t *clp;
	int ord;
	int error;
	clock_t delay_hz = hz;
	int send_tries = 0;
	int max_send_tries = SAM_MAX_SEND_TRIES;

	/*
	 * Set the server who processed this messages. It may be necessary
	 * for the client to reissue if the server changed because of failover.
	 */
	msg->hdr.originator = SAM_SHARED_SERVER;
	msg->hdr.server_ord = mp->ms.m_client_ord;
	ord = msg->hdr.client_ord;
	if (mp->ms.m_clienti == NULL || (ord > mp->ms.m_max_clients)) {
		return (ENOTCONN);
	}
	if (ord >= 1) {
		clp = sam_get_client_entry(mp, ord, 0);
		if (clp == NULL) {
			return (ENOTCONN);
		}
resend1:
		error = 0;
		mutex_enter(&clp->cl_wrmutex);

		if (clp->cl_flags & SAM_CLIENT_SOCK_BLOCKED) {
			/*
			 * Don't spend a lot of time if this
			 * client has been unresponsive in the past.
			 */
			max_send_tries = SAM_MIN_SEND_TRIES;
		}

		vp = clp->cl_sh.sh_fp ? clp->cl_sh.sh_fp->f_vnode : NULL;
		if (vp != NULL) {
			VN_HOLD(vp);
			SAM_SOCK_READER_LOCK(&clp->cl_sh);
			mutex_exit(&clp->cl_wrmutex);
			TRACE(T_SAM_CL_WRCLNT, mp, ord,
			    (msg->hdr.command<<16)|msg->hdr.operation,
			    msg->hdr.seqno);
			SAM_COUNT64(shared_server, msg_out);

			send_tries++;

			error = sam_put_sock_msg(mp, vp, msg,
			    &clp->cl_sh.sh_wmutex);

			SAM_SOCK_READER_UNLOCK(&clp->cl_sh);
			if (error) {
				TRACE(T_SAM_CL_WRCLNT_ER1, mp, error, ord, 0);
				if (error == EAGAIN) {
					if (send_tries < max_send_tries) {
						delay(delay_hz);
						delay_hz <<= 1;
						VN_RELE(vp);
						goto resend1;
					}
					mutex_enter(&clp->cl_wrmutex);
					clp->cl_flags |=
					    SAM_CLIENT_SOCK_BLOCKED;
					mutex_exit(&clp->cl_wrmutex);
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: EAGAIN "
					    "sending to client %s",
					    mp->mt.fi_name, clp->hname);
				}
				if (SOCKET_FATAL(error)) {
					sam_clear_client(mp, clp);
					error = ENOTCONN;
				}
			}
			VN_RELE(vp);
		} else {
			mutex_exit(&clp->cl_wrmutex);
			error = ENOTCONN;
			TRACE(T_SAM_CL_WRCLNT_ER2, mp, error, ord, 0);
		}
	} else if (ord == 0) {
		/*
		 * Process callout message from the server. Skip sending to
		 * server.
		 */
		for (ord = 1; ord <= mp->ms.m_max_clients; ord++) {
			clp = sam_get_client_entry(mp, ord, 0);
			if (clp == NULL || ord == mp->ms.m_client_ord) {
				continue;
			}
			send_tries = 0;
resend2:
			error = 0;
			mutex_enter(&clp->cl_wrmutex);

			if (clp->cl_flags & SAM_CLIENT_SOCK_BLOCKED) {
				/*
				 * Don't spend a lot of time if this
				 * client has been unresponsive in the past.
				 */
				max_send_tries = SAM_MIN_SEND_TRIES;
			}

			vp = clp->cl_sh.sh_fp ?
			    clp->cl_sh.sh_fp->f_vnode : NULL;
			if (vp) {
				VN_HOLD(vp);
				SAM_SOCK_READER_LOCK(&clp->cl_sh);
				mutex_exit(&clp->cl_wrmutex);
				TRACE(T_SAM_CL_WRCALLOUT, mp, ord,
				    (msg->hdr.command<<16)|
				    msg->hdr.operation, msg->hdr.seqno);
				msg->hdr.client_ord = ord;
				SAM_COUNT64(shared_server, msg_out);

				send_tries++;

				error = sam_put_sock_msg(mp, vp, msg,
				    &clp->cl_sh.sh_wmutex);

				SAM_SOCK_READER_UNLOCK(&clp->cl_sh);
				if (error) {
					TRACE(T_SAM_CL_WRCLNT_ER3, mp, error,
					    ord, 0);
					if (error == EAGAIN) {
						if (send_tries
						    < max_send_tries) {
							delay(delay_hz);
							delay_hz <<= 1;
							VN_RELE(vp);
							goto resend2;
						}
						mutex_enter(&clp->cl_wrmutex);
						clp->cl_flags |=
						    SAM_CLIENT_SOCK_BLOCKED;
						mutex_exit(&clp->cl_wrmutex);
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: EAGAIN "
						    "sending to client %s",
						    mp->mt.fi_name, clp->hname);
					}
					if (SOCKET_FATAL(error)) {
						sam_clear_client(mp, clp);
						error = ENOTCONN;
					}
				}
				VN_RELE(vp);
			} else {
				mutex_exit(&clp->cl_wrmutex);
			}
		}
	} else {
		error = ENOTCONN;
		TRACE(T_SAM_CL_WRCLNT_ER4, mp, error, ord, 0);
	}
done:
	return (error);
}


/*
 * ----- sam_add_client - Add shared client to server's list of clients.
 *
 * Set the client hname and ordinal in the entry.
 */

static int		/* Client ordinal, 1 .. n */
sam_add_client(sam_mount_t *mp, char *hname, int clord, uint64_t client_tags)
{
	client_entry_t *clp;
	int ret = EINVAL;
	extern int sam_max_shared_hosts;

	if (hname[0] == '\0' || clord < 1 || clord > sam_max_shared_hosts) {
		return (ret);
	}

	mutex_enter(&mp->ms.m_cl_wrmutex);
	if (!mp->ms.m_clienti) {
		sam_init_client_array(mp);
	}

	clp = sam_get_client_entry(mp, clord, 1);
	if (clp == NULL) {
		cmn_err(CE_WARN, "SAM-QFS: %s: sam_add_client "
		    "client entry allocation failed for host %s at ordinal %d",
		    mp->mt.fi_name, hname, clord);
		ret = EFAULT;
		goto out;
	}
	if (clp->cl_flags & SAM_CLIENT_OFF) {
		cmn_err(CE_WARN, "SAM-QFS: %s: sam_add_client "
		    "connection attempt from down host %s ordinal %d",
		    mp->mt.fi_name, hname, clord);
		ret = EHOSTDOWN;
		goto out;
	}
	if (clp->cl_sh.sh_fp == NULL) {
		strncpy(clp->hname, hname, sizeof (clp->hname));
		clp->cl_tags = client_tags;
		mp->ms.m_no_clients++;
		mp->ms.m_max_clients = MAX(mp->ms.m_max_clients, clord);
		mp->ms.m_max_clients = MAX(mp->ms.m_max_clients,
		    mp->ms.m_maxord);
		ret = 0;
	} else {
		ret = EEXIST;
	}

out:
	mutex_exit(&mp->ms.m_cl_wrmutex);
	return (ret);
}


/*
 * ----- sam_init_client_array(mp)
 *
 * Allocate and initialize the per-client array entries for a shared FS.
 */
void
sam_init_client_array(sam_mount_t *mp)
{
	ASSERT(MUTEX_HELD(&mp->ms.m_cl_wrmutex));
	ASSERT(mp->ms.m_clienti == NULL);

	/*
	 * Allocate the incore client host table index array.
	 * This is an array of pointers. Each pointer points to
	 * an array of 256 client host entries.
	 */
	mp->ms.m_clienti = (client_entry_t **)kmem_zalloc(
	    (SAM_INCORE_HOSTS_INDEX_SIZE *
	    sizeof (client_entry_t *)), KM_SLEEP);

	mp->ms.m_max_clients = 0;
	mp->ms.m_no_clients = 0;
}

/*
 * ----- sam_clear_client - clear the fp for this socket.
 */

static void
sam_clear_client(sam_mount_t *mp, client_entry_t *clp)
{
	int cleared = 0;

	mutex_enter(&clp->cl_wrmutex);
	if (clp->cl_sh.sh_fp) {
		sam_clear_sock_fp(&clp->cl_sh);
		clp->hname[0] = '\0';
		cleared = 1;
	}
	mutex_exit(&clp->cl_wrmutex);

	if (cleared) {
		mutex_enter(&mp->ms.m_cl_wrmutex);
		mp->ms.m_no_clients--;
		mutex_exit(&mp->ms.m_cl_wrmutex);
	}
}


/*
 * ----- sam_get_client_entry ---
 *
 * Return a pointer to the client host table entry
 * at the specified ordinal. This routine can
 * return a NULL pointer if the incore table
 * has not been setup or the 256 host chunk
 * that contains the specified ordinal has
 * no entries and has not been allocated and
 * create is not specified.
 */
client_entry_t *
sam_get_client_entry(sam_mount_t *mp, int clord, int create)
{
	int i;
	int clordi;
	int array_index;
	int chunk_index;
	client_entry_t *chunkp;
	client_entry_t *clp = NULL;

	if (mp == NULL) {
		return (NULL);
	}

	if (mp->ms.m_clienti == NULL) {
		/*
		 * The client index array has not
		 * been initialized.
		 */
		return (NULL);
	}

	/*
	 * Convert 1 based ordinal into 0 based index.
	 */
	clordi = clord - 1;

	/*
	 * Get the index into the array of
	 * pointers to the 256 entry chunks.
	 */
	array_index = clordi >> 8;
	chunkp = mp->ms.m_clienti[array_index];

	if (chunkp == NULL) {
		if (!create) {
			return (NULL);
		}
		/*
		 * This chunk of SAM_INCORE_HOSTS_TABLE_INC entries
		 * have not yet been allocated and create is specified.
		 */
		chunkp = (client_entry_t *)kmem_zalloc(
		    (SAM_INCORE_HOSTS_TABLE_INC *
		    sizeof (client_entry_t)), KM_SLEEP);

		if (chunkp == NULL) {
			return (NULL);
		}

		/*
		 * Initialize all the entries in this chunk.
		 */
		for (i = 0, clp = chunkp;
		    i < SAM_INCORE_HOSTS_TABLE_INC; i++, clp++) {
			sam_mutex_init(&clp->cl_wrmutex,
			    NULL, MUTEX_DEFAULT, NULL);
			sam_mutex_init(&clp->cl_msgmutex,
			    NULL, MUTEX_DEFAULT, NULL);
			SAM_SOCK_HANDLE_INIT(&clp->cl_sh);
			list_create(&clp->queue.list, sizeof (sam_msg_array_t),
			    offsetof(sam_msg_array_t, node));
		}

		/*
		 * Save the pointer in the correct spot in
		 * the index array.
		 */
		mp->ms.m_clienti[array_index] = chunkp;
	}

	/*
	 * Get the index into the chunk that contains
	 * the requested client entry.
	 */
	chunk_index = clordi & (SAM_INCORE_HOSTS_TABLE_INC - 1);
	clp = &chunkp[chunk_index];

	return (clp);
}


/*
 * ----- sam_free_incore_host_table -
 *
 * Free the entire incore client host table.
 */
void
sam_free_incore_host_table(sam_mount_t *mp)
{
	int i, j;
	client_entry_t *clp;
	sam_msg_array_t *tmep;

	if (mp->ms.m_clienti) {
		for (i = 0; i < SAM_INCORE_HOSTS_INDEX_SIZE; i++) {
			client_entry_t *chunkp;

			chunkp = mp->ms.m_clienti[i];
			mp->ms.m_clienti[i] = NULL;

			if (chunkp) {
				for (j = 0;
				    j < SAM_INCORE_HOSTS_TABLE_INC; j++) {
					/*
					 * Cleanup each client host entry.
					 */
					clp = &chunkp[j];
					mutex_destroy(&clp->cl_wrmutex);
					while ((tmep =
					    list_dequeue(&clp->queue.list)) !=
					    NULL) {
						kmem_cache_free(
						    samgt.msg_array_cache,
						    tmep);
					}
				}
				/*
				 * Free each chunk of the client hosts table.
				 */
				kmem_free((void *)chunkp,
				    SAM_INCORE_HOSTS_TABLE_INC *
				    sizeof (client_entry_t));
			}
		}
		/*
		 * Free the client host table index array.
		 */
		kmem_free((void *)mp->ms.m_clienti,
		    SAM_INCORE_HOSTS_INDEX_SIZE * sizeof (client_entry_t *));
		mp->ms.m_clienti = NULL;
	}
}
