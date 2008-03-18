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

// ident	$Id: VSNPoolDetailsData.java,v 1.19 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 * Used to populate the VSN Pool Summary Action Table.
 */
public final class VSNPoolDetailsData implements LargeDataSet {
    // Column headings
    public static final String[] headings = new String [] {
        "VSNPoolDetails.heading1",
        "VSNPoolDetails.heading2",
        "VSNPoolDetails.heading3",
        "VSNPoolDetails.heading4",
        "VSNPoolDetails.heading5"};

    // parent view bean
    private CommonViewBeanBase parentViewBean = null;

    /**
     * Retrieves VSN Pool Data from the Model.
     */
    public VSNPoolDetailsData(CommonViewBeanBase parentViewBean) {
        TraceUtil.initTrace();

        this.parentViewBean = parentViewBean;
    }

    public Object [] getData(int start,
                             int num,
                             String sortName,
                             String sortOrder) throws SamFSException {
        TraceUtil.trace3("Entering");

        String serverName = parentViewBean.getServerName();
        String poolName = (String)parentViewBean.
            getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        VSNPool vsnpool =
            sysModel.getSamQFSSystemArchiveManager().getVSNPool(poolName);

        if (vsnpool == null)
            throw new SamFSException(null, -2010);

        int sortby = Media.VSN_SORT_BY_NAME;
        if (sortName != null) {
            if ("VSNNameText".compareTo(sortName) == 0) {
                sortby = Media.VSN_SORT_BY_NAME;
            } else if ("FreeSpaceText".compareTo(sortName) == 0) {
                sortby = Media.VSN_SORT_BY_FREESPACE;
            }
        }

        boolean ascending = true;
        if (sortOrder != null && "descending".compareTo(sortOrder) == 0)
            ascending = false;

        Integer mediaType = (Integer)parentViewBean.
            getPageSessionAttribute(Constants.Archive.POOL_MEDIA_TYPE);

        Object [] vsns = null;
        if (mediaType.intValue() == BaseDevice.MTYPE_DISK ||
            mediaType.intValue() == BaseDevice.MTYPE_STK_5800) {
            vsns = vsnpool.getMemberDiskVolumes(start, num, sortby, ascending);
        } else {
            vsns = vsnpool.getMemberVSNs(start, num, sortby, ascending);
        }

        if (vsns == null)
            return new Object[0];

        TraceUtil.trace3("Exiting");
        return vsns;
    }

    public int getTotalRecords() throws SamFSException {
        TraceUtil.trace3("Entering");

        String serverName = parentViewBean.getServerName();
        String poolName = (String)parentViewBean.
            getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        VSNPool vsnpool =
            sysModel.getSamQFSSystemArchiveManager().getVSNPool(poolName);

        if (vsnpool == null)
            throw new SamFSException(null, -2010);

        TraceUtil.trace3("Exiting");
        return vsnpool.getNoOfVSNsInPool();
    }

    public CommonViewBeanBase getParentViewBean() {
        return parentViewBean;
    }
}
