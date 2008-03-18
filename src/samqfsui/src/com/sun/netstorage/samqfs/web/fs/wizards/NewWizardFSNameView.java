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

// ident	$Id: NewWizardFSNameView.java,v 1.41 2008/03/17 14:43:36 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for the define file system name step
 * of New File System Wizard.
 *
 */
public class NewWizardFSNameView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewWizardFSNameView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "label";

    // "qfsTypeSelect" is the common name used to group QFS and UFS radio
    // buttons together so that the browser will treat them as the two
    // options for the same radio button "qfsTypeSelect"
    public static final String CHILD_QFSTYPE_RADIOBUTTON = "qfsTypeSelect";
    public static final String CHILD_UFSTYPE_RADIOBUTTON = "ufsTypeSelect";
    public static final String
        CHILD_FSTYPE_RADIOBUTTON  = CHILD_QFSTYPE_RADIOBUTTON;

    public static final String
        CHILD_META_LOCATION_RADIOBUTTON = "metaLocationSelect";

    public static final String CHILD_ARCHIVE_CHECKBOX = "archiveCheck";
    public static final String CHILD_SHARED_CHECKBOX  = "sharedCheck";
    public static final String HAFS = "hafs";

    public static final String CHILD_ALERT = "Alert";

    // for disk discovery javascript message
    public static final String CHILD_HIDDEN_MESSAGE = "HiddenMessage";

    private boolean previous_error = false;

    // public static final String DAU_VIEW = "NewWizardQFSDAUView";

    public static final String TOGGLE_BUTTON = "ToggleButton";
    public static final String TOGGLE_BUTTON_LABELS = "ToggleButtonLabels";
    public static final String IS_ADVANCED_MODE = "isAdvancedMode";

    public static final String CHILD_ALLOC_RADIOBUTTON = "allocSelect";
    public static final String
        CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD = "numOfStripedGroupTextField";
    public static final String CHILD_DAU_SIZE_FIELD = "DAUSizeField";
    public static final String CHILD_DAU_SIZE_DROP_DOWN = "DAUSizeDropDown";
    public static final String CHILD_DAU_SIZE_HELP  = "DAUSizeHelp";
    public static final String CHILD_DAU_DROPDOWN   = "DAUDropDown";
    public static final String CHILD_STRIPE_FIELD   = "stripeValue";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewWizardFSNameView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardFSNameView(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
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
        registerChild(CHILD_QFSTYPE_RADIOBUTTON, CCRadioButton.class);
        registerChild(CHILD_UFSTYPE_RADIOBUTTON, CCRadioButton.class);
        registerChild(CHILD_META_LOCATION_RADIOBUTTON, CCRadioButton.class);
        registerChild(CHILD_ARCHIVE_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_SHARED_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_HIDDEN_MESSAGE, CCHiddenField.class);
        registerChild(HAFS, CCCheckBox.class);
        // registerChild(DAU_VIEW, NewWizardQFSDAUView.class);
        registerChild(TOGGLE_BUTTON, CCButton.class);
        registerChild(TOGGLE_BUTTON_LABELS, CCHiddenField.class);
        registerChild(CHILD_ALLOC_RADIOBUTTON, CCRadioButton.class);
        registerChild(CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_DAU_SIZE_FIELD, CCTextField.class);
        registerChild(CHILD_DAU_SIZE_DROP_DOWN, CCDropDownMenu.class);
        registerChild(CHILD_DAU_SIZE_HELP, CCStaticTextField.class);
        registerChild(CHILD_DAU_DROPDOWN, CCDropDownMenu.class);
        registerChild(IS_ADVANCED_MODE, CCHiddenField.class);
        registerChild(CHILD_STRIPE_FIELD, CCTextField.class);
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
        } else if (name.equals(CHILD_QFSTYPE_RADIOBUTTON)) {
            CCRadioButton radioChild =
                new CCRadioButton(this, CHILD_FSTYPE_RADIOBUTTON, null);
            radioChild.setOptions(
                new OptionList(
                    new String[] { "FSWizard.new.fstype.qfs" },
                    new String[] { "FSWizard.new.fstype.qfs" }));
            child = (View) radioChild;
        } else if (name.equals(CHILD_UFSTYPE_RADIOBUTTON)) {
            CCRadioButton radioChild =
                new CCRadioButton(this, CHILD_FSTYPE_RADIOBUTTON, null);
            radioChild.setOptions(
                new OptionList(
                    new String[] { "FSWizard.new.fstype.ufs" },
                    new String[] { "FSWizard.new.fstype.ufs" }));
            child = (View) radioChild;
        } else if (name.equals(CHILD_META_LOCATION_RADIOBUTTON)) {
            CCRadioButton radioChild = new CCRadioButton(this, name, null);
            radioChild.setOptions(
                new OptionList(
                    new String[] {
                        "FSWizard.new.fstype.qfs.metaSame",
                        "FSWizard.new.fstype.qfs.metaSeparate"},
                    new String[] {
                        "FSWizard.new.fstype.qfs.metaSame",
                        "FSWizard.new.fstype.qfs.metaSeparate"}));
            child = (View) radioChild;
        } else if (name.equals(CHILD_ARCHIVE_CHECKBOX) ||
                   name.equals(CHILD_SHARED_CHECKBOX) ||
                   name.equals(HAFS)) {
            CCCheckBox checkBoxChild =
                new CCCheckBox(this, name, "true", "false", false);
            child = (View) checkBoxChild;
        } else if (name.equals(TOGGLE_BUTTON)) {
            child = new CCButton(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child1 = new CCAlertInline(this, name, null);
            child1.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) child1;
        } else if (name.equals(CHILD_HIDDEN_MESSAGE)) {
            child = (View) new CCHiddenField(
                this, name, SamUtil.getResourceString("discovery.disk"));
        } else if (name.equals(TOGGLE_BUTTON_LABELS)) {
            child = (View) new CCHiddenField(
                this, name,
                SamUtil.getResourceString("common.button.advance.show").
                concat("###").concat(
                SamUtil.getResourceString("common.button.advance.hide")));
        } else if (name.equals(CHILD_ALLOC_RADIOBUTTON)) {
            CCRadioButton radioChild =
                new CCRadioButton(this, name, null);
            radioChild.setOptions(
                new OptionList(
                    new String[] {
                        "FSWizard.new.qfs.singleAllocation",
                        "FSWizard.new.qfs.dualAllocation.label",
                        "FSWizard.new.qfs.stripedGroup" },
                    new String[] {
                        "FSWizard.new.qfs.singleAllocation",
                        "FSWizard.new.qfs.dualAllocation.label",
                        "FSWizard.new.qfs.stripedGroup" }));
            child = (View) radioChild;
        } else if (name.equals(CHILD_DAU_SIZE_FIELD) ||
                   name.equals(CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD) ||
                   name.equals(CHILD_STRIPE_FIELD)) {
            child = (View) new CCTextField(this, name, null);
        } else if (name.equals(CHILD_DAU_SIZE_DROP_DOWN)) {
            OptionList dauSizeUnitDropDownOptions =
                new OptionList(
                    new String[] {
                        "FSWizard.new.dau.size.kbytes", // labels
                        "FSWizard.new.dau.size.mbytes"},
                    new String[] {
                        "kb", // values
                        "mb"});
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            myChild.setOptions(dauSizeUnitDropDownOptions);
            myChild.setBoundName(name);
            child = (View) myChild;
        } else if (name.equals(CHILD_DAU_DROPDOWN)) {
            OptionList dauOptions =
            new OptionList(
                new String[] {
                    "samqfsui.fs.wizards.new.DAUPage.option.16", // labels
                    "samqfsui.fs.wizards.new.DAUPage.option.32",
                    "samqfsui.fs.wizards.new.DAUPage.option.64"},
                new String[] {
                    "16", // values
                    "32",
                    "64"});
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            myChild.setOptions(dauOptions);
            myChild.setBoundName(name);
            child = (View) myChild;
        } else if (name.equals(CHILD_DAU_SIZE_HELP)) {
            child = (View) new CCStaticTextField(this, name, null);
        } else if (name.equals(IS_ADVANCED_MODE)) {
            child = (View) new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "NewWizardFSNameView : Invalid child name [" + name + "]");
        }
        TraceUtil.trace3("Exiting");
        return child;
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        String url = null;

        if (!previous_error) {
            url = "/jsp/fs/NewWizardFSNamePage.jsp";
        } else {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        String serverName = (String) wm.getValue(Constants.Wizard.SERVER_NAME);

        setErrorWhenNeeded(serverName, wm);

        // get samfs license type
        short fsLicense = SamQFSSystemModel.SAMQFS;
        Short fsLicenseValue = (Short)
            wm.getValue(Constants.Wizard.LICENSE_TYPE);
        if (fsLicenseValue != null) {
            fsLicense = fsLicenseValue.shortValue();
        }

        CCRadioButton qfsRadio =
            (CCRadioButton) getChild(CHILD_QFSTYPE_RADIOBUTTON);
        CCRadioButton metaLocationRadio =
            (CCRadioButton) getChild(CHILD_META_LOCATION_RADIOBUTTON);
        CCRadioButton ufsRadio =
            (CCRadioButton) getChild(CHILD_UFSTYPE_RADIOBUTTON);

        CCCheckBox archiveCheck =
            (CCCheckBox) getChild(CHILD_ARCHIVE_CHECKBOX);
        CCCheckBox sharedCheck =
            (CCCheckBox) getChild(CHILD_SHARED_CHECKBOX);

        // reset the state of radio buttons and checkboxes
        //
        // first time load the page, fsType == null
        //    by default, select qfs and separate metadata
        // 2nd time(previous button), fsType == qfs/ufs
        // if fsType == qfs, qfs section should be enabled as well
        // if fsType == ufs, qfs section should be disabled as well
        // String fsType = (String) wm.getValue(CHILD_FSTYPE_RADIOBUTTON);
        String fsType = (String) wm.getValue(CreateFSWizardImpl.FSTYPE_KEY);

        if (fsType == null) {
            // first time load this page, default to qfs type
            qfsRadio.setValue("FSWizard.new.fstype.qfs");

            if (metaLocationRadio.getValue() == null) {
                metaLocationRadio.setValue(
                    "FSWizard.new.fstype.qfs.metaSame");
                metaLocationRadio.setDisabled(false);
            }

        } else if (fsType.equals(CreateFSWizardImpl.FSTYPE_UFS)) {
            ufsRadio.setValue("FSWizard.new.fstype.ufs");
            metaLocationRadio.setDisabled(true);
            archiveCheck.setDisabled(true);
            sharedCheck.setDisabled(true);
        } else {
            // Continue to disable the metaLocation radio button group
            // if the System is a pure QFS Setup
            String hafs = (String) wm.getValue(HAFS);
            archiveCheck.setDisabled("true".equals(hafs));
            sharedCheck.setDisabled(false);
        }

        // preset allocation to single allocation is nothing has been set
        String allocation =
            (String) wm.getValue(CHILD_ALLOC_RADIOBUTTON);
        if (allocation == null) {
            allocation = "FSWizard.new.qfs.singleAllocation";
            // first time load this page, set default values
            CCRadioButton allocRadio = (CCRadioButton)
                getChild(CHILD_ALLOC_RADIOBUTTON);
            allocRadio.setValue(allocation);
        }

        TraceUtil.trace3("Exiting");
    }

    public boolean beginArchiveCheckDisplay(ChildDisplayEvent event) {
        // check if archive feature is installed
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        short fsLicense = SamQFSSystemModel.SAMQFS;
        Short fsLicenseValue = (Short)
            wm.getValue(Constants.Wizard.LICENSE_TYPE);
        if (fsLicenseValue != null) {
            fsLicense = fsLicenseValue.shortValue();
        }

        if (fsLicense == SamQFSSystemModel.QFS) {
            // standalone qfs installation, hide archive feature checkbox
            return false;
        } else {
            return true;
        }
    }

    public boolean beginSharedCheckDisplay(ChildDisplayEvent event) {
        // check if shared feature is installed
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        short fsLicense = SamQFSSystemModel.SAMQFS;
        Short fsLicenseValue = (Short)
            wm.getValue(Constants.Wizard.LICENSE_TYPE);
        if (fsLicenseValue != null) {
            fsLicense = fsLicenseValue.shortValue();
        }

        if (fsLicense == SamQFSSystemModel.SAMFS) {
            // shared feature is not available for samfs
            // hide shared feature checkbox
            return false;
        } else {
            return true;
        }
    }

    public boolean beginHafsDisplay(ChildDisplayEvent event) {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String serverName = (String)wm.getValue(Constants.Wizard.SERVER_NAME);

        boolean clusterNode = false;

        try {
            clusterNode = SamUtil.isClusterNode(serverName);
        } catch (SamFSException sfe) {
            TraceUtil.trace1(
                "Exception caught checking if server is a part of cluster!");
            TraceUtil.trace1("Reason: " + sfe.getMessage());
        }

        return clusterNode;
    }

    private void setErrorWhenNeeded(String serverName, SamWizardModel wm) {
        String t = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "FSWizard.new.error.steps";
            previous_error = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wm.getValue(Constants.Wizard.ERROR_SUMMARY);

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
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }
}
