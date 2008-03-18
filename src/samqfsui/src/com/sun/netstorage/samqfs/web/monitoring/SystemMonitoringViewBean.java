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

// ident	$Id: SystemMonitoringViewBean.java,v 1.7 2008/03/17 14:43:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Monitoring Console launch page
 */

public class SystemMonitoringViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "SystemMonitoring";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/monitoring/SystemMonitoring.jsp";

    private static final String DESCRIPTION = "Description";
    private static final String LAUNCH_BUTTON = "LaunchButton";
    private static final String SERVER_NAME = "ServerName";

    // Page Title and Property Sheet Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    /**
     * Constructor
     */
    public SystemMonitoringViewBean() {
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
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(DESCRIPTION, CCStaticTextField.class);
        registerChild(LAUNCH_BUTTON, CCButton.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
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
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(DESCRIPTION)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(LAUNCH_BUTTON)) {
            child = new CCButton(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            child = new CCHiddenField(this, name, getServerName());
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

    public void handleLaunchButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }
}
