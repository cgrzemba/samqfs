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

// ident	$Id: FSMountView.java,v 1.38 2008/06/11 16:58:00 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.propertysheet.CCPropertySheet;
import java.io.IOException;
import java.util.Properties;
import javax.servlet.ServletException;
import java.util.Enumeration;

/**
 * Creates the FSDevices Action Table and provides
 * handlers for the links within the table.
 */

public class FSMountView extends RequestHandlingViewBase {

    private static final String CHILD_PROPERTY_SHEET = "PropertySheet";
    private CCPropertySheetModel propertySheetModel  = null;
    private String UNS_MOUNT_OPTIONS = "unsupportedMountOptions";

    // is direct io set to on?
    private boolean directIO = false;

    private String serverName, fsName, pageType, sharedType;
    private Integer archive;

    public FSMountView(
        View parent, String name,
        String serverName, String fsName,
        Integer archive, String pageType, String sharedType) {
        super(parent, name);
        TraceUtil.trace3("Entering");

        this.serverName = serverName;
        this.fsName = fsName;
        this.archive = archive;
        this.pageType = pageType;
        this.sharedType = sharedType;

        createPropertySheetModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        registerChild(CHILD_PROPERTY_SHEET, CCPropertySheet.class);
        propertySheetModel.registerChildren(this);
    }

