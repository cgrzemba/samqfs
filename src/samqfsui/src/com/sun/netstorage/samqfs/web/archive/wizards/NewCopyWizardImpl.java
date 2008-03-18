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

// ident	$Id: NewCopyWizardImpl.java,v 1.30 2008/03/17 14:43:30 am143972 Exp $


package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.WizardResultView;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;

interface NewCopyWizardImplData {
    final String name = "NewCopyWizardImpl";
    final String title = "ArchiveWizard.copy.title";

    final Class [] pageClass = {
        CopyMediaParametersView.class,
        NewCopyTapeOptions.class,
        NewCopyDiskOptions.class,
        NewCopySummary.class,
        WizardResultView.class
    };

    final int COPY_MEDIA_PAGE = 0;
    final int TAPE_COPY_OPTIONS_PAGE = 1;
    final int DISK_COPY_OPTIONS_PAGE = 2;
    final int SUMMARY_PAGE = 3;
    final int RESULT_PAGE = 4;

    final int[] TapePrefOn = {
        COPY_MEDIA_PAGE,
        TAPE_COPY_OPTIONS_PAGE,
        SUMMARY_PAGE,
        RESULT_PAGE
    };

    final int[] TapePrefOff = {
        COPY_MEDIA_PAGE,
        TAPE_COPY_OPTIONS_PAGE,
        SUMMARY_PAGE,
        RESULT_PAGE
    };

    final int[] DiskPrefOn = {
        COPY_MEDIA_PAGE,
        DISK_COPY_OPTIONS_PAGE,
        SUMMARY_PAGE,
        RESULT_PAGE
    };

    final int[] DiskPrefOff = {
        COPY_MEDIA_PAGE,
        DISK_COPY_OPTIONS_PAGE,
        SUMMARY_PAGE,
        RESULT_PAGE
    };

    final String [] pageTitle = {
        "NewArchivePolWizard.page3.title",
        "NewArchivePolWizard.page4.title",
        "NewArchivePolWizard.page5.title",
        "NewArchivePolWizard.summary.title",
        "wizard.result.steptext"
    };

    final String [][] stepHelp = {
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

    final String [] stepText = {
        "NewArchivePolWizard.page3.steptext",
        "NewArchivePolWizard.page4.steptext",
        "NewArchivePolWizard.page5.steptext",
        "NewArchivePolWizard.summary.steptext",
        "wizard.result.steptext"
    };

    final String [] stepInstruction = {
        "NewArchivePolWizard.page3.instruction",
        "NewArchivePolWizard.page4.instruction",
        "NewArchivePolWizard.page5.instruction",
        "NewArchivePolWizard.summary.instruction",
        "wizard.result.instruction",
    };

    final String [] cancelmsg = {
        "",
        "",
        "",
        "",
        "",
        ""
    };
}

public class NewCopyWizardImpl extends SamWizardImpl {

    // wizard constants
    public static final String PAGEMODEL_NAME = "NewCopyPageModelName";
    public static final String PAGEMODEL_NAME_PREFIX = "NewCopyWizardMode";
    public static final String IMPL_NAME = NewCopyWizardImplData.name;
    public static final String IMPL_NAME_PREFIX = "NewCopyImpl";
    public static final String IMPL_CLASS_NAME =
        "com.sun.netstorage.samqfs.web.archive.wizards.NewCopyWizardImpl";

    public static final String COPY_GUIWRAPPER = "new_gui_copy_wrapper";
    public static final String ARCHIVING_TYPE = "archiving_type_string";
    public static final String MEDIA_TYPE = "new_copy_media_type";

    // Keep track of the label name and change the label when error occurs
    public static final String VALIDATION_ERROR = "ValidationError";

    // variables to define radio buttons as disk or tape
    public static final String TAPE = "tape";
    public static final String DISK = "disk";

    private boolean wizardInitialized = false;

    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");

        return new NewCopyWizardImpl(requestContext);
    }

