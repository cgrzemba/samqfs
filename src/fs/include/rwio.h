/*
 *	rwio.h - SAM-QFS file system memory map parameters.
 *
 *	Memory map parameters for SAM-QFS file system.
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
#pragma ident "$Revision: 1.44 $"
#endif


#ifndef	_SAM_FS_MMAP_H
#define	_SAM_FS_MMAP_H

#ifdef	sun
#include	<sys/types.h>
#include	<sys/vnode.h>
#include	<sys/cred.h>

#include	<vm/page.h>
#include	<vm/as.h>
#include	<vm/seg_vn.h>
#include	<vm/seg_map.h>
#endif	/* sun */

#ifdef	linux
#include	<linux/types.h>
#endif	/* linux */


#ifdef sun

/* ----- Struct for segment descriptor */

typedef struct segment_descriptor {
	offset_t offset;
	uint_t resid;
	uint_t ord_resid;
	int ordinal;
} segment_descriptor_t;

/* ----- Struct for "per I/O vector" element for async buffer */

typedef	struct iov_descriptor {
	offset_t blength;
	void *base;
	struct page **pplist;
	boolean_t pagelock_failed;
} iov_descriptor_t;

/* ----- Struct for expanded buf element and its components */

typedef struct sam_buf_link {
	struct sam_buf_link *next;
	struct sam_buf_link *prev;
} sam_buf_link_t;

#define	SAM_BUF_LINK_INIT(lp)	{	\
	(lp)->next = (lp);		\
	(lp)->prev = (lp);		\
}

#define	SAM_BUF_ENQ(hp, lp)	{	\
	(lp)->prev = (hp)->prev;	\
	(hp)->prev->next = (lp);	\
	(hp)->prev = (lp);		\
	(lp)->next = (hp);		\
}

#define	SAM_BUF_DEQ(lp)	{		\
	(lp)->prev->next = (lp)->next;	\
	(lp)->next->prev = (lp)->prev;	\
	(lp)->prev = NULL;		\
	(lp)->next = NULL;		\
}

typedef struct sam_buf {
	buf_t	buf;				/* must be the first element */
	sam_buf_link_t link;		/* managed by caller */
} sam_buf_t;


/*
 * ----- Struct for direct I/O buffer descriptor
 *
 * Fields which are read-only once initialized:
 *   ip
 *   procp
 *   asp
 *   aiouiop
 *   resid
 *   rem
 *   iov_count
 *
 * Fields modified only by the issuing thread:
 *   length
 *   iov
 *   write_bytes_outstanding
 *
 * Fields protected by bdp_mutex:
 *   io_count
 *   error
 *   dio_link
 *
 * Fields protected by the use of atomic operations:
 *   remaining_length
 *
 */

typedef struct buf_descriptor {
	sam_node_t *ip;
	struct proc *procp;
	struct as *asp;
	struct samaio_uio *aiouiop;
	offset_t resid;
	offset_t rem;
	offset_t write_bytes_outstanding;
	ksema_t io_sema;	/* I/O Sync - count of completed I/Os */
	int io_count;
	int error;
	offset_t length;
	offset_t remaining_length;
	int iov_count;
	kmutex_t bdp_mutex;
	sam_buf_link_t dio_link; /* Chain of outstanding i/o buffer pointers */
	iov_descriptor_t iov[1];
} buf_descriptor_t;


/* i/o-related macros */

/*
 * SAM_DIRECTIO_ALLOWED - determine if direct i/o is allowable on a file.
 *
 * We allow direct i/o if we can eliminate all in-memory pages for the file.
 * This requires that no pages be mapped, and no pages in use by the stager.
 */

#define	SAM_DIRECTIO_ALLOWED(ip) \
	((ip)->mm_pages == 0)

#endif /* sun */

/* segment.c function prototypes. */

#ifdef	sun
int sam_get_segment_ino(sam_node_t *bip, int segment_ord, sam_node_t ** ipp);
int sam_setup_segment(sam_node_t *bip, uio_t *uiop, enum uio_rw rw,
	segment_descriptor_t *seg, cred_t *credp);
int sam_get_segment(sam_node_t *bip, uio_t *uiop, enum uio_rw,
	segment_descriptor_t *seg, sam_node_t **ip, cred_t *credp);
void sam_release_segment(sam_node_t *bip, uio_t *uiop, enum uio_rw,
	segment_descriptor_t *seg, sam_node_t *ip);
int sam_read_segment_info(sam_node_t *bip,	offset_t offset, int size,
	char *buf);
int sam_clear_segment(sam_node_t *ip, offset_t length,
	enum sam_clear_ino_type flag, cred_t *credp);

/* ----- Memory map function prototypes.  */

int sam_read_io(vnode_t *vp, uio_t *uiop, int ioflag);
int sam_read_stage_n_io(vnode_t *vp, uio_t *uiop);
int sam_write_io(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *credp);
#endif	/* sun */

int sam_get_zerodaus(sam_node_t *ip, sam_lease_data_t *dp, cred_t *credp);
int sam_process_zerodaus(sam_node_t *ip, sam_lease_data_t *dp, cred_t *credp);

#ifdef	sun
int sam_dk_direct_io(sam_node_t *ip, enum uio_rw rw, int ioflag,
	uio_t *uiop, cred_t *credp);
void sam_dk_aio_direct_done(sam_node_t *ip, buf_descriptor_t *bdp,
	offset_t count);
#if defined(SOL_511_ABOVE)
int sam_issue_direct_object_io(sam_node_t *ip, enum uio_rw rw, struct buf *bp,
	sam_ioblk_t *iop, offset_t contig);
buf_t *sam_pageio_object(sam_node_t *ip, sam_ioblk_t *iop, page_t *pp,
	offset_t offset, uint_t pg_off, uint_t vn_len, int flags);
int sam_pg_object_sync_done(sam_node_t *ip, buf_t *bp, char *str);
#else /* defined SOL_511_ABOVE */
#define	sam_issue_direct_object_io(a, b, c, d, e)	(ENOTSUP)
#define	sam_pageio_object(a, b, c, d, e, f, g)		(NULL)
#define	sam_pg_object_sync_done(a, b, c)		(ENOTSUP)
#endif /* defined (SOL_511_ABOVE) */
void sam_page_wrdone(buf_t *bp);

void sam_flush_behind(sam_node_t *ip, cred_t *credp);
int sam_aphysio(vnode_t *vp, int ioflag, int rw, struct aio_req *aio, int *rvp,
	cred_t *credp);

int sam_getpages(vnode_t *vp, u_offset_t offset, uint_t length,
	struct page **pl, uint_t ps, struct seg *psegp, caddr_t addr,
	enum seg_rw rw, int sparse, boolean_t lock_set);
int sam_putapage(vnode_t *vp, page_t *pp, u_offset_t *offset, size_t *length,
	int flags, cred_t *credp);
int sam_putpages(vnode_t *vp, offset_t offset, size_t length, int flags,
	cred_t *credp);
#endif	/* sun */


#endif	/* _SAM_FS_MMAP_H */
