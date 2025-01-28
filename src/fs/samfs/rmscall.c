/*
 * ----- rmscall.c - Process the sam removable media system calls.
 *
 *	Processes the system calls supported on the SAM File System.
 *	Called by the samioc system call module.
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

#pragma ident "$Revision: 1.103 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/copyops.h>
#include <sys/conf.h>
#include <sys/file.h>
/* The following lines need to stay in this order. */
#include "pub/stat.h"
/* These are duplicated in sys/stat.h */
#undef S_ISBLK
#undef S_ISCHR
#undef S_ISDIR
#undef S_ISFIFO
#undef S_ISGID
#undef S_ISREG
#undef S_ISUID
#undef S_ISLNK
#undef S_ISSOCK
#include <sys/mode.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/mount.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#undef st_atime
#undef st_mtime
#undef st_ctime
#include "pub/rminfo.h"

#include "inode.h"
#include "mount.h"
#include "sam/param.h"
#include "sam/types.h"
#include "sam/checksum.h"
#include "sam/devnm.h"
#include "sam/fioctl.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "sam/mount.h"
#include "sam/syscall.h"
#include "sam/samevent.h"

#include "global.h"
#include "inode.h"
#include "ino_ext.h"
#include "segment.h"
#include "arfind.h"
#include "extern.h"
#include "trace.h"
#include "debug.h"
#include "qfs_log.h"

static int sam_exch_multivolume_copy(sam_node_t *bip, int copy1, int copy2);
static int sam_old_request_file(sam_node_t *ip, vnode_t *vp,
	struct sam_readrminfo_arg args);


/*
 * ----- Process the archive, stage, release, csum, setfa calls.
 */

