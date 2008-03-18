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

// ident	$Id: NewDataClassWizardDefineCopyCountView.java,v 1.10 2008/03/17 14:43:31 am143972 Exp $

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
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for define copy count / policy name
 * of New Data Class Wizard
 *
 */
public class NewDataClassWizardDefineCopyCountView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME =
        "NewDataClassWizardDefineCopyCountView";

    // Child view names (i.e. display fields).
    public static final String MIGRATE_FROM_LABEL = "MigrateFromText";
    public static final String MIGRATE_TO_LABEL = "MigrateToText";
    public static final String MIGRATE_FROM_DROPDOWN = "MigrateFromDropDown";
    public static final String MIGRATE_TO_DROPDOWN = "MigrateToDropDown";
    public static final String ALERT = "Alert";
    public static final String TEXT = "Text";
    public static final String POLICY_NAME_LABEL = "PolicyNameText";
    public static final String POLICY_NAME = "PolicyNameTextField";
    public static final String POLICY_DESC_LABEL = "PolicyDescriptionText";
    public static final String POLICY_DESC = "PolicyDescription";
    public static final String COPIES_LABEL = "CopiesText";
    public static final String COPIES_DROPDOWN = "CopiesDropDown";

    private boolean prevErr;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewDataClassWizardDefineCopyCountView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewDataClassWizardDefineCopyCountView(
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
        registerChild(TEXT, CCStaticTextField.class);
        registerChild(POLICY_NAME_LABEL, CCLabel.class);
        registerChild(COPIES_LABEL, CCLabel.class);
        registerChild(COPIES_DROPDOWN, CCDropDownMenu.class);
        registerChild(POLICY_NAME, CCTextField.class);
        registerChild(POLICY_DESC_LABEL, CCLabel.class);
        registerChild(POLICY_DESC, CCTextField.class);
        registerChild(MIGRATE_FROM_LABEL, CCLabel.class);
        registerChild(MIGRATE_TO_LABEL, CCLabel.class);
        registerChild(MIGRATE_FROM_DROPDOWN, CCDropDownMenu.class);
        registerChild(MIGRATE_TO_DROPDOWN, CCDropDownMenu.class);
        registerChild(ALERT, CCAlertInline.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering").
            append(name).toString());

        View child;
        if (name.equals(ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(POLICY_NAME) ||
            name.equals(POLICY_DESC)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(MIGRATE_FROM_LABEL) ||
                   name.equals(MIGRATE_TO_LABEL) ||
                   name.equals(POLICY_NAME_LABEL) ||
                   name.equals(POLICY_DESC_LABEL) ||
                   name.equals(COPIES_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(MIGRATE_FROM_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList stageOptions =
                new OptionList(
                    SelectableGroupHelper.MigrateFromPool.labels,
                    SelectableGroupHelper.MigrateFromPool.values);
            myChild.setOptions(stageOptions);
            child = myChild;
        } else if (name.equals(MIGRATE_TO_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList releaseOptions =
                new OptionList(
                    SelectableGroupHelper.MigrateToPool.labels,
                    SelectableGroupHelper.MigrateToPool.values);
            myChild.setOptions(releaseOptions);
            child = myChild;
        } else if (name.equals(COPIES_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList copyNumberOptions =
                new OptionList(
                    SelectableGroupHelper.copyNumber.labels,
                    SelectableGroupHelper.copyNumber.values);
            myChild.setOptions(copyNumberOptions);
            child = myChild;
        } else {
            throw new IllegalArgumentException("Invalid child name ["
                                               + name
                                               + "]");
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
            url = "/jsp/archive/wizards/NewDataClassWizardDefineCopyCount.jsp";
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

        String errorString = (String)
            wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (errorString != null) {
            if (errorString.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
                prevErr = true;

                String msgs = (String) wizardModel.getValue(
                    Constants.Wizard.ERROR_MESSAGE);
                int code = Integer.parseInt((String) wizardModel.getValue(
                    Constants.Wizard.ERROR_CODE));
                SamUtil.setErrorAlert(this,
                                 NewDataClassWizardDefineCopyCountView.ALERT,
                                      "NewArchivePolWizard.error.carryover",
                                      code,
                                      msgs,
                                      getServerName());
            }
        }

        // set the label to error mode if necessary
        setErrorLabel(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private void setErrorLabel(SamWizardModel wizardModel) {
        String labelName = (String) wizardModel.getValue(
            NewDataClassWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName == "") {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        wizardModel.setValue(
            NewDataClassWizardImpl.VALIDATION_ERROR, "");
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
