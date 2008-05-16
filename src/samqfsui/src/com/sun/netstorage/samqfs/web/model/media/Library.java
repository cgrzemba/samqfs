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

// ident	$Id: Library.java,v 1.31 2008/05/16 18:39:04 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.media;


import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.mgmt.media.StkPool;
import com.sun.netstorage.samqfs.mgmt.media.StkVSN;

import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.archive.RecycleParams;
import java.util.HashMap;

public interface Library extends BaseDevice {

    /*
     * These values are from include/sam/devinfo.h (sam_model)
     * devinfo.h defines a table with the name of the robot and the int
     * representing the robot. devstat.h defines the robot types
     *
     * Everytime SAM upgrades to support new robots, additions have to be made
     * to this list
     * It is unlikely that the robot type will change, e.g. ACL_2640 is 6347
     */
    public static final int RTYPE_ACL_2640 = 6347;
    public static final int RTYPE_ACL_452 = 6346;
    public static final int RTYPE_ATLP_3000 = 6360;
    public static final int RTYPE_ADIC_448 = 6351;
    public static final int RTYPE_ADIC_1000 = 6363;
    public static final int RTYPE_ADIC_100 = 6367;
    public static final int RTYPE_ATL_1500 = 6371;
    public static final int RTYPE_CYG_1803 = 6274;
    public static final int RTYPE_DLT_2700 = 6341;
    public static final int RTYPE_DOC_STOR = 6275;
    public static final int RTYPE_EXB_210 = 6349;
    public static final int RTYPE_EXB_X80 = 6364;
    public static final int RTYPE_GRAUACI = 6220;
    public static final int RTYPE_HP_C7200 = 6369;
    public static final int RTYPE_HP_OPLIB = 6276;
    public static final int RTYPE_IBM_ATL = 6225;
    public static final int RTYPE_IBM_3570C = 6356;
    public static final int RTYPE_IBM_3584 = 6366;
    public static final int RTYPE_LMS_4500 = 1281;
    public static final int RTYPE_METD_28 = 6343;
    public static final int RTYPE_METD_360 = 6344;
    public static final int RTYPE_ODINEO = 6372;
    public static final int RTYPE_PLASMOND = 6297;
    public static final int RTYPE_RAP_4500 = 6145;
    public static final int RTYPE_SPECLOG = 6352;
    public static final int RTYPE_QUAL_82XX = 6370;
    public static final int RTYPE_SONY_DMS = 6357;
    public static final int RTYPE_STK_API = 6222;
    public static final int RTYPE_STK_97XX = 6354;
    public static final int RTYPE_STK_LXX = 6365;
    public static final int RTYPE_SONY_PSC = 6234;
    public static final int RTYPE_FUJI_LT300 = 6231;

    // driver type

    public static final int SAMST = 1001;

    public static final int ACSLS = 1002;

    public static final int ADIC_GRAU = 1003;

    public static final int FUJITSU_LMF = 1004;

    public static final int IBM_3494 = 1005;

    public static final int SONY = 1006;

    public static final int OTHER = 1007;


    // sort by

    public static final int MAX_ROW_IN_PAGE = 25;


    public String getName() throws SamFSException;


    // don't use this except for add library wizard; will have very
    // undesirable consequences if called anywhere else
    public void setName(String name) throws SamFSException;


    public Drive[] getDrives() throws SamFSException;

    // don't use this except for add library wizard
    public void setDrives(Drive [] drives) throws SamFSException;


    public VSN[] getVSNs() throws SamFSException;


    public VSN[] getVSNs(int start, int size, int sortby, boolean ascending)
        throws SamFSException;


    public VSN getVSN(String vsnName) throws SamFSException;


    public VSN getVSN(int slotNo) throws SamFSException;


    public int getTotalVSNInLibrary() throws SamFSException;


    public String getVendor() throws SamFSException;


    public String getProductID() throws SamFSException;


    public String getSerialNo() throws SamFSException;


    public int[] getDetailedStatus();


    public int getNoOfLicensedSlots() throws SamFSException;


    public int getNoOfAvailableSlots() throws SamFSException;


    public int getNoOfCatalogEntries() throws SamFSException;


    public int getDriverType() throws SamFSException;

    // don't use this except for add library wizard; will have very
    // undesirable consequences if called anywhere else
    public void setDriverType(int type) throws SamFSException;


    public String getFirmwareLevel() throws SamFSException;


    public long getTotalCapacity() throws SamFSException;


    public long getTotalFreeSpace() throws SamFSException;


    public int getMediaType() throws SamFSException;


    public String getCatalogLocation() throws SamFSException;

    // don't use this except for add library wizard; will have very
    // undesirable consequences if called anywhere else
    public void setCatalogLocation(String location) throws SamFSException;


    public String getParamFileLocation() throws SamFSException;


    public Alarm[] getAssociatedAlarms() throws SamFSException;


    public void importVSN() throws SamFSException;


    public void importVSNInACSLS(long poolId, int count)
        throws SamFSException;


    public void importVSNInNWALib(String startVSN, String endVSN)
        throws SamFSException;


    public void unload() throws SamFSException;

    public String[] getMessages();

    public boolean containMixedMedia();

    /**
     * The following two methods are getter and setter for STK Library Parameter
     * Object resides in the Library Object. (4.5+ only)
     */
    public StkNetLibParam getStkNetLibParam() throws SamFSException;
    public void setStkNetLibParam(StkNetLibParam newParam)
        throws SamFSException;

    /**
     * The following two methods are getter and setter to keep track of Start
     * drive EQ and its increment value.  This is strictly used in Add Library
     * Wizard ACSLS Discovery path.  We need to keep track of these two numbers
     * in the library object in order to present the right values in the
     * looping situation.
     */
    public int getDriveStartNumber();
    public void setDriveStartNumber(int start);
    public int getDriveIncreNumber();
    public void setDriveIncreNumber(int incre);

    /**
     * setDevicePath is used only when adding ACSLS Library from Version 4.5.
     * Device Path is no longer returned from media discovery and presentation
     * layer needs to set the device path so the C-layer knows where to write
     * the parameter file on the library.
     */
    public void setDevicePath(String devicePath);


    /**
     * This method returns a StkPhyConf Object, a wrapper of StkPool,
     * two int [] structure, and 4 int. GUI needs to call this API to
     * pre-populate the Filter for Import VSN.  This API is introduced in 4.5.
     */
    public StkPool[] getPhyConfForStkLib() throws SamFSException;


    /**
     * This method returns an array of StkVSN in order to populate the
     * action table in the Import VSN page for 4.5+ ACSLS Servers.  The GUI
     * still needs to exclude the VSNs that are
     * 1. Already in used
     * 2. In drive
     * and take away the checkbox to prevent users from importing them.
     */
    public StkVSN [] getVSNsForStkLib(String filter) throws SamFSException;

    /**
     * This method returns a hashmap that contains VSN Names that reside in a
     * STK Library  This is used to provide information for
     * the logic layer so it knows which VSNs are in used that they need to be
     * excluded from the selection list.
     */
    public HashMap getVSNNamesForStkLib() throws SamFSException;

    /**
     * This method returns an array of String that contains the server name
     * and the library name that shares the same ACSLS server of this library.
     * The format of the string will be libraryName@serverName
     */
    public String getLibraryNamesWithSameACSLSServer() throws SamFSException;

    /**
     * This method imports the VSNs that are selected by the user.
     */
    public void importVSNInACSLS(String [] vsnNames) throws SamFSException;

}
