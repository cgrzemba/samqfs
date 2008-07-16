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

// ident	$Id: ProtoFSDetailsBean.java,v 1.2 2008/07/16 23:43:55 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import javax.servlet.http.HttpServletRequest;

public class ProtoFSDetailsBean {
    private String fsName = null;

    public ProtoFSDetailsBean() {
    }

    public String getPageTitleText() {
        String FS_NAME = Constants.PageSessionAttributes.FS_NAME;
        HttpServletRequest request = JSFUtil.getRequest();
        String fsname = request.getParameter(FS_NAME);
        if (fsname == null) {
            fsname = (String)JSFUtil.getAttribute(FS_NAME);
        } else {
            // this is the first time into the page, store away the fs name
            JSFUtil.setAttribute(FS_NAME, fsname);
        }

        return JSFUtil.getMessage("protofs.details.title", fsname);
    }

    public String getServerName() {
        String serverName = JSFUtil.getServerName();

        return serverName;
    }

    public void setServerName(String name) {
        // do nothing, server name is already saved by JSFUtil
    }
}
