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

// ident	$Id: VersionHighlightViewBean.java,v 1.10 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCImageField;
import com.sun.web.ui.view.html.CCStaticTextField;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Version Highlight page
 */

public class VersionHighlightViewBean extends ServerCommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "VersionHighlight";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/server/VersionHighlight.jsp";
    private static final int TAB_NAME = ServerTabsUtil.SERVER_SELECTION_TAB;

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_CONTAINER_VIEW  = "VersionHighlightView";
    public static final String CHILD_SERVER_HREF = "ServerSelectionHref";

    // For legend
    public static final String CHILD_TEXT = "Text";
    public static final String CHILD_IMAGE = "Image";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCBreadCrumbsModel breadCrumbsModel = null;

    // For Breadcrumb
    public static final String CHILD_BREADCRUMB = "BreadCrumb";

    // table models
    private Map models = null;

    /**
     * Constructor
     */
    public VersionHighlightViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL, TAB_NAME);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
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
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_CONTAINER_VIEW, VersionHighlightView.class);
        registerChild(CHILD_SERVER_HREF, CCHref.class);
        registerChild(CHILD_IMAGE, CCImageField.class);

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
        if (name.equals(CHILD_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_IMAGE)) {
            child = new CCImageField(this, name, null);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("VersionHighlight.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_SERVER_HREF)) {
            child = new CCHref(this, name, null);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new VersionHighlightView(this, models, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
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
        // populate the action table model
        VersionHighlightView view =
            (VersionHighlightView) getChild(CHILD_CONTAINER_VIEW);
        try {
            view.populateTableModels();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "VersionHighlightViewBean()",
                "Failed to populate version highlight",
                "");
            SamUtil.setErrorAlert(
                this,
                ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
                "VersionHighlight.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                "");
        }
        TraceUtil.trace3("Exiting");
    }


    private void initializeTableModels() {
        models = new HashMap();
        ServletContext sc =
                RequestManager.getRequestContext().getServletContext();

        // copy settings table
        CCActionTableModel model = new CCActionTableModel(
                sc, "/jsp/server/VersionHighlightTable.xml");
        models.put(VersionHighlightView.VERSION_TABLE, model);
    }

    /**
     * Handler function in breadcrumb link
     *
     * @param event RequestInvocation Event
     */
    public void handleServerSelectionHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("ServerSelectionHref");
        ViewBean targetView = getViewBean(ServerSelectionViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        targetView.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }
}
