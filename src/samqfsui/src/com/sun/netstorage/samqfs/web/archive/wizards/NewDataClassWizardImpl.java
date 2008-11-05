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

// ident	$Id: NewDataClassWizardImpl.java,v 1.24 2008/11/05 20:24:49 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.DataClassAttributes;
import com.sun.netstorage.samqfs.web.model.archive.PeriodicAudit;
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
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;


interface NewDataClassWizardImplData {
    final String name = "NewDataClassWizardImpl";
    final String title = "archiving.dataclass.wizard.title";

    final Class [] pageClass = {
        NewDataClassWizardDefineDataClassView.class,
        NewDataClassWizardDefineClassAttributesView.class,
        NewDataClassWizardDefineCopyCountView.class,
        NewDataClassWizardConfigureCopyView.class,
        NewDataClassWizardSummaryView.class,
        WizardResultView.class
    };

    final String [] pageTitle = {
        "archiving.dataclass.wizard.definedataclass.title",
        "archiving.dataclass.wizard.defineclassattributes.title",
        "archiving.dataclass.wizard.definecopycount.title",
        "archiving.dataclass.wizard.configurecopy.title",
        "archiving.dataclass.wizard.summary.title",
        "wizard.result.steptext"
    };

    final String [][] stepHelp = {
         {"archiving.dataclass.wizard.definedataclass.help.text1",
          "archiving.dataclass.wizard.definedataclass.help.text2",
          "archiving.dataclass.wizard.definedataclass.help.text3",
          "archiving.dataclass.wizard.definedataclass.help.text4",
          "archiving.dataclass.wizard.definedataclass.help.text5",
          "archiving.dataclass.wizard.definedataclass.help.text6",
          "archiving.dataclass.wizard.definedataclass.help.text7",
          "archiving.dataclass.wizard.definedataclass.help.text8",
          "archiving.dataclass.wizard.definedataclass.help.text9",
          "archiving.dataclass.wizard.definedataclass.help.text10"},
         {"archiving.dataclass.wizard.defineclassattributes.help.text1",
          "archiving.dataclass.wizard.defineclassattributes.help.text2"},
         {"archiving.dataclass.wizard.definecopycount.help.text1",
          "archiving.dataclass.wizard.definecopycount.help.text2",
          "archiving.dataclass.wizard.definecopycount.help.text3"},
         {"archiving.dataclass.wizard.configurecopy.help.text1"},
         {"archiving.dataclass.wizard.summary.help.text1"},
         {"wizard.result.help.text1",
          "wizard.result.help.text2"}
    };

    final String [] stepText = {
        "archiving.dataclass.wizard.definedataclass.title",
        "archiving.dataclass.wizard.defineclassattributes.title",
        "archiving.dataclass.wizard.definecopycount.title",
        "archiving.dataclass.wizard.configurecopy.title",
        "archiving.dataclass.wizard.summary.title",
        "wizard.result.steptext"
    };

    final String [] stepInstruction = {
        "archiving.dataclass.wizard.definedataclass.instruction",
        "archiving.dataclass.wizard.defineclassattributes.instruction",
        "archiving.dataclass.wizard.definecopycount.instruction",
        "archiving.dataclass.wizard.configurecopy.instruction",
        "archiving.dataclass.wizard.summary.instruction",
        "wizard.result.instruction"
    };

    final String [] cancelmsg = {
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        "archiving.dataclass.wizard.cancelmsg",
        ""
    };

    final int DEFINE_DATA_CLASS_PAGE = 0;
    final int DEFINE_CLASS_ATTRIBUTES_PAGE = 1;
    final int DEFINE_COPY_COUNT_PAGE = 2;
    final int CONFIGURE_COPY_PAGE = 3;
    final int SUMMARY_PAGE = 4;
    final int RESULT_PAGE = 5;
}

public class NewDataClassWizardImpl extends SamWizardImpl {

    // wizard constants
    public static final String PAGEMODEL_NAME = "NewDataClassPageModelName";
    public static final String PAGEMODEL_NAME_PREFIX = "NewDataClassWizardMode";
    public static final String IMPL_NAME = NewDataClassWizardImplData.name;
    public static final String IMPL_NAME_PREFIX = "NewDataClassWizardImpl";
    public static final String IMPL_CLASS_NAME =
        "com.sun.netstorage.samqfs.web.archive.wizards.NewDataClassWizardImpl";

    // Keep track of the Criteria Property (For ADD_POLICY PATH)
    public static final String CRITERIA_PROPERTIES = "CriteriaProperties";

    // Keep track of the Criteria (For APPLY_TO_EXISTING_POLICY PATH)
    public static final String CURRENT_CRITERIA = "CurrentCriteria";

    // Keep track of the Copy Number of which we are editing
    public static final String CURRENT_COPY_NUMBER = "CurrentCopyNumber";

    // Keep track of the total number of copies that user has to input
    public static String TOTAL_COPIES = "total_copies";

    // Keep track if no_archive already exists
    public static final String NO_ARCHIVE_EXISTS = "NoArchiveExists";

    // HashTable which is used to keep track of the copy number with its type
    private HashMap copyNumberHashMap = new HashMap();
    public static final String COPY_HASHMAP = "CopyHashMap";

    // Static String to keep track of copy type
    public static final String DISK_BASE = "DiskBase";
    public static final String TAPE_BASE = "TapeBase";

    // Keep track of the label name and change the label when error occurs
    public static final String VALIDATION_ERROR = "ValidationError";

    // Keep track of the date format for the locale
    public static final String DATE_FORMAT = "DateFormat";

    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");

