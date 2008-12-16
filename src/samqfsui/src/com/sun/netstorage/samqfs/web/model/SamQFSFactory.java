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

// ident	$Id: SamQFSFactory.java,v 1.26 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

public class SamQFSFactory {
    private static SamQFSAppModel model = null;
    public static SamQFSAppModel getSamQFSAppModel() throws SamFSException {
        if (model == null) {
            SamQFSUtil.doPrint("Loading library...");
            System.loadLibrary("fsmgmtjni");
            SamQFSUtil.doPrint("Done...");
            model = new com.sun.netstorage.samqfs.web.model.impl.
                jni.SamQFSAppModelImpl();
            SamQFSUtil.doPrint("Out of model constructor...");
        }

        return model;
    }
}
