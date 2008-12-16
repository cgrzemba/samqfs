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

// ident	$Id: SunPlexManagerView.java,v 1.6 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;

public class SunPlexManagerView extends RequestHandlingViewBase
                                implements CCPagelet {

    public static final String SUNPLEX_LINK = "SunPlexManagerLink";
    public static final String SUNPLEX_LINK_TEXT = "SunPlexManagerLinkText";
    public static final String SUNPLEX_DESC = "SunPlexManagerDescription";
    public static final String SUNPLEX_LABEL = "SunPlexLabel";

    public SunPlexManagerView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(SUNPLEX_LINK, CCHref.class);
        registerChild(SUNPLEX_LINK_TEXT, CCStaticTextField.class);
        registerChild(SUNPLEX_DESC, CCStaticTextField.class);
        registerChild(SUNPLEX_LABEL, CCLabel.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(SUNPLEX_LINK)) {
            return new CCHref(this, name, null);
        } else if (name.equals(SUNPLEX_DESC) ||
                   name.equals(SUNPLEX_LINK_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(SUNPLEX_LABEL)) {
            return new CCLabel(this, name, null);
        } else {
            throw new IllegalArgumentException("invalid child '" + name +"'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        // sun plex manager
        CCHref link = (CCHref)getChild(SUNPLEX_LINK);
        CCStaticTextField linkText =
            (CCStaticTextField)getChild(SUNPLEX_LINK_TEXT);
        CCStaticTextField desc = (CCStaticTextField)getChild(SUNPLEX_DESC);

        NonSyncStringBuffer  url = new NonSyncStringBuffer();
        url.append("https://");
        url.append(serverName);

        // default to not installed
        int sunPlexManagerState = 4;
        try {
            sunPlexManagerState =
                SamUtil.getModel(serverName).getPlexMgrState();
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "getPageletUrl",
                                     "unable to determine if cluster node",
                                     serverName);
            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "fs.details.iscluster.error",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        switch (sunPlexManagerState) {
            case SamQFSSystemModel.PLEXMGR_RUNNING:
                linkText.setValue("fs.cluster.sunplex.launch");
                desc.setValue("");
                url.append(":6789/SunPlexManager");
                break;

            case SamQFSSystemModel.PLEXMGR_INSTALLED_AND_REG:
                linkText.setValue("");
                desc.setValue("fs.cluster.sunplex.startlockhart");
                break;

            case SamQFSSystemModel.PLEXMGR_INSTALLED_NOT_REG:
                linkText.setValue("");
                desc.setValue("fs.cluster.sunplex.register");
                break;

            case SamQFSSystemModel.PLEXMGROLD_INSTALLED:
                linkText.setValue("fs.cluster.sunplex.launch");
                desc.setValue("");
                url.append(":3000");
                break;

            case SamQFSSystemModel.PLEXMGR_NOTINSTALLED:
                linkText.setValue("");
                desc.setValue("fs.cluster.sunplex.install");
                break;
            default:
        }

        // append the javascript to launch the new window
        link.setExtraHtml("onClick=\"window.open(\'" + url.toString() +
                          "\', \'new_window\'); return false;\"");


        TraceUtil.trace3("Exiting");
    }

    public String getPageletUrl() {
        CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();
        String serverName = parent.getServerName();
        String fsName = (String)
            parent.getPageSessionAttribute(Constants.fs.FS_NAME);

        boolean hafs = false;
        try {
            hafs =  SamUtil.getModel(serverName).getSamQFSSystemFSManager().
                getGenericFileSystem(fsName).isHA();
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "getPageletUrl",
                                     "unable to determine if cluster node",
                                     serverName);
            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "fs.details.iscluster.error",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        if (hafs) {
            return "/jsp/fs/SunPlexManagerPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
