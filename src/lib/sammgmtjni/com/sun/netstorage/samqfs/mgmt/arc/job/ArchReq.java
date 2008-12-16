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

// ident	$Id: ArchReq.java,v 1.11 2008/12/16 00:08:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Ctx;

/**
 * An ArchReq object encapsulates information about a set of files that are part
 * of the same filesystem and are either currently being archived or are going
 * to be archived in the future.
 */
public class ArchReq {

    public static final int AR_acnors = 0x0001; /* arcopy norestart error */
    public static final int AR_disk   = 0x0002; /* Archive to disk */
    public static final int AR_first  = 0x0004; /* 1st time request composed */
    public static final int AR_honeycomb = 0x0008; /* Archive to honeycomb */
    public static final int AR_join	= 0x0010; /* has joined files */
    public static final int AR_nonstage = 0x0020; /* has non-stagable files */
    public static final int AR_offline  = 0x0040; /* has offline files */
    public static final int AR_schederr = 0x0080; /* Scheduler error */
    public static final int AR_segment  = 0x0100; /* has segmented files */
    public static final int AR_unqueue  = 0x0200; /* Unqueue request */

    private String fsName;
    private String arsetName; // this might include copy #. need to check
    private long creationTime;
    private int state;
    private long seqNum;
    private int drivesUsed;
    private int files;
    private long spaceNeeded; // space to archive all files in kb
    private short flags;

    private ArCopyProc[] copyProcs;

    private ArchReq(String fsName, String arsetName, long creationTime,
		    int state, long seqNum, int drivesUsed, int files,
		    long spaceNeeded, short flags,
		    ArCopyProc[] copyProcs) {
            this.fsName = fsName;
            this.arsetName = arsetName;
            this.creationTime = creationTime;
            this.state = state;
            this.seqNum = seqNum;
	    this.drivesUsed = drivesUsed;
	    this.files = files;
	    this.spaceNeeded = spaceNeeded;
	    this.flags = flags;
            this.copyProcs = copyProcs;
    }

    // possible states. must match the ArState enum defined in aml/archreq.h
    public static final int ARS_create   = 1;
    public static final int ARS_schedule = 2;
    public static final int ARS_archive  = 3;
    public static final int ARS_done = 4;

    public String getFSName() { return fsName; }
    public String getArsetName() { return arsetName; }
    public long getCreationTime() { return creationTime; }

    public int getState() { return state; }
    public boolean isBeingComposed() {
	return (state == ARS_create);
    }
    public boolean isBeingArchived() {
	return (state == ARS_archive);
    }
    public boolean isBeingScheduled() {
	return (state == ARS_schedule);
    }

    public long getSeqNum() { return seqNum; }
    public int getDrivesUsed() { return drivesUsed; }
    public int getFiles() { return files; }

    /*
     * Get the space needed to archive all files for this archreq.
     * The returned result is in kb.
     *
     * This total space may be broken up to be archived by several
     * ArCopyProcs. The amount written is available from the
     * individual ArCopyProcs.
     */
    public long getSpaceNeeded() { return spaceNeeded; }
    public short getFlags() { return flags; }
    public boolean isDisk() { return ((flags & AR_disk) != 0); }
    public boolean isSTK5800() { return ((flags & AR_honeycomb) != 0); }

    /*
     * There will be at least 1 ArCopyProc for each ArchReq.
     * If DrivesUsed > 1 there will be that many CopyProcs
     */
    public ArCopyProc[] getArCopyProcs() { return copyProcs; }

    public static native ArchReq[] getAll(Ctx c) throws SamFSException;
}
