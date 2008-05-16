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

// ident	$Id: NewWizardQFSSummaryView.java,v 1.30 2008/05/16 18:38:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
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
 * A ContainerView object for the pagelet for New File System Wizard Summary
 * step (QFS only path).
 */
public class NewWizardQFSSummaryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewWizardQFSSummaryView";

    // Page 1
    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_WM_LABEL = "wmLabel";
    public static final String CHILD_FSNAME_FIELD  = "fsNameValue";
    public static final String CHILD_TYPE_FIELD = "fsTypeSelect";
    public static final String CHILD_ALLO_FIELD = "qfsSelect";
    public static final String CHILD_STRIPEG_FIELD = "stripeNumber";

    // Page 2
    // Child view names (i.e. display fields).
    public static final String CHILD_META_FIELD = "MetadataField";
    public static final String CHILD_DATA_FIELD = "DataField";

    // Page 3
    // Child view names (i.e. display fields).
    public static final String CHILD_DAU_FIELD  = "DAUDropDown";

    // Page 4
    public static final String CHILD_MOUNT_FIELD = "mountValue";
    public static final String CHILD_CREATEDIR_FIELD = "createCheckBox";
    public static final String CHILD_BOOTMOUNT_FIELD = "bootTimeCheckBox";
    public static final String CHILD_MOUNTOPT_FIELD  = "mountOptionCheckBox";
    public static final String CHILD_HWM_FIELD = "hwmValue";
    public static final String CHILD_LWM_FIELD = "lwmValue";
    public static final String CHILD_STRIPE_FIELD = "stripeValue";
    public static final String CHILD_TRACE_FIELD  = "traceDropDown";
    public static final
        String CHILD_OPTIMIZE_ORACLE_FIELD = "optimizeForOracle";

    public static final
        String CHILD_POTENTIAL_SERVER = "potentialMetadataServerTextField";
    public static final String CHILD_CLIENT = "clientTextField";
    public static final String CHILD_PRIMARYIP   = "primaryIP";
    public static final String CHILD_SECONDARYIP = "secondaryIP";

    public static final String CHILD_ALERT = "Alert";
    private boolean previous_error = false;

    protected String sharedChecked = null;
    // max size for device selection list in summary page
    public static final int DEVICE_SELECTION_LIST_MAX_SIZE = 5;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewWizardQFSSummaryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardQFSSummaryView(View parent, Model model, String name) {
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
        registerChild(CHILD_WM_LABEL, CCLabel.class);
        registerChild(CHILD_FSNAME_FIELD, CCStaticTextField.class);
        registerChild(CHILD_TYPE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_ALLO_FIELD, CCStaticTextField.class);
        registerChild(CHILD_STRIPEG_FIELD, CCStaticTextField.class);
        registerChild(CHILD_META_FIELD, CCSelectableList.class);
        registerChild(CHILD_DATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_DAU_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MOUNT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_CREATEDIR_FIELD, CCStaticTextField.class);
        registerChild(CHILD_BOOTMOUNT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MOUNTOPT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_OPTIMIZE_ORACLE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_HWM_FIELD, CCStaticTextField.class);
        registerChild(CHILD_LWM_FIELD, CCStaticTextField.class);
        registerChild(CHILD_STRIPE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_TRACE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_POTENTIAL_SERVER, CCStaticTextField.class);
        registerChild(CHILD_CLIENT, CCStaticTextField.class);
        registerChild(CHILD_PRIMARYIP, CCStaticTextField.class);
        registerChild(CHILD_SECONDARYIP, CCStaticTextField.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;
        if (name.equals(CHILD_LABEL) ||
            name.equals(CHILD_WM_LABEL)) {
            child = (View)new CCLabel(this, name, null);
        } else if (name.equals(CHILD_FSNAME_FIELD) ||
                   name.equals(CHILD_TYPE_FIELD) ||
                       name.equals(CHILD_ALLO_FIELD) ||
                   name.equals(CHILD_STRIPEG_FIELD) ||
                   name.equals(CHILD_DAU_FIELD) ||
                   name.equals(CHILD_MOUNT_FIELD) ||
                   name.equals(CHILD_CREATEDIR_FIELD) ||
                   name.equals(CHILD_BOOTMOUNT_FIELD) ||
                   name.equals(CHILD_MOUNTOPT_FIELD) ||
                   name.equals(CHILD_OPTIMIZE_ORACLE_FIELD) ||
                   name.equals(CHILD_HWM_FIELD) ||
                   name.equals(CHILD_LWM_FIELD) ||
                   name.equals(CHILD_STRIPE_FIELD) ||
                   name.equals(CHILD_TRACE_FIELD) ||
                   name.equals(CHILD_POTENTIAL_SERVER) ||
                   name.equals(CHILD_CLIENT) ||
                   name.equals(CHILD_PRIMARYIP) ||
                   name.equals(CHILD_SECONDARYIP)) {
            child = (View) new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_META_FIELD) ||
                   name.equals(CHILD_DATA_FIELD)) {
            child = (View) new CCSelectableList(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline alert = new CCAlertInline(this, name, null);
            alert.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) alert;
        } else {
            throw new IllegalArgumentException(
                "NewWizardQFSSummaryView : Invalid child name [" + name + "]");
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
        TraceUtil.trace3("Exiting");
        String url = null;

        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        sharedChecked = (String) wm.getWizardValue(
            NewWizardFSNameView.CHILD_SHARED_CHECKBOX);
        short fsLicense = SamQFSSystemModel.SAMQFS;
        Short fsLicenseValue = (Short)
            wm.getValue(Constants.Wizard.LICENSE_TYPE);
        if (fsLicenseValue != null) {
            fsLicense = fsLicenseValue.shortValue();
        }
        if (!previous_error) {
            if ((sharedChecked != null && sharedChecked.equals("true")) ||
                fsLicense == SamQFSSystemModel.QFS) {
                url = "/jsp/fs/NewWizardSharedQFSSummary.jsp";
            } else {
                url = "/jsp/fs/NewWizardQFSSummary.jsp";
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
        String serverName = (String)
            wizardModel.getValue(Constants.Wizard.SERVER_NAME);

        // populate device selection list
        String metaDataDevice = (String) wizardModel.getWizardValue(
            NewWizardQFSSummaryView.CHILD_META_FIELD);
        String[] metaDataList = metaDataDevice.split("<br>");

        OptionList metaDataOptions = new OptionList(metaDataList, metaDataList);
        CCSelectableList selectableMetadataList = ((CCSelectableList)
            getChild(NewWizardQFSSummaryView.CHILD_META_FIELD));
        selectableMetadataList.setOptions(metaDataOptions);
        int metaDataOptionSize = metaDataList.length;
        if (metaDataOptionSize <
            Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
            selectableMetadataList.setSize(metaDataOptionSize);
        } else {
            selectableMetadataList.setSize(
                Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
        }

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

        // Set Summary for Oracle FS Optimization
        String optimizeFS = (String) wizardModel.getValue(
            NewWizardMountView.CHILD_OPTIMIZE_CHECKBOX);
        if (optimizeFS != null && optimizeFS.equals("samqfsui.yes")) {
            wizardModel.setValue(CHILD_OPTIMIZE_ORACLE_FIELD, "samqfsui.yes");
        } else {
            wizardModel.setValue(CHILD_OPTIMIZE_ORACLE_FIELD, "samqfsui.no");
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

        // Assign fsType to summary page
        assignFSTypeValue(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    public boolean beginWmLabelDisplay(ChildDisplayEvent event) {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        boolean archiveEnabled = false;
        Boolean archiveEnabledValue = (Boolean) wizardModel.getValue(
            CreateFSWizardImpl.ARCHIVE_ENABLED_KEY);
        if (archiveEnabledValue != null) {
            archiveEnabled = archiveEnabledValue.booleanValue();
        }

        return archiveEnabled;
    }

    public boolean beginHwmValueDisplay(ChildDisplayEvent event) {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        boolean archiveEnabled = false;
        Boolean archiveEnabledValue = (Boolean) wizardModel.getValue(
            CreateFSWizardImpl.ARCHIVE_ENABLED_KEY);
        if (archiveEnabledValue != null) {
            archiveEnabled = archiveEnabledValue.booleanValue();
        }

        return archiveEnabled;
    }

    public boolean beginLwmValueDisplay(ChildDisplayEvent event) {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        boolean archiveEnabled = false;
        Boolean archiveEnabledValue = (Boolean) wizardModel.getValue(
            CreateFSWizardImpl.ARCHIVE_ENABLED_KEY);
        if (archiveEnabledValue != null) {
            archiveEnabled = archiveEnabledValue.booleanValue();
        }

        return archiveEnabled;
    }

    private void assignFSTypeValue(SamWizardModel wizardModel) {
        String desc = null;
        String fsTypeVal = (String) wizardModel.getValue(
            NewWizardFSNameView.CHILD_FSTYPE_RADIOBUTTON);

        if (fsTypeVal.equals("FSWizard.new.fstype.qfs")) {
            // QFS, check the type further

            boolean isArchive = Boolean.valueOf(
                (String) wizardModel.getValue(
                    NewWizardFSNameView.CHILD_ARCHIVE_CHECKBOX)).booleanValue();
            boolean isShared = Boolean.valueOf(
                (String) wizardModel.getValue(
                    NewWizardFSNameView.CHILD_SHARED_CHECKBOX)).booleanValue();

            if (isShared) {
                // shared
                desc = "filesystem.desc.qfs.server";
            } else {
                // Non-shared
                if (isArchive) {
                    desc = "filesystem.desc.qfs.archiving";
                } else {
                    desc = "filesystem.desc.qfs";
                }
            }
        } else if (fsTypeVal.equals("FSWizard.new.fstype.ufs")) {
            // UFS
            desc = "filesystem.desc.ufs";
        } else {
            // Something is wrong, fsType unknown
            desc = "filesystem.desc.unknown";
        }

        ((CCStaticTextField) getChild(CHILD_TYPE_FIELD)).
            setValue(SamUtil.getResourceString(desc));
    }
}
