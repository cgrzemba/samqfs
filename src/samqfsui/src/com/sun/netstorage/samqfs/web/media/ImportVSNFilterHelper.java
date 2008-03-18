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

// ident    $Id: ImportVSNFilterHelper.java,v 1.7 2008/03/17 14:43:39 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import java.io.Serializable;

/**
 * This class encapsulates basic information about the Import VSN Filter.
 * It now only contains an array of int for pools.  The rest of the coomponents
 * like lsms, panels, min/max rows/cols are declared as unnecessary in 5.0.
 * They have been removed.
 */
public class ImportVSNFilterHelper implements Serializable {

    protected int [] pools;

    public ImportVSNFilterHelper(int [] pools)
        throws SamFSException {

        this.pools  = pools;
    }

    public int [] getPools()  { return pools; }


    public String toString() {
        String s = "";

        if (pools == null) {
            s += "pools is null. ";
        } else {
            for (int i = 0; i < pools.length; i++) {
                s += ("pools[" + i + "] is " + pools[i] + "; ");
            }
        }

        return s;
    }
}
