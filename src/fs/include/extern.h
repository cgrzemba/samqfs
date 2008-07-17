/*
 * extern.h - SAM-FS function prototypes.
 *
 *	Defines the prototypes and global externs for the functions.
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

#ifdef linux
/* This is more a note for the human reader rather than the compiler. */
#error This file not used by linux builds.
#endif

#pragma ident "$Revision: 1.234 $"

#include "sam/osversion.h"

#ifndef	_SAM_FS_EXTERN_H
#define	_SAM_FS_EXTERN_H

#include <sys/types.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/fbuf.h>
#include "sam/samevent.h"
#include "mount.h"
#include "inode.h"
#include "dirent.h"
#include "segment.h"
#include "global.h"
#include "scd.h"
#include "acl.h"
#include "amld.h"



/* ----- sam_global_tbl_t is the global table for all SAM-FS filsystems. */

extern	sam_global_tbl_t	samgt;

/* ----- sam_zero_block is a block of zeros */

extern	char				*sam_zero_block;
extern	int				sam_zero_block_size;

/* ----- Externals for DNLC usage */
extern	boolean_t	sam_use_negative_dnlc;

/* ----- External for I/O */
extern int maxphys;			/* from <sys/sunddi.h> */

/* ----- Externals for VPM */

extern int sam_vpm_enable;

#if !defined(SOL_511_ABOVE)

typedef struct vmap {
	caddr_t	vs_addr;		/* mapped address */
	size_t	vs_len;			/* currently fixed at PAGESIZE */
	void	*vs_data;		/* opaque - private data */
} vmap_t;

extern int
vpm_map_pages(struct vnode *vp, u_offset_t off, size_t len,
    int fetchpage, vmap_t *vml, int nseg, int  *newpage,
    enum seg_rw rw);

extern void
vpm_unmap_pages(vmap_t *vml, enum seg_rw rw);

extern int
vpm_sync_pages(struct vnode *vp, u_offset_t off, size_t len,
    uint_t flags);

extern int
vpm_data_copy(struct vnode *vp, u_offset_t off, size_t len,
    struct uio *uio, int fetchpage, int *newpage, int zerostart,
    enum seg_rw rw);

#endif /* SOL_511_ABOVE */

/* ----- Rarely called Solaris functions */

#pragma rarely_called(cmn_err, uprintf)

/* thread.c */

int	sam_start_threads(sam_mount_t *mp);
void sam_stop_threads(sam_mount_t *mp);
int sam_init_thread(sam_thr_t *tp, void (*fct)(sam_mount_t *), void *data);
void sam_kill_thread(sam_thr_t *tp);
void sam_kill_threads(sam_thr_t *tp);
void sam_setup_threads(sam_mount_t *mp);
void sam_setup_thread(sam_thr_t *tp);

#pragma rarely_called(sam_init_thread)

/* balloc.c */

int sam_alloc_block(sam_node_t *ip, int bt, sam_bn_t *bn, int *ord);
int sam_prealloc_blocks(sam_mount_t *mp, sam_prealloc_t *pap);
void sam_copy_to_large(sam_node_t *ip, int size);
int sam_allocate_block(sam_node_t *ip, offset_t offset, sam_map_t flag,
	struct sam_ioblk *iop, struct map_params *mapp, cred_t *credp);

#pragma rarely_called(sam_copy_to_large)

/* block.c */

void sam_block_thread(sam_mount_t *mp);
void sam_free_block(sam_mount_t *mp, int bt, sam_bn_t bn, int ord);
void sam_start_releaser(sam_mount_t *mp);
void sam_check_wmstate(sam_mount_t *mp, fs_wmstate_e state);
void sam_report_fs_watermark(sam_mount_t *mp, int state);
int sam_init_block(sam_mount_t *mp);
void sam_kill_block(sam_mount_t *mp);

#pragma rarely_called(sam_start_releaser, sam_report_fs_watermark)

/* cvnops.c */

int sam_remove_listio(sam_node_t *ip);

/* event.c */

