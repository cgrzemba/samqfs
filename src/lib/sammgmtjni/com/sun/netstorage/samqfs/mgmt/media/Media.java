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

// ident	$Id: Media.java,v 1.23 2008/05/16 18:35:29 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
/*
 * media related native methods
 */
public class Media {

    /**
     * discover all installed direct-attached libraries and standalone drives.
     * exclude the already configured media from the result
     *
     * If the API caller already has information about the network-attached
     * libraries, (s)he should put this info in the inf argument so that
     * drives coresponding to these libraries are going to be excluded from
     * the set of returned devices.
     *
     * This is a full discovery - it rediscovers SAM-FS configured libraries.
     */
    public static native Discovered discoverUnused(Ctx c) throws SamFSException;


    /**
     * discover all STK ACSLS network attached libraries
     * @param array of StkClntConn (hostname and port number tuple)
     * @return array of libraries
     */
    public static native LibDev[] discoverStk(Ctx c, StkClntConn[] conns)
        throws SamFSException;

    /**
     * get information about an installed network-attached library so that
     * it can be added under SAM's control
     */
    public static native LibDev getNetAttachLib(Ctx c, NetAttachLibInfo nwAtt)
        throws SamFSException;


    // library configuration related methods

    public static native LibDev[] getLibraries(Ctx c)
        throws SamFSException;
    public static native LibDev getLibraryByPath(Ctx c, String path)
        throws SamFSException;
    public static native LibDev getLibraryByFSet(Ctx c, String fset)
        throws SamFSException;
    public static native LibDev getLibraryByEq(Ctx c, int eq)
        throws SamFSException;

    public static native void addLibrary(Ctx c, LibDev lib)
        throws SamFSException;
    /**
     * Add multiple libraries (array)
     */
    public static native void addLibraries(Ctx c, LibDev[] libs)
	 throws SamFSException;

    public static native void removeLibrary(Ctx c, int eq, boolean unload)
        throws SamFSException;


    // standalone drive configuration related methods

    public static native DriveDev[] getStdDrives(Ctx c)
        throws SamFSException;
    public static native DriveDev getStdDriveByPath(Ctx c, String path)
        throws SamFSException;
    public static native DriveDev getStdDriveByEq(Ctx c, int eq)
        throws SamFSException;

    public static native void addStdDrive(Ctx c, DriveDev drv)
        throws SamFSException;
    public static native void removeStdDrive(Ctx c, int eq)
        throws SamFSException;


    // media control operations

    /**
     * audit media (tape/optical cartridge) in the specified slot.
     *
     * Caution: Skip to EOD is not interruptible and
     *   under certain conditions can take hours to complete
     */
    public static native void auditSlot(Ctx c, int libEq, int slot,
        int partition, boolean skipToEOD) throws SamFSException;

    public static native int getNumberOfCatEntries(Ctx c, int libEq)
        throws SamFSException;

    /* see CatEntry.java for a list of acceptable status bits */
    public static native void chgMediaStatus(Ctx c, int eq, int slot,
	boolean set, /* if true then set, otherwise reset */
        int status) throws SamFSException;

    public static native void cleanDrive(Ctx c, int eq)
        throws SamFSException;

    public static native void importCartridge(Ctx c, int libEq, ImportOpts opts)
        throws SamFSException;
    /**
     * import a range of VSN-s.
     * The following fields in 'opts' are ignored: vsn, volCount, pool.
     * @return the number of VSN-s successfully imported
     */
    public static native int importCartridges(Ctx c, int libEq, String startVSN,
        String endVSN, ImportOpts opts) throws SamFSException;
    /**
     * import an array of ACSLS VSN-s. Only used for STK ACSLS Volumes
     */
    public static native void importStkCartridges(Ctx c, int libEq,
        ImportOpts opts,
        String[] cartridges) throws SamFSException;

    public static native void exportCartridge(Ctx c, int libEq, int slot,
        boolean stkOneStep) throws SamFSException;
    public static native void moveCartridge(Ctx c, int libEq, int srcSlot,
        int destSlot) throws SamFSException;
    public static native void tapeLabel(Ctx c, int eq, int slot, int partition,
        String newVSN, /* 1-6 characters */
        String oldVSN, /* if any. null if media is blank */
        long blockSzKb, /* must be 16,32,64,128,256,512,1024 or 2048 */
        boolean wait,
        boolean erase  /* warning: might take a long time */)
        throws SamFSException;

