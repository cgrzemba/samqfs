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

// ident	$Id: FS.java,v 1.23 2008/04/23 19:58:38 ronaldso Exp $

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

    public static native void mount(Ctx c, String fsname) throws SamFSException;
    public static native void umount(Ctx c, String fsname)
        throws SamFSException;

    public static native void grow(Ctx c, FSInfo fs,
        DiskDev[] addMetadataDisks, DiskDev[] addDataDisks,
        StripedGrp[] addStripedGrps) throws SamFSException;

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