    protected NewCopyWizardImpl(RequestContext requestContext) {
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

        // retrieve and save policy name
        String policyName = requestContext.getRequest().getParameter(
            Constants.SessionAttributes.POLICY_NAME);
        TraceUtil.trace3(new NonSyncStringBuffer(
            "WizardImpl Const' policyName is ").append(policyName).toString());
        wizardModel.setValue(
            Constants.SessionAttributes.POLICY_NAME,
            policyName);

        initializeWizard(requestContext);
        initializeWizardControl(requestContext);
        TraceUtil.trace3("Exiting");
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return getWizardWindowModel(IMPL_NAME,
                                    NewCopyWizardImplData.title,
                                    IMPL_CLASS_NAME,
                                    cmdChild);
    }

    private void initializeWizard(RequestContext requestContext) {
        TraceUtil.trace3("Entering");
        wizardName = NewCopyWizardImplData.name;
        wizardTitle = NewCopyWizardImplData.title;
        pageClass = NewCopyWizardImplData.pageClass;
        pageTitle = NewCopyWizardImplData.pageTitle;
        stepHelp = NewCopyWizardImplData.stepHelp;
        stepText = NewCopyWizardImplData.stepText;
        stepInstruction = NewCopyWizardImplData.stepInstruction;
        cancelMsg = NewCopyWizardImplData.cancelmsg;

        setShowResultsPage(true);
        pages = NewCopyWizardImplData.DiskPrefOn;
        initializeWizardPages(pages);

        // initialize GUI Wrapper
        ArchiveCopyGUIWrapper myWrapper = getArchiveCopyGUIWrapper();

        // retrieve and save server name
        String serverName = requestContext.getRequest().getParameter(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        wizardModel.setValue(Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
                             serverName);

        // retrieve version and save it
        try {
            String version = SamUtil.getAPIVersion(serverName);
            wizardModel.setValue(Constants.Wizard.SERVER_VERSION, version);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "initializeWizard",
                                     "Unable to retrieve server version",
                                     serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * getFuturePages() (Overridden)
     * Need to override this method because we only need to show the first
     * step in the first page.  Based on the user selection in the first page,
     * the steps will be updated.
     */
    public String [] getFuturePages(String currentPageId) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering getFuturePages(): currentPageId is ").
            append(currentPageId).toString());
        int page = pageIdToPage(currentPageId) + 1;

        String [] futurePages = null;
        if (page - 1 == NewCopyWizardImplData.COPY_MEDIA_PAGE) {
            futurePages = new String[0];
        } else {
            int howMany = pages.length - page;
            futurePages = new String[howMany];
            for (int i = 0; i < howMany; i++) {
                // No conversion
                futurePages[i] = Integer.toString(page + i + 1);
            }
        }

        TraceUtil.trace3("Exiting");
        return futurePages;
    }

    public String [] getFutureSteps(String currentPageId) {
        TraceUtil.trace3("Entering getFutureSteps()");
        int page = pageIdToPage(currentPageId);
        String[] futureSteps = null;

        if (page == NewCopyWizardImplData.COPY_MEDIA_PAGE) {
            futureSteps = new String[0];
        } else {
            int howMany = pages.length - page - 1;
            futureSteps = new String[howMany];
            for (int i = 0; i < howMany; i++) {
                int futurePage = pages[page + i + 1];
                futureSteps[i] = stepText[futurePage];
            }
        }

        TraceUtil.trace3("Exiting");
        return futureSteps;
    }

    public boolean nextStep(WizardEvent wizardEvent) {
        // make this wizard active
        super.nextStep(wizardEvent);

        // when we get here, the wizard must has been initialized
        wizardInitialized = true;

        int id = pageIdToPage(wizardEvent.getPageId());

        switch (pages[id]) {
            case NewCopyWizardImplData.COPY_MEDIA_PAGE:
                return processCopyMediaPage(wizardEvent);
            case NewCopyWizardImplData.TAPE_COPY_OPTIONS_PAGE:
                return processTapeCopyOptionsPage(wizardEvent);
            case NewCopyWizardImplData.DISK_COPY_OPTIONS_PAGE:
                return processDiskCopyOptionsPage(wizardEvent);
        }

        return true;
    }

    public boolean previousStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Calling previousStep");

        // update WizardManager
        super.previousStep(wizardEvent);

        int id = pageIdToPage(wizardEvent.getPageId());

        switch (pages[id]) {
            case NewCopyWizardImplData.TAPE_COPY_OPTIONS_PAGE:
                // call this special function to have the LOCK checkbox
                // and the ignore recycle radio button remembered
                // when previous step is clicked
                return processLockandIgnoreRecycle(wizardEvent, true);
            case NewCopyWizardImplData.DISK_COPY_OPTIONS_PAGE:
                // call this special function to have the LOCK checkbox
                // and the ignore recycle radio button remembered
                // when previous step is clicked
                return processLockandIgnoreRecycle(wizardEvent, false);
        }

        return true;
    }

