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

// ident	$Id: ArchiveSetUpViewBean.java,v 1.37 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;

/**
 * Creates the 'Archive Setup' Page
 */

public class ArchiveSetUpViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ArchiveSetUp";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/archive/ArchiveSetUp.jsp";

    // Used for constructing the Action Table
    private static final String CONTAINER_VIEW = "ArchiveSetUpView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;

    // map of action table and its model
    private Map models = null;
    private GlobalArchiveDirective globalArchiveDirective = null;
    private BufferDirective[] stageBufferDirectiveArray = null;
    private DriveDirective[] stageDriveDirectiveArray = null;

    // property sheet children

    public static final String ARCHIVE_INTERVAL = "ArchiveInterval";
    public static final String ARCHIVE_INTERVAL_UNIT = "ArchiveIntervalUnits";
    public static final String ARCHIVE_SCAN_METHOD = "ArchiveScanMethod";
    public static final String ARCHIVE_LOG = "ArchiveLogFile";
    public static final String STAGE_LOG = "StageLogFile";
    public static final String RELEASE_LOG = "ReleaseLogFile";
    public static final String RELEASE_AGE = "ReleaseAge";
    public static final String RELEASE_AGE_UNIT = "ReleaseAgeUnits";

    // hidden field to retrieve the mediatable and lib table size
    public static final String CHILD_MEDIA_SIZE = "mediaNumber";
    public static final String CHILD_LIB_SIZE = "libNumber";
    public static final String CHILD_MEDIA_TYPES = "mediaTypes";

    public static final String CHILD_HIDDEN_MESSAGES = "HiddenMessages";

    /**
     * Constructor
     */
    public ArchiveSetUpViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        propertySheetModel = PropertySheetUtil.createModel(
            "/jsp/archive/ArchiveSetUpPropSheet.xml");
        initializeTableModels();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();

        // hidden fields
        registerChild(CHILD_LIB_SIZE, CCHiddenField.class);
        registerChild(CHILD_MEDIA_SIZE, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_MESSAGES, CCHiddenField.class);
        registerChild(CHILD_MEDIA_TYPES, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CONTAINER_VIEW, ArchiveSetUpView.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Creating child " + name);
        if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);

        } else if (name.equals(CONTAINER_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new ArchiveSetUpView(this, models, name);

        } else if (name.equals(CHILD_LIB_SIZE)
                || name.equals(CHILD_MEDIA_SIZE)
                || name.equals(CHILD_MEDIA_TYPES)) {

            return new CCHiddenField(this, name, null);

        } else if (name.equals(CHILD_HIDDEN_MESSAGES)) {
            return new CCHiddenField(this, name, getMessageString());

        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);

        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            return PropertySheetUtil.createChild(
                this, propertySheetModel, name);

        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/archive/ArchiveSetUpPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * create the table models
     */
    private void initializeTableModels() {
        TraceUtil.trace3("Entering");
        models = new HashMap();

        ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

        models.put(ArchiveSetUpView.MEDIA_PARAMETERS_TABLE,
            new CCActionTableModel(sc,
            "/jsp/archive/MediaParametersTable.xml"));
        models.put(ArchiveSetUpView.DRIVE_LIMITS_TABLE,
            new CCActionTableModel(sc, "/jsp/archive/DriveLimitsTable.xml"));

        TraceUtil.trace3("Exiting");
    }

    private void loadPropertySheetModel(
        CCPropertySheetModel propertySheetModel,
        GlobalArchiveDirective globalDir) {

        TraceUtil.trace3("Entering");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            if (globalDir != null) {
                long interval = globalDir.getInterval();
                if (interval != -1) {
                    // get the interval unit, the logic layer always gives unit
                    // and value from the server as it is stored.
                    // Convert it to proper units for display, e.g. 600 s = 10m
                    // if value is empty, default to TIME_MINUTE
                    int intervalUnit = globalDir.getIntervalUnit();

                    if (intervalUnit != -1) {
                        // convert it to appropriate units

                        propertySheetModel.setValue(ARCHIVE_INTERVAL,
                            String.valueOf(interval));
                        propertySheetModel.setValue(ARCHIVE_INTERVAL_UNIT,
                            String.valueOf(intervalUnit));
                    } else {

                        propertySheetModel.setValue(ARCHIVE_INTERVAL,
                            String.valueOf(interval));
                        propertySheetModel.setValue(ARCHIVE_INTERVAL_UNIT,
                            String.valueOf(SamQFSSystemModel.TIME_MINUTE));
                    }

                } else {
                    // If interval is not set, display an empty
                    // interval field but show units in TIME_MINUTE
                    propertySheetModel.setValue(ARCHIVE_INTERVAL, "");
                    propertySheetModel.setValue(ARCHIVE_INTERVAL_UNIT,
                        String.valueOf(SamQFSSystemModel.TIME_MINUTE));
                }


                int scanMethod = globalDir.getArchiveScanMethod();
                if (scanMethod != -1) {
                    propertySheetModel.setValue(ARCHIVE_SCAN_METHOD,
                                                Integer.toString(scanMethod));
                } else {
                    propertySheetModel.setValue(ARCHIVE_SCAN_METHOD,
                        new Integer(
                            GlobalArchiveDirective.NO_SCAN).toString());
                }

                String archiveLog = globalDir.getArchiveLogfile();

                if (archiveLog != null) {
                    TraceUtil.trace3(new NonSyncStringBuffer().append(
                        "Archive Log File: ").append(archiveLog).toString());

                    propertySheetModel.setValue(ARCHIVE_LOG, archiveLog);
                }
                String stagerLog =
                    sysModel.getSamQFSSystemArchiveManager().
                        getStagerLogFile();

                if (stagerLog != null) {
                    TraceUtil.trace3(new NonSyncStringBuffer().append(
                        "Stage Log File: ").append(stagerLog).toString());
                    propertySheetModel.setValue(STAGE_LOG, stagerLog);
                }

                String releaseLog =
                    sysModel.getSamQFSSystemArchiveManager().
                        getReleaserLogFile();
                if (releaseLog != null) {
                    TraceUtil.trace3(new NonSyncStringBuffer().append(
                        "Release Log File: ").append(releaseLog).toString());
                    propertySheetModel.setValue(RELEASE_LOG, releaseLog);
                }

                // min age should be returned as a int and another api
                // should be provided to give the unit
                String minAgeStr =
                    sysModel.getSamQFSSystemArchiveManager().
                        getMinReleaseAge();

                TraceUtil.trace3(new NonSyncStringBuffer().append(
                        "Release Age: ").append(minAgeStr).toString());
                if (!minAgeStr.equals("")) {
                    String minAge =
                        minAgeStr.substring(0, minAgeStr.length() - 1);
                    char minAgeUnitChar =
                        minAgeStr.charAt(minAgeStr.length() - 1);

                    int minAgeUnitInt = SamQFSSystemModel.TIME_SECOND;
                    switch (minAgeUnitChar) {
                        case 's':
                        case 'S':
                            minAgeUnitInt = SamQFSSystemModel.TIME_SECOND;
                            break;
                        case 'm':
                            minAgeUnitInt = SamQFSSystemModel.TIME_MINUTE;
                            break;
                        case 'H':
                        case 'h':
                            minAgeUnitInt = SamQFSSystemModel.TIME_HOUR;
                            break;
                        case 'd':
                            minAgeUnitInt = SamQFSSystemModel.TIME_DAY;
                            break;
                        case 'w':
                            minAgeUnitInt = SamQFSSystemModel.TIME_WEEK;
                            break;
                    }

                    propertySheetModel.setValue(RELEASE_AGE, minAge);
                    propertySheetModel.setValue(RELEASE_AGE_UNIT,
                        String.valueOf(minAgeUnitInt));

                } else {
                    propertySheetModel.setValue(RELEASE_AGE, "");
                    propertySheetModel.setValue(RELEASE_AGE_UNIT,
                        String.valueOf(SamQFSSystemModel.TIME_MINUTE));
                }
            }

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "loadPropertySheetModel",
                "Failed to retrieve model",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler for the 'Save' Button
     * All of the data is retrieved from the components and then
     * sent to the model.  When the button is clicked, the new data
     * appears in another page.
     */
    public void handlePageButtonSaveRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        // Use seaparate try catch blocks for saving archiver and stager info
        // to keep track all the errors in this section
        List errCause = new ArrayList(0);
        String errMsg = null;

        boolean warning = false; // some exceptions are warniongs in archiver

        ArchiveSetUpView theView = null;
        SamQFSSystemModel sysModel = null;

        try {
            boolean boolArchiveChange = false; // No change

            theView = (ArchiveSetUpView)getChild(CONTAINER_VIEW);
            sysModel = SamUtil.getModel(getServerName());
            if (sysModel != null) {
                GlobalArchiveDirective globalDir =
                    sysModel.getSamQFSSystemArchiveManager().
                    getGlobalDirective();

                // save archive interval and interval unit
                String intervalStr =
                    (String)getDisplayFieldValue(ARCHIVE_INTERVAL);
                intervalStr = intervalStr != null ? intervalStr.trim() : "";

                // If intervalStr is set to "", then reset archive interval
                long archiveInterval = -1;
                if (!intervalStr.equals("")) {
                    try {
                        archiveInterval = Long.parseLong(intervalStr);
                    } catch (NumberFormatException nfex) {
                        archiveInterval = -1;
                    }
                }
                TraceUtil.trace3("Archive interval = " + archiveInterval);
                if (archiveInterval != -1) {
                    if (archiveInterval != globalDir.getInterval()) {
                        globalDir.setInterval(archiveInterval);
                        boolArchiveChange |= true;
                    }
                    String intervalUnitStr =
                        (String)getDisplayFieldValue(ARCHIVE_INTERVAL_UNIT);
                    // If archiveInterval == -1 irrespective of what the unit is
                    // user is resetting the fields
                    int intervalUnit = Integer.parseInt(intervalUnitStr);
                    if (intervalUnit != globalDir.getIntervalUnit()) {
                        globalDir.setIntervalUnit(intervalUnit);
                        boolArchiveChange |= true;
                    }
                } else {
                    // dont have to check if value has actually changed
                    // value can never be -1 when returned from sam
                    globalDir.setInterval(-1);
                    boolArchiveChange |= true;
                }
                // archive scan method
                String archiveScanStr =
                    (String)getDisplayFieldValue(ARCHIVE_SCAN_METHOD);
                if (!archiveScanStr.equals(SelectableGroupHelper.NOVAL)) {
                    int archiveScanInt = Integer.parseInt(archiveScanStr);
                    if (archiveScanInt != globalDir.getArchiveScanMethod()) {
                        globalDir.setArchiveScanMethod(archiveScanInt);
                        boolArchiveChange |= true;
                    }
                } else {
                    globalDir.setArchiveScanMethod(-1);
                    boolArchiveChange |= true;
                }

                // archive log
                String archiveLog =
                    ((String)getDisplayFieldValue(ARCHIVE_LOG)).trim();
                if (!archiveLog.equals(globalDir.getArchiveLogfile())) {
                    globalDir.setArchiveLogfile(archiveLog);
                    boolArchiveChange |= true;
                }

                // Save all the archive related data first so that we don't have
                // to make multiple rpc calls for archive related information
                // on this page

                // save archiver media params
                // max file size
                boolArchiveChange |= theView.saveArchiveSizeMediaParameters(
                    "MaxArchiveFileSize", globalDir.getMaxFileSize());

                // overflow min size
                boolArchiveChange |= theView.saveArchiveSizeMediaParameters(
                    "MinSizeForOverflow",
                    globalDir.getMinFileSizeForOverflow());

                // save buffer size and buffer lock
                boolArchiveChange |= theView.saveBufferMediaParameters(
                    "archive", globalDir.getBufferSize());

                // save maximum drives for archiving
                boolArchiveChange |= theView.saveDriveLimits(
                    "archive", globalDir.getDriveDirectives());

                if (boolArchiveChange == true) { // user has changed setup
                    LogUtil.info(this.getClass(), "handlePageButtonSaveRequest",
                        "Start saving archiver related setup information");

                    // write the archvive related changes to the server
                    sysModel.getSamQFSSystemArchiveManager().
                        setGlobalDirective(globalDir);
                    globalDir.changeGlobalDirective();

                    LogUtil.info(this.getClass(), "handlePageButtonSaveRequest",
                        "Done saving archiver related setup configuration");
                } else {
                    LogUtil.info(this.getClass(), "handlePageButtonSaveRequest",
                        "No setup information for archive has been modified");
                }
            } else {
                // sysmodel is null
                throw new SamFSException(null, -2001);
            }
        } catch (SamFSException ex) {

            String processMsg = null;
            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause.add(SamUtil.getResourceString(
                            "ArchiveConfig.error.detail"));
            } else if (ex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause.add(SamUtil.getResourceString(
                            "ArchiveConfig.warning.detail"));

            } else {
                processMsg = "Failed to save global directives";
                errCause.add(ex.getMessage());
                errMsg = SamUtil.getResourceString("ArchiveSetup.error.save");

            }
            SamUtil.processException(ex,
                this.getClass(),
                "handlePageButtonSaveRequest",
                processMsg,
                getServerName());
        }

        // Staging Data
        try {
            if (sysModel != null) {
                boolean boolStageChange = false;
                // stage log
                String stageLog =
                    ((String) getDisplayFieldValue(STAGE_LOG)).trim();
                // set the stage log only if the current value set by the user
                // is different from that on the server
                // If file does not exist, setStagerLogFile creates the file
                if (!stageLog.equals(
                    sysModel.getSamQFSSystemArchiveManager().
                        getStagerLogFile())) {
                        TraceUtil.trace3("Saving stager log file");
                        sysModel.getSamQFSSystemArchiveManager().
                            setStagerLogFile(stageLog);
                }

                // stage buffer directives
                BufferDirective[] stageBufferDirectives =
                    sysModel.getSamQFSSystemArchiveManager().
                        getStagerBufDirectives();
                // save buffer size and buffer lock
                boolStageChange |= theView.saveBufferMediaParameters(
                    "stage", stageBufferDirectives);

                // stager drive directives
                // Maximum drives for staging
                DriveDirective[] stageDriveDirectives =
                    sysModel.getSamQFSSystemArchiveManager().
                        getStagerDriveDirectives();
                boolStageChange |= theView.saveDriveLimits(
                    "stage", stageDriveDirectives);

                if (boolStageChange == true) {
                    LogUtil.info(this.getClass(), "handlePageButtonSaveRequest",
                        "Start saving stager configuration");

                    // write stage buffer directives to server
                    sysModel.getSamQFSSystemArchiveManager().
                        changeStagerDirective(stageBufferDirectives);
                    sysModel.getSamQFSSystemArchiveManager().
                        changeStagerDriveDirective(stageDriveDirectives);

                    LogUtil.info(this.getClass(), "handlePageButtonSaveRequest",
                        "Done saving stager configuration");
                } else {
                    LogUtil.info(this.getClass(), "handlePageButtonSaveRequest",
                        "No setup information for stager has been modified");
                }
            } else {
                // sysmodel is null
                throw new SamFSException(null, -2001);
            }
        } catch (SamFSException ex) {

            errMsg = SamUtil.getResourceString("ArchiveSetup.error.save");
            errCause.add(ex.getMessage());
            SamUtil.processException(ex,
                this.getClass(),
                "handlePageButtonSaveRequest",
                "Failed to save stager configuration",
                getServerName());
        }

        // Release setup information
        try {
            // release log
            // setReleaserLogFile will create the file if it does not exist
            String releaseLog =
                ((String) getDisplayFieldValue(RELEASE_LOG)).trim();
            if (!releaseLog.equals(sysModel.
                getSamQFSSystemArchiveManager().getReleaserLogFile())) {
                    TraceUtil.trace3("Saving release log file");
                    sysModel.getSamQFSSystemArchiveManager().
                        setReleaserLogFile(releaseLog);
            }
            // minimum release age
            int minReleaseAge = -1;
            String minReleaseAgeStr =
                (String) getDisplayFieldValue(RELEASE_AGE);
            minReleaseAgeStr =
                (minReleaseAgeStr != null) ? minReleaseAgeStr.trim() : "";

            if (!minReleaseAgeStr.equals("")) {
                try {
                    minReleaseAge = Integer.parseInt(minReleaseAgeStr);
                } catch (NumberFormatException nfex) {
                    minReleaseAge = -1;
                }
            }
            // minimum release age unit
            String minReleaseAgeUnitStr =
                (String) getDisplayFieldValue(RELEASE_AGE_UNIT);
            int minReleaseAgeUnit = -1;
            String tmpUnitStr = "";
            // if a value is set for release age, unit should not be NOVAL
            if (!minReleaseAgeUnitStr.equals(SelectableGroupHelper.NOVAL)) {
                try {
                    minReleaseAgeUnit =
                        Integer.parseInt(minReleaseAgeUnitStr);
                    switch (minReleaseAgeUnit) {
                        case SamQFSSystemModel.TIME_SECOND:
                            tmpUnitStr = "s";
                            break;
                        case SamQFSSystemModel.TIME_MINUTE:
                            tmpUnitStr = "m";
                            break;
                        case SamQFSSystemModel.TIME_HOUR:
                            tmpUnitStr = "h";
                            break;
                        case SamQFSSystemModel.TIME_DAY:
                            tmpUnitStr = "d";
                            break;
                        case SamQFSSystemModel.TIME_WEEK:
                            tmpUnitStr = "w";
                            break;
                    }
                } catch (NumberFormatException nfe) {
                    minReleaseAgeUnit = -1;
                }
                // Logic layer currently takes in releaseAge in TimeWithUnit
                // TODO: Logic layer should take in age as long and
                // provide another setter to set the unit ???

                String minAge = "";
                if (minReleaseAge != -1 && !tmpUnitStr.equals("")) {
                    minAge = new StringBuffer().append(
                        minReleaseAge).append(tmpUnitStr).toString();
                }
                if (!minAge.equals(
                    sysModel.getSamQFSSystemArchiveManager().
                        getMinReleaseAge())) {
                    sysModel.getSamQFSSystemArchiveManager().
                        setMinReleaseAge(minAge);
                }
            }
        } catch (SamFSException ex) {
            errCause.add(ex.getMessage());
            errMsg = SamUtil.getResourceString("ArchiveSetup.error.save");
            SamUtil.processException(ex,
                this.getClass(),
                "handlePageButtonSaveRequest",
                "Failed to save releaser configuration",
                getServerName());

        }
        // If any exceptions were thrown, concatenate the causes here
        if ((errCause != null) && (errCause.size() > 0)) {
            // Display concatenated alerts
            NonSyncStringBuffer buffer = new NonSyncStringBuffer();
            Iterator it = errCause.iterator();
            while (it.hasNext()) {
                String err = (String)it.next();
                buffer.append(err).append("<br>");
            }
            if (!warning) {
                if (errMsg == null) {
                    errMsg =
                        SamUtil.getResourceString("ArchiveSetup.error.save");
                }
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    errMsg,
                    -2029,
                    buffer.toString(),
                    getServerName());
            } else {
                SamUtil.setWarningAlert(this,
                    CHILD_COMMON_ALERT,
                    errMsg,
                    buffer.toString());
            }
        } else {
            // everything is saved successfully
            showAlert();
        }

        // sleeping so that sam-amld has a chance to start
        // which is needed when populating data
        try {
            Thread.sleep(5000);
        } catch (InterruptedException iex) {
            TraceUtil.trace3("InterruptedException Caught: Reason: " +
                iex.getMessage());
        }
        this.forwardTo();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler for the 'Reset' Button
     */
    public void handlePageButtonResetRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        this.forwardTo();
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // init dropdown ArchiveIntervalUnits
        CCDropDownMenu dropDown =
            (CCDropDownMenu)getChild(ARCHIVE_INTERVAL_UNIT);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Times.labels,
            SelectableGroupHelper.Times.values));
        if (dropDown.getValue() == null) {
            dropDown.setValue(
                new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        // init dropdown ArchiveScanMethod
        dropDown = (CCDropDownMenu)getChild(ARCHIVE_SCAN_METHOD);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.ScanMethod.labels,
            SelectableGroupHelper.ScanMethod.values));
        if (dropDown.getValue() == null) {
            dropDown.setValue(
                new Integer(GlobalArchiveDirective.NO_SCAN).toString());
        }

        // init dropdown ReleaseAgeUnits
        dropDown = (CCDropDownMenu)getChild(RELEASE_AGE_UNIT);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Times.labels,
            SelectableGroupHelper.Times.values));
        if (dropDown.getValue() == null) {
            dropDown.setValue(
                new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        // disbale save button if no config authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            ((CCButton)getChild("PageButtonSave")).setDisabled(true);
            ((CCButton)getChild("PageButtonReset")).setDisabled(true);
        }

        // Inorder to reduce the number of remote calls per display cycle
        // get the sysModel and the archiveManager here and pass it as
        // arguments to loadPropertySheetModel() and View.populateTables()
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            if (sysModel != null) {
                GlobalArchiveDirective globalDir =
                    sysModel.getSamQFSSystemArchiveManager().
                    getGlobalDirective();

                globalArchiveDirective = globalDir;
                stageBufferDirectiveArray =
                    sysModel.getSamQFSSystemArchiveManager().
                        getStagerBufDirectives();
                stageDriveDirectiveArray =
                    sysModel.getSamQFSSystemArchiveManager().
                        getStagerDriveDirectives();

                loadPropertySheetModel(propertySheetModel, globalDir);

                ArchiveSetUpView view =
                    (ArchiveSetUpView)getChild(CONTAINER_VIEW);
                view.populateTableModels(globalDir);
            }
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "Loading the page failed",
                "Failed to retrieve model",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }


        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to setup the inline alert
     */
    private void showAlert() {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            this,
            CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString("ArchiveSetup.save"),
            getServerName());
        TraceUtil.trace3("Exiting");
    }

    private String getMessageString() {
        return new NonSyncStringBuffer(
            // Hidden fields for error messages (client side validation)

        SamUtil.getResourceString("ArchiveSetup.error.driveempty")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.driverange")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.bufferempty")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.archiveBufferrange")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.maxarchempty")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.maxarchiverange")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.minfsempty")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.minfsrange")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.logfileempty")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.archivelogfile")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.intervalempty")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.intervalrangeinsecs")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.intervalrangeinmins")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.intervalrangeinhours")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.intervalrangeindays")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.intervalrangeinweeks")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.miniagerangeinsecs")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.miniagerangeinmins")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.miniagerangeinhours")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.miniagerangeindays")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.miniagerangeinweeks")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.stagelogfile")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.releaselogfile")).
        append("###").append(
        SamUtil.getResourceString("ArchiveSetup.error.stageBufferrange")).
        toString();
    }

    public GlobalArchiveDirective getGlobalDirective() {
        return globalArchiveDirective;
    }

    public BufferDirective[] getStageBufferDirectives() {
        return stageBufferDirectiveArray;
    }

    public DriveDirective[] getStageDriveDirectives() {
        return stageDriveDirectiveArray;
    }
}
