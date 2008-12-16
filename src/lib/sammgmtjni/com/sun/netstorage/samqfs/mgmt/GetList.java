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

// ident	$Id: GetList.java,v 1.6 2008/12/16 00:08:53 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt;

/**
 * native utility methods
 */
public class GetList {

    private int totalCount;
    private String [] fileDetails;

    public GetList(int totalCount, String [] fileDetails) {
        this.totalCount = totalCount;
        this.fileDetails  = fileDetails;
    }

    public int  getTotalCount() { return totalCount; }
    public String [] getFileDetails() { return fileDetails; }
}
