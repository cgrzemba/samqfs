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

// ident	$Id: VSNSearchResultViewBean.java,v 1.21 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;
import com.iplanet.jato.model.ModelControlException;
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
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;

import java.io.IOException;

import javax.servlet.ServletException;

/**
 *  This class is the view bean for the VSN Search Result page
 */

public class VSNSearchResultViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "VSNSearchResult";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/VSNSearchResult.jsp";

    // cc components from the corresponding jsp page(s)...
    private static final String CHILD_CONTAINER_VIEW = "VSNSearchResultView";

    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_SEARCH_TEXT = "SearchText";

    public static final String CHILD_BREADCRUMB  = "BreadCrumb";
    public static final String CHILD_VSN_SEARCH_HREF    = "VSNSearchHref";
    public static final String CHILD_LIB_SUMMARY_HREF   = "LibrarySummaryHref";
    public static final String CHILD_LIB_DETAILS_HREF   = "LibraryDetailsHref";
    public static final String CHILD_VSN_SUMMARY_HREF   = "VSNSummaryHref";
    public static final String CHILD_HISTORIAN_HREF = "HistorianHref";

    // BreadCrumb use
    private CCBreadCrumbsModel breadCrumbsModel = null;
    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    /**
     * Constructor
     */
    public VSNSearchResultViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        registerChild(CHILD_CONTAINER_VIEW, VSNSearchResultView.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_SEARCH_TEXT, CCStaticTextField.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_VSN_SEARCH_HREF, CCHref.class);
        registerChild(CHILD_LIB_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_LIB_DETAILS_HREF, CCHref.class);
        registerChild(CHILD_VSN_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_HISTORIAN_HREF, CCHref.class);
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
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("VSNSearchResult.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_VSN_SEARCH_HREF) ||
            name.equals(CHILD_LIB_SUMMARY_HREF) ||
            name.equals(CHILD_LIB_DETAILS_HREF) ||
            name.equals(CHILD_VSN_SUMMARY_HREF) ||
            name.equals(CHILD_HISTORIAN_HREF)) {
            child = new CCHref(this, name, null);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new VSNSearchResultView(this, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_SEARCH_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String searchString = getSearchString();

        // populate the search String statictextfield
        ((CCStaticTextField)
            getChild(CHILD_SEARCH_TEXT)).setValue(searchString);

        // populate the action table model
        VSNSearchResultView view = (VSNSearchResultView)
            getChild(CHILD_CONTAINER_VIEW);

        try {
            view.populateData(
                getServerName(),
                (String) getParentViewBean().getPageSessionAttribute(
                    Constants.PageSessionAttributes.PARENT),
                getSearchString(),
                (String) getParentViewBean().getPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME));
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "VSNSearchResultViewBean()",
                "Failed to populate vsn search results",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "VSNSearchResult.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handlers for Breadcrumb links
     */
    public void handleVSNSearchHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(CHILD_VSN_SEARCH_HREF);
        ViewBean targetView =  getViewBean(VSNSearchViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SEARCH_STRING);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(CHILD_LIB_SUMMARY_HREF);
        ViewBean targetView = getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SEARCH_STRING);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(CHILD_VSN_SUMMARY_HREF);
        ViewBean targetView = getViewBean(VSNSummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SEARCH_STRING);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleHistorianHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(CHILD_HISTORIAN_HREF);
        ViewBean targetView = getViewBean(HistorianViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SEARCH_STRING);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    private String getSearchString() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SEARCH_STRING);
    }
}
