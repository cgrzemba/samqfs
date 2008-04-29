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

// ident	$Id: NewCopyWizardImpl.java,v 1.31 2008/04/29 17:08:07 ronaldso Exp $


package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.CopyParams;
import com.sun.netstorage.samqfs.web.media.AssignMediaView;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
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
import com.sun.web.ui.util.LogUtil;

interface NewCopyWizardImplData {
    final String name = "NewCopyWizardImpl";
    final String title = "ArchiveWizard.copy.title";

    final Class [] pageClass = {
        CopyParamsView.class,
        AssignMediaView.class,
        NewCopySummary.class,
        WizardResultView.class
    };

    final int COPY_MEDIA_PAGE = 0;
    final int ASSIGN_MEDIA_PAGE = 1;
    final int SUMMARY_PAGE = 2;
    final int RESULT_PAGE = 3;

    final int[] NewPoolExpSeq = {
        COPY_MEDIA_PAGE,
        ASSIGN_MEDIA_PAGE,
        SUMMARY_PAGE,
        RESULT_PAGE
    };

    final int[] ExistPoolSeq = {
        COPY_MEDIA_PAGE,
        SUMMARY_PAGE,
        RESULT_PAGE
    };

    final String [] pageTitle = {
        "archiving.copy.mediaparam.title",
        "archiving.copy.createexp.title",
        "common.wizard.summary.title",
        "wizard.result.steptext"
    };

    final String [][] stepHelp = {
        {"archiving.copy.mediaparam.help1",
         "archiving.copy.mediaparam.help2",
         "archiving.copy.mediaparam.help3",
         "archiving.copy.mediaparam.help4"},
        {"archiving.copy.newpoolexp.help1",
         "archiving.copy.newpoolexp.help2"},
        {"common.wizard.summary.instruction"},
        {"wizard.result.help.text1",
         "wizard.result.help.text2"}
    };

    final String [] stepText = {
        "archiving.copy.mediaparam.title",
        "archiving.copy.createexp.title",
        "common.wizard.summary.title",
        "wizard.result.steptext"
    };

    final String [] stepInstruction = {
        "",
        "archiving.copy.newpoolexp.instruction.1",
        "NewArchivePolWizard.summary.instruction",
        "wizard.result.instruction",
    };

    final String [] cancelmsg = {
        "",
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
    public static final String MEDIA_EXPRESSION = "media_expression";
    public static final String MEDIA_TYPE = "media_type";
    public static final String NEW_POOL_NAME = "new_pool_name";


    private boolean wizardInitialized = false;

    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.trace3("WizardInterface: create()");
        return new NewCopyWizardImpl(requestContext);
    }

    protected NewCopyWizardImpl(RequestContext requestContext) {
        super(requestContext, PAGEMODEL_NAME);
        TraceUtil.trace3("Entering NewCopyWizardImpl()");

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
        pages = NewCopyWizardImplData.ExistPoolSeq;
        initializeWizardPages(pages);

        // initialize GUI Wrapper
        ArchiveCopyGUIWrapper myWrapper = getArchiveCopyGUIWrapper();

        String serverName = getServerName();

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
            case NewCopyWizardImplData.ASSIGN_MEDIA_PAGE:
                return processAssignMediaPage(wizardEvent);
        }

