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

// ident	$Id: FaultSummary.java,v 1.8 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

public class FaultSummary {

    public int critical; // number of critical faults
    public int major;    // number of major faults
    public int minor;    // number of minor faults

    protected FaultSummary(int critical, int major, int minor) {
        this.critical = critical;
        this.major    = major;
        this.minor    = minor;
    }

    public String toString() {
        String s = "Faults(" + critical + "," + major + "," + minor + ")";
        return s;
    }
}
