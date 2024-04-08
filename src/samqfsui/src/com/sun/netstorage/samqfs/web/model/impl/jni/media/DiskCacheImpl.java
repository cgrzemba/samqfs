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

// ident	$Id: DiskCacheImpl.java,v 1.26 2009/03/04 21:54:42 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;


import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.SCSIDevInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;


public class DiskCacheImpl extends BaseDeviceImpl implements DiskCache {


    private DiskDev diskDev = null;
    private int diskType = -1;
    private int diskCacheType = DiskCache.NA;
    private long capacity = -1;
    private long availableSpace = -1;
    private int consumedSpace = -1;
    private int noOfInodesRemaining = -1;
    private String vendor = null;
    private String productId = null;
    private boolean alloc = false;

    // Only used for display purpose
    private String devicePathDisplayString = null;


    public DiskCacheImpl() {
    }

    /**
     * This constructor is built to convert a striped group into a diskCache
     * object for the Shrink File System Wizard.
     */
    public DiskCacheImpl(StripedGroup group) {
        if (group == null) {
            return;
        }
        super.setDevicePath(group.getName());
        capacity = group.getCapacity();
        availableSpace = group.getAvailableSpace();
        consumedSpace = group.getConsumedSpacePercentage();
        diskCacheType = DiskCache.STRIPED_GROUP;

        DiskCache[] members = group.getMembers();
        StringBuffer buf = new StringBuffer(group.getName());
        StringBuffer tab = new StringBuffer("&nbsp;&nbsp;&nbsp;&nbsp;");
        for (int i = 0; i < members.length; i++) {
            buf.append("<br>").append(tab).
                append(members[i].getDevicePathDisplayString());
        }
        devicePathDisplayString = buf.toString();

        if (members.length > 0) {
            vendor = members[0].getVendor();
            productId = members[0].getProductId();

            // Use first device's EQ when shrinking a striped group
            super.setEquipOrdinal(members[0].getEquipOrdinal());

            // Use the state of the first device of a striped group as the
            // state of the group itself
	    super.setState(members[0].getState());
        }
    }


    public DiskCacheImpl(DiskDev disk) {

        if (disk != null) {

            super.setDevicePath(disk.getDevicePath());
            super.setEquipOrdinal(disk.getEquipOrdinal());
            super.setEquipType(SamQFSUtil.getEquipTypeInteger(
                                                      disk.getEquipType()));
            super.setFamilySetName(disk.getFamilySetName());
            super.setFamilySetEquipOrdinal(disk.getFamilySetEquipOrdinal());
            super.setState(SamQFSUtil.convertStateToUI(disk.getState()));
            super.setAdditionalParamFilePath(
                    disk.getAdditionalParamFilePath());

            this.diskDev = disk;
            diskType = getDiskType(disk.getAU().getType(),
                                   disk.getAU().getRAID());
            diskCacheType = SamQFSUtil.
                getEquipTypeInteger(disk.getEquipType());
            if (diskCacheType == -1)
                diskCacheType = DiskCache.NA;

            SCSIDevInfo dev = disk.getAU().getSCSIDevInfo();
            if (dev != null) {
                if (dev.vendor != null) {
                    vendor = dev.vendor;
                }
                if (dev.prodID != null) {
                    productId = dev.prodID;
                }
            }

            capacity = disk.getAU().getSize();
            capacity = SamQFSUtil.convertSize(
                            capacity, SamQFSSystemModel.SIZE_KB,
                            SamQFSSystemModel.SIZE_MB);

            availableSpace = disk.getFreeSpace();
            availableSpace = SamQFSUtil.convertSize(
                                availableSpace, SamQFSSystemModel.SIZE_KB,
                                SamQFSSystemModel.SIZE_MB);

            if (capacity != 0) {
                consumedSpace = (int) ((capacity-availableSpace)*100/capacity);
            } else {
                consumedSpace = 0;
            }

            alloc = super.getState() == BaseDevice.ON;
        }
    }


    // methods

    public DiskDev getJniDisk() { return diskDev; }
    public int getDiskType() { return diskType; }


    public int getDiskCacheType() { return diskCacheType; }
    public void setDiskCacheType(int diskCacheType) {
        this.diskCacheType = diskCacheType;
    }


    /**
     * @since 4.5
     * @returns true if this dev is highly available
     */
    public static boolean isHADevice(String devPath) {
        // global or did device, or volume inside a diskset/diskgroup
        return (devPath.startsWith("/dev/global/") ||
                devPath.startsWith("/dev/did/") ||
                devPath.matches("/dev/((md)|(vx))/[^/]+/r?dsk/[^\\s]+"));
    }

