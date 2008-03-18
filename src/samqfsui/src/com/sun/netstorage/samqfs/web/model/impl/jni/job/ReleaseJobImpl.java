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

// ident	$Id: ReleaseJobImpl.java,v 1.9 2008/03/17 14:43:50 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.rel.*;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.ReleaseJob;

public class ReleaseJobImpl extends BaseJobImpl implements ReleaseJob {

    private ReleaserJob jniJob = null;
    private FileSystem fs = null;
    private String fsName = null;
    private String lwm = null;
    private String consumed = null;

    public ReleaseJobImpl() {
    }

    public ReleaseJobImpl(BaseJob base, FileSystem fs) throws SamFSException {
        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        this.fs = fs;
    }

    public ReleaseJobImpl(ReleaserJob jniJob) {
        super(jniJob);
        this.jniJob = jniJob;
        update();
    }

    public FileSystem getFileSystem() {
        return fs;
    }

    public void setFileSystem(FileSystem fs) {
        this.fs = fs;
    }

    public String getFileSystemName() throws SamFSException {
        if (fs != null)
            fsName = fs.getName();

        return fsName;
    }

    public String getLWM() throws SamFSException {
        if (fs != null)
            lwm = (new Integer(fs.getMountProperties().getLWM())).toString();

        return lwm;
    }

    public String getConsumedSpacePercentage() throws SamFSException {
        if (fs != null)
            consumed =
                (new Integer(fs.getConsumedSpacePercentage())).toString();

        return consumed;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append(super.toString());
        try {
            if (fs != null) {
                buf.append("Filesystem: ")
                    .append(fs.getName())
                    .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return buf.toString();
    }

    private void update() {
        if (jniJob != null) {
            fsName = jniJob.getFSName();
            lwm = Short.toString(jniJob.getLWM());
            consumed = Short.toString(jniJob.getDskPercentageUsed());
        }
    }
}
