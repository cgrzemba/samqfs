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

// ident	$Id: NewCriteriaSummary.java,v 1.15 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

public class NewCriteriaSummary extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewCriteriaSummary";

    // match criteria page
    public static final String STARTING_DIR_LABEL = "startingDirLabel";
    public static final String STARTING_DIR_TEXT = "startingDirText";
    public static final String NAME_PATTERN_LABEL = "namePatternLabel";
    public static final String NAME_PATTERN_TEXT = "namePatternText";
    public static final String MIN_SIZE_LABEL = "minSizeLabel";
    public static final String MIN_SIZE_TEXT = "minSizeText";
    public static final String MAX_SIZE_LABEL = "maxSizeLabel";
    public static final String MAX_SIZE_TEXT = "maxSizeText";
    public static final String OWNER_LABEL = "ownerLabel";
    public static final String OWNER_TEXT = "ownerText";
    public static final String GROUP_LABEL = "groupLabel";
    public static final String GROUP_TEXT = "groupText";
    public static final String ACCESS_AGE_LABEL = "accessAgeLabel";
    public static final String ACCESS_AGE_TEXT = "accessAgeText";
    public static final String STAGING_LABEL = "stagingLabel";
    public static final String STAGING_TEXT = "stagingText";
    public static final String RELEASING_LABEL = "releasingLabel";
    public static final String RELEASING_TEXT = "releasingText";

    // copy settings page
    public static final String ARCHIVE_AGE_LABEL = "archiveAgeLabel";
    public static final String ARCHIVE_AGE_TEXT = "archiveAgeText";
    public static final String UNARCHIVE_AGE_LABEL = "unarchiveAgeLabel";
    public static final String UNARCHIVE_AGE_TEXT = "unarchiveAgeText";
    public static final String RELEASE_OPTIONS_LABEL = "releaseOptionsLabel";
    public static final String RELEASE_OPTIONS_TEXT = "releaseOptionsText";

    // apply fs page
    public static final String APPLY_FS_LABEL = "applyFSLabel";
    public static final String APPLY_FS_TEXT = "applyFSText";

    // save page
    public static final String SAVE_METHOD_LABEL = "saveMethodLabel";
    public static final String SAVE_METHOD_TEXT = "saveMethodText";
    public static final String CHILD_ALERT = "Alert";

    private boolean display_error_page = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewCriteriaSummary(View parent, Model model) {
	    this(parent, model, PAGE_NAME);
    }

    public NewCriteriaSummary(View parent, Model model, String name) {
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
        registerChild(STARTING_DIR_LABEL, CCLabel.class);
        registerChild(STARTING_DIR_TEXT, CCStaticTextField.class);
        registerChild(NAME_PATTERN_LABEL, CCLabel.class);
        registerChild(NAME_PATTERN_TEXT, CCStaticTextField.class);
        registerChild(MIN_SIZE_LABEL, CCLabel.class);
        registerChild(MIN_SIZE_TEXT, CCStaticTextField.class);
        registerChild(MAX_SIZE_LABEL, CCLabel.class);
        registerChild(MAX_SIZE_TEXT, CCStaticTextField.class);
        registerChild(OWNER_LABEL, CCLabel.class);
        registerChild(OWNER_TEXT, CCStaticTextField.class);
        registerChild(GROUP_LABEL, CCLabel.class);
        registerChild(GROUP_TEXT, CCStaticTextField.class);
        registerChild(ACCESS_AGE_LABEL, CCLabel.class);
        registerChild(ACCESS_AGE_TEXT, CCStaticTextField.class);
        registerChild(STAGING_LABEL, CCLabel.class);
        registerChild(STAGING_TEXT, CCStaticTextField.class);
        registerChild(RELEASING_LABEL, CCLabel.class);
        registerChild(RELEASING_TEXT, CCStaticTextField.class);
        registerChild(ARCHIVE_AGE_LABEL, CCLabel.class);
        registerChild(ARCHIVE_AGE_TEXT, CCStaticTextField.class);
        registerChild(UNARCHIVE_AGE_LABEL, CCLabel.class);
        registerChild(UNARCHIVE_AGE_TEXT, CCStaticTextField.class);
        registerChild(RELEASE_OPTIONS_LABEL, CCLabel.class);
        registerChild(RELEASE_OPTIONS_TEXT, CCStaticTextField.class);
        registerChild(APPLY_FS_LABEL, CCLabel.class);
        registerChild(APPLY_FS_TEXT, CCStaticTextField.class);
        registerChild(SAVE_METHOD_LABEL, CCLabel.class);
        registerChild(SAVE_METHOD_TEXT, CCStaticTextField.class);
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(STARTING_DIR_LABEL) ||
            name.equals(NAME_PATTERN_LABEL) ||
            name.equals(MIN_SIZE_LABEL) ||
            name.equals(MAX_SIZE_LABEL) ||
            name.equals(OWNER_LABEL) ||
            name.equals(GROUP_LABEL) ||
            name.equals(ACCESS_AGE_LABEL) ||
            name.equals(STAGING_LABEL) ||
            name.equals(RELEASING_LABEL) ||
            name.equals(ARCHIVE_AGE_LABEL) ||
            name.equals(UNARCHIVE_AGE_LABEL) ||
            name.equals(RELEASE_OPTIONS_LABEL) ||
            name.equals(APPLY_FS_LABEL) ||
            name.equals(SAVE_METHOD_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(STARTING_DIR_TEXT) ||
                   name.equals(NAME_PATTERN_TEXT) ||
                   name.equals(MIN_SIZE_TEXT) ||
                   name.equals(MAX_SIZE_TEXT) ||
                   name.equals(OWNER_TEXT) ||
                   name.equals(GROUP_TEXT) ||
                   name.equals(ACCESS_AGE_TEXT) ||
                   name.equals(STAGING_TEXT) ||
                   name.equals(RELEASING_TEXT) ||
                   name.equals(ARCHIVE_AGE_TEXT) ||
                   name.equals(UNARCHIVE_AGE_TEXT) ||
                   name.equals(RELEASE_OPTIONS_TEXT) ||
                   name.equals(APPLY_FS_TEXT) ||
                   name.equals(SAVE_METHOD_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child = new CCAlertInline(this, name, null);
            child.setValue(CCAlertInline.TYPE_ERROR);
            return (View) child;
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
	    String url = "/jsp/archive/wizards/NewCriteriaSummary.jsp";
        if (display_error_page) {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        String t = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "NewCriteriaWizard.error.summary";
            display_error_page = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wm.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    display_error_page = false;
                } else {
                    display_error_page = true;
                }
            }

            if (display_error_page) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    getServerName());
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

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel)getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
