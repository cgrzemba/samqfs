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

// ident	$Id: NewPolicyWizardImpl.java,v 1.36 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.WizardResultView;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;
import java.util.HashMap;

interface NewPolicyWizardImplData {

    final String name  = "NewPolicyWizardImpl";
    final String title = "NewArchivePolWizard.title";

    final int PAGE_SELECT_TYPE = 0;
    final int PAGE_FILE_MATCH_CRITERIA = 1;
    final int PAGE_COPY_PARAMETERS = 2;
    final int PAGE_TAPE_COPY_OPTION = 3;
    final int PAGE_DISK_COPY_OPTION = 4;
    final int PAGE_SUMMARY = 5;
    final int PAGE_RESULT = 6;

    final int[] NoArchivePages = {
        PAGE_SELECT_TYPE,
        PAGE_SUMMARY,
        PAGE_RESULT
    };

    final Class[] pageClass = {
        NewPolicyWizardSelectTypeView.class,
        NewPolicyWizardFileMatchCriteriaView.class,
        CopyMediaParametersView.class,
        NewPolicyWizardTapeCopyOptionView.class,
        NewPolicyWizardDiskCopyOptionView.class,
        NewPolicyWizardSummaryView.class,
        WizardResultView.class
    };

    final String[] pageTitle = {
        "NewPolicyWizard.defineType.title",
        "NewPolicyWizard.defineName.title",
        "NewPolicyWizard.copymediaparameter.title",
        "NewPolicyWizard.tapecopyoption.title",
        "NewPolicyWizard.diskcopyoption.title",
        "NewArchivePolWizard.summary.title",
        "wizard.result.steptext"
    };
    final String[][] stepHelp = {
        {"NewPolicyWizard.definetype.help1",
            "NewPolicyWizard.definetype.help2",
            "NewPolicyWizard.definetype.help3",
            "NewPolicyWizard.definetype.help4",
            "NewPolicyWizard.definetype.help5",
            "NewPolicyWizard.definetype.help6",
            "NewPolicyWizard.definetype.help7",
            "NewPolicyWizard.definetype.help8",
            "NewPolicyWizard.definetype.help9"},
        {"NewPolicyWizard.definename.help1",
            "NewPolicyWizard.definename.help2",
            "NewPolicyWizard.definename.help3"},
        {"NewPolicyWizard.copymediaparam.help1",
            "NewPolicyWizard.copymediaparam.help2",
            "NewPolicyWizard.copymediaparam.help3",
            "NewPolicyWizard.copymediaparam.help4",
            "NewPolicyWizard.copymediaparam.help5",
            "NewPolicyWizard.copymediaparam.help.reserve"},
        {"NewPolicyWizard.copyoption.help.tapeoptions.help1",
            "NewPolicyWizard.copyoption.help.tapeoptions.help2",
            "NewPolicyWizard.copyoption.help.tapeoptions.help3",
            "NewPolicyWizard.copyoption.help.tapeoptions.help4",
            "NewPolicyWizard.copyoption.help.tapeoptions.help5",
            "NewPolicyWizard.copyoption.help.tapeoptions.help6",
            "NewPolicyWizard.copyoption.help.tapeoptions.help7",
            "NewPolicyWizard.copyoption.help.tapeoptions.help8"},
        {"NewPolicyWizard.copyoption.help.diskoptions.help1",
            "NewPolicyWizard.copyoption.help.diskoptions.help2",
            "NewPolicyWizard.copyoption.help.diskoptions.help3",
            "NewPolicyWizard.copyoption.help.diskoptions.help4",
            "NewPolicyWizard.copyoption.help.diskoptions.help5",
            "NewPolicyWizard.copyoption.help.diskoptions.help6",
            "NewPolicyWizard.copyoption.help.diskoptions.help7",
            "NewPolicyWizard.copyoption.help.diskoptions.help8",
            "NewPolicyWizard.copyoption.help.diskoptions.help9",
            "NewPolicyWizard.copyoption.help.diskoptions.help10",
            "NewPolicyWizard.copyoption.help.diskoptions.help11",
            "NewPolicyWizard.copyoption.help.diskoptions.help12"},
        {"NewArchivePolWizard.summary.help.text1"},
        {"wizard.result.help.text1",
            "wizard.result.help.text2"}
    };

    final String[] stepText = {
        "NewPolicyWizard.defineType.title",
        "NewPolicyWizard.defineName.title",
        "NewPolicyWizard.copymediaparameter.steptext",
        "NewPolicyWizard.tapecopyoption.steptext",
        "NewPolicyWizard.diskcopyoption.steptext",
        "NewArchivePolWizard.summary.steptext",
        "wizard.result.steptext"
    };

    final String[] stepInstruction = {
        "NewPolicyWizard.defineType.instruction",
        "NewPolicyWizard.defineName.instruction",
        "NewArchivePolWizard.page3.instruction",
        "NewArchivePolWizard.page4.instruction",
        "NewArchivePolWizard.page5.instruction",
        "NewArchivePolWizard.summary.instruction",
        "wizard.result.instruction"
    };

    final String[] cancelMsg = {
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
    };
}

public class NewPolicyWizardImpl extends SamWizardImpl {

    public static final String WIZARDPAGEMODELNAME = "NewPolicyPageModelName";
    public static final String WIZARDPAGEMODELNAME_PREFIX = "WizardModel";
    public static final String WIZARDIMPLNAME = NewPolicyWizardImplData.name;
    public static final String WIZARDIMPLNAME_PREFIX = "WizardImpl";
    public static final String WIZARDCLASSNAME =
        "com.sun.netstorage.samqfs.web.archive.wizards.NewPolicyWizardImpl";

    // key (used by first wizard page) to keep track if no_archive already
    // exists
    public static final String NO_ARCHIVE_EXISTS = "NoArchiveExists";

    // two keys (used by wizardModel) to keep track of all information entered
    // in this wizard
    public static final String CRITERIA_PROPERTIES = "CriteriaProperties";
    public static final String COPY_HASHMAP = "CopyHashMap";

    // Static String to keep track of policy type
    public static final String ARCHIVE_TYPE = "ArchiveType";
    public static final String NO_ARCHIVE = "NoArchive";
    public static final String ARCHIVE = "Archive";

    // Static String to keep track of copy type
    public static final String DISK_BASE = "DiskBase";
    public static final String TAPE_BASE = "TapeBase";

    // Keep track of the total number of copies that user has to input
    private static String TOTAL_COPIES = "total_copies";

    // HashTable which is used to keep track of the copy number with its type
    private HashMap copyNumberHashMap = new HashMap();

    // Keep track of the list of selected file systems
    // (used by summary view as well)
    public static String SELECTED_FS_LIST = "selectedFileSystems";

    // Keep track of the label name and change the label when error occurs
    public static final String VALIDATION_ERROR = "ValidationError";

