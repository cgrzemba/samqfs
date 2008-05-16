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

// ident	$Id: AllJobsModel.java,v 1.15 2008/05/16 18:38:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import java.util.ArrayList;

/**
 * A CCActionTableModel for the 'All Jobs' Action table.
 */
public final class AllJobsModel extends CCActionTableModel {

    private ArrayList latestRows;
    private String selectedFilter;
    private boolean filtered;
    private String serverName;

    public AllJobsModel(String svrName) {
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/jobs/AllJobsTable.xml");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        serverName = svrName;
        initHeaders();
        // createModel();
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {

        TraceUtil.trace3("Entering");
        for (int i = 0; i < AllJobsData.headings.length; i++) {
            setActionValue("Col" + i, AllJobsData.headings[i]);
        }

        setActionValue("FilterMenu", AllJobsData.filterOptions[0]);
        setActionValue("Button0", AllJobsData.button);

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
        AllJobsData recordModel = new AllJobsData(serverName);
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

            // see if this is current or pending
            String conditionString = null;
            if ((record[4].toString()).equals("Jobs.conditionCurrent")) {
                conditionString = "Current";
            } else {
                conditionString = "Pending";
            }

            String hrefValue = record[0].toString() + "," +
            record[1].toString() + "," +
            conditionString + "," + "All";

            setValue("JobIdHref", hrefValue);

            setValue("JobHidden", record[0]);

            String jobTypeString = record[1].toString();
            String jobTypeCondition = record[4].toString();

            String typeCondition =
                jobTypeString.concat(",").concat(jobTypeCondition);

            setValue("JobHiddenType", typeCondition);
            if ((record[1].toString()).equals("Jobs.jobType6")) {
                String description = record[3].toString();
                // easier to look for non-repair sub-string and do nothing
                // as repair is a substring of non-repair, so looking for
                // repair won't do any good
                if (description.indexOf(
                    SamUtil.getResourceString("Jobs.non-repair")) == -1) {
                    setValue("JobHiddenType",
                        typeCondition.concat(",").concat("repair"));
                }
            }
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
