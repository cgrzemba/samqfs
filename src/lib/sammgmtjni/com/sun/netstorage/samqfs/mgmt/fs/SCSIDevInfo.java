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

// ident	$Id: SCSIDevInfo.java,v 1.10 2008/12/16 00:08:55 am143972 Exp $
package com.sun.netstorage.samqfs.mgmt.fs;


public class SCSIDevInfo {

    public String vendor;
    public String prodID;
    public String version;
    public String devID; // used to uniquely identify a shared disk

    private SCSIDevInfo(String vendor, String prodID, String version,
			String devID) {
        this.vendor = vendor;
        this.prodID = prodID;
        this.version = version;
	this.devID = devID;
    }

    public String toString() {
        return (new String(vendor + "," + prodID + "," + version + "," +
			  ((null == devID) ? "nodevid" : devID)));
    }
}
