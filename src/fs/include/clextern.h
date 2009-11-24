/*
 * clextern.h - SAM-QFS function prototypes for clients.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef	_SAM_FS_CLEXTERN_H
#define	_SAM_FS_CLEXTERN_H

#ifdef sun
#pragma ident "$Revision: 1.67 $"
#endif

#ifdef sun
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/fbuf.h>
#endif /* sun */

#ifdef linux
#include <linux/types.h>
#include <linux/uio.h>
#endif /* linux */

#include "mount.h"
#include "inode.h"
#include "dirent.h"
#include "segment.h"
#include "global.h"
#include "scd.h"
#include "acl.h"
#include "amld.h"


/*
 * ----- sam_global_tbl_t is the global table for all SAM-FS filsystems.
 */

extern	sam_global_tbl_t	samgt;

/*
 * ----- sam_zero_block is a block of zeros
 */

extern	char	*sam_zero_block;
extern	int	sam_zero_block_size;

/*
 * ----- Externals for DNLC usage
 */
#ifdef sun
extern	boolean_t	sam_use_negative_dnlc;
#endif /* sun */


/*
 * thread.c
 */

#ifdef sun
int sam_start_threads(sam_mount_t *mp);
void sam_stop_threads(sam_mount_t *mp);
int sam_init_thread(sam_thr_t *tp, void (*fct)(), void *data);
#endif /* sun */
#ifdef	linux
int sam_init_thread(sam_thr_t *tp, int (*fct)(void *mp), void *data);
#endif /* linux */

void sam_kill_thread(sam_thr_t *tp);
void sam_kill_threads(sam_thr_t *tp);
void sam_setup_threads(sam_mount_t *mp);
void sam_setup_thread(sam_thr_t *tp);

/*
 * iget.c
 */

int sam_wait_space(sam_node_t *ip, int error);
int sam_enospc(sam_node_t *ip);
int sam_get_symlink(sam_node_t *bip, struct uio *uiop);


/*
 * iput.c
 */

boolean_t sam_is_fsflush(void);

/*
 * creclaim.c
 */

void sam_reclaim_thread(sam_mount_t *mp);
void sam_inactivate_inodes(sam_schedule_entry_t *entry);
void sam_start_relinquish_task(sam_node_t *ip, ushort_t removed_leases,
	uint32_t *leasegen);
void sam_start_extend_task(sam_node_t *ip, ushort_t extend_leases);
#ifdef sun
void sam_reestablish_leases(sam_schedule_entry_t *entry);
void sam_relinquish_leases(sam_schedule_entry_t *entry);
void sam_extend_leases(sam_schedule_entry_t *entry);
#endif /* sun */
#ifdef linux
int sam_reestablish_leases(void *entry);
int sam_relinquish_leases(void *entry);
int sam_extend_leases(void *entry);
#endif /* linux */

#ifdef sun
void sam_expire_client_leases(sam_schedule_entry_t *entry);
#endif /* sun */
#ifdef linux
int sam_expire_client_leases(void *entry);
#endif /* linux */

int sam_init_inode(sam_mount_t *mp);
void sam_sched_expire_client_leases(sam_mount_t *mp, clock_t ticks,
	boolean_t force);

/*
 * getdents.c function prototypes.
 */

#ifdef sun
int sam_getdents(sam_node_t *ip, uio_t *uiop, cred_t *credp, int *eofp,
		fmt_fs_t format);
#endif /* sun */
#ifdef linux
int sam_getdents(sam_node_t *ip, cred_t *credp, filldir_t filldir, void *d,
	struct file *fp, int *ret_count);
#endif /* linux */
int sam_check_zerodirblk(sam_node_t *pip, char *cp, sam_u_offset_t offset);


/*
 * cquota.c function prototypes.
 */

#ifdef sun
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
void sam_quot_unfree(sam_mount_t *mp, struct sam_quot *dqp);
void sam_quot_rel(sam_mount_t *mp, struct sam_quot *qp);
int sam_quota_flush(struct sam_quot *qp);
#endif /* sun */


