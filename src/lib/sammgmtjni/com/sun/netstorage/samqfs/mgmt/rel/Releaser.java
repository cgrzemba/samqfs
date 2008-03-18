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

// ident	$Id: Releaser.java,v 1.8 2008/03/17 14:44:01 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.rel;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Ctx;

public class Releaser {

    /*
     * options masks for release files must match the RL_OPT flags
     * in pub/mgmt/release.h
     */
    public static final int RECURSIVE = 0x00000001;
    public static final int NEVER  = 0x00000002;
    public static final int WHEN_1 = 0x00000004;
    public static final int PARTIAL = 0x00000008;
    public static final int RESET_DEFAULTS  = 0x00000010;


    public static native ReleaserDirective getDefaultDirective(Ctx c)
        throws SamFSException;

    public static native ReleaserDirective getGlobalDirective(Ctx c)
        throws SamFSException;

    public static native void setGlobalDirective(Ctx c, ReleaserDirective rd)
        throws SamFSException;


    /**
     * Release files and directories or set releaser options for them.
     *
     * If any options other than RECURSIVE is specified the disk space
     * will not be released. Instead the indicated options will be set
     * for each file in the file list.
     *
     * If RECURSIVE is specified and there are multiple entries in the
     * files array all directories must come before individual files.
     *
     * If the RESET_DEFAULTS flag is set, each files options will be
     * reset to their default values. If RESET_DEFAULTS is set along with
     * other options, the files will be reset to default and then
     * have the other options set.
     *
     * The NEVER and WHEN_1_COPY are mutually exclusive.
     *
     * partial_size specifies the number of KB to be retained if the
     * file is released from the disk cache. The value must be a multiple
     * of 8 and 8 <= partial_size <= maxpartial mount option(default 16).
     *
     * If PARTIAL is specified but partial_size is not, the mount
     * option partial will be applied.
     *
     * @return jobId of the release request job or null if the request
     *	was processed completely.
     */
    public static native String releaseFiles(Ctx c, String[] files,
	int options, int partial_size)
        throws SamFSException;
}
