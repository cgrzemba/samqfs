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

// ident	$Id: SamQFSSystemMediaManagerImpl.java,v 1.22 2008/03/17 14:43:46 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.mgmt.media.Discovered;
import com.sun.netstorage.samqfs.mgmt.media.DriveDev;
import com.sun.netstorage.samqfs.mgmt.media.LibDev;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.media.NetAttachLibInfo;
import com.sun.netstorage.samqfs.mgmt.media.StkClntConn;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DiskVolumeImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DriveImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.LibraryImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.VSNImpl;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import java.util.ArrayList;


public class SamQFSSystemMediaManagerImpl implements SamQFSSystemMediaManager {

    private SamQFSSystemModelImpl theModel;

    public SamQFSSystemMediaManagerImpl(SamQFSSystemModel model) {
	theModel = (SamQFSSystemModelImpl) model;
    }


    public int[] getAvailableMediaTypes() throws SamFSException {

        String[] mTypes = Media.getAvailMediaTypes(theModel.getJniContext());
        int[] mVals = null;
        if (mTypes == null) {
            mVals = new int[0];
        } else {
            mVals = new int[mTypes.length];
            for (int i = 0; i < mTypes.length; i++) {
                mVals[i] = SamQFSUtil.getMediaTypeInteger(mTypes[i]);
            }
        }

        return mVals;
    }

    public int [] getAvailableArchiveMediaTypes() throws SamFSException {
        // retrieve tape media types from the server
        int [] mt = getAvailableMediaTypes();

        int [] mediaTypes = null;
        int index = 0;
        if (hasHoneyCombVSNs()) { // prepend disk & honey comb
            mediaTypes = new int[mt.length + 2];
            mediaTypes[index++] = BaseDevice.MTYPE_DISK;
            mediaTypes[index++] = BaseDevice.MTYPE_STK_5800;
        } else { // prepend disk only
            mediaTypes = new int[mt.length + 1];
            mediaTypes[index++] = BaseDevice.MTYPE_DISK;
        }

        // append tape media types to the new list
        for (int i = 0; i < mt.length; i++) {
            mediaTypes[index++] = mt[i];
        }


        return mediaTypes;
    }

    protected boolean hasHoneyCombVSNs() throws SamFSException {

        // retrieve all the disk volumes and determine if there is at least one
        // honeycomb vsn available
        DiskVolume [] vsn = getDiskVSNs();
        if (vsn == null || vsn.length == 0) {
            return false;
        }

        for (int i = 0; i < vsn.length; i++) {
            if (vsn[i].isHoneyCombVSN()) {
                return true;
            }
        }

        return false;
    }

