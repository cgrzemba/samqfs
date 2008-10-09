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

// ident	$Id: FS.java,v 1.27 2008/10/09 14:32:35 pg125177 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;


/*
 * fs related native methods
 */
public class FS {

    // -------------------- SAM-FS/QFS specific methods -----------------------

    public static native FSInfo get(Ctx c, String fsName) throws SamFSException;
    public static native FSInfo[] getAll(Ctx c) throws SamFSException;
    public static native String[] getNames(Ctx c) throws SamFSException;
    public static native String[] getNamesAllTypes(Ctx c) throws SamFSException;


    public static native void create(Ctx c, FSInfo fsinfo, boolean mountAtBoot)
        throws SamFSException;

    /**
     * The steps are as follows:
     *
     * 1. create the filesystem- this includes setting mount at boot.
     * 2. create the mount point directory.
     * 3. setup the archiver.cmd file.
     * 4. activate the archiver.cmd file.
     * 5. mount the filesystem.
     */
    public static native void createAndMount(Ctx c, FSInfo fsinfo,
        boolean mountAtBoot, boolean createMntPoint, boolean mount)
        throws SamFSMultiStepOpException, SamFSException;

    public static void createAndMount(Ctx c, FSInfo fsinfo,
        boolean mountAtBoot, boolean createMntPoint, boolean mount,
	FSArchCfg archCfg)
        throws SamFSMultiStepOpException, SamFSException {
	createArchFS(c, fsinfo, mountAtBoot, createMntPoint, mount, archCfg);
    }

    /*
     * this function was made private in order to allow createAndMount
     * to be overloaded without making the function signature awkward in
     * the C layer.
     */
    private static native void createArchFS(Ctx c, FSInfo fsinfo,
        boolean mountAtBoot, boolean createMntPoint, boolean mount,
	FSArchCfg archCfg)
        throws SamFSMultiStepOpException, SamFSException;

    public static native void remove(Ctx c, String fsname)
        throws SamFSException;

    public static native void mount(Ctx c, String fsname)
	throws SamFSException;

    public static native void umount(Ctx c, String fsname)
        throws SamFSException;

    /*
     * Returns 0 for the general case but for shared file systems this
     * function returns the job id of the job that is contacting the
     * clients.
     */
    public static native int grow(Ctx c, FSInfo fs,
        DiskDev[] addMetadataDisks, DiskDev[] addDataDisks,
        StripedGrp[] addStripedGrps) throws SamFSException;

