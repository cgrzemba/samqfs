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

// ident	$Id: LibraryImpl.java,v 1.47 2008/12/16 00:12:21 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;

import com.sun.netstorage.samqfs.mgmt.media.StkClntConn;
import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.Fault;
import com.sun.netstorage.samqfs.mgmt.adm.FaultAttr;
import com.sun.netstorage.samqfs.mgmt.media.LibDev;
import com.sun.netstorage.samqfs.mgmt.media.DriveDev;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.mgmt.media.ImportOpts;
import com.sun.netstorage.samqfs.mgmt.media.StkPool;
import com.sun.netstorage.samqfs.mgmt.media.StkVSN;
import com.sun.netstorage.samqfs.web.model.Common;
import com.sun.netstorage.samqfs.web.model.ImportVSNFilter;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.impl.jni.alarm.AlarmImpl;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;
import java.util.HashMap;

public class LibraryImpl extends BaseDeviceImpl implements Library {


    private SamQFSSystemModelImpl model = null;
    private LibDev jniLib = null;

    private String name = new String();
    private ArrayList drives = new ArrayList();

    private HashMap vsns = new HashMap();

    private String vendor = new String();
    private String productID = new String();
    private String serialNo = new String();
    private int status = -1;
    private int noOfLicensedSlots = -1;
    private int noOfAvailableSlots = -1;
    private int noOfCatalogEntries = -1;
    private int driverType = -1;
    private String catalogPath = new String();
    private String paramFileLocation = new String();
    private String firmware = new String();
    private ArrayList associatedAlarms = new ArrayList();
    private int[] detailedStatus = new int[0];
    private int driveStartEQ = -1, driveIncreEQ = -1;


    public LibraryImpl() {
    }


    // To construct logic layer library object by passing in LibDev
    public LibraryImpl(SamQFSSystemModelImpl model, LibDev jniLib,
                       boolean discoveryCase)
        throws SamFSException {

        this.model = model;
        this.jniLib = jniLib;
        setup(discoveryCase);

    }

    public LibDev getJniLibrary() {

        return jniLib;

    }


    public String getName() throws SamFSException {

	return name;

    }


    // Explicitly used in Add Library Wizard ONLY
    public void setName(String name) throws SamFSException {

        if ((name != null) && (name != new String())) {
            this.name = name;
	    super.setFamilySetName(name);
	    if (jniLib != null)
		jniLib.setFamilySetName(name);

	    Drive[] drvs = getDrives();
	    if (drvs != null)
		for (int i = 0; i < drvs.length; i++)
		    ((DriveImpl) drvs[i]).setName(name);
	}

    }


    public Drive[] getDrives() throws SamFSException {

        // Update drive information before returning drive objects
        setup(false);
	return (Drive[]) drives.toArray(new Drive[0]);

    }

    public void setDrives(Drive[] newDrives) throws SamFSException {

        drives.clear();
        if ((newDrives != null) && (newDrives.length > 0)) {
            for (int i = 0; i < newDrives.length; i++) {
                drives.add(newDrives[i]);
            }
        }

    }


    public VSN[] getVSNs() throws SamFSException {

        vsns.clear();

        if ((model != null) && (jniLib != null)) {

            CatEntry[] cats =
                Media.getAllCatEntriesForLib(model.getJniContext(),
                                             jniLib.getEquipOrdinal(), -1, -1,
                                             Media.VSN_NO_SORT, false);

            if ((cats != null) && (cats.length > 0)) {
                for (int i = 0; i < cats.length; i++) {
                    vsns.put(cats[i].getVSN(), new VSNImpl(model, cats[i]));
                }
            }

        }

	return (VSN[]) vsns.values().toArray(new VSN[0]);

    }


    public int getTotalVSNInLibrary() throws SamFSException {

        return Media.getNumberOfCatEntries(model.getJniContext(),
                                           jniLib.getEquipOrdinal());

    }


