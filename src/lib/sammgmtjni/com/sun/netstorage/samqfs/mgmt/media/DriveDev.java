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

// ident	$Id: DriveDev.java,v 1.12 2008/03/17 14:43:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

import com.sun.netstorage.samqfs.mgmt.BaseDev;

public class DriveDev extends BaseDev {

    private String serialNum, vendor, product;
    private String devStatus;
    private String altPaths[];
    private boolean shared;  // added in 4.5
    private String loadedVSN;
    private String firmware;
    private String msgs[]; // messages describing drive's present condition
    private String logPath; // added in 4.6
    private long logModTime; // added in 4.6
    private long loadIdleTime; // added in 4.6
    private long tapeAlertFlags; // added in 4.6


    /* private constructor */
    private DriveDev(
        // arguments passed to parent's contructor
        String devPath, int eq, String eqType,
        String fsetName, int eqFSet, int state, String paramPath,
        // DriveDev specific arguments
        String serialNum, String vendor, String product,
        String devStatus, String[] altPaths, boolean shared, String loadedVSN,
        String firmware, String[] msgs, String logPath, long logModTime,
        long loadIdleTime, long tapeAlertFlags) {

        super(devPath, eq, eqType, fsetName, eqFSet, state, paramPath);
        this.serialNum = serialNum;
        this.vendor    = vendor;
        this.product   = product;
        this.devStatus = devStatus;
        this.altPaths  = altPaths;
        this.shared    = shared;
        this.loadedVSN = loadedVSN;
        this.firmware  = firmware;
        this.msgs = msgs;
        this.logPath    = logPath;
        this.logModTime = logModTime;
        this.loadIdleTime = loadIdleTime;
        this.tapeAlertFlags = tapeAlertFlags;

    }

    // public methods

    /* path needs to be selected from altPaths before library can be added */
    public void setPath(String selectedPath) { this.devPath = selectedPath; }

    /* Eq type must be set for library that are not recognized by SAM-FS/QFS */
    public void modifytEqType(String newEqType) { this.eqType = newEqType; }

    /* share drive */
    public void modifyShared(boolean shared) { this.shared = shared; }

    public String getSerialNum() { return serialNum; }
    public String getVendor() { return vendor; }
    public String getProductID() { return product; }
    public String getStatus() { return devStatus; }
    public String[] getAlternatePaths() { return altPaths; }
    public boolean isShared() { return shared; }
    public String getLoadedVSN() { return loadedVSN; }
    public String getFirmware() { return firmware; }
    public String[] getMessages() { return msgs; }
    public String getLogPath() { return logPath; }
    public long getLogModTime() { return logModTime; }
    public long getLoadIdleTime() { return loadIdleTime; }
    public long getTapeAlertFlags() { return tapeAlertFlags; }

    public String toString() {
        int i;
        String s = super.toString() +  " | " +
            serialNum + " " + vendor + " " + product + " " + devStatus +
	    " " + (shared ? "T" : "F");
        s += "\nLog Path: " + logPath;
        s += "\nLog Time: " + logModTime;
        s += "\nLoad Idle Time: " + loadIdleTime;
        s += "\nTapealert Flags: " + tapeAlertFlags;
        if (null != altPaths)
            for (i = 0; i < altPaths.length; i++)
                s += "\n     path" + i + ": " + altPaths[i];
        s += "\n     loadedVSN:" + ((loadedVSN != null) ? loadedVSN : "-") +
	    " firmware:" + firmware;
        if (null != msgs)
            for (i = 0; i < msgs.length; i++)
                if (msgs[i].length() > 0)
                    s += "\n     msg" + i + ": " + msgs[i];
        return s;
    }
    /* from tapealert.c */
    public static final int CLEAN_NOW = 0x80000;
    public static final int CLEAN_PERIODIC = 0x100000;
    public static final int EXPIRED_CLEANING_MEDIA  = 0x200000;
    public static final int INVALID_CLEANING_MEDIA  = 0x400000;
    public static final int STK_CLEAN_REQUESTED = 0x800000;
}
