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

// ident        $Id: ShrinkFSBean.java,v 1.11 2009/03/04 21:54:41 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.data.provider.FieldKey;
import com.sun.data.provider.RowKey;
import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.fs.FSUtil;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.ShrinkOption;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.StripedGroupImpl;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.Select;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.component.TableRowGroup;
import com.sun.web.ui.component.WizardStep;
import com.sun.web.ui.event.WizardEvent;
import com.sun.web.ui.event.WizardEventListener;
import com.sun.web.ui.model.Option;
import java.io.Serializable;
import java.util.HashMap;
import javax.faces.application.FacesMessage;
import javax.faces.component.UIComponent;
import javax.faces.context.FacesContext;
import javax.faces.validator.ValidatorException;

public class ShrinkFSBean implements Serializable {

    /**
     * Step Ids used in ShrinkFSWizard.jsp.  Make sure the ids match with the
     * JSP page.
     */
    protected static final String STEP_SELECT_STORAGE = "step_id_selectstorage";
    protected static final String STEP_METHOD = "step_id_method";
    protected static final String STEP_SPECIFY_DEVICE = "step_id_specifydevice";
    protected static final String STEP_OPTIONS = "step_id_options";
    protected static final String STEP_REVIEW = "step_id_review";

    /** Holds archiving information of the file system */
    protected boolean archive = false;

    /** Holds value of alert info. */
    protected boolean alertRendered = false;
    protected String alertType = null;
    protected String alertDetail = null;
    protected String alertSummary = null;

    /** Holds data and metadata table information in exclude device page. */
    protected String tableTitleExclude = null;
    protected TableRowGroup excludeTableRowGroup = null;
    protected Select selectExclude = new Select("excludeTable");
    private int eqToShrink = -1;
    protected String selectedStripedGroup = null;
    /** Holds the number of members in the striped group to be shrunk. */
    private int numberOfMembers = -1;
    /** Holds the disk cache information of the tables. */
    private DiskCache [] allocUnits = null;

    /** Holds the radio button information of specify method step. */
    protected boolean renderSubStepMethod = false;
    protected boolean renderedMethodRelease = true;
    protected boolean renderedMethodDistribute = true;
    protected boolean renderedMethodMove = true;
    protected boolean selectedMethodRelease = false;
    protected boolean selectedMethodDistribute = false;
    protected boolean selectedMethodMove = false;

    /** Holds table information in specify device page. */
    protected TableRowGroup availableTableRowGroup = null;
    protected Select selectAvailable = new Select("availableTable");
    private DiskCache [] availUnits = null;
    private String [] replacementPaths = null;

    protected String shrinkSharedFSText = null;

    /** Hold information in specify shrink options step. */
    protected String textLogFile = null;
    protected boolean selectedOptionDefault = false;
    protected boolean selectedOptionCustom = false;
    private static final
        String DEFAULT_LOG_FILE_LOCATION_PREFIX = "/var/log/shrink-";
    private static final String
        DEFAULT_LOG_FILE_LOCATION_SUFFIX = ".log";
    protected boolean displayName = false;
    protected boolean dryRun = false;
    protected boolean stageBack = false;
    protected boolean stagePartialBack = false;
    protected String selectedBlockSize = null;
    protected Option [] blockSizeSelections = null;
    protected String textStreams = null;
    private static final int DEFAULT_STREAM_SIZE = 8;
    private static final int DEFAULT_BLOCK_SIZE = 1;
    private static final int MINIMUM_BLOCK_SIZE = 1;
    private static final int MAXIMUM_BLOCK_SIZE = 16;

    /** Holds information in review selections step. */
    protected String summarySelectStorage = null;
    protected String summaryMethod = null;
    protected String summarySpecifyDevice = null;
    protected String summaryOptions = null;

    /** Hold wizardEventListener class object. */
    protected WizardEventListener wizardEventListener = null;
    protected WizardEventListener wizardStepEventListener = null;

    public ShrinkFSBean() {
        initWizard();
    }

