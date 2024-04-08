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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: SamQFSSystemMediaManagerImpl.java,v 1.26 2008/12/16 00:12:19 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.BaseVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.CatVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;
import com.sun.netstorage.samqfs.mgmt.arc.VSNOp;
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
import com.sun.netstorage.samqfs.web.model.media.VSNWrapper;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
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

    /**
     * Evaluate vsn expressions and return a list of VSNs that matches the
     * expression.  This method is currently strictly used by the VSN browser
     * pop up.
     *
     * @param mediaType - Type of media of which you want to evaluate potential
     *  VSN matches
     * @param policyName - Name of the policy of which you are adding to the
     *  VSN Pool. Leave this field null if you just want to evaluate the vsn
     *  expressions
     * @param copyNumber - Number of copy of which you are adding to the
     *  VSN Pool. Leave this field as -1 if you just want to evaluate the vsn
     *  expressions
     * @param startVSN - Start of VSN range, use null to get all VSNs
     * @param endVSN   - End of VSN range, use null to get all VSNs
     * @param expression - Contain regular expressions of which you want to
     *  evaluate potential VSN matches.  Leave this null or empty if you want to
     *  get all VSNs
     *
     * (Either enter startVSN and endVSN, or expression, or both null to catch
     *  all)
     *
     * @param maxEntry - Maximum number of entry you want to return
     * @return Array of VSNs that matches the input VSN expression(s)
     */
    public VSNWrapper evaluateVSNExpression(
        int mediaType, String policyName, int copyNumber,
        String startVSN, String endVSN, String expression,
        String poolName, int maxEntry)
        throws SamFSException {

        TraceUtil.trace2("Logic: Entering evaluateVSNExpression");

        expression = expression == null ? "" : expression.trim();
        startVSN   = startVSN   == null ? "" : startVSN.trim();
        endVSN = endVSN == null ? "" : endVSN.trim();

        TraceUtil.trace2("Logic: mediaType: " + mediaType);
        TraceUtil.trace2("Logic: startVSN: " + startVSN);
        TraceUtil.trace2("Logic: endVSN: " + endVSN);
        TraceUtil.trace2("Logic: expression: " + expression);
        TraceUtil.trace2("Logic: maxEntry: " + maxEntry);

        String [] expressionArray = null;

        if (startVSN.length() == 0 && endVSN.length() != 0) {
            startVSN = endVSN;
        } else if (startVSN.length() != 0 && endVSN.length() == 0) {
            endVSN = startVSN;
        }

        if (expression.length() == 0 &&
            startVSN.length() == 0 && endVSN.length() == 0 &&
            poolName == null) {
            // return empty
            return new VSNWrapper(new VSN[0], new DiskVolume[0], 0, 0, "");

        } else if (startVSN.length() != 0) {
            // Use start/end range
            String rangeExpression =
                SamQFSUtil.createExpression(startVSN, endVSN);
            if (rangeExpression == null) {
                throw new SamFSException(
                "Failed to generate range expression from start and end VSNs!");
            } else {
                expressionArray = rangeExpression.split(" ");
            }
        } else {
            // use user-defined expression
            expressionArray = expression.split(",");
        }

        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < expressionArray.length; i++) {
            if (buf.length() > 0) {
                buf.append(",");
            }
            buf.append(expressionArray[i]);

            TraceUtil.trace2(
                "expressionArray[" + i + "] is " + expressionArray[i]);
        }

        // Prepare vsnMap if we need to resolve an array of expressions,
        // except vsn pool
        VSNMap myVSNMap = null;

        if (poolName == null) {
            TraceUtil.trace2("Logic: Start building VSNMap!");

            // TODO: Check policyName and copyNumber here???
            myVSNMap =
                new VSNMap("<ignore_me>",
                           SamQFSUtil.getMediaTypeString(mediaType),
                           expressionArray,
                           null);
            TraceUtil.trace2("Logic: Done building VSNMap!");
            TraceUtil.trace2(
                "Logic: vsnmap.tostring: " + myVSNMap.toString());
        }

        long freeSpaceInMB = 0;
        VSNWrapper vsnWrapper = null;
        BaseVSNPoolProps mapProps = null;

        // Disk
        if (mediaType == BaseDevice.MTYPE_DISK ||
            mediaType == BaseDevice.MTYPE_STK_5800) {

            if (poolName == null) {
                mapProps =
                    VSNOp.getPoolPropsByMap(
                        theModel.getJniContext(),
                        myVSNMap,
                        0,
                        maxEntry,
                        Media.VSN_NO_SORT,
                        false);
            } else {
                mapProps =
                    VSNOp.getPoolPropsByPool(
                        theModel.getJniContext(),
                        VSNOp.getPool(theModel.getJniContext(), poolName),
                        0,
                        maxEntry,
                        Media.VSN_NO_SORT,
                        false);
            }

            freeSpaceInMB = mapProps.getFreeSpace();
            DiskVol [] dvols =
                ((DiskVSNPoolProps) mapProps).getDiskEntries();

            TraceUtil.trace2("Total DiskVol: " + dvols.length);

            DiskVolume [] vols = null;
            if (dvols != null) {
                vols = new DiskVolumeImpl[dvols.length];
                for (int i = 0; i < dvols.length; i++) {
                    vols[i] = new DiskVolumeImpl(dvols[i]);
                }
            }
            vsnWrapper = new VSNWrapper(
                            null, vols, freeSpaceInMB,
                            mapProps.getNumOfVSNs(),
                            buf.toString());

        // Tape
        } else {
            if (poolName == null) {
                mapProps =
                    VSNOp.getPoolPropsByMap(
                        theModel.getJniContext(),
                        myVSNMap,
                        0,
                        maxEntry,
                        Media.VSN_NO_SORT,
                        false);
            } else {
                mapProps =
                    VSNOp.getPoolPropsByPool(
                        theModel.getJniContext(),
                        VSNOp.getPool(theModel.getJniContext(), poolName),
                        0,
                        maxEntry,
                        Media.VSN_NO_SORT,
                        false);
            }

            freeSpaceInMB = mapProps.getFreeSpace();
            CatEntry [] cats =
               ((CatVSNPoolProps) mapProps).getCatEntries();

            TraceUtil.trace2("Total CatEntry: " + cats.length);

            VSN [] allVSNs = new VSN[cats.length];
            if ((cats != null) && (cats.length > 0)) {
                for (int i = 0; i < cats.length; i++) {
                    allVSNs[i] = new VSNImpl(theModel, cats[i]);
                }
            }
            vsnWrapper = new VSNWrapper(
                            allVSNs, null, freeSpaceInMB,
                            mapProps.getNumOfVSNs(),
                            buf.toString());
        }

        TraceUtil.trace2("Logic: Returning vsnWrapper!");

        return vsnWrapper;

    }

}
