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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: NewWizardFSSummaryView.java,v 1.30 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
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
 * A ContainerView object for the pagelet for New File System Wizard summary
 * step (SAM-FS/SAM-QFS path).
 *
 */
public class NewWizardFSSummaryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewWizardFSSummaryView";

    // Page 1
    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_FSNAME_FIELD = "fsNameValue";
    public static final String CHILD_TYPE_FIELD = "fsTypeSelect";

    // Page 2
    // Child view names (i.e. display fields).
    public static final String CHILD_DATA_FIELD = "DataField";

    // Page3
    // Child view names (i.e. display fields).
    public static final String CHILD_DAU_FIELD = "DAUDropDown";

    // Page4
    public static final String CHILD_MOUNT_FIELD = "mountValue";
    public static final String CHILD_CREATEDIR_FIELD = "createCheckBox";
    public static final String CHILD_BOOTMOUNT_FIELD = "bootTimeCheckBox";
    public static final String CHILD_MOUNTOPT_FIELD  = "mountOptionCheckBox";
    public static final String CHILD_HWM_FIELD = "hwmValue";
    public static final String CHILD_LWM_FIELD = "lwmValue";
    public static final String CHILD_STRIPE_FIELD = "stripeValue";
    public static final String CHILD_TRACE_FIELD = "traceDropDown";

    public static final
        String CHILD_POTENTIAL_SERVER = "potentialMetadataServerTextField";
    public static final String CHILD_CLIENT = "clientTextField";
    public static final String CHILD_PRIMARYIP = "primaryIP";
    public static final String CHILD_SECONDARYIP = "secondaryIP";

    // cluster stuff
    public static final String NODES_LABEL = "selectedNodesLabel";
    public static final String NODES = "selectedNodes";
    public static final String BOOT_TIME_LABEL = "bootTimeLabel";

    public static final String CHILD_ALERT = "Alert";

    private boolean previous_error = false;
    // private String sharedChecked = null;
    private boolean sharedEnabled = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewWizardFSSummaryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardFSSummaryView(View parent, Model model, String name) {
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
        registerChild(CHILD_FSNAME_FIELD, CCStaticTextField.class);
        registerChild(CHILD_TYPE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_DATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_DAU_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MOUNT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_CREATEDIR_FIELD, CCStaticTextField.class);
        registerChild(CHILD_BOOTMOUNT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MOUNTOPT_FIELD, CCStaticTextField.class);
        registerChild(CHILD_HWM_FIELD, CCStaticTextField.class);
        registerChild(CHILD_LWM_FIELD, CCStaticTextField.class);
        registerChild(CHILD_STRIPE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_TRACE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_POTENTIAL_SERVER, CCStaticTextField.class);
        registerChild(CHILD_CLIENT, CCStaticTextField.class);
        registerChild(CHILD_PRIMARYIP, CCStaticTextField.class);
        registerChild(CHILD_SECONDARYIP, CCStaticTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(BOOT_TIME_LABEL, CCLabel.class);
        registerChild(NODES_LABEL, CCLabel.class);
        registerChild(NODES, CCSelectableList.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;
        if (name.equals(CHILD_LABEL) ||
            name.equals(BOOT_TIME_LABEL) ||
            name.equals(NODES_LABEL)) {
            child = (View)new CCLabel(this, name, null);
        } else if (name.equals(CHILD_FSNAME_FIELD) ||
                   name.equals(CHILD_TYPE_FIELD) ||
                   name.equals(CHILD_DAU_FIELD) ||
                   name.equals(CHILD_MOUNT_FIELD) ||
                   name.equals(CHILD_CREATEDIR_FIELD) ||
                   name.equals(CHILD_BOOTMOUNT_FIELD) ||
                   name.equals(CHILD_MOUNTOPT_FIELD) ||
                   name.equals(CHILD_HWM_FIELD) ||
                   name.equals(CHILD_LWM_FIELD) ||
                   name.equals(CHILD_STRIPE_FIELD) ||
                   name.equals(CHILD_TRACE_FIELD) ||
                   name.equals(CHILD_POTENTIAL_SERVER) ||
                   name.equals(CHILD_CLIENT) ||
                   name.equals(CHILD_PRIMARYIP) ||
                   name.equals(CHILD_SECONDARYIP)) {
            child = (View)new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_DATA_FIELD) ||
                   name.equals(NODES)) {
            child = (View) new CCSelectableList(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline alert = new CCAlertInline(this, name, null);
            alert.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) alert;
        } else {
            throw new IllegalArgumentException(
                "NewWizardFSSummaryView : Invalid child name [" + name + "]");
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
        Boolean temp = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_SHARED);
        sharedEnabled = temp.booleanValue();

        /*
        sharedChecked = (String) wm.getWizardValue(
            NewWizardFSNameView.CHILD_SHARED_CHECKBOX);
        */
        if (!previous_error) {
            // if (sharedChecked != null && sharedChecked.equals("true")) {
            if (sharedEnabled) {
                url = "/jsp/fs/NewWizardSharedFSSummary.jsp";
            } else {
                url = "/jsp/fs/NewWizardFSSummary.jsp";
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

        Boolean temp =
            (Boolean)wizardModel.getValue(CreateFSWizardImpl.POPUP_SHARED);
        sharedEnabled = temp.booleanValue();

        /*
        sharedChecked = (String) wizardModel.getWizardValue(
            NewWizardFSNameView.CHILD_SHARED_CHECKBOX);
        */

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
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }

        // Assign fsType to CHILD_TYPE_FIELD
        assignFSTypeValue(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private void assignFSTypeValue(SamWizardModel wm) {
        String desc = null;
        String type = (String)wm.getValue(CreateFSWizardImpl.FSTYPE_KEY);
        if (CreateFSWizardImpl.FSTYPE_QFS.equals(type)) {
            // QFS, check the type further
            Boolean archiveEnabled =
                (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_ARCHIVING);
            Boolean sharedEnabled =
                (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_SHARED);

            if (sharedEnabled.booleanValue()) { // shared
                desc = "filesystem.desc.qfs.server";
            } else {
                // Non-shared
                if (archiveEnabled.booleanValue()) {
                    desc = "filesystem.desc.qfs.archiving";
                } else {
                    desc = "filesystem.desc.qfs";
                }
            }
        } else if (CreateFSWizardImpl.FSTYPE_UFS.equals(type)) {
            // UFS
            desc = "filesystem.desc.ufs";
        } else {
            // Something is wrong, fsType unknown
            desc = "filesystem.desc.unknown";
        }

        ((CCStaticTextField) getChild(CHILD_TYPE_FIELD)).
            setValue(SamUtil.getResourceString(desc));
    }

    public boolean beginSelectedNodesLabelDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isNonSharedHAFS();
    }

    public boolean beginSelectedNodesDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isNonSharedHAFS();
    }

    public boolean beginBootTimeLabelDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return !isNonSharedHAFS();
    }

    public boolean beginBootTimeCheckBoxDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return !isNonSharedHAFS();
    }

    /** determine if we are creating a non-shared hafs */
    public boolean isNonSharedHAFS() {
        String key = "is_non_shared_hafs_key";

        Boolean value = (Boolean)RequestManager.getRequest().getAttribute(key);
        if (value == null) {
            SamWizardModel wm = (SamWizardModel)getDefaultModel();

            Boolean hafs = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_HAFS);
            Boolean shared =
                (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_SHARED);

            value = new Boolean(hafs.booleanValue() && !shared.booleanValue());
            RequestManager.getRequest().setAttribute(key, value);
        }

        return value.booleanValue();
    }
}