int		/* ERRNO if error, 0 if successful. */
sam_set_file_operations(sam_node_t *ip, int cmd, char *ops, cred_t *credp)
{
	struct ino_status old_status, status;
	union ino_flags old_flags, flags;
	struct sam_mount *mp = ip->mp;
	char *op = ops;
	int copy = 0;
	int archive_w = 0;
	int release_i = 0;
	int stage_i = 0;
	int stage_p = 0;
	int wait = 0;
	int assoc_stage;
	int change_partial = FALSE, partial = 0;
	int change_algo = 0, algo = 0;
	int error = 0;
	int copies_verified = 0;

	status = old_status = ip->di.status.b;
	flags = old_flags = ip->flags;

	switch (cmd) {

	case SC_archive: {
		uint_t *arp = (uint_t *)(void *)&ip->di.ar_flags[0];
		int copies = 0x1f;

		while (*op != '\0' && error == 0) {
			switch (*op++) {
			case 'c':
				copies = 0;
				while (*op >= '1' && *op <= '4') {
					copies |= 1 << (*op - '1');
					op++;
				}
				break;
			case 'C':
				/*
				 * Concurrent archiving enables archiving while
				 * the file is opened for write.
				 */
				status.concurrent_archive = 1;
				/* Clear opened for write */
				ip->flags.b.write_mode = 0;
				break;
			case 'I':
				/*
				 * Permit inconsistent archive copies. File was
				 * modified while the copy was being archived.
				 */
				status.inconsistent_archive = 1;
				/* Clear opened for write */
				ip->flags.b.write_mode = 0;
				break;
			case 'd':
				*arp &= ~((AR_arch_i << 24) |
				    (AR_arch_i << 16) |
				    (AR_arch_i << 8) | AR_arch_i);
				status.noarch = 0;
				status.concurrent_archive = 0;
				status.inconsistent_archive = 0;
				break;
			case 'i': {
				int mask;
				int n;

				for (n = 0, mask = 1; n < MAX_ARCHIVE;
				    n++, mask += mask) {
					if (copies & mask) {
						ip->di.ar_flags[n] |=
						    AR_arch_i;
					}
				}
				/*
				 * Notify arfind and event daemon of archive.
				 */
				sam_send_to_arfind(ip, AE_archive, 0);
				}
				break;
			case 'n':
				/* Only superuser can set "never archive" */
				if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
					error = EACCES;
				} else {
					status.noarch = 1;
				}
				break;
			case 'w':
				archive_w = copies;
				break;
			case 'W':
				archive_w = 0x20;
				break;
			default:
				error = EINVAL;
				break;
			}
		}
		}
		break;

	case SC_cancelstage:
		error = sam_cancel_stage(ip, ECANCELED, credp);
		break;

	case SC_release:
		while (*op != '\0' && error == 0) {
			switch (*op++) {
			case 'd':
				release_i = 0;
				status.bof_online = 0;
				status.release = 0;
				status.nodrop = 0;
				if (ip->di.status.b.offline &&
				    ip->di.status.b.direct) {
					error = sam_drop_ino(ip, credp);
				}
				break;
			case 'a':
				status.release = 1;
				break;
			case 'i':
				/*
				 * Cannot release file if "release never" is set
				 */
				if (status.nodrop == 0) {
					release_i = 1;
				}
				break;
			case 'n':
				/*
				 * Only superuser can disable release disk space
				 */
				if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
					error = EACCES;
				} else {
					status.nodrop = 1;
				}
				break;
			case 'p':
				status.bof_online = 1;
				change_partial = TRUE;
				break;
			case 's':
				partial = 0;
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					partial = (10 * partial) + *op - '0';
					op++;
				}
				if ((SAM_MINPARTIAL <= partial) &&
				    (partial <= mp->mt.fi_maxpartial)) {
					change_partial = TRUE;
					status.bof_online = 1;
				} else {
					error = EINVAL;
				}
				break;

			default:
				error = EINVAL;
				break;
			}
		}
		if (change_partial && partial == 0) {
			partial = mp->mt.fi_partial;
		}
		if (S_ISSEGI(&ip->di) ||
		    (S_ISSEGS(&ip->di) && (ip->di.rm.info.dk.seg.ord != 0))) {
			change_partial = FALSE;
			status.bof_online = 0;
		}
		if (change_partial &&
		    (ip->flags.b.staging || ip->flags.b.hold_blocks)) {
			error = EBUSY;
		}
		if (status.nodrop && status.release) {
			error = ENOSYS;
		}
		break;

	case SC_stage:
		assoc_stage = 1;
		while (*op != '\0' && error == 0) {
			char ch = *op++;

			switch (ch) {
			case 'd':
				if (secpolicy_vnode_owner(credp, ip->di.uid)) {
					error = EACCES;
					break;
				}
				copy = 0;
				stage_i = 0;
				stage_p = 0;
				wait = 0;
				status.stage_all = 0;
				if (ip->di.status.b.offline &&
				    ip->di.status.b.direct) {
					error = sam_drop_ino(ip, credp);
				}
				status.direct = 0;
				break;
			case 'a':
				if (secpolicy_vnode_owner(credp, ip->di.uid)) {
					error = EACCES;
					break;
				}
				status.stage_all = 1;
				break;
			case 'i':
				/*
				 * Don't stage file if "stage never" is set or
				 * dma file
				 */
				if (status.direct == 0) {
					stage_i = 1;
				}
				break;
			case 'n':
				if (secpolicy_vnode_owner(credp, ip->di.uid)) {
					error = EACCES;
					break;
				}
				status.direct = 1;
				break;
			case 'p':
				stage_p = 1;
				break;
			case 's':
				assoc_stage = 0;
				break;
			case 'w':
				wait = 1;
				break;
			case '1':
			case '2':
			case '3':
			case '4':
				copy = ch - '0';
				break;
			default:
				error = EINVAL;
				break;
			}
		}
		if (stage_p && stage_i) {
			error = EINVAL;
		}
		break;

	case SC_ssum:
		while (*op != '\0' && error == 0) {
			switch (*op++) {
			case 'd':
				status.cs_use = 0;
				ip->di.rm.ui.flags &= ~RM_DATA_VERIFY;
				if (!ip->di.status.b.worm_rdonly) {
					/* If readonly */
					status.cs_gen = 0;
				}
				break;
			case 'e':
				/* Only superuser can set data verification */
				if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
					error = EACCES;
				} else {
					ip->di.rm.ui.flags |= RM_DATA_VERIFY;
				}
				break;
			case 'g':
				status.cs_gen = 1;
				break;
			case 'G':
				break;
			case 'u':
				if (ip->di.arch_status &&
				    !ip->di.status.b.cs_val &&
				    !S_ISDIR(ip->di.mode)) {
					error = EINVAL;
				} else {
					status.cs_use = 1;
				}
				break;
			default:
				op--;
				if ((*op >= '0') && (*op <= '9')) {
					for (algo = 0; *op >= '0' &&
					    *op <= '9'; op++) {
						algo = 10 * algo + *op - '0';
					}
					if ((algo >= CS_FUNCS) &&
					    !(algo & CS_USER_BIT)) {
						error = EINVAL;
					}
				} else {
					error = EINVAL;
				}
				/*
				 * if an archive copy already exists, and new
				 * algorithm does not match old, EINVAL.
				 */
				if ((ip->di.arch_status) &&
				    (ip->di.cs_algo != algo)) {
					error = EINVAL;
				} else {
					change_algo++;
				}
				break;
			}
		}

		/*
		 * Can't use checksum if it's not generated and file not yet
		 * archived.
		 */
		if (!ip->di.arch_status && (status.cs_use && !status.cs_gen)) {
			error = EINVAL;
		}
		break;

	case SC_setfa:
		if (!S_ISDIR(ip->di.mode) && !S_ISREG(ip->di.mode)) {
			error = EINVAL;
			break;
		}
		if (S_ISSEGS(&ip->di)) {
			break;
		}
		while (*op != '\0' && error == 0) {
			offset_t dfilesize = 0;
			offset_t filesize = 0;
			offset_t allocahead = 0;
			int stripe = 0;
			int stripe_group = 0;
			int stripe_shift = 0;
			int osd_group = 0;
			uchar_t new_osd_group, new_unit;
			sam_di_osd_t *oip;
			int ord;

			switch (*op++) {
			case 'A':
				if (SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					allocahead = (10 * allocahead) +
					    *op - '0';
					op++;
				}
				allocahead /= 1024;
				if (allocahead < 0 || allocahead > UINT_MAX) {
					error = EINVAL;
				}
				if (!S_ISREG(ip->di.mode)) {
					allocahead = 0;
				}
				break;
			case 'B':
				status.directio = 0;
				flags.b.directio = 0;
				break;
			case 'd':
				ip->di.stripe = mp->mt.fi_stripe[DD];
				ip->di.stride = 0;
				if (ip->di.blocks == 0 ||
				    !(ip->mp->mt.fi_config1 &
				    MC_MISMATCHED_GROUPS)) {
					ip->di.stripe_group = 0;
					status.stripe_group = 0;
				}
				status.stripe_width = 0;
				if (ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) {
					vnode_t *aio_vp = SAM_ITOV(ip);

					/*
					 * Cannot remove q option if file is
					 * opened or is shared and someone has a
					 * lease.
					 */
					if (ip->no_opens ||
					    (SAM_IS_SHARED_FS(mp) &&
					    ip->sr_leases)) {
						error = EBUSY;
						break;
					}
					ip->di.rm.ui.flags &= ~RM_CHAR_DEV_FILE;
					if (!S_ISDIR(ip->di.mode) &&
					    (aio_vp->v_type == VCHR)) {
						mutex_enter(&aio_vp->v_lock);
						sam_detach_aiofile(aio_vp);
						aio_vp->v_type = VREG;
						aio_vp->v_rdev = 0;
						mutex_exit(&aio_vp->v_lock);
					}
				}
				status.directio =
				    (ip->mp->mt.fi_config & MT_DIRECTIO) ?
				    1 : 0;
				flags.b.directio =
				    (ip->mp->mt.fi_config & MT_DIRECTIO) ?
				    1 : 0;
				ip->flags.b.qwrite =
				    (ip->mp->mt.fi_config & MT_QWRITE) ? 1 : 0;
				break;
			case 'D':
				status.directio = 1;
				flags.b.directio = 1;
				break;
			case 'g':
				if (SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					stripe_group = (10 * stripe_group) +
					    *op - '0';
					op++;
				}
				if (stripe_group > dev_nmsg_size) {
					error = EINVAL;
				} else {
					uchar_t new_stripe_group, new_unit;
					int ord;

					stripe_group |= DT_STRIPE_GROUP;
					for (ord = 0; ord < mp->mt.fs_count;
					    ord++) {
						if (stripe_group ==
						    mp->mi.m_fs[
						    ord].part.pt_type) {
							new_unit = (uchar_t)ord;
		new_stripe_group = (stripe_group & ~DT_STRIPE_GROUP_MASK);
							break;
						}
					}
					if (ord >= mp->mt.fs_count) {
						error = EINVAL;
					} else {
						/*
						 * Cannot set striped group if
						 * blocks are already allocated
						 * and the striped groups are
						 * mismatched.  Do not return an
						 * error if setting the same
						 * stripe group.
						 */
						if (S_ISREG(ip->di.mode) &&
						    ip->di.blocks &&
						    (ip->mp->mt.fi_config1 &
						    MC_MISMATCHED_GROUPS)) {

	/* N.B. Bad indentation here to meet cstyle requirements */
	if (!ip->di.status.b.stripe_group ||
	    ip->di.stripe_group != new_stripe_group) {
		error = EINVAL;
	}


						} else {
							/*
							 * Don't reset unit on
							 * metadata
							 * (directories)
							 */
							if (ip->di.status.b.meta
							    == 0) {
								ip->di.unit =
								    new_unit;
							}
							ip->di.stripe_group =
							    new_stripe_group;
							status.stripe_group = 1;
						}
					}
				}
				break;
			case 'l':
				if (SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					dfilesize = (10 * dfilesize) +
					    *op - '0';
					op++;
				}
				if (!S_ISREG(ip->di.mode)) {
					dfilesize = 0;
				}
				break;
			case 'L':
				if (SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					filesize = (10 * filesize) + *op - '0';
					op++;
				}
				if (!S_ISREG(ip->di.mode)) {
					filesize = 0;
				}
				break;
			case 'q':
				if (SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				if (ip->di.id.ino >= mp->mi.m_min_usr_inum ||
				    ip->di.id.ino == SAM_ROOT_INO) {
					if (samgt.samaio_vp) {
						ip->di.rm.ui.flags |=
						    RM_CHAR_DEV_FILE;
						status.directio = 1;
						flags.b.directio = 1;
						ip->flags.b.qwrite = 1;
					} else {
						error = ENXIO;
					}
				} else {
					error = EINVAL;
				}
				break;
			case 's':
				if (SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					stripe = (10 * stripe) + *op - '0';
					op++;
				}
				if (stripe > 255) {
					error = EINVAL;
				} else if ((mp->mt.fi_config1 &
				    MC_MISMATCHED_GROUPS) &&
				    (stripe > 0)) {
					error = EINVAL;
				} else {
					ip->di.stripe = (char)stripe;
					ip->di.stride = 0;
					status.stripe_width = 1;
				}
				break;
			case 'h':
				if (!SAM_IS_OBJECT_FS(mp) ||
				    !S_ISDIR(ip->di.mode)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					stripe = (10 * stripe) + *op - '0';
					op++;
				}
				if (stripe > SAM_MAX_OSD_STRIPE_WIDTH) {
					error = EINVAL;
					break;
				}
				ip->di.stride = 0;
				ip->di.rm.info.obj.stripe_width = stripe;
				ip->di.stripe = 0;
				status.stripe_width = 1;
				break;
			case 'v': {
				offset_t stripe_depth = 0;
				offset_t value;

				if (!SAM_IS_OBJECT_FS(mp) ||
				    !S_ISDIR(ip->di.mode)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					stripe_depth = (10 * stripe_depth) +
					    *op - '0';
					op++;
				}
				value = SAM_DEV_BSIZE;
				stripe_shift = SAM_DEV_BSHIFT;
				for (;;) {
					if (value >= stripe_depth) {
						stripe_depth = value;
						break;
					}
					stripe_shift++;
					value <<= 1;
				}
				stripe_depth /= 1024;
				if (stripe_depth < 0 ||
				    stripe_depth > UINT_MAX) {
					error = EINVAL;
				}
				}
				break;
			case 'o':
				if (!SAM_IS_OBJECT_FS(mp)) {
					error = EINVAL;
					break;
				}
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					osd_group = (10 * osd_group) +
					    *op - '0';
					op++;
				}
				if (osd_group < 0 ||
				    osd_group  >= dev_nmog_size) {
					error = EINVAL;
					break;
				}
				new_osd_group = (uchar_t)osd_group;
				osd_group |= DT_OBJECT_DISK;
				for (ord = 0; ord < mp->mt.fs_count; ord++) {
					if (osd_group == mp->mi.m_fs[
					    ord].part.pt_type) {
						new_unit = (uchar_t)ord;
						break;
					}
				}
				if (ord >= mp->mt.fs_count) {
					error = EINVAL;
					break;
				}
				/*
				 * Cannot set osd group if object id's
				 * are already created. Do not return an
				 * error if setting the same osd group.
				 */
				oip = (sam_di_osd_t *)
				    (void *)&ip->di.extent[0];
				if (S_ISREG(ip->di.mode) && oip->obj_id[0]) {
					if (old_status.stripe_group &&
					    ip->di.stripe_group !=
					    new_osd_group) {
						error = EINVAL;
						break;
					}
				}
				/*
				 * Don't reset unit on metadata (directories)
				 */
				if (ip->di.status.b.meta == 0) {
					ip->di.unit = new_unit;
				}
				ip->di.stripe_group = new_osd_group;
				status.stripe_group = 1;
				break;
			default:
				error = EINVAL;
				break;
			}
			if (dfilesize && filesize) {
				error = EINVAL;
			}
			if (dfilesize && ip->di.rm.info.dk.seg_size) {
				error = EINVAL;
			}
			if (dfilesize) {
				filesize = dfilesize;
			}
			if (error == 0 && filesize) {
				int prealloc = 0;

				if (ip->di.rm.size < filesize) {
					/*
					 * no blocks allocated and direct map
					 * requested.
					 */
					if (ip->di.blocks == 0 && dfilesize) {
						if (ip->size == 0) {
							status.direct_map = 1;
							status.on_large = 1;
						ip->di.status.b.direct_map = 1;
						ip->di.status.b.on_large = 1;
							prealloc = 1;
						}
					} else {
					if (ip->di.status.b.direct_map) {
							error = EINVAL;
					}
					}
					if (error == 0) {
						error = sam_map_block(ip,
						    (offset_t)0, filesize,
						    SAM_ALLOC_BLOCK, NULL,
						    credp);
						if (prealloc) {
							if (error) {
								/*
								 * clear flags,
								 * none
								 * allocated
								 */


	/* N.B. Bad indentation here to meet cstyle requirements */
	status.direct_map = 0;
	ip->di.status.b.direct_map = 0;
	if (old_status.on_large == 0) {
		status.on_large = 0;
		ip->di.status.b.on_large = 0;
	}



							}
						}
					}
				} else {
					error = EINVAL;
				}
			}
			if (error == 0 && allocahead) {
				ip->di.rm.info.dk.allocahead = (int)allocahead;
			}
			if (error == 0 && stripe_shift) {
				ip->di.rm.info.obj.stripe_shift = stripe_shift;
			}
		}
		break;

	case SC_segment:
