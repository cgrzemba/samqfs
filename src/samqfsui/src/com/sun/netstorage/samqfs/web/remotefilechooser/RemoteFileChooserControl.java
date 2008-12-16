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

// ident	$Id: RemoteFileChooserControl.java,v 1.9 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.filechooser.CCFileChooserWindow;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;

public class RemoteFileChooserControl extends CCFileChooserWindow {

    public static final String CHILD_CLEAR = "clearButton";
    public static final String CHILD_PATH_HIDDEN = "pathHidden";

    public RemoteFileChooserControl(
        ContainerView parent, RemoteFileChooserModel model, String name) {

        super(parent, model, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Ctor being called");
    }

    protected void registerChildren() {
        TraceUtil.trace3("register children");
        super.registerChildren();
        registerChild(CHILD_CLEAR, CCButton.class);

        // Used when control is read-only
        registerChild(CHILD_PATH_HIDDEN,  CCHiddenField.class);
    }

    protected View createChild(String name) {
        if (name.equals(CHILD_CLEAR)) {
            CCButton child = new CCButton(this, name,
                                          SamUtil.getResourceString(
                                                "browserWindow.clear"));
            child.setType(CCButton.TYPE_SECONDARY);
            return child;
        } else if (name.equals(CHILD_PATH_HIDDEN)) {
            return new CCHiddenField(this, name, null);
        } else {
            return super.createChild(name);
        }
    }
}
