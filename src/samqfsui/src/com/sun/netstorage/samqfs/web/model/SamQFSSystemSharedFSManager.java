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

// ident	$Id: SamQFSSystemSharedFSManager.java,v 1.24 2008/09/03 19:46:04 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;
import com.sun.netstorage.samqfs.mgmt.fs.FSArchCfg;
import com.sun.netstorage.samqfs.mgmt.fs.MountOptions;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.SharedFSFilter;
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
     * The following methods are used in the 5.0 release onwards.
     */

    /**
     * Function to add multiple clients to a shared file system. This
     * function may be run to completion in the background.
     * Returns:
     * 0 for successful completion
     * -1 for error
     * job_id will be returned if the job has not completed.
     */
    public long addClients(String serverName,
                           String fsName,
                           String [] clients) throws SamFSException;

    /**
     * The file system must be unmounted to remove clients. You can
     * disable access from clients without unmounting the file system
     * with the setClientState function.
     *
     * Returns: 0, 1, job ID
     * 0 for successful completion
     * -1 for error
     * job ID will be returned if the job has not completed.
     */
    public long removeClients(String serverName,
                              String fsName,
                              String [] clients) throws SamFSException;

    /*
     * Enable or disable client access. This operation is performed on
     * the metadata server. The states CL_STATE_ON and CL_STATE_OFF are
     * supported.
     */
    public void setClientState(String mdServer, String fsName,
	String[] hostNames, boolean on) throws SamFSException;

    /*
     * Method to retrieve data about shared file system hosts.
     *
     * By setting the options field you can determine what type
     * of hosts will be included and what data will be returned.
     *
     * The options field supports the flags:
     * MDS | CLIENTS | HOST_DETAILS
     *
     * Where MDS returns the potential metadata information too.
     *
     * For Storage Nodes the capacity is reported in the devices of the file
     * system. This call only returns information about the host, ip and
     * status.
     *
     * Keys shared by all classes of host include:
     * hostName = %s
     * type = client | mds | pmds
     * ip_addresses = space separated list of ips.
     * os = Operating System Version
     * version = sam/qfs version
     * arch = x86 | sparc
     * mounted = %d ( -1 means not mounted otherwise time mounted in seconds)
     * status = ON | OFF
     * error = assumed_dead | known_dead
     *
     * faults = %d (only on storage nodes. This key only present if faults
     *		    exist)
     *
     * If HOST_DETAILS flag is set in options the following will be obtained
     * for clients and real and potential metadata servers. Information
     * about decoding these can be found in the pub/mgmt/hosts.h header file.
     *
     * fi_status=<hex 32bit map>
     * fi_flags = <hex 32bit map>
     * mnt_cfg = <hex 32bit map>
     * mnt_cfg1 = <hex 32bit map>
     * no_msgs= int32
     * low_msg = uint32
     *
     * See model/fs/SharedFSFilter for the variable filters.
     */
    public SharedHostInfo [] getSharedFSHosts(
        String mdServer, String fsName, int options, SharedFSFilter [] filters)
        throws SamFSException;

    /*
     * Method to get summary status for a shared file system
     * Status is returned as key value strings.
     *
     * The method potentially returns three strings. One each for the
     * client status, pmds status and storage node status. If no storage
     * nodes are present no storage node string will be returned.
     *
     * The format is as follows:
     * clients=1024, unmounted=24, off=2, error=0
     * pmds = 8, unmounted=2, off=0, error=0
     */
    public MemberInfo [] getSharedFSSummaryStatus(
        String mdServer, String fsName) throws SamFSException;

    /*
     * A non-zero return indicates that a background job has been started
     * to complete this task.  Information can be obtained about this job by
     * using the Job.getAllActivities function with a filter on the job id.
     */
    public int mountClients(String mdServer, String fsName, String [] clients)
        throws SamFSException;

    /*
     * A non-zero return indicates that a background job has been started
     * to complete this task. Information can be obtained about this job by
     * using the Job.getAllActivities function with a filter on the job id.
     */
    public int unmountClients(String mdServer, String fsName, String [] clients)
        throws SamFSException;

    /*
     * A non-zero return indicates that a background job has been started
     * to complete this task. Information can be obtained about this job by
     * using the Job.getAllActivities function with a filter on the job id.
     */
    public int setSharedFSMountOptions(String mdServer, String fsName,
	String [] clients, MountOptions mo) throws SamFSException;


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
     * Get a list of all IP addresses assigned to this host.
     * @param hostName
     * @return Array of IP addresses.
     * @throws SamFSException if anything unexpected occurs.
     */
    public String[] getIPAddresses(String hostName) throws SamFSException;

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
     * Call this method when before the process exits to release any
     * resources (threads, etc.) that the garbage collector can't
     * take care of on its own.
     */
    public void freeResources();

    ////////////////////////////////////////////////////////////////////////////
    // The following methods are used strictly for 4.6 servers for backward
    // compatibility purpose

    /**
     *
     * Uses the C library to create a new shared file system.
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
     * Call this method to set how a shared member of a shared file system
     * talks to the metadata/potential metadata server.
     */
    public MDSAddresses [] getAdvancedNetworkConfig(
        String hostName, String fsName) throws SamFSException;


    /**
     * This is probably 4.6 only
     * @param hostNames
     * @param fsName
     * @param mdsName
     * @param addresses
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    public void setAdvancedNetworkConfigToMultipleHosts(
        String [] hostNames, String fsName,
        String mdsName, MDSAddresses [] addresses)
        throws SamFSMultiHostException, SamFSException;
}
