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

// ident	$Id: ReservationImpl.java,v 1.28 2008/05/16 18:38:57 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;

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

import javax.servlet.http.HttpServletRequest;


interface ReservationImplData {

    final String name  = "ReservationImpl";
    final String title = "Reservation.title";

    final int PAGE_METHOD = 0;
    final int PAGE_FS = 1;
    final int PAGE_POLICY = 2;
    final int PAGE_OWNER  = 3;
    final int PAGE_SUMMARY = 4;
    final int PAGE_RESULT  = 5;

    final int[] defaultPages  = {
        PAGE_METHOD,
        PAGE_FS,
        PAGE_SUMMARY,
        PAGE_RESULT
    };

    final Class[] pageClass = {
        ReservationMethodView.class,
        ReservationFSView.class,
        ReservationPolicyView.class,
        ReservationOwnerView.class,
        ReservationSummaryView.class,
        WizardResultView.class
    };

    final String[] cancelMsg = {
        "Reservation.method.cancelmsg",
        "Reservation.fs.cancelmsg",
        "Reservation.policy.cancelmsg",
        "Reservation.owner.cancelmsg",
        "Reservation.summary.cancelmsg",
        ""
    };

    final String[] pageTitle = {
        "Reservation.method.steptext",
        "Reservation.fs.steptext",
        "Reservation.policy.steptext",
        "Reservation.owner.steptext",
        "Reservation.summary.steptext",
        "wizard.result.steptext",

    };

    final String[][] stepHelp = {
        {"Reservation.method.help.text1",
         "Reservation.method.help.text2"},
        {"Reservation.fs.help.text1",
         "Reservation.fs.help.text2"},
        {"Reservation.policy.help.text1",
         "Reservation.policy.help.text2"},
        {"Reservation.owner.help.text1",
         "Reservation.owner.help.text2"},
        {"Reservation.summary.help.text1",
         "Reservation.summary.help.text2"},
        {"wizard.result.help.text1",
         "wizard.result.help.text2"}
    };

    final String[] stepText = {
        "Reservation.method.steptext",
        "Reservation.fs.steptext",
        "Reservation.policy.steptext",
        "Reservation.owner.steptext",
        "Reservation.summary.steptext",
        "wizard.result.steptext"
    };

    final String[] stepInstruction = {
        "Reservation.method.instruction",
        "Reservation.fs.instruction",
        "Reservation.policy.instruction",
        "Reservation.owner.instruction",
        "Reservation.summary.instruction",
        "wizard.result.instruction"
    };
}

public class ReservationImpl extends SamWizardImpl {

    public static final String WIZARDPAGEMODELNAME = "ReservationPageModelName";
    public static final String WIZARDPAGEMODELNAME_PREFIX = "WizardModel";
    public static final String WIZARDIMPLNAME = "ReservationWizardImpl";
    public static final String WIZARDIMPLNAME_PREFIX = "WizardImpl";
    public static final String WIZARDCLASSNAME =
        "com.sun.netstorage.samqfs.web.media.wizards.ReservationImpl";

    private boolean fsFlag = false, policyFlag = false, ownerFlag = false;
    private String libraryName = null;
    private String slotNumber  = null;
    private String eqValue = null;
    private String serverName  = null;

