/*
 * ----- objnode.h - Object node definition.
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
#ifndef _OBJNODE_H
#define	_OBJNODE_H

#pragma ident "$Revision: 1.3 $"

#include "pub/objnops.h"

#if defined(SOL_511_ABOVE)
extern void sam_objnode_alloc(sam_node_t *ip);
extern void sam_objnode_init(sam_node_t *ip);
extern void sam_objnode_free(sam_node_t *ip);
#else
#define	sam_objnode_alloc(ip)
#define	sam_objnode_init(ip)
#define	sam_objnode_free(ip)
#endif

extern struct objnodeops *samfs_objnodeopsp;
extern struct objnodeops *samfs_objnodeops_simmemp;
extern struct objnodeops *samfs_objnodeops_simiopsp;

extern int sam_obj_busy(objnode_t *objnodep, cred_t *cr, caller_context_t *ct);
extern int sam_obj_unbusy(objnode_t *objnodep, cred_t *cr,
    caller_context_t *ct);
extern int sam_obj_rele(objnode_t *objnodep);
extern void sam_obj_rw_enter(objnode_t *objnodep, krw_t enter_type);
extern int sam_obj_rw_tryenter(objnode_t *objnodep, krw_t enter_type);
extern void sam_obj_rw_exit(objnode_t *objnodep);

extern int sam_obj_par_create(objnode_t *pobjp, mode_t mode,
    uint64_t *oidp, objnode_t **objpp, void *cap, void *sec, void *curcmdpg,
    cred_t *credp);
extern int sam_obj_col_create(objnode_t *pobjp, mode_t mode,
    uint64_t *oidp, objnode_t **objpp, void *cap, void *sec, void *curcmdpg,
    cred_t *credp);
extern int sam_obj_create(objnode_t *pobjp, mode_t mode,
    uint64_t *num, uint64_t *buf, void *cap, void *sec, void *curcmdpg,
    cred_t *credp);
extern int sam_obj_remove(struct objnode *objnodep, void *cap, void *sec,
    void *curcmdpg, cred_t *credp);
extern int sam_obj_read(struct objnode *objnodep, uint64_t offset,
    uint64_t len, void *bufp, int segflg, int ioflag, uint64_t *size_read,
    void *cap, void *sec, void *curcmdpg, cred_t *credp);
extern int sam_obj_write(struct objnode *objnodep, uint64_t offset,
    uint64_t len, void *bufp, int segflg, int ioflag, uint64_t *size_written,
    void *cap, void *sec, void *curcmdpg, cred_t *credp);
extern int sam_obj_flush(struct objnode *objnodep, uint64_t offset,
    uint64_t len, uint8_t flush_scope, void *cap, void *sec, void *curcmdpg,
    cred_t *credp);
extern int sam_obj_truncate(struct objnode *objnodep, uint64_t offset,
    void *cap, void *sec, void *curcmdpg, cred_t *credp);
extern int sam_obj_append(struct objnode *objnodep, uint64_t len, void *bufp,
    int segflg, int ioflag, uint64_t *size_written, uint64_t *start_append_addr,
    void *cap, void *sec, void *curcmdpg, cred_t *credp);
extern int sam_obj_punch(struct objnode *objnodep, uint64_t starting_addr,
    uint64_t len, void *cap, void *sec, void *curcmdpg, cred_t *credp);

extern int sam_obj_dummy();

#endif /* _OBJNODE_H */
