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

// ident	$Id: LDSTableModel.java,v 1.10 2008/05/16 18:39:06 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.web.ui.model.CCActionTableModel;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;


/**
 * This is the Model Class for displaying large data set
 */

public class LDSTableModel extends CCActionTableModel {

    protected LargeDataSet dataSet = null;
    protected Object[] currentDataSet = null;
    protected boolean initModelRows = true;
    protected int totalRecords = -1;
    protected String sortName = null;
    protected String sortOrder = null;

    // Constructor
    public LDSTableModel(ServletContext cntx, String file) {
        super(cntx, file);
        TraceUtil.initTrace();
    }

    /**
     * Get total number of available rows.
     *
     * @return The total number of available rows.
     */
    public int getAvailableRows() {
        if (totalRecords == -1) {
            return 0;
        }
        return totalRecords;
    }

    /**
     * Get first row for a paginated view.  Starts with 1.
     *
     * @return The first row for a paginated view.
     */
    public int getFirstRow() {
        Boolean isPaginated = getShowPaginationControls();
        if (isPaginated == null || isPaginated.booleanValue()) {
            int page = (super.getPage() > 0) ? super.getPage() : 1;
            return (page - 1) * getMaxRows() + 1;
        } else {
            // Not paginating
            return 1;
        }
    }

    /**
     * Get last row for a paginated view.
     *
     * @return The last row for a paginated view.
     */
    public int getLastRow() {
        Boolean isPaginated = getShowPaginationControls();
        if (isPaginated == null || isPaginated.booleanValue()) {
            int page = (super.getPage() > 0) ? super.getPage() : 1;
            return (page * getMaxRows());
        } else {
            // Not paginated
            if (currentDataSet == null) {
                return 0;
            } else {
                return currentDataSet.length;
            }

        }
    }

    /**
     * Get first row index used to retrieve model rows for a paginated
     * view. This value is used when the model does not contain all
     * available rows. For example, the model only contains 10 rows of
     * data, but there are 1000 rows available in a paginated view.
     *
     * @return The first row index used to retrieve model rows for a
     * paginated view.
     */
    public int getFirstRowIndex() {
        // Always return the first model row.
        return 0;
    }

    /**
     * Get last row index used to retrieve model rows for a paginated
     * view.
     *
     * @return The last row index used to retrieve model rows for a
     * paginated view.
     */
    public int getLastRowIndex() {
        // Always return total number of rows.
        return getNumRows();
    }

    /**
     * Get max number of rows for a paginated view.
     *
     * @return The max number of rows for a paginated view.
     */
    public int getMaxRows() {
        // we need to know how many rows to retrieve in the model.
	return Constants.TableRow.MAX_ROW;
    }

    /**
     * Get an array containing an index of sorted rows. This array is
     * used to index through model rows to obtain the next sorted
     * object. Note: The sort() method will be evoked, if a sort has
     * not already been performed.
     *
     * @return The sort index.
     */
    public int[] getSortIndex() {
        // Since the model can only sort the data provided, data must
        // be sorted prior to initializing model rows. Since we don't
        // want the model to sort the current page, we'll return an
        // array of indexes in numerical order and bypass the sort.
        int[] sortIndex = new int[getNumRows()];

        for (int i = 0; i < getNumRows(); i++) {
            sortIndex[i] = i;
        }

        return sortIndex;
    }

    protected void getModelRows() throws SamFSException {
        clear();

        int firstRow = getFirstRow() -1;
        int num;
        Boolean isPaginated = getShowPaginationControls();
        if (isPaginated == null || isPaginated.booleanValue()) {
            num = getMaxRows();
        } else {
            num = getAvailableRows();
        }
        if ((sortName = getPrimarySortName()) != null) {
            sortOrder = getPrimarySortOrder();
        }

	TraceUtil.trace3(new StringBuffer().append("firstRow = ").append(
            firstRow).append(", num = ").append(num).toString());
	TraceUtil.trace3(new StringBuffer().append("sortName = ").append(
            sortName).append(",sortOrder = ").append(sortOrder).toString());

        HttpServletRequest request = (HttpServletRequest)
            RequestManager.getRequestContext().getRequest();
        String queryString = request.getQueryString();

	TraceUtil.trace3(new StringBuffer().append(
            "QueryString = ").append(queryString).toString());

        String tmpStr = request.getParameter("sortName");
        if (tmpStr != null) {
            sortName = tmpStr;
	}

        tmpStr = request.getParameter("sortOrder");
        if (tmpStr != null) {
            sortOrder = tmpStr;
	}

        TraceUtil.trace3(new StringBuffer().append("sortName = ").append(
            sortName).append(",sortOrder = ").append(sortOrder).toString());

        currentDataSet = dataSet.getData(firstRow, num, sortName, sortOrder);

        if (currentDataSet != null && currentDataSet.length < num) {
            int page = (super.getPage() > 0) ? super.getPage() : 1;
            totalRecords = (page - 1) * getMaxRows() + currentDataSet.length;
        }

        if (totalRecords == -1) {
            totalRecords = dataSet.getTotalRecords();
        }
    }

    public Object getCurrentData(int index) {
        if (index < 0 || index > currentDataSet.length) {
            return null;
        }

        return (Object)currentDataSet[index];
    }

    public Object[] getCurrentData() {
        return currentDataSet;
    }
}
