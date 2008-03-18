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

// ident	$Id: VSNPoolSummaryViewBean.java,v 1.15 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;

/**
 * ViewBean used to display the 'VSN Pool Summary' Page
 */
public class VSNPoolSummaryViewBean extends CommonViewBeanBase {
    // Page information...
    private static final String PAGE_NAME = "VSNPoolSummary";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/archive/VSNPoolSummary.jsp";

    // Used for constructing the Action Table
    public static final String CHILD_CONTAINER_VIEW = "VSNPoolSummaryView";
    public static final String CHILD_CONFIRM_MSG = "ConfirmMsg";
    public static final String POOL_NAMES = "poolNames";
    public static final String POOLS_IN_USE = "poolsInUse";
    public static final String DELETE_CONFIRMATION = "deleteConfirmation";
    public static final String SERVER_NAME = "ServerName";

    // Page Title Attributes and Components.
    private CCPageTitleModel ptModel = null;

    public VSNPoolSummaryViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        createPageTitleModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        PageTitleUtil.registerChildren(this, ptModel);
        registerChild(CHILD_CONTAINER_VIEW, VSNPoolSummaryView.class);
        registerChild(CHILD_CONFIRM_MSG, CCHiddenField.class);
        registerChild(POOL_NAMES, CCHiddenField.class);
        registerChild(POOLS_IN_USE, CCHiddenField.class);
        registerChild(DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);

        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        if (name.equals(CHILD_CONFIRM_MSG) ||
            name.equals(POOL_NAMES) ||
            name.equals(POOLS_IN_USE) ||
            name.equals(DELETE_CONFIRMATION)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            return new VSNPoolSummaryView(this, name);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");

        ptModel = PageTitleUtil.createModel(
            "/jsp/archive/VSNPoolSummaryPageTitle.xml");

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        CCHiddenField field = (CCHiddenField)getChild(CHILD_CONFIRM_MSG);
        field.setValue(
            SamUtil.getResourceString("VSNPoolSummary.confirmMsg1"));
        ((CCHiddenField)getChild(DELETE_CONFIRMATION)).setValue(
            SamUtil.getResourceString("VSNPoolSummary.confirmMsg1"));
        // populate the table model
        ((VSNPoolSummaryView)
         getChild(CHILD_CONTAINER_VIEW)).populateTableModel();
    }
}	
