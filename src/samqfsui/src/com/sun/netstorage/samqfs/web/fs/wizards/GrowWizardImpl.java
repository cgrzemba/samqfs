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

// ident	$Id: GrowWizardImpl.java,v 1.45 2008/11/19 22:30:43 ronaldso Exp $


// Possible Steps:
//
// Step A: Select if you want to add a metadata device or a data device
// Step B: Select devices for metadata
// Step C: Specify the number of striped groups to add
// Step D: Select devices for data
// Step E: Review Summary
// Step F: View Results
//
// =============================================================================
//
// Case 1: For an un-mounted "ma" type QFS,
//
// ==> Seq: B,D,E,F (non-striped)
// ==> Seq: B,C,D^(# of new striped groups),E,F
//
// Case 2: For a mounted "ma" type QFS,
//
// ==> Seq: A,B,D,E,F (non-striped, select both checkbox in A)
// ==> Seq: A,B,E,F   (non-striped or striped, select only grow metadata in A)
// ==> Seq: A,D,E,F   (non-striped, select only grow data in A)
// ==> Seq: A,B,C,D^(# of new striped groups),E,F (striped,select both box in A)
// ==> Seq: A,C,D^(# of new striped groups),E,F
// (striped,select only grow data in A)
//
// Case 3: For a mounted or un-mounted "ms" type QFS, (no striped group in ms)
//
// ==> Seq: D,E,F


package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.model.ModelControlException;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.WizardResultView;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;
import com.sun.web.ui.view.table.CCActionTable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;

interface GrowWizardImplData {

    final String name = "GrowWizardImpl";
    final String title = "FSWizard.grow.title";

    final Class[] pageClass = {
        GrowWizardMethodView.class,
        FSWizardMetadataDeviceSelectionPageView.class,
        GrowWizardStripedGroupNumberPageView.class,
        FSWizardStripedGroupDeviceSelectionPageView.class,
        FSWizardDataDeviceSelectionPageView.class,
        GrowWizardSummaryPageView.class,
        WizardResultView.class
    };

    final String[] pageTitle = {
        "FSWizard.grow.method.title",
        "FSWizard.metadataDevicePage.title",
        "FSWizard.grow.stripedGroupNumberPage.title",
        "FSWizard.stripedGroupDevicePage.title",
        "FSWizard.dataDevicePage.title",
        "FSWizard.grow.summaryPage.title",
        "wizard.result.steptext"
    };

    final String[][] stepHelp = {
        {"FSWizard.grow.stripedGroupNumberPage.help.text1"},
        {"FSWizard.devicePage.help.text1",
         "FSWizard.devicePage.help.text2",
         "FSWizard.devicePage.help.text3",
         "FSWizard.devicePage.help.text4",
         "FSWizard.devicePage.help.text5",
         "FSWizard.devicePage.help.text6",
         "FSWizard.devicePage.help.text7",
         "FSWizard.devicePage.help.text8",
         "FSWizard.devicePage.help.text9",
         "FSWizard.devicePage.help.text10",
         "FSWizard.devicePage.help.text11"},
        {"FSWizard.grow.stripedGroupNumberPage.help.text1",
         "FSWizard.grow.stripedGroupNumberPage.help.text2"},
        {"FSWizard.stripedGroupDevicePage.help.text1",
         "FSWizard.stripedGroupDevicePage.help.text2",
         "FSWizard.devicePage.help.text1",
         "FSWizard.devicePage.help.text2",
         "FSWizard.devicePage.help.text3",
         "FSWizard.devicePage.help.text4",
         "FSWizard.devicePage.help.text5",
         "FSWizard.devicePage.help.text6",
         "FSWizard.devicePage.help.text7",
         "FSWizard.devicePage.help.text8",
         "FSWizard.devicePage.help.text9",
         "FSWizard.devicePage.help.text10",
         "FSWizard.devicePage.help.text11"},
        {"FSWizard.devicePage.help.text1",
         "FSWizard.devicePage.help.text2",
         "FSWizard.devicePage.help.text3",
         "FSWizard.devicePage.help.text4",
         "FSWizard.devicePage.help.text5",
         "FSWizard.devicePage.help.text6",
         "FSWizard.devicePage.help.text7",
         "FSWizard.devicePage.help.text8",
         "FSWizard.devicePage.help.text9",
         "FSWizard.devicePage.help.text10",
         "FSWizard.devicePage.help.text11"},
        {"FSWizard.grow.summaryPage.help.text1",
         "FSWizard.grow.summaryPage.help.text2"},
         {"wizard.result.help.text1",
          "wizard.result.help.text2"}
    };

    final String[] stepText = {
        "FSWizard.grow.method.step.text",
        "FSWizard.metadataDevicePage.step.text",
        "FSWizard.grow.stripedGroupNumberPage.step.text",
        "FSWizard.stripedGroupDevicePage.step.text",
        "FSWizard.dataDevicePage.step.text",
        "FSWizard.grow.summaryPage.step.text",
        "wizard.result.steptext"
    };

    final String[] stepInstruction = {
        "FSWizard.grow.method.step.text",
        "FSWizard.metadataDevicePage.instruction.text",
        "FSWizard.grow.stripedGroupNumberPage.instruction.text",
        "FSWizard.stripedGroupDevicePage.instruction.text",
        "FSWizard.dataDevicePage.instruction.text",
        "FSWizard.grow.summaryPage.instruction.text",
        "wizard.result.instruction"
    };

