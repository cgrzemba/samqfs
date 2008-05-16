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

// ident	$Id: SamfsckJob.java,v 1.12 2008/05/16 18:35:28 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Job;


public class SamfsckJob extends Job {

    private String fsName;
    private char pstate;    // process state, as reported by ps
    private String user;    // who started this process
    private long time; // process start time, as reported by ps
    private boolean repair; // true if repair option was specified

    private SamfsckJob(String fsName, char pstate, String user, long pid,
        long time, boolean repair) {
        super(Job.TYPE_SAMFSCK, Job.STATE_CURRENT,
            pid * 100 + Job.TYPE_SAMFSCK, pid);
        this.fsName = fsName;
        this.pstate = pstate;
        this.user = user;
        this.time = time;
        this.repair = repair;
    }

    public String getFsName() { return fsName; }
    public char getProcState() { return pstate; }
    public String getUser() { return user; }
    /**
     * @deprecated  use getCreationTime() instead
     */
    public String time() { return String.valueOf(time); }
    public long getCreationTime() { return time; }
    public boolean getRepair() { return repair; }

    public static native SamfsckJob[] getAll(Ctx c)
        throws SamFSException;

    public String toString() {
        String s = super.toString() + " " + fsName + "," + state + "," + user +
            "," + time + ",rep:" + (repair ? "T" : "F");
        return s;
    }
}
