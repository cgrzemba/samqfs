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

// ident	$Id: VSNSearchViewBean.java,v 1.24 2008/03/17 14:43:40 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.Constants;

import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the VSNSearch page
 */

public class VSNSearchViewBean extends CommonViewBeanBase {


    // Page information...
    private static final String PAGE_NAME = "VSNSearch";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/media/VSNSearch.jsp";

    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_TEXTFIELD = "TextField";
    public static final String CHILD_BUTTON = "Button";
    public static final String CHILD_BREADCRUMB = "BreadCrumb";
    public static final String CHILD_HELP_TEXT = "HelpText";
    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String CHILD_ERROR_MESSAGES = "ErrorMessages";

    // For BreadCrumb
    public static final String CHILD_LIB_SUMMARY_HREF = "LibrarySummaryHref";
    public static final String CHILD_LIB_DETAILS_HREF = "LibraryDetailsHref";
    public static final String CHILD_VSN_SUMMARY_HREF = "VSNSummaryHref";
    public static final String CHILD_HISTORIAN_HREF   = "HistorianHref";

    private CCPageTitleModel pageTitleModel = null;
    private CCBreadCrumbsModel breadCrumbsModel = null;

    /**
     * Constructor
     */
    public VSNSearchViewBean() {
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
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_HELP_TEXT, CCTextField.class);
        registerChild(CHILD_LIB_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_LIB_DETAILS_HREF, CCHref.class);
        registerChild(CHILD_VSN_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_HISTORIAN_HREF, CCHref.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_BUTTON, CCButton.class);
        registerChild(CHILD_ERROR_MESSAGES, CCHiddenField.class);
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
        TraceUtil.trace3(new NonSyncStringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel = new CCBreadCrumbsModel("VSNSearch.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_LIB_SUMMARY_HREF) ||
            name.equals(CHILD_LIB_DETAILS_HREF) ||
            name.equals(CHILD_VSN_SUMMARY_HREF) ||
            name.equals(CHILD_HISTORIAN_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_TEXTFIELD) ||
            name.equals(CHILD_HELP_TEXT)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_STATICTEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_BUTTON)) {
            child = new CCButton(this, name, null);
        } else if (name.equals(CHILD_ERROR_MESSAGES)) {
            child = new CCHiddenField(
                this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("VSNSearch.errMsg1")).
                    append("###").append(
                    SamUtil.getResourceString("VSNSearch.errMsg2")).toString());
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