    /*
     * Method to remove a device from a file system by releasing all of
     * the data on the device.
     *
     * options is a string of key value pairs that are based on the
     * directives in shrink.cmd. If options is non-null the options
     * will be set for this file system in the shrink.cmd file
     * prior to invoking the shrink.
     *
     * Options Keys:
     *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	   display_all_files = TRUE | FALSE (default FALSE)
     *	   do_not_execute = TRUE | FALSE (default FALSE)
     *	   logfile = filename (default no logging)
     *	   stage_files = TRUE | FALSE (default FALSE)
     *	   stage_partial = TRUE | FALSE (default FALSE)
     *	   streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public static native int shrinkRelease(Ctx c, String fsname,
	int eqToRelease, String options) throws SamFSException;

    /*
     * Method to remove a device from a file system by copying the
     * data to other devices. If replacementEq is the eq of a device
     * in the file system the data will be copied to that device.
     * If replacementEq is -1 the data will be copied to available devices
     * in the FS.
     *
     * Options Keys:
     *	   logfile = filename (default no logging)
     *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	   display_all_files = TRUE | FALSE (default FALSE)
     *	   do_not_execute = TRUE | FALSE (default FALSE)
     *	   logfile = filename (default no logging)
     *	   streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public static native int shrinkRemove(Ctx c, String fsname,
	int eqToRemove, int replacementEq, String options)
	throws SamFSException;

    /*
     * Method to remove a device from a file system by copying the
     * data to a newly added device.
     *
     * Options Keys:
     *	   logfile = filename (default no logging)
     *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	   display_all_files = TRUE | FALSE (default FALSE)
     *	   do_not_execute = TRUE | FALSE (default FALSE)
     *	   logfile = filename (default no logging)
     *	   streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public static native int shrinkReplaceDev(Ctx c, String fsname,
	int eqToRemove, DiskDev replacementDev, String options)
	throws SamFSException;

    /*
     * Method to remove a striped group from a file system by copying the
     * data to a new striped group.
     *
     * Options Keys:
     *	   logfile = filename (default no logging)
     *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	   display_all_files = TRUE | FALSE (default FALSE)
     *	   do_not_execute = TRUE | FALSE (default FALSE)
     *	   logfile = filename (default no logging)
     *	   streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public static native int shrinkReplaceGroup(Ctx c, String fsname,
	int eqToRemove, StripedGrp replacementDev, String options)
	throws SamFSException;


    public static native void fsck(Ctx c, String fsName, String logfile,
        boolean repair) throws SamFSException;

    public static native String getFileMetrics(Ctx c, String fsName,
	int which, long Start, long End) throws SamFSException;

    // mount options methods
    public static MountOptions getMountOpts(Ctx c, String fsName)
        throws SamFSException {
            FSInfo fs = FS.get(c, fsName);
            return fs.getMountOptions();
    }
    public static native MountOptions getDefaultMountOpts(Ctx c, String type,
        int dauSize, boolean usesStripedGroups, boolean shared,
        boolean multireader) throws SamFSException;
    public static native void setMountOpts(Ctx c, String fs, MountOptions mo)
        throws SamFSException;
    public static native void setLiveMountOpts(Ctx c, String fs,
        MountOptions mo) throws SamFSException;
    public static native EQ getEqOrdinals(Ctx c, int num, int[] in_use)
        throws SamFSException;
    public static native void checkEqOrdinals(Ctx c, int[] eqs)
        throws SamFSException;
    public static native void resetEqOrdinals(Ctx c, String fs, int[] eqs)
        throws SamFSException;
    public static native void setDeviceState(Ctx c, String fsName, int state,
        int[] eqs) throws SamFSException;



    // --------------- shared file system methods ----------------

    /*
     * Method to get summary status for a shared file system
     * Status is returned as key value strings.
     *
     * The method potentially returns two strings. One each for the
     * client status, pmds status
     *
     * The format is as follows:
     * clients=1024, unmounted=24, off=2, error=0
     * pmds = 8, unmounted=2, off=0, error=0
     */
    public static native String[] getSharedFSSummaryStatus(Ctx c, String fsName)
	throws SamFSException;

    /*
     * A non-zero return indicates that a background job has been started
     * to complete this task.  Information can be obtained about this job by
     * using the Job.getAllActivities function with a filter on the job id.
     */
    public static native int mountClients(Ctx c, String fsName,
	String[] clients) throws SamFSException;

    /*
     * A non-zero return indicates that a background job has been started
     * to complete this task. Information can be obtained about this job by
     * using the Job.getAllActivities function with a filter on the job id.
     */
    public static native int unmountClients(Ctx c, String fsName,
	String[] clients) throws SamFSException;

    /*
     * A non-zero return indicates that a background job has been started
     * to complete this task. Information can be obtained about this job by
     * using the Job.getAllActivities function with a filter on the job id.
     */
    public static native int setSharedFSMountOptions(Ctx c, String fsName,
	String[] clients, MountOptions mo) throws SamFSException;


    // --------------- generic methods (non-samq specific) -------------------


    /**
     * get non samq filesystem (currently from mount table)
     */
    public static native String[] getGenericFilesystems(Ctx c, String filter)
        throws SamFSException;


    public static native String getGenericFilesystem(Ctx c, String name,
        String type) throws SamFSException;

    public static native void mountByType(Ctx c, String name, String type)
        throws SamFSException;

    public static native void removeGenericFS(Ctx c, String name, String type)
        throws SamFSException;

    public static native String[] getNFSOptions(Ctx c, String mntPoint)
        throws SamFSException;

    public static native void setNFSOptions(Ctx c, String mntPoint,
        String opts) throws SamFSException;

}
