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

// ident	$Id: Restore.java,v 1.17 2008/12/16 00:08:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;


public class Restore {

    // stage copy - these must match the definitions in restore.h
    public static final int DONT_STAGE = 2000;
    public static final int SAM_CHOOSES_COPY = 1000;

    /* metadata dump setup methods */


    /**
     * specifies when/where metadata dumps are taken for specified filesystem
     *
     * 'params' is a set of comma-delimited name-value pairs, as below:
     *   Location   = directory containing dumps
     *   names = filepattern
     *   frequency  = start time 'P' repeat value (in seconds since 1970)
     * optional params:
     *   retention  = retention value (seconds)
     *   prescript  = user specified executable (pre-dump)
     *   postscript = user specified executable (post-dump)
     *   compress   = pathname and arguments to compress dumps
     *   logfile    = logfile path
     */
    public static native void setParams(Ctx c, String fsName, String params)
	throws SamFSException;

    /**
     * retrieve information about when/where metadata dumps are taken
     */
    public static native String getParams(Ctx c, String fsName)
	throws SamFSException;



    /* dump information retrieval */


    /**
     * get all dumps for specified filesystem
     * Compatible with API version 1.3 only.
     *
     * return a list of file paths.
     */
    public static native String[] getDumps(Ctx c,
                                           String fsName)
	throws SamFSException;

    /**
     * get all dumps for specified filesystem in the specified directory.
     * This method is new as of API version 1.4.
     *
     * return a list of file paths.
     */
    public static native String[] getDumps(Ctx c,
                                           String fsName,
                                           String directory)
	throws SamFSException;

    /**
     * get dump status for a specified filesystem and list of dumps.
     * This method is compatible only with API version 1.3.
     *
     * return a list of strings containing the status of each dump
     */
    public static native String[] getDumpStatus(Ctx c,
                                                String fsName,
                                                String[] dumps)
    throws SamFSException;

    /**
     * get dump status for a list of dumps in specified directory in a specified
     * filesystem.  This method is new as of API version 1.4.
     * return a list of strings containing the status of each dump
     */
    public static native String[] getDumpStatus(Ctx c,
                                                String fsName,
                                                String directory,
                                                String[] dumps)
    throws SamFSException;

    /**
     * get all known directories containing snapshots for a given filesystem.
     * This method is new as of API version 1.5
     *
     * return a list of directory paths.
     */
    public static native String[] getIndexDirs(Ctx c,
                                               String fsName)
	throws SamFSException;

    /**
     * get all indexed snapshots for a given filesystem.  Returns an array
     * of key/value pairs of the form  "name=%s,date=%ld"
     *
     * This method is new as of API version 1.5.
     *
     */
    public static native String[] getIndexedSnaps(Ctx c,
                                           String fsName,
                                           String directory)
	throws SamFSException;


    /* take dump, decompress and index dump */

    /**
     * decompress the dump
     * return an id (this is a long duration task)
     */
    public static native String decompressDump(Ctx c, String fsName,
        String dumpName)
    throws SamFSException;

    /**
     * creates a dump
     * return an id (this is a long duration task)
     */
    public static native String takeDump(Ctx c, String fsName,
        String dumpName)
    throws SamFSException;

    /* file manipulation methods */

    /**
     *  return an id (this is a long duration task)
     *  starts the search for a list of paths that match the specified filepath
     *  filepath may contain wildcard characters "?" and "*" and standard
     *  fnmatch(3) patterns
     */
    public static native String searchFiles(Ctx c, String fsname,
        String dumpPath, int maxEntries, String filePath, String filters)
	throws SamFSException;

    /* Version manipulation */


    /**
     *  get versions of specified file
     */
    public static
            native String[] listVersions(Ctx c, String fsname,
        String dumpPath, int maxEntries, String filePath, String filters)
	throws SamFSException;


    /**
     * return details for up to 4 arhive copies of the specified file
     *
     * format of details will be as follow:
     * <protection><size><owner><group><date created><date modified><dump file>
     * Copy 1: [state] <date archived> <media archived on> <position on media>
     * Copy 2: [state] <date archived> <media archived on> <position on media>
     * Copy 3: [state] <date archived> <media archived on> <position on media>
     * Copy 4: [state] <date archived> <media archived on> <position on media>
     */
    public static native String[] getVersionDetails(Ctx c, String fsname,
        String dumpPath, String filePath)
	throws SamFSException;


    /**
     * gets the search results for a specified filesystem
     * return a list of versions
     */
    public static native String[] getSearchResults(Ctx c, String fsname)
    throws SamFSException;

    /**
     *  restore specified inode, and stage the file (optional)
     *  The list of integers (one for each file to be restored) specifies the
     *  staging option. If staging is required the archive copy number (0-3) or
     *  SAM_CHOOSES_COPY is specified. If staging is not required for a file,
     *  the copy number is set to DONT_STAGE.
     *  This is an asynchronous interface
     *  the restore process on the remote server will enter the SAMR_RESTORING
     *  state and refuse most other requests until it re-enters the SAMR_IDLE
     *  state. A jobid is returned to track this operation
     */
    public static native String restoreInodes(Ctx c, String fsname,
        String dumpPath, String[] filePaths, String[] destRelPaths,
        int[] copies, int replaceType)
	throws SamFSException;

    public static native void cleanDump(Ctx c, String fsName,
        String dumpPath)
        throws SamFSException;

    // This method is new as of API version 1.4.
    public static native void deleteDump(Ctx c, String fsName,
        String dumpPath)
        throws SamFSException;

    // This method is new as of API version 1.4.
    public static native void setIsDumpRetainedPermanently(Ctx c,
        String dumpPath, boolean retainValue)
        throws SamFSException;

}
