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

// ident	$Id: StagerJob.java,v 1.13 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.stg.job;

import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class StagerJob extends Job {

    private StagerStream ss;
    private StgFileInfo crtFileInfo;

    private StagerJob(StagerStream stgStream) {
        super(Job.TYPE_STAGE,
            stgStream.active ? Job.STATE_CURRENT : Job.STATE_PENDING,
            stgStream.seqNum * 100 + Job.TYPE_STAGE);
        this.ss = stgStream;
    }

    public String getVSN() { return ss.vsn; }
    public String getMediaType() { return ss.mediaType; }
    public long getCreationTime() { return ss.creationTime; }
    public StgFileInfo getCrtFileInfo() { return ss.crtFileInfo; }
    public int getStateFlag() { return ss.stateFlags; }

    // stateFlags value, 0 indicates initial value
    public static final int STAGER_STATE_COPY = 1;    /* copying file */
    public static final int STAGER_STATE_LOADING = 2;    /* loading vsn */
    public static final int STAGER_STATE_DONE = 3;    /* unloading vsn */

    /* positioning removable media */
    public static final int STAGER_STATE_POSITIONING = 4;
    /* waiting for media to be loaded or imported */
    public static final int STAGER_STATE_WAIT = 5;
    /* no resources available */
    public static final int STAGER_STATE_NORESOURCES = 6;

    // sort method; must match st_sort_key_t in pub/mgmt/stage.h
    public static final int ST_SORT_BY_FILENAME = 0;
    public static final int ST_SORT_BY_UID = 1;
    public static final int ST_NO_SORT = 2;

    public long getNumberOfFiles(Ctx c) throws SamFSException {
        return StagerJob.getNumOfFiles(c, this.ss);
    }
    private static native long getNumOfFiles(Ctx c, StagerStream stgStream)
        throws SamFSException;

    public StgFileInfo[] getFilesInfo(Ctx c, int start, int size, short sortMet,
        boolean ascending) throws SamFSException {
        return StagerJob.getFiles(c, this.ss, start, size, sortMet, ascending);
    }
    private static native StgFileInfo[] getFiles(Ctx c, StagerStream stgStream,
        int start, int size, short sortMet, boolean ascending)
        throws SamFSException;

    public static StagerJob[] getAll(Ctx c) throws SamFSException {
        StagerStream[] stgStreams = StagerStream.getAll(c);
        StagerJob[] stgJobs = new StagerJob[stgStreams.length];
        int i;
        for (i = 0; i < stgStreams.length; i++)
            stgJobs[i] = new StagerJob(stgStreams[i]);
        return stgJobs;
    }

    public void cancel(Ctx c) throws SamFSException {
        StagerJob.clearRequest(c, getMediaType(), getVSN());
    }

    /**
     *  first argument may include directory names.
     */
    public static native void cancelStgForFiles(Ctx c, String[] names,
        boolean recursive) throws SamFSException;
    public static native void clearRequest(Ctx c, String mediaType, String vsn)
        throws SamFSException;

    public String toString() {
        String s = super.toString() + " " + getVSN() + "," + getMediaType() +
            "," + getCreationTime() + " " + getCrtFileInfo();
        return s;
    }
}
