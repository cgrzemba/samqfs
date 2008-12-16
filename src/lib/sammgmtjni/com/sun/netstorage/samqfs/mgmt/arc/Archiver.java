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
/*
 * archiving related native methods
 */

// ident	$Id: Archiver.java,v 1.20 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;

public class Archiver {

    // global directive

    public static native ArGlobalDirective getArGlobalDirective(Ctx c)
        throws SamFSException;
    public static native ArGlobalDirective getDefaultArGlobalDirective(Ctx c)
        throws SamFSException;
    public static native void setArGlobalDirective(Ctx c, ArGlobalDirective ag)
        throws SamFSException;

    // fs directives

    public static native ArFSDirective[] getArFSDirectives(Ctx c)
        throws SamFSException;

    public static native ArFSDirective getArFSDirective(Ctx c, String fsName)
        throws SamFSException;
    public static native ArFSDirective getDefaultArFSDirective(Ctx c)
        throws SamFSException;

    public static native void setArFSDirective(Ctx c, ArFSDirective arfsDir)
        throws SamFSException;
    public static native void resetArFSDirective(Ctx c, String fsName)
        throws SamFSException;

    // criteria

    public static native String[] getCriteriaNames(Ctx c)
        throws SamFSException;
    public static native Criteria[] getCriteria(Ctx c)
        throws SamFSException;
    public static native Criteria[] getCriteriaForFS(Ctx c, String fsys)
        throws SamFSException;
    public static native boolean isValidGroup(Ctx ctx, String grpName)
        throws SamFSException;
    public static native boolean isValidUser(Ctx ctx, String usrName)
        throws SamFSException;

    // copy params

    public static native CopyParams[] getCopyParams(Ctx c)
        throws SamFSException;
    public static native String[] getCopyParamsNames(Ctx c)
        throws SamFSException;
    public static native CopyParams getCopyParamsForCopy(Ctx c, String copyName)
        throws SamFSException;
    public static native CopyParams[] getCopyParamsForSet(Ctx c,
        String setName) throws SamFSException;
    public static native CopyParams getDefaultCopyParams(Ctx c)
        throws SamFSException;

    public static native void setCopyParams(Ctx x, CopyParams cpParams)
        throws SamFSException;
    public static native void resetCopyParams(Ctx x, String copyName)
        throws SamFSException;

    /*
     * get utilization of archive copies sorted by usage
     * This can be used to get copies with low free space
     *
     * @param int count n copies with top usage
     *
     * @return a list of formatted strings
     *   name=copy name
     *   type=mediatype (TBD)
     *   free=freespace in kbytes
     *   capacity=freespace in kbytes
     *   usage=%
     */
    public static native String[] getCopyUtil(Ctx c, int topCount)
        throws SamFSException;

    // Archive Sets

    public static native ArSet[] getArSets(Ctx c) throws SamFSException;

    public static native ArSet getArSet(Ctx c, String setName)
        throws SamFSException;

    public static native void createArSet(Ctx c, ArSet set)
        throws SamFSException;

    public static native void modifyArSet(Ctx c, ArSet set)
        throws SamFSException;

    public static native void deleteArSet(Ctx c, String setName)
        throws SamFSException;


    /* IS class and policy APIs */

    /**
     * Associate a data class with an archive set. This can result in the
     * set to which the class was previously assigned becoming an unassigned
     * set.
     *
     * Limits: It is unclear at this time if this function will be
     * supported outside of the limited environment of an
     * intellistore.
     */
    public static native void associateClassWithPolicy(Ctx c,
            String className, String setName)
        throws SamFSException;


    /**
     * Delete a data class. This function deletes a data class from
     * the archiver configuration and also clears any class related
     * attributes supported in the intellistore environment. This can
     * result in the set to which the class was previously assigned
     * becoming unassigned.
     *
     * Limits: It is unclear at this time if this function will be
     * supported outside of the limited environment of an
     * intellistore.
     */
    public static native void deleteClass(Ctx c, String className)
        throws SamFSException;


    /**
     * Modifies the archiver configuration so that the data class
     * criteria are applied in the order that the classes appear in
     * the list.  If fs_name is null or an empty string, the new order
     * is applied to all file systems, otherwise the order is applied
     * only to the named file system.
     *
     * Limits: It is unclear at this time if this function will be
     * supported outside of the limited environment of an
     * intellistore.
     */
    public static native void setClassOrder(Ctx c, String fsName,
                Criteria[] classes)
	throws SamFSException;


    /**
     *  API to activate configuration changes.
     *
     *  This function also performs checks that verify that the various
     *  existing archiving-related settings are consistent with each other.
     *
     *  If an internal error occurs, throw SamFSException.
     *  If archiver configuration-related  errors are detected, throw
     *  MultiMsgSamFSException.
     *
     *  Returns a list of warnings or null (success).
     */
    public static native String[] activateCfg(Ctx c)
        throws SamFSException,  /* if an API internal error occurs */
        SamFSMultiMsgException; /* configuration related errors */

    /**
     * same as activateCfg() but warnings are treated as exceptions
     */
    public static native void activateCfgThrowWarnings(Ctx c)
        throws SamFSException,  /* if an API internal error occurs */
        SamFSMultiMsgException, /* configuration related errors */
        SamFSWarnings; /* warnings */

    // archiver control methods. map to the corresponding CLI

    public static native void runForFS(Ctx c, String fsName)
        throws SamFSException;
    public static native void run(Ctx c)
        throws SamFSException;
    public static native void idleForFS(Ctx c, String fsName)
        throws SamFSException;
    public static native void idle(Ctx c)
        throws SamFSException;
    public static native void stopForFS(Ctx c, String fsName)
        throws SamFSException;
    public static native void stop(Ctx c)
        throws SamFSException;
    public static native void restart(Ctx c)
        throws SamFSException;

    // available in 4.4 (api_version 1.3.2 and beyond)
    public static native void rerun(Ctx c)
        throws SamFSException;

    /*
     * options flags for archiveFiles must match the AR_OPT flags
     * in pub/mgmt/archive.h
     */
    public static final int COPY_1 = 0x00000001;
    public static final int COPY_2 = 0x00000002;
    public static final int COPY_3 = 0x00000004;
    public static final int COPY_4 = 0x00000008;
    public static final int DEFAULTS = 0x00000010;
    public static final int NEVER = 0x00000020;
    public static final int RECURSIVE = 0x00000040;
    public static final int CONCURRENT = 0x00000080;
    public static final int INCONSISTENT = 0x00000100;

    /**
     * Archive files and directories or set archive options for them.
     *
     * If any options other than RECURSIVE or COPY_X are specified the
     * command will set options flags for the files but will not schedule
     * archiving for them.
     *
     * If no COPY_X flags are specified the command will apply to all
     * archive copies. COPY_X options only apply when archiving. They
     * have no impact on setting archive options.
     *
     * INCONSISTENT and CONCURRENT are advanced options.
     * INCONSISTENT means that an archive copy can be created even if the
     *		file is modified while it is being copied to the media. These
     *		copies can only be used after a samfsrestore.
     *
     * CONCURRENT means that a file can be archived even if opened for
     *		write. This can lead to wasted media.
     *
     * @return jobID of the command requesting files to be archived or
     *		null if that command completed. It should be noted however
     *		that archiving is asynchronous and will not necessarily have
     *		completed even if null is returned.
     */
    public static native String archiveFiles(Ctx c,
	String files[], int options)
	throws SamFSException;

}
