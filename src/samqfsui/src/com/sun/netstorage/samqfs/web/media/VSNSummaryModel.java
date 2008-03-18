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

// ident	$Id: VSNSummaryModel.java,v 1.23 2008/03/17 14:43:40 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.Authorization;

import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LDSTableModel;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;


/**
 * This is the Model Class of VSN Summary page
 */

public final class VSNSummaryModel extends LDSTableModel {

    // Constructor
    public VSNSummaryModel(LargeDataSet data) {

        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/media/VSNSummaryTable.xml");

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        dataSet = data;

        initActionButtons();
        initActionMenu();
        initHeaders();
        initProductName();

        TraceUtil.trace3("Exiting");
    }

    // Initialize action buttons.
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("LabelButton", "vsn.button.label");
        setActionValue("SamQFSWizardReserveButton", "vsn.button.reserve");
        setActionValue("UnreserveButton", "vsn.button.unreserve");
        setActionValue("EditVSNButton", "vsn.button.editvsn");
        TraceUtil.trace3("Exiting");
    }

    private void initActionMenu() {
        TraceUtil.trace3("Entering");
        setActionValue("ActionMenu", "VSNSummary.option1");
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("SlotColumn", "vsn.heading.slot");
        setActionValue("VSNColumn", "vsn.heading.vsn");
        setActionValue("BarcodeColumn", "vsn.heading.barcode");
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

            String vsnName = vsn.getVSN();
            String barcode = vsn.getBarcode();
            int slotNumber = vsn.getSlotNumber();
            long capacity  = vsn.getCapacity();
            long freeSpace = vsn.getAvailableSpace();
            long spaceConsumed = 0;
            long accessCount   = vsn.getAccessCount();
            boolean isReserved = vsn.isReserved();
            String loaded = null;
            String flagsInfo = null;

            if (capacity != 0) {
                spaceConsumed = 100 * (capacity - freeSpace) / capacity;
            }

            flagsInfo = MediaUtil.getFlagsInformation(vsn);
            // Remove trailing commas and space
            if (flagsInfo.indexOf(',') == flagsInfo.length() - 2) {
                flagsInfo = flagsInfo.substring(0, flagsInfo.length() - 2);
            }

            if (vsn.getDrive() == null) {
                // VSN is not loaded in any drives
                loaded = "not-loaded";
            } else {
                loaded = "loaded";
            }

            setValue("SlotText", new Integer(slotNumber));
            setValue("VSNText", vsnName);
            setValue("BarcodeText", barcode);
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

            String reservedString =
                SamUtil.getResourceString("EditVSN.reserved");
            // Show HREF if at least one media flag exists, exclude "Reserved".
            // Show "Reserved" in plain text if it is the only word in this
            // column
            boolean showInHref =
                hasPermission() && !reservedString.equals(flagsInfo);
            setValue(
                "MediaAttributesLinkText", showInHref ? flagsInfo : "");
            setValue(
                "MediaAttributesText", showInHref ? "" : flagsInfo);

            String delimitor = "###";
            setValue("InformationHiddenField",
                new NonSyncStringBuffer().append(new Integer(slotNumber)).
                append(delimitor).append(vsnName).append(delimitor).
                append(barcode).append(delimitor).append(isReserved).
                append(delimitor).
                append(loaded).toString());

            setValue(VSNSummaryTiledView.CHILD_VSN_HREF,
                Integer.toString(slotNumber));
            setValue(VSNSummaryTiledView.CHILD_EDIT_HREF,
                Integer.toString(slotNumber));
        }
        TraceUtil.trace3("Exiting");
    }

    private boolean hasPermission() throws SamFSException {
        return SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG);
    }
}