        return true;
    }

    public boolean previousStep(WizardEvent wizardEvent) {
        super.previousStep(wizardEvent);
        return true;
    }

    public boolean finishStep(WizardEvent event) {
        // make sure the wizard is still active
        if (!super.finishStep(event)) {
            return true;
        }

        boolean result = true, warningException = false;
        String errMsg = null, errCode = null, warningSummary = null;
        SamFSWarnings warning = null;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            SamQFSSystemArchiveManager archiveManager =
                    sysModel.getSamQFSSystemArchiveManager();

            // retrieve the relevant policy
            ArchivePolicy thePolicy =
                    archiveManager.getArchivePolicy(getPolicyName());

            // retrieve guiwrapper and save it
            ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();
            ArchiveVSNMap vsnMap = wrapper.getArchiveCopy().getArchiveVSNMap();

            if (pages == NewCopyWizardImplData.NewPoolExpSeq) {
                String expression =
                    (String) wizardModel.getValue(MEDIA_EXPRESSION);

                if ("pool".equals((String) wizardModel.getValue(
                                AssignMediaView.MEDIA_ASSIGN_MODE))) {
                    // Create new volume pool
                    String poolName =
                        (String) wizardModel.getValue(NEW_POOL_NAME);
                    int mediaType = Integer.parseInt(
                                    (String) wizardModel.getValue(MEDIA_TYPE));

                    // Create the volume pool, insert try/catch block here to
                    // make sure we catch the warning, but yet it will contine
                    // adding this pool for this copy
                    try {
                        LogUtil.info(
                            "Start creating Volume Pool named " + poolName);
                        TraceUtil.trace3("About to create pool " + poolName);
                        archiveManager.createVSNPool(
                                poolName, mediaType, expression);
                        LogUtil.info(
                            "Done creating Volume Pool named " + poolName);
                    } catch (SamFSWarnings wex) {
                        warning = wex;
                    }

                    vsnMap.setPoolExpression(poolName);
                    vsnMap.setArchiveMediaType(mediaType);
                } else {
                    TraceUtil.trace3(
                        "About to create copy by defining an expression.");
                }
            } else {
                TraceUtil.trace3(
                    "About to use existing pool to create a copy.");
            }

            TraceUtil.trace3(
                "Reserved: " + wrapper.getArchiveCopy().getReservationMethod());

            // Commit changes
            vsnMap.setWillBeSaved(true);

            // new add the copy to the policy
            thePolicy.addArchiveCopy(wrapper);

            // Now we are done adding the wrapper to the copy, throw the
            // warnings at create pool time if there are any
            if (warning != null) {
                throw warning;
            }

        } catch (SamFSWarnings sfw) {
            result = false;
            warningException = true;
            warningSummary = "ArchiveConfig.error";
            errMsg = "ArchiveConfig.warning.detail";
            TraceUtil.trace1("SamFSWarnings caught.", sfw);
        } catch (SamFSMultiMsgException sfme) {
            result = false;
            warningException = true;
            warningSummary = "ArchiveConfig.error";
            errMsg = sfme.getMessage();
            TraceUtil.trace1("SamFSMultiMsgException caught.", sfme);
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
            TraceUtil.trace1("SamFSException caught.", samEx);
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

    private boolean processCopyMediaPage(WizardEvent event) {
        CopyParamsView pageView = (CopyParamsView) event.getView();
        boolean result = pageView.validate(event);

        if (!result) {
            return result;
        }

        // begin by retrieving the archive gui wrapper
        ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();
        ArchivePolCriteriaCopy criteriaCopy =
                wrapper.getArchivePolCriteriaCopy();
        ArchiveCopy archiveCopy = wrapper.getArchiveCopy();
        ArchiveVSNMap vsnMap = archiveCopy.getArchiveVSNMap();

        // Safe to save archive age
        String archiveAge = (String)
            wizardModel.getValue(CopyParamsView.ARCHIVE_AGE);
        String ageUnitStr = (String)
            wizardModel.getValue(CopyParamsView.ARCHIVE_AGE_UNIT);
        criteriaCopy.setArchiveAge(Long.parseLong(archiveAge));
        criteriaCopy.setArchiveAgeUnit(Integer.parseInt(ageUnitStr));

        String archiveMenu = (String) wizardModel.getValue(
                                    CopyParamsView.ARCHIVE_MEDIA_MENU);
        if (archiveMenu.equals(CopyParamsView.MENU_CREATE_POOL) ||
            archiveMenu.equals(CopyParamsView.MENU_CREATE_EXP)) {
            // Update wizard steps if user is about to create a pool or an
            // expression for this copy
            pages = NewCopyWizardImplData.NewPoolExpSeq;
            initializeWizardPages(pages);

            if (archiveMenu.equals(CopyParamsView.MENU_CREATE_POOL)) {
                wizardModel.setValue(AssignMediaView.MEDIA_ASSIGN_MODE, "pool");
            } else {
                wizardModel.setValue(AssignMediaView.MEDIA_ASSIGN_MODE, "exp");
            }

            // reset
            vsnMap.setMapExpression("");
            vsnMap.setPoolExpression("");
            vsnMap.setArchiveMediaType(-1);

        } else if (archiveMenu.startsWith(CopyParamsView.MENU_ANY_VOL)) {
            // Use any available volumes
            // Save pool information if user chooses an existing pool
            String [] info = archiveMenu.split(",");
            vsnMap.setMapExpression(".");
            vsnMap.setPoolExpression("");
            vsnMap.setArchiveMediaType(Integer.parseInt(info[1]));
            wizardModel.setValue(MEDIA_TYPE, info[1]);
            wizardModel.setValue(NEW_POOL_NAME, "");
        } else {
            // Update wizard steps if user is using existing pools
            pages = NewCopyWizardImplData.ExistPoolSeq;
            initializeWizardPages(pages);

            // Save pool information if user chooses an existing pool
            String [] poolInfo = archiveMenu.split(",");
            vsnMap.setMapExpression("");
            vsnMap.setPoolExpression(poolInfo[0]);
            vsnMap.setArchiveMediaType(Integer.parseInt(poolInfo[1]));
            wizardModel.setValue(NEW_POOL_NAME, "");
            wizardModel.setValue(MEDIA_TYPE, poolInfo[1]);
        }

        // Reserve volume for archive policy and copy
        boolean reserved =
            Boolean.valueOf((String) wizardModel.getValue(
                    CopyParamsView.RESERVED)).booleanValue();
        archiveCopy.setReservationMethod(
            reserved ? CopyParams.RM_SET : -1);

        return result;
    }

    private boolean processAssignMediaPage(WizardEvent event) {
        AssignMediaView pageView = (AssignMediaView) event.getView();
        boolean result = pageView.validate(
            "pool".equals((String) wizardModel.getValue(
                AssignMediaView.MEDIA_ASSIGN_MODE)));

        // If error is found, return
        if (!result) {
            return result;
        }

        // Safe to save information
        try {
            String expression = pageView.getExpression();
            int mediaType = pageView.getMediaType();
            wizardModel.setValue(MEDIA_TYPE, Integer.toString(mediaType));
            wizardModel.setValue(MEDIA_EXPRESSION, expression);

            TraceUtil.trace2(
                "From AssignMediaView: expression is " + expression);
            TraceUtil.trace2(
                "From AssignMediaView: mediaType is " + mediaType);

            String mode = (String) wizardModel.getValue(
                                AssignMediaView.MEDIA_ASSIGN_MODE);
            if ("exp".equals(mode)) {
                ArchiveCopyGUIWrapper wrapper = getArchiveCopyGUIWrapper();
                ArchiveCopy archiveCopy = wrapper.getArchiveCopy();
                ArchiveVSNMap vsnMap = archiveCopy.getArchiveVSNMap();
                vsnMap.setMapExpression(expression);
                vsnMap.setArchiveMediaType(mediaType);
                wizardModel.setValue(NEW_POOL_NAME, "");
            } else {
                String poolName = pageView.getPoolName();
                TraceUtil.trace2(
                    "From AssignMediaView: poolName is " + poolName);
                wizardModel.setValue(NEW_POOL_NAME, poolName);
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to retrieve expression from AssignMediaView!", samEx);
            event.setSeverity(WizardEvent.ACKNOWLEDGE);
            event.setErrorMessage(SamUtil.getResourceString(
                "AssignMedia.error.retrieve.expression"));
            result = false;
        }

        return result;
    }

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
                TraceUtil.trace1(
                    "Failed to retreive copy GUI wrapper in copy wizard!", sfe);
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
        // clear out previous errors if wizard has been initialized
        if (wizardInitialized) {
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        }

        return super.getPageClass(pageId);
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
}
