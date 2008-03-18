/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#ifndef	CMN_CSD_H
#define	CMN_CSD_H

#pragma ident "$Revision: 1.11 $"


#include <sys/types.h>
#include <zlib.h>
#include <sys/acl.h>

#include "sam/types.h"
#include "sam/fs/ino.h"
#include "src/fs/cmd/dump-restore/csd.h"

#include "mgmt/cmn_csd_types.h"

void
csd_error(
	int,
	int,
	char *, ...
);

int
common_get_csd_header(
	gzFile			gzf,		/* IN  */
	boolean_t	*swapped,		/* OUT */
	boolean_t	*data_possible,		/* OUT */
	csd_hdrx_t	*csd_header		/* OUT */
);

int
common_csd_read_header(
	gzFile			gzf,
	int			csd_version,
	boolean_t		swapped,
	csd_fhdr_t		*hdr);

int
common_csd_read(
	gzFile			gzf,
	char 			*name,
	int			namelen,
	boolean_t		swapped,
	struct sam_perm_inode	*perm_inode);

void
common_csd_read_next(
	gzFile			gzf,
	boolean_t		swap,
	int			csd_version,
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section	**vsnpp,
	char			*link,
	void			*data,
	int			*n_aclp,
	aclent_t		**aclpp);

void
common_skip_file_data(
	gzFile			gzf,
	u_longlong_t		nbytes);

void
common_skip_embedded_file_data(gzFile gzf);

void
common_skip_pad_data(
	gzFile			gzf,
	long			nbytes);

void
common_csd_read_mve(
	gzFile			gzf,
	int			csd_version,
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section	**vsnpp,
	boolean_t		swapped);

u_longlong_t
common_copy_file_data_fr_dump(
	gzFile			gzf,
	u_longlong_t		nbyte,
	char			*name);

u_longlong_t
common_write_embedded_file_data(
	gzFile			gzf,
	char			*name);

/* structure for time stacking */
typedef struct {
	int time_stack_is_full;		/* cannot malloc more; skip dirs */
	struct sam_ioctl_idtime *time_stack;  /* ptr to base of stack */
	int time_stack_size;		/* size of stack (capacity)	*/
	int time_stack_next_free;	/* next free slot in stack	*/
} timestackvars_t;

#define	TIME_STACK_SIZE_INCREMENT 1024	/* expand time_stack by this	*/
					/* many entries when it fills	*/
/* structures for permissions stacking */
typedef struct {
	mode_t	perm;			/* permissions */
	char *path;			/* pointer to path */
} perm_stack_t;

typedef struct {
	int perm_stack_is_full;		/* cannot malloc more; skip dirs */
	int perm_stack_size;		/* size of stack (capacity)	*/
	int perm_stack_next_free;	/* next free slot in stack	*/
	perm_stack_t *perm_stack;	/* pointer to start of stack 	*/
} permstackvars_t;

#define	PERM_STACK_SIZE_INCREMENT 1024	/* expand perm_stack by this	*/

void common_sam_restore_a_file(
	gzFile,
	char *,
	struct sam_perm_inode *,
	struct sam_vsn_section *,
	char *,
	void*,
	int,
	aclent_t *,
	int	dflag,
	int	*dread,
	int	*in_dirfd,
	replace_t	repl,
	timestackvars_t *tvars,
	permstackvars_t *pvars);

void common_pop_permissions_stack(permstackvars_t *);

void common_pop_times_stack(int, timestackvars_t *);

#endif /* CMN_CSD_H */
