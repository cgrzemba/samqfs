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

// ident	$Id: SamQFSSystemModelImpl.java,v 1.100 2008/09/04 19:30:35 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.GetList;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSAccessDeniedException;
import com.sun.netstorage.samqfs.mgmt.SamFSCommException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSIncompatVerException;
import com.sun.netstorage.samqfs.mgmt.adm.License;
import com.sun.netstorage.samqfs.mgmt.adm.SysInfo;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.Host;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;
import com.sun.netstorage.samqfs.web.model.ConfigStatus;
import com.sun.netstorage.samqfs.web.model.DaemonInfo;
import com.sun.netstorage.samqfs.web.model.LogAndTraceInfo;
import com.sun.netstorage.samqfs.web.model.PkgInfo;
import com.sun.netstorage.samqfs.web.model.SamExplorerOutputs;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAdminManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAlarmManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemJobManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SystemCapacity;
import com.sun.netstorage.samqfs.web.model.SystemInfo;
import com.sun.netstorage.samqfs.web.model.admin.Configuration;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.NFSOptions;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.Filter;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

public class SamQFSSystemModelImpl implements SamQFSSystemModel {


    public static long DEFAULT_RPC_TIME_OUT = 65;

    public static long DEFAULT_RPC_CONNECTION_TIME_OUT = 5;

    public static long DEFAULT_RPC_MAX_TIME_OUT = 110;

    private String hostname = new String();

    private boolean down = false;
    private boolean accessDenied = false;
    private boolean serverSupported = true;
    private boolean objectBasedFSSupported = true;

    private SamFSConnection connection = null;
    private String serverAPIVersion = new String();
    private String serverProductVersion = new String();
    private Ctx context = null;

    private short license = -1;

    private int versionStatus = SamQFSSystemModel.VERSION_SAME;

    private SamQFSSystemFSManager fsManager = null;
    private SamQFSSystemArchiveManager archiveManager = null;
    private SamQFSSystemMediaManager mediaManager = null;
    private SamQFSSystemJobManager jobManager = null;
    private SamQFSSystemAlarmManager alarmManager = null;
    private SamQFSSystemAdminManager adminManager = null;

    private String scVersion = "N/A", scName = "N/A";

    private boolean storageNode = false;

    public SamQFSSystemModelImpl(String hostname, boolean storageNode) {

        this(hostname);
        this.storageNode = storageNode;
    }

    public SamQFSSystemModelImpl(String hostname) {

        if (!SamQFSUtil.isValidString(hostname))
            throw new IllegalArgumentException();

        this.hostname = hostname;

        try {
            connection = SamFSConnection.
                getNewSetTimeout(hostname, DEFAULT_RPC_CONNECTION_TIME_OUT);
            connection.setTimeout(DEFAULT_RPC_TIME_OUT);
            context = new Ctx(connection, new AuditableImpl());

            try {
                serverAPIVersion = connection.getServerAPIver();
                serverProductVersion = connection.getServerVer();
            } catch (Exception e) {
                TraceUtil.trace1(
                    "Exception caught while retrieving api & product version!",
                    e);
                serverAPIVersion = "1.0";
                serverProductVersion = "4.1";
            }

            TraceUtil.trace1("Server API Version: " + serverAPIVersion);

            down = false;
            accessDenied = false;

            // Check if the backend is within 2 revs,
            // Show "Not supported" when it's out of the support rev range
            serverSupported = serverAPIVersion.compareTo("1.5") >= 0;

            // Assign flag to determine if host is capable of creating object
            // based file system
            objectBasedFSSupported = serverAPIVersion.compareTo("1.6") >= 0;

            TraceUtil.trace3("serverSupported: " + serverSupported);
            TraceUtil.trace3(
                "objectBasedFSSupported: " + objectBasedFSSupported);

            if (serverSupported) {
                try {
                    scVersion = SysInfo.getSCVersion(context);
                    if (scVersion != null)
                        if (scVersion.length() > 0) // if SC present
                            scName = SysInfo.getSCName(context);
                } catch (SamFSException e) {
                    TraceUtil.trace1(
                        "Exception caught while retrieving cluster info!",
                        e);
                }
            } else {
                scVersion = "";
            }
            TraceUtil.trace3("scVersion: " + scVersion);

        } catch (Exception e) {
            TraceUtil.trace1(
                "Exception caught in main sysModel constructor!",
                e);
            down = true;
            scVersion = "";
            if (e instanceof SamFSIncompatVerException) {
                if (((SamFSIncompatVerException) e).isClientVerNewer()) {
                    versionStatus = SamQFSSystemModel.VERSION_CLIENT_NEWER;
                } else {
                    versionStatus = SamQFSSystemModel.VERSION_SERVER_NEWER;
                }
            } else if (e instanceof SamFSAccessDeniedException) {
                accessDenied = true;
            }

            TraceUtil.trace3("accessDenied: " + accessDenied);
            TraceUtil.trace3("versionStatus: " + versionStatus);
        }

        processLicense();

    }



