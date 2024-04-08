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

// ident	$Id: Fault.java,v 1.13 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class Fault {

    public final static int ALL_FAULTS = -1; // must match GET_ALL in faults.h

    /**
     * @return a list of all faults (least recent first)
     */
    /** @deprecated since 4.6, Use getAll() instead */
    public static native FaultAttr[] get(Ctx c, int numFaults, byte severity,
        byte state, long errID) throws SamFSException;

    public static native FaultAttr[] getAll(Ctx c) throws SamFSException;

    public static native FaultAttr[] getByLibName(Ctx c, String libName)
        throws SamFSException;

    public static native FaultAttr[] getByLibEq(Ctx c, int eq)
        throws SamFSException;

    /** @deprecated  Use ack()/delete() instead */
    public static native void update(Ctx c, long errID, String hostname,
        byte newstate) throws SamFSException;

    public static native void ack(Ctx c, long errIDs[])
        throws SamFSException;
    public static native void delete(Ctx c, long errIDs[])
        throws SamFSException;

    /**
     * @return true if fault information generation is enabled.
     * NOTE: If false, then the data returned by the functions above might
     * not be the most current data.
     */
    public static native boolean isOn(Ctx c)
        throws SamFSException;

    public static native FaultSummary getSummary(Ctx c)
        throws SamFSException;
}