    public VSN[] getVSNs(int start, int size, int sortby, boolean ascending)
        throws SamFSException {

        VSN[] vsnList = new VSN[0];

        if ((model != null) && (jniLib != null)) {

            CatEntry[] cats =
                Media.getAllCatEntriesForLib(model.getJniContext(),
                                             jniLib.getEquipOrdinal(), start,
                                             size, (short) sortby, ascending);
            if ((cats != null) && (cats.length > 0)) {
                vsnList = new VSN[cats.length];
                for (int i = 0; i < cats.length; i++) {
                    vsnList[i] = new VSNImpl(model, cats[i]);
                }
            }

        }

	return vsnList;

    }

    public VSN getVSN(String vsn) throws SamFSException {

        VSN vsnObj = null;

	if ((SamQFSUtil.isValidString(vsn)) && (model != null)) {

            CatEntry[] list = Media.getCatEntriesForVSN(model.getJniContext(),
                                                        vsn);
            if ((list != null) && (list.length > 0)) {
                if (list.length == 1) {
                    vsnObj = new VSNImpl(model, list[0]);
                } else {
                    // more than one vsn exists for the given vsn name
                    int targetEq = getFamilySetEquipOrdinal();
                    int match = -1;
                    for (int i = 0; i < list.length; i++) {
                        if (targetEq == list[i].getLibraryEqu()) {
                            match = i;
                            break;
                        }
                    }
                    if (match != -1)
                        vsnObj = new VSNImpl(model, list[match]);
                }
            }

        }

        return vsnObj;

    }


    public VSN getVSN(int slotNo) throws SamFSException {

        VSN vsnObj = null;

        if (model != null) {

            CatEntry cat = Media.getCatEntryForSlot(model.getJniContext(),
                                                    getEquipOrdinal(),
                                                    slotNo, 0);
            if (cat != null)
                vsnObj = new VSNImpl(model, cat);

        }

        return vsnObj;

    }

    public String getVendor() throws SamFSException {

	return vendor;

    }


    public String getProductID() throws SamFSException {

	return productID;

    }


    public String getSerialNo() throws SamFSException {

	return serialNo;

    }


    // this is the real status
    public int[] getDetailedStatus() {

        return detailedStatus;

    }


    public int getNoOfLicensedSlots() throws SamFSException {

	return noOfLicensedSlots;

    }


    public int getNoOfAvailableSlots() throws SamFSException {

	return noOfAvailableSlots;

    }


    public int getDriverType() throws SamFSException {

	return driverType;

    }


    public void setDriverType(int driverType) throws SamFSException {

        this.driverType = driverType;

    }


    public String getFirmwareLevel() throws SamFSException {

        return firmware;

    }


    public long getTotalCapacity() throws SamFSException {

        long total = -1;
        if ((model != null) && (jniLib != null)) {
            total = Media.getLibCapacity(
                        model.getJniContext(), jniLib.getEquipOrdinal());
        }

        return total;

    }


    public long getTotalFreeSpace() throws SamFSException {

        long total = 0;
        if ((model != null) && (jniLib != null)) {
            total = Media.getLibFreeSpace(
                        model.getJniContext(), jniLib.getEquipOrdinal());
        }

        return total;

    }


    public int getMediaType() throws SamFSException {

        int mediaType = -1;
        if (jniLib != null)
            mediaType = SamQFSUtil.getEquipTypeInteger(jniLib.getEquipType());

        return mediaType;

    }


    public String getCatalogLocation() throws SamFSException {

	return catalogPath;

    }


    public void setCatalogLocation(String catalogLocation)
        throws SamFSException {
        if ((catalogLocation != null) && (catalogLocation != new String())) {
            this.catalogPath = catalogLocation;
            if (jniLib != null)
                jniLib.setCatalogPath(catalogLocation);
        }
    }


    public String getParamFileLocation() throws SamFSException {

	return paramFileLocation;

    }

