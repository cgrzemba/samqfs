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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: Util.java,v 1.13 2008/12/16 00:08:53 am143972 Exp $
package com.sun.netstorage.samqfs.mgmt;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * native utility methods
 */
public class Util {

    /**
     * write a message to syslog facility LOG_LOCAL6.debug
     */
    public static native void writeToSyslog(String msg);

    /**
     * set the trace level for the native code
     * 0=off - 3=most verbose
     */
    public static native void setNativeTraceLevel(int level);

    /**
     * check if this is a regular expresion according to regcmp(3C)
     */
    public static native boolean isValidRegExp(String regExp);

}
