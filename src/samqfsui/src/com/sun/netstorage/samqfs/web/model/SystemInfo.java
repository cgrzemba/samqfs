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

// ident $Id: SystemInfo.java,v 1.7 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates basic information about a Solaris server
 */
public class SystemInfo {

    // constant below must match those in mgmt.h
    final String KEY_HOSTID   = "Hostid";
    final String KEY_HOSTNAME = "Hostname";
    final String KEY_OSNAME   = "OSname";
    final String KEY_RELEASE  = "Release";
    final String KEY_VERSION  = "Version";
    final String KEY_MACHINE  = "Machine";
    final String KEY_CPUS = "Cpus";
    final String KEY_MEM  = "Memory";
    final String KEY_ARCHIT   = "Architecture";
    final String KEY_IPADDRS  = "IPaddress";

    protected String hostid, hostName, osName, release, version, machine;
    protected int cpus;
    protected long mem;
    protected String archit;
    protected String ipAddresses; // space delimited list

    public SystemInfo(Properties props) throws SamFSException {
        if (props == null)
            return;

        hostid   = props.getProperty(KEY_HOSTID);
        hostName = props.getProperty(KEY_HOSTNAME);
        osName   = props.getProperty(KEY_OSNAME);
        release  = props.getProperty(KEY_RELEASE);
        version  = props.getProperty(KEY_VERSION);
        machine  = props.getProperty(KEY_MACHINE);
        cpus =
            ConversionUtil.strToIntVal(props.getProperty(KEY_CPUS));
        mem =
            ConversionUtil.strToLongVal(props.getProperty(KEY_MEM));
        archit = props.getProperty(KEY_ARCHIT);
        ipAddresses = props.getProperty(KEY_IPADDRS);
    }
    public SystemInfo(String sysInfoStr) throws SamFSException {
        this(ConversionUtil.strToProps(sysInfoStr));
    }

    public String getHostid()   { return hostid; }
    public String getHostname() { return hostName; }
    public String getOSname()   { return osName; }
    public String getRelease()  { return release; }
    public String getVersion()  { return version; }
    public String getMachine()  { return machine; }
    public int getCPUs() { return cpus; }
    public long getMemoryMB()   { return mem; }
    public String getArchitecture()  { return archit; }
    public String getIPAddresses() { return ipAddresses; }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_HOSTID   + E + hostid   + C +
                   KEY_HOSTNAME + E + hostName + C +
                   KEY_OSNAME   + E + osName   + C +
                   KEY_RELEASE  + E + release  + C +
                   KEY_VERSION  + E + version  + C +
                   KEY_MACHINE  + E + machine  + C +
                   KEY_CPUS + E + cpus + C +
                   KEY_MEM  + E + mem  + C +
                   KEY_ARCHIT   + E + archit   + C +
                   KEY_IPADDRS  + E + ipAddresses;
        return s;
    }
}
