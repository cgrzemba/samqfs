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

// ident	$Id: LabelJob.java,v 1.9 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class LabelJob extends Job {

    private DriveDev drv;

    public LabelJob(DriveDev drv) {
        super(Job.TYPE_LABEL, Job.STATE_CURRENT,
            drv.getEquipOrdinal() * 100 + Job.TYPE_LABEL);
            this.drv = drv;
    }

    public DriveDev getDrive() { return drv; }

    public static LabelJob[] getAll(Ctx c) throws SamFSException {
        DriveDev[] drvs = getDrives(c);
        LabelJob[] jobs = new LabelJob[drvs.length];
        for (int i = 0; i < drvs.length; i++)
            jobs[i] = new LabelJob(drvs[i]);
        return jobs;
    }

    private static native DriveDev[] getDrives(Ctx c)
        throws SamFSException;
}
