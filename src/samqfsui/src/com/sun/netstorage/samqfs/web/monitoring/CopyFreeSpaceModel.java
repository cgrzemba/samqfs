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

// ident	$Id: CopyFreeSpaceModel.java,v 1.14 2008/03/17 14:43:52 am143972 Exp $

/**
 * This is the model class of the copy with low free space frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.Properties;

public final class CopyFreeSpaceModel extends CCActionTableModel {

    // according to Archiver.java comment
    /**
     * name=copy name
     * type=mediatype
     * free=freespace in kbytes
     * usage=%
     */
    public static final String NAME = "name";
    public static final String TYPE = "type";
    public static final String FREE_SPACE = "free";
    public static final String USAGE  = "usage";
    public static final String CAPACITY  = "capacity";

    // Constructor
    public CopyFreeSpaceModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/CopyFreeSpaceTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("CopyPolicyNameColumn", "Monitor.title.policyname");
        setActionValue("CopyNumberColumn", "Monitor.title.copynumber");
        setActionValue("CopyTypeColumn", "Monitor.title.type");
        setActionValue("CopyUsageColumn", "Monitor.title.usage");
        setActionValue("CopyCapacityColumn", "Monitor.title.capacity");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        String [] details = SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getTopUsageCopies(25);

        if (details == null) {
            return;
        }

        for (int i = 0; i < details.length; i++) {
            Properties props = ConversionUtil.strToProps(details[i]);

            if (i > 0) {
                appendRow();
            }

            String copyName    = props.getProperty(NAME, "");
            String [] copyInfo = copyName.split("\\.");
            setValue("CopyPolicyNameText", copyInfo[0]);
            setValue("CopyNumberText", copyInfo.length == 2 ? copyInfo[1] : "");
            setValue("CopyTypeText", SamUtil.getMediaTypeString(
                                                props.getProperty(TYPE, "")));

            int capacity = -1;
            try {
                capacity = Integer.parseInt(props.getProperty(CAPACITY, ""));
            } catch (NumberFormatException numEx) {
                // do nothing
            }

            setValue(
                "CopyCapacityText",
                capacity == -1 ? "" :
                new Capacity(capacity, SamQFSSystemModel.SIZE_KB).toString());

            int usage = -1;
            try {
                usage = Integer.parseInt(props.getProperty(USAGE, ""));
            } catch (NumberFormatException numEx) {
                // do nothing
            }
            setValue("CopyUsageImage",
                new StringBuffer(
                    Constants.Image.USAGE_BAR_DIR).
                    append(usage).append(".gif").toString());
        }

        return;
    }
}
