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

// ident	$Id: NewWizardFSStdSummaryView.java,v 1.9 2008/05/16 18:38:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for New File System wizard summary
 * page for UFS path.
 *
 */
public class NewWizardFSStdSummaryView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewWizardFSStdSummaryView";

    // Page 1
    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_TYPE_FIELD = "fsTypeSelect";

    // Page 2
    // Child view names (i.e. display fields).
    public static final String CHILD_DATA_FIELD = "DataField";

    // Page3
    public static final String CHILD_MOUNT_FIELD = "mountValue";
    public static final String CHILD_CREATEDIR_FIELD = "createCheckBox";
    public static final String CHILD_BOOTMOUNT_FIELD = "bootTimeCheckBox";
    public static final String CHILD_READONLY_FIELD = "readOnlyCheckBox";
    public static final String CHILD_NOSETUID_FIELD = "noSetUIDCheckBox";
    public static final String CHILD_MOUNTAFTERCREATE_FIELD =
        "mountAfterCreateCheckBox";

    // page4
    public static final String CHILD_SYNTAX_FIELD = "syntaxSelectRadioButton";
    public static final String CHILD_PATH = "locationTextField";

    public static final String CHILD_ALERT = "Alert";
    private boolean previous_error = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewWizardFSStdSummaryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardFSStdSummaryView(View parent, Model model, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        setDefaultModel(model);

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Child manipulation methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_TYPE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_DATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_MOUNT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_CREATEDIR_FIELD, CCStaticTextField.class);
        registerChild(CHILD_BOOTMOUNT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_READONLY_FIELD, CCStaticTextField.class);
        registerChild(CHILD_NOSETUID_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MOUNTAFTERCREATE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_SYNTAX_FIELD, CCStaticTextField.class);
        registerChild(CHILD_PATH, CCStaticTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;
        if (name.equals(CHILD_LABEL)) {
            child = (View) new CCLabel(this, name, null);
        } else if (name.equals(CHILD_TYPE_FIELD) ||
                   name.equals(CHILD_MOUNT_FIELD) ||
                   name.equals(CHILD_CREATEDIR_FIELD) ||
                   name.equals(CHILD_BOOTMOUNT_FIELD) ||
                   name.equals(CHILD_READONLY_FIELD) ||
                   name.equals(CHILD_NOSETUID_FIELD) ||
                   name.equals(CHILD_MOUNTAFTERCREATE_FIELD) ||
                   name.equals(CHILD_SYNTAX_FIELD) ||
                   name.equals(CHILD_PATH)) {
            child = (View) new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_DATA_FIELD)) {
            child = (View) new CCSelectableList(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline alert = new CCAlertInline(this, name, null);
            alert.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) alert;
        } else {
            throw new IllegalArgumentException(
                "NewWizardFSStdSummaryView : Invalid child name [" +
                name + "]");
        }
        TraceUtil.trace3("Exiting");
        return child;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardBody methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        String url = "/jsp/fs/NewWizardFSStdSummary.jsp";
        if (previous_error) {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String dataDevice = (String) wizardModel.getWizardValue(
            NewWizardQFSSummaryView.CHILD_DATA_FIELD);
        String[] dataList = dataDevice.split("<br>");
        OptionList dataOptions = new OptionList(dataList, dataList);
        CCSelectableList selectDataOptions = ((CCSelectableList)
            getChild(NewWizardQFSSummaryView.CHILD_DATA_FIELD));
        selectDataOptions.setOptions(dataOptions);
        int dataOptionsSize = dataList.length;
        if (dataOptionsSize < Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
            selectDataOptions.setSize(dataOptionsSize);
        } else {
            selectDataOptions.setSize(
                Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
        }

        String t = (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "FSWizard.new.error.steps";
            previous_error = true;
            String errorDetails =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wizardModel.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previous_error = false;
                } else {
                    previous_error = true;
                }
            }

            if (previous_error) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    (String) wizardModel.getValue(
                                Constants.Wizard.SERVER_NAME));
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
        TraceUtil.trace3("Exiting");
    }
}