        return new NewDataClassWizardImpl(requestContext);
    }

    public NewDataClassWizardImpl(RequestContext requestContext) {
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
                                    NewDataClassWizardImplData.title,
                                    IMPL_CLASS_NAME,
                                    cmdChild);
    }

    private void initializeWizard(RequestContext requestContext) {
        TraceUtil.trace3("Entering");
        wizardName = NewDataClassWizardImplData.name;
        wizardTitle = NewDataClassWizardImplData.title;
        pageClass = NewDataClassWizardImplData.pageClass;
        pageTitle = NewDataClassWizardImplData.pageTitle;
        stepHelp = NewDataClassWizardImplData.stepHelp;
        stepText = NewDataClassWizardImplData.stepText;
        stepInstruction = NewDataClassWizardImplData.stepInstruction;
        cancelMsg = NewDataClassWizardImplData.cancelmsg;

        setWizardPages(false, -1);
        setShowResultsPage(true);
        initializeWizardPages(pages);

        // set NO_ARCHIVE_EXISTS to true if no_archive already exists
        wizardModel.setValue(
            NO_ARCHIVE_EXISTS,
            Boolean.toString(
                PolicyUtil.policyExists(
                    getServerName(),
                    Constants.Archive.NOARCHIVE_POLICY_NAME)));

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
    private void setWizardPages(boolean isNoArchive, int numOfCopies) {
        // Wizard initialization
        if (numOfCopies == -1 && isNoArchive) {
            pages = new int [] {
                NewDataClassWizardImplData.DEFINE_DATA_CLASS_PAGE,
                NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE,
                NewDataClassWizardImplData.SUMMARY_PAGE,
                NewDataClassWizardImplData.RESULT_PAGE };
        } else {
            // Do not know how many copies yet
            if (numOfCopies == -1) {
                pages = new int [] {
                    NewDataClassWizardImplData.DEFINE_DATA_CLASS_PAGE,
                    NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE,
                    NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE,
                    NewDataClassWizardImplData.SUMMARY_PAGE,
                    NewDataClassWizardImplData.RESULT_PAGE };
            } else {
                pages = new int[5 + numOfCopies];
                pages[0] = NewDataClassWizardImplData.DEFINE_DATA_CLASS_PAGE;
                pages[1] =
                    NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE;
                pages[2] = NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE;
                for (int i = 3; i < pages.length; i++) {
                    pages[i] = NewDataClassWizardImplData.CONFIGURE_COPY_PAGE;
                }
                pages[pages.length - 2] =
                    NewDataClassWizardImplData.SUMMARY_PAGE;
                pages[pages.length - 1] =
                    NewDataClassWizardImplData.RESULT_PAGE;
            }
        }

        initializeWizardPages(pages);
    }

    /**
     * getFuturePages() (Overridden)
     * Need to override this method because we only need to show the first two
     * steps in the first page.  Based on the user selection in the first page,
     * the steps will be updated.
     */
    public String[] getFuturePages(String currentPageId) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering getFuturePages(): currentPageId is ").
            append(currentPageId).toString());
        int page = pageIdToPage(currentPageId);

        String [] futurePages = null;
        if (pages[page] == NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE) {
            TraceUtil.trace3("Exiting");
            return new String[0];
        } else if (pages[page] ==
            NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE) {
            String policyName = (String) wizardModel.getValue(
                NewDataClassWizardSummaryView.POLICY_NAME);
            if (policyName.equals(SamUtil.getResourceString(
                "archiving.dataclass.wizard.createnew"))) {
                TraceUtil.trace3("Exiting");
                return new String[] {
                    Integer.toString(
                        NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE)};
            }
        } else if
            (pages[page] == NewDataClassWizardImplData.DEFINE_DATA_CLASS_PAGE) {
            TraceUtil.trace3("Exiting");
            return new String[] {
                 Integer.toString(
                     NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE)};
        }

        int howMany = pages.length - page + 1;
        futurePages = new String[howMany];
        for (int i = 0; i < howMany; i++) {
            // No conversion
            futurePages[i] = Integer.toString(page + 1 + i);
        }

        TraceUtil.trace3("Exiting");
        return futurePages;
    }

    public String[] getFutureSteps(String currentPageId) {
        TraceUtil.trace3("Entering getFutureSteps()");
        int page = pageIdToPage(currentPageId);
        String[] futureSteps = null;

        if (pages[page] == NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE) {
            TraceUtil.trace3("Exiting");
            return new String[0];
        } else if (pages[page] ==
            NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE) {
            String policyName = (String) wizardModel.getValue(
                NewDataClassWizardSummaryView.POLICY_NAME);
            if (policyName.equals(SamUtil.getResourceString(
                "archiving.dataclass.wizard.createnew"))) {
                TraceUtil.trace3("Exiting");
                return new String [] {
                    stepText[
                        NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE]};
            }
            // break out and go to the last case if not create new
        } else if
            (pages[page] == NewDataClassWizardImplData.DEFINE_DATA_CLASS_PAGE) {
            TraceUtil.trace3("Exiting");
            return new String [] {
                stepText[
                    NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE]};
        }

        int howMany = pages.length - page - 1;

        futureSteps = new String[howMany];

        for (int i = 0; i < howMany; i++) {
            int futureStep = page + 1 + i;
            int futurePage = pages[futureStep];
            futureSteps[i] = SamUtil.getResourceString(
                stepText[futurePage],
                new String[] {
                    Integer.toString(futureStep - 2)});
        }

        TraceUtil.trace3("Exiting");
        return futureSteps;
    }

    public Class getPageClass(String pageId) {
        Class result = super.getPageClass(pageId);
        int page = pageIdToPage(pageId);
        // Page 0: Define Data Class
        // Page 1: Define Class Attributes
        // Page 2: Define Copy Count
        // Page 3-6: Config copy
        // Page (Total-2): Summary
        // Page (Total-1): Result

        // Save Copy Number
        int totalPageSize = pages.length;
        if (totalPageSize >= 6 && page > 2 && page < totalPageSize - 2) {
            int currentCopyNumber = page - 2;
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

        if (pages[page] == NewDataClassWizardImplData.CONFIGURE_COPY_PAGE) {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    Integer.toString(
                        page - 2)});
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

        if (pages[page] == NewDataClassWizardImplData.CONFIGURE_COPY_PAGE) {
            text = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] {
                    Integer.toString(
                        page - 2)});
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
            case NewDataClassWizardImplData.DEFINE_DATA_CLASS_PAGE:
                return processDefineDataClassPage(event);
            case NewDataClassWizardImplData.DEFINE_CLASS_ATTRIBUTES_PAGE:
                return processDefineClassAttributesPage(event);
            case NewDataClassWizardImplData.DEFINE_COPY_COUNT_PAGE:
                return processDefineCopyCountPage(event);
            case NewDataClassWizardImplData.CONFIGURE_COPY_PAGE:
                return processConfigureCopyPage(event, true);
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
            case NewDataClassWizardImplData.CONFIGURE_COPY_PAGE:
                return processConfigureCopyPage(event, false);
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

        // Determine which API to call in finishStep()
        // Call:
        // (1) thePolicy.addArchivePolCriteria(theCriteria, fsList);
        // if the selected policy exists
        //
        // (2) archiveManager.createArchivePolicy(
        // policyName,criteriaProperties,
        // wrapperArray, selectedFSArray);
        // if the selected policy DOES NOT exists (include no_archive)

        SamQFSSystemModel sysModel = null;
        String policyName = null;
        try {
            sysModel = SamUtil.getModel(getServerName());

            // add the file systems (add to all archiving fs for CIS demo)
            FileSystem [] myArchivingFS =
                sysModel.getSamQFSSystemFSManager().
                    getAllFileSystems(FileSystem.ARCHIVING);
            String [] fsArray = new String[myArchivingFS.length];
            for (int i = 0; i < myArchivingFS.length; i++) {
                fsArray[i] = myArchivingFS[i].getName();
            }

            policyName = (String) wizardModel.getValue(
                NewDataClassWizardDefineCopyCountView.POLICY_NAME);
            String applyToPolicy = (String) wizardModel.getValue(
                NewDataClassWizardDefineDataClassView.SELECT_POLICY_DROPDOWN);

            if (applyToPolicy.equals(SamUtil.getResourceString(
                "archiving.dataclass.wizard.createnew"))) {
                policyName = (String) wizardModel.getValue(
                    NewDataClassWizardDefineCopyCountView.POLICY_NAME);
                // Create the policy
                createPolicy(policyName, fsArray);
            } else if (applyToPolicy.equals(
                Constants.Archive.NOARCHIVE_POLICY_NAME) &&
                !noArchiveExists()) {
                policyName = Constants.Archive.DEFAULT_POLICY_NAME;
                // Create the policy
                createPolicy(policyName, fsArray);
            } else {
                // Add criteria to policy
                addCriteriaToPolicy(fsArray);
            }

            String className = (String) wizardModel.getValue(
                NewDataClassWizardDefineDataClassView.CLASS_NAME);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "archiving.dataclass.create.success", className));

        } catch (SamFSException ex) {
            boolean multiMsgOccurred = false;
            boolean warningOccurred = false;
            String processMsg = null;

            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                multiMsgOccurred = true;
            } else if (ex instanceof SamFSWarnings) {
                warningOccurred = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
            } else {
                processMsg = new NonSyncStringBuffer(
                    "Failed to add new data class for policy ").append(
                    policyName).toString();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                processMsg,
                getServerName());

            int errCode = ex.getSAMerrno();
            if (multiMsgOccurred) {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchiveConfig.error");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "ArchiveConfig.error.detail");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            } else if (warningOccurred) {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_WARNING);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchiveConfig.error");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "ArchiveConfig.warning.detail");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            } else {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "NewCriteriaWizard.error.summary");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    ex.getMessage());
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            }
            return true;
        }
        return true;
    }

    private void createPolicy(String policyName, String [] fsArray)
        throws SamFSException {

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

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        SamQFSSystemArchiveManager archiveManager =
            sysModel.getSamQFSSystemArchiveManager();
        if (archiveManager == null) {
            throw new SamFSException(null, -2001);
        }

        // if we get this far, update set the map save bit
        for (int i = 0; i < wrapperArray.length; i++) {
            wrapperArray[i].getArchiveCopy().
                getArchiveVSNMap().setWillBeSaved(true);
        }

        String description = (String) wizardModel.getValue(
            NewDataClassWizardDefineCopyCountView.POLICY_DESC);

        LogUtil.info(
            this.getClass(),
            "finishStep",
            new NonSyncStringBuffer("Start creating new archive policy ").
                append(policyName).toString());

        DataClassAttributes attributes =
            criteriaProperties.getDataClassAttributes();
        TraceUtil.trace2("Attributes".concat(attributes.toString()));

        // create the policy
        archiveManager.createArchivePolicy(
            policyName,
            description,
            ArSet.AR_SET_TYPE_GENERAL,
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

    private void addCriteriaToPolicy(String [] fsArray)
        throws SamFSException {

        ArchivePolCriteria theCriteria = getCurrentPolicyCriteria();

        // check duplicate
        ArrayList resultList = SamUtil.getModel(getServerName()).
            getSamQFSSystemArchiveManager().
            isDuplicateCriteria(theCriteria, fsArray, false);
        String duplicate = (String) resultList.get(0);
        if (duplicate.equals("true")) {
            wizardModel.setValue(
                Constants.Wizard.DUP_CRITERIA,
                resultList.get(1));
            wizardModel.setValue(
                Constants.Wizard.DUP_POLNAME,
                resultList.get(2));
            throw new SamFSException(null, -2025);
        }

        String policyName = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.SELECT_POLICY_DROPDOWN);

        DataClassAttributes attributes =
            theCriteria.getArchivePolCriteriaProperties().
            getDataClassAttributes();
        TraceUtil.trace2("Attributes".concat(attributes.toString()));

        // now get the policy, update & save it
        ArchivePolicy thePolicy = SamUtil.getModel(getServerName()).
            getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
        thePolicy.addArchivePolCriteria(theCriteria, fsArray);

        LogUtil.info(
            this.getClass(),
            "finishStep",
            new NonSyncStringBuffer(
                "Done creating new criteria for policy ").append(
                policyName).toString());

        wizardModel.setValue(
            Constants.AlertKeys.OPERATION_RESULT,
            Constants.AlertKeys.OPERATION_SUCCESS);
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
            "success.summary");
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
            SamUtil.getResourceString(
                "archiving.dataclass.wizard.success.applyToPolicy",
                new String [] {
                    policyName,
                    theCriteria.getArchivePolCriteriaProperties().getClassName()
                }));

        TraceUtil.trace2(new NonSyncStringBuffer(
            "Succesfully created new criteria for policy: ").append(
            policyName).toString());

    }

    private boolean processDefineDataClassPage(WizardEvent event) {
        // parameters in this method go to the criteria properties object

        // Validate class name
        String className = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.CLASS_NAME);
        className = className != null ? className.trim() : "";
        // class name can't be blank
        if ("".equals(className)) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineDataClassView.CLASS_NAME_LABEL,
                "archiving.dataclass.wizard.error.classNameEmpty",
                true);
            return false;
        }
        // class name can't contain a space
        if (className.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineDataClassView.CLASS_NAME_LABEL,
                "archiving.dataclass.wizard.error.classNameSpace",
                true);
            return false;
        }

        // class name cannot be the same as the DefaultClass name in
        // explicit default policy
        try {
            if (SamUtil.getModel(getServerName()).
                getSamQFSSystemArchiveManager().
                    isClassNameInUsed(className)) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.CLASS_NAME_LABEL,
                    "archiving.dataclass.wizard.error.classNameInUsed",
                    true);
                return false;
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to check if class name is in used!");
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineDataClassView.CLASS_NAME_LABEL,
                "archiving.policy.wizard.selectdataclass.getdataclass.failed",
                true);
            return false;
        }

        // retrieve the criteria properties in the wizardModel
        ArchivePolCriteriaProp properties = null;

        // Create the scratch Criteria object if data class is about to be
        // applied to an existing policy.  Other wise, create a scratch
        // Criteria PROPERTIES object instead.

        // See what user wants to create/apply to an existing policy
        // no validation is needed for this drop down
        String policyName = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.SELECT_POLICY_DROPDOWN);
        wizardModel.setValue(
            NewDataClassWizardSummaryView.POLICY_NAME, policyName);

        // Update wizard pages
        // Do not show copy count & config copy page if user picks no_archive,
        // and if user wants to apply to an existing policy.
        // i.e. Only show these two pages if user wants to creat a new policy
        setWizardPages(
            !policyName.equals(SamUtil.getResourceString(
                "archiving.dataclass.wizard.createnew")), -1);

        // Set total copies back to 0 if policy is no_archive
        if (policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME)) {
            wizardModel.setValue(TOTAL_COPIES, new Integer(0));
        }

        // Trim description, and set it if applying to existing policy
        // call setDescription after policy is created if we need to create a
        // policy.  The criteria object is created along with the policy.
        String description = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.DESCRIPTION);
        description = description == null ? "": description.trim();
        wizardModel.setValue(
            NewDataClassWizardDefineDataClassView.DESCRIPTION, description);

        // Save the className
        try {
            if (policyName.equals(SamUtil.getResourceString(
                "archiving.dataclass.wizard.createnew")) ||
                (policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME) &&
                !noArchiveExists())) {
                properties = createCriteriaProperties(className, description);
                // set description in finishStep();
            } else {
                // apply to existing policy
                ArchivePolCriteria criteria = getCurrentPolicyCriteria();
                properties = criteria.getArchivePolCriteriaProperties();
            }
        } catch (SamFSException samEx) {
            // TODO: Handle exception, FATAL ERROR
            setWizardModelErrorMessage(
                samEx.getMessage(), samEx.getSAMerrno());
            return false;
        }

        // validate starting directory
        String startingDir = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.START_DIR);
        startingDir = startingDir != null ? startingDir.trim() : "";
        // starting dir can't be blank
        if (startingDir.equals("")) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineDataClassView.START_DIR_LABEL,
                "NewCriteriaWizard.matchCriteriaPage.error.startingDirEmpty",
                true);
            return false;
        }
        // starting dir can't contain a space
        if (startingDir.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineDataClassView.START_DIR_LABEL,
                "NewCriteriaWizard.matchCriteriaPage.error.startingDirSpace",
                true);
            return false;
        }
        // starting dir can't be absolute
        if (startingDir.charAt(0) == '/') {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineDataClassView.START_DIR_LABEL,
                "NewCriteriaWizard.matchCriteriaPage.error." +
                "startingDirAbsolutePath",
                true);
            return false;
        }

        // Save
        properties.setStartingDir(startingDir);

        // validate name pattern
        String namePattern = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.NAME_PATTERN);
        namePattern = namePattern != null ? namePattern.trim() : "";
        // only validate name pattern if a value is type
        if (!namePattern.equals("")) {
            if (namePattern.indexOf(' ') != -1) {
                // no spaces allowed
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.NAME_PATTERN_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.namePattern",
                    true);
                return false;
            }
            if (!PolicyUtil.isValidNamePattern(namePattern)) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.NAME_PATTERN_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.namePattern",
                    true);
                return false;
            }

            properties.setNamePattern(namePattern);

            // Save the pattern type only if user defines something for the
            // pattern
            String namePatternType = (String) wizardModel.getValue(
                NewDataClassWizardDefineDataClassView.NAME_PATTERN_DROPDOWN);
            properties.setNamePatternType(Integer.parseInt(namePatternType));
        } else {
            properties.setNamePattern("");
            properties.setNamePatternType(Criteria.REGEXP);
        }


        String namePatternType = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.NAME_PATTERN_DROPDOWN);
        properties.setNamePatternType(Integer.parseInt(namePatternType));

        // validate min size and max size
        String drivesMin = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.MIN_SIZE);
        String drivesMax = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.MAX_SIZE);

        drivesMin = drivesMin != null ? drivesMin.trim() : "";
        drivesMax = drivesMax != null ? drivesMax.trim() : "";
        long max = -1, min = -1;
        int maxu = -1, minu = -1;

        // reset strings in summary page
        wizardModel.setValue(NewDataClassWizardSummaryView.MIN_SIZE, "");
        wizardModel.setValue(NewDataClassWizardSummaryView.MAX_SIZE, "");

        if (!drivesMin.equals("")) {
            // verify minisize
            try {
                min = Long.parseLong(drivesMin);
                if (min < 0)  {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardDefineDataClassView.MIN_SIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.minSize",
                        true);
                    return false;
                } else {
                    String drivesMinSizeUnit =
                        (String) wizardModel.getValue(
                            NewDataClassWizardDefineDataClassView.
                            MIN_SIZE_DROPDOWN);
                  // now that we have a valid size, verify the units
                  if (!SelectableGroupHelper.NOVAL.equals(drivesMinSizeUnit)) {
                      minu = Integer.parseInt(drivesMinSizeUnit);
                  } else {
                      setJavascriptErrorMessage(event,
                          NewDataClassWizardDefineDataClassView.MIN_SIZE_LABEL,
                       "NewCriteriaWizard.matchCriteriaPage.error.minSizeUnit",
                                                true);
                      return false;
                  }
                  wizardModel.setValue(NewDataClassWizardSummaryView.MIN_SIZE,
                     new NonSyncStringBuffer().append(min).append(" ").
                            append(SamUtil.getSizeUnitL10NString(minu)).
                            toString());
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.MIN_SIZE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.minSize",
                    true);
                return false;
            }

            // verify that the minimum file size falls within the acceptable
            // range of 0 bytes and 7813PB
            if (PolicyUtil.isOverFlow(min, minu)) {
                setJavascriptErrorMessage(event,
                    NewDataClassWizardDefineDataClassView.MIN_SIZE_LABEL,
                    "CriteriaDetails.error.overflowminsize",
                    true);
                return false;
            }
        } else {
            properties.setMinSize(-1);
            properties.setMinSizeUnit(-1);
            wizardModel.setValue(NewDataClassWizardSummaryView.MIN_SIZE, "");
        }

        // Max Size
        if (!drivesMax.equals("")) {
            try {
                max = Long.parseLong(drivesMax);
                if (max < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardDefineDataClassView.MAX_SIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.maxSize",
                        true);
                    return false;
                } else {
                    // verify the max size
                    String drivesMaxSizeUnit =
                        (String) wizardModel.getValue(
                            NewDataClassWizardDefineDataClassView.
                            MAX_SIZE_DROPDOWN);
                   if (!SelectableGroupHelper.NOVAL.equals(drivesMaxSizeUnit)) {
                        maxu = Integer.parseInt(drivesMaxSizeUnit);
                    } else {
                        setJavascriptErrorMessage(event,
                         NewDataClassWizardDefineDataClassView.MAX_SIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.maxSizeUnit",
                                                  true);
                        return false;
                    }
                    wizardModel.setValue(NewDataClassWizardSummaryView.MAX_SIZE,
                        new NonSyncStringBuffer().append(max).append(" ").
                            append(SamUtil.getSizeUnitL10NString(maxu)).
                            toString());
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.MAX_SIZE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.maxSize",
                    true);
                return false;
            }
            if (PolicyUtil.isOverFlow(max, maxu)) {
                setJavascriptErrorMessage(event,
                    NewDataClassWizardDefineDataClassView.MAX_SIZE_LABEL,
                    "CriteriaDetails.error.overflowmaxsize",
                    true);
                return false;
            }
        } else {
            properties.setMaxSize(-1);
            properties.setMaxSizeUnit(-1);
            wizardModel.setValue(NewDataClassWizardSummaryView.MAX_SIZE, "");
        }

        // check if max is really greater than min (if both fields are filled)
        if (drivesMin.length() != 0 && drivesMax.length() != 0) {
            if (!PolicyUtil.isMaxGreaterThanMin(min, minu, max, maxu)) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.MAX_SIZE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.minMaxSize",
                    true);
                return false;
            }
        }

        // Save values
        properties.setMinSize(min);
        properties.setMinSizeUnit(minu);
        properties.setMaxSize(max);
        properties.setMaxSizeUnit(maxu);


        // Validate Access Age
        String accessAge = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.ACCESS_AGE);
        String accessAgeUnit = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.ACCESS_AGE_DROPDOWN);
        accessAge = accessAge != null ? accessAge.trim() : "";

        if (!accessAge.equals("")) {
            long accessAgeValue = -1;

            // if access Age is defined, a unit must be defined as well
            if (accessAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.ACCESS_AGE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.accessAgeUnit",
                    true);
                return false;
            }

            try {
                // make sure accessAge is a positive integer
                accessAgeValue = Long.parseLong(accessAge);
                if (accessAgeValue <= 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardDefineDataClassView.ACCESS_AGE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.accessAge",
                        true);
                    return false;
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.ACCESS_AGE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.accessAge",
                    true);
                return false;
            }

            properties.setAccessAge(accessAgeValue);
            properties.setAccessAgeUnit(Integer.parseInt(accessAgeUnit));
            wizardModel.setValue(NewDataClassWizardSummaryView.ACCESS_AGE,
                new NonSyncStringBuffer().append(accessAgeValue).append(" ").
                    append(SamUtil.getTimeUnitL10NString(
                        Integer.parseInt(accessAgeUnit))).toString());
        } else {
            properties.setAccessAge(-1);
            properties.setAccessAgeUnit(-1);
            wizardModel.setValue(NewDataClassWizardSummaryView.ACCESS_AGE, "");
        }

        // validate owner
        String owner = (String)
            wizardModel.getValue(NewDataClassWizardDefineDataClassView.OWNER);
        owner = owner != null ? owner.trim() : "";
        if (!owner.equals("")) {
            if (owner.indexOf(' ') != -1 ||
                !PolicyUtil.isUserValid(owner, getServerName())) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.OWNER_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.owner",
                    true);
                return false;
            }
            properties.setOwner(owner);
        }

        // validate group
        String group = (String)
            wizardModel.getValue(NewDataClassWizardDefineDataClassView.GROUP);
        group = group != null ? group.trim() : "";
        if (!group.equals("")) {
            if (group.indexOf(' ') != -1) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.GROUP_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.group",
                    true);
                return false;
            }
            if (!PolicyUtil.isGroupValid(group, getServerName())) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.GROUP_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.groupNotExist",
                    true);
                return false;
            }
            properties.setGroup(group);
        }

        // Validate Include file Date mm/dd/yyyy
        String date = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.INCLUDE_FILE_DATE);
        date = date != null ? date.trim() : "";
        boolean validDate = true;

        if (!date.equals("")) {
            Date myDate;
            try {
                String format = (String) wizardModel.getValue(
                                    NewDataClassWizardImpl.DATE_FORMAT);
                myDate = new SimpleDateFormat(format).parse(date);
                validDate = myDate != null;

                // Convert to the way the underlying layer likes
                date = new SimpleDateFormat("yyyy-MM-dd").format(myDate);
            } catch (NullPointerException nullEx) {
                validDate = false;
            } catch (ParseException parseEx) {
                validDate = false;
            }
            if (!validDate) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineDataClassView.
                        INCLUDE_FILE_DATE_LABEL,
                    "archiving.dataclass.wizard.error.invaliddate",
                    true);
                return false;
            }
            properties.setAfterDate(date);
        } else {
            properties.setAfterDate("");
        }

        // reset copy to 0 in summary
        wizardModel.setValue(
            NewDataClassWizardSummaryView.NUM_COPIES,
            Integer.toString(0));

        // if we get this far, the whole page checked out
        return true;
    }

    private boolean processDefineClassAttributesPage(WizardEvent event) {
        String policyName = (String) wizardModel.getValue(
            NewDataClassWizardSummaryView.POLICY_NAME);
        ArchivePolCriteriaProp properties = null;

        try {
            if (policyName.equals(SamUtil.getResourceString(
                "archiving.dataclass.wizard.createnew")) ||
                (policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME) &&
                !noArchiveExists())) {
                properties = getCriteriaProperties();
            } else {
                // apply to existing policy
                ArchivePolCriteria criteria = getCurrentPolicyCriteria();
                properties = criteria.getArchivePolCriteriaProperties();
            }
        } catch (SamFSException samEx) {
            // TODO: Handle exception, FATAL ERROR
            setWizardModelErrorMessage(
                samEx.getMessage(), samEx.getSAMerrno());
            return false;
        }

        DataClassAttributes attributes = properties.getDataClassAttributes();

        // autoworm
        String temp = (String) wizardModel.getValue(
            NewDataClassWizardDefineClassAttributesView.AUTO_WORM);
        attributes.setAutoWormEnabled(temp.equals("true"));

        // expiration time type
        if ("date".equals(((String) wizardModel.getValue(
            NewDataClassWizardDefineClassAttributesView.EXPT_TYPE)))) {
            // abs exp
            String expTime = (String) wizardModel.getValue(
            NewDataClassWizardDefineClassAttributesView.EXPIRATION_TIME);

            expTime = expTime != null ? expTime.trim() : "";
            Date myDate = null;
            boolean validDate = true;

            // validate time
            if (!expTime.equals("")) {
                String format = (String) wizardModel.getValue(
                                    NewDataClassWizardImpl.DATE_FORMAT);
                try {
                    myDate = new SimpleDateFormat(format).parse(expTime);
                    validDate = myDate != null;
                } catch (NullPointerException nullEx) {
                    validDate = false;
                } catch (ParseException parseEx) {
                    validDate = false;
                }

                if (!validDate) {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardDefineClassAttributesView.
                            EXPIRATION_TIME_LABEL,
                        "archiving.dataclass.wizard.error.invaliddate",
                        true);
                    return false;
                }
                attributes.setAbsoluteExpirationTime(myDate);
                attributes.setAbsoluteExpirationEnabled(true);
            } else {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineClassAttributesView.
                        EXPIRATION_TIME_LABEL,
                    "archiving.dataclass.wizard.error.invaliddate",
                    true);
                return false;
            }
        } else { // relative expiration time
            temp = (String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.DURATION);
            if (temp != null) {
                try {
                    long duration = Long.parseLong(temp);
                    int unit = Integer.parseInt((String) wizardModel.getValue(
                                NewDataClassWizardDefineClassAttributesView.
                                DURATION_UNIT));

                    attributes.setRelativeExpirationTime(duration);
                    attributes.setRelativeExpirationTimeUnit(unit);
                    attributes.setAbsoluteExpirationEnabled(false);
                } catch (NumberFormatException nfe) {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardDefineClassAttributesView.
                            EXPIRATION_TIME_LABEL,
                        "archiving.classattributes.duration.nan",
                        true);
                    return false;
                }
            } else {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineClassAttributesView.
                        EXPIRATION_TIME_LABEL,
                    "archiving.classattributes.duration.null",
                    true);
                return false;
            }
        }

        // auto deletion
        attributes.setAutoDeleteEnabled(
            "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.AUTO_DELETE)));

        // dedup
        boolean dedup = "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.DEDUP));
        attributes.setDedupEnabled(dedup);
        if (dedup) {
            attributes.setBitbybitEnabled(
                "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.BITBYBIT)));
        } else {
            attributes.setBitbybitEnabled(false);
        }

        // periodic audit
        temp = (String) wizardModel.getValue(
            NewDataClassWizardDefineClassAttributesView.PERIODIC_AUDIT);
        PeriodicAudit periodicAudit = null;
        if (temp.equals(PeriodicAudit.NONE.getStringValue())) {
            periodicAudit = PeriodicAudit.NONE;
        } else if (temp.equals(PeriodicAudit.DISK.getStringValue())) {
            periodicAudit = PeriodicAudit.DISK;
        } else {
            periodicAudit = PeriodicAudit.ALL;
        }

        if (!periodicAudit.equals(PeriodicAudit.NONE)) { // set audit period
            temp = (String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.AUDIT_PERIOD);
            if (temp != null && temp.length() > 0) {
                try {
                    long period = Long.parseLong(temp);
                    int unit = Integer.parseInt((String) wizardModel.getValue(
                        NewDataClassWizardDefineClassAttributesView.
                        AUDIT_PERIOD_UNIT));

                    attributes.setAuditPeriod(period);
                    attributes.setAuditPeriodUnit(unit);
                } catch (NumberFormatException nfe) {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardDefineClassAttributesView.
                            AUDIT_PERIOD_LABEL,
                        "archiving.classattributes.auditperiod.nan",
                        true);
                    return false;
                }
            }
        } else { // blank out audit period
            // TODO: should audit period be blanked out here?
        }
        attributes.setPeriodicAudit(periodicAudit);

        // logging
        attributes.setLogDataAuditEnabled(
            "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.LOG_AUDIT)));
        attributes.setLogDeduplicationEnabled(
            "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.LOG_DEDUP)));
        attributes.setLogAutoWormEnabled(
            "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.LOG_AUTOWORM)));
        attributes.setLogAutoDeletionEnabled(
            "true".equals((String) wizardModel.getValue(
                NewDataClassWizardDefineClassAttributesView.LOG_AUTODELETION)));

        return true;
    }

    private boolean processDefineCopyCountPage(WizardEvent event) {
        // Validate policy name
        String policyName = (String) wizardModel.getValue(
            NewDataClassWizardDefineCopyCountView.POLICY_NAME);
        policyName = policyName != null ? policyName.trim() : "";

        // policy name can't be blank
        if (policyName.equals("")) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameEmpty",
                true);
            return false;
        }
        // policy name can't contain a space
        if (policyName.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameSpace",
                true);
            return false;
        }

        if (policyName.endsWith("*")) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameAsterisk",
                true);
            return false;
        }

        // if a general policy is named 'no_archive' flag error
        if (policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME)) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardDefineCopyCountView.POLICY_NAME_LABEL,
                "archiving.dataclass.wizard.error.policyNameReserved",
                true);
            return false;
        }

        try {
            if (isPolicyNameInUsed(policyName)) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardDefineCopyCountView.POLICY_NAME_LABEL,
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
            NewDataClassWizardSummaryView.POLICY_NAME, policyName);

        // Trim description
        String description = (String) wizardModel.getValue(
            NewDataClassWizardDefineCopyCountView.POLICY_DESC);
        wizardModel.setValue(
            NewDataClassWizardDefineCopyCountView.POLICY_DESC,
            description == null ? "": description.trim());

        // Update wizard pages
        int numOfCopies = Integer.parseInt((String) wizardModel.getValue(
            NewDataClassWizardDefineCopyCountView.COPIES_DROPDOWN));
        wizardModel.setValue(TOTAL_COPIES, new Integer(numOfCopies));
        wizardModel.setValue(
            NewDataClassWizardSummaryView.NUM_COPIES,
            Integer.toString(numOfCopies));


        // setWizardPages first parameter is false because no_archive will
        // never reach this page
        setWizardPages(false, numOfCopies);

        // retrieve the criteria properties in the wizardModel
        ArchivePolCriteriaProp properties = getCriteriaProperties();

        String migrateFrom = (String) wizardModel.getValue(
            NewDataClassWizardDefineCopyCountView.MIGRATE_FROM_DROPDOWN);
        String migrateTo   = (String) wizardModel.getValue(
            NewDataClassWizardDefineCopyCountView.MIGRATE_TO_DROPDOWN);

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
            NewDataClassWizardConfigureCopyView.COPY_TIME);
        String archiveAgeUnit = (String) wizardModel.getValue(
            NewDataClassWizardConfigureCopyView.COPY_TIME_UNIT);
        archiveAge = archiveAge != null ? archiveAge.trim() : "";
        if (archiveAge.equals("")) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardConfigureCopyView.COPY_TIME_LABEL,
                "archiving.dataclass.wizard.error.copyTimeEmpty",
                isNextStep);
            error = true;
        } else if (archiveAge.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardConfigureCopyView.COPY_TIME_LABEL,
                "archiving.dataclass.wizard.error.copyTimeSpace",
                isNextStep);
            error = true;
        } else {
            try {
                aage = Long.parseLong(archiveAge);
                if (aage <= 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewDataClassWizardConfigureCopyView.COPY_TIME_LABEL,
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
                    NewDataClassWizardConfigureCopyView.COPY_TIME_LABEL,
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
            NewDataClassWizardConfigureCopyView.EXPIRATION_TIME);
        String unarchiveAgeUnits = (String) wizardModel.getValue(
            NewDataClassWizardConfigureCopyView.EXPIRATION_TIME_UNIT);
        unarchiveAge = unarchiveAge != null ? unarchiveAge.trim() : "";

        long uage = -1;
        int uageu = -1;
        if (!unarchiveAge.equals("")) {
            if (archiveAge.indexOf(' ') != -1) {
                setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardConfigureCopyView.EXPIRATION_TIME_LABEL,
                    "archiving.dataclass.wizard.error.expirationTimeSpace",
                    isNextStep);
                error = true;
            }
            try {
                uage = Long.parseLong(unarchiveAge);
                if (uage <= 0) {
                    setJavascriptErrorMessage(
                    event,
                    NewDataClassWizardConfigureCopyView.EXPIRATION_TIME_LABEL,
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
                    NewDataClassWizardConfigureCopyView.EXPIRATION_TIME_LABEL,
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
        String archiveType = DISK_BASE;
        String mediaPool = (String) wizardModel.getValue(
            NewDataClassWizardConfigureCopyView.MEDIA_POOL);
        if (mediaPool.indexOf(",") != -1) {
            String [] poolInfo = mediaPool.split(",");
            archiveType =
                Integer.parseInt(poolInfo[1]) == BaseDevice.MTYPE_DISK ?
                    DISK_BASE : TAPE_BASE;

            ArchiveVSNMap archiveVSNMap = archiveCopy.getArchiveVSNMap();
            archiveVSNMap.setArchiveMediaType(Integer.parseInt(poolInfo[1]));

            // Save Media Pool Name
            NonSyncStringBuffer buf = new NonSyncStringBuffer(poolInfo[0]);

            //  Scratch Pool
            mediaPool = (String) wizardModel.getValue(
                NewDataClassWizardConfigureCopyView.SCRATCH_POOL);

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
        } else {
            // No media pools are set up.  Show error messsage
            setJavascriptErrorMessage(
                event,
                NewDataClassWizardConfigureCopyView.MEDIA_POOL_LABEL,
                "archiving.dataclass.wizard.error.mediapool",
                isNextStep);
            error = true;
        }

        // Enable recycling?
        String enableRecycling = (String) wizardModel.getValue(
            NewDataClassWizardConfigureCopyView.ENABLE_RECYCLING);
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
    private ArchivePolCriteriaProp createCriteriaProperties(
        String className, String description)
        throws SamFSException {
        // get the default policy criteria
        ArchivePolCriteriaProp criteriaProperties =
            PolicyUtil.getArchiveManager(getServerName()).
                getDefaultArchivePolCriteriaPropertiesWithClassName(
                className, description);

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
     * message = Javascript message in pop up
     * setLabel = boolean set to false for previous button
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

    /**
     * Used in APPLY_TO_CRITERIA path.  Creating a data class and apply to an
     * existing policy.
     */
    private ArchivePolCriteria getCurrentPolicyCriteria()
        throws SamFSException {
        ArchivePolCriteria criteria =
            (ArchivePolCriteria)wizardModel.getValue(CURRENT_CRITERIA);

        // if this is the first its asked for, retrieve it
        if (criteria != null) {
            return criteria;
        }

        // retrieve the policy
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        String policyName = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.SELECT_POLICY_DROPDOWN);
        String className = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.CLASS_NAME);
        String description = (String) wizardModel.getValue(
            NewDataClassWizardDefineDataClassView.DESCRIPTION);
        ArchivePolicy thePolicy =
            sysModel.getSamQFSSystemArchiveManager().
                getArchivePolicy(policyName);

        // get the default policy criteria
        criteria = thePolicy.
            getDefaultArchivePolCriteriaForPolicy(className, description);

        // save
        wizardModel.setValue(CURRENT_CRITERIA, criteria);

        return criteria;
    }

    private boolean noArchiveExists() {
        boolean result = Boolean.valueOf((String)
            wizardModel.getValue(NO_ARCHIVE_EXISTS)).booleanValue();
        return result;
    }
}
