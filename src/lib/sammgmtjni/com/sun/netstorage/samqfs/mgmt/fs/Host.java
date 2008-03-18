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

// ident	$Id: Host.java,v 1.10 2008/03/17 14:43:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Ctx;


public class Host {

    String name;
    String ipAddrs[];
    int srvPrio;
    boolean isCrtServer;
    int hostsMounted;

    public Host(String name, String ipAddrs[], int srvPrio,
		 boolean crtServer) {
	this.name = name;
	this.ipAddrs = ipAddrs;
	this.srvPrio = srvPrio;
	this.isCrtServer = crtServer;
    }

    public Host(String name, String ipAddrs[], int srvPrio,
		 boolean crtServer, int hostsMounted) {
	this.name = name;
	this.ipAddrs = ipAddrs;
	this.srvPrio = srvPrio;
	this.isCrtServer = crtServer;
	this.hostsMounted = hostsMounted;
    }


    /*
     * instance methods
     */

    public String getName() { return name; }
    public String[] getIPs() { return ipAddrs; }
    public int getSrvPrio() { return srvPrio; }
    public boolean isCrtServer() { return isCrtServer; }
    public int getHostsMounted() { return hostsMounted; }
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
     * public class methods. must be called on a [potential] metadata server
     */

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

}
