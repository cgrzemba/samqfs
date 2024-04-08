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

// ident	$Id: NewPolicyWizardTapeCopyOptionView.java,v 1.16 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.HashMap;

/**
 * A ContainerView object for the pagelet for Tape Copy Option Parameters.
 *
 */
public class NewPolicyWizardTapeCopyOptionView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewPolicyWizardTapeCopyOptionView";

    // Child view names (i.e. display fields).
    public static final String CHILD_OFFLINE_COPY_TEXT = "OfflineCopyText";
    public static final String CHILD_OFFLINE_COPY_DROPDOWN =
        "OfflineCopyDropDown";
    public static final String CHILD_DRIVES_TEXT = "DrivesText";
    public static final String CHILD_DRIVES_TEXTFIELD = "DrivesTextField";
    public static final String CHILD_DRIVES_MIN_TEXT = "DrivesMinText";
    public static final String CHILD_DRIVES_MIN_TEXTFIELD =
        "DrivesMinTextField";
    public static final String CHILD_DRIVES_MIN_DROPDOWN =
        "DrivesMinSizeDropDown";
    public static final String CHILD_DRIVES_MAX_TEXT = "DrivesMaxText";
    public static final String CHILD_DRIVES_MAX_TEXTFIELD =
        "DrivesMaxTextField";
    public static final String CHILD_DRIVES_MAX_DROPDOWN =
        "DrivesMaxSizeDropDown";
    public static final String CHILD_START_AGE_TEXT = "StartAgeText";
    public static final String CHILD_START_AGE_TEXTFIELD = "StartAgeTextField";
    public static final String CHILD_START_AGE_DROPDOWN = "StartAgeDropDown";
    public static final String CHILD_START_COUNT_TEXT = "StartCountText";
    public static final String CHILD_START_COUNT_TEXTFIELD =
        "StartCountTextField";
    public static final String CHILD_START_SIZE_TEXT = "StartSizeText";
    public static final String CHILD_START_SIZE_TEXTFIELD =
        "StartSizeTextField";
    public static final String CHILD_START_SIZE_DROPDOWN = "StartSizeDropDown";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";

    // Page Title Attributes and Components.
    private static CCPageTitleModel pageTitleModel = null;

    private boolean prevErr;
    private boolean error;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewPolicyWizardTapeCopyOptionView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewPolicyWizardTapeCopyOptionView(
        View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_OFFLINE_COPY_TEXT, CCLabel.class);
        registerChild(CHILD_OFFLINE_COPY_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_DRIVES_TEXT, CCLabel.class);
        registerChild(CHILD_DRIVES_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_DRIVES_MIN_TEXT, CCLabel.class);
        registerChild(CHILD_DRIVES_MIN_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_DRIVES_MIN_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_DRIVES_MAX_TEXT, CCLabel.class);
        registerChild(CHILD_DRIVES_MAX_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_DRIVES_MAX_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_START_AGE_TEXT, CCLabel.class);
        registerChild(CHILD_START_AGE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_START_AGE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_START_COUNT_TEXT, CCLabel.class);
        registerChild(CHILD_START_COUNT_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_START_SIZE_TEXT, CCLabel.class);
        registerChild(CHILD_START_SIZE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_START_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(CHILD_ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_ERROR)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_OFFLINE_COPY_TEXT) ||
            name.equals(CHILD_DRIVES_MIN_TEXT) ||
            name.equals(CHILD_DRIVES_MAX_TEXT) ||
            name.equals(CHILD_DRIVES_TEXT) ||
            name.equals(CHILD_START_AGE_TEXT) ||
            name.equals(CHILD_START_COUNT_TEXT) ||
            name.equals(CHILD_START_SIZE_TEXT)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_OFFLINE_COPY_DROPDOWN)) {
            CCDropDownMenu child = new CCDropDownMenu(this, name, null);
            OptionList offlineCopyOptions =
                new OptionList(
                    SelectableGroupHelper.OfflineCopyMethod.labels,
                    SelectableGroupHelper.OfflineCopyMethod.values);
            child.setOptions(offlineCopyOptions);
            return child;
        } else if (name.equals(CHILD_DRIVES_TEXTFIELD) ||
            name.equals(CHILD_DRIVES_MIN_TEXTFIELD) ||
	    name.equals(CHILD_DRIVES_MAX_TEXTFIELD) ||
            name.equals(CHILD_START_AGE_TEXTFIELD) ||
            name.equals(CHILD_START_COUNT_TEXTFIELD) ||
            name.equals(CHILD_START_SIZE_TEXTFIELD)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(CHILD_START_AGE_DROPDOWN)) {
            CCDropDownMenu child =  new CCDropDownMenu(this, name, null);
            OptionList ageOptions =
                new OptionList(
                    SelectableGroupHelper.Time.labels,
                    SelectableGroupHelper.Time.values);
            child.setOptions(ageOptions);
            return child;
        } else if (name.equals(CHILD_START_SIZE_DROPDOWN)) {
            CCDropDownMenu child =  new CCDropDownMenu(this, name, null);
            OptionList sizeOptions =
                new OptionList(
                    SelectableGroupHelper.Size.labels,
                    SelectableGroupHelper.Size.values);
            child.setOptions(sizeOptions);
            return child;
        } else if (name.equals(CHILD_DRIVES_MIN_DROPDOWN)) {
            CCDropDownMenu child =  new CCDropDownMenu(this, name, null);
            OptionList sizeOptions =
                new OptionList(
                    SelectableGroupHelper.Size.labels,
                    SelectableGroupHelper.Size.values);
            child.setOptions(sizeOptions);
            return child;
        } else if (name.equals(CHILD_DRIVES_MAX_DROPDOWN)) {
            CCDropDownMenu child =  new CCDropDownMenu(this, name, null);
            OptionList sizeOptions =
                new OptionList(
                    SelectableGroupHelper.Size.labels,
                    SelectableGroupHelper.Size.values);
            child.setOptions(sizeOptions);
            return child;
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
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
            url = "/jsp/archive/wizards/NewPolicyWizardTapeCopyOption.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        if (!showPreviousError(wizardModel)) {
            try {
                prePopulateFields(wizardModel);
            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "beginDisplay",
                    "Failed to pre-populate input fields",
                    getServerName());
                SamUtil.setErrorAlert(this,
                    NewPolicyWizardTapeCopyOptionView.CHILD_ALERT,
                    "NewArchivePolWizard.page4.failedPopulate",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
        }

        // Set label to red if error is detected
        setErrorLabel(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private boolean showPreviousError(SamWizardModel wizardModel) {
        // see if any errors occcurred from previous page
        String error = (String)
            wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);

        if (error != null && error.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            prevErr = true;
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
            SamUtil.setErrorAlert(this,
                NewPolicyWizardTapeCopyOptionView.CHILD_ALERT,
                "NewArchivePolWizard.error.carryover",
                code,
                msgs,
                getServerName());
            return true;
        } else {
            return false;
        }
    }

    private void prePopulateFields(SamWizardModel wizardModel)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        // Grab the HashMap that contains copy information from wizardModel
        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewPolicyWizardImpl.COPY_HASHMAP);

        // Grab the copy number that this page represents,
        // the copy number is assigned in getPageClass()
        Integer copyNumber =
            (Integer) wizardModel.getValue(Constants.Wizard.COPY_NUMBER);

        // Retrieve the CopyInfo data structure from the HashMap
        CopyInfo info =
            (CopyInfo) copyNumberHashMap.get(copyNumber);

        // Grab the wrapper from the CopyInfo datastructure, then grab the
        // ArchiveCopy object to start prepopulating information
        ArchiveCopyGUIWrapper myWrapper = info.getCopyWrapper();
        ArchiveCopy copy = myWrapper.getArchiveCopy();

        // Offline Copy
        int offlineCopyMethod = copy.getOfflineCopyMethod();
        if (offlineCopyMethod != -1) {
            ((CCDropDownMenu) getChild(CHILD_OFFLINE_COPY_DROPDOWN)).
                setValue(Integer.toString(offlineCopyMethod));
        } else {
            ((CCDropDownMenu) getChild(CHILD_OFFLINE_COPY_DROPDOWN)).
                setValue(SelectableGroupHelper.NOVAL);
        }

        // Drives
        int drives = copy.getDrives();
        if (drives != -1) {
            ((CCTextField) getChild(CHILD_DRIVES_TEXTFIELD)).
                setValue(Integer.toString(drives));
        } else {
            ((CCTextField) getChild(CHILD_DRIVES_TEXTFIELD)).setValue("");
        }

        // Minimum Drive Size
        long minDriveSize = copy.getMinDrives();
        int  minDriveUnit = copy.getMinDrivesUnit();
        if (minDriveSize != -1 && minDriveUnit != -1) {
            ((CCTextField) getChild(CHILD_DRIVES_MIN_TEXTFIELD)).
                setValue(Long.toString(minDriveSize));
            ((CCDropDownMenu) getChild(CHILD_DRIVES_MIN_DROPDOWN)).
                setValue(Integer.toString(minDriveUnit));
        } else {
            ((CCTextField) getChild(CHILD_DRIVES_MIN_TEXTFIELD)).setValue("");
            ((CCDropDownMenu) getChild(CHILD_DRIVES_MIN_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        // Maximum Drive Size
        long maxDriveSize = copy.getMaxDrives();
        int  maxDriveUnit = copy.getMaxDrivesUnit();
        if (maxDriveSize != -1 && maxDriveUnit != -1) {
            ((CCTextField) getChild(CHILD_DRIVES_MAX_TEXTFIELD)).
                setValue(Long.toString(maxDriveSize));
            ((CCDropDownMenu) getChild(CHILD_DRIVES_MAX_DROPDOWN)).
                setValue(Integer.toString(maxDriveUnit));
        } else {
            ((CCTextField) getChild(CHILD_DRIVES_MAX_TEXTFIELD)).setValue("");
            ((CCDropDownMenu) getChild(CHILD_DRIVES_MAX_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        // Start Age
        long startAge = copy.getStartAge();
        int  startAgeUnit = copy.getStartAgeUnit();
        if (startAge != -1 && startAgeUnit != -1) {
            ((CCTextField) getChild(CHILD_START_AGE_TEXTFIELD)).
                setValue(Long.toString(startAge));
            ((CCDropDownMenu) getChild(CHILD_START_AGE_DROPDOWN)).
                setValue(Integer.toString(startAgeUnit));
        } else {
            ((CCTextField) getChild(CHILD_START_AGE_TEXTFIELD)).setValue("");
            ((CCDropDownMenu) getChild(CHILD_START_AGE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        // Start Count
        int startCount = copy.getStartCount();
        if (startCount != -1) {
            ((CCTextField) getChild(CHILD_START_COUNT_TEXTFIELD)).
                setValue(Long.toString(startCount));
        } else {
            ((CCTextField) getChild(CHILD_START_COUNT_TEXTFIELD)).setValue("");
        }

        // Start Size
        long startSize = copy.getStartSize();
        int  startSizeUnit = copy.getStartSizeUnit();
        if (startSize != -1 && startSizeUnit != -1) {
            ((CCTextField) getChild(CHILD_START_SIZE_TEXTFIELD)).
                setValue(Long.toString(startSize));
            ((CCDropDownMenu) getChild(CHILD_START_SIZE_DROPDOWN)).
                setValue(Integer.toString(startSizeUnit));
        } else {
            ((CCTextField) getChild(CHILD_START_SIZE_TEXTFIELD)).setValue("");
            ((CCDropDownMenu) getChild(CHILD_START_SIZE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        TraceUtil.trace3("Exiting");
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
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
