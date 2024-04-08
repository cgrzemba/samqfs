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

// ident	$Id: VSNPoolSummaryTiledView.java,v 1.12 2008/12/16 00:10:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Tiled View for the VSNPool Action Table.
 */
public class VSNPoolSummaryTiledView extends CommonTiledViewBase {
    VSNPoolSummaryTiledView(View parent,
                            CCActionTableModel model,
                            String name) {
        super(parent, model, name);
    }

    public void handleVSNPoolHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        if (event instanceof TiledViewRequestInvocationEvent) {
            model.setRowIndex(((TiledViewRequestInvocationEvent)
                               event).getTileNumber());
        }

        String poolName = (String)getDisplayFieldValue("VSNPoolHref");
        String mt = (String)getDisplayFieldValue("MediaTypeHidden");
        Integer mediaType = new Integer(mt);

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(VSNPoolDetailsViewBean.class);

        // save the vsnpool name for the details page
        source.setPageSessionAttribute(Constants.Archive.VSN_POOL_NAME,
                                       poolName);
        source.setPageSessionAttribute(Constants.Archive.POOL_MEDIA_TYPE,
                                       mediaType);


        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
        TraceUtil.trace3("Exiting");
    }
}