    final String[] cancelmsg = {
        "",
        "",
        "",
        "",
        "",
        ""
    };

    final int PAGE_METHOD = 0;
    final int PAGE_METADATA = 1;
    final int PAGE_STRIPED_GROUP_NUM = 2;
    final int PAGE_STRIPED_GROUP = 3;
    final int PAGE_DATA = 4;
    final int PAGE_SUMMARY = 5;
    final int PAGE_RESULT = 6;
}

public class GrowWizardImpl extends SamWizardImpl {

    public static final String WIZARDPAGEMODELNAME = "GrowPageModelName";
    public static final String WIZARDPAGEMODELNAME_PREFIX = "GrowWizardModel";
    public static final String WIZARDIMPLNAME = GrowWizardImplData.name;
    public static final String WIZARDIMPLNAME_PREFIX = "GrowWizardImpl";
    public static final String WIZARDCLASSNAME =
        "com.sun.netstorage.samqfs.web.fs.wizards.GrowWizardImpl";

    // Flag to set if a view is used in the grow wizard to grow a shared
    // file system
    public static final String ATTR_GROW_SHARED_FS = "attr_grow_shared_fs";

    // Wizard model value to determine if file system has combined
    public static final String ATTR_COMBINED = "attr_combined";

    // Wizard model value to determine if file system is mounted or not
    public static final String ATTR_IS_MOUNTED = "attr_is_mounted";

    private boolean wizardInitialized = false;

    // Variable to hold previously entered number of striped groups
    private String OLD_NUM_STRIPED_GROUPS = "OLD_NUM_STRIPED_GROUPS";

    // Hold the number of devices that can be added to the file system.
    // A file system cannot hold more than 252 LUNs
    private int maxDeviceToAdd = -1;

    protected String serverName = null;
    protected String fsName = null;

    // Save a copy of file system object of which is about to grow
    private FileSystem thisFS = null;

    // An example of "page" steps
    private int [] mountSteps = new int [] {GrowWizardImplData.PAGE_METHOD,
                                            GrowWizardImplData.PAGE_METADATA,
                                            GrowWizardImplData.PAGE_DATA,
                                            GrowWizardImplData.PAGE_SUMMARY,
                                            GrowWizardImplData.PAGE_RESULT};