int sam_event_open(void *arg, int size, cred_t *credp);
int sam_event_init(sam_mount_t *mp, int bufsize);
void sam_event_fini(sam_mount_t *mp);
void sam_event_umount(sam_mount_t *mp);
void sam_send_event(sam_node_t *ip, enum sam_event_num event, ushort_t param,
	sam_time_t time);

/* iget.c */

int sam_wait_space(sam_node_t *ip, int error);
int sam_enospc(sam_node_t *ip);
int sam_get_symlink(sam_node_t *bip, struct uio *uiop);
int sam_get_special_file(sam_node_t *ip, vnode_t **vpp, cred_t *credp);
void sam_ilock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t);
void sam_iunlock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t);
void sam_dlock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t);
void sam_dunlock_two(sam_node_t *ip1, sam_node_t *ip2, krw_t);

#pragma rarely_called(sam_wait_space, sam_enospc)

/* inode.c */

int sam_inode_freelist_build(sam_mount_t *mp, int32_t count, int32_t *result);
int sam_inode_freelist_check(sam_mount_t *mp, int itype, int32_t count);

#pragma rarely_called(sam_inode_freelist_build, sam_inode_freelist_check)

/* iput.c */

boolean_t sam_is_fsflush(void);
void sam_detach_aiofile(vnode_t *vp);


/* creclaim.c */

void sam_reclaim_thread(sam_mount_t *mp);
void sam_inactivate_inodes(sam_schedule_entry_t *entry);
void sam_reestablish_leases(sam_schedule_entry_t *entry);
void sam_start_relinquish_task(sam_node_t *ip, ushort_t removed_leases,
	uint32_t *leasegen);
void sam_relinquish_leases(sam_schedule_entry_t *entry);
void sam_sched_expire_client_leases(sam_mount_t *mp, clock_t ticks,
	boolean_t force);


/* reclaim.c function prototypes. */

int sam_init_inode(sam_mount_t *mp);
void sam_resync_server(sam_schedule_entry_t *entry);
void sam_expire_client_leases(sam_schedule_entry_t *entry);
void sam_expire_server_leases(sam_schedule_entry_t *entry);
void sam_sched_expire_server_leases(sam_mount_t *mp, clock_t ticks,
	boolean_t force);
void sam_wait_release_blk_list(struct sam_mount *mp);


/* ialloc.c function prototypes. */

int sam_alloc_inode(sam_mount_t *mp, mode_t mode, sam_ino_t *ino);
void sam_free_inode(sam_mount_t *mp, struct sam_disk_inode *dp);
int sam_alloc_inode_ext(sam_node_t *bip, mode_t mode, int count,
	sam_id_t *fid);
int sam_alloc_inode_ext_dp(sam_mount_t *mp, struct sam_disk_inode *dp,
	mode_t mode, int count, sam_id_t *fid);
void sam_free_inode_ext(sam_node_t *bip, mode_t mode, int copy, sam_id_t *fid);
void sam_free_inode_ext_dp(sam_mount_t *mp, struct sam_disk_inode *dp,
	mode_t mode, int copy, sam_id_t *fid);


/* getdents.c function prototypes. */

int sam_getdents(sam_node_t *ip, uio_t *uiop, cred_t *credp, int *eofp,
		fmt_fs_t format);
int sam_check_zerodirblk(sam_node_t *pip, char *cp, sam_u_offset_t offset);

#pragma rarely_called(sam_check_zerodirblk)

/* lookup.c function prototypes. */

int sam_lookup_name(sam_node_t *pip, char *cp, sam_node_t **ipp,
		struct sam_name *namep, cred_t *credp);
void sam_invalidate_dnlc(vnode_t *vp);
int sam_validate_dir_blk(sam_node_t *pip, offset_t offset, int bsize,
			struct fbuf **fbpp);


/* mount.c function prototypes. */

int sam_flush_and_destroy_vnode(vnode_t *, kmutex_t *, boolean_t *);


/* create.c function prototypes. */

