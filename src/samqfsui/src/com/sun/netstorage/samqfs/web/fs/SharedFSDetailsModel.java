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

// ident	$Id: SharedFSDetailsModel.java,v 1.11 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.ArrayList;

/**
 * This class the model class for FileSystemSummary actiontable
 */

public final class SharedFSDetailsModel extends CCActionTableModel {

    private ArrayList latestRows;

    public String partialErrMsg = null;
    public String sharedMDServer = null;

    private String fsName = null;
    private String hostName = null;

    // Constructor
    public SharedFSDetailsModel(String xmlFile, String hostName, String name) {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            xmlFile);

        TraceUtil.trace3("Entering Shared FS Model: serverName is " + hostName +
            "fsName is " + name);

        fsName = name;
        this.hostName = hostName;
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        initActionButtons();
        initActionMenu();
        initHeaders();
        initProductName();
        TraceUtil.trace3("Exiting");
    }

    // Initialize action buttons.
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("AddButton", "SharedFSDetails.button.add");
        setActionValue("DeleteButton", "SharedFSDetails.button.delete");
        TraceUtil.trace3("Exiting");
    }

    // Initialize action menu
    private void initActionMenu() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("init action menu");
        setActionValue("ActionMenu", "SharedFSDetails.option.heading");
        TraceUtil.trace3("Exiting");
    }

    // Initialize table header
    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("HostColumn", "SharedFSDetails.heading.host");
        setActionValue("TypeColumn", "SharedFSDetails.heading.type");
        setActionValue("StateColumn", "SharedFSDetails.heading.state");
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

    // Initialize table model
    public void initModelRows() throws SamFSException, SamFSMultiHostException {
        TraceUtil.trace3("Entering");
        SharedFSDetailsData recordModel =
            new SharedFSDetailsData(hostName, fsName);

        clear();
        latestRows = new ArrayList();
        boolean allMounted = true;
        boolean allClientNotMounted = true;
        boolean allClientMounted = true;
        boolean mdMounted = false;

        partialErrMsg = recordModel.partialErrMsg;
        sharedMDServer = recordModel.sharedMDServer;
        for (int i = 0; i < recordModel.size(); i++) {
            Object[] record = (Object[]) recordModel.get(i);

            latestRows.add(new Integer(i));
            // append new row
            if (i > 0) {
                appendRow();
            }

            boolean isMounted = ((Boolean) record[2]).booleanValue();
            String mountState =
                isMounted?
                    SamUtil.getResourceString("FSSummary.mount") :
                    SamUtil.getResourceString("FSSummary.unmount");

            int type = ((Integer) record[1]).intValue();

            allMounted = !isMounted;
            allClientNotMounted =
                !isMounted || type == SharedMember.TYPE_MD_SERVER;
            allClientMounted =
                isMounted || type == SharedMember.TYPE_MD_SERVER;
            mdMounted =
                isMounted && type == SharedMember.TYPE_MD_SERVER;

            setValue("HostText", record[0]);
            setValue("TypeText", FSUtil.getSharedFSDescriptionString(type));
            setValue("StateText", mountState);
            setValue("HiddenMount", mountState);
            setValue("FSHiddenField", record[0]);
            setValue("HiddenType", FSUtil.getSharedFSDescriptionString(type));
            setValue(SharedFSDetailsTiledView.CHILD_HREF, record[0]);
        }

        TraceUtil.trace3("model FsName " + fsName);
        TraceUtil.trace3("model HostName " + hostName);
        setValue("HiddenFsName", fsName);
        setValue("HiddenHostName", hostName);

        // The following hidden values are used to determine if some
        // buttons should be disabled or enabled.
        setValue("HiddenAllMount",
            allMounted ? "allUnMounted" : "notAllUnMounted");
        setValue("HiddenAllClientMount",
            allClientNotMounted ?
                "allClientUnMounted": "notAllClientUnMounted");
        setValue("HiddenAllClientMount",
            allClientMounted ? "allClientMounted" : "");
        setValue("HiddenMDMount",
            mdMounted ? "MDMounted" : "MDUnMounted");
        TraceUtil.trace3("Exiting");
    }
}
