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

// ident	$Id: LibRecParams.java,v 1.9 2008/05/16 18:35:29 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.rec;

public class LibRecParams extends RecyclerParams {

    private String path; /* path to library device */

    /**
     * private constructor
     */
    private LibRecParams(int hwm, String datasize, boolean ignore,
        String email, int minGain, int vsnCount, int minObs, long chgFlags,
        String path) {
            super(hwm, datasize, ignore, email,
                minGain, vsnCount, minObs, chgFlags);
            this.path = path;
    }

    /**
     * public constructor
     */
    public LibRecParams(String path) {
        super();
        this.path = path;
    }

    public String getPath() { return path; }

    public String toString() {
        return path + super.toString();
    }

}
