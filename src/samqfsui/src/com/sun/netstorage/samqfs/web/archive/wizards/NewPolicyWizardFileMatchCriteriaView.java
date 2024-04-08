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

// ident	$Id: NewPolicyWizardFileMatchCriteriaView.java,v 1.14 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for File Match Criteria Page.
 *
 */
public class NewPolicyWizardFileMatchCriteriaView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME =
        "NewPolicyWizardFileMatchCriteriaView";

    // Child view names (i.e. display fields).
    public static final String CHILD_STAGE_ATTR_TEXT = "StageAttText";
    public static final String CHILD_RELEASE_ATTR_TEXT = "ReleaseAttText";
    public static final String CHILD_STAGE_DROPDOWN = "StageDropDown";
    public static final String CHILD_RELEASE_DROPDOWN = "ReleaseDropDown";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_POL_NAME_TEXT = "PolicyNameText";
    public static final String CHILD_POL_NAME_TEXTFIELD = "PolicyNameTextField";
    public static final String CHILD_COPIES_TEXT = "CopiesText";
    public static final String CHILD_COPIES_DROPDOWN = "CopiesDropDown";

    private boolean prevErr;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewPolicyWizardFileMatchCriteriaView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewPolicyWizardFileMatchCriteriaView(
        View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_POL_NAME_TEXT, CCLabel.class);
        registerChild(CHILD_COPIES_TEXT, CCLabel.class);
        registerChild(CHILD_COPIES_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_POL_NAME_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_STAGE_ATTR_TEXT, CCLabel.class);
        registerChild(CHILD_RELEASE_ATTR_TEXT, CCLabel.class);
        registerChild(CHILD_STAGE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_RELEASE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering").
            append(name).toString());

        View child;
        if (name.equals(CHILD_ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_POL_NAME_TEXTFIELD)) {
            child = new CCTextField(this, name, null);
        } else if (
            name.equals(CHILD_STAGE_ATTR_TEXT) ||
            name.equals(CHILD_RELEASE_ATTR_TEXT) ||
            name.equals(CHILD_POL_NAME_TEXT) ||
            name.equals(CHILD_COPIES_TEXT)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_STAGE_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList stageOptions =
                new OptionList(
                    SelectableGroupHelper.StagingForWizard.labels,
                    SelectableGroupHelper.StagingForWizard.values);
            myChild.setOptions(stageOptions);
            child = myChild;
        } else if (name.equals(CHILD_RELEASE_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList releaseOptions =
                new OptionList(
                    SelectableGroupHelper.ReleasingForWizard.labels,
                    SelectableGroupHelper.ReleasingForWizard.values);
            myChild.setOptions(releaseOptions);
            child = myChild;
        } else if (name.equals(CHILD_COPIES_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList copyNumberOptions =
                new OptionList(
                    SelectableGroupHelper.copyNumber.labels,
                    SelectableGroupHelper.copyNumber.values);
            myChild.setOptions(copyNumberOptions);
            child = myChild;
        } else {
            throw new IllegalArgumentException(
                "NewPolicyWizardFileMatchCriteriaView : Invalid child name [" +
                    name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String url = null;
        if (!prevErr) {
            url = "/jsp/archive/wizards/NewPolicyWizardFileMatchCriteria.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }

        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // show the yellow warning message if another wizard instance is found
        showWizardValidationMessage(wizardModel);

        // Set label to red if error is detected
        setErrorLabel(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private void showWizardValidationMessage(SamWizardModel wizardModel) {
        String errorMessage =
            (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);

        if (errorMessage != null &&
            errorMessage.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            String errorDetails =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_DETAIL);
            String errorSummary =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_SUMMARY);

            if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }

    private void setErrorLabel(SamWizardModel wizardModel) {
        String labelName = (String) wizardModel.getValue(
            NewPolicyWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName == "") {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        wizardModel.setValue(
            NewPolicyWizardImpl.VALIDATION_ERROR, "");
    }
}
