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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: SamWizardImpl.java,v 1.20 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.iplanet.jato.ModelManager;
import com.iplanet.jato.RequestContext;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.view.ContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;
import com.sun.web.ui.model.wizard.WizardInterfaceExt;

public class SamWizardImpl implements WizardInterface, WizardInterfaceExt {

    protected SamWizardModel wizardModel = null;

    public String wizardName  = null;
    public String wizardTitle = null;
    protected final String resourceBundle  =
        "com.sun.netstorage.samqfs.web.resources.Resources";

    protected Class[] pageClass = null;
    protected int firstPage = 0;
    protected int lastPage  = 0;
    protected int resultPage = 0;
    protected int[] pages = null;

    protected String[] pageTitle = null;
    protected String[] stepText  = null;
    protected String[] stepInstruction = null;
    protected String[][] stepHelp = null;

    protected String[] cancelMsg = null;
    protected String finishMsg = null;

    // This wizard's unique ID
    protected String hostName;
    protected int wizardID;
    protected boolean wizardActive = false;

    // the following boolean can be removed after all wizards finish
    // migrating to the new wizard style (with result page)
    private boolean showResultsPage;

    // Keep track of the label name and change the label when error occurs
    public static final String VALIDATION_ERROR = "ValidationError";

    // store a reference of model manager which will be used to clean up model
    // when this wizard is finished
    protected ModelManager modelManager = null;

    protected static CCWizardWindowModel getWizardWindowModel(
        String wizardImplName,
        String wizardTitle,
        String wizardClassName,
        String fwdViewBeanName) {

        CCWizardWindowModel model = new CCWizardWindowModel();
        model.setValue(
            CCWizardWindowModelInterface.MASTHEAD_SRC,
            Constants.Wizard.MASTHEAD_SRC);
        model.setValue(
            CCWizardWindowModelInterface.MASTHEAD_ALT,
            Constants.Wizard.MASTHEAD_ALT);
        model.setValue(
            CCWizardWindowModelInterface.BASENAME,
            Constants.Wizard.BASENAME);
        model.setValue(
            CCWizardWindowModelInterface.BUNDLEID,
            Constants.Wizard.BUNDLEID);
        model.setValue(
            CCWizardWindowModelInterface.WINDOW_HEIGHT,
            Constants.Wizard.WINDOW_HEIGHT);
        model.setValue(
            CCWizardWindowModelInterface.WINDOW_WIDTH,
            Constants.Wizard.WINDOW_WIDTH);
        model.setValue(
            CCWizardWindowModelInterface.WIZARD_NAME, wizardImplName);
        model.setValue(
            CCWizardWindowModelInterface.TITLE, wizardTitle);
        model.setValue(
            CCWizardWindowModelInterface.WIZARD_CLASS_NAME,
            wizardClassName);
        model.setValue(
            CCWizardWindowModelInterface.WIZARD_REFRESH_CMDCHILD,
            fwdViewBeanName);

        return model;
    }

    protected SamWizardImpl(RequestContext requestContext, String modelName) {
        TraceUtil.initTrace();
        TraceUtil.trace2(new StringBuffer("modelName = ").
            append(modelName).toString());

        wizardModel = getWizardModel(requestContext, modelName);
        showResultsPage = false;
        initializeModel();
    }

    protected void initializeWizardControl(RequestContext requestContext) {
        hostName = (String) wizardModel.getValue(Constants.Wizard.SERVER_NAME);
        wizardID = SamWizardManager.getNewWizardID(wizardName, hostName);
        TraceUtil.trace2(new StringBuffer(
            "wizardType = ").append(wizardName).toString());
        TraceUtil.trace2(new StringBuffer(
            "hostName = ").append(hostName).toString());
        TraceUtil.trace2(new StringBuffer(
            "wizardID = ").append(wizardID).toString());
    }

    private void initializeModel() {
        wizardModel.clearWizardData();
        wizardModel.selectWizardContext();
    }

    public SamWizardModel getModel() {
        return wizardModel;
    }

    public String getPageName(String pageId) {
        return null;
    }

