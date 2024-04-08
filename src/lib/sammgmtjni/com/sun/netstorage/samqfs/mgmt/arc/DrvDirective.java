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

// ident	$Id: DrvDirective.java,v 1.10 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class DrvDirective {

    // this value must match those defined in pub/mgmt/directives.h
    private static final long DD_count = 0x00000001;

    private String autoLib;
    private int count;
    private long chgFlags;

    private DrvDirective(String autoLib, int count, long chgFlags) {
        this.autoLib = autoLib;
        this.count = count;
        this.chgFlags = chgFlags;
    }

    /**
     * public constructor
     */
    public DrvDirective(String autolib, int count) {
        this(autolib, count, ~(long)0);
    }

    // public methods

    public String getAutoLib() { return autoLib; }

    public int getCount() { return count; }
    public void setCount(int count) {
        this.count = count;
        chgFlags |= DD_count;
    }
    public void resetCount() {
        chgFlags &= ~DD_count;
    }

    public String toString() {
        return (autoLib + ",drvcount=" + count);
    }
}
