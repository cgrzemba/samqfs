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

// ident	$Id: NetAttachLibInfo.java,v 1.9 2008/05/16 18:35:29 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class NetAttachLibInfo {

    private String name;
    private int type;
    private int eq;
    private String catalogPath;
    private String paramFilePath;

    // network attached library types
    // copied from device type info in devinfo.h and devstat.h
    public static final int DT_ROBOT    = 0x1800;
    public static final int DT_TAPE_R   = (DT_ROBOT | 0x0040);

    // GRAU through api interface
    public static final int DT_GRAUACI  = (DT_TAPE_R | 12);
    // STK through api interface
    public static final int DT_STKAPI   = (DT_TAPE_R | 14);
    // STK through api interface
    public static final int DT_IBMATL   = (DT_TAPE_R | 17);
    // Fujitsu LMF api interface
    public static final int DT_LMF = (DT_TAPE_R | 23);
    // Sony through api interface
    public static final int DT_SONYPSC  = (DT_TAPE_R | 26);

    public NetAttachLibInfo(String name, int type, int eq,
        String catalogPath, String paramFilePath) {
        this.name = new String(name);
        this.type = type;
        this.eq = eq;
        this.catalogPath = new String(catalogPath);
        this.paramFilePath = new String(paramFilePath);
    }

    public String getName() { return name; }
    public int getType() { return type; }
    public int getEq() { return eq; }
    public String getCatalogPath() { return catalogPath; }
    public String getParamFilePath() { return paramFilePath; }
}
