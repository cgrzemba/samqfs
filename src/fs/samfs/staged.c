/*
 * ----- staged.c - Process the sam stage daemon functions.
 *
 *	Processes the stage daemon functions supported on the SAM File System.
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

#pragma ident "$Revision: 1.82 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/sunddi.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/resource.h"
#include "sam/samevent.h"
#include "pub/sam_errno.h"
#include <pub/stat.h>
#include <aml/tar.h>

#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "extern.h"
#include "arfind.h"
#include "fsdaemon.h"
#include "rwio.h"
#include "debug.h"
#include "syslogerr.h"
#include "trace.h"

int sam_stagerd_file(sam_node_t *ip, offset_t length, cred_t *credp);


/* ----- Private functions for stager daemon support. */
static int sam_build_stager_multivolume_copy(sam_node_t *ip, sam_id_t id,
				int copy, sam_stage_request_t *req,
				sam_stage_request_t *req_ext);

int sam_stage_n_umem = 1;

/*
 * ----- sam_stagerd_file - stage an archived file via stager daemon.
 *
 * Issue the stage request to stage the file, but do not wait for a
 * response from the daemon. The caller waits for a signal in
 * sam_proc_offline.
 * NOTE. inode_rwl WRITER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_stagerd_file(
	sam_node_t *ip,		/* pointer to vnode. */
	offset_t length,	/* requested length */
	cred_t *credp)		/* credentials pointer */
{
	int error;
	offset_t cur_size;
	sam_stage_request_t *req;
	int req_ext_cnt;
	sam_stage_request_t *req_ext;
	int i;

	req = kmem_alloc(sizeof (*req), KM_SLEEP);

	/*
	 * Start associative staging if stage_all enabled.
	 */
	if (ip->flags.b.stage_all) {
		if (error = sam_send_stageall_cmd(ip)) {
			goto out;
		}
	}

	if (error = sam_build_stagerd_req(ip, -1, req, &req_ext_cnt, &req_ext,
	    credp)) {
		goto out;
	}

	/*
	 * Allocate the file for stage with wait. Wait if caller
	 * waiting on stage and NOT archiver.
	 */
	cur_size = ip->size;
	if (length && !ip->flags.b.nowaitspc) {
		offset_t size;

		/*
		 *  Set stage wait flag on request.
		 */
		req->filesys.wait = 1;

		/*
		 * If stage -n, not NFS, total allocated umem is <
		 * physmem, and buffer is less than 1% of physmem,
		 * allocate buffer in memory pages which flush to swap.
		 */
		size = ip->stage_len;
		if (sam_stage_n_umem &&
		    ip->flags.b.stage_n && !SAM_THREAD_IS_NFS() &&
		    ((size + ip->mp->mi.m_umem_size) <=
		    (physmem * PAGESIZE)) &&
		    (size <= ((physmem * PAGESIZE) / 100))) {
			if (ip->st_buf) {
				if (size > ip->st_umem_len) {
					atomic_add_64(
					    &ip->mp->mi.m_umem_size,
					    -ip->st_umem_len);
					ddi_umem_free(ip->st_cookie);
					ip->st_buf = NULL;
					ip->st_umem_len = 0;
				}
			}
			if (ip->st_buf == NULL) {
				if ((ip->st_buf =
				    ddi_umem_alloc((size_t)size,
				    (DDI_UMEM_PAGEABLE|DDI_UMEM_SLEEP),
				    &ip->st_cookie))) {
					ip->st_umem_len = size;
					atomic_add_64(
					    &ip->mp->mi.m_umem_size, size);
				}
			}
		}
	}

	if (error == 0) {
		sam_node_t *bip;

		if (S_ISSEGS(&ip->di)) {
			bip = ip->segment_ip;
		} else {
			bip = ip;
		}
		ip->flags.b.changed = 1;
		ip->size = cur_size;
		req->pid = curproc->p_pid;
		req->user = crgetuid(credp);
		req->owner = bip->di.uid;
		req->group = bip->di.gid;
		req->filesys.stage_off = ip->stage_off;
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		TRACES(T_SAM_FIFO_STAGE, SAM_ITOV(ip),
		    (char *)req->arcopy[0].section[0].vsn);
		error = sam_send_stage_cmd(ip->mp, SCD_stager, req,
		    sizeof (*req));

		/*
		 *	Send any request extension, used for multivolume.
		 */
		for (i = 0; i < req_ext_cnt; i++) {
			if (error == 0) {
				req_ext[i].pid = curproc->p_pid;
				req_ext[i].user = crgetuid(credp);
				req_ext[i].owner = bip->di.uid;
				req_ext[i].group = bip->di.gid;
				req_ext[i].filesys.stage_off = ip->stage_off;
				error = sam_send_stage_cmd(ip->mp,
				    SCD_stager, &req_ext[i],
				    sizeof (*req));
			}
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	if (req_ext_cnt > 0) {
		kmem_free((void *)req_ext, (req_ext_cnt *
		    sizeof (sam_stage_request_t)));
	}

	if (error) {
		/*
		 * Clear staging flag so blocks will be released in
		 * sam_drop_ino.  Decrement outstanding stage count and wake up
		 * any waiting processes.
		 */
		ip->flags.b.staging = 0;
		(void) sam_drop_ino(ip, CRED());
	}

out:
	kmem_free(req, sizeof (*req));
	return (error);
}


/*
 * ----- sam_build_stagerd_req - stage an archived file via stager daemon.
 *
 * Build a request to stage the file via stager daemon.
 */

int
sam_build_stagerd_req(
	sam_node_t *ip,
	int copy,
	sam_stage_request_t *req,
	int *req_ext_cnt,		/* ret count of extended stagerd */
					/*   requests */
	sam_stage_request_t **req_ext,	/* ret extended stagerd request ptr */
	cred_t *credp)			/* credentials pointer */
{
	int error;
	struct sam_perm_inode *permip;
	buf_t *bp;
	int n_vsns;
	int i, j;
	int mask;
	int max_cnt;
	offset_t partial_size;
	int truncate_file = FALSE;

	*req_ext_cnt = 0;
	*req_ext = NULL;

	/*
	 * If shared file system, always directio stage. Otherwise,
	 * directio or pagedio is based on stagerd.cmd directive,
	 * as set in syscall_stage_response().
	 */
	ip->flags.b.stage_directio = 0;
	if (SAM_IS_SHARED_FS(ip->mp) ||
	    (!ip->flags.b.stage_n && ip->flags.b.directio)) {
		if (vn_has_cached_data(SAM_ITOV(ip))) {
			sam_flush_pages(ip, B_INVAL);
		}
		ip->flags.b.stage_directio = 1;
	}

	bzero(req, sizeof (sam_stage_request_t));

	req->id = ip->di.id;
	req->fseq = ip->mp->mt.fi_eq;
	req->copy = ip->copy;
	req->len = ip->stage_len;
	req->offset = 0;

	if (ip->di.status.b.cs_val) {
		req->flags |= STAGE_CSVALID;
	}

	if (ip->di.status.b.cs_use) {
		req->flags |= STAGE_CSUSE;
		req->cs_algo = ip->di.cs_algo;
	}

	if (ip->flags.b.stage_p) {
		req->flags |= STAGE_PARTIAL;
	}

	if (ip->flags.b.stage_n) {

		truncate_file = TRUE;

		ip->stage_len = MAX(((ip->stage_off & ~PAGEMASK) +
		    (ip->stage_len + PAGESIZE - 1) & PAGEMASK),
		    ip->mp->mt.fi_stage_n_window);
		ip->stage_off = ip->stage_off & PAGEMASK;
		ip->stage_n_off = ip->stage_off;	/* Save this window */
		ip->stage_len = MIN(ip->stage_len,
		    ip->di.rm.size - ip->stage_off);

		req->flags |= STAGE_NEVER;
		req->len = ip->stage_len;
		req->offset = ip->stage_off;
	}

	if (error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip)) {
		return (error);
	}
	mask = 1;
	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (copy >= 0 && copy != i) {
			mask += mask;
			continue;
		}
		n_vsns = permip->ar.image[i].n_vsns;
		if (n_vsns) {				/* archive copy */
			req->arcopy[i].media = ip->di.media[i];
			req->arcopy[i].n_vsns = n_vsns;

			if (ip->di.arch_status & mask) {
				req->arcopy[i].flags |= STAGE_COPY_ARCHIVED;

				if (ip->di.ar_flags[i] & AR_damaged) {
					req->arcopy[i].flags |=
					    STAGE_COPY_DAMAGED;
				}
				if (ip->di.ar_flags[i] & AR_stale) {
					req->arcopy[i].flags |=
					    STAGE_COPY_STALE;
				}
				if (permip->ar.image[i].arch_flags &
				    SAR_diskarch) {
					req->arcopy[i].flags |=
					    STAGE_COPY_DISKARCH;
				}
				if (permip->ar.image[i].arch_flags &
				    SAR_pax_hdr) {
					req->arcopy[i].flags |=
					    STAGE_COPY_PAXHDR;
				}
			}
		}
		mask += mask;
	}

	/*
	 *	Determine maximum number of request extensions needed.
	 */
	max_cnt = 0;
	for (i = 0; i < MAX_ARCHIVE; i++) {
		int cnt;

		if (copy >= 0 && copy != i) {
			continue;
		}
		n_vsns = permip->ar.image[i].n_vsns;
		if (n_vsns == 1) {
			uint_t file_offset;

			req->arcopy[i].section[0].position =
			    (uint32_t)permip->ar.image[i].position;

			file_offset = permip->ar.image[i].file_offset;
			if ((permip->ar.image[i].arch_flags &
			    SAR_size_block) == 0) {
				file_offset = file_offset/TAR_RECORDSIZE;
			}
			req->arcopy[i].section[0].offset = file_offset;
			req->arcopy[i].section[0].length = ip->di.rm.size;
			bcopy(permip->ar.image[i].vsn,
			    req->arcopy[i].section[0].vsn,
			    sizeof (vsn_t));
		} else if (n_vsns > 1) {
			cnt = howmany(n_vsns, SAM_MAX_VSN_SECTIONS) - 1;
			if (cnt > max_cnt) {
				max_cnt = cnt;
			}
		}
	}

	if (max_cnt > 0) {
		sam_stage_request_t *ext;

		/*
		 * Allocate number of extension requests needed.
		 */
		ext = (sam_stage_request_t *)kmem_zalloc(
		    max_cnt * sizeof (sam_stage_request_t), KM_NOSLEEP);
		if (ext == NULL) {
			return (ENOMEM);
		}

		/*
		 * Initialize allocated extension requests.
		 */
		for (j = 0; j < max_cnt; j++) {
			bcopy(req, &ext[j], sizeof (sam_stage_request_t));
			ext[j].flags |= STAGE_EXTENDED;

			for (i = 0; i < MAX_ARCHIVE; i++) {
				ext[j].arcopy[i].ext_ord = j + 1;
			}
		}
		*req_ext_cnt = max_cnt;
		*req_ext = ext;
	}

	/*
	 *	Build stage request.
	 */
	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (copy >= 0 && copy != i) {
			continue;
		}
		n_vsns = permip->ar.image[i].n_vsns;

		if (n_vsns > 1) {
			sam_id_t id;

			if (ip->di.version >= SAM_INODE_VERS_2) {
				brelse(bp);
				if (error = sam_get_multivolume_copy_id(ip,
				    i, &id)) {
					return (error);
				}
			} else if (ip->di.version == SAM_INODE_VERS_1) {
				/* Previous vers */
				sam_perm_inode_v1_t *permip_v1 =
				    (sam_perm_inode_v1_t *)permip;

				id = permip_v1->aid[i];
				brelse(bp);
			}

			error = sam_build_stager_multivolume_copy(ip, id, i,
			    req, *req_ext);

			if (sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip)) {
				if (*req_ext_cnt > 0) {
					kmem_free((void *)*req_ext,
					    (*req_ext_cnt *
					    sizeof (sam_stage_request_t)));
					*req_ext_cnt = 0;
					*req_ext = NULL;
				}
				return (EIO);
			}
		}
	}

	brelse(bp);

	/*
	 *  If stage never, free blocks from previous window.
	 */
	if (truncate_file) {

		/*
		 *  If partial extents are online get partial size.
		 */
		partial_size = 0;
		if (ip->di.status.b.pextents) {
			partial_size = sam_partial_size(ip);
		}
		error = sam_proc_truncate(ip, partial_size, SAM_RELEASE, credp);
	}

	if (error) {
		if (*req_ext_cnt > 0) {
			kmem_free((void *)*req_ext,
			    (*req_ext_cnt * sizeof (sam_stage_request_t)));
			*req_ext_cnt = 0;
			*req_ext = NULL;
		}
	}
	return (error);
}


