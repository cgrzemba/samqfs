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

// ident	$Id: CreateFileSystemView.java,v 1.2 2008/06/11 21:16:19 kilemba Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.model.admin.Configuration;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCImageField;
import com.sun.web.ui.view.html.CCStaticTextField;

public class CreateFileSystemView extends RequestHandlingViewBase
                                  implements CCPagelet {


    // child views
    public static final String TILED_VIEW = "ProtoFSTiledView";
    public static final String IMAGE = "asteriskImg";
    public static final String STORAGE_NODE_HREF = "addStorageNodeHref";
    public static final String STORAGE_NODE_TEXT = "addStorageNodeText";
    public static final String CREATE_FS_HREF = "createFSonMDSHref";
    public static final String CREATE_FS_TEXT = "createFSonMFSText";
    public static final String PROTO_FS_HELP = "ProtoFSHelpText";

    public CreateFileSystemView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(TILED_VIEW, ProtoFSTiledView.class);
        registerChild(STORAGE_NODE_HREF, CCHref.class);
        registerChild(CREATE_FS_HREF, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.endsWith("Href")) {
            return new CCHref(this, name, null);
        } else if (name.endsWith("Text")) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(IMAGE)) {
            return new CCImageField(this, name, null);
        } else if (name.equals(TILED_VIEW)) {
            return new ProtoFSTiledView(this, name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        Configuration config =
            ((FirstTimeConfigViewBean)getParentViewBean()).getConfiguration();
        if (config.getProtoFSCount() == 1) {
            String fsName = config.getProtoFSName().get(0);

            ((CCStaticTextField)getChild(PROTO_FS_HELP)).setValue(
               SamUtil.getResourceString("firsttime.oneprotofs.found", fsName));
        }
    }

    // implement CCPagelet
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String url = "/jsp/archive/BlankPagelet.jsp";
        FirstTimeConfigViewBean parent =
            (FirstTimeConfigViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        Configuration config = parent.getConfiguration();

        int count = config.getProtoFSCount();
        if (count == 1) {
            url = "/jsp/admin/OneProtoFS.jsp";
        } else if (count > 1) {
            url = "/jsp/admin/ManyProtoFS.jsp";
        }

        TraceUtil.trace3("Exiting");
        return url;
    }
}
