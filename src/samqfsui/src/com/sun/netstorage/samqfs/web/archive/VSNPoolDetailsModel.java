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

// ident	$Id: VSNPoolDetailsModel.java,v 1.20 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LDSTableModel;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;

/**
 * A CCActionTableModel for the VSN Pool Details Action table.
 */
public final class VSNPoolDetailsModel extends LDSTableModel {

    public VSNPoolDetailsModel(LargeDataSet data, String xmlFile) {

        super(RequestManager.getRequestContext().getServletContext(), xmlFile);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        dataSet = data;
        TraceUtil.trace3("Exiting");
    }

    /**
     * initialize column headers
     */
    public void initHeaders(int mediaType) {

        TraceUtil.trace3("Entering");
        // Create column headings
        setActionValue("VSNName", "VSNPoolDetails.heading1");
        setActionValue("Usage", "common.capacity.usage");

        if (mediaType == BaseDevice.MTYPE_DISK ||
            mediaType == BaseDevice.MTYPE_STK_5800) {
            setActionValue("Library", "archiving.diskvsn.path");
            setActionValue("Barcode", "archiving.diskvsn.host");
        } else {
            setActionValue("Library", "VSNPoolDetails.heading2");
            setActionValue("Barcode", "VSNPoolDetails.heading3");
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * ppulate the tbale model
     */
    public void initModelRows(int mediaType) throws SamFSException {
        if (mediaType == BaseDevice.MTYPE_DISK ||
            mediaType == BaseDevice.MTYPE_STK_5800) {
            initDiskPoolModelRows(mediaType);
        } else {
            initTapePoolModelRows();
        }
    }

    /**
     * disk-based pools
     */
    public void initDiskPoolModelRows(int mediaType) throws SamFSException {
        TraceUtil.trace3("Entering");

        super.getModelRows();

        // retrieve the server name
        String serverName = ((VSNPoolDetailsData)dataSet).
            getParentViewBean().getServerName();
        for (int i = 0; i < currentDataSet.length; i++) {
            if (i > 0)
                appendRow();

            DiskVolume vol = (DiskVolume)currentDataSet[i];
            setValue("VSNNameText", vol.getName());
            setValue("LibraryText", vol.getPath());

            String host = vol.getRemoteHost();
            if (host == null || host.length() == 0) {
                host = SamUtil.getResourceString(
                    "archiving.currentserver", serverName);
            }
            setValue("BarcodeText", host);

            // change resource & host
            if (mediaType == BaseDevice.MTYPE_STK_5800) {
                setValue("LibraryText", vol.getRemoteHost());
                String [] hp = vol.getRemoteHost().split(":");
                setValue("BarcodeText", hp[0]);
            }

            long capacity = vol.getCapacityKB();
            long free = vol.getAvailableSpaceKB();

            int percentUsed = PolicyUtil.getPercentUsage(capacity, free);

            String imagePath = Constants.Image.ICON_BLANK;
            if (percentUsed != -1) {
                imagePath = Constants.Image.USAGE_BAR_DIR.concat(
                    Integer.toString(percentUsed)).concat(".gif");
            }

            setValue("UsageImage", imagePath);

            String capacityText =
                "(".concat((new Capacity(capacity, SamQFSSystemModel.SIZE_KB))
                           .toString()).concat(")");

            setValue("CapacityText", capacityText);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * tape-based pools
     */
    public void initTapePoolModelRows() throws SamFSException {
        TraceUtil.trace3("Entering");

        super.getModelRows();
        boolean containsReservedVSN = false;

        for (int i = 0; i < currentDataSet.length; i++) { // for each vsn

            if (i > 0) appendRow();

            VSN vsn = (VSN) currentDataSet[i];

            // mark reserved VSNs
            String vsnName = vsn.getVSN();
            if (vsn.isReserved()) {
                containsReservedVSN = true;
                vsnName = vsnName.concat(" ").concat(Constants.Symbol.DAGGER);
            }

            setValue("VSNNameText", vsnName);
            if (vsn.getLibrary() != null)
                setValue("LibraryText", vsn.getLibrary().getName());
            setValue("BarcodeText", vsn.getBarcode());

            long capacity = vsn.getCapacity();
            long free = vsn.getAvailableSpace();

            // Check if VSN is in a library, set VSNHref to the library name
            // and the slot number of the library of which the VSN resides
            // else, set to $STANDALONE and grab the drive EQ of which the
            // VSN resides
            if (vsn.getLibrary() == null) {
                setValue("VSNHref", "$STANDALONE;" +
                         Integer.toString(vsn.getDrive().getEquipOrdinal()));
            } else {
                setValue("VSNHref", vsn.getLibrary().getName() + ";" +
                         Integer.toString(vsn.getSlotNumber()));
            }

            int percentUsed = PolicyUtil.getPercentUsage(capacity, free);

            String imagePath = Constants.Image.ICON_BLANK;
            if (percentUsed != -1) {
                imagePath = Constants.Image.USAGE_BAR_DIR.concat(
                    Integer.toString(percentUsed)).concat(".gif");
            }

            setValue("UsageImage", imagePath);

            String capacityText =
                "(".concat((new Capacity(capacity, SamQFSSystemModel.SIZE_MB))
                           .toString()).concat(")");

            setValue("CapacityText", capacityText);
        }

        // insert a marker in the request so that the begin display method of
        // the parent view bean can insert text describing the asterisk [*]
        RequestManager.getRequestContext().getRequest().
            setAttribute(Constants.Archive.CONTAINS_RESERVED_VSN,
                         Boolean.valueOf(containsReservedVSN));

        TraceUtil.trace3("Exiting");
    }
}
