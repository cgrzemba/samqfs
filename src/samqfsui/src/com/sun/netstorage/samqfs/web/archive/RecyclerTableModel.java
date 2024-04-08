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

// ident	$Id: RecyclerTableModel.java,v 1.11 2008/12/16 00:10:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.RecycleParams;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;

/**
 * A CCActionTableModel for the buffer  Action table.
 */
public final class RecyclerTableModel extends CCActionTableModel {

    // Column headings
    public String[] headings = new String [] {
        "Recycler.perform",
        "Recycler.libname",
        "Recycler.hwm",
        "Recycler.minigain",
        "Recycler.vsn",
        "Recycler.size"
    };

    public RecyclerTableModel() {
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/RecyclerTable.xml");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        initHeaders();
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {
        TraceUtil.trace3("Entering");
        for (int i = 0; i < headings.length; i++) {
            setActionValue(new NonSyncStringBuffer().append(
                "RecyclerCol").append(i).toString(), headings[i]);
        }
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows(String serverName) throws SamFSException {
        TraceUtil.trace3("Entering");

        clear();
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        RecycleParams[] recyclers =
            sysModel.getSamQFSSystemArchiveManager().getRecycleParams();
        for (int i = 0; i < recyclers.length; i++) {
            if (i > 0) {
                appendRow();
            }
            String lib = recyclers[i].getLibraryName();
            setValue("libName", lib);
            setValue("LibNameHiddenField", lib);
        }
        TraceUtil.trace3("Exiting");
    }
}
