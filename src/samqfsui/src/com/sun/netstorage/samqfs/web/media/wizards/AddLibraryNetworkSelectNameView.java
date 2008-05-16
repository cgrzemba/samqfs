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

// ident	$Id: AddLibraryNetworkSelectNameView.java,v 1.16 2008/05/16 18:38:57 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.RequestHandlingViewBase;

import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.web.ui.view.alert.CCAlertInline;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;

/**
 * A ContainerView object for the pagelet for Select Name Step (network)
 *
 */
public class AddLibraryNetworkSelectNameView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "AddLibraryNetworkSelectNameView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_NAME_FIELD  = "nameValue";
    public static final String CHILD_PARAM_FIELD = "paramValue";
    public static final String CHILD_ALERT = "Alert";

    private boolean previousError = false;
    private Library myLibrary = null;
    private Drive[] myDrives  = null;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AddLibraryNetworkSelectNameView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public AddLibraryNetworkSelectNameView(
        View parent,
        Model model,
        String name) {
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
        registerChild(CHILD_NAME_FIELD, CCTextField.class);
        registerChild(CHILD_PARAM_FIELD, CCTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer().append("Entering: name is ").
            append(name).toString());
        View child = null;
        if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_NAME_FIELD) ||
            name.equals(CHILD_PARAM_FIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else {
            throw new IllegalArgumentException(
                "AddLibraryNetworkSelectNameView : Invalid child name ["
                    + name + "]");
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
        String url = null;
        if (!previousError) {
            url = "/jsp/media/wizards/AddLibraryNetworkSelectName.jsp";
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
        String err =
            (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "AddLibrary.error.carryover";
            previousError = true;
            String errorDetails =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wizardModel.getValue(Constants.Wizard.ERROR_SUMMARY);

                previousError =
                    !errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT);
            }

            if (previousError) {
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

    public String getErrorMsg() {
        TraceUtil.trace3("Entering");
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        boolean isValid = true;
        String msgs = null;

        String nameValue =
            ((String) wizardModel.getValue(CHILD_NAME_FIELD)).trim();
        String paramValue =
            ((String) wizardModel.getValue(CHILD_PARAM_FIELD)).trim();

        // Check if nameValue is strictly a non special character string, except
        // underscore "_"

        // Check (1)
        if (nameValue.length() == 0 || paramValue.length() == 0) {
            isValid = false;
            msgs    = "AddLibraryNetwork.mandatorynotfilledin";
        }

        // Check if all input fields are space-free
        if (!(SamUtil.isValidString(nameValue) &&
            SamUtil.isValidString(paramValue))) {
            isValid = false;
            msgs    = "wizard.space.errMsg";
        }

        // Check (2)
        // Check if nameValue is strictly a letter and digit string if defined
        if (isValid && nameValue.length() != 0 &&
            !SamUtil.isValidNonSpecialCharString(nameValue)) {
            isValid = false;
            msgs    = "AddLibrary.error.invalidlibname";
        }

        // if everything is still ok, try to retrieve the library
        if (isValid) {
            try {
                getNetworkLibraryObject();
            } catch (SamFSException ex) {
                isValid = false;
                msgs    = "AddLibraryNetwork.page3.errMsg3";
            }
        }

        // if there are no problems getting the library object, now try
        // to retrieve the drives
        // Getting drives has to be done here because users may mistype
        // the location of the parameter file.  Instead of telling the users
        // they give the wrong parameter file string after clicking the FINISH
        // button, check here and give an error to the user when they click NEXT

        if (isValid) {
            try {
                myDrives = myLibrary.getDrives();
            } catch (SamFSException ex) {
                TraceUtil.trace1("EXCEPTION CAUGHT WHEN RETRIEVING DRIVES");
                isValid = false;
                msgs    = "AddLibraryNetwork.page3.errMsg4";
            }
        }

        TraceUtil.trace3("Exiting");
        return isValid ? null : msgs;
    }

    public Library getMyLibrary() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return myLibrary;
    }

    public Drive[] getMyDrives() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return myDrives;
    }

    private void getNetworkLibraryObject() throws SamFSException {
        TraceUtil.trace3("Entering");

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // No need to check the correctness of the field.  They have been
        // checked prior to the call of this function
        String nameValue =
            ((String) wizardModel.getValue(CHILD_NAME_FIELD)).trim();
        String catalogValue =
            Constants.Wizard.DEFAULT_CATALOG_LOCATION.concat(nameValue);
        String paramValue =
            ((String) wizardModel.getValue(CHILD_PARAM_FIELD)).trim();

        int libraryType  = Integer.parseInt((String) wizardModel.getValue(
            AddLibrarySelectTypeView.LIBRARY_DRIVER));

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        myLibrary =
            sysModel.getSamQFSSystemMediaManager().discoverNetworkLibrary(
                nameValue,
                libraryType,
                catalogValue,
                paramValue,
                -1);

        if (myLibrary == null) {
            throw new SamFSException(null, -2502);
        }

        wizardModel.setValue(AddLibraryImpl.MY_LIBRARY, myLibrary);
        TraceUtil.trace3("Exiting");
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        return (String) wizardModel.getValue(Constants.Wizard.SERVER_NAME);
    }
}