/*
 * rename.c function prototypes.
 */

int sam_rename_inode(sam_node_t *opip, char *onm, sam_node_t *npip, char *nnm,
		sam_node_t **ip, cred_t *credp);


/* page.c function prototypes. */

void sam_write_done(sam_node_t *ip, uint_t blength);
int sam_flush_pages(sam_node_t *ip, int flags);


typedef enum {SAM_STOP_DAEMON, SAM_START_DAEMON} sam_dstate_t;

/*
 * stage.c stage function prototypes.
 */

int sam_read_proc_offline(sam_node_t *ip, sam_node_t *bip, uio_t *uiop,
	cred_t *credp, int *stage_n_set);
void sam_stage_n_io_done(sam_node_t *ip);
int sam_proc_offline(sam_node_t *ip, offset_t offset, offset_t length,
	offset_t *size, cred_t *credp, int *dontblock);
int sam_cancel_stage(sam_node_t *ip, int error, cred_t *credp);
int	sam_wait_archive(sam_node_t *ip, int archive_w);
int sam_wait_rm(sam_node_t *ip, int flag);

#ifdef sun
void sam_start_stop_rmedia(sam_mount_t *mp, sam_dstate_t);
#endif


/*
 * syscall.c function prototypes.
 */

#ifdef sun
int sam_proc_stat(vnode_t *vp, int cmd, void *args, cred_t *credp);
int sam_proc_file_operations(vnode_t *vp, int cmd, void *arg, cred_t *credp);
int sam_readrm(sam_node_t *ip, char *buf, int bufsize);
int sam_set_realvp(vnode_t *vp, vnode_t **rvp);
int sam_stage_file(sam_node_t *ip, int wait, int copy, cred_t *credp);
int sam_get_multivolume(sam_node_t *bip, struct sam_vsn_section **vsnpp,
		int copy, int *size);
#endif /* sun */

#ifdef linux
int sam_proc_stat(struct inode *li, int cmd, void *args, cred_t *credp);
#endif

int sam_vsn_stat_inode_operation(sam_node_t *ip, int copy,
		int buf_size, void *buf);

/*
 * map.c function prototypes.
 */

int sam_map_block(sam_node_t *ip, offset_t offset, offset_t count,
	sam_map_t flag, struct sam_ioblk *iop, cred_t *credp);
void sam_clear_map_cache(sam_node_t *ip);
int sam_validate_indirect_block(sam_node_t *ip, sam_indirect_extent_t *iep,
	int level);
void sam_zero_sparse_blocks(sam_node_t *ip, offset_t off1, offset_t off2);
offset_t sam_round_to_dau(sam_node_t *ip, offset_t off, sam_round_t dir);
int sam_zero_dau(sam_node_t *ip, offset_t boff, offset_t len,
	sam_map_t flag, ushort_t dt, ushort_t bt);
#ifdef sun
int sam_map_fault(sam_node_t *ip, sam_u_offset_t boff, sam_offset_t len,
	struct map_params *mapp);
#endif

/*
 * scd.c function prototypes.
 */

int sam_send_scd_cmd(enum SCD_daemons scdi, void *cmd, int size);
int sam_send_stage_cmd(sam_mount_t *mp, enum SCD_daemons scdi, void *cmd,
	int size);
int	sam_wait_scd_cmd(enum SCD_daemons scdi, enum uio_seg segflag, void *cmd,
	int size);

#ifdef sun
int sam_send_stageall_cmd(sam_node_t *ip);
int sam_wait_stageall_cmd();
void sam_fsc_daemon_init(sam_mount_t *mp);
void sam_fsc_daemon_fini(sam_mount_t *mp);
#endif /* sun */


/*
 * bio.c function prototypes.
 */

#ifdef sun
int sam_bread(struct sam_mount *mp, dev_t dev, sam_daddr_t blkno, long bsize,
	buf_t **bpp);