    public void reconnect() throws SamFSException {

        try {

            SamQFSUtil.doPrint("Debug: Calling reconnect...");
            System.out.println("Debug: Calling reconnect...");

            // destroy the previous connection, if possible
            if (connection != null) {
                try {
                    connection.destroy();
                } catch (SamFSException e) {
                    // do nothing
                }
            }
            connection = SamFSConnection.
                getNewSetTimeout(hostname, DEFAULT_RPC_CONNECTION_TIME_OUT);
            connection.setTimeout(DEFAULT_RPC_TIME_OUT);
            context = new Ctx(connection, new AuditableImpl());

            try {
                serverAPIVersion = connection.getServerAPIver();
                serverProductVersion = connection.getServerVer();
            } catch (Exception e) {
                serverAPIVersion = "1.0";
                serverProductVersion = "4.1";
            }

            down = false;

            scVersion = "n/a2";
	    scName = "n/a2";
            try {
                scVersion = SysInfo.getSCVersion(context);

                if (scVersion != null)
                    if (scVersion.length() > 0) // if SC present
                        scName = SysInfo.getSCName(context);
            } catch (SamFSException e) {
                TraceUtil.trace1("[ignored]" + e);
            }
        } catch (Exception e) {
            down = true;
            if (e instanceof SamFSIncompatVerException) {
                if (((SamFSIncompatVerException) e).isClientVerNewer())
                    versionStatus = SamQFSSystemModel.VERSION_CLIENT_NEWER;
                else
                    versionStatus = SamQFSSystemModel.VERSION_SERVER_NEWER;
            }
        }
        processLicense();
    }


    public void reinitialize() throws SamFSException {
        if (connection != null) {
            SamQFSUtil.doPrint("Debug: Calling reinit...");
            connection.reinit();
        }
    }

    public void terminateConn() {
        if (connection != null) {
            try {
                connection.destroy();
                connection = null;
            } catch (SamFSException e) { }; // never thrown from native code
        }
    }

    public int getVersionStatus() { return versionStatus; }

    public String getServerAPIVersion() { return serverAPIVersion; }

    public String getServerProductVersion() { return serverProductVersion; }

    public String getHostname() { return hostname; }

    public String getServerHostname() {
	String serverHostName = hostname;
        if (getJniConnection() != null) {
            serverHostName = getJniConnection().getServerHostname();
        }
	return serverHostName;
    }

    // SC related method group. since 4.5
    public boolean isClusterNode() {
        if (scVersion != null)
            if (scVersion.length() > 0)
                return true;
        return false;
    }
    public String getClusterVersion() { return scVersion; }
    public String getClusterName() { return scName; }
    public int getPlexMgrState() throws SamFSException {
        return SysInfo.getSCUIState(context);
    }

    public String getArchitecture() {

        String architecture = new String();
        if (connection != null) {
            architecture = getJniConnection().getServerArch();
        }
        return architecture;
    }


    public boolean isDown() { return down; }
    public void setDown() { down = true; }


    public short getLicenseType() throws SamFSException {
        if (license == -1)
	    processLicense();
        return license;
    }

    public synchronized Ctx getJniContext() {
        return context;
    }


    public synchronized SamFSConnection getJniConnection() {
        return connection;
    }

    public SystemCapacity getCapacity() throws SamFSException {
        String capacity = SysInfo.getCapacity(context);
        return new SystemCapacity(ConversionUtil.strToProps(capacity));
    }

    public SystemInfo getSystemInfo() throws SamFSException {
        return new SystemInfo(SysInfo.getOSInfo(context));
    }

