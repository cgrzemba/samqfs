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

//	ident	$Id: ScheduledTasksTiledView.java,v 1.9 2008/12/16 00:10:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.fs.RecoveryPointScheduleViewBean;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.web.ui.model.CCActionTableModel;
import java.io.IOException;
import java.util.Date;
import javax.servlet.ServletException;

public class ScheduledTasksTiledView extends CommonTiledViewBase {
    public ScheduledTasksTiledView(View parent,
                                   CCActionTableModel tableModel,
                                   String name) {
        super(parent, tableModel, name);
    }

    public void handleDetailsHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        int tileNumber =
            ((TiledViewRequestInvocationEvent)rie).getTileNumber();

        model.setRowIndex(tileNumber);

        String taskId = getDisplayFieldStringValue("TaskId");
        String taskName = getDisplayFieldStringValue("TaskName");


        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target = null;

		ScheduleTaskID id = ScheduleTaskID.getScheduleTaskID(taskId);
		if (id.equals(ScheduleTaskID.SNAPSHOT)) {
                    target = (CommonViewBeanBase)
                            getViewBean(RecoveryPointScheduleViewBean.class);
		} else if (id.equals(ScheduleTaskID.REPORT)) {
                    target = (CommonViewBeanBase)
                            getViewBean(FileMetricSummaryViewBean.class);
		} else {
                    source.forwardTo(getRequestContext());
		}

        source.setPageSessionAttribute(Constants.admin.TASK_ID, taskId);
        source.setPageSessionAttribute(Constants.admin.TASK_NAME, taskName);

        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        // finally forward to the target view bean
        source.forwardTo(target);
    }
}