    public static native void load(Ctx c, int libEq, int slot, int partition,
        boolean wait) throws SamFSException;
    public static native void unload(Ctx c, int eq, boolean wait)
        throws SamFSException;

    public static native void reserve(Ctx c, int eq, int slot, int partition,
        ReservInfo resInfo) throws SamFSException;
    public static native void unreserve(Ctx c, int eq, int slot, int partition)
        throws SamFSException;

    public static native void changeDriveState(Ctx c, int eq, int newState)
        throws SamFSException;

    /**
     * return null if VSN is not loaded
     */
    public static native DriveDev isVSNLoaded(Ctx c, String vsn)
        throws SamFSException;

    public static native int getLicensedSlots(Ctx c, int libEq)
        throws SamFSException;

    public static native long getLibCapacity(Ctx c, int libEq) /* MBytes */
        throws SamFSException;
    public static native long getLibFreeSpace(Ctx c, int libEq) /* MBytes */
        throws SamFSException;

    // these values must match those defined in pub/mgmt/device.h
    public static final short VSN_SORT_BY_NAME = 0;
    public static final short VSN_SORT_BY_FREESPACE = 1;
    public static final short VSN_SORT_BY_SLOT = 2;
    public static final short VSN_NO_SORT = 4;

    /**
     * retrieve information about the media in the specified lib/slot/part
     */
    public static native CatEntry getCatEntryForSlot(Ctx c, int libEq,
        int slot, int partition) throws SamFSException;
    /**
     * the regular expression passed as an argument must follow the SAM-FS/QFS
     * syntax rules for specifiying VSN regular expression.
     */
    public static native CatEntry[] getCatEntriesForRegexp(Ctx c,
        String vsnRegexp, int start, int size, short sortMethod,
        boolean ascending) throws SamFSException;
    /**
     * the method can return multiple CatEntry-s only if the same VSN is
     * used for different media types
     */
    public static native CatEntry[] getCatEntriesForVSN(Ctx c, String vsn)
        throws SamFSException;

    public static native CatEntry[] getCatEntriesForLib(Ctx c,
        int libEq, int startSlot, int endSlot, short sortMet, boolean ascending)
        throws SamFSException;
    public static native CatEntry[] getAllCatEntriesForLib(Ctx c, int libEq,
        int start, int size, short sortMet, boolean asc)
        throws SamFSException;

    public static native String[] getAvailMediaTypes(Ctx c)
        throws SamFSException;

    // MAX EQ - must match definition in include/sam/types.h
    public static final int EQU_MAX = 65534;
    /*
     * get unusable vsns
     * @param equ_t   - eq of lib, if EQU_MAX, all lib
     * @param uint32_t -
     *  status field bit flags
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
     * @returns a list of formatted strings
     *  name=vsn
     *  type=mediatype
     *  flags=intValue representing flags that are set
     */
    public static native String[] getVSNs(Ctx c, int libEq, int flags)
        throws SamFSException;

    // Stk configuration information
    /**
     * get general information for Stk ACSLS Library
     * @param StkClntConn stkConn (hostname and port tuple)
     * @return int [] array of an information.
     */
    // these values must match those defined in pub/mgmt/device.h
    public static final short LSM_INFO	= 0;
    public static final short PANEL_INFO = 1;
    public static final short POOL_INFO	= 2;
    public static final short CAP_INFO = 3;
    public static final short VOLUME_INFO = 4;
    public static final short DRIVE_SERIAL_NUM_INFO = 5;
    public static final short DRIVE_INFO = 6;
    public static final short LSM_SERIAL_NUM_INFO = 7;
    public static final short VOLUME_WITH_DATE_INFO = 8;

    public static native int[] getInfosForStkLib(Ctx c, StkClntConn stkConn,
	int type, String UserDate, String DateType)
	    throws SamFSException;

    /**
     * get library storage modules (LSM) for Stk ACSLS Library
     * @param StkClntConn stkConn (hostname and port tuple)
     * @return int [] array of Library Storage Module Number
     */
    public static native int[] getLsmsForStkLib(Ctx c, StkClntConn stkConn)
	    throws SamFSException;

