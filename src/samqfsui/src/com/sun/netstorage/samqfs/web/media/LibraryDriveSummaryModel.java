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

// ident	$Id: LibraryDriveSummaryModel.java,v 1.23 2008/03/17 14:43:39 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.StkDevice;
import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;

import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;


public final class LibraryDriveSummaryModel extends CCActionTableModel {

    private boolean sharedCapable = false;

    // Constructor
    public LibraryDriveSummaryModel(boolean sharedCapable) {

        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            (sharedCapable ?
                "/jsp/media/LibrarySharedDriveSummaryTable.xml" :
                "/jsp/media/LibraryDriveSummaryTable.xml"));

        this.sharedCapable = sharedCapable;

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        initActionButtons();
        initHeaders();
        initProductName();
        TraceUtil.trace3("Exiting");
    }

    // Initialize action buttons
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue(
            "ChangeStateButton", "LibraryDriveSummary.button.changestate");
        setActionValue("IdleButton", "LibraryDriveSummary.button.idle");
        setActionValue("UnloadButton", "LibraryDriveSummary.button.unload");
        setActionValue("CleanButton", "LibraryDriveSummary.button.clean");

        if (sharedCapable) {
            setActionValue("ActionMenu", "LibraryDriveSummary.option.share");
        }
        TraceUtil.trace3("Exiting");
    }

    // Initialize the action table headers
    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("VSNColumn", "DriveSummary.heading.vsn");
        setActionValue("EQColumn", "DriveSummary.heading.eq");
        setActionValue("VendorColumn", "DriveSummary.heading.vendor");
        setActionValue(
            "ProductIDColumn", "DriveSummary.heading.productID");
        setActionValue("UsageColumn", "DriveSummary.heading.usage");
        setActionValue("StateColumn", "DriveSummary.heading.state");
        setActionValue(
            "SerialNumberColumn", "DriveSummary.heading.serialnumber");
        setActionValue("FirmwareColumn", "DriveSummary.heading.firmware");
        setActionValue("StatusColumn", "DriveSummary.heading.status");
        setActionValue("MessageColumn", "DriveSummary.heading.message");

        if (sharedCapable) {
            setActionValue("SharedColumn", "DriveSummary.heading.shared");
        }
        TraceUtil.trace3("Exiting");
    }

    // Initialize product name for secondary page
    private void initProductName() {
        TraceUtil.trace3("Entering");
        setProductNameAlt("secondaryMasthead.productNameAlt");
        setProductNameSrc("secondaryMasthead.productNameSrc");
        setProductNameHeight(Constants.ProductNameDim.HEIGHT);
        setProductNameWidth(Constants.ProductNameDim.WIDTH);
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows(String serverName, String libraryName)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        // First clean up the tableModel
        clear();

        // Preparing for data
        // if libraryName is null, throw exception (can't find library)
        if (libraryName == null) {
            throw new SamFSException(null, -2502);
        }

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        Drive[] allDrives = null;
        VSN testVSN = null;
        String name = null, vendor = null, productID = null;
        int eq = 0, state = -1, slotNumber = -1;
        long capacity = 0, freeSpace = 0, spaceConsumed = 0;

        Library myLibrary =
            sysModel.getSamQFSSystemMediaManager().
            getLibraryByName(libraryName);
        if (myLibrary == null) {
            throw new SamFSException(null, -2502);
        }

        allDrives = myLibrary.getDrives();
        if (allDrives == null) {
            return;
        }

        for (int i = 0; i < allDrives.length; i++) {
            testVSN   = allDrives[i].getVSN();

            if (testVSN != null) {
                name = testVSN.getVSN();
                capacity  = testVSN.getCapacity();
                freeSpace = testVSN.getAvailableSpace();
                slotNumber = testVSN.getSlotNumber();

                if (capacity != 0) {
                    spaceConsumed = (long) 100 * (capacity - freeSpace)
                        / capacity;
                }
            } else {
                name = "";
                spaceConsumed = -1;
            }

            eq = allDrives[i].getEquipOrdinal();
            vendor    = allDrives[i].getVendor();
            productID = allDrives[i].getProductID();
            state    = allDrives[i].getState();

            // Start populating tableModel
            // append new row
            if (i > 0) {
                appendRow();
            }

            setValue("VSNText", name);
            setValue("EQText", new StringBuffer(allDrives[i].getDevicePath()).
                append(" (").append(eq).append(")").toString());
            setValue("VendorText", vendor);
            setValue("ProductIDText",
                new StringBuffer(vendor).append(" ").
                append(productID).toString());

            if (spaceConsumed < 0 || spaceConsumed > 100) {
                setValue("usageText", new Long(-1));
                setValue("capacityText", "");
                setValue("UsageBarImage", Constants.Image.ICON_BLANK);
            } else {
                setValue("capacityText",
                    new NonSyncStringBuffer("(").append(
                        new Capacity(capacity, SamQFSSystemModel.SIZE_MB)).
                        append(")").toString());
                setValue("usageText", new Long(spaceConsumed));
                setValue("UsageBarImage",
                    new NonSyncStringBuffer(Constants.Image.USAGE_BAR_DIR).
                        append(spaceConsumed).append(".gif").toString());
            }

            setValue("StateText", SamUtil.getStateString(state));
            setValue("VSNHidden", name);
            setValue("EQHidden", new Integer(eq));
            setValue("StateHidden", new Integer(state));

            setValue("FirmwareText", allDrives[i].getFirmwareLevel());
            setValue("SerialNumberText", allDrives[i].getSerialNo());

            // Messages
            String [] messages = allDrives[i].getMessages();

            if (messages == null || messages.length == 0) {
                setValue("MessageText", "");
            } else {
                NonSyncStringBuffer strBuf = new NonSyncStringBuffer();
                for (int j = 0; j < messages.length; j++) {
                    if (j != 0) {
                        strBuf.append("<br>");
                    }
                    strBuf.append(messages[j]);
                }
                setValue("MessageText", strBuf.toString());
            }

            // Status Information
            int [] statusInfoCodes = allDrives[i].getDetailedStatus();

            if (statusInfoCodes == null || statusInfoCodes.length == 0) {
                setValue("StatusText", "");
            } else {
                NonSyncStringBuffer strBuf = new NonSyncStringBuffer();
                for (int j = 0; j < statusInfoCodes.length; j++) {
                    if (j != 0) {
                        strBuf.append("<br>");
                    }

                    if (statusInfoCodes[j] == SamQFSSystemMediaManager.
                        RM_STATUS_AUTO_LIB_ALL_SLOTS_OCCUPIED) {
                        // use 10017b (Drive)
                        strBuf.append(SamUtil.getResourceString(
                            new NonSyncStringBuffer(
                            Integer.toString(statusInfoCodes[j])).
                            append("b").toString()));
                    } else {
                        strBuf.append(SamUtil.getResourceString(
                            Integer.toString(statusInfoCodes[j])));
                    }
                }
                setValue("StatusText", strBuf.toString());
            }

            setValue(
                LibraryDriveSummaryTiledView.CHILD_VSN_HREF,
                new Integer(slotNumber));

            if (sharedCapable) {
                boolean shared =
                    isDriveShared(myLibrary, allDrives[i].getDevicePath());
                setValue("SharedText",
                    shared ? "samqfsui.yes" : "samqfsui.no");
                setValue("SharedHidden",
                    Boolean.toString(shared));
            }
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Return if a drive is marked as shared in the parameter file
     */
    private boolean isDriveShared(Library myLibrary, String drivePath)
        throws SamFSException {
        StkNetLibParam param = myLibrary.getStkNetLibParam();
        // param can be null (no Exception is thrown), so check
        if (param != null) {
            StkDevice [] devices = param.getStkDevice();
            if (devices != null) {
                for (int i = 0; i < devices.length; i++) {
                    if (devices[i].getPathName().equals(drivePath)) {
                        return devices[i].isShared();
                    }
                }
            }
        }

        // if it reaches here, the drive is not found for some reason
        // could be that cataserverd is not running
        throw new SamFSException(null, -2516);
    }
}