#ifdef METADATA_SERVER
		if (!samgt.license.license.lic_u.b.segment) {
			error = ENOTSUP;
			break;
		}
#endif	/* METADATA_SERVER */
		if (SAM_IS_SHARED_FS(mp) || SAM_IS_OBJECT_FS(mp)) {
			error = ENOTSUP;
			break;
		}
		while (*op != '\0' && error == 0) {
			offset_t segment_size = 0;
			int stripe = 0;

			switch (*op++) {
			case 'd':

				if (ip->di.status.b.segment &&
				    !(S_ISSEGI(&ip->di) || S_ISSEGS(&ip->di))) {
					status.segment = 0;
				} else {
					error = EINVAL;
				}
				ip->di.stage_ahead = 0;
				break;
			case 'l':	/* Set segment size in megabytes */
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					segment_size = (10 * segment_size) +
					    *op - '0';
					op++;
				}
				if (S_ISSEGI(&ip->di) || S_ISSEGS(&ip->di)) {
					error = EACCES;
				} else if (!S_ISDIR(ip->di.mode) &&
				    !S_ISREG(ip->di.mode)) {
					error = EINVAL;
				} else if ((ip->di.rm.size > segment_size) &&
				    S_ISREG(ip->di.mode)) {
					error = EINVAL;
				}
				if (S_ISREG(ip->di.mode) &&
				    ip->di.status.b.direct_map) {
					error = EINVAL;
				} else {
					segment_size = segment_size /
					    SAM_MIN_SEGMENT_SIZE;
					if (segment_size <= 0) {
						error = EINVAL;
					} else {
						ip->di.rm.info.dk.seg_size =
						    (uint_t)segment_size;
						status.segment = 1;
					}
				}
				break;
			case 's':		/* Set stripe for segment */
				while (*op != '\0' && '0' <= *op &&
				    *op <= '9') {
					stripe = (10 * stripe) + *op - '0';
					op++;
				}
				if (stripe > 255) {
					error = EINVAL;
				} else {
					ip->di.stage_ahead = (uchar_t)stripe;
				}
				break;
			default:
				error = EINVAL;
				break;
			}
		}
		break;

	default:	/* Can't happen */
		error = ENOTTY;
		break;
	}

	if (!error && cmd != SC_cancelstage) {
#ifdef METADATA_SERVER
		/*
		 * Stage_n (direct) is mutually exclusive with partial except
		 * when enabled by db_features license key.
		 */
		if (!samgt.license.license.lic_u.b.db_features) {
			if (status.direct && status.bof_online) {
				error = ENOTSUP;
			}
		}
#endif	/* METADATA_SERVER */
		/*
		 * Stage_n (direct) is mutually exclusive with stageall and dma.
		 */
		if (status.direct && status.stage_all) {
			error = EINVAL;
		}
		/*
		 * Checksum use requires archiving and staging, and
		 * cannot be done with partial release.
		 */
		if ((status.cs_use) &&
		    (status.direct || status.noarch || status.bof_online)) {
			error = EINVAL;
		}

		if (!error) {
			if (old_flags.b.directio != flags.b.directio) {
				sam_set_directio(ip, flags.b.directio);
			}

			/* Set permanent ino status bits */
			ip->di.status.b = status;

			/*
			 * If clearing bof_online (partial selected).
			 */
			if (old_status.bof_online && (status.bof_online == 0)) {
				if (ip->di.status.b.offline) {
					if (S_ISREG(ip->di.mode)) {
						/*
						 * If regular offline file
						 * clearing partial, release
						 * partial blocks.
						 */
						if (ip->flags.b.staging ||
						    ip->flags.b.hold_blocks) {
							error = EBUSY;
							ip->di.status.b =
							    old_status;
						} else {
							ip->di.status.b.pextents
							    = 0;
							ip->di.psize.partial =
							    0;
							error = sam_drop_ino(
							    ip, credp);
						}
					}
				} else {
					/*
					 * If clearing bof_online and is online,
					 * reset pextents and partial.
					 */
					ip->di.status.b.pextents = 0;
					ip->di.psize.partial = 0;
				}
			}

			if (change_partial) {
				offset_t old_partial = sam_partial_size(ip);

				ip->di.psize.partial = partial;
				if (!ip->di.status.b.offline) {
					/* Set pextents if online */
					ip->di.status.b.pextents = 1;
				} else {
					if (sam_partial_size(ip) >
					    old_partial) {
						ip->di.status.b.pextents = 0;
						ip->size = 0;
					}
					error = sam_drop_ino(ip, credp);
				}
			}

			/*
			 * Can't change the stage never flag if staging.
			 */
			if (!ip->flags.b.staging && !ip->flags.b.hold_blocks) {
				ip->flags.b.stage_n = status.direct;
				ip->flags.b.stage_all = status.stage_all;
			}
			if (change_algo) {
				ip->di.cs_algo = (uchar_t)algo;
			}
			if (S_ISSEGS(&ip->di)) {
				ip->segment_ip->di.attribute_time =
				    ip->di.attribute_time;
				ip->segment_ip->flags.b.changed = 1;
			}
			ip->flags.b.changed = 1;
		}
#if !defined(DIRLINK)
		if (!error && S_ISREG(ip->di.mode))
#else
		if (!error)
#endif
		{
			/*
			 * Only one of the flags (stage_p, stage_i, release_i or
			 * archive_i ) will be set.  Stage_file checks for
			 * already staging and !wait.
			 */
			if (stage_p && ip->di.status.b.offline &&
			    status.bof_online) {
				ip->flags.b.stage_p = 1;
				error = sam_stage_file(ip, wait, copy, credp);
			}
			if (stage_i && ip->di.status.b.offline) {
				if (assoc_stage == 0) {
					ip->flags.b.stage_all = 0;
				}
				if (wait == 0 &&
				    ip->di.status.b.cs_use &&
				    ip->di.status.b.cs_val) {
					ip->flags.b.nowaitspc = 1;
				}
				error = sam_stage_file(ip, wait, copy, credp);
				ip->flags.b.stage_all =
				    ip->di.status.b.stage_all;
			}
			/*
			 * Don't release a file that's staging or in use by
			 * SANergy
			 */
			if (release_i) {
#if !defined(DIRLINK)
				if (!S_ISSEGI(&ip->di)) {
#else
				{
#endif
					if (ip->flags.b.staging ||
					    ip->flags.b.hold_blocks) {
						error = EBUSY;
					} else if (ip->di.rm.ui.flags &
					    RM_DATA_VERIFY) {
						int copy, mask;
						int copies = 0;

						for (copy = 0, mask = 1;
						    copy < MAX_ARCHIVE;
						    copy++, mask += mask) {
							if (ip->di.arch_status &
							    mask) {
								copies++;
							}


	/* N.B. Bad indentation here to meet cstyle requirements */
	if ((ip->di.ar_flags[copy] & AR_verified) &&
	    !(ip->di.ar_flags[copy] & AR_damaged)) {
		copies_verified++;
	}


						}
						if (copies_verified != copies ||
						    copies == 1 ||
						    !ip->di.status.b.archdone) {
							error = EACCES;
						}
					}

					if (!error) {
						RW_UNLOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						RW_LOCK_OS(&ip->data_rwl,
						    RW_WRITER);
						RW_LOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						error = sam_drop_ino(ip, credp);
						RW_UNLOCK_OS(&ip->data_rwl,
						    RW_WRITER);
					}
				}
			}
			if (archive_w) {
				/*
				 * Wait until at least 1 copy has archived. If
				 * segment, wait until at least 1 copy has
				 * archived for each segment.  Don't wait, if
				 * the data segment inode with callback call.
				 */
				if (S_ISSEGI(&ip->di)) {
					sam_callback_segment_t callback;

					callback.p.wait_rm.archive_w =
					    archive_w;
					error = sam_callback_segment(ip,
					    CALLBACK_wait_rm,
					    &callback, TRUE);
				} else if (!S_ISSEGS(&ip->di)) {
					error = sam_wait_archive(ip, archive_w);
				}
			}
		}
	}
	return (error);
}


