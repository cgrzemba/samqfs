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

// ident	$Id: DriveImpl.java,v 1.22 2008/03/17 14:43:50 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.DriveDev;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;


public class DriveImpl extends BaseDeviceImpl implements Drive {


    private SamQFSSystemModelImpl model = null;
    private DriveDev jniDrive = null;

    private String libName = new String();
    private String vsn = new String();
    private String vendor = new String();
    private String productID = new String();
    private String serialNo = new String();
    private String firmwareLevel = new String();
    private int driveState = -1;
    private int[] detailedStatus = new int[0];
    private boolean shared = false;
    private long loadIdleTime = 0;
    private long tapeAlertFlags = 0;

    public DriveImpl() {
    }

    public DriveImpl(SamQFSSystemModelImpl model, DriveDev jniDrive) {

        this.model = model;
        this.jniDrive = jniDrive;
        setup();

    }


    public DriveDev getJniDrive() {

        return jniDrive;

    }


    // Explicitly used in Add Library Wizard ONLY
    public void setName(String name) throws SamFSException {

        if ((name != null) && (name != new String())) {
	    super.setFamilySetName(name);
	    if (jniDrive != null)
		jniDrive.setFamilySetName(name);
	}

    }


    public Library getLibrary() throws SamFSException {

        Library lib = null;

        if ((SamQFSUtil.isValidString(getFamilySetName())) &&
            (model != null))
            lib = model.getSamQFSSystemMediaManager().
                     getLibraryByName(getFamilySetName());

	return lib;

    }


    public String getLibraryName() {

        return getFamilySetName();

    }


    public void setLibrary(Library library) throws SamFSException {

	if (library != null)
            super.setFamilySetName(library.getName());

    }