    public boolean finishStep(WizardEvent event) {
        boolean result = true, warningException = false;
        String errMsg = null, errCode = null, warningSummary = null;

        // make sure the wizard is still active
        if (!super.finishStep(event)) {
            return true;
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            // retrieve the relevant policy
            ArchivePolicy thePolicy =
                sysModel.getSamQFSSystemArchiveManager().
                    getArchivePolicy(getPolicyName());

            // retrieve guiwrapper and save it
            ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();

            ArchiveVSNMap vsnMap = wrapper.getArchiveCopy().getArchiveVSNMap();
            int mediaType = vsnMap.getArchiveMediaType();

            // for API version 1.3 (Rel 4.4) and up, create the disk volume
            // first, if new disk archiving with new disk volumes
            String version =
                (String)wizardModel.getValue(Constants.Wizard.SERVER_VERSION);

            String selectedVSN = CopyMediaParametersView.EXISTING_VSN;

            boolean isNewDiskVSN =
                CopyMediaParametersView.NEW_DISK_VSN.equals(selectedVSN);

            // set the willbesaved flag on the archive vsn map
            vsnMap.setWillBeSaved(true);

            // new add the copy to the policy
            thePolicy.addArchiveCopy(wrapper);


        } catch (SamFSWarnings sfw) {
            result = false;
            warningException = true;
            warningSummary = "ArchiveConfig.error";
            errMsg = "ArchiveConfig.warning.detail";
        } catch (SamFSMultiMsgException sfme) {
            result = false;
            warningException = true;
            warningSummary = "ArchiveConfig.error";
            errMsg = "ArchiveConfig.warning.detail";
        } catch (SamFSException samEx) {
            result = false;
            SamUtil.processException(
                samEx,
                this.getClass(),
                "finishStep",
                "Failed to create new copy",
                getServerName());
            errMsg = samEx.getMessage();
            errCode = Integer.toString(samEx.getSAMerrno());
        }

        if (result) {
            // SUCCESS
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                "ArchivePolCopy.action.add");
        } else {
            // WARNING
            if (warningException) {
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
                    "ArchivePolCopy.error.add");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    errMsg);
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE,
                    errCode);
            }
        }
        return true;
    }

    private boolean processLockandIgnoreRecycle(
        WizardEvent wizardEvent, boolean isTapeCopy) {
        TraceUtil.trace3("Entering");

        // retrieve the wrapper
        ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();
        ArchiveCopy copy = wrapper.getArchiveCopy();

        if (!isTapeCopy) {
            // ignore recycling only shows if disk copy side
            String ignoreRecycling = (String) wizardModel.getValue(
                NewCopyDiskOptions.CHILD_IGNORE_RECYCLING_CHECKBOX);

            boolean isIgnore = "true".equals(ignoreRecycling);
            copy.setIgnoreRecycle(isIgnore);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private boolean processCopyMediaPage(WizardEvent event) {
        TraceUtil.trace3("Entering");
        // begin by retrieving the archive gui wrapper
        ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();

        CopyMediaValidator validator =
            new CopyMediaValidator(wizardModel, event, wrapper);

        boolean result = validator.validate();

        // set values for the summary page
        wizardModel.setValue(NewCopySummary.ARCHIVE_AGE,
                             validator.getSummaryArchiveAge());
        wizardModel.setValue(NewCopySummary.ARCHIVE_TYPE,
                             validator.getSummaryArchiveType());
        wizardModel.setValue(NewCopySummary.DISK_VSN,
                             validator.getSummaryDiskVSNName());
        wizardModel.setValue(NewCopySummary.DISK_ARCHIVE_PATH,
                             validator.getSummaryDiskVSNPath());
        wizardModel.setValue(NewCopySummary.VSN_POOL_NAME,
                             validator.getSummaryVSNPoolName());
        wizardModel.setValue(NewCopySummary.SPECIFY_VSN,
                             validator.getSummarySpecifiedVSNs());
        wizardModel.setValue(NewCopySummary.RESERVE,
                             validator.getSummaryRMString());

        return result;
    }

    private boolean processTapeCopyOptionsPage(WizardEvent event) {
        ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();
        ArchiveCopy copy = wrapper.getArchiveCopy();

        // offline copy
        String offlineCopyString = (String) wizardModel.getValue(
            NewCopyTapeOptions.CHILD_OFFLINE_COPY_DROPDOWN);
        if (!SelectableGroupHelper.NOVAL.equals(offlineCopyString)) {
            copy.setOfflineCopyMethod(Integer.parseInt(offlineCopyString));
        } else {
            copy.setOfflineCopyMethod(-1);
        }

        wizardModel.setValue(
            NewCopySummary.OFFLINE_COPY,
            getOfflineCopyString(Integer.parseInt(offlineCopyString)));

        // drives
        String drives = (String) wizardModel.getValue(
            NewCopyTapeOptions.CHILD_DRIVES_TEXTFIELD);
        drives = drives != null ? drives.trim() : "";

        if (!drives.equals("")) {
            try {
                int d = Integer.parseInt(drives);
                if (d < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_DRIVES_TEXT,
                        "NewPolicyWizard.copyoption.error.drives",
                        true);
                    return false;
                } else {
                    copy.setDrives(d);
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_DRIVES_TEXT,
                    "NewPolicyWizard.copyoption.error.drives",
                    true);
                return false;
            }
        } else {
            copy.setDrives(-1);
        }

        // if both min and max are not set, don't bother
        String drivesMin = ((String)wizardModel.getValue(
            NewCopyTapeOptions.CHILD_DRIVES_MIN_TEXTFIELD)).trim();


        String drivesMax = ((String)wizardModel.getValue(
            NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXTFIELD)).trim();

        drivesMin = drivesMin != null ? drivesMin.trim() : "";
        drivesMax = drivesMax != null ? drivesMax.trim() : "";

        // if only one of min or max is set reject it
        // min without max
        if (!drivesMin.equals("") && drivesMax.equals("")) {
            setJavascriptErrorMessage(
                event,
                NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXT,
                "NewPolicyWizard.copyoption.error.minmaxeither",
                true);
            return false;
        }

        // max without min
        if (drivesMin.equals("") && !drivesMax.equals("")) {
            setJavascriptErrorMessage(
                event,
                NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXT,
                "NewPolicyWizard.copyoption.error.minmaxeither",
                true);
            return false;
        }

        if (!drivesMin.equals("") && !drivesMax.equals("")) {
            String drivesMinSizeUnit = (String)wizardModel.getValue(
                NewCopyTapeOptions.CHILD_DRIVES_MIN_DROPDOWN);
            boolean mn = false;
            boolean mnu = false;
            boolean mx = false;
            boolean mxu = false;

            long min = -1;
            int minu = -1;
            long max = -1;
            int maxu = -1;

            // verify minisize
            try {
                min = Long.parseLong(drivesMin);
                if (min < 0)  {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_DRIVES_MIN_TEXT,
                        "NewPolicyWizard.copyoption.error.mindrives",
                        true);
                    return false;
                } else {
                    mn = true;
                    // now that we have a valid size, verify the units
                    if (!SelectableGroupHelper.NOVAL.equals(drivesMinSizeUnit))
                    {
                        minu = Integer.parseInt(drivesMinSizeUnit);
                        mnu = true;
                    } else {
                        setJavascriptErrorMessage(
                            event,
                            NewCopyTapeOptions.CHILD_DRIVES_MIN_TEXT,
                            "NewPolicyWizard.copyoption.error.mindrivesunit",
                            true);
                        return false;
                    }
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_DRIVES_MIN_TEXT,
                    "NewPolicyWizard.copyoption.error.mindrives",
                    true);
                return false;
            }

            // verify the max size
            String drivesMaxSizeUnit = (String)wizardModel.getValue(
                NewCopyTapeOptions.CHILD_DRIVES_MAX_DROPDOWN);
            try {
                max = Long.parseLong(drivesMax);
                if (max < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXT,
                        "NewPolicyWizard.copyoption.error.maxdrives",
                        true);
                    return false;
                } else {
                    mx = true;
                    if (!SelectableGroupHelper.NOVAL.equals(drivesMaxSizeUnit))
                    {
                        maxu = Integer.parseInt(drivesMaxSizeUnit);
                        mxu = true;
                    } else {
                        setJavascriptErrorMessage(
                            event,
                            NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXT,
                            "NewPolicyWizard.copyoption.error.maxdrivesunit",
                            true);
                        return false;
                    }
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXT,
                    "NewPolicyWizard.copyoption.error.maxdrives",
                    true);
                return false;
            }

            // check if max is really greater than min
            if (!PolicyUtil.isMaxGreaterThanMin(min, minu, max, maxu)) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_DRIVES_MAX_TEXT,
                    "NewPolicyWizard.copyoption.error.mingreaterthanmax",
                    true);
                return false;
            }

            // if we get here ... we have all four values and they are valid
            copy.setMinDrives(min);
            copy.setMinDrivesUnit(minu);
            copy.setMaxDrives(max);
            copy.setMaxDrivesUnit(maxu);

            wizardModel.setValue(
                NewCopySummary.MAX_DRIVE,
                new NonSyncStringBuffer().append(max).append(" ").
                    append(SamUtil.getSizeUnitL10NString(maxu)).toString());
            wizardModel.setValue(
                NewCopySummary.MIN_DRIVE,
                new NonSyncStringBuffer().append(min).append(" ").
                    append(SamUtil.getSizeUnitL10NString(minu)).toString());

        } else {
            // set everything to -1
            copy.setMinDrives(-1);
            copy.setMinDrivesUnit(-1);
            copy.setMaxDrives(-1);
            copy.setMaxDrivesUnit(-1);
            wizardModel.setValue(NewCopySummary.MAX_DRIVE, "");
            wizardModel.setValue(NewCopySummary.MIN_DRIVE, "");
        }

        // buffer size & lock support have been removed in 4.6
        copy.setBufferSize(-1);

        // start age
        String startAge = (String) wizardModel.getValue(
            NewCopyTapeOptions.CHILD_START_AGE_TEXTFIELD);
        startAge = startAge != null ? startAge.trim() : "";
        if (!startAge.equals("")) {
            try {
                long age = Long.parseLong(startAge);
                if (age < 0 || age > Integer.MAX_VALUE) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startage",
                        true);
                    return false;
                }
                String startAgeUnit = (String) wizardModel.getValue(
                    NewCopyTapeOptions.CHILD_START_AGE_DROPDOWN);
                if (startAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startageunit",
                        true);
                    return false;
                }
                int ageUnit = Integer.parseInt(startAgeUnit);
                copy.setStartAge(age);
                copy.setStartAgeUnit(ageUnit);
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_START_AGE_TEXT,
                    "NewPolicyWizard.copyoption.error.startageunit",
                    true);
                return false;
            }
        } else {
            copy.setStartAge(-1);
            copy.setStartAgeUnit(-1);
        }

        // start count
        String startCount = (String)wizardModel.getValue(
            NewCopyTapeOptions.CHILD_START_COUNT_TEXTFIELD);
        startCount = startCount != null ? startCount.trim() : "";
        if (!startCount.equals("")) {
            try {
                int count = Integer.parseInt(startCount);
                if (count < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_START_COUNT_TEXT,
                        "NewPolicyWizard.copyoption.error.startcount",
                        true);
                    return false;
                }
                copy.setStartCount(count);
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_START_COUNT_TEXT,
                    "NewPolicyWizard.copyoption.error.startcount",
                    true);
                return false;
            }
        } else {
            copy.setStartCount(-1);
        }

        // Start size
        String startSize = (String) wizardModel.getValue(
            NewCopyTapeOptions.CHILD_START_SIZE_TEXTFIELD);
        startSize = startSize != null ? startSize.trim() : "";
        if (!startSize.equals("")) {
            try {
                long size = Long.parseLong(startSize);
                if (size < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_START_SIZE_TEXT,
                        "NewPolicyWizard.copyoption.error.startcount",
                        true);
                    return false;
                }
                String startSizeUnit = (String)wizardModel.getValue(
                    NewCopyTapeOptions.CHILD_START_SIZE_DROPDOWN);
                if (startSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyTapeOptions.CHILD_START_SIZE_TEXT,
                        "NewPolicyWizard.copyoption.error.startsizeunit",
                        true);
                    return false;
                }
                int sizeUnit = Integer.parseInt(startSizeUnit);
                copy.setStartSize(size);
                copy.setStartSizeUnit(sizeUnit);
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyTapeOptions.CHILD_START_SIZE_TEXT,
                    "NewPolicyWizard.copyoption.error.startcount",
                    true);
                return false;
            }
        } else {
            copy.setStartSize(-1);
            copy.setStartSizeUnit(-1);
        }

        // if we get this far, everything in this checked out
        wizardModel.setValue(COPY_GUIWRAPPER, wrapper);
        return true;
    }

    private boolean processDiskCopyOptionsPage(WizardEvent event) {
        // retrieve the wrapper
        ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();
        ArchiveCopy copy = wrapper.getArchiveCopy();

        // offline copy method
        String offlineCopyString = (String) wizardModel.getValue(
             NewCopyDiskOptions.CHILD_OFFLINE_COPY_DROPDOWN);
        if (offlineCopyString.equals(SelectableGroupHelper.NOVAL)) {
            copy.setOfflineCopyMethod(-1);
        } else {
            int ocMethod = Integer.parseInt(offlineCopyString);
            copy.setOfflineCopyMethod(ocMethod);
            wizardModel.setValue(
                NewCopySummary.OFFLINE_COPY,
                getOfflineCopyString(Integer.parseInt(offlineCopyString)));
        }

        // buffer size and lock support have been removed in 4.6
        copy.setBufferSize(-1);

        // start age
        String startAge = (String) wizardModel.getValue(
            NewCopyDiskOptions.CHILD_START_AGE_TEXTFIELD);
        startAge = startAge != null ? startAge.trim() : "";
        if (!startAge.equals("")) {
            try {
                long age = Long.parseLong(startAge);
                if (age < 0 || age > Integer.MAX_VALUE) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startage",
                        true);
                    return false;
                }
                String startAgeUnit = (String) wizardModel.getValue(
                    NewCopyDiskOptions.CHILD_START_AGE_DROPDOWN);
                if (startAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_START_AGE_TEXT,
                        "NewPolicyWizard.copyoption.error.startageunit",
                        true);
                    return false;
                }
                int ageUnit = Integer.parseInt(startAgeUnit);
                copy.setStartAge(age);
                copy.setStartAgeUnit(ageUnit);
                wizardModel.setValue(
                    NewCopySummary.START_AGE,
                    new NonSyncStringBuffer().append(age).append(" ").
                        append(getTimeUnitString(ageUnit)).toString());
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyDiskOptions.CHILD_START_AGE_TEXT,
                    "NewPolicyWizard.copyoption.error.startage",
                    true);
                return false;
            }
        } else {
            copy.setStartAge(-1);
            copy.setStartAgeUnit(-1);
            wizardModel.setValue(NewCopySummary.START_AGE, "");
        }

        // start count
        String startCount = (String)wizardModel.getValue(
                    NewCopyDiskOptions.CHILD_START_COUNT_TEXTFIELD);
        startCount = startCount != null ? startCount.trim() : "";
        if (!startCount.equals("")) {
            try {
                int count = Integer.parseInt(startCount);
                if (count < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_START_COUNT_TEXT,
                        "NewPolicyWizard.copyoption.error.startcount",
                        true);
                    return false;
                }
                copy.setStartCount(count);
                wizardModel.setValue(
                    NewCopySummary.START_COUNT,
                    startCount);
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyDiskOptions.CHILD_START_COUNT_TEXT,
                    "NewPolicyWizard.copyoption.error.startcount",
                    true);
                return false;
            }
        } else {
            copy.setStartCount(-1);
            wizardModel.setValue(NewCopySummary.START_COUNT, "");
        }

        // Start size
        String startSize = (String)wizardModel.getValue(
                    NewCopyDiskOptions.CHILD_START_SIZE_TEXTFIELD);
        startSize = startSize != null ? startSize.trim() : "";
        if (!startSize.equals("")) {
            try {
                long size = Long.parseLong(startSize);
                if (size < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_START_SIZE_TEXT,
                        "NewPolicyWizard.copyoption.error.startsize",
                        true);
                    return false;
                }
                String startSizeUnit = (String)wizardModel.getValue(
                    NewCopyDiskOptions.CHILD_START_SIZE_DROPDOWN);
                if (startSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_START_SIZE_TEXT,
                        "NewPolicyWizard.copyoption.error.startsizeunit",
                        true);
                    return false;
                }
                int sizeUnit = Integer.parseInt(startSizeUnit);
                copy.setStartSize(size);
                copy.setStartSizeUnit(sizeUnit);
                wizardModel.setValue(
                    NewCopySummary.START_SIZE,
                    new NonSyncStringBuffer().append(startSize).append(" ").
                        append(SamUtil.getSizeUnitL10NString(
                            sizeUnit)).toString());
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyDiskOptions.CHILD_START_SIZE_TEXT,
                    "NewPolicyWizard.copyoption.error.startsize",
                    true);
                return false;
            }
        } else {
            copy.setStartSize(-1);
            copy.setStartSizeUnit(-1);
            wizardModel.setValue(NewCopySummary.START_SIZE, "");
        }

        // hwm
        String recycleHwm = (String) wizardModel.getValue(
            NewCopyDiskOptions.CHILD_RECYCLE_HWM_TEXTFIELD);
        recycleHwm = recycleHwm != null ? recycleHwm.trim() : "";
        if (!recycleHwm.equals("")) {
            try {
                int hwm = Integer.parseInt(recycleHwm);
                if ((hwm > 100) || (hwm < 0)) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_RECYCLE_HWM_TEXT,
                        "NewArchivePolWizard.page5.errRecycleHwm",
                        true);
                    return false;
                }
                copy.setRecycleHWM(hwm);
                wizardModel.setValue(
                    NewCopySummary.RECYCLE_HWM,
                    recycleHwm);
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyDiskOptions.CHILD_RECYCLE_HWM_TEXT,
                    "NewArchivePolWizard.page5.errRecycleHwm",
                    true);
                return false;
            }
        } else {
            copy.setRecycleHWM(-1);
            wizardModel.setValue(NewCopySummary.RECYCLE_HWM, "");
        }

        // ignore recycling
        String ignoreRecycling = (String) wizardModel.getValue(
            NewCopyDiskOptions.CHILD_IGNORE_RECYCLING_CHECKBOX);

        boolean isIgnore = ignoreRecycling.equals("true") ? true : false;
        copy.setIgnoreRecycle(isIgnore);

        // notification mail
        String mailAddress = (String)wizardModel.getValue(
            NewCopyDiskOptions.CHILD_MAIL_ADDRESS);
        mailAddress = mailAddress != null ? mailAddress.trim() : "";

        if (!SamUtil.isValidEmailAddress(mailAddress)) {
            setJavascriptErrorMessage(
                event,
                NewCopyDiskOptions.CHILD_MAIL_ADDRESS_TEXT,
                "NewArchivePolWizard.page5.errMailingAddress",
                true);
            return false;
        }
        if (!mailAddress.equals("")) {
            copy.setNotificationAddress(mailAddress);
        }

        // min gain
        String minGain = (String)wizardModel.getValue(
            NewCopyDiskOptions.CHILD_MIN_GAIN_TEXTFIELD);
        minGain = minGain != null ? minGain.trim() : "";

        if (!minGain.equals("")) {
            try {
                int gain = Integer.parseInt(minGain);
                if ((gain > 100) || (gain < 0)) {
                    setJavascriptErrorMessage(
                        event,
                        NewCopyDiskOptions.CHILD_MIN_GAIN_TEXT,
                        "NewPolicyWizard.copyoption.error.mingain",
                        true);
                    return false;
                }
                copy.setMinGain(gain);
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCopyDiskOptions.CHILD_MIN_GAIN_TEXT,
                    "NewPolicyWizard.copyoption.error.mingain",
                    true);
                return false;
            }
        } else {
            copy.setMinGain(-1);
        }

        // save it
        wizardModel.setValue(COPY_GUIWRAPPER, wrapper);
        return true;
    }

    /**
     * NOTE: potential concurrency issue here
     */
    public ArchiveCopyGUIWrapper getArchiveCopyGUIWrapper() {
        ArchiveCopyGUIWrapper wrapper =
            (ArchiveCopyGUIWrapper)wizardModel.getValue(COPY_GUIWRAPPER);
        if (wrapper == null) {
            try {
                SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
                wrapper = sysModel.getSamQFSSystemArchiveManager().
                    getArchiveCopyGUIWrapper();
                wizardModel.setValue(COPY_GUIWRAPPER, wrapper);
            } catch (SamFSException sfe) {
                SamUtil.processException(
                    sfe,
                    this.getClass(),
                    "getArchiveCopyGUIWrapper",
                    "Failed to to retrieve copy gui wrapper",
                    getServerName());
            }
        }
        return wrapper;
    }

    public void closeStep(WizardEvent wizardEvent) {
        TraceUtil.trace2("Clearing out wizard model...");
        wizardModel.clear();
        TraceUtil.trace2("Done!");
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

    // overwrite getPageClass() to clear the WIZARD_ERROR value when
    // the wizard is initialized
    public Class getPageClass(String pageId) {
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "Entered with pageID = ").append(pageId).toString());
        int page = pageIdToPage(pageId);

        // clear out previous errors if wizard has been initialized
        if (wizardInitialized) {
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        }

        return super.getPageClass(pageId);
    }

    private String getTimeUnitString(int value) {
        switch (value) {
            case SamQFSSystemModel.TIME_SECOND:
                return SamUtil.getResourceString("ArchiveSetup.seconds");
            case SamQFSSystemModel.TIME_MINUTE:
                return SamUtil.getResourceString("ArchiveSetup.minutes");
            case SamQFSSystemModel.TIME_HOUR:
                return SamUtil.getResourceString("ArchiveSetup.hours");
            case SamQFSSystemModel.TIME_DAY:
                return SamUtil.getResourceString("ArchiveSetup.days");
            case SamQFSSystemModel.TIME_WEEK:
                return SamUtil.getResourceString("ArchiveSetup.weeks");
            default:
                return "";
        }
    }

    private String getReservationString(int value) {
        switch (value) {
            case ArchivePolicy.RES_FS:
                return SamUtil.getResourceString(
                    "NewArchivePolWizard.page4.reserveOption1");
            case ArchivePolicy.RES_POLICY:
                return SamUtil.getResourceString(
                    "NewArchivePolWizard.page4.reserveOption2");
            case ArchivePolicy.RES_DIR:
                return SamUtil.getResourceString(
                    "NewArchivePolWizard.page4.reserveOption3");
            case ArchivePolicy.RES_USER:
                return SamUtil.getResourceString(
                    "NewArchivePolWizard.page4.reserveOption4");
            case ArchivePolicy.RES_GROUP:
                return SamUtil.getResourceString(
                    "NewArchivePolWizard.page4.reserveOption5");
            default:
                return "";
        }
    }

    private String getOfflineCopyString(int value) {
        switch (value) {
            case ArchivePolicy.OC_DIRECT:
                return SamUtil.getResourceString(
                    "NewPolicyWizard.tapecopyoption.offlineCopy.direct");
            case ArchivePolicy.OC_STAGEAHEAD:
                return SamUtil.getResourceString(
                    "NewPolicyWizard.tapecopyoption.offlineCopy.stageAhead");
            case ArchivePolicy.OC_STAGEALL:
                return SamUtil.getResourceString(
                    "NewPolicyWizard.tapecopyoption.offlineCopy.stageAll");
            default:
                return "NewPolicyWizard.tapecopyoption.offlineCopy.default";
        }
    }

    private String getSpecifyVSNString(
        String startVSN, String endVSN, String vsnRange) {
        NonSyncStringBuffer returnStringBuffer = new NonSyncStringBuffer();

        if (startVSN != null && endVSN != null &&
            (startVSN.length() != 0) && (endVSN.length() != 0)) {
            returnStringBuffer.append(
                SamUtil.getResourceString(
                    "NewPolicyWizard.specifyVSN",
                    new String[] {startVSN, endVSN}));
        }

        if (vsnRange != null && vsnRange.length() != 0) {
            if (returnStringBuffer.length() != 0) {
                returnStringBuffer.append(", ");
            }
            return returnStringBuffer.append(vsnRange).toString();
        } else {
            return returnStringBuffer.toString();
        }
    }

    private String getServerName() {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private String getPolicyName() {
        String policyName = (String) wizardModel.getValue(
            Constants.SessionAttributes.POLICY_NAME);
        return policyName == null ? "" : policyName;
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
