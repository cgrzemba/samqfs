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

// ident	$Id: PendingJobsModel.java,v 1.13 2008/03/17 14:43:38 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import java.util.ArrayList;

/**
 * A CCActionTableModel for the 'Pending Jobs' Action table.
 */
public final class PendingJobsModel extends CCActionTableModel {

    private ArrayList latestRows;
    private String selectedFilter;
    private boolean filtered;
    private String serverName;

    public PendingJobsModel(String serverName) {
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/jobs/PendingJobsTable.xml");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.serverName = serverName;
        initHeaders();
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {

        TraceUtil.trace3("Entering");
        for (int i = 0; i < PendingJobsData.headings.length; i++) {
            setActionValue("Col" + i, PendingJobsData.headings[i]);
        }

        setActionValue("FilterMenu", PendingJobsData.filterOptions[0]);
        setActionValue("Button0", PendingJobsData.button);

        setSelectionType(CCActionTableModelInterface.SINGLE);
        setProductNameAlt("secondaryMasthead.productNameAlt");
        setProductNameSrc("secondaryMasthead.productNameSrc");
        setProductNameHeight(Constants.ProductNameDim.HEIGHT);
        setProductNameWidth(Constants.ProductNameDim.WIDTH);
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows() throws SamFSException {
        TraceUtil.trace3("Entering");
        latestRows = new ArrayList();
        PendingJobsData recordModel = new PendingJobsData(serverName);
        clear();
        for (int i = 0; i < recordModel.size(); i++) {
            Object [] record = (Object []) recordModel.get(i);

            if (filtered &&
                !selectedFilter.equals(record[1].toString()) &&
                !selectedFilter.equals(Constants.Filter.FILTER_ALL)) {
                continue;
            }
            latestRows.add(new Integer(i));

            if (i > 0) {
                appendRow();
            }
            for (int j = 0; j < record.length; j++) {
                setValue("Text" + j, record[j]);
            }

            String hrefValue = record[0].toString() + "," +
                record[1].toString() + "," + "Pending";

            setValue("JobIdHref", hrefValue);
            setValue("JobHidden", record[0]);
            setValue("JobHiddenType", record[1]);
        }
        TraceUtil.trace3("Exiting");
    }

    public void setFilter(String selectedFilter) {
        TraceUtil.trace3("Entering");
        this.filtered = true;
        this.selectedFilter = selectedFilter;
        TraceUtil.trace3("Exiting");
    }

    public ArrayList getLatestIndex() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return latestRows;
    }
}
