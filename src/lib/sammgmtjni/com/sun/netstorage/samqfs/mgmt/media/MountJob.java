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

// ident	$Id: MountJob.java,v 1.8 2008/03/17 14:43:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class MountJob extends Job {

    public static final short LD_BUSY   = 0x000001; // entry is busy don't touch
    public static final short LD_IN_USE = 0x000002; // entry in use (occupied)
    public static final short LD_P_ERROR = 0x000004; // clear vsn requested
    public static final short LD_WRITE  = 0x000008; // write access request flag
    public static final short LD_FS_REQ = 0x000010; // file system requested
    public static final short LD_BLOCK_IO = 0x000020; // block IO for opt/tp IO
    public static final short LD_STAGE    = 0x000040; // stage request flag

    public int id; // preview id
    public short flags;
    public int robotEq;
    public String mediaType;
    public long pid;
    public  String userName;
    public  String vsn;

    private MountJob(int id, short flags, int robotEq, String mediaType,
        long pid, String userName, String vsn) {
        super(Job.TYPE_MOUNT, Job.STATE_PENDING, id * 100 + Job.TYPE_MOUNT);
        this.id = id;
        this.flags = flags;
        this.robotEq = robotEq;
        this.mediaType = mediaType;
        this.pid = pid;
        this.userName = userName;
        this.vsn = vsn;
    }

    public static native MountJob[] getAll(Ctx c) throws SamFSException;
    public void cancel(Ctx c) throws SamFSException {
        cancel(c, this.vsn, this.id);
    }
    public static native void cancel(Ctx c, String vsn, int idx)
        throws SamFSException;

    public String toString() {
        String s = super.toString() + " " + id + "," + flags + ",eq:" + robotEq
            + "," + mediaType + "," + pid + "," + userName + "," + vsn;
        return s;
    }
}
