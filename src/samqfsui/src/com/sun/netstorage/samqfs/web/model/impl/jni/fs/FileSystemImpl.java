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

// ident	$Id: FileSystemImpl.java,v 1.53 2008/12/16 00:12:20 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.fs;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.mgmt.fs.SamfsckJob;
import com.sun.netstorage.samqfs.mgmt.fs.StripedGrp;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.model.fs.ShrinkOption;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.impl.jni.archive.
    ArchivePolCriteriaImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.archive.ArchivePolicyImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DiskCacheImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.StripedGroupImpl;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;
import java.util.GregorianCalendar;


public class FileSystemImpl extends GenericFileSystemImpl
    implements FileSystem {


    private FSInfo fsInfo = null;
    private SamQFSSystemModelImpl model = null;
    private SamQFSAppModel app = null;

    private int fsType = -1;
    private String fsTypeName = "n/a";
    private int archType = -1;
    private int equipOrdinal = -1;

    private int shareStatus = -1;
    private String serverName = new String(); // metadata server name
    private int dauSize = -1;

    private GregorianCalendar timeAboveHWM = null;
    private GregorianCalendar dateCreated = null;

    private FileSystemMountProperties mountProps = null;

    private DiskCache[] metaDevices = new DiskCache[0];
    private DiskCache[] dataDevices = new DiskCache[0];
    private StripedGroup[] stripedGrps = new StripedGroup[0];
    private String logfile = new String();

    public FileSystemImpl() {
    }

    public FileSystemImpl(SamQFSSystemModelImpl model, FSInfo fsInfo)
    throws SamFSException {

        if ((model == null) || (fsInfo == null))
            throw new SamFSException("logic.invalidFSParam");
        this.app = SamQFSFactory.getSamQFSAppModel();
        if (app == null)
            throw new SamFSException("internal error: null application model");
        this.model = model;
        this.fsInfo = fsInfo;
        hostName = model.getServerHostname();
        setup();
    }


    public Ctx getJniContext() {
        return model.getJniContext();
    }


    public FSInfo getJniFSInfo() {
        return fsInfo;
    }
    public void setJniFSInfo(FSInfo fsInfo) {
        this.fsInfo = fsInfo;
        setup();
    }


    // get-type methods


    public int getFSTypeByProduct() {

        int type = FileSystem.FS_SAM;
        if (getFSType() == FileSystem.SEPARATE_METADATA) {
            if (getArchivingType() == FileSystem.ARCHIVING)
                type = FileSystem.FS_SAMQFS;
            else
                type = FileSystem.FS_QFS;
        }
        return type;
    }

    public String getFSTypeName() {
        return fsTypeName;
    }

    public int getFSType() {
        return fsType;
    }


    public int getArchivingType() {
        return archType;
    }


    public int getEquipOrdinal() {
        return equipOrdinal;
    }


    public int getShareStatus() {
        return shareStatus;
    }

    /**
     * introduced in 5.0 to determine if this file system is a hpc shared qfs
     * proto file system - partially configured.
     */
    public boolean isProtoFS() {
	return false;
    }

    /**
     * Determine if file system is a mb type file system.  It is a
     * Sun StorageTek QFS or Sun SAM-QFS disk cache family set with one or more
     * meta devices.  Metadata resides on these meta devices.  File data resides
     * on the object storage device(s) (OSDs).
     *
     * @since 5.0
     * @return true if file system is a "mb" type file system
     */
    public boolean isMbFS() {
        return FSInfo.OBJECT_BASED_QFS.equals(fsInfo.getEqType());
    }

    /**
     * Determine if file system is a mat type file system.  It is a
     * Sun StorEdge QFS disk cache family set with one or more meta devices.
     * Metadata resides on these meta devices.  File data resides on the data
     * device(s). This standalone file system has no namespace and is only used
     * as the OSD target backing store of an object storage device (OSD) in an
     * mb file system.
     *
     * @since 5.0
     * @return true if file system is a "mat" type file system
     */
    public boolean isMatFS() {
        return FSInfo.OBJECT_TARGET.equals(fsInfo.getEqType());
    }

    public int getDAUSize() {
        return dauSize;
    }


    public GregorianCalendar getTimeAboveHWM() {
        return timeAboveHWM;
    }
    public void setTimeAboveHWM(GregorianCalendar timeAboveHWM) {
        this.timeAboveHWM = timeAboveHWM;
    }


    public GregorianCalendar getDateCreated() {
        return dateCreated;
    }


    public String getServerName() {
        return serverName;
    }


    public FileSystemMountProperties getMountProperties() {
        return mountProps;
    }


    public DiskCache[] getMetadataDevices() {
        return metaDevices;
    }


    public DiskCache[] getDataDevices() {
        return dataDevices;
    }

    public StripedGroup[] getStripedGroups() {
        return stripedGrps;
    }

    /**
     * Strictly used only in Shrink File System Wizard
     * @return data, metadata devices, and striped groups
     */
    public DiskCache[] getAllDevices() {
        DiskCache [] allDevices = new DiskCache[
            metaDevices.length + dataDevices.length + stripedGrps.length];
        System.arraycopy(
            dataDevices, 0, allDevices, 0, dataDevices.length);
        System.arraycopy(
            metaDevices, 0, allDevices, dataDevices.length, metaDevices.length);
        DiskCache [] stripedDevices = convertStripedGroups(stripedGrps);
        System.arraycopy(
            stripedDevices, 0, allDevices,
            dataDevices.length + metaDevices.length, stripedDevices.length);
        return allDevices;
    }

    private DiskCache [] convertStripedGroups(StripedGroup [] groups) {
        DiskCache [] dcGroups = new DiskCache[groups.length];
        for (int i = 0; i < groups.length; i++) {
            dcGroups[i] = new DiskCacheImpl(groups[i]);
        }
        return dcGroups;
    }

    // overrides the definition in the parent class
    public GenericFileSystem[] getHAFSInstances()
    throws SamFSMultiHostException {
        // get cluster nodes first
        ClusterNodeInfo[] nodes = null;
        try {
            nodes = model.getClusterNodes();
        } catch (SamFSException e) {
            throw new SamFSMultiHostException(e.getMessage());
        }
        /**
         * try to get fs information from each node.
         * ignore errors from unmanaged nodes
         */
        ArrayList fsList = new ArrayList();
        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();
        SamQFSSystemModel sysm;
        SamQFSSystemFSManager fsMgr;
        String nodeName = null;
        FileSystem fs = null;
        for (int n = 0; n < nodes.length; n++) {
            // get system model
            try {
                nodeName = nodes[n].getName();
                if (nodeName == null) {
                    TraceUtil.trace1("no name for node " + nodeName);
                    continue;
                }
                sysm = app.getSamQFSSystemModel(nodeName);
            } catch (SamFSException e) {
                TraceUtil.trace1("no model found for node " + nodeName);
                continue;
            }
            // get fs information
            try {
                fsMgr = sysm.getSamQFSSystemFSManager();
                fs = fsMgr.getFileSystem(this.getName()); // null=notfound
            } catch (SamFSException e) {
                errorHostNames.add(nodeName);
                errorExceptions.add(e);
            }
            if (fs != null) {
                fsList.add(fs);
            }
        } // for

        FileSystem[] fsArray =
            (FileSystem[]) fsList.toArray(new FileSystem[0]);
        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]),
                fsArray); // result embedded inside exception
        }
        return fsArray;
    }

    // action methods

    public void changeMountOptions() throws SamFSException {

        FS.setMountOpts(getJniContext(), getName(),
            ((FileSystemMountPropertiesImpl) getMountProperties()).
            getJniMountOptions());

        if (getState() == FileSystem.MOUNTED)
            FS.setLiveMountOpts(getJniContext(), getName(),
                ((FileSystemMountPropertiesImpl) getMountProperties()).
                getJniMountOptions());

        fsInfo = FS.get(getJniContext(), getName());
        mountProps =
            new FileSystemMountPropertiesImpl(fsInfo.getMountOptions());
        mountProps.setFileSystem(this);

    }


    public void mount() throws SamFSException {
        if (fsInfo.isShared() && !fsInfo.isMdServer()) {
            // Make sure the MD server is mounted
            String serverName = fsInfo.getServerName();
            TraceUtil.trace3("Logic: get mdserver name " +
                serverName);

            // if serverName is empty, skip the check because sam-fsd is not
            // running
            if (!serverName.equals("")) {
                SamQFSSystemModel mdserver = app.getSamQFSSystemModel(
                    serverName);
                if (mdserver == null) {
                    throw new SamFSException("logic.sharedUnMountedFS.null");
                }
                FSInfo mdsinfo = FS.get(((SamQFSSystemModelImpl)mdserver).
                    getJniContext(), getName());
                if (!mdsinfo.isMounted()) {
                    throw new SamFSException("logic.sharedUnMountedFS");
                }
            }
        }
        TraceUtil.trace3("Logic: before mounting the fs ");
        FS.mount(getJniContext(), getName());
        fsInfo = FS.get(getJniContext(), getName());
        TraceUtil.trace3("Logic: after mounting the fs ");
        this.state = FileSystem.MOUNTED;

    }


    public void unmount() throws SamFSException {

        if (fsInfo.isShared() && fsInfo.isMdServer()) {
            // Make sure all clients are unmounted before
            // allowing the MD server to be unmounted.

            SamQFSSystemSharedFSManager fsm =
                app.getSamQFSSystemSharedFSManager();
            SharedMember[] members = null;
            try {
                members = fsm.getSharedMembers(model.getHostname(), getName());
            } catch (SamFSMultiHostException multiEx) {
                /**
                 * ignore MultiHost exception if the only underlying exceptions
                 * are 'filesystem not found' exceptions
                 * (unless a 'not found' exception is thrown on the MDS server)
                 */
                SamFSException exs[] = multiEx.getExceptions();
                String hosts[] = multiEx.getHostNames();
                members = (SharedMember[]) multiEx.getPartialResult();
                if (members != null) {
                    for (int i = 0; i < hosts.length; i++) {
                        if (exs[i].getSAMerrno() != SamFSException.NOT_FOUND ||
                            hosts[i].equals(fsInfo.getServerName())) { // MDS
                            throw multiEx;
                        } // else ignore the exception
                    }
                }
            }

            if (members == null) {
                throw new SamFSException("logic.sharedMountedFS.null");
            }

            for (int i = 0; i < members.length; i++) {
                if (members[i].getType() != SharedMember.TYPE_MD_SERVER) {
                    if (members[i].isMounted()) {
                        throw new SamFSException("logic.sharedMountedClient");
                    }
                }
            }
        }

        FS.umount(getJniContext(), getName());
        fsInfo = FS.get(getJniContext(), getName());
        this.state = FileSystem.UNMOUNTED;

    }


    public void stopFSArchive() throws SamFSException {

        Archiver.stopForFS(getJniContext(), getName());

    }


    public void idleFSArchive() throws SamFSException {

        Archiver.idleForFS(getJniContext(), getName());

    }


    public void runFSArchive() throws SamFSException {

        Archiver.runForFS(getJniContext(), getName());

    }


    public void grow(DiskCache[] metadata, DiskCache[] data,
        StripedGroup[] groups) throws SamFSException {

        DiskDev[] diskDevs = new DiskDev[0];
        DiskDev[] metaDevs = new DiskDev[0];
        StripedGrp[] grps = new StripedGrp[0];

        if ((data != null) && (data.length > 0)) {
            diskDevs = new DiskDev[data.length];
System.out.println("data length: " + data.length);
            for (int i = 0; i < data.length; i++) {
System.out.print("i: " + i + " getting jni disk!");
                diskDevs[i] = ((DiskCacheImpl) data[i]).getJniDisk();
System.out.println(" done");
            }
        }

        if ((metadata != null) && (metadata.length > 0)) {
            metaDevs = new DiskDev[metadata.length];
            for (int i = 0; i < metadata.length; i++)
                metaDevs[i] = ((DiskCacheImpl) metadata[i]).getJniDisk();
        }

        if ((groups != null) && (groups.length > 0)) {
            grps = new StripedGrp[groups.length];
            for (int i = 0; i < groups.length; i++)
                grps[i] = ((StripedGroupImpl) groups[i]).getJniStripedGroup();
        }

        FS.grow(getJniContext(), getJniFSInfo(), metaDevs, diskDevs, grps);
        fsInfo = FS.get(getJniContext(), getName());
        setup();

    }


    public long samfsck(boolean checkAndRepair, String logFilePath)
    throws SamFSException {

        FS.fsck(getJniContext(), getName(), logFilePath, checkAndRepair);

        long jobId = -1;
        SamfsckJob[] jniJobs = SamfsckJob.getAll(getJniContext());
        if ((jniJobs != null) && (jniJobs.length > 0)) {
            jobId = jniJobs[0].getID();
        }

        return jobId;

    }


    public String getFsckLogfileLocation() {
        return logfile;
    }
    public void setFsckLogfileLocation(String logfile) {
        this.logfile = logfile;
    }


    public String toString() {

        long capacity = 0;
        long availSpace = 0;
        int consumedSpace = 0;

        try {
            capacity = getCapacity();
            availSpace = getAvailableSpace();
            consumedSpace = getConsumedSpacePercentage();
        } catch (Exception e) {
            e.printStackTrace();
        }

        StringBuffer buf = new StringBuffer();

        buf.append("Name: " + name + "\n");
        buf.append("FS Type: " + fsType + ", HA:" + ha +"\n");
        buf.append("Archiving Type: " + archType + "\n");
        buf.append("Equip Ordinal: " + equipOrdinal + "\n");
        buf.append("Mount Point: " + mountPoint + "\n");
        buf.append("State: " + state + "\n");
        buf.append("Share Status: " + shareStatus + "\n");
        buf.append("NFSShared: " + nfsShared + "\n");
        buf.append("Capacity: " + capacity + "\n");
        buf.append("Available Space: " + availSpace + "\n");
        buf.append("Consumed Space: " + consumedSpace + "% \n");
        buf.append("DAU Size: " + dauSize + "\n");
        buf.append("Time Above HWM: " + SamQFSUtil.dateTime(timeAboveHWM)
        + "\n");
        buf.append("Date Created: " + SamQFSUtil.dateTime(dateCreated) + "\n");
        buf.append("Server Name: " + serverName + "\n");
        buf.append("Mount Properties: \n" + mountProps.toString() + "\n");

        try {

            if (metaDevices != null) {
                for (int i = 0; i < metaDevices.length; i++)
                    buf.append("Metadata Devices: \n" +
                        metaDevices[i].toString() + "\n\n");
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

        try {

            if (dataDevices != null) {
                for (int i = 0; i < dataDevices.length; i++)
                    buf.append("Data Devices: \n" +
                        dataDevices[i].toString() + "\n\n");
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

        try {

            if (stripedGrps != null) {
                for (int i = 0; i < stripedGrps.length; i++)
                    buf.append("Striped Groups: \n" +
                        stripedGrps[i].toString() + "\n\n");
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

        return buf.toString();

    }



    // This method includes all the private fields that are retrieved and filled
    // in with information from FSInfo
    private void setup() {

        name = fsInfo.getName();

        if (fsInfo.isArchiving())
            archType = FileSystem.ARCHIVING;
        else
            archType = FileSystem.NONARCHIVING;

        equipOrdinal = fsInfo.getEqu();

        mountPoint = fsInfo.getMountPoint();

        if (fsInfo.isMounted())
            state = FileSystem.MOUNTED;
        else
            state = FileSystem.UNMOUNTED;

        if (fsInfo.isShared()) {
            if (fsInfo.isClient())
                shareStatus = FileSystem.SHARED_TYPE_CLIENT;
            else if (fsInfo.isMdServer())
                shareStatus = FileSystem.SHARED_TYPE_MDS;
            else if (fsInfo.isPotentialMdServer())
                shareStatus = FileSystem.SHARED_TYPE_PMDS;
            else shareStatus = FileSystem.SHARED_TYPE_UNKNOWN;
            serverName = fsInfo.getServerName();
        } else {
            shareStatus = FileSystem.UNSHARED;
            serverName = new String();
        }
        capacity = fsInfo.getCapacity();
        avail = fsInfo.getAvailableSpace();

        consumed = capacity != 0 ?
            (int)((capacity - avail) * 100 / capacity) : -1;

        if (FSInfo.NFS_SHARED.equals(fsInfo.getNFSShareState()))
            nfsShared = true;

        dauSize = fsInfo.getDAUSize();

        dateCreated = SamQFSUtil.convertTime(fsInfo.getCreationTime());

        mountProps =
            new FileSystemMountPropertiesImpl(fsInfo.getMountOptions());
        mountProps.setFileSystem(this);

        DiskDev[] list = null;
        this.setHA(true); // will stay true if all devs are ha

        list = fsInfo.getMetadataDevices();
        if (list != null) {
            metaDevices = new DiskCache[list.length];
            for (int i = 0; i < list.length; i++) {
                metaDevices[i] = new DiskCacheImpl(list[i]);
                ((DiskCacheImpl) metaDevices[i]).
                    setDiskCacheType(DiskCache.METADATA);
                if (ha && !metaDevices[i].getDevicePath().equals("nodev"))
                    this.setHA(metaDevices[i].isHA());
            }
        }

        list = fsInfo.getDataDevices();
        if (list != null) {
            dataDevices = new DiskCache[list.length];
            for (int i = 0; i < list.length; i++) {
                dataDevices[i] = new DiskCacheImpl(list[i]);
                if (ha)
                    this.setHA(dataDevices[i].isHA());
            }
        }

        StripedGrp[] jniList = fsInfo.getStripedGroups();
        if (jniList != null) {
            stripedGrps = new StripedGroup[jniList.length];
            for (int i = 0; i < jniList.length; i++)
                stripedGrps[i] = new StripedGroupImpl(jniList[i]);
        }

        if ((metaDevices != null) && (metaDevices.length > 0))
            fsType = FileSystem.SEPARATE_METADATA;
        else
            fsType = FileSystem.COMBINED_METADATA;

    }

    public ArchivePolCriteria[] getArchivePolCriteriaForFS()
    throws SamFSException {

        TraceUtil.trace3("Logic: Enter getArchivePolCriteriaForFS()");

        ArrayList list = new ArrayList();

        if (model != null) {
            // Refresh policy information
            model.getSamQFSSystemArchiveManager().getAllArchivePolicies();

            // get the FSDirective for the file system and get the policy names
            ArFSDirective fsDir =
                Archiver.getArFSDirective(getJniContext(), getName());

            if (fsDir != null) {

                Criteria[] c = fsDir.getCriteria();
                if ((c != null) && (c.length > 0)) {

                    for (int i = 0; i < c.length; i++) {

                        String policyName = c[i].getSetName();

                        if (SamQFSUtil.isValidString(policyName)) {

                            ArchivePolicyImpl pol = (ArchivePolicyImpl)
                            model.getSamQFSSystemArchiveManager().
                                getArchivePolicy(policyName);

                            if (pol != null) {
                                ArchivePolCriteria[] polCrits =
                                    pol.getArchivePolCriteriaForFS(getName());
                                if (polCrits != null) {
                                    for (int j = 0; j < polCrits.length; j++) {
                                        ArchivePolCriteriaImpl crit =
                                            (ArchivePolCriteriaImpl)
                                            polCrits[j];
                                        if (crit.isFSPresent(getName())) {
                                            // this is necessary
                                            // to maintain the ordering for GUI
                                            // for same policy appearing
                                            // multiple times within one fs
                                            Criteria[] cList =
                                                crit.getJniCriteria();
                                            boolean found = false;
                                            if (cList != null) {
                                                for (int k = 0;
                                                k < cList.length;
                                                k++) {
                                                    if (c[i].sameAs(cList[k])) {
                                                        found = true;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (found) {
                                                list.add(crit);
                                            }
                                        }
                                    }
                                }
                            }

                        }

                    }
                }
            }
        }

        TraceUtil.trace3("Logic: Exit getArchivePolCriteriaForFS()");

        return (ArchivePolCriteria[]) list.toArray(new ArchivePolCriteria[0]);

    }


    public void addPolCriteria(ArchivePolCriteria[] newPolCriteriaList)
    throws SamFSException {

        TraceUtil.trace3("Logic: Enter addPolCriteria()");

        if (newPolCriteriaList != null) {
            for (int i = 0; i < newPolCriteriaList.length; i++) {
                ((ArchivePolCriteriaImpl) newPolCriteriaList[i]).
                    addFileSystemForCriteria(getName(), false);
            }
            Archiver.activateCfgThrowWarnings(model.getJniContext());
            model.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        }

        TraceUtil.trace3("Logic: Exit addPolCriteria()");

    }


    public void removePolCriteria(ArchivePolCriteria[] newPolCriteriaList)
    throws SamFSException {

        TraceUtil.trace3("Logic: Enter removePolCriteria()");

        if (newPolCriteriaList != null) {
            for (int i = 0; i < newPolCriteriaList.length; i++) {
                ((ArchivePolCriteriaImpl) newPolCriteriaList[i]).
                    deleteFileSystemForCriteria(getName(), false);
            }
            Archiver.activateCfgThrowWarnings(model.getJniContext());
            model.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        }

        TraceUtil.trace3("Logic: Exit removePolCriteria()");

    }


    public void reorderPolCriteria(ArchivePolCriteria[] reorderedList)
    throws SamFSException {

        TraceUtil.trace3("Logic: Enter reorderPolCriteria()");

        ArFSDirective fsDir =
            Archiver.getArFSDirective(getJniContext(), getName());

        if (reorderedList != null) {
            Criteria[] crit = new Criteria[reorderedList.length];
            for (int i = 0; i < reorderedList.length; i++) {
                Criteria[] crits =
                    ((ArchivePolCriteriaImpl)
                    reorderedList[i]).getJniCriteria();
                crit[i] = new Criteria(getName(), crits[0]);
            }
            fsDir.setCriteria(crit);

            Ctx ctx = getJniContext();
            Archiver.setArFSDirective(ctx, fsDir);

            Archiver.activateCfgThrowWarnings(model.getJniContext());
            model.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        }

        TraceUtil.trace3("Logic: Exit reorderPolCriteria()");

    }

    /**
     * Retrieve the status flag of a file system.
     * This is added for the Monitoring Console.  Presentation layer is
     * responsible to decode the flag to avoid adding tens of APIs in the logic
     * layer just for one consumer, i.e. Monitoring Console.
     *
     * Definitions of the flags are as follow: (copy over from FSInfo.java)
     *
     * public static final int FS_MOUNTED  = 0x00000001;
     * public static final int FS_MOUNTING = 0x00000002;
     * public static final int FS_UMOUNT_IN_PROGRESS = 0x00000004;
     *
     * // host is now metadata server
     * public static final int FS_SERVER = 0x00000010;
     *
     * // host is not metadata server
     * public static final int FS_CLIENT = 0x00000020;
     *
     * // host can't be metadata server
     * public static final int FS_NODEVS = 0x00000040;
     *
     * // metadata server running SAM
     * public static final int FS_SAM = 0x00000080;
     *
     * // lock write operations
     * public static final int FS_LOCK_WRITE = 0x00000100;
     *
     * //lock name operations
     * public static final int FS_LOCK_NAME = 0x00000200;
     *
     * // lock remove name operations
     * public static final int FS_LOCK_RM_NAME = 0x00000400;
     *
     * // lock all operations
     * public static final int FS_LOCK_HARD = 0x00000800;
     *
     * // host is failing over
     * public static final int FS_FREEZING = 0x01000000;
     *
     * // host is frozen
     * public static final int FS_FROZEN = 0x02000000;
     *
     * // host is thawing
     * public static final int FS_THAWING = 0x04000000;
     *
     * // server is resyncing
     * public static final int FS_RESYNCING = 0x08000000;
     *
     * // releasing is active on fs
     * public static final int FS_RELEASING = 0x20000000;
     *
     * // staging is active on fs
     * public static final int FS_STAGING = 0x40000000;
     *
     * // archiving is active on fs
     * public static final int FS_ARCHIVING = 0x80000000;
     *
     * @return status flag of the file system
     */
    public int getStatusFlag() {

        return fsInfo.getStatusFlags();
    }

    /**
     * Set a new device state for all the devices with eqs that match one of
     * the eq in the array of integer
     *
     * @param newState
     * @param eqs
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    public void setDeviceState(int newState, int [] eqs) throws SamFSException {

        FS.setDeviceState(
            getJniContext(),
            fsInfo.getName(),
            SamQFSUtil.convertStateToJni(newState),
            eqs);
    }

    // Added 5.0
    /*
     * Method to remove a device from a file system by releasing all of
     * the data on the device.
     *
     * options is a string of key value pairs that are based on the
     * directives in shrink.cmd. If options is non-null the options
     * will be set for this file system in the shrink.cmd file
     * prior to invoking the shrink.
     *
     * Options Keys:
     *   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 stage_files = TRUE | FALSE (default FALSE)
     *	 stage_partial = TRUE | FALSE (default FALSE)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkRelease(int eqToRelease, ShrinkOption options)
        throws SamFSException {

        return FS.shrinkRelease(
                    getJniContext(), fsInfo.getName(),
                    eqToRelease, options.toString());
    }

    /*
     * Method to remove a device from a file system by copying the
     * data to other devices. If replacementEq is the eq of a device
     * in the file system the data will be copied to that device.
     * If replacementEq is -1 the data will be copied to available devices
     * in the FS.
     *
     * Options Keys:
     *	 logfile = filename (default no logging)
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkRemove(
        int eqToRemove, int replacementEq, ShrinkOption options)
	throws SamFSException {

        return FS.shrinkRemove(
                    getJniContext(), fsInfo.getName(),
                    eqToRemove, replacementEq, options.toString());
    }

    /*
     * Method to remove a device from a file system by copying the
     * data to a newly added device.
     *
     * Options Keys:
     *	 logfile = filename (default no logging)
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkReplaceDev(
        int eqToRemove, DiskCache replacement, ShrinkOption options)
	throws SamFSException {

        return FS.shrinkReplaceDev(
                    getJniContext(), fsInfo.getName(),
                    eqToRemove,
                    ((DiskCacheImpl) replacement).getJniDisk(),
                    options.toString());
    }

    /*
     * Method to remove a striped group from a file system by copying the
     * data to a new striped group.
     *
     * Options Keys:
     *	 logfile = filename (default no logging)
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkReplaceGroup(
        int eqToRemove, StripedGroup replacement, ShrinkOption options)
	throws SamFSException {

        return FS.shrinkReplaceGroup(
                    getJniContext(), fsInfo.getName(),
                    eqToRemove,
                    ((StripedGroupImpl) replacement).getJniStripedGroup(),
                    options.toString());
    }
}
