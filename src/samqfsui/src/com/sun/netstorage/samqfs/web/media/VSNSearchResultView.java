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

// ident	$Id: VSNSearchResultView.java,v 1.15 2008/03/17 14:43:40 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.model.ModelControlException;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;

/**
 * This is the View class of VSN Search Result page.
 */
public class VSNSearchResultView extends CommonTableContainerView {

    // child name for tiled view class
    public static final String CHILD_TILED_VIEW = "VSNSearchResultTiledView";

    private VSNSearchResultModel model = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public VSNSearchResultView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "VSNSearchResultTable";
        model = new VSNSearchResultModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_TILED_VIEW, VSNSearchResultTiledView.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        if (name.equals(CHILD_TILED_VIEW)) {
            // The ContainerView used to register and create children
            // for "column" elements defined in the model's XML
            // document. Note that we're creating a seperate
            // ContainerView only to evoke JATO's TiledView behavior
            // for these elements. If TiledView behavior is not
            // required, creating a ContainerView object is not
            // necessary. By default, CCActionTable will attempt to
            // retrieve all children from it's parent (i.e., this view).
            VSNSearchResultTiledView child =
                new VSNSearchResultTiledView(this, model, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else {
            TraceUtil.trace3("Exiting");
            return super.createChild(model, name,
                VSNSearchResultView.CHILD_TILED_VIEW);
        }
    }

    public void populateData(
        String serverName,
        String parent,
        String searchString,
        String libraryName)
        throws SamFSException {
        TraceUtil.trace3("Entering");
        model.initModelRows(serverName, parent, searchString, libraryName);
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // Disable Tooltip
        CCActionTable myTable = (CCActionTable) getChild(CHILD_ACTION_TABLE);
        CCRadioButton myRadio = (CCRadioButton) myTable.getChild(
            CCActionTable.CHILD_SELECTION_RADIOBUTTON);
        myRadio.setTitle("");
        myRadio.setTitleDisabled("");
        TraceUtil.trace3("Exiting");
    }
}
