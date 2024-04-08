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

// ident	$Id: NotifSummary.java,v 1.16 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class NotifSummary {

    private String adminName;
    private String emailAddr;
    private boolean[] subj;

    public NotifSummary(String adminName, String emailAddr, boolean[] subj) {
        this.adminName = adminName;
        this.emailAddr = emailAddr;
        this.subj = subj;
    }

    // notification types. must match the notf_subj_t enum in notif_summary.h
    // use it as an index in subj[]
    public static final int NOTIF_SUBJ_DEVICEDOWN  = 0; // dev dn: Y/N?
    public static final int NOTIF_SUBJ_ARCHINTR = 1; // arch interrupted?
    public static final int NOTIF_SUBJ_MEDREQD  = 2; // media required
    public static final int NOTIF_SUBJ_RECYSTATRC = 3; // recycler status
    public static final int NOTIF_SUBJ_DUMPINTR   = 4; // dump interrupted
    public static final int NOTIF_SUBJ_FSNOSPACE  = 5; // file system full
    public static final int NOTIF_SUBJ_HWMEXCEED  = 6; // HWM exceeded?
    public static final int NOTIF_SUBJ_ACSLSERR   = 7; // acsls err
    public static final int NOTIF_SUBJ_ACSLSWARN  = 8; // acsls warning
    public static final int NOTIF_SUBJ_DUMPWARN   = 9; // dump warning

    // long outstanding wait for tape
    public static final int NOTIF_SUBJ_LONGWAITTAPE   = 10;

    // fs inconsistent, only Intellistore setup
    public static final int NOTIF_SUBJ_FSINCONSISTENT = 11;
    // system health, only Intellistore setup
    public static final int NOTIF_SUBJ_SYSTEMHEALTH   = 12;


    public String getAdminName() { return adminName; }
    public void setAdminName(String name) { this.adminName = name; }

    public String getEmailAddr() { return emailAddr; }
    public void setEmailAddr(String email) { this.emailAddr = email; }

    public boolean[] getSubj() { return subj; }
    public void setSubj(boolean subj[]) { this.subj = subj; }

    public String toString() {
        String s = adminName + "," + emailAddr + ",";
        for (int i = 0; i < subj.length; i++)
            s += subj[i] ? "T" : "F";
        return s;
    }

    public static native NotifSummary[] get(Ctx c)
        throws SamFSException;
    public static native void delete(Ctx c, NotifSummary sum)
        throws SamFSException;
    public static native void modify(Ctx c, String oldEmailAddr,
        NotifSummary sum) throws SamFSException;
    public static native void add(Ctx c, NotifSummary sum)
        throws SamFSException;
    public static native String getEmailAddrsForSubject(Ctx c, int subject)
        throws SamFSException;

}