int sam_bread_db(struct sam_mount *mp, dev_t dev, daddr_t blkno, long bsize,
	buf_t **bpp);

void sam_blkstale(dev_t dev, daddr_t blkno);
int sam_get_client_sblk(sam_mount_t *mp, enum SHARE_flag wait_flag,
	long bsize, buf_t **bpp);
int sam_sbread(sam_node_t *ip, int ord, sam_daddr_t blkno, long size,
	buf_t **bp);
int sam_bwrite(struct sam_mount *mp, buf_t *bp);
int sam_bwrite2(struct sam_mount *mp, buf_t *bp);
int sam_bwrite_noforcewait_dorelease(struct cam_mount *mp, buf_t *bp);

#define	SAM_BREAD(mp, dev, blkno, bsize, bpp) \
	(TRANS_ISTRANS(mp) ? \
		sam_bread(mp, dev, blkno, bsize, bpp) : \
		sam_bread(NULL, dev, blkno, bsize, bpp))
#define	SAM_BWRITE(mp, bp) \
	(TRANS_ISTRANS(mp) ? sam_bwrite(mp, bp) : sam_bwrite(NULL, bp))
#define	SAM_BWRITE2(mp, bp) \
	(TRANS_ISTRANS(mp) ? sam_bwrite2(mp, bp) : sam_bwrite2(NULL, bp))
#endif /* sun */

#ifdef linux
int sam_bread(struct samdent *dp, sam_daddr_t blkno, long bsize, char *buf);
int sam_sbread(sam_node_t *ip, int ord, sam_daddr_t blkno, long size,
	char *buf);
int sam_get_client_sblk(sam_mount_t *mp, enum SHARE_flag wait_flag,
						long bsize, char *bpp);
#endif /* linux */

typedef enum sam_bufcache_flag {SAM_FLUSH_BC, SAM_STALE_BC
	} sam_bufcache_flag_t;
int sam_flush_indirect_block(sam_node_t *ip, sam_bufcache_flag_t,
	int kptr[], int ik, uint32_t *extent_bn, uchar_t *extent_ord,
	int level, int *set);


/*
 * clcall.c function prototypes.
 */

int sam_share_mount(void *arg, int size, cred_t *credp);
int sam_proc_get_lease(sam_node_t *ip, sam_lease_data_t *dp,
	sam_share_flock_t *flp, void *lmfcb, enum SHARE_flag wait_flag,
	cred_t *credp);
int sam_proc_relinquish_lease(sam_node_t *ip, ushort_t lease_mask,
	boolean_t set_size, uint32_t *leasegen);
int sam_proc_extend_lease(sam_node_t *ip, ushort_t lease_mask);
boolean_t sam_client_record_lease(sam_node_t *ip, enum LEASE_type leasetype,
	int duration);
int sam_truncate_shared_ino(sam_node_t *ip, sam_lease_data_t *dp,
	cred_t *credp);

void sam_client_remove_leases(sam_node_t *ip, ushort_t lease_mask, int exp);
void sam_client_remove_all_leases(sam_node_t *ip);
void sam_client_remove_lease_chain(sam_node_t *ip);
int sam_truncate_shared_ino(sam_node_t *ip, sam_lease_data_t *dp,
	cred_t *credp);
int sam_proc_rm_lease(sam_node_t *ip, ushort_t lease_mask, krw_t rw_type);

int sam_issue_lease_request(sam_node_t *ip, sam_san_lease_msg_t *msg,
	enum SHARE_flag wait_flag, ushort_t actions, void *lmfcb);
int sam_proc_inode(sam_node_t *ip, enum INODE_operation op, void *ptr,
	cred_t *credp);
int sam_proc_block(sam_mount_t *mp, sam_node_t *ip, enum BLOCK_operation op,
	enum SHARE_flag wait_flag, void *ptr);
int sam_proc_block_vfsstat(sam_mount_t *mp, enum SHARE_flag wait_flag);
int sam_stop_inode(sam_node_t *ip);
int sam_send_mount_cmd(sam_mount_t *mp, sam_san_mount_msg_t *msg,
	ushort_t operation, int wait_time);

