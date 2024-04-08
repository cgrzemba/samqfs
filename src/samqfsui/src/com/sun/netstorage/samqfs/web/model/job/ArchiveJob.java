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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: ArchiveJob.java,v 1.13 2008/12/16 00:12:22 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

public interface ArchiveJob extends BaseJob {

    public static final int ARCHIVE_SCANNING = 1;
    public static final int ARCHIVE_IDLE = 2;
    public static final int ARCHIVE_SLEEPING = 3;
    public static final int ARCHIVE_STOPPED = 4;

    public String getFileSystemName();

    public String getPolicyName();

    public int getCopyNumber();

    public String getVSNName();

    public int getMediaType();

    // I think this can be derived from the fact the getDrive() call
    // on VSN should return null in case it is not loaded yet
    public boolean isVSNLoaded() throws SamFSException;

    public String getStageVSNName();

    public int getStageMediaType();

    // I think this can be derived from the fact the getDrive() call
    // on VSN should return null in case it is not loaded yet
    public boolean isStageVSNLoaded() throws SamFSException;

    public int getTotalNoOfFilesAlreadyCopied();

    public long getDataVolumeAlreadyCopied();

    public String getCurrentFileName();

    public int getTotalNoOfFilesToBeCopied();

    public long getDataVolumeToBeCopied();

    public int getArchivingStatus() throws SamFSException;
}
