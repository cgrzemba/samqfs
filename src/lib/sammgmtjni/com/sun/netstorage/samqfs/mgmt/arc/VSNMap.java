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

// ident	$Id: VSNMap.java,v 1.9 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class VSNMap {

    private String copyName;
    private String mediaType;
    private String vsnNames[];
    private String poolNames[];

    public VSNMap(String copyName, String mediaType, String[] vsnNames,
        String[] poolNames) {
        this.copyName = copyName;
        this.mediaType = mediaType;
        this.vsnNames = vsnNames;
        this.poolNames = poolNames;
    }

    public boolean isEmpty() {
        if (vsnNames != null)
            if (vsnNames.length != 0)
                return false;
        if (poolNames != null)
            if (poolNames.length != 0)
                return false;
        return true;
    }

    public String getCopyName() { return copyName; }
    public void setCopyName(String copyName) { this.copyName = copyName; }

    public String getMediaType() { return mediaType; }
    public void setMediaType(String mediaType) { this.mediaType = mediaType; }

    public String[] getVSNNames() { return vsnNames; }
    public void setVSNNames(String[] vsnNames) { this.vsnNames = vsnNames; }

    public String[] getPoolNames() { return poolNames; }
    public void setPoolNames(String[] poolNames) { this.poolNames = poolNames; }

    public String toString() {
        int i;
        String s = "VSNMap[" + copyName + "]: " + mediaType + " ";
        if (vsnNames != null)
            for (i = 0; i < vsnNames.length; i++)
                s += vsnNames[i] + " ";
        if (poolNames != null) {
            s += "pools: ";
            for (i = 0; i < poolNames.length; i++)
                s += poolNames[i] + " ";
        }
        return s;
    }

}