/*
 * Process unarchive, exarchive, damage, undamage, rearch, or unrearch
 * an archive copy.
 */

/* ARGSUSED1 */
int		/* ERRNO if error, 0 if successful. */
sam_proc_archive_copy(vnode_t *vp, int cmd, void *args, cred_t *credp)
{
	sam_node_t *ip;
	struct sam_archive_copy_arg *ap = (struct sam_archive_copy_arg *)args;
	struct sam_perm_inode *permip;
	buf_t *bp;
	int error = EINVAL;

	/*
	 * Read permanent inode.  Clear archive copy info.
	 */
	ip = SAM_VTOI(vp);
	if ((ip->di.id.ino != SAM_INO_INO) &&
	    ((error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip))
	    == 0)) {
		int		arfindEvent;
		enum sam_event_num event;
		int copy, mask;
		sam_id_t id[MAX_ARCHIVE];

		/*
		 * Check each archive copy for match for specified arguments.
		 * copy number, media, and VSN.
		 * Unlock buffer and return error if nonexistent copy.
		 */
		arfindEvent = AE_none;
		event = ev_none;
		for (copy = 0, mask = 1; copy < MAX_ARCHIVE;
		    copy++, mask += mask) {

			if ((ap->copies & mask) == 0) {
				continue;
			}
			if (ap->media != 0 && ap->media != ip->di.media[copy]) {
				continue;
			}
			if (*ap->vsn != '\0' &&
			    strncmp(ap->vsn, permip->ar.image[copy].vsn,
			    sizeof (ap->vsn)) != 0) {
				continue;
			}

			/*
			 * Error if no archive copy and no stale offline archive
			 * copy.
			 */

			if (((ip->di.arch_status & mask) == 0) &&
			    (((ip->di.ar_flags[copy] & AR_stale) == 0) ||
			    !ip->di.status.b.offline)) {
				brelse(bp);
				return (EC_NO_ARCHIVE_COPY);
			}
		}

		/*
		 * if -o, make it online. buffer is locked, so release and
		 * reread
		 */

		if (ip->di.status.b.offline && (ap->flags & SU_online)) {
			brelse(bp);
			if ((error = sam_stage_file(ip, 1, 0, credp)) == 0) {
				error = sam_read_ino(ip->mp, ip->di.id.ino,
				    &bp, &permip);
			}
			if (error) {
				return (error);
			}
		}
		/*
		 * Check each archive copy for match for specified arguments.
		 * copy number, media, and VSN.
		 */
		for (copy = 0, mask = 1; copy < MAX_ARCHIVE;
		    copy++, mask += mask) {
			id[copy].ino = 0;

			if ((ap->copies & mask) == 0) {
				continue;
			}
			if (ap->media != 0 && ap->media != ip->di.media[copy]) {
				continue;
			}
			if (*ap->vsn != '\0' &&
			    strncmp(ap->vsn, permip->ar.image[copy].vsn,
			    sizeof (ap->vsn)) != 0) {
				continue;
			}

			switch (ap->operation) {

			case OP_unarchive:
				/*
				 * if file offline, make sure the last
				 * good copy (undamaged) is not unarchived
				 */
				if (ip->di.status.b.offline) {
					int i, good_mask = 0, test_mask;

					/*  Determine good/undamaged copies */
					for (i = 0, test_mask = 1;
					    i < MAX_ARCHIVE;
					    i++, test_mask += test_mask) {
						if ((ip->di.arch_status &
						    test_mask) == 0) {
							continue;
						}
						if (!(ip->di.ar_flags[i] &
						    (AR_damaged|
						    AR_unarchived))) {
							good_mask |= test_mask;
						}
					}
					if ((good_mask & ~mask) == 0 &&
					    !(ap->flags & SU_force)) {
						brelse(bp);
						return (EC_WOULD_UNARCH_LAST);
					}
				}
				arfindEvent = AE_unarchive;
				if (ip->di.version >= SAM_INODE_VERS_2) {
					if (ip->di.ext_attrs & ext_mva) {
						brelse(bp);
						if ((error =
						    sam_get_multivolume_copy_id(
						    ip, copy,
						    &id[copy])) == 0) {
							error = sam_read_ino(
							    ip->mp,
							    ip->di.id.ino, &bp,
							    &permip);
						}
						if (error) {
							return (error);
						}
					}
				} else if (ip->di.version == SAM_INODE_VERS_1) {
					sam_perm_inode_v1_t *permip_v1 =
					    (sam_perm_inode_v1_t *)permip;
					/* Prev version */
					id[copy] = permip_v1->aid[copy];
					permip_v1->aid[copy].ino = 0;
					permip_v1->aid[copy].gen = 0;
				}
				bzero((char *)&permip->ar.image[copy],
				    sizeof (sam_archive_info_t));
				permip->di.arch_status = ip->di.arch_status &=
				    ~mask;
				permip->di.ar_flags[copy] =
				    ip->di.ar_flags[copy] = AR_unarchived;
				permip->di.media[copy] = ip->di.media[copy] = 0;
				if ((ip->di.arch_status == 0) &&
				    (ip->di.status.b.offline) &&
				    (ip->di.rm.size != 0)) {
					ip->di.status.b.damaged = 1;
				}
				break;

			case OP_exarchive: {
				int dcopy;
				sam_id_t aid;
				sam_archive_info_t ar; /* Archive copy record */
				media_t media;		/* Archive media */
				uchar_t ar_flags;	/* Archive flags */
				uchar_t arch_status_m; /* Src archive status */
				uchar_t arch_status_n;	/* Dest arch status */

				dcopy = ap->dcopy;
				arch_status_m = ip->di.arch_status &
				    (1 << copy);
				arch_status_m >>= copy;
				arch_status_n = ip->di.arch_status &
				    (1 << dcopy);
				arch_status_n >>= dcopy;
				/*
				 * If no destination copy, we probably need to
				 * archive this copy again after we swap.  Clear
				 * archdone.
				 */
				if (arch_status_n == 0) {
					arfindEvent = AE_unarchive;
				}
				event = ev_archange;
				permip->di.arch_status = ip->di.arch_status =
				    (ip->di.arch_status & ~(1 << copy) & ~(1 << dcopy)) |
				    (arch_status_m << dcopy) |
				    (arch_status_n << copy);
				media = ip->di.media[dcopy];
				permip->di.media[dcopy] =
				    ip->di.media[dcopy] = ip->di.media[copy];
				permip->di.media[copy] = ip->di.media[copy] =
				    media;
				ar_flags = ip->di.ar_flags[dcopy];
				permip->di.ar_flags[dcopy] =
				    ip->di.ar_flags[dcopy] =
				    ip->di.ar_flags[copy];
				permip->di.ar_flags[copy] =
				    ip->di.ar_flags[copy] = ar_flags;
				ar = permip->ar.image[dcopy];
				permip->ar.image[dcopy] =
				    permip->ar.image[copy];
				permip->ar.image[copy] = ar;
				if (ip->di.version >= SAM_INODE_VERS_2) {
					if (ip->di.ext_attrs & ext_mva) {
						brelse(bp);
						if ((error =
						    sam_exch_multivolume_copy(
						    ip, copy,
						    dcopy)) == 0) {
							error = sam_read_ino(
							    ip->mp,
							    ip->di.id.ino, &bp,
							    &permip);
						}
						if (error) {
							return (error);
						}
					}
				} else if (ip->di.version == SAM_INODE_VERS_1) {
					sam_perm_inode_v1_t *permip_v1 =
					    (sam_perm_inode_v1_t *)permip;
					/* Prev version */
					aid = permip_v1->aid[dcopy];
					permip_v1->aid[dcopy] =
					    permip_v1->aid[copy];
					permip_v1->aid[copy] = aid;
				}
				}
				break;

			case OP_damage: {
				int i, mask;

				ip->di.ar_flags[copy] |= AR_damaged;
				if (ap->flags & SU_archive) {
					ip->di.ar_flags[copy] |= AR_rearch;
					arfindEvent = AE_rearchive;
				}
				event = ev_archange;

				/*
				 * If file is offline and all copies are
				 * damaged, mark the inode as damaged also.
				 */
				if (ip->di.status.b.offline) {
					for (i = 0, mask = 1;
					    i < MAX_ARCHIVE;
					    i++, mask += mask) {
						if ((ip->di.arch_status &
						    mask) &&
						    ((ip->di.ar_flags[i] &
						    AR_damaged) == 0)) {
							break;
						}
					}
					if (i == MAX_ARCHIVE) {
						ip->di.status.b.damaged = 1;
					}
				}
				}
				break;

			case OP_undamage:
				ip->di.ar_flags[copy] &= ~(AR_damaged|AR_stale);
				ip->di.arch_status |= mask;
				ip->di.status.b.damaged = 0;
				event = ev_archange;
				break;

			case OP_rearch:
				ip->di.ar_flags[copy] |= AR_rearch;
				ip->di.ar_flags[copy] &= ~AR_verified;
				arfindEvent = AE_rearchive;
				break;

			case OP_unrearch:
				ip->di.ar_flags[copy] &= ~AR_rearch;
				ip->di.arch_status |= mask;
				break;

			case OP_verified:
				ip->di.ar_flags[copy] |= AR_verified;
				break;

			default:
				error = EINVAL;
				break;
			}
			if (error == 0) {
				ip->flags.b.changed = 1;
				if (arfindEvent != AE_none) {
					ip->di.status.b.archdone = 0;
					sam_send_to_arfind(ip, arfindEvent,
					    copy + 1);
				}
				if (event != ev_none) {
					int cn = copy + 1;
					/*
					 * Notify event daemon of unarchive,
					 * rearchive.
					 */
					if (ap->operation == OP_exarchive) {
						cn = 0;
					}
					if (ip->mp->ms.m_fsev_buf) {
						sam_send_event(ip->mp, &ip->di,
						    event, cn, 0,
						    ip->di.modify_time.tv_sec);
					}
				}
			}
		}
		if (TRANS_ISTRANS(ip->mp)) {
			TRANS_WRITE_DISK_INODE(ip->mp, bp, permip, ip->di.id);
		} else {
			bdwrite(bp);
		}
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (id[copy].ino) {
				if (ip->di.version >= SAM_INODE_VERS_2) {
					/* Curr version */
					sam_free_inode_ext(ip, S_IFMVA, copy,
					    &ip->di.ext_id);
				} else if (ip->di.version == SAM_INODE_VERS_1) {
					/* Version 1 */
					sam_free_inode_ext(ip, S_IFMVA,
					    copy, &id[copy]);
				}
			}
		}
	}
	return (error);
}


