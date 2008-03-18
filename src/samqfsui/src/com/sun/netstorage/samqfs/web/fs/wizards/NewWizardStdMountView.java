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

// ident	$Id: NewWizardStdMountView.java,v 1.11 2008/03/17 14:43:36 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for New File System Wizard mount
 * option step for UFS.
 *
 */
public class NewWizardStdMountView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewWizardStdMountView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_MOUNT_FIELD = "mountValue";
    public static final String CHILD_CREATE_TEXT = "createText";
    public static final String CHILD_BOOT_CHECKBOX = "bootTimeCheckBox";
    public static final String CHILD_READONLY_CHECKBOX = "readOnlyCheckBox";
    public static final String CHILD_NOSETUID_CHECKBOX = "noSetUIDCheckBox";
    public static final String
        CHILD_MOUNT_AFTER_CREATE_CHECKBOX = "mountAfterCreateCheckBox";
    public static final String CHILD_ALERT = "Alert";

    public static final String CHILD_ERROR = "errorOccur";
    private boolean previous_error = false;


    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewWizardStdMountView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardStdMountView(View parent, Model model, String name) {
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
        registerChild(CHILD_MOUNT_FIELD, CCTextField.class);
        registerChild(CHILD_CREATE_TEXT, CCStaticTextField.class);
        registerChild(CHILD_BOOT_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_READONLY_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_NOSETUID_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_MOUNT_AFTER_CREATE_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;

        if (name.equals(CHILD_LABEL)) {
            child = (View)new CCLabel(this, name, null);
        } else if (name.equals(CHILD_MOUNT_FIELD)) {
            child = (View) new CCTextField(this, name, null);
        } else if (name.equals(CHILD_CREATE_TEXT)) {
            child = (View) new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_BOOT_CHECKBOX)
            || name.equals(CHILD_READONLY_CHECKBOX)
            || name.equals(CHILD_MOUNT_AFTER_CREATE_CHECKBOX)
            || name.equals(CHILD_NOSETUID_CHECKBOX)) {
            CCCheckBox boxChild =
                new CCCheckBox
                (this, name, "samqfsui.yes", "samqfsui.no", false);
            boxChild.setBoundName(name);
            child = (View) boxChild;
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline alert = new CCAlertInline(this, name, null);
            alert.setValue(CCAlertInline.TYPE_ERROR);
            TraceUtil.trace3("Exiting");
            child = (View) alert;
        } else if (name.equals(CHILD_ERROR)) {
            CCHiddenField text = new CCHiddenField(this, name, null);
            TraceUtil.trace3("Exiting");
            child = (View) text;
        } else {
            throw new IllegalArgumentException(
                "NewWizardStdMountView : Invalid child name [" + name + "]");
        }
        TraceUtil.trace3("Exiting");
        return child;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardPage methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        String url = null;
        if (!previous_error) {
            SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
            String fsType =
                (String) wizardModel.getValue(CreateFSWizardImpl.FSTYPE_KEY);

            if (fsType.equals(CreateFSWizardImpl.FSTYPE_UFS)) {
                url = "/jsp/fs/NewWizardStdMountPage.jsp";
            } else {
                url = "/jsp/fs/wizardErrorPage.jsp";
            }
        } else {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }

        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String mountPoint = (String)wizardModel.getValue(CHILD_MOUNT_FIELD);

        // Check if the mount point exists in the model.
        // If not, this is the first time the user entered this page.
        // so populate the default values.
        if (mountPoint == null) {
            fillDefaultValues(wizardModel);
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

    /**
     * Utility method to populate the default values in this page
     */
    private void fillDefaultValues(SamWizardModel wizardModel) {
        String fsType =
            (String) wizardModel.getValue(CreateFSWizardImpl.FSTYPE_KEY);
        String serverName =
            (String)wizardModel.getValue(Constants.Wizard.SERVER_NAME);

        try {
            // if file system type is UFS,
            // std mount options are available
            if (fsType.equals(CreateFSWizardImpl.FSTYPE_UFS)) {
                // TBD: Call api to get mount options
                SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            }
        } catch (SamFSException smfex) {
            ((CCHiddenField) getChild("errorOccur")).setValue("exception");
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay()",
                "Failed to populate mount option value",
                serverName);
            SamUtil.setErrorAlert(
                this,
                NewWizardStdMountView.CHILD_ALERT,
                "FSWizard.new.error.steps",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
        TraceUtil.trace3("Exiting");
    }

    public String getErrorMsg() {
        TraceUtil.trace3("Entering");

        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String mountPoint = ((String) wm.getWizardValue("mountValue")).trim();

        boolean isValid = true;
        String msgs = new String();

        if (mountPoint.equals("")) {
            isValid = false;
            msgs = "FSWizard.new.error.mountpoint";
        } else if (!SamUtil.isValidNonSpecialCharString(mountPoint)) {
            isValid = false;
            msgs = "FSWizard.new.error.invalidmountpoint";
        // Check if the mount point given is absolute path or not
        } else if (!mountPoint.startsWith("/")) {
            isValid = false;
            msgs = "FSWizard.new.error.mountpoint.absolutePath";
        }

        TraceUtil.trace3("Exiting");
        return isValid ? null : msgs;
    }

    /** disable 'mount at boot time' if one a cluster node */
}