/*
 * ----- sam_build_stagerd_multivolume_copy
 *
 * Build copy information for multivolume request to stage the file
 * via stager daemon.
 */

static int
sam_build_stager_multivolume_copy(
	sam_node_t *ip,
	sam_id_t id,
	int copy,
	sam_stage_request_t *req,
	sam_stage_request_t *req_ext)	/* extended stagerd request pointer */
{
	int error = 0;
	buf_t *bp;
	struct sam_inode_ext *aip;
	int i;

	if (ip->di.version >= SAM_INODE_VERS_2) {	/* Version 2 or 3 */
		int n_vsns;
		int t_vsns = 0;
		int ext_idx = -1;
		int sct_idx = 0;

		/*
		 * Build copy information for max number of sections configured
		 * for stage daemon.
		 */
		while (id.ino) {

			/*
			 * Read archive copy's extension inode containing VSN
			 * section info.
			 */
			if (error = sam_read_ino(ip->mp, id.ino, &bp,
					(struct sam_perm_inode **)&aip)) {
				return (error);
			}

			if (EXT_HDR_ERR(aip, id, ip)) {
				sam_req_ifsck(ip->mp, -1,
				    "sam_build_stager_multivolume_copy: "
				    "EXT_HDR [1]",
				    &ip->di.id);
				brelse(bp);
				return (ENOCSI);
			}
			if (t_vsns == 0) {
				/* total vsns for copy */
				t_vsns = aip->ext.mva.t_vsns;
			}
			n_vsns = aip->ext.mva.n_vsns; /* vsns this extension */
			for (i = 0; i < n_vsns; i++) {

	/* N.B. Bad indentation here to meet cstyle requirements */
	if (ext_idx < 0) { /* filling first request */
		req->arcopy[copy].section[sct_idx].position =
		    aip->ext.mva.vsns.section[i].position;
		req->arcopy[copy].section[sct_idx].offset =
		    aip->ext.mva.vsns.section[i].offset/TAR_RECORDSIZE;
		req->arcopy[copy].section[sct_idx].length =
		    aip->ext.mva.vsns.section[i].length;
		bcopy(aip->ext.mva.vsns.section[i].vsn,
		    req->arcopy[copy].section[sct_idx].vsn, sizeof (vsn_t));
	} else {	/* filling extension req */
		req_ext[ext_idx].arcopy[copy].section[sct_idx].position =
		    aip->ext.mva.vsns.section[i].position;
		req_ext[ext_idx].arcopy[copy].section[sct_idx].offset =
		    aip->ext.mva.vsns.section[i].offset/TAR_RECORDSIZE;
		req_ext[ext_idx].arcopy[copy].section[sct_idx].length =
		    aip->ext.mva.vsns.section[i].length;
		bcopy(aip->ext.mva.vsns.section[i].vsn,
		    req_ext[ext_idx].arcopy[copy].section[sct_idx].vsn,
		    sizeof (vsn_t));
	}



				if (++sct_idx == SAM_MAX_VSN_SECTIONS) {
					/* max per request */
					ext_idx++;
					sct_idx = 0;
				}
			}
			id = aip->hdr.next_id;
			brelse(bp);

			t_vsns -= n_vsns;
			if (t_vsns == 0) {	/* all vsns processed */
				break;
			}
		}

	} else if (ip->di.version == SAM_INODE_VERS_1) { /* Previous version */
		int j, k;
		int sct_idx;

		/*
		 * Read archive copy's extension inode containing VSN section
		 * info.
		 */
		if (error = sam_read_ino(ip->mp, id.ino, &bp,
				(struct sam_perm_inode **)&aip)) {
			return (error);
		}

		if (EXT_HDR_ERR(aip, id, ip)) {
			sam_req_ifsck(ip->mp, -1,
			    "sam_build_stager_multivolume_copy: EXT_HDR [2]",
			    &ip->di.id);
			brelse(bp);
			return (ENOCSI);
		}

		/*
		 * From count of VSN sections in inode extension.  Build copy
		 * information for max number of sections configured for stage
		 * daemon.
		 */
		j = aip->ext.mv1.n_vsns;
		if (j > SAM_MAX_VSN_SECTIONS) {
			j = SAM_MAX_VSN_SECTIONS;
		}

		for (i = 0; i < j; i++) {
			req->arcopy[copy].section[i].position =
			    aip->ext.mv1.vsns.section[i].position;
			req->arcopy[copy].section[i].offset =
			    aip->ext.mv1.vsns.section[i].offset/TAR_RECORDSIZE;
			req->arcopy[copy].section[i].length =
			    aip->ext.mv1.vsns.section[i].length;
			bcopy(aip->ext.mv1.vsns.section[i].vsn,
			    req->arcopy[copy].section[i].vsn, sizeof (vsn_t));
		}
		if (i >= aip->ext.mv1.n_vsns) {
			sct_idx = -1;
			id = aip->hdr.next_id;
			brelse(bp);
		} else {
			sct_idx = i;
		}


		/*
		 * If needed, read more archive copy's extension inodes and
		 * build * request extension information for stage daemon.
		 */

		j = 0;
		k = 0;

		while (id.ino) {

			if (sct_idx == -1) {
				/*
				 * Read archive copy's extension inode
				 * containing VSN section information.
				 */
				if (error = sam_read_ino(ip->mp, id.ino, &bp,
					(struct sam_perm_inode **)&aip)) {
					return (error);
				}

				if (EXT_HDR_ERR(aip, id, ip)) {
					sam_req_ifsck(ip->mp, -1,
					    "sam_build_stager_multivolume_copy:"
					    " EXT_HDR [3]",
					    &ip->di.id);
					brelse(bp);
					return (ENOCSI);
				}
				sct_idx = 0;
			}
			for (i = sct_idx; i < aip->ext.mv1.n_vsns; i++) {
				req_ext[j].arcopy[copy].section[k].position =
				    aip->ext.mv1.vsns.section[i].position;
				req_ext[j].arcopy[copy].section[k].offset =
				    aip->ext.mv1.vsns.section[i].offset /
				    TAR_RECORDSIZE;
				req_ext[j].arcopy[copy].section[k].length =
				    aip->ext.mv1.vsns.section[i].length;
				bcopy(aip->ext.mv1.vsns.section[i].vsn,
				    req_ext[j].arcopy[copy].section[k].vsn,
				    sizeof (vsn_t));
			}
			k++;
			if (k == SAM_MAX_VSN_SECTIONS) {
				k = 0;
				j++;
			}

			sct_idx = -1;
			id = aip->hdr.next_id;
			brelse(bp);
		}
	}

	return (error);
}


