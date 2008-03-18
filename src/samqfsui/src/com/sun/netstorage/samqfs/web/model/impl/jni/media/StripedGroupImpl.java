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

// ident	$Id: StripedGroupImpl.java,v 1.10 2008/03/17 14:43:50 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.StripedGrp;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;


public class StripedGroupImpl extends BaseDeviceImpl implements StripedGroup {

    private StripedGrp stripedGrp = null;
    private String name = new String();
    private DiskCache[] members = new DiskCache[0];
    private long capacity = -1;
    private long availableSpace = -1;
    private int consumedSpace = -1;


    public StripedGroupImpl() {
    }


    public StripedGroupImpl(String name, DiskCache[] members)
        throws SamFSException {

        this.name = name;
        this.members = members;

        compute();

        DiskDev[] disks = new DiskDev[members.length];
        for (int i = 0; i < members.length; i++) {
            disks[i] = ((DiskCacheImpl) members[i]).getJniDisk();
        }
        this.stripedGrp = new StripedGrp(name, disks);

    }


    public StripedGroupImpl(StripedGrp grp) {

        this.stripedGrp = grp;
        this.name = grp.getName();
        DiskDev[] list = grp.getMembers();
        if (list != null) {
            members = new DiskCache[list.length];
            for (int i = 0; i < list.length; i++)
                members[i] = new DiskCacheImpl(list[i]);
        }

        compute();

    }


    public StripedGrp getJniStripedGroup() {

        return stripedGrp;

    }


    // for striped group, it would return "gXXX".
    public String getName() {

	return name;

    }


    public int getDiskCacheType() {

        int type = -1;
        if (members != null)
            type = members[0].getDiskCacheType();

        return type;

    }


    public DiskCache[] getMembers() {

	return members;

    }

    // TBD: we need to decide the unit of capacity returned

    public long getCapacity() {

	return capacity;

    }


    public long getAvailableSpace() {

	return availableSpace;

    }


    public int getConsumedSpacePercentage() {

	return consumedSpace;

    }


    public String toString() {

        StringBuffer buf = new StringBuffer();

        buf.append("Name: " + name + "\n");

        buf.append("Disk Cache Information: " + name + "\n");
	if (members != null) {
            try {
		for (int i = 0; i < members.length; i++)
		    buf.append(members[i].toString()  + "\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
	}
        buf.append("---- \n");

        buf.append("Capacity: " + capacity + "\n");
        buf.append("Available Space: " + availableSpace + "\n");
        buf.append("Consumed Space: " + consumedSpace + "% \n");

        return buf.toString();

    }


    private void compute() {

        this.capacity = 0;
        this.availableSpace = 0;

        for (int i = 0; i < members.length; i++) {
            this.capacity += members[i].getCapacity();
            this.availableSpace += members[i].getAvailableSpace();
        }

	this.consumedSpace = 0;
        if (capacity > 0) {
            this.consumedSpace = (int)
                ((capacity-availableSpace)*100/capacity);
        }

    }
}
