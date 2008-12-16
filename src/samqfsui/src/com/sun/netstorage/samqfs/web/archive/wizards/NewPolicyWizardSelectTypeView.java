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

// ident	$Id: NewPolicyWizardSelectTypeView.java,v 1.15 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
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
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for Select Type Page.
 *
 */
public class NewPolicyWizardSelectTypeView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewPolicyWizardSelectTypeView";

    // Child view names (i.e. display fields).
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_START_DIR_TEXT = "StartingDir";
    public static final String ARCHIVE_MENU = "ArchiveMenu";
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_START_DIR_TEXTFIELD =
        "StartingDirTextField";
    public static final String CHILD_MIN_SIZE_TEXT = "MinSizeText";
    public static final String CHILD_MIN_SIZE_TEXTFIELD = "MinSizeTextField";
    public static final String CHILD_MIN_SIZE_DROPDOWN = "MinSizeDropDown";
    public static final String CHILD_MAX_SIZE_TEXT = "MaxSizeText";
    public static final String CHILD_MAX_SIZE_TEXTFIELD = "MaxSizeTextField";
    public static final String CHILD_MAX_SIZE_DROPDOWN = "MaxSizeDropDown";
    public static final String CHILD_ACCESS_AGE_TEXT = "AccessAgeText";
    public static final String CHILD_ACCESS_AGE_TEXTFIELD =
        "AccessAgeTextField";
    public static final String CHILD_ACCESS_AGE_DROPDOWN = "AccessAgeDropDown";
    public static final String CHILD_NAME_PATTERN_TEXT = "NamePatternText";
    public static final String CHILD_NAME_PATTERN_TEXTFIELD =
        "NamePatternTextField";
    public static final String CHILD_OWNER_TEXT = "OwnerText";
    public static final String CHILD_OWNER_TEXTFIELD = "OwnerTextField";
    public static final String CHILD_GROUP_TEXT = "GroupText";
    public static final String CHILD_GROUP_TEXTFIELD = "GroupTextField";
    public static final String CHILD_HELP_TEXT = "HelpText";
    public static final String APPLY_TO_FS = "SelectionListApplyToFS";
    public static final String APPLY_TO_FS_TEXT = "ApplyToFSText";

    // Hidden field to determine if noArchive exists
    public static final String NO_ARCHIVE_EXISTS = "noArchiveExists";
    public static final String ERROR_OCCUR = "errorOccur";

    // Page Title Attributes and Components.
    private static CCPageTitleModel pageTitleModel = null;

    private boolean error = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewPolicyWizardSelectTypeView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewPolicyWizardSelectTypeView(
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
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(ARCHIVE_MENU, CCDropDownMenu.class);
        registerChild(CHILD_START_DIR_TEXT, CCLabel.class);
        registerChild(CHILD_START_DIR_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_MIN_SIZE_TEXT, CCLabel.class);
        registerChild(CHILD_MIN_SIZE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_MIN_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_MAX_SIZE_TEXT, CCLabel.class);
        registerChild(CHILD_MAX_SIZE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_MAX_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_ACCESS_AGE_TEXT, CCLabel.class);
        registerChild(CHILD_ACCESS_AGE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_ACCESS_AGE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_NAME_PATTERN_TEXT, CCLabel.class);
        registerChild(CHILD_NAME_PATTERN_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_OWNER_TEXT, CCLabel.class);
        registerChild(CHILD_OWNER_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_GROUP_TEXT, CCLabel.class);
        registerChild(CHILD_GROUP_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_HELP_TEXT, CCStaticTextField.class);
        registerChild(NO_ARCHIVE_EXISTS, CCHiddenField.class);
        registerChild(ERROR_OCCUR, CCHiddenField.class);
        registerChild(APPLY_TO_FS, CCSelectableList.class);
        registerChild(APPLY_TO_FS_TEXT, CCLabel.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_START_DIR_TEXT) ||
            name.equals(CHILD_LABEL) ||
            name.equals(CHILD_MIN_SIZE_TEXT) ||
            name.equals(CHILD_MAX_SIZE_TEXT) ||
            name.equals(CHILD_ACCESS_AGE_TEXT) ||
            name.equals(CHILD_NAME_PATTERN_TEXT) ||
            name.equals(CHILD_OWNER_TEXT) ||
            name.equals(CHILD_GROUP_TEXT) ||
            name.equals(APPLY_TO_FS_TEXT)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_START_DIR_TEXTFIELD) ||
            name.equals(CHILD_MIN_SIZE_TEXTFIELD) ||
            name.equals(CHILD_MAX_SIZE_TEXTFIELD) ||
            name.equals(CHILD_ACCESS_AGE_TEXTFIELD) ||
            name.equals(CHILD_NAME_PATTERN_TEXTFIELD) ||
            name.equals(CHILD_OWNER_TEXTFIELD) ||
            name.equals(CHILD_GROUP_TEXTFIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_HELP_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(ARCHIVE_MENU)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList radioButtonOptions =
                new OptionList(
                    new String [] {
                        "NewPolicyWizard.defineType.archive",
                        "NewPolicyWizard.defineType.no_archive"},
                    new String [] {
                        NewPolicyWizardImpl.ARCHIVE,
                        NewPolicyWizardImpl.NO_ARCHIVE
                    });
            myChild.setOptions(radioButtonOptions);
            child = myChild;
        } else if (name.equals(CHILD_MIN_SIZE_DROPDOWN) ||
            name.equals(CHILD_MAX_SIZE_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList sizeOptions =
                new OptionList(
                    SelectableGroupHelper.Size.labels,
                    SelectableGroupHelper.Size.values);
            myChild.setOptions(sizeOptions);
            child = myChild;
        } else if (name.equals(CHILD_ACCESS_AGE_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList sizeOptions =
                new OptionList(
                    SelectableGroupHelper.Time.labels,
                    SelectableGroupHelper.Time.values);
            myChild.setOptions(sizeOptions);
            child = myChild;
        } else if (name.equals(NO_ARCHIVE_EXISTS)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(ERROR_OCCUR)) {
            if (error) {
                child = new CCHiddenField(
                    this, name, Constants.Wizard.EXCEPTION);
            } else {
                child = new CCHiddenField(this, name, Constants.Wizard.SUCCESS);
            }
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(APPLY_TO_FS)) {
            child = new CCSelectableList(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "NewPolicyWizardSelectTypeView : Invalid child name [" +
                    name + "]");
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
        TraceUtil.trace3("Exiting");

        if (error) {
            return "/jsp/fs/wizardErrorPage.jsp";
        } else {
            return "/jsp/archive/wizards/NewPolicyWizardSelectType.jsp";
        }
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

        // show the yellow warning message if another wizard instance is found
        showWizardValidationMessage(wizardModel);

        // populate all fields appropriately
        prePopulateFields(wizardModel);

        // Set label to red if error is detected
        setErrorLabel(wizardModel);

        // Set true/false to hidden field to determine if "create a policy to"
        // section needs to be shown or not
        ((CCHiddenField) getChild(NO_ARCHIVE_EXISTS)).setValue(
            Boolean.toString(noArchiveExists(wizardModel)));

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
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }

    private void prePopulateFields(SamWizardModel wizardModel) {
        TraceUtil.trace3("Entering");

        if (noArchiveExists(wizardModel)) {
            wizardModel.setValue(
                ARCHIVE_MENU, NewPolicyWizardImpl.ARCHIVE);
        } else {
             String archiveMenuValue =
                (String) wizardModel.getValue(ARCHIVE_MENU);
             CCDropDownMenu archiveMenu =
                (CCDropDownMenu) getChild(ARCHIVE_MENU);
             if (archiveMenuValue == null) {
                 archiveMenu.setValue(NewPolicyWizardImpl.ARCHIVE);
             }
        }

        // Starting Directory
        CCTextField startDirTextField =
            (CCTextField) getChild(CHILD_START_DIR_TEXTFIELD);
        String startDir = (String) startDirTextField.getValue();
        if (startDir == null || startDir.equals("")) {
            startDirTextField.setValue(".");
        }

        String minSizeUnit = (String)
            wizardModel.getValue(CHILD_MIN_SIZE_DROPDOWN);
        if (minSizeUnit == null ||
            minSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
            ((CCDropDownMenu) getChild(CHILD_MIN_SIZE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        String maxSizeUnit = (String)
            wizardModel.getValue(CHILD_MAX_SIZE_DROPDOWN);
        if (maxSizeUnit == null ||
            maxSizeUnit.equals(SelectableGroupHelper.NOVAL)) {
            ((CCDropDownMenu) getChild(CHILD_MAX_SIZE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        String accessAgeUnit = (String)
            wizardModel.getValue(CHILD_ACCESS_AGE_DROPDOWN);
        if (accessAgeUnit == null ||
            accessAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
            ((CCDropDownMenu) getChild(CHILD_ACCESS_AGE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        CCSelectableList list = (CCSelectableList) getChild(APPLY_TO_FS);
        try {
            // get all the filesystems
            FileSystem[] myArchivingFS =
                SamUtil.getModel(getServerName()).
                getSamQFSSystemFSManager().
                getAllFileSystems(FileSystem.ARCHIVING);
            if (myArchivingFS == null || myArchivingFS.length == 0) {
                throw new SamFSException(null, -1000);
            }
            String [] fsArray = new String[myArchivingFS.length];
            for (int i = 0; i < myArchivingFS.length; i++) {
                fsArray[i] = myArchivingFS[i].getName();
            }

            OptionList fsList = new OptionList(fsArray, fsArray);
            list.setOptions(fsList);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "prePopulateFields()",
                "Failed to populate fs data",
                getServerName());
            error = true;
            SamUtil.setErrorAlert(
                this,
                CHILD_ALERT,
                "NewPolicyWizard.populate.fs.failed",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    private boolean noArchiveExists(SamWizardModel wizardModel) {
        return  Boolean.valueOf(
            (String) wizardModel.getValue(
                NewPolicyWizardImpl.NO_ARCHIVE_EXISTS)).booleanValue();
    }

    /**
     * Hide label if noArchive exists
     */
    public boolean beginLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        return !noArchiveExists((SamWizardModel) getDefaultModel());
    }

    /**
     * Hide label if noArchive exists
     */
    public boolean beginArchiveMenuDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        return !noArchiveExists((SamWizardModel) getDefaultModel());
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
