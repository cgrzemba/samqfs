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

// ident	$Id: RegistrationViewBean.java,v 1.8 2008/05/16 19:39:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;


import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.ProductRegistrationInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.propertysheet.CCPropertySheet;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.html.CCLabel;

import java.io.IOException;
import javax.servlet.ServletException;


/**
 * This class is the view bean for the Registration (CNS) of the SAM-QFS product
 * @version 1.0
 */

public class RegistrationViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME   = "Registration";
    private static final String DEFAULT_URL = "/jsp/admin/Registration.jsp";

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_PAGE_TITLE = "PageTitle";
    public static final String CHILD_PROPERTY_SHEET = "PropertySheet";
    public static final String CHILD_PROXY_HOSTNAME_TEXT = "proxyHostnameValue";
    public static final String CHILD_PROXY_HOSTPORT_TEXT = "proxyPortValue";
    public static final String CHILD_PROXY_UNAME_TEXT = "proxyUnameValue";
    public static final String CHILD_PROXY_PWD_TEXT = "proxyPwd";
    public static final String CHILD_UNAME_LABEL = "unameLabel";
    public static final String CHILD_CONTACTNAME_LABEL = "contactNameLabel";
    public static final String CHILD_CONTACTEMAIL_LABEL = "contactEmailLabel";
    public static final String CHILD_PROXY_UNAME_LABEL = "proxyUnameLabel";
    public static final
        String CHILD_PROXY_HOSTNAME_LABEL = "proxyHostnameLabel";
    public static final String CHILD_PROXY_HOSTPORT_LABEL = "proxyPortLabel";

    // hidden fields for prompts
    private static final String CHILD_HIDDEN_MESSAGES = "HiddenMessages";

    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel psheetModel = null;

    /**
     * Constructor
     */
    public RegistrationViewBean() {
        super(PAGE_NAME, DEFAULT_URL);
        psheetModel = new CCPropertySheetModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/RegistrationPropertySheet.xml");
        pageTitleModel = PageTitleUtil.createModel(
                "/jsp/util/CommonPageTitle.xml");

        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        registerChild(CHILD_PROPERTY_SHEET, CCPropertySheet.class);
        registerChild(CHILD_HIDDEN_MESSAGES, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        psheetModel.registerChildren(this);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {


        if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(CHILD_PROPERTY_SHEET)) {
            return new CCPropertySheet(this, psheetModel, name);

        } else if (psheetModel != null
            && psheetModel.isChildSupported(name)) {
            // Create child from property sheet model.
            return psheetModel.createChild(this, name);

        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {

            return PageTitleUtil.createChild(this, pageTitleModel, name);

        } else if (name.equals(CHILD_HIDDEN_MESSAGES)) {
            // TBD
            return new CCHiddenField(this, name, "");

        } else {
            throw new IllegalArgumentException(
            "Invalid child name [" + name + "]");
        }
    }

    /**
     * Called after the constructor is invoked.
     * It's not called when a handler is invoked.
     * @param event DisplayEvent
     * @throws ModelControlException
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        super.beginDisplay(event);
        // CommonPageTitle has both Submit and Cancel, hide Cancel button
        ((CCButton) getChild("Cancel")).setVisible(false);
        // set onclick action for Sumbit
        NonSyncStringBuffer extraHtml = new NonSyncStringBuffer();
        // extraHtml.append("onclick=\"return validate();\"");
        ((CCButton) getChild("Submit")).setExtraHtml(extraHtml.toString());
        populateDisplay();
    }

    /**
     * Handler for "Submit"
     */
    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        // If the required fields are not filled in, prompt the user
        String uName = (String)psheetModel.getValue("unameValue");
        String contactName = (String)psheetModel.getValue("contactNameValue");
        String email = (String)psheetModel.getValue("emailValue");

        boolean proxyEnabled =
            ((String)psheetModel.
                getValue("connectionTypeRadioButton")).
                    equalsIgnoreCase("proxy") ? true : false;

        try {
            // login, contactName or contactEmail is not provided
            if (uName == null || uName.trim().length() == 0 ||
                contactName == null || contactName.trim().length() == 0 ||
                email == null || email.trim().length() == 0) {

                throw new SamFSException("registration.error.incompleteInfo");

            }

            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            if (!proxyEnabled) {
                sysModel.getSamQFSSystemAdminManager().
                    registerProduct(
                        uName,
                        ((String)psheetModel.getValue("pwdValue")).getBytes(),
                        contactName,
                        email);
            } else {

                String proxyHost =
                        (String)psheetModel.getValue("proxyHostnameValue");
                String proxyPort =
                        (String)psheetModel.getValue("proxyPortValue");
                boolean proxyAuth =
                    "true".equals(
                        (String)psheetModel.getValue("proxyAuthValue"));
                String proxyUname =
                        (String)psheetModel.getValue("proxyUnameValue");

                if (proxyHost == null || proxyPort.trim().length() == 0 ||
                    proxyPort == null || proxyPort.trim().length() == 0 ||
                    (proxyAuth &&
                    (proxyUname == null || proxyUname.trim().length() == 0))) {

                    throw new SamFSException(
                        "registration.error.incompleteInfo");
                }

                if (proxyAuth) {
                    sysModel.getSamQFSSystemAdminManager().
                        registerProduct(
                            uName,
                            ((String)psheetModel.
                                getValue("pwdValue")).getBytes(),
                            contactName,
                            email,
                            proxyHost,
                            Integer.parseInt(proxyPort),
                            proxyUname,
                            ((String)psheetModel.
                                getValue("proxyPwd")).getBytes());
                } else {
                    sysModel.getSamQFSSystemAdminManager().
                        registerProduct(
                            uName,
                            ((String)psheetModel.
                                getValue("pwdValue")).getBytes(),
                            contactName,
                            email,
                            proxyHost,
                            Integer.parseInt(proxyPort));
                }
            }

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to register product");
            TraceUtil.trace1("Cause: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmit",
                "Failed to register product",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CHILD_COMMON_ALERT,
                "registration.error.register",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        forwardTo();
    }

    private void populateDisplay() {
        TraceUtil.trace3("Entering");

        if (psheetModel == null) {
            return;
        }
        psheetModel.clear();


        // fill the privacy statement always
        psheetModel.setValue(
            "purposeValue",
            SamUtil.getResourceString("registration.purpose.statement"));

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            ProductRegistrationInfo registration =
                sysModel.getSamQFSSystemAdminManager().getProductRegistration();

            psheetModel.setValue("unameValue", registration.getSunLogin());
            psheetModel.setValue("contactNameValue", registration.getName());
            psheetModel.setValue("emailValue", registration.getEmailAddress());
            if (registration.isProxyEnabled()) {

                psheetModel.setValue("connectionTypeRadioButton", "proxy");

                // enable the text fields
                ((CCTextField)
                    getChild(CHILD_PROXY_HOSTNAME_TEXT)).
                        setDisabled(false);
                ((CCTextField)
                    getChild(CHILD_PROXY_HOSTPORT_TEXT)).
                        setDisabled(false);

                psheetModel.setValue("proxyHostnameValue",
                    registration.getProxyHostname());
                psheetModel.setValue("proxyPortValue",
                    String.valueOf(registration.getProxyPort()));
                if (registration.isProxyAuth()) {
                    psheetModel.setValue("proxyAuthValue", "true");
                    psheetModel.setValue("proxyUnameValue",
                                        registration.getProxyUser());
                    // enable the text fields
                    ((CCTextField)
                        getChild(CHILD_PROXY_UNAME_TEXT)).
                            setDisabled(false);
                    ((CCTextField)
                        getChild(CHILD_PROXY_PWD_TEXT)).
                            setDisabled(false);

                }
            } else {
                psheetModel.setValue("connectionTypeRadioButton", "direct");

            }
            if (registration.isRegistered()) {
                // Set an info alert

                SamUtil.setInfoAlert(
                    getParentViewBean(),
                    CHILD_COMMON_ALERT,
                    "registration.info.productregistered",
                    "",
                    getServerName());
            }

        } catch (SamFSException samEx) {

            TraceUtil.trace1(
                "Failed to retrieve product registration information.");
            TraceUtil.trace1("Cause: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "loadPropertySheet",
                "Failed to retrieve product registration information",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CHILD_COMMON_ALERT,
                "Registration.error.retrieve",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }
}