/*
 * ----- sam_request_file - Process the sam request system call.
 *	The removable media information is copied from the user to the removable
 *	media file information inode extension and the size of the file is set
 *	to zero.  File must NOT be opened, or else EBUSY is returned.
 */

int				/* ERRNO if error, 0 if successful. */
sam_request_file(void *arg)
{
	int error = 0;
	struct sam_readrminfo_arg args;
	struct sam_rminfo rb;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	void *path;
	void *buf;
	buf_t *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;
	sam_resource_file_t *rfp;
	int media = 0, n;
	int end_od = FALSE, end_tp = FALSE;
	offset_t size;
	int vsns;
	int ino_vsns;
	struct sam_rminfo *urb;
	struct sam_section *vsnp;
	sam_mount_t *mp;
	int issync;
	int trans_size;
	int terr = 0;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	if (args.bufsize < sizeof (struct sam_rminfo)) {
		return (EINVAL);
	}
	path = (void *)(long)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname((char *)path, UIO_USERSPACE, NO_FOLLOW,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-FS */
		VN_RELE(vp);
		return (ENOTTY);
	}
	ip = SAM_VTOI(rvp);

	/*
	 * If we're coming down this path as a shared client
	 * of this mounted FS then kick the request back because
	 * we don't support this operation.
	 */

	if (SAM_IS_CLIENT_OR_READER(ip->mp)) {
		return (ENOTSUP);
	}

	if ((error = sam_open_operation(ip))) {
		return (error);
	}

	trans_size = (int)TOP_SETATTR_SIZE(ip);
	TRANS_BEGIN_CSYNC(ip->mp, issync, TOP_SETATTR, trans_size);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (ip->di.version == SAM_INODE_VERS_1) {
		error = sam_old_request_file(ip, rvp, args);
		goto out;
	}

	/*
	 * File must be a removable media file or an empty regular file and
	 * not opened or busy. Filesystem must not be read-only.
	 * File must have write permission.
	 */
	if ((!S_ISREQ(ip->di.mode)) && !(S_ISREG(ip->di.mode) &&
	    ((ip->di.blocks == 0) && !ip->di.status.b.offline))) {
		error = EEXIST;
	} else if (ip->flags.b.rm_opened) {
		error = EBUSY;
	} else if (ip->di.status.b.damaged) {
		error = ENODATA;
	} else if (ip->mp->mt.fi_mflag & MS_RDONLY) {
		/* If read-only filesystem */
		error = EROFS;
	} else if ((error = sam_access_ino(ip, S_IWRITE, TRUE, CRED())) == 0) {
		buf = (void *)(long)args.buf.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			buf = (void *)args.buf.p64;
		}
		bzero((caddr_t)&rb, sizeof (rb));
		if (copyin((caddr_t)buf, (caddr_t)&rb,
			sizeof (struct sam_rminfo))) {
			error = EFAULT;
#ifdef METADATA_SERVER
		} else if ((rb.flags & RI_foreign) &&
			!samgt.license.license.lic_u.b.foreign_tape) {
			error = ENOTSUP;
#endif	/* METADATA_SERVER */
		} else if ((rb.n_vsns == 0) || (rb.n_vsns > MAX_VOLUMES)) {
			error = EINVAL;
		} else if (rb.position &&
		    secpolicy_fs_config(CRED(), ip->mp->mi.m_vfsp)) {
			error = EPERM;
		}
	}
	if (error) {
		goto out;
	}

	if (ip->di.psize.rmfile) {
		if (ip->di.ext_attrs & ext_rfa) {
			if (ip->di.ext_id.ino == 0) {
				sam_req_ifsck(ip->mp, -1,
				    "sam_request_file: eid.ino = 0",
				    &ip->di.id);
				error = ENOCSI;
			} else {
				/*
				 * Compute number of vsns we have based on
				 * current size.
				 */
				vsns = (ip->di.psize.rmfile -
				    sizeof (sam_resource_file_t)) /
				    sizeof (struct sam_section) + 1;
				/*
				 * Compute number of vsns we can hold if all
				 * extensions are filled.
				 */
				vsns = 1 + roundup(vsns - 1, MAX_VSNS_IN_INO);
				/*
				 * If we don't have enough extension space to
				 * hold new info, free the extensions and
				 * reallocate below.  Otherwise, we'll use the
				 * extensions already allocated.
				 */
				if (rb.n_vsns > vsns) {
					sam_free_inode_ext(ip, S_IFRFA,
					    SAM_ALL_COPIES,
					    &ip->di.ext_id);
					ip->di.ext_attrs &= ~ext_rfa;
					ip->di.psize.rmfile = 0;
				} else {
					ip->di.psize.rmfile =
					    SAM_RESOURCE_SIZE(rb.n_vsns);
				}
			}
		} else {
			/*
			 * File contains an existing removable media entry
			 * stored as meta data. Remove it. Removable media
			 * entries are now stored as inode extensions.
			 */
			if ((error = sam_clear_ino(ip, (offset_t)0,
			    STALE_ARCHIVE,
			    CRED())) == 0) {
				ip->di.status.b.on_large = 0;
				ip->di.psize.rmfile = 0;
			}
		}
	}
	if (error) {
		goto out;
	}

	if (ip->di.status.b.segment) {
		ip->di.status.b.segment = 0;
		ip->di.rm.info.dk.seg_size = 0;
	}

	/* Allocate enough inode extensions to hold the removable media info */
	if (ip->di.psize.rmfile == 0) {
		int n_exts;

		n_exts = 1 + howmany(rb.n_vsns - 1, MAX_VSNS_IN_INO);
		if ((error = sam_alloc_inode_ext(ip, S_IFRFA, n_exts,
		    &ip->di.ext_id)) == 0) {
			ip->di.ext_attrs |= ext_rfa;
			ip->di.psize.rmfile = SAM_RESOURCE_SIZE(rb.n_vsns);
		}
	}
	if (error) {
		goto out;
	}

	/*
	 * Find first inode extension and set removable media
	 * info from arg values.
	 */
	if ((error = sam_read_ino(ip->mp, ip->di.ext_id.ino, &bp,
					(struct sam_perm_inode **)&eip)) == 0) {
		if (EXT_HDR_ERR(eip, ip->di.ext_id, ip) ||
				EXT_MODE_ERR(eip, S_IFRFA) ||
				(!(EXT_1ST_ORD(eip)))) {
			sam_req_ifsck(ip->mp, -1,
				"sam_request_file: EXT_HDR/EXT_MODE",
					&ip->di.id);
			brelse(bp);
			error = ENOCSI;
		} else {
			eid = eip->hdr.next_id;
			rfp = &eip->ext.rfa.info;
			bzero((caddr_t)rfp, sizeof (struct sam_resource_file));
		}
	}
	if (error) {
		goto fail;
	}

	/* Convert media.  Enter media dependent information. */
	for (n = 0; !(end_od && end_tp); n++) {
		if (!end_od) {
			if (dev_nmod[n] == NULL) {
				end_od = TRUE;
			} else if (strncmp(rb.media, dev_nmod[n],
			    sizeof (rb.media)) == 0) {
				media = DT_OPTICAL | n;
				rfp->resource.archive.mc.od.version =
				    rb.version;
				strncpy(rfp->resource.archive.mc.od.file_id,
				    rb.file_id,
				    sizeof (rfp->resource.archive.mc.od.file_id)
				    -1);
				strncpy(rfp->resource.mc.od.owner_id,
				    rb.owner_id,
				    sizeof (rfp->resource.mc.od.owner_id)-1);
				strncpy(rfp->resource.mc.od.group_id,
				    rb.group_id,
				    sizeof (rfp->resource.mc.od.group_id)-1);
				bcopy(rb.info, rfp->resource.mc.od.info,
				    sizeof (rfp->resource.mc.od.info));
				break;
			}
		}
		if (!end_tp) {
			if (dev_nmtp[n] == NULL) {
				end_tp = TRUE;
			} else if (strncmp(rb.media, dev_nmtp[n],
			    sizeof (rb.media)) == 0) {
				media = DT_TAPE | n;
				break;
			}
		}
	}
	if (media != 0) {
		rfp->resource.archive.rm_info.media = (media_t)media;
	} else {
		brelse(bp);
		error = EINVAL;
		goto fail;
	}

	/* Set up misc removable media info in the resource record */
	rfp->resource.revision = SAM_RESOURCE_REVISION;
	rfp->resource.required_size = howmany(rb.required_size, 1024);
	rfp->n_vsns = rb.n_vsns;

	/* Set up archive info in the resource record */
	rfp->resource.archive.n_vsns = rb.n_vsns;
	strncpy(rfp->resource.archive.vsn, rb.section[0].vsn, sizeof (vsn_t)-1);
	if (rb.n_vsns > 1) {
		rfp->resource.archive.rm_info.volumes = 1;
		ip->di.rm.ui.flags |= RM_VOLUMES;
	}
	if (rb.flags & RI_foreign) {
		rfp->resource.archive.rm_info.stranger = 1;
	}
	if (rb.flags & RI_nopos) {
		/*
		 * FIXME - resolve overloaded use of process_wtm bit.  This flag
		 * has another use but we're out of space in the bit field
		 * (sam_rminfo_t structure).
		 */
		rfp->resource.archive.rm_info.process_wtm = 1;
	}
	rfp->resource.archive.rm_info.position = (uint_t)rb.position;
	if (rb.position || ((rb.flags & RI_foreign) || (rb.flags & RI_nopos))) {
		rfp->resource.archive.rm_info.valid = 1;
		if (rb.flags & RI_blockio) {
			ip->di.rm.ui.flags |= RM_BLOCK_IO;
			ip->di.rm.ui.flags &= ~RM_BUFFERED_IO;
		} else if (rb.flags & RI_bufio) {
			ip->di.rm.ui.flags &= ~RM_BLOCK_IO;
			ip->di.rm.ui.flags |= RM_BUFFERED_IO;
		}
	}

	/* First vsn is stored in section 0 of the first inode extension */
	size = rb.section[0].length;
	if ((rb.flags & RI_foreign) ||
	    (rb.section[0].position && (rb.section[0].length == 0)) ||
	    (rb.flags & RI_nopos)) {
		rb.section[0].length = MAXOFFSET_T;
		size = MAXOFFSET_T;
	}
	bcopy((char *)&rb.section[0], (char *)&rfp->section[0],
	    sizeof (struct sam_vsn_section));

	TRACES(T_SAM_REQ_RM, rvp, (char *)rfp->resource.archive.vsn);
	if (TRANS_ISTRANS(ip->mp)) {
		TRANS_WRITE_DISK_INODE(ip->mp, bp, eip, ip->di.ext_id);
	} else {
		bdwrite(bp);
	}

	/* Copy rest of vsn info from arg buf to remaining inode extensions */
	urb = (struct sam_rminfo *)buf;
	vsnp = &urb->section[1];

	vsns = 1;
	while (eid.ino && (vsns < rb.n_vsns)) {
		if ((error = sam_read_ino(ip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip))) {
			goto fail;
		}
		if (EXT_HDR_ERR(eip, eid, ip) ||
		    EXT_MODE_ERR(eip, S_IFRFA) ||
		    (EXT_1ST_ORD(eip))) {
			sam_req_ifsck(ip->mp, -1,
			    "sam_request_file: EXT_MODE/EXT_HDR/1ST_ORD",
			    &ip->di.id);
			brelse(bp);
			error = ENOCSI;
			goto fail;
		}
		eid = eip->hdr.next_id;
		ino_vsns = MIN((rb.n_vsns - vsns), MAX_VSNS_IN_INO);
		eip->ext.rfv.ord = vsns;
		eip->ext.rfv.n_vsns = ino_vsns;
		if (copyin((caddr_t)vsnp,
		    (caddr_t)&eip->ext.rfv.vsns.section[0],
				sizeof (struct sam_vsn_section) * ino_vsns)) {
			brelse(bp);
			error = EFAULT;
			goto fail;
		}
		for (n = 0; n < ino_vsns; n++) {
			size += eip->ext.rfv.vsns.section[n].length;
			if ((rb.flags & RI_foreign) ||
			    (eip->ext.rfv.vsns.section[n].position &&
			    (eip->ext.rfv.vsns.section[n].length == 0))) {
				eip->ext.rfv.vsns.section[n].length =
				    MAXOFFSET_T;
				size = MAXOFFSET_T;
			}
		}
		vsnp += ino_vsns;
		vsns += ino_vsns;
		if (TRANS_ISTRANS(ip->mp)) {
			TRANS_WRITE_DISK_INODE(ip->mp, bp, eip, eid);
		} else {
			bdwrite(bp);
		}
	}

	ip->di.mode = (ip->di.mode & ~S_IFMT) | S_IFREQ;
	if (ip->mp->mt.fi_version < SAMFS_SBLKV2) {
		ip->di.status.b.remedia = 1;
	}
	ip->di.rm.size = size;
	ip->size = size;
	ip->flags.bits |= SAM_DIRECTIO;
	ip->di.rm.ui.flags &= ~RM_VALID;
	TRANS_INODE(ip->mp, ip);
	sam_mark_ino(ip, SAM_UPDATED);

