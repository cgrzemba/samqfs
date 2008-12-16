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

// ident	$Id: ImportOpts.java,v 1.9 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;


public class ImportOpts {

    private String vsn;
    private String barcode;
    private String mediaType;
    private boolean audit, foreignTape;
    /* options for network-attached StorageTek automated libraries only */
    private int volCount;
    private long pool;

    public ImportOpts(String vsn, String barcode, String mediaType,
        boolean audit, boolean foreignTape,
        /* StorageTek options */
        int volCount, long pool) {
            this.vsn = vsn;
            this.barcode = barcode;
            this.mediaType = mediaType;
            this.audit = audit;
            this.foreignTape = foreignTape;
            this.volCount = volCount;
            this.pool = pool;
    }

    public ImportOpts(String vsn, String barcode, String mediaType,
        boolean audit, boolean foreignTape) {
            this(vsn, barcode, mediaType, audit, foreignTape, -1, -1);
    }
}
