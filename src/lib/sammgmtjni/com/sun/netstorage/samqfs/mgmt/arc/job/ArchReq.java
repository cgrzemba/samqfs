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

// ident	$Id: ArchReq.java,v 1.8 2008/03/17 14:43:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Ctx;

/**
 * An ArchReq object encapsulates information about a set of files that are part
 * of the same filesystem and are either currently being archived or are going
 * to be archived in the future.
 */
public class ArchReq {

    private String fsName;
    private String arsetName; // this might include copy #. need to check
    private long creationTime;
    private int state;
    private long seqNum;

    private ArCopyProc[] copyProcs;

    private ArchReq(String fsName, String arsetName, long creationTime,
        int state, long seqNum,
        ArCopyProc[] copyProcs) {
            this.fsName = fsName;
            this.arsetName = arsetName;
            this.creationTime = creationTime;
            this.state = state;
            this.seqNum = seqNum;
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
    public long getSeqNum() { return seqNum; }
    public ArCopyProc[] getArCopyProcs() { return copyProcs; }

    public static native ArchReq[] getAll(Ctx c) throws SamFSException;
}
