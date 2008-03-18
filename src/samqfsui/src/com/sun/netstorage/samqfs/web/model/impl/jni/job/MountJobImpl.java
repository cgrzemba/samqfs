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

// ident	$Id: MountJobImpl.java,v 1.11 2008/03/17 14:43:49 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import java.util.GregorianCalendar;

public class MountJobImpl extends BaseJobImpl implements MountJob {
    private VSN vsn = null;
    private String vsnName = null;
    private int mediaType = -1;
    private String libName = null;
    private boolean archiveMount = true;
    private long processId = -1;
    private String username = null;
    private GregorianCalendar time = null;
    private int statusFlag = 0;

    public MountJobImpl() {
    }

    public MountJobImpl(BaseJob base,
                        VSN vsn,
                        boolean archiveMount,
                        long processId,
                        String username,
                        GregorianCalendar time) throws SamFSException {

        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        this.vsn = vsn;
        this.archiveMount = archiveMount;
        this.processId = processId;
        this.username = username;
        this.time = time;
        this.statusFlag = 0;
    }

    public MountJobImpl(SamQFSSystemModelImpl model,
                        com.sun.netstorage.samqfs.mgmt.media.MountJob jniJob) {
        super(jniJob);
        if (jniJob != null) {

            // API does not support the commented out stuff
            vsnName = jniJob.vsn;
            mediaType = SamQFSUtil.getMediaTypeInteger(jniJob.mediaType);

            if ((jniJob.robotEq > -1) && (model != null)) {
                try {
                    libName = model.getSamQFSSystemMediaManager().
                                  getLibraryByEQ(jniJob.robotEq).getName();
                } catch (Exception e) {}
            }

            if ((jniJob.flags &
                 com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_STAGE) ==
                com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_STAGE)
                archiveMount = false;

            statusFlag = jniJob.flags;
            processId = jniJob.pid;
            username = jniJob.userName;
        }
    }

    public VSN getVSN() {
        return vsn;
    }

    public String getVSNName() throws SamFSException {
        try {
            if (vsn != null)
                vsnName = vsn.getVSN();
        } catch (Exception e) {}

        return vsnName;
    }

    public int getMediaType() throws SamFSException {
        try {
            if (vsn != null)
                mediaType = vsn.getLibrary().getMediaType();
        } catch (Exception e) {}

        return mediaType;
    }

    public String getLibraryName() throws SamFSException {
        Library lib = null;

        if (vsn != null)
            lib = vsn.getLibrary();

        if (lib != null)
            libName = lib.getName();

        return libName;
    }

    public boolean isArchiveMount() throws SamFSException {
        return archiveMount;
    }

    public void setIsArchiveMount(boolean mount) throws SamFSException {
        this.archiveMount = mount;
    }

    public long getProcessId() throws SamFSException {
        return processId;
    }

    public String getInitiatingUsername() throws SamFSException {
        return username;
    }

    public GregorianCalendar getTimeInQueue() throws SamFSException {
        return time;
    }

    public void setTimeInQueue(GregorianCalendar time) throws SamFSException {
        this.time = time;
    }

    public int getStatusFlag() {
        return this.statusFlag;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append(super.toString());
        if (vsn != null)
            try {
                buf.append("VSN: ").append(vsn.getVSN()).append("\n");
            } catch (Exception e) {
            }

        buf.append("Archive Mount: ")
            .append(archiveMount)
            .append("\n")
            .append("Process Id: ")
            .append(processId)
            .append("\n")
            .append("Initiating Username: ")
            .append(username)
            .append("\n")
            .append("Time In Queue: ")
            .append(SamQFSUtil.dateTime(time))
            .append("\n");

        return buf.toString();
    }
}
