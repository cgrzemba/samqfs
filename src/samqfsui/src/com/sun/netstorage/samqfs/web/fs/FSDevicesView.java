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

// ident	$Id: FSDevicesView.java,v 1.17 2009/03/04 21:54:41 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.util.LogUtil;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Creates the FSDevices Action Table and provides
 * handlers for the links within the table.
 */

public class FSDevicesView extends CommonTableContainerView {

    // Page children
    private static final String INSTRUCTION = "Instruction";
    private static final String ALL_DEVICES = "AllDevices";
    private static final String SELECTED_DEVICES = "SelectedDevices";
    private static final String NO_SELECTION_MSG = "NoSelectionMsg";
    private static final String DISABLE_MSG = "DisableMsg";
    private static final String NO_MOUNT_MSG = "NoMountMsg";
    private static final String NO_SHARED_CLIENT_MSG = "NoSharedClientMsg";

    private static final String BUTTON_ENABLE = "ButtonEnable";
    private static final String BUTTON_DISABLE = "ButtonDisable";

    // the table model
    private CCActionTableModel model = null;

    /**
     * File System Device View constructor
     * @param parent
     * @param name
     */
    public FSDevicesView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        CHILD_ACTION_TABLE = "FSDevicesTable";

        model = createTableModel();
        registerChildren();
    }

    /**
     * Register page children
     */
    public void registerChildren() {
        super.registerChildren(model);
        registerChild(INSTRUCTION, CCStaticTextField.class);
        registerChild(ALL_DEVICES, CCHiddenField.class);
        registerChild(SELECTED_DEVICES, CCHiddenField.class);
        registerChild(NO_SELECTION_MSG, CCHiddenField.class);
        registerChild(DISABLE_MSG, CCHiddenField.class);
        registerChild(NO_MOUNT_MSG, CCHiddenField.class);
        registerChild(NO_SHARED_CLIENT_MSG, CCHiddenField.class);
    }

    /**
     * Creating page child
     * @param name
     * @return View of the page child
     */
    public View createChild(String name) {
        if (name.equals(INSTRUCTION)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(ALL_DEVICES)
            || name.equals(SELECTED_DEVICES)
            || name.equals(NO_SELECTION_MSG)
            || name.equals(DISABLE_MSG)
            || name.equals(NO_MOUNT_MSG)
            || name.equals(NO_SHARED_CLIENT_MSG)) {
            return new CCHiddenField(this, name, null);
        } else {
            return super.createChild(model, name);
        }
    }

    /**
     * Initialize table model
     * @return FSDevicesModel
     */
    private CCActionTableModel createTableModel() {
        model = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/FSDevicesTable.xml");
        model.setActionValue(BUTTON_ENABLE, "FSDevices.button.enable");
        model.setActionValue(BUTTON_DISABLE, "FSDevices.button.disable");
        model.setActionValue("ColumnName", "FSDevices.heading.name");
        model.setActionValue("ColumnType", "FSDevices.heading.type");
        model.setActionValue("ColumnEQ", "FSDevices.heading.eq");
        model.setActionValue("ColumnAlloc", "FSDevices.heading.alloc");
        model.setActionValue("ColumnCapacity", "FSDevices.heading.usage");
        return model;
    }

    /**
     * Called when page is displayed
     * @param event
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        CCActionTable myTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);

        boolean hasPermission =
            SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.CONFIG);
        TraceUtil.trace3("FSDevice: has permission? " + hasPermission);

        String serverName = getServerName();
        String fsName = getFSName();
        String samfsServerAPIVersion = "1.5";
        FileSystem fs = null;
        boolean shared = false;
        boolean isMDS = false;

        try {
            samfsServerAPIVersion =
                SamUtil.getServerInfo(serverName).getSamfsServerAPIVersion();
            TraceUtil.trace3("API Version: " + samfsServerAPIVersion);

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            fs =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fs == null) {
                TraceUtil.trace1("fs is null, beginDisplay()!");
                throw new SamFSException(null, -1000);
            }

            int fsType = fs.getFSTypeByProduct();

            boolean samqfs = fsType != GenericFileSystem.FS_NONSAMQ;
            int sharedStatus = !samqfs ? -1 : fs.getShareStatus();

            shared = sharedStatus != FileSystem.UNSHARED;
            isMDS =
                samqfs && sharedStatus == FileSystem.SHARED_TYPE_MDS;


            TraceUtil.trace3("samqfs type? " + samqfs);
            TraceUtil.trace3("shared? " + shared);
            TraceUtil.trace3("sharedMDS50? " + isMDS);

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Error in FSDevice beginDisplay!", samEx);
        }

        if (hasPermission && (isMDS || !shared)) {
            // Disable Tooltip
            disableTableSelectionToolTip();

        } else {
            // disable the radio button row selection column
            // if user has no permission to change device
            // states, or user is managing a 4.6 server that changing device
            // states are not available
            model = (CCActionTableModel) myTable.getModel();
            model.setSelectionType(CCActionTableModel.NONE);

            // Disable table buttons as well
            ((CCButton) getChild(BUTTON_ENABLE)).setDisabled(true);
            ((CCButton) getChild(BUTTON_DISABLE)).setDisabled(true);
        }

        try {
            populateData(fs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "FSDevicesViewBean()",
                "Unable to populate device table",
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSDevices.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        ((CCHiddenField) getChild(NO_SELECTION_MSG)).setValue(
            SamUtil.getResourceString("common.noneselected"));
        ((CCHiddenField) getChild(DISABLE_MSG)).setValue(
            SamUtil.getResourceString("FSDevices.disable.confirm"));
    }

    /**
     * Populating page data
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    public void populateData(FileSystem fs)
        throws SamFSException {

        StringBuffer buf = new StringBuffer();

        // Clear table model
        model.clear();

        TraceUtil.trace3("populateData: type: " + fs.getShareStatus());

        boolean isMounted = fs.getState() == FileSystem.MOUNTED;

        // populate javascript error message if file system is unmounted because
        // enable/disable allocation is not allowed
        // Also if file system is located in a shared client, same limitation
        // applies
        ((CCHiddenField) getChild(NO_MOUNT_MSG)).setValue(
            isMounted ?
                "" :
                SamUtil.getResourceString("fs.devices.notmounted"));
         ((CCHiddenField) getChild(NO_SHARED_CLIENT_MSG)).setValue(
             fs.getShareStatus() == FileSystem.SHARED_TYPE_CLIENT ?
                 SamUtil.getResourceString("fs.devices.sharedclient") :
                 "");

        DiskCache [] showDiskCache = fs.getMetadataDevices();
        int entryCounter = 0;

        if (showDiskCache != null) {
            buf.append(
                populateTable(showDiskCache, 0, isMounted, null));
            entryCounter = showDiskCache.length;
        }

        showDiskCache = fs.getDataDevices();
        if (showDiskCache != null) {
            buf.append(populateTable(
                showDiskCache, entryCounter, isMounted, null));
            entryCounter += showDiskCache.length;
        }

        // Must be a stripe group
        StripedGroup[] group = fs.getStripedGroups();
        if (group != null) {
            for (int i = 0; i < group.length; i++) {
                String type = group[i].getName();
                showDiskCache = group[i].getMembers();
                buf.append(populateTable(
                    showDiskCache, entryCounter, isMounted, type));
            }
        }

        // remove trailing comma if buf contains something
        if (buf.length() != 0) {
            buf.deleteCharAt(buf.length() - 1);
        }
        ((CCHiddenField) getChild(ALL_DEVICES)).setValue(buf.toString());
        TraceUtil.trace3("Done populating fs device data.");
    }

    private StringBuffer populateTable(
        DiskCache [] showDiskCache, int tableCounter,
        boolean isMounted, String type) {

        StringBuffer buf = new StringBuffer();

        for (int i = 0; i < showDiskCache.length; i++, tableCounter++) {
            // Skip all devices that are off
            if (showDiskCache[i].getState() == DiskCache.OFF) {
                continue;
            }
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
                        displayType = "FSDevices.devicetype.metadata";
                        break;
                    case DiskCache.MD:
                        displayType = "FSDevices.devicetype.dual";
                        break;
                    case DiskCache.MR:
                        displayType = "FSDevices.devicetype.single";
                        break;
                    case DiskCache.NA:
                    default:
                        displayType = "FSDevices.devicetype.na";
                        break;
                }
                displayType = SamUtil.getResourceString(displayType);
            }
            model.setValue("TextType", displayType);
            model.setValue(
                "TextEQ",
                new Integer(showDiskCache[i].getEquipOrdinal()));
            buf.append(showDiskCache[i].getEquipOrdinal()).append(",");

            model.setValue(
                "ImageAlloc",
                showDiskCache[i].getState() == DiskCache.ON ?
                    Constants.Image.ICON_AVAILABLE :
                    Constants.Image.ICON_BLANK_ONE_PIXEL);

            if (isMounted) {
                long cap = showDiskCache[i].getCapacity();
                int consumed =
                    showDiskCache[i].getConsumedSpacePercentage();
                model.setValue("ImageUsageBar",
                    Constants.Image.USAGE_BAR_DIR + consumed + ".gif");
                model.setValue("capacityText",
                    "(" +
                    new Capacity(cap, SamQFSSystemModel.SIZE_MB) + ")");
                model.setValue("usageText", new Integer(consumed));
            } else {
                model.setValue(
                    "ImageUsageBar", Constants.Image.ICON_BLANK_ONE_PIXEL);
                model.setValue("capacityText", "");
                model.setValue("usageText", new Integer(-1));
            }
        }
        return buf;
    }

    public void handleButtonEnableRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        handleButton(true);
    }

    public void handleButtonDisableRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        handleButton(false);
    }

    /**
     * Called when either the enable/disable allocation button is clicked.
     * The buttons are enabled only if user has permission to change the device
     * state, and if user is managing a 5.0+ server.
     *
     * UFS type file system won't even call this view.
     * @param enable
     */
    private void handleButton(boolean enable) {
        String selectedDevs = (String) getDisplayFieldValue(SELECTED_DEVICES);
        selectedDevs = selectedDevs == null ? "" : selectedDevs;
        if (selectedDevs.length() == 0) {
            // Should not get here
            TraceUtil.trace1("Developer's bug found. selectedDevs is empty!");
            getParentViewBean().forwardTo(getRequestContext());
            return;
        }

        TraceUtil.trace3("handleButton: Selected Devs: " + selectedDevs);

        String [] selectedArray = selectedDevs.split(",");
        selectedArray = selectedArray == null ? new String[0] : selectedArray;
        int [] eqs = new int[selectedArray.length];
        try {
            for (int i = 0; i < eqs.length; i++) {
                eqs[i] = Integer.parseInt(selectedArray[i]);
            }
        } catch (NumberFormatException numEx) {
            // Should not get here
            TraceUtil.trace1("Developer's bug found in handlebutton()!", numEx);
            throw numEx;
        }

        String fsName = getFSName();

        TraceUtil.trace3("Ready to change fs device state for fs " + fsName);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            FileSystem fileSystem =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fileSystem == null) {
                throw new SamFSException(null, -1000);
            }

            LogUtil.info(
                this.getClass(),
                "handleButton",
                "Start " + (enable ? "enabling" : "disabling") +
                    "filesystem devices " + selectedDevs);

            fileSystem.setDeviceState(
                enable ? BaseDevice.ON : BaseDevice.NOALLOC,
                eqs);

            LogUtil.info(
                this.getClass(),
                "handleButton",
                "Done " + (enable ? "enabling" : "disabling") +
                    "filesystem devices " + selectedDevs);
            TraceUtil.trace3(
                "Done changing state to " + (enable ? "enable" : "disable"));

            // set page alert
            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    enable ?
                        "FSDevices.success.enable" :
                        "FSDevices.success.disable",
                    selectedDevs),
                "",
                getServerName());

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleButton()",
                "Failed to change file system device states",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    enable ?
                        "FSDevices.error.enable" :
                        "FSDevices.error.disable",
                    selectedDevs),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        getParentViewBean().forwardTo(getRequestContext());
    }

    private void disableTableSelectionToolTip() {
        CCActionTable myTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);

        // de-select all rows & Disable tooltip of selection boxes
        try {
            for (int i = 0; i < model.getSize(); i++) {
                model.setRowSelected(i, false);
                CCCheckBox myCheckBox =
                (CCCheckBox) myTable.getChild(
                     CCActionTable.CHILD_SELECTION_CHECKBOX + i);
                myCheckBox.setTitle("");
                myCheckBox.setTitleDisabled("");
            }
        } catch (ModelControlException modelEx) {
            TraceUtil.trace1(
                "ModelControlException caught while disabling tooltip!",
                modelEx);
        }
    }

    private String getServerName() {
        return
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

    private String getFSName() {
        return
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
    }
}
