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

// ident	$Id: NewCriteriaMatchCriteria.java,v 1.19 2008/12/16 00:12:08 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
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
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

public class NewCriteriaMatchCriteria extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewCriteriaMatchCriteria";

    // children
    public static final String STARTING_DIR_LABEL = "StartingDirLabel";
    public static final String STARTING_DIR = "StartingDir";
    public static final String MINSIZE_LABEL = "MinSizeLabel";
    public static final String MINSIZE = "MinSize";
    public static final String MINSIZE_UNITS = "MinSizeUnits";
    public static final String MAXSIZE_LABEL = "MaxSizeLabel";
    public static final String MAXSIZE = "MaxSize";
    public static final String MAXSIZE_UNITS = "MaxSizeUnits";
    public static final String ACCESS_AGE_LABEL = "AccessAgeLabel";
    public static final String ACCESS_AGE = "AccessAge";
    public static final String ACCESS_AGE_UNITS = "AccessAgeUnits";
    public static final String NAME_PATTERN_LABEL = "NamePatternLabel";
    public static final String NAME_PATTERN = "NamePattern";
    public static final String OWNER_LABEL = "OwnerLabel";
    public static final String OWNER = "Owner";
    public static final String GROUP_LABEL = "GroupLabel";
    public static final String GROUP = "Group";
    public static final String STAGING_LABEL = "StagingLabel";
    public static final String STAGING = "Staging";
    public static final String RELEASING_LABEL = "ReleasingLabel";
    public static final String RELEASING = "Releasing";
    public static final String NAME_PATTERN_HELP = "NamePatternHelp";
    public static final String APPLY_TO_FS = "SelectionListApplyToFS";
    public static final String APPLY_TO_FS_TEXT = "ApplyToFSText";
    public static final String ERROR_OCCUR = "errorOccur";
    public static final String CHILD_ALERT = "Alert";

    private boolean display_error_page = false;
    private boolean isNoArchive = false;

    public NewCriteriaMatchCriteria(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewCriteriaMatchCriteria(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(STARTING_DIR_LABEL, CCLabel.class);
        registerChild(MINSIZE_LABEL, CCLabel.class);
        registerChild(MAXSIZE_LABEL, CCLabel.class);
        registerChild(ACCESS_AGE_LABEL, CCLabel.class);
        registerChild(NAME_PATTERN_LABEL, CCLabel.class);
        registerChild(OWNER_LABEL, CCLabel.class);
        registerChild(GROUP_LABEL, CCLabel.class);
        registerChild(STAGING_LABEL, CCLabel.class);
        registerChild(RELEASING_LABEL, CCLabel.class);
        registerChild(NAME_PATTERN_HELP, CCStaticTextField.class);
        registerChild(STARTING_DIR, CCTextField.class);
        registerChild(MINSIZE, CCTextField.class);
        registerChild(MAXSIZE, CCTextField.class);
        registerChild(ACCESS_AGE, CCTextField.class);
        registerChild(NAME_PATTERN, CCTextField.class);
        registerChild(OWNER, CCTextField.class);
        registerChild(GROUP, CCTextField.class);
        registerChild(ERROR_OCCUR, CCHiddenField.class);
        registerChild(MINSIZE_UNITS, CCDropDownMenu.class);
        registerChild(MAXSIZE_UNITS, CCDropDownMenu.class);
        registerChild(ACCESS_AGE_UNITS, CCDropDownMenu.class);
        registerChild(STAGING, CCDropDownMenu.class);
        registerChild(RELEASING, CCDropDownMenu.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(APPLY_TO_FS, CCSelectableList.class);
        registerChild(APPLY_TO_FS_TEXT, CCLabel.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(STARTING_DIR_LABEL) ||
            name.equals(MINSIZE_LABEL) ||
            name.equals(MAXSIZE_LABEL) ||
            name.equals(ACCESS_AGE_LABEL) ||
            name.equals(NAME_PATTERN_LABEL) ||
            name.equals(OWNER_LABEL) ||
            name.equals(GROUP_LABEL) ||
            name.equals(STAGING_LABEL) ||
            name.equals(RELEASING_LABEL) ||
            name.equals(APPLY_TO_FS_TEXT)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(STARTING_DIR) ||
            name.equals(MINSIZE) ||
            name.equals(MAXSIZE) ||
            name.equals(ACCESS_AGE) ||
            name.equals(NAME_PATTERN) ||
            name.equals(OWNER) ||
            name.equals(GROUP)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(MINSIZE_UNITS) ||
            name.equals(MAXSIZE_UNITS) ||
            name.equals(ACCESS_AGE_UNITS) ||
            name.equals(STAGING) ||
            name.equals(RELEASING)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(NAME_PATTERN_HELP)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child = new CCAlertInline(this, name, null);
            child.setValue(CCAlertInline.TYPE_ERROR);
            return (View) child;
        } else if (name.equals(APPLY_TO_FS)) {
            return new CCSelectableList(this, name, null);
        } else if (name.equals(ERROR_OCCUR)) {
            if (display_error_page) {
                return new CCHiddenField(
                    this, name, Constants.Wizard.EXCEPTION);
            } else {
                return new CCHiddenField(this, name, Constants.Wizard.SUCCESS);
            }
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    // implement CCWizardPage
    public String getPageletUrl() {
        return
            display_error_page ?
                "/jsp/fs/wizardErrorPage.jsp" :
                "/jsp/archive/wizards/NewCriteriaMatchCriteria.jsp";
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // init dropdowns
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MINSIZE_UNITS);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Size.labels,
            SelectableGroupHelper.Size.values));

        if (dropDown.getValue() == null) {
            dropDown.setValue(
                new Integer(SamQFSSystemModel.SIZE_KB).toString());
        }

        dropDown = (CCDropDownMenu)getChild(MAXSIZE_UNITS);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Size.labels,
            SelectableGroupHelper.Size.values));

        if (dropDown.getValue() == null) {
            dropDown.setValue(
                new Integer(SamQFSSystemModel.SIZE_KB).toString());
        }

        if (!isNoArchive) {
            dropDown = (CCDropDownMenu)getChild(STAGING);
            dropDown.setOptions(new OptionList(
                SelectableGroupHelper.StagingForWizard.labels,
                SelectableGroupHelper.StagingForWizard.values));

            dropDown = (CCDropDownMenu)getChild(RELEASING);
            dropDown.setOptions(new OptionList(
                SelectableGroupHelper.ReleasingForWizard.labels,
                SelectableGroupHelper.ReleasingForWizard.values));
        }

        dropDown = (CCDropDownMenu)getChild(ACCESS_AGE_UNITS);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Time.labels,
            SelectableGroupHelper.Time.values));

        if (dropDown.getValue() == null) {
            dropDown.setValue(
                new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        populateApplyToFSField();

        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        // Set label to red if error is detected
        setErrorLabel(wm);

        // prepopulate Starting directory to be "." if user has no input yet
        String startingDir = (String) wm.getValue(STARTING_DIR);
        startingDir = startingDir != null ? startingDir.trim() : "";

        if (startingDir.equals("")) {
            wm.setValue(STARTING_DIR, ".");
        }
        showWizardValidationMessage(wm);
    }

    private void populateApplyToFSField() {
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
            display_error_page = true;
            SamUtil.setErrorAlert(
                this,
                CHILD_ALERT,
                "NewPolicyWizard.populate.fs.failed",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private void setErrorLabel(SamWizardModel wizardModel) {
        String labelName = (String) wizardModel.getValue(
            NewCriteriaWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName == "") {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        wizardModel.setValue(
            NewCriteriaWizardImpl.VALIDATION_ERROR, "");
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

    /**
     * Hide staging drop down if working on noArchive policy
     */
    public boolean beginStagingLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        return !workingOnNoArchive();
    }

    /**
     * Hide staging drop down if working on noArchive policy
     */
    public boolean beginStagingDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        return !workingOnNoArchive();
    }

    /**
     * Hide releasing drop down if working on noArchive policy
     */
    public boolean beginReleasingLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        return !workingOnNoArchive();
    }

    /**
     * Hide releasing drop down if working on noArchive policy
     */
    public boolean beginReleasingDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        return !workingOnNoArchive();
    }

    private boolean workingOnNoArchive() {
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        String policyName = (String)
            wm.getValue(Constants.SessionAttributes.POLICY_NAME);
        return policyName.equals(Constants.Archive.NOARCHIVE_POLICY_NAME);
    }
}
