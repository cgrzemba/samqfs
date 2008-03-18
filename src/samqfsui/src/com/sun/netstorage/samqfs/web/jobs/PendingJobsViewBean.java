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

// ident	$Id: PendingJobsViewBean.java,v 1.12 2008/03/17 14:43:38 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;


/**
 * ViewBean used to display the 'Pending Jobs' Page
 */
public class PendingJobsViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "PendingJobs";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/jobs/PendingJobs.jsp";

    // Used for constructing the Action Table
    private static final String
        CHILD_JOBS_CONTAINER_VIEW = "PendingJobsView";

    public static final String CHILD_CONFIRM_MSG = "ConfirmMsg";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    public PendingJobsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_JOBS_CONTAINER_VIEW, PendingJobsView.class);
        registerChild(CHILD_CONFIRM_MSG, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Creating child " + name);
        if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_JOBS_CONTAINER_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new PendingJobsView(this, name, getServerName());
        } else if (name.equals(CHILD_CONFIRM_MSG)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        CCPageTitleModel model =
            PageTitleUtil.createModel("/jsp/jobs/PendingJobsPageTitle.xml");
        model.setPageTitleText("PendingJobs.title");
        TraceUtil.trace3("Exiting");
        return model;
    }

    public void beginDisplay(DisplayEvent event) {
        String serverName = getServerName();
        PendingJobsView view = (PendingJobsView)
            getChild(CHILD_JOBS_CONTAINER_VIEW);

        CCHiddenField field = (CCHiddenField)getChild(CHILD_CONFIRM_MSG);
        field.setValue(SamUtil.getResourceString("Jobs.confirmMsg1"));

        try {
            view.populateData();
        }
        catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay()",
                "Cannot populate pending jobs",
                serverName);

            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "PendingJobs.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
    }
}