    public PkgInfo[] getPkgInfo() throws SamFSException {

        PkgInfo[] pkgInfo = null;
        String[] pkgInfoStrs = SysInfo.getPackageInfo(context, null);

        if (pkgInfoStrs == null)
            return new PkgInfo[0];
        else
            pkgInfo = new PkgInfo[pkgInfoStrs.length];
        for (int i = 0; i < pkgInfoStrs.length; i++)
            pkgInfo[i] = new PkgInfo(pkgInfoStrs[i]);
        return pkgInfo;
    }

    public LogAndTraceInfo[] getLogAndTraceInfo() throws SamFSException {

        LogAndTraceInfo[] logInfo = null;
        String[] logAndTraceStrs = SysInfo.getLogInfo(context);

        if (logAndTraceStrs == null)
            return new LogAndTraceInfo[0];
        else
            logInfo = new LogAndTraceInfo[logAndTraceStrs.length];
        for (int i = 0; i < logAndTraceStrs.length; i++)
            logInfo[i] = new LogAndTraceInfo(logAndTraceStrs[i]);
        return logInfo;
    }

    public DaemonInfo[] getDaemonInfo() throws SamFSException {

        DaemonInfo[] dmnInfo = null;
        String[] daemonInfoStrs =
            Job.getAllActivities(context, 100, "type=SAMD");

        if (daemonInfoStrs == null)
            return new DaemonInfo[0];
        else
            dmnInfo = new DaemonInfo[daemonInfoStrs.length];
        for (int i = 0; i < daemonInfoStrs.length; i++)
            dmnInfo[i] = new DaemonInfo(daemonInfoStrs[i]);
        return dmnInfo;
    }


    public ConfigStatus[] getConfigStatus() throws SamFSException {

        ConfigStatus[] cfgStatus = null;
        String[] cfgStatusStrs = SysInfo.getConfigStatus(context);

        if (cfgStatusStrs == null)
            return new ConfigStatus[0];
        else
            cfgStatus = new ConfigStatus[cfgStatusStrs.length];
        for (int i = 0; i < cfgStatusStrs.length; i++)
            cfgStatus[i] = new ConfigStatus(cfgStatusStrs[i]);
        return cfgStatus;
    }

    public String[] getConfigStatusDetails(String keyConfig) {
        return new String[0]; // TODO
    }

    // since 4.5
    public ClusterNodeInfo[] getClusterNodes() throws SamFSException {

        ClusterNodeInfo[] nodes = null;
        String[] scNodeStrs = SysInfo.getSCNodes(context);

        if (scNodeStrs == null)
            return new ClusterNodeInfo[0];
        else
            nodes = new ClusterNodeInfo[scNodeStrs.length];
        for (int i = 0; i < scNodeStrs.length; i++)
            nodes[i] = new ClusterNodeInfo(scNodeStrs[i]);
        return nodes;
    }

    // since 4.5
    public SamExplorerOutputs[] getSamExplorerOutputs() throws SamFSException {

        SamExplorerOutputs[] outputs = null;
        String[] explorerOutputs = SysInfo.listSamExplorerOutputs(context);

        if (explorerOutputs == null)
            return new SamExplorerOutputs[0];
        else
            outputs = new SamExplorerOutputs[explorerOutputs.length];
        for (int i = 0; i < explorerOutputs.length; i++)
            outputs[i] = new SamExplorerOutputs(explorerOutputs[i]);
        return outputs;
    }

    // since 4.5
    public long runSamExplorer(String location, int logLines)
        throws SamFSException {

        if (location != null && logLines > 0) {
            String jobID =
                SysInfo.runSamExplorer(getJniContext(), location, logLines);
	    return 100 * ConversionUtil.strToLongVal(jobID)
                + Job.TYPE_RUN_EXPLORER;

        } else {
            throw new SamFSException(
                "Developer's bug: location is null or logLines <= 0!");
        }
    }

