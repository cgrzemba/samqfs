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
#pragma ident   "$Revision: 1.4 $"

#ifndef _scrkd_xml_H
#define	_scrkd_xml_H

#include "scrkd_xml_string.h"
#include "csn/scrkd.h"

int build_heartbeat_xml(sf_prod_info_t *sf, cl_reg_cfg_t *cl, xml_string_t *);

int build_fault_telemetry_xml(sf_prod_info_t *sf, cl_reg_cfg_t *cl,
    xml_string_t *, int, int, char *, char *, char *, int);

int build_product_regstr_xml(sf_prod_info_t *sf, cl_reg_cfg_t *cl,
    xml_string_t *sb);


#endif	/* _scrkd_xml_H */
