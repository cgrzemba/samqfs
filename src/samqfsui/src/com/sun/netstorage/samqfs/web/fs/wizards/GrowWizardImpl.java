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

// ident	$Id: GrowWizardImpl.java,v 1.40 2008/03/17 14:43:35 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.ServerInfo;
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
import java.util.Hashtable;
import java.util.Iterator;
import javax.servlet.http.HttpSession;

interface GrowWizardImplData {

    final String name = "GrowWizardImpl";
    final String title = "FSWizard.grow.title";

    final Class[] pageClass = {
        FSWizardMetadataDeviceSelectionPageView.class,
        GrowWizardStripedGroupNumberPageView.class,
        FSWizardStripedGroupDeviceSelectionPageView.class,
        FSWizardDataDeviceSelectionPageView.class,
        GrowWizardSummaryPageView.class,
        WizardResultView.class
    };

    final String[] pageTitle = {
        "FSWizard.metadataDevicePage.title",
        "FSWizard.grow.stripedGroupNumberPage.title",
        "FSWizard.stripedGroupDevicePage.title",
        "FSWizard.dataDevicePage.title",
        "FSWizard.grow.summaryPage.title",
        "wizard.result.steptext"
    };

    final String[][] stepHelp = {
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
        "FSWizard.metadataDevicePage.step.text",
        "FSWizard.grow.stripedGroupNumberPage.step.text",
        "FSWizard.stripedGroupDevicePage.step.text",
        "FSWizard.dataDevicePage.step.text",
        "FSWizard.grow.summaryPage.step.text",
        "wizard.result.steptext"
    };

    final String[] stepInstruction = {
        "FSWizard.metadataDevicePage.instruction.text",
        "FSWizard.grow.stripedGroupNumberPage.instruction.text",
        "FSWizard.stripedGroupDevicePage.instruction.text",
        "FSWizard.dataDevicePage.instruction.text",
        "FSWizard.grow.summaryPage.instruction.text",
        "wizard.result.instruction"
    };

    final String[] cancelmsg = {
        "FSWizard.metadataDevicePage.cancel",
        "FSWizard.grow.stripedGroupNumberPage.cancel",
        "FSWizard.stripedGroupDevicePage.cancel",
        "FSWizard.dataDevicePage.cancel",
        "FSWizard.grow.summaryPage.cancel",
        ""
    };

    final int PAGE_METADATA = 0;
    final int PAGE_STRIPED_GROUP_NUM = 1;
    final int PAGE_STRIPED_GROUP = 2;
    final int PAGE_DATA = 3;
    final int PAGE_SUMMARY = 4;
    final int PAGE_RESULT = 5;


    // The follow are examples of fsPages and qfsPages
    final int[] qfsPages = {
        PAGE_METADATA,
        PAGE_DATA,
        PAGE_SUMMARY,
        PAGE_RESULT
    };
}

public class GrowWizardImpl extends SamWizardImpl {

    public static final String WIZARDPAGEMODELNAME = "GrowPageModelName";
    public static final String WIZARDPAGEMODELNAME_PREFIX = "GrowWizardModel";
    public static final String WIZARDIMPLNAME = GrowWizardImplData.name;
    public static final String WIZARDIMPLNAME_PREFIX = "GrowWizardImpl";
    public static final String WIZARDCLASSNAME =
        "com.sun.netstorage.samqfs.web.fs.wizards.GrowWizardImpl";

    // Constants defining FileSystem Type
    public static final String FSTYPE_KEY = "fsTypeKey";
    public static final String FSTYPE_FS  = "fs";
    public static final String FSTYPE_QFS = "qfs";

    // Variables to store allDevices, selectedMetaDevices, selectedDataDevices
    private DiskCache[] allAllocatableDevices = null;
    private ArrayList selectedDataDevicesList = null;
    private ArrayList selectedMetaDevicesList = null;
    private ArrayList selectedStripedGroupDevicesList = null;

    private FileSystem fileSystemHandle = null;

    private String fsType = FSTYPE_FS;

    private boolean fs = false;
    private boolean wizardInitialized = false;

    // max number of striped groups we can grow a filesystem
    private int availableGroups = 0;
    private int numStripedGroups = -1;

