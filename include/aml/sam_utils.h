/*
 * sam_utils.h - SAM-FS API internal utility functions
 *
 * Definitions for SAM-FS API internal utility functions.
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

#ifndef _AML_SAM_UTILS_H
#define	_AML_SAM_UTILS_H

#pragma ident "$Revision: 1.11 $"


/* Define maximum lengths for parameters */

#define	MAX_TAPE_VSN_LEN	6
#define	MAX_OPTIC_VSN_LEN	31
#define	MAX_USER_INFO_LEN	127

/* Define internal functions contained in sam_utils.c */

int is_ansi_tp_label(char *s, size_t size);
int sam_get_dev(ushort_t eq_ord, dev_ent_t **dev, char **fifo_path,
	operator_t *operator);
int sam_send_cmd(sam_cmd_fifo_t *cmd_block, int wait_resp, char *fifo_path);

#endif /* _AML_SAM_UTILS_H */
