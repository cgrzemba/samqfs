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

// ident	$Id: StageJob.java,v 1.13 2008/03/17 14:43:51 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

public interface StageJob extends BaseJob {

    public static int SORT_BY_FILE_NAME = 11;
    // get all stage file entries
    public static int ALL_FILES = -1;

    public String getFileSystemName();

    public String getVSNName();

    public int getMediaType();

    public String getPosition();

    public String getOffset();

    public String getFileName();

    public String getFileSize();

    public String getStagedFileSize();

    public String getInitiatingUserName();

    public StageJobFileData[] getFileData() throws SamFSException;

    public long getNumberOfFiles() throws SamFSException;

    public int getStateFlag();

    /**
     * sortBy - has to use definitions in StagerJob.java
     */
    public StageJobFileData[] getFileData(int start,
                                          int size,
                                          short sortby,
                                          boolean ascending)
        throws SamFSException;

}