    public View createChild(String name) {
        // Propertysheet child
        if (name.equals(CHILD_PROPERTY_SHEET)) {
            return new CCPropertySheet(this, propertySheetModel, name);
            // Create child from propertysheet model.
        } else if (propertySheetModel.isChildSupported(name)) {
            return propertySheetModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Create propertysheet model
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace2("PageType = " + getPageType());

        if (propertySheetModel == null)  {
            String psXml = "";
            if (getPageType().equals(FSMountViewBean.TYPE_SHAREDQFS)) {
                psXml = "/jsp/fs/FSMountSharedQfsPropertySheet.xml";
            } else if (
                getPageType().equals(FSMountViewBean.TYPE_SHAREDSAMQFS)) {
                psXml = "/jsp/fs/FSMountSharedSamqfsPropertySheet.xml";
            } else if (
                getPageType().equals(FSMountViewBean.TYPE_UNSHAREDQFS)) {
                psXml = "/jsp/fs/FSMountUnsharedQfsPropertySheet.xml";
            } else if (
                getPageType().equals(FSMountViewBean.TYPE_UNSHAREDSAMQFS)) {
                psXml = "/jsp/fs/FSMountUnsharedSamqfsPropertySheet.xml";
            } else {
                psXml = "/jsp/fs/FSMountUnsharedSamfsPropertySheet.xml";
            }

            propertySheetModel = PropertySheetUtil.createModel(psXml);
        }

        return propertySheetModel;
    }

    /**
     * Load the data for propertysheet model
     */
    public void loadPropertySheetModel() throws SamFSException {
        propertySheetModel.clear();
        String sharedMetaClient = (String)
        getParentViewBean().getPageSessionAttribute(
            Constants.SessionAttributes.SHARED_METADATA_CLIENT);
        SamQFSSystemModel model = null;
        if (sharedMetaClient != null) {
            model = SamUtil.getModel(sharedMetaClient);
        } else {
            model = SamUtil.getModel(serverName);
        }
        FileSystem fs =
            model.getSamQFSSystemFSManager().getFileSystem(fsName);
        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        FileSystemMountProperties properties = fs.getMountProperties();
        int mountState = fs.getState();
        loadBasicProperties(properties);
        loadGeneralProperties(properties, mountState);
        loadPerformanceProperties(properties);
        loadIODiscoveryProperties(properties);
        loadUnsupportedMountOptions(fs);

        // Unshared QFS has only the above four properties sections.
        // So, check for all other types and fill in the properties.
        if (isArchive()) {
            loadArchiveProperties(properties);
        } else if (getPageType().equals(FSMountViewBean.TYPE_SHAREDQFS)) {
            loadSharedQFSProperties(properties, mountState);
        } else if (getPageType().equals(FSMountViewBean.TYPE_SHAREDSAMQFS)) {
            loadArchiveProperties(properties);
            loadSharedQFSProperties(properties, mountState);
        }

        // if force direct io is set to on, disable io discovery properties
        if (directIO) {
            ((CCTextField) getChild("consecreadValue")).setDisabled(true);
            ((CCTextField) getChild("wellreadminValue")).setDisabled(true);
            ((CCDropDownMenu) getChild("wellreadminUnit")).setDisabled(true);
            ((CCTextField) getChild("misreadminValue")).setDisabled(true);
            ((CCDropDownMenu) getChild("misreadminUnit")).setDisabled(true);
            ((CCTextField) getChild("consecwriteValue")).setDisabled(true);
            ((CCTextField) getChild("wellwriteminValue")).setDisabled(true);
            ((CCDropDownMenu) getChild("wellwriteminUnit")).setDisabled(true);
            ((CCTextField) getChild("miswriteminValue")).setDisabled(true);
            ((CCDropDownMenu) getChild("miswriteminUnit")).setDisabled(true);
            ((CCRadioButton) getChild("dioszeroValue")).setDisabled(true);
        }
    }

    /**
     * Populate the Basic Properties Section
     */
    private void loadBasicProperties(FileSystemMountProperties properties) {
        if (getPageType().equals(FSMountViewBean.TYPE_SHAREDSAMQFS) ||
            getPageType().equals(FSMountViewBean.TYPE_UNSHAREDSAMQFS) ||
            getPageType().equals(FSMountViewBean.TYPE_UNSHAREDSAMFS)) {
            int hwmValue = properties.getHWM();
            propertySheetModel.setValue("hwmValue",
                Integer.toString(hwmValue));
            int lwmValue = properties.getLWM();
            propertySheetModel.setValue("lwmValue",
                Integer.toString(lwmValue));
        }
        int stripeValue = properties.getStripeWidth();
        String stripeValStr = "";
        if (stripeValue != -1) {
            stripeValStr = Integer.toString(stripeValue);
        }
        propertySheetModel.setValue("stripeValue", stripeValStr);
        propertySheetModel.setValue(
            "traceValue", properties.isTrace() ? "yes" : "no");

        if (getSharedClientType().equals("Shared Client")) {
            ((CCTextField)getChild("stripeValue")).setDisabled(true);
            ((CCRadioButton)getChild("traceValue")).setDisabled(true);
        }
    }

    /**
     * Populate the General Properties Section
     */
    private void loadGeneralProperties(
        FileSystemMountProperties properties, int mountState) {

        propertySheetModel.setValue(
            "mountreadonlyValue", properties.isReadOnlyMount() ? "yes" : "no");

        if (mountState == FileSystem.MOUNTED) {
            ((CCRadioButton)getChild("mountreadonlyValue")).setDisabled(true);
        }

        propertySheetModel.setValue(
            "nouidValue", properties.isNoSetUID() ? "yes" : "no");

        // qwrite is supported by ALL fs types
        propertySheetModel.setValue(
            "quickwriteValue",
            properties.isQuickWrite() ? "yes" : "no");

        if (getSharedClientType().equals("Shared Client")) {
            ((CCRadioButton)getChild("nouidValue")).setDisabled(true);
        }
    }

    /**
     * Populate the Performance Properties Section
     */
    private void loadPerformanceProperties(
        FileSystemMountProperties properties) {

        long readaheadValue = properties.getReadAhead();
        propertySheetModel.setValue("readaheadValue",
            Long.toString(readaheadValue));
        int readaHeadUnitValue = properties.getReadAheadUnit();
        propertySheetModel.setValue("readaheadUnit",
            SamUtil.getSizeUnitString(readaHeadUnitValue));

        long writebehindValue = properties.getWriteBehind();
        propertySheetModel.setValue("writebehindValue",
            Long.toString(writebehindValue));
        int writeBehindUnitValue = properties.getWriteBehindUnit();
        propertySheetModel.setValue("writebehindUnit",
            SamUtil.getSizeUnitString(writeBehindUnitValue));

        long writethroValue = properties.getWriteThrottle();
        propertySheetModel.setValue("writethroValue",
            Long.toString(writethroValue));
        int writeThroUnitValue = properties.getWriteThrottleUnit();
        propertySheetModel.setValue("writethroUnit",
            SamUtil.getSizeUnitString(writeThroUnitValue));

        long flushbehindValue = properties.getFlushBehind();
        propertySheetModel.setValue("flushbehindValue",
            Long.toString(flushbehindValue));
        int flushBehindUnitValue = properties.getFlushBehindUnit();
        propertySheetModel.setValue("flushbehindUnit",
            SamUtil.getSizeUnitString(flushBehindUnitValue));

        if (!(getPageType().equals(FSMountViewBean.TYPE_UNSHAREDQFS) ||
            getPageType().equals(FSMountViewBean.TYPE_SHAREDQFS))) {
            long stageflushValue = properties.getStageFlushBehind();
            propertySheetModel.setValue("stageflushValue",
                Long.toString(stageflushValue));
            int stageFlushUnitValue = properties.getStageFlushBehindUnit();
            propertySheetModel.setValue("stageflushUnit",
                SamUtil.getSizeUnitString(stageFlushUnitValue));
        }

        propertySheetModel.setValue(
            "softwareValue", properties.isSoftRAID() ? "on" : "off");

        directIO = properties.isForceDirectIO();
        propertySheetModel.setValue("forceValue", directIO ? "on" : "off");

        // force_nfs_async is for all fs
        propertySheetModel.setValue(
            "forceNFSAsyncValue",
            properties.isForceNFSAsync() ? "on" : "off");

        if (getSharedClientType().equals("Shared Client")) {
            ((CCTextField)getChild("readaheadValue")).setDisabled(true);
            ((CCTextField)getChild("writebehindValue")).setDisabled(true);
            ((CCTextField)getChild("writethroValue")).setDisabled(true);
            ((CCTextField)getChild("flushbehindValue")).setDisabled(true);
            ((CCRadioButton)getChild("softwareValue")).setDisabled(true);
            ((CCRadioButton)getChild("forceValue")).setDisabled(true);
        }
    }

    /**
     * Populate the IO Discovery Properties Section
     */
    private void loadIODiscoveryProperties(
        FileSystemMountProperties properties) {

        int consecReadValue = properties.getConsecutiveReads();
        propertySheetModel.setValue("consecreadValue",
            Integer.toString(consecReadValue));

        long wellreadMinValue = properties.getWellAlignedReadMin();
        propertySheetModel.setValue("wellreadminValue",
            Long.toString(wellreadMinValue));
        int wellreadMinUnitValue = properties.getWellAlignedReadMinUnit();
        propertySheetModel.setValue("wellreadminUnit",
            SamUtil.getSizeUnitString(wellreadMinUnitValue));

        long misreadMinValue = properties.getMisAlignedReadMin();
        propertySheetModel.setValue("misreadminValue",
            Long.toString(misreadMinValue));
        int misreadMinUnitValue = properties.getMisAlignedReadMinUnit();
        propertySheetModel.setValue("misreadminUnit",
            SamUtil.getSizeUnitString(misreadMinUnitValue));

        int consecWriteValue = properties.getConsecutiveWrites();
        propertySheetModel.setValue("consecwriteValue",
            Integer.toString(consecWriteValue));

        long wellwriteMinValue = properties.getWellAlignedWriteMin();
        propertySheetModel.setValue("wellwriteminValue",
            Long.toString(wellwriteMinValue));
        int wellwriteMinUnitValue = properties.getWellAlignedWriteMinUnit();
        propertySheetModel.setValue("wellwriteminUnit",
            SamUtil.getSizeUnitString(wellwriteMinUnitValue));

        long miswriteMinValue = properties.getMisAlignedWriteMin();
        propertySheetModel.setValue("miswriteminValue",
            Long.toString(miswriteMinValue));
        int miswriteMinUnitValue = properties.getMisAlignedWriteMinUnit();
        propertySheetModel.setValue("miswriteminUnit",
            SamUtil.getSizeUnitString(miswriteMinUnitValue));

        propertySheetModel.setValue(
            "dioszeroValue", properties.isDirectIOZeroing() ? "yes" : "no");

        if (getSharedClientType().equals("Shared Client")) {
            ((CCTextField)   getChild("consecreadValue")).setDisabled(true);
            ((CCTextField)   getChild("wellreadminValue")).setDisabled(true);
            ((CCTextField)   getChild("misreadminValue")).setDisabled(true);
            ((CCTextField)   getChild("consecwriteValue")).setDisabled(true);
            ((CCTextField)   getChild("wellwriteminValue")).setDisabled(true);
            ((CCTextField)   getChild("miswriteminValue")).setDisabled(true);
            ((CCRadioButton) getChild("dioszeroValue")).setDisabled(true);
        }
    }

    /**
     * Populate the Archive Properties Section
     */
    private void loadArchiveProperties(FileSystemMountProperties properties) {
        int partReleaseValue = properties.getDefaultPartialReleaseSize();
        propertySheetModel.setValue("partreleaseValue",
            Integer.toString(partReleaseValue));
        int partRelUnitValue = properties.getDefaultPartialReleaseSizeUnit();
        propertySheetModel.setValue("partreleaseUnit",
            SamUtil.getSizeUnitString(partRelUnitValue));

        int maxPartValue = properties.getDefaultMaxPartialReleaseSize();
        propertySheetModel.setValue("maxpartValue",
            Integer.toString(maxPartValue));
        int maxPartUnitValue = properties.getDefaultMaxPartialReleaseSizeUnit();
        propertySheetModel.setValue("maxpartUnit",
            SamUtil.getSizeUnitString(maxPartUnitValue));

        long partStageValue = properties.getPartialStageSize();
        propertySheetModel.setValue("partstageValue",
            Long.toString(partStageValue));
        int partStageUnitValue = properties.getPartialStageSizeUnit();
        propertySheetModel.setValue("partstageUnit",
            SamUtil.getSizeUnitString(partStageUnitValue));

        int stageRetriesValue = properties.getNoOfStageRetries();
        propertySheetModel.setValue("stageretriesValue",
            Integer.toString(stageRetriesValue));

        long stageWindowValue = properties.getStageWindowSize();
        propertySheetModel.setValue("stagewindowValue",
            Long.toString(stageWindowValue));
        int stageWindowUnitSize = properties.getStageWindowSizeUnit();
        propertySheetModel.setValue("stagewindowUnit",
            SamUtil.getSizeUnitString(stageWindowUnitSize));

        propertySheetModel.setValue(
            "questionValue", properties.isArchiverAutoRun() ? "yes" : "no");

        // for QFS license, disable all archive related properties
        String hostName = serverName;

        if (SamUtil.getSystemType(hostName) == SamQFSSystemModel.QFS) {
            ((CCTextField) getChild("partreleaseValue")).setDisabled(true);
            ((CCDropDownMenu)getChild("partreleaseUnit")).setDisabled(true);
            ((CCTextField) getChild("maxpartValue")).setDisabled(true);
            ((CCDropDownMenu) getChild("maxpartUnit")).setDisabled(true);
            ((CCTextField) getChild("partstageValue")).setDisabled(true);
            ((CCDropDownMenu) getChild("partstageUnit")).setDisabled(true);
            ((CCTextField) getChild("stageretriesValue")).setDisabled(true);
            ((CCTextField) getChild("stagewindowValue")).setDisabled(true);
            ((CCDropDownMenu)getChild("stagewindowUnit")).setDisabled(true);
            ((CCRadioButton) getChild("questionValue")).setDisabled(true);
        }
    }

    /**
     * Populate the Shared QFS Properties Section
     */
    private void loadSharedQFSProperties(
        FileSystemMountProperties properties, int mountState) {

        propertySheetModel.setValue(
            "mountBackgroundValue",
            properties.isMountInBackground() ? "yes" : "no");

        int mountretriesValue = properties.getNoOfMountRetries();
        propertySheetModel.setValue("mountretriesValue",
            Integer.toString(mountretriesValue));

        int metarefreshValue = properties.getMetadataRefreshRate();
        propertySheetModel.setValue("metarefreshValue",
            Integer.toString(metarefreshValue));

        String minBlock = "";
        long minblockValue = properties.getMinBlockAllocation();
        if (minblockValue != -1) {
            minBlock = Long.toString(minblockValue);
        }
        propertySheetModel.setValue("minblockValue", minBlock);
        int minblockUnit = properties.getMinBlockAllocationUnit();
        propertySheetModel.setValue("minblockUnit",
            SamUtil.getSizeUnitString(minblockUnit));

        String maxBlock = "";
        long maxblockValue = properties.getMaxBlockAllocation();
        if (maxblockValue != -1) {
            maxBlock = Long.toString(maxblockValue);
        }
        propertySheetModel.setValue("maxblockValue", maxBlock);
        int maxblockUnit = properties.getMaxBlockAllocationUnit();
        propertySheetModel.setValue("maxblockUnit",
            SamUtil.getSizeUnitString(maxblockUnit));

        int readleaseValue = properties.getReadLeaseDuration();
        propertySheetModel.setValue("readleaseValue",
            Integer.toString(readleaseValue));

        int writeleaseValue = properties.getWriteLeaseDuration();
        propertySheetModel.setValue("writeleaseValue",
            Integer.toString(writeleaseValue));

        int appendleaseValue = properties.getAppendLeaseDuration();
        propertySheetModel.setValue("appendleaseValue",
            Integer.toString(appendleaseValue));

        // Maximum Concurrent Streams flag is not supported since v4.6.
        // This field will be replaced by Minimum Number of Threads
        propertySheetModel.setVisible("prop_maxstream", false);
        int minpoolValue = properties.getMinPool();
        propertySheetModel.setValue("minPoolValue",
            Integer.toString(minpoolValue));

        propertySheetModel.setValue(
            "multihostValue", properties.isMultiHostWrite() ? "yes" : "no");

        propertySheetModel.setValue(
            "syncValue", properties.isSynchronizedMetadata() ? "yes" : "no");

        propertySheetModel.setValue(
            "cattrValue",
            properties.isConsistencyChecking() ? "yes" : "no");

        int metaStripeValue = properties.getMetadataStripeWidth();
        propertySheetModel.setValue("metastripeValue",
            Integer.toString(metaStripeValue));

        int leaseTimeoValue = properties.getLeaseTimeout();
        propertySheetModel.setValue("leaseTimeoValue",
            Integer.toString(leaseTimeoValue));

        if (getSharedClientType().equals("Shared Client")) {
            ((CCRadioButton)getChild("mountBackgroundValue")).setDisabled(true);
            ((CCTextField)getChild("mountretriesValue")).setDisabled(true);
            ((CCTextField)getChild("metarefreshValue")).setDisabled(true);
            ((CCTextField)getChild("minblockValue")).setDisabled(true);
            ((CCTextField)getChild("maxblockValue")).setDisabled(true);
            ((CCTextField)getChild("readleaseValue")).setDisabled(true);
            ((CCTextField)getChild("writeleaseValue")).setDisabled(true);
            ((CCTextField)getChild("appendleaseValue")).setDisabled(true);
            ((CCTextField)getChild("maxstreamValue")).setDisabled(true);
            ((CCRadioButton)getChild("multihostValue")).setDisabled(true);
            ((CCRadioButton)getChild("syncValue")).setDisabled(true);
            ((CCRadioButton)getChild("cattrValue")).setDisabled(true);
            ((CCTextField)getChild("metastripeValue")).setDisabled(true);
            ((CCTextField)getChild("leaseTimeoValue")).setDisabled(true);
        }

        // for mounted fs, disable the following options
        if (mountState == FileSystem.MOUNTED) {
            ((CCRadioButton)getChild("mountBackgroundValue")).setDisabled(true);
            ((CCTextField)getChild("mountretriesValue")).setDisabled(true);
            ((CCTextField)getChild("maxstreamValue")).setDisabled(true);
        }
    }

    private void loadUnsupportedMountOptions(FileSystem fs)
        throws SamFSException {

        String options = fs.getMountProperties().getUnsupportedMountOptions();
        // if no unsupported mount options set, hid the section and return
        if (options == null || options.length() == 0) {
            propertySheetModel.setVisible("unsupported", false);
            return;
        }

        Properties props = ConversionUtil.strToProps(options);
        Enumeration e = props.propertyNames();

        // construct the display table
        StringBuffer buf = new StringBuffer();
        buf.append("<table cellspacing=\"10\" <tr>");
        for (int counter = 0; e.hasMoreElements(); counter++) {
            if ((counter != 0) && ((counter % 4) == 0)) {
                buf.append("</tr><tr>");
            }

            String name = (String)e.nextElement();
            String value = props.getProperty(name);
            buf.append("<td><span class=\"LblLev2Txt\">")
            .append(name).append(": </span>")
            .append(value)
            .append("</td>");
        }
        // close the table
        buf.append("</tr></table>");

        // set the value
        CCStaticTextField field =
            (CCStaticTextField)getChild(UNS_MOUNT_OPTIONS);
        field.setEscape(false);
        field.setValue(buf.toString());
    }

    /**
     * Handle request for Button 'Save'
     */
    public void handleSaveButtonRequest(
        RequestInvocationEvent event, String host)
        throws ServletException, IOException, SamFSException {

        TraceUtil.trace3("Entering");
        FileSystem fs = null;

        String exceHost = null;
        if (host == null) {
            exceHost = serverName;
            TraceUtil.trace3("set page session for exechost " + exceHost);

            SamQFSSystemModel sysModel = SamUtil.getModel(exceHost);
            fs = sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
        } else {
            exceHost = host;
            SamQFSSystemModel model = SamUtil.getModel(host);

            fs = model.getSamQFSSystemFSManager().getFileSystem(fsName);
        }

        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        FileSystemMountProperties properties = fs.getMountProperties();
        int mountState = fs.getState();

        LogUtil.info(
            this.getClass(),
            "handleSaveButtonRequest",
            "Starting save the mount options ");

        saveBasicProperties(properties);
        saveGeneralProperties(properties, mountState);

        if (!getSharedClientType().equals("Shared Client")) {
            savePerformanceProperties(properties);
        }
        if (!getSharedClientType().equals("Shared Client") && !directIO) {
            saveIODiscoveryProperties(properties);
        }

        // Unshared QFS has only the above four properties sections.
        // So, check for all other types and fill in the properties.
        if (isArchive()) {
            saveArchiveProperties(properties);
        } else if (getPageType().equals(FSMountViewBean.TYPE_SHAREDQFS)) {
            if (!getSharedClientType().equals("Shared Client")) {
                saveSharedQFSProperties(properties, mountState);
            }
        } else if (getPageType().equals(FSMountViewBean.TYPE_SHAREDSAMQFS)) {
            if (!getSharedClientType().equals("Shared Client")) {
                saveArchiveProperties(properties);
                saveSharedQFSProperties(properties, mountState);
            }
        }

        if ((host == null && getSharedClientType().equals("Shared Server")) ||
            (host != null && !getSharedClientType().equals("Shared Client"))) {
            TraceUtil.trace3("Begining editing SharedMountOptions");
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            if (host != null) {
                fsManager.setSharedMountOptions(host, fsName, properties);
            } else {
                fsManager.setSharedMountOptions(
                    exceHost, fsName, properties);
            }
        } else {
            fs.changeMountOptions();
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Save the data entered in Basic Properties Section
     */
    private void saveBasicProperties(FileSystemMountProperties properties) {
        TraceUtil.trace3("Entering");
        if (getPageType().equals(FSMountViewBean.TYPE_SHAREDSAMQFS) ||
            getPageType().equals(FSMountViewBean.TYPE_UNSHAREDSAMQFS) ||
            getPageType().equals(FSMountViewBean.TYPE_UNSHAREDSAMFS)) {
            String hwmString = ((String)
                getDisplayFieldValue("hwmValue")).trim();
            if (hwmString.length() > 0) {
                int hwm = Integer.parseInt(hwmString);
                if (hwm != properties.getHWM()) {
                    properties.setHWM(hwm);
                }
            } else {
                properties.setHWM(-1);
            }

            String lwmString = ((String)
            getDisplayFieldValue("lwmValue")).trim();
            if (lwmString.length() > 0) {
                int lwm = Integer.parseInt(lwmString);
                if (lwm != properties.getLWM()) {
                    properties.setLWM(lwm);
                }
            } else {
                properties.setLWM(-1);
            }
        }

        if (!getSharedClientType().equals("Shared Client")) {
            String stripeString =
                ((String) getDisplayFieldValue("stripeValue")).trim();
            if (stripeString.length() > 0) {
                int stripe = Integer.parseInt(stripeString);
                if (stripe != properties.getStripeWidth()) {
                    properties.setStripeWidth(stripe);
                }
            } else {
                properties.setStripeWidth(-1);
            }

            String trace = (String) getDisplayFieldValue("traceValue");
            if ("yes".equals(trace)) {
                if (!properties.isTrace()) {
                    properties.setTrace(true);
                }
            } else {
                if (properties.isTrace()) {
                    properties.setTrace(false);
                }
            }
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Save the data entered in Shared QFS Properties Section
     */
    private void saveSharedQFSProperties(
        FileSystemMountProperties properties, int mountState) {

        // mount in background and mount retries can be changed only
        // when fs is unmounted
        if (mountState != FileSystem.MOUNTED) {
            String mntbck = (String)
            getDisplayFieldValue("mountBackgroundValue");
            if ("yes".equals(mntbck)) {
                if (!properties.isMountInBackground()) {
                    properties.setMountInBackground(true);
                }
            } else {
                if (properties.isMountInBackground()) {
                    properties.setMountInBackground(false);
                }
            }

            String mntretriesStr =
                ((String) getDisplayFieldValue("mountretriesValue")).trim();
            if (mntretriesStr.length() > 0) {
                int mountretries = Integer.parseInt(mntretriesStr);
                if (mountretries != properties.getNoOfMountRetries()) {
                    properties.setNoOfMountRetries(mountretries);
                }
            } else {
                properties.setNoOfMountRetries(-1);
            }
        }

        String metarefreshStr =
            ((String) getDisplayFieldValue("metarefreshValue")).trim();
        if (metarefreshStr.length() > 0) {
            int metarefresh = Integer.parseInt(metarefreshStr);
            if (metarefresh != properties.getMetadataRefreshRate()) {
                properties.setMetadataRefreshRate(metarefresh);
            }
        } else {
            properties.setMetadataRefreshRate(-1);
        }

        String minblockStr =
            ((String) getDisplayFieldValue("minblockValue")).trim();
        if (minblockStr.length() > 0) {
            long minblock = Long.parseLong(minblockStr);
            if (minblock != properties.getMinBlockAllocation()) {
                properties.setMinBlockAllocation(minblock);
            }
        } else {
            properties.setMinBlockAllocation(-1);
        }

        int minblockUnit = SamUtil.getSizeUnit(
            (String)getDisplayFieldValue("minblockUnit"));
        if (minblockUnit != properties.getMinBlockAllocationUnit()) {
            properties.setMinBlockAllocationUnit(minblockUnit);
        }

        String maxblockStr =
            ((String)getDisplayFieldValue("maxblockValue")).trim();
        if (maxblockStr.length() > 0) {
            long maxblock = Long.parseLong(maxblockStr);
            if (maxblock != properties.getMaxBlockAllocation()) {
                properties.setMaxBlockAllocation(maxblock);
            }
        } else {
            properties.setMaxBlockAllocation(-1);
        }

        int maxblockUnit = SamUtil.getSizeUnit(
            (String)getDisplayFieldValue("maxblockUnit"));
        if (maxblockUnit != properties.getMaxBlockAllocationUnit()) {
            properties.setMaxBlockAllocationUnit(maxblockUnit);
        }

        String readleaseStr =
            ((String) getDisplayFieldValue("readleaseValue")).trim();
        if (readleaseStr.length() > 0) {
            int readlease = Integer.parseInt(readleaseStr);
            if (readlease != properties.getReadLeaseDuration()) {
                properties.setReadLeaseDuration(readlease);
            }
        } else {
            properties.setReadLeaseDuration(-1);
        }

        String writeleaseStr =
            ((String) getDisplayFieldValue("writeleaseValue")).trim();
        if (writeleaseStr.length() > 0) {
            int writelease = Integer.parseInt(writeleaseStr);
            if (writelease != properties.getWriteLeaseDuration()) {
                properties.setWriteLeaseDuration(writelease);
            }
        } else {
            properties.setWriteLeaseDuration(-1);
        }

        String appendleaseStr =
            ((String) getDisplayFieldValue("appendleaseValue")).trim();
        if (appendleaseStr.length() > 0) {
            int appendlease = Integer.parseInt(appendleaseStr);
            if (appendlease != properties.getAppendLeaseDuration()) {
                properties.setAppendLeaseDuration(appendlease);
            }
        } else {
            properties.setAppendLeaseDuration(-1);
        }

        String minPoolStr =
            ((String) getDisplayFieldValue("minPoolValue")).trim();
        if (minPoolStr.length() > 0) {
            int minPool = Integer.parseInt(minPoolStr);
            if (minPool != properties.getMinPool()) {
                properties.setMinPool(minPool);
            }
        } else {
            properties.setMinPool(-1);
        }

        String multihost = (String) getDisplayFieldValue("multihostValue");
        if ("yes".equals(multihost)) {
            if (!properties.isMultiHostWrite()) {
                properties.setMultiHostWrite(true);
            }
        } else {
            if (properties.isMultiHostWrite()) {
                properties.setMultiHostWrite(false);
            }
        }

        String sync = (String)getDisplayFieldValue("syncValue");
        if ("yes".equals(sync)) {
            if (!properties.isSynchronizedMetadata()) {
                properties.setSynchronizedMetadata(true);
            }
        } else {
            if (properties.isSynchronizedMetadata()) {
                properties.setSynchronizedMetadata(false);
            }
        }

        String cattr = (String) getDisplayFieldValue("cattrValue");
        if ("yes".equals(cattr)) {
            if (!properties.isConsistencyChecking()) {
                properties.setConsistencyChecking(true);
            }
        } else {
            if (properties.isConsistencyChecking()) {
                properties.setConsistencyChecking(false);
            }
        }

        String metadataStripeStr =
            ((String) getDisplayFieldValue("metastripeValue")).trim();
        if (metadataStripeStr.length() > 0) {
            int metadataStripe = Integer.parseInt(metadataStripeStr);
            if (metadataStripe != properties.getMetadataStripeWidth()) {
                properties.setMetadataStripeWidth(metadataStripe);
            }
        } else {
            properties.setMetadataStripeWidth(-1);
        }

        String leaseTimeoValue =
            ((String) getDisplayFieldValue("leaseTimeoValue")).trim();
        if (leaseTimeoValue.length() > 0) {
            int leaseTimeo = Integer.parseInt(leaseTimeoValue);
            if (leaseTimeo != properties.getLeaseTimeout()) {
                properties.setLeaseTimeout(leaseTimeo);
            }
        } else {
            properties.setLeaseTimeout(-1);
        }
    }

    /**
     * Save the data entered in General Properties Section
     */
    private void saveGeneralProperties(
        FileSystemMountProperties properties, int mountState) {

        // mountReadonly can be changed only when fs is unmounted
        if (mountState != FileSystem.MOUNTED) {
            String mountReadonly =
                (String)getDisplayFieldValue("mountreadonlyValue");

            if ("yes".equals(mountReadonly)) {
                if (!properties.isReadOnlyMount()) {
                    properties.setReadOnlyMount(true);
                }
            } else {
                if (properties.isReadOnlyMount()) {
                    properties.setReadOnlyMount(false);
                }
            }
        }

        if (!getSharedClientType().equals("Shared Client")) {
            String noSetuid = (String) getDisplayFieldValue("nouidValue");
            if ("yes".equals(noSetuid)) {
                if (!properties.isNoSetUID()) {
                    properties.setNoSetUID(true);
                }
            } else {
                if (properties.isNoSetUID()) {
                    properties.setNoSetUID(false);
                }
            }
        }

        if (!getPageType().equals(FSMountViewBean.TYPE_UNSHAREDSAMFS) &&
            !getSharedClientType().equals("Shared Client")) {
            String quickWrite = (String)
            getDisplayFieldValue("quickwriteValue");
            if ("yes".equals(quickWrite)) {
                if (!properties.isQuickWrite()) {
                    properties.setQuickWrite(true);
                }
            } else {
                if (properties.isQuickWrite()) {
                    properties.setQuickWrite(false);
                }
            }
        }
    }

    /**
     * Save the data entered in Performance Properties Section
     */
    private void savePerformanceProperties(
        FileSystemMountProperties properties) {

        String readaheadStr =
            ((String) getDisplayFieldValue("readaheadValue")).trim();
        if (readaheadStr.length() > 0) {
            long readahead = Long.parseLong(readaheadStr);
            if (readahead != properties.getReadAhead()) {
                properties.setReadAhead(readahead);
            }
        } else {
            properties.setReadAhead(-1);
        }

        int readaheadUnit = SamUtil.getSizeUnit(
            (String)getDisplayFieldValue("readaheadUnit"));
        if (readaheadUnit != properties.getReadAheadUnit()) {
            properties.setReadAheadUnit(readaheadUnit);
        }

        String writebehindStr =
            ((String) getDisplayFieldValue("writebehindValue")).trim();
        if (writebehindStr.length() > 0) {
            long writebehind = Long.parseLong(writebehindStr);
            if (writebehind != properties.getWriteBehind()) {
                properties.setWriteBehind(writebehind);
            }
        } else {
            properties.setWriteBehind(-1);
        }

        int writebehindUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("writebehindUnit"));
        if (writebehindUnit != properties.getWriteBehindUnit()) {
            properties.setWriteBehindUnit(writebehindUnit);
        }

        String writethrottleStr =
            ((String) getDisplayFieldValue("writethroValue")).trim();
        if (writethrottleStr.length() > 0) {
            long writethrottle = Long.parseLong(writethrottleStr);
            if (writethrottle != properties.getWriteThrottle()) {
                properties.setWriteThrottle(writethrottle);
            }
        } else {
            properties.setWriteThrottle(-1);
        }

        int writethroUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("writethroUnit"));
        if (writethroUnit != properties.getWriteThrottleUnit()) {
            properties.setWriteThrottleUnit(writethroUnit);
        }

        String flushbehindStr =
            ((String) getDisplayFieldValue("flushbehindValue")).trim();
        if (flushbehindStr.length() > 0) {
            int flushbehind = Integer.parseInt(flushbehindStr);
            if (flushbehind != properties.getFlushBehind()) {
                properties.setFlushBehind(flushbehind);
            }
        } else {
            properties.setFlushBehind(-1);
        }

        int flushbehindUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("flushbehindUnit"));
        if (flushbehindUnit != properties.getFlushBehindUnit()) {
            properties.setFlushBehindUnit(flushbehindUnit);
        }

        if (!(getPageType().equals(FSMountViewBean.TYPE_SHAREDQFS) ||
            getPageType().equals(FSMountViewBean.TYPE_UNSHAREDQFS))) {
            String stageflushStr =
                ((String) getDisplayFieldValue("stageflushValue")).trim();
            if (stageflushStr.length() > 0) {
                int stageflush = Integer.parseInt(stageflushStr);
                if (stageflush != properties.getStageFlushBehind()) {
                    properties.setStageFlushBehind(stageflush);
                }
            } else {
                properties.setStageFlushBehind(-1);
            }

            int stageflushUnit = SamUtil.getSizeUnit(
                (String) getDisplayFieldValue("stageflushUnit"));
            if (stageflushUnit != properties.getStageFlushBehindUnit()) {
                properties.setStageFlushBehindUnit(stageflushUnit);
            }
        }

        String raid = (String)getDisplayFieldValue("softwareValue");
        if ("on".equals(raid)) {
            if (!properties.isSoftRAID()) {
                properties.setSoftRAID(true);
            }
        } else {
            if (properties.isSoftRAID()) {
                properties.setSoftRAID(false);
            }
        }

        String forceio = (String)getDisplayFieldValue("forceValue");
        if ("on".equals(forceio)) {
            directIO = true;
            if (!properties.isForceDirectIO()) {
                properties.setForceDirectIO(true);
            }
        } else {
            directIO = false;
            if (properties.isForceDirectIO()) {
                properties.setForceDirectIO(false);
            }
        }

        // force_nfs_async is for all fs
        String forceNFSAsync =
            (String) getDisplayFieldValue("forceNFSAsyncValue");
        properties.setForceNFSAsync("on".equals(forceNFSAsync));
    }

    /**
     * Save the data entered in IO Discovery Properties Section
     */
    private void saveIODiscoveryProperties(
        FileSystemMountProperties properties) {

        TraceUtil.trace2("Saving IO Discovery Properties...");
        String consecreadStr =
            ((String) getDisplayFieldValue("consecreadValue")).trim();
        if (consecreadStr.length() > 0) {
            int consecread = Integer.parseInt(consecreadStr);
            if (consecread != properties.getConsecutiveReads()) {
                properties.setConsecutiveReads(consecread);
            }
        } else {
            properties.setConsecutiveReads(-1);
        }

        String consecwriteStr =
            ((String) getDisplayFieldValue("consecwriteValue")).trim();
        if (consecwriteStr.length() > 0) {
            int consecwrite = Integer.parseInt(consecwriteStr);
            if (consecwrite != properties.getConsecutiveWrites()) {
                properties.setConsecutiveWrites(consecwrite);
            }
        } else {
            properties.setConsecutiveWrites(-1);
        }

        String wellreadminStr =
            ((String) getDisplayFieldValue("wellreadminValue")).trim();
        if (wellreadminStr.length() > 0) {
            int wellreadmin = Integer.parseInt(wellreadminStr);
            if (wellreadmin != properties.getWellAlignedReadMin()) {
                properties.setWellAlignedReadMin(wellreadmin);
            }
        } else {
            properties.setWellAlignedReadMin(-1);
        }

        int wellreadminUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("wellreadminUnit"));
        if (wellreadminUnit != properties.getWellAlignedReadMinUnit()) {
            properties.setWellAlignedReadMinUnit(wellreadminUnit);
        }

        String misreadminStr =
            ((String) getDisplayFieldValue("misreadminValue")).trim();
        if (misreadminStr.length() > 0) {
            int misreadmin = Integer.parseInt(misreadminStr);
            if (misreadmin != properties.getMisAlignedReadMin()) {
                properties.setMisAlignedReadMin(misreadmin);
            }
        } else {
            properties.setMisAlignedReadMin(-1);
        }

        int misreadminUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("misreadminUnit"));
        if (misreadminUnit != properties.getMisAlignedReadMinUnit()) {
            properties.setMisAlignedReadMinUnit(misreadminUnit);
        }

        String wellwriteminStr =
            ((String) getDisplayFieldValue("wellwriteminValue")).trim();
        if (wellwriteminStr.length() > 0) {
            int wellwritemin = Integer.parseInt(wellwriteminStr);
            if (wellwritemin != properties.getWellAlignedWriteMin()) {
                properties.setWellAlignedWriteMin(wellwritemin);
            }
        } else {
            properties.setWellAlignedWriteMin(-1);
        }

        int wellwriteminUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("wellwriteminUnit"));
        if (wellwriteminUnit != properties.getWellAlignedWriteMinUnit()) {
            properties.setWellAlignedWriteMinUnit(wellwriteminUnit);
        }

