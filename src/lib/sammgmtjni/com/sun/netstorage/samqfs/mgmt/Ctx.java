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

// ident	$Id: Ctx.java,v 1.13 2008/05/16 18:35:26 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt;

import com.sun.netstorage.samqfs.mgmt.SamFSConnection;

/**
 * REMOVE ME AFTER C clean up.
 *
 * dumpPath and readPath are no longer used.  Remove the following constructors:
 * 1. Ctx(SamFSConnection c, String dumpPath, String readPath)
 * 2. Ctx(SamFSConnection c, String dumpPath)
 */

/* Context object - needs to be passed as an arg to all native calls */
public class Ctx {

    private long handle;
    private String userID;
    private Auditable auditable;

    /**
     * Remove the next two variables
     */
    private String dumpPath;
    private String readPath;

    /*
     * public constructors
     */
    public Ctx(SamFSConnection c, String dumpPath, String readPath) {
        this.handle = c.connHandle;
        this.userID = null;

        /**
         * Remove the next two lines
         */
        this.dumpPath = "";
        this.readPath = "";
    }
    public Ctx(SamFSConnection c, String dumpPath) {
        this(c, dumpPath, null);
    }
    public Ctx(String userID, SamFSConnection c) {
        this(c, null, null);
        this.userID = userID;
    }
    public Ctx(SamFSConnection c) {
        this(c, null, null);
    }

    public Ctx(SamFSConnection c, Auditable a) {
        this(c, null, null);
        this.auditable = a;
    }

    /*
     * public instance methods
     */

    public String getUserID() {
        return auditable.getUserID();
    }

    public void setUserID(String userID) {
        this.userID = userID;
    }
}