    public String toString() {

        StringBuffer buf = new StringBuffer();
        buf.append("=====================================\n");

        buf.append("Hostname: " + hostname + "\n");

        try {

            FileSystem[] fsList = getSamQFSSystemFSManager().
                getAllFileSystems();
            ArchivePolicy[] policyList = getSamQFSSystemArchiveManager().
                getAllArchivePolicies();
            Library[] libList = getSamQFSSystemMediaManager().getAllLibraries();

            buf.append("File Systems:\n");
            for (int i = 0; i < fsList.length; i++)
                buf.append(fsList[i]);

            buf.append("Archive Policies:\n");
            for (int i = 0; i < policyList.length; i++)
                buf.append(policyList[i]);

            buf.append("Libraries:\n");
            for (int i = 0; i < libList.length; i++)
                buf.append(libList[i]);

        } catch (Exception e) {
            processException(e);
        }

        buf.append("=====================================\n");

        return buf.toString();

    }

    public SamQFSSystemFSManager getSamQFSSystemFSManager() {
	if (fsManager == null) {
	    fsManager = new SamQFSSystemFSManagerImpl(this);
	}
	return fsManager;
    }

    public SamQFSSystemArchiveManager getSamQFSSystemArchiveManager() {
	if (archiveManager == null) {
	    archiveManager = new SamQFSSystemArchiveManagerImpl(this);
	}
	return archiveManager;
    }

    public SamQFSSystemMediaManager getSamQFSSystemMediaManager() {
	if (mediaManager == null) {
	    mediaManager = new SamQFSSystemMediaManagerImpl(this);
	}
	return mediaManager;
    }

    public SamQFSSystemJobManager getSamQFSSystemJobManager() {
	if (jobManager == null) {
	    jobManager = new SamQFSSystemJobManagerImpl(this);
	}
	return jobManager;
    }

    public SamQFSSystemAlarmManager getSamQFSSystemAlarmManager() {
	if (alarmManager == null) {
	    alarmManager = new SamQFSSystemAlarmManagerImpl(this);
	}
	return alarmManager;
    }

    public SamQFSSystemAdminManager getSamQFSSystemAdminManager() {
	if (adminManager == null) {
	    adminManager = new SamQFSSystemAdminManagerImpl(this);
	}
	return adminManager;
    }


    /**
     * ****************************************************
     * Private methods
     * ****************************************************
     */

    private void getLicenseTypeFromJni() throws SamFSException {

        license = -1;
        short jniVal = License.getFSType(getJniContext());
        switch (jniVal) {
        case License.QFS :
            license = SamQFSSystemModel.QFS;
            break;
        case License.SAMFS :
            license = SamQFSSystemModel.SAMFS;
            break;
        case License.SAMQFS :
            license = SamQFSSystemModel.SAMQFS;
            break;
        }

    }


    private void processLicense() {
        if (!down) {
            try {
                getLicenseTypeFromJni();
            } catch (SamFSException se) {
                license = -1;
                processException(se);
            }
        } else {
            license = -1;
        }

        TraceUtil.trace3("license: " + license);
    }


    public void processException(Exception e) {

        try {
	    if (e instanceof SamFSCommException) {
                if (((SamFSException) e).getSAMerrno() == 30804)
                    reinitialize();
                else
                    reconnect();
            }
        } catch (Exception ex) {
        }

    }

    public boolean doesFileExist(String filePath)
        throws SamFSException {

        boolean exist = false;
        if (SamQFSUtil.isValidString(filePath))
            exist = FileUtil.fileExists(getJniContext(), filePath);
        return exist;
    }

    public boolean isAccessDenied() {

        return accessDenied;
    }

    public boolean isServerSupported() {

        return serverSupported;
    }

    public boolean isObjectBasedFSSupported() {

        return objectBasedFSSupported;
    }

    /**
     * Added in 4.6 for CIS support
     * @return Metadata Server Name of the CIS setup
     */
    public String getMetadataServerName() throws SamFSException {

        return Host.getMetadataServerName(getJniContext(), null);
    }

    /**
     * Added in 4.6
     * Make NFS without dependency of file systems
     * @return Array of NFS Options of this system
     */
    public NFSOptions[] getNFSOptions() throws SamFSException {
        NFSOptions[] opts = null;

        String[] optsStrs =
            FS.getNFSOptions(getJniContext(), "");
        if (optsStrs == null)
            return new NFSOptions[0];
        else
            opts = new NFSOptions[optsStrs.length];

        for (int i = 0; i < optsStrs.length; i++)
            opts[i] = new NFSOptions(optsStrs[i]);

        return opts;
    }