    private String method = null;
    private boolean wizardInitialized = false;

    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering create()");
        TraceUtil.trace3("Exiting");
        return new ReservationImpl(requestContext);
    }

    public ReservationImpl(RequestContext requestContext) {
         super(requestContext, WIZARDPAGEMODELNAME);
         processInitialRequest(requestContext.getRequest());
         initializeWizard();
         initializeWizardControl(requestContext);
    }

    // initialize wizard
    private void initializeWizard() {
        TraceUtil.trace3("Entering");

        wizardName = ReservationImplData.name;
        wizardTitle = ReservationImplData.title;
        pageClass  = ReservationImplData.pageClass;
        pageTitle  = ReservationImplData.pageTitle;
        stepHelp   = ReservationImplData.stepHelp;
        stepText   = ReservationImplData.stepText;
        stepInstruction = ReservationImplData.stepInstruction;
        cancelMsg  = ReservationImplData.cancelMsg;

        pages = ReservationImplData.defaultPages;
        setShowResultsPage(true);
        initializeWizardPages(pages);

        TraceUtil.trace3("Exiting");
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return
            getWizardWindowModel(
                WIZARDIMPLNAME,
                ReservationImplData.title,
                WIZARDCLASSNAME,
                cmdChild);
    }

    private void processInitialRequest(HttpServletRequest request) {
        serverName  = (String) request.getParameter(
            Constants.WizardParam.SERVER_NAME_PARAM);
        libraryName = (String) request.getParameter(
            Constants.WizardParam.LIBRARY_NAME_PARAM);
        slotNumber  = (String) request.getParameter(
            Constants.WizardParam.SLOT_NUMBER_PARAM);
        eqValue = (String) request.getParameter(
            Constants.WizardParam.EQ_VALUE_PARAM);

        if (serverName != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Wizard Key: serverName is ").append(serverName).toString());
            wizardModel.setValue(
                Constants.Wizard.SERVER_NAME,
                serverName);
        } else {
            serverName = "";
            SamUtil.doPrint("Wizard Key: serverName is null");
        }

        if (libraryName != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Wizard Key: libraryName is ").append(libraryName).toString());
        } else {
            libraryName = "";
            SamUtil.doPrint("Wizard Key: libraryName is null");
        }

        if (slotNumber != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Wizard Key: slotNumber is ").append(slotNumber).toString());
        } else {
            slotNumber = "";
            SamUtil.doPrint("Wizard Key: slotNumber is null");
        }

        if (eqValue != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Wizard Key: eqValue is ").append(eqValue).toString());
        } else {
            eqValue = "";
            SamUtil.doPrint("Wizard Key: eqValue is null");
        }
    }

    /**
     * getFuturePages() (Overridden)
     * Need to override this method because we only need to show the first
     * step in the first page.  Based on the user selection in the first page,
     * the steps will be updated.
     */
    public String[] getFuturePages(String currentPageId) {
        TraceUtil.trace3("Entering getFuturePages()");
        int page = pageIdToPage(currentPageId) + 1;
        String [] futurePages = null;

        if (page - 1 == ReservationImplData.PAGE_METHOD) {
            futurePages = new String[0];
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

        if (page == ReservationImplData.PAGE_METHOD) {
            futureSteps = new String[0];
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

    public boolean nextStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

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
            case ReservationImplData.PAGE_METHOD:
                result = processMethodPage(wizardEvent);
                if (result) {
                    updateWizardSteps();
                }
                break;

            case ReservationImplData.PAGE_FS:
                result = processFSPage(wizardEvent, true);
                break;

            case ReservationImplData.PAGE_POLICY:
                result = processPolicyPage(wizardEvent, true);
                break;

            case ReservationImplData.PAGE_OWNER:
                result = processOwnerPage(wizardEvent);
                break;
        }

         TraceUtil.trace3("Entering");
         return result;
    }

    public boolean previousStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        // update WizardManager
        super.previousStep(wizardEvent);

        String pageId = wizardEvent.getPageId();
        boolean result = true;
        TraceUtil.trace3(new NonSyncStringBuffer(
            "previousStep() pageID: ").append(pageId).toString());

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case ReservationImplData.PAGE_FS:
                result = processFSPage(wizardEvent, false);
                break;

            case ReservationImplData.PAGE_POLICY:
                result = processPolicyPage(wizardEvent, false);
                break;
        }
        TraceUtil.trace3("Exiting");
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

    public boolean finishStep(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");
        boolean returnValue = true;
        String errMsg = null, errCode = null;
        int eq = -1;

        // make sure this wizard is still active before commit
        if (super.finishStep(wizardEvent) == false) {
            return false;
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            VSN myVSN = null;

            String ogdName = null, fsName = null, policyName = null;
            int ownerType = -1;

            if (fsFlag) {
                fsName = (String) wizardModel.getValue(
                    ReservationSummaryView.CHILD_FS_FIELD);
            }

            if (policyFlag) {
                policyName = (String) wizardModel.getValue(
                    ReservationSummaryView.CHILD_POLICY_FIELD);
            }

            if (ownerFlag) {
                if (method.equals("ReservationOwner.radio1")) {
                    ownerType = VSN.OWNER;
                    ogdName = (String) wizardModel.getValue(
                        ReservationSummaryView.CHILD_OWNER_FIELD);
                } else if (method.equals("ReservationOwner.radio2")) {
                    ownerType = VSN.GROUP;
                    ogdName = (String) wizardModel.getValue(
                        ReservationSummaryView.CHILD_GROUP_FIELD);
                } else {
                    ownerType = VSN.STARTING_DIR;
                    ogdName   = (String) wizardModel.getValue(
                       ReservationSummaryView.CHILD_DIR_FIELD);
                }

                SamUtil.doPrint(new NonSyncStringBuffer().append(
                    "ogdName is ").append(ogdName).toString());
            }

            // Get the selected VSN Object
            myVSN = getSelectedVSN();

            // set slot_num attribute in wizardModel for alert messages
            eq = myVSN.getSlotNumber();
            wizardModel.setValue(
                Constants.Wizard.SLOT_NUM,
                Integer.toString(eq));

            SamUtil.doPrint("finishStep: About to reserve !!!");
            LogUtil.info(this.getClass(), "finishStep",
                "Start reserving VSN");

            myVSN.reserve(policyName, fsName, ownerType, ogdName);

            LogUtil.info(this.getClass(), "finishStep",
                "Done reserving VSN");
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep",
                "Failed to reserve VSN",
                serverName);
            errMsg  = ex.getMessage();
            errCode = Integer.toString(ex.getSAMerrno());
            returnValue = false;
        }

        if (returnValue) {
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "VSNSummary.msg.reserve",
                    Integer.toString(eq)));
        } else {
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "VSNSummary.error.reserve");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                errMsg);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                errCode);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private String saveSelectFSSettings(WizardEvent wizardEvent, boolean next) {
        TraceUtil.trace3("Entering");

        try {
            ReservationFSView pageView =
                (ReservationFSView) wizardEvent.getView();
            CCActionTable actionTable =
                (CCActionTable) pageView.getChild("SelectFSTable");
            actionTable.restoreStateData();
            CCActionTableModel fsModel =
                 (CCActionTableModel) actionTable.getModel();

            Integer [] selectedRows = fsModel.getSelectedRows();

            // display an error message if no rows are selected
            if (selectedRows.length == 0) {
                if (next) {
                    setErrorMessage(wizardEvent, "Reservation.page2.errMsg");
                    TraceUtil.trace3("Exiting");
                    return Constants.Wizard.NO_ENTRY;
                } else {
                    // It is not an error that the user clicks PREVIOUS
                    // without selecting anything
                    TraceUtil.trace3("Clearing fs field");
                    wizardModel.setValue(
                        ReservationSummaryView.CHILD_FS_FIELD, "");
                    TraceUtil.trace3("Exiting");
                    return Constants.Wizard.SUCCESS;
                }
            }

            int row = selectedRows[0].intValue();
            fsModel.setRowIndex(row);
            String fsName = (String) fsModel.getValue("NameText");
            wizardModel.setValue(
                ReservationSummaryView.CHILD_FS_FIELD, fsName);

            SamUtil.doPrint(new NonSyncStringBuffer(
                "Selected FS name is ").append(fsName).toString());
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "saveSelectFSSettings",
                "Failed to save filesystem selections",
                serverName);
            wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE,
                mcex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
        }

        TraceUtil.trace3("Exiting");
        return Constants.Wizard.SUCCESS;
    }

    private String saveSelectPolicySettings(
        WizardEvent wizardEvent, boolean next) {
        TraceUtil.trace3("Entering");
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "saveSelectPolicySettings: next is ").append(next).toString());

        try {
            CCActionTable actionTable = null;

            ReservationPolicyView pageView =
                (ReservationPolicyView) wizardEvent.getView();
            actionTable =
                (CCActionTable) pageView.getChild("SelectPolicyTable");

            actionTable.restoreStateData();
            CCActionTableModel polModel =
                (CCActionTableModel) actionTable.getModel();

            Integer [] selectedRows = polModel.getSelectedRows();

            // display an error message if no rows are selected
            if (selectedRows.length == 0) {
                if (next) {
                    setErrorMessage(wizardEvent, "Reservation.page3.errMsg");
                    TraceUtil.trace3("Exiting");
                    return Constants.Wizard.NO_ENTRY;
                } else {
                    TraceUtil.trace3("Clearing policy field");
                    wizardModel.setValue(
                        ReservationSummaryView.CHILD_POLICY_FIELD, "");
                    TraceUtil.trace3("Exiting");
                    return Constants.Wizard.SUCCESS;
                }
            }

            int row = selectedRows[0].intValue();
            polModel.setRowIndex(row);
            String polName = (String) polModel.getValue("NameText");

            wizardModel.setValue(
                ReservationSummaryView.CHILD_POLICY_FIELD, polName);

            SamUtil.doPrint(new NonSyncStringBuffer(
                "Selected Policy Name is ").append(polName).toString());

        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "saveSelectPolicySettings",
                "Failed to save policy selections",
                serverName);

            wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE,
                mcex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
        }

        TraceUtil.trace3("Exiting");
        return Constants.Wizard.SUCCESS;
    }

    private boolean processMethodPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        ReservationMethodView pageView =
            (ReservationMethodView) wizardEvent.getView();

        fsFlag = ((String) pageView.getDisplayFieldValue("CheckBox1")).
                                                            equals("true");
        policyFlag = ((String) pageView.getDisplayFieldValue("CheckBox2")).
                                                            equals("true");
        ownerFlag = ((String) pageView.getDisplayFieldValue("CheckBox3")).
                                                            equals("true");
        NonSyncStringBuffer buf = new NonSyncStringBuffer();

        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "fsFlag is ").append(fsFlag).toString());
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "policyFlag is ").append(policyFlag).toString());
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "ownerFlag is ").append(ownerFlag).toString());

        if (!fsFlag && !policyFlag && !ownerFlag) {
            setErrorMessage(wizardEvent, "Reservation.method.errMsg");
            TraceUtil.trace3("Exiting");
            return false;
        }

        if (fsFlag) {
            buf.append(SamUtil.getResourceString(
                "ReservationMethod.checkbox1"));
        } else {
            wizardModel.setValue(ReservationSummaryView.CHILD_FS_FIELD, "");
        }

        if (policyFlag) {
            if (buf.length() != 0) {
                buf.append(", ");
            }
            buf.append(SamUtil.getResourceString(
                "ReservationMethod.checkbox2"));
        } else {
            wizardModel.setValue(ReservationSummaryView.CHILD_POLICY_FIELD, "");
        }

        if (ownerFlag) {
            if (buf.length() != 0) {
                buf.append(", ");
            }
            buf.append(SamUtil.getResourceString(
                "ReservationMethod.checkbox3"));
        } else {
            wizardModel.setValue(ReservationSummaryView.CHILD_OWNER_FIELD, "");
            wizardModel.setValue(ReservationSummaryView.CHILD_GROUP_FIELD, "");
            wizardModel.setValue(ReservationSummaryView.CHILD_DIR_FIELD, "");
        }

        wizardModel.setValue(ReservationSummaryView.CHILD_METHOD_FIELD,
            buf.toString());
        TraceUtil.trace3("Exiting");
        return true;
    }

    private boolean processFSPage(WizardEvent wizardEvent, boolean isNextStep) {
        TraceUtil.trace3("Entering");
        String errMsg = saveSelectFSSettings(wizardEvent, isNextStep);
        TraceUtil.trace3(new NonSyncStringBuffer(
            "errMsg in processFSPage() is ").append(errMsg).toString());

        boolean success = false;

        if (errMsg != null) {
            if (errMsg.length() != 0) {
                if (errMsg.equals(Constants.Wizard.NO_ENTRY)) {
                    // user forget to enter something
                    return false;
                } else if (errMsg.equals(Constants.Wizard.SUCCESS)) {
                    success = true;
                }
            }
        }

        if (!success) {
            wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE, errMsg);
            wizardModel.setValue(Constants.Wizard.ERROR_CODE,
                Integer.toString(-2509));
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private boolean processPolicyPage(WizardEvent wizardEvent,
        boolean isNextStep) {
        TraceUtil.trace3("Entering");
        String errMsg = saveSelectPolicySettings(wizardEvent, isNextStep);

        boolean success = false;

        if (errMsg != null) {
            if (errMsg.length() != 0) {
                if (errMsg.equals(Constants.Wizard.NO_ENTRY)) {
                    // user forget to enter something
                    return false;
                } else if (errMsg.equals(Constants.Wizard.SUCCESS)) {
                    success = true;
                }
            }
        }

        if (!success) {
            wizardModel.setValue(Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(Constants.Wizard.ERROR_MESSAGE, errMsg);
            wizardModel.setValue(Constants.Wizard.ERROR_CODE,
                Integer.toString(-2509));
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private boolean processOwnerPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entering");

        method = (String)
            wizardModel.getValue(ReservationOwnerView.CHILD_RADIO_BUTTON1);

        // No "/" is allowed in any of these three textboxes
        // " " (space) is allowed in any of these three textboxes

        // 0 ==> no error
        // 1 ==> input with slash(es)
        // 2 ==> no input

        if (method == null) {
            TraceUtil.trace3("Exiting: method is null");
            setErrorMessage(wizardEvent, "Reservation.page4.errMsg1");
            return false;
        } else {
            method = method.trim();
        }

        int showError = 0;

        SamUtil.doPrint(new NonSyncStringBuffer("method is ").
            append(method).toString());

        if (method.length() == 0) {
            showError = 2;
        } else if (method.equals("ReservationOwner.radio1")) {
             // reset other two fields
            wizardModel.setValue(ReservationOwnerView.CHILD_GROUP_FIELD, "");
            wizardModel.setValue(ReservationOwnerView.CHILD_DIR_FIELD, "");

            String ownerValue = (String)
                wizardModel.getValue(ReservationOwnerView.CHILD_OWNER_FIELD);
            if (ownerValue != null && ownerValue.length() != 0) {
                if (ownerValue.trim().indexOf("/") != -1) {
                    showError = 1;
                } else if (ownerValue.trim().length() == 0) {
                    showError = 2;
                }
            } else {
                showError = 2;
            }

        } else if (method.equals("ReservationOwner.radio2")) {
            // reset two other fields
            wizardModel.setValue(ReservationOwnerView.CHILD_OWNER_FIELD, "");
            wizardModel.setValue(ReservationOwnerView.CHILD_DIR_FIELD, "");

            String groupValue = (String)
                wizardModel.getValue(ReservationOwnerView.CHILD_GROUP_FIELD);
            if (groupValue != null && groupValue.length() != 0) {
                if (groupValue.trim().indexOf("/") != -1) {
                    showError = 1;
                } else if (groupValue.trim().length() == 0) {
                    showError = 2;
                }
            } else {
                showError = 2;
            }
        } else if (method.equals("ReservationOwner.radio3")) {
            // reset two other fields
            wizardModel.setValue(ReservationOwnerView.CHILD_OWNER_FIELD, "");
            wizardModel.setValue(ReservationOwnerView.CHILD_GROUP_FIELD, "");

            String dirValue = (String)
                wizardModel.getValue(ReservationOwnerView.CHILD_DIR_FIELD);

            if (dirValue != null && dirValue.length() != 0) {
                if (dirValue.trim().indexOf("/") != -1) {
                    showError = 1;
                } else if (dirValue.trim().length() == 0) {
                    showError = 2;
                }
            } else {
                showError = 2;
            }
        }

        if (showError == 1 || showError == 2) {
            if (showError == 1) {
                setErrorMessage(wizardEvent, "Reservation.page4.errMsg2");
            } else {
                setErrorMessage(wizardEvent, "Reservation.page4.errMsg1");
            }
            TraceUtil.trace3("Exiting");
            return false;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private void setErrorMessage(WizardEvent wizardEvent, String errMsg) {
        wizardEvent.setSeverity(WizardEvent.ACKNOWLEDGE);
        wizardEvent.setErrorMessage(errMsg);
    }

    private VSN getSelectedVSN() throws SamFSException {
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        Library myLibrary = null;
        VSN myVSN    = null;

        if (libraryName == null) {
            throw new SamFSException(null, -2502);
        }

        if (libraryName.equals("<Historian>")) {
            libraryName = "Historian";
        }
        if (slotNumber == null) {
            throw new SamFSException(null, -2507);
        }
        myLibrary = sysModel.getSamQFSSystemMediaManager().
            getLibraryByName(libraryName);
        if (myLibrary == null) {
            throw new SamFSException(null, -2501);
        }

        try {
            myVSN = myLibrary.getVSN(Integer.parseInt(slotNumber));
        } catch (NumberFormatException numEx) {
            // Could not retrieve VSN
            SamUtil.doPrint(
                "NumberFormatException caught while parsing slotKey");
            throw new SamFSException(null, -2507);
        }

        if (myVSN == null) {
            throw new SamFSException(null, -2507);
        }

        return myVSN;
    }

    /**
     * updateWizardSteps()
     * This method is used to update the "page" variable of the wizard, which
     * decides what to show in the future steps, etc.
     */
    private void updateWizardSteps() {
        pages = new int[getTotalNumberOfPages()];

        pages[0] = ReservationImplData.PAGE_METHOD;

        if (fsFlag) {
            pages[1] = ReservationImplData.PAGE_FS;
        }
        if (policyFlag) {
            if (fsFlag) {
                pages[2] = ReservationImplData.PAGE_POLICY;
            } else {
                pages[1] = ReservationImplData.PAGE_POLICY;
            }
        }

        if (ownerFlag) {
            if (fsFlag && policyFlag) {
                pages[3] = ReservationImplData.PAGE_OWNER;
            } else if (fsFlag || policyFlag) {
                pages[2] = ReservationImplData.PAGE_OWNER;
            } else {
                pages[1] = ReservationImplData.PAGE_OWNER;
            }
        }

        pages[getTotalNumberOfPages() - 2] = ReservationImplData.PAGE_SUMMARY;
        pages[getTotalNumberOfPages() - 1] = ReservationImplData.PAGE_RESULT;
        initializeWizardPages(pages);
    }

    /**
     * getTotalNumberOfPages()
     * Base on the three flags, calculate the total number of pages in this
     * wizard, and return an integer.
     */
    private int getTotalNumberOfPages() {
        int total = 3; // Method page, Summary page, and Result Page
        if (fsFlag) {
            total++;
        }
        if (policyFlag) {
            total++;
        }
        if (ownerFlag) {
            total++;
        }
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "Total Number of pages in Reservation Wizard is ").append(total).
            toString());
        return total;
    }
}
