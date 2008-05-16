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

// ident	$Id: FSArchivePoliciesTiledView.java,v 1.19 2008/05/16 18:38:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.io.IOException;
import java.util.StringTokenizer;
import javax.servlet.ServletException;

/**
 *  This class is the tiled view class for FSArchivePolicies actiontable.
 */

public class FSArchivePoliciesTiledView extends RequestHandlingTiledViewBase {

    private FSArchivePoliciesModel model;

    public FSArchivePoliciesTiledView(
        View parent, FSArchivePoliciesModel model, String name) {

        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.model = model;
        registerChildren();
        setPrimaryModel(model);
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        model.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (model.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return model.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Handler for the "Policy Name" link.
     */
    public void handlePolicyHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Handling PolicyHref request");
        if (event instanceof TiledViewRequestInvocationEvent) {
            model.setRowIndex(
                ((TiledViewRequestInvocationEvent)event).getTileNumber());
        }

        String policyHrefString = (String) getDisplayFieldValue("PolicyHref");
        // policy details href = policyName:policyType
        StringTokenizer tokens = new StringTokenizer(policyHrefString, ":");
        String policyName = tokens.nextToken();
        String policyType = tokens.nextToken();
        short intType = ArSet.AR_SET_TYPE_GENERAL;
        try {
            intType = Short.parseShort(policyType);
        } catch (NumberFormatException nfe) {
            // should never get here!!!
        }

        ViewBean targetView = getViewBean(PolicyDetailsViewBean.class);

        // Set policyName in page session attributes
        ViewBean vb = getParentViewBean();
        vb.setPageSessionAttribute(Constants.Archive.POLICY_NAME, policyName);
        vb.setPageSessionAttribute(Constants.Archive.POLICY_TYPE,
                                   new Integer(intType));

        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));

        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Done");
    }

    /**
     * Handler for the "Criteria Number" link.
     */
    public void handleCriteriaHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Handling CriteriaHref request");
        if (event instanceof TiledViewRequestInvocationEvent) {
            model.setRowIndex(
                ((TiledViewRequestInvocationEvent)event).getTileNumber());
        }

        String policyCriteriaHref =
            (String) getDisplayFieldValue("CriteriaHref");
        // criteria details href = policyName:policyType:criteriaNumber
        // or for default policy = policyName:policyType
        StringTokenizer tokens = new StringTokenizer(policyCriteriaHref, ":");
        int count = tokens.countTokens();
        String policyName = tokens.nextToken();
        String policyType = tokens.nextToken();

        short intType = ArSet.AR_SET_TYPE_GENERAL;
        int criteriaNum = 0;
        try {
            intType = Short.parseShort(policyType);
            if (count > 2) {
                criteriaNum = Integer.parseInt(tokens.nextToken());
            }
        } catch (NumberFormatException nfe) {
            // should never get here!!!
        }

        ViewBean targetView;

        if (count > 2) {
            targetView = getViewBean(CriteriaDetailsViewBean.class);
        } else {
            targetView = getViewBean(PolicyDetailsViewBean.class);
        }

        ViewBean vb = getParentViewBean();
        vb.setPageSessionAttribute(Constants.Archive.POLICY_NAME, policyName);
        vb.setPageSessionAttribute(Constants.Archive.POLICY_TYPE,
                                   new Integer(intType));
        vb.setPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER,
                                   new Integer(criteriaNum));

        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));

        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Done");
    }
}
