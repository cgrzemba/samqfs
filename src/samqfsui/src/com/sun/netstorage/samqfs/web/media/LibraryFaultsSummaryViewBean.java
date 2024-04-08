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

// ident	$Id: LibraryFaultsSummaryViewBean.java,v 1.25 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.alarms.CurrentAlarmSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;
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

import java.io.IOException;
import java.util.StringTokenizer;

import javax.servlet.ServletException;
import javax.servlet.http.HttpSession;


/**
 *  This class is the view bean for the Library Faults Summary page
 */
public class LibraryFaultsSummaryViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "LibraryFaultsSummary";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/LibraryFaultsSummary.jsp";

    public static final String CHILD_BREADCRUMB = "BreadCrumb";

    // cc components from the corresponding jsp page(s)...
    private static final String CHILD_CONTAINER_VIEW  =
        "LibraryFaultsSummaryView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // child that used in javascript confirm messages
    public static final String CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD =
        "ConfirmMessageHiddenField";

    // For BreadCrumb
    private CCBreadCrumbsModel breadCrumbsModel;
    public static final String CHILD_LIB_SUMMARY_HREF = "LibrarySummaryHref";
    public static final String CHILD_FAULT_SUMMARY_HREF =
        "CurrentAlarmSummaryHref";
    public static final
        String LIBRARY_DRIVE_SUMMARY_HREF = "LibraryDriveSummaryHref";

    /**
     * Constructor
     */
    public LibraryFaultsSummaryViewBean() {
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
        registerChild(CHILD_LIB_SUMMARY_HREF, CCHref.class);
        registerChild(LIBRARY_DRIVE_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_FAULT_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_CONTAINER_VIEW, LibraryFaultsSummaryView.class);
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
        } else if (name.equals(CHILD_LIB_SUMMARY_HREF) ||
            name.equals(LIBRARY_DRIVE_SUMMARY_HREF) ||
            name.equals(CHILD_FAULT_SUMMARY_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel = new CCBreadCrumbsModel(
                "LibraryFaultsSummary.breadcrumbtitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new LibraryFaultsSummaryView(this, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // Javascript used StaticTextField
        } else if (name.equals(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name,
                SamUtil.getResourceString("CurrentAlarmSummary.confirmMsg1"));
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

    private void loadPageTitleModel() {
        TraceUtil.trace3("Entering");

        // Clear the pageTitle Model
        pageTitleModel.clear();

        String libraryName = getLibraryName();
        String libNameInPageTitle = null;

        // if String does not contain # sign
        // use the string LIB_NAME directly,
        // otherwise, parse it.  The string is assigned in Fault Summary Page
        // It is designed in this way so CommonViewBeanBase knows what to se
        // for the default TAB_NAME
        if (libraryName.indexOf("#") == -1) {
            libNameInPageTitle = libraryName;
        } else {
            StringTokenizer tokens = new StringTokenizer(libraryName, "#");
            if (tokens.hasMoreTokens()) {
                libNameInPageTitle = tokens.nextToken();
            }
        }
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("LibraryFaultsSummary.pageTitle1",
            libNameInPageTitle));

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handlers for Breadcrumb links
     */
    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(CHILD_LIB_SUMMARY_HREF);
        ViewBean targetView =  getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleCurrentAlarmSummaryHrefRequest(
        RequestInvocationEvent event) throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(CHILD_FAULT_SUMMARY_HREF);
        ViewBean targetView =  getViewBean(CurrentAlarmSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleLibraryDriveSummaryHrefRequest(
        RequestInvocationEvent event) throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(LIBRARY_DRIVE_SUMMARY_HREF);
        ViewBean targetView =  getViewBean(LibraryDriveSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // update page title
        loadPageTitleModel();

        LibraryFaultsSummaryView view = (LibraryFaultsSummaryView)
            getChild(CHILD_CONTAINER_VIEW);
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            AlarmSummary myAlarmSummary = sysModel.
                getSamQFSSystemAlarmManager().getAlarmSummary();

            HttpSession session =
                RequestManager.getRequestContext().getRequest().getSession();
            String filterMenu = (String)
                session.getAttribute(
                Constants.SessionAttributes.MEDIAFILTER_MENU);

            if (filterMenu == null) {
                filterMenu = Constants.Alarm.ALARM_ALL;
            }
            view.populateData(getServerName(), getLibraryName(), filterMenu);
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay()",
                "Failed to update Masthead fault information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "CurrentAlarmSummary.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    private String getLibraryName() {
        return (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
    }
}