int sam_create_name(sam_node_t *pip, char *cp, sam_node_t **ipp,
	struct sam_name *namep, vattr_t *vap, cred_t *credp);
int sam_make_ino(sam_node_t *pip, vattr_t *vap, sam_node_t **ipp,
	cred_t *credp);
int sam_restore_name(sam_node_t *pip, char *cp, struct sam_name *namep,
	struct sam_perm_inode *perm_ino, sam_id_t *id, cred_t *credp);
void sam_set_unit(sam_mount_t *mp, struct sam_disk_inode *di);
int	sam_set_symlink(sam_node_t *pip, sam_node_t *ip, char *sln,
			int n_chars, cred_t *credp);
int sam_set_old_symlink(sam_node_t *ip, char *sln, int n_chars, cred_t *credp);
int sam_get_old_symlink(sam_node_t *bip, struct uio *uiop, cred_t *credp);
int sam_check_worm_capable(sam_node_t *ip, boolean_t all_flag);


/* cquota.c function prototypes. */

int sam_quota_stat(struct sam_quota_arg *argp);
int sam_is_quota_inode(sam_mount_t *mp, sam_node_t *ip);
int sam_quota_fonline(sam_mount_t *mp, sam_node_t *ip, cred_t *cr);
int sam_quota_foffline(sam_mount_t *mp, sam_node_t *ip, cred_t *cr);
void sam_quota_inode_fini(struct sam_node *ip);
int sam_quota_get_index(sam_node_t *ip, int type);
int sam_quota_get_index_di(sam_disk_inode_t *di, int type);
int testquota(struct sam_dquot *pr, int checktotal);
int sam_quota_fcheck(sam_mount_t *mp, int type, int index, int ol, int tot,
	cred_t *credp);
struct sam_quot *sam_quot_get(sam_mount_t *mp, sam_node_t *ip, int type,
	int index);
void sam_quot_unhash(sam_mount_t *mp, struct sam_quot *dqp);
void sam_quot_unfree(sam_mount_t *mp, struct sam_quot *dqp);
void sam_quot_rel(sam_mount_t *mp, struct sam_quot *qp);
int sam_quota_flush(struct sam_quot *qp);


/* quota.c function prototypes. */

int sam_quota_op(struct sam_quota_arg *argp);
int sam_quota_set_info(sam_node_t *, struct sam_dquot *, int, int, int);
int sam_quota_balloc(sam_mount_t *mp, sam_node_t *ip, long long nblks,
	long long nblks2, cred_t *cr);
int sam_quota_bret(sam_mount_t *mp, sam_node_t *ip, long long nblks,
	long long nblks2);
int sam_quota_falloc(sam_mount_t *mp, uid_t uid, gid_t gid, int aid,
	cred_t *cr);
int sam_quota_chown(sam_mount_t *mp, sam_node_t *ip, uid_t uid, gid_t gid,
	cred_t *cr);
int sam_quota_fret(sam_mount_t *mp, sam_node_t *ip);
int sam_set_admid(void *arg, int size, cred_t *credp);
int sam_set_aid(sam_node_t *ip, struct sam_inode_samaid *sa);
void sam_quota_init(sam_mount_t *mp);
int sam_quota_halt(sam_mount_t *mp);
void sam_quota_fini(sam_mount_t *mp);


/* remove.c function prototypes. */

int sam_remove_name(sam_node_t *ip, char *, sam_node_t *, struct sam_name *,
		cred_t *credp);
int sam_empty_dir(sam_node_t *ip);
int sam_get_hardlink_parent(sam_node_t *bip, sam_node_t *pip);


/* rename.c function prototypes. */

int sam_rename_inode(sam_node_t *opip, char *onm, sam_node_t *npip, char *nnm,
		sam_node_t **ip, cred_t *credp);


/* page.c function prototypes. */

void sam_write_done(sam_node_t *ip, uint_t blength);
int sam_flush_pages(sam_node_t *ip, int flags);
int sam_wait_async_io(sam_node_t *ip, boolean_t write_lock);
void sam_read_ahead(vnode_t *vp, sam_u_offset_t offset,
		sam_u_offset_t vn_off, struct seg *segp, caddr_t addr,
		long long read_ahead);


