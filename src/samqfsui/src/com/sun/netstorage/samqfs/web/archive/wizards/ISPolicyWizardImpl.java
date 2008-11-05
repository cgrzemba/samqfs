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

// ident	$Id: ISPolicyWizardImpl.java,v 1.10 2008/11/05 20:24:49 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
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


interface ISPolicyWizardImplData {
    final String name = "ISPolicyWizardImpl";
    final String title = "archiving.policy.wizard.title";

    final Class [] pageClass = {
        ISPolicyWizardDefineCopyCountView.class,
        ISPolicyWizardConfigureCopyView.class,
        ISPolicyWizardSelectDataClassView.class,
        ISPolicyWizardSummaryView.class,
        WizardResultView.class
    };

    final String [] pageTitle = {
        "archiving.dataclass.wizard.definecopycount.title",
        "archiving.dataclass.wizard.configurecopy.title",
        "archiving.policy.wizard.selectdataclass.steptitle",
        "archiving.dataclass.wizard.summary.title",
        "wizard.result.steptext"
    };

    final String [][] stepHelp = {
         {"archiving.dataclass.wizard.definecopycount.help.text1",
          "archiving.dataclass.wizard.definecopycount.help.text1"},
         {"archiving.dataclass.wizard.configurecopy.help.text1",
          "archiving.dataclass.wizard.configurecopy.help.text1"},
         {"archiving.policy.wizard.selectdataclass.help.1",
          "archiving.policy.wizard.selectdataclass.help.2"},
         {"archiving.dataclass.wizard.summary.help.text1"},
         {"wizard.result.help.text1",
          "wizard.result.help.text2"}
    };

    final String [] stepText = {
        "archiving.dataclass.wizard.definecopycount.title",
        "archiving.dataclass.wizard.configurecopy.title",
        "archiving.policy.wizard.selectdataclass.steptitle",
        "archiving.dataclass.wizard.summary.title",
        "wizard.result.steptext"
    };

    final String [] stepInstruction = {
        "archiving.dataclass.wizard.definecopycount.instruction",
        "archiving.dataclass.wizard.configurecopy.instruction",
        "archiving.policy.wizard.selectdataclass.instruction",
        "archiving.policy.wizard.summary.instruction",
        "wizard.result.instruction"
    };

    final String [] cancelmsg = {
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        ""
    };

    final int DEFINE_COPY_COUNT_PAGE = 0;
    final int CONFIGURE_COPY_PAGE = 1;
    final int SELECT_DATA_CLASS = 2;
    final int SUMMARY_PAGE = 3;
    final int RESULT_PAGE = 4;
}

public class ISPolicyWizardImpl extends SamWizardImpl {

    // wizard constants
    public static final String PAGEMODEL_NAME = "ISPolicyPageModelName";
    public static final String PAGEMODEL_NAME_PREFIX = "ISPolicyWizardMode";
    public static final String IMPL_NAME = ISPolicyWizardImplData.name;
    public static final String IMPL_NAME_PREFIX = "ISPolicyWizardImpl";
    public static final String IMPL_CLASS_NAME =
        "com.sun.netstorage.samqfs.web.archive.wizards.ISPolicyWizardImpl";

    // Keep track of the Criteria Property (For ADD_POLICY PATH)
    public static final String CRITERIA_PROPERTIES = "CriteriaProperties";

    // Keep track of the Copy Number of which we are editing
    public static final String CURRENT_COPY_NUMBER = "CurrentCopyNumber";

    // Keep track of the total number of copies that user has to input
    public static String TOTAL_COPIES = "total_copies";

    // HashTable which is used to keep track of the copy number with its type
    private HashMap copyNumberHashMap = new HashMap();
    public static final String COPY_HASHMAP = "CopyHashMap";

    // Static String to keep track of copy type
    public static final String DISK_BASE = "DiskBase";
    public static final String TAPE_BASE = "TapeBase";

    // Keep track of the selected data class
    public static final String SELECTED_DATA_CLASS = "SelectedDataClass";

    // Keep track of the label name and change the label when error occurs
    public static final String VALIDATION_ERROR = "ValidationError";


    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");

