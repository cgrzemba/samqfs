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

// ident	$Id: SamfsckJobImpl.java,v 1.13 2008/05/16 18:39:02 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.SamfsckJob;
import java.util.GregorianCalendar;

public class SamfsckJobImpl extends BaseJobImpl implements SamfsckJob {

    private com.sun.netstorage.samqfs.mgmt.fs.SamfsckJob jniJob = null;
    private String fsName = null;
    private String userName = null;
    private boolean repair;

    public SamfsckJobImpl(BaseJob base, String fsName, String userName) {
        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        if (fsName != null)
            this.fsName = fsName;

        if (userName != null)
            this.userName = userName;
    }

    public SamfsckJobImpl(com.sun.netstorage.samqfs.mgmt.fs.SamfsckJob jniJob) {

        super(jniJob);
        this.jniJob = jniJob;

        if (jniJob != null) {
            if (SamQFSUtil.isValidString(jniJob.getUser()))
                userName = jniJob.getUser();
            repair = jniJob.getRepair();
            if (SamQFSUtil.isValidString(jniJob.getFsName()))
                fsName = jniJob.getFsName();
        }
    }

    public String getFileSystemName() {
        return fsName;
    }

    public String getInitiatingUser() {
        return userName;
    }

    public boolean isRepair() {
        return repair;
    }

    // override
    public GregorianCalendar getStartDateTime() {
        return SamQFSUtil.convertTime(jniJob.getCreationTime());
    }
}