    public Model getPageModel(String pageId) {
        return wizardModel;
    }

    public Class getPageClass(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // check if this wizard is active
        int result =
            SamWizardManager.validateWizardID(wizardName, hostName, wizardID);
        switch (result) {
            case -1:
                wizardActive = false;
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_ERROR,
                    Constants.Wizard.WIZARD_ERROR_YES);
                wizardModel.setValue(
                    Constants.Wizard.ERROR_MESSAGE,
                    "SamWizard.wizardControl.error.message");
                wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1100");
                wizardModel.setValue(
                    Constants.Wizard.ERROR_DETAIL,
                    Constants.Wizard.ERROR_FATAL);
                wizardModel.setValue(
                    Constants.Wizard.ERROR_SUMMARY,
                    "SamWizard.wizardControl.error.summary");
                break;

            case 0:
            case 1: // ignore wizard warnings
                wizardActive = true;
                break;
        }

        return pageClass[pages[pageIdToPage(pageId)]];
    }

    public String getFirstPageId() {
        return pageToPageId(firstPage);
    }

    public String getNextPageId(String pageId) {
        TraceUtil.trace2(new StringBuffer().append(
            "Entered with pageID = ").append(pageId).toString());

        // The following if case can be removed after all wizards migrate
        // to the new style (with result page)

        if (isFinishPageId(pageId) && !showResultsPage) {
            return null;
        }

        int currentPageId = Integer.parseInt(pageId);

        return Integer.toString(currentPageId + 1);
    }

    public String getName() {
        return wizardName;
    }

    public void setName(String name) {
        wizardName = name;
    }

    public String getResourceBundle() {
        return resourceBundle;
    }

    public String getTitle() {
        return wizardTitle;
    }

    public String getStepTitle(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        return pageTitle[pages[pageIdToPage(pageId)]];
    }

    public String[] getFuturePages(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // add 1 to skip this page
        int page = pageIdToPage(pageId) + 1;

        int howMany = pages.length - page;
        String[] futurePages = new String[howMany];
        for (int i = 0; i < howMany; i++) {
            futurePages[i] = Integer.toString(page + i);
        }

        return futurePages;
    }

    public String[] getFutureSteps(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // add 1 to skip this page
        int page = pageIdToPage(pageId) + 1;
        int howMany = pages.length - page;
        String[] futureSteps = new String[howMany];

        for (int i = 0; i < howMany; i++) {
            futureSteps[i] = stepText[pages[page + i]];
        }

        return futureSteps;
    }

    public String getStepInstruction(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        return stepInstruction[pages[pageIdToPage(pageId)]];
    }

    public String getStepText(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        return stepText[pages[pageIdToPage(pageId)]];
    }

    public String[] getStepHelp(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        return stepHelp[pages[pageIdToPage(pageId)]];
    }

    public boolean isFinishPageId(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        return (lastPage == pageIdToPage(pageId));
    }

    public boolean hasPreviousPageId(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        return (firstPage != pageIdToPage(pageId));
    }

    // Events
    public boolean done(String wizardName) {
        return true;
    }

    public boolean nextStep(WizardEvent wizardEvent) {
        SamWizardManager.updateActiveWizardID(
            wizardName, hostName, wizardID, false);
        return true;
    }

    public boolean previousStep(WizardEvent wizardEvent) {
        SamWizardManager.updateActiveWizardID(
            wizardName, hostName, wizardID, false);
        return true;
    }

    public boolean gotoStep(WizardEvent wizardEvent) {
        return true;
    }

    public boolean finishStep(WizardEvent wizardEvent) {
        // check if this wizard is still active and make next wizard active
        int result =
            SamWizardManager.updateActiveWizardID(
                wizardName, hostName, wizardID, true);
        switch (result) {
            case -1:
                wizardActive = false;
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "SamWizard.wizardControl.error.summary");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "SamWizard.wizardControl.error.message");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE,
                    "1100");
                break;

            case 0:
                wizardActive = true;
                break;

            case 1:
                // should never get here!!!
                wizardActive = true;
                break;
        }

        return wizardActive;
    }

    public boolean cancelStep(WizardEvent wizardEvent) {
        // This statement is commented out because, the wizard implementation
        // is calling the beginDisplay() of the last page that the user is on
        // and beginDisplay usually needs some data from the wizardModel.
        // If the issue is not fixed in the Wizard implementation,
        // leave this line commented.

        // wizardModel.clearWizardData();

        // if this is active wizard, make next one active
        SamWizardManager.updateActiveWizardID(
            wizardName, hostName, wizardID, true);

        return true;
    }

    public String getCancelPrompt(String pageId) {
        return SamUtil.getResourceString("common.wizard.exitmessage");
    }

    public String getFinishPrompt(String pageId) {
        return finishMsg;
    }

    protected String pageToPageId(int page) {
        return Integer.toString(page + 1);
    }

    protected int pageIdToPage(String pageId) {
        return Integer.parseInt(pageId) - 1;
    }

    public String toString() {
        return wizardName;
    }

    /**
     * Don't display an alert
     * when the user revisits a previously seen page.
     */
    public boolean warnOnRevisitStep() {
        return false;
    }

    protected void initializeWizardPages(int[] inPages) {
        if (inPages.length < 1) {
            return;
        }

        firstPage  = 0;

        // The following needs to be fixed after all wizards are migrated
        // to new style (with result page)
        if (showResultsPage) {
            lastPage   = inPages.length - 2;
            resultPage = inPages.length - 1;
        } else {
            lastPage   = inPages.length - 1;
        }
    }

    private SamWizardModel getWizardModel(
        RequestContext requestContext,
        String modelName) {

        String modelInstanceName =
            requestContext.getRequest().getParameter(modelName);
        if (modelInstanceName == null) {
            TraceUtil.trace2("modelInstanceName not found in request");
            modelInstanceName = modelName;
        }
        TraceUtil.trace2(new StringBuffer("Got modelInstanceName = ").
            append(modelInstanceName).toString());
        modelManager = requestContext.getModelManager();
        wizardModel =
            (SamWizardModel) modelManager.getModel(
                SamWizardModel.class,
                modelInstanceName,
                true,
                true);

        return wizardModel;
    }

    public boolean canBeStepLink(String str) {
        return true;
    }

    public void closeStep(WizardEvent wizardEvent) {
    }

    public String getPlaceholderText(String str) {
        return null;
    }

    public boolean helpTab(WizardEvent wizardEvent) {
        return true;
    }

    public boolean isSubstep(String str) {
        return false;
    }

    public boolean stepTab(WizardEvent wizardEvent) {
        return true;
    }

    public String getResultsPageId(String pageId) {
        TraceUtil.trace2(new StringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // Simply return pageToPageId(resultPage) after all wizards migrate
        // to the new style (with result page)
        if (showResultsPage) {
            return pageToPageId(resultPage);
        } else {
            return null;
        }
    }

    /**
     * The following method can be removed after all wizards migrate to
     * new style (with result page)
     */
    protected void setShowResultsPage(boolean showResultsPage) {
        this.showResultsPage = showResultsPage;
    }

    // Returns true if an error was found, false if no error found.
    public static boolean handleWizardError(ContainerView view,
                                            SamWizardModel wizardModel,
                                            String alertChildName,
                                            String errorSummary) {

        boolean hadError = false;
        String t = (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code;
            try {
                code = Integer.parseInt(
                    (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
            } catch (NumberFormatException e) {
                // Dummy code
                code = 8001234;
            }
            hadError = true;
            String errorDetails =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wizardModel.getValue(Constants.Wizard.ERROR_SUMMARY);
                hadError =
                    !errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT);
            }

            if (hadError) {
                SamUtil.setErrorAlert(
                    view,
                    alertChildName,
                    errorSummary,
                    code,
                    msgs,
                    (String) wizardModel.getValue(
                            Constants.Wizard.SERVER_NAME));
            } else {
                SamUtil.setWarningAlert(
                    view,
                    alertChildName,
                    errorSummary,
                    msgs);
            }
        }

        return hadError;
    }
}
