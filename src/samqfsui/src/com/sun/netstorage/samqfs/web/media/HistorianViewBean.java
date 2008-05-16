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

// ident	$Id: HistorianViewBean.java,v 1.21 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;

import java.io.IOException;

import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Historian page
 */

public class HistorianViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "Historian";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/Historian.jsp";

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_CONTAINER_VIEW = "HistorianView";
    public static final String CHILD_BREADCRUMB = "BreadCrumb";
    public static final String CHILD_LIB_SUMMARY_HREF = "LibrarySummaryHref";
    public static final String CHILD_PAGEACTIONS_MENU = "PageActionsMenu";
    public static final
        String CHILD_PAGEACTIONS_MENU_HREF = "PageActionsMenuHref";
    public static final String CHILD_STATICTEXT = "StaticText";

    // server Name hidden field for pop up retrieval
    public static final
        String CHILD_SERVER_NAME_HIDDEN_FIELD = "ServerNameHiddenField";

    // Hidden field for confirm message
    public static final
        String CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD = "ConfirmMessageHiddenField";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel  = null;
    private CCBreadCrumbsModel breadCrumbsModel = null;

    /**
     * Constructor
     */
    public HistorianViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_CONTAINER_VIEW, HistorianView.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_LIB_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_PAGEACTIONS_MENU_HREF, CCHref.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_SERVER_NAME_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
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
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new HistorianView(this, name);
        // For server name hidden field
        } else if (name.equals(CHILD_SERVER_NAME_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name, getServerName());
        } else if (name.equals(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("media.confirm.unreserve.vsn")).
                    append("###").append(
                    SamUtil.getResourceString("media.confirm.export.vsn")).
                    toString());
        // Javascript StaticText
        } else if (name.equals(CHILD_STATICTEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel = new CCBreadCrumbsModel("Historian.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_LIB_SUMMARY_HREF) ||
            name.equals(CHILD_PAGEACTIONS_MENU_HREF)) {
            child = new CCHref(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
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
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/HistorianPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("LibrarySummaryHref");
        ViewBean targetViewBean = getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetViewBean.getName()),
            str);
        forwardTo(targetViewBean);

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSearchPageActionButtonRequest(
        RequestInvocationEvent event) throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        // VSN Search Page
        setPageSessionAttribute(
            Constants.PageSessionAttributes.PARENT,
            "HistorianViewBean");
        setPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME,
            Constants.MediaAttributes.HISTORIAN_NAME);

        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));
        forwardTo(getViewBean(VSNSearchViewBean.class));

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) {
        TraceUtil.trace3("Entering");

        // populate the action table model
        HistorianView view = (HistorianView) getChild(CHILD_CONTAINER_VIEW);

        try {
            view.populateData();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "HistorianViewBean()",
                "Failed to populate historian information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "Historian.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }
}