    private boolean wizardInitialized = false;

    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        return new NewPolicyWizardImpl(requestContext);
    }

    public NewPolicyWizardImpl(RequestContext requestContext) {
        super(requestContext, WIZARDPAGEMODELNAME);

        // retrieve and save the server name
        String serverName = requestContext.getRequest().getParameter(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        TraceUtil.trace3(new NonSyncStringBuffer(
            "WizardImpl Const' serverName is ").append(serverName).toString());
        wizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            serverName);

        // set NO_ARCHIVE_EXISTS to true if no_archive already exists
        wizardModel.setValue(
            NO_ARCHIVE_EXISTS,
            Boolean.toString(
                PolicyUtil.policyExists(
                    serverName,
                    Constants.Archive.NOARCHIVE_POLICY_NAME)));

        // retrieve and save fs name if launched from FS Summary page
        String selectedFSName = requestContext.getRequest().getParameter(
            Constants.Wizard.FS_NAME_PARAM);
        wizardModel.setValue(
            SELECTED_FS_LIST,
            selectedFSName == null ? "" : selectedFSName);

        initializeWizard();
        initializeWizardControl(requestContext);

        // retrieve the server version and save it
        try {
            String version = SamUtil.getAPIVersion(serverName);
            wizardModel.setValue(Constants.Wizard.SERVER_VERSION, version);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "NewPolicyWizardImpl",
                                     "unable to retrieve server version",
                                     serverName);
        }
    }

    // initialize wizard
    private void initializeWizard() {
        TraceUtil.trace3("Entering");

        wizardName = NewPolicyWizardImplData.name;
        wizardTitle = NewPolicyWizardImplData.title;
        pageClass = NewPolicyWizardImplData.pageClass;
        pageTitle = NewPolicyWizardImplData.pageTitle;
        stepHelp = NewPolicyWizardImplData.stepHelp;
        stepText = NewPolicyWizardImplData.stepText;
        stepInstruction = NewPolicyWizardImplData.stepInstruction;
        cancelMsg = NewPolicyWizardImplData.cancelMsg;

        // initialize HashMap in wizardModel to be blank
        wizardModel.setValue(COPY_HASHMAP, copyNumberHashMap);

        // initialize number of copy that is being filled as 0
        wizardModel.setValue(TOTAL_COPIES, new Integer(0));

        // initialize selected file system list to be empty if no param is
        // set from FS Summary Page
        String selectedFS = (String) wizardModel.getValue(SELECTED_FS_LIST);
        if (selectedFS == null || selectedFS.length() == 0) {
            wizardModel.setValue(SELECTED_FS_LIST, "");
        }

        // turn show results page to true
        setShowResultsPage(true);

        // reset page logic based on preferences
        pages = NewPolicyWizardImplData.NoArchivePages;
        initializeWizardPages(pages);

        // create initial criteria properties object here,
        // we will wait until processSelectType to create the Copy Wrapper Array
        try {
            // get the default policy criteria
            ArchivePolCriteriaProp criteriaProperties =
                PolicyUtil.getArchiveManager(getServerName()).
                    getDefaultArchivePolCriteriaProperties();

            // save
            wizardModel.setValue(CRITERIA_PROPERTIES, criteriaProperties);
            // wizardModel.setValue(COPY_GUI_WRAPPER_ARRAY, null);

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "NewPolicyWizardImpl()",
                "Failed to get preferences status",
                getServerName());

            setWizardModelErrorMessage(ex.getMessage(), ex.getSAMerrno());
        }
        TraceUtil.trace3("Exiting");
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return
            getWizardWindowModel(
                WIZARDIMPLNAME,
                NewPolicyWizardImplData.title,
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
        int page = pageIdToPage(currentPageId) + 1;

        int totalCopyNumber =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue();

        String [] futurePages = null;
        if (pages[page - 1] == NewPolicyWizardImplData.PAGE_SELECT_TYPE) {
            futurePages = new String[0];
        } else if (pages[page - 1] ==
            NewPolicyWizardImplData.PAGE_FILE_MATCH_CRITERIA) {
            futurePages = new String[1];
            futurePages[0] = Integer.toString(page);
        } else if (isPageCopyParameters(page - 1)) {
            futurePages = new String[0];
        } else if (isPageDiskOrTapeCopyOption(page - 1)) {
            // For these two steps, show "Copy Media Parameters" step if
            // there exists the next cycle
            // Otherwise, use the last else case
            futurePages = new String[1];
            futurePages[0] = Integer.toString(page);
        } else {
            int howMany = pages.length - page;
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

        int totalCopyNumber =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue();

        if (pages[page] == NewPolicyWizardImplData.PAGE_SELECT_TYPE) {
            // shows nothing, no_archive don't have policy name page in 4.4
            futureSteps = new String[0];
        } else if (
            pages[page] == NewPolicyWizardImplData.PAGE_FILE_MATCH_CRITERIA) {
            // no_archive will not see this step after redesign in 4.4
            // just show "Copy Media Parameters" step
            futureSteps = new String[1];
            String futureStepText = SamUtil.getResourceString(
                stepText[2],
                new String[] {
                    Integer.toString(
                        getCopyNumberForCopyMediaParametersPage(page + 1))});
            futureSteps[0] = futureStepText;

        // Be extra cautious in the next two else case
        // These two cases can be a part of the two-step loop
        // pass page to each of the designated method,
        // return true if "page" is really a Copy Parameters Page
        // return false otherwise

        } else if (isPageCopyParameters(page)) {
            // Always show no future step for this page
            // We need user input to determine what the next page is
            futureSteps = new String[0];
        } else if (isPageDiskOrTapeCopyOption(page)) {
            // For these two steps, show "Copy Media Parameters" step if
            // there exists the next cycle
            // Otherwise, use the last else case
            futureSteps = new String[1];
            String futureStepText = SamUtil.getResourceString(
                stepText[2],
                new String[] {
                    Integer.toString(
                        getCopyNumberForCopyMediaParametersPage(page + 1))});
            futureSteps[0] = futureStepText;
        } else {
            int howMany = pages.length - page - 1;

            futureSteps = new String[howMany];

            for (int i = 0; i < howMany; i++) {
                int futureStep = page + 1 + i;
                int futurePage = pages[futureStep];
                futureSteps[i] = stepText[futurePage];
            }
        }

        TraceUtil.trace3("Exiting");
        return futureSteps;
    }

    public String getStepInstruction(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        if (pages[pageIdToPage(pageId)] ==
            NewPolicyWizardImplData.PAGE_SELECT_TYPE && noArchiveExists()) {
            return "NewPolicyWizard.defineType.instruction.noarchive";
        } else {
            return stepInstruction[pages[pageIdToPage(pageId)]];
        }
    }

    // overwrite getPageClass() to clear the WIZARD_ERROR value when
    // the wizard is initialized
    public Class getPageClass(String pageId) {
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);

        // Set COPY_NUMBER to the corresponding copy number so the View
        // knows which copy information needs to be pulled from the
        // wrapper array to pre-populate fields
        if (pages[page] == NewPolicyWizardImplData.PAGE_COPY_PARAMETERS) {
            wizardModel.setValue(
                Constants.Wizard.COPY_NUMBER,
                new Integer(
                    getCopyNumberForCopyMediaParametersPage(page)));
            TraceUtil.trace2("This is Copy # " +
                getCopyNumberForCopyMediaParametersPage(page));
        } else if (
            pages[page] == NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION ||
            pages[page] == NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION) {
            wizardModel.setValue(
                Constants.Wizard.COPY_NUMBER,
                new Integer(
                    getCopyNumberForCopyOptionPage(page)));
            TraceUtil.trace2("This is Copy # " +
                getCopyNumberForCopyOptionPage(page));
        }

        // clear out previous errors if wizard has been initialized
        if (wizardInitialized) {
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        }

        return super.getPageClass(pageId);
    }

    public String getStepText(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);
        String text = null;

        if (pages[page] == NewPolicyWizardImplData.PAGE_COPY_PARAMETERS) {
            text = SamUtil.getResourceString(
                stepText[pages[page]],
                new String[] {
                    Integer.toString(
                        getCopyNumberForCopyMediaParametersPage(page))});
        } else if (
            pages[page] == NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION ||
            pages[page] == NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION) {
            text = SamUtil.getResourceString(
                stepText[pages[page]],
                new String[] {
                    Integer.toString(
                        getCopyNumberForCopyOptionPage(page))});
        } else {
            if (pages[page] == NewPolicyWizardImplData.PAGE_SELECT_TYPE
                && noArchiveExists()) {
                text = "NewPolicyWizard.defineType.title.noarchive";
            } else {
                text = stepText[pages[page]];
            }
        }

        return text;
    }

    private int getCopyNumberForCopyMediaParametersPage(int page) {
        int totalCopies =
            ((Integer)wizardModel.getValue(TOTAL_COPIES)).intValue();

        // Check if the "page" is actually a Copy Parameters Page
        if (page < totalCopies * 2 + 1) {
            for (int i = 0; i < totalCopies; i++) {
                if (page ==
                    NewPolicyWizardImplData.PAGE_COPY_PARAMETERS + i * 2) {
                    return (i + 1);
                }
            }
        }
        // should not come to here
        return -1;
    }

    private int getCopyNumberForCopyOptionPage(int page) {
        int totalCopies =
            ((Integer)wizardModel.getValue(TOTAL_COPIES)).intValue();

        // Check if the "page" is actually a Tape/Disk Copy Option Page
        if (page < totalCopies * 2 + 2) {
            for (int i = 0; i < totalCopies; i++) {
                if (page ==
                    NewPolicyWizardImplData.PAGE_COPY_PARAMETERS + 1 + i * 2) {
                    return (i + 1);
                }
            }
        }
        // should not come to here
        return -1;
    }


    public String getStepTitle(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        int page = pageIdToPage(pageId);
        String title = null;

        if (pages[page] == NewPolicyWizardImplData.PAGE_COPY_PARAMETERS) {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    Integer.toString(
                        getCopyNumberForCopyMediaParametersPage(page))});
        } else if (
            pages[page] == NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION ||
            pages[page] == NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION) {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    Integer.toString(
                        getCopyNumberForCopyOptionPage(page))});
        } else {
            if (pages[page] == NewPolicyWizardImplData.PAGE_SELECT_TYPE
                && noArchiveExists()) {
                title = "NewPolicyWizard.defineType.title.noarchive";
            } else {
                title = pageTitle[pages[page]];
            }
        }

        return title;
    }

    public String [] getStepHelp(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        if (pages[pageIdToPage(pageId)] == 0 && noArchiveExists()) {
            String [] helpNoRadio =
                new String [] {
                    "NewPolicyWizard.definetype.help2",
                    "NewPolicyWizard.definetype.help3",
                    "NewPolicyWizard.definetype.help4",
                    "NewPolicyWizard.definetype.help5",
                    "NewPolicyWizard.definetype.help6",
                    "NewPolicyWizard.definetype.help7",
                    "NewPolicyWizard.definetype.help8"};
            return helpNoRadio;
        } else {
            return stepHelp[pages[pageIdToPage(pageId)]];
        }
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
            case NewPolicyWizardImplData.PAGE_SELECT_TYPE:
                result = processSelectTypePage(wizardEvent);
                break;

            case NewPolicyWizardImplData.PAGE_FILE_MATCH_CRITERIA:
                result = processFileMatchCriteriaPage(wizardEvent);
                break;

            case NewPolicyWizardImplData.PAGE_COPY_PARAMETERS:
                result = processCopyParametersPage(wizardEvent);
                break;

            case NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION:
                result = processTapeCopyOptionPage(wizardEvent);
                break;

            case NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION:
                result = processDiskCopyOptionPage(wizardEvent);
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
            case NewPolicyWizardImplData.PAGE_COPY_PARAMETERS:
                result = processCopyParametersPage(wizardEvent);
                break;

            case NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION:
                result = processTapeCopyOptionPage(wizardEvent);
                break;

            case NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION:
                result = processDiskCopyOptionPage(wizardEvent);
                break;
        }

        // Clear validation error.  User is returning to the previous page so
        // do not show faulty label here.
        wizardModel.setValue(VALIDATION_ERROR, "");

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean finishStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        boolean returnValue = true, warningException = false;
        String errMsg = null, errCode = null, warningSummary = null;

        // make sure this wizard is still active before commit
        if (super.finishStep(wizardEvent) == false) {
            return true;
        }

        String policyName = (String) wizardModel.getValue(
            NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXTFIELD);
        int totalCopies =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue();

        Object [] fsList = (Object []) wizardModel.getValues(
            NewPolicyWizardSelectTypeView.APPLY_TO_FS);
        String[] selectedFSArray = new String[fsList.length];
        for (int i = 0; i < selectedFSArray.length; i++) {
            selectedFSArray[i] = (String) fsList[i];
        }

        // create the appropriate size of wrapperArray
        // size 0 for no_archive
        ArchiveCopyGUIWrapper[] wrapperArray =
            new ArchiveCopyGUIWrapper[totalCopies];

        // Grab the CriteriaProperty to get ready to pass to the create API
        ArchivePolCriteriaProp criteriaProperties = getCriteriaProperties();

        try {
            // Assign wrapperArray with the right wrapper object
            for (int i = 0; i < totalCopies; i++) {
                wrapperArray[i] = getCopyGUIWrapper(i + 1);
            }

            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            SamQFSSystemArchiveManager archiveManager =
                sysModel.getSamQFSSystemArchiveManager();
            if (archiveManager == null) {
                throw new SamFSException(null, -2001);
            }

            LogUtil.info(
                this.getClass(),
                "finishStep",
                new NonSyncStringBuffer("Start creating new archive policy ").
                    append(policyName).toString());

            // if we get this far, update set the map save bit
            for (int i = 0; i < wrapperArray.length; i++) {
                wrapperArray[i].getArchiveCopy().
                    getArchiveVSNMap().setWillBeSaved(true);
            }

            // create the policy
            archiveManager.createArchivePolicy(
                policyName,
                criteriaProperties,
                wrapperArray,
                selectedFSArray);

            LogUtil.info(
                this.getClass(),
                "finishStep",
                new NonSyncStringBuffer("Done creating new archive policy ").
                    append(policyName).toString());

        } catch (SamFSWarnings sfw) {
            returnValue = false;
            warningException = true;
            warningSummary = "ArchiveConfig.error";
            errMsg = "ArchiveConfig.warning.detail";
        } catch (SamFSMultiMsgException sfme) {
            returnValue = false;
            warningException = true;
            warningSummary = "ArchiveConfig.error";
            errMsg = "ArchiveConfig.warning.detail";
        } catch (SamFSException samEx) {
            returnValue = false;
            TraceUtil.trace1(new NonSyncStringBuffer(
                "Exception caught in finishStep: Reason:").append(
                samEx.getMessage()).toString());
            SamUtil.processException(
                samEx,
                this.getClass(),
                "finishStep",
                "Failed to create new copy",
                getServerName());
            errMsg = samEx.getMessage();
            errCode = Integer.toString(samEx.getSAMerrno());
        }


        // Set variables for the result page
        if (returnValue) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Successfully creating new archive policy ").
                    append(policyName).toString());
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "ArchivePolSummary.msg.create",
                    policyName));
        } else {
            // WARNING
            if (warningException) {
                SamUtil.doPrint(new NonSyncStringBuffer(
                    "Successfully creating new archive policy ").
                    append(policyName).append(", but warning is caught.").
                    toString());
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_WARNING);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    warningSummary);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    errMsg);
            } else {
                // ERROR
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchivePolSummary.error.failedCreatePolicy");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    errMsg);
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE,
                    errCode);
            }
        }

        TraceUtil.trace3("Exiting");
        return true;
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
        wizardModel.setValue(
            Constants.Wizard.WIZARD_ERROR,
            Constants.Wizard.WIZARD_ERROR_YES);
        wizardModel.setValue(
            Constants.Wizard.ERROR_MESSAGE,
            message);
        wizardModel.setValue(
            Constants.Wizard.ERROR_CODE,
            Integer.toString(errorCode));
    }
    /**
     * Process the necessary information in Select Type page
     * @return boolean (true for success, false for fail)
     */

    boolean processSelectTypePage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        // Start validating fields, return false if error is detected
        // Check if archive type is selected
        String archiveMenu = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.ARCHIVE_MENU);
        if (archiveMenu == null) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_LABEL,
                "NewArchivePolWizard.page1.errMsg1",
                true);
            return false;
        }

        // If archive type is No archiving, show error if no_archive exists
        // already. (Default Policy)
        if (archiveMenu.equals(NewPolicyWizardImpl.NO_ARCHIVE)) {
            // Re-initialize pages again, just in case user tries to go to
            // to the archiving route, click the step link 1 and back to this
            // page
            pages = NewPolicyWizardImplData.NoArchivePages;

            // set archive type
            wizardModel.setValue(ARCHIVE_TYPE, NO_ARCHIVE);

            // Set total copies back to 0
            wizardModel.setValue(TOTAL_COPIES, new Integer(0));

            // Set Summary
            wizardModel.setValue(
                NewPolicyWizardSummaryView.CHILD_POL_NAME_TEXTFIELD,
                Constants.Archive.DEFAULT_POLICY_NAME);
            wizardModel.setValue(
                NewPolicyWizardSummaryView.CHILD_NUM_COPIES_TEXTFIELD,
                "0");
        } else {
            // Reset policy name to empty in case user chooses NO_ARCHIVE
            // can come back to step one and select ARCHIVE
            String archiveType = (String) wizardModel.getValue(ARCHIVE_TYPE);
            archiveType = archiveType == null ? "" : archiveType;
            if (archiveType.equals(NO_ARCHIVE)) {
                wizardModel.setValue(
                NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXTFIELD,
                    "");
            }

            // set archive type
            wizardModel.setValue(ARCHIVE_TYPE, ARCHIVE);

            // update wizard step
            pages = new int [] {
                NewPolicyWizardImplData.PAGE_SELECT_TYPE,
                NewPolicyWizardImplData.PAGE_FILE_MATCH_CRITERIA,
                NewPolicyWizardImplData.PAGE_COPY_PARAMETERS,
                NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION,
                NewPolicyWizardImplData.PAGE_SUMMARY,
                NewPolicyWizardImplData.PAGE_RESULT
            };
        }
        initializeWizardPages(pages);


        // Call API to store information
        ArchivePolCriteriaProp criteriaProperties = getCriteriaProperties();

        // Check if Starting Directory is filled in (REQUIRED FIELD)
        String startingDirectory = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_START_DIR_TEXTFIELD);

        startingDirectory =
            startingDirectory != null ? startingDirectory.trim() : "";
        // Error if starting directory is empty
        if (startingDirectory.equals("")) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_START_DIR_TEXT,
                "NewArchivePolWizard.page1.errMsg3",
                true);
            return false;
        }

        // Error if the starting directory contains a space in it
        if (startingDirectory.indexOf(" ") != -1) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_START_DIR_TEXT,
                "NewArchivePolWizard.page1.errMsg7",
                true);
            return false;
        }

        // Error if the starting directory is a relative path name
        if (startingDirectory.startsWith("/")) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_START_DIR_TEXT,
                "NewArchivePolWizard.page1.errMsg10",
                true);
            return false;
        }

        // save starting directory (required key)
        criteriaProperties.setStartingDir(startingDirectory);

        // The following fields are not mandatory
        // Validate entries if user input something in the field
        // Otherwise, no need to validate anuything

        String minSize = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_TEXTFIELD);

        String minSizeDropDown = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_DROPDOWN);

        String maxSize = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_MAX_SIZE_TEXTFIELD);

        String maxSizeDropDown = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_MAX_SIZE_DROPDOWN);

        String namePattern = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_NAME_PATTERN_TEXTFIELD);

        String owner = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_OWNER_TEXTFIELD);

        String group = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_GROUP_TEXTFIELD);

        String accessAge = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_ACCESS_AGE_TEXTFIELD);

        String accessAgeUnit = (String) wizardModel.getValue(
            NewPolicyWizardSelectTypeView.CHILD_ACCESS_AGE_DROPDOWN);

        Object [] fsList = (Object []) wizardModel.getValues(
            NewPolicyWizardSelectTypeView.APPLY_TO_FS);

        // Better deal with empty string than null strings, trim them too
        minSize = minSize != null ? minSize.trim() : "";
        minSizeDropDown = minSizeDropDown != null ? minSizeDropDown.trim() : "";
        maxSize = maxSize != null ? maxSize.trim() : "";
        maxSizeDropDown = maxSizeDropDown != null ? maxSizeDropDown.trim() : "";
        accessAge = accessAge != null ? accessAge.trim() : "";
        accessAgeUnit = accessAgeUnit != null ?
            accessAgeUnit.trim() : "";
        namePattern = namePattern != null ? namePattern.trim() : "";
        owner = owner != null ? owner.trim() : "";
        group = group != null ? group.trim() : "";

        // First check if min/max contains a value but not choosing a unit,
        // or the other way around
        if (!minSize.equals("") &&
            minSizeDropDown.equals(SelectableGroupHelper.NOVAL)) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_TEXT,
                "NewArchivePolWizard.page2.errMsgMinSizeDropDown",
                true);
            return false;
        } else if (!maxSize.equals("") &&
            maxSizeDropDown.equals(SelectableGroupHelper.NOVAL)) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_MAX_SIZE_TEXT,
                "NewArchivePolWizard.page2.errMsgMaxSizeDropDown",
                true);
            return false;
        }

        // Now check if minSize and maxSize are integers (long),
        // and is greater than 0
        long minimumSize = -1, maximumSize = -1;
        try {
            if (!minSize.equals("")) {
                minimumSize = Long.parseLong(minSize);
                if (minimumSize < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_TEXT,
                        "NewArchivePolWizard.page2.errMsgMinSizeNegInt",
                        true);
                    return false;
                }
            }
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("Minimum Size is not an integer");
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_TEXT,
                "NewArchivePolWizard.page2.errMsgMinSize",
                true);
            return false;
        }
        try {
            if (!maxSize.equals("")) {
                maximumSize = Long.parseLong(maxSize);
                if (maximumSize < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardSelectTypeView.CHILD_MAX_SIZE_TEXT,
                        "NewArchivePolWizard.page2.errMsgMaxSizeNegInt",
                        true);
                    return false;
                }
            }
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("Maximum Size is not an integer");
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_MAX_SIZE_TEXT,
                "NewArchivePolWizard.page2.errMsgMaxSize",
                true);
            return false;
        }

        // Now check if maxSize is greater than minSize
        if (!minSize.equals("") &&
            !maxSize.equals("") &&
            !PolicyUtil.isMaxGreaterThanMin(
                minimumSize, Integer.parseInt(minSizeDropDown),
                maximumSize, Integer.parseInt(maxSizeDropDown))) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_TEXT,
                "NewArchivePolWizard.page2.errMsgMinMaxSize",
                true);
            return false;
        } else {
            // check that mininum file size is between 0 and 7813PB
            if (PolicyUtil.isOverFlow(minimumSize,
                                      Integer.parseInt(minSizeDropDown))) {
                setJavascriptErrorMessage(wizardEvent,
                        NewPolicyWizardSelectTypeView.CHILD_MIN_SIZE_TEXT,
                                  "CriteriaDetails.error.overflowminsize",
                                          true);
                return false;
            }

            // check that maximum file size is between 0 and 7813PB
            if (PolicyUtil.isOverFlow(maximumSize,
                                      Integer.parseInt(maxSizeDropDown))) {
                setJavascriptErrorMessage(wizardEvent,
                        NewPolicyWizardSelectTypeView.CHILD_MAX_SIZE_TEXT,
                                  "CriteriaDetails.error.overflowmaxsize",
                                          true);
                return false;
            }

            try {
                // minimum / maximum size
                criteriaProperties.setMinSize(minimumSize);
                criteriaProperties.setMinSizeUnit(
                    Integer.parseInt(minSizeDropDown));
                criteriaProperties.setMaxSize(maximumSize);
                criteriaProperties.setMaxSizeUnit(
                    Integer.parseInt(maxSizeDropDown));
            } catch (NumberFormatException numEx) {
                // This should not happen, try/catch just in case.
                TraceUtil.trace1(
                    "Internal fatal error. Number exception caught!");
                setWizardModelErrorMessage(numEx.getMessage(), 8001234);
                // return true to proceed to next page and show error
                return true;
            }
        }

        // Setting summary & storing information
        try {
            if (!minSize.equals("")) {
                wizardModel.setValue(
                    NewPolicyWizardSummaryView.CHILD_MINIMUM_SIZE_TEXTFIELD,
                    new NonSyncStringBuffer(minSize).append(" ").append(
                        SamUtil.getSizeUnitL10NString(
                            Integer.parseInt(minSizeDropDown))));
            }

            if (!maxSize.equals("")) {
                wizardModel.setValue(
                    NewPolicyWizardSummaryView.CHILD_MAXIMUM_SIZE_TEXTFIELD,
                    new NonSyncStringBuffer(maxSize).append(" ").append(
                        SamUtil.getSizeUnitL10NString(
                            Integer.parseInt(maxSizeDropDown))));
            }
        } catch (NumberFormatException numEx) {
            // This should not happen, try/catch just in case.
            TraceUtil.trace1("Internal fatal error. Number exception caught!");
            // Do not throw exception, simply do not set the summary
        }


        // Check if namePattern is valid, no space in between as well
        if (namePattern.indexOf(" ") != -1 ||
            !PolicyUtil.isValidNamePattern(namePattern)) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_NAME_PATTERN_TEXT,
                "NewArchivePolWizard.page2.errNamePattern",
                true);
            return false;
        } else {
            // Store information
            criteriaProperties.setNamePattern(namePattern);
        }

        // Check if owner has no space in between
        if (owner.indexOf(" ") != -1 ||
            (!PolicyUtil.isUserValid(owner, getServerName()) &&
                !owner.equals(""))) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_OWNER_TEXT,
                "NewArchivePolWizard.page2.errOwner",
                true);
            return false;
        } else {
            // Store information
            criteriaProperties.setOwner(owner);
        }

        // Check if group exists has no space in between
        // no try/catch block here because PolicyUtil.isGroupValid eats
        // the exception if there is one
        // If exception is caught, PolicyUtil.isGroupValid will return false
        if (group.indexOf(" ") != -1) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_GROUP_TEXT,
                "NewArchivePolWizard.page2.errGroup",
                true);
            return false;
        } else if (!PolicyUtil.isGroupValid(group, getServerName()) &&
            !group.equals("")) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.CHILD_GROUP_TEXT,
                "NewArchivePolWizard.page2.errGroupExist",
                true);
            return false;
        } else {
            // Store information
            criteriaProperties.setGroup(group);
        }

        if (!accessAge.equals("")) {
            long accessAgeValue = -1;

            // if access Age is defined, a unit must be defined as well
            if (accessAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardSelectTypeView.CHILD_ACCESS_AGE_TEXT,
                    "NewCriteriaWizard.matchCriteriaPage.error.accessAgeUnit",
                    true);
                return false;
            }

            try {
                // make sure accessAge is a positive integer
                accessAgeValue = Long.parseLong(accessAge);
                if (accessAgeValue <= 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardSelectTypeView.CHILD_ACCESS_AGE_TEXT,
                        "NewCriteriaWizard.matchCriteriaPage.error.accessAge",
                        true);
                    return false;
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardSelectTypeView.CHILD_ACCESS_AGE_TEXT,
                    "NewCriteriaWizard.matchCriteriaPage.error.accessAge",
                    true);
                return false;
            }

            criteriaProperties.setAccessAge(accessAgeValue);
            criteriaProperties.setAccessAgeUnit(
                Integer.parseInt(accessAgeUnit));

            wizardModel.setValue(
                NewPolicyWizardSummaryView.CHILD_ACCESS_AGE_TEXTFIELD,
                new NonSyncStringBuffer().
                    append(accessAge).append(" ").
                    append(SamUtil.getTimeUnitL10NString(
                        Integer.parseInt(accessAgeUnit))).toString());
        } else {
            // reset access age to -1
            criteriaProperties.setAccessAge(-1);
            criteriaProperties.setAccessAgeUnit(-1);

            wizardModel.setValue(
                NewPolicyWizardSummaryView.CHILD_ACCESS_AGE_TEXTFIELD, "");
        }

        //  Apply to FS
        if (fsList.length == 0) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardSelectTypeView.APPLY_TO_FS_TEXT,
                "Reservation.page2.errMsg",
                true);
            return false;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * Process the necessary information in File Match Criteria page
     * @return boolean (true for success, false for fail)
     */

    boolean processFileMatchCriteriaPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        // Grab the criteriaProperty to store information
        ArchivePolCriteriaProp criteriaProperty = getCriteriaProperties();

        // Setting summary & storing information
        String archiveType = (String) wizardModel.getValue(ARCHIVE_TYPE);
        if (archiveType.equals(ARCHIVE)) {

            // Validate Policy Name
            String policyName = (String) wizardModel.getValue(
                NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXTFIELD);
            policyName = policyName != null ? policyName.trim() : "";

            if (policyName.equals("")) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXT,
                    "NewArchivePolWizard.page1.errMsg14",
                    true);
                return false;
            }
            if (policyName.indexOf(" ") != -1 ||
                !Character.isLetter(policyName.charAt(0)) ||
                !SamUtil.isValidLetterOrDigitString(policyName)) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXT,
                    "NewArchivePolWizard.page1.errMsg8",
                    true);
                return false;
            }
            if (policyName.endsWith("*")) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXT,
                    "NewArchivePolWizard.page1.errMsg13",
                    true);
                return false;
            }

            // if a general policy is named 'no_archive' flag error
            if (policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME)) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXT,
                    "NewArchivePolWizard.page1.errMsg15",
                    true);
                return false;
            }
            try {
                if (isPolicyNameInUsed(policyName)) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                       NewPolicyWizardFileMatchCriteriaView.CHILD_POL_NAME_TEXT,
                        "NewArchivePolWizard.page1.errMsg11",
                        true);
                    return false;
                }
            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "processSelectTypePage",
                    "Failed to retrieve existing policy information",
                    getServerName());
                setWizardModelErrorMessage(
                    samEx.getMessage(), samEx.getSAMerrno());
                return true;
            }

            // Save number of copies user want to create for this policy
            String numOfCopies = (String) wizardModel.getValue(
                NewPolicyWizardFileMatchCriteriaView.CHILD_COPIES_DROPDOWN);

            wizardModel.setValue(TOTAL_COPIES, Integer.valueOf(numOfCopies));
            wizardModel.setValue(
                NewPolicyWizardSummaryView.CHILD_NUM_COPIES_TEXTFIELD,
                Integer.valueOf(numOfCopies));

            String stageAttributes = (String) wizardModel.getValue(
                NewPolicyWizardFileMatchCriteriaView.CHILD_STAGE_DROPDOWN);
            String releaseAttributes = (String) wizardModel.getValue(
                NewPolicyWizardFileMatchCriteriaView.CHILD_RELEASE_DROPDOWN);

            try {
                // Releasing Behavior
                if (!releaseAttributes.equals(SelectableGroupHelper.NOVAL)) {
                    wizardModel.setValue(
                        NewPolicyWizardSummaryView.CHILD_RELEASE_TEXTFIELD,
                        PolicyUtil.getReleasingOptionString(
                            Integer.parseInt(releaseAttributes)));
                    // Store information
                    criteriaProperty.setReleaseAttributes(
                        Integer.parseInt(releaseAttributes), -1);
                }

                // Staging Behavior
                if (!stageAttributes.equals(SelectableGroupHelper.NOVAL)) {
                    wizardModel.setValue(
                        NewPolicyWizardSummaryView.CHILD_STAGE_TEXTFIELD,
                        PolicyUtil.getStagingOptionString(
                            Integer.parseInt(stageAttributes)));
                    // Store information
                    criteriaProperty.setStageAttributes(
                        Integer.parseInt(stageAttributes));
                }
            } catch (NumberFormatException numEx) {
                // This should not happen, try/catch just in case.
                TraceUtil.trace1(
                    "Internal fatal error. Number exception caught!");
                // Do not throw exception, simply do not set the summary
            }
        }

        TraceUtil.trace3("Exiting");
        // if we get this far, the whole page checked out
        return true;
    }

    /**
     * Process the necessary information in Copy Parameters page
     * NOTE: The error handling of this method is a little different than
     * how we normally handle.  We cannot return false right away
     * when error is caught.  The Copy Parameters Page is a part of the
     * cycle and it has to be re-used again and again.  We cannot rely
     * on the wizardModel to handle the field values for us.
     * @return boolean (true for success, false for fail)
     */

    boolean processCopyParametersPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        int page = pageIdToPage(wizardEvent.getPageId());
        int currentCopyNumber = getCopyNumberForCopyMediaParametersPage(page);
        String mediaRadio = null;

        // errorMessage holds the error message when an error is found.
        // Again, we do not return false right away because we need to save
        // the values of the rest of the page.  Read function heading for
        // more information.  When we assign the errorMessage, make sure
        // we assign the message only if errorMessage is null.  Say if the user
        // has two errors made in the page, we will still show the FIRST message
        // that the user made, instead of the LAST message.
        String errorMessage = null;

        ArchiveCopyGUIWrapper myWrapper = null;

        // Grab the wrapper object from the hashMap based on the copy number
        try {
            myWrapper = getCopyGUIWrapper(currentCopyNumber);
        } catch (SamFSException samEx) {
            // Keep returning true here, go to the next page and show an error
            TraceUtil.trace1("Exception caught while retrieving copy wrapper");
            setWizardModelErrorMessage(samEx.getMessage(), samEx.getSAMerrno());
            return true;
        }

        // Grab the criteria copy and the archive copy from the wrapper
        ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();
        // ArchiveVSNMap archiveVSNMap = archiveCopy.getArchiveVSNMap();
        ArchivePolCriteriaCopy criteriaCopy =
            myWrapper.getArchivePolCriteriaCopy();

        CopyMediaValidator validator =
            new CopyMediaValidator(wizardModel,
                                   wizardEvent,
                                   myWrapper,
                                   currentCopyNumber);


        boolean result = validator.validate();
        String archiveType =
            validator.getMediaType() == BaseDevice.MTYPE_DISK ?
            DISK_BASE : TAPE_BASE;

        // TODO: need to update copy hashmap here
        updateHashMap(currentCopyNumber, archiveType, myWrapper);

        updateWizardSteps();
        return result;
    }

    /**
     * Process the necessary information in Tape Copy Option page
     * The error handling scheme of this method is the same as
     * processCopyParametersPage().  Please view the comment of that function.
     * @return boolean (true for success, false for fail)
     */

    boolean processTapeCopyOptionPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        int page = pageIdToPage(wizardEvent.getPageId());
        int currentCopyNumber = getCopyNumberForCopyOptionPage(page);

        ArchiveCopyGUIWrapper myWrapper = null;

        // Grab the wrapper object from the hashMap based on the copy number
        try {
            myWrapper = getCopyGUIWrapper(currentCopyNumber);
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception caught while retrieving copy wrapper.");
            setWizardModelErrorMessage(samEx.getMessage(), samEx.getSAMerrno());
            return true;
        }

        ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();

        // offline copy
        String offlineCopyString = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_OFFLINE_COPY_DROPDOWN);

        if (!SelectableGroupHelper.NOVAL.equals(offlineCopyString)) {
            archiveCopy.setOfflineCopyMethod(
                Integer.parseInt(offlineCopyString));
        } else {
            archiveCopy.setOfflineCopyMethod(-1);
        }

        // drives
        String drives = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_TEXTFIELD);
        drives = drives != null ? drives.trim() : "";

        if (!drives.equals("")) {
            try {
                int d = Integer.parseInt(drives);
                if (d < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_TEXT,
                        "NewPolicyWizard.copyoption.error.drives",
                        true);
                    return false;
                } else {
                    archiveCopy.setDrives(d);
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_TEXT,
                    "NewPolicyWizard.copyoption.error.drives",
                    true);
                return false;
            }
        } else {
            archiveCopy.setDrives(-1);
        }

        String drivesMin = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MIN_TEXTFIELD);
        String drivesMax = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_TEXTFIELD);
        String drivesMinUnit = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MIN_DROPDOWN);
        String drivesMaxUnit = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_DROPDOWN);

        drivesMin = drivesMin != null ? drivesMin.trim() : "";
        drivesMax = drivesMax != null ? drivesMax.trim() : "";

        // Check to see if either minimum and maximum drivers are both set,
        // or both not set.  Error if only one is defined.
        if (!drivesMin.equals("") && drivesMax.equals("")) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_TEXT,
                "NewPolicyWizard.copyoption.error.minmaxeither",
                true);
            return false;
        } else if (drivesMin.equals("") && !drivesMax.equals("")) {
            // max without min
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MIN_TEXT,
                "NewPolicyWizard.copyoption.error.minmaxeither",
                true);
            return false;
        } else if (!drivesMin.equals("") &&
            drivesMinUnit.equals(SelectableGroupHelper.NOVAL)) {
            // First check if min/max contains a value but not choosing a unit,
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MIN_TEXT,
                "NewPolicyWizard.copyoption.error.mindrivesunit",
                true);
            return false;
        } else if (!drivesMax.equals("") &&
            drivesMaxUnit.equals(SelectableGroupHelper.NOVAL)) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_TEXT,
                "NewPolicyWizard.copyoption.error.maxdrivesunit",
                true);
            return false;
        }

        // Now check if drivesMin and drivesMax are integers (long),
        // and is greater than 0
        long drivesMinimum = -1, drivesMaximum = -1;
        try {
            if (!drivesMin.equals("")) {
                drivesMinimum = Long.parseLong(drivesMin);
                if (drivesMinimum < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MIN_TEXT,
                        "NewPolicyWizard.copyoption.error.mindrives",
                        true);
                    return false;
                }
            }
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("Drive Minimum Size is not an integer");
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MIN_TEXT,
                "NewPolicyWizard.copyoption.error.mindrives",
                true);
            return false;
        }
        try {
            if (!drivesMax.equals("")) {
                drivesMaximum = Long.parseLong(drivesMax);
                if (drivesMaximum < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_TEXT,
                        "NewPolicyWizard.copyoption.error.maxdrives",
                        true);
                    return false;
                }
            }
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("Drive Maximum Size is not an integer");
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_TEXT,
                "NewPolicyWizard.copyoption.error.maxdrives",
                true);
            return false;
        }

        // Now check if maxSize is greater than minSize
        if (!drivesMin.equals("") &&
            !drivesMax.equals("") &&
            !PolicyUtil.isMaxGreaterThanMin(
                drivesMinimum, Integer.parseInt(drivesMinUnit),
                drivesMaximum, Integer.parseInt(drivesMaxUnit))) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardTapeCopyOptionView.CHILD_DRIVES_MAX_TEXT,
                "NewPolicyWizard.copyoption.error.mingreaterthanmax",
                true);
            return false;
        }

        // If the code reaches here , either both drive min/max are defined
        // with valid values or both not defined. Set min/max, or to reset it.
        if (!drivesMin.equals("")) {
            // if we get here ... we have all four values and they are valid
            archiveCopy.setMinDrives(drivesMinimum);
            archiveCopy.setMinDrivesUnit(Integer.parseInt(drivesMinUnit));
            archiveCopy.setMaxDrives(drivesMaximum);
            archiveCopy.setMaxDrivesUnit(Integer.parseInt(drivesMaxUnit));
        } else {
            // set everything to -1
            archiveCopy.setMinDrives(-1);
            archiveCopy.setMinDrivesUnit(-1);
            archiveCopy.setMaxDrives(-1);
            archiveCopy.setMaxDrivesUnit(-1);
        }

        // buffer size & lock is removed from the wizard
        archiveCopy.setBufferSize(-1);
        archiveCopy.setBufferLocked(false);

        // start age
        String startAge = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_START_AGE_TEXTFIELD);
        String startAgeUnit = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_START_AGE_DROPDOWN);

        startAge = startAge != null ? startAge.trim() : "";


        if (!startAge.equals("")) {
            try {
                long age = Long.parseLong(startAge);
                int ageUnit = Integer.parseInt(startAgeUnit);

                // Validation
                if (age < 0 || age > Integer.MAX_VALUE) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.
                            CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startage",
                        true);
                    return false;
                } else if (
                    startAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.
                            CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startageunit",
                        true);
                    return false;
                } else {
                    // Safe to store information
                    archiveCopy.setStartAge(age);
                    archiveCopy.setStartAgeUnit(ageUnit);
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardTapeCopyOptionView.
                        CHILD_START_AGE_TEXT,
                    "NewPolicyWizard.copyoption.error.startage",
                    true);
                return false;
            }
        } else {
            archiveCopy.setStartAge(-1);
            archiveCopy.setStartAgeUnit(-1);
        }

        // start count
        String startCount = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_START_COUNT_TEXTFIELD);
        startCount = startCount != null ? startCount.trim() : "";
        if (!startCount.equals("")) {
            try {
                int count = Integer.parseInt(startCount);
                if (count < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.
                            CHILD_START_COUNT_TEXT,
                        "NewPolicyWizard.copyoption.error.startcount",
                        true);
                    return false;
                } else {
                    archiveCopy.setStartCount(count);
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardTapeCopyOptionView.
                        CHILD_START_COUNT_TEXT,
                    "NewPolicyWizard.copyoption.error.startcount",
                    true);
                return false;
            }
        } else {
            archiveCopy.setStartCount(-1);
        }

        // Start size
        String startSize = (String) wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_START_SIZE_TEXTFIELD);
        String startSizeUnit = (String)wizardModel.getValue(
            NewPolicyWizardTapeCopyOptionView.CHILD_START_SIZE_DROPDOWN);

        startSize = startSize != null ? startSize.trim() : "";
        if (!startSize.equals("")) {
            try {
                long size = Long.parseLong(startSize);
                int sizeUnit = Integer.parseInt(startSizeUnit);
                if (size >= 0 &&
                    !startSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
                     // store information
                    archiveCopy.setStartSize(size);
                    archiveCopy.setStartSizeUnit(sizeUnit);
                } else {
                    if (size < 0) {
                        setJavascriptErrorMessage(
                            wizardEvent,
                            NewPolicyWizardTapeCopyOptionView.
                                CHILD_START_SIZE_TEXT,
                            "NewPolicyWizard.copyoption.error.startsize",
                            true);
                        return false;
                    }
                    if (startSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
                        setJavascriptErrorMessage(
                            wizardEvent,
                            NewPolicyWizardTapeCopyOptionView.
                                CHILD_START_SIZE_TEXT,
                            "NewPolicyWizard.copyoption.error.startsizeunit",
                            true);
                        return false;
                    }
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardTapeCopyOptionView.
                        CHILD_START_SIZE_TEXT,
                    "NewPolicyWizard.copyoption.error.startsize",
                    true);
                return false;
            }
        } else {
            archiveCopy.setStartSize(-1);
            archiveCopy.setStartSizeUnit(-1);
        }

        // store wrapper
        updateHashMap(
            currentCopyNumber,
            getCopyType(new Integer(currentCopyNumber)),
            myWrapper);

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * Process the necessary information in Disk Copy Option page
     * Error handling is the same as processCopyParametersPage, please
     * see the comments in that function.
     * @return boolean (true for success, false for fail)
     */

    boolean processDiskCopyOptionPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        int page = pageIdToPage(wizardEvent.getPageId());
        int currentCopyNumber = getCopyNumberForCopyOptionPage(page);

        ArchiveCopyGUIWrapper myWrapper = null;

        // Grab the wrapper object from the hashMap based on the copy number
        try {
            myWrapper = getCopyGUIWrapper(currentCopyNumber);
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception caught while retrieving copy wrapper.");
            setWizardModelErrorMessage(samEx.getMessage(), samEx.getSAMerrno());
            return true;
        }

        ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();

        // offline copy method
        String offlineCopyString = (String) wizardModel.getValue(
             NewPolicyWizardDiskCopyOptionView.CHILD_OFFLINE_COPY_DROPDOWN);
        if (!offlineCopyString.equals(SelectableGroupHelper.NOVAL)) {
            int ocMethod = Integer.parseInt(offlineCopyString);
            archiveCopy.setOfflineCopyMethod(ocMethod);
        } else {
            archiveCopy.setOfflineCopyMethod(-1);
        }

        // buffer size & lock are removed from the wizard in 4.6
        archiveCopy.setBufferSize(-1);
        archiveCopy.setBufferLocked(false);

        // start age
        String startAge = (String)wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_START_AGE_TEXTFIELD);
        String startAgeUnit = (String)wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_START_AGE_DROPDOWN);

        startAge = startAge != null ? startAge.trim() : "";
        if (!startAge.equals("")) {
            try {
                long age = Long.parseLong(startAge);
                int ageUnit = Integer.parseInt(startAgeUnit);

                // Validation
                if (age < 0 || age > Integer.MAX_VALUE) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.
                            CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startage",
                        true);
                    return false;
                } else if (
                    startAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardTapeCopyOptionView.
                            CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startageunit",
                        true);
                    return false;
                } else {
                    // Safe to store information
                    archiveCopy.setStartAge(age);
                    archiveCopy.setStartAgeUnit(ageUnit);
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardDiskCopyOptionView.
                        CHILD_START_AGE_TEXT,
                    "NewPolicyWizard.copyoption.error.startage",
                    true);
                return false;
            }
        } else {
            // reset
            archiveCopy.setStartAge(-1);
            archiveCopy.setStartAgeUnit(-1);
        }

        // start count
        String startCount = (String)wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_START_COUNT_TEXTFIELD);
        startCount = startCount != null ? startCount.trim() : "";

        if (!startCount.equals("")) {
            try {
                int count = Integer.parseInt(startCount);
                if (count < 0) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardDiskCopyOptionView.
                            CHILD_START_COUNT_TEXT,
                        "NewPolicyWizard.copyoption.error.startcount",
                        true);
                    return false;
                } else {
                    // store information
                    archiveCopy.setStartCount(count);
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardDiskCopyOptionView.
                        CHILD_START_COUNT_TEXT,
                    "NewPolicyWizard.copyoption.error.startcount",
                    true);
                return false;
            }
        } else {
            // reset
            archiveCopy.setStartCount(-1);
        }

        // Start size
        String startSize = (String)wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_START_SIZE_TEXTFIELD);
        String startSizeUnit = (String)wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_START_SIZE_DROPDOWN);
        startSize = startSize != null ? startSize.trim() : "";

        if (!startSize.equals("")) {
            try {
                long size = Long.parseLong(startSize);
                if (size >= 0 &&
                    !startSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    int sizeUnit = Integer.parseInt(startSizeUnit);
                    // Store information
                    archiveCopy.setStartSize(size);
                    archiveCopy.setStartSizeUnit(sizeUnit);
                } else {
                    if (size < 0) {
                        setJavascriptErrorMessage(
                            wizardEvent,
                            NewPolicyWizardDiskCopyOptionView.
                                CHILD_START_SIZE_TEXT,
                            "NewPolicyWizard.copyoption.error.startsize",
                            true);
                        return false;
                    } else if (
                        startSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
                        setJavascriptErrorMessage(
                            wizardEvent,
                            NewPolicyWizardDiskCopyOptionView.
                                CHILD_START_SIZE_TEXT,
                            "NewPolicyWizard.copyoption.error.startsizeunit",
                            true);
                        return false;
                    }
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardDiskCopyOptionView.
                        CHILD_START_SIZE_TEXT,
                    "NewPolicyWizard.copyoption.error.startsize",
                    true);
                return false;
            }
        } else {
            // reset
            archiveCopy.setStartSize(-1);
            archiveCopy.setStartSizeUnit(-1);
        }

        // hwm
        String recycleHwm = (String)wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_RECYCLE_HWM_TEXTFIELD);
        recycleHwm = recycleHwm != null ? recycleHwm.trim() : "";

        if (!recycleHwm.equals("")) {
            try {
                int hwm = Integer.parseInt(recycleHwm);
                if ((hwm > 100) || (hwm < 0)) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardDiskCopyOptionView.
                            CHILD_RECYCLE_HWM_TEXT,
                        "NewArchivePolWizard.page5.errRecycleHwm",
                        true);
                    return false;
                } else {
                    // store information
                    archiveCopy.setRecycleHWM(hwm);
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardDiskCopyOptionView.
                        CHILD_RECYCLE_HWM_TEXT,
                    "NewArchivePolWizard.page5.errRecycleHwm",
                    true);
                return false;
            }
        } else {
            // reset
            archiveCopy.setRecycleHWM(-1);
        }

        // ignore recycling
        String ignoreRecycling = (String) wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_IGNORE_RECYCLING_CHECKBOX);
        boolean isIgnore = ignoreRecycling.equals("true") ? true : false;
        archiveCopy.setIgnoreRecycle(isIgnore);

        // notification mail
        String mailAddress = (String) wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_MAIL_ADDRESS);
        mailAddress = mailAddress != null ? mailAddress.trim() : "";

        if (!SamUtil.isValidEmailAddress(mailAddress)) {
            setJavascriptErrorMessage(
                wizardEvent,
                NewPolicyWizardDiskCopyOptionView.CHILD_MAIL_ADDRESS_TEXT,
                "NewArchivePolWizard.page5.errMailingAddress",
                true);
            return false;
        } else {
            archiveCopy.setNotificationAddress(mailAddress);
        }

        // min gain
        String minGain = (String) wizardModel.getValue(
            NewPolicyWizardDiskCopyOptionView.CHILD_MIN_GAIN_TEXTFIELD);
        minGain = minGain != null ? minGain.trim() : "";

        if (!minGain.equals("")) {
            try {
                int gain = Integer.parseInt(minGain);
                if ((gain > 100) || (gain < 0)) {
                    setJavascriptErrorMessage(
                        wizardEvent,
                        NewPolicyWizardDiskCopyOptionView.CHILD_MIN_GAIN_TEXT,
                        "NewPolicyWizard.copyoption.error.mingain",
                        true);
                    return false;
                } else {
                    // store information
                    archiveCopy.setMinGain(gain);
                }
            } catch (NumberFormatException numEx) {
                setJavascriptErrorMessage(
                    wizardEvent,
                    NewPolicyWizardDiskCopyOptionView.CHILD_MIN_GAIN_TEXT,
                    "NewPolicyWizard.copyoption.error.mingain",
                    true);
                return false;
            }
        } else {
            // reset
            archiveCopy.setMinGain(-1);
        }

        // store wrapper
        updateHashMap(
            currentCopyNumber,
            getCopyType(new Integer(currentCopyNumber)),
            myWrapper);

        TraceUtil.trace3("Exiting");
        return true;

    }

    /**
     * To retrieve the Criteria Properties Object
     * @return - the Criteria Object that is needed to store the user input
     */
    private ArchivePolCriteriaProp getCriteriaProperties() {
        return (ArchivePolCriteriaProp)
            wizardModel.getValue(CRITERIA_PROPERTIES);
    }

    /**
     * To retrieve the Wrapper Object based on the copy number
     * @return ArchiveCopyGUIWrapper of which the copy number is being entered
     */
    public ArchiveCopyGUIWrapper getCopyGUIWrapper(int copyNumber)
        throws SamFSException {
        CopyInfo info =
            (CopyInfo) copyNumberHashMap.get(new Integer(copyNumber));

        // If the corresponding CopyInfo object does not exist, create it
        // Eventually the default wrapper will be updated with user input
        // and update it in updateWizardSteps()
        if (info == null) {
            SamUtil.doPrint("HashMap contains null CopyInfo object!");
            SamUtil.doPrint(
                "Getting default wrapper, create CopyInfo and insert to map");
            ArchiveCopyGUIWrapper defaultWrapper = getDefaultWrapper();
            copyNumberHashMap.put(
                new Integer(copyNumber),
                new CopyInfo(DISK_BASE, defaultWrapper));
            wizardModel.setValue(COPY_HASHMAP,  copyNumberHashMap);
            return defaultWrapper;
        } else {
            return info.getCopyWrapper();
        }
    }

    /**
     * To retrieve the default wrapper from the back-end
     * @return Default Wrapper object from the back-end
     */
    public ArchiveCopyGUIWrapper getDefaultWrapper() throws SamFSException {
        SamUtil.doPrint("getDefaultWrapper called");
        ArchiveCopyGUIWrapper wrapper =
            PolicyUtil.getArchiveManager(
                getServerName()).getArchiveCopyGUIWrapper();
        if (wrapper == null) {
            throw new SamFSException(null, -2023);
        } else {
            return wrapper;
        }
    }

    /**
     * To check if the input policy name is already in used
     * @return true if exists, false if not exists
     */
    private boolean isPolicyNameInUsed(String policyName)
        throws SamFSException {
        TraceUtil.trace3("Entering");
        String[] existingPolicyNames =
            PolicyUtil.getArchiveManager(
                getServerName()).getAllArchivePolicyNames();
        for (int i = 0; i < existingPolicyNames.length; i++) {
            if (policyName.equals(existingPolicyNames[i])) {
                SamUtil.doPrint(new NonSyncStringBuffer("PolicyName ").append(
                    policyName).append(" is in used!").toString());
                return true;
            }
        }
        // if code reaches here, policy name is not in used.
        return false;
    }

    /**
     * To update the wizard logic (pages) after user defines number of copies
     * @return none
     */
    private void updateWizardSteps() {
        int totalPages = -1;

        int numOfCopies =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue();
        int currentHashMapSize = getCopyNumberHashMapSize();

        totalPages = 4 + 2 * numOfCopies;

        pages = new int[totalPages];
        pages[0] = NewPolicyWizardImplData.PAGE_SELECT_TYPE;
        pages[1] = NewPolicyWizardImplData.PAGE_FILE_MATCH_CRITERIA;

        for (int i = 0; i < numOfCopies; i++) {
            pages[2 + i * 2] = NewPolicyWizardImplData.PAGE_COPY_PARAMETERS;

            // If user already defines the type of the copy(ies), use
            // the type defined by the user, otherwise, randomly use a copy type
            if (currentHashMapSize > i) {
                String copyType = getCopyType(new Integer(i + 1));
                if (copyType.equals(TAPE_BASE)) {
                    pages[3 + i * 2] =
                        NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION;
                } else {
                    pages[3 + i * 2] =
                        NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION;
                }
            } else {
                // Simply use either one of the option, will not show to user
                // due to the steps overriden in getFuturePages
                // The following code is done purely for wizard flow
                pages[3 + i * 2] =
                    NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION;
            }
        }

        pages[2 + numOfCopies * 2] = NewPolicyWizardImplData.PAGE_SUMMARY;
        pages[3 + numOfCopies * 2] = NewPolicyWizardImplData.PAGE_RESULT;

        initializeWizardPages(pages);
    }

    /**
     * To update the hashMap accordingly
     * Simply overwrite the entry (CopyInfo) if the copy number already exists.
     *
     * @param page - unique page number
     * @param type - type of the archive copy of which we add to the HashMap
     * @return void
     */
    private void updateHashMap(
        int copyNumber, String type, ArchiveCopyGUIWrapper myWrapper) {

        // add or replace the copy number entry of the hashMap
        copyNumberHashMap.put(
            new Integer(copyNumber),
            new CopyInfo(type, myWrapper));
        wizardModel.setValue(COPY_HASHMAP, copyNumberHashMap);
    }

    /**
     * To retrieve the size of the Copy Number HashMap
     * @return size of the copy number HashMap
     */
    private int getCopyNumberHashMapSize() {
        return copyNumberHashMap.size();
    }

    /**
     * To retrieve the copy type based on the provided key
     * @return copy type
     * @parameter number - key of the hashMap
     */
    private String getCopyType(Integer number) {
        String copyType = "";
        CopyInfo info = (CopyInfo) copyNumberHashMap.get(number);
        if (info != null) {
            copyType = info.getCopyType();
        }
        return copyType;
    }

    private boolean isPageCopyParameters(int page) {
        int totalCopies =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue();

        // Check if the "page" is actually a Copy Parameters Page
        if (page < totalCopies * 2 + 1) {
            for (int i = 0; i < totalCopies; i++) {
                if (page ==
                    NewPolicyWizardImplData.PAGE_COPY_PARAMETERS + i * 2) {
                    return true;
                }
            }
        }

        return false;
    }

    private boolean isPageDiskOrTapeCopyOption(int page) {
        // totalCopies need to -1 here because if the option page is a part
        // of the last copy cycle, we need to show the rest of the wizard pages
        // We can return false here and let the last else case in getFuture<>
        // to take care of the rest of the wizard steps.
        int totalCopies =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue() - 1;

        if (page < totalCopies * 2 + 2) {
            for (int i = 0; i < totalCopies; i++) {
                if (
                    (page ==
                        NewPolicyWizardImplData.PAGE_DISK_COPY_OPTION + i * 2)
                    ||
                    (page ==
                        NewPolicyWizardImplData.PAGE_TAPE_COPY_OPTION + i * 2))
                {
                    return true;
                }
            }
        }
        return false;
    }

    private String getServerName() {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private boolean noArchiveExists() {
        return Boolean.valueOf(
            (String) wizardModel.getValue(NO_ARCHIVE_EXISTS)).booleanValue();
    }

    /**
     * setJavascriptErrorMessage(wizardEvent, labelName, message, setLabel)
     * labelName = Name of the label of which you want to highlight
     * message   = Javascript message in pop up
     * setLabel  = boolean set to false for previous button
     */
    private void setJavascriptErrorMessage(
        WizardEvent event, String labelName, String message, boolean setLabel) {
        event.setSeverity(WizardEvent.ACKNOWLEDGE);
        event.setErrorMessage(message);
        if (setLabel) {
            wizardModel.setValue(VALIDATION_ERROR, labelName);
        }
    }
}
