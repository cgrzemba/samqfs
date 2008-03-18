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

// ident	$Id: SamQFSSystemSharedFSManagerImpl.java,v 1.40 2008/03/17 14:43:46 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.FSArchCfg;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.mgmt.fs.Host;
import com.sun.netstorage.samqfs.mgmt.fs.MountOptions;
import com.sun.netstorage.samqfs.mgmt.fs.StripedGrp;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;
import com.sun.netstorage.samqfs.web.model.MDSAddresses;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.FileSystemImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.FileSystemMountPropertiesImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.SharedMemberImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DiskCacheImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.SharedDiskCacheImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.StripedGroupImpl;
import com.sun.netstorage.samqfs.web.model.impl.mt.Barrier;
import com.sun.netstorage.samqfs.web.model.impl.mt.MethodInfo;
import com.sun.netstorage.samqfs.web.model.impl.mt.ThreadPool;
import com.sun.netstorage.samqfs.web.model.impl.mt.ThreadPoolMember;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;
import java.util.LinkedList;

public class SamQFSSystemSharedFSManagerImpl extends MultiHostUtil implements
        SamQFSSystemSharedFSManager {

    private SamQFSAppModelImpl appModel = null;
    private ThreadPool threadPool = null;

    public SamQFSSystemSharedFSManagerImpl(SamQFSAppModelImpl model) {
        this.appModel = model;
	threadPool = new ThreadPool(10);
	// initialize thread pool (start threads)
        threadPool.init();
    }
    public void freeResources() {
        threadPool.destroy();
    }

    public SharedDiskCache[] discoverAllocatableUnitsForShared(String[] servers,
                                                               String[] clients,
                                                               boolean ignoreHA)
        throws SamFSMultiHostException {
            DiskCacheImpl[][] allDCs;
	    SharedDiskCacheImpl[] sdcArr;
	    ArrayList sdcLst = new ArrayList();
            int i, j, clientsLen, serversLen;
	    SamQFSSystemModelImpl model;
            ArrayList errorHostNames = new ArrayList();
            ArrayList errorExceptions = new ArrayList();

	    if (null == clients) {
                clientsLen = 0;
            } else {
                clientsLen = clients.length;
            }
	    if (null == servers) {
                throw new SamFSMultiHostException(
                    "internal error:need to pass metadata server!");
            } else {
                serversLen = servers.length;
            }

	    allDCs = new DiskCacheImpl[servers.length + clientsLen][];

            /* run discovery in parallel on all clients and servers */
            parallelDisco(servers, clients, allDCs,
                errorHostNames, errorExceptions, ignoreHA);

            if (!errorHostNames.isEmpty()) {
               throw new SamFSMultiHostException(
                   "logic.sharedFSOperationPartialFailure",
                   (SamFSException[])
                   errorExceptions.toArray(new SamFSException[0]),
                   (String[]) errorHostNames.toArray(new String[0]));
            }

	    /* initialize SharedDiskCache array */
	    if (allDCs[0] == null) {
                return null;
            }

	    for (i = 0; i < allDCs[0].length; i++) {
		/* construct SharedDiskCache from DiskCache */
		if (allDCs[0][i].getJniDisk().getAU().getType() == AU.SLICE) {
		    /* skip volumes (cannot use them for shared fs) */
		    sdcLst.add(new SharedDiskCacheImpl(allDCs[0][i]));
                }
	    }
	    sdcArr = (SharedDiskCacheImpl[])
                sdcLst.toArray(new SharedDiskCacheImpl[0]);

	    /* now merge the information based on device id */

	    DiskCacheImpl crtDskCache; // crt client/potential MDS disk cache

            for (int s = 1; s < serversLen; s++) {
		for (i = 0; i < sdcArr.length; i++) {
		    for (j = 0; j < allDCs[s].length; j++) {
			crtDskCache = allDCs[s][j];
			if (matchAUs(sdcArr[i].getJniDisk(),
                            crtDskCache.getJniDisk())) {
			    /*
			     * add crt server to the list of servers
			     * who can see this device
			     */
			    sdcArr[i].addServer(servers[s]);
			    /* add device path on this server */
			    sdcArr[i].addServerDevpath(
				crtDskCache.getDevicePath());
			}
		    }
                }
	    }
            for (int c = 0; c < clientsLen; c++) {
		for (i = 0; i < sdcArr.length; i++) {
		    for (j = 0; j < allDCs[serversLen + c].length; j++) {
			crtDskCache = allDCs[serversLen + c][j];
			if (matchAUs(sdcArr[i].getJniDisk(),
                            crtDskCache.getJniDisk())) {

			    /* if used on client c */
			    if (crtDskCache.
				getJniDisk().getAU().getUsedBy() != null) {
                                TraceUtil.trace3(clients[c] + ": " +
                                    crtDskCache.getJniDisk().getAU().getPath()
                                    + " used by " + crtDskCache.getJniDisk().
                                    getAU().getUsedBy());
				sdcArr[i].setUsedByClient(true);
			    } else {
			    /*
			     * add crt client to the list of clients
			     * who can see this device
			     */
			    sdcArr[i].addClient(clients[c]);
			    /* add device path on this client */
			    sdcArr[i].addClientDevpath(
				crtDskCache.getDevicePath());
			    }
			}
		    }
                }
	    }

	    return sdcArr;
    }

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
        throws SamFSMultiStepOpException, SamFSMultiHostException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        // if this new shared file system has the same metadata and data devices
        boolean isMetaSame =
            (metadataDevices == null || metadataDevices.length == 0);

        // check arguments

        if ((!SamQFSUtil.isValidString(fsName)) ||
            (!SamQFSUtil.isValidString(mountPoint)))
            throw new SamFSMultiHostException("logic.invalidFSParam");

        if ((dataDevices == null || dataDevices.length == 0) &&
            (stripedGroups == null || stripedGroups.length == 0)) {
            throw new SamFSMultiHostException("logic.invalidFSParam");
        }

        int mdServerIndex = -1;
        for (int i = 0; i < members.length; i++) {
            if (members[i].getType() == SharedMember.TYPE_MD_SERVER) {
                mdServerIndex = i;
                break;
            }
        }

        // Throw error if this new shared file system does not have metadata
        // server set
        if (mdServerIndex == -1) {
            throw new SamFSMultiHostException("logic.invalidFSParam");
        }

        SamQFSSystemModelImpl[] models = getSystemModels(members);

        String mdServerName = members[mdServerIndex].getHostName();
        SamQFSSystemModelImpl mdServerModel = models[mdServerIndex];

        DiskDev[] meta = null;
        DiskDev[] data = null;
        StripedGrp[] grp = null;

        String eqTypeForJni = single ? "mr" : "md";

        // find a block of equipment ordinals that are available on all hosts

        int countEqOrdinalsRequired = countOrdinalsRequired(
            metadataDevices, dataDevices, stripedGroups);

        int firstAvailableOrdinal = getAvailableOrdinals(
            countEqOrdinalsRequired, models);

        int equ = firstAvailableOrdinal++;

        // set equipment ordinals
        SharedDiskCache[] metaSDC = null;

        if (isMetaSame) {
            metaSDC = new SharedDiskCache[0];
            meta    = new DiskDev[0];
        } else {
            metaSDC = (SharedDiskCache[]) metadataDevices;
            meta = new DiskDev[metadataDevices.length];
            for (int i = 0; i < metadataDevices.length; i++) {
                meta[i] = ((DiskCacheImpl) metadataDevices[i]).getJniDisk();
                meta[i].setEquipType("mm");
                meta[i].setEquipOrdinal(firstAvailableOrdinal++);
            }
        }

        SharedDiskCache[] dataSDC = null;
        if (dataDevices != null && dataDevices.length > 0) {
            dataSDC = (SharedDiskCache[]) dataDevices;
            if (dataDevices != null && dataDevices.length > 0) {
                data = new DiskDev[dataDevices.length];
                for (int i = 0; i < dataDevices.length; i++) {
                    data[i] = ((DiskCacheImpl) dataDevices[i]).getJniDisk();
                    data[i].setEquipType(eqTypeForJni);
                    data[i].setEquipOrdinal(firstAvailableOrdinal++);
                }
            }
        }

        if (stripedGroups != null) {
            grp = new StripedGrp[stripedGroups.length];
            for (int i = 0; i < stripedGroups.length; i++) {
                grp[i] = ((StripedGroupImpl) stripedGroups[i]).
                    getJniStripedGroup();
                DiskDev[] disks = grp[i].getMembers();
                for (int j = 0; j < disks.length; j++) {
                    disks[j].setEquipOrdinal(firstAvailableOrdinal++);
                }
            }
        }

        // finish populating fs object

        MountOptions opt = ((FileSystemMountPropertiesImpl) mountProps).
            getJniMountOptions();

        Host[] hosts = membersToHosts(members, models);

        FSInfo fsInfoMDS = new FSInfo(
            fsName, equ, DAUSize,
            isMetaSame ? FSInfo.COMBINED_METADATA : FSInfo.SEPARATE_METADATA,
            meta, data, grp, opt, mountPoint,
            mdServerName, true, false, hosts);

        FSInfo fsInfoPMDS = new FSInfo(
            fsName, equ, DAUSize,
            isMetaSame ? FSInfo.COMBINED_METADATA : FSInfo.SEPARATE_METADATA,
            meta, data, grp, opt, mountPoint,
            mdServerName, true, true, hosts);

        FSInfo fsInfoClient = new FSInfo(
            fsName, equ, DAUSize,
            isMetaSame ? FSInfo.COMBINED_METADATA : FSInfo.SEPARATE_METADATA,
            meta, data, grp, opt, mountPoint,
            mdServerName, false, false, hosts);

        // create fs on the metadata server
        FileSystem fs = null;
        String savePath = null;
        SamFSMultiStepOpException archWarning = null;

        try {
            if (archiveConfig == null) {
                FS.createAndMount(mdServerModel.getJniContext(),
                                  fsInfoMDS,
                                  mountAtBoot,
                                  createMountPoint,
                                  mountAfterCreate);
            } else {
                /**
                 * If there are warnings on the metadata server, we should
                 * continue and setup all of the other hosts. Any other failure
                 * should lead to a termination.
                 */
                try {
                    FS.createAndMount(mdServerModel.getJniContext(),
                                      fsInfoMDS,
                                      mountAtBoot,
                                      createMountPoint,
                                      mountAfterCreate,
                                      archiveConfig);
                } catch (SamFSMultiStepOpException e) {
                    /*
                     * if the archiver has warnings step 6 of create
                     * will be the failed step. Do not throw the exception
                     * if that is the case.
                     */
                    if (e.getFailedStep() != 6) {
                        throw e;
                    }

                    /*
                     * Hang on to the warnings to pass them along later
                     */
                    archWarning = e;
                }
            }

            fs = new FileSystemImpl(mdServerModel, fsInfoMDS);

        } catch (SamFSException e) {
            errorHostNames.add(mdServerName);
            errorExceptions.add(e);
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        // create fs on the other hosts
        FSInfo info = null;
        for (int i = 0; i < members.length; i++) {
           switch (members[i].getType()) {
               case SharedMember.TYPE_MD_SERVER:
                   // Already done
                   info = null;
                   break;
               case SharedMember.TYPE_POTENTIAL_MD_SERVER:
                   info = fsInfoPMDS;
                   break;
               case SharedMember.TYPE_CLIENT:
                   info = fsInfoClient;
                   break;
           }
           if (info != null) {
               String name = members[i].getHostName();
               SamQFSSystemModelImpl model = getSystemModel(name,
                   errorHostNames, errorExceptions);
               if (errorHostNames.isEmpty()) {
                   try {
                       setDevicePaths(metaSDC, meta, dataSDC, data,
                           stripedGroups, grp, members[i]);

                       FS.createAndMount(model.getJniContext(), info,
                           mountAtBoot, createMountPoint, mountAfterCreate);

                   } catch (SamFSException e) {
                       errorHostNames.add(name);
                       errorExceptions.add(e);
                   }
               }
           }
        }
        if (archWarning != null) {
            throw archWarning;
        }

        if (!errorHostNames.isEmpty()) {
           throw new SamFSMultiHostException(
               "logic.sharedFSOperationPartialFailure",
               (SamFSException[])
               errorExceptions.toArray(new SamFSException[0]),
               (String[]) errorHostNames.toArray(new String[0]));
        }

        return fs;
    }

    /**
     * @param potentialMdServer if true, then add the host as a metadata server
     */
    public FileSystem addHostToSharedFS(String fsName,
                                       String mdServerName,
                                       String mountPoint,
                                       String hostName,
                                       String[] hostIP,
                                       boolean readOnly,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate,
                                       boolean potentialMdServer,
                                       boolean bg)
        throws SamFSMultiHostException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();
        String uname = null;

        if ((!SamQFSUtil.isValidString(fsName)) ||
            (!SamQFSUtil.isValidString(mountPoint)))
            throw new SamFSMultiHostException("logic.invalidFSParam");

        // STEP 1: get the models for the 2 servers involved

        SamQFSSystemModelImpl mdServerModel = getSystemModel(mdServerName,
            errorHostNames, errorExceptions);

        SamQFSSystemModelImpl hostModel = getSystemModel(hostName,
            errorHostNames, errorExceptions);

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        // STEP 2: retrieve filesystem information from mds and check
        // if eqs for this fs are locally in use

        FSInfo mdsInfo = null;
        FSInfo info = null;
        Host host = null;
        try {
            mdsInfo = FS.get(mdServerModel.getJniContext(), fsName);

            MountOptions mdsOpts = mdsInfo.getMountOptions();
            mdsOpts.setReadOnlyMount(readOnly);
            mdsOpts.setMountInBackground(bg);

            // check if eqs are already used on the host about to be added
            try {
                verifyEQsAreAvailOnNewHost(mdsInfo, hostModel);

            } catch (SamFSException e) {
                String msg;
                if (e.getSAMerrno() == SamFSException.EQU_ORD_IN_USE)
                    msg = "logic.sharedFSEQInUse";
                else
                    msg = "logic.sharedFSOperationPartialFailure";
                errorHostNames.add(hostName);
                errorExceptions.add(e);
                throw new SamFSMultiHostException(msg,
                    (SamFSException[])
                    errorExceptions.toArray(new SamFSException[0]),
                    (String[]) errorHostNames.toArray(new String[0]));
            }

            Host[] hosts = mdsInfo.getHosts();
            int priority = 0; // client
            if (potentialMdServer) {
                // Make the priority for the new server 1 lower then
                // the current lowest priority server.
                for (int i = 0; i < hosts.length; i++) {
                    if (hosts[i].getSrvPrio() > priority) {
                        priority = hosts[i].getSrvPrio();
                    }
                }
                priority++;
            }

            // Get the name that would be returned by gethostname on the
            // host.
            uname = hostModel.getServerHostname();

            // Add the new host to the array of members
            host = new Host(uname, hostIP, priority, false);
            Host[] hosts2 = new Host[hosts.length + 1];
            for (int i = 0; i < hosts.length; i++) {
                hosts2[i] = hosts[i];
            }
            hosts2[hosts.length] = host;

            DiskDev[] dataDevices = mdsInfo.getDataDevices();
            DiskDev[] metaDevices = mdsInfo.getMetadataDevices();
            StripedGrp[] stripedGroups = mdsInfo.getStripedGroups();
            boolean combMetaAndData = true;
            if (metaDevices != null && metaDevices.length > 0) {
                combMetaAndData = false;
            }

            // in non SC configs, same devs have different names on each host
            // so the appropriate names must be set
            if (!hostModel.isClusterNode()) {

                DiskCache[] disksKnown = hostModel.getSamQFSSystemFSManager().
                    discoverAvailableAllocatableUnits(null);

                fixDiskPath(dataDevices, disksKnown);

                if (!combMetaAndData) {
                    if (potentialMdServer) {
                        fixDiskPath(metaDevices, disksKnown);
                    }
                    for (int i = 0; i < stripedGroups.length; i++) {
                        fixDiskPath(stripedGroups[i].getMembers(),
                            disksKnown);
                    }
                }
            } else {
                mountAtBoot = false;
            }

            info = new FSInfo(fsName, mdsInfo.getEqu(),
                    mdsInfo.getDAUSize(), combMetaAndData ?
                    FSInfo.COMBINED_METADATA : FSInfo.SEPARATE_METADATA,
                metaDevices, dataDevices, stripedGroups,
                mdsOpts, mountPoint, mdServerName,
                potentialMdServer, potentialMdServer, hosts2);
        } catch (SamFSMultiHostException multiEx) {
            // generated locally. simply rethrow it
            throw multiEx;
        } catch (SamFSException e) {
            errorHostNames.add(mdServerName);
            errorExceptions.add(e);
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        // STEP 3: make the actual changes on the 2 hosts involved

        FileSystem fs = null;
        try {
            // Add the new host to the MD Server's config file
            Host.addToConfig(mdServerModel.getJniContext(),
                fsName, host);

            FS.createAndMount(hostModel.getJniContext(), info, mountAtBoot,
                createMountPoint, mountAfterCreate);

            fs = new FileSystemImpl(hostModel, info);

        } catch (SamFSException e) {
            errorHostNames.add(hostName);
            errorExceptions.add(e);
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        return fs;
    }


    /**
     * delete a shared FS member or the whole FS if hostName is the MDS
     *
     * mdServerName is optional
     * if null then we try to retrieve MDS name from FS structre
     */
    public void deleteSharedFileSystem(String hostName, String fsName,
        String mdServerName)
        throws SamFSMultiHostException {

        TraceUtil.trace1("deleteSharedFS(" + hostName + "," + fsName +
            "," + ((mdServerName == null) ? "-" : mdServerName));

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        SamQFSSystemModelImpl modelMDS = null;
        SamQFSSystemModelImpl model =
            getSystemModel(hostName, errorHostNames, errorExceptions);

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }


        FSInfo info = null;
        try {
            info = FS.get(model.getJniContext(), fsName);
            if (mdServerName == null) {
                mdServerName = info.getServerName();
            }
            modelMDS = getSystemModel(mdServerName,
                errorHostNames, errorExceptions);
        } catch (SamFSException e) {
            // if FS does not exist on hostName, remove it from MDS config
            if (e.getSAMerrno() == SamFSException.NOT_FOUND) {
                if (mdServerName == null) { // give up
                    throw
                        new SamFSMultiHostException("logic.sharedNotSharedFS");
                }
                try {
                    modelMDS = getSystemModel(mdServerName,
                        errorHostNames, errorExceptions);
                    Host.removeFromConfig(modelMDS.getJniContext(),
                        fsName, hostName);
                } catch (SamFSException ex) {
                    errorHostNames.add(mdServerName);
                    errorExceptions.add(e);
                    throw new SamFSMultiHostException(
                        "logic.sharedFSOperationPartialFailure",
                        (SamFSException[])
                        errorExceptions.toArray(new SamFSException[0]),
                        (String[]) errorHostNames.toArray(new String[0]));
                }
                return;
                // otherwise (all other exceptions) throw MultiHostException
            } else {
                errorHostNames.add(hostName);
                errorExceptions.add(e);
                throw new SamFSMultiHostException(
                    "logic.sharedFSOperationPartialFailure",
                    (SamFSException[])
                    errorExceptions.toArray(new SamFSException[0]),
                    (String[]) errorHostNames.toArray(new String[0]));
            }
        }


        // get all member hosts
        Host[] hosts = null;
        if (errorHostNames.isEmpty()) {
            try {
                hosts = Host.getConfig(modelMDS.getJniContext(), fsName);
            } catch (SamFSException e) {
                errorHostNames.add(mdServerName);
                errorExceptions.add(e);
            }
        } else {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        /**
         *  Phase1: verify preconditions on all member hosts
         *
         *  this requires getting the FS info from each member host
         */
        FSInfo[] infos = new FSInfo[hosts.length];
        SamQFSSystemModelImpl[] models =
            new SamQFSSystemModelImpl[hosts.length];

        for (int i = 0; i < hosts.length; i++) {
            try {
                models[i] = getSystemModel(hosts[i].getName(),
                    errorHostNames, errorExceptions);

                if (models[i] != null) {

                    // get FS from host i and verify that FS is not mounted
                    infos[i] = FS.get(models[i].getJniContext(), fsName);
                    if (infos[i].isMounted()) {
                        errorHostNames.add(hosts[i].getName());
                        errorExceptions.add(new SamFSException(
                            "logic.sharedMountedFS"));
                    } else {
                        TraceUtil.trace1("model was null");
		    }
                }
            } catch (SamFSException e) {
		/*
		 * must be able to recover from partial failures of create FS,
		 * so we ignore cases when the FS does not exist on client/PMDS
		 * These cases are not treated as exceptions, since we need to
		 * be able to clean the other hosts that are members of the FS.
		 */
		if (e.getSAMerrno() != SamFSException.NOT_FOUND
                    || modelMDS == models[i]) {
                           errorHostNames.add(hosts[i].getName());
                           errorExceptions.add(e);
                }
            }
        }

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }


        /*
         *  Phase2: perform the actual delete operation
         */

        if (info.isMdServer()) {
	    SamQFSSystemFSManagerImpl fsMngr = null;
	    FileSystemImpl fsImpl = null;

	    // Step 1: determine if fs is archiving and precheck
	    // the removal of archiving information.
	    if (info.isArchiving()) {
		// get the criteria and see if removal is allowed.
		fsMngr = new SamQFSSystemFSManagerImpl(modelMDS);

		try {
		    fsImpl = new FileSystemImpl(modelMDS, info);
		    ArchivePolCriteria [] crit =
			fsImpl.getArchivePolCriteriaForFS();

		    if (crit != null) {
			for (int i = 0; i < crit.length; i++) {
			    crit[i].canFileSystemBeRemoved(info.getName());
			}
		    }
		} catch (SamFSException sfe) {
		    errorHostNames.add(info.getName());
		    errorExceptions.add(sfe);
		}
	    }

            if (!errorHostNames.isEmpty()) {
                throw new SamFSMultiHostException(
                    "logic.sharedFSOperationPartialFailure",
                    (SamFSException[])
                    errorExceptions.toArray(new SamFSException[0]),
                    (String[]) errorHostNames.toArray(new String[0]));
            }


            // Step 2: Delete FS from all hosts except the MD server
            for (int i = 0; i < infos.length; i++)
                // if FS exist on this host i
                if (infos[i] != null) {
                    if (!infos[i].isMdServer()) {
                        try {
			    infos[i].remove(models[i].getJniContext());
                        } catch (SamFSException e) {
                            // If not found, no body cares. We are trying
                            // to delete it.
                            if (e.getSAMerrno() != e.NOT_FOUND) {
                                errorHostNames.add(hosts[i].getName());
                                errorExceptions.add(e);
                            }
                        }
                    }
		}

            if (!errorHostNames.isEmpty()) {
                throw new SamFSMultiHostException(
                    "logic.sharedFSOperationPartialFailure",
                    (SamFSException[])
                    errorExceptions.toArray(new SamFSException[0]),
                    (String[]) errorHostNames.toArray(new String[0]));
            }

            /*
             * Step 3: Delete FS from the MD server. If this is for an
             * archiving filesystem call the non-shared delete
             * function so that the archiving information and the
             * metadata snapshot schedules will be cleaned up.
             */
            try {
		if (info.isArchiving()) {
		    fsMngr.deleteFileSystem(fsImpl);
		} else {
		    info.remove(modelMDS.getJniContext());
		}
            } catch (SamFSException e) {
                errorHostNames.add(mdServerName);
                errorExceptions.add(e);
            }
        } else if (info.isPotentialMdServer()) {
            // Delete FS
            // MD server is automatically notified.
            try {
                info.remove(model.getJniContext());
            } catch (SamFSException e) {
                errorHostNames.add(hostName);
                errorExceptions.add(e);
            }
        } else if (info.isClient()) {
            try {
                // Delete FS
                info.remove(model.getJniContext());

                // Notify MD Server.
                Host.removeFromConfig(modelMDS.getJniContext(),
                    fsName, hostName);
            } catch (SamFSException e) {
                errorHostNames.add(hostName);
                errorExceptions.add(e);
            }
        } else {
            // Should never get here
            throw new SamFSMultiHostException("logic.sharedNotSharedFS");
        }
        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }
    }


    public boolean failingover(String mdServer, String fsName)
        throws SamFSException {

        SamQFSSystemModelImpl model = (SamQFSSystemModelImpl)
            this.appModel.getSamQFSSystemModel(mdServer);

        if (model.isDown()) {
            throw new SamFSException("logic.hostIsDown");
        }
        FSInfo info = FS.get(model.getJniContext(), fsName);
        if (info == null) {
            throw new SamFSException("logic.invalidFS");
        }
        return info.failoverStatus() != 0;
    }


    public int getSharedFSType(String hostName, String fsName)
        throws SamFSException {

        SamQFSSystemModelImpl model = (SamQFSSystemModelImpl)
            this.appModel.getSamQFSSystemModel(hostName);

        if (model.isDown()) {
            throw new SamFSException("logic.hostIsDown");
        }
        FSInfo info = FS.get(model.getJniContext(), fsName);
        if (info == null) {
            throw new SamFSException("logic.invalidFS");
        }
        if (info.isMdServer()) {
            return SharedMember.TYPE_MD_SERVER;
        } else if (info.isPotentialMdServer()) {
            return SharedMember.TYPE_POTENTIAL_MD_SERVER;
        } else if (info.isClient()) {
            return SharedMember.TYPE_CLIENT;
        }

        return SharedMember.TYPE_NOT_SHARED;
    }


    public SharedMember[] getSharedMembers(String mdServer, String fsName)
        throws SamFSMultiHostException {
        ArrayList members = new ArrayList();
        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        SamQFSSystemModelImpl model = getSystemModel(mdServer,
            errorHostNames, errorExceptions);
        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        // get list of all member hosts
        Host[] hosts = null;
        try {
            hosts = Host.getConfig(model.getJniContext(), fsName);
        } catch (SamFSException e) {
            errorHostNames.add(mdServer);
            errorExceptions.add(e);
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        // retrieve filesystem information from each host
        for (int i = 0; i < hosts.length; i++) {
            boolean mounted = false;
	    int type = SharedMember.TYPE_NOT_SHARED;
            try {
                SamQFSSystemModelImpl hostModel = getSystemModel(
                    hosts[i].getName(), errorHostNames, errorExceptions);
                if (hostModel == null)
                    continue;

                FSInfo info = FS.get(hostModel.getJniContext(), fsName);

                if (info.isClient()) {
                    type = SharedMember.TYPE_CLIENT;
                } else if (info.isMdServer()) {
                    type = SharedMember.TYPE_MD_SERVER;
                } else if (info.isPotentialMdServer()) {
                    type = SharedMember.TYPE_POTENTIAL_MD_SERVER;
                }
                mounted = info.isMounted();
                members.add(new SharedMemberImpl(
                    hosts[i].getName(),
                    hosts[i].getIPs(),
                    type,
                    mounted));
            } catch (SamFSException e) {
                /*
                 * include host i in the result even if FS not found in it, in
                 * order to allow the user to delete it and recover from a
                 * partial failure that resulted in host i showing as a member
                 */
                if (e.getSAMerrno() == SamFSException.NOT_FOUND) {
                    members.add(new SharedMemberImpl(
                    hosts[i].getName(),
                    hosts[i].getIPs(),
                    type,
                    mounted));
		}
                errorHostNames.add(hosts[i].getName());
                errorExceptions.add(e);
            }
        }

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]),
                (SharedMember[]) members.toArray(new SharedMember[0]));
        }

        return (SharedMember[]) members.toArray(new SharedMember[0]);
    }

    public void setSharedMountOptions(String mdServer, String fsName,
        FileSystemMountProperties options)
        throws SamFSMultiHostException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();
        FileSystemMountPropertiesImpl jniOptions =
            (FileSystemMountPropertiesImpl) options;

        SharedMember[] members = getSharedMembers(mdServer, fsName);
        for (int i = 0; i < members.length; i++) {
            try {
                SamQFSSystemModelImpl current = getSystemModel(
                    members[i].getHostName(), errorHostNames, errorExceptions);
                if (current == null) {
                    continue;
                }
                if (members[i].isMounted()) {
                    FS.setLiveMountOpts(current.getJniContext(), fsName,
                        jniOptions.getJniMountOptions());
                }
                FS.setMountOpts(current.getJniContext(), fsName,
                    jniOptions.getJniMountOptions());
            }
            catch (SamFSException e) {
                errorHostNames.add(members[i].getHostName());
                errorExceptions.add(e);
            }
        }
        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }
    }


    public String[] getHostsNotUsedBy(String fsName, String mdServer)
        throws SamFSException {

        LinkedList hostList = new LinkedList();
        SamQFSSystemModelImpl model = (SamQFSSystemModelImpl)
            this.appModel.getSamQFSSystemModel(mdServer);

        if (model.isDown()) {
            throw new SamFSException("logic.hostIsDown");
        }

        String mdsVersion = model.getServerProductVersion();
        SamQFSSystemModelImpl[] allModels = (SamQFSSystemModelImpl[])
            this.appModel.getAllSamQFSSystemModels();
        Host[] hosts = Host.getConfig(model.getJniContext(), fsName);
        for (int j = 0; j < allModels.length; j++) {
            boolean match = false;

            SamQFSSystemModelImpl crtModel = allModels[j];
	    if (crtModel == null) {
                TraceUtil.trace1("model " + j + " is null. skipping");
		continue;
	    }
	    if (!crtModel.getServerProductVersion().equals(mdsVersion)) {
		continue;
	    }

            String name = crtModel.getHostname();
	    String uname = crtModel.getServerHostname();

            for (int i = 0; i < hosts.length; i++) {
                if (hosts[i].getName().equals(name) ||
                    hosts[i].getName().equals(uname)) {
                    match = true;
                    break;
                }
            }

            if (!match && !crtModel.isDown()) {
                hostList.add(name);
            }
        }

        return (String[]) hostList.toArray(new String[0]);
    }


    public String[] getIPAddresses(String hostName) throws SamFSException {

        SamQFSSystemModelImpl model = (SamQFSSystemModelImpl)
            this.appModel.getSamQFSSystemModel(hostName);

        if (model.isDown()) {
            throw new SamFSException("logic.hostIsDown");
        }
        return Host.discoverIPsAndNames(model.getJniContext());
    }


    public SharedMember createSharedMember(String name, String[] ips,
        int type) {
        return new SharedMemberImpl(name, ips, type, false);
    }


    public MDSAddresses [] getAdvancedNetworkConfig(
        String hostName, String fsName) throws SamFSException {

        SamQFSSystemModelImpl model = (SamQFSSystemModelImpl)
            this.appModel.getSamQFSSystemModel(hostName);

        if (model.isDown()) {
            throw new SamFSException("logic.hostIsDown");
        }

        if (fsName == null || fsName.length() == 0) {
            throw new SamFSException("logic.invalidFS");
        }

        String [] properties =
            Host.getAdvancedNetCfg(model.getJniContext(), fsName);

        MDSAddresses [] myAddresses = new MDSAddresses[properties.length];

        for (int i = 0; i < properties.length; i++) {
            myAddresses[i] = new MDSAddresses(properties[i]);
        }

        return myAddresses;
    }

    private void setAdvancedNetworkConfig(
        String hostName, String fsName, MDSAddresses [] addresses)
        throws SamFSException {

        SamQFSSystemModelImpl model = (SamQFSSystemModelImpl)
            this.appModel.getSamQFSSystemModel(hostName);

        if (model.isDown()) {
            throw new SamFSException("logic.hostIsDown");
        }

        if (addresses == null || addresses.length == 0) {
            throw new SamFSException("logic.emptynetworkconfig");
        }

        String [] hosts = new String[addresses.length];
        for (int i = 0; i < addresses.length; i++) {
            hosts[i] = addresses[i].toString();
        }

        Host.setAdvancedNetCfg(model.getJniContext(), fsName, hosts);
    }

    public void setAdvancedNetworkConfigToMultipleHosts(
        String [] hostNames, String fsName,
        String mdsServer, MDSAddresses [] addresses)
        throws SamFSMultiHostException, SamFSException {

        if (hostNames == null || hostNames.length == 0) {
            throw new SamFSMultiHostException("Developers' bug found!");
        }

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        for (int i = 0; i < hostNames.length; i++) {
            try {
                setAdvancedNetworkConfig(hostNames[i], fsName, addresses);
            } catch (SamFSException e) {
               errorHostNames.add(hostNames[i]);
               errorExceptions.add(e);
            }
        }

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        configureHostFileIfNecessary(fsName, mdsServer, addresses);
    }

    /**
     * This method checks if the hosts.<fs_name> already has the IP defined
     * that the user selects in the Advanced Network Configuration Page.  This
     * method will add the IP of the MDS if it is not found in the hosts.fs
     * file.
     */
    private void configureHostFileIfNecessary(
        String fsName, String mdsServer, MDSAddresses [] addresses)
        throws SamFSException {

        if (fsName == null || addresses == null || fsName.length() == 0 ||
            addresses.length == 0) {
            return;
        }

        // retrieve all shared members of this shared file system
        SharedMember[] sharedMember = getSharedMembers(mdsServer, fsName);

        // Check if each MDSAddress object to see if there is any new IP
        // addresses that needs to be added to host.fs
        for (int j = 0; j < addresses.length; j++) {
            String hostName = addresses[j].getHostName();

            for (int i = 0; i < sharedMember.length; i++) {
                if (hostName.equals(sharedMember[i].getHostName())) {
                    // get the IP array with the new IPs that needs to be added
                    String [] newIPs =
                        compareIPs(
                            sharedMember[i].getIPs(),
                            addresses[j].getIPAddress());
                    // Only call addHost if there are any new IPs added.
                    if (newIPs.length != sharedMember[i].getIPs().length) {
                        addIPToHost(fsName, mdsServer, hostName, newIPs);
                    }
                }
            }
        }

    }

    /**
     * Call Host.addToConfig to update host.fs file
     */
    private void addIPToHost(
        String fsName, String mdServerName, String hostName, String [] newIPs)
        throws SamFSException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        // STEP 1: get the model for the mds
        SamQFSSystemModelImpl mdServerModel =
            getSystemModel(mdServerName, errorHostNames, errorExceptions);

        if (!errorHostNames.isEmpty()) {
            // Simply throw the first SamFSException in the ArrayList
            throw (SamFSException) errorExceptions.get(0);
        }

        // STEP 2: retrieve The hosts information from the mds.
        Host newHost = null;
        FSInfo mdsInfo = FS.get(mdServerModel.getJniContext(), fsName);
        Host[] hosts = mdsInfo.getHosts();

        // Find the host you are trying to change.
        // Modify its ip addresses.
        for (int i = 0; i < hosts.length; i++) {
            if (hosts[i].getName().equals(hostName)) {
                newHost =
                    new Host(hosts[i].getName(), newIPs,
                             hosts[i].getSrvPrio(),
                             hosts[i].isCrtServer());
                break;
            }
        }
        if (newHost == null) {
            throw new SamFSException(
                "Failed to add IP to the corresponding metadata server");
        }

        // STEP 3: Make the call to the mds to modify the hosts configuration.
        // Add the modified host to the MD Server's config file
        Host.addToConfig(mdServerModel.getJniContext(), fsName, newHost);
    }

    /**
     * Helper function of configureHostFileIfNecessary.
     * To return a new array that contains all the existing IP plus any new
     * IPs.
     */
    private String [] compareIPs(String [] existingIPs, String [] newIPs) {
        StringBuffer buf = new StringBuffer();
        boolean addNew = true;

        for (int i = 0; i < newIPs.length; i++) {
            for (int j = 0; j < existingIPs.length; j++) {
                if (newIPs[i].equals(existingIPs[j])) {
                    addNew = false;
                    break;
                }
            }
            // Need to add this IP
            if (addNew) {
                if (buf.length() > 0) buf.append("###");
                buf.append(newIPs[i]);
            }
            addNew = true;
        }

        if (buf.length() == 0) {
            // nothing to add
            return existingIPs;
        }

        String [] needToAdd = buf.toString().split("###");
        String [] resultIPs =
            new String[existingIPs.length + needToAdd.length];

        for (int k = 0; k < existingIPs.length; k++) {
            resultIPs[k] = existingIPs[k];
        }
        for (int k = 0; k < needToAdd.length; k++) {
            resultIPs[k + existingIPs.length] = needToAdd[k];
        }

        return resultIPs;
    }

    protected boolean matchAUs(DiskDev dc1, DiskDev dc2) {
	boolean match = false;
	AU au1, au2;
	au1 = dc1.getAU();
        au2 = dc2.getAU();
	if (au1.getSCSIDevInfo() != null &&
	    au2.getSCSIDevInfo() != null)
	    /* if same disk */
	    if (0 ==
		au1.getSCSIDevInfo().devID.compareTo(
		au2.getSCSIDevInfo().devID)) {
                /* if same slice (compare last char of the path) */
		if (au1.getPath().charAt(au1.getPath().length() - 1) ==
		    au2.getPath().charAt(au2.getPath().length() - 1)) {
		    match = true;
                }
	    }
	return match;
    }
    private DiskDev[] fixDiskPath(DiskDev[] disksUsed,
        DiskCache[] disksKnown) throws SamFSException {

        for (int i = 0; i < disksUsed.length; i++) {
            boolean matched = false;
            for (int j = 0; j < disksKnown.length; j++) {
                DiskCacheImpl dci = (DiskCacheImpl) disksKnown[j];
                if (matchAUs(disksUsed[i], dci.getJniDisk())) {
                    disksUsed[i].setDevicePath(dci.getDevicePath());
                    matched = true;
                    continue;
                }
            }
            if (!matched) {
                throw new SamFSException("logic.internal.no-matching-disk");
            }
        }
        return disksUsed;
    }


    protected void parallelDisco(String[] servers, String[] clients,
        DiskCache[][] allDCs, /* put the result in here */
        ArrayList errHostNames, ArrayList errExceptions,
        boolean ignoreHA) throws SamFSMultiHostException {

	int n; // thread index
	String[] hosts = new String[servers.length +
				   ((clients != null) ? clients.length : 0)];
	for (n = 0; n < servers.length; n++)
	    hosts[n] = servers[n];
	if (clients != null) {
	    for (int i = 0; i < clients.length; i++, n++) {
		hosts[n] = clients[i];
            }
        }

        // create barrier object
        Barrier bar = new Barrier();

        // get threads from pool; specify barrier as the ThreadListener
        ThreadPoolMember[] t = new ThreadPoolMember[hosts.length];
	for (n = 0; n < hosts.length; n++) {
	    t[n] = threadPool.getNewThread(bar, "disco-" + n);
        }

	// add threads to barrier
	bar.addThreads(t);

	/*
	 * run discovery without filtering on all clients, since they are not
	 * required to see the shared metadata storage but if they do, then we
	 * need to make sure that they are not already using that storage.
	 *
	 * run discovery+filtering on metadata servers
	 */
        try {
            Object[] methodArgs;
	    SamQFSSystemModel model;

	    for (n = 0; n < hosts.length; n++) {
                methodArgs = new Object[3];
		try {
		    model = this.appModel.getSamQFSSystemModel(hosts[n]);
		} catch (SamFSException e) {
		    t[n].setThrowable(e); // set exception for this thread
		    continue;
		}

                methodArgs[0] = (n < servers.length)
                    ? Boolean.TRUE  // discover only avail. AU-s
                    : Boolean.FALSE; // discover all AU-s

		if (model.isClusterNode() && !ignoreHA) {
                    // may be optimized
                    methodArgs[1] = new String[] { hosts[n] };
                } else {
                    methodArgs[1] = null;
                }

                methodArgs[2] = Boolean.TRUE; // AU-s will be used for sharedfs

		t[n].startMethod(new MethodInfo("discoverAUs",
		    model.getSamQFSSystemFSManager(),
                    methodArgs));
	    }
        } catch (NoSuchMethodException nsme) {
            TraceUtil.trace1(
                "NoSuchMethodException caught in parallelDisco()!");
            throw new SamFSMultiHostException(nsme.getMessage());
        }

	// wait for all threads to finish
        bar.waitForAll();

	for (n = 0; n < hosts.length; n++) {
	    Throwable e;
	    // check if an exception occured in thread n
	    if (null != (e = t[n].getThrowable())) {
		errHostNames.add(hosts[n]);
		errExceptions.add(e);
	    } else {
		// no exception, get the result
		DiskCache[] dc = (DiskCache[]) t[n].getResult();
		TraceUtil.trace1(dc.length + " AU-s discovered");
		allDCs[n] = new DiskCacheImpl[dc.length];
		for (int d = 0; d < dc.length; d++)
		    allDCs[n][d] = (DiskCacheImpl) dc[d];
	    }
	}

	threadPool.releaseAllToPool();
    }

    private Host[] membersToHosts(SharedMember[] members,
				  SamQFSSystemModelImpl[] models) {

        Host[] hosts = new Host[members.length];
        int potentialMDServerPriority = 2;
        boolean isCurrentMDServer = false;
        for (int i = 0; i < hosts.length; i++) {
            int priority = 0;
            isCurrentMDServer = false;
            switch (members[i].getType()) {
                case SharedMember.TYPE_MD_SERVER:
                     priority = 1;
                     isCurrentMDServer = true;
                     break;
                case SharedMember.TYPE_POTENTIAL_MD_SERVER:
                     priority = potentialMDServerPriority++;
                     break;
                case SharedMember.TYPE_CLIENT:
                     priority = 0;
                     break;
            }

            hosts[i] = new Host(
                models[i].getServerHostname(),
                members[i].getIPs(),
                priority,
                isCurrentMDServer);
        }
        return hosts;
    }

    private String getDevicePathForHost(
        SharedDiskCache sdc, SharedMember member) {
        String[] devPaths = null;
        String[] hostNames = null;
        switch (member.getType()) {
            case SharedMember.TYPE_MD_SERVER:
            case SharedMember.TYPE_POTENTIAL_MD_SERVER:
                 devPaths = sdc.getServerDevpaths();
                 hostNames = sdc.availFromServers();
                 break;
            case SharedMember.TYPE_CLIENT:
                 devPaths = sdc.getClientDevpaths();
                 hostNames = sdc.availFromClients();
                 break;
        }

        String memberName = member.getHostName();
        String path = null;
        for (int i = 0; i < devPaths.length; i++) {
            if (hostNames[i].equals(memberName)) {
               path = devPaths[i];
               break;
            }
        }
        return path;
    }

    private void setDevicePaths(SharedDiskCache[] metaSDC, DiskDev[] meta,
        SharedDiskCache[] dataSDC, DiskDev[] data,
        StripedGroup[] stripedGroups, StripedGrp[] grp,
        SharedMember member) {

        for (int j = 0; j < meta.length; j++) {
            meta[j].setDevicePath(getDevicePathForHost(metaSDC[j], member));
        }

        if (data != null && dataSDC != null) {
            for (int j = 0; j < data.length; j++) {
                data[j].setDevicePath(getDevicePathForHost(dataSDC[j], member));
            }
        }

        if (stripedGroups != null) {
            for (int j = 0; j < stripedGroups.length; j++) {
                SharedDiskCache[] grpSDC = (SharedDiskCache[])
                    stripedGroups[j].getMembers();
                DiskDev[] disks = grp[j].getMembers();
                for (int k = 0; k < disks.length; k++) {
                    disks[k].setDevicePath(
                        getDevicePathForHost(grpSDC[k], member));
                }
            }
        }
    }
}