/*
 * ----- sam_get_multivolume_copy_id - get the multivolume copy id.
 *
 * Return the inode num/gen of the first extension inode containing the
 * multivolume archive vsn info for the requested copy.
 *
 * Caller must have the inode read lock held.  This lock is not released.
 */
int					/* ERRNO, else 0 if successful. */
sam_get_multivolume_copy_id(
	sam_node_t *bip,		/* Pointer to base inode. */
	int copy,			/* Copy number to find. */
	sam_id_t *idp)			/* Ptr to id of first (returned). */
{
	int error = 0;
	sam_id_t eid;
	buf_t *bp;
	struct sam_inode_ext *eip;


	idp->ino = idp->gen = 0;

	if (bip->di.version == SAM_INODE_VERS_1) { /* Not current version */
		return (EINVAL);
	}
	if (!(bip->di.ext_attrs & ext_mva)) {	/* No multivolume exts. */
		return (EINVAL);
	}

	/* Find first inode containing copy info */
	eid = bip->di.ext_id;
	while (eid.ino) {
		if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
			break;
		}
		if (EXT_HDR_ERR(eip, eid, bip)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_multivolume_copy_id: EXT_HDR",
			    &bip->di.id);
			error = ENOCSI;
			brelse(bp);
			break;
		}
		if ((S_ISMVA(eip->hdr.mode)) && (copy == eip->ext.mva.copy)) {
			idp->ino = eid.ino;
			idp->gen = eid.gen;
			brelse(bp);
			break;
		}
		eid = eip->hdr.next_id;
		brelse(bp);
	}

	return (error);
}