    /**
     * get panels information for Stk ACSLS Library
     * @param StkClntConn stkConn (hostname and port tuple)
     * @return int[] array of Panel Numbers
     */

    public static native int[] getPanelsForStkLib(Ctx c, StkClntConn stkConn)
	    throws SamFSException;

    /**
     * get scratch pools for Stk ACSLS Library
     * @param StkClntConn stkConn (hostname and port tuple)
     * @return StkPool [] array of StkPools
     */
    public static native StkPool[] getPoolsForStkLib(Ctx c, StkClntConn stkConn)
	    throws SamFSException;

    /**
     * get cell information i.e min row, max row, min col, max col
     * for Stk ACSLS Library
     * @param libEq library equiment number
     * @param StkIdentifier (hostname and port tuple)
     * @return StkCell
     */
    public static native StkCell getCellsForStkLib(Ctx c, String mediaType,
	    StkClntConn stkConn) throws SamFSException;


    public static final int FILTER_BY_SCRATCH_POOL = 0;
    public static final int FILTER_BY_VSN_RANGE = 1;
    public static final int FILTER_BY_VSN_EXPRESSION = 2;
    public static final int NO_FILTER = 3;

    /**
     * The following constants are Key-value processing for use by logic layer
     * These will be removed from the jni layer after it is copied to the logic
     * layer.
     *
     * valid keys are defined below; keys are case-sensitive for now
     */
    protected static final String KEY_ACCESS_DATE = "access_date";
    protected static final String KEY_EQ_TYPE = "equ_type";
    protected static final String KEY_FILTER_TYPE = "filter_type";
    protected static final String KEY_SCRATCH_POOL_ID = "scratch_pool_id";
    protected static final String KEY_START_VSN = "start_vsn";
    protected static final String KEY_END_VSN = "end_vsn";
    protected static final String KEY_VSN_EXPRESSION = "vsn_expression";
    protected static final String KEY_LSM = "lsm";
    protected static final String KEY_PANEL = "panel";
    protected static final String KEY_START_ROW  = "start_row";
    protected static final String KEY_END_ROW = "end_row";
    protected static final String KEY_START_COL = "start_col";
    protected static final String KEY_END_COL = "end_col";
    /*
     *	get a list of vsns that match the criteria specified in the filter.
     *  @param c rpc client connection
     *	@param stkConn (Stk server hostname and port tuple)
     *  @param filter set of comma-delimited name-value pairs, as below:
     *
     *  Required key values:-
     *  access_date = access date of vsn(should be set to local date by default)
     *   yyyy-mm-dd or yyyy-mm-dd-yyyy-mm-dd
     *  equ_type    = media type
     *  filter_type = one of the following:-
     *    FILTER_BY_SCRATCH_POOL,
     *    FILTER_BY_VSN_RANGE,
     *    FILTER_BY_VSN_EXPRESSION,
     *    NO_FILTER
     *
     *  Optional key values:-
     *  for FILTER_BY_SCRATCH_POOL
     *   scratch_pool_id  = id
     *  for FILTER_BY_VSN_RANGE
     *   start_vsn  = starting vsn number
     *   end_vsn    = ending vsn number
     *  for FILTER_BY_VSN_EXPRESSION
     *   vsn_expression = pattern
     *
     *  @return StkVSN [] array of StkVSN
     */
    public static native StkVSN[] getVSNsForStkLib(
        Ctx c, StkClntConn stkConn, String filter) throws SamFSException;

    /**
     * get all VSNs (names) in Stk
     *
     * @param mediaType
     * @return StkVolume[]
     * throws SamFSException
     */
    public static native String[] getVSNNamesForStkLib(Ctx c, String mediaType)
		throws SamFSException;

    /**
     * get scratch pool information for Stk ACSLS Library
     * @param libEq library equiment number
     * @param StkIdentifier (hostname and port tuple)
     * @return StkPool[]
     */
    public static native StkPool[] getStkScratchPools(Ctx c, String mediaType,
        StkClntConn stkConn) throws SamFSException;

    /**
     * share/unshare stk drive
     * @param libEq library equiment number
     * @param driveEq drive equipment number
     * @param shared share or unshare
     */
    public static native void changeStkDriveShareStatus(Ctx c, int libEq,
        int driveEq, boolean shared) throws SamFSException;
}
