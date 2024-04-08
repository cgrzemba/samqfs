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

// ident	$Id: HistorianModel.java,v 1.24 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.LDSTableModel;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;

/**
 * This is the Model Class of Historian page
 */

public final class HistorianModel extends LDSTableModel {

    private boolean hasPermission = false;

    // Constructor
    public HistorianModel(LargeDataSet data, boolean hasPermission) {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/media/HistorianTable.xml");

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.dataSet = data;
        this.hasPermission = hasPermission;

        initActionButtons();
        initHeaders();
        initProductName();

        TraceUtil.trace3("Exiting");
    }

    // Initialize action buttons
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("ExportButton", "vsn.button.export");
        setActionValue("SamQFSWizardReserveButton", "vsn.button.reserve");
        setActionValue("UnreserveButton", "vsn.button.unreserve");
        setActionValue("EditVSNButton", "vsn.button.editvsn");
        TraceUtil.trace3("Exiting");
    }


    // Initialize the action table headers
    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("SlotColumn", "vsn.heading.slot");
        setActionValue("VSNColumn", "vsn.heading.vsn");
        setActionValue("BarcodeColumn", "vsn.heading.barcode");
        setActionValue("MediaTypeColumn", "vsn.heading.mediatype");
        setActionValue("UsageColumn", "vsn.heading.usage");
        setActionValue("AccessCountColumn", "vsn.heading.accesscount");
        setActionValue("MediaAttributesColumn", "vsn.heading.attr");
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

    public void initModelRows() throws SamFSException {
        TraceUtil.trace3("Entering");

        super.getModelRows();

        for (int i = 0; i < currentDataSet.length; i++) {
            // append new row
            if (i > 0) {
                appendRow();
            }

            VSN vsn = (VSN) currentDataSet[i];

            String flagsInfo = null;
            String vsnName = vsn.getVSN();
            String barcode = vsn.getBarcode();
            int slotNumber = vsn.getSlotNumber();
            long capacity = vsn.getCapacity();
            long freeSpace = vsn.getAvailableSpace();
            long spaceConsumed = 0;
            long accessCount = vsn.getAccessCount();
            boolean isReserved = vsn.isReserved();

            if (capacity != 0) {
                spaceConsumed = 100 * (capacity - freeSpace) / capacity;
            }

            flagsInfo = MediaUtil.getFlagsInformation(vsn);

            setValue("SlotText", new Integer(slotNumber));
            setValue("VSNText", vsnName);
            setValue("BarcodeText", barcode);
            setValue("MediaTypeText", "Historian.mediaType");
            setValue("AccessCountText", new Long(accessCount));

            if (spaceConsumed < 0 || spaceConsumed > 100) {
                setValue("usageText", new Long(-1));
                setValue("capacityText", "");
                setValue("UsageBarImage", Constants.Image.ICON_BLANK);
            } else {
                setValue("capacityText",
                    new NonSyncStringBuffer("(").append(
                        new Capacity(capacity, SamQFSSystemModel.SIZE_MB)).
                        append(")").toString());
                setValue("usageText", new Long(spaceConsumed));
                setValue("UsageBarImage",
                new NonSyncStringBuffer(Constants.Image.USAGE_BAR_DIR).
                        append(spaceConsumed).append(".gif").toString());
            }

            setValue(
                "MediaAttributesLinkText", hasPermission ? flagsInfo : "");
            setValue(
                "MediaAttributesText", hasPermission ? "" : flagsInfo);

            String delimitor = "###";
            setValue("InformationHidden",
                new NonSyncStringBuffer().append(new Integer(slotNumber)).
                append(delimitor).append(isReserved).toString());

            setValue(HistorianTiledView.CHILD_VSN_HREF,
                Integer.toString(slotNumber));
            setValue(HistorianTiledView.CHILD_MEDIA_HREF,
                Integer.toString(slotNumber));
        }

        TraceUtil.trace3("Exiting");
    }
}
