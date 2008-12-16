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

// ident	$Id: HistorianData.java,v 1.22 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.web.ui.model.CCActionTableModelInterface;

/**
 *  This class is the data class for Historian actiontable
 */

public final class HistorianData implements LargeDataSet {

    private String serverName;

    /**
     * Construct a model
     */
    public HistorianData(String serverName) {
        TraceUtil.initTrace();
        this.serverName = serverName;
        TraceUtil.trace3("Entering");
    }

    public Object[] getData(
        int start,
        int num,
        String sortName,
        String sortOrder)
        throws SamFSException {

        SamUtil.doPrint(new NonSyncStringBuffer().append("Entering start = ").
            append(start).append(" num = ").append(num).append(
            " sortName = ").append(sortName).append(" sortOrder = ").append(
            sortOrder).toString());

        Library myLibrary = getHistorian();
        if (myLibrary == null) {
            throw new SamFSException(null, -2506);
        }

        int sortby = Media.VSN_SORT_BY_SLOT;

        if (sortName != null) {
            if ("SlotText".compareTo(sortName) == 0) {
            sortby = Media.VSN_SORT_BY_SLOT;
            } else if ("VSNText".compareTo(sortName) == 0) {
                sortby = Media.VSN_SORT_BY_NAME;
            } else if ("FreeSpaceText".compareTo(sortName) == 0) {
                sortby = Media.VSN_SORT_BY_FREESPACE;
            }
        }

        boolean ascending = true;
        if (sortOrder != null &&
            CCActionTableModelInterface.DESCENDING.compareTo(sortOrder) == 0) {
            ascending = false;
        }

        VSN allVSN[] = myLibrary.getVSNs(start, num, sortby, ascending);
        if (allVSN == null) {
            allVSN = new VSN[0];
        }

        TraceUtil.trace3("Exiting");
        return (Object[]) allVSN;
    }

    public int getTotalRecords() throws SamFSException {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return getHistorian().getTotalVSNInLibrary();
    }

    private Library getHistorian() throws SamFSException {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        Library myLibrary = sysModel.getSamQFSSystemMediaManager().
             getLibraryByName(Constants.MediaAttributes.HISTORIAN_NAME);
        if (myLibrary == null) {
            throw new SamFSException(null, -2506);
        } else {
            TraceUtil.trace3("Exiting");
            return myLibrary;
        }
    }
}
