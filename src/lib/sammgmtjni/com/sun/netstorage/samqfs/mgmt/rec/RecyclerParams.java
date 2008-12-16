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

// ident	$Id: RecyclerParams.java,v 1.12 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.rec;

public class RecyclerParams {

    // these values must match those defined in pub/mgmt/recycle.h */

    private static final long RC_hwm  = 0x00000001;
    private static final long RC_data_quantity  = 0x00000002;
    private static final long RC_ignore = 0x00000004;
    private static final long RC_email_addr = 0x00000008;
    private static final long RC_mingain = 0x00000010;
    private static final long RC_vsncount = 0x00000020;
    private static final long RC_mail = 0x00000040;
    private static final long RC_minobs = 0x00000080;
    // private fields

    private int hwm;
    private String datasize; // bytes
    private boolean ignore;
    private String email;
    private int minGain;
    private int vsnCount;
    private long chgFlags;
    private int minObs; // only valid in archiver.cmd for disk vols

    /**
     * private constructor
     */
    protected RecyclerParams(int hwm, String datasize, boolean ignore,
        String email, int minGain, int vsnCount, int minObs, long chgFlags) {
            this.hwm = hwm;
            this.datasize = datasize;
            this.ignore = ignore;
            this.email = email;
            this.minGain = minGain;
            this.vsnCount = vsnCount;
            this.minObs = minObs;
            this.chgFlags = chgFlags;
    }

    /**
     * public constructor
     */
    public RecyclerParams() {
        chgFlags = 0;
    }


    // public methods

    public int getHWM() { return hwm; }
    public void setHWM(int hwm) {
        this.hwm = hwm;
        chgFlags |= RC_hwm;
    }
    public void resetHWM() {
        chgFlags &= ~RC_hwm;
    }

    public String getDatasize() { return datasize; }
    public void setDatasize(String datasize) {
        this.datasize = datasize;
        chgFlags |= RC_data_quantity;
    }
    public void resetDatasize() {
        chgFlags &= ~RC_data_quantity;
    }

    public boolean getIgnore() { return ignore; }
    public void setIgnore(boolean ignore) {
        this.ignore = ignore;
        chgFlags |= RC_ignore;
    }
    public void resetIgnore() {
        chgFlags &= ~RC_ignore;
    }

    public String getEmailAddr() { return email; }
    public void setEmailAddr(String email) {
        this.email = email;
        chgFlags |= RC_email_addr;
    }
    public void resetEmailAddr() {
        chgFlags &= ~RC_email_addr;
    }

    public int getMinGain() { return minGain; }
    public void setMinGain(int minGain) {
        this.minGain = minGain;
        chgFlags |= RC_mingain;
    }
    public void resetMinGain() {
        chgFlags &= ~RC_mingain;
    }

    public int getVSNCount() { return vsnCount; }
    public void setVSNCount(int vsnCount) {
        this.vsnCount = vsnCount;
        chgFlags |= RC_vsncount;
    }
    public void resetVSNCount() {
        chgFlags &= ~RC_vsncount;
    }

    public int getMinObs() { return minObs; }
    public void setMinObs(int minObs) {
        this.minObs = minObs;
        chgFlags |= RC_minobs;
    }
    public void resetMinObs() {
        chgFlags &= ~RC_minobs;
    }

    public String toString() {
        String s = "hwm=" + hwm + ",datasz=" + datasize + ",ignore=" +
        (ignore ? "T" : "F") + ",email=" + email + ",minGain=" + minGain +
        ",vsnCnt=" + vsnCount + ",minObs=" + minObs;
        return s;
    }
}
