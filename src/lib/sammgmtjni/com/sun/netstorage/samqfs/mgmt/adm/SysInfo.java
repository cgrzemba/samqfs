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

// ident	$Id: SysInfo.java,v 1.18 2008/05/14 21:02:55 pg125177 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class SysInfo {

    /**
     * @return a string containing file system and media capacity usage info
     * format of the string is as follows:
     * MountedFS	= <number of mounted file systems>
     * DiskCache	= <diskcache of mounted SAM-FS/QFS in kbytes>
     * AvailableDiskCache	= <available disk cache in kbytes>
     * LibCount		= <number of libraries>
     * MediaCapacity		= <capacity of library in kbytes>
     * AvailableMediaCapacity	= <available capacity in kbytes>
     * SlotCount	= <number of configured slots>
     *
     * NOTE: If this function is unable to retrieve any of the information it
     * will simply be omitted. Capacity information is only reported for mounted
     * file systems and is only reported for shared file system if the function
     * is called on the metadata server.
     */
    public static native String getCapacity(Ctx c) throws SamFSException;

    /**
     * @return a string containing system information
     * format for the string is as follows:
     * Hostid=<hostid>,
     * Hostname=<hostname>,
     * OSname=<sysname>,
     * Release=<release>,
     * Version=<version>,
     * Machine=<machine>,
     * Cpus=<number>,
     * Memory=<memory in Mbytes>,
     * Architecture=<arc>,
     * IPaddress=<ipaddress1 ipaddress2..>
     *
     * NOTE: If this function is unable to retrieve any of the information it
     * will simply be omitted
     */
    public static native String getOSInfo(Ctx c) throws SamFSException;


    /**
     * @return an array of Strings, each string will be formatted as follows:
     * Name=name, (e.g. sam-fsd, Device Log Libname eq, Archiver Log fsname)
     * type=Log/Trace,
     * state=on/off,
     * path=filename
     * flags=flags, (e.g. all date err default) space separated flag
     * size=size,
     * modtime=last modified time (num of seconds since 1970)
     */
    public static native String[] getLogInfo(Ctx c) throws SamFSException;


    /**
     * @return an array of Strings, each string formatted as follows:
     * PKGINST = SUNWsamfsu
     *	NAME = Sun SAM and Sun SAM-QFS software Solaris 10 (usr)
     * CATEGORY = system
     *	ARCH = sparc
     * VERSION = 4.6.5 REV=debug REV=5.10.2007.03.12
     * VENDOR = Sun Microsystems Inc.
     *	DESC = Storage and Archive Manager File System
     * PSTAMP = ns-east-6420050118171403
     * INSTDATE = Jan 20 2005 13:58
     * HOTLINE = Please contact your local service provider
     * STATUS = completely installed
     */
    public static native String[] getPackageInfo(Ctx c, String pkgname)
        throws SamFSException;


    /**
     * @return an array of Strings, each String formatted as follows:
     * CONFIG = archiver.cmd e.g.
     * STATUS = OK | WARNINGS | ERRORS | MODIFIED
     */
    public static native String[] getConfigStatus(Ctx c)
	throws SamFSException;


    /**
     * @return SC version from /etc/cluster/release
     */
    public static native String getSCVersion(Ctx c) throws SamFSException;

    /**
     * @return SC name, as specified during SC install
     */
    public static native String getSCName(Ctx c) throws SamFSException;

    /**
     * @return SC nodes information, each String formatted like this:
     * sc_nodename = <nodename>
     * sc_nodestatus = UP/DOWN/unknown
     * sc_nodeid = <nodeclusterid>
     * sc_nodeprivaddr = <private IP address>
     */
    public static native String[] getSCNodes(Ctx c) throws SamFSException;

    /**
     * @return SC UI (PlexMgr) state. see pub/mgmt/mgmt.h for return codes
     */
    public static native int getSCUIState(Ctx c) throws SamFSException;

    /**
     * @return an array of Strings, each String contails the
     * path to a samexplorer output
     */
    public static native String[] listSamExplorerOutputs(Ctx c)
        throws SamFSException;


    /**
     * run sam explorer
     * location is the directory in which the output will be saved.
     * logLines is the nubmer of lines from each logfile that will be
     * included in the output.
     *
     * @return null if explorer successfully completes or an activity id for
     * a SAMARUNEXPLORER activity if explorer is still running.
     */
    public static native String runSamExplorer(Ctx c, String location,
            int logLines)
        throws SamFSException;


    /**
     * retrieves the status of the various processes and tasks in a SAM-FS
     * configuration. This allows the user to perform active monitoring
     *
     * * @return String[] array of formated strings with status information
     *
     * return key-value:-
     * activityid=pid (if absent, then it is not running)
     * details=daemon or process name
     * type=SAMDXXXX
     * description=long user friendly name
     * starttime=secs
     * parentid=ppid
     * modtime=logfile modetime in secs
     * path=/var/opt/SUNWsamfs/devlog/2
     *
     */
    public static native String[] getProcessStatus(Ctx c)
        throws SamFSException;

    public static final int COMPONENT_STATUS_OK = 0;
    public static final int COMPONENT_STATUS_WARNING  = 1;
    public static final int COMPONENT_STATUS_ERR = 2;
    public static final int COMPONENT_STATUS_FAILURE  = 3;

    /**
     * retrieves the status summary of the various components and tasks
     * in a SAM-FS configuration.
     * SAM-FS daemons, File systems and Disk Storage, automated libraries, tape
     * drives, tape and disk VSNs, archiver, stager, releaser and recycler are
     * examples of components in SAM-FS configuration. The Archive Scan Queues,
     * Archive Copy Requests, Media load requests, and Stage queues are
     * different tasks in a SAM-FS configuration.
     *
     * @return String[] array of int with error status information
     * Returned list of strings:-
     *  name=daemons,status=
     *  name=fs,status=
     *  name=copyUtil,status=
     *  name=libraries,status=
     *  name=drives,status=
     *  name=loadQ,status=
     *  name=unusableVsn,status=
     *  name=arcopyQ,status=
     *  name=stageQ,status=
     */
    public static native String[] getComponentStatusSummary(Ctx c)
        throws SamFSException;

    /*
     * This method is to support the First Time Configuration Checklist.
     * It provides the information that allows the GUI to show feedback
     * to the users that things have occurred.
     *
     * Key value string showing the status.
     * Keys ==> Value type
     * lib_count = int
     * lib_names = space separated list.
     * tape_count = int
     * qfs_count = int
     * disk_vols_count = int
     * volume_pools = int
     * object_qfs_protos = int (number of HPC file systems
     *				currently partially created)
     * ojbect_qfs_names = space separated list of names.
     * Storage_nodes = int (only provided if object_qfs_proto_count == 1)
     * clients = int (only provided if object_qfs_proto_count == 1)
     */
    public static native String getConfigurationSummary(Ctx c)
	throws SamFSException;

}
