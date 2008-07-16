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

// ident        $Id: AddStorageNodeBean.java,v 1.1 2008/07/16 17:09:31 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.data.provider.FieldKey;
import com.sun.data.provider.RowKey;
import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.fs.FSUtil;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.util.Capacity;
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
import javax.faces.application.FacesMessage;
import javax.faces.component.UIComponent;
import javax.faces.context.FacesContext;
import javax.faces.validator.ValidatorException;

public class AddStorageNodeBean implements Serializable {

    /**
     * Step Ids used in AddStorageNodeWizard.jsp.  Make sure the ids match with
     * the JSP page.
     */
    protected static final String STEP_OVERVIEW = "step_id_overview";
    protected static final String STEP_SPECIFY_HOST = "step_id_specify_host";
    protected static final String STEP_SPECIFY_GROUP = "step_id_specify_group";
    protected static final String STEP_CREATE_GROUP = "step_id_create_group";
    protected static final
        String STEP_SPECIFY_META_DEVICE = "step_id_specify_metadevice";
    protected static final
        String STEP_SPECIFY_DATA_DEVICE = "step_id_specify_datadevice";
    protected static final String STEP_REVIEW = "step_id_review";

    /** Holds value of alert info. */
    protected boolean alertRendered = false;
    protected String alertType = null;
    protected String alertDetail = null;
    protected String alertSummary = null;

    /** Holds value of components in Step 2 specify host step. */
    protected String textHostNameIP = null;

    /** Holds value of components in Step 3 specify storage step. */
    protected boolean selectedExistingGroup = true;
    protected boolean selectedCreateGroup = false;
    protected String selectedGroup = null;
    protected Option [] groupSelections = null;

    /** Holds value of components in Step 3.1 create new group step. */
    protected String groupName = null;
    protected boolean selectedEqualSize = false;
    protected boolean selectedSpecifySize = false;
    protected boolean selectedStripedGroup = false;
    protected String textBlockSize = null;
    protected String textBlockPerDevice = null;
    protected String selectedBlockSizeUnit = null;
    protected Option [] blockSizeUnitSelections = null;

    /** Holds table information in Step 4 select metadata device step. */
    protected TableRowGroup metaTableRowGroup = null;
    protected Select selectMeta = new Select("metaTable");
    private DiskCache [] availUnits = null;
    private String [] selectedMetaDevices = null;

    /** Holds table information in Step 5 select data device step. */
    protected TableRowGroup dataTableRowGroup = null;
    protected TableDataProvider dataSummaryList = null;
    protected Select selectData = new Select("dataTable");
    private String [] selectedDataDevices = null;

    /** Holds value of components in Step 6: Review Selections */
    protected String textGroup = null;
    protected String textNewGroupProp = null;
    protected String textSelectedMDevice = null;
    protected String textSelectedDataDevice = null;

    /** Hold wizardEventListener class object. */
    protected WizardEventListener wizardEventListener = null;
    protected WizardEventListener wizardStepEventListener = null;

    /** Default value for block size & block per device */
    private static final int DEFAULT_BLOCK_SIZE = 64;
    private static final int
        DEFAULT_BLOCK_SIZE_UNIT = SamQFSSystemModel.SIZE_KB;
    private static final int DEFAULT_BLOCK_PER_DEVICE = 2;

