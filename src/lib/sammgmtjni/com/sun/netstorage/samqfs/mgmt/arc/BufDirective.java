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

// ident	$Id: BufDirective.java,v 1.9 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class BufDirective {

    // these values must match those defined in pub/mgmt/directive.h */

    private static final long BD_size = 0x00000002;
    private static final long BD_lock = 0x00000004;

    private String mediaType;
    private String size; // bytes.
    private boolean lock;
    private long chgFlags;

    /**
     * private constructor
     */
    private BufDirective(String mediaType, String size, boolean lock,
        long chgFlags) {
            this.mediaType = mediaType;
            this.size = size;
            this.lock = lock;
            this.chgFlags = chgFlags;
    }

    /**
     * public constructor
     */
    public BufDirective(String mediaType, String size, boolean lock) {
        this(mediaType, size, lock, ~(long)0);
    }

    public String getMediaType() { return mediaType; }

    public String getSize() { return size; }
    public void setSize(String size) {
            this.size = size;
            chgFlags |= BD_size;
    }
    public void resetSize() {
        chgFlags &= ~BD_size;
    }

    public boolean isLock() { return lock; }
    public void setLock(boolean lock) {
            this.lock = lock;
            chgFlags |= BD_lock;
    }
    public void resetLock() {
        chgFlags &= ~BD_lock;
    }

    public String toString() {
        return (mediaType + ",sz=" + size + ",lock=" + (lock ? "T" : "F"));
    }
}
