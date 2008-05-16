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

// ident	$Id: PendingScanningModel.java,v 1.10 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;

/**
 * A CCActionTableModel for the Pending Archive Scanning Action table.
 */
public final class PendingScanningModel extends CCActionTableModel {

    private String serverName;
    private long jobId;

    public PendingScanningModel(String serverName, long jobId) {
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/jobs/PendingScanningTable.xml");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.serverName = serverName;
        this.jobId = jobId;
        initHeaders();
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {
        TraceUtil.trace3("Entering");
        for (int i = 0; i < PendingScanningData.headings.length; i++) {
            setActionValue("PendingCol" + i, PendingScanningData.headings[i]);
        }
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows() throws SamFSException {
        TraceUtil.trace3("Entering");

        PendingScanningData recordModel =
            new PendingScanningData(serverName, jobId);
        clear();

        for (int i = 0; i < recordModel.size(); i++) {
            Object [] record = (Object []) recordModel.get(i);
            if (i > 0) {
                appendRow();
            }

            for (int j = 0; j < record.length; j++) {
                setValue("PendingText" + j, record[j]);
            }
        }
        TraceUtil.trace3("Exiting");
    }
}
