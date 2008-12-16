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

// ident	$Id: AddLibraryACSLSParamView.java,v 1.13 2008/12/16 00:12:15 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.RequestHandlingViewBase;

import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.web.ui.view.alert.CCAlertInline;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;

/**
 * A ContainerView object for the pagelet for defining ACSLS Parameters
 *
 */
public class AddLibraryACSLSParamView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "AddLibraryACSLSParamView";

    // Child view names (i.e. display fields).
    public static final String LABEL = "Label";
    public static final String ALERT = "Alert";
    public static final String STK_HOST_NAME = "STKHostName";
    public static final String ACSLS_PORT_NUMBER    = "ACSLSPortNumber";
    public static final String LIBRARY_NAME_VALUE   = "LibraryNameValue";
    public static final String SAVE_TO = "SaveTo";
    public static final String ACCESS_ID_VALUE = "AccessIDValue";
    public static final String SSI_HOST_VALUE  = "SSIHostValue";
    public static final String USE_SECURE_RPC  = "UseSecureRPC";
    public static final String SSI_INET_PORT_VALUE  = "SSIInetPortValue";
    public static final String CSI_HOST_PORT_VALUE  = "CSIHostPortValue";

    private boolean previousError = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AddLibraryACSLSParamView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public AddLibraryACSLSParamView(
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
        registerChild(LABEL, CCLabel.class);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(STK_HOST_NAME, CCStaticTextField.class);
        registerChild(ACSLS_PORT_NUMBER, CCStaticTextField.class);
        registerChild(LIBRARY_NAME_VALUE, CCTextField.class);
        registerChild(SAVE_TO, CCStaticTextField.class);
        registerChild(ACCESS_ID_VALUE, CCTextField.class);
        registerChild(SSI_HOST_VALUE, CCTextField.class);
        registerChild(USE_SECURE_RPC, CCCheckBox.class);
        registerChild(SSI_INET_PORT_VALUE, CCTextField.class);
        registerChild(CSI_HOST_PORT_VALUE, CCTextField.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());
        View child = null;
        if (name.equals(LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(ACSLS_PORT_NUMBER) ||
            name.equals(STK_HOST_NAME) ||
            name.equals(SAVE_TO)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(LIBRARY_NAME_VALUE) ||
            name.equals(ACCESS_ID_VALUE) ||
            name.equals(SSI_HOST_VALUE) ||
            name.equals(SSI_INET_PORT_VALUE) ||
            name.equals(CSI_HOST_PORT_VALUE)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(USE_SECURE_RPC)) {
            child = new CCCheckBox(
                this, name,
                Boolean.toString(true), Boolean.toString(false), false);
        } else if (name.equals(ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else {
            throw new IllegalArgumentException(
                "AddLibraryACSLSParamView : Invalid child name ["
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
            url = "/jsp/media/wizards/AddLibraryACSLSParam.jsp";
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
        populateTitleInformation(wizardModel);

        try {
            populateTextFields(wizardModel);
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "AddLibraryACSLSParamView - beginDisplay()",
                "Failed to populate ACSLS Library Parameter Information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                AddLibraryACSLSSelectLibraryView.CHILD_ALERT,
                "AddLibrary.acsls.selectlibrary.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        changeComponentState();
        TraceUtil.trace3("Exiting");
    }

    public String getErrorMsg() {
        TraceUtil.trace3("Entering");
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        String libraryName = (String) wizardModel.getValue(LIBRARY_NAME_VALUE);
        String accessID = (String) wizardModel.getValue(ACCESS_ID_VALUE);
        String SSIHost = (String) wizardModel.getValue(SSI_HOST_VALUE);
        String useSecureRPC = (String) wizardModel.getValue(USE_SECURE_RPC);
        String SSIInetPort = (String) wizardModel.getValue(SSI_INET_PORT_VALUE);
        String CSIHostPort = (String) wizardModel.getValue(CSI_HOST_PORT_VALUE);
        String acslsPort = (String) wizardModel.getValue(ACSLS_PORT_NUMBER);


        libraryName  = (libraryName == null)  ? "" : libraryName;
        accessID = (accessID == null) ? "" : accessID;
        SSIHost  = (SSIHost == null)  ? "" : SSIHost;
        useSecureRPC = (useSecureRPC == null) ? "" : useSecureRPC;
        SSIInetPort  = (SSIInetPort == null)  ? "" : SSIInetPort;
        CSIHostPort  = (CSIHostPort == null)  ? "" : CSIHostPort;

        // Only the libraryName field is mandatory
        if (libraryName.length() == 0) {
            return "AddLibrary.acsls.param.error.nolibraryname";
        }

        // Check if all input fields are space-free
        if (!(SamUtil.isValidString(libraryName) &&
            SamUtil.isValidString(accessID) &&
            SamUtil.isValidString(SSIHost) &&
            SamUtil.isValidString(SSIInetPort) &&
            SamUtil.isValidString(CSIHostPort))) {
            return "wizard.space.errMsg";
        }

        // Check if nameValue is strictly a letter and digit string if defined
        if (!SamUtil.isValidNonSpecialCharString(libraryName)) {
            return "AddLibrary.acsls.param.error.librarynamespecchar";
        }

        // catalogFile, eqValue, driveStartEQ, driveIncreEQ were taken out in
        // 4.6 as a part of simplication effort.

        // SSIInetPort is non-zero positive integer if it's entered
        try {
            if (SSIInetPort.length() != 0) {
                int ssiinetport = Integer.parseInt(SSIInetPort);
                if (ssiinetport <= 0) {
                    return "AddLibrary.acsls.param.error.invalidssiinetport";
                }
            }
        } catch (NumberFormatException e) {
            return "AddLibrary.acsls.param.error.invalidssiinetport";
        }

        // CSIHostPort is non-zero positive integer if it's entered
        try {
            if (CSIHostPort.length() != 0) {
                int csihostport = Integer.parseInt(CSIHostPort);
                if (csihostport <= 0) {
                    return "AddLibrary.acsls.param.error.invalidcsihostport";
                }
            }
        } catch (NumberFormatException e) {
            return "AddLibrary.acsls.param.error.invalidcsihostport";
        }

        TraceUtil.trace3("Exiting");
        return null;
    }

    /**
     * Enable the Secure RPC textfields if the check box is checked
     */
    private void changeComponentState() {
        if (((CCCheckBox) getChild(USE_SECURE_RPC)).isChecked()) {
            ((CCTextField) getChild(SSI_INET_PORT_VALUE)).setDisabled(false);
            ((CCTextField) getChild(CSI_HOST_PORT_VALUE)).setDisabled(false);
        }
    }

    /**
     * To populate the step title media type, acsls host & port, save setting to
     * field, etc.
     */
    private void populateTitleInformation(SamWizardModel wizardModel) {
        // acsls host name and port number are auto-populated
        ((CCStaticTextField) getChild(SAVE_TO)).setValue(
            new NonSyncStringBuffer(
                Constants.Media.DEFAULT_PARAM_LOCATION).append("<").
                append(SamUtil.getResourceString(
                    "LibrarySummary.heading.name")).append(">").toString());
    }

    /**
     * To populate all the text fields by retrieving the information from
     * the STK LIBRARY hash Map.
     */
    private void populateTextFields(SamWizardModel wizardModel)
        throws SamFSException {
        int stkLibraryIndex = ((Integer) wizardModel.getValue(
            AddLibraryImpl.SA_STK_LIBRARY_INDEX)).intValue();

        Library myLibrary = getStkLibrary(wizardModel, stkLibraryIndex);

        if (myLibrary == null) {
            throw new SamFSException(null, -2502);
        }

        // Setting the ACSLS Host Name
        ((CCStaticTextField) getChild(STK_HOST_NAME)).setValue(
            (String) wizardModel.getValue(
                AddLibrarySelectTypeView.ACSLS_HOST_NAME));

        StkNetLibParam param = myLibrary.getStkNetLibParam();

        if (myLibrary.getName() == null || myLibrary.getName().length() == 0) {
            // First time the GUI is working on this library param page
            ((CCTextField) getChild(LIBRARY_NAME_VALUE)).setValue(
                new NonSyncStringBuffer(Constants.Media.STK_LIBRARY_PREFIX).
                    append(SamUtil.replaceSpaceWithUnderscore(
                        SamUtil.getMediaTypeString(getMediaType(myLibrary)))).
                    toString());
        } else {
            ((CCTextField) getChild(LIBRARY_NAME_VALUE)).
                setValue(myLibrary.getName());
        }

        if (param == null) {
            // reset the rest of the fields to be empty
            ((CCTextField) getChild(ACCESS_ID_VALUE)).setValue("");
            ((CCTextField) getChild(SSI_HOST_VALUE)).setValue("");
            ((CCCheckBox) getChild(USE_SECURE_RPC)).setChecked(false);
            ((CCTextField) getChild(SSI_INET_PORT_VALUE)).setValue("");
            ((CCTextField) getChild(CSI_HOST_PORT_VALUE)).setValue("");

        } else {
            // STK Param Specific
            ((CCTextField) getChild(ACCESS_ID_VALUE)).
                setValue(param.getAccess());
            ((CCTextField) getChild(SSI_HOST_VALUE)).
                setValue(param.getSamServerName());

            String ssiInetPort = Integer.toString(param.getSamRecvPort());
            String csiHostPort = Integer.toString(param.getSamSendPort());
            int recvPort = param.getSamRecvPort();
            int sendPort = param.getSamSendPort();

            if (recvPort <= 0 && sendPort <= 0) {
                ((CCCheckBox) getChild(USE_SECURE_RPC)).setChecked(false);
                ((CCTextField) getChild(SSI_INET_PORT_VALUE)).setValue("");
                ((CCTextField) getChild(CSI_HOST_PORT_VALUE)).setValue("");
            } else {
                // Secure RPC Enabled
                ((CCCheckBox) getChild(USE_SECURE_RPC)).setChecked(true);
                ((CCTextField) getChild(SSI_INET_PORT_VALUE)).
                    setValue(Integer.toString(recvPort));
                ((CCTextField) getChild(CSI_HOST_PORT_VALUE)).
                    setValue(Integer.toString(sendPort));
            }
        }
    }

    /**
     * Helper method to retrieve the Library object from the ArrayList by
     * providing the index
     */
    private Library getStkLibrary(SamWizardModel wizardModel, int index)
        throws SamFSException {
        Library [] myStkLibraryArray = (Library [])
            wizardModel.getValue(AddLibraryImpl.SA_STK_LIBRARY_ARRAY);
        if (index > myStkLibraryArray.length - 1) {
            TraceUtil.trace1("Developers Bug found in Impl::getStkLibrary()");
            return null;
        } else {
            Library myLibrary = myStkLibraryArray[index];
            // Drive [] myDrives = myLibrary.getDrives();
            return myStkLibraryArray[index];
        }
    }

    /**
     * Helper function
     */
    private int getMediaType(Library myLibrary) throws SamFSException {
        Drive [] myDrives = myLibrary.getDrives();
        if (myDrives != null && myDrives.length != 0) {
            return myDrives[0].getEquipType();
        } else {
            throw new SamFSException(null, -2516);
        }
    }


    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        return (String) wizardModel.getValue(Constants.Wizard.SERVER_NAME);
    }

    private void setErrorMessageIfNeeded(SamWizardModel wizardModel) {
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

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previousError = false;
                } else {
                    previousError = true;
                }
            }

            if (previousError) {
                SamUtil.setErrorAlert(
                    this,
                    ALERT,
                    errorSummary,
                    code,
                    msgs,
                    getServerName());
            } else {
                SamUtil.setWarningAlert(
                    this,
                    ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }
}
