/*
 * ----- objnops.h - Object node operations definition.
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
#ifndef _OBJNOPS_H
#define	_OBJNOPS_H

#pragma ident "$Revision: 1.2 $"

typedef struct objnodeops objnodeops_t;

/*
 * Object ops mode
 */
#define	OBJECTOPS_NORMAL	0	/* Normal SAM-FS calls */
#define	OBJECTOPS_SIM_IOPS	1	/* READ/WRITE returns immediate */
#define	OBJECTOPS_SIM_MEM	2	/* READ/WRITE from memory only */

/*
 * OSD IO Option
 */
#define	OBJECTIO_OPTION_FUA	0x8	/* Cache but flush to stable storage */
#define	OBJECTIO_OPTION_DPO	0x10	/* Cannot Cache - Direct SAMQFS IO */

/*
 * Object memory size if OBJECTOPS_MEM_SIM
 */
#define	OBJECTOPS_MEM_SIM_SIZE	1024 * 1024 /* Allocate 1 MBytes */

/*
 * Object types - ip->di2.objtype
 */
typedef enum objtype {

	OBJNON  = 0,
	OBJROOT = 1,
	OBJPAR  = 2,
	OBJCOL  = 3,
	OBJUSER = 4

} objtype_t;

typedef struct objnode {

	kmutex_t	obj_mutex;	/* protects access to this object */
	uint_t		obj_busy;	/* this object is in use */

	uint32_t	obj_flags;	/* general flags */
	krwlock_t	obj_lock;	/* Read/Write lock for this object */

	struct objnodeops *obj_op;	/* object ops */
	enum objtype	obj_type;	/* object type */
	void		*obj_data;	/* private data e.g. inode */
	char		*obj_membuf;	/* Memory buffer if OBJECTOPS_MEM_SIM */
	struct list	obj_tasklist;	/* list of task on this object */

} objnode_t;

/*
 * OBJNODE_OPS defines all the object operations.  It is used to define
 * the objnodeops structure
 */
#define	OBJNODE_OPS							\
	int	(*objop_busy)(objnode_t *, cred_t *,			\
				caller_context_t *);			\
	int	(*objop_unbusy)(objnode_t *, cred_t *,			\
				caller_context_t *);			\
	int	(*objop_rele)(objnode_t *);				\
	void	(*objop_rw_enter)(objnode_t *, krw_t);			\
	int	(*objop_rw_tryenter)(objnode_t *, krw_t);		\
	void	(*objop_rw_exit)(objnode_t *);				\
	int	(*objop_par_create)(objnode_t *, mode_t, uint64_t *,	\
				objnode_t **, void *, void *,		\
				void *, cred_t *);			\
	int	(*objop_col_create)(objnode_t *, mode_t, uint64_t *,	\
				objnode_t **, void *, void *, void *,	\
				cred_t *);				\
	int	(*objop_create)(objnode_t *, mode_t, uint64_t *num,	\
				uint64_t *buf, void *, void *, void *,	\
				cred_t *);				\
	int	(*objop_read)(objnode_t *, uint64_t, uint64_t, void *,	\
				int, int, uint64_t *, void *, void *,	\
				void *, cred_t *);			\
	int	(*objop_write)(objnode_t *, uint64_t, uint64_t, void *,	\
				int, int, uint64_t *, void *, void *,	\
				void *, cred_t *);			\
	int	(*objop_flush)(objnode_t *, uint64_t, uint64_t,		\
				uint8_t, void *, void *, void *,	\
				cred_t *);				\
	int	(*objop_remove)(objnode_t *, void *, void *, void *,	\
				cred_t *);				\
	int	(*objop_truncate)(objnode_t *, uint64_t, void *, void *,\
				void *, cred_t *);			\
	int	(*objop_append)(struct objnode *, uint64_t, void *,	\
				int, int, uint64_t *, uint64_t *,	\
				void *, void *, void *, cred_t *);	\
	int	(*objop_setattr)(struct objnode *, uint32_t, uint32_t,	\
				uint64_t, void *, void *, void *,	\
				cred_t *);				\
	int	(*objop_getattr)(struct objnode *, uint32_t, uint32_t,	\
				uint64_t, void *, void *, void *,	\
				cred_t *);				\
	int	(*objop_getattrpage)(objnode_t *objnodep,		\
				uint32_t pagenum, uint64_t len,		\
				void *buf, void *, void *,		\
				cred_t *credp);				\
	int	(*objop_punch)(struct objnode *, uint64_t, uint64_t,	\
				void *, void *, void *, cred_t *);	\
	int	(*objop_dummy)()

	/* NB: No ";" */

/*
 * Object Operations
 */
struct objnodeops {
	OBJNODE_OPS;	/* Signatures of all object operations */
};

