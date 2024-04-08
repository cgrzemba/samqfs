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

// ident	$Id: BufferDirective.java,v 1.5 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

public interface BufferDirective {

    public int getMediaType();

    public long getSize();
    public void setSize(long size);

    public boolean isLocked();
    public void setLocked(boolean lock);

    // these methods are to be used for arch_max and overflow_min fields
    // in global archiving directive; the ones returning int should not
    // be used for arch_max and overflow_min.
    // Ideally, the ones returning int should be deleted, but then there
    // are other GUI pages that need to be corrected too with these methods.

    public String getSizeString();
    public void setSizeString(String sizeStr);
}
