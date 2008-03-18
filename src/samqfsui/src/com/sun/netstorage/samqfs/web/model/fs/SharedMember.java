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

// ident	$Id: SharedMember.java,v 1.7 2008/03/17 14:43:45 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

public interface SharedMember {
    public final static int TYPE_NOT_SHARED = -1;
    public final static int TYPE_MD_SERVER = 0;
    public final static int TYPE_POTENTIAL_MD_SERVER = 1;
    public final static int TYPE_CLIENT = 2;

    public String getHostName();
    public String[] getIPs();
    public int getType();
    public boolean isMounted();
}