out:
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	mp = ip->mp;

	TRANS_END_CSYNC(mp, terr, issync, TOP_SETATTR, trans_size);
	if (error == 0) {
		error = terr;
	}

	VN_RELE(vp);
	SAM_CLOSE_OPERATION(mp, error); /* can't use ip->mp since vp released */
	return (error);

fail:
	sam_free_inode_ext(ip, S_IFRFA, SAM_ALL_COPIES, &ip->di.ext_id);
	ip->di.ext_attrs &= ~ext_rfa;
	ip->di.psize.rmfile = 0;
	goto out;
}


/*
 * ----- sam_old_request_file - Process the sam request system call for
 *	file systems 3.5.0 and older.
 *	The removable media information is copied from the user to the removable
 *	media file information extent and the size of the file is set to zero.
 *	NOTE, the buffer cache is used to bypass file paging.
 *  File must NOT be opened, or else EBUSY is returned.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_old_request_file(sam_node_t *ip, vnode_t *vp,
    struct sam_readrminfo_arg args)
{
	int error;
	struct sam_rminfo rb;
	int rm_size;
	void *buf;

	/*
	 * File must be a removable media file or an empty regular file and
	 * not opened or busy. Filesystem must not be read-only.
	 * File must have write permission.
	 */
	if (!S_ISREQ(ip->di.mode) && (ip->di.extent[0] != 0)) {
		error = EEXIST;
	} else if (ip->flags.b.rm_opened) {
		error = EBUSY;
	} else if (ip->di.status.b.damaged) {
		error = ENODATA;
	} else if (ip->mp->mt.fi_mflag & MS_RDONLY) {
		/* If read-only filesystem */
		error = EROFS;
	} else if ((error = sam_access_ino(ip, S_IWRITE, TRUE, CRED())) == 0) {
		buf = (void *)(long)args.buf.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			buf = (void *)args.buf.p64;
		}
		bzero((caddr_t)&rb, sizeof (rb));
		if (copyin((caddr_t)buf, (caddr_t)&rb,
			sizeof (struct sam_rminfo))) {
			error = EFAULT;
#ifdef METADATA_SERVER
		} else if ((rb.flags & RI_foreign) &&
			!samgt.license.license.lic_u.b.foreign_tape) {
			error = ENOTSUP;
#endif	/* METADATA_SERVER */
		} else if (ip->di.psize.rmfile) {
		/*
		 * If requested resource entry is bigger than existing one,
		 * remove and recreate the empty. Create on meta device if one.
		 */
			if (SAM_RESOURCE_SIZE(rb.n_vsns) >
			    ip->di.psize.rmfile) {
				if ((error = sam_clear_ino(ip, (offset_t)0,
				    STALE_ARCHIVE,
				    CRED())) == 0) {
					ip->di.status.b.on_large = 0;
					ip->di.psize.rmfile = 0;
				}
			}
		}
	}
	if (error) {
		return (error);
	}

	if (ip->di.status.b.segment) {
		ip->di.status.b.segment = 0;
		ip->di.rm.info.dk.seg_size = 0;
	}
	if (ip->di.psize.rmfile == 0) {
		int dt, first_unit;

		if (ip->mp->mt.mm_count) {
			dt = MM;
			ip->di.status.b.meta = 1;
		} else {
			dt = DD;
		}
		rm_size = SM_BLK(ip->mp, dt);
		first_unit = ip->mp->mi.m_dk_start[dt];
		ip->di.unit = ip->mp->mi.m_fs[first_unit].dk_unit;
		if (SAM_RESOURCE_SIZE(rb.n_vsns) > rm_size) {
			rm_size = LG_BLK(ip->mp, dt);
			ip->di.status.b.on_large = 1;
		}
	} else {
		rm_size = SAM_RM_SIZE(ip->di.psize.rmfile);
	}
	if (SAM_RESOURCE_SIZE(rb.n_vsns) > rm_size) {
		error = EINVAL;
	} else {
		while ((error = sam_map_block(ip, (offset_t)0,
		    (offset_t)rm_size,
		    SAM_WRITE_BLOCK, NULL, CRED())) != 0 &&
		    IS_SAM_ENOSPC(error)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			error = sam_wait_space(ip, error);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (error) {
				break;
			}
		}
	}
	ASSERT(ip->mp->mi.m_bn_shift == 0);
	if (error == 0) {
		struct sam_rminfo *urb;
		sam_resource_file_t *rfp;
		daddr_t blkno;
		buf_t *bp;

		urb = (struct sam_rminfo *)buf;
		blkno = ip->di.extent[0] << SAM2SUN_BSHIFT;
		bp = getblk(ip->mp->mi.m_fs[ip->di.extent_ord[0]].dev, blkno,
		    rm_size);
		clrbuf(bp);
		rfp = (sam_resource_file_t *)(void *)bp->b_un.b_addr;

		if (rb.position &&
		    secpolicy_fs_config(CRED(), ip->mp->mi.m_vfsp)) {
			error = EPERM;
		} else {
			int media = 0, n;
			int end_od = FALSE, end_tp = FALSE;
			sam_resource_t  *rsp = &rfp->resource;
			sam_archive_t   *arp = &rsp->archive;

			/*
			 * Convert media.
			 * Enter media dependent information.
			 */
			for (n = 0; !(end_od && end_tp); n++) {
				if (!end_od) {
					if (dev_nmod[n] == NULL) {
						end_od = TRUE;
					} else if (strncmp(rb.media,
					    dev_nmod[n],
					    sizeof (rb.media)) == 0) {
						media = DT_OPTICAL | n;
						arp->mc.od.version = rb.version;
						strncpy(arp->mc.od.file_id,
						    rb.file_id,
						    sizeof (arp->mc.od.file_id)
						    -1);
						strncpy(rsp->mc.od.owner_id,
						    rb.owner_id,
						    sizeof (rsp->mc.od.owner_id)
						    -1);
						strncpy(rsp->mc.od.group_id,
						    rb.group_id,
						    sizeof (rsp->mc.od.group_id)
						    -1);
						bcopy(rb.info, rsp->mc.od.info,
						    sizeof (rsp->mc.od.info));
						break;
					}
				}

				if (!end_tp) {
					if (dev_nmtp[n] == NULL) {
						end_tp = TRUE;
					} else if (strncmp(rb.media,
					    dev_nmtp[n],
					    sizeof (rb.media)) == 0) {
						media = DT_TAPE | n;
						break;
					}
				}
			}
			if (media != 0) {
				rfp->resource.archive.rm_info.media =
				    (media_t)media;
			} else {
				error = EINVAL;
			}
		}

		if (error == 0) {
			offset_t size;
			int n;

			rfp->resource.revision = SAM_RESOURCE_REVISION;
			rfp->resource.required_size =
			    howmany(rb.required_size, 1024);
			rfp->n_vsns = rb.n_vsns;
			rfp->resource.archive.n_vsns = rb.n_vsns;
			strncpy(rfp->resource.archive.vsn, rb.section[0].vsn,
			    sizeof (vsn_t)-1);
			bcopy((char *)&rb.section[0], (char *)&rfp->section[0],
			    sizeof (rfp->section[0]));
			if (rfp->n_vsns > 1) {
				rfp->resource.archive.rm_info.volumes = 1;
				ip->di.rm.ui.flags |= RM_VOLUMES;
				if (copyin((caddr_t)&urb->section[1],
				    (caddr_t)&rfp->section[1],
				    (sizeof (struct sam_section) *
				    (rfp->n_vsns - 1)))) {
					error = EFAULT;
				}
			}
			if (rb.flags & RI_foreign) {
				rfp->resource.archive.rm_info.stranger = 1;
			}
			if (rb.flags & RI_nopos) {
				rfp->resource.archive.rm_info.process_wtm = 1;
			}
			for (n = 0, size = 0; n < rfp->n_vsns; n++) {
				size += rfp->section[n].length;
				if ((rb.flags & RI_foreign) ||
				    (rfp->section[n].position &&
				    (rfp->section[n].length == 0)) ||
				    (rb.flags & RI_nopos)) {
					rfp->section[n].length = MAXOFFSET_T;
					size = MAXOFFSET_T;
				}
			}
			TRACES(T_SAM_REQ_RM, vp,
			    (char *)rfp->resource.archive.vsn);
			bdwrite(bp);
			if (error == 0) {
				ip->di.mode = (ip->di.mode & ~S_IFMT) | S_IFREQ;
				if (ip->mp->mt.fi_version < SAMFS_SBLKV2) {
					ip->di.status.b.remedia = 1;
				}
				ip->di.rm.size = size;
				ip->size = size;
				ip->flags.bits |= SAM_DIRECTIO;
				ip->di.rm.ui.flags &= ~RM_VALID;
				ip->di.psize.rmfile =
				    SAM_RESOURCE_SIZE(rb.n_vsns);
				rfp->resource.archive.rm_info.position =
				    (uint_t)rb.position;
				if (rb.position || (rb.flags & RI_foreign) ||
				    (rb.flags & RI_nopos)) {
					rfp->resource.archive.rm_info.valid = 1;
					if (rb.flags & RI_blockio) {
						ip->di.rm.ui.flags |=
						    RM_BLOCK_IO;
						ip->di.rm.ui.flags &=
						    ~RM_BUFFERED_IO;
					} else if (rb.flags & RI_bufio) {
						ip->di.rm.ui.flags &=
						    ~RM_BLOCK_IO;
						ip->di.rm.ui.flags |=
						    RM_BUFFERED_IO;
					}
				}
				TRANS_INODE(ip->mp, ip);
				sam_mark_ino(ip, SAM_UPDATED);
			}
		} else {
			brelse(bp);
		}
	}
	return (error);
}


