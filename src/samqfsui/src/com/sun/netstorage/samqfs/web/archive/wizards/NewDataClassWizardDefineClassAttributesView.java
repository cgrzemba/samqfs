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

// ident	$Id: NewDataClassWizardDefineClassAttributesView.java,v 1.9 2008/12/16 00:12:08 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import com.sun.web.ui.view.wizard.CCWizardPage;


/**
 * A ContainerView object for the pagelet for define class attributes in
 * new Data Class Wizard
 *
 */
public class NewDataClassWizardDefineClassAttributesView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME =
        "NewDataClassWizardDefineClassAttributesView";
    public static final String URL =
        "/jsp/archive/wizards/NewDataClassWizardDefineClassAttributes.jsp";

    // Child view names (i.e. display fields).
    public static final String ALERT = "Alert";
    public static final String ERROR = "errorOccur";
    public static final String PAGE_TITLE = "PageTitle";
    public static final String HELP_TEXT = "HelpText";

    // labels (registered to set to red color if error occurred
    public static final String EXPIRATION_TIME_LABEL = "expirationTimeLabel";
    public static final String PERIODIC_AUDIT_LABEL = "periodicAuditLabel";
    public static final String AUDIT_PERIOD_LABEL = "auditPeriodLabel";

    // class attributes
    public static final String EXPIRATION_TIME = "absolute_expiration_time";
    public static final String EXPIRATION_TIME_HIDDEN =
        "absolute_expiration_time-hidden";
    public static final String EXPT_TYPE = "expirationTimeType";
    public static final String DURATION = "relative_expiration_time";
    public static final String DURATION_UNIT = "relative_expiration_time_unit";
    public static final String NEVER_EXPIRE = "neverExpire";
    public static final String PERIODIC_AUDIT = "periodicaudit";
    public static final String AUDIT_PERIOD = "auditperiod";
    public static final String AUDIT_PERIOD_UNIT = "auditperiodunit";
    public static final String AUTO_WORM = "autoworm";
    public static final String AUTO_DELETE = "autodelete";
    public static final String DEDUP = "dedup";
    public static final String BITBYBIT = "bitbybit";

    // logging
    public static final String LOG_AUDIT = "log_data_audit";
    public static final String LOG_DEDUP = "log_deduplication";
    public static final String LOG_AUTOWORM = "log_autoworm";
    public static final String LOG_AUTODELETION = "log_autodeletion";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewDataClassWizardDefineClassAttributesView(
        View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewDataClassWizardDefineClassAttributesView(
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
        registerChild(ERROR, CCHiddenField.class);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(EXPIRATION_TIME_LABEL, CCLabel.class);
        registerChild(PERIODIC_AUDIT_LABEL, CCLabel.class);
        registerChild(AUTO_WORM, CCCheckBox.class);
        registerChild(AUDIT_PERIOD_LABEL, CCLabel.class);
        registerChild(EXPIRATION_TIME, CCTextField.class);
        registerChild(EXPIRATION_TIME_HIDDEN, CCHiddenField.class);
        registerChild(EXPT_TYPE, CCRadioButton.class);
        registerChild(DURATION, CCTextField.class);
        registerChild(DURATION_UNIT, CCDropDownMenu.class);
        registerChild(NEVER_EXPIRE, CCCheckBox.class);
        registerChild(AUTO_DELETE, CCCheckBox.class);
        registerChild(DEDUP, CCCheckBox.class);
        registerChild(BITBYBIT, CCCheckBox.class);
        registerChild(PERIODIC_AUDIT, CCDropDownMenu.class);
        registerChild(AUDIT_PERIOD, CCTextField.class);
        registerChild(AUDIT_PERIOD_UNIT, CCDropDownMenu.class);
        registerChild(LOG_AUDIT, CCCheckBox.class);
        registerChild(LOG_DEDUP, CCCheckBox.class);
        registerChild(LOG_AUTOWORM, CCCheckBox.class);
        registerChild(LOG_AUTODELETION, CCCheckBox.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(HELP_TEXT)) {
            String format =
                (String) ((SamWizardModel) getDefaultModel()).
                    getValue(NewDataClassWizardImpl.DATE_FORMAT);
            child = new CCStaticTextField(this, name, format);
        } else if (name.equals(ERROR)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(PAGE_TITLE)) {
            child = new CCPageTitle(this, createPageTitleModel(), name);
        } else if (name.equals(AUTO_WORM) ||
            name.equals(NEVER_EXPIRE) ||
            name.equals(AUTO_DELETE) ||
            name.equals(DEDUP) ||
            name.equals(BITBYBIT) ||
            name.equals(LOG_AUDIT) ||
            name.equals(LOG_DEDUP) ||
            name.equals(LOG_AUTOWORM) ||
            name.equals(LOG_AUTODELETION)) {
            child = new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(EXPIRATION_TIME_LABEL) ||
            name.equals(PERIODIC_AUDIT_LABEL) ||
            name.equals(AUDIT_PERIOD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(EXPIRATION_TIME) ||
            name.equals(DURATION) ||
            name.equals(AUDIT_PERIOD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(EXPT_TYPE)) {
            child = new CCRadioButton(this, name, null);
        } else if (name.equals(EXPIRATION_TIME_HIDDEN)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(DURATION_UNIT) ||
            name.equals(PERIODIC_AUDIT) ||
            name.equals(AUDIT_PERIOD_UNIT)) {
            child = new CCDropDownMenu(this, name, null);
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
        return URL;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // show the yellow warning message if another wizard instance is found
        showWizardValidationMessage(wizardModel);

        // set the label to error mode if necessary
        setErrorLabel(wizardModel);

        // populate component content (Content of radiobutton, dropdown, etc)
        populateComponentContent(wizardModel);

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

    private void populateComponentContent(SamWizardModel wizardModel) {
        // dataclass attributes
        CCRadioButton rb = (CCRadioButton)getChild(EXPT_TYPE);
        rb.setOptions(new OptionList(new String [] {"Date", "Duration"},
                                     new String [] {"date", "duration"}));

        // set the expiration time units
        OptionList timeUnits = new OptionList(
            SelectableGroupHelper.ExpirationTime.labels,
            SelectableGroupHelper.ExpirationTime.values);

        // duration units
        ((CCDropDownMenu)getChild(DURATION_UNIT)).setOptions(timeUnits);
        // audit period nits
        ((CCDropDownMenu)getChild(AUDIT_PERIOD_UNIT)).setOptions(timeUnits);

        // periodic copy editing
        ((CCDropDownMenu)getChild(PERIODIC_AUDIT)).setOptions(
            new OptionList(
                SelectableGroupHelper.periodicAuditSetting.labels,
                SelectableGroupHelper.periodicAuditSetting.values));

        // Choose Date as the Expiration Time if no value is selected
        if (rb.getValue() == null) {
            rb.setValue("date");
        }

        // Set the correct state of Absolute File Comparison Check Box
         if ("true".equals((String) wizardModel.getValue(DEDUP))) {
            ((CCCheckBox) getChild(BITBYBIT)).setDisabled(false);
         }
    }

    private CCPageTitleModel createPageTitleModel() {
        return new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
    }

    private String getServerName(SamWizardModel wizardModel) {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
