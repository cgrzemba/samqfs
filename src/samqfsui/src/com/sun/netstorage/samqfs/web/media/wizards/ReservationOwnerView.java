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

// ident	$Id: ReservationOwnerView.java,v 1.16 2008/03/17 14:43:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.html.OptionList;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;

import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.web.ui.view.alert.CCAlertInline;

/**
 * A ContainerView object for the pagelet
 *
 */
public class ReservationOwnerView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "ReservationOwnerView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_RADIO_BUTTON1  = "RadioButton1";
    public static final String CHILD_RADIO_BUTTON2  = "RadioButton2";
    public static final String CHILD_RADIO_BUTTON3  = "RadioButton3";
    public static final String CHILD_OWNER_TEXT = "ownerText";
    public static final String CHILD_OWNER_FIELD = "ownerValue";
    public static final String CHILD_GROUP_TEXT = "groupText";
    public static final String CHILD_GROUP_FIELD = "groupValue";
    public static final String CHILD_DIR_TEXT = "dirText";
    public static final String CHILD_DIR_FIELD = "dirValue";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";

    private static OptionList radioOptions1 = new OptionList(
        new String [] {""}, // empty label
        new String [] {"ReservationOwner.radio1"});

    private static OptionList radioOptions2 = new OptionList(
        new String [] {""}, // empty label
        new String [] {"ReservationOwner.radio2"});

    private static OptionList radioOptions3 = new OptionList(
        new String [] {""}, // empty label
        new String [] {"ReservationOwner.radio3"});

    private boolean error = false;


    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public ReservationOwnerView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public ReservationOwnerView(View parent, Model model, String name) {
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
        registerChild(CHILD_RADIO_BUTTON1, CCRadioButton.class);
        registerChild(CHILD_RADIO_BUTTON2, CCRadioButton.class);
        registerChild(CHILD_RADIO_BUTTON3, CCRadioButton.class);
        registerChild(CHILD_OWNER_TEXT, CCStaticTextField.class);
        registerChild(CHILD_GROUP_TEXT, CCStaticTextField.class);
        registerChild(CHILD_DIR_TEXT, CCStaticTextField.class);
        registerChild(CHILD_OWNER_FIELD, CCTextField.class);
        registerChild(CHILD_GROUP_FIELD, CCTextField.class);
        registerChild(CHILD_DIR_FIELD, CCTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer().append("Entering: name is ").
            append(name).toString());
        View child = null;
        if (name.equals(CHILD_OWNER_TEXT) ||
            name.equals(CHILD_GROUP_TEXT) ||
            name.equals(CHILD_DIR_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_DIR_FIELD) ||
            name.equals(CHILD_OWNER_FIELD) ||
            name.equals(CHILD_GROUP_FIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_RADIO_BUTTON1)) {
            CCRadioButton myChild = new CCRadioButton(this, name, null);
            myChild.setOptions(radioOptions1);
            child = (View) myChild;
        } else if (name.equals(CHILD_RADIO_BUTTON2)) {
            CCRadioButton myChild =
                new CCRadioButton(this, CHILD_RADIO_BUTTON1, null);
            myChild.setOptions(radioOptions2);
            child = (View) myChild;
        } else if (name.equals(CHILD_RADIO_BUTTON3)) {
            CCRadioButton myChild =
                new CCRadioButton(this, CHILD_RADIO_BUTTON1, null);
            myChild.setOptions(radioOptions3);
            child = (View) myChild;
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ERROR)) {
            if (error) {
                child = new CCHiddenField(this, name,
                    Constants.Wizard.EXCEPTION);
            } else {
                child = new CCHiddenField(this, name,
                    Constants.Wizard.SUCCESS);
            }
        } else {
            throw new IllegalArgumentException(
                "ReservationOwnerView : Invalid child name [" + name + "]");
        }
        TraceUtil.trace3("Exiting");
        return child;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardPage methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return "/jsp/media/wizards/ReservationOwner.jsp";
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        boolean previousError = false;

        if (error) {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.EXCEPTION);
        } else {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.SUCCESS);
        }

        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        String serverName = (String) wm.getValue(Constants.Wizard.SERVER_NAME);
        String err = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);

        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            ((CCHiddenField)
                getChild(CHILD_ERROR)).setValue(Constants.Wizard.EXCEPTION);

            String msgs = (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "Reservation.error.carryover";
            previousError = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wm.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previousError = false;
                } else {
                    previousError = true;
                }
            }

            if (previousError) {
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

        enableComponents();
        TraceUtil.trace3("Exiting");
    }

    private void enableComponents() {
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        String selection = (String) wm.getValue(CHILD_RADIO_BUTTON1);

        // Enable the corresponding textfield
        if (selection != null) {
            if (selection.equals("ReservationOwner.radio1")) {
                ((CCTextField) getChild(CHILD_OWNER_FIELD)).setDisabled(false);
                ((CCTextField) getChild(CHILD_GROUP_FIELD)).setDisabled(true);
                ((CCTextField) getChild(CHILD_DIR_FIELD)).setDisabled(true);

                String ownerField = (String) wm.getValue(CHILD_OWNER_FIELD);
                CCTextField ownerValue = (CCTextField)
                    getChild(CHILD_OWNER_FIELD);
                ownerValue.setValue(ownerField);
                ownerValue.resetStateData();
            } else if (selection.equals("ReservationOwner.radio2")) {
                ((CCTextField) getChild(CHILD_GROUP_FIELD)).setDisabled(false);
                ((CCTextField) getChild(CHILD_OWNER_FIELD)).setDisabled(true);
                ((CCTextField) getChild(CHILD_DIR_FIELD)).setDisabled(true);

                String groupField = (String) wm.getValue(CHILD_GROUP_FIELD);
                CCTextField groupValue = (CCTextField)
                    getChild(CHILD_GROUP_FIELD);
                groupValue.setValue(groupField);
                groupValue.resetStateData();
            } else if (selection.equals("ReservationOwner.radio3")) {
                ((CCTextField) getChild(CHILD_DIR_FIELD)).setDisabled(false);
                ((CCTextField) getChild(CHILD_GROUP_FIELD)).setDisabled(true);
                ((CCTextField) getChild(CHILD_OWNER_FIELD)).setDisabled(true);

                String dirField = (String) wm.getValue(CHILD_DIR_FIELD);
                CCTextField dirValue = (CCTextField)
                    getChild(CHILD_DIR_FIELD);
                dirValue.setValue(dirField);
                dirValue.resetStateData();
            }
        } else {
            ((CCTextField) getChild(CHILD_OWNER_FIELD)).setDisabled(true);
            ((CCTextField) getChild(CHILD_GROUP_FIELD)).setDisabled(true);
            ((CCTextField) getChild(CHILD_DIR_FIELD)).setDisabled(true);
        }
    }
}
