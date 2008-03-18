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

// ident	$Id: SamFSTimeoutException.java,v 1.6 2008/03/17 14:43:55 am143972 Exp $
package com.sun.netstorage.samqfs.mgmt;

/**
 * exception thrown to indicate that a server operation is taking more than
 * the RPC timeout interval
 * This exception does NOT indicate a network problem
 * Since this exception is generated on the client side, it is not localized.
 * The Upper layer must generate an appropriate localized error message.
 */
public class SamFSTimeoutException extends SamFSException {

    public SamFSTimeoutException(String msg, int samerrno) {
        super(msg, samerrno);
    }

}
