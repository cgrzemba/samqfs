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

// ident	$Id: ScheduleFsMetricViewBean.java,v 1.5 2008/03/17 14:40:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for create media report
 */

public class ScheduleFsMetricViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME   = "ScheduleFsMetric";
    private static final String DISPLAY_URL = "/jsp/admin/ScheduleFsMetric.jsp";

    public static final String CONTAINER_VIEW    = "ScheduleFsMetricView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    public ScheduleFsMetricViewBean() {
        super(PAGE_NAME, DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();

        // preserve server name in PopUp
        String serverName = getServerName();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CONTAINER_VIEW, ScheduleFsMetricView.class);

        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CONTAINER_VIEW)) {
            child = new ScheduleFsMetricView(this, name);
        } else {
            throw new IllegalArgumentException(new StringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;

    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);
        String fsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);

        if (fsName == null) {
            fsName =
                RequestManager.getRequest().getParameter(
                    Constants.PageSessionAttributes.FS_NAME);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.FS_NAME, fsName);
        }
        System.out.println("file system name=" + fsName);

        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("reports.collection.schedule", fsName));
        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/util/CommonPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        ScheduleFsMetricView view =
            (ScheduleFsMetricView) getChild(CONTAINER_VIEW);

        view.submitRequest();
        setSubmitSuccessful(true);

        forwardTo(getRequestContext());
    }
}
