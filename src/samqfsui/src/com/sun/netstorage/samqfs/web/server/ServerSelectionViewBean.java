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

// ident	$Id: ServerSelectionViewBean.java,v 1.32 2008/03/17 14:43:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
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
import com.sun.web.ui.view.html.CCImageField;
import com.sun.web.ui.view.html.CCLabel;
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

    public static final String CHILD_IMAGE = "Image";

    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_AVAILABLE_SERVERS  = "AvailableServers";
    public static final String CHILD_DISK_CACHE = "DiskCache";
    public static final
        String CHILD_DISK_CACHE_AVAILABLE = "DiskCacheAvailable";
    public static final String CHILD_TOTAL_FS = "TotalFS";


    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // child that used in javascript confirm messages
    public static final String CHILD_STATICTEXT = "StaticText";
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
        TraceUtil.trace3("Entering");
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_IMAGE, CCImageField.class);
        registerChild(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_CONTAINER_VIEW, ServerSelectionView.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_AVAILABLE_SERVERS, CCStaticTextField.class);
        registerChild(CHILD_DISK_CACHE, CCStaticTextField.class);
        registerChild(CHILD_DISK_CACHE_AVAILABLE, CCStaticTextField.class);
        registerChild(CHILD_TOTAL_FS, CCStaticTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_AVAILABLE_SERVERS) ||
            name.equals(CHILD_DISK_CACHE) ||
            name.equals(CHILD_DISK_CACHE_AVAILABLE) ||
            name.equals(CHILD_TOTAL_FS)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD)) {
            child = new CCHiddenField(
                this,
                name,
                new NonSyncStringBuffer(
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
        } else if (name.equals(CHILD_STATICTEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_IMAGE)) {
            child = new CCImageField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
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
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        if (!ServerUtil.isClientBrowserSupported()) {
            SamUtil.setErrorAlert(
                this,
                ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
                "ServerSelection.error.unsupportedbrowser",
                -2400,
                null,
                "");
        } else {
            // populate the action table model
            ServerSelectionView view =
                (ServerSelectionView) getChild(CHILD_CONTAINER_VIEW);
            try {
                view.populateTableModels();
                populateCapacitySummary();
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
        }
        TraceUtil.trace3("Exiting");
    }

    private void setUserPermission() {
        // this call is enough to initialize user authorizations
        SecurityManager manager = SecurityManagerFactory.getSecurityManager();
    }

    private void populateCapacitySummary() throws SamFSException {
        // Grab information from page session that were set by the View

        String capacitySummary =
            (String) this.getPageSessionAttribute(
                Constants.ServerAttributes.CAPACITY_SUMMARY);

        String [] info = capacitySummary.split("###");

        if (info.length != 4) {
            // Internal Error
            throw new SamFSException(null, -2551);
        } else {
            ((CCStaticTextField) getChild(CHILD_AVAILABLE_SERVERS)).
                setValue(info[0]);
            ((CCStaticTextField) getChild(CHILD_DISK_CACHE)).
                setValue(ServerUtil.generateNumberWithUnitString(info[1]));
            ((CCStaticTextField) getChild(CHILD_DISK_CACHE_AVAILABLE)).
                setValue(ServerUtil.generateNumberWithUnitString(info[2]));
            ((CCStaticTextField) getChild(CHILD_TOTAL_FS)).
                setValue(info[3]);
        }
    }
}