    /**
     * Static Create method called by the parent viewbean
     * @param requestContext
     * @return
     */
    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace2("in create()");
        return  new GrowWizardImpl(requestContext);
    }

    /**
     * Default constructor
     * @param requestContext
     */
    public GrowWizardImpl(RequestContext requestContext) {
        super(requestContext, WIZARDPAGEMODELNAME);
        initializeWizard(requestContext);
        initializeWizardControl(requestContext);
    }

    /**
     * Creating wizard window model
     * @param cmdChild
     * @return
     */
    public static CCWizardWindowModel createModel(String cmdChild) {
        return
            getWizardWindowModel(
                WIZARDIMPLNAME,
                GrowWizardImplData.title,
                WIZARDCLASSNAME,
                cmdChild);
    }

    /**
     * Overwrite getPageClass() to put striped group number into wizardModel
     * so that it can be accessed by NewWizardStripedGroupPage
     * @param pageId
     * @return
     */
    public Class getPageClass(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        if (pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP) {
            wizardModel.setValue(
                Constants.Wizard.STRIPED_GROUP_NUM,
                getStripedGroupNumber(page));
            TraceUtil.trace3("This is striped group # " +
                (page - GrowWizardImplData.PAGE_STRIPED_GROUP));

        }
        // clear out previous errors if wizard has been initialized
        if (wizardInitialized) {
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        }
        return super.getPageClass(pageId);
    }

    /**
     * Return an array of string contains the future steps shown in each step
     * @Override
     * @param pageId
     * @return
     */
    public String[] getFutureSteps(String pageId) {
        TraceUtil.trace3("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String[] futureSteps = null;

        if (pages[page] == GrowWizardImplData.PAGE_METHOD ||
            pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP_NUM) {
            return new String[0];
        } else if (
            pages[page] == GrowWizardImplData.PAGE_METADATA
            && getNumberOfAvailStripedGrps() != -1
            && isMounted() && isAddingData()) {
            return
                new String [] {
                    SamUtil.getResourceString(
                        stepText[GrowWizardImplData.PAGE_STRIPED_GROUP_NUM],
                        new String [] {
                            Integer.toString(
                                GrowWizardImplData.PAGE_STRIPED_GROUP_NUM)})};
        }
        int howMany = pages.length - page - 1;
        futureSteps = new String[howMany];

        for (int i = 0; i < howMany; i++) {
            int futureStep = page + 1 + i;
            int futurePage = pages[futureStep];
            if (futurePage == GrowWizardImplData.PAGE_STRIPED_GROUP) {
                futureSteps[i] =
                    SamUtil.getResourceString(
                        stepText[futurePage],
                        new String [] {
                            Integer.toString(
                                getStripedGroupNumber(futureStep))});
            } else {
                futureSteps[i] = stepText[futurePage];
            }
        }

        return futureSteps;
    }

    /**
     * Return the step text for each step
     * @param pageId
     * @return
     */
    public String getStepText(String pageId) {
        TraceUtil.trace3("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String text = null;

        if (pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP) {
            text = SamUtil.getResourceString(
                stepText[pages[page]],
                new String[] {Integer.toString(getStripedGroupNumber(page))});
        } else {
            text = stepText[pages[page]];
        }

        return text;
    }

    /**
     * Return the step title for each step
     * @param pageId
     * @return
     */
    public String getStepTitle(String pageId) {
        TraceUtil.trace3("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String title = null;

        if (pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP) {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] { Integer.toString(
                    getStripedGroupNumber(page))});
        } else {
            title = pageTitle[pages[page]];
        }

        return title;
    }

    /**
     * This method is called when the "next" button is clicked in a wizard step
     * @param wizardEvent
     * @return
     */
    public boolean nextStep(WizardEvent wizardEvent) {
        String pageId = wizardEvent.getPageId();
        TraceUtil.trace3("Entered with pageID = " + pageId);

        // make this wizard active
        super.nextStep(wizardEvent);

        // when we get here, the wizard must has been initialized
        wizardInitialized = true;

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case GrowWizardImplData.PAGE_METHOD:
                // Process the selections in the method page
                return processMethodPage(wizardEvent);
            case GrowWizardImplData.PAGE_METADATA:
                // Process the selections in the MetaDatadevices Page
                return processMetaDataDevicesPage(wizardEvent);

            case GrowWizardImplData.PAGE_STRIPED_GROUP_NUM:
                // Process striped group number Page
                return processStripedGroupNumberPage(wizardEvent);

            case GrowWizardImplData.PAGE_STRIPED_GROUP:
                // Process striped group number Page
                return processStripedGroupDevicesPage(wizardEvent);

            case GrowWizardImplData.PAGE_DATA:
                // Process the selections in the Datadevices Page
                return processDataDevicesPage(wizardEvent);
        }

        return true;
    }

    /**
     * This method is called when the finish button is clicked on the very last
     * step (review selections).
     * finishStep() will collect all data input in each of wizard step then
     * call API to grow the fs
     * @param wizardEvent
     * @return
     */
    public boolean finishStep(WizardEvent wizardEvent) {
        String pageId = wizardEvent.getPageId();
        TraceUtil.trace3("Entered with pageID = " + pageId);

        // make sure this wizard is still active before commit
        if (super.finishStep(wizardEvent) == false) {
            return true;
        }

        DiskCache[] metadataDevices =
            convertArrayListToArray(getSelectedMetaDataDevices());
        DiskCache[] dataDevices =
            convertArrayListToArray(getSelectedDataDevices());

        StripedGroup[] stripedGroups = null;
        try {
            stripedGroups = getSelectedStripedGroupsArray();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                "Failed to create striped groups",
                serverName);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.grow.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return true;
        }

        try {
            // Now grow the FileSystem
            LogUtil.info(
                this.getClass(),
                "finishStep",
                "Start growing FS " + fsName);

            getFileSystem().grow(
                metadataDevices,
                dataDevices,
                stripedGroups);

            wizardModel.setValue(Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_SUCCESS);

            LogUtil.info(
                this.getClass(),
                "finishStep",
                "Done growing FS " + fsName);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "FSSummary.growfs", fsName));
            return true;
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                "Failed to grow fs",
                serverName);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.grow.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return true;
        }
    }

    /**
     * This method is called when the cancel button is clicked
     * @param wizardEvent
     * @return
     */
    public boolean cancelStep(WizardEvent wizardEvent) {
        super.cancelStep(wizardEvent);

        // clear previous error
        wizardModel.setValue(
            Constants.Wizard.WIZARD_ERROR,
            Constants.Wizard.WIZARD_ERROR_NO);
        wizardModel.clear();
        return true;
    }

    /**
     * This method is called when the close button is clicked
     * @param wizardEvent
     */
    public void closeStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Clearing out wizard model...");
        wizardModel.clear();
        TraceUtil.trace3("Done!");
    }

    /**
     * Private method to process the method page
     */
    private boolean processMethodPage(WizardEvent wizardEvent) {
        String addingMeta = (String)
                wizardModel.getValue(GrowWizardMethodView.CHECKBOX_META);
        String addingData = (String)
                wizardModel.getValue(GrowWizardMethodView.CHECKBOX_DATA);
        if (Boolean.valueOf(addingMeta).booleanValue() == false &&
            Boolean.valueOf(addingData).booleanValue() == false) {
            setWizardAlert(wizardEvent, "FSWizard.grow.error.method");
            return false;
        } else if (Boolean.valueOf(addingMeta).booleanValue() == false) {
            // clear selected meta device array
            wizardModel.setValue(
                Constants.Wizard.SELECTED_METADEVICES,
                null);
        } else if (Boolean.valueOf(addingData).booleanValue() == false) {
            // clear selected device array
            wizardModel.setValue(
                Constants.Wizard.SELECTED_DATADEVICES,
                null);
        }

        if (-1 == getNumberOfAvailStripedGrps()) {
            updateWizardSteps(-1);
        } else {
            updateWizardSteps(1);
        }

        initializeWizardPages(pages);
        return true;
    }

    /**
     * Private method to process the MetaData Devices page
     */
    private boolean processMetaDataDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered processMetaDataDevicesPage.., ");
        boolean result = true;

        // Get a handle to the View
        FSWizardDeviceSelectionPageView view =
            (FSWizardDeviceSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardDeviceSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "processMetaDataDevicesPage()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1("Exception while restoreStateData: " +
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }

        CCActionTableModel dataModel = (CCActionTableModel)dataTable.getModel();
        Integer [] selectedRows = dataModel.getSelectedRows();
        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);
        // If the user did not select any devices, show error
        if (selectedRows.length < 1) {
            setWizardAlert(wizardEvent, "FSWizard.grow.error.metadata");
            return false;
        } else {
            if (selectedRows.length > maxDeviceToAdd) {
                setWizardAlert(wizardEvent, "FSWizard.maxlun");
                return false;
            }
        }

        // Traverse through the selection and populate selectedDataDevicesList
        String deviceSelected = null;
        ArrayList selectedMetaDevicesList = getSelectedMetaDataDevices();
        selectedMetaDevicesList.clear();

        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            selectedMetaDevicesList.add(deviceSelected);
        }

        // Set the value of the selection to display in the summary page
        Collections.sort(selectedMetaDevicesList);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_METADEVICES,
            selectedMetaDevicesList);

        // The following codechecks for lun overlapping

        // Generate an array of strings to check for overlapping luns
        int size = selectedMetaDevicesList.size();
        String[] dataLUNs = new String[size];
        for (int i = 0; i < size; i++) {
            dataLUNs[i] = (String) selectedMetaDevicesList.get(i);
        }

        SamQFSSystemModel sysModel = null;
        String[] overlapLUNs = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            overlapLUNs =
                sysModel.getSamQFSSystemFSManager().checkSlicesForOverlaps(
                    dataLUNs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectMetaDataDeviceStep()",
                "Failed to check metadata LUNs for overlaps",
                serverName);

            wizardModel.setValue(
                Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_FAILED);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_MESSAGE, ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // if found overlapped LUNs, generate appropriate error message
        if (overlapLUNs != null && overlapLUNs.length > 0) {
            StringBuffer badLUNs = new StringBuffer(
                SamUtil.getResourceString(
                "FSWizard.error.overlapMetadataLUNs"));
            badLUNs.append("<br>");
            for (int i = 0; i < overlapLUNs.length; i++) {
                badLUNs.append(overlapLUNs[i]).append("<br>");
            }
            TraceUtil.trace2(badLUNs.toString());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, badLUNs.toString());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
            wizardModel.setValue(
                Constants.Wizard.ERROR_DETAIL,
                Constants.Wizard.ERROR_INLINE_ALERT);
            wizardModel.setValue(
                Constants.Wizard.ERROR_SUMMARY,
                "FSWizard.error.deviceError");
            return false;
        }

        return result;
    }

    /**
     * Private method to process the Data Devices page
     */
    private boolean processDataDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");

        // Get a handle to the View
        FSWizardDeviceSelectionPageView view =
            (FSWizardDeviceSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardDeviceSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1(
                "Exception while restoreStateData: " + ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }

        CCActionTableModel dataModel = (CCActionTableModel)dataTable.getModel();
        Integer [] selectedRows = dataModel.getSelectedRows();

        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);

        ArrayList selectedDataDevicesList = getSelectedDataDevices();

        // Traverse through the selection and populate selectedDataDevicesList
        String deviceSelected = null;
        selectedDataDevicesList.clear();

        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            selectedDataDevicesList.add(deviceSelected);
        }

        Collections.sort(selectedDataDevicesList);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_DATADEVICES,
            selectedDataDevicesList);

        // when growing a fs, it's allowed to grow metadata devices only
        // so if user didn't select any data devices, return now
        if (selectedRows.length < 1) {
            if (isCombined()) {
                setWizardAlert(wizardEvent, "FSWizard.grow.error.data");
                return false;
            } else if (isMounted() && isAddingData()) {
                // user explicitly said he/she wants to add data devices in the
                // first step (method)
                setWizardAlert(
                    wizardEvent, "FSWizard.grow.error.data.explicit");
                return false;
            } else {
                wizardModel.setValue(
                    GrowWizardSummaryPageView.CHILD_DATA_FIELD, "");
                return true;
            }
        } else {
            if (isCombined()) {
                if (selectedRows.length > maxDeviceToAdd) {
                    setWizardAlert(wizardEvent, "FSWizard.maxlun");
                    return false;
                }
            } else {
                int metaDevices = getSelectedMetaDataDevices().size();
                if (selectedRows.length > maxDeviceToAdd - metaDevices) {
                    setWizardAlert(wizardEvent, "FSWizard.maxlun");
                    return false;
                }
            }
        }

        // Set the value of the selection to display in the summary page
        Collections.sort(selectedDataDevicesList);

        // The following code checks for lun overlapping
        // Generate an array of strings to check for overlapping luns
        int size = selectedDataDevicesList.size();
        String[] dataLUNs = new String[size];
        for (int i = 0; i < size; i++) {
            dataLUNs[i] = (String) selectedDataDevicesList.get(i);
        }

        SamQFSSystemModel sysModel = null;
        String[] overlapLUNs = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            overlapLUNs =
                sysModel.getSamQFSSystemFSManager().checkSlicesForOverlaps(
                    dataLUNs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed to check data LUNs for overlaps",
                serverName);

            wizardModel.setValue(
                Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_FAILED);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_MESSAGE, ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // if found overlapped LUNs, generate appropriate error message
        if (overlapLUNs != null && overlapLUNs.length > 0) {
            StringBuffer badLUNs = new StringBuffer(
                SamUtil.getResourceString(
                "FSWizard.error.overlapDataLUNs"));
            badLUNs.append("<br>");
            for (int i = 0; i < overlapLUNs.length; i++) {
                badLUNs.append(overlapLUNs[i]).append("<br>");
            }

            TraceUtil.trace2(badLUNs.toString());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, badLUNs.toString());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
            wizardModel.setValue(
                Constants.Wizard.ERROR_DETAIL,
                Constants.Wizard.ERROR_INLINE_ALERT);
            wizardModel.setValue(
                Constants.Wizard.ERROR_SUMMARY,
                "FSWizard.error.deviceError");
            return false;
        }

        return true;
    }


    /**
     * Private method to process the Striped Group Devices page
     */
    private boolean processStripedGroupDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");

        ArrayList selectedStripedGroupDevicesList =
                            getSelectedStripedGroupDevices();
        // Get a handle to the View
        FSWizardDeviceSelectionPageView view =
            (FSWizardDeviceSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardDeviceSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1(
                "Exception while restoreStateData: " + ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }

        CCActionTableModel dataModel = (CCActionTableModel)dataTable.getModel();
        Integer [] selectedRows = dataModel.getSelectedRows();
        int groupNum = ((Integer) wizardModel.getValue(
            Constants.Wizard.STRIPED_GROUP_NUM)).intValue();
        // If the user did not select any devices, show error
        // check id selected luns exceed 252 limit
        if (selectedRows.length < 1) {
            setWizardAlert(wizardEvent, "FSWizard.grow.error.data");
            return false;
        } else {
            int metaLuns = getSelectedMetaDataDevices().size();
            int totalStripeLuns = 0;
            if (selectedStripedGroupDevicesList != null) {
                for (int j = 0; j < groupNum; j++) {
                    ArrayList groupDevices =
                        (ArrayList) selectedStripedGroupDevicesList.get(j);
                    totalStripeLuns += groupDevices.size();
                }
            }
            int usedLuns = metaLuns + totalStripeLuns + selectedRows.length;
            if (usedLuns > maxDeviceToAdd) {
                setWizardAlert(wizardEvent, "FSWizard.maxlun");
                return false;
            }
        }
        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);
        // Traverse through the selection and populate
        // selectedStripedGroupDevicesList

        String deviceSelected = null;

        ArrayList thisGroupDevices;
        if (selectedStripedGroupDevicesList.size() <= groupNum) {
            thisGroupDevices = new ArrayList();
            selectedStripedGroupDevicesList.add(groupNum, thisGroupDevices);
        } else {
            thisGroupDevices =
                (ArrayList) selectedStripedGroupDevicesList.get(groupNum);
        }
        thisGroupDevices.clear();

        // get a list of selected devices
        // also check if they all have the same size
        long groupDeviceSize = -1;
        boolean deviceError = false;
        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            DiskCache disk = getDiskCacheObject(deviceSelected);
            if (groupDeviceSize < 0) {
                groupDeviceSize = disk.getCapacity();
            }
            if (groupDeviceSize != disk.getCapacity()) {
                deviceError = true;
            }
            thisGroupDevices.add(deviceSelected);
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES,
            selectedStripedGroupDevicesList);

        // we process deviceError here because of the counter display problem
        if (deviceError) {
            wizardModel.setValue(
                    Constants.Wizard.WIZARD_ERROR,
                    Constants.Wizard.WIZARD_ERROR_YES);
                wizardModel.setValue(
                    Constants.Wizard.ERROR_MESSAGE,
                    "FSWizard.error.stripedGroup.deviceSizeError");
                wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
                wizardModel.setValue(
                    Constants.Wizard.ERROR_DETAIL,
                    Constants.Wizard.ERROR_INLINE_ALERT);
                wizardModel.setValue(
                    Constants.Wizard.ERROR_SUMMARY,
                    "FSWizard.error.deviceError");
                return false;
        }

        // Set the value of the selection to display in the summary page
        Collections.sort(thisGroupDevices);

        // The following code checks for lun overlapping

        // Generate an array of strings to check for overlapping luns
        int size = thisGroupDevices.size();
        String[] dataLUNs = new String[size];
        for (int i = 0; i < size; i++) {
            dataLUNs[i] = (String) thisGroupDevices.get(i);
        }

        SamQFSSystemModel sysModel = null;
        String[] overlapLUNs = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            overlapLUNs =
                sysModel.getSamQFSSystemFSManager().checkSlicesForOverlaps(
                    dataLUNs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectStripedGroupStep()",
                "Failed to check striped group for overlaps",
                serverName);

            wizardModel.setValue(
                Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_FAILED);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_MESSAGE, ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // if found overlapped LUNs, generate appropriate error message
        if (overlapLUNs != null && overlapLUNs.length > 0) {
            StringBuffer badLUNs = new StringBuffer(
                SamUtil.getResourceString(
                "FSWizard.error.overlapDataLUNs"));
            badLUNs.append("<br>");
            for (int i = 0; i < overlapLUNs.length; i++) {
                badLUNs.append(overlapLUNs[i]).append("<br>");
            }

            TraceUtil.trace2(badLUNs.toString());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, badLUNs.toString());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
            wizardModel.setValue(
                Constants.Wizard.ERROR_DETAIL,
                Constants.Wizard.ERROR_INLINE_ALERT);
            wizardModel.setValue(
                Constants.Wizard.ERROR_SUMMARY,
                "FSWizard.error.deviceError");
            return false;
        }

        // clear out previous error
        wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        return true;
    }

    /**
     * Private method to validate striped group number
     */
    private boolean processStripedGroupNumberPage(WizardEvent wizardEvent) {
        String numGroups = (String) wizardModel.getValue(
            GrowWizardStripedGroupNumberPageView.
                CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD);
        int availableGroups = getNumberOfAvailStripedGrps();
        int numStripedGroups = -1;

        if (numGroups != null) {
            try {
                numStripedGroups = Integer.parseInt(numGroups);
            } catch (NumberFormatException nfe) {
                TraceUtil.trace1(
                    "User entry of striped group number contains " +
                    "non-numeric characters!", nfe);
            }
        }
        if (numStripedGroups < 0 || numStripedGroups > availableGroups) {
            setWizardAlert(wizardEvent,
                SamUtil.getResourceString("FSWizard.grow.error.numStripedGroup",
                    new String[] { Integer.toString(availableGroups) }));
            return false;
        }

        updateWizardSteps(numStripedGroups);
        initializeWizardPages(pages);

        // set data field in summary page
        if (numStripedGroups == 0) {
            wizardModel.setValue(
                Constants.Wizard.SELECTED_DATADEVICES, null);
        }

        ArrayList selectedStripedGroupDevicesList =
                            getSelectedStripedGroupDevices();

        // clear striped group devices if necessary
        Integer oldNumGroups = (Integer)
            wizardModel.getValue(OLD_NUM_STRIPED_GROUPS);
        if (oldNumGroups != null) {
            int oldNum = oldNumGroups.intValue();
            if (oldNum > numStripedGroups &&
                selectedStripedGroupDevicesList != null) {
                TraceUtil.trace2("Clearing out striped group devices");
                // clear striped group devices
                int oldSize = selectedStripedGroupDevicesList.size();
                for (int i = oldSize; i > numStripedGroups; i--) {
                    selectedStripedGroupDevicesList.remove(i-1);
                }
                wizardModel.setValue(
                    Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES,
                    selectedStripedGroupDevicesList);
            }
        }
        wizardModel.setValue(
            OLD_NUM_STRIPED_GROUPS, new Integer(numStripedGroups));

        // If no errors until now, then return true
        return true;
    }

    /**
     * Get a handle of the file system that we are about to grow
     * @return
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    private FileSystem getFileSystem() throws SamFSException {
        if (thisFS == null) {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            thisFS =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (thisFS == null) {
                throw new SamFSException(null, -1000);
            }
        }
        return thisFS;
    }

    /**
     * Pre-process the necessary file system information to the wizard model
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    private void populateFSInfoInWizardModel() throws SamFSException {
        FileSystem myFS = getFileSystem();

        boolean growSharedFS =
            myFS.getShareStatus() == FileSystem.SHARED_TYPE_MDS;
        wizardModel.setValue(
            ATTR_GROW_SHARED_FS,
            Boolean.toString(growSharedFS));
        TraceUtil.trace2("GrowFS, growing shared fs: " + growSharedFS);

        // Need to populate the next six wizard parameters for the device
        // selection views because the views are shared between the Grow
        // wizard and the New File System Wizard.

        // fs type is always going to be qfs. ufs is non-growable.
        wizardModel.setValue(CreateFSWizardImpl.POPUP_FSTYPE, "qfs");

        wizardModel.setValue(
            CreateFSWizardImpl.POPUP_ARCHIVING,
            new Boolean(myFS.getArchivingType() == FileSystem.ARCHIVING));

        // We want to set POPUP_SHARED "sharedEnabled" to "false" because we do
        // not want the AllocatableDevices to be casted into SharedDiskCache [].
        wizardModel.setValue(
            CreateFSWizardImpl.POPUP_SHARED, new Boolean(false));

        wizardModel.setValue(
            CreateFSWizardImpl.POPUP_HAFS, new Boolean(false));
        wizardModel.setValue(
            CreateFSWizardImpl.POPUP_HPC,
            new Boolean(myFS.isMbFS()));
        wizardModel.setValue(
            CreateFSWizardImpl.POPUP_MATFS,
            new Boolean(myFS.isMatFS()));

        StripedGroup[] group = myFS.getStripedGroups();

        int stripedDeviceLuns = 0;
        int availableGroups = -1;

        if (group != null && group.length > 0) {
            availableGroups =
                Constants.Wizard.MAX_STRIPED_GROUPS - group.length;

            // used devices for striped group
            for (int i = 0; i < group.length; i++) {
                stripedDeviceLuns += group[i].getMembers().length;
            }

            if (isMounted()) {
                pages = mountSteps;
            } else {
                updateWizardSteps(1);
            }

            TraceUtil.trace2("group length = " + group.length);
            TraceUtil.trace2("available num of groups = " + availableGroups);
        } else {
            // same metadata and data device
            updateWizardSteps(-1);
        }

        initializeWizardPages(pages);

        // get the available lun to grow for samfs
        if (isCombined()) {
            int dataDeviceLuns = myFS.getDataDevices().length;
            maxDeviceToAdd = Constants.Wizard.MAX_LUNS - dataDeviceLuns;
        } else {
            int metaDeviceLuns = myFS.getMetadataDevices().length;
            // retrieve data if any
            int dataDevicesLuns = 0;
            DiskCache[] dataDevices = myFS.getDataDevices();
            if (dataDevices != null && dataDevices.length > 0)
                dataDevicesLuns = dataDevices.length;
            maxDeviceToAdd = Constants.Wizard.MAX_LUNS - metaDeviceLuns -
                dataDevicesLuns - stripedDeviceLuns;
        }

        TraceUtil.trace3("Setting AvailableGroups: " + availableGroups);
        wizardModel.setValue(
            Constants.Wizard.AVAILABLE_STRIPED_GROUPS,
            new Integer(availableGroups));
    }

    private int getNumberOfAvailStripedGrps() {
        Integer sgroups =
            (Integer) wizardModel.getValue(
                Constants.Wizard.AVAILABLE_STRIPED_GROUPS);
        return
            sgroups == null ?
                -1:
                sgroups.intValue();
    }

    /**
     * Utility method to return an Array of DataDevices
     * from an ArrayList of device paths
     */
    private DiskCache[] convertArrayListToArray(ArrayList selectedDevicePaths) {
        if (selectedDevicePaths == null) {
            return new DiskCache[0];
        }

        int numDevices = selectedDevicePaths.size();
        DiskCache[] selectedDevices = new DiskCache[numDevices];

        Iterator iter = selectedDevicePaths.iterator();
        int i = 0;
        while (iter.hasNext()) {
            selectedDevices[i] = getDiskCacheObject((String)iter.next());
            i++;
        }

        return selectedDevices;
    }

    /**
     * Utility method to return an Array of StripedGroup
     */
    private StripedGroup[] getSelectedStripedGroupsArray()
        throws SamFSException {

        ArrayList groupList = getSelectedStripedGroupDevices();
        if (groupList == null) {
            return new StripedGroup[0];
        }

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        int numGroups = groupList.size();
        StripedGroup[] stripedGroups = new StripedGroup[numGroups];
        for (int i = 0; i < numGroups; i++) {
            ArrayList groupDevices = (ArrayList) groupList.get(i);
            DiskCache[] disks = convertArrayListToArray(groupDevices);
            stripedGroups[i] = sysModel.getSamQFSSystemFSManager().
                createStripedGroup("",  disks);
        }

        return stripedGroups;
    }

    private DiskCache getDiskCacheObject(String devicePath) {
        DiskCache [] devices =
            (DiskCache[]) wizardModel.getValue(
                                Constants.Wizard.ALLOCATABLE_DEVICES);
        for (int i = 0; i < devices.length; i++) {
            if (devices[i].getDevicePath().equals(devicePath)) {
                return devices[i];
            }
        }

        // Should not reach here!! the selection is from this List
        // so there should be a match
        TraceUtil.trace1(
            "Selection device does not exist in Current Devicelist");
        return null;
    }

    /**
     * All Allocatable Units are stored in wizard Model.
     * This way we can avoid calling the backend everytime the user hits
     * the previous button.
     * Also this will help in keeping track of all the selections that the user
     * made during the course of the wizard, and will he helpful to remove
     * those entries for the Metadata LUN selection page.
     */
    private void populateDevicesInWizardModel() throws SamFSException {
        TraceUtil.trace2("Populating allocatable units to wizard model!");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        DiskCache [] devices =
            sysModel.getSamQFSSystemFSManager().
                discoverAvailableAllocatableUnits(null);
        wizardModel.setValue(
            Constants.Wizard.ALLOCATABLE_DEVICES,
            devices);
    }

    private void setWizardAlert(WizardEvent wizardEvent, String message) {
        wizardEvent.setSeverity(WizardEvent.ACKNOWLEDGE);
        wizardEvent.setErrorMessage(message);
    }

    // initialize wizard data members
    private void initializeWizard(RequestContext requestContext) {
        TraceUtil.trace2("Initializing wizard...");

        wizardName  = GrowWizardImplData.name;
        wizardTitle = GrowWizardImplData.title;
        pageClass = GrowWizardImplData.pageClass;
        pageTitle = GrowWizardImplData.pageTitle;
        stepHelp  = GrowWizardImplData.stepHelp;
        stepText  = GrowWizardImplData.stepText;
        stepInstruction = GrowWizardImplData.stepInstruction;
        cancelMsg = GrowWizardImplData.cancelmsg;

        fsName = requestContext.getRequest().getParameter(
            Constants.Parameters.FS_NAME);
        serverName = requestContext.getRequest().getParameter(
            Constants.Parameters.SERVER_NAME);
        wizardModel.setValue(Constants.Wizard.FS_NAME, fsName);
        wizardModel.setValue(Constants.Wizard.SERVER_NAME, serverName);

        TraceUtil.trace2("File System Name: " + fsName +
                         "Server Name: " + serverName);

        try {
            populateDevicesInWizardModel();
            populateFSInfoInWizardModel();

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "initializeWizard()",
                "Failed to initialize growfs wizard",
                serverName);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.ERROR_CODE,
                Integer.toString(ex.getSAMerrno()));
        }

        // now generate pages based on fs type
        setShowResultsPage(true);
        initializeWizardPages(pages);

        TraceUtil.trace2("wizard initialized!");
    }

    /**
     * Update the wizard flow
     * @param stripedGroups
     */
    private void updateWizardSteps(int stripedGroups) {
        // For wizard step logic, see the top of this java file
        int totalPages =
            (isMounted() && !isCombined()) ?
                3 : 2;

        if (isAddingMeta()) {
            totalPages++; // metadata page
        }

        if (isAddingData()) {
            if (stripedGroups >= 0) {
                totalPages++; // striped group number page
                totalPages += stripedGroups; // striped group pages
            } else {
                totalPages++; // data page
            }
        }

        pages = new int[totalPages];
        int pageCount = 0;

        if (isMounted() && !isCombined()) {
            pages[pageCount++] = GrowWizardImplData.PAGE_METHOD;
        }
        if (isAddingMeta()) {
            pages[pageCount++] = GrowWizardImplData.PAGE_METADATA;
        }

        if (isAddingData()) {
            if (stripedGroups >= 0) {
                pages[pageCount++] = GrowWizardImplData.PAGE_STRIPED_GROUP_NUM;
                for (int i = 0; i < stripedGroups; i++) {
                    pages[pageCount++] = GrowWizardImplData.PAGE_STRIPED_GROUP;
                }
            } else {
                pages[pageCount++] = GrowWizardImplData.PAGE_DATA;
            }
        }

        pages[pageCount++] = GrowWizardImplData.PAGE_SUMMARY;
        pages[pageCount] = GrowWizardImplData.PAGE_RESULT;
    }

    private int getStripedGroupNumber(int page) {
        int pageAhead = 0;
        if (pages[0] == GrowWizardImplData.PAGE_METHOD) {
            pageAhead++;
        }
        if (isAddingMeta()) {
            pageAhead++;
        }
        if (isAddingData()) {
            // striped group number page
            pageAhead++;
        }

        return page - pageAhead;
    }

    /**
     * Helper method to determine the metadata placement
     * @return boolean value if metadata and data are in the same device
     */
    private boolean isCombined() {
        Boolean combined = (Boolean) wizardModel.getValue(ATTR_COMBINED);
        try {
            if (combined == null) {
                boolean c =
                    getFileSystem().getFSType() == FileSystem.COMBINED_METADATA;
                wizardModel.setValue(ATTR_COMBINED, new Boolean(c));
                TraceUtil.trace2(
                    "Setting ATTR_COMBINED to " + c);
                return c;
            } else {
                return
                    combined == null ?
                        true :
                        combined.booleanValue();
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to retrieve file system!", samEx);
            return true;
        }
    }

    private ArrayList getSelectedDataDevices() {
        ArrayList selectedDataDevicesList =
            (ArrayList) wizardModel.getValue(
                Constants.Wizard.SELECTED_DATADEVICES);
        if (selectedDataDevicesList == null) {
            selectedDataDevicesList = new ArrayList();
            wizardModel.setValue(
                Constants.Wizard.SELECTED_DATADEVICES,
                selectedDataDevicesList);
        }
        return selectedDataDevicesList;
    }

    private ArrayList getSelectedMetaDataDevices() {
        ArrayList selectedMetaDevicesList =
            (ArrayList) wizardModel.getValue(
                Constants.Wizard.SELECTED_METADEVICES);
        if (selectedMetaDevicesList == null) {
            selectedMetaDevicesList = new ArrayList();
            wizardModel.setValue(
                Constants.Wizard.SELECTED_METADEVICES,
                selectedMetaDevicesList);
        }
        return selectedMetaDevicesList;
    }

    private ArrayList getSelectedStripedGroupDevices() {
        ArrayList selectedStripedGroupDevicesList =
            (ArrayList) wizardModel.getValue(
                Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES);
        if (selectedStripedGroupDevicesList == null) {
            selectedStripedGroupDevicesList = new ArrayList();
            wizardModel.setValue(
                Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES,
                selectedStripedGroupDevicesList);
        }
        return selectedStripedGroupDevicesList;
    }

    /**
     * Helper method to determine if the file system is mounted
     * @return boolean value if metadata and data are in the same device
     */
    private boolean isMounted() {
        Boolean mounted = (Boolean) wizardModel.getValue(ATTR_IS_MOUNTED);
        try {
            if (mounted == null) {
                boolean m = getFileSystem().getState() == FileSystem.MOUNTED;
                wizardModel.setValue(ATTR_IS_MOUNTED, new Boolean(m));
                TraceUtil.trace2("Setting ATTR_IS_MOUNTED to " + m);
                return m;
            } else {
                return
                    mounted == null ?
                        false :
                        mounted.booleanValue();
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to retrieve file system!", samEx);
            return false;
        }
    }

    /**
     * Return true if file system is umounted and file system has separate
     * data and metadata devices.
     * Return true if file system is mounted and user selects to grow metadata
     * devices.
     * Otherwise return false.
     */
    private boolean isAddingMeta() {
        if (!isMounted() && !isCombined()) {
            return true;
        } else if (isMounted()) {
            String addingMeta = (String)
                wizardModel.getValue(GrowWizardMethodView.CHECKBOX_META);
            return
                addingMeta == null ?
                    false :
                    Boolean.valueOf(addingMeta).booleanValue();
        } else {
            return false;
        }
    }

    /**
     * Return true all the time except when user specifically selects not to
     * grow data devices.  This happens only when the file syste is mounted.
     */
    private boolean isAddingData() {
        if (!isMounted()) {
            return true;
        }
        String addingData =
            (String) wizardModel.getValue(GrowWizardMethodView.CHECKBOX_DATA);
        return
            addingData == null ?
                true :
                Boolean.valueOf(addingData).booleanValue();
    }
}
