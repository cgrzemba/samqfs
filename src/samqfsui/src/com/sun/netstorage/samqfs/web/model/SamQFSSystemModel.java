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

// ident	$Id: SamQFSSystemModel.java,v 1.55 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.fs.NFSOptions;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;

public interface SamQFSSystemModel {

    // VERSION possibilites
    public static final int VERSION_SAME = 1001;
    public static final int VERSION_CLIENT_NEWER = 1002;
    public static final int VERSION_SERVER_NEWER = 1003;


    // licensed FS type
    public static final short QFS = 1; // stand alone QFS
    public static final short SAMFS = 2; // SAM-FS
    public static final short SAMQFS = 3; // SAM-QFS

    // size units
    public static final int SIZE_B  = 0;
    public static final int SIZE_KB = 1;
    public static final int SIZE_MB = 2;
    public static final int SIZE_GB = 3;
    public static final int SIZE_TB = 4;
    public static final int SIZE_PB = 41;


    // time units
    public static final int TIME_SECOND = 5;
    public static final int TIME_MINUTE = 6;
    public static final int TIME_HOUR   = 7;
    public static final int TIME_DAY    = 8;
    public static final int TIME_WEEK   = 9;
    public static final int TIME_YEAR   = 10;

    // additional time units used for periodicity
    public static final int TIME_MONTHS = 11;
    public static final int TIME_DAY_OF_MONTH = 12;
    public static final int TIME_DAY_OF_WEEK  = 13;

    // activities
    public static final int ACTIVITY_ARCHIVE_RESTART = 20;
    public static final int ACTIVITY_ARCHIVE_IDLE = 21;
    public static final int ACTIVITY_ARCHIVE_RUN  = 22;
    public static final int ACTIVITY_ARCHIVE_RERUN = 23;
    public static final int ACTIVITY_ARCHIVE_STOP = 24;

    public static final int ACTIVITY_STAGE_IDLE = 30;
    public static final int ACTIVITY_STAGE_RUN = 31;

    // CIS custom report directory (Added in 4.6)
    public static final
        String CIS_REPORT_DIR = "/var/opt/SUNWsamfs/CIS-reports";


    // This API return whether client and server version are the same or
    // different. The other method getServerAPIVersion() gives the actual
    // server version when the server is the same or older version than
    // the client. If server is newer than client, then getServerAPIVersion()
    // (probably) will not work.
    public int getVersionStatus();

    public String getServerAPIVersion();

    public String getServerProductVersion();

    public short getLicenseType() throws SamFSException;

    public void reconnect() throws SamFSException;

    public void reinitialize() throws SamFSException;

    /**
     * Returns the name of the host as seen in the server selection page.
     */
    public String getHostname();

    /**
     * Returns the name of the host returned by gethostname (3C) on that
     * host.
     */
    public String getServerHostname();


    // SunCluster-related methods. since 4.5
    public boolean isClusterNode();
    public String getClusterVersion();
    public String getClusterName();
    /**
     * @returns one of the constants specified below
     */
    public int getPlexMgrState() throws SamFSException;

    // SC UI should be reachable at https://hostname:6789/SunPlexManager/
    public static final int PLEXMGR_RUNNING = 0;
    // smcwebserver must be started on the node
    public static final int PLEXMGR_INSTALLED_AND_REG = 1;
    // SC UI must register with the webserver on the node
    public static final int PLEXMGR_INSTALLED_NOT_REG = 2;
    // SC UI (non-lockhart) should be reachable at https://<hostname>:3000/
    public static final int PLEXMGROLD_INSTALLED = 3;
    // SC UI packages must be installed
    public static final int PLEXMGR_NOTINSTALLED = 4;


    /**
     * Returns the architecture of the host
     */
    public String getArchitecture();

    public boolean isDown();

    public void setDown();

    public boolean isAccessDenied();

    public boolean isServerSupported();

    public SystemCapacity getCapacity() throws SamFSException;

    public SystemInfo getSystemInfo() throws SamFSException;

    public LogAndTraceInfo[] getLogAndTraceInfo() throws SamFSException;

    public PkgInfo[] getPkgInfo() throws SamFSException;

    public DaemonInfo[] getDaemonInfo() throws SamFSException;

    public ConfigStatus[] getConfigStatus() throws SamFSException;
    public String[] getConfigStatusDetails(String configKey)
        throws SamFSException;

    // Added in 4.5
    public ClusterNodeInfo[] getClusterNodes() throws SamFSException;

    // Added in 4.5
    public SamExplorerOutputs[] getSamExplorerOutputs() throws SamFSException;
    public long runSamExplorer(String location, int logLines)
        throws SamFSException;

    // Added in 4.6 for CIS support
    public String getMetadataServerName() throws SamFSException;

    public SamQFSSystemFSManager getSamQFSSystemFSManager();

    public SamQFSSystemArchiveManager getSamQFSSystemArchiveManager();

    public SamQFSSystemMediaManager getSamQFSSystemMediaManager();

    public SamQFSSystemJobManager getSamQFSSystemJobManager();

    public SamQFSSystemAlarmManager getSamQFSSystemAlarmManager();

    public SamQFSSystemAdminManager getSamQFSSystemAdminManager();

    public boolean doesFileExist(String filePath) throws SamFSException;

    /**
     * @since 4.6
     * @return an array of NFSOptions (System-wide, file system independent)
     */
    public NFSOptions[] getNFSOptions() throws SamFSException;

    /**
     * @since 4.6
     * set nfs options (System-wide, file system independent)
     */
    public void setNFSOptions(NFSOptions opts) throws SamFSException;

    /**
     * @since 4.6
     * @return NFS Option object of the NFS Share directory
     * get a single nfs option by dir name (System-wide)
     */
    public NFSOptions getNFSOption(String dirName) throws SamFSException;

    /**
     * @since 4.6
     * @return an array of StageFile that holds all the custom reports found in
     *  CIS_REPORT_DIR directory.
     */
    public StageFile [] getCISReportEntries() throws SamFSException;

}
