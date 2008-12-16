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

// ident	$Id: MediaReportTiledView.java,v 1.8 2008/12/16 00:10:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
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

/**
 * This is the TiledView class for Fault Summary Page. This is used to
 * identify if a device HREF link is needed. Also this tiledview class is needed
 *  to avoid concurrency issue.
 */


public class MediaReportTiledView extends RequestHandlingTiledViewBase {

    public static final String CHILD_PATH_HREF = "ReportNameHref";
    public static final String CHILD_DATE = "ReportDateText";

    // Action table model.
    private CCActionTableModel model = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public MediaReportTiledView
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

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Request Handlers
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    public void handleReportNameHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        // Since children are bound to the primary model of a
        // TiledView, the model contains the submitted value
        // associated with each row. And because the primary model is
        // always reset to the first() position during submit, we need
        // to make sure we move to the right location in the model
        // before reading values. Otherwise, we would always read
        // values from the first row only.
        int index = ((TiledViewRequestInvocationEvent)event).getTileNumber();
        if (event instanceof TiledViewRequestInvocationEvent) {
            model.setRowIndex(index);
        }
        System.out.println("Setting index " + index + " as selected row");
        model.setRowSelected(index, true);
        String path = (String) getDisplayFieldValue(CHILD_PATH_HREF);

        ((CommonViewBeanBase) getParentViewBean()).setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME, path);
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "Setting page session attribute PATH_NAME to ").
            append(path).toString());

        // enable Delete button
        ((CCButton)getChild("DeleteButton")).setDisabled(false);

        ((CommonViewBeanBase) getParentViewBean()).
                        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");

    }

    public boolean beginReportNameHrefDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        String path = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME);

        if (getDisplayFieldStringValue(CHILD_PATH_HREF).equals(path)) {
            return false;
        }
        return true;
    }

    public boolean beginReportNameNoHrefDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        String path = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME);

        if (getDisplayFieldStringValue(CHILD_PATH_HREF).equals(path)) {
            return true;
        }
        return false;
    }
}
