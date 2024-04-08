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

// ident	$Id: LastJob.java,v 1.9 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class LastJob extends Job {

    private long time; /* last time this job ran */
    private String info;

    /**
     *  private constructor
     */
    protected LastJob(int id, short type, long time, String info) {
        super(type, Job.STATE_HISTORY, id);
        this.time = time;
        this.info = info;
    }

    public long getTime() { return time; }

    /**
     * return additional info (depends on the job type).
     * currently, can be either filename or filesystem name
     */
    public String getInfo() { return info; }

    public static native LastJob getForFS(Ctx c, String fsName, short jobType)
        throws SamFSException;
    public static native LastJob[] getForAllFS(Ctx c, short jobType)
        throws SamFSException;
}
