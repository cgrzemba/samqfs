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

// ident	$Id: SamQFSSystemMediaManager.java,v 1.20 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.StkClntConn;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.media.VSNWrapper;

/**
 *
 * This interface is used to manage media associated
 * with an individual server.
 *
 */
public interface SamQFSSystemMediaManager {


    // static integers representing the removable media status bits
    public static int RM_STATUS_MEDIA_SCAN = 10001;
    public static int RM_STATUS_AUTO_LIB_OPERATIONAL = 10002;
    public static int RM_STATUS_MAINT_MODE = 10003;
    public static int RM_STATUS_UNRECOVERABLE_ERR_SCAN = 10004;
    public static int RM_STATUS_DEVICE_AUDIT = 10005;
    public static int RM_STATUS_LABEL_PRESENT = 10006;
    public static int RM_STATUS_FOREIGH_MEDIA = 10007;
    public static int RM_STATUS_LABELLING_ON = 10008;
    public static int RM_STATUS_WAIT_FOR_IDLE = 10009;
    public static int RM_STATUS_NEEDS_OPERATOR_ATTENTION = 10010;
    public static int RM_STATUS_NEEDS_CLEANING = 10011;
    public static int RM_STATUS_UNLOAD_REQUESTED = 10012;
    public static int RM_STATUS_RESERVED_DEVICE = 10013;
    public static int RM_STATUS_PROCESS_WRITING_MEDIA = 10014;
    public static int RM_STATUS_DEV_OPEN = 10015;
    public static int RM_STATUS_TAPE_POSITIONING = 10016;
    public static int RM_STATUS_AUTO_LIB_ALL_SLOTS_OCCUPIED = 10017;
    public static int RM_STATUS_DEV_READONLY_READY = 10018;
    public static int RM_STATUS_DEV_SPUN_UP_READY = 10019;
    public static int RM_STATUS_DEV_PRESENT = 10020;
    public static int RM_STATUS_WRITE_PROTECTED = 10021;


    /**
     *
     * Returns an array of integers representing the available
     * media types.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of integers representing the available
     * media types.
     *
     */
    public int[] getAvailableMediaTypes() throws SamFSException;


    /**
     *
     * Returns an array of integers representing the available
     * media types, including disk.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of integers representing the available
     * media types, including disk.
     *
     */
    public int[] getAvailableArchiveMediaTypes()
        throws SamFSException;


    /**
     *
     * Returns an array of <code>Library</code> objects
     * representing the tape libraries available on this server.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of <code>Library</code> objects
     *
     */
    public Library[] discoverLibraries() throws SamFSException;


    /**
     *
     * Returns an array of STK <code>Library</code> objects
     * representing the STK tape libraries available on this server.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of <code>Library</code> objects
     *
     */
    public Library[] discoverSTKLibraries(StkClntConn [] conns)
        throws SamFSException;

    /**
     *
     * Returns a <code>Library</code> object with the specified
     * properties, if one exists, or <code>null</code> otherwise.
     * @param libName Name of library to search for
     * @param mediaType
     * @param catalog
     * @param paramFile
     * @param ordinal
     * @throws SamFSException if anything unexpected happens.
     * @return A <code>Library</code> object or <code>null</code>.
     *
     */
    public Library discoverNetworkLibrary(String libName, int mediaType,
                                          String catalog, String paramFile,
                                          int ordinal) throws SamFSException;


    /**
     *
     * Get all the libraries that are available to this server.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of <code>Library</code> objects.
     *
     */
    public Library[] getAllLibraries() throws SamFSException;


    /**
     *
     * Get the library named <code>libraryName</code> if it exists.
     * @param libraryName Name of library to search for.
     * @throws SamFSException if anything unexpected happens.
     * @return A <code>Library</code> object, or <code>null</code> if
     * no match is found.
     *
     */
    public Library getLibraryByName(String libraryName) throws SamFSException;


    /**
     *
     * Get the library with EQ of <code>libraryEQ</code> if it exists.
     * @param libraryEQ EQ of library to search for.
     * @throws SamFSException if anything unexpected happens.
     * @return A <code>Library</code> object, or <code>null</code> if
     * no match is found.
     *
     */
    public Library getLibraryByEQ(int libraryEQ) throws SamFSException;


    /**
     *
     * Add a library to this server.
     * @param library
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void addLibrary(Library library) throws SamFSException;


    /**
     * Add an array of libraries to this server
     * @param libraryArray
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void addLibraries(Library [] library) throws SamFSException;


    /**
     *
     * Add a library to this server.
     * @param library
     * @param includedDrives
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void addLibrary(Library library, Drive[] includedDrives)
        throws SamFSException;

    /**
     *
     * Add a network library to this server.
     * @param library
     * @param includedDrives
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void addNetworkLibrary(Library library, Drive[] includedDrives)
        throws SamFSException;


    /**
     *
     * Remove a library from this server.
     * @param library
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void removeLibrary(Library library) throws SamFSException;


    /**
     *
     * Search for VSN objects named <code>vsnName</code> in libraries
     * associated with this server.
     * @param vsnName
     * @return An array of VSN objects.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public VSN[] searchVSNInLibraries(String vsnName) throws SamFSException;


    /**
     * Retrieve the list of disk VSNs used on this host
     */
    public DiskVolume[] getDiskVSNs()
        throws SamFSException;

    /**
     * Retrieve information for the specified VSN
     */
    public DiskVolume getDiskVSN(String name)
        throws SamFSException;

    /**
     * Create a new disk VSN to be used locally.
     * The volume itself may be local or remote, one the specified host
     * @param remoteHost null if the disk volume should be on a local disk
     * @param path
     */
    public void createDiskVSN(String name, String remoteHost, String path)
        throws SamFSException;

    /**
     * create a new honey comb disk vsn.
     * @param - String name - the name of the new vsn
     * @param - String host - target host
     * @param - String port - target port
     */
    public void createHoneyCombVSN(String name, String host, int port)
        throws SamFSException;

    /**
     * Update previously-set flags for the specified VSN.
     */
    public void updateDiskVSNFlags(DiskVolume vol)
        throws SamFSException;

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
     *   name=vsn
     *   type=mediatype
     *   flags=intValue representing flags that are set
     *
     */
    public String [] getUnusableVSNs(int eq, int flag) throws SamFSException;

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
     * @param poolName - the pool of which you want to resolve
     * @param maxEntry - Maximum number of entry you want to return
     * @return Array of VSNs that matches the input VSN expression(s)
     */
    public VSNWrapper evaluateVSNExpression(
        int mediaType, String policyName, int copyNumber,
        String startVSN, String endVSN, String expression,
        String poolName, int maxEntry)
        throws SamFSException;

    // Default maximum entry the GUI will fetch in Assign Media View
    public static final int MAXIMUM_ENTRIES_FETCHED = 1000;

    // Default maximum entry the GUI will fetch in Copy VSNs, Pool Details, and
    // Show VSN pop ups
    public static final int MAXIMUM_ENTRIES_FETCHED_OTHERS = 10000;
}
