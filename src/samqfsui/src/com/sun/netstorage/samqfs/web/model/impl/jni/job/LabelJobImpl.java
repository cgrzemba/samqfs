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

// ident	$Id: LabelJobImpl.java,v 1.11 2008/12/16 00:12:21 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.DriveDev;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.LabelJob;

public class LabelJobImpl extends BaseJobImpl implements LabelJob {

    private com.sun.netstorage.samqfs.mgmt.media.LabelJob jniJob = null;
    private String vsnName = null;
    private String drvName = null;
    private String libName = null;
    private int complete = -1;

    public LabelJobImpl(BaseJob base,
                        String vsnName,
                        String drvName,
                        String libName,
                        int complete) {

        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        if (vsnName != null)
            this.vsnName = vsnName;

        if (drvName != null)
            this.drvName = drvName;

        if (libName != null)
            this.libName = libName;

        this.complete = complete;
    }

    public LabelJobImpl(com.sun.netstorage.samqfs.mgmt.media.LabelJob jniJob) {
        super(jniJob);
        this.jniJob = jniJob;
        update();
    }

    public String getVSNName() throws SamFSException {
        return vsnName;
    }

    public String getDriveName() throws SamFSException {
        return drvName;
    }

    public String getLibraryName() throws SamFSException {
        return libName;
    }

    public int percentageComplete() throws SamFSException {
        return complete;
    }

    private void update() {
        if (jniJob != null) {
            DriveDev drv = jniJob.getDrive();
            if (drv != null) {
                vsnName = drv.getLoadedVSN();
                drvName = drv.getDevicePath();
                libName = drv.getFamilySetName();
            }
        }
    }
}
