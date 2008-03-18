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

// ident	$Id: CopyFields.java,v 1.4 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

public interface CopyFields {
    /*
     * the two divs :
     *  - copyDivName -> copy is present
     *  - noCopyDivName -> this copy is not present
     */
    public static final String COPY_DIV = "copyDivName";
    public static final String NO_COPY_DIV = "noCopyDivName";
    public static final String ADD_COPY_ID = "addCopyId";
    public static final String REMOVE_COPY_ID = "removeCopyId";
    public static final String COPY_OPTIONS_ID = "editCopyOptionsId";

    // input fields
    public static final String COPY_NUMBER = "copyNumber";
    public static final String COPY_TIME = "copyTime";
    public static final String COPY_TIME_UNIT = "copyTimeUnit";
    public static final String EXPIRATION_TIME = "expirationTime";
    public static final String EXPIRATION_TIME_UNIT = "expirationTimeUnit";
    public static final String NEVER_EXPIRE = "neverExpire";
    public static final String RELEASER_BEHAVIOR = "releaserBehavior";
    public static final String MEDIA_POOL = "mediaPool";
    public static final String SCRATCH_POOL = "scratchPool";
    public static final String AVAILABLE_MEDIA = "availableMedia";
    public static final String AVAILABLE_MEDIA_HREF = "availableMediaHref";
    public static final String AVAILABLE_MEDIA_STRING =
        "availableMediaString";
    public static final String ENABLE_RECYCLING = "enableRecycling";
    public static final String MEDIA_TYPE = "mediaType";
    public static final String ASSIGNED_VSNS = "assignedVSNs";

    // buttons
    public static final String ADD_COPY = "addCopy";
    public static final String REMOVE_COPY = "removeCopy";
    public static final String COPY_OPTIONS = "editCopyOptions";

    // hidden fields
    public static final String COPY_LIST = "availableCopyList";
    public static final String UI_RESULT = "uiResult";
    public static final String TV_PREFIX = "tiledViewPrefix";
}
