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

// ident	$Id: CopyInformationTiledView.java,v 1.11 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.web.ui.model.CCActionTableModel;
import java.io.IOException;
import javax.servlet.ServletException;

public class CopyInformationTiledView extends CommonTiledViewBase {
    public CopyInformationTiledView(View parent,
                                    CCActionTableModel model,
                                    String name) {
        super(parent, model, name);
    }

    public void handleCopyOptionsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        //
        model.setRowIndex(
            ((TiledViewRequestInvocationEvent)evt).getTileNumber());

        String copyNumber = getDisplayFieldStringValue("CopyNumberHidden");
        String copyMediaType = getDisplayFieldStringValue("MediaTypeHidden");

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(CopyOptionsViewBean.class);

        // preserve copy number and media type so we don't have to look it up
        // in the pages that follow
        source.setPageSessionAttribute(Constants.Archive.COPY_NUMBER,
                                       new Integer(copyNumber));
        source.setPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE,
                                       new Integer(copyMediaType));

        // bread crumb
        BreadCrumbUtil.breadCrumbPathForward(source,
           PageInfo.getPageInfo().getPageNumber(source.getName()));

        // forward to copy options view bean
        source.forwardTo(target);
    }

    public void handleCopyVSNsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        //
        model.setRowIndex(
            ((TiledViewRequestInvocationEvent)evt).getTileNumber());

        String copyNumber = getDisplayFieldStringValue("CopyNumberHidden");
        String copyMediaType = getDisplayFieldStringValue("MediaTypeHidden");

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(CopyVSNsViewBean.class);

        // preserve copy number and media type so we don't have to look it up
        // in the pages that follow
        source.setPageSessionAttribute(Constants.Archive.COPY_NUMBER,
                                       new Integer(copyNumber));
        source.setPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE,
                                       new Integer(copyMediaType));

        // bread crumb
        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        // forward to copy options view bean
        source.forwardTo(target);
    }
}