#define	OBJNODE_BUSY(objnodep, cr, ct)					\
	(*(objnodep)->obj_op->objop_busy)(objnodep, cr, ct);

#define	OBJNODE_UNBUSY(objnodep, cr, ct)				\
	(*(objnodep)->obj_op->objop_unbusy)(objnodep, cr, ct);

#define	OBJNODE_RELE(objnodep)						\
	(*(objnodep)->obj_op->objop_rele)(objnodep)

#define	OBJNODE_RW_ENTER(objnodep, enter_type)				\
	(*(objnodep)->obj_op->objop_rw_enter)(objnodep, enter_type);

#define	OBJNODE_RW_TRYENTER(objnodep, enter_type)			\
	(*(objnodep)->obj_op->objop_rw_tryenter)(objnodep, enter_type);

#define	OBJNODE_RW_EXIT(objnodep)					\
	(*(objnodep)->obj_op->objop_rw_exit(objnodep);

#define	OBJNODE_PAR_CREATE(objnodep, mode, id, new_objnodepp, cap, sec,	\
				curcmdpg, cr)				\
	(*(objnodep)->obj_op->objop_par_create)(objnodep, mode, id,	\
	new_objnodepp, cap, sec, curcmdpg, cr);

#define	OBJNODE_COL_CREATE(objnodep, mode, id, new_objnodepp, cap, sec,	\
				curcmdpg, cr)				\
	(*(objnodep)->obj_op->objop_col_create)(objnodep, mode, id,	\
	new_objnodepp, cap, sec, curcmdpg, cr);

#define	OBJNODE_CREATE(objnodep, mode, num, list, cap, sec, curcmdpg,	\
				cr)					\
	(*(objnodep)->obj_op->objop_create)(objnodep, mode, num,	\
	list, cap, sec, curcmdpg, cr);

#define	OBJNODE_READ(objnodep, offset, len, bufp, segflg, io_option,	\
	    size_read, cap, sec, curcmdpg, cr)				\
	(*(objnodep)->obj_op->objop_read)(objnodep, offset, len, bufp,	\
	    segflg, io_option, size_read, cap, sec, curcmdpg, cr);

#define	OBJNODE_WRITE(objnodep, offset, len, bufp, segflg, io_option,	\
	    size_written, cap, sec, curcmdpg, cr)			\
	(*(objnodep)->obj_op->objop_write)(objnodep, offset, len, bufp,	\
	    segflg, io_option, size_written, cap, sec, curcmdpg, cr)

#define	OBJNODE_FLUSH(objnodep, offset, len, flush_scope, cap, sec,	\
	    curcmdpg, cr)						\
	(*(objnodep)->obj_op->objop_flush)(objnodep, offset, len,	\
	    flush_scope, cap, sec, curcmdpg, cr);

#define	OBJNODE_REMOVE(objnodep, cap, sec, curcmdpg, cr)		\
	(*(objnodep)->obj_op->objop_remove)(objnodep, cap, sec, curcmdpg, cr);

#define	OBJNODE_TRUNCATE(objnodep, offset, cap, sec, curcmdpg, cr)	\
	(*(objnodep)->obj_op->objop_truncate)(objnodep, offset, cap,	\
	    sec, cr);

#define	OBJNODE_APPEND(objnodep, len, bufp, segflg, io_option,		\
				size_written, start_append_addr, cap,	\
				sec, curcmdpg, cr)			\
	(*(objnodep)->obj_op->objop_append)(objnodep, len, bufp,	\
	    segflg, io_option, size_written, start_append_addr, cap,	\
	    sec, curcmdpg, cr);

#define	OBJNODE_SETATTR(objnodep, pagenum, attrnum, len, buf, cap, sec,	\
				credp)					\
	(*(objnodep)->obj_op->objop_setattr)(objnodep, pagenum,		\
	    attrnum, len, buf, cap, sec, credp);

#define	OBJNODE_GETATTR(objnodep, pagenum, attrnum, len, buf, cap, sec,	\
				credp)					\
	(*(objnodep)->obj_op->objop_getattr)(objnodep, pagenum,		\
	    attrnum, len, buf, cap, sec, credp);

#define	OBJNODE_GETATTRPAGE(objnodep, pagenum, len, buf, cap, sec,	\
				credp)					\
	(*(objnodep)->obj_op->objop_getattrpage)(objnodep, pagenum,	\
	    len, buf, cap, sec, credp);

#define	OBJNODE_PUNCH(objnodep, start, length, cap, sec, curcmdpg, cr)	\
	*(objnodep)->obj_op->objop_punch)(objnodep, start, length,	\
	    cap, sec, curcmdpg, cr);

#define	OBJNODE_DUMMY()							\
	(*(objnodep)->obj_op->objop_dummy)();

#endif /* _OBJNOPS_H */
