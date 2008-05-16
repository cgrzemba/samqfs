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

// ident	$Id: Stager.java,v 1.13 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.stg;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.BufDirective;
import com.sun.netstorage.samqfs.mgmt.arc.DrvDirective;

public class Stager {

    /*
     * options masks for stage files must match the ST_OPT flags
     * in pub/mgmt/stage.h
     */
    public static final int RESET_DEFAULTS = 0x0001;
    public static final int NEVER = 0x0002;
    public static final int ASSOCIATIVE = 0x0004;
    public static final int PARTIAL = 0x0008;
    public static final int RECURSIVE = 0x0010;
    public static final int COPY_1 = 0x1000;
    public static final int COPY_2 = 0x2000;
    public static final int COPY_3 = 0x4000;
    public static final int COPY_4 = 0x8000;

    public static native StagerParams getParams(Ctx c)
        throws SamFSException;
    public static native StagerParams getDefaultParams(Ctx c)
        throws SamFSException;
    public static native void setParams(Ctx c, StagerParams stg)
        throws SamFSException;

    public static native void resetBufDirective(Ctx c, BufDirective bufDir)
        throws SamFSException;
    public static native void resetDrvDirective(Ctx c, DrvDirective drvDir)
        throws SamFSException;

    public static native void idle(Ctx c)
        throws SamFSException;
    public static native void run(Ctx c)
        throws SamFSException;

    /*
     *
     * stage files
     * Used to stage files and directories and to set
     * stage attributes for files and directories.
     *
     * The elements in the lists are matched by index. So the 5th
     * copy entry is used for the fifth filepath entry and the 5th options
     * entry is applied to the staging.
     *
     * In order to signal that a file specified as stage -a be set to
     * stage -n the RESET_DEFAULTS flag must be set also and vice versa.
     *
     * The copies array indicates what copy will be staged from.
     * If the value is 0 the system picks the copy otherwise a valid
     * copy number in the range 1-4 can be specified.
     *
     * Filepaths must contain fully qualified paths.
     *
     * Flags for setting stager options
     * NEVER
     * ASSOCIATIVE
     * RESET_DEFAULTS
     * RECURSIVE
     *
     * Flags for controlling staging
     * PARTIAL
     * RECURSIVE
     *
     * @return null if stage_files successfully completes or an activity id for
     * a SAMASTAGEFILES activity if stage_files is still running.
     *
     */
    public static native String stageFiles(Ctx c, String[] filepaths,
        int options)
        throws SamFSException;


    /**
     * get names of files in staging queue
     *
     * @param mediaType
     * @param vsn
     *
     * @return String[] array of file names
     */
    public static native String[] getFileNamesInQueue(Ctx c, String mediaType,
        String vsn) throws SamFSException;
}
