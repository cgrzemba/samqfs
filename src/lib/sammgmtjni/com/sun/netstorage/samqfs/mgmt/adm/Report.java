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

// ident	$Id: Report.java,v 1.13 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.mgmt.media.StkClntConn;

public class Report {

    public static final String REPORTS_DIR = "/var/opt/SUNWsamfs/reports/";

    public static final int TYPE_ALL = 0;
    public static final int TYPE_FS  = 1;
    public static final int TYPE_MEDIA = 2;
    public static final int TYPE_ACSLS = 3;

    // Sections to be included in the generic media report
    public static final int INCL_SUMMARY_VSN = 0x00000001;
    public static final int INCL_SUMMARY_POOL = 0x00000002;
    public static final int INCL_SUMMARY_MEDIA = 0x00000004;
    public static final int INCL_BAD_VSN  = 0x10000000;
    public static final int INCL_FOREIGN_VSN = 0x00080000;
    public static final int INCL_FULL_VSN = 0x00000800;
    public static final int INCL_NO_SLOT_VSN = 0x00200000;
    public static final int INCL_RO_VSN = 0x00800000;
    public static final int INCL_WRITEPROTECT_VSN = 0x01000000;
    public static final int INCL_RECYCLE_VSN = 0x00400000;
    // c- code has no way to distinguish between different reservation policies
    // i.e. by owner, group etc.
    public static final int INCL_RESERVED_VSN = 0x00000200;
    public static final int INCL_VSN_NEAR_CAPACITY = 0x00000400;
    public static final int INCL_INUSE_VSN = 0x00020000; // accessed today
    public static final int INCL_BLANK_VSN = 0x00040000;
    public static final int INCL_DUPLICATE_VSN = 0x00004000;

    // Sections to be included in the ACSLS media report
    public static final int INCL_DRIVE = 0x00004000;
    public static final int INCL_ACSLS_POOL = 0x00008000;
    public static final int INCL_IMPORTED_VSN = 0x00010000;
    public static final int INCL_ACCESSED_VSN = 0x00020000;
    public static final int INCL_LOCK = 0x00040000;


    /**
     * generate a report. The report parameters i.e. type, subtype, mailing
     * addresses, and STK ACSLS client connection information are provided as
     * input parameters.
     *
     * @param type report type (fs, generic media, acsls)
     * @param includeFlags   specific sections to be included in the report
     * @param email[]   array of email addresses to whom report is to be mailed
     * @param StkClntConn   ACSLS host and port information for ACSLS reports
     *
     */
    public static native void generate(
            Ctx c,
            int type,
            int includeFlags,
            String[] email,
            /* required if type = TYPE_ACSLS */
            StkClntConn conn)
            throws SamFSException;


    /**
     * delete a report
     */
    public static native void delete(Ctx c, String path) throws SamFSException;


    /**
     * retrieves all the reports currently existing in the system. Reports must
     * have been previously generated by using the generate() api
     *
     * @return String[]
     *  array of formated strings describing the report name and generated time.
     * The contents of the report should be retrieved using the tailFile() in
     * FileUtil
     */
    public static native String[] getAll(Ctx c) throws SamFSException;
}
