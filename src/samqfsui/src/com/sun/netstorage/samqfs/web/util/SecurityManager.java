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

// ident	$Id: SecurityManager.java,v 1.7 2008/05/16 18:39:07 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

/**
 * this interface defines the FS Manager Security interface.
 */
public interface SecurityManager {

    /**
     * determine if the currently logged in user has the <code>auth</code>
     * authorization
     *
     * @param Authorization auth -
     * @return boolean - true if the user has the current authorization
     */
    public boolean hasAuthorization(Authorization auth);

    // temp method for gui testing
    public String getCurrentAuthorization();

    /**
     * retrieve the username of the currently logged in user
     */
    public String getUserName();
}
