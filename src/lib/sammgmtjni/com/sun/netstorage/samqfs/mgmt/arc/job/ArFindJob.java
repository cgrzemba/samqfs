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

// ident	$Id: ArFindJob.java,v 1.9 2008/03/17 14:43:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc.job;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Job;

public class ArFindJob extends Job {

    private String fsName;
    private ArFindFsStats arFindStats;
    private ArFindFsStats arFindStatsScan;

    private ArFindJob(String fsName, long pid,
        ArFindFsStats arFindStats, ArFindFsStats arFindStatsScan) {
        super(Job.TYPE_ARFIND,
            (pid == 0) ? Job.STATE_PENDING : Job.STATE_CURRENT,
            ((pid != 0) ? pid : Math.abs(fsName.hashCode())) * 100 +
            Job.TYPE_ARFIND, pid);
        this.fsName = fsName;
        this.arFindStats = arFindStats;
        this.arFindStatsScan = arFindStatsScan;
    }

    public String getFSName() { return fsName; }
    public ArFindFsStats getStats() { return arFindStats; }
    public ArFindFsStats getStatsScan() { return arFindStatsScan; }

    public static native ArFindJob[] getAll(Ctx c) throws SamFSException;

    public String toString() {
        String s = super.toString() + " " +
            fsName + ":\n " + arFindStats + "\n " + arFindStatsScan;
        return s;
    }
}