#ifdef sun
int sam_proc_name(sam_node_t *ip, enum NAME_operation op,
	void *ptr, int argsize, char *cp, char *ncp, cred_t *credp,
	sam_ino_record_t *nrec);
int sam_v_to_v32(vattr_t *vap, sam_vattr_t *va32p);
#endif /* sun */

uint64_t sam_server_has_tag(sam_mount_t *mp, uint64_t tagsbit);
#ifdef METADATA_SERVER
uint64_t sam_client_has_tag(client_entry_t *clp, uint64_t tagsbit);
#endif

#ifdef linux
int sam_proc_name(sam_node_t *ip, enum NAME_operation op,
	void *ptr, int argsize, struct qstr *cp, char *ncp, cred_t *credp,
	sam_ino_record_t *nrec);
int qfs_iattr_to_v32(struct iattr *iap, sam_vattr_t *va32p);
void sam_send_shared_mount(sam_mount_t *mp, int wait_time);
void sam_send_shared_mount_blocking(sam_mount_t *mp);
extern int samqfs_setup_inode(struct inode *li, sam_node_t *ip);
extern int sam_unmount_fs(sam_mount_t *mp, int fflag, sam_unmount_flag_t flag);
extern void sam_set_cred(cred_t *credp, sam_cred_t *sam_credp);
#endif /* linux */


/*
 * clcomm.c function prototypes.
 */

int sam_client_rdsock(void *arg, int size, cred_t *credp);
int sam_write_to_server(sam_mount_t *mp, sam_san_message_t *msg);

#ifdef sun
int sam_read_sock(boolean_t client, int client_ord, sam_mount_t *mp,
	vnode_t *vp, char *rdmsg, cred_t *credp);
int sam_put_sock_msg(sam_mount_t *mp, vnode_t *vp, sam_san_message_t *msg,
	kmutex_t *wrmutex);
void sam_sharefs_thread(sam_mount_t *mp);
#endif /* sun */

#ifdef linux
int sam_read_sock(boolean_t client, int client_ord, sam_mount_t *mp,
	struct socket *sock, char *rdmsg, cred_t *credp);
int sam_put_sock_msg(sam_mount_t *mp, struct socket *sock,
	sam_san_message_t *msg, kmutex_t *wrmutex);
int sam_sharefs_thread(void * mp);
int sam_statfs_thread(void * mp);
#endif /* linux */

int sam_set_sock_fp(int sockfd, sam_sock_handle_t *shp);
void sam_clear_sock_fp(sam_sock_handle_t *sh);


/*
 * clmisc.c function prototypes.
 */

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

#ifdef sun
int sam_client_lookup_name(sam_node_t *pip, char *cp, int stale_interval,
	int flags, sam_node_t **ipp, cred_t *credp);
int sam_get_client_ino(sam_ino_record_t *irec, sam_node_t *pip, char *cp,
	sam_node_t **ipp, cred_t *credp);
#endif /* sun */

#ifdef linux
int sam_client_lookup_name(sam_node_t *pip, struct qstr *cp,
	int stale_interval, int flags, sam_node_t **ipp, cred_t *credp);
int sam_get_client_ino(sam_ino_record_t *irec, sam_node_t *pip,
	struct qstr *cp, sam_node_t **ipp, cred_t *credp);
#endif /* linux */

int sam_refresh_client_ino(sam_node_t *ip, int interval, cred_t *credp);
void sam_directed_actions(sam_node_t *ip, ushort_t actions, offset_t offset,
	cred_t *credp);
int sam_client_frlock_ino(sam_node_t *ip, int cmd, sam_flock_t *flp,
	int filemode, offset_t offset, void *lmfcb, cred_t *credp);
void sam_update_frlock(sam_node_t *ip, int cmd, sam_share_flock_t *flockp,
	int flag, offset_t offset);
