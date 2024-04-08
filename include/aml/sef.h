/*
 * sef.h - include file for system engineering file
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

#ifndef _AML_SEF_H
#define	_AML_SEF_H

#pragma   ident  "@(#)$Revision: 1.14 $"

typedef enum {
	SEF_UNLOAD,
	SEF_SAMPLE
} sef_where_t;

/* Function prototypes */
boolean_t sef_status(void);
void sef_init(dev_ent_t *un, int fd);
int sef_data(dev_ent_t *un, int fd);
void get_supports_sef(dev_ent_t *un, int fd);
int sef_data_sample(int fd, dev_ent_t *un, sef_where_t where);

#endif /* _AML_SEF_H */
