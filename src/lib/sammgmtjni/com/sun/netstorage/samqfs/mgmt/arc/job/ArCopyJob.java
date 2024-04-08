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

// ident	$Id: ArCopyJob.java,v 1.12 2008/12/16 00:08:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc.job;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import java.util.ArrayList;

/**
 *
 * This class does not have a direct equivalent in the C API.
 * It roughly maps to a ArCopy and it also includes information from the
 * corresponding ArchReq.
 */

public class ArCopyJob extends Job {

    private ArchReq archReq;
    private ArCopyProc arCopy;

    public ArCopyJob(ArchReq archReq, ArCopyProc arCopy) {
        super(Job.TYPE_ARCOPY,
            ((archReq.getState() == ArchReq.ARS_archive) &&
             (arCopy.getPID() != 0)) ? Job.STATE_CURRENT : Job.STATE_PENDING,
             ((arCopy.getPID() != 0) ? arCopy.getPID() :
                 archReq.getSeqNum() + 1) * 100 + Job.TYPE_ARCOPY);
        this.archReq = archReq;
        this.arCopy = arCopy;
    }

    public String getFSName() { return archReq.getFSName(); }
    public String getARSetName() { return archReq.getArsetName(); }
    public long getCreationTime() { return archReq.getCreationTime(); }

    public long getSpaceNeeded() {
	if (archReq.isBeingComposed() || archReq.getDrivesUsed() <= 1) {
	    // If the archReq is still being composed or there
	    // are fewer than 1 drive being used return the value for the
	    // entire archreq.
	    return archReq.getSpaceNeeded();
	}

	return arCopy.getSpaceNeeded();
    }

    /*
     * Method is misnamed. It returns KB.
     */
    public long getBytesWritten() {
	// If the archreq is still being composed or no drives are assigned
	// then return 0. Else if the bytes written is not sane return 0.
	if (archReq.isBeingComposed() || archReq.getDrivesUsed() < 1) {
	    return 0;
	} else if (arCopy.getBytesWritten() > archReq.getSpaceNeeded()) {
	    return 0;
	}
	return arCopy.getBytesWritten();
    }

    public int getFiles() {
	if (archReq.isBeingComposed() || archReq.getDrivesUsed() <= 1) {
	    // If the archReq is still being composed or there
	    // is only 1 drive assigned return the value for the
	    // entire archreq.
	    return archReq.getFiles();
	}
	return arCopy.getFiles();
    }

    public int getFilesWritten() {
	int drives = archReq.getDrivesUsed();
	if (drives == 0 || (arCopy.getFilesWritten() > archReq.getFiles())) {
	    // There are no archive copies so nothing has been written.
	    // or the files written value doesn't make sense. So return 0.
	    return 0;
	} else {
	    return arCopy.getFilesWritten();
	}
    }

    public String getMediaType() {
	String media = arCopy.getMediaType();
	if (media == null || media.length() == 0) {
	    if (archReq.isDisk()) {
		return new String("dk");
	    } else if (archReq.isSTK5800()) {
		return new String("cb");
	    }
	}
	return media;
    }

    public String getVSN() { return arCopy.getVSN(); }
    public String getStatus() {
	return arCopy.getOprMsg();
    }

    public static ArCopyJob[] getAll(Ctx c) throws SamFSException {
        ArchReq[] archReqs = ArchReq.getAll(c);
        ArrayList cpJobs = new ArrayList();
        ArCopyJob[] a;
        int i, j;
        for (i = 0; i < archReqs.length; i++) {
	    for (j = 0; j < archReqs[i].getArCopyProcs().length; j++) {
		cpJobs.add(new ArCopyJob(archReqs[i],
					 archReqs[i].getArCopyProcs()[j]));
	    }
	}
        a = new ArCopyJob[cpJobs.size()];
        return (ArCopyJob[])cpJobs.toArray(a);
    }

    public String toString() {
        String s = super.toString() + " " + getFSName() + "," + getARSetName() +
        " created:" + getCreationTime() + " written:" + getBytesWritten() + ","
        + getSpaceNeeded() + "k,files:" + getFiles() + "," + getMediaType() +
        "," + getVSN();
        return s;
    }
}