/*
 * ----- sam_close_stage - close a staging file
 *
 *  Process stage daemon close -- clear the offline bit if entire
 *  file staged with no error, release the last segment if one, and wake
 *  up any processes waiting on the stage.
 *  NOTE. inode_rwl WRITER lock set on entry and exit.
 */

/* ARGSUSED1 */
int				/* ERRNO if error, 0 if successful. */
sam_close_stage(sam_node_t *ip, cred_t *credp)
{
	offset_t st_length;
	int process_stage = 0;

	ip->stage_size = 0;
	ip->flags.b.staging = 0;
	ip->flush_off = 0;			/* Flush behind stage offset */
	ip->flush_len = 0;			/* Flush behind stage length */
	st_length = ip->stage_off + ip->stage_len;

	if ((!ip->stage_err) && (ip->size == st_length)) {
		ip->real_stage_off = ip->stage_off;
		ip->real_stage_len = ip->stage_len;
		if (ip->flags.b.stage_n) {
			ip->real_stage_off = ip->stage_n_off;
			if (ip->stage_n_off < ip->stage_off) {
				ip->real_stage_len += (ip->stage_off -
				    ip->stage_n_off);
			}
		}
		/* If successful stage with no error */
		if (!ip->flags.b.stage_n) {
			if ((ip->size == ip->di.rm.size) ||
			    S_ISREQ(ip->di.mode) ||
			    S_ISSEGI(&ip->di)) {
				if (ip->di.status.b.offline) {
					time_t residence_time;
					int	copy;

					ip->di.status.b.offline = 0;
					ip->di.status.b.stage_failed = 0;
					if (ip->di.status.b.bof_online) {
						/* Partial extents */
						ip->di.status.b.pextents = 1;
					}
					residence_time = SAM_SECOND();
					ip->di.residence_time = residence_time;
					if (S_ISSEGS(&ip->di)) {
					ip->segment_ip->di.residence_time =
					    residence_time;
					ip->segment_ip->flags.b.changed = 1;
					}
					if (ip->mp->ms.m_fsev_buf) {
						sam_send_event(ip->mp, &ip->di,
						    ev_online, 0, 0,
						    ip->di.residence_time);
					}

					/*
					 * Clear archdone if any copy was
					 * unarchived.
					 */
					for (copy = 0; copy < MAX_ARCHIVE;
					    copy++) {
						if (ip->di.ar_flags[copy] &
						    AR_unarchived) {
						ip->di.status.b.archdone = 0;
							sam_send_to_arfind(ip,
							    AE_unarchive,
							    copy + 1);
						}
					}
				}
			} else {
				if (ip->flags.b.stage_p) {
					if (ip->size == sam_partial_size(ip)) {
						/* Partial extents */
						ip->di.status.b.pextents = 1;
					}
				} else {
					/* Process next section */
					process_stage = 1;
				}
			}
		} else if (ip->di.rm.ui.flags & RM_VOLUMES) {
			/* Process next section for stage_n */
			process_stage = 1;
		}
		if (SAM_IS_OBJECT_FILE(ip)) {
			sam_osd_update_blocks(ip, 0);
		}
	} else {
		if (ip->stage_err == 0)  {
			ip->stage_err = ECOMM;
		}

		/* For stage_n, indicate there was no data staged */
		ip->stage_len = ip->stage_off = 0;
		ip->real_stage_len = ip->real_stage_off = 0;

		/*
		 * Free blocks allocated for the stage. Flush any pages that are
		 * holding partial.
		 */
		(void) sam_drop_ino(ip, CRED());
		sam_flush_pages(ip, 0);
	}
	if (S_ISREQ(ip->di.mode) || S_ISSEGI(&ip->di)) {
		/*
		 * Free the extent used for the rm information or segment info.
		 */
		sam_flush_pages(ip, B_FREE);
	} else if (!(S_ISREG(ip->di.mode))) {
		sam_flush_pages(ip, 0);
	}
	/*
	 * If sam-stagerd is killed for failover, set ENOTACTIVE.  If sam-amld
	 * is not running and this is not a cancel error, then set the internal
	 * error to EREMCHG which means reissue this stage.  sam-amld is not
	 * started if only disk archiving is enabled (amld_pid=0)..
	 */
	if (SAM_IS_SHARED_FS(ip->mp) && (samgt.stagerd_pid < 0)) {
		ip->stage_err = ENOTACTIVE;
	} else if ((samgt.amld_pid < 0) && ip->stage_err &&
	    ip->stage_err != ECANCELED) {
		ip->stage_err = EREMCHG; /* Reissue this copy for the stage */
		process_stage = 1;
	}

	/*
	 * Clear stage_pid after pages flushed so modification date is
	 * not set. This signals the close has completed.
	 */
	ip->stage_pid = 0;

	/*
	 * Signal any threads waiting for file data to be staged.
	 * If anyone is waiting and there was a stage error, the
	 * wait routine should take care of moving onto the next
	 * archive copy.  If no one is waiting then this could have been a
	 * stage no-wait or an NFS read.  In either case, attempt to stage
	 * from the next archive copy.
	 * If shared fs and fail over is about to begin, signal will be sent
	 * after fs has frozen.
	 */
	if (SAM_IS_SHARED_FS(ip->mp)) {
		(void) sam_proc_rm_lease(ip, CL_STAGE, RW_WRITER);
		if (ip->stage_err != ENOTACTIVE) {
			sam_notify_staging_clients(ip);
		}
	}
	mutex_enter(&ip->rm_mutex);
	if (ip->rm_wait) {
		if (ip->stage_err != ENOTACTIVE) {
			cv_broadcast(&ip->rm_cv);
		}
		mutex_exit(&ip->rm_mutex);
	} else if (process_stage && !ip->flags.b.stage_n &&
	    !ip->flags.b.stage_p && (samgt.stagerd_pid > 0) &&
	    ip->di.status.b.cs_use == 0) {
		mutex_exit(&ip->rm_mutex);
		(void) sam_proc_offline(ip, (offset_t)0, (offset_t)0,
		    NULL, CRED(),
		    NULL);
	} else {
		mutex_exit(&ip->rm_mutex);
	}
	ip->flags.b.changed = 1;
	/*
	 * Reset stage_n and stage_all after stage completes if archiver
	 * is not archiving using direct tape to tape access.
	 *
	 */
	if (!ip->flags.b.staging) {
		ip->flags.b.stage_p = 0;
		if (!ip->flags.b.arch_direct) {
			ip->flags.b.stage_n = ip->di.status.b.direct;
			ip->flags.b.stage_all = ip->di.status.b.stage_all;
		}
	}
	return (0);
}