    public AddStorageNodeBean() {
        initWizard();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Remote Calls
    
    // TODO: Replace with real API call!
    private String [] getGroupNames() {
        return new String [] {
            "Group 1",
            "Group 2"
        };
    }

    private DiskCache [] getAvailUnits() throws SamFSException {
        TraceUtil.trace3("REMOTE CALL: Getting avail units from fs!");

        // TODO: sysModel for the host specified in first step
        SamQFSSystemModel sysModel = SamUtil.getModel(JSFUtil.getServerName());
        if (sysModel == null) {
            throw new SamFSException(null, -2501);
        }
        return
            sysModel.getSamQFSSystemFSManager().
                discoverAvailableAllocatableUnits(null);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Step 2: Specify Host

    public String getTextHostNameIP() {
        return textHostNameIP;
    }

    public void setTextHostNameIP(String textHostNameIP) {
        this.textHostNameIP = textHostNameIP;
    }

    public void validateHostNameIP(
        FacesContext context, UIComponent component, Object value)
        throws ValidatorException{

        String errMsg = null;

        // TODO: validate host name or IP
        clearAlertInfo();

        if (errMsg != null) {
            setAlertInfo(Constants.Alert.ERROR, errMsg, null);
            FacesMessage message = new FacesMessage();
            message.setDetail(errMsg);
            message.setSeverity(FacesMessage.SEVERITY_ERROR);
            throw new ValidatorException(message);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Step 3: Specify Storage Node Group

    public Option[] getGroupSelections() {
        if (null == groupSelections) {
            groupSelections = createGroupSelections();
        }
        return groupSelections;
    }

    public void setGroupSelections(Option[] groupSelections) {
        this.groupSelections = groupSelections;
    }

    public boolean isSelectedCreateGroup() {
        return selectedCreateGroup;
    }

    public void setSelectedCreateGroup(boolean selectedCreateGroup) {
        this.selectedCreateGroup = selectedCreateGroup;
    }

    public boolean isSelectedExistingGroup() {
        return selectedExistingGroup;
    }

    public void setSelectedExistingGroup(boolean selectedExistingGroup) {
        this.selectedExistingGroup = selectedExistingGroup;
    }

    public String getSelectedGroup() {
        return selectedGroup;
    }

    public void setSelectedGroup(String selectedGroup) {
        this.selectedGroup = selectedGroup;
    }

    private Option [] createGroupSelections() {
        String [] groupNames = getGroupNames();
        Option [] groupOption = new Option[groupNames.length];

        for (int i = 0; i < groupNames.length; i++) {
            groupOption[i] = new Option(groupNames[i], groupNames[i]);
        }
        return groupOption;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Step 3.1 Create New Group

    public String getGroupName() {
        groupName =
            JSFUtil.getMessage(
                "addsn.creategroup.label.group",
                new String [] {
                    Integer.toString(getGroupNames().length + 1)});
        return groupName;
    }

    public void setGroupName(String groupName) {
        this.groupName = groupName;
    }

    public boolean isSelectedEqualSize() {
        return selectedEqualSize;
    }

    public void setSelectedEqualSize(boolean selectedEqualSize) {
        this.selectedEqualSize = selectedEqualSize;
    }

    public boolean isSelectedSpecifySize() {
        return selectedSpecifySize;
    }

    public void setSelectedSpecifySize(boolean selectedSpecifySize) {
        this.selectedSpecifySize = selectedSpecifySize;
    }

    public boolean isSelectedStripedGroup() {
        return selectedStripedGroup;
    }

    public void setSelectedStripedGroup(boolean selectedStripedGroup) {
        this.selectedStripedGroup = selectedStripedGroup;
    }

    public String getTextBlockPerDevice() {
        if (textBlockPerDevice == null || textBlockPerDevice.length() == 0) {
            textBlockPerDevice = Integer.toString(DEFAULT_BLOCK_PER_DEVICE);
        }
        return textBlockPerDevice;
    }

    public void setTextBlockPerDevice(String textBlockPerDevice) {
        this.textBlockPerDevice = textBlockPerDevice;
    }

    public String getTextBlockSize() {
        if (textBlockSize == null || textBlockSize.length() == 0) {
            textBlockSize = Integer.toString(DEFAULT_BLOCK_SIZE);
            selectedBlockSizeUnit = Integer.toString(DEFAULT_BLOCK_SIZE_UNIT);
        }
        return textBlockSize;
    }

    public void setTextBlockSize(String textBlockSize) {
        this.textBlockSize = textBlockSize;
    }

    public Option[] getBlockSizeUnitSelections() {
        if (blockSizeUnitSelections == null) {
            blockSizeUnitSelections = createBlockSizeUnitSelections();
        }
        return blockSizeUnitSelections;
    }

    public void setBlockSizeUnitSelections(Option[] blockSizeUnitSelections) {
        this.blockSizeUnitSelections = blockSizeUnitSelections;
    }

    public String getSelectedBlockSizeUnit() {
        // reset unit
        if (!selectedSpecifySize) {
            selectedBlockSizeUnit = Integer.toString(DEFAULT_BLOCK_SIZE_UNIT);
        }
        return selectedBlockSizeUnit;
    }

    public void setSelectedBlockSizeUnit(String selectedBlockSizeUnit) {
        this.selectedBlockSizeUnit = selectedBlockSizeUnit;
    }

    public void validateBlockSize(
        FacesContext context, UIComponent component, Object value)
        throws ValidatorException{

        String errMsg = null;

        // TODO: validate block size
        clearAlertInfo();

        if (errMsg != null) {
            setAlertInfo(Constants.Alert.ERROR, errMsg, null);
            FacesMessage message = new FacesMessage();
            message.setDetail(errMsg);
            message.setSeverity(FacesMessage.SEVERITY_ERROR);
            throw new ValidatorException(message);
        }
    }

    public void validateBlockPerDevice(
        FacesContext context, UIComponent component, Object value)
        throws ValidatorException{

        String errMsg = null;

        // TODO: validate block per device
        clearAlertInfo();

        if (errMsg != null) {
            setAlertInfo(Constants.Alert.ERROR, errMsg, null);
            FacesMessage message = new FacesMessage();
            message.setDetail(errMsg);
            message.setSeverity(FacesMessage.SEVERITY_ERROR);
            throw new ValidatorException(message);
        }
    }

    private Option [] createBlockSizeUnitSelections() {
        String [] labels = SelectableGroupHelper.Sizes.labels;
        String [] values = SelectableGroupHelper.Sizes.values;
        Option [] groupOption = new Option[labels.length];


        for (int i = 0; i < labels.length; i++) {
            groupOption[i] =
                new Option(
                    values[i],
                    JSFUtil.getMessage(labels[i]));
        }
        return groupOption;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Step 4: Select devices for metadata storage

    public Select getSelectMeta() {
        return selectMeta;
    }

    public void setSelectMeta(Select selectMeta) {
        this.selectMeta = selectMeta;
    }

    public TableRowGroup getMetaTableRowGroup() {
        return metaTableRowGroup;
    }

    public void setMetaTableRowGroup(TableRowGroup metaTableRowGroup) {
        this.metaTableRowGroup = metaTableRowGroup;
    }

    public TableDataProvider getMetaSummaryList() {
        if (availUnits == null) {
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
    protected String [] getSelectedDevices(boolean meta) {
        TableDataProvider provider =
            meta ?
                getMetaSummaryList() :
                getDataSummaryList();
        RowKey[] rows =
            meta ?
                getMetaTableRowGroup().getSelectedRowKeys() :
                getDataTableRowGroup().getSelectedRowKeys();

        if (rows != null && rows.length > 0) {
            // save the selection
            if (meta) {
                getSelectMeta().setKeepSelected(true);
            } else {
                getSelectData().setKeepSelected(true);
            }

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
            return selectedPath.toString().split("###");
        }
        return null;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Step 5: Select devices for data storage

    public Select getSelectData() {
        return selectData;
    }

    public void setSelectData(Select selectData) {
        this.selectData = selectData;
    }

    public TableRowGroup getDataTableRowGroup() {
        return dataTableRowGroup;
    }

    public void setDataTableRowGroup(TableRowGroup dataTableRowGroup) {
        this.dataTableRowGroup = dataTableRowGroup;
    }


    public TableDataProvider getDataSummaryList() {
        selectedMetaDevices = selectedMetaDevices == null ?
                                new String[0] : selectedMetaDevices;

        DiskCache [] dataDevices =
            availUnits.length == 0  ||
            availUnits.length - selectedMetaDevices.length == 0 ?
                 // shouldn't happen
                new DiskCache[0] :
                new DiskCache[availUnits.length - selectedMetaDevices.length];
        int k = 0;
        boolean used = false;
        for (int i = 0; i < availUnits.length; i++) {
            for (int j = 0; j < selectedMetaDevices.length; j++) {
                if (selectedMetaDevices[j].equals(
                    availUnits[i].getDevicePath())) {
                    used = true;
                    // break;
                }
            }
            if (!used) {
               dataDevices[k++] = availUnits[i];
            } else {
                // reset
                used = false;
            }
        }

        dataSummaryList = new ObjectArrayDataProvider(dataDevices);
        return dataSummaryList;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Step 6: Review Selections

    public String getTextGroup() {
        if (selectedExistingGroup) {
            // Use existing group
            textGroup = selectedGroup;
        } else {
            // Create new group
            textGroup =
                JSFUtil.getMessage(
                    "addsn.creategroup.label.summary.group",
                    new String [] {
                        Integer.toString(getGroupNames().length + 1),
                        JSFUtil.getMessage("addsn.specifygroup.label.create")
                    });

        }
        return textGroup;
    }

    public void setTextGroup(String textGroup) {
        this.textGroup = textGroup;
    }

    public String getTextNewGroupProp() {
        if (selectedCreateGroup) {
            if (selectedEqualSize) {
                textNewGroupProp =
                    JSFUtil.getMessage("addsn.creategroup.method.equalsize");
            } else if (selectedSpecifySize) {
                textNewGroupProp =
                    JSFUtil.getMessage(
                        "addsn.creategroup.method.specifysize") +
                    "<br>" +
                    JSFUtil.getMessage("addsn.creategroup.label.blocksize") +
                    Capacity.newCapacityInJSF(
                        Long.parseLong(textBlockSize),
                        Integer.parseInt(selectedBlockSizeUnit)).toString() +
                    "<br>" +
                    JSFUtil.getMessage("addsn.creategroup.label.blockperdevice")
                    + textBlockPerDevice;
            } else {
                textNewGroupProp =
                    JSFUtil.getMessage("addsn.creategroup.method.striped");
            }
        }
        return textNewGroupProp;
    }

    public void setTextNewGroupProp(String textNewGroupProp) {
        this.textNewGroupProp = textNewGroupProp;
    }

    public String getTextSelectedDataDevice() {
        textSelectedDataDevice =
            JSFUtil.getMessage(
                "addsn.summary.device",
                new String [] {
                    Integer.toString(selectedDataDevices.length),
                    "1GB"});
        return textSelectedDataDevice;
    }

    public void setTextSelectedDataDevice(String textSelectedDataDevice) {
        this.textSelectedDataDevice = textSelectedDataDevice;
    }

    public String getTextSelectedMDevice() {
        textSelectedMDevice =
            JSFUtil.getMessage(
                "addsn.summary.device",
                new String [] {
                    Integer.toString(selectedMetaDevices.length),
                    "1GB"});
        return textSelectedMDevice;
    }

    public void setTextSelectedMDevice(String textSelectedMDevice) {
        this.textSelectedMDevice = textSelectedMDevice;
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
        return alertRendered;
    }

    public void setAlertRendered(boolean alertRendered) {
        this.alertRendered = alertRendered;
    }

    public void setAlertInfo(String type, String summary, String detail) {
        alertRendered = true;
        this.alertType = type;
        this.alertSummary = summary;
        this.alertDetail = detail;
    }

    public void clearAlertInfo() {
        alertRendered = false;
        this.alertType = null;
        this.alertSummary = null;
        this.alertDetail = null;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Helper methods

    private void initWizard() {
        clearAlertInfo();

        textHostNameIP = null;
        selectedExistingGroup = true;
        selectedCreateGroup = false;
        selectedGroup = null;
        groupSelections = null;
        groupName = null;
        selectedEqualSize = true;
        selectedSpecifySize = false;
        selectedStripedGroup = false;
        textBlockSize = Integer.toString(DEFAULT_BLOCK_SIZE);
        textBlockPerDevice = Integer.toString(DEFAULT_BLOCK_PER_DEVICE);
        selectedBlockSizeUnit = Integer.toString(DEFAULT_BLOCK_SIZE_UNIT);
        blockSizeUnitSelections = null;
        metaTableRowGroup = null;
        selectMeta = new Select("metaTable");
        availUnits = null;
        selectedMetaDevices = null;
        dataTableRowGroup = null;
        selectData = new Select("dataTable");
        selectedDataDevices = null;
        dataSummaryList = null;
        textGroup = null;
        textNewGroupProp = null;
        textSelectedMDevice = null;
        textSelectedDataDevice = null;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Wizard Impl Control

    public WizardEventListener getWizardEventListener() {
        if (wizardEventListener == null) {
            wizardEventListener = new AddStorageNodeWizardEventListener();
        }
        return wizardEventListener;
    }

    public WizardEventListener getWizardStepEventListener() {
        if (wizardStepEventListener == null) {
            wizardStepEventListener = new AddStorageNodeWizardEventListener();
        }
        return wizardStepEventListener;
    }

    class AddStorageNodeWizardEventListener implements WizardEventListener {

        public boolean handleEvent(WizardEvent event) {
System.out.println("handleEvent called! " + event.getEvent());
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
System.out.println("NEXT: ID is " + id);
            if (STEP_SPECIFY_HOST.equals(id)) {
                // no op yet
            } else if (STEP_SPECIFY_GROUP.equals(id)) {
                // no op yet
            } else if (STEP_CREATE_GROUP.equals(id)) {
                // no op
            } else if (STEP_SPECIFY_META_DEVICE.equals(id)) {
                return processSpecifyMetaDevices(event);
            } else if (STEP_SPECIFY_DATA_DEVICE.equals(id)) {
                return processSpecifyDataDevices(event);
            }

            return true;
        }

        private boolean handleFinishButton(WizardEvent event) {
            String messageSummary = "summary";
            String messageDetails = "details";

            try {
                SamQFSSystemSharedFSManager sharedFSManager =
                                                    getSharedFSManager();

                boolean ipEntered = isIPAddress(textHostNameIP);

                sharedFSManager.addStorageNode(
                    JSFUtil.getServerName(),
                    FSUtil.getFSName(),
                    ipEntered ? null : textHostNameIP,
                    ipEntered ? textHostNameIP : null,
                    getDiskCacheObj(selectedMetaDevices),
                    getDiskCacheObj(selectedDataDevices),
                    createNodeDataString());
                /*
                setAlertInfo(
                    Constants.Alert.INFO,
                    JSFUtil.getMessage(
                        "SharedFS.message.removeclients.ok",
                        ConversionUtil.arrayToStr(selectedClients, ',')),
                    null);
                 */
                setAlertInfo(
                    Constants.Alert.INFO, messageSummary, messageDetails);
            } catch (SamFSException samEx) {
                /*
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                        "SharedFS.message.removeclients.failed",
                        ConversionUtil.arrayToStr(selectedClients, ',')),
                    samEx.getMessage());
                 */
                setAlertInfo(
                    Constants.Alert.ERROR, "summary", samEx.getMessage());
            }

            return true;
        }

        private boolean processSpecifyMetaDevices(WizardEvent event) {
            selectedMetaDevices = getSelectedDevices(true);

            // User has to select at least one device
            if (selectedMetaDevices == null ||
                selectedMetaDevices.length == 0) {
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
            } else {
                clearAlertInfo();
                return true;
            }

        }

        private boolean processSpecifyDataDevices(WizardEvent event) {
            selectedDataDevices = getSelectedDevices(false);

            // User has to select at least one device
            if (selectedDataDevices == null ||
                selectedDataDevices.length == 0) {
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
            } else {
                clearAlertInfo();
                return true;
            }

        }

        private DiskCache [] getDiskCacheObj(String [] paths)
            throws SamFSException {
            boolean found = false;
            DiskCache [] targetDc = new DiskCache[paths.length];
            for (int i = 0; i < paths.length; i++) {
                for (int j = 0; j < availUnits.length; j++) {
                    if (availUnits[j].getDevicePath().equals(paths[i])) {
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

        private boolean checkOverlapLuns() {
            String [] overlapLUNs = null;
            try {
                SamQFSSystemModel sysModel =
                    SamUtil.getModel(JSFUtil.getServerName());
                if (sysModel == null) {
                    throw new SamFSException(null, -2501);
                }

                overlapLUNs = sysModel.getSamQFSSystemFSManager().
                                  checkSlicesForOverlaps(selectedMetaDevices);
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

        private SamQFSSystemSharedFSManager getSharedFSManager()
            throws SamFSException {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();

            if (appModel == null) {
                throw new SamFSException("App model is null");
            }

            SamQFSSystemSharedFSManager sharedFSManager =
                            appModel.getSamQFSSystemSharedFSManager();
            if (sharedFSManager == null) {
                throw new SamFSException("shared fs manager is null");
            }
            return sharedFSManager;
        }

        /**
         * Check to see if user enters a host name or an IP address
         * @param textHostNameIP
         * @return
         */
        private boolean isIPAddress(String textHostNameIP) {
            // TODO: beef up this check
            String [] ipArray = textHostNameIP.split("\\.");
            return ipArray.length == 4;
        }

        /**
         * Create nodeData String.  It is a key value string that includes the
         * following keys:
         * host = hostname
         * dataip = ip address
         * group = groupId (o1, o2, o3 etc.)
         * @return
         */
        private String createNodeDataString() {
            // TODO: FIX THIS
            StringBuffer buf = new StringBuffer("group=o");
            if (selectedExistingGroup) {
                // Use existing group
                String [] arr = selectedGroup.split("Group");
                buf.append(arr[1]);
            } else {
                // Create new group
               buf.append(getGroupNames().length + 1);
            }

            buf.append(",").append(
                isIPAddress(textHostNameIP) ?
                    "dataip=" + textHostNameIP :
                    "host=" + textHostNameIP);
            return buf.toString();
        }
    }
}
