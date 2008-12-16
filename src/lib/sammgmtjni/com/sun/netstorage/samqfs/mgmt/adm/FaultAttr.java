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

// ident	$Id: FaultAttr.java,v 1.11 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

public class FaultAttr {

    // fault state (must match the enum defined in pub/mgmt/faults.h
    public final static byte FLT_ST_UNRESOLVED   = 0;
    public final static byte FLT_ST_ACKED = 1;

    // fault severity (must match the enum defined in pub/mgmt/faults.h
    public final static byte FLT_SEV_CRITICAL = 0;
    public final static byte FLT_SEV_MAJOR    = 1;
    public final static byte FLT_SEV_MINOR    = 2;

    // no library specified (must match the definition in pub/mgmt/faults.h
    public final static String NO_LIBRARY = "No Library";

    private long errID;
    private String compID;
    private byte severity;
    private long timestamp;
    private String hostname;
    private String message;
    private byte state;
    private String libName;
    private int eq;

    /**
     * private constructor (no public constructor needed)
     */
    private FaultAttr(long errID, String compID, byte severity, long timestamp,
        String hostname, String message, byte state, String libName, int eq) {
            this.errID = errID;
            this.compID = compID;
            this.severity = severity;
            this.timestamp = timestamp;
            this.hostname = hostname;
            this.message = message;
            this.state = state;
            this.libName = libName;
            this.eq = eq;
    }

    // public methods

    public long getErrID() { return errID; }
    public String getComponentID() { return compID; }
    public byte getSeverity() { return severity; }
    public long getTimestamp() { return timestamp; }
    public String getHostname() { return hostname; }
    public String getMessage() { return message; }
    public byte getState() { return state; }
    public String getLibName() { return libName; }
    public int getEq() { return eq; }

    public String toString() {
        String s = "err:" + errID + "," + compID + ",sev:" + severity +
        ",time:" + timestamp + "," + hostname + "," + state + "," + libName
        + "," + eq;
        return s;
    }
}
