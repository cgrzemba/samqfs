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

// ident	$Id: FileSystemSummaryModel.java,v 1.70 2008/09/10 17:40:24 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.ArrayList;

/**
 * This class the model class for FileSystemSummary actiontable
 */

public class FileSystemSummaryModel extends CCActionTableModel {

    // variables for filter.
    private boolean filtered = false;
    private int filterSetting = -1;
    private ArrayList latestRows = new ArrayList();

    private int systemSetup = -1;

    // the following constants should match indice used in js/fs/FSSummary.js
    // for those GUI components that will be dynamically enabled/disabled on
    // client side by javascript
    public static final int ViewPolicyButton = 0;
    public static final int ViewFilesButton = 1;
    public static final int SamQFSWizardNewPolicyButton = 2;

    // this is for QFS standalone installation
    public static final int ViewFilesButton_QFS = 0;

    // The following integer definition should match the values that are
    // defined in the "option value" in the dropdown menu in the JSP
    public static final int OPERATIONS_LABEL = 0;
    public static final int EDIT_MOUNT_OPTION = 1;
    public static final int CHECK_FS = 2;
    public static final int MOUNT = 3;
    public static final int UNMOUNT = 4;
    public static final int GROW_FS = 5;
    public static final int DELETE = 6;
    public static final int ARCHIVE_ACTIVITIES = 7;
    public static final int SCHEDULE_DUMP = 8;
    public static final int SHRINK_FS = 9;

    // The following integer arrays hold the sequence of menu items that are
    // shown in the page (physical location)
    public static final int [] qfsOnly  = {
        OPERATIONS_LABEL,
        EDIT_MOUNT_OPTION,
        CHECK_FS,
        MOUNT,
        UNMOUNT,
        GROW_FS,
        SHRINK_FS,
        DELETE
    };

    public static final int [] sam = {
        OPERATIONS_LABEL,
        EDIT_MOUNT_OPTION,
        CHECK_FS,
        MOUNT,
        UNMOUNT,
        GROW_FS,
        SHRINK_FS,
        DELETE,
        ARCHIVE_ACTIVITIES,
        SCHEDULE_DUMP
    };

    public static final String UFS_ROOT = "/";

    // Constructor
    private FileSystemSummaryModel() {

    }

    public FileSystemSummaryModel(int systemSetup, String xmlSchema) {

        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            xmlSchema);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.systemSetup = systemSetup;