/* rmedia.c removable media function prototypes.  */

typedef enum {	SAM_DELAY,
		SAM_NODELAY,
		SAM_DELAY_DAEMON_ACTIVE,
		SAM_NODELAY_DAEMON_ACTIVE
} sam_req_t;
typedef enum {SAM_STOP_DAEMON, SAM_START_DAEMON} sam_dstate_t;

int sam_open_rm(sam_node_t *ip, int filemode, cred_t *credp);
int sam_close_rm(sam_node_t *ip, int filemode, cred_t *credp);
int sam_unload_rm(sam_node_t *ip, int filemode, int eox, int wtm,
	cred_t *credp);
int sam_position_rm(sam_node_t *ip, offset_t pos, cred_t *credp);
int sam_rm_direct_io(sam_node_t *ip, enum uio_rw rw, uio_t *uiop,
		cred_t *credp);


/* stage.c stage function prototypes.  */

int sam_read_proc_offline(sam_node_t *ip, sam_node_t *bip, uio_t *uiop,
	cred_t *credp, int *stage_n_set);
void sam_stage_n_io_done(sam_node_t *ip);
int sam_proc_offline(sam_node_t *ip, offset_t offset, offset_t length,
	offset_t *size, cred_t *credp, int *dontblock);
int sam_cancel_stage(sam_node_t *ip, int error, cred_t *credp);
int	sam_wait_archive(sam_node_t *ip, int archive_w);
int sam_wait_rm(sam_node_t *ip, int flag);
void sam_start_stop_rmedia(sam_mount_t *mp, sam_dstate_t);

#pragma rarely_called(sam_read_proc_offline, sam_proc_offline)
#pragma rarely_called(sam_cancel_stage)

/* staged.c stage daemon prototypes.  */

int sam_stagerd_file(sam_node_t *ip, offset_t length, cred_t *credp);
int sam_get_multivolume_copy_id(sam_node_t *bip, int copy, sam_id_t *idp);
int sam_close_stage(sam_node_t *ip, cred_t *credp);
int sam_stage_write_io(vnode_t *vp, uio_t *uiop);
int sam_stage_n_write_io(vnode_t *vp, uio_t *uiop);
int sam_rmmap_block(sam_node_t *ip, offset_t offset, offset_t count,
	sam_map_t flag, struct sam_ioblk *iop);
int sam_build_stagerd_req(sam_node_t *ip, int copy, sam_stage_request_t *req,
	int *req_ext_cnt, sam_stage_request_t **req_ext, cred_t *credp);


/* syscall.c function prototypes. */

int sam_proc_stat(vnode_t *vp, int cmd, void *args, cred_t *credp);
int sam_vsn_stat_inode_operation(sam_node_t *ip, int copy,
				int buf_size, void *buf);
int sam_proc_file_operations(vnode_t *vp, int cmd, void *arg, cred_t *credp);
int sam_readrm(sam_node_t *ip, char *buf, int bufsize);
int sam_set_realvp(vnode_t *vp, vnode_t **rvp);
int sam_stage_file(sam_node_t *ip, int wait, int copy, cred_t *credp);
int sam_get_multivolume(sam_node_t *bip, struct sam_vsn_section **vsnpp,
	int copy, int *size);


/* rmscall.c function prototypes. */

int sam_set_file_operations(sam_node_t *ip, int cmd, char *ops, cred_t *credp);
int sam_proc_archive_copy(vnode_t *vp, int cmd, void *args, cred_t *credp);
int sam_request_file(void *arg);
int sam_read_old_rm(sam_node_t *ip, buf_t **rbp);


/* amld.c command interface prototypes */

enum sam_block_flag {SAM_NOBLOCK = 0, SAM_BLOCK = 1, SAM_BLOCK_DAEMON_ACTIVE};

