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

// ident	$Id: GrowWizardStripedGroupNumberPageView.java,v 1.10 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

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
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for strip group number page of
 * the Grow File System Wizard.
 *
 */
public class GrowWizardStripedGroupNumberPageView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String
        PAGE_NAME = "GrowWizardStripedGroupNumberPageView";

    public static final String
        CHILD_NUM_OF_STRIPED_GROUP_LABEL = "numOfStripedGroupLabel";
    public static final String
        CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD = "numOfStripedGroupTextField";

    public static final String CHILD_ALERT = "Alert";

    private boolean previous_error = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public GrowWizardStripedGroupNumberPageView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public GrowWizardStripedGroupNumberPageView(
        View parent, Model model, String name) {

        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
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
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_NUM_OF_STRIPED_GROUP_LABEL, CCLabel.class);
        registerChild(CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD, CCTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;
        if (name.equals(CHILD_NUM_OF_STRIPED_GROUP_TEXTFIELD)) {
            child = (View)new CCTextField(this, name, null);
        } else if (name.equals(CHILD_NUM_OF_STRIPED_GROUP_LABEL)) {
            child = (View) new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child1 = new CCAlertInline(this, name, null);
            child1.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) child1;
        } else {
            throw new IllegalArgumentException(
                "GrowWizardStripedGroupNumberPageView : Invalid child name [" +
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
        String url = null;
        if (!previous_error) {
            url = "/jsp/fs/GrowWizardStripedGroupNumberPage.jsp";
        } else {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        CCLabel stripedGroupLabel =
            (CCLabel) getChild(CHILD_NUM_OF_STRIPED_GROUP_LABEL);

        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        // reset the state of allocation radiobutton
        // selectedValue can be eithor "null', "qfs', 'fs'
        // first time load the step, selectedValue == null
        // 2nd time(previous button), selectedValue == qfs/fs
        // if selectedValue == qfs, allocation == single/dual/striped
        //    stripedGroupDropDown is enabled only when allocation == striped
        // else if selectedValue == fs,
        // reset the 2nd radio == "" and disabled it
        Integer availableGroups = (Integer)
            wm.getValue(Constants.Wizard.AVAILABLE_STRIPED_GROUPS);
        stripedGroupLabel.setValue(SamUtil.getResourceString(
            "FSWizard.grow.numOfStripedGroup",
            new String[] { availableGroups.toString() }));

        String t = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "FSWizard.new.error.steps";
            previous_error = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary =
                    (String) wm.getValue(Constants.Wizard.ERROR_SUMMARY);

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
                    (String) wm.getValue(Constants.Wizard.SERVER_NAME));
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
}