        initActionButtons();
        initActionMenu();
        initFilterMenu();
        initHeaders();
        initProductName();
        TraceUtil.trace3("Exiting");
    }

    // Initialize action buttons.
    protected void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("SamQFSWizardNewFSButton", "FSSummary.NewFSButton");
        setActionValue("ViewPolicyButton", "FSSummary.ViewPolicyButton");
        setActionValue("ViewFilesButton", "FSSummary.ViewFilesButton");
        setActionValue(
            "SamQFSWizardNewPolicyButton", "FSSummary.NewArchivePolicyButton");
        TraceUtil.trace3("Exiting");
    }

    // Initialize action menu
    protected void initActionMenu() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("init action menu");
        setActionValue("ActionMenu", "FSSummary.actionMenu.defaultOption");
        TraceUtil.trace3("Exiting");
    }

    // Initialize filter menu
    protected void initFilterMenu() {
        TraceUtil.trace3("Entering");
        setActionValue("FilterMenu", "FSSummary.filterOption.qfs");
        TraceUtil.trace3("Exiting");
    }

    // Initialize table header
    protected void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("fsName", "FSSummary.heading.fsName");
        setActionValue("type", "FSSummary.heading.type");
        setActionValue("usage", "FSSummary.heading.diskusage");
        setActionValue("highWaterMark", "FSSummary.heading.highWaterMark");
        setActionValue("mountPoint", "FSSummary.heading.mountPoint");
        setActionValue("nfsShared", "FSSummary.heading.nfsShared");
        setActionValue("dumpScheduled", "FSSummary.heading.dumpScheduled");
        TraceUtil.trace3("Exiting");
    }

    // Initialize product name for secondary page
    protected void initProductName() {
        TraceUtil.trace3("Entering");
        setProductNameAlt("secondaryMasthead.productNameAlt");
        setProductNameSrc("secondaryMasthead.productNameSrc");
        setProductNameHeight(Constants.ProductNameDim.HEIGHT);
        setProductNameWidth(Constants.ProductNameDim.WIDTH);
        TraceUtil.trace3("Exiting");
    }

    // Initialize table model
    /**
     * Special Note on the description column of the FS Summary Action Table.
     * Due to the change of terminology, the FS Description (formerly
     * FS Type) is changed.  Due to the tight relationship of the fsType and
     * the code logic behind it, FSUtil.getFSDescription method was created to
     * get the description of the file system.  Moreover, this new method will
     * be used in the filter of the FS Summary page as well.
     */
    public void initModelRows(String serverName) throws SamFSException {
        clear();
        latestRows.clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        SamQFSSystemFSManager fsManager = sysModel.getSamQFSSystemFSManager();
        FileSystem[] fsList = fsManager.getAllFileSystems();
        GenericFileSystem[] fileSystems = null;

        fileSystems = fsManager.getNonSAMQFileSystems();

        if (fsList == null && fileSystems == null) {
            TraceUtil.trace3("fsList is null or fileSystems is null!");
            return;
        }

        // add SAM fs to the model
        int i = 0;
        boolean archiveEnabled  = false;
        boolean growEnabled = false;
        boolean shrinkEnabled = false;
        boolean mountEnabled = false;
        boolean umountEnabled = false;
        boolean deleteEnabled = false;
        boolean fsckEnabled = false;

        for (i = 0; fsList != null && i < fsList.length; i++) {
            String name  = null,
                   point = null,
                   state = null;

            long cap  = 0,
                 free = 0;
            int consumed = 0,
                hwm = 0;

            // name
            name = fsList[i].getName();

            // type
            int fsType = fsList[i].getFSTypeByProduct();

            // definition comes from FSUtil, default to UNKNOWN (-1)
            int fsDescription = FSUtil.getFileSystemDescription(fsList[i]);

            // state
            int fsState = fsList[i].getState();
            state = (fsState == FileSystem.MOUNTED) ?
                "FSSummary.mount" : "FSSummary.unmount";

            // share status
            int fsShareStatus = fsList[i].getShareStatus();
            fsckEnabled = (fsShareStatus == FileSystem.UNSHARED) ||
                ((fsShareStatus == FileSystem.SHARED_TYPE_MDS) &&
                    (fsState == FileSystem.UNMOUNTED));

            // capacity
            cap = fsList[i].getCapacity();

            // space consumed
            consumed = fsList[i].getConsumedSpacePercentage();

            // free space
            free = fsList[i].getAvailableSpace();

            // moint point
            point = fsList[i].getMountPoint();

            // high water mark
            FileSystemMountProperties properties =
                fsList[i].getMountProperties();
            hwm = properties.getHWM();

            // check if the fs is archive enabled. Note archive
            // management can only be done on the MDS if the fs is
            // shared. So make sure this is the mds or the fs is not
            // shared.
            archiveEnabled = !fsList[i].isHA() &&
                systemSetup != FileSystemSummaryView.SETUP_QFS &&
                (fsList[i].getArchivingType() == FileSystem.ARCHIVING) &&
                ((fsShareStatus == FileSystem.UNSHARED) ||
                (fsShareStatus == FileSystem.SHARED_TYPE_MDS));

            // filter the model if need
            if (filtered &&
                shouldThisFSBeExcluded(
                    fsDescription, fsList[i].hasNFSShares())) {
                continue;
            }

            latestRows.add(new Integer(i));

            // append new row
            if (i > 0) {
                appendRow();
            }

            setValue("fsNameText", name);
            setValue("typeText",
                FSUtil.getFileSystemDescriptionString(fsList[i]));

            if (fsState == FileSystem.UNMOUNTED) {
                // If FS is unmounted, do not show capacity
                setValue("usageText", new Integer(-1));
                setValue("capacityText", state);
                setValue("UsageBarImage", Constants.Image.ICON_BLANK);
            } else {
                if (consumed < 0 || consumed > 100) {
                    setValue("UsageBarImage", Constants.Image.ICON_BLANK);
                } else if (archiveEnabled && consumed >= hwm) {
                    // have hwm and usage exceeeds hwm
                    setValue("UsageBarImage",
                        new StringBuffer(
                            Constants.Image.RED_USAGE_BAR_DIR).
                            append(consumed).append(".gif").toString());
                } else {
                    // have no hwm, or have hwm but usage does not exceed hwm
                    setValue("UsageBarImage",
                        new StringBuffer(
                           Constants.Image.USAGE_BAR_DIR).
                           append(consumed).append(".gif").toString());
                }

                setValue("capacityText",
                    new StringBuffer("(").append(
                    new Capacity(cap, SamQFSSystemModel.SIZE_KB)).
                    append(")").toString());
                setValue("usageText", new Integer(consumed));
            }

            if (archiveEnabled) {
                // if fs is able to run archive, it has a hwm
                setValue("highWaterMarkText", new Integer(hwm));
            } else {
                setValue("highWaterMarkText", "");
            }
            setValue("mountPointText", FSUtil.truncateMountPointString(point));

            // calculate state that will be used by javascript on client side
            // to dynamically enable/disable GUI compoments
            StringBuffer enabledButtons = new StringBuffer();
            StringBuffer enabledMenuOptions = new StringBuffer();

            String serverAPIVersion = "1.5";
            try {
                serverAPIVersion =
                    SamUtil.getServerInfo(serverName).
                                getSamfsServerAPIVersion();
                TraceUtil.trace3("API Version: " + serverAPIVersion);
            } catch (SamFSException samEx) {
                TraceUtil.trace1("Error getting samfs version!", samEx);
            }

            // 5.0
            if (SamUtil.isVersionCurrentOrLaterThan(serverAPIVersion, "1.6")) {
                growEnabled =
                    !fsList[i].isHA() &&
                    (fsShareStatus == FileSystem.UNSHARED ||
                     fsShareStatus == FileSystem.SHARED_TYPE_MDS);
                shrinkEnabled =
                    !fsList[i].isHA() &&
                    fsState == FileSystem.MOUNTED &&
                    (fsShareStatus == FileSystem.UNSHARED ||
                     fsShareStatus == FileSystem.SHARED_TYPE_MDS);
            // 4.6
            } else {
                growEnabled = !fsList[i].isHA() &&
                    (fsState == FileSystem.UNMOUNTED) &&
                    (fsShareStatus == FileSystem.UNSHARED);
                shrinkEnabled = false;
            }

            deleteEnabled =
            (fsState == FileSystem.UNMOUNTED) && !UFS_ROOT.equals(point);

            mountEnabled = fsState == FileSystem.UNMOUNTED;
            umountEnabled = fsState == FileSystem.MOUNTED &&
                                        !UFS_ROOT.equals(point);

            // Grow button is always enabled for both types of servers, if
            // the file system is not type HA, and is unmounted, and unshared
            if (growEnabled) {
                enabledMenuOptions.append(getPosition(GROW_FS)).append(',');
            }

            if (shrinkEnabled) {
                enabledMenuOptions.append(getPosition(SHRINK_FS)).append(',');
            }

            if (systemSetup == FileSystemSummaryView.SETUP_QFS) {
                enabledButtons.append(ViewFilesButton_QFS);
            } else {
                enabledButtons.append(ViewFilesButton).append(',');
                if (archiveEnabled) {
                    enabledButtons.append(ViewPolicyButton).append(',');
                    enabledButtons.append(SamQFSWizardNewPolicyButton);
                }
            }

            // remove the trailing ',' if there's one
            int len = enabledButtons.length();
            if (len > 0 && enabledButtons.charAt(len - 1) == ',') {
                enabledButtons.deleteCharAt(len - 1);
            }

            enabledMenuOptions.append(
                getPosition(EDIT_MOUNT_OPTION)).append(',');
            if (fsckEnabled) {
                enabledMenuOptions.append(
                    getPosition(CHECK_FS)).append(',');
            }
            if (mountEnabled) {
                enabledMenuOptions.append(
                    getPosition(MOUNT)).append(',');
            }
            if (umountEnabled) {
                enabledMenuOptions.append(
                    getPosition(UNMOUNT)).append(',');
            }
            if (deleteEnabled) {
                enabledMenuOptions.append(
                    getPosition(DELETE)).append(',');
            }

            if (archiveEnabled) {
                enabledMenuOptions.append(
                    getPosition(ARCHIVE_ACTIVITIES)).append(',');
            }

            String dumpScheduleDisp = "";

            if (archiveEnabled && fsState == FileSystem.MOUNTED) {
                enabledMenuOptions.append(
                    getPosition(SCHEDULE_DUMP)).append(',');

                RecoveryPointSchedule mySchedule =
                    SamUtil.getRecoveryPointSchedule(
                        sysModel, fsList[i].getName());
                if (mySchedule != null) {
                    // show schedule string for archive fs
                    dumpScheduleDisp = SamUtil.getSchedulePeriodicString(
                        mySchedule.getPeriodicity(),
                        mySchedule.getPeriodicityUnit());

                    // if schedule disabled indicate so
                    if (mySchedule.isDisabled()) {
                        dumpScheduleDisp = (new StringBuffer(dumpScheduleDisp))
                            .append(" ")
                            .append(SamUtil.getResourceString(
                            "fs.recoverypointschedule.disabled")).toString();
                    }
                }
            }

            setValue("dumpScheduledText", dumpScheduleDisp);

            // remove the trailing ',' if there's one
            len = enabledMenuOptions.length();
            if (len > 0 && enabledMenuOptions.charAt(len - 1) == ',') {
                enabledMenuOptions.deleteCharAt(len - 1);
            }
            setValue("HiddenDynamicButtons", enabledButtons.toString());
            setValue("HiddenDynamicMenuOptions", enabledMenuOptions.toString());
            // encode fsType, fsSharedStatus and fsSharedType into href string
            // which will be used in the handler
            // for samfs/qfs, href = fsName:fsType:fsSharedStatus:fsSharedType
            // for non-sam fs, href = fsName:fsType
            setValue(FileSystemSummaryTiledView.CHILD_FS_HREF,
                new StringBuffer(name).append(':')
                    .append(fsType).append(':')
                    .append("qfs").append(':')
                    .append(archiveEnabled).append(':')
                    .append(fsShareStatus).toString());
            setValue("FSHiddenField", name);
            setValue("ProtoFSField", fsList[i].isProtoFS());

            initQueryParams();

            // populate the nfs-shared column
            String nfsShared = fsList[i].hasNFSShares() ?
                SamUtil.getResourceString("samqfsui.yes") : "";
            setValue("nfsSharedText", nfsShared);
        }

        // STOP the code to proceed further if filter is set and only QFS is
        // the only file system type the user wishes to see.
        if (filtered && filterSetting == 0) {
            TraceUtil.trace3("Exiting");
            return;
        }

        // now add non-sam fs to the model
        for (int j = 0; fileSystems != null && j < fileSystems.length; j++) {
            String name  = null,
                   point = null,
                   type  = null,
                   state = null;

            long cap  = 0,
                 free = 0;
            int consumed = 0;

            name  = fileSystems[j].getName();
            int fsState = fileSystems[j].getState();
            if (fsState == GenericFileSystem.MOUNTED) {
                state = "FSSummary.mount";
            } else if (fsState == GenericFileSystem.UNMOUNTED) {
                state = "FSSummary.unmount";
            } else {
                state = "";
            }

            int fsType = fileSystems[j].getFSTypeByProduct();
            type  = fileSystems[j].getFSTypeName();
            cap  = fileSystems[j].getCapacity();
            free = fileSystems[j].getAvailableSpace();
            consumed = fileSystems[j].getConsumedSpacePercentage();
            point = fileSystems[j].getMountPoint();

            // definition comes from FSUtil, default to UNKNOWN (-1)
            int fsDescription = FSUtil.getFileSystemDescription(fileSystems[j]);

            // filter the model if need
            if (filtered &&
                shouldThisFSBeExcluded(
                    fsDescription, fileSystems[j].hasNFSShares())) {
                continue;
            }
            latestRows.add(new Integer(i + j));

            // append new row
            if (i + j > 0) {
                appendRow();
            }

            // Show <type> instead of path name for ufs file system
            // e.g. <ufs>, <vxfs>
            setValue("fsNameText",
                SamUtil.getResourceString("FSSummary.nosam.descriptor", type));

            setValue("typeText",
                FSUtil.getFileSystemDescriptionString(fileSystems[j]));

            if (fsState == GenericFileSystem.UNMOUNTED) {
                // If FS is unmounted, do not show capacity
                setValue("usageText", new Integer(-1));
                setValue("capacityText", state);
                setValue("UsageBarImage", Constants.Image.ICON_BLANK);
            } else {
                if (consumed < 0 || consumed > 100) {
                    setValue("UsageBarImage", Constants.Image.ICON_BLANK);
                } else {
                    setValue("UsageBarImage",
                        new StringBuffer(Constants.Image.USAGE_BAR_DIR).
                            append(consumed).append(".gif").toString());
                }

                setValue("capacityText",
                    new StringBuffer("(").append(
                    new Capacity(cap, SamQFSSystemModel.SIZE_KB)).
                    append(")").toString());
                setValue("usageText", new Integer(consumed));
            }

            setValue("highWaterMarkText", "");
            setValue("mountPointText", FSUtil.truncateMountPointString(point));
            setValue("dumpScheduledText", "");

            // calculate state that will be used by javascript on client side
            // to dynamically enable/disable GUI compoments
            StringBuffer enabledButtons = new StringBuffer();
            StringBuffer enabledMenuOptions = new StringBuffer();

            // Grow & Shrink are both not available for non-SAM file systems

            deleteEnabled =
                (fsState == GenericFileSystem.UNMOUNTED) &&
                !UFS_ROOT.equals(point);

            mountEnabled =
                fsState == GenericFileSystem.UNMOUNTED &&
                fsDescription != FSUtil.FS_DESC_ZFS;
            umountEnabled =
                (fsState == GenericFileSystem.MOUNTED) &&
                fsDescription != FSUtil.FS_DESC_ZFS &&
                !UFS_ROOT.equals(point);

            // View files button is always enable for 4.6+ servers
            if (systemSetup == FileSystemSummaryView.SETUP_QFS) {
                enabledButtons.append(ViewFilesButton_QFS);
            } else {
                enabledButtons.append(ViewFilesButton).append(',');
            }

            // NOTE: "grow" not supported for non-sam fs yet!

            // remove the trailing ',' if there's one
            int len = enabledButtons.length();
            if (len > 0 && enabledButtons.charAt(len - 1) == ',') {
                enabledButtons.deleteCharAt(len - 1);
            }

            // NOTE: "edit mount options" not supported for non-sam fs yet
            // enabledMenuOptions.append(
            //   getPosition(EDIT_MOUNT_OPTION)).append(',');

            // NOTE: "fsck" not supported for non-sam fs yet
            // enabledMenuOptions.append(
            //   getPosition(CHECK_FS)).append(',');

            if (mountEnabled) {
                enabledMenuOptions.append(
                    getPosition(MOUNT)).append(',');
            }
            if (umountEnabled) {
                enabledMenuOptions.append(
                    getPosition(UNMOUNT)).append(',');
            }

            if (deleteEnabled) {
                enabledMenuOptions.append(
                    getPosition(DELETE)).append(',');
            }

            // remove the trailing ',' if there's one
            len = enabledMenuOptions.length();
            if (len > 0 && enabledMenuOptions.charAt(len - 1) == ',') {
                enabledMenuOptions.deleteCharAt(len - 1);
            }

            setValue("HiddenDynamicButtons", enabledButtons.toString());
            setValue("HiddenDynamicMenuOptions", enabledMenuOptions.toString());
            // encode fsType, fsSharedStatus and fsSharedType into href string
            // which will be used in the handler
            // for samfs/qfs, href = fsName:fsType:fsSharedStatus:fsSharedType
            // for non-sam fs, href = fsName:fsType
            setValue(FileSystemSummaryTiledView.CHILD_FS_HREF,
                new StringBuffer(name).append(':')
                    .append(fsType).append(':')
                    .append(type).append(':')
                    .append(archiveEnabled).toString());
            setValue("FSHiddenField", name);

            initQueryParams();

            // populate the nfs-shared column
            String nfsShared = fileSystems[j].hasNFSShares() ?
                SamUtil.getResourceString("samqfsui.yes") : "";
            setValue("nfsSharedText", nfsShared);
        }

        TraceUtil.trace3("Exiting");
    }

    // Set the index for Href
    private void initQueryParams() {
        TraceUtil.trace3("Entering");
        String index = new Integer(getRowIndex()).toString();
        TraceUtil.trace3("index in model " + index);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Set filter to only show rows matching given alarm severity.
     *
     * @param severity The alarm severity.
     */
    public void setFilter(String selectedType) {
        TraceUtil.trace3("Entering");
        this.filtered = true;

        if (selectedType.equals(Constants.Filter.FILTER_ALL)) {
            this.filterSetting = -1;
        } else {
            this.filterSetting = -1;

            try {
                filterSetting = Integer.parseInt(selectedType);
            } catch (NumberFormatException numEx) {
                // Should not come to this case, otherwise it's a developer bug
                this.filterSetting = -1;
            }
        }

        TraceUtil.trace3("Exiting");
    }

    // Return a ArrayList holds the latest table index
    public ArrayList getLatestIndex() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return latestRows;
    }

    /**
     * This method determines if we should exclude this file system in the
     * display.  Strictly used by the filter in FS Summary page
     */
    private boolean shouldThisFSBeExcluded(
        int fsDescription, boolean hasNFSShares) {

        switch (filterSetting) {
            // Case 0: QFS
            // False if QFS is selected in the filter list, and fsDescription
            // lays in between FS_DESC_QFS (0) and
            // FS_DESC_QFS_POTENTIAL_SERVER_ARCHIVING (7)
            // Otherwise True
            case 0:
                return !(fsDescription >= FSUtil.FS_DESC_QFS &&
                    fsDescription <=
                        FSUtil.FS_DESC_QFS_POTENTIAL_SERVER_ARCHIVING);

            // Case 1: ufs
            // False if UFS is selected in the filter list, and fsDescription
            // lays on FS_DESC_UFS
            case 1:
                return fsDescription != FSUtil.FS_DESC_UFS;

            // Case 2: vxfs
            // False if nfs-shared is selected in the filter list, but
            case 2:
                return fsDescription != FSUtil.FS_DESC_VXFS;

            // Case 3: nfs-shared
            // False if nfs-shared is selected in the filter list, but
            case 3:
                return !hasNFSShares;

            // Other cases
            // include all fs, don't skip
            default:
                return false;
        }
    }

    /**
     * Helper function to return the position of a menu item in any
     * particular system setup.  The return value is used to determine
     * if that menu item has to be enabled by javascript or not.
     */
    private int getPosition(int menuItem) {
        int [] menuItems;

        switch (systemSetup) {
            case FileSystemSummaryView.SETUP_QFS:
                menuItems = qfsOnly;
                break;
            default:
                menuItems = sam;
                break;
        }

        for (int i = 0; i < menuItems.length; i++) {
            if (menuItem == menuItems[i]) {
                return i;
            }
        }

        // Should not reach here
        return 0;
    }
}
