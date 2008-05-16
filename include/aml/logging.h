/*
 * sam_logging.h - SAM-FS logging information
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

#if !defined(_AML_LOGGING_H)
#define	_AML_LOGGING_H

#pragma ident "$Revision: 1.14 $"

#include "sam/fioctl.h"
#include "sam/syscall.h"
#include "aml/fifo.h"

/* function prototypes */

void logioctl(int, char, void *);

typedef struct {
	struct timeval	time;
	sam_fs_fifo_t	fifo_cmd;
} fifo_log_t;

typedef struct {
	struct timeval	time;
	int	ioctl_type;
	char	ioctl_system;
	union {
		struct {
			sam_handle_t handle;	/* Handle struct */
			int	ret_err;	/* Errno from mount request */
			offset_t space;		/* Space on device */
			dev_t	rdev;		/* Raw device */
			void	*mt_handle;
			sam_resource_t resource;	/* Resource record */
			char	fifo_name[32];
		} fsmount;
#if defined(USE_IOCTL_INTERFACE)
		sam_ioctl_fsunload_t	fsunload;
		sam_ioctl_error_t	fserror;
		sam_ioctl_fsinval_t	fsinval;
		sam_ioctl_stage_t	fsstage;
		sam_ioctl_fsbeof_t	fsbeof;
#endif /* (USE_IOCTL_INTERFACE) */
		sam_ioctl_swrite_t	swrite;
		sam_ioctl_stsize_t	stsize;
		sam_fsstage_arg_t	sc_fsstage;
		sam_fsmount_arg_t	sc_fsmount;
		sam_fsbeof_arg_t	sc_fsbeof;
		sam_fsunload_arg_t	sc_fsunload;
		sam_fserror_arg_t	sc_fserror;
		sam_fsinval_arg_t	sc_fsinval;
		sam_fsiocount_arg_t	sc_fsiocount;
		sam_position_arg_t	sc_position;
	} ioctl_data;
} ioctl_log_t;

typedef struct {
	int	flags;
	int	first;
	int	in;
	int	out;
	int	limit;
	mutex_t	in_mutex;
} ioctl_fet_t;

typedef struct {
	int	flags;
	int	first;
	int	in;
	int	out;
	int	limit;
} fifo_fet_t;

#if defined(DEC_INIT) && !defined(lint)
ioctl_fet_t  *ioctl_fet = (ioctl_fet_t *)NULL;
#endif   /* DEC_INIT */

extern ioctl_fet_t *ioctl_fet;

#define	FS_FIFO_LOG	SAM_AMLD_HOME"/fs_fifo_log"
#define	FS_IOCTL_LOG	SAM_AMLD_HOME"/fs_ioctl_log"

#endif /* !defined(_AML_LOGGING_H) */
