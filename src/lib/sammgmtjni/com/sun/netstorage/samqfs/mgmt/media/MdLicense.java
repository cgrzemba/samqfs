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

// ident	$Id: MdLicense.java,v 1.10 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

/* media license information */
public class MdLicense {

    private String mediaType;
    private int maxSlots;
    private int robotType = -1; // added in 4.3

    /* private constructor */
    private MdLicense(String mediaType, int maxSlots, int robotType) {
        this.mediaType = mediaType;
        this.maxSlots  = maxSlots;
        this.robotType = robotType;
    }

    // public methods

    public String getMediaType() { return mediaType; }
    public int getMaxSlots() { return maxSlots; }
    public int getRobotType() { return robotType; }

    public String toString() {
        return robotType + "," + mediaType + "," + maxSlots;
    }
}
