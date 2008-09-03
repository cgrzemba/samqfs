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

// ident	$Id: ProtoFSDetailsBean.java,v 1.4 2008/09/03 19:46:03 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import javax.servlet.http.HttpServletRequest;
import com.sun.netstorage.samqfs.web.model.MemberInfo;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;

public class ProtoFSDetailsBean {
    private String fsName = null;

    private int clientCount = 0;

    public ProtoFSDetailsBean() {

        initBean();
    }

    private void initBean() {
        String serverName = JSFUtil.getServerName();

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();

            MemberInfo [] memberInfo = appModel.getSamQFSSystemSharedFSManager()
                .getSharedFSSummaryStatus(JSFUtil.getServerName(),
                                          getFileSystemName());

            if (memberInfo != null) {
                for (int i = 0; i < memberInfo.length; i++) {
                    if (memberInfo[i].isClients()) {
                        clientCount = memberInfo[i].getClients();
                    }
                }
            }
        } catch (SamFSException sfe) {
            //
        }
    }

    public String getPageTitleText() {
        return JSFUtil.getMessage("protofs.details.title", getFileSystemName());

    }

    protected String getFileSystemName() {
        if (this.fsName != null)
            return this.fsName;

        // must be the first time on this page, retrieve the fs name from the
        // request
        String FS_NAME = Constants.PageSessionAttributes.FS_NAME;
        HttpServletRequest request = JSFUtil.getRequest();
        this.fsName = request.getParameter(FS_NAME);
        if (this.fsName == null) {
            this.fsName = (String)JSFUtil.getAttribute(FS_NAME);
        } else {
            // this is the first time into the page, store away the fs name
            JSFUtil.setAttribute(FS_NAME, fsName);
        }

        return this.fsName;
    }

    public String getServerName() {
        String serverName = JSFUtil.getServerName();

        return serverName;
    }

    public String getCreateFSHelpText() {
        return JSFUtil.getMessage("protofs.details.createfshelp",
                                  new String [] {getFileSystemName(),
                                                 getServerName()});
    }

    public String getAddClientsHelpText() {
        return JSFUtil.getMessage("protofs.details.addclientshelp",
                                 "" + this.clientCount);
    }
}
