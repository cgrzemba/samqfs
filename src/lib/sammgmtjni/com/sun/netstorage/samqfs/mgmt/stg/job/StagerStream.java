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

// ident	$Id: StagerStream.java,v 1.12 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.stg.job;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class StagerStream {

    boolean active;
    String mediaType;
    int seqNum;
    String vsn;
    long count;
    long creationTime;
    StgFileInfo crtFileInfo;
    int stateFlags; /* see values defined below */

    private StagerStream(boolean active, String mediaType, int seqNum,
        String vsn, long count, long creationTime, StgFileInfo crtFileInfo,
        int stateFlags) {
            this.active = active;
            this.mediaType = mediaType;
            this.seqNum = seqNum;
            this.vsn = vsn;
            this.count = count;
            this.creationTime = creationTime;
            this.crtFileInfo = crtFileInfo;
            this.stateFlags = stateFlags;
    }

    static native StagerStream[] getAll(Ctx c) throws SamFSException;

}
