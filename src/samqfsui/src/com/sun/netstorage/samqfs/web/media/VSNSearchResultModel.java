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

// ident	$Id: VSNSearchResultModel.java,v 1.18 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.web.ui.model.CCActionTableModel;

import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;

/**
 * This is the model class of the VSN Search Result page
 */
public final class VSNSearchResultModel extends CCActionTableModel {

    // Constructor
    public VSNSearchResultModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/media/VSNSearchResultTable.xml");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        initHeaders();
        initProductName();
        TraceUtil.trace3("Exiting");
    }

    // Initialize the action table headers
    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("VSNColumn", "VSNSearchResult.heading1");
        setActionValue("MediaTypeColumn", "VSNSearchResult.heading2");
        setActionValue("LibraryNameColumn", "VSNSearchResult.heading3");
        setActionValue("CapacityColumn", "VSNSearchResult.heading4");
        setActionValue("FreeSpaceColumn", "VSNSearchResult.heading5");
        setActionValue("SpaceConsumedColumn", "VSNSearchResult.heading6");
        TraceUtil.trace3("Exiting");
    }

    // Initialize product name for secondary page
    private void initProductName() {
        TraceUtil.trace3("Entering");
        setProductNameAlt("secondaryMasthead.productNameAlt");
        setProductNameSrc("secondaryMasthead.productNameSrc");
        setProductNameHeight(Constants.ProductNameDim.HEIGHT);
        setProductNameWidth(Constants.ProductNameDim.WIDTH);
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows(
        String serverName,
        String parent,
        String searchString,
        String libraryName)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        // Clear up the model first
        clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        VSN allVSN[] = null;
        Library myLibrary = null;
        long usage = 0, capacity = 0;

        allVSN = sysModel.getSamQFSSystemMediaManager().
            searchVSNInLibraries(searchString);
        if (allVSN == null) {
            return;
        }

        // Finish retrieving information, now start populating table model
        // Now search in allVSN, if the VSN is in libName, show in AT
        for (int i = 0; i < allVSN.length; i++) {
            // Search every VSN in allVSN if parent is Library Summary Page
            // Otherwise (VSNSummary or VSNDetails) check the libName and skip
            // the entries which do not match the libName
            if (!parent.equals("LibrarySummaryViewBean")) {
                if (!allVSN[i].getLibrary().getName().equals(libraryName)) {
                    continue;
                }
            }

            capacity = allVSN[i].getCapacity();
            if (capacity != 0) {
                usage = 100 * (capacity -
                    allVSN[i].getAvailableSpace()) / capacity;
            }
            // append new row
            if (i > 0) {
                appendRow();
            }

            setValue("VSNText", allVSN[i].getVSN());
            setValue("MediaTypeText",
                SamUtil.getMediaTypeString(allVSN[i].getLibrary().
                getEquipType()));
            setValue("LibraryNameText", allVSN[i].getLibrary().getName());
            setValue("CapacityText", new Long(allVSN[i].getCapacity()));
            setValue("FreeSpaceText", new Long(allVSN[i].getAvailableSpace()));
            setValue("SpaceConsumedText", new Long(usage));
            setValue("VSNHidden", allVSN[i].getVSN());

            // VSN Details needs two pieces of information to retrieve
            // the correct VSN Object (SLOTNUM + LIB_NAME)
            SamUtil.doPrint(new NonSyncStringBuffer("Slot Number: ").
                append(allVSN[i].getSlotNumber()).toString());
            SamUtil.doPrint(new NonSyncStringBuffer("Library Name: ").
                append(allVSN[i].getLibrary().getName()).toString());

            setValue("VSNHref",
                new NonSyncStringBuffer(
                    Integer.toString(allVSN[i].getSlotNumber())).
                append(",").
                append(allVSN[i].getLibrary().getName()).toString());
        } // End for loop
        TraceUtil.trace3("Exiting");
    }
}
