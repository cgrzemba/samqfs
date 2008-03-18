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

// ident	$Id: AddLibraryImpl.java,v 1.46 2008/03/17 14:43:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.model.ModelControlException;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.StkClntConn;
import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.WizardResultView;

import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;

import javax.servlet.http.HttpServletRequest;


interface AddLibraryImplData {

    final String name  = "AddLibraryImpl";
    final String title = "AddLibrary.title";

    final int PAGE_SELECT_TYPE  = 0;
    final int PAGE_DIRECT_SELECT_LIBRARY = 1;
    final int PAGE_SELECT_MEDIA_TYPE = 2;
    final int PAGE_NETWORK_SELECT_NAME = 3;
    final int PAGE_ACSLS_SELECT_LIBRARY = 4;
    final int PAGE_ACSLS_PARAM = 5;
    final int PAGE_SUMMARY = 6;
    final int PAGE_RESULT = 7;

    final int[] directPrefOffPages  = {
        PAGE_SELECT_TYPE,
        PAGE_DIRECT_SELECT_LIBRARY,
        PAGE_SUMMARY,
        PAGE_RESULT
    };

    final int[] directMixedMediaPrefOffPages  = {
        PAGE_SELECT_TYPE,
        PAGE_DIRECT_SELECT_LIBRARY,
        PAGE_SELECT_MEDIA_TYPE,
        PAGE_SUMMARY,
        PAGE_RESULT
    };

    final int[] networkPrefOffPages = {
        PAGE_SELECT_TYPE,
        PAGE_NETWORK_SELECT_NAME,
        PAGE_SUMMARY,
        PAGE_RESULT
    };

    final int[] acslsPrefOffPages = {
        PAGE_SELECT_TYPE,
        PAGE_ACSLS_SELECT_LIBRARY,
        PAGE_ACSLS_PARAM,
        PAGE_SUMMARY,
        PAGE_RESULT
    };


    final Class[] pageClass = {
        AddLibrarySelectTypeView.class,
        AddLibraryDirectSelectLibraryView.class,
        AddLibrarySelectDriveView.class,
        AddLibraryNetworkSelectNameView.class,
        AddLibraryACSLSSelectLibraryView.class,
        AddLibraryACSLSParamView.class,
        AddLibrarySummaryView.class,
        WizardResultView.class
    };

    final String[] pageTitle = {
        "AddLibrary.page1.steptext",
        "AddLibraryDirect.page2.steptext",
        "AddLibrary.selectdrive.steptext",
        "AddLibraryNetwork.page3.steptext",
        "AddLibrary.acsls.selectlibrary.steptitle",
        "AddLibrary.acsls.param.steptitle",
        "AddLibraryDirect.summary.steptext",
        "wizard.result.steptext"
    };

    final String[][] stepHelp = {
        {"AddLibrary.page1.help.text1",
            "AddLibrary.page1.help.text2"},
        {"AddLibraryDirect.page2.help.text1",
            "AddLibraryDirect.page2.help.text2"},
        {"AddLibrary.selectdrive.help.text1",
            "AddLibrary.selectdrive.help.text2"},
        {"AddLibraryNetwork.page3.help.text1",
            "AddLibraryNetwork.page3.help.text2"},
        {"AddLibrary.acsls.selectlibrary.help.1",
            "AddLibrary.acsls.selectlibrary.help.2"},
        {"AddLibrary.acsls.param.help.1",
            "AddLibrary.acsls.param.help.2",
             "AddLibrary.acsls.param.help.3",
             "AddLibrary.acsls.param.help.4"},
        {"AddLibrary.summary.help.text1",
            "AddLibrary.summary.help.text2"},
        {"wizard.result.help.text1",
            "wizard.result.help.text2"}
    };

    final String[] stepText = {
        "AddLibrary.page1.steptext",
        "AddLibraryDirect.page2.steptext",
        "AddLibrary.selectdrive.steptext",
        "AddLibraryNetwork.page3.steptext",
        "AddLibrary.acsls.selectlibrary.steptitle",
        "AddLibrary.acsls.param.steptitle",
        "AddLibraryDirect.summary.steptext",
        "wizard.result.steptext"
    };

    final String[] stepInstruction = {
        "AddLibrary.page1.instruction",
        "AddLibraryDirect.page2.instruction",
        "AddLibrary.selectdrive.instruction",
        "AddLibraryNetwork.page3.instruction",
        "AddLibrary.acsls.selectlibrary.instruction",
        "AddLibrary.acsls.param.instruction",
        "AddLibrary.summary.direct.instruction",
        "wizard.result.instruction"
    };
}

public class AddLibraryImpl extends SamWizardImpl {

    public static final String WIZARDPAGEMODELNAME = "AddLibPageModelName";
    public static final String WIZARDPAGEMODELNAME_PREFIX = "WizardModel";
    public static final String WIZARDIMPLNAME = AddLibraryImplData.name;
    public static final String WIZARDIMPLNAME_PREFIX = "WizardImpl";
    public static final String WIZARDCLASSNAME =
        "com.sun.netstorage.samqfs.web.media.wizards.AddLibraryImpl";

    // Key of the library object of which is working on
    public static final String MY_LIBRARY  = "myLibrary";

