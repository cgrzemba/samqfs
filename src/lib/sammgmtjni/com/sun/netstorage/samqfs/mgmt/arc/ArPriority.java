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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: ArPriority.java,v 1.7 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class ArPriority {

    private String name;
    private float prio;
    private int chgFlags;

    private ArPriority(String name, float prio, int chgFlags) {
            this.name = name;
            this.prio = prio;
            this.chgFlags = chgFlags;
    }

    public String toString() {
        String s = name + "," + prio +
            " [cf:0x" + Integer.toHexString(chgFlags) + "]";
        return s;
    }
}