int sam_put_amld_cmd(sam_fs_fifo_t *cmd, enum sam_block_flag blk_flag);
int sam_send_amld_cmd(sam_fs_fifo_t *cmd, enum sam_block_flag blk_flag);
void sam_shutdown_amld(void);


/* segment.c function prototypes. */

int sam_flush_segment_index(sam_node_t *bip);
int sam_read_segment_info(sam_node_t *bip, offset_t offset, int size,
	char *buf);
int sam_callback_segment(sam_node_t *bip, enum CALLBACK_type type,
	sam_callback_segment_t *callback, boolean_t write_lock);
int sam_callback_one_segment(sam_node_t *bip, int segment_ord,
	enum CALLBACK_type type, sam_callback_segment_t *callback,
	boolean_t write_lock);


/* map.c function prototypes. */

int sam_check_first_block(sam_node_t *ip);

int sam_map_block(sam_node_t *ip, offset_t offset, offset_t count,
	sam_map_t flag, struct sam_ioblk *iop, cred_t *credp);
void sam_clear_map_cache(sam_node_t *ip);
int sam_validate_indirect_block(sam_node_t *ip, sam_indirect_extent_t *iep,
	int level);
int sam_map_fault(sam_node_t *ip, sam_u_offset_t boff, sam_offset_t len,
	struct map_params *mapp);
void sam_zero_sparse_blocks(sam_node_t *ip, offset_t off1, offset_t off2);
offset_t sam_round_to_dau(sam_node_t *ip, offset_t off, sam_round_t dir);
int sam_zero_dau(sam_node_t *ip, offset_t boff, offset_t len,
	sam_map_t flag, ushort_t dt, ushort_t bt);


/* scd.c function prototypes. */

int sam_send_stage_cmd(sam_mount_t *mp, enum SCD_daemons scdi, void *cmd,
	int size);
int sam_send_scd_cmd(enum SCD_daemons scdi, void *cmd, int size);
int	sam_wait_scd_cmd(enum SCD_daemons scdi, enum uio_seg segflag,
			void *cmd, int size);
int sam_send_stageall_cmd(sam_node_t *ip);
int sam_wait_stageall_cmd(void);
void sam_fsc_daemon_init(sam_mount_t *mp);
void sam_fsc_daemon_fini(sam_mount_t *mp);


/* update.c function prototypes. */

int sam_update_filsys(sam_mount_t *mp, int flag);
int sam_update_inode(sam_node_t *ip, enum sam_sync_type, boolean_t sync_attr);
int sam_write_ino_sector(sam_mount_t *mp, buf_t *bp, sam_ino_t ino);
int sam_flush_extents(sam_node_t *ip);
int sam_free_lg_block(sam_mount_t *mp, sam_bn_t bn, int ord);
int sam_merge_sm_blks(sam_node_t *ip, sam_bn_t bn, int ord);
void sam_req_fsck(sam_mount_t *mp, int slice, char *msg);
void sam_req_ifsck(sam_mount_t *mp, int slice, char *msg, sam_id_t *ino);
int sam_sync_meta(sam_node_t *pip, sam_node_t *cip, cred_t *credp);
boolean_t sam_update_all_sblks(sam_mount_t *mp);
boolean_t sam_update_sblk(sam_mount_t *mp, uchar_t ord, int sblk_no,
	boolean_t sbinfo_only);
int sam_update_the_sblks(sam_mount_t *mp);

#pragma rarely_called(sam_req_fsck)


/* vnops.c function prototypes. */

int sam_EIO(void);


/* bio.c function prototypes. */

void sam_blkstale(dev_t dev, daddr_t blkno);
int sam_bread(struct sam_mount *mp, dev_t dev, sam_daddr_t blkno, long bsize,
	buf_t **bpp);
int sam_bread_db(struct sam_mount *mp, dev_t dev, daddr_t blkno, long bsize,
	buf_t **bpp);
int sam_get_client_sblk(sam_mount_t *mp, enum SHARE_flag wait_flag,
	long bsize, buf_t **bpp);
