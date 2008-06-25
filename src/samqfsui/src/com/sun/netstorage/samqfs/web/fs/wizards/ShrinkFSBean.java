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

// ident        $Id: ShrinkFSBean.java,v 1.1 2008/06/25 21:03:58 ronaldso Exp $

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
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.Select;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.component.TableRowGroup;
import com.sun.web.ui.component.WizardStep;
import com.sun.web.ui.event.WizardEvent;
import com.sun.web.ui.event.WizardEventListener;
import com.sun.web.ui.model.Option;
import java.io.Serializable;
import javax.faces.component.UIComponent;
import javax.faces.context.FacesContext;

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

    /** Holds value of alert info. */
    protected boolean alertRendered = false;
    protected String alertType = null;
    protected String alertDetail = null;
    protected String alertSummary = null;

    /** Holds table information in exclude device page. */
    protected String tableTitleExclude = null;
    protected TableRowGroup excludeTableRowGroup = null;
    protected Select selectExclude = new Select("excludeTable");
    /** Holds the disk cache information of the tables. */
    private DiskCache [] allocUnits = null;

    /** Holds the radio button information of specify method step. */
    protected boolean renderedMethodRelease = true;
    protected boolean renderedMethodDistribute = true;
    protected boolean renderedMethodMove = true;
    protected boolean selectedMethodRelease = false;
    protected boolean selectedMethodDistribute = false;
    protected boolean selectedMethodMove = false;

    /** Holds table information in specify device page. */
    protected boolean renderedSubStep = true;
    protected TableRowGroup availableTableRowGroup = null;
    protected Select selectAvailable = new Select("availableTable");
    private DiskCache [] availUnits = null;

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
    protected String selectedBlockSize = null;
    protected Option [] blockSizeSelections = null;
    protected String textStreams = null;
    private static final int DEFAULT_STREAM_SIZE = 8;
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
System.out.println("Entering ShrinkFSBean()!");

        tableTitleExclude =
            JSFUtil.getMessage(
                "fs.shrink.selectstorage.tabletitle", getFSName());

        // Step 2: Default value set to release
        selectedMethodRelease = true;

        // Step 3: Default value to use default settings
        selectedOptionDefault = true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Remote Calls
    private DiskCache [] getAllocUnits() throws SamFSException {
System.out.println("Getting alloc units from fs!");
        FileSystem fs = getFileSystem();
        if (fs == null) {
            TraceUtil.trace1("fs is null, populateData()!");
            throw new SamFSException(null, -1000);
        }

        return fs.getDataDevices();
    }

    private DiskCache [] getAvailUnits() throws SamFSException {
        SamQFSSystemModel sysModel = SamUtil.getModel(JSFUtil.getServerName());
        if (sysModel == null) {
            throw new SamFSException(null, -2501);
        }
        return
            sysModel.getSamQFSSystemFSManager().
                discoverAvailableAllocatableUnits(null);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Exclude Device Step
    public TableRowGroup getExcludeTableRowGroup() {
        return excludeTableRowGroup;
    }

    public void setExcludeTableRowGroup(TableRowGroup excludeTableRowGroup) {
        this.excludeTableRowGroup = excludeTableRowGroup;
    }

    public Select getSelectExclude() {
        return selectExclude;
    }

    public void setSelectExclude(Select selectExclude) {
        this.selectExclude = selectExclude;
    }

    public String getTableTitleExclude() {
        return tableTitleExclude;
    }

    public void setTableTitleExclude(String tableTitleExclude) {
        this.tableTitleExclude = tableTitleExclude;
    }

    public TableDataProvider getExcludeSummaryList() {
        if (allocUnits == null) {
            try {
                allocUnits = getAllocUnits();
            } catch (SamFSException samEx) {
                samEx.printStackTrace();
                // TODO: set error
            }
        }
        return new ObjectArrayDataProvider(allocUnits);
    }

    /**
     * Retrieve the selected row keys form the table
     * @param alloc true if used for selecting allocated device to shrink
     * @return an array of Strings that hold the keys of selected rows
     */
    protected String getSelectedKey(boolean alloc) {
        TableDataProvider provider =
            alloc ? getExcludeSummaryList() : getAvailableSummaryList();
        RowKey[] rows =
            alloc ?
                getExcludeTableRowGroup().getSelectedRowKeys() :
                getAvailableTableRowGroup().getSelectedRowKeys();
        String selected = null;
System.out.println("getSelectedKey(" + alloc + ") is called!");
        if (rows != null && rows.length > 0) {
System.out.println("inside if case! selected:");
            FieldKey field = provider.getFieldKey("devicePath");
            selected = (String) provider.getValue(field, rows[0]);
        }

        // safe to clear the selection
/*
        if (alloc) {
            getSelectExclude().clear();
        } else {
            getSelectAvailable().clear();
        }
 */

        return selected;
    }

    ////////////////////////////////////////////////////////////////////////////
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

    ////////////////////////////////////////////////////////////////////////////
    // Step 2.1 Specify device to store data step
    public TableRowGroup getAvailableTableRowGroup() {
        return availableTableRowGroup;
    }

    public void setAvailableTableRowGroup(TableRowGroup availableTableRowGroup) {
        this.availableTableRowGroup = availableTableRowGroup;
    }

    public Select getSelectAvailable() {
        return selectAvailable;
    }

    public void setSelectAvailable(Select selectAvailable) {
        this.selectAvailable = selectAvailable;
    }

    public TableDataProvider getAvailableSummaryList() {
        if (availUnits == null) {
            try {
                availUnits = getAvailUnits();
            } catch (SamFSException samEx) {
                samEx.printStackTrace();
                // TODO: set error
            }
        }
        return new ObjectArrayDataProvider(availUnits);
    }

    public boolean isRenderedSubStep() {
        return renderedSubStep;
    }

    public void setRenderedSubStep(boolean renderedSubStep) {
        this.renderedSubStep = renderedSubStep;
    }

    ////////////////////////////////////////////////////////////////////////////
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
        FacesContext context, UIComponent component, Object value) {

        // TODO:
        // Check empty spaces
        // Check illegal characters
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

    ////////////////////////////////////////////////////////////////////////////
    // Step 4: Review Selections

    public String getSummaryMethod() {
        // Detect the selection in step 2, and set the correct summary
        // If stripped group is being shrinked and step 2 does not exist in the
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
        summarySelectStorage = getSelectedKey(true);
        return summarySelectStorage;
    }

    public void setSummarySelectStorage(String summarySelectStorage) {
        this.summarySelectStorage = summarySelectStorage;
    }

    public String getSummarySpecifyDevice() {
        summarySpecifyDevice = getSelectedKey(false);
        return summarySpecifyDevice;
    }

    public void setSummarySpecifyDevice(String summarySpecifyDevice) {
        this.summarySpecifyDevice = summarySpecifyDevice;
    }

    ////////////////////////////////////////////////////////////////////////////
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
        System.out.println("Calling isAlertRendered: " + alertRendered);
        return alertRendered;
    }

    public void setAlertRendered(boolean alertRendered) {
        this.alertRendered = alertRendered;
    }

    public void setAlertInfo(String type, String summary, String detail) {
        System.out.println("calling setAlertInfo!");
        alertRendered = true;
        this.alertType = type;
        this.alertSummary = summary;
        this.alertDetail = detail;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Helper methods
    private String getFSName() {
        return FSUtil.getFSName();
    }

    private FileSystem getFileSystem() throws SamFSException {
        String fsName = getFSName();
        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        if (appModel == null) {
            throw new SamFSException("App model is null");
        }
        SamQFSSystemModel sysModel = SamUtil.getModel(JSFUtil.getServerName());
        return sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
    }

    ////////////////////////////////////////////////////////////////////////////
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
System.out.println("handleEvent called! " + event.getEvent());
            switch (event.getEvent()) {
                case WizardEvent.START:
                    break;
                case WizardEvent.COMPLETE:
                case WizardEvent.CLOSE:
                case WizardEvent.CANCEL:
                    event.getWizard().getChildren().clear();
                    break;
                case WizardEvent.NEXT:
                    return handleNextButton(event);
                case WizardEvent.PREVIOUS:
                    break;
                case WizardEvent.FINISH:
                    return handleFinishButton(event);
                case WizardEvent.HELPTAB:
                case WizardEvent.STEPSTAB:
                case WizardEvent.GOTOSTEP:
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
            System.out.println("NEXT: ID is " + id);
            return true;
        }

        private boolean handleFinishButton(WizardEvent event) {
            setAlertInfo(Constants.Alert.INFO, "Summary", "Details!");
            return true;
        }
    }
}
