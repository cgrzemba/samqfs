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

// ident	$Id: SamQFSSystemFSManagerImpl.java,v 1.85 2009/01/29 15:50:19 ronaldso Exp $



package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.GetList;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.FSArchCfg;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.mgmt.fs.MountOptions;
import com.sun.netstorage.samqfs.mgmt.fs.Restore;
import com.sun.netstorage.samqfs.mgmt.fs.StripedGrp;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.web.model.DirInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileCopyDetails;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericMountOptions;
import com.sun.netstorage.samqfs.web.model.fs.Metric;
import com.sun.netstorage.samqfs.web.model.fs.RemoteFile;
import com.sun.netstorage.samqfs.web.model.fs.RestoreDumpFile;
import com.sun.netstorage.samqfs.web.model.fs.RestoreFile;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.model.impl.jni.archive.ArchivePolicyImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.FileSystemImpl;
import com.sun.netstorage.samqfs.web.model.
                                    impl.jni.fs.FileSystemMountPropertiesImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.GenericFileSystemImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.RestoreFileImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DiskCacheImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.StripedGroupImpl;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.Filter;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Properties;

public class SamQFSSystemFSManagerImpl extends MultiHostUtil
    implements SamQFSSystemFSManager {

    private SamQFSSystemModelImpl theModel;
    private Hashtable fsCache;


    public SamQFSSystemFSManagerImpl(SamQFSSystemModel model) {
	theModel = (SamQFSSystemModelImpl) model;
        fsCache = new Hashtable();
    }

    // filesystem related methods


    // This method returns all sam/qfs names
    public String[] getAllFileSystemNames() throws SamFSException {
        String[] fsNames = FS.getNames(theModel.getJniContext());

        if (fsNames == null) {
            fsNames = new String[0];
        }

        return fsNames;
    }

    // This method returns all fs names found in server
    // Currently it only include vfstab fs names.
    public String[] getFileSystemNamesAllTypes() throws SamFSException {
        String[] fsNames = FS.getNamesAllTypes(theModel.getJniContext());

        if (fsNames == null) {
            fsNames = new String[0];
        }

        return fsNames;
    }

    public FileSystem[] getAllFileSystems() throws SamFSException {
        FileSystem[] fs = null;

        FSInfo[] fsInfo = FS.getAll(theModel.getJniContext());

        if (fsInfo == null) {
            fs = new FileSystem[0];
        } else {
            fs = new FileSystem[fsInfo.length];
            for (int i = 0; i < fsInfo.length; i++) {
                fs[i] = new FileSystemImpl(theModel, fsInfo[i]);
            }
        }

        return fs;
    }

    public FileSystem[] getAllFileSystems(int archivingType)
        throws SamFSException {
        ArrayList fsList = new ArrayList();
        FileSystem[] fsAll = getAllFileSystems();

        if ((fsAll != null) && (fsAll.length > 0)) {
            for (int i = 0; i < fsAll.length; i++) {
                if (fsAll[i].getArchivingType() == archivingType)
                    fsList.add(fsAll[i]);
            }
        }

        return (FileSystem[]) fsList.toArray(new FileSystem[0]);
    }


    public GenericFileSystem[] getNonSAMQFileSystems()
        throws SamFSException {
        GenericFileSystemImpl[] fsArr = null;
        String[] fsStrs =
            FS.getGenericFilesystems(theModel.getJniContext(), "ufs,vxfs,zfs");

        if (fsStrs == null)
            return new GenericFileSystem[0];
        else
            fsArr = new GenericFileSystemImpl[fsStrs.length];
        for (int i = 0; i < fsStrs.length; i++) {
            fsArr[i] = new GenericFileSystemImpl(theModel.getHostname(),
                                                 fsStrs[i]);
                fsCache.put(fsArr[i].getName(), fsArr[i]);
        }
        return fsArr;
    }

    /**
     * @since 4.4
     * retrieve filesystem information for the specified filesystem
     */
    public GenericFileSystem getGenericFileSystem(String fsName)
        throws SamFSException {
        GenericFileSystem fs = null;

        if (fsName == null) {
            return fs;
        }

        if (!fsName.startsWith("/")) {
            // SAM-FS/QFS
            FSInfo fsInfo = null;
            try {
                fsInfo = FS.get(theModel.getJniContext(), fsName);
            } catch (SamFSException e) {
                if (e.getSAMerrno() != SamFSException.NOT_FOUND)
                    throw e;
            }
            if (fsInfo != null)
                fs = new FileSystemImpl(theModel, fsInfo);
        }

        if (fs == null) {
            // ufs/vxfs/zfs
            if (fsCache.get(fsName) == null)
                // repopulate the cache
                getNonSAMQFileSystems();
            fs = (GenericFileSystem) fsCache.get(fsName);
        }
        return fs;
    }

    /**
     * @since 4.4
     */
    public void invalidateCachedFS(String fsName) {
        fsCache.remove(fsName);
    }

    /**
     * retrieves the named filesystem
     */
    public FileSystem getFileSystem(String fsName) throws SamFSException {
        return (FileSystem) getGenericFileSystem(fsName);
    }


    public FileSystemMountProperties getDefaultMountProperties(int fsType,
                        int archType, int dauSize, boolean stripedGrp,
                        int shareStatus, boolean multiReader)
        throws SamFSException {

        String type = FSInfo.COMBINED_METADATA;
        if (fsType == FileSystem.SEPARATE_METADATA)
            type = FSInfo.SEPARATE_METADATA;

        boolean shared = true;
        if (shareStatus == FileSystem.UNSHARED)
            shared = false;

        MountOptions mount = FS.getDefaultMountOpts(theModel.getJniContext(),
						    type, dauSize,
                                                    stripedGrp, shared,
                                                    multiReader);

        return new FileSystemMountPropertiesImpl(mount);
    }

    public FileSystem createFileSystem(String fsName, int fsType, int archType,
                                       int equipOrdinal, String mountPoint,
                                       int shareStatus, int DAUSize,
                                       FileSystemMountProperties mountProps,
                                       DiskCache[] metadataDevices,
                                       DiskCache[] dataDevices,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate)
        throws SamFSMultiStepOpException, SamFSException {

        FileSystem fs =
            createFileSystem(fsName, fsType, archType, equipOrdinal,
                             mountPoint,
                             shareStatus, DAUSize, mountProps,
                             metadataDevices, dataDevices, null, false,
                             mountAtBoot, createMountPoint, mountAfterCreate);

        return fs;
    }

    public FileSystem createFileSystem(String fsName, int fsType, int archType,
                                       int equipOrdinal, String mountPoint,
                                       int shareStatus, int DAUSize,
                                       FileSystemMountProperties mountProps,
                                       DiskCache[] metadataDevices,
                                       DiskCache[] dataDevices,
                                       StripedGroup[] stripedGroups,
                                       boolean single,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate)
        throws SamFSMultiStepOpException, SamFSException {
        return createFileSystem(fsName,
                                fsType,
                                archType,
                                equipOrdinal,
                                mountPoint,
                                shareStatus,
                                DAUSize,
                                mountProps,
                                metadataDevices,
                                dataDevices,
                                stripedGroups,
                                single,
                                mountAtBoot,
                                createMountPoint,
                                mountAfterCreate,
                                null,
                                false);
    }

    public FileSystem createFileSystem(String fsName, int fsType, int archType,
                                       int equipOrdinal, String mountPoint,
                                       int shareStatus, int DAUSize,
                                       FileSystemMountProperties mountProps,
                                       DiskCache[] metadataDevices,
                                       DiskCache[] dataDevices,
                                       StripedGroup[] stripedGroups,
                                       boolean single,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate,
                                       FSArchCfg archiveConfig,
                                       boolean isMATFS)
        throws SamFSMultiStepOpException, SamFSException {

        if ((!SamQFSUtil.isValidString(fsName)) ||
            (!SamQFSUtil.isValidString(mountPoint)))
            throw new SamFSException("logic.invalidFSParam");

        int equ = equipOrdinal;
        if (equ == -1)
            equ = FSInfo.EQU_AUTO;

        String type = FSInfo.COMBINED_METADATA;
        if (fsType == FileSystem.SEPARATE_METADATA)
            type = FSInfo.SEPARATE_METADATA;

        DiskDev[] meta = null;
        DiskDev[] data = null;
        StripedGrp[] grp = null;

        String eqTypeForJni = "md";
        if ((single) && (metadataDevices != null) &&
            (metadataDevices.length > 0))
            eqTypeForJni = "mr";

        if (metadataDevices != null) {
            meta = new DiskDev[metadataDevices.length];
            for (int i = 0; i < metadataDevices.length; i++) {
                meta[i] = ((DiskCacheImpl) metadataDevices[i]).getJniDisk();
            }
        }

        if (dataDevices != null) {
            data = new DiskDev[dataDevices.length];
            for (int i = 0; i < dataDevices.length; i++) {
                data[i] = ((DiskCacheImpl) dataDevices[i]).getJniDisk();
                data[i].setEquipType(eqTypeForJni);
            }
        }

        if (stripedGroups != null) {
            grp = new StripedGrp[stripedGroups.length];
            for (int i = 0; i < stripedGroups.length; i++) {
                grp[i] = ((StripedGroupImpl) stripedGroups[i]).
                    getJniStripedGroup();
            }
        }

        MountOptions opt = ((FileSystemMountPropertiesImpl) mountProps).
            getJniMountOptions();

        // create a mat fs
        if (isMATFS) type = FSInfo.OBJECT_TARGET;

        FSInfo fsInfo = new FSInfo(fsName, equ, DAUSize, type, meta, data,
                                   grp, opt, mountPoint);

        if (archiveConfig == null) {
            FS.createAndMount(theModel.getJniContext(),
                              fsInfo,
                              mountAtBoot,
                              createMountPoint,
                              mountAfterCreate);
        } else {
            FS.createAndMount(theModel.getJniContext(),
                              fsInfo,
                              mountAtBoot,
                              createMountPoint,
                              mountAfterCreate,
                              archiveConfig);
        }

        FileSystem fs = new FileSystemImpl(theModel, fsInfo);
        return fs;
    }

    public FileSystem createHAFileSystem(String[] hostnames,
                                       String fsName, int fsType,
                                       int equipOrdinal, String mountPoint,
                                       int DAUSize,
                                       FileSystemMountProperties mountProps,
                                       DiskCache[] metadataDevices,
                                       DiskCache[] dataDevices,
                                       StripedGroup[] stripedGroups,
                                       boolean singleDAU,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate)
        throws SamFSMultiHostException  {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        // check arguments

        if ((!SamQFSUtil.isValidString(fsName)) ||
            (!SamQFSUtil.isValidString(mountPoint)))
            throw new SamFSMultiHostException("logic.invalidFSParam");

        if (metadataDevices == null || metadataDevices.length == 0) {
            throw new SamFSMultiHostException("logic.invalidFSParam");
        }

        if ((dataDevices == null || dataDevices.length == 0) &&
            (stripedGroups == null || stripedGroups.length == 0)) {
            throw new SamFSMultiHostException("logic.invalidFSParam");
        }

        // populate FSInfo object

        int equ = equipOrdinal;
        if (equ == -1)
            equ = FSInfo.EQU_AUTO;

        String type = FSInfo.COMBINED_METADATA;
        if (fsType == FileSystem.SEPARATE_METADATA)
            type = FSInfo.SEPARATE_METADATA;

        DiskDev[] meta = null;
        DiskDev[] data = null;
        StripedGrp[] grp = null;

        String eqTypeForJni = "md";
        if ((singleDAU) && (metadataDevices != null) &&
            (metadataDevices.length > 0))
            eqTypeForJni = "mr";

        if (metadataDevices != null) {
            meta = new DiskDev[metadataDevices.length];
            for (int i = 0; i < metadataDevices.length; i++) {
                meta[i] = ((DiskCacheImpl) metadataDevices[i]).getJniDisk();
            }
        }

        if (dataDevices != null) {
            data = new DiskDev[dataDevices.length];
            for (int i = 0; i < dataDevices.length; i++) {
                data[i] = ((DiskCacheImpl) dataDevices[i]).getJniDisk();
                data[i].setEquipType(eqTypeForJni);
            }
        }

        if (stripedGroups != null) {
            grp = new StripedGrp[stripedGroups.length];
            for (int i = 0; i < stripedGroups.length; i++) {
                grp[i] = ((StripedGroupImpl) stripedGroups[i]).
                    getJniStripedGroup();
            }
        }

        MountOptions opt = ((FileSystemMountPropertiesImpl) mountProps).
            getJniMountOptions();
        opt.setSynchronizedMetadata((short)1);

	FSInfo fsInfo = new FSInfo(fsName, equ, DAUSize,
            FSInfo.SEPARATE_METADATA, meta, data, grp, opt, mountPoint);

        // get sysmodels and create fs on each host in hostnames[]

        SamQFSSystemModelImpl[] models = getSystemModels(hostnames);
        FileSystem fs = null;

        try {
            fs = new FileSystemImpl(models[0], fsInfo);
        } catch (SamFSException e) {
            throw new SamFSMultiHostException(e.getMessage());
        }

        for (int i = 0; i < models.length; i++) {
            try {
                FS.createAndMount(models[i].getJniContext(), fsInfo,
                    false, // always set mountAtBoot to false
                    createMountPoint, mountAfterCreate);

                if (i == 0) {
                    fsInfo.doNotMkfs(); // don't mkfs on the remaining nodes
                    mountAfterCreate = false; // don't attempt any more mounts
                }
            } catch (SamFSException e) {
                errorHostNames.add(hostnames[i]);
                errorExceptions.add(e);
                if (i == 0)
                    break; // if cannot mkfs, then abandon fs creation
            }
        }

        if (!errorHostNames.isEmpty()) {
           throw new SamFSMultiHostException(
               "logic.sharedFSOperationPartialFailure",
               (SamFSException[])
               errorExceptions.toArray(new SamFSException[0]),
               (String[]) errorHostNames.toArray(new String[0]),
               fs);
        }
        return fs;
    }

    public void addHostToHAFS(FileSystem fs, String host)
        throws SamFSMultiStepOpException, SamFSException {

        SamQFSSystemModelImpl newHostModel =
            (SamQFSSystemModelImpl)getApp().getSamQFSSystemModel(host);
        FSInfo fsInfo = null;
        MountOptions opts = null;

        // check if eqs are already used on the host about to be added
        try {
            fsInfo = ((FileSystemImpl)fs).getJniFSInfo();
            verifyEQsAreAvailOnNewHost(fsInfo, newHostModel);
        }
        catch (SamFSException e) {
             if (e.getSAMerrno() == SamFSException.EQU_ORD_IN_USE)
                 e = new SamFSException("logic.sharedFSEQInUse",
                     e.getSAMerrno());
             throw e; // rethrow exception
        }

        // go ahead and add fs to the new host
        opts = fsInfo.getMountOptions();
        opts.setArchive(opts.isArchive()); // will set the chg.flag
        opts.setSynchronizedMetadata((short)1);

        fsInfo.doNotMkfs(); // don't mkfs on the new host

        FS.createAndMount(newHostModel.getJniContext(), fsInfo,
            false, // always set mountAtBoot to false
            true,  // create mount point
            false); // do not attempt to mount
    }

    public void removeHostFromHAFS(FileSystem fs, String host)
        throws SamFSException {
        FS.remove(((SamQFSSystemModelImpl)getApp().getSamQFSSystemModel(host)).
            getJniContext(), fs.getName());
    }

    public void createDirectory(String fullPath) throws SamFSException {
        if (SamQFSUtil.isValidString(fullPath)) {
            FileUtil.createDir(theModel.getJniContext(), fullPath);
        }
    }

    public DiskCache[] discoverAUs(boolean availOnly, String[] hosts,
        boolean forShared) // if true, the AU-s will be used for sharedfs
        throws SamFSException {

        AU[] auList;
        SamFSConnection connection = theModel.getJniConnection();
        TraceUtil.trace2("will launch discoverAUs(" + availOnly + ", " +
            ((hosts == null) ? "null)" :
            ("hosts" + SamQFSUtil.arr2Str(hosts))) + ")");

        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_MAX_TIME_OUT);
        }

        Ctx ctx = theModel.getJniContext();

        if (hosts != null) // look for ha devs
	    auList = AU.discoverHAAUs(ctx, hosts, availOnly);
        else
            auList = availOnly ? AU.discoverAvailAUs(ctx)
                     : AU.discoverAUs(ctx);

        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_TIME_OUT);
        }

        DiskCache[] disks = null;
        if (auList == null) {
            disks = new DiskCache[0];
        } else {
            disks = new DiskCache[auList.length];
            DiskDev ddev;
            for (int i = 0; i < auList.length; i++) {
                ddev = new DiskDev(auList[i]);
                if (hosts != null /* discovery for ha */ && !forShared)
                    ddev.convertPathToGlobal(); // did->global
                disks[i] = new DiskCacheImpl(ddev);
            }
        }

        return disks;
    }

    public DiskCache[] discoverAvailableAllocatableUnits(String[] haHosts)
        throws SamFSException {

        SamQFSUtil.doPrint("Debug: discoverAvailableAllocatableUnits()");

	return discoverAUs(true, haHosts, false); // available ones only
    }

    public StripedGroup createStripedGroup(String name, DiskCache[] disks)
        throws SamFSException {

        if (!SamQFSUtil.isValidString(name)) {
            name = new String();
        }

        return new StripedGroupImpl(name, disks);

    }

    public String[] checkSlicesForOverlaps(String[] slices)
        throws SamFSException {
        String[] list = new String[0];

        if (!theModel.getServerAPIVersion().equals("1.0")) {
            list = AU.checkSlicesForOverlaps(theModel.getJniContext(),
                                             slices);
        }

        return list;
    }

    public ArchivePolCriteria[] getAllAvailablePolCriteria(FileSystem fs)
        throws SamFSException {

        ArrayList available = new ArrayList();

        if ((fs != null) && (fs.getArchivingType() == FileSystem.ARCHIVING)) {
            ArchivePolicy[] policies =
                theModel.getSamQFSSystemArchiveManager().
                getAllArchivePolicies();
            ArrayList tempList = new ArrayList();

            if (policies != null) {
                for (int i = 0; i < policies.length; i++) {
                    if (policies[i].getPolicyType() ==
                            ArSet.AR_SET_TYPE_GENERAL ||
                        policies[i].getPolicyType() ==
                            ArSet.AR_SET_TYPE_NO_ARCHIVE) {

                        ArchivePolCriteria[] polCrits =
                            ((ArchivePolicyImpl) policies[i]).
                            getArchivePolCriteriaForFS(fs.getName());

                        tempList.clear();
                        for (int j = 0; j < polCrits.length; j++) {
                            tempList.add(polCrits[j]);
                        }

                       ArchivePolCriteria[] polTotCrits =
                           policies[i].getArchivePolCriteria();

                       for (int j = 0; j < polTotCrits.length; j++) {
                           int index = tempList.indexOf(polTotCrits[j]);
                           if (index == -1) {
                               available.add(polTotCrits[j]);
                           }
                       }
                    }
                }
            }
        }

        return (ArchivePolCriteria[])
            available.toArray(new ArchivePolCriteria[0]);

    }

    public void deleteFileSystem(GenericFileSystem gfs) throws SamFSException {
        if (gfs == null)
            throw new SamFSException("logic.invalidFS");
        else if (gfs.getState() == FileSystem.MOUNTED)
            throw new SamFSException("logic.mountedFS");

        // check if non SAM-FS/QFS filesystem
        if (gfs.getFSTypeByProduct() == GenericFileSystem.FS_NONSAMQ) {
            FS.removeGenericFS(theModel.getJniContext(),
                               gfs.getName(),
                               gfs.getFSTypeName());
            return;
        }
        // SAM-FS/QFS system, therefore a cast is needed
        FileSystem fs = (FileSystem) gfs;

        if ((theModel.getLicenseType() == SamQFSSystemModel.SAMFS ||
             theModel.getLicenseType() == SamQFSSystemModel.SAMQFS) &&
             fs.getArchivingType() == FileSystem.ARCHIVING) {
            /*
             * Check to see if it is legal to remove this fs's
             * criteria. You can remove criteria if this file system
             * is not the last member of them. Otherwise you must remove
             * the criteria first (logic layer imposition). However
             * you can remove the default policy criteria since this is
             * an fs removal.
             */
            ArchivePolCriteria [] crit = fs.getArchivePolCriteriaForFS();
            if (crit != null) {
                for (int i = 0; i < crit.length; i++) {
		    if (crit[i].isLastFSForCriteria(fs.getName()) &&
			!crit[i].isForDefaultPolicy()) {
			throw new SamFSException("logic.lastFSForCriteria");
		    }
                }
            }

            /*
             * Resetting the ArFSDirective will clean up the traces of
             * any archive policies that were applied to the the file
             * system. This potentially includes removing copy params,
             * and vsn maps if the removal of any criteria results
             * in the deletion of a policy.
             */
            ArFSDirective ars = Archiver.getArFSDirective(
                theModel.getJniContext(), fs.getName());
            if (ars != null) {
                Archiver.resetArFSDirective(
                    theModel.getJniContext(), fs.getName());
            }
            /*
             * Do NOT Activate the archiver configuration at this time.
             * Doing so would result in an error because there are no
             * vsns assigned to the archiving filesystem that is being
             * removed.
             */
        }

        FS.remove(theModel.getJniContext(), fs.getName());

        /*
         * if archiving fs check for and cleanup any task schedules
         */
        if ((theModel.getLicenseType() == SamQFSSystemModel.SAMFS ||
             theModel.getLicenseType() == SamQFSSystemModel.SAMQFS) &&
             fs.getArchivingType() == FileSystem.ARCHIVING) {

            Schedule [] schedule = theModel.getSamQFSSystemAdminManager()
                .getSpecificTasks(ScheduleTaskID.SNAPSHOT, fs.getName());
            if (schedule != null && schedule.length > 0) {
                theModel.getSamQFSSystemAdminManager()
                    .removeTaskSchedule(schedule[0]);
            }

            // activate the new archiver configuration
            Archiver.activateCfgThrowWarnings(theModel.getJniContext());
	}

    }

    public RemoteFile[] getDirEntries(int maxEntries, String dirPath,
                                      Filter filter) throws SamFSException {

        RemoteFile [] remoteFiles = null;
        String[] fileNames = FileUtil.getDirEntries(theModel.getJniContext(),
                                                   maxEntries, dirPath,
                                                   (filter == null) ? null
                                                       :filter.toString());
        String[] fDetails;
        int[] fTypes;

        if (fileNames != null) {
            fileNames = prependDirName(dirPath, fileNames, 0, fileNames.length);
            remoteFiles = new RemoteFile[fileNames.length];
            fDetails = FileUtil.getFileDetails(theModel.getJniContext(),
                                              "",
                                              fileNames);
            fTypes = FileUtil.getFileStatus(theModel.getJniContext(),
                                           fileNames);
            for (int i = 0; i < fileNames.length; i++) {
                remoteFiles[i] =
                    new RemoteFile(fileNames[i],
                                   fTypes[i],
                                   ConversionUtil.strToProps(fDetails[i]));
            }
        }
	return remoteFiles;
    }

    public long startMetadataDump(String fsName, String fullDumpPath)
        throws SamFSException {
        String id =
            Restore.takeDump(theModel.getJniContext(), fsName, fullDumpPath);
        return 100 * ConversionUtil.strToLongVal(id) + Job.TYPE_DUMPFS;
    }

    // It needs to detect the version of the server, and call the proper
    // server function.
    public RestoreDumpFile[] getAvailableDumpFiles(
        String fsName, String directory) throws SamFSException {

        RestoreDumpFile[] files;
        String[] dumpNames = Restore.getDumps(theModel.getJniContext(),
                                              fsName,
                                              directory);
        int n = (dumpNames == null) ? 0 : dumpNames.length;

        String[] dumpInfos = Restore.getDumpStatus(theModel.getJniContext(),
                                                   fsName,
                                                   directory,
                                                   dumpNames);
        files = new RestoreDumpFile[n];
        for (int i = 0; i < n; i++) {
            files[i] = new RestoreDumpFile(dumpNames[i],
                           ConversionUtil.strToProps(dumpInfos[i]));
        }

        return files;
    }

    /**
     *  enable dump for use. will index and/or decompress
     *  @return jobid
     */
    public long enableDumpFileForUse(
        String fsName, String directory, String dumpFilename)
        throws SamFSException {
        String dumpPath = SamUtil.buildPath(directory, dumpFilename);

        String idStr = Restore.decompressDump(theModel.getJniContext(),
            fsName, dumpPath);
        if (idStr == null)
            return -1;
        return 100 * ConversionUtil.strToLongVal(idStr) + Job.TYPE_ENABLEDUMP;
    }


    /**
     * @return Information about the specified file from specified dump
     * directory paramater is taken away as the back end in 4.6 returns the
     * full qualified path of a dumpFile.
     */
    public RestoreFile getRestoreFile(String fileName) throws SamFSException {
        return new RestoreFileImpl(
                fileName, new String[] {"fake details"});
    }

    public RestoreFile getEntireFSRestoreFile() throws SamFSException {
        // The path for restoring the entire file system must be "."
        // Other members are inconsequential
        RestoreFile file =
                        new RestoreFileImpl(".", new String[] {"fake details"});
        // Restore path must be "."
        file.setRestorePath(".");
        return file;

    }

    protected String[] prependDirName(String dirName,
                                      String[] fileNames,
                                      int startIdx,
                                      int pageCount) {

        // Bounds of fileNames and startIdx and pageCount guaranteed by caller.
        String[] fullNames = new String[fileNames.length];
        int count = startIdx + pageCount;
        for (int i = startIdx; i < count; i++) {
            fullNames[i] = dirName + "/" + fileNames[i];
        }
        return fullNames;
    }


    /* get details about a list of named files from a specified dump */
    protected RestoreFile[] getRestoreFiles(String fsName,
                                            String dumpFullPath,
                                            String[] fileNames,
                                            int startIdx,
                                            int pageCount)
                                            throws SamFSException {

        // Bounds of fileNames and startIdx and pageCount guaranteed by caller.
        RestoreFile[] restoreFiles = new RestoreFile[fileNames.length];
        String[] fDetails;
        int count = startIdx + pageCount;
        for (int i = startIdx; i < count; i++) {
            fDetails = Restore.getVersionDetails(
                            theModel.getJniContext(), fsName,
                            dumpFullPath, fileNames[i]);
            restoreFiles[i] =
                new RestoreFileImpl(fileNames[i], fDetails);
	}
        return restoreFiles;
    }

    /**
     * Restore specified inodes. Return jobID (4.6 above)
     * directory paramater is taken away as the back end in 4.6 returns the
     * full qualified path of a dumpFile.
     */
    public long restoreFiles(String fsName,
                             String dumpFilename,
                             int replaceType,
                             RestoreFile[] files) throws SamFSException {
        if (replaceType != RESTORE_REPLACE_NEVER &&
            replaceType != RESTORE_REPLACE_ALWAYS &&
            replaceType != RESTORE_REPLACE_WITH_NEWER) {
            // Developer error!
            throw new SamFSException("Invalid replace type:  " +
                                     String.valueOf(replaceType));
        }

        SamFSConnection connection = theModel.getJniConnection();
        TraceUtil.trace2(
            "Change Timeout to " +
            SamQFSSystemModelImpl.DEFAULT_RPC_MAX_TIME_OUT);

        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_MAX_TIME_OUT);
        }

        TraceUtil.trace2("Run Restore.restoreInodes()!");
        String taskID =
            Restore.restoreInodes(theModel.getJniContext(), fsName,
                                  dumpFilename,
                                  new String[] { files[0].getAbsolutePath() },
                                  new String[] { files[0].getRestorePath() },
                                  new int[] { files[0].getStageCopy() },
                                  replaceType);
        TraceUtil.trace2(
            "Change Timeout back to " +
            SamQFSSystemModelImpl.DEFAULT_RPC_TIME_OUT);
        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_TIME_OUT);
        }

        return 100 * ConversionUtil.strToLongVal(taskID) + Job.TYPE_RESTORE;
    }

    public void cleanDump(String fsName, String directory, String dumpName)
        throws SamFSException {
        String dumpPath = SamUtil.buildPath(directory,  dumpName);
        Restore.cleanDump(theModel.getJniContext(), fsName, dumpPath);
    }

    // New in 4.5
    public void deleteDump(String fsName, String directory, String dumpName)
        throws SamFSException {
        String fullPath = SamUtil.buildPath(directory,  dumpName);
        Restore.deleteDump(theModel.getJniContext(), fsName, fullPath);
    }

    // New in 4.5
    public void setIsDumpRetainedPermanently(String fsName,
                                             String directory,
                                             String dumpName,
                                             boolean retainValue)
                                             throws SamFSException {
        String fullPath = SamUtil.buildPath(directory,  dumpName);
        Restore.setIsDumpRetainedPermanently(theModel.getJniContext(),
                                             fullPath, retainValue);
    }

    /**
     * @since 4.4
     */
    public GenericFileSystem createUFS(DiskCache dev, String mountPoint,
                                       GenericMountOptions mountOpts,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate)
        throws SamFSMultiStepOpException, SamFSException {

          DiskDev[] data = new DiskDev[] { ((DiskCacheImpl) dev).getJniDisk() };
          MountOptions jniMo = new MountOptions();
          jniMo.setReadOnlyMount(mountOpts.isReadOnlyMount());
          jniMo.setNoSetUID(mountOpts.isNoSetUID());
	  FSInfo fsInfo = new FSInfo(dev.getDevicePath(), // fsName
                                     0, // equ
                                     0, // DAU
                                     FSInfo.UFS_DATA,
                                     null, // meta
                                     data, // data
                                     null, // grp
                                     jniMo,
                                     mountPoint);
          FS.createAndMount(theModel.getJniContext(), fsInfo, mountAtBoot,
                                     createMountPoint,
                                     mountAfterCreate);
          return new GenericFileSystemImpl(theModel.getHostname(),
                                           dev.getDevicePath(),
                                           "ufs",
                                           GenericFileSystem.MOUNTED,
                                           mountPoint,
                                           dev.getCapacity(),
                                           dev.getAvailableSpace());
    }

    public StageFile parseStageFileDetails(String details)
        throws SamFSException {
        Properties properties =
            ConversionUtil.strToProps(details);

        String fileName = properties.getProperty(StageFile.NAME);

        String temp = properties.getProperty(StageFile.TYPE);
        int type = temp != null ? Integer.parseInt(temp.trim()) : -1;

        temp = properties.getProperty(StageFile.SIZE);
        long size = temp != null ? Long.parseLong(temp.trim()) : -1;

        temp = properties.getProperty(StageFile.CREATE_DATE);
        long ctime = temp != null ? Long.parseLong(temp.trim()) : -1;

        temp = properties.getProperty(StageFile.MODIFIED_DATE);
        long mtime = temp != null ? Long.parseLong(temp.trim()) : -1;

        temp = properties.getProperty(StageFile.ACCESSED_DATE);
        long atime = temp != null ? Long.parseLong(temp.trim()) : -1;

        temp = properties.getProperty(StageFile.PROTECTION);
        int protection = temp != null ? Integer.parseInt(temp.trim()) : -1;

        // instantiate the StageFile object and return it
        StageFile file =  new StageFile(fileName, size, mtime, type);

        file.setCreatedTime(ctime);
        file.setAccessedTime(atime);
        file.setProtection(protection);

        temp = properties.getProperty(StageFile.CPROTECTION);
        file.setCProtection(temp != null ? temp : "");

        // archive attributes
        temp = properties.getProperty(StageFile.ARCHIVE_ATTS);
        file.setArchiveAttributes(temp != null ? Integer.parseInt(temp) : -1);

        // stage attributes
        temp = properties.getProperty(StageFile.STAGE_ATTS);
        file.setStageAttributes(temp != null ? Integer.parseInt(temp) : -1);

        // release attributes
        temp = properties.getProperty(StageFile.RELEASE_ATTS);
        file.setReleaseAttributes(temp != null ? Integer.parseInt(temp) : -1);

        // Partial Release Size
        temp = properties.getProperty(StageFile.PARTIAL_RELEASE);
        file.setPartialReleaseSize(temp != null ? Long.parseLong(temp) : -1);

        // Segment Count
        temp = properties.getProperty(StageFile.SEGMENT_COUNT);
        file.setSegmentCount(temp != null ? Integer.parseInt(temp) : -1);

        // Segment Size
        temp = properties.getProperty(StageFile.SEGMENT_SIZE);
        file.setSegmentSize(temp != null ? Long.parseLong(temp) : -1);

        // Segment Stage Ahead
        temp = properties.getProperty(StageFile.SEGMENT_STAGE_AHEAD);
        file.setSegmentStageAhead(temp != null ? Long.parseLong(temp) : -1);

        // Arch Done
        temp = properties.getProperty(StageFile.ARCH_DONE);
        file.setArchDone(temp != null ? Integer.parseInt(temp) : -1);

        // Stage Pending
        temp = properties.getProperty(StageFile.STAGE_PENDING);
        file.setStagePending(temp != null ? Integer.parseInt(temp) : -1);

        // User
        temp = properties.getProperty(StageFile.USER);
        file.setUser(temp != null ? temp : "");

        // Group
        temp = properties.getProperty(StageFile.GROUP);
        file.setGroup(temp != null ? temp : "");

        // Online Count
        temp = properties.getProperty(StageFile.STATE_ONLINE);
        file.setOnlineCount(temp != null ? Integer.parseInt(temp) : 0);

        // Offline Count
        temp = properties.getProperty(StageFile.STATE_OFFLINE);
        file.setOfflineCount(temp != null ? Integer.parseInt(temp) : 0);

        // Partial Online Count
        temp = properties.getProperty(StageFile.STATE_PARTIAL_ONLINE);
        file.setPartialOnlineCount(temp != null ? Integer.parseInt(temp) : 0);

        // WORM
        temp = properties.getProperty(StageFile.WORM);
        if (null == temp) {
            file.setWormState(StageFile.WORM_DISABLED);
            file.setWormStart(-1);
            file.setWormDuration(-1);
            file.setWormEnd(-1);
        } else {
            if ("capable".equals(temp)) {
                file.setWormState(StageFile.WORM_CAPABLE);
            } else if ("active".equals(temp)) {
                file.setWormState(StageFile.WORM_ACTIVE);
            } else if ("expired".equals(temp)) {
                file.setWormState(StageFile.WORM_EXPIRED);
            } else {
                file.setWormState(StageFile.WORM_DISABLED);
            }

            temp = properties.getProperty(StageFile.WORM_DURATION);
            if (null == temp) {
                file.setWormDuration(-1);
                file.setWormPermanent(false);
            } else if ("permanent".equals(temp)) {
                file.setWormDuration(-1);
                file.setWormPermanent(true);
            } else {
                // in minutes
                file.setWormDuration(Long.parseLong(temp));
                file.setWormPermanent(false);
            }

            temp = properties.getProperty(StageFile.WORM_START);
            file.setWormStart(
                null == temp ?
                    -1 :
                    // remains in sec
                    Long.parseLong(temp));

            file.setWormEnd(
                file.isWormPermanent() ?
                    -1 :
                    file.getWormStart() +
                    file.getWormDuration() * 60);
        }

        file.setProperties(properties);
        file.setRawDetails(details);

        // Special check if entry contains copy information
        temp = properties.getProperty(FileCopyDetails.KEY_COPY);
        if (temp == null) {
            file.setFileCopyDetails(new FileCopyDetails[0]);
        } else {
            handleCopyInformation(
                file,
                FileCopyDetails.KEY_COPY.concat("=").concat(temp));
        }

        return file;
    }

    /**
     * This method gets the copy information of a file if there's any.
     * The content will look like:
     * copy=1###media=dk@@@copy=2###media=dk@@@copy=3###media=dk@@@
     * copy=4###media=dk
     */
    private void handleCopyInformation(StageFile file, String content)
        throws SamFSException {
        String [] copyArray = content.split(StageFile.COPY_DELIMITOR);
        FileCopyDetails [] copyDetails = new FileCopyDetails[copyArray.length];

        // Work thru every copy, convert them to FileCopyDetails
        for (int i = 0; i < copyArray.length; i++) {
            String tmp = copyArray[i].replaceAll(
                        StageFile.COPY_CONTENT_DELIMITOR, ",");
            copyDetails[i] =
                new FileCopyDetails(ConversionUtil.strToProps(tmp));
        }

        file.setFileCopyDetails(copyDetails);
    }


    /**
     * Get file details in File Browser, both live and recovery point content
     *
     * @since 4.6
     */
    public DirInfo getAllFilesInformation(
                                        String fsName,
                                        String snapPath,
                                        String relativeDir,
                                        String lastFile,
                                        int maxEntries,
                                        int whichDetails,
                                        Filter filter) throws SamFSException {

        String filterString = filter == null ? null : filter.toString();
        GetList details = FileUtil.listCollectFileDetails(
                                theModel.getJniContext(),
                                fsName,
                                snapPath,
                                relativeDir,
                                lastFile,
                                maxEntries,
                                whichDetails,
                                filterString);

        String [] fileDetails = details.getFileDetails();
        StageFile [] stageFile = null;

        if (fileDetails != null && fileDetails.length > 0) {
            stageFile = new StageFile[fileDetails.length];

            // construct a StageFile object for each of the file details
            for (int i = 0; i < fileDetails.length; i++) {
                stageFile[i] = parseStageFileDetails(fileDetails[i]);
            }
        }

        return new DirInfo(details.getTotalCount(), stageFile);
    }

    /**
     * Get file details of a single file in File Browser, both live and
     * recovery point content
     *
     * @since 4.6
     */
    public StageFile getFileInformation(
                                        String fsName,
                                        String snapPath,
                                        String filePath,
                                        int whichDetails)
                                        throws SamFSException {
        String detail = FileUtil.collectFileDetails(
                                    theModel.getJniContext(),
                                    fsName,
                                    snapPath,
                                    filePath,
                                    whichDetails);

        StageFile stageFile = parseStageFileDetails(detail);
        return stageFile;
    }

    public Metric getMetric(
        String fsName, int metricType, long startTime, long endTime)
        throws SamFSException {

        String xml = FS.getFileMetrics(
                theModel.getJniContext(),
                fsName,
                metricType,
                startTime,
                endTime);
        return (new Metric(xml, metricType, startTime, endTime));
    }

    public long stageFiles(int copy, String [] filePaths, int options)
        throws SamFSException {

	if (copy == 1) {
	    options |= Stager.COPY_1;
	} else if (copy == 2) {
	    options |= Stager.COPY_2;
	} else if (copy == 3) {
	    options |= Stager.COPY_3;
	} else if (copy == 4) {
	    options |= Stager.COPY_4;
	}
	String taskID = Stager.stageFiles(theModel.getJniContext(),
            filePaths, options);

	return 100 * ConversionUtil.strToLongVal(taskID)
	    + Job.TYPE_STAGE_FILES;
    }

    /**
     * archive files and directories or set archive options for them
     *
     * @param list of files to be archived
     * @param options defined in Archiver.java (JNI)
     * @return job ID
     * @since 4.6
     */
    public long archiveFiles(String [] files, int options)
        throws SamFSException {

        if (files == null || files.length == 0) {
            throw new SamFSException("Logic: Empty file array is passed!");
        }
        String taskID =
            Archiver.archiveFiles(theModel.getJniContext(), files, options);

        return 100 * ConversionUtil.strToLongVal(taskID) +
                Job.TYPE_ARCHIVE_FILES;
    }


    /**
     * release files and directories or set archive options for them
     *
     * @param list of files to be archived
     * @param options defined in Archiver.java (JNI)
     * @param release partial size. If PARTIAL is specified but partial_size is
     *  not, the mount option partial will be applied.
     * @return job ID
     * @since 4.6
     */
    public long releaseFiles(String [] files, int options, int partialSize)
        throws SamFSException {

        if (files == null || files.length == 0) {
            throw new SamFSException("Logic: Empty file array is passed!");
        }
        String taskID =
            Releaser.releaseFiles(
                theModel.getJniContext(), files, options, partialSize);

        return 100 * ConversionUtil.strToLongVal(taskID)
                + Job.TYPE_RELEASE_FILES;
    }


    /**
     * Change File Attributes (Archive, Release, Stage)
     *
     * @param Type of attribute to be changed (ARCHIVE, RELEASE, STAGE)
     * @param file name of which the attributes will be applied
     * @param New attributes to be applied to the files
     * @param Existing attributes of the files
     * @param if recursive check box is checked (false for file entries)
     * @param Partial Size (only applicable to RELEASE)
     * @return job ID
     * @since 4.6
     *
     *
     * This method will set the file attributes that user chooses in the File
     * Details Pop Up under the File Browser component.  It will reset the
     * file attributes with the DEFAULT flag if necessary.
     *
     * Archive is straight forward.  There are only two options available.
     * 1. Never Archive
     * 2. Archive according to policy (default)
     *
     * Release is a bit tricky.  We need to reset the attributes before applying
     * new attributes to a file if we are changing the attributes in between
     * these two groups of attributes.  They are mutually exclusive.
     *
     * Group A: Never Release
     * Group B: Release After One Copy, Partial Release,
     *   When Space is Required (default)
     *
     * Moreover RECURSIVE option needs to be included for directory entries.
     * This gets even trickier if we should apply the RECURSIVE flag because
     * users may choose to apply the new option without clearing the existing
     * option of the files in the directory.  A check box should show up in the
     * pop up window and ask for user input. For more details please read
     * comment within the code.
     *
     * Stage is somewhat similar to Release. The two mutually exclusive groups
     * are as follow:-
     *
     * Group A: Never Stage
     * Group B: Associative Staging, When a file is accessed (default)
     *
     * Same rules apply to Stage for directory entries.
     *
     */
    public long changeFileAttributes(
        int type, String file,
        int newOption, int existingOption,
        boolean recursive, int partialSize)
        throws SamFSException {

        // Clear existing attributes before applying new attributes?
        boolean clear = false;

        // return job ID
        long jobID = -1;

        if (file == null || file.length() == 0) {
            throw new SamFSException(
                "File cannot be null when issuing changeFileAttributes call!");
        }

        /**
         * For directory entries, the RECURSIVE check box will appear in the
         * pagelet.  The bits are set based on the following rules:-
         *
         * 1. RECURSIVE bit is set ONLY if user checks the RECURSIVE check box.
         *    Checking this box means you want to override the existing flags
         *    of all files in this directory.  Unchecking this box means you
         *    do not want to apply the new flag to the existing files.
         *    Regardless of checking this RECURSIVE box or not, the flag of the
         *    directory itself will be changed so new files that go into
         *    this directory will follow the new flags.
         *
         *    There is ONE case that we are not covering here. i.e. Applying
         *    the newOption WITHOUT applying the DEFAULT flag.  A directory
         *    may contains many files and each file contains different flags.
         *    Without resetting their flags to DEFAULT before applying new flags
         *    may generate lots of errors that are impossible for the GUI to
         *    handle.
         *
         *    If RECURSIVE is not checked, the code should treat the directory
         *    as a file and run through the test as stated in #2 to determine
         *    if DEFAULT needs to be included.
         *
         * 2. In RELEASE & STAGE cases, DEFAULT flags are included in newOption
         *    if the file is changing its attributes from one group to the
         *    other.  Please read the method comment above for more details.
         *
         */
        switch (type) {
            case SamQFSSystemFSManager.ARCHIVE:
                // No need to check mutual exclusive case
                newOption =
                    recursive ?
                        newOption | Archiver.RECURSIVE | Archiver.DEFAULTS :
                        newOption;

                jobID = archiveFiles(new String [] {file}, newOption);
                break;

            case SamQFSSystemFSManager.RELEASE:
                // validation of partial release size happens in presentation
                // layer
                if (recursive) {
                    newOption = newOption |
                                Releaser.RESET_DEFAULTS | Releaser.RECURSIVE;
                } else {
                    // files & dir with checkbox unchecked
                    if (existingOption == Releaser.NEVER) {
                        clear =
                            newOption == Releaser.WHEN_1 ||
                            newOption == Releaser.PARTIAL;
                    } else {
                        clear = newOption == Releaser.NEVER;
                    }
                    // Reset to default if switching between Group A & Group B,
                    // see comments above
                    if (clear) {
                        newOption = newOption | Releaser.RESET_DEFAULTS;
                    }
                }

                jobID = releaseFiles(
                            new String [] {file},
                            newOption,
                            partialSize == -1 ? 0 : partialSize);
                break;

            case SamQFSSystemFSManager.STAGE:
                if (recursive) {
                    newOption = newOption |
                                Stager.RESET_DEFAULTS | Stager.RECURSIVE;
                } else {
                    // files & dir with checkbox unchecked
                    if (existingOption == Stager.NEVER) {
                        clear =
                            newOption == Stager.ASSOCIATIVE ||
                            newOption == Stager.PARTIAL;
                    } else {
                        clear = newOption == Stager.NEVER;
                    }
                    // Reset to default if switching between Group A & Group B,
                    // see comments above
                    if (clear) {
                        newOption = newOption | Stager.RESET_DEFAULTS;
                    }
                }

                stageFiles(0, new String [] {file}, newOption);
                break;

            default:
                throw new SamFSException("Logic: Incorrect type defined!");
        }

        return jobID;
    }

    /**
     * Delete the files
     * @param String[] list of fully qualified filenames
     */
    public void deleteFiles(String[] paths) throws SamFSException {
        FileUtil.deleteFiles(theModel.getJniContext(), paths);
    }

    /**
     * Delete the file
     * @param String fully qualified filename
     */
    public void deleteFile(String path) throws SamFSException {
        FileUtil.deleteFile(theModel.getJniContext(), path);
    }

    /**
     * Get all known directories containing snapshots for a given filesystem.
     *
     * @since 4.6
     * @return a list of directory paths.
     */
    public String [] getIndexDirs(String fsName) throws SamFSException {

        String [] dirs = Restore.getIndexDirs(theModel.getJniContext(), fsName);
	dirs = dirs == null ? new String[0] : dirs;
	if (dirs.length == 0) {
	    return dirs;
	}

	// Skip directories that no longer exists (user issues rm command)
	ArrayList<String> verifiedDirs = new ArrayList<String>();
	for (String dir : dirs) {
	    if (FileUtil.fileExists(theModel.getJniContext(), dir)) {
		verifiedDirs.add(dir);
	    } else {
		TraceUtil.trace3("FSMgrImpl::getIndexDirs: Skipping " + dir);
	    }
	}
	return (String []) verifiedDirs.toArray(new String[0]);
    }

    /**
     * Get all indexed snapshots for a given filesystem.  Returns an array
     * of key/value pairs of the form  "name=%s,date=%ld"
     *
     * @since 4.6
     * @return all indexed snapshots for a given file system
     */
    public String [] getIndexedSnaps(String fsName, String directory)
        throws SamFSException {

        return Restore.getIndexedSnaps(theModel.getJniContext(),
                                       fsName,
                                       directory);
    }
}
