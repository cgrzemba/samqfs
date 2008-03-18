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

// ident	$Id: SamQFSSystemSharedFSManager.java,v 1.14 2008/03/17 14:43:42 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;
import com.sun.netstorage.samqfs.mgmt.fs.FSArchCfg;

import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;

/**
 *
 * This interface is used to manage shared file systems.
 *
 */
public interface SamQFSSystemSharedFSManager {

    /**
     * @return An array of <code>SharedDiskCache</code> objects .
     * representing the locally available allocatable units plus information
     * about which of the user specified hosts can see those au-s.
     * @param servers[] - array of md servers; FIRST one must be the real one
     * @param clients[] - array of clients
     * @throws SamFSMultiHostException if anything unexpected happens
     * @return a list of available devices along with information about where
     * those devices are accesible from.
     */
    public SharedDiskCache[] discoverAllocatableUnitsForShared(
        String[] servers,
	String[] clients,
        boolean ignoreHA)
        throws SamFSMultiHostException;

    /**
     *
     * Uses the C library to create a new shared file system.
     * @since 4.6
     * @param fsName
     * @param mountPoint
     * @param DAUSize
     * @param members
     * @param mountProps
     * @param metadataDevices
     * @param dataDevices
     * @param stripedGroups
     * @param mountAtBoot
     * @param createMountPoint
     * @param mountAfterCreate
     * @param archiveConfig
     * @throws SamFSMultiHostException if anything unexpected happens.
     * @return A FileSystem object.
     *
     */
    public FileSystem createSharedFileSystem(String fsName,
                                       String mountPoint,
                                       int DAUSize,
                                       SharedMember[] members,
                                       FileSystemMountProperties mountProps,
                                       DiskCache[] metadataDevices,
                                       DiskCache[] dataDevices,
                                       StripedGroup[] stripedGroups,
                                       boolean single,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate,
                                       FSArchCfg archiveConfig)
        throws SamFSMultiStepOpException, SamFSMultiHostException;

    /**
     *
     * Uses the C library to create a new shared file system.
     * @param fsName
     * @param mdServerName
     * @param mountPoint
     * @param hostName
     * @param hostIP IP address used for shared FS communication
     * @param mountAtBoot
     * @param createMountPoint
     * @param mountAfterCreate
     * @param potentialMdServer true if this host is to be a potential
     * metadata server, false if this host will be a client.
     * @throws SamFSMultiHostException if anything unexpected happens.
     * @return A FileSystem object.
     *
     */
    public FileSystem addHostToSharedFS(String fsName,
                                       String mdServerName,
                                       String mountPoint,
                                       String hostName,
                                       String[] hostIP,
                                       boolean readOnly,
                                       boolean mountAtBoot,
                                       boolean creatMountPoint,
                                       boolean mountAfterCreate,
                                       boolean potentialMdServer,
                                       boolean bg)
        throws SamFSMultiHostException;


    /**
     *
     * Delete the specified shared file system on the named host.
     * If you pass the hostname of a client or potential MD server,
     * it will remove the fs from that host only.  If you pass the
     * hostname of the metadata server, it will remove the fs from
     * all hosts and then delete the fs itself.
     * <p>
     * The FS must be unmounted on all hosts before
     * this method is called.
     * @param hostName
     * @param fsName Name of the File System to act on
     * @param mdHostName Metadata server name or null if not known
     * @throws SamFSMultiHostException if anything unexpected happens.
     *
     */
    public void deleteSharedFileSystem(String hostName, String fsName,
        String mdServerName)
        throws SamFSMultiHostException;

    /**
     * Get the failover status of the named shared file system.
     * @param mdServer Name of current metadata server
     * @param fsName Name of the File System to query
     * @return true if this server is in the process of failing over, false
     * if it is not.
     */
    public boolean failingover(String mdServer, String fsName)
        throws SamFSException;

    /**
     * Returns the type of server for the named file system.  Possible
     * return values are defined in:
     * <code>com.sun.netstorage.samqfs.web.model.fs.SharedMember</code>
     * @param hostName
     * @param fsName Name of file system to be queried.
     * @return One of the TYPE_ constants defined in SharedMember.
     *
     */
    public int getSharedFSType(String hostName, String fsName)
        throws SamFSException;

    /**
     * Get all the members of this shared file system.
     * @param mdServer Host name of the metadata server
     * @param fsName Name of the file system to query
     * @return An array of SharedMember representing the hosts that
     * are currently configured as part of this shared file system.
     */
    public SharedMember[] getSharedMembers(String mdServer, String fsName)
        throws SamFSMultiHostException;


    /**
     * Set the mount options for the named file system on all hosts
     * that have access to the shared file system.
     * @param mdServer Name of the metadata server
     * @param fsName Name of the shared file system to be operated on.
     * @param options Options to be applied to the FS on all hosts.
     * @throws SamFSMultiHostException if the operation fails
     * for some of the hosts.
     *
     */
    public void setSharedMountOptions(String mdServer, String fsName,
        FileSystemMountProperties options)
        throws SamFSMultiHostException;


    /**
     * Returns an array of host names representing hosts that are
     * currently managed by the application, but are not members of
     * the named shared file system.
     *
     * @param mdServer Metadata server for the named file system
     * @param fsName Name of the shared file system.
     * @return An array of host names.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public String[] getHostsNotUsedBy(String fsName, String mdServer)
        throws SamFSException;

    /**
     * Get a list of all IP addresses assigned to this host.
     * @param hostName
     * @return Array of IP addresses.
     * @throws SamFSException if anything unexpected occurs.
     */
    public String[] getIPAddresses(String hostName) throws SamFSException;

    /**
     * Create an instance of SharedMember with the specified properties
     * @param name Host name
     * @param ips Array of IP addresses
     * @param type One of the TYPE_ values defined in the SharedMember
     * interface.
     * @return The new SharedMember
     */
    public SharedMember createSharedMember(String name, String[] ips,
        int type);

    /**
     * Call this method when before the process exits to release any
     * resources (threads, etc.) that the garbage collector can't
     * take care of on its own.
     */
    public void freeResources();

    /**
     * Call this method to set how a shared member of a shared file system
     * talks to the metadata/potential metadata server.
     * This method is called in 4.5+ servers.  No version checking is needed
     * as 4.4 servers will never access to the Advanced Network Configuration
     * Setup Page.
     */
    public MDSAddresses [] getAdvancedNetworkConfig(
        String hostName, String fsName) throws SamFSException;


    public void setAdvancedNetworkConfigToMultipleHosts(
        String [] hostNames, String fsName,
        String mdsName, MDSAddresses [] addresses)
        throws SamFSMultiHostException, SamFSException;

}
