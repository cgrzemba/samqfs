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

// ident	$Id: VSNOp.java,v 1.10 2008/03/17 14:43:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
/*
 * VSN related native methods
 */
public class VSNOp {

    public static native VSNPool[] getPools(Ctx c) throws SamFSException;
    public static native VSNPool getPool(Ctx c, String poolName)
        throws SamFSException;
    /* VSN sort methods are defined in Media.java */
    public static native VSNPoolProps getPoolProps(Ctx c, String poolName,
        int start, int size, short sortMet, boolean ascending)
        throws SamFSException;

    /**
     * @return the name of the Copy using this pool or null if pool not used
     */
    public static native String getCopyUsingPool(Ctx c, String poolName)
        throws SamFSException;

    public static native void addPool(Ctx c, VSNPool pool)
        throws SamFSException;
    public static native  void modifyPool(Ctx c, VSNPool pool)
        throws SamFSException;
    public static native void removePool(Ctx c, String poolName)
        throws SamFSException;

    public static native VSNMap[] getMaps(Ctx c) throws SamFSException;
    public static native VSNMap getMap(Ctx c, String copyName)
        throws SamFSException;

    public static native void addMap(Ctx c, VSNMap map)
        throws SamFSException;
    public static native  void modifyMap(Ctx c, VSNMap map)
        throws SamFSException;
    public static native void removeMap(Ctx c, String copyName)
        throws SamFSException;

    /*
     * methods below have been added in 4.4 and do not have counterparts
     * on 4.3/older servers
     */

    /**
     * works for both tape and disk volumes
     */
    public static native BaseVSNPoolProps getPoolPropsByPool(
        Ctx c, VSNPool pool, int start,
        int size, short sortMet, boolean ascending)
        throws SamFSException;

    /**
     * currently works for tape volumes only
     */
    public static native BaseVSNPoolProps getPoolPropsByMap(
        Ctx c, VSNMap map, int start,
        int size, short sortMet, boolean ascending)
        throws SamFSException;

}