/*
 * ----- sam_exch_multivolume_copy - exchange two multivolume copies.
 * Handle the exchange of copy info for two multivolume copies caused
 * by the exarchive command.
 *
 * Caller must have the inode read lock held.  This lock is not released.
 */

static int			/* ERRNO if an error, 0 if successful. */
sam_exch_multivolume_copy(
	sam_node_t *bip,	/* Pointer to base inode. */
	int copy1,		/* Copy1 number to become copy2. */
	int copy2)		/* Copy2 number to become copy1. */
{
	int error = 0;
	sam_id_t eid;
	buf_t *bp;
	struct sam_inode_ext *eip;


	ASSERT(bip->di.version >= SAM_INODE_VERS_2);
	if (!(bip->di.ext_attrs & ext_mva)) {	/* No multivolume exts. */
		return (EINVAL);
	}

	/* Examine all inode extensions for multivolume copy1 or copy2. */
	eid = bip->di.ext_id;
	while (eid.ino) {
		if ((error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip))) {
			break;
		}

		if (EXT_HDR_ERR(eip, eid, bip)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_exch_multivolume_copy: EXT_HDR", &bip->di.id);
			error = ENOCSI;
			brelse(bp);
			break;
		}
		if (S_ISMVA(eip->hdr.mode)) {
			if (copy1 == eip->ext.mva.copy) {
				eip->ext.mva.copy = copy2;
			} else if (copy2 == eip->ext.mva.copy) {
				eip->ext.mva.copy = copy1;
			}
		}
		eid = eip->hdr.next_id;
		brelse(bp);
	}

	return (error);
}


/*
 * ----- sam_read_old_rm - Get removable media information.
 */

int				/* ERRNO if error, 0 if successful. */
sam_read_old_rm(sam_node_t *ip, buf_t **rbp)
{
	int error = 0;
	buf_t *bp;
	int rm_size;

	ASSERT(ip->di.version == SAM_INODE_VERS_1);

	/* Removable media info stored as meta data block */
	if (ip->di.status.b.offline) {
		if ((error = sam_proc_offline(ip, 0,
		    (offset_t)ip->di.psize.rmfile,
		    NULL, CRED(), NULL))) {
			return (error);
		}
	}

	if (ip->di.extent[0] == 0) {
		return (ENOENT);
	}

	if ((error = sam_access_ino(ip, S_IREAD, TRUE, CRED()))) {
		return (error);
	}

	rm_size = SAM_RM_SIZE(ip->di.psize.rmfile);
	ASSERT(ip->mp->mi.m_bn_shift == 0);
	error = SAM_BREAD(ip->mp, ip->mp->mi.m_fs[ip->di.extent_ord[0]].dev,
	    ip->di.extent[0], rm_size, &bp);
	if (error) {
		return (error);
	}
	*rbp = bp;
	return (0);
}