/*
 * ----- sam_stage_write_io - Write a SAM-QFS file that is being staged.
 *
 * Map the file offset to the correct page boundary.
 * Move the data from the stager's buffer to the file data pages.
 */

int				/* ERRNO if error, 0 if successful. */
sam_stage_write_io(
	vnode_t *vp,		/* Pointer to vnode */
	uio_t *uiop)		/* Pointer to user I/O vector array. */
{
	int error;
	sam_ssize_t nbytes, start_nbytes;
	sam_u_offset_t offset, reloff, start_off;
	caddr_t base, lbase;		/* Kernel address of mapped in block */
	sam_node_t *ip;
	offset_t size;

	ip = SAM_VTOI(vp);
	if (S_ISREQ(ip->di.mode)) {
		size = ip->di.psize.rmfile;
	} else if (S_ISSEGI(&ip->di)) {
		size = ip->di.rm.info.dk.seg.fsize;
	} else {
		size = ip->di.rm.size;
	}

	for (;;) {
		start_off = offset = uiop->uio_loffset;
		start_nbytes = uiop->uio_resid;
		reloff = offset & (MAXBSIZE - 1);
		nbytes = MIN(((sam_u_offset_t)MAXBSIZE - reloff),
		    uiop->uio_resid);
		ip->flush_len += nbytes;

		TRACE(T_SAM_STWRIO1, vp, (sam_tr_t)offset,
		    (sam_tr_t)reloff, nbytes);
		if (((offset + (uint_t)nbytes) > ip->size) ||
		    (nbytes == MAXBSIZE)) {
			if ((error = sam_map_block(ip, uiop->uio_loffset,
			    (offset_t)nbytes,
			    SAM_FORCEFAULT, NULL, CRED())) != 0) {
				return (error);
			}
		}

		if (!sam_vpm_enable) {
			SAM_SET_LEASEFLG(ip->mp);
			base = segmap_getmapflt(segkmap, vp, offset,
			    nbytes, 0, S_WRITE);
			SAM_CLEAR_LEASEFLG(ip->mp);
			lbase = (caddr_t)((sam_size_t)(base + reloff) &
			    PAGEMASK);
		}

		/*
		 * A maximum PAGESIZE number of bytes are transferred each
		 * uiomove pass because a uiomove across pages may cause cache
		 * to be invalidated.  If the page is not locked, it can cause a
		 * mmap read to get bad data.
		 */
		for (;;) {
			uint_t n, lreloff;

			offset = uiop->uio_loffset;
			lreloff = offset & ((sam_u_offset_t)PAGESIZE - 1);
			n = MIN(PAGESIZE, nbytes);

			/* Assert page-aligned offset. */
			/* Assert PAGESIZE io, or last write */
			ASSERT(lreloff == 0);
			ASSERT((n == PAGESIZE) || ((offset + n) >= size));

			/* Check asserts for non-DEBUG builds, return error. */
			if ((lreloff != 0) || ((n != PAGESIZE) &&
			    ((offset + n) < size))) {
				error = ECOMM;
				break;
			}

			if (sam_vpm_enable) {
				SAM_SET_LEASEFLG(ip->mp);
				error = vpm_data_copy(vp, offset, n, uiop,
				    0, NULL, 0, S_WRITE);
				SAM_CLEAR_LEASEFLG(ip->mp);

				if (error) {
					break;
				}
			} else {
				SAM_SET_LEASEFLG(ip->mp);
				segmap_pagecreate(segkmap, lbase,
				    PAGESIZE, F_SOFTLOCK);

				error = uiomove(lbase, n, UIO_WRITE, uiop);

				(void) segmap_fault(kas.a_hat, segkmap, lbase,
				    PAGESIZE, F_SOFTUNLOCK, S_WRITE);
				SAM_CLEAR_LEASEFLG(ip->mp);

				if (error) {
					break;
				}
			}

			/*
			 * Check for uiomove failure: new_offset - orig_offset
			 * != number of bytes moved. If so, return ECOMM.
			 * Should not happen.
			 */
			if ((uiop->uio_loffset - offset) != n) {
				error = ECOMM;
			}
			if (!sam_vpm_enable) {
				lbase += n;
			}
			nbytes -= n;
			if ((nbytes <= 0) || error) {
				break;
			}
		}
		if (sam_vpm_enable) {
			(void) vpm_sync_pages(vp, start_off, start_nbytes,
			    error ? SM_INVAL : SM_WRITE|SM_ASYNC|SM_DONTNEED);
		} else {
			segmap_release(segkmap, base, error ? SM_INVAL :
			    SM_WRITE | SM_ASYNC | SM_DONTNEED);
		}

		TRACE(T_SAM_STWRIO2, vp,
		    sam_vpm_enable ? (sam_tr_t)start_off : (sam_tr_t)base,
		    (sam_tr_t)uiop->uio_resid,
		    (sam_tr_t)reloff);
		ip->flags.b.stage_pages = 1; /* Dirty stage pages in memory */
		if (!ip->flags.b.staging) {
			error = ECANCELED;
		}
		if (error || (uiop->uio_resid <= 0)) {
			break;
		}
	}
	if (error == 0) {
		/*
		 * Flush behind stage pages to make them clean. This helps the
		 * overall staging time because stage pages are sync. written at
		 * close.  Do not flush stage_n pages.
		 */
		if (!ip->flags.b.stage_n && ip->mp->mt.fi_stage_flush_behind &&
		    (ip->flush_len > ip->mp->mt.fi_stage_flush_behind)) {
			(void) sam_flush_behind(ip, CRED());
		}
	} else {
		if (ip->stage_err == 0) {
			ip->stage_err = (short)error;
		}
	}
	return (error);
}