    // Remote Calls

    private DiskCache [] getAllocUnits() throws SamFSException {
        TraceUtil.trace3("REMOTE CALL: Getting alloc units from fs!");

        FileSystem fs = getFileSystem();
        if (fs == null) {
            TraceUtil.trace1("fs is null, populateData()!");
            throw new SamFSException(null, -1000);
        }

        // Show file system devices only if they are allocatable
        return fs.getAllDevices(true);
    }

    private DiskCache [] getAvailUnits() throws SamFSException {
        // Only calls to the backend if we know the step is going to render
        if (!selectedMethodMove) {
            return new DiskCache[0];
        }

        TraceUtil.trace3("REMOTE CALL: Getting avail units from fs!");

        SamQFSSystemModel sysModel = SamUtil.getModel(JSFUtil.getServerName());
        if (sysModel == null) {
            throw new SamFSException(null, -2501);
        }
        return
            sysModel.getSamQFSSystemFSManager().
                discoverAvailableAllocatableUnits(null);
    }

    private boolean isFSSharedMD() {
        // To determine if we need to show additional instruction to user and
        // remind him/her to select a replaceable device that can be accessed
        // by all participating shared members.
        try {
            FileSystem fs = getFileSystem();
            if (fs == null) {
                TraceUtil.trace1("fs is null, populateData()!");
                throw new SamFSException(null, -1000);
            }
            return fs.getShareStatus() == FileSystem.SHARED_TYPE_MDS;
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "isFSSharedMD()",
                "Failed to determine the shared fs status!",
                JSFUtil.getServerName());
            TraceUtil.trace1(
                "Failed to determine the shared fs status!", samEx);
        }
        return false;
    }

    // Exclude Device Step

    public TableRowGroup getExcludeTableRowGroup() {
        return excludeTableRowGroup;
    }

    public void setExcludeTableRowGroup(TableRowGroup excludeTableRowGroup) {
        this.excludeTableRowGroup = excludeTableRowGroup;
    }

    public String getTableTitleExclude() {
        return tableTitleExclude;
    }

    public void setTableTitleExclude(String tableTitleExclude) {
        this.tableTitleExclude = tableTitleExclude;
    }

    public Select getSelectExclude() {
        return selectExclude;
    }

    public void setSelectExclude(Select selectExclude) {
        this.selectExclude = selectExclude;
    }

    public String getSelectedStripedGroup() {
        return selectedStripedGroup;
    }

    public void setSelectedStripedGroup(String selectedStripedGroup) {
        this.selectedStripedGroup = selectedStripedGroup;
    }

    public TableDataProvider getExcludeSummaryList() {
        if (allocUnits == null) {
            try {
                allocUnits = getAllocUnits();
            } catch (SamFSException samEx) {
                samEx.printStackTrace();
                TraceUtil.trace1(
                    "Failed to retrieve device information of file system " +
                    getFSName(), samEx);
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                        "fs.shrink.selectstorage.populate.failed",
                        getFSName()),
                    samEx.getMessage());
                allocUnits = new DiskCache[0];
            }
        }
        return new ObjectArrayDataProvider(allocUnits);
    }

    /**
     * Retrieve the selected row keys form the table
     * @return the eq of the selected device
     */
    protected int getSelectedPathExclude() {
        int selectedEQ = -1;
        TableDataProvider provider = getExcludeSummaryList();
        RowKey[] rows = getExcludeTableRowGroup().getSelectedRowKeys();

        if (rows != null && rows.length > 0) {
            // Get device display string and populate review selection page
            // field
            FieldKey field = provider.getFieldKey("devicePathDisplayString");
            summarySelectStorage = (String) provider.getValue(field, rows[0]);

            // Get the EQ of the selected device
            field = provider.getFieldKey("equipOrdinal");
            Integer selected = (Integer) provider.getValue(field, rows[0]);
            // preserve EQ for method return value
            selectedEQ = selected.intValue();

            TraceUtil.trace3(
                "getSelectedPathExclude EQ: " + selected.intValue());

            // Check disk cache type and determine if "release" radio button
            // needs to be rendered
            field = provider.getFieldKey("diskCacheType");
            selected = (Integer) provider.getValue(field, rows[0]);
            TraceUtil.trace3(
                "getSelectedPathExclude dc: " + selected.intValue());

            if (DiskCache.STRIPED_GROUP == selected.intValue()) {
                field = provider.getFieldKey("devicePath");
                selectedStripedGroup =
                    (String) provider.getValue(field, rows[0]);

                // a backdoor way to save the number of members in a striped
                // group
                numberOfMembers = summarySelectStorage.split("<br>").length - 1;
            TraceUtil.trace3(
                "getSelectedPathExclude: # of members: " + numberOfMembers);
            } else {
                selectedStripedGroup = null;
            }

            // Skip method step if striped group is selected
            updateWizardSteps(selected.intValue());

            // save the selection
            getSelectExclude().setKeepSelected(true);
        } else {
            summarySelectStorage = null;
        }

        return selectedEQ;
    }

    private void updateWizardSteps(int dcType) {
        // do not render the method step if striped group is selected
        if (DiskCache.STRIPED_GROUP == dcType) {
            renderSubStepMethod = false;
            selectedMethodMove = false;
            selectedMethodRelease = false;
            selectedMethodDistribute = true;
        } else {
            // Release option in method step only shows up if the file system is
            // an archiving file system, and the selected device must not be a
            // a striped group.  No metadata device can be shrunk.
            if (archive) {
                renderSubStepMethod = true;
                renderedMethodRelease = true;
                selectedMethodRelease = true;
                selectedMethodDistribute = false;
                selectedMethodMove = false;
            } else {
                renderSubStepMethod = false;
                renderedMethodRelease = false;
                selectedMethodRelease = false;
                selectedMethodDistribute = true;
                selectedMethodMove = false;
            }
        }
    }

    // Specify Method Radio Button Step

    public boolean isRenderedMethodDistribute() {
        return renderedMethodDistribute;
    }

    public void setRenderedMethodDistribute(boolean renderedMethodDistribute) {
        this.renderedMethodDistribute = renderedMethodDistribute;
    }

    public boolean isRenderedMethodMove() {
        return renderedMethodMove;
    }

    public void setRenderedMethodMove(boolean renderedMethodMove) {
        this.renderedMethodMove = renderedMethodMove;
    }

    public boolean isRenderSubStepMethod() {
        return renderSubStepMethod;
    }

    public void setRenderSubStepMethod(boolean renderSubStepMethod) {
        this.renderSubStepMethod = renderSubStepMethod;
    }

    public boolean isRenderedMethodRelease() {
        return renderedMethodRelease;
    }

    public void setRenderedMethodRelease(boolean renderedMethodRelease) {
        this.renderedMethodRelease = renderedMethodRelease;
    }

    public boolean isSelectedMethodDistribute() {
        return selectedMethodDistribute;
    }

    public void setSelectedMethodDistribute(boolean selectedMethodDistribute) {
        this.selectedMethodDistribute = selectedMethodDistribute;
    }

    public boolean isSelectedMethodMove() {
        return selectedMethodMove;
    }

    public void setSelectedMethodMove(boolean selectedMethodMove) {
        this.selectedMethodMove = selectedMethodMove;
    }

    public boolean isSelectedMethodRelease() {
        return selectedMethodRelease;
    }

    public void setSelectedMethodRelease(boolean selectedMethodRelease) {
        this.selectedMethodRelease = selectedMethodRelease;
    }

    // Step 2.1 Specify device to store data step
    public TableRowGroup getAvailableTableRowGroup() {
        return availableTableRowGroup;
    }

    public void setAvailableTableRowGroup(
        TableRowGroup availableTableRowGroup) {
        this.availableTableRowGroup = availableTableRowGroup;
    }

    public Select getSelectAvailable() {
        return selectAvailable;
    }

    public void setSelectAvailable(Select selectAvailable) {
        this.selectAvailable = selectAvailable;
    }

    public TableDataProvider getAvailableSummaryList() {
        if (availUnits == null || availUnits.length == 0) {
            try {
                availUnits = getAvailUnits();
            } catch (SamFSException samEx) {
                samEx.printStackTrace();
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                        "common.specifydevice.failed"),
                    samEx.getMessage());
                availUnits = new DiskCache[0];
            }
        }
        return new ObjectArrayDataProvider(availUnits);
    }

    /**
     * Retrieve the selected row keys form the table
     * @return the replacement path
     */
    protected String [] getSelectedReplacePath() {
        TableDataProvider provider = getAvailableSummaryList();
        RowKey[] rows = getAvailableTableRowGroup().getSelectedRowKeys();

        if (rows != null && rows.length > 0) {
            // save the selection
            getSelectAvailable().setKeepSelected(true);

            StringBuffer selectedPath = new StringBuffer();
            StringBuffer summaryStr = new StringBuffer();

            for (int i = 0; i < rows.length; i++) {
                if (i > 0) {
                    selectedPath.append("###");
                    summaryStr.append("<br>");
                }
                FieldKey field = provider.getFieldKey("devicePath");
                selectedPath.append(
                    (String) provider.getValue(field, rows[i]));

                field = provider.getFieldKey("devicePathDisplayString");
                summaryStr.append(
                    (String) provider.getValue(field, rows[i]));
            }
            summarySpecifyDevice = summaryStr.toString();
            return selectedPath.toString().split("###");

        } else {
            summarySpecifyDevice = null;
        }
        return null;
    }

    public String getShrinkSharedFSText() {
        shrinkSharedFSText =
            isFSSharedMD() ?
                JSFUtil.getMessage("fs.selectedevice.sharedfsreplace") :
                "";
        return shrinkSharedFSText;
    }

    public void setShrinkSharedFSText(String shrinkSharedFSText) {
        this.shrinkSharedFSText = shrinkSharedFSText;
    }

    // Step 3: Specify Shrink Options

    public boolean isSelectedOptionCustom() {
        return selectedOptionCustom;
    }

    public void setSelectedOptionCustom(boolean selectedOptionCustom) {
        this.selectedOptionCustom = selectedOptionCustom;
    }

    public boolean isSelectedOptionDefault() {
        return selectedOptionDefault;
    }

    public void setSelectedOptionDefault(boolean selectedOptionDefault) {
        this.selectedOptionDefault = selectedOptionDefault;
    }

    public String getTextLogFile() {
        if (textLogFile == null || textLogFile.length() == 0) {
            textLogFile =
                DEFAULT_LOG_FILE_LOCATION_PREFIX + getFSName() +
                DEFAULT_LOG_FILE_LOCATION_SUFFIX;
        }
        return textLogFile;
    }

    public void setTextLogFile(String textLogFile) {
        this.textLogFile = textLogFile;
    }

    public void validateLogFile(
        FacesContext context, UIComponent component, Object value)
        throws ValidatorException{

        String errMsg = null;

        if (((String) value).length() == 0) {
            errMsg =
                JSFUtil.getMessage("fs.shrink.options.logfile.required");
        } else if (!SamUtil.isValidNonSpecialCharString((String) value)) {
            errMsg =
                JSFUtil.getMessage("fs.shrink.options.logfile.invalid");
        } else {
            clearAlertInfo();
        }

        if (errMsg != null) {
            setAlertInfo(Constants.Alert.ERROR, errMsg, null);
            FacesMessage message = new FacesMessage();
            message.setDetail(errMsg);
            message.setSeverity(FacesMessage.SEVERITY_ERROR);
            throw new ValidatorException(message);
        }
    }

    public boolean isDisplayName() {
        return displayName;
    }

    public void setDisplayName(boolean displayName) {
        this.displayName = displayName;
    }

    public boolean isDryRun() {
        return dryRun;
    }

    public void setDryRun(boolean dryRun) {
        this.dryRun = dryRun;
    }

    public boolean isStageBack() {
        return stageBack;
    }

    public void setStageBack(boolean stageBack) {
        this.stageBack = stageBack;
    }

    public boolean isStagePartialBack() {
        return stagePartialBack;
    }

    public void setStagePartialBack(boolean stagePartialBack) {
        this.stagePartialBack = stagePartialBack;
    }

    public Option[] getBlockSizeSelections() {
        if (blockSizeSelections == null) {
            blockSizeSelections = createBlockSizeMenu();
        }
        return blockSizeSelections;
    }

    public void setBlockSizeSelections(Option[] blockSizeSelections) {
        this.blockSizeSelections = blockSizeSelections;
    }

    public String getSelectedBlockSize() {
        return selectedBlockSize;
    }

    public void setSelectedBlockSize(String selectedBlockSize) {
        this.selectedBlockSize = selectedBlockSize;
    }

    public String getTextStreams() {
        if (textStreams == null || textStreams.length() == 0) {
            textStreams = Integer.toString(DEFAULT_STREAM_SIZE);
        }
        return textStreams;
    }

    public void setTextStreams(String textStreams) {
        this.textStreams = textStreams;
    }

    /**
     * Create a drop down menu with 1 to 16
     * @return an array of option for block size menu
     */
    private Option[] createBlockSizeMenu() {
        Option[] menuOptions =
            new Option[MAXIMUM_BLOCK_SIZE - MINIMUM_BLOCK_SIZE + 1];
        int j = 0;
        for (int i = MINIMUM_BLOCK_SIZE; i <= MAXIMUM_BLOCK_SIZE; i++, j++) {
            menuOptions[j] =
                new Option(Integer.toString(i), Integer.toString(i));
        }
        return menuOptions;
    }

    // Step 4: Review Selections

    public String getSummaryMethod() {
        // Detect the selection in step 2, and set the correct summary
        // If stripped group is being shrunk and step 2 does not exist in the
        // wizard flow, set to "move data to a new device".
        if (selectedMethodRelease) {
            summaryMethod =
                JSFUtil.getMessage("fs.shrink.method.radio.release");
        } else if (selectedMethodDistribute) {
            summaryMethod =
                JSFUtil.getMessage("fs.shrink.method.radio.distribute");
        } else {
            summaryMethod =
                JSFUtil.getMessage("fs.shrink.method.radio.move");
        }
        return summaryMethod;
    }

    public void setSummaryMethod(String summaryMethod) {
        this.summaryMethod = summaryMethod;
    }

    public String getSummaryOptions() {
        if (selectedOptionDefault) {
            summaryOptions =
                JSFUtil.getMessage("fs.shrink.options.radio.default");
        } else {
            summaryOptions =
                JSFUtil.getMessage("fs.shrink.options.radio.custom");
        }
        return summaryOptions;
    }

    public void setSummaryOptions(String summaryOptions) {
        this.summaryOptions = summaryOptions;
    }

    public String getSummarySelectStorage() {
        return summarySelectStorage;
    }

    public void setSummarySelectStorage(String summarySelectStorage) {
        this.summarySelectStorage = summarySelectStorage;
    }

    public String getSummarySpecifyDevice() {
        return summarySpecifyDevice;
    }

    public void setSummarySpecifyDevice(String summarySpecifyDevice) {
        this.summarySpecifyDevice = summarySpecifyDevice;
    }

    // Alert
    public String getAlertDetail() {
        if (!alertRendered) {
            alertDetail = null;
        }
        return alertDetail;
    }

    public void setAlertDetail(String alertDetail) {
        this.alertDetail = alertDetail;
    }

    public String getAlertSummary() {
        if (!alertRendered) {
            alertSummary = null;
        }
        return alertSummary;
    }

    public void setAlertSummary(String alertSummary) {
        this.alertSummary = alertSummary;
    }

    public String getAlertType() {
        if (!alertRendered) {
            alertType = Constants.Alert.INFO;
        }
        return alertType;
    }

    public void setAlertType(String alertType) {
        this.alertType = alertType;
    }

    public boolean isAlertRendered() {
        return alertRendered;
    }

    public void setAlertRendered(boolean alertRendered) {
        this.alertRendered = alertRendered;
    }

    public void setAlertInfo(String type, String summary, String detail) {
        alertRendered = true;
        this.alertType = type;
        this.alertSummary = summary;
        this.alertDetail = JSFUtil.getMessage(detail);
    }


    public void clearAlertInfo() {
        alertRendered = false;
        this.alertType = null;
        this.alertSummary = null;
        this.alertDetail = null;
    }


    // Helper methods
    private String getFSName() {
        return FSUtil.getFSName();
    }

    private FileSystem getFileSystem() throws SamFSException {
        return
            SamUtil.getModel(JSFUtil.getServerName()).
                getSamQFSSystemFSManager().getFileSystem(getFSName());
    }

    private void initWizard() {
        clearAlertInfo();

        archive = false;
        eqToShrink = -1;
        replacementPaths = null;

        tableTitleExclude = null;
        excludeTableRowGroup = null;
        selectExclude = new Select("excludeTable");
        allocUnits = null;
        selectedStripedGroup = null;
        getSelectExclude().clear();

        renderSubStepMethod = false;
        renderedMethodRelease = true;
        renderedMethodDistribute = true;
        renderedMethodMove = true;
        selectedMethodRelease = false;
        selectedMethodDistribute = false;
        selectedMethodMove = false;

        availableTableRowGroup = null;
        selectAvailable = new Select("availableTable");
        availUnits = null;
        getSelectAvailable().clear();

        textLogFile = null;
        selectedOptionDefault = false;
        selectedOptionCustom = false;
        displayName = false;
        dryRun = false;
        stageBack = false;
        stagePartialBack = false;
        selectedBlockSize = null;
        blockSizeSelections = null;
        textStreams = null;

        summarySelectStorage = null;
        summaryMethod = null;
        summarySpecifyDevice = null;
        summaryOptions = null;

        tableTitleExclude =
            JSFUtil.getMessage(
                "fs.shrink.selectstorage.tabletitle", getFSName());

        try {
            FileSystem myFS = getFileSystem();
            archive = myFS.getArchivingType() == FileSystem.ARCHIVING;
        } catch (SamFSException samEx) {
            archive = false;
            TraceUtil.trace1("Error getting archive type!", samEx);
        }

        // Step 1.2: Default value to use default settings
        selectedOptionDefault = true;
    }


    // Wizard Impl Control

    public WizardEventListener getWizardEventListener() {
        if (wizardEventListener == null) {
            wizardEventListener = new ShrinkFSWizardEventListener();
        }
        return wizardEventListener;
    }

    public WizardEventListener getWizardStepEventListener() {
        if (wizardStepEventListener == null) {
            wizardStepEventListener = new ShrinkFSWizardEventListener();
        }
        return wizardStepEventListener;
    }

    class ShrinkFSWizardEventListener implements WizardEventListener {

        public boolean handleEvent(WizardEvent event) {
            TraceUtil.trace3("handleEvent called! " + event.getEvent());

            switch (event.getEvent()) {
                case WizardEvent.START:
                case WizardEvent.COMPLETE:
                case WizardEvent.CLOSE:
                case WizardEvent.CANCEL:
                    initWizard();
                    break;
                case WizardEvent.NEXT:
                    return handleNextButton(event);
                case WizardEvent.PREVIOUS:
                    clearAlertInfo();
                    break;
                case WizardEvent.FINISH:
                    return handleFinishButton(event);
                case WizardEvent.STEPSTAB:
                case WizardEvent.GOTOSTEP:
                    clearAlertInfo();
                    break;
                case WizardEvent.HELPTAB:
                case WizardEvent.NOEVENT:
                    break;
            }
            return true;
        }

        public void setTransient(boolean transientFlag) {

        }

        public Object saveState(FacesContext context) {
            return null;
        }

        public void restoreState(FacesContext context, Object state) {

        }

        public boolean isTransient() {
            return false;
        }

        private boolean handleNextButton(WizardEvent event) {
            WizardStep step = event.getStep();
            String id = step.getId();

            if (STEP_SELECT_STORAGE.equals(id)) {
                return handleSelectStorage(event);
            } else if (STEP_METHOD.equals(id)) {
                return handleMethod(event);
            } else if (STEP_SPECIFY_DEVICE.equals(id)) {
                return handleSpecifyDevice(event);
            } else if (STEP_OPTIONS.equals(id)) {
                // no op
            } else if (STEP_REVIEW.equals(id)) {
                // no op
            }
            return true;
        }

        private boolean handleFinishButton(WizardEvent event) {
            String messageSummary = null;
            String messageDetails = null;
            FileSystem myFS = null;

            try {
                // Check permission
                if (!SecurityManagerFactory.getSecurityManager().
                    hasAuthorization(Authorization.CONFIG)) {
                    throw new SamFSException("common.nopermission");
                }

                myFS = getFileSystem();

                ShrinkOption options =
                    selectedOptionCustom ?
                        new ShrinkOption(
                            textLogFile,
                            Integer.parseInt(selectedBlockSize),
                            displayName,
                            dryRun,
                            Integer.parseInt(textStreams),
                            stageBack,
                            stagePartialBack) :
                        new ShrinkOption(
                            textLogFile,
                            DEFAULT_BLOCK_SIZE,
                            false,
                            false,
                            DEFAULT_STREAM_SIZE,
                            false,
                            false);

                // OK message
                messageSummary = JSFUtil.getMessage(
                                    "fs.shrink.ok",
                                    new String [] {
                                        getFSName()});

                // Shrink Release
                if (selectedMethodRelease) {
                    messageDetails = JSFUtil.getMessage(
                                "fs.shrink.release.ok.details",
                                new String [] {
                                    summarySelectStorage});
                    myFS.shrinkRelease(eqToShrink, options);

                // Shrink Remove
                } else if (selectedMethodDistribute) {
                    messageDetails = JSFUtil.getMessage(
                                "fs.shrink.remove.ok.details",
                                new String [] {
                                    summarySelectStorage});
                    myFS.shrinkRemove(eqToShrink, -1, options);

                // Should never come to this case because shrink replace
                // has been scoped out in 5.0
                // Shrink & Replace with newly discovered device
                } else {
                    DiskCache [] replacement = getReplacementPathObj();
                    // non-striped group replacement
                    if (selectedStripedGroup == null) {
                        messageDetails = JSFUtil.getMessage(
                                "fs.shrink.replace.ok.details",
                                new String [] {
                                    summarySelectStorage,
                                    replacementPaths[0]});
                        myFS.shrinkReplaceDev(
                            eqToShrink,
                            replacement[0],
                            options);
                    } else {
                        messageDetails = JSFUtil.getMessage(
                                "fs.shrink.replace.stripedgroup.ok.details",
                                new String [] {
                                    "<br>".concat(
                                        summarySelectStorage).concat("<br>"),
                                    "<br>&nbsp;&nbsp;&nbsp;&nbsp;".concat(
                                        ConversionUtil.arrayToStr(
                                            replacementPaths,
                                            "<br>&nbsp;&nbsp;&nbsp;&nbsp;"))});
                        myFS.shrinkReplaceGroup(
                            eqToShrink,
                            new StripedGroupImpl("", replacement),
                            options);
                    }
                }
                setAlertInfo(
                    Constants.Alert.INFO, messageSummary, messageDetails);

            } catch (SamFSException samEx) {
                TraceUtil.trace1("SamFSException caught!", samEx);
                LogUtil.error(this, samEx);
                samEx.printStackTrace();
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "handleFinishButton",
                    samEx.getMessage(),
                    JSFUtil.getServerName());
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage("fs.shrink.failed", getFSName()),
                    samEx.getMessage());
            }
            return true;
        }

        private boolean handleSelectStorage(WizardEvent event) {
            eqToShrink = getSelectedPathExclude();

            // User has to select at least one device
            if (-1 == eqToShrink && null == selectedStripedGroup) {
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage("common.specifydevice.error"),
                    null);
                return false;
            }
            try {
                if (isLastDevice(eqToShrink)) {
                    setAlertInfo(
                        Constants.Alert.ERROR,
                        JSFUtil.getMessage("fs.shrink.lastdevice"),
                        null);
                    return false;
                }
            } catch (SamFSException samEx) {
                TraceUtil.trace1(
                    "Exception Caught in handleSelectStorage: ", samEx);
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage("fs.shrink.readerror", getFSName()),
                    null);
                return false;
            }

            // No error, clear alert
            clearAlertInfo();

            return true;
        }

        private boolean handleMethod(WizardEvent event) {
            // reset summary specify device
            if (!selectedMethodMove) {
                summarySpecifyDevice = null;
            }
            return true;
        }

        private boolean handleSpecifyDevice(WizardEvent event) {
            replacementPaths = getSelectedReplacePath();

            // User has to select at least one device
            if (replacementPaths == null || replacementPaths.length == 0) {
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage("common.specifydevice.error"),
                    null);
                return false;
            } else {
                clearAlertInfo();
            }

            if (!checkOverlapLuns()) {
                return false;
            } else if (!validateStripedGroupReplacement()) {
                return false;
            } else {
                clearAlertInfo();
                return true;
            }
        }

        private boolean checkOverlapLuns() {
            String [] overlapLUNs = null;
            try {
                SamQFSSystemModel sysModel =
                    SamUtil.getModel(JSFUtil.getServerName());
                if (sysModel == null) {
                    throw new SamFSException(null, -2501);
                }

                overlapLUNs = sysModel.getSamQFSSystemFSManager().
                                  checkSlicesForOverlaps(replacementPaths);
            } catch (SamFSException ex) {
                TraceUtil.trace1(
                    "Failed to check if selected devices are overlapped!", ex);
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                        "common.specifydevice.checkoverlap.failed"),
                    ex.getMessage());
                return false;
            }

            if (null == overlapLUNs || overlapLUNs.length == 0) {
                return true;
            } else {
                StringBuffer badLUNs = new StringBuffer();
                for (int i = 0; i < overlapLUNs.length; i++) {
                    if (i > 0) {
                        badLUNs.append("<br>");
                    }
                    badLUNs.append(overlapLUNs[i]);
                }
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage("FSWizard.error.overlapDataLUNs"),
                    badLUNs.toString());
                return false;
            }
        }

        private boolean validateStripedGroupReplacement() {
            // Not a striped group
            if (-1 == numberOfMembers) {
                return true;
            }

            // error if number of devices of replacement group doesn't match
            // the number of members of striped group to be shrunk
            if (numberOfMembers != replacementPaths.length) {
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                        "fs.shrink.replace.stripedgroup.error",
                        Integer.toString(numberOfMembers)),
                    null);
                return false;
            } else {
                return true;
            }
        }

        private DiskCache [] getReplacementPathObj() throws SamFSException {
            boolean found = false;
            DiskCache [] targetDc = new DiskCache[replacementPaths.length];
            for (int i = 0; i < replacementPaths.length; i++) {
                for (int j = 0; j < availUnits.length; j++) {
                    if (availUnits[j].getDevicePath().
                        equals(replacementPaths[i])) {
                        targetDc[i] = availUnits[j];
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                throw new SamFSException(
                    "Developer bug found in getReplacementPathObj!");
            }
            return targetDc;
        }

        private boolean isLastDevice(int eqToShrink) throws SamFSException {
            FileSystem thisFS = getFileSystem();
            return thisFS.isLastActiveDevice(eqToShrink);
        }
    }
}
