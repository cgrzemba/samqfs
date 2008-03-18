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

// ident	$Id: BaseDev.java,v 1.9 2008/03/17 14:43:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt;

public class BaseDev {

    /* private data members */
    protected String devPath;
    protected int eq;
    protected String eqType;
    protected String fsetName;
    protected int eqFSet;
    protected int state; /* see below a list of valid states */
    protected String paramFilePath;

    // valid device states. must match the dstate_t enum defined in devstat.h
    public static final int DEV_ON = 0; /* Normal operations */
    public static final int DEV_RO = 1; /* Read only operations */
    public static final int DEV_IDLE = 2; /* No new opens allowed */
    public static final int DEV_UNAVAIL = 3; /* Unavailable for file system */
    public static final int DEV_OFF  = 4; /* Off to this machine */
    public static final int DEV_DOWN = 5; /* Maintenance use only */

    /* private constructors */
    protected BaseDev() {; }
    protected BaseDev(String devPath, int eq, String eqType,
        String fsetName, int eqFSet, int state, String paramPath) {
            this.devPath = devPath;
            this.eq = eq;
            this.eqType = eqType;
            this.fsetName = fsetName;
            this.eqFSet = eqFSet;
            this.state = state;
            this.paramFilePath = paramPath;
    }

    /* public instance methods */
    public String getDevicePath() { return devPath; }

    public int getEquipOrdinal() { return eq; }
    public void setEquipOrdinal(int equipOrdinal) { eq = equipOrdinal; }

    public String getEquipType() { return eqType; }
    public void setEquipType(String eqType) { this.eqType = eqType; }

    public String getFamilySetName() { return fsetName; }
    public void setFamilySetName(String fsetName) { this.fsetName = fsetName; }

    public int getFamilySetEquipOrdinal() { return eqFSet; }
    public void setFamilySetEquipOrdinal(int eqFSet) { this.eqFSet = eqFSet; }

    public int getState() { return state; }
    public void setState(int state) { this.state = state; }

    public String getAdditionalParamFilePath() { return paramFilePath; }
    public void setAdditionalParamFilePath(String paramFilePath) {
            this.paramFilePath = paramFilePath;
    }

    public String toString() {
        return (devPath + " " + eq + " " + eqType + " " + eqFSet + " " +
            state + " " + paramFilePath);
    }
}
