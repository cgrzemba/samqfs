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

// ident	$Id: FileUtil.java,v 1.15 2008/03/17 14:43:55 am143972 Exp $
package com.sun.netstorage.samqfs.mgmt;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * native utility methods
 */
public class FileUtil {

    /*
     * flags to determine what information to return
     * from getExtFileDetails. Must match the FD_ options
     * in pub/mgmt/file_util.h
     */
    public static final int FILE_TYPE = 0x00000001;
    public static final int SIZE    = 0x00000002;
    public static final int CREATED = 0x00000004;
    public static final int MODIFIED  = 0x00000008;
    public static final int ACCESSED  = 0x00000010;
    public static final int USER    = 0x00000020;
    public static final int GROUP   = 0x00000040;
    public static final int MODE    = 0x00000080;
    public static final int FNAME   = 0x00000100;
    public static final int SAM_STATE    = 0x00001000;
    public static final int RELEASE_ATTS = 0x00002000;
    public static final int STAGE_ATTS   = 0x00004000;
    public static final int SEGMENT_ATTS = 0x00008000;
    public static final int CHAR_MODE    = 0x00010000;
    public static final int COPY_SUMMARY = 0x00020000;
    public static final int COPY_DETAIL  = 0x00040000;
    public static final int SEGMENT_ALL  = 0x00080000;
    public static final int ARCHIVE_ATTS = 0x00100000;

    /*
     * file_type=%d values returned by getExtFileDetails. These must
     * match the values in pub/mgmt/file_util.h
     */
    public static final int FTYPE_LNK  = 0;
    public static final int FTYPE_REG  = 1;
    public static final int FTYPE_DIR  = 2;
    public static final int FTYPE_FIFO = 3;
    public static final int FTYPE_SPEC = 4;
    public static final int FTYPE_UNK  = 5;

    /*
     * release_atts=%d values returned by getExtFileDetails
     * These must match the values in pub/mgmt/file_util.h
     */
    public static final int RL_ATT_NEVER = 0x0001;
    public static final int RL_ATT_WHEN1COPY = 0x0002;

    /*
     * stage_atts=%d values returned by getExtFileDetails
     * These must match the values in pub/mgmt/file_util.h
     */
    public static final int ST_ATT_NEVER = 0x0010;
    public static final int ST_ATT_ASSOCIATIVE = 0x0020;

    /*
     * archive_atts=%d values returned by getExtFileDetails
     * These must match the values in pub/mgmt/file_util.h
     */
    public static final int AR_ATT_NEVER = 0x0100;
    public static final int AR_ATT_CONCURRENT   = 0x0200;
    public static final int AR_ATT_INCONSISTENT = 0x0400;

    /**
     * return up to maxEntries entries from the specified directory,
     * in alphabetical order.
     *
     * 'filters' is a set of comma-delimited name-value pairs, as below:
     *   Filename = regexp (in fnmatch(3) format)
     *   Owner    = user id (decimal)
     *   Group    = group id (decimal)
     *   ModifiedBefore = date (number of GMT seconds since the epoch)
     *   ModifiedAfter  = date
     *   CreatedBefore  = date
     *   CreatedAfter   = date
     *   BiggerThan   = size in bytes
     *   SmallerThan  = size in bytes
     */
    public static native String[] getDirEntries(Ctx c, int maxEntries,
        String dirPath, String filters)
        throws SamFSException;

    /**
     * leave startFile NULL if reading from beginning of directory.
     * if lastFile != NULL, results will start with entries that both
     * match the filter and sort alphabetically after startFile.
     */
    public static native GetList getDirectoryEntries(Ctx c, int maxEntries,
        String dirPath, String lastFile, String filters)
        throws SamFSException;

    /**
     * 0x0 - missing - no such file/dir
     * 0x1 - on disk, file
     * 0x2 - on disk, dir
     * 0x3 - needs staging, file
     * 0x4 - neither file or directory (link, special...), not used
     */
    public static native int[] getFileStatus(Ctx c, String[] filePaths)
        throws SamFSException;

    /**
     * return details such as file size, time when file was created,
     * last modified etc. for each file specified as input
     *
     * format of details will be as follows:
     * "size=%lld,created=%lu,modified=%lu"
     *
     */
    public static native String[] getFileDetails(Ctx c, String fsname,
        String[] files)
        throws SamFSException;

    /**
     * create a file (such as log file).
     * return success even if the file already exists
     */
    public static native void createFile(Ctx c, String file)
        throws SamFSException;

    /**
     * check if the file exist
     */
    public static native boolean fileExists(Ctx c, String filePath)
        throws SamFSException;

    /**
     * tail a file
     */
    public static native String[] tailFile(Ctx c, String filePath, int number)
        throws SamFSException;

    /**
     * this method may be used before mount to ensure that mount point exist.
     * It returns success even if the directory already exists.
     */
    public static native void createDir(Ctx c, String dir)
        throws SamFSException;

