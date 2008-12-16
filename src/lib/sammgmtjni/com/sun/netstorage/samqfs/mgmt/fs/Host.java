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

// ident	$Id: Host.java,v 1.20 2008/12/16 00:08:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Ctx;


public class Host {
    public static final int CL_STATE_OFF = 0;
    public static final int CL_STATE_ON  = 1;

    /* Shared FS Host Function options must match options in pub/mgmt/hosts.h */
    public static final int MDS		= 0x0001;
    public static final int CLIENTS	= 0x0002;
    public static final int HOST_DETAILS = 0x0008;

    String name;
    String ipAddrs[];
    int srvPrio;
    boolean isCrtServer;
    int state;

    public Host(String name, String ipAddrs[], int srvPrio,
		 boolean crtServer) {
	this.name = name;
	this.ipAddrs = ipAddrs;
	this.srvPrio = srvPrio;
	this.isCrtServer = crtServer;
	this.state = CL_STATE_ON;
    }

    public Host(String name, String ipAddrs[], int srvPrio,
		 boolean crtServer, int state) {
	this.name = name;
	this.ipAddrs = ipAddrs;
	this.srvPrio = srvPrio;
	this.isCrtServer = crtServer;
	this.state = state;
    }


    /*
     * instance methods
     */

    public String getName() { return name; }
    public String[] getIPs() { return ipAddrs; }
    public int getSrvPrio() { return srvPrio; }
    public boolean isCrtServer() { return isCrtServer; }
    public int getState() { return state; }
    public String toString() {
	String s = name + ",";
	int i;
	if (null != ipAddrs)
	    for (i = 0; i < ipAddrs.length; i++)
		s += ipAddrs[i] + ",";
	s += "crt=" + (isCrtServer ? "T" : "F");
	return s;
    }


    /*
     * public class methods. must be called on the metadata server
     */

    /**
     * Function to add multiple clients to a shared file system. This
     * function may be run to completion in the background.
     *
     * options is a key value string supporting the following options:
     * mount_point="/fully/qualified/path"
     * mount_fs= yes | no
     * mount_at_boot = yes | no
     * bg_mount = yes | no
     * read_only = yes | no
     * potential_mds = yes | no
     *
     * Returns:
     * 0 for successful completion
     * -1 for error
     * job_id will be returned if the job has not completed.
     */
    public static native int addHosts(Ctx c, String fsname,
					Host[] hosts, String options)
	throws SamFSException;


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
    public static native int removeHosts(Ctx c, String fsname,
					   String[] hostNames)
	throws SamFSException;

    /*
     * Enable or disable client access. This operation is performed on
     * the metadata server. The states CL_STATE_ON and CL_STATE_OFF are
     * supported.
     */
    public static native void setClientState(Ctx c, String fsname,
	String[] hostNames, int state) throws SamFSException;

    /*
     * Method to retrieve data about shared file system hosts.
     *
     * By setting the options field you can determine what type
     * of hosts will be included and what data will be returned.
     *
     * The options field supports the flags:
     * MDS | CLIENTS | HOST_DETAILSS
     *
     * Where MDS returns the potential metadata information too.
     *
     * Keys shared by all classes of host include:
     * hostname = %s
     * type = client | mds | pmds
     * ip_addresses = space separated list of ips.
     * os = Operating System Version
     * version = sam/qfs version
     * arch = x86 | sparc
     * mounted = %d ( -1 means not mounted otherwise time mounted in seconds)
     * status = ON | OFF
     * error = assumed_dead | known_dead
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
     */
    public static native String[] getSharedFSHosts(Ctx c, String fsName,
	int options) throws SamFSException;

    /**
     * return a list of IP adresses and names under which this host is known
     */
    public static native String[] discoverIPsAndNames(Ctx c)
        throws SamFSException;

    /**
     * return a list of key value pairs describing the advanced network
     * configuration for a shared file system. This information comes
     * from the hosts.<fsName>.local file. If there is no hosts.<fsName>.local
     * file an empty list will be returned.
     *
     * The returned strings will be key value pairs with the following
     * keys:
     * hostname = <String>,
     * ipaddresses = <space separated list of ip/hostnames>
     */
    public static native String[] getAdvancedNetCfg(Ctx c, String fsName)
	throws SamFSException;


    /**
     * Setup the advanced network configuration for a shared file system.
     * This method will replace the existing configuration.
     * The host_strs should be in the format specified for
     * getAdvancedNetCfg
     */
    public static native void setAdvancedNetCfg(Ctx c, String fsName,
       	String[] hosts) throws SamFSException;

    /**
     * Returns the name of the server that is the metadata server host for
     * the named file system. If null is passed in for fsName, the metadata
     * server for the first shared file system found will be returned.
     */
    public static native String getMetadataServerName(Ctx c, String fsName)
        throws SamFSException;


    /* Pre 5.0 Shared File System Compatability Functions */

    /**
     * return the hosts that are allowed to access
     * the specified shared filesystem
     */
    public static native Host[] getConfig(Ctx ctx, String fsName)
	throws SamFSException;

    /**
     * remove specified host from the configuration of
     * the specified shared filesystem
     */
    public static native void removeFromConfig(Ctx ctx,
	String fsName, String hostName) throws SamFSException;

    /**
     * add specified host to the configuration of
     * the specified shared filesystem
     */
    public static native void addToConfig(Ctx c, String fsName, Host newHost)
	throws SamFSException;

}