    public NFSOptions getNFSOption(String dirName) throws SamFSException {
        NFSOptions [] nfsOpts = getNFSOptions();

        // Directory name will be the key
        // Iterate through the array of NFSOptions and get a match
        for (int n = 0; n < nfsOpts.length; n++) {
            if (nfsOpts[n].getDirName().equals(dirName)) {
                return nfsOpts[n];
            }
        }

        // Not found
        throw new SamFSException("logic.nfssharenotfound");
    }

    /**
     * Added in 4.6
     * Set NFS Options in this system
     * @return void
     */
    public void setNFSOptions(NFSOptions opts) throws SamFSException {
        // always use the root directory to set NFS Options
        FS.setNFSOptions(getJniContext(), "/", opts.toString());
    }

    /**
     * @since 4.6
     * @return an array of StageFile that holds all the custom reports found in
     *  CIS_REPORT_DIR directory.
     */
    public StageFile [] getCISReportEntries() throws SamFSException {
        StageFile [] stageFile = null;
        int theseDetails =
                FileUtil.FNAME |
                FileUtil.SIZE  |
                FileUtil.CREATED;

        // Build file filter, get all entries with wildcard "*report*"
        Filter filter = new Filter();
        filter.filterOnNamePattern(true, "*report*");

        try {
            GetList details = FileUtil.listCollectFileDetails(
                getJniContext(),
                getRootDirectoryName(),
                null,
                CIS_REPORT_DIR,
                null,
                -1,
                theseDetails,
                filter.toString());

            String [] fileDetails = details.getFileDetails();
            if (fileDetails != null && fileDetails.length > 0) {
                stageFile = new StageFile[fileDetails.length];

                // construct a StageFile object for each of the file details
                for (int i = 0; i < fileDetails.length; i++) {
                    stageFile[i] =
                        fsManager.parseStageFileDetails(fileDetails[i]);
                }
            }
        } catch (SamFSException samEx) {
            // First check if the CIS specific report directory exists
            // return an empty array if the directory doesn't exist
            if (samEx.getSAMerrno() == 31212) {
                return new StageFile[0];
            }
            // Throw exception if other errors are found
            throw samEx;
        }

        return stageFile == null ? new StageFile[0] : stageFile;
    }

    private String getRootDirectoryName() throws SamFSException {
        GenericFileSystem [] genfs =
            getSamQFSSystemFSManager().getNonSAMQFileSystems();
        for (int i = 0; i < genfs.length; i++) {
            if ("/".equals(genfs[i].getMountPoint())) {
                return genfs[i].getName();
            }
        }
        return "";
    }

    /**
     * @since 5.0
     * @return boolean -  true if the server represented by this model has
     * archiving media configured otherwize false.
     */
    public boolean hasArchivingMedia() throws SamFSException {
        // a disk volume check returns faster, check for a disk volume first
        DiskVolume [] diskVolume = getSamQFSSystemMediaManager().getDiskVSNs();
        if (diskVolume != null && diskVolume.length > 0)
            return true;

        // if no disk volumes found, load configuration
        Configuration config =
            getSamQFSSystemAdminManager().getConfigurationSummary(false);

        // A tape or disk volume will do
        return (config.getTapeCount() > 0) || (config.getDiskVolumeCount() > 0);
    }

    /**
     * @since 5.0
     * @return boolean - true if the server represented by this model has
     * at least one archiving file system configured, otherwise false.
     */
    public boolean hasArchivingFileSystem() throws SamFSException {
         boolean found = false;
         FileSystem [] fs = getSamQFSSystemFSManager().getAllFileSystems();

         if (fs != null && fs.length > 0) {
             for (int i = 0; !found && i < fs.length; i++) {
                 // make the file is archiving and if shared, then its the
                 // metadata server - this is where archiving occurs.
                 found = ((fs[i].getArchivingType() == FileSystem.ARCHIVING) &&
                          ((fs[i].getShareStatus() == FileSystem.UNSHARED) ||
                      (fs[i].getShareStatus() == FileSystem.SHARED_TYPE_MDS)));
             }
         }

         return found;
    }

    /**
     * @since 5.0
     * @return boolean - true if the server is a storage node server that is not
     * meant to be managed as a full blown SAM-QFS server.
     */
    public boolean isStorageNode() {
        return storageNode;
    }
}