    public Alarm[] getAssociatedAlarms() throws SamFSException {

        FaultAttr[] faults = Fault.getByLibName(model.getJniContext(),
                                                getName());
        associatedAlarms.clear();
        if ((faults != null) && (faults.length > 0)) {
            for (int i = 0; i < faults.length; i++) {
                AlarmImpl a = new AlarmImpl(model, faults[i]);
                associatedAlarms.add(a);
            }
        }

        return (Alarm[]) associatedAlarms.toArray(new Alarm[0]);

    }

    public void importVSN() throws SamFSException {

        if ((model != null) && (jniLib != null)) {
            Media.importCartridge(model.getJniContext(),
                                  jniLib.getEquipOrdinal(),
                                  null);
        }

    }


    public void importVSNInACSLS(long poolId, int count)
        throws SamFSException {

        if ((model != null) && (jniLib != null) &&
            (poolId >= 0) && (count > 0)) {

            ImportOpts opt = new ImportOpts(null, null, null, false, false,
                                            count, poolId);
            Media.importCartridge(model.getJniContext(),
                                  jniLib.getEquipOrdinal(),
                                  opt);
        }

    }


    public void importVSNInNWALib(String startVSN, String endVSN)
        throws SamFSException {

        if ((model != null) && (jniLib != null)) {

            if ((SamQFSUtil.isValidString(startVSN)) &&
                (!SamQFSUtil.isValidString(endVSN))) {

                ImportOpts opt = new ImportOpts(startVSN, null, null, false,
                                                false, -1, -1);
                Media.importCartridge(model.getJniContext(),
                                      jniLib.getEquipOrdinal(),
                                      opt);

            } else if ((SamQFSUtil.isValidString(startVSN)) &&
                       (SamQFSUtil.isValidString(endVSN))) {

                ImportOpts opt = new ImportOpts(startVSN, null, null,
                                                false, false, -1, -1);
                if (startVSN.equals(endVSN)) {
                    Media.importCartridge(model.getJniContext(),
                                          jniLib.getEquipOrdinal(),
                                          opt);
                } else {
                    Media.importCartridges(model.getJniContext(),
                                           jniLib.getEquipOrdinal(), startVSN,
                                           endVSN, opt);
                }
            }
        }
    }


    public void unload() throws SamFSException {

        if ((model != null) && (jniLib != null)) {
            Media.unload(model.getJniContext(), jniLib.getEquipOrdinal(),
                         false);
        }
    }

