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

// ident	$Id: ProtoFSTiledView.java,v 1.1 2008/06/11 20:33:02 kilemba Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.model.admin.Configuration;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.util.List;

public class ProtoFSTiledView extends RequestHandlingTiledViewBase {

    // child views
    public static final String PROTO_FS_HREF = "ProtofsHref";
    public static final String PROTO_FS_TEXT = "ProtofsText";

    public ProtoFSTiledView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        setPrimaryModel((DefaultModel)getDefaultModel());
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(PROTO_FS_HREF, CCHref.class);
        registerChild(PROTO_FS_TEXT, CCStaticTextField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(PROTO_FS_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(PROTO_FS_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid name '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        Configuration config =
            ((FirstTimeConfigViewBean)getParentViewBean()).getConfiguration();
        ((DefaultModel)getPrimaryModel()).setSize(config.getProtoFSCount());

        TraceUtil.trace3("Exiting");
    }

    public boolean beginProtofsHrefDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int tileIndex = ((DefaultModel)getPrimaryModel()).getLocation();
        List <String> fs = ((FirstTimeConfigViewBean)getParentViewBean())
            .getConfiguration().getProtoFSName();

        CCHref href = (CCHref)getChild(PROTO_FS_HREF);
        if (fs != null && (fs.size() > tileIndex)) {
            href.setValue(fs.get(tileIndex));

        } else {
            href.setValue(null);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginProtofsTextDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int tileIndex = ((DefaultModel)getPrimaryModel()).getLocation();
        List <String> fs = ((FirstTimeConfigViewBean)getParentViewBean())
            .getConfiguration().getProtoFSName();

        CCStaticTextField text = (CCStaticTextField)getChild(PROTO_FS_TEXT);
        if (fs != null && (fs.size() > tileIndex)) {
            StringBuffer buf = new StringBuffer();
            buf.append("" + (1 + tileIndex))
                .append(". ")
                .append(fs.get(tileIndex));

            text.setValue(buf.toString());
        } else {
            text.setValue(null);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }
}