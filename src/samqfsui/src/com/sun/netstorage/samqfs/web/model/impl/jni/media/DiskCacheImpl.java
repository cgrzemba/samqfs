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

// ident	$Id: DiskCacheImpl.java,v 1.18 2008/04/23 19:58:40 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;


import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.fs.SCSIDevInfo;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;


public class DiskCacheImpl extends BaseDeviceImpl implements DiskCache {


    private DiskDev diskDev = null;
    private int diskType = -1;
    private int diskCacheType = DiskCache.NA;
    private long capacity = -1;
    private long availableSpace = -1;
    private int consumedSpace = -1;
    private int noOfInodesRemaining = -1;
    private String vendor = new String();
    private String productId = new String();
    private boolean alloc = false;


    public DiskCacheImpl() {
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
            availableSpace = disk.getFreeSpace();
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
	return SamQFSUtil.convertSize(capacity, SamQFSSystemModel.SIZE_KB,
                                      SamQFSSystemModel.SIZE_MB);
    }

    public long getAvailableSpace() {
	return SamQFSUtil.convertSize(availableSpace,
                                      SamQFSSystemModel.SIZE_KB,
                                      SamQFSSystemModel.SIZE_MB);
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
}
