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

// ident	$Id: LibrarySummaryModel.java,v 1.26 2008/03/17 14:43:40 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCSeverity;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alarm.CCAlarmObject;


/**
 * This is the Model Class of Library Summary page
 */

public final class LibrarySummaryModel extends CCActionTableModel {

    // Constructor
    public LibrarySummaryModel(String xmlFile) {

        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(), xmlFile);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        initActionButtons();
        initActionMenu();
        initHeaders();
        initProductName();

        TraceUtil.trace3("Exiting");
    }

    // Initialize action buttons
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("SamQFSWizardAddButton", "LibrarySummary.button1");
        setActionValue("ViewVSNButton", "LibrarySummary.button2");
        setActionValue("ViewDriveButton", "LibrarySummary.button3");
        setActionValue("ImportButton", "LibrarySummary.button4");
        TraceUtil.trace3("Exiting");
    }

    private void initActionMenu() {
        TraceUtil.trace3("Entering");
        setActionValue("ActionMenu", "LibrarySummary.option.changestate");
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("NameColumn", "LibrarySummary.heading.name");
        setActionValue("FaultColumn", "LibrarySummary.heading.fault");
        setActionValue("EQColumn", "LibrarySummary.heading.eq");
        setActionValue("VendorColumn", "LibrarySummary.heading.vendor");
        setActionValue("ProductIDColumn", "LibrarySummary.heading.productID");
        setActionValue("DriverColumn", "LibrarySummary.heading.driver");
        setActionValue("UsageColumn", "LibrarySummary.heading.usage");
        setActionValue("StateColumn", "LibrarySummary.heading.state");
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

    public void initModelRows(String serverName) throws SamFSException {
        TraceUtil.trace3("Entering");

        // clear the model
        clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        CCAlarmObject alarm = null;
        String name = null, vendor = null, productID = null;
        String driver = null;
        String alarmType = null;
        int eq = 0, license_slot = 0, alarms = 0, state = -1;
        Alarm[] allAlarms = null;
        long spaceConsumed = -1, capacity = 0;

        Library allLibrary[] =
            sysModel.getSamQFSSystemMediaManager().getAllLibraries();
        if (allLibrary == null) {
            return;
        }

        for (int i = 0; i < allLibrary.length; i++) {
           // append new row
            if (i > 0) {
                appendRow();
            }

            name = allLibrary[i].getName();
            eq   = allLibrary[i].getEquipOrdinal();
            vendor    = allLibrary[i].getVendor();
            productID = allLibrary[i].getProductID();

            driver = SamUtil.getLibraryDriverString(
                allLibrary[i].getDriverType());

            state    = allLibrary[i].getState();
            allAlarms = allLibrary[i].getAssociatedAlarms();

            int [] alarmInfo = SamUtil.getAlarmInfo(allAlarms);
            alarm = MediaUtil.getAlarm(alarmInfo[0]);

            // add < & > for Historian, and set HISTORIAN as Driver name
            if (name.equalsIgnoreCase(
                Constants.MediaAttributes.HISTORIAN_NAME)) {
                name = SamUtil.getResourceString(
                    "LibrarySummary.string.Historian");
                driver = "";
                spaceConsumed = -1;
            } else if (state != BaseDevice.ON) {
                spaceConsumed = -1;
            } else {
                try {
                    capacity = allLibrary[i].getTotalCapacity();
                } catch (SamFSException ex) {
                    if (ex.getSAMerrno() == 30356) {
                        // unable to get capacity, catserverd not running
                        capacity = 0;
                    } else {
                        throw ex;
                    }
                }
                if (capacity != 0) {
                    spaceConsumed = (long) 100 * (
                        capacity - allLibrary[i].getTotalFreeSpace()) /
                        capacity;
                } else {
                    spaceConsumed = -1;
                }
            }

            setValue("NameText", name);
            setValue("EQText", new Integer(eq));
            setValue("VendorText", vendor);
            setValue("ProductIDText", productID);
            setValue("DriverText", driver);
            setValue("StateText", SamUtil.getStateString(state));
            setValue("NameHidden", name);
            setValue("DriverHidden", driver);
            setValue("StateHidden", new Integer(state));

            // If the name is Historian, do not set any Alarm
            if (name.equals(SamUtil.getResourceString(
                "LibrarySummary.string.Historian"))) {
                setValue("Alarm", new CCAlarmObject(CCSeverity.OK));
            } else {
                setValue("Alarm", alarm);
            }

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

            setValue(LibrarySummaryTiledView.CHILD_DETAIL_HREF, name);
            setValue(LibrarySummaryTiledView.CHILD_ALARM_HREF, name);
        }

        TraceUtil.trace3("Exiting");
    }
}