int sam_get_client_inode(sam_mount_t *mp, sam_id_t *fp,
	enum SHARE_flag wait_flag, long bsize, buf_t **bpp);
int sam_sbread(sam_node_t *ip, int ord, sam_daddr_t blkno, long size,
	buf_t **bp);
int sam_bwrite(struct sam_mount *mp, buf_t *bp);
int sam_bwrite2(struct sam_mount *mp, buf_t *bp);
int sam_bwrite_noforcewait_dorelease(struct sam_mount *mp, buf_t *bp);

#define	SAM_BREAD(mp, dev, blkno, bsize, bpp) \
	(TRANS_ISTRANS(mp) ? \
		sam_bread(mp, dev, blkno, bsize, bpp) : \
		sam_bread(NULL, dev, blkno, bsize, bpp))
#define	SAM_BWRITE(mp, bp) \
	(TRANS_ISTRANS(mp) ? sam_bwrite(mp, bp) : sam_bwrite(NULL, bp))
#define	SAM_BWRITE2(mp, bp) \
	(TRANS_ISTRANS(mp) ? sam_bwrite2(mp, bp) : sam_bwrite2(NULL, bp))

typedef enum sam_bufcache_flag {
	SAM_FLUSH_BC,
	SAM_STALE_BC
} sam_bufcache_flag_t;

int sam_flush_indirect_block(sam_node_t *ip, sam_bufcache_flag_t,
	int kptr[], int ik, uint32_t *extent_bn, uchar_t *extent_ord,
	int level, int *set);


/* clcall.c function prototypes. */

int sam_share_mount(void *arg, int size, cred_t *credp);
void sam_send_shared_mount(sam_mount_t *mp);
int sam_proc_get_lease(sam_node_t *ip, sam_lease_data_t *dp,
	sam_share_flock_t *flp, void *lmfcb, cred_t *credp);
int sam_proc_relinquish_lease(sam_node_t *ip, ushort_t lease_mask,
	boolean_t set_size, uint32_t *leasegen);
boolean_t sam_client_record_lease(sam_node_t *ip, enum LEASE_type leasetype,
	int duration);
void sam_client_remove_leases(sam_node_t *ip, ushort_t lease_mask, int exp);
void sam_client_remove_all_leases(sam_node_t *ip);
void sam_client_remove_lease_chain(sam_node_t *ip);
int sam_truncate_shared_ino(sam_node_t *ip, sam_lease_data_t *dp,
	cred_t *credp);
int sam_proc_rm_lease(sam_node_t *ip, ushort_t lease_mask, krw_t rw_type);
int sam_issue_lease_request(sam_node_t *ip, sam_san_lease_msg_t *msg,
	enum SHARE_flag wait_flag, ushort_t actions, void *lmfcb);
int sam_proc_name(sam_node_t *ip, enum NAME_operation op,
	void *ptr, int argsize, char *cp, char *ncp, cred_t *credp,
	sam_ino_record_t *nrec);
int sam_proc_inode(sam_node_t *ip, enum INODE_operation op, void *ptr,
	cred_t *credp);
int sam_proc_block(sam_mount_t *mp, sam_node_t *ip, enum BLOCK_operation op,
	enum SHARE_flag wait_flag, void *ptr);
int sam_proc_block_vfsstat(sam_mount_t *mp, enum SHARE_flag wait_flag);
int sam_send_mount_cmd(sam_mount_t *mp, sam_san_mount_msg_t *msg,
	ushort_t operation, int wait_time);
int sam_stop_inode(sam_node_t *ip);
int sam_v_to_v32(vattr_t *vap, sam_vattr_t *va32p);

uint64_t sam_server_has_tag(sam_mount_t *mp, uint64_t tagsbit);
#ifdef METADATA_SERVER
uint64_t sam_client_has_tag(client_entry_t *clp, uint64_t tagsbit);
#endif

/* clcomm.c function prototypes. */

