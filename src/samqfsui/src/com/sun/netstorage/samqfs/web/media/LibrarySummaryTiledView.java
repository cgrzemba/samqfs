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

// ident	$Id: LibrarySummaryTiledView.java,v 1.19 2008/03/17 14:43:40 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCImageField;

import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

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
 * This is the TiledView class for Library Summary Page. This is used to
 * identify if an alarm HREF link is needed. Also this tiledview class is needed
 * to avoid concurrency issue.
 */

public class LibrarySummaryTiledView extends RequestHandlingTiledViewBase {
    // Child view names (i.e. display fields).
    public static final String CHILD_ALARM_HREF  = "AlarmHref";
    public static final String CHILD_DETAIL_HREF = "DetailHref";

    // Action table model.
    private LibrarySummaryModel model = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public LibrarySummaryTiledView
        (View parent, LibrarySummaryModel model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
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

    /**
     * Method to append alt text to images due to 508 compliance
     */
    public boolean beginUsageBarImageDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        CCHiddenField myHiddenField = (CCHiddenField) getChild("usageText");
        boolean isOn = false;

        try {
            isOn = ((Long) myHiddenField.getValue()).longValue() != -1;
        } catch (NumberFormatException numEx) {
            // Developers bug
            TraceUtil.trace1(
                "Developer's bug found in LibrarySummaryTiledView.java!");
        }

        if (isOn) {
            ((CCImageField) getChild("UsageBarImage")).setAlt(
                SamUtil.getResourceString(
                    "usage.alt",
                    ((Long) myHiddenField.getValue()).toString()));
            ((CCImageField) getChild("UsageBarImage")).setTitle(
                SamUtil.getResourceString(
                    "usage.alt",
                    ((Long) myHiddenField.getValue()).toString()));
        } else {
            // No image
            ((CCImageField) getChild("UsageBarImage")).setAlt("");
            ((CCImageField) getChild("UsageBarImage")).setTitle("");
        }
        return true;
    }


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Request Handlers
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    public void handleDetailHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        forwardToPage(true, event);
        TraceUtil.trace3("Exiting");
    }

    public void handleAlarmHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        forwardToPage(false, event);
        TraceUtil.trace3("Exiting");
    }

    private void forwardToPage(
        boolean isDetailsHref,
        RequestInvocationEvent event) {

        String str;
        ViewBean targetViewBean;

        // Since children are bound to the primary model of a
        // TiledView, the model contains the submitted value
        // associated with each row. And because the primary model is
        // always reset to the first() position during submit, we need
        // to make sure we move to the right location in the model
        // before reading values. Otherwise, we would always read
        // values from the first row only.
        if (event instanceof TiledViewRequestInvocationEvent) {
            model.setRowIndex(((TiledViewRequestInvocationEvent)
                event).getTileNumber());
        }

        if (isDetailsHref) {
            str = (String) getDisplayFieldValue(CHILD_DETAIL_HREF);
            TraceUtil.trace3(new NonSyncStringBuffer(
                "handleAlarmHref: selected item is ").append(str).toString());
            if (str.equals("<Historian>")) {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME,
                    "Historian");
                targetViewBean = getViewBean(HistorianViewBean.class);
            } else {
                targetViewBean = getViewBean(LibraryDriveSummaryViewBean.class);

                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME,
                    str);
                // assign true/false to determine if shared information
                // should be shown
                try {
                    getParentViewBean().setPageSessionAttribute(
                        Constants.PageSessionAttributes.SHARE_CAPABLE,
                        Boolean.toString(
                            MediaUtil.isDriveSharedCapable(
                                getServerName(), str)));
                } catch (SamFSException samEx) {
                    SamUtil.processException(
                        samEx,
                        this.getClass(),
                        "handleDetailHrefRequest",
                        "Failed to determine if library is shared capable",
                        getServerName());
                    SamUtil.setErrorAlert(
                        targetViewBean,
                        CommonViewBeanBase.CHILD_COMMON_ALERT,
                        "LibrarySummary.error.viewdrive",
                        samEx.getSAMerrno(),
                        samEx.getMessage(),
                        getServerName());
                }
                targetViewBean = getViewBean(LibraryDriveSummaryViewBean.class);
            }
        } else {
            str = (String) getDisplayFieldValue(CHILD_ALARM_HREF);
            TraceUtil.trace3(new NonSyncStringBuffer(
                "handleAlarmHref: selected item is ").append(str).toString());
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.LIBRARY_NAME,
                str);
            targetViewBean = getViewBean(LibraryFaultsSummaryViewBean.class);
        }

        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));
        ((CommonViewBeanBase)getParentViewBean()).forwardTo(targetViewBean);
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }
}
