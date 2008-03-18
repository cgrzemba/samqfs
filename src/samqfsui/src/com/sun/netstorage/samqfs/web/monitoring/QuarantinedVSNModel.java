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

// ident	$Id: QuarantinedVSNModel.java,v 1.15 2008/03/17 14:43:53 am143972 Exp $

/**
 * This is the model class of the Quarantined VSN frame, also known as
 * Unusable VSN in the GUI
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.Properties;

public final class QuarantinedVSNModel extends CCActionTableModel {

    // According to Media.java in JNI
    private static final String TYPE = "type";
    private static final String NAME = "vsn";
    private static final String STATUS = "status";


    // Constructor
    public QuarantinedVSNModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/QuarantinedVSNTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("QuarVSNColumn", "Monitor.title.vsn");
        setActionValue("QuarTypeColumn", "Monitor.title.type");
        setActionValue("QuarStatusColumn", "Monitor.title.status");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        // get only the bad, duplicate, foreign or unavail vsns
        String [] details = SamUtil.getModel(serverName).
            getSamQFSSystemMediaManager().
                getUnusableVSNs(
                    Media.EQU_MAX,
                    CatEntry.CES_bad_media | CatEntry.CES_dupvsn |
                        CatEntry.CES_non_sam | CatEntry.CES_unavail);

        if (details == null) {
            return;
        }

        for (int i = 0; i < details.length; i++) {
            TraceUtil.trace3("Unused VSN: ".concat(details[i]));
            Properties props = ConversionUtil.strToProps(details[i]);

            if (i > 0) {
                appendRow();
            }

            setValue("QuarVSNText", props.getProperty(NAME, ""));

            setValue(
                "QuarTypeText",
                SamUtil.getMediaTypeString(
                    SamQFSUtil.getEquipTypeInteger(
                        props.getProperty(TYPE, ""))));

            long status = -1;
            String statusStr = props.getProperty(STATUS, "");
            TraceUtil.trace2("statusStr: " + statusStr + "<end>");
            try {
                status = Long.parseLong(statusStr);
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1(
                    "NumberFormatException caught while parsing status! " +
                    numEx.getMessage());
            }
            TraceUtil.trace2("Unusable VSN status: " + status);
            setValue("QuarStatusText", SamUtil.getStatusString(status));
        }

        return;

    }

}