int sam_client_rdsock(void *arg, int size, cred_t *credp);
int sam_read_sock(boolean_t client, int client_ord, sam_mount_t *mp,
	vnode_t *vp, char *rdmsg, cred_t *credp);
int sam_write_to_server(sam_mount_t *mp, sam_san_message_t *msg);
int sam_put_sock_msg(sam_mount_t *mp, vnode_t *vp, sam_san_message_t *msg,
	kmutex_t *wrmutex);
int sam_set_sock_fp(int sockfd, sam_sock_handle_t *shp);
void sam_clear_sock_fp(sam_sock_handle_t *sh);
void sam_sharefs_thread(sam_mount_t *mp);


/* clmisc.c function prototypes. */

int sam_set_client(void *arg, int size, cred_t *credp);
int sam_stale_indirect_blocks(sam_node_t *ip, offset_t offset);
void sam_build_header(sam_mount_t *mp, sam_san_header_t *hdr, ushort_t cmd,
	enum SHARE_flag wait_flag, ushort_t operation,
	ushort_t length, ushort_t out_length);
int sam_update_shared_filsys(sam_mount_t *mp, enum SHARE_flag wait_flag,
	int flag);
int sam_update_shared_ino(sam_node_t *ip, enum sam_sync_type st,
	boolean_t write_lock);
int sam_update_shared_sblk(sam_mount_t *mp, enum SHARE_flag wait_flag);
int sam_client_get_acl(sam_node_t *ip, cred_t *credp);
int sam_client_lookup_name(sam_node_t *pip, char *cp, int stale_interval,
	sam_node_t **ipp, cred_t *credp);
int sam_refresh_client_ino(sam_node_t *ip, int interval, cred_t *credp);
int	sam_stale_hash_ino(sam_id_t *id, sam_mount_t *mp, sam_node_t **ipp);
void sam_directed_actions(sam_node_t *ip, ushort_t actions, offset_t offset,
	cred_t *credp);
int sam_client_frlock_ino(sam_node_t *ip, int cmd, sam_flock_t *flp,
	int filemode, offset_t offset, void *lmfcb, cred_t *credp);
void sam_update_frlock(sam_node_t *ip, int cmd, sam_share_flock_t *flockp,
	int flag, offset_t offset);
int sam_get_client_ino(sam_ino_record_t *irec, sam_node_t *pip, char *cp,
	sam_node_t **ipp, cred_t *credp);
void sam_set_cl_attr(sam_node_t *ip, ushort_t actions, sam_cl_attr_t *cl_attr,
	boolean_t force_sync, boolean_t write_lock);
void sam_share_daemon_hold_fs(sam_mount_t *mp);
void sam_share_daemon_rele_fs(sam_mount_t *mp);
void sam_share_daemon_cnvt_fs(sam_mount_t *mp);

#pragma rarely_called(sam_stale_indirect_blocks)

/*
 * byte swapping functions (clmisc.c)
 */
int sam_byte_swap_header(sam_san_header_t *hdr);
int sam_byte_swap_message(mblk_t *mbp);
#ifdef _SAM_FS_SBLK_H
int byte_swap_sb(struct sam_sblk *, size_t);
int byte_swap_lb(struct sam_label_blk *);
#pragma rarely_called(byte_swap_sb, byte_swap_lb)
#endif	/* _SAM_FS_SBLK_H */
#ifdef SAM_HOST_H
int byte_swap_hb(struct sam_host_table_blk *);
#pragma rarely_called(byte_swap_hb)
#endif	/* SAM_HOST_H */

#pragma rarely_called(sam_byte_swap_header, sam_byte_swap_message)

/* client.c function prototypes. */

void sam_client_cmd(sam_mount_t *mp,  sam_san_message_t *msg);
void sam_failover_done(sam_mount_t *mp);
int sam_sighup_daemons(sam_mount_t *mp);
void sam_wake_sharedaemon(sam_mount_t *mp, int event);
int sam_reset_client_ino(sam_node_t *ip, int cmd, sam_ino_record_t *irec);
void sam_complete_lease(sam_node_t *ip, sam_san_message_t *msg);
void sam_restart_waiter(sam_mount_t *mp, sam_id_t *id);
void sam_remove_frlock(sam_schedule_entry_t *entry);


