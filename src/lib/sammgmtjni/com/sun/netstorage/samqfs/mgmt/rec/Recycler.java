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

// ident	$Id: Recycler.java,v 1.7 2008/03/17 14:44:00 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.rec;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/*
 * recycling related native methods
 */
public class Recycler {

    public static native RecyclerParams getDefaultParams(Ctx c)
        throws SamFSException;

    /* retrieve recycle params for all installed libraries */
    public static native LibRecParams[] getAllLibRecParams(Ctx c)
        throws SamFSException;

    /* retrieve recycle params for specified library */
    public static native LibRecParams getLibRecParams(Ctx c, String libPath)
        throws SamFSException;
    /* set recycle params for specified library */
    public static native void setLibRecParams(Ctx c, LibRecParams libRecParams)
        throws SamFSException;

    // path includes the log filename
    public static native String getLogPath(Ctx c)
        throws SamFSException;
    public static native void setLogPath(Ctx c, String path)
        throws SamFSException;
    /**
     * by default, no logging is done. however, there is a default log location
     * that can be used as an argument to the set method above.
     */
    public static native String getDefaultLogPath(Ctx c)
        throws SamFSException;

    // these values must match those defined in pub/mgmt/recyc_sh_wrap.h
    public static final int RC_label_on  = 0x00000001;
    public static final int RC_export_on = 0x00000010;

    public static native int getActions(Ctx c)
        throws SamFSException;

    public static native void addActionLabel(Ctx c)
        throws SamFSException;
    public static native void addActionExport(Ctx c, String emailAddr)
        throws SamFSException;
    public static native void delAction(Ctx c)
        throws SamFSException;
}
