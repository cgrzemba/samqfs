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

// ident	$Id: VSNPoolDetailsView.java,v 1.15 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.media.VSNDetailsViewBean;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.File;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Creates the ArchivePolSummary Action Table and provides
 * handlers for the links within the table.
 */
public class VSNPoolDetailsView extends CommonTableContainerView {
    private VSNPoolDetailsModel model;

    public VSNPoolDetailsView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "VSNPoolDetailsTable";

        // get the parent view bean
        CommonViewBeanBase parentVB = (CommonViewBeanBase)getParentViewBean();
        LargeDataSet dataSet = new VSNPoolDetailsData(parentVB);

        Integer mt = (Integer)parentVB.
            getPageSessionAttribute(Constants.Archive.POOL_MEDIA_TYPE);

        String xmlFile =  "/jsp/archive/VSNPoolDetailsTapeTable.xml";
        if (mt.intValue() == BaseDevice.MTYPE_DISK ||
            mt.intValue() == BaseDevice.MTYPE_STK_5800) {
             xmlFile =  "/jsp/archive/VSNPoolDetailsDiskTable.xml";
        }
        model = new VSNPoolDetailsModel(dataSet, xmlFile);

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {

        TraceUtil.trace3("Entering");
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {

        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return super.createChild(model, name);
    }

    public void populateData() throws SamFSException {

        TraceUtil.trace3("Entering");

        CCActionTable actionTable = (CCActionTable)
            getChild(CHILD_ACTION_TABLE);
        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "populateData",
                "ModelControlException occurred within framework",
                ((CommonViewBeanBase) getParentViewBean()).getServerName());
            throw new SamFSException("Exception occurred with framework");
        }

        // media type
        Integer mediaType = (Integer)getParentViewBean().
            getPageSessionAttribute(Constants.Archive.POOL_MEDIA_TYPE);

        model.initModelRows(mediaType.intValue());
        model.initHeaders(mediaType.intValue());

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        Integer mt = (Integer)source.
            getPageSessionAttribute(Constants.Archive.POOL_MEDIA_TYPE);

        String s = (String) getDisplayFieldValue("VSNHref");
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(VSNDetailsViewBean.class);

        // Convert value into an array
        String [] tokenArray = s.split(";");

        // If tokenArr[0] is $STANDALONE, set session attribute EQ to
        // tokenArr[1].
        // Otherwise, VSN is in a library, set LIB_NAME to tokenArray[0] and
        // set SLOTNUM to tokenArray[1]
        source.setPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME, tokenArray[0]);

        if (tokenArray[0].equals("$STANDALONE")) {
            source.setPageSessionAttribute(
                Constants.PageSessionAttributes.EQ, tokenArray[1]);
        } else {
            source.setPageSessionAttribute(
                Constants.PageSessionAttributes.SLOT_NUMBER, tokenArray[1]);
        }

        BreadCrumbUtil.breadCrumbPathForward(source,
        PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
        TraceUtil.trace3("Exiting");
    }
}
