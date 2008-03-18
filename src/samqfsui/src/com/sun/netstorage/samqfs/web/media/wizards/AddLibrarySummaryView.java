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

// ident	$Id: AddLibrarySummaryView.java,v 1.8 2008/03/17 14:43:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Constants;

import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.wizard.CCWizardPage;

import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;

/**
 * A ContainerView object for the pagelet
 *
 */
public class AddLibrarySummaryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "AddLibrarySummaryView";

    // Child view names (i.e. display fields).

    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_VENDORID_FIELD = "vendorIDValue";
    public static final String CHILD_PRODUCTID_FIELD = "productIDValue";
    public static final String CHILD_MANAGETOOL_FIELD = "ManagementTool";
    public static final String CHILD_LIBRARY_COUNT = "LibraryCount";
    public static final String CHILD_LIBRARY_SERIAL_NUM = "LibrarySerialNumber";
    public static final String CHILD_PARAM_FIELD = "paramValue";
    public static final String CHILD_ATTACHED_FIELD = "attachedValue";
    public static final String CHILD_NAME_FIELD = "nameValue";
    public static final String CHILD_MEDIA_TYPE_FIELD = "mediaTypeValue";
    public static final String CHILD_ALERT = "Alert";
    public static final String ACSLS_HOST_NAME = "ACSLSHostName";
    public static final String ACSLS_PORT_NUMBER = "ACSLSPortNumber";
    public static final String ACCESS_ID_VALUE = "AccessIDValue";
    public static final String SSI_HOST_VALUE  = "SSIHostValue";
    public static final String USE_SECURE_RPC_VALUE  = "UseSecureRPCValue";
    public static final String SSI_INET_PORT_VALUE  = "SSIInetPortValue";
    public static final String CSI_HOST_PORT_VALUE  = "CSIHostPortValue";
    private boolean previousError = false;

    private static final int DIRECT_ATTACHED  = 0;
    private static final int NETWORK_ATTACHED = 1;
    private static final int ACSLS = 2;


    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AddLibrarySummaryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public AddLibrarySummaryView(View parent, Model model, String name) {
        super(parent, name);
        // The wizard framework will call this constructor with
        // the model that was returned from the Wizard method
        // getPageModel(currentPageId). If the wizard does not
        // return a model, an instance of DefaultModel will be
        // returned.
        //
        // Alternatively the application can set any Model
        // here.

        // There are still issues with linking WizardState
        // and the individual wizard pages
        // If extend is required by the framework then
        // this can be part of CCWizardPage
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
        registerChild(CHILD_VENDORID_FIELD, CCStaticTextField.class);
        registerChild(CHILD_PRODUCTID_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MANAGETOOL_FIELD, CCStaticTextField.class);
        registerChild(CHILD_PARAM_FIELD, CCStaticTextField.class);
        registerChild(CHILD_NAME_FIELD, CCStaticTextField.class);
        registerChild(CHILD_ATTACHED_FIELD, CCStaticTextField.class);
        registerChild(CHILD_MEDIA_TYPE_FIELD, CCStaticTextField.class);
        registerChild(CHILD_LIBRARY_COUNT, CCStaticTextField.class);
        registerChild(CHILD_LIBRARY_SERIAL_NUM, CCStaticTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(ACSLS_HOST_NAME, CCStaticTextField.class);
        registerChild(ACSLS_PORT_NUMBER, CCStaticTextField.class);
        registerChild(ACCESS_ID_VALUE, CCStaticTextField.class);
        registerChild(SSI_HOST_VALUE, CCStaticTextField.class);
        registerChild(USE_SECURE_RPC_VALUE, CCStaticTextField.class);
        registerChild(SSI_INET_PORT_VALUE, CCStaticTextField.class);
        registerChild(CSI_HOST_PORT_VALUE, CCStaticTextField.class);
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
        } else if (name.equals(CHILD_VENDORID_FIELD) ||
            name.equals(CHILD_MANAGETOOL_FIELD) ||
            name.equals(CHILD_PARAM_FIELD) ||
            name.equals(CHILD_PRODUCTID_FIELD) ||
            name.equals(CHILD_ATTACHED_FIELD) ||
            name.equals(CHILD_NAME_FIELD) ||
            name.equals(CHILD_MEDIA_TYPE_FIELD) ||
            name.equals(CHILD_LIBRARY_COUNT) ||
            name.equals(CHILD_LIBRARY_SERIAL_NUM) ||
            name.equals(ACSLS_HOST_NAME) ||
            name.equals(ACSLS_PORT_NUMBER) ||
            name.equals(ACCESS_ID_VALUE) ||
            name.equals(SSI_HOST_VALUE) ||
            name.equals(USE_SECURE_RPC_VALUE) ||
            name.equals(SSI_INET_PORT_VALUE) ||
            name.equals(CSI_HOST_PORT_VALUE)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "AddLibrarySummaryView : Invalid child name ["
                    + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return child;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardBody methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        if (!previousError) {
            TraceUtil.trace3("Exiting");
            switch (getAttachType()) {
                case DIRECT_ATTACHED:
                    return "/jsp/media/wizards/AddLibraryDirectSummary.jsp";
                case NETWORK_ATTACHED:
                    return "/jsp/media/wizards/AddLibraryNetworkSummary.jsp";
                case ACSLS:
                    return "/jsp/media/wizards/AddLibraryACSLSSummary.jsp";
                default:
                    return "/jsp/util/WizardErrorPage.jsp";
            }
        }

        TraceUtil.trace3("Exiting");
        return "/jsp/util/WizardErrorPage.jsp";
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        setErrorIfNeeded(wm);

        if (getAttachType() == ACSLS) {
            // Fill in the fields for ACSLS Libraries
            // Number of Virtual libraries that are going to be added
            Library [] stkLibs =
                (Library []) wm.getValue(AddLibraryImpl.SA_STK_LIBRARY_ARRAY);
            wm.setValue(
                AddLibrarySummaryView.CHILD_LIBRARY_COUNT,
                Integer.toString(stkLibs.length));

            // serial number
            wm.setValue(
                AddLibrarySummaryView.CHILD_LIBRARY_SERIAL_NUM,
                (String) wm.getValue(AddLibraryImpl.SA_ACSLS_SERIAL_NO));

            NonSyncStringBuffer myBuffer = new NonSyncStringBuffer();
            // Library name
            try {
                for (int i = 0; i < stkLibs.length; i++) {
                    if (myBuffer.length() > 0) {
                        myBuffer.append(", ");
                    }
                    myBuffer.append(stkLibs[i].getName());
                }
            } catch (SamFSException samEx) {
                wm.setValue(
                    AddLibrarySummaryView.CHILD_NAME_FIELD, "");
            }
            wm.setValue(
                AddLibrarySummaryView.CHILD_NAME_FIELD,
                myBuffer.toString());

            // Media Type
            String [] mediaTypes =
                (String []) wm.getValue(AddLibraryImpl.SA_STK_MEDIA_TYPES);
            myBuffer = null;
            myBuffer = new NonSyncStringBuffer();
            for (int i = 0; i < mediaTypes.length; i++) {
                if (myBuffer.length() > 0) {
                    myBuffer.append(", ");
                }
                myBuffer.append(mediaTypes[i]);
            }
            wm.setValue(
                AddLibrarySummaryView.CHILD_MEDIA_TYPE_FIELD,
                myBuffer.toString());

            boolean enabled =
                Boolean.toString(true).equals(
                (String) wm.getValue(AddLibraryACSLSParamView.USE_SECURE_RPC));
            ((CCStaticTextField) getChild(USE_SECURE_RPC_VALUE)).setValue(
                enabled ?
                    SamUtil.getResourceString("samqfsui.yes") :
                    SamUtil.getResourceString("samqfsui.no"));
        } else if (getAttachType() == NETWORK_ATTACHED) {
            int managementTool = Integer.parseInt((String)
                wm.getValue(AddLibrarySelectTypeView.LIBRARY_DRIVER));
            wm.setValue(
                CHILD_MANAGETOOL_FIELD,
                SamUtil.getMediaTypeString(managementTool));
        }
        TraceUtil.trace3("Exiting");
    }

    private void setErrorIfNeeded(SamWizardModel wm) {
        String serverName = (String) wm.getValue(Constants.Wizard.SERVER_NAME);
        String err = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);

        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs = (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "AddLibrary.error.carryover";
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
    }

    private int getAttachType() {
        String attached = (String)
            ((SamWizardModel) getDefaultModel()).getValue(
                AddLibrarySelectTypeView.LIBRARY_TYPE);
        if (attached.equals("AddLibrary.type.direct")) {
            return DIRECT_ATTACHED;
        } else if (attached.equals("AddLibrary.type.acsls")) {
            return ACSLS;
        } else {
            return NETWORK_ATTACHED;
        }
    }
}
