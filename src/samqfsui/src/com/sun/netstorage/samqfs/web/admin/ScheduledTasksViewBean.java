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

//	ident	$Id: ScheduledTasksViewBean.java,v 1.7 2008/12/16 00:10:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;

public class ScheduledTasksViewBean extends CommonViewBeanBase {
    private static final String PAGE_NAME = "ScheduledTasks";
    private static final String DEFAULT_URL = "/jsp/admin/ScheduledTasks.jsp";

    private static final String SCHEDULED_TASKS_VIEW = "ScheduledTasksView";
    private static final String PAGE_TITLE = "PageTitle";

    public static final String ALL_IDS = "allScheduleIds";
    public static final String ALL_NAMES = "allScheduleNames";
    public static final String ERROR_MSG = "errMsg";

    /**
     * default constructor
     */
    public ScheduledTasksViewBean() {
        super(PAGE_NAME, DEFAULT_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * register this view's child views
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(SCHEDULED_TASKS_VIEW, ScheduledTasksView.class);
        super.registerChildren();
        registerChild(ALL_IDS, CCHiddenField.class);
        registerChild(ALL_NAMES, CCHiddenField.class);
        registerChild(ERROR_MSG, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * create a named child view
     */
    public View createChild(String name) {
        TraceUtil.trace3(new StringBuffer(
            "Entering: child: ").append(name).toString());
        if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else if (name.equals(SCHEDULED_TASKS_VIEW)) {
            return new ScheduledTasksView(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(ALL_NAMES) ||
                   name.equals(ALL_IDS) ||
                   name.equals(ERROR_MSG)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    /** begin display of jsp */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        ScheduledTasksView view =
            (ScheduledTasksView)getChild(SCHEDULED_TASKS_VIEW);
        view.populateTableModel();

        ((CCHiddenField)getChild(ERROR_MSG)).setValue(
            SamUtil.getResourceString("admin.scheduledtasks.remove.confirm"));

        TraceUtil.trace3("Exiting");
    }
}
