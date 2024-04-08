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

// ident	$Id: PolicySummaryTiledView.java,v 1.13 2008/12/16 00:10:55 am143972 Exp $

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

public class PolicySummaryTiledView extends CommonTiledViewBase {
    public PolicySummaryTiledView(View parent,
                                  CCActionTableModel model,
                                  String name) {
        super(parent, model, name);
    }

    public void handlePolicyNameHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // find the policy that was clicked
        model.setRowIndex(
            ((TiledViewRequestInvocationEvent)rie).getTileNumber());
        String policyName = getDisplayFieldStringValue("PolicyNameHref");
        String type = getDisplayFieldStringValue("PolicyTypeValue");
        Integer policyType = new Integer(type);

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        source.setPageSessionAttribute(Constants.Archive.POLICY_NAME,
                                       policyName);
        source.setPageSessionAttribute(Constants.Archive.POLICY_TYPE,
                                       policyType);
        CommonViewBeanBase target = null;

        target = (CommonViewBeanBase)
            getViewBean(PolicyDetailsViewBean.class);

        // breadcrumbing
        BreadCrumbUtil.breadCrumbPathForward(source,
           PageInfo.getPageInfo().getPageNumber(source.getName()));

        // finally forward to the viewbean
        source.forwardTo(target);
    }
}
