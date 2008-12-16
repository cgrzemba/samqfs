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

// ident	$Id: ReservInfo.java,v 1.9 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class ReservInfo {

    private long resTime;
    private String resCopyName, resOwner, resFS;

    public ReservInfo(long resTime, String resCopyName, String resOwner,
        String resFS) {
            this.resTime = resTime;
            this.resCopyName = resCopyName;
            this.resOwner = resOwner;
            this.resFS = resFS;
    }

    public long getResTime() { return resTime; }
    public String getResCopyName() { return resCopyName; }
    public String getResOwner() { return resOwner; }
    public String getResFS() { return resFS; }
}
