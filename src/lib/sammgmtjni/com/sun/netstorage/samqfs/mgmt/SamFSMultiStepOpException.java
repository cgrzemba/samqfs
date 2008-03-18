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

// ident	$Id: SamFSMultiStepOpException.java,v 1.6 2008/03/17 14:43:55 am143972 Exp $
package com.sun.netstorage.samqfs.mgmt;

public class SamFSMultiStepOpException extends SamFSException {

    protected int failedStep;

    /**
     *  @return the step that failed in this multistep operation
     *  or -1 if initial checks failed
     *
     * This allows the upper layer to generate an appropriate error message.
     * (the error message included in this exception pertains to the specific
     * step that failed to complete).
     */
    public int getFailedStep() {
        return failedStep;
    }

    protected SamFSMultiStepOpException(String msg, int errno, int failedStep) {
        super(msg, errno);
        this.failedStep = failedStep;
    }

}
