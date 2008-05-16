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

// ident	$Id: SingleTableView.java,v 1.12 2008/05/16 18:39:04 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;

public class SingleTableView extends CommonTableContainerView {

    // child name for tiled view class
    public static final String TILED_VIEW = "SingleTableTiledView";

    private CCActionTableModel tableModel = null;

    /**
     * Default Constructor
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public SingleTableView(View parent, String name) {
        super(parent, name);

        CHILD_ACTION_TABLE = "ActionTable";

        switch (getPageID()) {
            case FrameNavigatorViewBean.PAGE_DAEMON:
                tableModel = new DaemonModel();
                break;
            case FrameNavigatorViewBean.PAGE_FILESYSTEM:
                tableModel = new FileSystemModel();
                break;
            case FrameNavigatorViewBean.PAGE_COPYFREESPACE:
                tableModel = new CopyFreeSpaceModel();
                break;
            case FrameNavigatorViewBean.PAGE_LIBRARY:
                tableModel = new LibraryModel();
                break;
            case FrameNavigatorViewBean.PAGE_TAPEMOUNTQUEUE:
                tableModel = new TapeMountQueueModel();
                break;
            case FrameNavigatorViewBean.PAGE_DRIVE:
                tableModel = new DriveModel();
                break;
            case FrameNavigatorViewBean.PAGE_QUARANTINEDVSN:
                tableModel = new QuarantinedVSNModel();
                break;
            case FrameNavigatorViewBean.PAGE_ARCOPYQUEUE:
                tableModel = new ArCopyQueueModel();
                break;
            case FrameNavigatorViewBean.PAGE_STAGINGQUEUE:
                tableModel = new StagingQueueModel();
                break;
            default:
                // should not reach here
                break;
        }

        registerChildren();
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        super.registerChildren(tableModel);
        registerChild(TILED_VIEW, SingleTableTiledView.class);
    }

    public View createChild(String name) {
        View child = null;

        if (name.equals(TILED_VIEW)) {
            child = new SingleTableTiledView(this, tableModel, name);
        } else {
            child = super.createChild(tableModel, name, TILED_VIEW);
        }
        return (View) child;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        String title = "";

        try {
            switch (getPageID()) {
                case FrameNavigatorViewBean.PAGE_DAEMON:
                    ((DaemonModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.daemons";
                    break;
                case FrameNavigatorViewBean.PAGE_FILESYSTEM:
                    ((FileSystemModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.fs";
                    break;
                case FrameNavigatorViewBean.PAGE_COPYFREESPACE:
                    ((CopyFreeSpaceModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.copies";
                    break;
                case FrameNavigatorViewBean.PAGE_LIBRARY:
                    ((LibraryModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.libraries";
                    break;
                case FrameNavigatorViewBean.PAGE_TAPEMOUNTQUEUE:
                    ((TapeMountQueueModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.tapemountqueue";
                    break;
                case FrameNavigatorViewBean.PAGE_DRIVE:
                    ((DriveModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.drives";
                    break;
                case FrameNavigatorViewBean.PAGE_QUARANTINEDVSN:
                    ((QuarantinedVSNModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.quarantinedvsn";
                    break;
                case FrameNavigatorViewBean.PAGE_ARCOPYQUEUE:
                    ((ArCopyQueueModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.arcopyqueue";
                    break;
                case FrameNavigatorViewBean.PAGE_STAGINGQUEUE:
                    ((StagingQueueModel) tableModel).
                        initModelRows(getServerName());
                    title = "Monitor.page.stagingqueue";
                    break;
            }
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                getClass(),
                "beginDisplay",
                "Failed to retrieve content panel content!",
                getServerName());
            TraceUtil.trace1("Error fetching content for frame " + getPageID());
        }

        // Set table title
        tableModel.setTitle(SamUtil.getResourceString(title).
            concat(" - ").concat(SamUtil.getCurrentTimeString()));
    }

    private int getPageID() {
        // frame ID
        return ((Integer) getParentViewBean().
            getPageSessionAttribute("PAGE_ID")).intValue();
    }

    private String getServerName() {
        return (String) getParentViewBean().
            getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

} // end of SingleTableView class