    /**
     * Handler for Search button
     */
    public void handleButtonRequest(RequestInvocationEvent event)
            throws ServletException, IOException {
        boolean error = false;

        TraceUtil.trace3("Entering");
        LogUtil.info(
            this.getClass(),
            "handleButtonRequest",
            "Start searching VSN");
        String searchString =
            ((String) getDisplayFieldValue(CHILD_TEXTFIELD)).trim();
        SamUtil.doPrint(new NonSyncStringBuffer("searchString: ").
            append(searchString).toString());

        String parent = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        SamUtil.doPrint(new NonSyncStringBuffer("parent: ").append(
            parent).toString());

        ViewBean targetView = null;
        VSN allVSN[] = null, myVSN = null;
        int totalMatch = 0;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            if (parent.equals("VSNSummaryViewBean") ||
                parent.equals("HistorianViewBean")) {
                // search only one library or the historian
                totalMatch = 0; // reset counter

                allVSN = sysModel.getSamQFSSystemMediaManager().
                    searchVSNInLibraries(searchString);

                // Now search in allVSN, count the ones which is
                // in library LIB_NAME

                SamUtil.doPrint(new NonSyncStringBuffer("LIB_NAME: ").
                    append(getLibraryName()).toString());
                SamUtil.doPrint(new NonSyncStringBuffer(
                    "allVSN's length: ").append(allVSN.length).toString());

                String searchKey = getLibraryName();

                if (allVSN != null) {
                    for (int i = 0; i < allVSN.length; i++) {
                        if (allVSN[i].getLibrary().getName().
                            equals(searchKey)) {
                            SamUtil.doPrint("ADD 1 to TOTALMATCH!");
                            totalMatch++;
                        }
                    }
                } else {
                    // no match
                    SamUtil.doPrint("NO MATCH");
                    setPageSessionAttribute(
                        Constants.PageSessionAttributes.SEARCH_STRING,
                        searchString);
                    targetView = getViewBean(VSNSearchResultViewBean.class);
                }

                // If there is only one match, go to VSNDetails page directly
                // Otherwise, go to RESULT page
                if (totalMatch == 1) {
                    // one match
                    Library myLibrary = sysModel.getSamQFSSystemMediaManager().
                        getLibraryByName(searchKey);
                    int realSlot =
                        myLibrary.getVSN(searchString).getSlotNumber();

                    // assign the real slot number to page session
                    setPageSessionAttribute(
                        Constants.PageSessionAttributes.SLOT_NUMBER,
                        Integer.toString(realSlot));
                    targetView = getViewBean(VSNDetailsViewBean.class);
                } else {
                    // no match or multiple match
                    setPageSessionAttribute(
                        Constants.PageSessionAttributes.SEARCH_STRING,
                        searchString);
                    targetView = getViewBean(VSNSearchResultViewBean.class);
                }
            } else if (parent.equals("LibrarySummaryViewBean")) {
                // search all libraries
                SamUtil.doPrint("Search all libraries...");
                allVSN = sysModel.getSamQFSSystemMediaManager().
                    searchVSNInLibraries(searchString);

                if (allVSN != null) {
                    if (allVSN.length == 1) {
                        // found one
                        SamUtil.doPrint("ONE VSN found!");
                        // only 1 found, go straight to VSN Details
                        // Need to set the LIB_NAME attribute as well
                        setPageSessionAttribute(
                            Constants.PageSessionAttributes.LIBRARY_NAME,
                            allVSN[0].getLibrary().getName());
                        setPageSessionAttribute(
                            Constants.PageSessionAttributes.SLOT_NUMBER,
                            Integer.toString(allVSN[0].getSlotNumber()));
                        targetView = getViewBean(VSNDetailsViewBean.class);
                    } else {
                        SamUtil.doPrint("No or multiple found");
                        // no result or multiple results
                        setPageSessionAttribute(
                            Constants.PageSessionAttributes.SEARCH_STRING,
                            searchString);
                        targetView = getViewBean(VSNSearchResultViewBean.class);
                    }
                } else {
                    SamUtil.doPrint("Nothing found!");
                    setPageSessionAttribute(
                        Constants.PageSessionAttributes.SEARCH_STRING,
                        searchString);
                    targetView = getViewBean(VSNSearchResultViewBean.class);
                }
            }
        } catch (SamFSException samEx) {
            error = true;
            targetView = getParentViewBean();

            SamUtil.processException(
                samEx, this.getClass(),
                "handleButtonRequest",
                "Fail to perform search operation",
                getServerName());
            SamUtil.setErrorAlert(
                targetView,
                CHILD_COMMON_ALERT,
                "VSNSearch.error.search",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        LogUtil.info(
            this.getClass(),
            "handleButtonRequest",
            "Done searching VSN");

        if (!error) {
            BreadCrumbUtil.breadCrumbPathForward(
                getParentViewBean(),
                PageInfo.getPageInfo().getPageNumber(
                    getParentViewBean().getName()));
        }

        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Breadcrumb link handler
     */
    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue("LibrarySummaryHref");
        ViewBean targetView = getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Breadcrumb link handler
     */
    public void handleVSNSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue("VSNSummaryHref");
        ViewBean targetView = getViewBean(VSNSummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Breadcrumb link handler
     */
    public void handleHistorianHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue("HistorianHref");
        ViewBean targetView = getViewBean(HistorianViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    private String getLibraryName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
    }
}
