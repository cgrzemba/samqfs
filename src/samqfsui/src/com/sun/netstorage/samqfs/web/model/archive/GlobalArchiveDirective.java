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

// ident	$Id: GlobalArchiveDirective.java,v 1.14 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

/*
 * The getters of this interface can be mapped from
 * ar_global_directive in archive.h
 */
public interface GlobalArchiveDirective {

    // static ints for archiver scan method
    public static final int CONTINUOUS_SCAN = 0;
    public static final int SCAN_DIRS = 1;
    public static final int SCAN_INODES = 2;
    public static final int NO_SCAN = 3;
    public static final int SCAN_NOT_SET = 4;

    public BufferDirective[] getMaxFileSize();

    public void addMaxFileSizeDirective(BufferDirective dir);
    public void deleteMaxFileSizeDirective(BufferDirective dir);

    public BufferDirective[] getMinFileSizeForOverflow();
    public void addMinFileSizeForOverflow(BufferDirective dir);
    public void deleteMinFileSizeForOverflow(BufferDirective dir);

    public BufferDirective[] getBufferSize();
    public void addBufferSize(BufferDirective dir);
    public void deleteBufferSize(BufferDirective dir);

    public DriveDirective[] getDriveDirectives();
    public void addDriveDirective(DriveDirective dir);
    public void deleteDriveDirective(DriveDirective dir);

    public String getArchiveLogfile();
    public void setArchiveLogfile(String logFile);

    public long getInterval();
    public void setInterval(long interval);

    public int getIntervalUnit();
    public void setIntervalUnit(int unit);

    public int getArchiveScanMethod();
    public void setArchiveScanMethod(int method);

    // this method needs to be called for any change in
    // GlobalArchiveDirective to take effect, i.e. after the
    // client is done calling changeXXX() or setXXX() methods,
    // this method needs to be called for these changes to be
    // effective
    public void changeGlobalDirective() throws SamFSException;
}
