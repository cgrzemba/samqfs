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

// ident	$Id: VSNPool.java,v 1.14 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.VSN;

public interface VSNPool {

    public static int MAX_VSN_IN_POOL_DISPLAY = 100;

    // pool name can not be changed for an existing pool
    public String getPoolName();

    public int getMediaType();

    // Space available is in KB.
    public long getSpaceAvailable() throws SamFSException;

    public String getVSNExpression();

    public VSN[] getMemberVSNs(int start,
                               int size,
                               int sortby,
                               boolean ascending) throws SamFSException;

    public void setMemberVSNs(int mediaType,
                              String expression) throws SamFSException;

    public int getNoOfVSNsInPool() throws SamFSException;

    /**
     * @since 4.4
     * @return a list of volumes, if this is a pool of disk volumes
     */
    public DiskVolume[] getMemberDiskVolumes(int start,
                                             int size,
                                             int sortby,
                                             boolean ascending)
        throws SamFSException;
}