    // Keep this variable even if it may not be in used.  Keep this variable
    // around in case we need to support backward compatibility in the future
    // Variable to hold samfs server api version number
    // api 1.0 = samfs 4.1
    // api 1.1 = samfs 4.2
    // api 1.2 = samfs 4.3
    // api 1.3 = samfs 4.4
    // api 1.4 = samfs 4.5
    // api 1.5 = samfs 4.6
    private String samfsServerAPIVersion = "1.5"; // previous release version

    // Variable to hold previously entered number of striped groups
    private String OLD_NUM_STRIPED_GROUPS = "OLD_NUM_STRIPED_GROUPS";

    // TODO: this should be moved to base class
    protected String serverName = null;
    protected String fsName = null;

    // Static Create method called by the viewbean
    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace2("in create()");
        return  new GrowWizardImpl(requestContext);
    }

    public GrowWizardImpl(RequestContext requestContext) {
        super(requestContext, WIZARDPAGEMODELNAME);
        initializeWizard(requestContext);
        initializeWizardControl(requestContext);
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return
            getWizardWindowModel(
                WIZARDIMPLNAME,
                GrowWizardImplData.title,
                WIZARDCLASSNAME,
                cmdChild);
    }

    // overwrite getPageClass() to put striped group number into wizardModel
    // so that it can be accessed by NewWizardStripedGroupPage
    public Class getPageClass(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        if (pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP) {
            wizardModel.setValue(
                Constants.Wizard.STRIPED_GROUP_NUM,
                new Integer(page - GrowWizardImplData.PAGE_STRIPED_GROUP));
            TraceUtil.trace2("This is striped group # " +
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

    public String[] getFutureSteps(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String[] futureSteps = null;

        int howMany = pages.length - page - 1;
        futureSteps = new String[howMany];

        for (int i = 0; i < howMany; i++) {
            int futureStep = page + 1 + i;
            int futurePage = pages[futureStep];
            if (futurePage == GrowWizardImplData.PAGE_STRIPED_GROUP) {
                futureSteps[i] = SamUtil.getResourceString(
                    stepText[futurePage],
                    new String[] {Integer.toString(
                futureStep - GrowWizardImplData.PAGE_STRIPED_GROUP) });
            } else {
                futureSteps[i] = stepText[futurePage];
            }
        }

        return futureSteps;
    }

    public String getStepText(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String text = null;

        if (pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP) {
            text = SamUtil.getResourceString(
                stepText[pages[page]],
                new String[] { Integer.toString(
                    page - GrowWizardImplData.PAGE_STRIPED_GROUP) });
        } else {
            text = stepText[pages[page]];
        }

        return text;
    }

    public String getStepTitle(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String title = null;

        if (pages[page] == GrowWizardImplData.PAGE_STRIPED_GROUP) {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] { Integer.toString(
                    page - GrowWizardImplData.PAGE_STRIPED_GROUP) });
        } else {
            title = pageTitle[pages[page]];
        }

        return title;
    }

    public boolean nextStep(WizardEvent wizardEvent) {
        String pageId = wizardEvent.getPageId();
        TraceUtil.trace2("Entered with pageID = " + pageId);

        // make this wizard active
        super.nextStep(wizardEvent);

        // when we get here, the wizard must has been initialized
        wizardInitialized = true;

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
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
     * finishStep will collect all data input in each of
     * wizard step then call API to grow the fs
     */
    public boolean finishStep(WizardEvent wizardEvent) {

        String pageId = wizardEvent.getPageId();
        TraceUtil.trace2("Entered with pageID = " + pageId);

        // make sure this wizard is still active before commit
        if (super.finishStep(wizardEvent) == false) {
            return true;
        }

        DiskCache[] metadataDevices = getSelectedDeviceArray(
            selectedMetaDevicesList);
        DiskCache[] dataDevices = getSelectedDeviceArray(
            selectedDataDevicesList);

        StripedGroup[] stripedGroups = null;
        try {
            stripedGroups =
                getSelectedStripedGroupsArray(selectedStripedGroupDevicesList);
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

        SamQFSSystemModel sysModel = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            FileSystem fileSystem =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fileSystem == null) {
                throw new SamFSException(null, -1000);
            }

            // Now grow the FileSystem
            sysModel.getSamQFSSystemFSManager().growFileSystem(
                fileSystem,
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

            TraceUtil.trace2("Done growing FS " + fsName);
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

    public boolean cancelStep(WizardEvent wizardEvent) {
        super.cancelStep(wizardEvent);

        // clear previous error
        wizardModel.setValue(
            Constants.Wizard.WIZARD_ERROR,
            Constants.Wizard.WIZARD_ERROR_NO);
        wizardModel.clear();
        return true;
    }

    public void closeStep(WizardEvent wizardEvent) {
        TraceUtil.trace2("Clearing out wizard model...");
        wizardModel.clear();
        TraceUtil.trace2("Done!");
    }

    /**
     * Private method to process the MetaData Devices page
     */
    private boolean processMetaDataDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");
        TraceUtil.trace2("Entered processMetaDataDevicesPage.., ");
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
            int availableLuns = ((Integer) wizardModel.getValue(
                Constants.Wizard.AVAILABLE_LUNS)).intValue();
            if (selectedRows.length > availableLuns) {
                setWizardAlert(wizardEvent, "FSWizard.maxlun");
                return false;
            }
        }

        // Traverse through the selection and populate selectedDataDevicesList
        String deviceSelected = null;
        DiskCache device = null;
        if (selectedMetaDevicesList == null) {
            selectedMetaDevicesList = new ArrayList();
        }
        selectedMetaDevicesList.clear();

        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            selectedMetaDevicesList.add(deviceSelected);
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_METADEVICES,
            selectedMetaDevicesList);

        // Set the value of the selection to display in the summary page
        Collections.sort(selectedMetaDevicesList);

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

        StringBuffer deviceList = new StringBuffer();
        for (int i = 0; i < selectedMetaDevicesList.size(); i++) {
            deviceList.append((String) selectedMetaDevicesList.get(i)).
                append("<br>");
        }

        return result;
    }

    /**
     * Private method to process the Data Devices page
     */
    private boolean processDataDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");
        TraceUtil.trace2("Entered processDataDevicesPage.., ");

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
        // Traverse through the selection and populate selectedDataDevicesList
        String deviceSelected = null;
        DiskCache device = null;
        if (selectedDataDevicesList == null) {
            selectedDataDevicesList = new ArrayList();
        }
        selectedDataDevicesList.clear();

        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            selectedDataDevicesList.add(deviceSelected);
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_DATADEVICES,
            selectedDataDevicesList);

        // when growing a fs, it's allowed to grow metadata devices only
        // so if user didn't select any data devices, return now
        if (selectedRows.length < 1) {
            if (fsType == FSTYPE_QFS) {
                wizardModel.setValue(
                    GrowWizardSummaryPageView.CHILD_DATA_FIELD, "");
                return true;
            } else {
                setWizardAlert(wizardEvent, "FSWizard.grow.error.data");
                return false;
            }
        } else {
            int availableLuns = ((Integer) wizardModel.getValue(
                Constants.Wizard.AVAILABLE_LUNS)).intValue();
            if (fsType == FSTYPE_QFS) {
                int metaDevices = selectedMetaDevicesList.size();
                if (selectedRows.length > availableLuns - metaDevices) {
                    setWizardAlert(wizardEvent, "FSWizard.maxlun");
                    return false;
                }
            } else {
                if (selectedRows.length > availableLuns) {
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

        StringBuffer deviceList = new StringBuffer();
        for (int i = 0; i < selectedDataDevicesList.size(); i++) {
            deviceList.append((String) selectedDataDevicesList.get(i)).
                append("<br>");
        }
        wizardModel.setValue(
            GrowWizardSummaryPageView.CHILD_DATA_FIELD,
            deviceList.toString());

        return true;
    }


    /**
     * Private method to process the Striped Group Devices page
     */
    private boolean processStripedGroupDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");
        TraceUtil.trace2("Entered processStripedGroupDevicesPage.., ");

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
            int availableLuns = ((Integer) wizardModel.getValue
                (Constants.Wizard.AVAILABLE_LUNS)).intValue();
            int metaLuns = selectedMetaDevicesList.size();
            int totalStripeLuns = 0;
            if (selectedStripedGroupDevicesList != null) {
                for (int j = 0; j < groupNum; j++) {
                    ArrayList groupDevices =
                        (ArrayList) selectedStripedGroupDevicesList.get(j);
                    totalStripeLuns += groupDevices.size();
                }
            }
            int usedLuns = metaLuns + totalStripeLuns + selectedRows.length;
            if (usedLuns > availableLuns) {
                setWizardAlert(wizardEvent, "FSWizard.maxlun");
                return false;
            }
        }
        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);
        // Traverse through the selection and populate
        // selectedStripedGroupDevicesList

        String deviceSelected = null;
        DiskCache device = null;
        if (selectedStripedGroupDevicesList == null) {
            selectedStripedGroupDevicesList = new ArrayList();
        }

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

        StringBuffer deviceList = new StringBuffer();
        for (int j = 0; j < selectedStripedGroupDevicesList.size(); j++) {
            ArrayList groupDevices =
                (ArrayList) selectedStripedGroupDevicesList.get(j);
            deviceList.append(SamUtil.getResourceString(
                "FSWizard.new.stripedGroup.deviceListing",
                new String[] { Integer.toString(j) })).append("<br>");
            for (int i = 0; i < groupDevices.size(); i++) {
                deviceList.append("&nbsp;&nbsp;&nbsp;").
                    append((String) groupDevices.get(i)).
                    append("<br>");
            }
        }

        wizardModel.setValue(
            GrowWizardSummaryPageView.CHILD_DATA_FIELD,
            deviceList.toString());

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
        String numGroups = (String) wizardModel.getWizardValue(
            GrowWizardStripedGroupNumberPageView.
                CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD);

        if (numGroups != null) {
            try {
                numStripedGroups = Integer.parseInt(numGroups);
            } catch (NumberFormatException nfe) {
            }
        }
        if (numStripedGroups < 0 || numStripedGroups > availableGroups) {
            setWizardAlert(wizardEvent,
                SamUtil.getResourceString("FSWizard.grow.error.numStripedGroup",
                    new String[] { Integer.toString(availableGroups) }));
            return false;
        }

        generateWizardPages(numStripedGroups);
        initializeWizardPages(pages);

        // set data field in summary page
        if (numStripedGroups == 0) {
            wizardModel.setValue(
            GrowWizardSummaryPageView.CHILD_DATA_FIELD, "");
        }

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

    private void getFileSystemHandle() throws SamFSException {
        TraceUtil.trace3("Entering");
        int availableLuns = -1;

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        fileSystemHandle =
            sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
        if (fileSystemHandle == null) {
            throw new SamFSException(null, -1000);
        }

        // Set the FSType in the model
        fsType = FSTYPE_FS;
        if (fileSystemHandle.getFSType() == FileSystem.SEPARATE_METADATA) {
            fsType = FSTYPE_QFS;
        }
        wizardModel.setValue(FSTYPE_KEY, fsType);

        // get the available lun to grow for samfs
        if (fsType == FSTYPE_FS) {
            int dataDeviceLuns = fileSystemHandle.getDataDevices().length;
            availableLuns = Constants.Wizard.MAX_LUNS - dataDeviceLuns;
        }

        StripedGroup[] group = fileSystemHandle.getStripedGroups();
        int stripedDeviceLuns = 0;
        if (group != null && group.length > 0) {
            availableGroups =
                Constants.Wizard.MAX_STRIPED_GROUPS - group.length;
            numStripedGroups = 0;
            // used devices for striped group
            for (int i = 0; i < group.length; i++)
                stripedDeviceLuns += group[i].getMembers().length;
            TraceUtil.trace2("group length = " + group.length);
            TraceUtil.trace2("available num of groups = " + availableGroups);
        }

        if (fsType == FSTYPE_QFS) {
            int metaDeviceLuns = fileSystemHandle.getMetadataDevices().length;
            // retrieve data if any
            int dataDevicesLuns = 0;
            DiskCache[] dataDevices = fileSystemHandle.getDataDevices();
            if (dataDevices != null && dataDevices.length > 0)
                dataDevicesLuns = dataDevices.length;
            availableLuns = Constants.Wizard.MAX_LUNS - metaDeviceLuns -
                dataDevicesLuns - stripedDeviceLuns;
        }

        wizardModel.setValue(
            Constants.Wizard.AVAILABLE_STRIPED_GROUPS,
            new Integer(availableGroups));

        wizardModel.setValue(
            Constants.Wizard.AVAILABLE_LUNS,
            new Integer(availableLuns));
    }

    /**
     * Utility method to return an Array of DataDevices
     * from an ArrayList of device paths
     */
    private DiskCache[] getSelectedDeviceArray(ArrayList selectedDevicePaths) {
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
    private StripedGroup[] getSelectedStripedGroupsArray(ArrayList groupList)
        throws SamFSException {

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
            DiskCache[] disks = getSelectedDeviceArray(groupDevices);
            stripedGroups[i] = sysModel.getSamQFSSystemFSManager().
                createStripedGroup("",  disks);
        }

        return stripedGroups;
    }

    private DiskCache getDiskCacheObject(String devicePath) {
        for (int i = 0; i < allAllocatableDevices.length; i++) {
            if (allAllocatableDevices[i].getDevicePath().equals(devicePath)) {
                return allAllocatableDevices[i];
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
        DiskCache[] devices = (DiskCache[]) wizardModel.getValue(
            Constants.Wizard.ALLOCATABLE_DEVICES);

        if (devices != null) {
            return;
        }

        if (allAllocatableDevices == null) {
            TraceUtil.trace2(
                "DeviceList is Null., Getting devices from sysmodel");
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            allAllocatableDevices =
                sysModel.getSamQFSSystemFSManager().
                    discoverAvailableAllocatableUnits(null);
        }

        wizardModel.setValue(
            Constants.Wizard.ALLOCATABLE_DEVICES,
            allAllocatableDevices);
    }

    private void setExceptionInModel(SamFSException ex) {
        wizardModel.setValue(
            Constants.Wizard.WIZARD_ERROR, Constants.Wizard.WIZARD_ERROR_YES);
        wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
        wizardModel.setValue(
            Constants.Wizard.ERROR_CODE, Integer.toString(ex.getSAMerrno()));
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
        TraceUtil.trace2("fsNameParam = " + fsName +
            ", serverNameParam = " + serverName);

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            populateDevicesInWizardModel();
            getFileSystemHandle();

            // get samfs server api version number
            // TODO: this is not concurrent
            HttpSession session =
                RequestManager.getRequestContext().getRequest().getSession();
            Hashtable serverTable = (Hashtable) session.getAttribute(
                Constants.SessionAttributes.SAMFS_SERVER_INFO);

            if (serverTable != null && serverName != null) {
                ServerInfo serverInfo = (ServerInfo)
                    serverTable.get(serverName);
                if (serverInfo != null) {
                    // get samfs server api version number
                    samfsServerAPIVersion =
                        serverInfo.getSamfsServerAPIVersion();
                    TraceUtil.trace2("got samfsServerAPIVersion from cache: " +
                        samfsServerAPIVersion);
                } // else defaults to initialized value 1.1
            }
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

        TraceUtil.trace2("samfsServerAPIVersion = " + samfsServerAPIVersion);

        // store samfs server api version number in wizard model so that
        // it can be accessed in all wizard pages
        wizardModel.setValue(
            Constants.Wizard.SERVER_API_VERSION, samfsServerAPIVersion);

        // now generate pages based on fs type
        generateWizardPages(numStripedGroups);
        setShowResultsPage(true);
        initializeWizardPages(pages);

        TraceUtil.trace2("wizard initialized!");
    }

    private void generateWizardPages(int stripedGroups) {
        int totalPages = 2; // summary & result page
        if (fsType == FSTYPE_QFS) {
            totalPages++; // metadata page
        }
        if (stripedGroups < 0) {
            totalPages++; // data page
        } else {
            totalPages++; // striped group number page
            totalPages += stripedGroups; // striped group pages
        }

        pages = new int[totalPages];
        int pageCount = 0;

        if (fsType == FSTYPE_QFS) {
            pages[pageCount++] = GrowWizardImplData.PAGE_METADATA;
        }

        if (stripedGroups < 0) {
            pages[pageCount++] = GrowWizardImplData.PAGE_DATA;
        } else {
            pages[pageCount++] = GrowWizardImplData.PAGE_STRIPED_GROUP_NUM;
            for (int i = 0; i < stripedGroups; i++) {
                pages[pageCount++] = GrowWizardImplData.PAGE_STRIPED_GROUP;
            }
        }

        pages[pageCount++] = GrowWizardImplData.PAGE_SUMMARY;
        pages[pageCount] = GrowWizardImplData.PAGE_RESULT;
    }
}