        String miswriteminStr =
            ((String) getDisplayFieldValue("miswriteminValue")).trim();
        if (miswriteminStr.length() > 0) {
            int miswritemin = Integer.parseInt(miswriteminStr);
            if (miswritemin != properties.getMisAlignedWriteMin()) {
                properties.setMisAlignedWriteMin(miswritemin);
            }
        } else {
            properties.setMisAlignedWriteMin(-1);
        }

        int miswriteminUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("miswriteminUnit"));
        if (miswriteminUnit != properties.getMisAlignedWriteMinUnit()) {
            properties.setMisAlignedWriteMinUnit(miswriteminUnit);
        }

        String dioszero = (String) getDisplayFieldValue("dioszeroValue");
        if ("yes".equals(dioszero)) {
            if (!properties.isDirectIOZeroing()) {
                properties.setDirectIOZeroing(true);
            }
        } else {
            if (properties.isDirectIOZeroing()) {
                properties.setDirectIOZeroing(false);
            }
        }
    }

    /**
     * Save the data entered in Archive Properties Section
     */
    private void saveArchiveProperties(FileSystemMountProperties properties) {

        String partialRelStr =
            ((String) getDisplayFieldValue("partreleaseValue")).trim();
        if (partialRelStr.length() > 0) {
            int partialRel = Integer.parseInt(partialRelStr);
            if (partialRel != properties.getDefaultPartialReleaseSize()) {
                properties.setDefaultPartialReleaseSize(partialRel);
            }
        } else {
            properties.setDefaultPartialReleaseSize(-1);
        }

        int partreleaseUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("partreleaseUnit"));
        if (partreleaseUnit != properties.getDefaultPartialReleaseSizeUnit()) {
            properties.setDefaultPartialReleaseSizeUnit(partreleaseUnit);
        }

        String maxpartialRelStr =
            ((String) getDisplayFieldValue("maxpartValue")).trim();
        if (maxpartialRelStr.length() > 0) {
            int maxpartialRel = Integer.parseInt(maxpartialRelStr);
            if (maxpartialRel != properties.getDefaultMaxPartialReleaseSize()) {
                properties.setDefaultMaxPartialReleaseSize(maxpartialRel);
            }
        } else {
            properties.setDefaultMaxPartialReleaseSize(-1);
        }

        int maxpartUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("maxpartUnit"));
        if (maxpartUnit != properties.getDefaultMaxPartialReleaseSizeUnit()) {
            properties.setDefaultMaxPartialReleaseSizeUnit(maxpartUnit);
        }

        String partialStageStr =
            ((String) getDisplayFieldValue("partstageValue")).trim();
        if (partialStageStr.length() > 0) {
            long partialStage = Long.parseLong(partialStageStr);
            if (partialStage != properties.getPartialStageSize()) {
                properties.setPartialStageSize(partialStage);
            }
        } else {
            properties.setPartialStageSize(-1);
        }

        int partstageUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("partstageUnit"));
        if (partstageUnit != properties.getPartialStageSizeUnit()) {
            properties.setPartialStageSizeUnit(partstageUnit);
        }

        String stagetryStr =
            ((String) getDisplayFieldValue("stageretriesValue")).trim();
        if (stagetryStr.length() > 0) {
            int stagetry = Integer.parseInt(stagetryStr);
            if (stagetry != properties.getNoOfStageRetries()) {
                properties.setNoOfStageRetries(stagetry);
            }
        } else {
            properties.setNoOfStageRetries(-1);
        }

        String windowStr =
            ((String) getDisplayFieldValue("stagewindowValue")).trim();
        if (windowStr.length() > 0) {
            long window = Long.parseLong(windowStr);
            if (window != properties.getStageWindowSize()) {
                properties.setStageWindowSize(window);
            }
        } else {
            properties.setStageWindowSize(-1);
        }

        int stagewindowUnit = SamUtil.getSizeUnit(
            (String) getDisplayFieldValue("stagewindowUnit"));
        if (stagewindowUnit != properties.getStageWindowSizeUnit()) {
            properties.setStageWindowSizeUnit(stagewindowUnit);
        }

        String runArchive = (String)getDisplayFieldValue("questionValue");
        if ("yes".equals(runArchive)) {
            if (!properties.isArchiverAutoRun()) {
                properties.setArchiverAutoRun(true);
            }
        } else {
            if (properties.isArchiverAutoRun()) {
                properties.setArchiverAutoRun(false);
            }
        }
    }

    private String getPageType() {
        return this.pageType;
    }

    private String getSharedClientType() {
        return this.sharedType;
    }

    private boolean isArchive() {
        if (archive == null) {
            // shared fs
            return false;
        } else {
            return archive.intValue() == FileSystem.ARCHIVING;
        }
    }
}
