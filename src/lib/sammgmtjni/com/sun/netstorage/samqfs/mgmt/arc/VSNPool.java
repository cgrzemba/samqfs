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

// ident	$Id: VSNPool.java,v 1.9 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class VSNPool {

    private String name;
    private String mediaType;
    private String vsnNames[];

    public VSNPool(String name, String mediaType, String vsnNames[]) {
            this.name = name;
            this.mediaType = mediaType;
            this.vsnNames = vsnNames;
    }

    public String getName() { return name; }
    public String getMediaType() { return mediaType; }

    public String[] getVSNs() { return vsnNames; }
    public void setVSNs(String vsnNames[]) { this.vsnNames = vsnNames; }
    // no setters for the other fields

    public String toString() {
        String s = "VSNPool[" + name + "]: " + mediaType;
        if (vsnNames != null)
            for (int i = 0; i < vsnNames.length; i++)
                s += " " + vsnNames[i];
        return s;
    }
}
