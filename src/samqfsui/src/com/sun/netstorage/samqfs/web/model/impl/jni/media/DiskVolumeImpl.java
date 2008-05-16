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

// ident $Id: DiskVolumeImpl.java,v 1.8 2008/05/16 18:39:02 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 *  information about a disk volume (VSN)
 */
public class DiskVolumeImpl implements DiskVolume {

    DiskVol vol; // native disk volume

    public DiskVolumeImpl(DiskVol vol) {
        this.vol = vol;
        if (vol == null)
            TraceUtil.trace1("Internal error: null DiskVol");
    }

    public String getName() { return vol.getVolName(); }

    /**
     * return null if local disk VSN
     * return host:port if honey comb target
     */
    public String getRemoteHost() {

        return vol.getHost();
    }

    /**
     *  return absolute disk path
     */
    public String getPath() { return vol.getPath(); }

    /**
     * return capacity in kbytes. -1 =  info could not be obtained
     */
    public long getCapacityKB() {
        long cap = vol.getCapacity();
        if (cap == 0) // 0 indicates a problem
            cap = -1;
        return cap;
    }

    /**
     * return available space in kbytes. -1 = info could not be obtained
     */
    public long getAvailableSpaceKB() {
        long avail = vol.getAvailableSpace();
        if (0 == vol.getCapacity()) // 0 capacity indicates a problem
            avail = -1;
        return avail;
    }

    // honey comb disk volume
    public boolean isHoneyCombVSN() {
        return vol.isHoneyComb();
    }

    /*
     * disk VSN media flags information
     */

    public boolean isLabeled() { return vol.isLabeled(); }
    public void setLabeled(boolean labeled) {
        vol.setLabeled(labeled);
    }

    public boolean isReadOnly() { return vol.isReadOnly(); }
    public void setReadOnly(boolean readOnly) {
        vol.setReadOnly(readOnly);
    }

    public boolean isBadMedia() { return vol.isBadMedia(); }
    public void setBadMedia(boolean badMedia) {
        vol.setBadMedia(badMedia);
    }

    public boolean isUnavailable() { return vol.isUnavailable(); }
    public void setUnavailable(boolean unavailable) {
        vol.setUnavailable(unavailable);
    }

    public boolean isRemote() { return vol.isRemote(); }

    public boolean isUnknown() { return vol.isUnknown(); }


    public void updateFlags(Ctx ctx) throws SamFSException {
        vol.setStatusFlags(ctx);
    }
}
