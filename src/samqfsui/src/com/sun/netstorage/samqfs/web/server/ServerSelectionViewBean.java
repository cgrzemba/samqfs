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

// ident	$Id: ServerSelectionViewBean.java,v 1.33 2008/04/16 17:07:26 ronaldso Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.util.HashMap;
import java.util.Map;
import javax.servlet.ServletContext;
import com.sun.netstorage.samqfs.web.util.SecurityManager;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;


/**
 *  This class is the view bean for the Server Selection page
 */

public class ServerSelectionViewBean extends ServerCommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ServerSelection";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/server/ServerSelection.jsp";
    private static final int TAB_NAME = ServerTabsUtil.SERVER_SELECTION_TAB;

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_CONTAINER_VIEW  = "ServerSelectionView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // Page children
    public static final String CHILD_STATICTEXT1 = "StaticText1";
    public static final String CHILD_STATICTEXT2 = "StaticText2";
    public static final String CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD =
        "ConfirmMessageHiddenField";

    // table models
    private Map models = null;

    /**
     * Constructor
     */
    public ServerSelectionViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL, TAB_NAME);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        setUserPermission();

        pageTitleModel = createPageTitleModel();
        registerChildren();
        initializeTableModels();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_STATICTEXT1, CCStaticTextField.class);
        registerChild(CHILD_STATICTEXT2, CCStaticTextField.class);
        registerChild(CHILD_CONTAINER_VIEW, ServerSelectionView.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD)) {
            child = new CCHiddenField(
                this,
                name,
                new StringBuffer(
                    SamUtil.getResourceString("ServerSelection.confirmMsg1")).
                    append("\n").append(
                    SamUtil.getResourceString("ServerSelection.confirmMsg2")).
                    toString());

        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new ServerSelectionView(this, models, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // Javascript used StaticTextField
        } else if (name.startsWith("StaticText")) {
            child = new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        return (View) child;
    }

    private void initializeTableModels() {
	models = new HashMap();
	ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

	// server table
	CCActionTableModel model = new CCActionTableModel(
            sc, "/jsp/server/ServerSelectionTable.xml");
	models.put(ServerSelectionView.SERVER_TABLE, model);
    }

    private CCPageTitleModel createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }

        return pageTitleModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        if (!ServerUtil.isClientBrowserSupported()) {
            TraceUtil.trace1("Unsupported browser detected!");
            SamUtil.setErrorAlert(
                this,
                ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
                "ServerSelection.error.unsupportedbrowser",
                -2400,
                null,
                "");
        } else {
            TraceUtil.trace3("Populating Host Table Model!...");
            // populate the action table model
            ServerSelectionView view =
                (ServerSelectionView) getChild(CHILD_CONTAINER_VIEW);
            try {
                view.populateTableModels();
            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "ServerSelectionViewBean()",
                    "Failed to populate server information",
                    "");
                SamUtil.setErrorAlert(
                    this,
                    ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
                    "ServerSelection.error.populate",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    "");
            }

            ((CCStaticTextField) getChild(CHILD_STATICTEXT1)).setValue(
                SamUtil.getResourceString(
                    "ServerSelection.help.1",
                    SamUtil.getResourceString("masthead.altText")));
            ((CCStaticTextField) getChild(CHILD_STATICTEXT2)).setValue(
                SamUtil.getResourceString(
                    "ServerSelection.help.2",
                    Constants.Symbol.DOT));
        }
        TraceUtil.trace3("Exiting");
    }

    private void setUserPermission() {
        // this call is enough to initialize user authorizations
        SecurityManager manager = SecurityManagerFactory.getSecurityManager();
    }
}