    public boolean isHA() {
        return isHADevice(this.getDevicePath());
    }

    public long getCapacity() {
	return capacity;
    }

    public long getAvailableSpace() {
	return availableSpace;
    }

    public int getConsumedSpacePercentage() {
	return consumedSpace;
    }


    public int getNoOfInodesRemaining() {

        noOfInodesRemaining = -1;

        if (getDiskCacheType() == DiskCache.METADATA) {
            if (getJniDisk() != null) {
                noOfInodesRemaining = (int) getJniDisk().getFreeInodes();
            }
        }

	return noOfInodesRemaining;

    }


    public String toString() {

        StringBuffer buf = new StringBuffer();

        buf.append(super.toString());

        buf.append("Disk Type: " + diskType + "\n");
        buf.append("Disk Cache Type: " + diskCacheType + "\n");
        buf.append("Capacity: " + capacity + "\n");
        buf.append("Vendor: " + vendor + "\n");
        buf.append("Product ID: " + productId + "\n");
        buf.append("Available Space: " + availableSpace + "\n");
        buf.append("Consumed Space: " + consumedSpace + "% \n");
        buf.append("No Of Inodes Remaining: " + noOfInodesRemaining + "\n");

        return buf.toString();

    }


    private int getDiskType(int jniType, String raid) {

        int type = DiskCache.SLICE;
        int raidVal = 0;
        if (SamQFSUtil.isValidString(raid)) {
            if (raid.charAt(0) == '1') {
                raidVal = 1;
            } else if (raid.charAt(0) == '5') {
                raidVal = 5;
            }
        }

        switch (jniType) {
            case AU.SVM_LOGICAL_VOLUME:
                if (raidVal == 0) {
                    type = DiskCache.SVM_LOGICAL_VOLUME;
                } else if (raidVal == 1) {
                    type = DiskCache.SVM_LOGICAL_VOLUME_MIRROR;
                } else if (raidVal == 5) {
                    type = DiskCache.SVM_LOGICAL_VOLUME_RAID_5;
                }
                break;
            case AU.VXVM_LOGICAL_VOLUME:
                if (raidVal == 0) {
                    type = DiskCache.VXVM_LOGICAL_VOLUME;
                } else if (raidVal == 1) {
                    type = DiskCache.VXVM_LOGICAL_VOLUME_MIRROR;
                } else if (raidVal == 5) {
                    type = DiskCache.VXVM_LOGICAL_VOLUME_RAID_5;
                }
                break;
            case AU.ZFS_ZVOL:
                type = DiskCache.ZFS_ZVOL;
                break;

            case AU.OSD:
                type = DiskCache.OSD;
                break;

        }

        return type;

    }


    public String getVendor() {
        return vendor;
    }


    public String getProductId() {
        return productId;
    }

    public boolean isAlloc() {
        return alloc;
    }

    public int getEquipOrdinal() {
        return super.getEquipOrdinal();
    }

    public String getDevicePath() {
        return super.getDevicePath();
    }

    public String getDevicePathDisplayString() {
        // striped group string has already been built
        if (devicePathDisplayString != null) {
            return devicePathDisplayString;
        }
        String pathString = getDevicePath();
        String [] sliceElement = pathString.split("/");
        int type = getDiskType();

        // if the slice is a SVM volume, we need to show the disk group
        if ((type == DiskCache.SVM_LOGICAL_VOLUME ||
            type == DiskCache.SVM_LOGICAL_VOLUME_MIRROR ||
            type == DiskCache.SVM_LOGICAL_VOLUME_RAID_5) &&
            sliceElement.length == 6) {
            devicePathDisplayString = sliceElement[3] + "/" + sliceElement[5];
        } else if (type == DiskCache.OSD) {
            int index = pathString.indexOf("/dsk/osd/");
            devicePathDisplayString = pathString.substring(index + 9);
        } else {
            int index = pathString.indexOf("/dsk/");
            devicePathDisplayString = pathString.substring(index + 5);
        }
        return devicePathDisplayString;
    }

    public boolean isDataOnlyDevice() {
        return
	    diskCacheType == DiskCache.MR ||
	    diskCacheType == DiskCache.MD ||
            diskCacheType == DiskCache.STRIPED_GROUP;
    }
}