    public Library[] discoverLibraries() throws SamFSException {

        SamFSConnection connection = theModel.getJniConnection();
        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_MAX_TIME_OUT);
        }
        Discovered dis = Media.discoverUnused(theModel.getJniContext());
        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_TIME_OUT);
        }

        LibDev[] libs = dis.libraries();
        ArrayList list = new ArrayList();
        if ((libs != null) && (libs.length > 0)) {
            for (int i = 0; i < libs.length; i++) {
                list.add(new LibraryImpl(theModel, libs[i], true));
            }
        }

        return (Library[]) list.toArray(new Library[0]);

    }

    public Library [] discoverSTKLibraries(StkClntConn [] conns)
        throws SamFSException {

        SamFSConnection connection = theModel.getJniConnection();
        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_MAX_TIME_OUT);
        }
        LibDev[] libs = Media.discoverStk(theModel.getJniContext(), conns);
        if (connection != null) {
            connection.setTimeout(SamQFSSystemModelImpl.
                                  DEFAULT_RPC_TIME_OUT);
        }

        ArrayList list = new ArrayList();
        if ((libs != null) && (libs.length > 0)) {
            for (int i = 0; i < libs.length; i++) {
                list.add(new LibraryImpl(theModel, libs[i], true));
            }
        }

        return (Library[]) list.toArray(new Library[0]);
    }


    public Library discoverNetworkLibrary(String libName, int mediaType,
                                          String catalog, String paramFile,
                                          int ordinal) throws SamFSException {

        int jniType = -1;
        switch (mediaType) {
        case BaseDevice.MTYPE_STK_ACSLS:
            jniType = NetAttachLibInfo.DT_STKAPI;
            break;
        case BaseDevice.MTYPE_ADIC_DAS:
            jniType = NetAttachLibInfo.DT_GRAUACI;
            break;
        case BaseDevice.MTYPE_SONY_PETASITE:
            jniType = NetAttachLibInfo.DT_SONYPSC;
            break;
        case BaseDevice.MTYPE_FUJ_LMF:
            jniType = NetAttachLibInfo.DT_LMF;
            break;
        case BaseDevice.MTYPE_IBM_3494:
            jniType = NetAttachLibInfo. DT_IBMATL;
            break;
        }

        if (ordinal == -1)
            ordinal = FSInfo.EQU_AUTO;

        NetAttachLibInfo nwaLib = new NetAttachLibInfo(libName, jniType,
                                                       FSInfo.EQU_AUTO,
                                                       catalog,
                                                       paramFile);
        LibDev lib = Media.getNetAttachLib(theModel.getJniContext(), nwaLib);
        if ((ordinal != FSInfo.EQU_AUTO) &&
            (lib.getEquipOrdinal() != ordinal)) {
            lib.setEquipOrdinal(ordinal);
        }
        Library newLib = null;
        if (lib != null)
            newLib = new LibraryImpl(theModel, lib, true);

        return newLib;

    }


    public Library[] getAllLibraries() throws SamFSException {

        ArrayList list = new ArrayList();
        LibDev[] libs = Media.getLibraries(theModel.getJniContext());
        if ((libs != null) && (libs.length > 0)) {
            for (int i = 0; i < libs.length; i++) {
                list.add(new LibraryImpl(theModel, libs[i], true));
            }
        }

        return (Library[]) list.toArray(new Library[0]);

    }


    public Library getLibraryByName(String libraryName) throws SamFSException {

        Library lib = null;

        if (SamQFSUtil.isValidString(libraryName)) {

            if ((libraryName.equals("historian")) ||
                (libraryName.equals("Historian"))) {

                Library[] libs = getAllLibraries();
                if (libs != null) {
                    for (int i = 0; i < libs.length; i++) {
                        String libName = libs[i].getName();
                        if (("historian".equals(libName)) ||
                            ("Historian".equals(libName))) {
                            lib = libs[i];
                            break;
                        }
                    }
                }
            } else {
                LibDev jni =
                    Media.getLibraryByFSet(theModel.getJniContext(),
                        libraryName);
                if (jni != null)
                    lib = new LibraryImpl(theModel, jni, true);
            }

        }

        return lib;

    }


    public Library getLibraryByEQ(int libraryEQ) throws SamFSException {

        Library lib = null;
        LibDev jni = Media.getLibraryByEq(theModel.getJniContext(), libraryEQ);
        if (jni != null)
            lib = new LibraryImpl(theModel, jni, true);

        return lib;

    }


    public void addLibrary(Library library) throws SamFSException {

        if (library == null)
            throw new SamFSException("logic.invalidLibrary");

        Media.addLibrary(theModel.getJniContext(),
                ((LibraryImpl) library).getJniLibrary());
    }


    public void addLibraries(Library [] libArray) throws SamFSException {

        if (libArray == null || libArray.length == 0)
            throw new SamFSException("logic.invalidLibrary");

        LibDev [] libs = new LibDev[libArray.length];
        for (int i = 0; i < libArray.length; i++) {
            LibDev myLib = ((LibraryImpl) libArray[i]).getJniLibrary();
            if (myLib == null) {
                throw new SamFSException("logic.invalidLibrary");
            } else {
                libs[i] = myLib;
            }
        }

        Media.addLibraries(theModel.getJniContext(), libs);
    }


    public void addLibrary(Library library, Drive [] includedDrives)
        throws SamFSException {

        if (library == null)
            throw new SamFSException("logic.invalidLibrary");

        LibDev lib = ((LibraryImpl) library).getJniLibrary();
        DriveDev[] drvs = new DriveDev[includedDrives.length];
        for (int i = 0; i < includedDrives.length; i++)
            drvs[i] = ((DriveImpl) includedDrives[i]).getJniDrive();
        lib.setDrives(drvs);
        Media.addLibrary(theModel.getJniContext(),
                         ((LibraryImpl) library).getJniLibrary());
    }


    public void addNetworkLibrary(Library library, Drive[] includedDrives)
        throws SamFSException {

        LibDev lib = ((LibraryImpl) library).getJniLibrary();
	if (includedDrives != null) {
        DriveDev[] drvs = new DriveDev[includedDrives.length];
        for (int i = 0; i < includedDrives.length; i++)
            drvs[i] = ((DriveImpl) includedDrives[i]).getJniDrive();
            lib.setDrives(drvs);
	}

        Media.addLibrary(theModel.getJniContext(), lib);
    }


    public void removeLibrary(Library library) throws SamFSException {

        // not sure whether the best default is true or false for unload here
        if (library != null) {
            Media.removeLibrary(theModel.getJniContext(),
                library.getEquipOrdinal(), false);
        }

    }

    public VSN[] searchVSNInLibraries(String vsnName) throws SamFSException {

        ArrayList list = new ArrayList();
        CatEntry[] cat = null;

        // Exception is caught here becuase API throws exception if no vsn
        // is found (instead of returning null)
        try {
            cat = Media.getCatEntriesForVSN(theModel.getJniContext(), vsnName);
        } catch (SamFSException e) {
            theModel.processException(e);
        }

        if ((cat != null) && (cat.length > 0)) {
            for (int i = 0; i < cat.length; i++) {
                list.add(new VSNImpl(theModel, cat[i]));
            }
        }

        return (VSN[]) list.toArray(new VSN[0]);

    }


    public VSN getVSN(CatEntry entry) throws SamFSException {

        return new VSNImpl(theModel, entry);

    }

    public DiskVolume[] getDiskVSNs() throws SamFSException {
            DiskVolume[] vols = null;
	    DiskVol[] nativeVols = DiskVol.getAll(theModel.getJniContext());

            if (nativeVols != null) {
                vols = new DiskVolumeImpl[nativeVols.length];
                for (int i = 0; i < nativeVols.length; i++) {
                    vols[i] = new DiskVolumeImpl(nativeVols[i]);
                }
            }
            return vols;
    }


    public DiskVolume getDiskVSN(String name) throws SamFSException {
        DiskVolumeImpl vol;
        DiskVol nativeVol;

        nativeVol = DiskVol.get(theModel.getJniContext(), name);
        vol = new DiskVolumeImpl(nativeVol);
        return vol;
    }


    public void createDiskVSN(String name, String remoteHost, String path)
        throws SamFSException {

        /*
         * Step 1/2: if remote volume, then add managed host to the client list
         * on remoteHost, and create the directory
         */
        if (remoteHost != null) {
            SamQFSSystemModelImpl modelForRHost = (SamQFSSystemModelImpl)
            SamQFSFactory.getSamQFSAppModel().getSamQFSSystemModel(remoteHost);
            String fullPath = "";
            if (path != null) {
                fullPath = path;
                if (!path.endsWith("/"))
                    fullPath += "/";
            }
            fullPath += name;
            if (SamQFSUtil.isValidString(fullPath)) {
                FileUtil.createDir(modelForRHost.getJniContext(), fullPath);
            }
            DiskVol.addClient(modelForRHost.getJniContext(),
                              theModel.getHostname());
        }
        /*
         * Step 2/2: create disk volume on the managed (current) host
         */
        DiskVol nativeVol = new DiskVol(name, remoteHost, path);
        DiskVol.add(theModel.getJniContext(), nativeVol);
    }

    public void createHoneyCombVSN(String name, String host, int port)
        throws SamFSException {
        String hostColonPort = host.concat(":").concat(Integer.toString(port));

        DiskVol nativeVol = new DiskVol(name, hostColonPort);
        DiskVol.add(theModel.getJniContext(), nativeVol);
    }

    public void updateDiskVSNFlags(DiskVolume vol)
        throws SamFSException {

	    ((DiskVolumeImpl)vol).updateFlags(theModel.getJniContext());

    }

    /**
     * Get Unusable VSNs
     *
     * @param eq   - eq of lib, if EQU_MAX, all lib
     * @param flag - status field bit flags
     *  - If 0, use default RM_VOL_UNUSABLE_STATUS
     *  (from src/archiver/include/volume.h)
     *  CES_needs_audit
     *  CES_cleaning
     *  CES_dupvsn
     *  CES_unavail
     *  CES_non_sam
     *  CES_bad_media
     *  CES_read_only
     *  CES_writeprotect
     *  CES_archfull
     * @return a list of formatted strings
     *  name=vsn
     *  type=mediatype
     *  flags=intValue representing flags that are set
     *
     */
    public String [] getUnusableVSNs(int eq, int flag) throws SamFSException {
        return Media.getVSNs(theModel.getJniContext(), eq, flag);
    }
}
