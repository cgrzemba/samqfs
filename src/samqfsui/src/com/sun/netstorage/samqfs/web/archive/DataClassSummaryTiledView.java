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

// ident	$Id: DataClassSummaryTiledView.java,v 1.9 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.web.ui.model.CCActionTableModel;
import java.io.IOException;
import javax.servlet.ServletException;

public class DataClassSummaryTiledView extends CommonTiledViewBase {

    public static final String CLASS_NAME_HREF  = "ClassNameHref";
    public static final String POLICY_NAME_HREF = "PolicyNameHref";
    public static final String POLICY_NAME = "PolicyNameText";

    public DataClassSummaryTiledView(View parent,
                                  CCActionTableModel model,
                                  String name) {
        super(parent, model, name);
    }

    public void handleClassNameHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // find the data class that was clicked
        forwardToViewBean(true, rie);
    }

    public void handlePolicyNameHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // find the data class that was clicked
        forwardToViewBean(false, rie);
    }

    private void forwardToViewBean(
        boolean isClassNameHref, RequestInvocationEvent rie) {
        model.setRowIndex(
            ((TiledViewRequestInvocationEvent) rie).getTileNumber());
        String info = isClassNameHref?
            (String) getDisplayFieldValue(CLASS_NAME_HREF) :
            (String) getDisplayFieldValue(POLICY_NAME_HREF);
        String [] infos = info.split("###");

        CommonViewBeanBase targetVB =
            isClassNameHref ?
            (CommonViewBeanBase) getViewBean(DataClassDetailsViewBean.class) :
            (CommonViewBeanBase) getViewBean(ISPolicyDetailsViewBean.class);

        getParentViewBean().setPageSessionAttribute(
            Constants.Archive.POLICY_NAME, infos[0]);

        if (isClassNameHref) {
            getParentViewBean().setPageSessionAttribute(
                Constants.Archive.CRITERIA_NUMBER, new Integer(infos[1]));
        }

        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));

        ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetVB);
    }

    /** skip the policy name href when dealing with the allsets policy */
    public boolean beginPolicyNameHrefDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        String policyName = getDisplayFieldStringValue(POLICY_NAME);
        return !ArchivePolicy.POLICY_NAME_NOARCHIVE.equals(policyName);
    }

    public boolean beginNoHrefPolicyNameDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        String policyName = getDisplayFieldStringValue(POLICY_NAME);
        return ArchivePolicy.POLICY_NAME_NOARCHIVE.equals(policyName);
    }
}