    public String toString() {

        StringBuffer buf = new StringBuffer();
        buf.append("Name: " + name + "\n");

        buf.append("Base Device: " + super.toString() + "\n");
        buf.append("Vendor: " + vendor + "\n");
        buf.append("Product ID: " + productID + "\n");
        buf.append("Serial No: " + serialNo + "\n");
        buf.append("Status: " + status + "\n");
        buf.append("No OfLicensed Slots: " + noOfLicensedSlots + "\n");
        buf.append("No Of Available Slots: " + noOfAvailableSlots + "\n");
        buf.append("Catalog Location: " + catalogPath + "\n");

        buf.append("\nDrives: \n");
	if (drives != null) {
            try {
		for (int i = 0; i < drives.size(); i++)
		    buf.append(((Drive)(drives.get(i))).toString()  + "\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
	}

        buf.append("\nVSNs: \n");
	if (vsns != null) {
            VSN[] vsnList = (VSN[]) vsns.values().toArray(new VSN[0]);
            try {
		for (int i = 0; i < vsnList.length; i++)
		    buf.append(vsnList[i].toString()  + "\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
	}

        buf.append("\nAlarms: \n");
	if (associatedAlarms != null) {
            try {
		for (int i = 0; i < associatedAlarms.size(); i++)
		    buf.append(((Alarm)associatedAlarms.get(i)).toString()  +
                               "\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
	}

        return buf.toString();

    }

    private void setup(boolean discoveryCase) throws SamFSException {

        if ((model != null) && (jniLib != null)) {

            String devPath = jniLib.getDevicePath();
            if (!SamQFSUtil.isValidString(devPath)) {
                String[] altPaths = jniLib.getAlternatePaths();
                if ((altPaths != null) && (altPaths.length > 0)) {
                    devPath = altPaths[0];
                }
            }
            super.setDevicePath(devPath);
            jniLib.setPath(devPath);
            super.setEquipOrdinal(jniLib.getEquipOrdinal());
            super.setEquipType(SamQFSUtil.
                               getEquipTypeInteger(jniLib.getEquipType()));
            super.setFamilySetName(jniLib.getFamilySetName());
            super.setFamilySetEquipOrdinal(jniLib.
                                           getFamilySetEquipOrdinal());
            super.setState(SamQFSUtil.convertStateToUI(jniLib.getState()));
            super.setAdditionalParamFilePath(
                                        jniLib.getAdditionalParamFilePath());
            super.setLogPath(jniLib.getLogPath());
            super.setLogModTime(jniLib.getLogModTime());

            name = jniLib.getFamilySetName();
            if ("historian".equals(devPath))
                name = "historian";

            drives.clear();
            DriveDev[] drvs = jniLib.getDrives();
            if ((model != null) && (drvs != null) && (drvs.length > 0)) {
                for (int i = 0; i < drvs.length; i++) {
                    DriveImpl tempDrive = new DriveImpl(model, drvs[i]);
                    String path = tempDrive.getDevicePath();
                    if (discoveryCase) {
                        if (SamQFSUtil.isValidString(path))
                            drives.add(tempDrive);
                    } else
                        drives.add(tempDrive);
                }
            }

            // exclude the drives that have null path from JNI
            // again, to workaround Ming's discovery issue
            // probably could be more efficient. Later.
            if (discoveryCase) {
                LibDev lib = getJniLibrary();
                Drive[] drives = getDrives();
                DriveDev[] drvsJni = new DriveDev[drives.length];
                for (int i = 0; i < drives.length; i++)
                    drvsJni[i] = ((DriveImpl) drives[i]).getJniDrive();
                lib.setDrives(drvsJni);
            }

            vendor = jniLib.getVendor();
            productID = jniLib.getProductID();
            serialNo = jniLib.getSerialNum();
            status = SamQFSUtil.convertStateToUI(jniLib.getState());
            detailedStatus =
                SamQFSUtil.
                    getRemovableMediaStatusIntegers(jniLib.getStatus());

            noOfLicensedSlots = 0;
            noOfAvailableSlots = 0;

            driverType =
                SamQFSUtil.getDriverTypeFromEQType(jniLib.getEquipType());
            catalogPath = jniLib.getCatalogPath();
            paramFileLocation = jniLib.getAdditionalParamFilePath();
            firmware = jniLib.getFirmwareLevel();

            // DO NOT get slot information here because sam-catserverd MAY NOT
            // run at this point.  Add Library Wizard will use this routine
            // when the library structure gets converted into Library Objects
        }

    }


    // Need to over-ride these methods

    public void setEquipOrdinal(int equipOrdinal) {

	super.setEquipOrdinal(equipOrdinal);
        if (jniLib != null)
            jniLib.setEquipOrdinal(equipOrdinal);

    }


    public void setEquipType(int equipType) {

	super.setEquipType(equipType);
        if (jniLib != null)
            jniLib.setEquipType(SamQFSUtil.getMediaTypeString(equipType));

    }


    public void setFamilySetName(String familySetName) {

        if (familySetName != null) {
            super.setFamilySetName(familySetName);
            if (jniLib != null)
                jniLib.setFamilySetName(familySetName);
        }

    }


    public void setFamilySetEquipOrdinal(int familySetEquipOrdinal) {

        super.setFamilySetEquipOrdinal(familySetEquipOrdinal);
        if (jniLib != null)
            jniLib.setFamilySetEquipOrdinal(familySetEquipOrdinal);

    }


    public void setState(int state) {

	super.setState(state);
        if (jniLib != null)
            jniLib.setState(SamQFSUtil.convertStateToJni(state));

    }


    public void setAdditionalParamFilePath(String path) {

        if (path != null) {
            super.setAdditionalParamFilePath(path);
            if (jniLib != null)
                jniLib.setAdditionalParamFilePath(path);
        }

    }


    public String[] getMessages() {

        String[] msg = null;

        if (jniLib != null) {
            msg = jniLib.getMessages();
        }
        if (msg == null) {
            msg = new String[0];
        }

        return msg;

    }

    public int getNoOfCatalogEntries() throws SamFSException {

        noOfCatalogEntries =
		Media.getNumberOfCatEntries(model.getJniContext(),
					    jniLib.getEquipOrdinal());

        return noOfCatalogEntries;
    }

    /**
     * This method returns a boolean to indicate if the library contains
     * mixed media type.  The media types are defined in BaseDevice class.
     *
     * @return return true or false to indicate if the library has mixed media
     */
    public boolean containMixedMedia() {

        if (drives == null || drives.size() == 0) {
            return false;
        }

        int mediaType = -1, checkType = -1;

        for (int i = 0; i < drives.size(); i++) {
            if (mediaType != -1) {
                checkType = ((DriveImpl) drives.get(i)).getEquipType();
            } else {
                mediaType = ((DriveImpl) drives.get(i)).getEquipType();
                continue;
            }
            if (checkType != mediaType) {
                return true;
            }
        }

        return false;
    }

    /**
     * The following two methods are getter and setter for STK Library Parameter
     * Object resides in the Library Object.
     */
    public StkNetLibParam getStkNetLibParam() throws SamFSException {

        return jniLib.getStkParam();
    }

    public void setStkNetLibParam(StkNetLibParam newParam)
        throws SamFSException {

        jniLib.setStkParam(newParam);
    }

    public int getDriveStartNumber() {

        return driveStartEQ;
    }

    public void setDriveStartNumber(int start) {

        this.driveStartEQ = start;
    }

    public int getDriveIncreNumber() {

        return driveIncreEQ;
    }

    public void setDriveIncreNumber(int incre) {

        this.driveIncreEQ = incre;
    }

    // setDevicePath is used only when adding ACSLS Library from Version 4.5.
    // Device Path is no longer returned from media discovery and presentation
    // layer needs to set the device path so the C-layer knows where to write
    // the parameter file on the library.
    public void setDevicePath(String devicePath) {
        super.setDevicePath(devicePath);
        jniLib.setPath(devicePath);
    }

    /**
     * This method returns a StkPhyConf Object, a wrapper of StkPool,
     * two int [] structure, and 4 int. GUI needs to call this API to
     * pre-populate the Filter for Import VSN.
     */
    public StkPool[] getPhyConfForStkLib() throws SamFSException {

        return
            Media.getStkScratchPools(
                model.getJniContext(),
                getDriveMediaType(),
                getStkClntConn());
    }


    /**
     * This method returns an array of StkVSN in order to populate the
     * action table in the Import VSN page for 4.5+ ACSLS Servers.  The GUI
     * still needs to exclude the VSNs that are
     * 1. Already in used
     * 2. In drive
     * and take away the checkbox to prevent users from importing them.
     */
    public StkVSN [] getVSNsForStkLib(String filter) throws SamFSException {

        ImportVSNFilter myFilter = new ImportVSNFilter(filter);
        myFilter.setEQType(getDriveMediaType());

        return
            Media.getVSNsForStkLib(
                model.getJniContext(),
                getStkClntConn(),
                myFilter.toString());
    }

    /**
     * This method returns a hashmap that contains VSN Names that reside
     * in a STK Library.  This is used to provide information for
     * the logic layer so it knows which VSNs are in used that they need to be
     * excluded from the selection list.
     */
    public HashMap getVSNNamesForStkLib() throws SamFSException {

        String [] vsnNames =
            Media.getVSNNamesForStkLib(
                model.getJniContext(),
                getDriveMediaType());

        HashMap myMap = new HashMap();

        for (int i = 0; i < vsnNames.length; i++) {
            myMap.put(vsnNames[i], null);
        }

        return myMap;
    }

    private StkClntConn getStkClntConn() throws SamFSException {

        StkNetLibParam stkParam = jniLib.getStkParam();
        // param can be null (no Exception is thrown), so check
        if (stkParam != null) {
            StkClntConn clnConn =
            new StkClntConn(
                stkParam.getAcsServerName(),
                Integer.toString(stkParam.getAcsPort()));
            clnConn.setAccess(stkParam.getAccess());
            clnConn.setSamServerName(stkParam.getSamServerName());
            clnConn.setSamRecvPort(Integer.toString(stkParam.getSamRecvPort()));
            clnConn.setSamSendPort(Integer.toString(stkParam.getSamSendPort()));

            return clnConn;
        }

        // if it reaches here, the client connection info is not found
        // could be that cataserverd is not running
        throw new SamFSException(null, -2517);

    }

    private String getDriveMediaType() throws SamFSException {
        Drive[] drvs =  getDrives();

        if (drvs == null || drvs.length == 0) {
            return null;
        }

        for (int i = 0; i < drvs.length; i++) {
            if (((DriveImpl) drvs[i]).getState() != BaseDevice.OFF) {
                return SamQFSUtil.getMediaTypeString(
                    ((DriveImpl) drvs[i]).getEquipType());
            }
        }

        return SamQFSUtil.getMediaTypeString(
            ((DriveImpl) drvs[0]).getEquipType());

    }

    /**
     * This method returns a string that contains the server name
     * and the library name that shares the same ACSLS server of this library.
     * The format of the string will be libraryName@serverName,.......,......
     */
    public String getLibraryNamesWithSameACSLSServer()
        throws SamFSException {
        if (Library.ACSLS != driverType) {
            throw new SamFSException("logic.wronglibcomparison");
        }

        String acsServerName = jniLib.getStkParam().getAcsServerName();
        StringBuffer buf = new StringBuffer();

        // STEP1: get all managed servers
        SamQFSAppModel app = SamQFSFactory.getSamQFSAppModel();
        SamQFSSystemModel[] models = app.getAllSamQFSSystemModels();
        if (models == null) {
            return "";
        }

        // STEP2: get libraries from each server and eliminate duplicates
        for (int i = 0; i < models.length; i++) {
            if (models[i].isDown()) {
                TraceUtil.trace1("Skip " + models[i].getHostname() + ", down!");
                continue;
            }

            try {
                Library [] crtLibs =
                    models[i].getSamQFSSystemMediaManager().getAllLibraries();
                // for each library on host i
                for (int c = 0; c < crtLibs.length; c++) {

                    if (BaseDevice.MTYPE_HISTORIAN == crtLibs[c].getEquipType())
                        continue; // skip for all historians
                    else if (Library.ACSLS != crtLibs[c].getDriverType())
                        continue; // skip for all non ACSLS libraries

                    if (Common.shareACSLSServer(acsServerName, crtLibs[c])) {
                        if (buf.length() > 0) {
                            buf.append(",");
                        }
                        buf.append(crtLibs[c].getName()).
                            append("@").append(models[i].getHostname());
                    }
                }
	    } catch (SamFSException e) {
                TraceUtil.trace1(
                    "Skipped model for which cannot get libraries");
            }
	}
        return buf.toString();
    }

    /**
     * This method imports the VSNs that are selected by the user.
     */
    public void importVSNInACSLS(String [] vsnNames) throws SamFSException {
        if (vsnNames == null || vsnNames.length == 0) {
            throw new SamFSException("vsn names array is null!");
        }

        ImportOpts opts =
            new ImportOpts(vsnNames[0], null, null, false, false, -1, -1);

        Media.importStkCartridges(
                            model.getJniContext(),
                            getEquipOrdinal(),
                            opts,
                            vsnNames);
    }
}
