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

// ident	$Id: SamFSIncompatVerException.java,v 1.9 2008/12/16 00:08:53 am143972 Exp $
package com.sun.netstorage.samqfs.mgmt;

/**
 * exception thrown to indicate that client and server are running incompatible
 * versions of the SAM-FS/QFS management software (incompatible API versions)
 *
 * This Exception does not include an error message/number.
 * A localized message should be generated by the upper layer, if needed.
 *
 * Note: client/server code changes do not always result in such an Exception.
 * Server may be updated and an Exception will NOT be thrown as long as the
 * RPC interface stays unchanged.
 */
public class SamFSIncompatVerException extends SamFSException {

    private boolean clientNewer;

    /**
     * @return true if client uses an incompatible newer API, or
     * false if the server uses an incompatible newer API
     */
    public boolean isClientVerNewer() {
        return clientNewer;
    }

    protected SamFSIncompatVerException(boolean clientNewer) {
            super(SamFSException.INCOMPAT_VERSIONS);
            this.clientNewer = clientNewer;
    }

}