    /**
     * get the lines from the text file at filePath. The first line
     * of the file cooresponds to a start value of 1.
     */
    public static native String[] getTxtFile(Ctx c, String filePath,
            int start, int count)
	throws SamFSException;

    /**
     * return extended details about a file in key value strings. This
     * function can be used to obtain information for any type of file
     * beyond that which is provided by getFileDetails. In addition it
     * can be used to get sam specific information about files that
     * reside in a sam file system.
     *
     * The flags set in whichDetails will determine what information
     * is returned in each key value string. Many of the keys for sam
     * attributes will not show up if the attribute is not set for the
     * file. e.g. if neither release=never nor release=when1copy is set
     * no release= key value will be returned.
     *
     * Flags		Potential resulting key-value pairs
     * FILE_TYPE	file_type=%d FTYPE_XXX
     * SIZE		size = %lld
     * CREATED		created=%llu
     * MODIFIED		modified=%llu
     * ACCESSED		accessed=%llu
     * USER		user=%s
     * GROUP		group=%s
     * MODE		mode=%o e.g. 0600
     * RELEASE_ATTS
     *  partial_release_sz=%lld release_atts==%d RL_ATT_NEVER | RL_ATT_WHEN1COPY
     * STAGE_ATTS	stage_atts=%d ST_ATT_NEVER | ST_ATT_ASSOC
     * SEGMENT_ATTS	seg_size_mb=%ld,seg_stage_ahead=%ld
     * SAM_STATE
     *   seg_count=%d,damaged=%d,partial_online=%d,
     *   offline=%d,online=%d,archdone=%d,stage_pending=%d
     * ARCHIVE_ATTS
     *   archive_atts=%d  AR_ATT_NEVER | AR_ATT_CONCURRENT | AR_ATT_INCONSISTENT
     *
     * Note: SAM_STATE
     * In sam there are a number of pieces of state information for any given
     * file that may be interesting. Furthermore for segmented files each of
     * these pieces of information is pertinent for each segment.
     *
     * The states online, partial_online and offline are mutually exclusive for
     * a given file or single segment of a file.
     *
     * For a non-segmented file that is online and archdone the function would
     * return "online=1,archdone=1"
     *
     * For a segmented file that has 2 segments of which one is online, one is
     * offline and both are archdone this function will return:
     * "seg_count=2,online=1,offline=1,archdone=2"
     */
    public static native String[] getExtFileDetails(Ctx c, String[] files,
	    int whichDetails)
        throws SamFSException;

    /**
     * Given a file path return information about its offline copies.
     *	This function returns information about the status and location of
     *	the archive copies for the file specified in file_path.
     *
     *	e.g. non-segmented file on 1 vsn:
     *	copy=1,created=1130378055,media=dk,vsn=dk2
     *
     *	e.g output for a segmented file on 2 different vsns:
     *	copy=1,seg_count=3,created=1130289560,media=dk,vsn=dk1 dk2
     *
     *	each copy may also have a count of stale, damaged, inconsistent or
     *	unarchived. If any of these are returned the copy is not a valid
     *	candidate for staging.
     *
     *  e.g. a segmented file with 1 damaged segment copy and 1 stale segment
     *	copy wold look like:
     *
     *	copy=1,seg_count=3,damaged=1,stale=1,
     *		created=1130289560,media=dk,vsn=dk1 dk2
     *
     *  Stale means the ondisk copy changed since the archive copy was made.
     *
     *	damaged means the copy has been marked as damaged by the user with
     *	the damage command or by the system as a result of a fatal error
     *	when trying to stage. damaged copies ARE NOT???? available for staging
     *
     *	inconsistent means that the file was modified while the
     *	archive copy was being made. By default such a copy would not
     *	be allowed. However there is a flag to the archive command
     *	that allows the archival of inconsistent copies. inconsistent
     *	copies can only be staged after a samfsrestore. They are not
     *	candidates for staging in a general case. However, knowledge
     *	of their existence is certainly desireable to a user.
     *
     *  which details is reserved for future use.
     *
     */
    public static native String[] getCopyDetails(Ctx c, String file_path,
	    int whichDetails)
        throws SamFSException;

    /**
     * Lists a directory and returns details for the files within
     *
     * leave startFile NULL if reading from beginning of directory.
     * if lastFile != NULL, results will start with entries that both
     * match the filter and sort alphabetically after startFile.
     */
    public static native GetList listCollectFileDetails(Ctx c, String fsname,
		String snapPath, String relativeDir, String lastFile,
		int howMany, int whichDetails, String restrictions)
	throws SamFSException;

    /*  Gets details for a single specific file */
    public static native String collectFileDetails(Ctx c, String fsname,
		String snapPath, String filePath, int whichDetails)
	throws SamFSException;

    public static native void deleteFiles(Ctx c, String[] filepaths)
        throws SamFSException;

    public static native void deleteFile(Ctx c, String filepath)
        throws SamFSException;
}