        return new ISPolicyWizardImpl(requestContext);
    }

    public ISPolicyWizardImpl(RequestContext requestContext) {
        super(requestContext, PAGEMODEL_NAME);

        TraceUtil.trace3("Entering");

        // retrieve and save the server name
        String serverName = requestContext.getRequest().getParameter(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        TraceUtil.trace3(new NonSyncStringBuffer(
            "WizardImpl Const' serverName is ").append(serverName).toString());
        wizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            serverName);

        initializeWizard(requestContext);
        initializeWizardControl(requestContext);
        TraceUtil.trace3("Exiting");
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return getWizardWindowModel(IMPL_NAME,
                                    ISPolicyWizardImplData.title,
                                    IMPL_CLASS_NAME,
                                    cmdChild);
    }

    private void initializeWizard(RequestContext requestContext) {
        TraceUtil.trace3("Entering");
        wizardName = ISPolicyWizardImplData.name;
        wizardTitle = ISPolicyWizardImplData.title;
        pageClass = ISPolicyWizardImplData.pageClass;

        pageTitle = ISPolicyWizardImplData.pageTitle;
        stepHelp = ISPolicyWizardImplData.stepHelp;
        stepText = ISPolicyWizardImplData.stepText;
        stepInstruction = ISPolicyWizardImplData.stepInstruction;

        cancelMsg = ISPolicyWizardImplData.cancelmsg;

        setWizardPages(-1);
        setShowResultsPage(true);
        initializeWizardPages(pages);

        // Create a scratch criteria properties object and store in wizardModel
        try {
            createCriteriaProperties();
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to create scratch criteriaProp Object!");
            SamUtil.processException(
                samEx,
                this.getClass(),
                "initializeWizard()",
                "Failed to create scratch criteriaProp Object!",
                getServerName());
            setWizardModelErrorMessage(
                samEx.getMessage(), samEx.getSAMerrno());
        }

        // initialize number of copy that is being filled as 0
        wizardModel.setValue(TOTAL_COPIES, new Integer(0));

        // initialize HashMap in wizardModel to be blank
        wizardModel.setValue(COPY_HASHMAP, copyNumberHashMap);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Return an integer array of page sequence, need to call initialize pages
     * after updating the class variable "pages".
     */
    private void setWizardPages(int numOfCopies) {
        // Wizard initialization
            // Do not know how many copies yet
        if (numOfCopies == -1) {
            pages = new int [] {
                ISPolicyWizardImplData.DEFINE_COPY_COUNT_PAGE,
                ISPolicyWizardImplData.SELECT_DATA_CLASS,
                ISPolicyWizardImplData.SUMMARY_PAGE,
                ISPolicyWizardImplData.RESULT_PAGE };
        } else {
            pages = new int[4 + numOfCopies];
            pages[0] = ISPolicyWizardImplData.DEFINE_COPY_COUNT_PAGE;
            for (int i = 1; i < numOfCopies + 1; i++) {
                pages[i] = ISPolicyWizardImplData.CONFIGURE_COPY_PAGE;
            }
            pages[pages.length - 3] =
                ISPolicyWizardImplData.SELECT_DATA_CLASS;
            pages[pages.length - 2] =
                ISPolicyWizardImplData.SUMMARY_PAGE;
            pages[pages.length - 1] =
                ISPolicyWizardImplData.RESULT_PAGE;
        }

        initializeWizardPages(pages);
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
        if (pages[page] == ISPolicyWizardImplData.DEFINE_COPY_COUNT_PAGE) {
            futurePages = new String[0];
        } else {
            int howMany = pages.length - page + 1;
            futurePages = new String[howMany];
            for (int i = 0; i < howMany; i++) {
                // No conversion
                futurePages[i] = Integer.toString(page + 1 + i);
            }
        }

        TraceUtil.trace3("Exiting");
        return futurePages;
    }

    public String[] getFutureSteps(String currentPageId) {
        TraceUtil.trace3("Entering getFutureSteps()");
        int page = pageIdToPage(currentPageId);
        String[] futureSteps = null;

        if (pages[page] == ISPolicyWizardImplData.DEFINE_COPY_COUNT_PAGE) {
            futureSteps = new String[0];
        } else {
            int howMany = pages.length - page - 1;

            futureSteps = new String[howMany];

            for (int i = 0; i < howMany; i++) {
                int futureStep = page + 1 + i;
                int futurePage = pages[futureStep];
                futureSteps[i] = SamUtil.getResourceString(
                    stepText[futurePage],
                    new String[] {
                        Integer.toString(futureStep)});
            }
        }

        TraceUtil.trace3("Exiting");
        return futureSteps;
    }

    public Class getPageClass(String pageId) {
        Class result = super.getPageClass(pageId);
        int page = pageIdToPage(pageId);
        // Page 0: Define Copy Count
        // Page 1-4: Config copy
        // Page (Total-3): Select Data Class
        // Page (Total-2): Summary
        // Page (Total-1): Result

        // Save Copy Number
        int totalPageSize = pages.length;
        if (totalPageSize >= 5 && page >= 1 && page < totalPageSize - 3) {
            int currentCopyNumber = page;
            wizardModel.setValue(
                CURRENT_COPY_NUMBER, new Integer(currentCopyNumber));
        }
        return result;
    }

    public String getStepTitle(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        int page = pageIdToPage(pageId);
        String title = null;

        if (pages[page] == ISPolicyWizardImplData.CONFIGURE_COPY_PAGE) {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    Integer.toString(page)});
        } else {
            title = pageTitle[pages[page]];
        }

        return title;
    }

    public String getStepText(String pageId) {
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);
        String text = null;

        if (pages[page] == ISPolicyWizardImplData.CONFIGURE_COPY_PAGE) {
            text = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    Integer.toString(page)});
        } else {
            text = pageTitle[pages[page]];
        }

        return text;

    }

    public boolean nextStep(WizardEvent event) {
        String pageId = event.getPageId();
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // make this wizard active
        super.nextStep(event);

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case ISPolicyWizardImplData.DEFINE_COPY_COUNT_PAGE:
                return processDefineCopyCountPage(event);
            case ISPolicyWizardImplData.CONFIGURE_COPY_PAGE:
                return processConfigureCopyPage(event, true);
            case ISPolicyWizardImplData.SELECT_DATA_CLASS:
                return processSelectDataClassPage(event);
        }

        return true;
    }

    public boolean previousStep(WizardEvent event) {
        String pageId = event.getPageId();
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // make this wizard active
        super.previousStep(event);

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case ISPolicyWizardImplData.CONFIGURE_COPY_PAGE:
                return processConfigureCopyPage(event, false);
            case ISPolicyWizardImplData.SELECT_DATA_CLASS:
                return processSelectDataClassPage(event);
        }

        // always return true (show no error message) for previous button
        return true;
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

    public boolean finishStep(WizardEvent event) {
        if (!super.finishStep(event)) {
            return true;
        }

        SamQFSSystemModel sysModel = null;
        SamQFSSystemArchiveManager archiveManager = null;
        String policyName = null, className = null;
        SamFSException exception = null;

        boolean policyCreationFailed = false;

        try {
            sysModel = SamUtil.getModel(getServerName());
            archiveManager = sysModel.getSamQFSSystemArchiveManager();
            if (archiveManager == null) {
                throw new SamFSException(null, -2001);
            }

            // add the file systems (add to all archiving fs for CIS demo)
            FileSystem [] myArchivingFS =
                sysModel.getSamQFSSystemFSManager().
                    getAllFileSystems(FileSystem.ARCHIVING);
            String [] fsArray = new String[myArchivingFS.length];
            for (int i = 0; i < myArchivingFS.length; i++) {
                fsArray[i] = myArchivingFS[i].getName();
            }

            policyName = (String) wizardModel.getValue(
                ISPolicyWizardDefineCopyCountView.POLICY_NAME);

            // Create the policy
            createPolicy(archiveManager, policyName, fsArray);

        } catch (SamFSWarnings ex) {
            exception = ex;
            TraceUtil.trace1(
                "SamFSWarnings thrown while creating policy".
                concat(policyName));
            TraceUtil.trace1(ex.getMessage());
        } catch (SamFSException ex) {
            policyCreationFailed = true;
            exception = ex;
            TraceUtil.trace1(
                "SamFSException thrown while creating policy".
                concat(policyName));
            TraceUtil.trace1(ex.getMessage());
        }

        if (!policyCreationFailed) {
            className =
                (String) wizardModel.getValue(SELECTED_DATA_CLASS);

            // If user wants to associate a data class with this newly created
            // policy, associate now
            try {
                if (className.length() != 0) {
                    LogUtil.info(
                        this.getClass(),
                        "finishStep",
                        "Start associating class ".concat(className).concat(
                            " to policy ").concat(policyName));
                    archiveManager.
                        associateClassWithPolicy(className, policyName);
                    LogUtil.info(
                        this.getClass(),
                        "finishStep",
                        "Done associating class ".concat(className).concat(
                            " to policy ").concat(policyName));
                }
            } catch (SamFSWarnings ex) {
                exception = ex;
                TraceUtil.trace1(
                    "SamFSWarnings thrown while associating class ".
                    concat(className).concat(" to policy ").
                    concat(policyName));
                TraceUtil.trace1(ex.getMessage());
            } catch (SamFSException ex) {
                exception = ex;
                TraceUtil.trace1(
                    "SamFSException thrown while associating class ".
                    concat(className).concat(" to policy ").
                    concat(policyName));
            }
        }

        if (exception == null) {
            // no warnings / exception at all
            TraceUtil.trace1("Creating policy success!");
            setResultMessage(
                (short) 0,
                "success.summary",
                SamUtil.getResourceString(
                    "archiving.policy.wizard.create.success", policyName),
                0);
        } else if (exception instanceof SamFSWarnings) {
            // Warning occurs either in create policy or associate class
            setResultMessage(
                (short) -2,
                "ArchiveConfig.error",
                "ArchiveConfig.warning.detail",
                exception.getSAMerrno());
        } else if (exception instanceof SamFSMultiMsgException) {
            // MultiMsg is thrown when policy is created.  Associate class
            // with policy won't get called because policy is not being created.

            // Or there is either a warning or no error in policy creation time,
            // but a multiMsg exception was thrown during class association time
            setResultMessage(
                (short) -1,
                "ArchiveConfig.error",
                "ArchiveConfig.error.detail",
                exception.getSAMerrno());
        } else {
            // Any other error that can happen either in policy creation time,
            // or class association time
            String message;

            if (policyCreationFailed) {
                // Failed already in policy creation time
                message = "Failed to add new policy ".concat(policyName);
                setResultMessage(
                    (short) -1,
                    "archiving.policy.wizard.create.failed",
                    exception.getMessage(),
                    exception.getSAMerrno());
            } else {
                // Failed in class association time
                message = "Failed to associate class ".concat(className).
                    concat(" to policy ").concat(policyName);
                setResultMessage(
                    (short) -1,
                    SamUtil.getResourceString(
                        "archiving.policy.wizard.associateclass.failed",
                        new String [] {className, policyName}),
                    exception.getMessage(),
                    exception.getSAMerrno());
            }
        }
        return true;
    }

    /**
     * Simply set the result message in the result step
     *
     * status:
     * 0 => OK
     * -1 => Error
     * -2 => Warning
     */
    private void setResultMessage(
        short status, String summary, String detail, int errCode) {

        String statusString;

        switch (status) {
            case -1:
                statusString = Constants.AlertKeys.OPERATION_FAILED;
                break;

            case -2:
                statusString = Constants.AlertKeys.OPERATION_WARNING;
                break;

            default:
                statusString = Constants.AlertKeys.OPERATION_SUCCESS;
                break;
        }

        wizardModel.setValue(
            Constants.AlertKeys.OPERATION_RESULT, statusString);
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY, summary);
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL, detail);
        if (status == -1 || status == -2) {
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
        }
    }

    private void createPolicy(
        SamQFSSystemArchiveManager archiveManager,
        String policyName,
        String [] fsArray) throws SamFSException {

        // create the appropriate size of wrapperArray
        // size 0 for no_archive

        int totalCopies =
            ((Integer) wizardModel.getValue(TOTAL_COPIES)).intValue();
        ArchiveCopyGUIWrapper[] wrapperArray =
            new ArchiveCopyGUIWrapper[totalCopies];

        // Grab the CriteriaProperty to get ready to pass to the create API
        ArchivePolCriteriaProp criteriaProperties = getCriteriaProperties();

        // Assign wrapperArray with the right wrapper object
        for (int i = 0; i < totalCopies; i++) {
            wrapperArray[i] = getCopyGUIWrapper(i + 1);
        }

        // if we get this far, update set the map save bit
        for (int i = 0; i < wrapperArray.length; i++) {
            wrapperArray[i].getArchiveCopy().
                getArchiveVSNMap().setWillBeSaved(true);
        }

        String description = (String) wizardModel.getValue(
            ISPolicyWizardDefineCopyCountView.POLICY_DESC);

        LogUtil.info(
            this.getClass(),
            "finishStep",
            new NonSyncStringBuffer("Start creating new archive policy ").
                append(policyName).toString());

        // create the policy
        archiveManager.createArchivePolicy(
            policyName,
            description,
            ArSet.AR_SET_TYPE_UNASSIGNED,
            true,
            criteriaProperties,
            wrapperArray,
            fsArray);

        LogUtil.info(
            this.getClass(),
            "finishStep",
            new NonSyncStringBuffer("Done creating new archive policy ").
                append(policyName).toString());
    }

    private boolean processDefineCopyCountPage(WizardEvent event) {
        // Validate policy name
        String policyName = (String) wizardModel.getValue(
            ISPolicyWizardDefineCopyCountView.POLICY_NAME);
        policyName = policyName != null ? policyName.trim() : "";

        // policy name can't be blank
        if (policyName.equals("")) {
            setJavascriptErrorMessage(
                event,
                ISPolicyWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameEmpty",
                true);
            return false;
        }
        // policy name can't contain a space
        if (policyName.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                ISPolicyWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameSpace",
                true);
            return false;
        }

        if (policyName.endsWith("*")) {
            setJavascriptErrorMessage(
                event,
                ISPolicyWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameAsterisk",
                true);
            return false;
        }

        // if a general policy is named 'no_archive' flag error
        if (policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME)) {
            setJavascriptErrorMessage(
                event,
                ISPolicyWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameReserved",
                true);
            return false;
        }

        try {
            if (isPolicyNameInUsed(policyName)) {
                setJavascriptErrorMessage(
                    event,
                    ISPolicyWizardDefineCopyCountView.POLICY_NAME_LABEL,
                    "archiving.dataclass.wizard.error.policyNameInUsed",
                    true);
                return false;
            }
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "processDefineCopyCOuntPage",
                "Failed to retrieve existing policy information",
                getServerName());
            setWizardModelErrorMessage(
                samEx.getMessage(), samEx.getSAMerrno());
            return true;
        }

        wizardModel.setValue(
            ISPolicyWizardSummaryView.POLICY_NAME, policyName);

        // Trim description
        String description = (String) wizardModel.getValue(
            ISPolicyWizardDefineCopyCountView.POLICY_DESC);
        wizardModel.setValue(
            ISPolicyWizardDefineCopyCountView.POLICY_DESC,
            description == null ? "": description.trim());

        // Update wizard pages
        int numOfCopies = Integer.parseInt((String) wizardModel.getValue(
            ISPolicyWizardDefineCopyCountView.COPIES_DROPDOWN));
        wizardModel.setValue(TOTAL_COPIES, new Integer(numOfCopies));
        wizardModel.setValue(
            ISPolicyWizardSummaryView.NUM_COPIES,
            Integer.toString(numOfCopies));


        // setWizardPages first parameter is false because no_archive will
        // never reach this page
        setWizardPages(numOfCopies);

        // retrieve the criteria properties in the wizardModel
        ArchivePolCriteriaProp properties = getCriteriaProperties();

        String migrateFrom = (String) wizardModel.getValue(
            ISPolicyWizardDefineCopyCountView.MIGRATE_FROM_DROPDOWN);
        String migrateTo   = (String) wizardModel.getValue(
            ISPolicyWizardDefineCopyCountView.MIGRATE_TO_DROPDOWN);

        try {
            // Staging Behavior
            if (!migrateTo.equals(SelectableGroupHelper.NOVAL)) {
                // Store information
                properties.setStageAttributes(Integer.parseInt(migrateTo));
            }

            // Releasing Behavior
            if (!migrateFrom.equals(SelectableGroupHelper.NOVAL)) {
                // Store information
                properties.setReleaseAttributes(
                    Integer.parseInt(migrateFrom), -1);
            }
        } catch (NumberFormatException numEx) {
            // This should not happen, try/catch just in case.
            TraceUtil.trace1(
                "Internal fatal error. Number exception caught!");
            // Do not throw exception, simply do not set the summary
        }

        return true;
    }

    private boolean processSelectDataClassPage(WizardEvent event) {

        ISPolicyWizardSelectDataClassView view =
            (ISPolicyWizardSelectDataClassView) event.getView();

        String selected =
            (String) view.getDisplayFieldValue(
                ISPolicyWizardSelectDataClassView.SELECTED_CLASS);
        wizardModel.setValue(SELECTED_DATA_CLASS, selected);

        return true;
    }

    private boolean processConfigureCopyPage(
        WizardEvent event, boolean isNextStep) {

        ArchiveCopyGUIWrapper myWrapper = null;

        // Grab the wrapper object from the hashMap based on the copy number
        try {
            myWrapper = getCopyGUIWrapper(getCurrentCopyNumber().intValue());
        } catch (SamFSException samEx) {
            // Keep returning true here, go to the next page and show an error
            TraceUtil.trace1("Exception caught while retrieving copy wrapper");
            setWizardModelErrorMessage(samEx.getMessage(), samEx.getSAMerrno());
            return true;
        }

        // Grab the criteria copy and the archive copy from the wrapper
        ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();
        ArchivePolCriteriaCopy criteriaCopy =
            myWrapper.getArchivePolCriteriaCopy();

        // Validate all fields before saving to the corresponding copy that
        // user is editing

        // Copy Time (Mandatory)
        // get the archive age
        long aage = -1;
        int aageu = -1;
        boolean error = false;

        String archiveAge = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.COPY_TIME);
        String archiveAgeUnit = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.COPY_TIME_UNIT);
        archiveAge = archiveAge != null ? archiveAge.trim() : "";
        if (archiveAge.equals("")) {
            setJavascriptErrorMessage(
                event,
                ISPolicyWizardConfigureCopyView.COPY_TIME_LABEL,
                "archiving.dataclass.wizard.error.copyTimeEmpty",
                isNextStep);
            error = true;
        } else if (archiveAge.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                ISPolicyWizardConfigureCopyView.COPY_TIME_LABEL,
                "archiving.dataclass.wizard.error.copyTimeSpace",
                isNextStep);
            error = true;
        } else {
            try {
                aage = Long.parseLong(archiveAge);
                if (aage <= 0) {
                    setJavascriptErrorMessage(
                        event,
                        ISPolicyWizardConfigureCopyView.COPY_TIME_LABEL,
                        "archiving.dataclass.wizard.error.copyTimeNumeric",
                        isNextStep);
                    error = true;
                } else {
                    // --> save to safe age
                    criteriaCopy.setArchiveAge(aage);

                    // Get the age unit
                    aageu = Integer.parseInt(archiveAgeUnit);
                    criteriaCopy.setArchiveAgeUnit(aageu);
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    ISPolicyWizardConfigureCopyView.COPY_TIME_LABEL,
                    "archiving.dataclass.wizard.error.copyTimeNumeric",
                    isNextStep);
                error = true;
            }
        }

        if (error && isNextStep) {
            // show error message
            return false;
        } else {
            // reset flag (no false, or user clicks Previous)
            error = false;
        }

        // Expiration Time
        String unarchiveAge = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.EXPIRATION_TIME);
        String unarchiveAgeUnits = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.EXPIRATION_TIME_UNIT);
        unarchiveAge = unarchiveAge != null ? unarchiveAge.trim() : "";

        long uage = -1;
        int uageu = -1;
        if (!unarchiveAge.equals("")) {
            if (archiveAge.indexOf(' ') != -1) {
                setJavascriptErrorMessage(
                    event,
                    ISPolicyWizardConfigureCopyView.EXPIRATION_TIME_LABEL,
                    "archiving.dataclass.wizard.error.expirationTimeSpace",
                    isNextStep);
                error = true;
            }
            try {
                uage = Long.parseLong(unarchiveAge);
                if (uage <= 0) {
                    setJavascriptErrorMessage(
                    event,
                    ISPolicyWizardConfigureCopyView.EXPIRATION_TIME_LABEL,
                    "archiving.dataclass.wizard.error.expirationTimeNumeric",
                        isNextStep);
                    error = true;
                } else {
                    // --> save to safe age
                    criteriaCopy.setUnarchiveAge(uage);
                    uageu = Integer.parseInt(unarchiveAgeUnits);
                    criteriaCopy.setUnarchiveAgeUnit(uageu);
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    ISPolicyWizardConfigureCopyView.EXPIRATION_TIME_LABEL,
                    "archiving.dataclass.wizard.error.expirationTimeNumeric",
                    isNextStep);
                error = true;
            }
        } else {
            // Field is blank, assume never expire
            criteriaCopy.setUnarchiveAge(-1);
            criteriaCopy.setUnarchiveAgeUnit(-1);
        }

        if (error && isNextStep) {
            // show error message
            return false;
        } else {
            // reset flag (no false, or user clicks Previous)
            error = false;
        }

        // TODO: Do we need to check the "Never Expire" checkbox here???

        // Media Pool
        String mediaPool = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.MEDIA_POOL);
        String [] poolInfo = mediaPool.split(",");
        String archiveType =
            Integer.parseInt(poolInfo[1]) == BaseDevice.MTYPE_DISK ?
                DISK_BASE : TAPE_BASE;

        ArchiveVSNMap archiveVSNMap = archiveCopy.getArchiveVSNMap();
        archiveVSNMap.setArchiveMediaType(Integer.parseInt(poolInfo[1]));

        // Save Media Pool Name
        NonSyncStringBuffer buf = new NonSyncStringBuffer(poolInfo[0]);

        //  Scratch Pool
        mediaPool = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.SCRATCH_POOL);

        // Skip if scratch pool is not defined
        if (mediaPool.indexOf(",") != -1) {
            poolInfo = mediaPool.split(",");
            archiveType =
                Integer.parseInt(poolInfo[1]) == BaseDevice.MTYPE_DISK ?
                    DISK_BASE : TAPE_BASE;
            buf.append(",").append(poolInfo[0]);
        }

        // input all pool names separated by comma
        archiveVSNMap.setPoolExpression(buf.toString());

        // Enable recycling?
        String enableRecycling = (String) wizardModel.getValue(
            ISPolicyWizardConfigureCopyView.ENABLE_RECYCLING);
        archiveCopy.setIgnoreRecycle(enableRecycling.equals("false"));


        updateHashMap(archiveType, myWrapper);


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
     * To retrieve the Criteria Properties Object
     * @return - the Criteria Object that is needed to store the user input
     */
    private ArchivePolCriteriaProp createCriteriaProperties()
        throws SamFSException {
        // get the default policy criteria
        ArchivePolCriteriaProp criteriaProperties =
            PolicyUtil.getArchiveManager(getServerName()).
                getDefaultArchivePolCriteriaProperties();

        // save
        wizardModel.setValue(CRITERIA_PROPERTIES, criteriaProperties);
        return criteriaProperties;
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

    /**
     * Return the current copy number of which user is editing
     */
    private Integer getCurrentCopyNumber() {
        return (Integer) wizardModel.getValue(CURRENT_COPY_NUMBER);
    }

    /**
     * To retrieve the Wrapper Object based on the copy number
     * @return ArchiveCopyGUIWrapper of which the copy number is being entered
     */
    public ArchiveCopyGUIWrapper getCopyGUIWrapper(int copyNumber)
        throws SamFSException {
        CopyInfo info = (CopyInfo)
            copyNumberHashMap.get(new Integer(copyNumber));

        // If the corresponding CopyInfo object does not exist, create it
        // Eventually the default wrapper will be updated with user input
        // and update it in updateWizardSteps()
        if (info == null) {
            SamUtil.doPrint("HashMap contains null CopyInfo object!");
            SamUtil.doPrint(
                "Getting default wrapper, create CopyInfo and insert to map");
            ArchiveCopyGUIWrapper defaultWrapper = getDefaultWrapper();

            // set copy number
            ArchiveCopy copy = defaultWrapper.getArchiveCopy();
            copy.setCopyNumber(copyNumber);

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
     * To update the hashMap accordingly
     * Simply overwrite the entry (CopyInfo) if the copy number already exists.
     *
     * @param type - type of the archive copy of which we add to the HashMap
     * @return void
     */
    private void updateHashMap(String type, ArchiveCopyGUIWrapper myWrapper) {

        // add or replace the copy number entry of the hashMap
        copyNumberHashMap.put(
            getCurrentCopyNumber(),
            new CopyInfo(type, myWrapper));
        wizardModel.setValue(COPY_HASHMAP, copyNumberHashMap);
    }

    private String getServerName() {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
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
}
