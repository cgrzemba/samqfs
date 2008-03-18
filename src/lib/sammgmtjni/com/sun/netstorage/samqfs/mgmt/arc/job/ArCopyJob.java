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

// ident	$Id: ArCopyJob.java,v 1.9 2008/03/17 14:43:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc.job;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import java.util.Vector;

/**
 *
 * This class does not have a direct equivalent in the C API.
 * It roughly maps to a ArCopy and it also includes informations from the
 * corresponding ArchReq
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

    public long getBytesWritten() { return arCopy.getBytesWritten(); } // kbtes
    public long getSpaceNeeded() { return arCopy.getSpaceNeeded(); } // kbytes
    public int getFiles() { return arCopy.getFiles(); }
    public String getMediaType() { return arCopy.getMediaType(); }
    public String getVSN() { return arCopy.getVSN(); }

    public static ArCopyJob[] getAll(Ctx c) throws SamFSException {
        ArchReq[] archReqs = ArchReq.getAll(c);
        Vector cpJobs = new Vector();
        ArCopyJob[] a;
        int i, j;
        for (i = 0; i < archReqs.length; i++)
            for (j = 0; j < archReqs[i].getArCopyProcs().length; j++)
                cpJobs.addElement(
                    new ArCopyJob(archReqs[i],
                        archReqs[i].getArCopyProcs()[j]));
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
