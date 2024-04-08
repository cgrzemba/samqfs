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

// ident	$Id: SharedDiskCache.java,v 1.10 2008/12/16 00:12:23 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.media;

/**
 *  @see SamQFSSystemSharedFSManager.discoverAUsAvailForShared()
 *  @see DiskCache
 */
public interface SharedDiskCache extends DiskCache {

    /**
     * clients that can see this device AND don't use it
     */
    public String[] availFromClients();

    /**
     * return true if this slice is already in use on at least one
     * of the client hosts
     */
    public boolean usedByClient();

    /**
     * potential metadata servers that can see this device
     */
    public String[] availFromServers();


    /**
     * return logical devices names (paths).
     * the n-th element represents the local device name for
     * this DiskCache on the n-th client
     */
    public String[] getClientDevpaths();

    /**
     *  return logical device names (paths)
     * the n-th element represents the local device name for
     * this DiskCache on the n-th potential metadata server
     */
    public String[] getServerDevpaths();

}
