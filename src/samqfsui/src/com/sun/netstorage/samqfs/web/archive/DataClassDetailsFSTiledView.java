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

// ident	$Id: DataClassDetailsFSTiledView.java,v 1.6 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.web.ui.model.CCActionTableModel;

public class DataClassDetailsFSTiledView extends CommonTiledViewBase {
    public DataClassDetailsFSTiledView(
        View parent,
        CCActionTableModel model,
        String name) {
        super(parent, model, name);
    }

    public void handleFSNameHrefRequest(RequestInvocationEvent evt) {
        model.setRowIndex(
            ((TiledViewRequestInvocationEvent)evt).getTileNumber());
        String fsName = (String)getDisplayFieldValue("FSNameHref");

        // parent & server name
        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        String serverName = source.getServerName();

        // save the file system name
        source.setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME, fsName);

        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSDetailsViewBean.class);

        // set the breadcrumbs
        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
    }
}
