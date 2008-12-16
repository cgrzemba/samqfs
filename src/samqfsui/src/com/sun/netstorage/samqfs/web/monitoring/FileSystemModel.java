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

// ident	$Id: FileSystemModel.java,v 1.13 2008/12/16 00:12:23 am143972 Exp $

/**
 * This is the model class of the file system frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.model.CCActionTableModel;

public final class FileSystemModel extends CCActionTableModel {

    // Constructor
    public FileSystemModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/FileSystemTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("FSNameColumn", "Monitor.title.name");
        setActionValue("FSMountedColumn", "Monitor.title.mounted");
        setActionValue("FSCapacityColumn", "Monitor.title.capacity");
        setActionValue("FSUsageColumn", "Monitor.title.usage");
        setActionValue("FSHWMColumn", "Monitor.title.hwm");
    }

    public void initModelRows(String serverName) throws SamFSException {

        // first clear the model
        clear();

        FileSystem allFS [] =
            SamUtil.getModel(serverName).
                getSamQFSSystemFSManager().getAllFileSystems();

        if (allFS == null) {
            return;
        }

        for (int i = 0; i < allFS.length; i++) {
            if (i > 0) {
                appendRow();
            }

            FileSystemMountProperties properties =
                            allFS[i].getMountProperties();

            int usage = allFS[i].getConsumedSpacePercentage();
            int hwm   = properties.getHWM();

            String fsName   = allFS[i].getName();
            boolean mounted = allFS[i].getState() == FileSystem.MOUNTED;
            long capacity   = allFS[i].getCapacity();
            String activity = getActivityString(allFS[i].getStatusFlag());
            boolean isArchiving =
                allFS[i].getArchivingType() == FileSystem.ARCHIVING;


            // set file system name
            setValue(
                "FSNameText",
                isArchiving && hwm < usage ?
                    SamUtil.makeRed(fsName) :
                    fsName);

            // set if file system is mounted
            setValue(
                "FSMountedImage",
                mounted ?
                    Constants.Image.ICON_AVAILABLE :
                    Constants.Image.ICON_BLANK);

            // set file system capacity
            setValue(
                "FSCapacityText",
                mounted ?
                    new Capacity(
                        capacity, SamQFSSystemModel.SIZE_KB).toString() :
                    SamUtil.getResourceString("FSSummary.unmount"));

            // set file system usage bar
            setValue(
                "FSUsageBarImage",
                isArchiving ?
                    SamUtil.getImageBar(hwm < usage, usage) :
                    SamUtil.getImageBar(false, usage));

            // set file system high water mark if fs is an archiving fs
            // show empty string if fs is not an archiving fs
            setValue(
                "FSHWMText",
                isArchiving && hwm < usage ?
                    SamUtil.makeRed(Integer.toString(hwm)) :
                    isArchiving ?
                        Integer.toString(hwm) :
                        "");

            // FUTURE: set file system activities
        }

        return;
    }

    /**
     * Return the file system activity string.  Now we only show if the file
     * system is archiving|releasing|staging.
     *
     * @param statusFlags - flag retrieved from FSInfo
     * @return file system activity string - i18n-ed
     */
    private String getActivityString(int statusFlags) {
        if ((statusFlags & FSInfo.FS_ARCHIVING) == FSInfo.FS_ARCHIVING) {
            return SamUtil.getResourceString("Monitor.archiving");
        } else if ((statusFlags & FSInfo.FS_RELEASING) == FSInfo.FS_RELEASING) {
            return SamUtil.getResourceString("Monitor.releasing");
        } else if ((statusFlags & FSInfo.FS_STAGING) == FSInfo.FS_STAGING) {
            return SamUtil.getResourceString("Monitor.staging");
        } else {
            return "";
        }
    }
}
