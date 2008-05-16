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

// ident	$Id: PendingJobsTiledView.java,v 1.12 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.io.IOException;
import javax.servlet.ServletException;

public class PendingJobsTiledView extends RequestHandlingTiledViewBase {
    // Child view names (i.e. display fields).

    // Action table model.
    PendingJobsModel model;


    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public PendingJobsTiledView(
        View parent, PendingJobsModel model, String name) {

        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.model = model;
        registerChildren();
        setPrimaryModel(model);
        TraceUtil.trace3("Exiting");
    }


    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        model.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (model.isChildSupported(name)) {
            // Create child from action table model.
            TraceUtil.trace3("Exiting");
            return model.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    public void handleJobIdHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        int index = -1;

        if (event instanceof TiledViewRequestInvocationEvent) {
            model.setRowIndex(((TiledViewRequestInvocationEvent)
                event).getTileNumber());
        }

        // figure out the type of Job selected
        String s = (String) getDisplayFieldValue("JobIdHref");

        ViewBean targetView = null;
        //  split the string to see if this is a staging job
        String jobIdSplit []  = s.split(",");

        if (jobIdSplit[1].equals("Jobs.jobType2")) {
            targetView = getViewBean(StageJobsViewBean.class);
            targetView.setPageSessionAttribute(
                Constants.PageSessionAttributes.STAGE_JOB_ID, jobIdSplit[0]);
        } else {
            targetView = getViewBean(JobsDetailsViewBean.class);
        }

        ViewBean vb = getParentViewBean();
        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));

        // since the jobid string will be overwritten by new forwardTo(),
        // set it in source vb.
        vb.setPageSessionAttribute(
            Constants.PageSessionAttributes.JOB_ID, s);

        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }
}