/*
 * ----- sam_stage_n_write_io - Write a SAM-QFS file that is being staged
 *
 * using the stage_n window.  Map the file offset to the correct page
 * boundary. Move the data from the stager's buffer to the stage_n data pages.
 */

int				/* ERRNO if error, 0 if successful. */
sam_stage_n_write_io(
	vnode_t *vp,		/* Pointer to vnode */
	uio_t *uiop)		/* Pointer to user I/O vector array. */
{
	int error;
	sam_ssize_t nbytes;
	sam_u_offset_t offset, reloff;
	caddr_t base;		/* Kernel address of mapped in block */
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	offset = uiop->uio_loffset;
	reloff = offset & ((sam_u_offset_t)PAGESIZE - 1);
	nbytes = uiop->uio_resid;
	if ((offset < ip->stage_off) ||
	    (offset + nbytes) > (ip->stage_off + ip->stage_len)) {
		cmn_err(CE_WARN,
"SAM-QFS: %s: stagerd error, ino=%d.%d off=%llx len=%lld soff=%lld slen=%lld",
		    ip->mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen,
		    (offset_t)offset, (offset_t)nbytes,
		    ip->stage_off, ip->stage_len);
		return (ECOMM);
	}

	TRACE(T_SAM_STNWRIO1, vp, (sam_tr_t)offset, (sam_tr_t)reloff, nbytes);
	base = (caddr_t)ip->st_buf + ((offset - ip->stage_off) & PAGEMASK);
	ip->size = offset + nbytes;

	/*
	 * Move the data from the stager daemon pages into the stage_n
	 * memory buffer.
	 */
	SAM_SET_LEASEFLG(ip->mp);
	error = uiomove((base + reloff), nbytes, UIO_WRITE, uiop);
	SAM_CLEAR_LEASEFLG(ip->mp);
	TRACE(T_SAM_STNWRIO2, vp, (sam_tr_t)base,
	    (sam_tr_t)uiop->uio_resid, error);
	if ((error == 0) && !ip->flags.b.staging) {
		error = ECANCELED;
	}
	if (error != 0) {
		if (ip->stage_err == 0) {
			ip->stage_err = (short)error;
		}
	}
	return (error);
}


