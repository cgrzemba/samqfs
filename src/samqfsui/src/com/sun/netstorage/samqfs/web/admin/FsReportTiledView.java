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

// ident	$Id: FsReportTiledView.java,v 1.5 2008/03/17 14:40:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 * The ContainerView used to register and create children for "column"
 * elements defined in the model's XML document.
 *
 * This ContainerView demonstrates how to evoke JATO's TiledView
 * behavior, which automatically appends the current model row index
 * to each child's qualified name. This approach can be useful when
 * retrieving values from rows of text fields, for example.
 *
 * Note that "column" element children must be in a separate
 * ContainerView when implementing JATO's TiledView. Otherwise,
 * HREFs and buttons used for table actions will be incorrectly
 * indexed as if they were also tiled. This results in JATO
 * throwing RuntimeException due to malformed qualified field
 * names because the tile index is not found during a request
 * event.
 *
 * That said, a separate ContainerView object is not necessary
 * when TiledView behavior is not required. All children can be
 * registered and created in the same ContainerView. By default,
 * CCActionTable will attempt to retrieve all children from the
 * ContainerView given to the CCActionTable.setContainerView method.
 *
 */

public class FsReportTiledView extends RequestHandlingTiledViewBase {

    public static final String CHILD_PATH_HREF = "PathHref";
    public static final String CHILD_DATE = "ReportDateText";

    // Action table model.
    private CCActionTableModel model = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FsReportTiledView
        (View parent, CCActionTableModel model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.model = model;
        registerChildren();

        // Set the primary model to evoke JATO's TiledView behavior,
        // which will automatically add the current model row index to
        // each qualified name.
        setPrimaryModel(model);
        TraceUtil.trace3("Exiting");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Child manipulation methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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


    public boolean beginPathHrefDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        String path = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME);

        if (getDisplayFieldStringValue("PathHiddenField").equals(path)) {
            return false;
        }
        return true;
    }

    public boolean beginPathNoHrefDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        String path = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME);

        if (getDisplayFieldStringValue("PathHiddenField").equals(path)) {
            return true;
        }
        return false;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Request Handlers
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    public void handlePathHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        int rowIndex = ((TiledViewRequestInvocationEvent)event).getTileNumber();
        model.setRowSelected(rowIndex, true);

        String path = (String) getDisplayFieldValue(CHILD_PATH_HREF);

        ((CommonViewBeanBase) getParentViewBean()).setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME, path);

        TraceUtil.trace3(new StringBuffer().append(
            "Setting page session attribute PATH_NAME to ").
            append(path).toString());

        ((CommonViewBeanBase) getParentViewBean()).
                            forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");

    }
}
