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

// ident	$Id: NewPolicyWizardSummaryView.java,v 1.14 2008/03/17 14:43:32 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

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
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A ContainerView object for the pagelet for the New Archive Policy Summary
 * page.
 *
 */
public class NewPolicyWizardSummaryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewPolicyWizardSummaryView";

    // Child view names (i.e. display fields).
    public static final String CHILD_POL_NAME_TEXT = "PolicyName";
    public static final String CHILD_POL_NAME_TEXTFIELD = "PolicyNameTextField";
    public static final String CHILD_START_DIR_TEXT = "StartingDir";
    public static final String CHILD_START_DIR_TEXTFIELD =
        "StartingDirTextField";
    public static final String CHILD_NUM_COPIES_TEXT = "NumCopies";
    public static final String CHILD_NUM_COPIES_TEXTFIELD =
        "NumCopiesTextField";
    public static final String CHILD_MIN_SIZE_TEXT = "MinSizeText";
    public static final String CHILD_MINIMUM_SIZE_TEXTFIELD =
        "MinimumSizeTextField";
    public static final String CHILD_MAX_SIZE_TEXT = "MaxSizeText";
    public static final String CHILD_MAXIMUM_SIZE_TEXTFIELD =
        "MaximumSizeTextField";
    public static final String CHILD_NAME_PATTERN_TEXT = "NamePatternText";
    public static final String CHILD_NAME_PATTERN_TEXTFIELD =
        "NamePatternTextField";
    public static final String CHILD_OWNER_TEXT = "OwnerText";
    public static final String CHILD_OWNER_TEXTFIELD = "OwnerTextField";
    public static final String CHILD_GROUP_TEXT = "GroupText";
    public static final String CHILD_GROUP_TEXTFIELD = "GroupTextField";
    public static final String CHILD_STAGE_TEXT = "StageAttText";
    public static final String CHILD_STAGE_TEXTFIELD = "StageAttTextField";
    public static final String CHILD_RELEASE_TEXT = "ReleaseAttText";
    public static final String CHILD_RELEASE_TEXTFIELD = "ReleaseAttTextField";
    public static final String CHILD_FS_TEXT = "FsText";
    public static final String CHILD_FS_DATA_FIELD = "FSDataField";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ACCESS_AGE_TEXT = "AccessAgeText";
    public static final String CHILD_ACCESS_AGE_TEXTFIELD =
        "AccessAgeTextFieldWithUnits";

    private boolean previousError;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewPolicyWizardSummaryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewPolicyWizardSummaryView(
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
        registerChild(CHILD_START_DIR_TEXT, CCLabel.class);
        registerChild(CHILD_START_DIR_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_POL_NAME_TEXT, CCLabel.class);
        registerChild(CHILD_POL_NAME_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_NUM_COPIES_TEXT, CCLabel.class);
        registerChild(CHILD_NUM_COPIES_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_MIN_SIZE_TEXT, CCLabel.class);
        registerChild(CHILD_MINIMUM_SIZE_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_MAX_SIZE_TEXT, CCLabel.class);
        registerChild(CHILD_MAXIMUM_SIZE_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_NAME_PATTERN_TEXT, CCLabel.class);
        registerChild(CHILD_NAME_PATTERN_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_OWNER_TEXT, CCLabel.class);
        registerChild(CHILD_OWNER_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_GROUP_TEXT, CCLabel.class);
        registerChild(CHILD_GROUP_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_STAGE_TEXT, CCLabel.class);
        registerChild(CHILD_STAGE_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_RELEASE_TEXT, CCLabel.class);
        registerChild(CHILD_RELEASE_TEXTFIELD, CCStaticTextField.class);
        registerChild(CHILD_FS_TEXT, CCLabel.class);
        registerChild(CHILD_FS_DATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ACCESS_AGE_TEXT, CCLabel.class);
        registerChild(CHILD_ACCESS_AGE_TEXTFIELD, CCStaticTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        View child = null;
        if (name.equals(CHILD_ALERT)) {
           child = new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_FS_DATA_FIELD)) {
            child = new CCSelectableList(this, name, null);
        } else if (name.equals(CHILD_POL_NAME_TEXT) ||
            name.equals(CHILD_START_DIR_TEXT) ||
            name.equals(CHILD_NUM_COPIES_TEXT) ||
            name.equals(CHILD_MIN_SIZE_TEXT) ||
            name.equals(CHILD_MAX_SIZE_TEXT) ||
            name.equals(CHILD_NAME_PATTERN_TEXT) ||
            name.equals(CHILD_OWNER_TEXT) ||
            name.equals(CHILD_GROUP_TEXT) ||
            name.equals(CHILD_STAGE_TEXT) ||
            name.equals(CHILD_RELEASE_TEXT) ||
            name.equals(CHILD_FS_TEXT) ||
            name.equals(CHILD_ACCESS_AGE_TEXT)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_POL_NAME_TEXTFIELD) ||
            name.equals(CHILD_START_DIR_TEXTFIELD) ||
            name.equals(CHILD_NUM_COPIES_TEXTFIELD) ||
            name.equals(CHILD_MINIMUM_SIZE_TEXTFIELD) ||
            name.equals(CHILD_MAXIMUM_SIZE_TEXTFIELD) ||
            name.equals(CHILD_NAME_PATTERN_TEXTFIELD) ||
            name.equals(CHILD_OWNER_TEXTFIELD) ||
            name.equals(CHILD_GROUP_TEXTFIELD) ||
            name.equals(CHILD_STAGE_TEXTFIELD) ||
            name.equals(CHILD_RELEASE_TEXTFIELD) ||
            name.equals(CHILD_ACCESS_AGE_TEXTFIELD)) {
            child = new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "NewPolicyWizardSummaryView : Invalid child name [" +
                name + "]");
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
        String url = null;
        if (!previousError) {
            url = "/jsp/archive/wizards/NewPolicyWizardSummary.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }

        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        if (!showPreviousError(wizardModel)) {
            populateFSDataField(wizardModel);
        }

        TraceUtil.trace3("Exiting");
    }

    private boolean showPreviousError(SamWizardModel wizardModel) {
        String error =
            (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (error != null) {
            if (error.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
                previousError = true;

                String msgs = (String)
                    wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
                int code = Integer.parseInt(
                    (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
                SamUtil.setErrorAlert(
                    this,
                NewPolicyWizardSummaryView.CHILD_ALERT,
                    "NewArchivePolWizard.error.carryover",
                    code,
                    msgs,
                    getServerName());
                return true;
            }
        }
        return false;
    }

    private void populateFSDataField(SamWizardModel wizardModel) {
        Object [] fsList = (Object []) wizardModel.getValues(
            NewPolicyWizardSelectTypeView.APPLY_TO_FS);
        String[] selectedFSArray = new String[fsList.length];
        for (int i = 0; i < selectedFSArray.length; i++) {
            selectedFSArray[i] = (String) fsList[i];
        }

        // Convert the array to a list
        List myFSList = Arrays.asList(selectedFSArray);

        // Sort it
        Collections.sort(myFSList);

        // Convert the list back to an array
        selectedFSArray = (String []) myFSList.toArray();

        OptionList dataOptions =
            new OptionList(selectedFSArray, selectedFSArray);
        CCSelectableList selectDataOptions =
            ((CCSelectableList) getChild(CHILD_FS_DATA_FIELD));
        selectDataOptions.setOptions(dataOptions);

        int dataOptionsSize = selectedFSArray.length;
        if (dataOptionsSize < Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
            selectDataOptions.setSize(dataOptionsSize);
        } else {
            selectDataOptions.setSize(
                Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
