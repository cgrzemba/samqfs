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

// ident	$Id: DiskVSNSummaryView.java,v 1.13 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;

/**
 * the container view for the disk vsn summary table
 */
public class DiskVSNSummaryView extends CommonTableContainerView {
    // the table model
    private CCActionTableModel model = null;

    /** create a new instance of DiskVSNSummaryView */
    public DiskVSNSummaryView(View view, String name) {
        super(view, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "DiskVSNSummaryTable";
        createTableModel();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this container's children */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    /** create a named child */
    public View createChild(String name) {
        return super.createChild(model, name);
    }

    /**
     * create an empty instance of the table model
     */
    private void createTableModel() {
        TraceUtil.trace3("Entering");

        model = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/DiskVSNSummaryTable.xml");
        TraceUtil.trace3("Exiting");
    }

    /**
     * initilize the table column headers
     */
    private void initializeColumnHeaders() {
        TraceUtil.trace3("Entering");

        // New VSN button label
        model.setActionValue("NewDiskVSN", "archiving.new");
        model.setActionValue("EditMediaFlags", "archiving.diskvsn.edit.flags");

        // column headers
        model.setActionValue("vsnNameColumn", "archiving.diskvsn.name");
        model.setActionValue("vsnPathColumn", "archiving.diskvsn.path");
        model.setActionValue("vsnHostColumn", "archiving.diskvsn.host");
        model.setActionValue("usageColumn", "common.capacity.usage");
        model.setActionValue("flagsColumn", "archiving.diskvsn.flags");

        TraceUtil.trace3("Exiting");
    }

    /**
     * populate the table model rows
     */
    public void populateTableModel() {
        TraceUtil.trace3("Entering");
        DiskVSNSummaryViewBean parent =
            (DiskVSNSummaryViewBean)getParentViewBean();
        String serverName = parent.getServerName();
        NonSyncStringBuffer vsns = new NonSyncStringBuffer();

        // remove tooltips
        ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
         getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            DiskVolume [] diskVolumes =
                sysModel.getSamQFSSystemMediaManager().getDiskVSNs();

            for (int i = 0; i < diskVolumes.length; i++) {
                if (i > 0) {
                    model.appendRow();
                }
                String hostName = diskVolumes[i].getRemoteHost();
                if (serverName.equals(hostName) ||
                    hostName == null ||
                    hostName.length() <= 0) {
                    hostName = SamUtil.getResourceString(
                                   "archiving.diskvsn.currentserver",
                                   serverName);
                }

                model.setValue("vsnName", diskVolumes[i].getName());
                model.setValue("hiddenVSNName", diskVolumes[i].getName());
                model.setValue("vsnHost", hostName);
                model.setValue("vsnPath", diskVolumes[i].getPath());

                // honeycomb targets
                if (diskVolumes[i].isHoneyCombVSN()) {
                    String [] hp = hostName.split(":");

                    model.setValue("vsnHost", hp[0]);
                    model.setValue("vsnPath", hostName);
                }

                long capacity = diskVolumes[i].getCapacityKB();
                long free = diskVolumes[i].getAvailableSpaceKB();

                int percentUsed = PolicyUtil.getPercentUsage(capacity, free);

                String imagePath = Constants.Image.ICON_BLANK;
                if (percentUsed != -1) {
                    imagePath = Constants.Image.USAGE_BAR_DIR.concat(
                        Integer.toString(percentUsed)).concat(".gif");
                }

                model.setValue("UsageImage", imagePath);

                String capacityText = "(".concat((new Capacity(capacity,
                    SamQFSSystemModel.SIZE_KB)).toString()).concat(")");

                model.setValue("CapacityText", capacityText);

                model.setValue("flagsHref", diskVolumes[i].getName());
                model.setValue("flags", getFlagString(diskVolumes[i]));

                // save the vsn names for validation
                vsns.append(diskVolumes[i].getName()).append(";");

                // remove row selections from prior editing
                model.setRowSelected(i, false);
            }
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "populateTableModel",
                                     "Unable to retrieve disk vsns",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "archiving.diskvsn.load.error",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // save the vsns for client-side validation
        CCHiddenField field = (CCHiddenField)parent.getChild(parent.VSNS);
        field.setValue(vsns.toString());

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        initializeColumnHeaders();

        // disable edit media flags button
        CCButton editMediaFlags = (CCButton)getChild("EditMediaFlags");
        editMediaFlags.setDisabled(true);

        // disable selection & new button if no config authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {

            ((CCButton)getChild("NewDiskVSN")).setDisabled(true);
            model.setSelectionType(CCActionTableModel.NONE);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * construct the 'media flags' display string
     */
    private String getFlagString(DiskVolume vsn) {
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        int count = 0;

        // check bad media
        if (vsn.isBadMedia()) {
            buffer.append(SamUtil.getResourceString(
                "archiving.diskvsn.flags.badmedia"));
            count ++;
        }

        // check unavailable
        if (vsn.isUnavailable()) {
            if (count > 0) {
                buffer.append(" ...");
                return buffer.toString();
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.unavailable"));
                count ++;
            }
        }

        // check readOnly
        if (vsn.isReadOnly()) {
            if (count > 0) {
                buffer.append(" ...");
                return buffer.toString();
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.readonly"));
                count ++;
            }
        }

        // check labeleled
        if (vsn.isLabeled()) {
            if (count > 0) {
                buffer.append(" ...");
                return buffer.toString();
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.labeled"));
                count ++;
            }
        }

        // check unknown
        if (vsn.isUnknown()) {
            if (count > 0) {
                buffer.append(" ...");
                return buffer.toString();
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.unknown"));
                count ++;
            }
        }

        // check remote
        if (vsn.isRemote()) {
            if (count > 0) {
                buffer.append(" ...");
                return buffer.toString();
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.remote"));
                count ++;

            }
        }

        return buffer.toString();
    }
}
