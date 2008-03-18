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

// ident	$Id: FSDevicesView.java,v 1.10 2008/03/17 14:43:33 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;

/**
 * Creates the FSDevices Action Table and provides
 * handlers for the links within the table.
 */

public class FSDevicesView extends CommonTableContainerView {

    // Page Title Attributes and Components.
    protected CCPageTitleModel pageTitleModel = null;
    protected String fsName;
    protected String serverName;

    // the table model
    private CCActionTableModel model = null;

    public FSDevicesView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "FSDevicesTable";
        ViewBean vb = getParentViewBean();
        serverName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        fsName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        TraceUtil.trace2("Got serverName and fsName from page session: "
            + serverName + ", " + fsName);

        pageTitleModel = createPageTitleModel();
        model = createTableModel();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        PageTitleUtil.registerChildren(this, pageTitleModel);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        }
        TraceUtil.trace3("Exiting");
        return super.createChild(model, name);
    }

    /**
     * Create PageTitle Model
     * @return PageTitleModel
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        CCPageTitleModel model =
            PageTitleUtil.createModel("/jsp/fs/FSDevicesPageTitle.xml");
        model.setPageTitleText(fsName + " Devices");
        TraceUtil.trace3("Exiting");
        return model;
    }

    /**
     * Initialize table model
     * @return FSDevicesModel
     */
    private CCActionTableModel createTableModel() {
        TraceUtil.trace3("Entering");

        model = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/FSDevicesTable.xml");
        model.setActionValue("ColumnName", "FSDevices.heading1");
        model.setActionValue("ColumnType", "FSDevices.heading2");
        model.setActionValue("ColumnInode", "FSDevices.heading3");
        model.setActionValue("ColumnEQ", "FSDevices.heading4");
        model.setActionValue("ColumnCapacity", "FSDevices.heading5");
        model.setActionValue("ColumnFreeSpace", "FSDevices.heading6");
        model.setActionValue("ColumnConsumed", "FSDevices.heading7");

        TraceUtil.trace3("Exiting");
        return model;
    }

    /**
     * beginDisplay start to display the page
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("FSDevices.pageTitle1", fsName));

        TraceUtil.trace3("Exiting");
    }

    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");

        // Clear table model
        model.clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        FileSystem fs =
            sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        boolean isMounted = fs.getState() == FileSystem.MOUNTED;

        DiskCache [] showDiskCache = fs.getMetadataDevices();
        int entryCounter = 0;

        if (showDiskCache != null) {
            populateTable(showDiskCache, 0, isMounted, false, null);
            entryCounter = showDiskCache.length;
        }

        showDiskCache = fs.getDataDevices();
        if (showDiskCache != null) {
            populateTable(showDiskCache, entryCounter, isMounted, true, null);
            entryCounter += showDiskCache.length;
        }

        // Must be a stripe group
        StripedGroup[] group = fs.getStripedGroups();
        if (group != null) {
            for (int i = 0; i < group.length; i++) {
                String type = group[i].getName();
                showDiskCache = group[i].getMembers();
                populateTable(
                    showDiskCache, entryCounter, isMounted, true, type);
            }
        }

        TraceUtil.trace3("Exiting");
    }

    private void populateTable(
        DiskCache [] showDiskCache, int tableCounter,
        boolean isMounted, boolean showBlankInode, String type) {

        for (int i = 0; i < showDiskCache.length; i++, tableCounter++) {
            if (tableCounter > 0) {
                model.appendRow();
            }

            model.setValue("TextName", showDiskCache[i].getDevicePath());

            // type variable is not null if group devices are being populated
            // otherwise, check diskCacheType and determine the type
            String displayType = type;
            if (displayType == null) {
                int deviceCache = showDiskCache[i].getDiskCacheType();
                switch (deviceCache) {
                    case DiskCache.METADATA:
                        displayType = "FSDevices.devicetype1";
                        break;
                    case DiskCache.MD:
                        displayType = "FSDevices.devicetype2";
                        break;
                    case DiskCache.MR:
                        displayType = "FSDevices.devicetype3";
                        break;
                    case DiskCache.NA:
                    default:
                        displayType = "FSDevices.devicetype4";
                        break;
                }
                displayType = SamUtil.getResourceString(displayType);
            }
            model.setValue("TextType", displayType);

            int inodesRemaining = showDiskCache[i].getNoOfInodesRemaining();
            if (showBlankInode || inodesRemaining == -1) {
                model.setValue("TextInode", "");
            } else {
                model.setValue("TextInode", new Integer(inodesRemaining));
            }

            model.setValue(
                "TextEQ",
                new Integer(showDiskCache[i].getEquipOrdinal()));

            if (isMounted) {
                long cap = showDiskCache[i].getCapacity();
                long free = showDiskCache[i].getAvailableSpace();
                int consumed =
                    showDiskCache[i].getConsumedSpacePercentage();
                model.setValue("TextCapacity",
                    new Capacity(cap, SamQFSSystemModel.SIZE_MB));
                model.setValue("TextFreeSpace",
                    new Capacity(free, SamQFSSystemModel.SIZE_MB));
                model.setValue("TextConsumed", new Integer(consumed));
            } else {
                model.setValue("TextCapacity", "");
                model.setValue("TextFreeSpace", "");
                model.setValue("TextConsumed", "");
            }
        }
    }
}
