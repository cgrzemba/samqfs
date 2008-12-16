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

// ident	$Id: LibrarySummaryViewBean.java,v 1.23 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;

import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Library Summary page
 */

public class LibrarySummaryViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "LibrarySummary";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/LibrarySummary.jsp";

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_CONTAINER_VIEW  = "LibrarySummaryView";

    // Javascript confirm message
    public static final String CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD =
        "ConfirmMessageHiddenField";

    // Server Information for pop up pages
    public static final String SERVER_INFORMATION = "ServerInformation";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    /**
     * Constructor
     */
    public LibrarySummaryViewBean() {
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
        registerChild(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(SERVER_INFORMATION, CCHiddenField.class);
        registerChild(CHILD_CONTAINER_VIEW, LibrarySummaryView.class);
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
        // For Javascript Confirm messages
        } else if (name.equals(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("media.confirm.delete.library")).
                    append("###").append(
                    SamUtil.getResourceString("media.confirm.unload.library")).
                    toString());
        // For server name hidden field
        } else if (name.equals(SERVER_INFORMATION)) {
            child = new CCHiddenField(this, name, null);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new LibrarySummaryView(this, name);
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

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/LibrarySummaryPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSearchPageActionButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.PARENT,
            "LibrarySummaryViewBean");
        ViewBean targetView = getViewBean(VSNSearchViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();

        try {
            ((CCHiddenField) getChild(SERVER_INFORMATION)).setValue(
                new NonSyncStringBuffer(serverName).
                    append(ServerUtil.delimitor).
                    append(SamUtil.getServerInfo(serverName).
                        getSamfsServerAPIVersion()).toString());
            // populate the action table model
            LibrarySummaryView view = (LibrarySummaryView)
                getChild(CHILD_CONTAINER_VIEW);
            view.populateData(serverName);
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "LibrarySummaryViewBean()",
                "Failed to populate library summary information",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "LibrarySummary.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        TraceUtil.trace3("Exiting");
    }
}
