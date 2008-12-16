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

// ident	$Id: LibDev.java,v 1.18 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

import com.sun.netstorage.samqfs.mgmt.BaseDev;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class LibDev extends BaseDev {

    private String serialNum, vendor, product, firmware;
    private DriveDev drives[];
    private MdLicense mlicenses[];

    private String devStatus; /* samu status string */
    private String[] altPaths;
    // private int totalDrives; not needed by gui
    private String catalogPath;
    private String[] msgs;
    private StkNetLibParam stkParam;
    private String logPath;
    private long logModTime;

    /* private constructor */
    private LibDev(
        // arguments passed to parent's contructor
        String devPath, int eq, String eqType,
        String fsetName, int eqFSet, int state, String paramPath,
        // LibDev specific arguments
        String serialNum, String vendor, String product,
        String firmware, DriveDev[] drives, MdLicense[] mlicenses,
        String devStatus, String[] altPaths, String catalogPath,
        String[] msgs, StkNetLibParam stkParam,
        String logPath, long logModTime) {

        super(devPath, eq, eqType, fsetName, eqFSet, state, paramPath);
        this.serialNum = serialNum;
        this.vendor    = vendor;
        this.product   = product;
        this.firmware  = firmware;
        this.drives    = drives;
        this.mlicenses = mlicenses;
        this.devStatus = devStatus;
        this.altPaths  = altPaths;
        this.catalogPath = catalogPath;
        this.msgs = msgs;
        this.stkParam = stkParam;
        this.logPath = logPath;
        this.logModTime = logModTime;
    }

    /* private constructor */
    // deprecated after Ron makes changes to the GUI to add stkParam to Library
    private LibDev(
        // arguments passed to parent's contructor
        String devPath, int eq, String eqType,
        String fsetName, int eqFSet, int state, String paramPath,
        // LibDev specific arguments
        String serialNum, String vendor, String product,
        String firmware, DriveDev[] drives, MdLicense[] mlicenses,
        String devStatus, String[] altPaths, String catalogPath,
        String[] msgs) {

        super(devPath, eq, eqType, fsetName, eqFSet, state, paramPath);
        this.serialNum = serialNum;
        this.vendor    = vendor;
        this.product   = product;
        this.firmware  = firmware;
        this.drives    = drives;
        this.mlicenses = mlicenses;
        this.devStatus = devStatus;
        this.altPaths  = altPaths;
        this.catalogPath = catalogPath;
        this.msgs = msgs;
    }
    // public methods

    /* path needs to be selected from altPaths before library can be added */
    public void setPath(String selectedPath) { this.devPath = selectedPath; }

    public void setCatalogPath(String catalogPath) {
	this.catalogPath = catalogPath;
    }

    /* Eq type must be set for library that are not recognized by SAM-FS/QFS */
    public void modifytEqType(String newEqType) { this.eqType = newEqType; }


    public String getSerialNum() { return serialNum; }
    public String getVendor()    { return vendor; }
    public String getProductID() { return product; }
    public String getFirmwareLevel() { return firmware; }

    public DriveDev[] getDrives() { return drives; }
    public void setDrives(DriveDev[] drives) { this.drives = drives; }
    public void setStkParam(StkNetLibParam param) {this.stkParam = param; }

    public MdLicense[] getMediaLicenses() { return mlicenses; }
    public String getStatus() { return devStatus; }
    public String[] getAlternatePaths()   { return altPaths; }
    public String getCatalogPath() { return catalogPath; }
    public String[] getMessages()  { return msgs; }
    public StkNetLibParam getStkParam() { return stkParam; }
    public String getLogPath() { return logPath; }
    public long getLogModTime() { return logModTime; }

    public String toString() {
        int i;
        String s = super.toString() +  " | " +
            serialNum + " " + vendor + " " + product + " " + devStatus;

        if (null != msgs)
            for (i = 0; i < msgs.length; i++)
                s += "\n     msg" + i + ": " + msgs[i];
        if (null != altPaths)
            for (i = 0; i < altPaths.length; i++)
                s += "\n     path" + i + ": " + altPaths[i];
        if (null != mlicenses) {
            s += "\n Media licenses: ";
            for (i = 0; i < mlicenses.length; i++)
                s += "  " + mlicenses[i];
        }
        if (null != drives) {
            s += "\n Library drives[" + drives.length + "]:\n";
            for (i = 0; i < drives.length; i++)
                s += "  "  + drives[i] + "\n";
        }

        if (null != stkParam) {
            s += "\n Stk Param: " + stkParam;
        } else {
            s += "\n Stk Param: NULL";
        }

        s += "\nLog Path: " + logPath;
        s += "\nLog Time: " + logModTime;
        return s;
    }

    // public methods which call native code

    public void add(Ctx c) throws SamFSException {
        Media.addLibrary(c, this);
    }
    public void remove(Ctx c, boolean unload) throws SamFSException {
        Media.removeLibrary(c, this.getEquipOrdinal(), unload);
    }
}
