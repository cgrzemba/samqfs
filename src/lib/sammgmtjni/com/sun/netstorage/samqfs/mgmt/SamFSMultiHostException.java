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

// ident	$Id: SamFSMultiHostException.java,v 1.9 2008/12/16 00:08:53 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt;

/**
 * Used to return a set of exceptions that occurred on
 * multiple hosts during a single logical operation.
 */
public class SamFSMultiHostException extends SamFSException {

    private SamFSException[] exceptionArr;
    private String[] hostNameArr;
    private Object partialResult;

    public SamFSMultiHostException(String msg, SamFSException[] exceptions,
        String[] hostNameArr, Object partialResult) {
        super(msg);
        this.exceptionArr = exceptions;
        this.hostNameArr = hostNameArr;
        this.partialResult = partialResult;
    }

    public SamFSMultiHostException(String msg, SamFSException[] exceptions,
        String[] hostNameArr) {
        this(msg, exceptions, hostNameArr, null);
    }

    public SamFSMultiHostException(String msg) {
        this(msg, null, null, null);
    }

    /**
     * Returns an array of exceptions.  Each error occurred on the
     * corresponding host in the array returned by getHostNames().
     */
    public SamFSException[] getExceptions() {
        return exceptionArr;
    }

    /**
     * Returns an array of host names.  The corresponding error messages
     * can be obtained by calling getExceptions().
     */
    public String[] getHostNames() {
        return hostNameArr;
    }

    /**
     * Returns the partial result as a generic Object, which can be cast
     * to the expected return value of the function that threw this exception.
     * May also return null.
     */
    public Object getPartialResult() {
        return partialResult;
    }
}
