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

// ident	$Id: AddLibraryDirectSelectLibraryView.java,v 1.16 2008/12/17 21:41:42 ronaldso Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.html.OptionList;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.html.CCTextField;

/**
 * A ContainerView object for the pagelet for select library step.(direct)
 *
 */
public class AddLibraryDirectSelectLibraryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "AddLibraryDirectSelectLibraryView";

    // Child view names (i.e. display fields).
    public static final String CHILD_DROPDOWNMENU = "DropDownMenu";
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_NAME_FIELD = "nameValue";
    public static final String CHILD_ERROR = "errorOccur";

    // hidden field to keep track of all library names for javascript use
    // Library name will be populated upon user selects a library in the menu
    public static final String LIBRARY_NAMES = "libraryNames";

    private boolean error = false, previousError = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AddLibraryDirectSelectLibraryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public AddLibraryDirectSelectLibraryView(
        View parent,
        Model model,
        String name) {

        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);

        try {
            prepareDropDownContent();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "AddLibraryDirectSelectLibraryView",
                "Failed to populate available libraries",
                getServerName());
            error = true;
            SamUtil.setErrorAlert(
                this,
                AddLibraryDirectSelectLibraryView.CHILD_ALERT,
                "AddLibraryDirect.error.populate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_DROPDOWNMENU, CCDropDownMenu.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(CHILD_NAME_FIELD, CCTextField.class);
        registerChild(LIBRARY_NAMES, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_DROPDOWNMENU)) {
            child = new CCDropDownMenu(this, name, null);
        } else if (name.equals(CHILD_NAME_FIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else if (name.equals(CHILD_ERROR) ||
            name.equals(LIBRARY_NAMES)) {
            child = new CCHiddenField(this, name, null);
        } else {
            TraceUtil.trace3("Exiting");
            throw new IllegalArgumentException(
                "AddLibraryDirectSelectLibraryView : Invalid child name ["
                        + name + "]");
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
        if (!previousError) {
            url = "/jsp/media/wizards/AddLibraryDirectSelectLibrary.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        if (error) {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.EXCEPTION);
        } else {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.SUCCESS);
        }

        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        String err = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs = (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "AddLibrary.error.carryover";
            previousError = true;

            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.EXCEPTION);

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
        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        String selectedValue = (String) wm.getValue(CHILD_DROPDOWNMENU);
        boolean isValid = true;
        String msgs = null;

        if (selectedValue == null) {
            isValid = false;
            msgs = "AddLibraryDirect.page2.errMsg";
        } else if (selectedValue.length() == 0) {
            isValid = false;
            msgs = "AddLibraryDirect.page2.errMsg";
        }

        String nameValue = (String) wm.getValue(CHILD_NAME_FIELD);
        nameValue = (nameValue == null) ? "" : nameValue;

        // Check all input fields if they contain any spaces
        if (!(SamUtil.isValidString(nameValue))) {
            isValid = false;
            msgs    = "wizard.space.errMsg";
        }

        // Check if nameValue is strictly a letter and digit string if defined
        if (isValid && nameValue.length() != 0 &&
            !SamUtil.isValidNameString(nameValue)) {
                isValid = false;
                // Share error message with Add Library Network path
                msgs    = "AddLibrary.error.invalidlibname";
        }

        TraceUtil.trace3("Exiting");
        return isValid ? null : msgs;
    }

    private void prepareDropDownContent() throws SamFSException {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        Library [] allLibrary =
            (Library []) wizardModel.getValue(
                AddLibraryImpl.SA_DISCOVERD_DA_LIBRARY_ARRAY);

        String [] label = new String[allLibrary.length];
        String [] data  = new String[allLibrary.length];

        StringBuffer buf = new StringBuffer();

        for (int i = 0; i < allLibrary.length; i++) {
            if (allLibrary[i] != null) {
                String vendor    = allLibrary[i].getVendor();
                String productID = allLibrary[i].getProductID();
                label[i] = vendor + ", " + productID;
                data[i]  = Integer.toString(i);

                if (buf.length() > 0) {
                    buf.append("###");
                }

                buf.append(SamUtil.replaceSpaceWithUnderscore(
                    allLibrary[i].getName()));
            }
        }

        ((CCHiddenField) getChild(LIBRARY_NAMES)).setValue(buf.toString());
        ((CCDropDownMenu) getChild(CHILD_DROPDOWNMENU)).
            setLabelForNoneSelected("AddLibrary.libnamedropdown");
        ((CCDropDownMenu) getChild(CHILD_DROPDOWNMENU)).
            setOptions(new OptionList(label, data));
    }

    private String getServerName() {
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        return (String) wm.getValue(Constants.Wizard.SERVER_NAME);
    }
}