    // used for mixed media libraries (direct-attached)
    public static final String DRIVE_COUNT = "driveCount";
    private boolean hasMixedMedia = false;

    private Drive[] sortedDrives = null;
    private String attached = null;

    private boolean wizardInitialized = false;

    // Keep track of the serial number of ACSLS library of which is going
    // to be added
    public static final
        String SA_ACSLS_SERIAL_NO = "sessionattr_acsls_serial";

    // Keep track of an array of ACSLS Libraries that are about to be added
    public static final
        String SA_STK_LIBRARY_ARRAY = "sessionattr_stklib_array";

    // Keep track of an array of ACSLS Libraries returned by
    // Media.discoverStkLibraries (JNI Layer)
    public static final
        String SA_DISCOVERD_STK_LIBRARY_ARRAY  = "sessionattr_dis_stklib_array";

    // Keep track of an array of Direct Attached Libraries returned by
    // Media.discoverUnused (JNI Layer)
    public static final
        String SA_DISCOVERD_DA_LIBRARY_ARRAY  = "sessionattr_dis_dalib_array";

    // Keep track of the index of ACSLS Library of which the code is currently
    // referred to collect/present information in ACSLS Param Page
    public static final
        String SA_STK_LIBRARY_INDEX = "sessionattr_stklib_index";

