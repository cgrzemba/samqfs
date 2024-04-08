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

// ident	$Id: StgFileInfo.java,v 1.10 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.stg.job;

public class StgFileInfo {

    public String fileName;
    public String size; // bytes
    public String position;
    public String offset;
    public String vsn;
    public String user; // who initiated the staging

    private StgFileInfo(String fileName, String size, String position,
        String offset, String vsn, String user) {
            this.fileName = fileName;
            this.size = size;
            this.position = position;
            this.offset   = offset;
            this.vsn  = vsn;
            this.user = user;
    }

    public String toString() {
        return fileName + "," + size + "," + position + "," + offset + "," +
            vsn + "," + user;
    }
}
