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

// ident	$Id: EQ.java,v 1.10 2008/12/16 00:08:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/* Equipment Ordinal */
public class EQ {

    private int first_free; // first free eq
    private int[] in_use; // array of eqs in use

    private EQ(int[] in_use, int first_free) {
        this.first_free = first_free;
        this.in_use = in_use;
    }


    public int getFirstFreeEq() { return first_free; }
    public int[] getEqsInUse() { return in_use; }

    public String toString() {
            String s = "First free eq: " + Integer.toString(first_free) +
                "Eqs in Use: ";
            if (in_use != null) {
                for (int i = 0; i < in_use.length; i++) {
                    s += "   " + in_use[i] + "\n";
                }
            } else {
              s += "  none\n";
            }

            return s;
    }

}