    // Keep track of the stk virtual library types (Array of String)
    public static final
        String SA_STK_MEDIA_TYPES = "sessionattr_stklib_mediatypes";

    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        return new AddLibraryImpl(requestContext);
    }

    public AddLibraryImpl(RequestContext requestContext) {
        super(requestContext, WIZARDPAGEMODELNAME);
        processInitialRequest(requestContext.getRequest());
        initializeWizard();
        initializeWizardControl(requestContext);
    }

    // initialize wizard
    private void initializeWizard() {
        TraceUtil.trace3("Entering");

        wizardName  = AddLibraryImplData.name;
        wizardTitle = AddLibraryImplData.title;
        pageClass   = AddLibraryImplData.pageClass;
        pageTitle   = AddLibraryImplData.pageTitle;
        stepHelp  = AddLibraryImplData.stepHelp;
        stepText  = AddLibraryImplData.stepText;
        stepInstruction = AddLibraryImplData.stepInstruction;
        pages = AddLibraryImplData.directPrefOffPages;

        setShowResultsPage(true);
        initializeWizardPages(pages);

        // set default ACSLS Port Number
        wizardModel.setValue(
            AddLibrarySelectTypeView.ACSLS_PORT_NUMBER,
            Integer.toString(Constants.Media.ACSLS_DEFAULT_PORT));
        TraceUtil.trace3("Exiting");
    }

    private void processInitialRequest(HttpServletRequest request) {
        String serverName  = (String) request.getParameter(
            Constants.WizardParam.SERVER_NAME_PARAM);
        String samfsServerAPIVersion = (String) request.getParameter(
            Constants.WizardParam.SERVER_VERSION_PARAM);

        // Version is retrieved by not used in 4.6.  I leave it here just in
        // case if we need version information in the future

        if (serverName != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Wizard Key: serverName is ").append(serverName).toString());
        } else {
            serverName = "";
            SamUtil.doPrint("Wizard Key: serverName is null");
        }

        wizardModel.setValue(Constants.Wizard.SERVER_NAME, serverName);

        TraceUtil.trace2(new NonSyncStringBuffer(
            "AddLibraryImpl: serverName is ").append(serverName).toString());
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return
            getWizardWindowModel(
                WIZARDIMPLNAME,
                AddLibraryImplData.title,
                WIZARDCLASSNAME,
                cmdChild);
    }

    public boolean cancelStep(WizardEvent wizardEvent) {
        super.cancelStep(wizardEvent);
        SamUtil.doPrint("Start clearing wizard Model ...");
        // Clear the wizardModel
        wizardModel.clear();
        SamUtil.doPrint("Done clearing wizard Model ...");
        return true;
    }

    public void closeStep(WizardEvent wizardEvent) {
        SamUtil.doPrint("Start clearing wizard Model ...");
        // Clear the wizardModel
        wizardModel.clear();
        SamUtil.doPrint("Done clearing wizard Model ...");
    }

    /**
     * getFuturePages() (Overridden)
     * Need to override this method because we only need to show the first
     * step in the first page.  Based on the user selection in the first page,
     * the steps will be updated.
     */
    public String[] getFuturePages(String currentPageId) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering getFuturePages(): currentPageId is ").
            append(currentPageId).toString());
        int page = pageIdToPage(currentPageId);

        String [] futurePages = null;
        if (pages[page] == AddLibraryImplData.PAGE_SELECT_TYPE ||
            pages[page] == AddLibraryImplData.PAGE_DIRECT_SELECT_LIBRARY ||
            pages[page] == AddLibraryImplData.PAGE_ACSLS_SELECT_LIBRARY) {
            futurePages = new String[0];
        } else {
            int howMany = pages.length - page + 1;
            futurePages = new String[howMany];
            for (int i = 0; i < howMany; i++) {
                // No conversion
                futurePages[i] = Integer.toString(page + i);
            }
        }

        TraceUtil.trace3("Exiting");
        return futurePages;
    }

    public String[] getFutureSteps(String currentPageId) {
        TraceUtil.trace3("Entering getFutureSteps()");
        int page = pageIdToPage(currentPageId);
        String[] futureSteps = null;

        if (pages[page] == AddLibraryImplData.PAGE_SELECT_TYPE ||
            pages[page] == AddLibraryImplData.PAGE_ACSLS_SELECT_LIBRARY ||
            pages[page] == AddLibraryImplData.PAGE_DIRECT_SELECT_LIBRARY) {
            futureSteps = new String[0];
        } else {
            int howMany = pages.length - page - 1;

            futureSteps = new String[howMany];

            for (int i = 0; i < howMany; i++) {
                int futureStep = page + 1 + i;
                int futurePage = pages[futureStep];
                if (futurePage == AddLibraryImplData.PAGE_ACSLS_PARAM) {
                    String [] mediaTypes =
                        (String []) wizardModel.getValue(SA_STK_MEDIA_TYPES);
                    futureSteps[i] = SamUtil.getResourceString(
                        stepText[futurePage],
                        new String[] {
                            mediaTypes[getStkLibraryIndex(futureStep)]});
                } else {
                    futureSteps[i] = stepText[futurePage];
                }
            }
        }

        TraceUtil.trace3("Exiting");
        return futureSteps;
    }

    // overwrite getPageClass() to clear the WIZARD_ERROR value when
    // the wizard is initialized
    public Class getPageClass(String pageId) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);

        // clear out previous errors if wizard has been initialized
        if (wizardInitialized) {
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        }

        // Set ACSLS HASHMAP INDEX to the corresponding hashMap entry so the
        // View knows which StkNetLibParam needs to be pulled from the
        // wrapper array to pre-populate fields
        if (pages[page] == AddLibraryImplData.PAGE_ACSLS_PARAM) {
            wizardModel.setValue(
                SA_STK_LIBRARY_INDEX,
                new Integer(
                    getStkLibraryIndex(page)));
            TraceUtil.trace2(new StringBuffer("This is STK Library # ").append(
                getStkLibraryIndex(page)).toString());
        }

        return super.getPageClass(pageId);
    }

    public String getStepText(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);
        String [] mediaTypes =
            (String []) wizardModel.getValue(SA_STK_MEDIA_TYPES);

        if (pages[page] == AddLibraryImplData.PAGE_ACSLS_PARAM) {
            return SamUtil.getResourceString(
                stepText[pages[page]],
                new String[] {
                    mediaTypes[getStkLibraryIndex(page)]});
        } else {
            return stepText[pages[page]];
        }
    }

    public String getStepTitle(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        int page = pageIdToPage(pageId);
        String [] mediaTypes =
            (String []) wizardModel.getValue(SA_STK_MEDIA_TYPES);

        if (pages[page] == AddLibraryImplData.PAGE_ACSLS_PARAM) {
            return SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    mediaTypes[getStkLibraryIndex(page)]});
        } else {
            return pageTitle[pages[page]];
        }
    }

    public String[] getStepHelp(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);
        return stepHelp[pages[pageIdToPage(pageId)]];
    }

    public String getStepInstruction(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);
        String instruction = null;

        // set different instruction content based on the type of summary page
        if (pages[page] == AddLibraryImplData.PAGE_SUMMARY) {
            if (attached.equals("AddLibrary.type.direct")) {
                instruction = SamUtil.getResourceString(
                    "AddLibrary.summary.direct.instruction");
            } else if (attached.equals("AddLibrary.type.network")) {
                instruction = SamUtil.getResourceString(
                    "AddLibrary.summary.network.instruction");
            } else if (attached.equals("AddLibrary.type.acsls")) {
                instruction = SamUtil.getResourceString(
                    "AddLibrary.summary.acsls.instruction");
            } else {
                instruction = stepInstruction[pages[page]];
            }
        } else {
            instruction = stepInstruction[pages[page]];
        }

        return instruction;
    }

    public boolean nextStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering nextStep()");

        // make this wizard active
        super.nextStep(wizardEvent);

        // when we get here, the wizard must has been initialized
        wizardInitialized = true;

        String pageId = wizardEvent.getPageId();
        boolean result = true;
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "nextStep() pageID: ").append(pageId).toString());

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case AddLibraryImplData.PAGE_SELECT_TYPE:
                result = processSelectTypePage(wizardEvent);
                break;

            case AddLibraryImplData.PAGE_DIRECT_SELECT_LIBRARY:
                result = processDirectSelectLibraryPage(wizardEvent);
                break;

            case AddLibraryImplData.PAGE_SELECT_MEDIA_TYPE:
                result = processSelectMediaTypePage(wizardEvent, true);
                break;

            case AddLibraryImplData.PAGE_ACSLS_SELECT_LIBRARY:
                result = processACSLSSelectLibraryPage(wizardEvent, true);
                break;

            case AddLibraryImplData.PAGE_ACSLS_PARAM:
                result = processACSLSParamPage(wizardEvent, true);
                break;

            case AddLibraryImplData.PAGE_NETWORK_SELECT_NAME:
                result = processNetworkSelectNamePage(wizardEvent);
                break;
        }

        TraceUtil.trace3("Exiting");
        return result;
    }

    public boolean previousStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Calling previousStep");

        // update WizardManager
        super.previousStep(wizardEvent);

        String pageId = wizardEvent.getPageId();
        boolean result = true;
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "previousStep() pageID: ").append(pageId).toString());

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case AddLibraryImplData.PAGE_SELECT_MEDIA_TYPE:
                result = processSelectMediaTypePage(wizardEvent, false);
                break;

            case AddLibraryImplData.PAGE_ACSLS_SELECT_LIBRARY:
                result = processACSLSSelectLibraryPage(wizardEvent, false);
                break;

            case AddLibraryImplData.PAGE_ACSLS_PARAM:
                result = processACSLSParamPage(wizardEvent, false);
                break;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean finishStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        // make sure this wizard is still active before commit
        if (!super.finishStep(wizardEvent)) {
            return true;
        }

        if (attached.equals("AddLibrary.type.direct")) {
            addSingleLibrary(true);
        } else if (attached.equals("AddLibrary.type.network")) {
            addSingleLibrary(false);
        } else if (attached.equals("AddLibrary.type.acsls")) {
            addACSLSLibrary();
        } else {
            TraceUtil.trace1("Developer's bug found in Impl::finishStep!");
            return false;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private void addSingleLibrary(boolean isDirectAttached) {

        Library myLibrary = getMyLibrary();
        String libName    = null;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            libName = myLibrary.getName();

            // use default catalog value
            myLibrary.setCatalogLocation(
                Constants.Wizard.DEFAULT_CATALOG_LOCATION.concat(libName));

            // Only for direct-attached libraries
            // This routine will remove drives that have a different media type
            // as selected (if applicable)
            if (isDirectAttached) {
                updateDriveList();
            }

            SamUtil.doPrint(new NonSyncStringBuffer("GOING to add: ").
                append(myLibrary.getName()).toString());

            LogUtil.info(this.getClass(),
                "finishStep",
                "Start adding Direct-attached Tape Library");

            if (isDirectAttached) {
                LogUtil.info(this.getClass(),
                    "finishStep",
                    "Start adding Direct-attached Tape Library");
                if (sortedDrives == null) {
                    sysModel.getSamQFSSystemMediaManager().
                        addLibrary(myLibrary);
                } else {
                    sysModel.getSamQFSSystemMediaManager().
                        addLibrary(myLibrary, sortedDrives);
                }
            } else {
                LogUtil.info(this.getClass(),
                    "finishStep",
                    "Start adding Network-attached Tape Library");
                sysModel.getSamQFSSystemMediaManager().addNetworkLibrary(
                    myLibrary, sortedDrives);
            }

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to add library " + libName);
            LogUtil.info(this.getClass(),
                "finishStep",
                "FAILED to add Tape Library " + libName);
            SamUtil.processException(
                samEx,
                this.getClass(),
                "finishStep",
                "Failed to add Tape Library",
                getServerName());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(samEx.getSAMerrno()));
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "LibrarySummary.error.add");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                samEx.getMessage());
            return;
        }

        LogUtil.info(this.getClass(),
            "finishStep",
            "Done adding tape Library");

        wizardModel.setValue(
            Constants.AlertKeys.OPERATION_RESULT,
            Constants.AlertKeys.OPERATION_SUCCESS);
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
            "success.summary");
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
            SamUtil.getResourceString(
                "LibrarySummary.action.add",
                libName));
    }

    private void addACSLSLibrary() {
        Library [] myStkLibraryArray =
            (Library []) wizardModel.getValue(SA_STK_LIBRARY_ARRAY);
        NonSyncStringBuffer addLibString = new NonSyncStringBuffer();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            for (int i = 0; i < myStkLibraryArray.length; i++) {
                String libName = myStkLibraryArray[i].getName();

                if (libName != null) {
                    if (addLibString.length() != 0) {
                        addLibString.append(", ");
                    }
                    addLibString.append(libName);
                }
            }

            LogUtil.info(this.getClass(),
                "finishStep",
                "Start adding ACSLS Library" + addLibString.toString());
            sysModel.
                getSamQFSSystemMediaManager().addLibraries(myStkLibraryArray);
            LogUtil.info(this.getClass(),
                "finishStep",
                "Done adding ACSLS Library" + addLibString.toString());
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to add libraries " + addLibString.toString());
            LogUtil.info(this.getClass(),
                "finishStep",
                "FAILED to add Tape Libraries " + addLibString.toString());
            SamUtil.processException(
                samEx,
                this.getClass(),
                "finishStep",
                "Failed to add Tape Libraries",
                getServerName());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(samEx.getSAMerrno()));
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "LibrarySummary.error.add");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                samEx.getMessage());
            return;
        }

        wizardModel.setValue(
            Constants.AlertKeys.OPERATION_RESULT,
            Constants.AlertKeys.OPERATION_SUCCESS);
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
            "success.summary");
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
            SamUtil.getResourceString(
                "LibrarySummary.action.add",
                addLibString.toString()));
    }

    /**
     * setErrorMessage(wizardEvent, errMsg)
     * Set the error message in the wizard
     */

    private void setErrorMessage(WizardEvent wizardEvent, String errMsg) {
        wizardEvent.setSeverity(WizardEvent.ACKNOWLEDGE);
        wizardEvent.setErrorMessage(errMsg);
    }

    /**
     * setWizardModelErrorMessage(message, errorcode)
     * Set the DETAIL_CODE and DETAIL_MSG in wizardModel
     */
    private void setWizardModelErrorMessage(String message, int errorCode) {
        wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
            Constants.Wizard.WIZARD_ERROR_YES);
        wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE, message);
        wizardModel.setValue(Constants.Wizard.ERROR_CODE,
            Integer.toString(errorCode));
    }
    /**
     * processSelectTypePage(wizardEvent)
     * Process the necessary information in Select Type page
     * Return boolean (true for success, false for fail)
     */

    private boolean processSelectTypePage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        AddLibrarySelectTypeView pageView =
            (AddLibrarySelectTypeView) wizardEvent.getView();

        String errMsg = pageView.getErrorMsg();
        if (errMsg != null) {
            setErrorMessage(wizardEvent, errMsg);
            return false;
        }

        attached = (String) pageView.getDisplayFieldValue(
                AddLibrarySelectTypeView.LIBRARY_TYPE);
        TraceUtil.trace3("Attached method: " + attached);
        if (attached.equals("AddLibrary.type.direct")) {
            // Direct Attached
            pages = AddLibraryImplData.directPrefOffPages;

            // Discover direct attached libraries
            discoverDirectAttachedLibraries();

        } else if (attached.equals("AddLibrary.type.acsls")) {
            // acsls
            pages = AddLibraryImplData.acslsPrefOffPages;

            // Discover STK Libraries
            discoverSTKLibraries();

        } else {
            // others
            pages = AddLibraryImplData.networkPrefOffPages;
        }

        initializeWizardPages(pages);

        wizardModel.setValue(
            AddLibrarySummaryView.CHILD_ATTACHED_FIELD,
            attached);

        // reset the Drive array to null
        sortedDrives = null;

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * processDirectSelectLibraryPage(wizardEvent)
     * Process the necessary information in Direct Select Library page
     * Return boolean (true for success, false for fail)
     */

    private boolean processDirectSelectLibraryPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        AddLibraryDirectSelectLibraryView pageView =
            (AddLibraryDirectSelectLibraryView) wizardEvent.getView();
        String errMsg = pageView.getErrorMsg();
        if (errMsg != null) {
            setErrorMessage(wizardEvent, errMsg);
            return false;
        }

        String selectedValue =
            (String) pageView.getDisplayFieldValue(pageView.CHILD_DROPDOWNMENU);
        String nameValue = (String)
            pageView.getDisplayFieldValue(pageView.CHILD_NAME_FIELD);
        nameValue = nameValue == null ? "" : nameValue.trim();

        String vendor = null, productID = null;
        String name = null;
        Library myLibrary = null;

        TraceUtil.trace3(new NonSyncStringBuffer(
            "NP: selectedValue is ").append(selectedValue).toString());

        try {
            Library[] allLibrary =
                (Library []) wizardModel.getValue(
                    SA_DISCOVERD_DA_LIBRARY_ARRAY);
            int selectedIndex    = Integer.parseInt(selectedValue);
            myLibrary = allLibrary[selectedIndex];
            vendor    = myLibrary.getVendor();
            productID = myLibrary.getProductID();
            name = myLibrary.getName();

            // Library name cannot have a space, use underscroll
            name = SamUtil.replaceSpaceWithUnderscore(name);
            myLibrary.setName(name);

            // Library Name field is optional.  If it is blank, use default
            // name that is already set above
            if (nameValue.length() != 0) {
                myLibrary.setName(nameValue);
            }

            if (myLibrary.containMixedMedia()) {
                TraceUtil.trace2("LIBRARY CONTAINS MIXED MEDIA!");
                hasMixedMedia = true;
                pages = AddLibraryImplData.directMixedMediaPrefOffPages;
                initializeWizardPages(pages);
            } else {
                hasMixedMedia = false;
                wizardModel.setValue(
                    AddLibrarySummaryView.CHILD_MEDIA_TYPE_FIELD,
                    SamUtil.getMediaTypeString(myLibrary.getMediaType()));
                TraceUtil.trace2("LIBRARY DOES NOT CONTAIN MIXED MEDIA!");
            }
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "processDirectSelectLibraryName()",
                "Failed to retrieve library information",
                getServerName());
            setWizardModelErrorMessage(ex.getMessage(), -2509);
        }

        wizardModel.setValue(MY_LIBRARY, myLibrary);

        wizardModel.setValue(
            AddLibrarySummaryView.CHILD_VENDORID_FIELD,
            vendor);
        wizardModel.setValue(
            AddLibrarySummaryView.CHILD_PRODUCTID_FIELD,
            productID);

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * processSelectMediaTypePage(wizardEvent)
     * Process the selected media type information of the drives that are
     * going to be included in the newly added library
     * @return Return boolean (true for success, false for fail)
     */

    private boolean processSelectMediaTypePage(
        WizardEvent wizardEvent, boolean isNextStep) {
        TraceUtil.trace3("Entering");

        try {
            AddLibrarySelectDriveView pageView =
                (AddLibrarySelectDriveView) wizardEvent.getView();
            CCActionTable actionTable =
                (CCActionTable) pageView.getChild(
                    AddLibrarySelectDriveView.CHILD_ACTIONTABLE);
            actionTable.restoreStateData();
            CCActionTableModel driveModel =
                 (CCActionTableModel) actionTable.getModel();

            Integer [] selectedRows = driveModel.getSelectedRows();

            // display an error message if no row is selected
            if (selectedRows.length == 0) {
                if (isNextStep) {
                    setErrorMessage(
                        wizardEvent, "AddLibrary.selectdrive.errmsg.chooseone");
                    TraceUtil.trace3("Exiting");
                    return false;
                } else {
                    // It is not an error that the user clicks PREVIOUS
                    // without selecting anything
                    wizardModel.setValue(
                        AddLibrarySummaryView.CHILD_MEDIA_TYPE_FIELD, "");
                    TraceUtil.trace3("Exiting");
                    return true;
                }
            }

            int row = selectedRows[0].intValue();
            driveModel.setRowIndex(row);

            String mType = (String) driveModel.getValue("TypeText");
            Integer driveCount =
                (Integer) driveModel.getValue("DriveCountText");
            int mediaType  = SamUtil.getMediaType(mType);

            wizardModel.setValue(
                AddLibrarySummaryView.CHILD_MEDIA_TYPE_FIELD,
                SamUtil.getMediaTypeString(mediaType));
            wizardModel.setValue(DRIVE_COUNT, driveCount);

            SamUtil.doPrint(new NonSyncStringBuffer(
                "Selected Media Type is ").append(
                SamUtil.getMediaTypeString(mediaType)).toString());
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "processSelectMediaTypePage",
                "Failed to save media type selections",
                getServerName());
            wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE,
                mcex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * processACSLSSelectLibraryPage(wizardEvent)
     * Process the selected ACSLS Library that is about to add under
     * FSM Provisioning
     * @return Return boolean (true for success, false for fail)
     */

    private boolean processACSLSSelectLibraryPage(
        WizardEvent wizardEvent, boolean isNextStep) {
        TraceUtil.trace3("Entering");

        try {
            AddLibraryACSLSSelectLibraryView pageView =
                (AddLibraryACSLSSelectLibraryView) wizardEvent.getView();
            CCActionTable actionTable =
                (CCActionTable) pageView.getChild(
                    AddLibraryACSLSSelectLibraryView.CHILD_ACTIONTABLE);
            actionTable.restoreStateData();
            CCActionTableModel libModel =
                 (CCActionTableModel) actionTable.getModel();

            Integer [] selectedRows = libModel.getSelectedRows();

            // display an error message if no row is selected
            if (selectedRows.length == 0) {
                if (isNextStep) {
                    setErrorMessage(
                        wizardEvent,
                        "AddLibrary.acsls.selectlibrary.error.chooseone");
                    TraceUtil.trace3("Exiting");
                    return false;
                } else {
                    // It is not an error that the user clicks PREVIOUS
                    // without selecting anything
                    wizardModel.setValue(SA_ACSLS_SERIAL_NO, "");
                    TraceUtil.trace3("Exiting");
                    return true;
                }
            }

            int row = selectedRows[0].intValue();
            libModel.setRowIndex(row);

            String serialNumber = (String) libModel.getValue("SerialHidden");
            String types = (String) libModel.getValue("TypeHidden");
            wizardModel.setValue(SA_ACSLS_SERIAL_NO, serialNumber);

            Library [] discoveredLibrariesFromJNI = (Library [])
                wizardModel.getValue(SA_DISCOVERD_STK_LIBRARY_ARRAY);

            try {
                createStkLibrariesArray(
                    discoveredLibrariesFromJNI, serialNumber, types);
            } catch (SamFSException samEx) {
                setErrorMessage(
                    wizardEvent,
                    "AddLibrary.acsls.selectlibrary.error.savesetting");
                TraceUtil.trace3("Exiting");
                return false;
            }
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "processACSLSSelectLibraryPage",
                "Failed to save ACSLS Library selections",
                getServerName());
            wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE,
                mcex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
        }

        // Depends on the number of virtual libraries that are about to be
        // be added, update "pages" and set up the loop steps here
        int numOfLoops = getStkLibrariesArraySize();
        pages = new int[4 + numOfLoops];
        pages[0] = AddLibraryImplData.PAGE_SELECT_TYPE;
        pages[1] = AddLibraryImplData.PAGE_ACSLS_SELECT_LIBRARY;
        for (int i = 0; i < numOfLoops; i++) {
            pages[2 + i] = AddLibraryImplData.PAGE_ACSLS_PARAM;
        }
        pages[2 + numOfLoops] = AddLibraryImplData.PAGE_SUMMARY;
        pages[3 + numOfLoops] = AddLibraryImplData.PAGE_RESULT;

        initializeWizardPages(pages);

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * processACSLSParamPage(wizardEvent)
     * Process the ACSLS Library parameters (per virtual library)
     * that is about to add under FSM Provisioning
     * @return Return boolean (true for success, false for fail)
     */

    private boolean processACSLSParamPage(
        WizardEvent wizardEvent, boolean isNextStep) {
        // Error Checking
        AddLibraryACSLSParamView pageView =
            (AddLibraryACSLSParamView) wizardEvent.getView();
        String errMsg = null;

        // Run error checking only if user clicks NEXT
        // Simply save any values if user clicks PREV, all the values will
        // eventually be checked when user clicks NEXT
        if (isNextStep) {
            errMsg = pageView.getErrorMsg();
            if (errMsg != null) {
                setErrorMessage(wizardEvent, errMsg);
                return false;
            }
        }
        String libraryName = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.LIBRARY_NAME_VALUE);
        String accessID = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.ACCESS_ID_VALUE);
        String SSIHost = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.SSI_HOST_VALUE);
        String useSecureRPC = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.USE_SECURE_RPC);
        String SSIInetPort = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.SSI_INET_PORT_VALUE);
        String CSIHostPort = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.CSI_HOST_PORT_VALUE);
        String acslsPort = (String) wizardModel.getValue(
            AddLibraryACSLSParamView.ACSLS_PORT_NUMBER);

        libraryName  = (libraryName == null)  ? "" : libraryName;
        accessID = (accessID == null) ? "" : accessID;
        SSIHost  = (SSIHost == null)  ? "" : SSIHost;
        useSecureRPC = (useSecureRPC == null) ? "" : useSecureRPC;
        SSIInetPort  = (SSIInetPort == null)  ? "" : SSIInetPort;
        CSIHostPort  = (CSIHostPort == null)  ? "" : CSIHostPort;

        Library myLibrary = null;

        try {
            int stkLibraryIndex = ((Integer) wizardModel.getValue(
                AddLibraryImpl.SA_STK_LIBRARY_INDEX)).intValue();
            myLibrary = getStkLibrary(stkLibraryIndex);
            if (myLibrary == null) {
                throw new SamFSException(null, -2502);
            }
            myLibrary.setName(libraryName);

            String catalogFile  =
                Constants.Wizard.DEFAULT_CATALOG_LOCATION.concat(libraryName);
            if (catalogFile.length() != 0) {
                myLibrary.setCatalogLocation(catalogFile);
            }

            String paramLocation =
                new NonSyncStringBuffer(
                    Constants.Media.DEFAULT_PARAM_LOCATION).
                    append(libraryName).toString();

            // Presentation layer has to set device path as the path is no
            // automatically returned when media discovery occurs.
            myLibrary.setDevicePath(paramLocation);
            StkNetLibParam param = myLibrary.getStkNetLibParam();
            if (param == null) {
                param =
                    new StkNetLibParam(
                        paramLocation,
                        SSIHost,
                        Integer.parseInt(acslsPort));
            } else {
                param.setPath(paramLocation);
            }

            if (accessID.length() != 0) {
                param.setAccess(accessID);
            } else {
                // reset
                param.setAccess(null);
            }

            if (SSIHost.length() != 0) {
                param.setSamServerName(SSIHost);
            } else {
                param.setSamServerName("");
            }

            if (SSIInetPort.length() != 0) {
                param.setSamRecvPort(Integer.parseInt(SSIInetPort));
            } else {
                param.setSamRecvPort(-1);
            }

            if (CSIHostPort.length() != 0) {
                param.setSamSendPort(Integer.parseInt(CSIHostPort));
            } else {
                param.setSamSendPort(-1);
            }

            myLibrary.setStkNetLibParam(param);

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "processACSLSParamame()",
                "Failed to save ACSLS Parameters to the hashMap",
                getServerName());
            setWizardModelErrorMessage(ex.getMessage(), -2509);
            return false;
        }

        // update array
        updateStkLibrariesArray(myLibrary);

        return true;
    }

    /**
     * processNetworkSelectNamePage(wizardEvent)
     * Process the necessary information in Network Select Name page
     * Return boolean (true for success, false for fail)
     */

    private boolean processNetworkSelectNamePage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        AddLibraryNetworkSelectNameView pageView =
            (AddLibraryNetworkSelectNameView) wizardEvent.getView();

        // Validate entries
        String errMsg = pageView.getErrorMsg();
        if (errMsg != null) {
            setErrorMessage(wizardEvent, errMsg);
            return false;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private void updateDriveList() throws SamFSException {
        Library myLibrary    = getMyLibrary();

        // rm drives that have different media type as selected if applicable
        if (hasMixedMedia) {
            Drive [] allDrives = myLibrary.getDrives();
            if (allDrives == null) {
                SamUtil.doPrint("allDrives is NULL");
                throw new SamFSException(null, -2508);
            }

            int driveCount =
                ((Integer) wizardModel.getValue(DRIVE_COUNT)).intValue();
            int mediaType = SamUtil.getMediaType(
                (String) wizardModel.getValue(
                    AddLibrarySummaryView.CHILD_MEDIA_TYPE_FIELD));


            Drive [] myNewDrives = new Drive[driveCount];
            int newDriveIndex = 0;

            for (int i = 0; i < allDrives.length; i++) {
                if (allDrives[i].getEquipType() == mediaType) {
                    myNewDrives[newDriveIndex] = allDrives[i];
                    newDriveIndex++;
                }
            }

            // Update sortedDrives as a part of addLibrary in finishStep
            sortedDrives = myNewDrives;
        }

        // update myLibrary object
        wizardModel.setValue(MY_LIBRARY, myLibrary);
    }

    private String getServerName() {
        return (String) wizardModel.getValue(Constants.Wizard.SERVER_NAME);
    }

    private Library getMyLibrary() {
        return (Library) wizardModel.getValue(MY_LIBRARY);
    }

    /**
     * This method creates the ACSLS Library HashMap
     */
    private void createStkLibrariesArray(
        Library [] discoveredLibraries, String serialNumber, String types)
            throws SamFSException {

        String [] typeArray = types.split(", ");

        // save the types for wizard steps
        wizardModel.setValue(SA_STK_MEDIA_TYPES, typeArray);

        Library [] myStkLibraryArray =
            (Library []) wizardModel.getValue(SA_STK_LIBRARY_ARRAY);

        if (myStkLibraryArray == null) {
            myStkLibraryArray = new Library[typeArray.length];
        }

        int counter = 0;
        for (int i = 0; i < discoveredLibraries.length; i++) {
            if (discoveredLibraries[i].getSerialNo().equals(serialNumber)) {
                myStkLibraryArray[counter++] = discoveredLibraries[i];
                if (counter > typeArray.length) {
                    break;
                }
            }
        }

        // update array
        wizardModel.setValue(SA_STK_LIBRARY_ARRAY, myStkLibraryArray);
    }

    /**
     * Helper methods of STK ArrayList to retrieve the size of the hashMap
     */
    private int getStkLibrariesArraySize() {
        Library [] myStkLibraryArray =
            (Library []) wizardModel.getValue(SA_STK_LIBRARY_ARRAY);
        return myStkLibraryArray.length;
    }

    /**
     * Helper method to retrieve the Library object from the ArrayList by
     * providing the index
     */
    private Library getStkLibrary(int index) {
        Library [] myStkLibraryArray =
            (Library []) wizardModel.getValue(SA_STK_LIBRARY_ARRAY);
        if (index > getStkLibrariesArraySize() - 1) {
            TraceUtil.trace1("Developers Bug found in Impl::getStkLibrary()");
            return null;
        } else {
            return myStkLibraryArray[index];
        }
    }

    /**
     * Helper method to update hashMap after setting StkNetLibParam
     *
     * hashMapKey is the media type in integer, e.g. 110
     */
    private void updateStkLibrariesArray(Library myNewStkLibrary) {
        int index = ((Integer) wizardModel.getValue(
            SA_STK_LIBRARY_INDEX)).intValue();
        Library [] myStkLibraryArray =
            (Library []) wizardModel.getValue(SA_STK_LIBRARY_ARRAY);

        myStkLibraryArray[index] = myNewStkLibrary;

        // update stkArray
        wizardModel.setValue(SA_STK_LIBRARY_ARRAY, myStkLibraryArray);
    }

    /**
     * Helper method to figure out which STK library the code is currently
     * gathering information
     */
    private int getStkLibraryIndex(int page) {
        return page - 2;
    }

    /**
     * Discover Direct Attached Libraries
     */
    private void discoverDirectAttachedLibraries() {
        Library [] allLibrary = null;

        TraceUtil.trace1("Start discovering direct attached libraries..");
        try {
            allLibrary =
                SamUtil.getModel(getServerName()).
                    getSamQFSSystemMediaManager().discoverLibraries();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "AddLibraryDirectSelectLibraryView",
                "Failed to populate available libraries",
                getServerName());
            setWizardModelErrorMessage(samEx.getMessage(), -2510);
        }
        TraceUtil.trace1("Done discovering direct attached libraries..");

        allLibrary = allLibrary == null ? new Library[0] : allLibrary;
        wizardModel.setValue(SA_DISCOVERD_DA_LIBRARY_ARRAY, allLibrary);
    }

    /**
     * Discover STK ACSLS Libraries
     */
    private void discoverSTKLibraries() {
        Library [] myDiscoveredLibraries = null;
        String acslsHostName =
           (String) wizardModel.getValue(
               AddLibrarySelectTypeView.ACSLS_HOST_NAME);
        String acslsPortNumber =
           (String) wizardModel.getValue(
               AddLibrarySelectTypeView.ACSLS_PORT_NUMBER);
        TraceUtil.trace3("Host: " + acslsHostName +
                         ", Port: " + acslsPortNumber);
        StkClntConn [] conns = new StkClntConn[1];
        conns[0] = new StkClntConn(acslsHostName, acslsPortNumber);

        TraceUtil.trace2("Start discovering STK Libraries ....");
        try {
           myDiscoveredLibraries =
                SamUtil.getModel(getServerName()).
                    getSamQFSSystemMediaManager().discoverSTKLibraries(conns);
        } catch (SamFSException samEx) {
            TraceUtil.trace2("Failed to discover STK Libraries.  Reason: " +
                samEx.getMessage());
            SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "AddLibraryDirectSelectLibraryView",
                    "Failed to populate available libraries",
                    getServerName());
            setWizardModelErrorMessage(samEx.getMessage(), -2510);
        }
        TraceUtil.trace2("Done discovering STK Libraries.");

        myDiscoveredLibraries = myDiscoveredLibraries == null ?
            new Library[0] : myDiscoveredLibraries;
        TraceUtil.trace3(
            "Number of libraries found: " + myDiscoveredLibraries.length);

        // Save all discovered library in session
        wizardModel.setValue(
            SA_DISCOVERD_STK_LIBRARY_ARRAY,
            myDiscoveredLibraries);
    }
}