/* srcomm.c function prototypes. */

int sam_server_rdsock(void *arg, int size, cred_t *credp);
int sam_write_to_client(sam_mount_t *mp, sam_san_message_t *msg);
void sam_init_client_array(sam_mount_t *mp);
client_entry_t *sam_get_client_entry(sam_mount_t *mp, int clord, int create);
void sam_free_incore_host_table(sam_mount_t *mp);

/* server.c function prototypes. */

int sam_calculate_lease_timeout(int client_interval);
void sam_server_cmd(sam_mount_t *mp,  mblk_t *mbp);
int sam_process_lease_request(sam_node_t *ip, sam_san_message_t *msg);
void sam_callout_acl(sam_node_t *ip, int client_ord);
void sam_callout_abr(sam_node_t *ip, int client_ord);
int sam_setup_stage(sam_node_t *ip, cred_t *credp);
int sam_process_inode_request(sam_node_t *ip, int ord, ushort_t op,
	sam_san_inode_t *inp);
int sam_client_getino(sam_node_t *ip, int client_ord);
int sam_proc_callout(sam_node_t *ip, enum CALLOUT_operation op,
	ushort_t actions, int client_ord, sam_callout_arg_t *arg);
int sam_proc_notify(sam_node_t *ip, enum NOTIFY_operation op, int client_ord,
	sam_notify_arg_t *arg);


/* srmisc.c function prototypes. */

int sam_voluntary_failover(void *arg, int size, cred_t *credp);
int sam_set_server(void *arg, int size, cred_t *credp);
int sam_sgethosts(void *arg, int size, int wr, cred_t *credp);
int sam_sys_shareops(void *arg, int size, cred_t *credp, int *rvp);
int sam_shared_failover(sam_mount_t *mp, enum MOUNT_operation cmd, int ord);
void sam_failover_old_server(sam_mount_t *mp, char *server, cred_t *credp);
void sam_notify_staging_clients(sam_node_t *ip);
int sam_onoff_client(void *arg, int size, cred_t *credp);


/* syscall.c function prototypes. */

int sam_syscall(int cmd, void *arg, int size, rval_t *rvp);


/* psyscall.c function prototypes. */

sam_mount_t *sam_find_filesystem(uname_t fs_name);
sam_mount_t *find_mount_point(int fseq);
sam_mount_t *sam_dup_mount_info(sam_mount_t *);
void sam_stale_mount_info(sam_mount_t *);
void sam_destroy_stale_mount(sam_mount_t *);


/* cacl.c function prototypes. */

int sam_acl_access(sam_node_t *ip, mode_t mode, cred_t *credp);
int sam_acl_get_vsecattr(sam_node_t *ip, vsecattr_t *vsap);
int sam_get_acl(sam_node_t *bip, sam_ic_acl_t **aclpp);
int sam_check_acl(int cnt, sam_acl_t *entp, int flag);
int sam_build_acl(sam_ic_acl_t *aclp);
void sam_free_icacl(sam_ic_acl_t *aclp);
void sam_free_acl(sam_node_t *ip);
int sam_reset_client_acl(sam_node_t *ip, int nacl, int ndfacl, void *,
			int len);

/* acl.c function prototypes. */

int sam_acl_create(sam_node_t *ip, sam_node_t *pip);
int sam_acl_inherit(sam_node_t *bip, sam_node_t *pip, sam_ic_acl_t **aclpp);
int sam_acl_setattr(sam_node_t *ip, struct vattr *vap);
int sam_acl_flush(sam_node_t *ip);
int sam_acl_set_vsecattr(sam_node_t *ip, vsecattr_t *vsap);
int sam_set_acl(sam_node_t *bip, sam_ic_acl_t **aclpp);
int sam_acl_inactive(sam_node_t *ip);

#endif /* _SAM_FS_EXTERN_H */
