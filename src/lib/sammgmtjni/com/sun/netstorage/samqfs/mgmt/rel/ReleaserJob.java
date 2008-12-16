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

// ident	$Id: ReleaserJob.java,v 1.10 2008/12/16 00:08:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.rel;

import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class ReleaserJob extends Job {

    private String fsName;
    private short lwm;
    private short dskPercentageUsed;

    /**
     * private constructor
     */
    private ReleaserJob(String fsName, short lwm, short dskPercentageUsed) {
        super(Job.TYPE_RELEASE, Job.STATE_CURRENT,
            Math.abs((long)fsName.hashCode()) * 100 + Job.TYPE_RELEASE);
        this.fsName = fsName;
        this.lwm = lwm;
        this.dskPercentageUsed = dskPercentageUsed;
    }

    // public methods

    public String getFSName() { return fsName; }
    public short getLWM() { return lwm; }
    public short getDskPercentageUsed() { return dskPercentageUsed; }

    public static native ReleaserJob[] getAll(Ctx c) throws SamFSException;

    public String toString() {
        String s = super.toString() + " " + fsName + "," + lwm + "," +
            dskPercentageUsed + "%";
        return s;
    }
}