    public VSN getVSN() throws SamFSException {

        VSN vsnObj = null;

	if ((SamQFSUtil.isValidString(vsn)) && (model != null)) {

            CatEntry[] list = null;
            try {
                list =  Media.getCatEntriesForVSN(model.getJniContext(), vsn);
            } catch (SamFSException e) {
                // C API throws exception if VSN no longer exists
                model.processException(e);
            }
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


    public String getVSNName() {

        return vsn;

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


    public String getFirmwareLevel() throws SamFSException {

	return firmwareLevel;

    }



    public void idle() throws SamFSException {

        setState(Drive.IDLE);

    }

    public int[] getDetailedStatus() {

        return detailedStatus;

    }


    public void unload() throws SamFSException {

        if ((model != null) && (jniDrive != null)) {
            Media.unload(model.getJniContext(), jniDrive.getEquipOrdinal(),
                         false);
        }

    }


    public void clean() throws SamFSException {

        if ((model != null) && (jniDrive != null)) {
            Media.cleanDrive(model.getJniContext(),
                             jniDrive.getEquipOrdinal());
        }

    }


    public String[] getMessages() {

        String[] msg = null;

        if (jniDrive != null) {
            msg = jniDrive.getMessages();
        }
        if (msg == null) {
            msg = new String[0];
        }

        return msg;

    }


    public String toString() {

        StringBuffer buf = new StringBuffer();

        buf.append("Drive: " + super.getFamilySetName() + "\n\n");
        buf.append("VSN: " + vsn + "\n\n");

	buf.append(super.toString() + "\n");
        buf.append("Vendor: " + vendor + "\n");
        buf.append("Product ID: " + productID + "\n");
        buf.append("Serial No: " + serialNo + "\n");
        buf.append("Firmware Level: " + firmwareLevel + "\n");
        buf.append("driveState: " + driveState + "\n");
        buf.append("Load Idle Time: " + loadIdleTime + "\n");
        buf.append("Tape Alert Flags: " + tapeAlertFlags + "\n");

        return buf.toString();

    }


    private void setup() {

        if (jniDrive != null) {

            String devPath = jniDrive.getDevicePath();
            if (!SamQFSUtil.isValidString(devPath)) {
                String[] altPaths = jniDrive.getAlternatePaths();
                if ((altPaths != null) && (altPaths.length > 0)) {
                    devPath = altPaths[0];
                }
            }
            super.setDevicePath(devPath);
            jniDrive.setPath(devPath);
            super.setEquipOrdinal(jniDrive.getEquipOrdinal());
            super.setEquipType(SamQFSUtil.
                               getEquipTypeInteger(jniDrive.getEquipType()));
            super.setFamilySetName(jniDrive.getFamilySetName());
            super.setFamilySetEquipOrdinal(jniDrive.
                                           getFamilySetEquipOrdinal());
            super.setState(SamQFSUtil.convertStateToUI(jniDrive.getState()));
            super.setAdditionalParamFilePath(
                                        jniDrive.getAdditionalParamFilePath());

            vsn = jniDrive.getLoadedVSN();
            vendor = jniDrive.getVendor();
            productID = jniDrive.getProductID();
            serialNo = jniDrive.getSerialNum();
            firmwareLevel = jniDrive.getFirmware();
            driveState = SamQFSUtil.convertStateToUI(jniDrive.getState());
            detailedStatus =
                SamQFSUtil.
                    getRemovableMediaStatusIntegers(jniDrive.getStatus());
            super.setLogPath(jniDrive.getLogPath());
            super.setLogModTime(jniDrive.getLogModTime());
            loadIdleTime = jniDrive.getLoadIdleTime();
            tapeAlertFlags = jniDrive.getTapeAlertFlags();
        }

    }


    // over-ride methods

    public void setEquipOrdinal(int equipOrdinal) {

	super.setEquipOrdinal(equipOrdinal);
        if (jniDrive != null)
            jniDrive.setEquipOrdinal(equipOrdinal);

    }


    public void setEquipType(int equipType) {

	super.setEquipType(equipType);
        if (jniDrive != null)
            jniDrive.setEquipType(SamQFSUtil.getMediaTypeString(equipType));

    }


    public void setFamilySetName(String familySetName) {

        if (familySetName != null) {
            super.setFamilySetName(familySetName);
            if (jniDrive != null)
                jniDrive.setFamilySetName(familySetName);
        }

    }


    public void setFamilySetEquipOrdinal(int familySetEquipOrdinal) {

        super.setFamilySetEquipOrdinal(familySetEquipOrdinal);
        if (jniDrive != null)
            jniDrive.setFamilySetEquipOrdinal(familySetEquipOrdinal);

    }


    public void setState(int state) {

	super.setState(state);
        if (jniDrive != null)
            jniDrive.setState(SamQFSUtil.convertStateToJni(state));

    }


    public void setAdditionalParamFilePath(String path) {

        if (path != null) {
            super.setAdditionalParamFilePath(path);
            if (jniDrive != null)
                jniDrive.setAdditionalParamFilePath(path);
        }

    }

    public boolean unLabeled() {
         boolean unlabel = false;
         String[] message = getMessages();
         String realMsgs = message[0];
         if (realMsgs.startsWith("Unlabeled"))
             unlabel = true;
         return unlabel;
    }

    public boolean isShared() {
        if (jniDrive != null) {
            return jniDrive.isShared();
        }

        return false;
    }

    /**
     * Retrieve the load idle time for the drive
     * @since 4.6
     * @return the load idle time for the drive
     */
    public long getLoadIdleTime() {
        return loadIdleTime;
    }

    /**
     * Set the load idle time for the drive
     * @since 4.6
     * @param loadIdleTime - the load idle time for the drive
     */
    public void setLoadIdleTime(long loadIdleTime) {
        this.loadIdleTime = loadIdleTime;
    }

    /**
     * Retrieve the tape alert flags for the drive
     * @since 4.6
     * @return the tape alert flags for the drive
     */
    public long getTapeAlertFlags() {
        return tapeAlertFlags;
    }

    /**
     * Set the tape alert flags for the drive
     * @since 4.6
     * @param tapeAlertFlags - the tape alert flags for the drive
     */
    public void setTapeAlertFlags(long tapeAlertFlags) {
        this.tapeAlertFlags = tapeAlertFlags;
    }

    public void setShared(boolean shared) throws SamFSException {
        if (jniDrive != null) {
            jniDrive.modifyShared(shared);
            Media.changeStkDriveShareStatus(
                model.getJniContext(),
                getLibrary().getEquipOrdinal(),
                getEquipOrdinal(), shared);
        }
    }
}
