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

// ident	$Id: NewWizardMountView.java,v 1.38 2008/12/16 00:12:12 am143972 Exp $

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
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for New File System Wizard mount
 * option step.
 */
public class NewWizardMountView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewWizardMountView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_MOUNT_FIELD = "mountValue";
    public static final String CHILD_HWM_FIELD = "hwmValue";
    public static final String CHILD_LWM_FIELD = "lwmValue";
    public static final String CHILD_TRACE_DROPDOWN  = "traceDropDown";
    public static final String CHILD_CREATE_TEXT = "createText";
    public static final String CHILD_BOOT_CHECKBOX = "bootTimeCheckBox";
    public static final String CHILD_MOUNT_CHECKBOX = "mountOptionCheckBox";
    public static final String CHILD_OPTIMIZE_CHECKBOX = "optimizeCheckBox";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_FSNAME_LABEL = "fsNameLabel";
    public static final String CHILD_FSNAME_FIELD = "fsNameValue";

    public static final String CHILD_ERROR = "errorOccur";
    private boolean previous_error = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewWizardMountView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardMountView(View parent, Model model, String name) {
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
        registerChild(CHILD_HWM_FIELD, CCTextField.class);
        registerChild(CHILD_LWM_FIELD, CCTextField.class);
        registerChild(CHILD_TRACE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_CREATE_TEXT, CCStaticTextField.class);
        registerChild(CHILD_BOOT_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_MOUNT_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_OPTIMIZE_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(CHILD_FSNAME_LABEL, CCLabel.class);
        registerChild(CHILD_FSNAME_FIELD, CCTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;

        if (name.equals(CHILD_LABEL) ||
            name.equals(CHILD_FSNAME_LABEL)) {
            child = (View)new CCLabel(this, name, null);
        } else if (name.equals(CHILD_HWM_FIELD) ||
                   name.equals(CHILD_LWM_FIELD) ||
                   name.equals(CHILD_MOUNT_FIELD) ||
                   name.equals(CHILD_FSNAME_FIELD)) {
            child = (View) new CCTextField(this, name, null);
        } else if (name.equals(CHILD_CREATE_TEXT)) {
            child = (View) new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_BOOT_CHECKBOX) ||
            name.equals(CHILD_OPTIMIZE_CHECKBOX)) {
            CCCheckBox boxChild =
                new CCCheckBox
                (this, name, "samqfsui.yes", "samqfsui.no", false);
            boxChild.setBoundName(name);
            child = (View) boxChild;
        } else if (name.equals(CHILD_MOUNT_CHECKBOX)) {
            CCCheckBox boxChild =
                new CCCheckBox
                (this, name, "samqfsui.yes", "samqfsui.no", true);
            boxChild.setBoundName(name);
            child = (View) boxChild;
        } else if (name.equals(CHILD_TRACE_DROPDOWN)) {
            CCDropDownMenu dropdownChild =
                new CCDropDownMenu(this, name, null);
            OptionList traceOptions =
                new OptionList(
                    new String[] {
                        "FSWizard.new.traceoption1", // labels
                        "FSWizard.new.traceoption2"},
                    new String[] {
                        "samqfsui.yes", // values
                        "samqfsui.no"});
            dropdownChild.setOptions(traceOptions);
            dropdownChild.setBoundName(name);
            child = (View) dropdownChild;
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
                "NewWizardMountView : Invalid child name [" + name + "]");
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
            boolean archiveEnabled = false;
            Boolean archiveEnabledValue = (Boolean) wizardModel.getValue(
                CreateFSWizardImpl.ARCHIVE_ENABLED_KEY);
            if (archiveEnabledValue != null) {
                archiveEnabled = archiveEnabledValue.booleanValue();
            }
            short fsLicense = SamQFSSystemModel.SAMQFS;
            Short fsLicenseValue = (Short)
                wizardModel.getValue(Constants.Wizard.LICENSE_TYPE);
            if (fsLicenseValue != null) {
                fsLicense = fsLicenseValue.shortValue();
            }
            TraceUtil.trace3("archiveEnabled = " + archiveEnabled);
            TraceUtil.trace3("fsLicense = " + fsLicense);
            if (fsLicense == SamQFSSystemModel.QFS ||
                !archiveEnabled) {
                url = "/jsp/fs/NewWizardQFSMountPage.jsp";
            } else {
                url = "/jsp/fs/NewWizardMountPage.jsp";
            }
        } else {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        return url;
    }

    /**
     * Hide Optimize Check Box if Metadata and data on the same device
     */
    public boolean beginOptimizeCheckBoxDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        boolean result = true;

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String mdStorage = (String)
            wizardModel.getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
        Boolean hpc =
            (Boolean)wizardModel.getValue(CreateFSWizardImpl.POPUP_HPC);
        Boolean matfs =
            (Boolean)wizardModel.getValue(CreateFSWizardImpl.POPUP_MATFS);

        if (NewWizardMetadataOptionsView.SAME_DEVICE.equals(mdStorage)) {
            // if (metaLocationVal.equals("FSWizard.new.fstype.qfs.metaSame")) {
            // Reset this value to no in case user goes back and forth in
            // the wizard.  This call will ensure the qwrite and force-directio
            // do not get enabled accidentally
            wizardModel.setValue(CHILD_OPTIMIZE_CHECKBOX, "samqfsui.no");
            result = false;
        }

        // hide the optimize for oracle checkbox if creating an hpc (mb) or a
        // mat file system.
        if (hpc.booleanValue() || matfs.booleanValue()) {
            result = false;
        }

        return result;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String mountPoint = (String)wizardModel.getValue(CHILD_MOUNT_FIELD);
        String serverName =
          (String)wizardModel.getValue(Constants.Wizard.SERVER_NAME);


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
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }

        // if creating a HAFS, update the help text accordingly and set the
        // mount point to begin with /global/<fsname>
        if (isHAFS()) {
            ((CCStaticTextField)getChild(CHILD_CREATE_TEXT)).
                setValue("FSWizard.new.mount.hafs");
            String temp = (String)wizardModel.getValue(CHILD_MOUNT_FIELD);
            mountPoint = temp == null ?
                "/global/".concat(
                    SamUtil.getResourceString("FSWizard.new.insertfsname")) :
                temp;
            wizardModel.setValue(CHILD_MOUNT_FIELD, mountPoint);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Utility method to populate the default values in this page
     */
    private void fillDefaultValues(SamWizardModel wizardModel) {
        String fsType =
            (String) wizardModel.getValue(CreateFSWizardImpl.FSTYPE_KEY);
        // String dauSize = (String)
        //    wizardModel.getValue(NewWizardFSNameView.CHILD_DAU_DROPDOWN);
        // int dau = Integer.parseInt(dauSize);

        // String blockSize = (String)
        //    wizardModel.getValue(NewWizardBlockAllocationView.BLOCK_SIZE);
        // int dau = Integer.parseInt(blockSize);

        // TODO: retrieve from wizard model
        int dau = 64;

        String serverName =
          (String)wizardModel.getValue(Constants.Wizard.SERVER_NAME);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            int type = -1, archType = -1;
            int shareStatus = FileSystem.UNSHARED;
            if (fsType.equals(CreateFSWizardImpl.FSTYPE_QFS)) {
                type = FileSystem.SEPARATE_METADATA;
                archType = FileSystem.NONARCHIVING;
            } else if (fsType.equals(CreateFSWizardImpl.FSTYPE_FS)) {
                type = FileSystem.COMBINED_METADATA;
                archType = FileSystem.ARCHIVING;
            }

            FileSystemMountProperties properties =
                sysModel.getSamQFSSystemFSManager().
                    getDefaultMountProperties(
                    type,
                    archType,
                    dau,
                    false,
                    shareStatus,
                    false);

            String traceString =
                properties.isTrace() ?
                    "samqfsui.yes" : "samqfsui.no";

            String hwmString = Integer.toString(properties.getHWM());
            String lwmString = Integer.toString(properties.getLWM());
            wizardModel.setValue(CHILD_HWM_FIELD, hwmString);
            wizardModel.setValue(CHILD_LWM_FIELD, lwmString);

            wizardModel.setValue(CHILD_TRACE_DROPDOWN, traceString);
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
                NewWizardMountView.CHILD_ALERT,
                "FSWizard.new.error.steps",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
        TraceUtil.trace3("Exiting");
    }

    // disable mount at boot time if creating an hafs
    public boolean beginBootTimeCheckBoxDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return !isHAFS();
    }

    private boolean isHAFS() {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        Boolean hafs = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_HAFS);

        return hafs;
    }
}
