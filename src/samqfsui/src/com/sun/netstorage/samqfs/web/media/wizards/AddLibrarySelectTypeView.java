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

// ident	$Id: AddLibrarySelectTypeView.java,v 1.17 2008/12/16 00:12:15 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;

/**
 * A ContainerView object for the pagelet for Select Type step (both)
 *
 */
public class AddLibrarySelectTypeView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "AddLibrarySelectTypeView";

    // Child view names (i.e. display fields).
    public static final String ALERT = "Alert";
    public static final String TEXT = "Text";
    public static final String LIBRARY_TYPE = "RadioType1";
    public static final String TYPE_ACSLS = "RadioType2";
    public static final String TYPE_NETWORK = "RadioType3";
    public static final String TYPE_LABEL = "LabelType";
    public static final String HOST_LABEL = "LabelHost";
    public static final String PORT_LABEL = "LabelPort";
    public static final String LIBRARY_DRIVER = "LibraryDriver";
    public static final String HIDDEN_MESSAGE = "HiddenMessage";
    public static final String ACSLS_HOST_NAME = "ACSLSHostName";
    public static final String ACSLS_PORT_NUMBER = "ACSLSPortNumber";

    private static OptionList radioOptions1 = new OptionList(
        new String [] {"AddLibrary.type.direct"},
        new String [] {"AddLibrary.type.direct"});

    private static OptionList radioOptions2 = new OptionList(
        new String [] {"AddLibrary.type.acsls"},
        new String [] {"AddLibrary.type.acsls"});

    private static OptionList radioOptions3 = new OptionList(
        new String [] {"AddLibrary.type.network"},
        new String [] {"AddLibrary.type.network"});

    // OptionList object for drop down menu
    private OptionList driverOptions = new OptionList(
        new String[] {
            "media.type.adicdas",
            "media.type.sonypetasite",
            "media.type.fujilmf",
            "media.type.ibm3494"
        },

        new String[] {
            Integer.toString(BaseDevice.MTYPE_ADIC_DAS),
            Integer.toString(BaseDevice.MTYPE_SONY_PETASITE),
            Integer.toString(BaseDevice.MTYPE_FUJ_LMF),
            Integer.toString(BaseDevice.MTYPE_IBM_3494)
        });

    private boolean previousError = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AddLibrarySelectTypeView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public AddLibrarySelectTypeView(View parent, Model model, String name) {
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
        registerChild(TEXT, CCStaticTextField.class);
        registerChild(HIDDEN_MESSAGE, CCHiddenField.class);
        registerChild(LIBRARY_TYPE, CCRadioButton.class);
        registerChild(LIBRARY_DRIVER, CCDropDownMenu.class);
        registerChild(TYPE_ACSLS, CCRadioButton.class);
        registerChild(TYPE_NETWORK, CCRadioButton.class);
        registerChild(TYPE_LABEL, CCLabel.class);
        registerChild(HOST_LABEL, CCLabel.class);
        registerChild(PORT_LABEL, CCLabel.class);
        registerChild(ACSLS_HOST_NAME, CCTextField.class);
        registerChild(ACSLS_PORT_NUMBER, CCTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(
            new StringBuffer("Entering: name is ").append(name).toString());

        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (name.equals(HIDDEN_MESSAGE)) {
            return new CCHiddenField(
                this, name, SamUtil.getResourceString("discovery.media"));
        } else if (name.equals(LIBRARY_TYPE)) {
            CCRadioButton myChild = new CCRadioButton(this, name, null);
            myChild.setOptions(radioOptions1);
            return (View) myChild;
        } else if (name.equals(TYPE_ACSLS)) {
            CCRadioButton myChild = new CCRadioButton(this, LIBRARY_TYPE, null);
            myChild.setOptions(radioOptions2);
            return (View) myChild;
        } else if (name.equals(TYPE_NETWORK)) {
            CCRadioButton myChild = new CCRadioButton(this, LIBRARY_TYPE, null);
            myChild.setOptions(radioOptions3);
            return (View) myChild;
        } else if (name.equals(LIBRARY_DRIVER)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(ACSLS_HOST_NAME) ||
            name.equals(ACSLS_PORT_NUMBER)) {
            return new CCTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "AddLibrarySelectTypeView : Invalid child name [" + name + "]");
        }
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
            url = "/jsp/media/wizards/AddLibrarySelectType.jsp";
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
        setErrorMessageIfNeeded(wizardModel);
        populateComponents();
        TraceUtil.trace3("Exiting");
    }


    /**
     * To populate the radio button group and drop down menu.  Enable/disable
     * the fellow text fields of the radio button
     */
    private void populateComponents() {
        CCTextField acslsHostName = (CCTextField) getChild(ACSLS_HOST_NAME);
        CCTextField acslsPortNum  = (CCTextField) getChild(ACSLS_PORT_NUMBER);
        CCRadioButton myRadio = (CCRadioButton) getChild(LIBRARY_TYPE);
        CCDropDownMenu driverMenu = (CCDropDownMenu) getChild(LIBRARY_DRIVER);
        driverMenu.setLabelForNoneSelected("AddLibrary.driver.noneselect");
        driverMenu.setOptions(driverOptions);

        // Enable corresponding text fields
        if (myRadio.getValue() != null) {
            if (myRadio.getValue().equals("AddLibrary.type.network")) {
                driverMenu.setDisabled(false);
                acslsHostName.setDisabled(true);
                acslsPortNum.setDisabled(true);
            } else if (myRadio.getValue().equals("AddLibrary.type.acsls")) {
                acslsHostName.setDisabled(false);
                acslsPortNum.setDisabled(false);
                driverMenu.setDisabled(true);
            } else {
                acslsHostName.setDisabled(true);
                acslsPortNum.setDisabled(true);
                driverMenu.setDisabled(true);
            }
        } else {
            myRadio.setValue("AddLibrary.type.direct");
            acslsHostName.setDisabled(true);
            acslsPortNum.setDisabled(true);
            driverMenu.setDisabled(true);
        }

        acslsHostName.resetStateData();
        acslsPortNum.resetStateData();

        TraceUtil.trace3("Exiting");
    }

    public String getErrorMsg() {
        TraceUtil.trace3("Entering");

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        String acslsHost = (String) wizardModel.getValue(ACSLS_HOST_NAME);
        String acslsPort = (String) wizardModel.getValue(ACSLS_PORT_NUMBER);

        acslsHost = (acslsHost == null) ? "" : acslsHost.trim();
        acslsPort = (acslsPort == null) ? "" : acslsPort.trim();

        wizardModel.setValue(ACSLS_HOST_NAME, acslsHost);
        wizardModel.setValue(ACSLS_PORT_NUMBER, acslsPort);

        String type   = (String) wizardModel.getValue(LIBRARY_TYPE);
        String driver = (String) wizardModel.getValue(LIBRARY_DRIVER);

        if (type == null) {
            // no radio button is selected
            return "AddLibrary.selecttype.errMsg.type";
        } else if (type.equals("AddLibrary.type.acsls")) {
            TraceUtil.trace3("Exiting");
            return validateACSLSFields(acslsHost, acslsPort);
        } else if (type.equals("AddLibrary.type.network")
            && driver.equals("")) {
            // other network attached driver
            return "AddLibrary.selecttype.errMsg.driver";
        }

        TraceUtil.trace3("Exiting");
        return null;
    }

    private void setErrorMessageIfNeeded(SamWizardModel wizardModel) {
        String serverName =
            (String) wizardModel.getValue(Constants.Wizard.SERVER_NAME);
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
                    ALERT,
                    errorSummary,
                    code,
                    msgs,
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    this,
                    ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }

    private String validateACSLSFields(String acslsHost, String acslsPort) {
        if ("".equals(acslsHost)) {
            ((CCLabel) getChild(HOST_LABEL)).setShowError(true);
            return "AddLibrary.selecttype.errMsg.acslshost.blank";
        } else if ("".equals(acslsPort)) {
            ((CCLabel) getChild(PORT_LABEL)).setShowError(true);
            return "AddLibrary.selecttype.errMsg.acslsport.blank";
        } else if (!SamUtil.isValidString(acslsHost)) {
            // no space is allowed in acsls host field
            ((CCLabel) getChild(HOST_LABEL)).setShowError(true);
            return "AddLibrary.selecttype.errMsg.acslshost.nospace";
        } else if (!SamUtil.isValidString(acslsPort)) {
            // no space is allowed in acsls port field
            ((CCLabel) getChild(PORT_LABEL)).setShowError(true);
            return "AddLibrary.selecttype.errMsg.acslsport.nospace";
        } else {
            try {
                int portNumber = Integer.parseInt(acslsPort);
                if (portNumber <= 0) {
                    // port number has to be a non-zero positive integer
                    ((CCLabel) getChild(PORT_LABEL)).setShowError(true);
                    return "AddLibrary.selecttype.errMsg.acslsport.positive";
                }
            } catch (NumberFormatException numEx) {
                // port number has to be a non-zero positive integer
                ((CCLabel) getChild(PORT_LABEL)).setShowError(true);
                return "AddLibrary.selecttype.errMsg.acslsport.positive";
            }
        }

        // if code reaches here, no error found
        return null;
    }
}