/*
 * ----- sam_rmmap_block - Map logical block to rm physical block.
 *
 *  For the given inode and logical byte address: if iop is set, return
 *  the I/O descriptor.
 */

int				/* ERRNO if error, 0 if successful. */
sam_rmmap_block(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_WRITE or SAM_READ. */
	sam_ioblk_t *iop)	/* Ioblk array. */
{
	offset_t bn_off;
	sam_bn_t bn;
	int pboff;
	int pdu;
	sam_rm_t *rmp;

	TRACE(T_SAM_RMMAP, SAM_ITOV(ip), (sam_tr_t)ip->size, (sam_tr_t)offset,
	    count);
	if ((ip->di.rm.media & DT_CLASS_MASK) != DT_OPTICAL) {
		return (ENODEV);
	}
	pdu = SM_BLK(ip->mp, MM);
	rmp = &ip->di.rm;
	if (iop) {		/* ioblk initialization done here */
		iop->count = 0;
	}

	if (flag <= SAM_READ_PUT) {	/* If reading */
		if ((flag == SAM_READ) &&
		    (offset + (offset_t)count > ip->size)) {
			return (0);
		}
		pboff = (int)(offset & (pdu - 1));	/* Byte offset */
	} else {		/* If writing, verify on pdu boundary */
		if (rmp->ui.flags & RM_PARTIAL_PDU) {
			return (EINVAL);
		}
		pboff = (int)(offset & (pdu - 1));	/* Byte offset */
		if (pboff != 0) {
			return (ESPIPE);
		}
		if (count & (pdu - 1)) {	/* if partial sector written */
			rmp->ui.flags |= RM_PARTIAL_PDU;
		}
		if (count > ip->space) {
			return (ENOSPC);
		}
		ip->space -= count;
		ip->size += (offset_t)count;
		ip->cl_allocsz = ip->size;
		ip->di.rm.size = ip->size;
		rmp->ui.flags |= RM_FILE_WRITTEN;
	}
	bn_off = rmp->info.rm.position + (offset >> SAM_DEV_BSHIFT);
	bn = (sam_bn_t)bn_off;
	if (bn_off != (offset_t)bn) {
		return (EFBIG);
	}
	if (iop) {
		iop->imap.flags = 0;
		iop->ord = 0;
		iop->pboff = pboff;
		iop->count = count;
		iop->blkno = bn;
		iop->contig = ip->mp->mt.fi_readahead;
		if (iop->contig > (ip->di.rm.size - offset)) {
			if (flag == SAM_READ_PUT) {
				/* If called from getpage/putpage */
				iop->contig = ((ip->di.rm.size + pdu - 1) &
				    ~SM_BLK(ip->mp, MM)) - offset;
			} else {
				iop->contig = (ip->di.rm.size - offset);
			}
		}
		TRACE(T_SAM_RMMAP1, SAM_ITOV(ip), pboff, ip->rdev, bn)
	}
	return (0);
}
