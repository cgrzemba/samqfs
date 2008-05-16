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

// ident $Id: GenericFileSystem.java,v 1.10 2008/05/16 18:39:00 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;

/**
 * basic information about a generic Solaris filesystem
 */
public interface GenericFileSystem {

    // type
    public static final int FS_SAM = 10;
    public static final int FS_QFS = 11;
    public static final int FS_SAMQFS  = 12;
    public static final int FS_NONSAMQ = 13; // ufs/zfs/...

    // state
    public static final int MOUNTED   = 0;
    public static final int UNMOUNTED = 1;


    // getters

    public String getName();
    public int getFSTypeByProduct(); // FS_...
    public String getFSTypeName(); // from mount table
    public int getState();
    public String getMountPoint();

    /**
     * @since 4.5
     * @return true if this fs is created on highly available storage
     */
    public boolean isHA();
    /**
     * @since 4.5
     * @return array of object representing fs instances for each host
     * @throws SamFSMultiHostException which may include a partial result
     */
    public GenericFileSystem[] getHAFSInstances()
	throws SamFSMultiHostException;

    /**
     * @since 4.4
     * @return true if this fs includes at least one directory
     * that is NFS-shared
     */
    public boolean hasNFSShares();

    public long getCapacity();
    public long getAvailableSpace();
    public int getConsumedSpacePercentage();

    /**
     * @since 4.4
     * the name of the host where this filesystem is used.
     * for shared-QFS clients, this is different from the metadata server name
     */
    public String getHostName();

    // fs operations

    public void mount() throws SamFSException;
    public void unmount() throws SamFSException;


    /**
     * @since 4.4
     * @return an array of NFSOptions (one element per shared directory)
     */
    public NFSOptions[] getNFSOptions() throws SamFSException;

    /**
     * @since 4.4
     * set nfs options for a particular shared directory
     */
    public void setNFSOptions(NFSOptions opts) throws SamFSException;

    //   public void removeNFSShares(String[] dirs) throws SamFSException;

}
