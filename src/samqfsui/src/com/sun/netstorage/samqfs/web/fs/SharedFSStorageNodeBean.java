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

// ident        $Id: SharedFSStorageNodeBean.java,v 1.2 2008/06/11 23:03:36 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.component.Hyperlink;

public class SharedFSStorageNodeBean {

    /** Page Modes for target pages */
    

    public SharedFSStorageNodeBean() {
    }

    public Hyperlink [] getBreadCrumbs(String fsName){
        Hyperlink [] links = new Hyperlink[3];
        for (int i = 0; i < links.length; i++){
            links[i] = new Hyperlink();
            links[i].setImmediate(true);
            links[i].setId("breadcrumbid" + i);
            if (i == 0){
                // Back to FS Summary
                links[i].setText(JSFUtil.getMessage("FSSummary.title"));
                links[i].setUrl(
                    "/fs/FSSummary?" +
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME + "=" +
                    JSFUtil.getServerName());
            } else if (i == 1){
                // Back to Shared FS Summary
                links[i].setText(JSFUtil.getMessage("SharedFS.title"));
                links[i].setUrl(
                    "SharedFSSummary.jsp?" +
                     Constants.PageSessionAttributes.SAMFS_SERVER_NAME + "=" +
                     JSFUtil.getServerName() + "&" +
                     Constants.PageSessionAttributes.FILE_SYSTEM_NAME + "=" +
                     fsName);
            } else {
                // client / sn page
                links[i].setText(
                    JSFUtil.getMessage("SharedFS.pagetitle.sn"));
            }
        }

        return links;
    }
}