void sam_set_cl_attr(sam_node_t *ip, ushort_t actions, sam_cl_attr_t *cl_attr,
	boolean_t force_sync, boolean_t write_lock);
void sam_share_daemon_hold_fs(sam_mount_t *mp);
void sam_share_daemon_rele_fs(sam_mount_t *mp);
void sam_share_daemon_cnvt_fs(sam_mount_t *mp);

/*
 * byte swapping functions (clmisc.c)
 */

int sam_byte_swap_header(sam_san_header_t *hdr);

#ifdef sun
int sam_byte_swap_message(mblk_t *mbp);
#endif /* sun */

#ifdef linux
int sam_byte_swap_message(struct sk_buff *mbp);
#endif	/* linux */

#ifdef linux
sam_iecache_t *sam_iecache_find(sam_node_t *ip, int ord,
	unsigned long long blkno, long bsize);
int sam_iecache_update(sam_node_t *ip, int ord, unsigned long long blkno,
	long bsize, char *bufp);
int sam_iecache_clear(sam_node_t *ip);
int sam_iecache_ent_clear(sam_node_t *ip, uchar_t ord, uint32_t blkno);
#endif /* linux */


/*
 * client.c function prototypes.
 */

#ifdef sun
void sam_remove_frlock(sam_schedule_entry_t *entry);
#endif /* sun */

#ifdef linux
int sam_remove_frlock(void *entry);
#endif /* linux */

void sam_failover_done(sam_mount_t *mp);
int sam_sighup_daemons(sam_mount_t *mp);
void sam_wake_sharedaemon(sam_mount_t *mp, int event);
int sam_reset_client_ino(sam_node_t *ip, int cmd, sam_ino_record_t *irec);
void sam_complete_lease(sam_node_t *ip, sam_san_message_t *msg);
void sam_restart_waiter(sam_mount_t *mp, sam_id_t *id);
void sam_client_cmd(sam_mount_t *mp,  sam_san_message_t *msg);


/*
 * syscall.c function prototypes.
 */

#ifdef sun
int sam_syscall(int cmd, void *arg, int size, rval_t *rvp);
#endif /* sun */

#ifdef linux
int sam_syscall(int cmd, void *arg, int size, int *rvp);
#endif /* linux */


/*
 * psyscall.c function prototypes.
 */

sam_mount_t *sam_find_filesystem(uname_t fs_name);
sam_mount_t *find_mount_point(int fseq);


/*
 * cacl.c function prototypes.
 */

#ifdef sun
int sam_acl_access(sam_node_t *ip, mode_t mode, cred_t *credp);
int sam_acl_get_vsecattr(sam_node_t *ip, vsecattr_t *vsap);
int sam_get_acl(sam_node_t *ip, sam_ic_acl_t **aclpp);
int sam_check_acl(int cnt, sam_acl_t *entp, int flag);
int sam_build_acl(sam_ic_acl_t *aclp);
void sam_free_acl(sam_node_t *ip);
void sam_free_icacl(sam_ic_acl_t *aclp);
#endif /* sun */


#ifdef linux
int sam_init_block(sam_mount_t *mp);
int sam_sys_shareops(void *arg, int size, cred_t *credp, int *rvp);

int rfs_block_read_full_page(struct page *page, get_block_t *get_block);
int rfs_suser(void);
void rfs_d_drop(struct dentry *de);
unsigned int rfs_full_name_hash(const unsigned char *name, unsigned int len);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
int rfs_write_inode_now(struct inode *inode, int sync);
int rfs_try_module_get(struct module *module);
void rfs_module_put(struct module *module);
#endif /* KERNEL_MAJOR */
struct dentry *rfs_d_splice_alias(struct inode *inode, struct dentry *dentry);

loff_t rfs_i_size_read(struct inode *inode);
void rfs_i_size_write(struct inode *inode, loff_t i_size);
struct socket *rfs_sockfd_lookup(int fd, int *err);
void rfs_sockfd_put(struct socket *sock);
#endif /* linux */

#endif /* _SAM_FS_CLEXTERN_H */
