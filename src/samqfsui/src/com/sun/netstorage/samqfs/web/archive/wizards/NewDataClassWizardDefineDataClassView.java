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

// ident	$Id: NewDataClassWizardDefineDataClassView.java,v 1.14 2008/12/16 00:12:08 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCDateTimeModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.datetime.CCDateTimeWindow;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for define data class in
 * new Data Class Wizard
 *
 */
public class NewDataClassWizardDefineDataClassView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME =
        "NewDataClassWizardDefineDataClassView";

    // Child view names (i.e. display fields).
    public static final String ALERT = "Alert";
    public static final String HELP_TEXT = "HelpText";
    public static final String CLASS_NAME_LABEL = "DataClassNameText";
    public static final String CLASS_NAME = "DataClassName";
    public static final String DESCRIPTION_LABEL = "DescriptionText";
    public static final String DESCRIPTION = "Description";
    public static final String START_DIR_LABEL = "StartingDirText";
    public static final String START_DIR = "StartingDir";
    public static final String MIN_SIZE_LABEL = "MinSizeText";
    public static final String MIN_SIZE = "MinSizeTextField";
    public static final String MIN_SIZE_DROPDOWN = "MinSizeDropDown";
    public static final String MAX_SIZE_LABEL = "MaxSizeText";
    public static final String MAX_SIZE = "MaxSizeTextField";
    public static final String MAX_SIZE_DROPDOWN = "MaxSizeDropDown";
    public static final String ACCESS_AGE_LABEL = "AccessAgeText";
    public static final String ACCESS_AGE = "AccessAgeTextField";
    public static final String ACCESS_AGE_DROPDOWN = "AccessAgeDropDown";
    public static final String NAME_PATTERN_LABEL = "NamePatternText";
    public static final String NAME_PATTERN = "NamePattern";
    public static final String NAME_PATTERN_DROPDOWN = "NamePatternDropDown";
    public static final String OWNER_LABEL = "OwnerText";
    public static final String OWNER = "Owner";
    public static final String GROUP_LABEL = "GroupText";
    public static final String GROUP = "Group";
    public static final String INCLUDE_FILE_DATE_LABEL = "IncludeFileDateText";
    public static final String INCLUDE_FILE_DATE = "IncludeFileDate";
    public static final String SELECT_POLICY_LABEL = "SelectPolicyText";
    public static final String SELECT_POLICY_DROPDOWN = "SelectPolicyDropDown";
    public static final String ERROR = "errorOccur";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewDataClassWizardDefineDataClassView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewDataClassWizardDefineDataClassView(
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
        registerChild(ALERT, CCAlertInline.class);
        registerChild(HELP_TEXT, CCStaticTextField.class);
        registerChild(CLASS_NAME_LABEL, CCLabel.class);
        registerChild(CLASS_NAME, CCTextField.class);
        registerChild(DESCRIPTION_LABEL, CCLabel.class);
        registerChild(DESCRIPTION, CCTextField.class);
        registerChild(START_DIR_LABEL, CCLabel.class);
        registerChild(START_DIR, CCTextField.class);
        registerChild(MIN_SIZE_LABEL, CCLabel.class);
        registerChild(MIN_SIZE, CCTextField.class);
        registerChild(MIN_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(MAX_SIZE_LABEL, CCLabel.class);
        registerChild(MAX_SIZE, CCTextField.class);
        registerChild(MAX_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(ACCESS_AGE_LABEL, CCLabel.class);
        registerChild(ACCESS_AGE, CCTextField.class);
        registerChild(ACCESS_AGE_DROPDOWN, CCDropDownMenu.class);
        registerChild(NAME_PATTERN_LABEL, CCLabel.class);
        registerChild(NAME_PATTERN, CCTextField.class);
        registerChild(NAME_PATTERN_DROPDOWN, CCDropDownMenu.class);
        registerChild(OWNER_LABEL, CCLabel.class);
        registerChild(OWNER, CCTextField.class);
        registerChild(GROUP_LABEL, CCLabel.class);
        registerChild(GROUP, CCTextField.class);
        // registerChild(VALIDATE, CCCheckBox.class);
        registerChild(INCLUDE_FILE_DATE_LABEL, CCLabel.class);
        registerChild(INCLUDE_FILE_DATE, CCTextField.class);
        // registerChild(APPLY_TO_FS_LABEL, CCLabel.class);
        // registerChild(APPLY_TO_FS, CCSelectableList.class);
        registerChild(SELECT_POLICY_LABEL, CCLabel.class);
        registerChild(SELECT_POLICY_DROPDOWN, CCDropDownMenu.class);
        registerChild(ERROR, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(START_DIR_LABEL) ||
            name.equals(MIN_SIZE_LABEL) ||
            name.equals(MAX_SIZE_LABEL) ||
            name.equals(ACCESS_AGE_LABEL) ||
            name.equals(NAME_PATTERN_LABEL) ||
            name.equals(OWNER_LABEL) ||
            name.equals(GROUP_LABEL) ||
            name.equals(ACCESS_AGE_LABEL) ||
            name.equals(CLASS_NAME_LABEL) ||
            name.equals(DESCRIPTION_LABEL) ||
            name.equals(INCLUDE_FILE_DATE_LABEL) ||
            name.equals(SELECT_POLICY_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(HELP_TEXT)) {
            // Figure out the Date Format for this locale
            CCDateTimeWindow dtWindow =
                new CCDateTimeWindow(this, new CCDateTimeModel(), "dummy");
            String format = dtWindow.getDateFormatPattern();
            TraceUtil.trace2("Locale Date Format: ".concat(format));
            SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
            wizardModel.setValue(NewDataClassWizardImpl.DATE_FORMAT, format);

            child = new CCStaticTextField(this, name, format);
        } else if (name.equals(START_DIR) ||
            name.equals(MIN_SIZE) ||
            name.equals(MAX_SIZE) ||
            name.equals(ACCESS_AGE) ||
            name.equals(NAME_PATTERN) ||
            name.equals(OWNER) ||
            name.equals(GROUP) ||
            name.equals(INCLUDE_FILE_DATE) ||
            name.equals(CLASS_NAME) ||
            name.equals(DESCRIPTION)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(MIN_SIZE_DROPDOWN) ||
            name.equals(MAX_SIZE_DROPDOWN) ||
            name.equals(ACCESS_AGE_DROPDOWN) ||
            name.equals(NAME_PATTERN_DROPDOWN) ||
            name.equals(SELECT_POLICY_DROPDOWN)) {
            child = new CCDropDownMenu(this, name, null);
        } else if (name.equals(ERROR)) {
            child = new CCHiddenField(this, name, null);
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
        TraceUtil.trace3("Exiting");
        return "/jsp/archive/wizards/NewDataClassWizardDefineDataClass.jsp";
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // show the yellow warning message if another wizard instance is found
        showWizardValidationMessage(wizardModel);

        // populate all drop down
        populateDropDownMenus(wizardModel);

        // populate all fields appropriately
        prePopulateFields(wizardModel);

        // set the label to error mode if necessary
        setErrorLabel(wizardModel);

        TraceUtil.trace3("Exiting");
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
                SamUtil.setWarningAlert(this,
                                        ALERT,
                                        errorSummary,
                                        msgs);
            }
        }
    }

    private void prePopulateFields(SamWizardModel wizardModel) {
        TraceUtil.trace3("Entering");

        // Starting Directory
        CCTextField startDirTextField =
            (CCTextField) getChild(START_DIR);
        String startDir = (String) startDirTextField.getValue();
        if (startDir == null || startDir.equals("")) {
            startDirTextField.setValue(".");
        }

        String minSizeUnit = (String)
            wizardModel.getValue(MIN_SIZE_DROPDOWN);
        if (minSizeUnit == null ||
            minSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
            ((CCDropDownMenu) getChild(MIN_SIZE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        String maxSizeUnit = (String)
            wizardModel.getValue(MAX_SIZE_DROPDOWN);
        if (maxSizeUnit == null ||
            maxSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
            ((CCDropDownMenu) getChild(MAX_SIZE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        String accessAgeUnit = (String)
            wizardModel.getValue(ACCESS_AGE_DROPDOWN);
        if (accessAgeUnit == null ||
            accessAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
            ((CCDropDownMenu) getChild(ACCESS_AGE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        // Take out from CIS Demo
        populatePolicyList(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private void populateDropDownMenus(SamWizardModel wizardModel) {
        TraceUtil.trace3("Entering");

        // Min / Max Size
        OptionList options =
            new OptionList(
                SelectableGroupHelper.Size.labels,
                SelectableGroupHelper.Size.values);
        CCDropDownMenu dropDown = (CCDropDownMenu) getChild(MIN_SIZE_DROPDOWN);
        dropDown.setOptions(options);
        dropDown = (CCDropDownMenu) getChild(MAX_SIZE_DROPDOWN);
        dropDown.setOptions(options);

        // Access Age
        options =
            new OptionList(
                SelectableGroupHelper.Time.labels,
                SelectableGroupHelper.Time.values);
        dropDown = (CCDropDownMenu) getChild(ACCESS_AGE_DROPDOWN);
        dropDown.setOptions(options);

        // Name Pattern
        options =
            new OptionList(
                SelectableGroupHelper.namePattern.labels,
                SelectableGroupHelper.namePattern.values);
        dropDown = (CCDropDownMenu) getChild(NAME_PATTERN_DROPDOWN);
        dropDown.setOptions(options);

        // Select Policy drop down is populated in beginDisplay

        TraceUtil.trace3("Exiting");
    }

    private void populatePolicyList(SamWizardModel wizardModel) {
        TraceUtil.trace3("Entering");
        CCDropDownMenu menu = (CCDropDownMenu) getChild(SELECT_POLICY_DROPDOWN);
        try {
            // Always add "Create new" at the top of the menu
            // Show all the existing policy names, including no_archive
            // If no_archive does not exist, add an entry to the bottom of the
            // menu
            String [] allPolicyNames =
                PolicyUtil.getArchiveManager(getServerName(wizardModel)).
                getAllNonDefaultNonAllSetsArchivePolicyNames();
            String createNewString =
                SamUtil.getResourceString(
                    "archiving.dataclass.wizard.createnew");
            String noArchiveString = Constants.Archive.NOARCHIVE_POLICY_NAME;

            boolean noArchiveExists =
                PolicyUtil.policyExists(
                    getServerName(wizardModel), noArchiveString);

            String [] entries =
                new String[allPolicyNames.length + (noArchiveExists ? 1 : 2)];
            entries[0] = createNewString;

            for (int i = 0; i < allPolicyNames.length; i++) {
                entries[i + 1] = allPolicyNames[i];
            }

            if (!noArchiveExists) {
                entries[entries.length - 1] = noArchiveString;
            }

            OptionList options = new OptionList(entries, entries);
            menu.setOptions(options);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay",
                "Failed to pre-populate input fields",
                getServerName(wizardModel));
            SamUtil.setErrorAlert(this,
                ALERT,
                "archiving.dataclass.wizard.error.populate.policylist",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName(wizardModel));
            ((CCHiddenField) getChild(ERROR)).setValue("true");
        }
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

    private String getServerName(SamWizardModel wizardModel) {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
