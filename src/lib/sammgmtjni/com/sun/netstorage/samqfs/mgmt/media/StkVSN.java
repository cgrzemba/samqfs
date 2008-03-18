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

// ident	$Id: StkVSN.java,v 1.7 2008/03/17 14:43:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class StkVSN {

    private int acsNum, lsmNum, panelNum, rowID, colID, poolID;
    private String name;
    private String status;
    private String mediaType;
    private String usageType; // data, cleaning


    /**
     * constructor
     *
     * The constructor is made public due to the use in the simulator.
     * Real mode presentation/logic layer code never uses this constructor.
     */
    public StkVSN(String name, int acsNum, int lsmNum, int panelNum,
            int rowID, int colID, int poolID,
            String status, String mediaType,
            String usageType) {
        this.name = name;
        this.acsNum = acsNum;
        this.lsmNum = lsmNum;
        this.panelNum = panelNum;
        this.rowID = rowID;
        this.colID = colID;
        this.poolID = poolID;
        this.mediaType = mediaType;
        this.usageType = usageType;
        this.status = status;

    }

    public String getName() { return name; }
    public int getAcsNum() { return acsNum; }
    public int getLsmNum() { return lsmNum; }
    public int getPanelNum() { return panelNum; }
    public int getRowID() { return rowID; }
    public int getColID() { return colID; }
    public int getPoolID() { return poolID; }
    public String getMediaType() { return mediaType; }
    public String getUsageType() { return usageType; }
    public String getStatus() { return status; }

    public String toString() {
        String s = "name = " + name +
                ", acsNum = " + acsNum +
                ", lsmNum = " + lsmNum +
                ", panelNum = " + panelNum +
                ", rowID = " + rowID +
                ", colID = " + colID +
                ", poolID = " + poolID +
                ", mediaType = " + mediaType +
                ", usageType = " + usageType +
                ", status = " + status;
        return s;
    }
}
