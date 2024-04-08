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

// ident	$Id: BaseDeviceImpl.java,v 1.11 2008/12/16 00:12:21 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;


import com.sun.netstorage.samqfs.web.model.media.BaseDevice;

public class BaseDeviceImpl implements BaseDevice {

    private String devicePath = null;
    private String familySetName = null;
    private String additionalParamFilePath  = null;
    private String logPath = null;
    private int familySetEquipOrdinal = -1;
    private int state = -1;
    private int equipOrdinal = -1;
    private int equipType = -1;
    private long logModTime = -1;

    public BaseDeviceImpl() {
    }

    public BaseDeviceImpl(String devicePath, int equipOrdinal,
                          int equipType, String familySetName,
                          int familySetEquipOrdinal, int state,
                          String additionalParamFilePath,
                          String logPath, long logModTime) {

        this.devicePath = devicePath == null ? "" : devicePath;
	this.equipOrdinal  = equipOrdinal;
	this.equipType = equipType;
        this.familySetName = familySetName == null ? "" : familySetName;
	this.familySetEquipOrdinal = familySetEquipOrdinal;
	this.state = state;
        this.additionalParamFilePath =
            additionalParamFilePath == null ? "" : additionalParamFilePath;
        this.logModTime = logModTime;
        this.logPath = logPath == null ? "" : logPath;
    }


    // getters

    public String getDevicePath() {
	return devicePath;
    }

    public void setDevicePath(String devicePath) {
        if (devicePath != null)
            this.devicePath = devicePath;
    }

    public int getEquipOrdinal() {
	return equipOrdinal;
    }

    public void setEquipOrdinal(int equipOrdinal) {
	this.equipOrdinal = equipOrdinal;
    }


    public int getEquipType() {
	return equipType;
    }

    public void setEquipType(int equipType) {
	this.equipType = equipType;
    }

    public String getFamilySetName() {
	return familySetName;
    }

    public void setFamilySetName(String familySetName) {
        if (familySetName != null)
            this.familySetName = familySetName;
    }

    public int getFamilySetEquipOrdinal() {
	return familySetEquipOrdinal;
    }

    public void setFamilySetEquipOrdinal(int familySetEquipOrdinal) {
        this.familySetEquipOrdinal = familySetEquipOrdinal;
    }

    public int getState() {
	return state;
    }

    public void setState(int state) {
	this.state = state;
    }

    public String getAdditionalParamFilePath() {
	return additionalParamFilePath;
    }

    public void setAdditionalParamFilePath(String path) {
        if (path != null)
            this.additionalParamFilePath = path;
    }

    /**
     * Retrieve the log path of this device (Library/Drive)
     * @since 4.6
     * @return log path of the device
     */
    public String getLogPath() {
        return logPath;
    }

    /**
     * Set the log path of this device (Library/Drive)
     * @since 4.6
     * @param logPath - log path of the device
     */
    public void setLogPath(String logPath) {
        this.logPath = logPath;
    }

    /**
     * Retrieve the last modification time of log path of this device
     * @since 4.6
     * @return the last modification time of log path of this device
     */
    public long getLogModTime() {
        return logModTime;
    }

    /**
     * Set the last modification time of log path of this device
     * @since 4.6
     * @param logModTime - the last modification time of log path of this device
     */
    public void setLogModTime(long logModTime) {
        this.logModTime = logModTime;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("Device Path: " + devicePath + "\n");
        buf.append("Equip Ordinal: " + equipOrdinal + "\n");
        buf.append("Equip Type: " + equipType + "\n");
        buf.append("FamilySet Name: " + familySetName + "\n");
        buf.append("FamilySet Equip Ordinal: " + familySetEquipOrdinal + "\n");
        buf.append("State: " + state + "\n");
        buf.append("Additional Parameter File: " +
                    additionalParamFilePath + "\n");
        buf.append("logPath: " + logPath + "\n");
        buf.append("logModTime: " + logModTime + "\n");

        return buf.toString();
    }
}
