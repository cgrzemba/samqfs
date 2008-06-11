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

// ident	$Id: FirstTimeConfigView.java,v 1.2 2008/06/11 21:16:19 kilemba Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.media.wizards.AddLibraryImpl;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import javax.servlet.ServletException;

public class FirstTimeConfigView extends RequestHandlingViewBase {

    // child views
    public static final String TEST = "test";
    public static final String ADD_LIBRARY = "addLibraryWizard";
    public static final String ADD_LIBRARY_FORWARD = "addLibraryWizardForward";
    private CCWizardWindowModel addLibraryWizardModel = null;

    private String serverName = null;

    public FirstTimeConfigView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        initializeWizard();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(ADD_LIBRARY, CCWizardWindow.class);
        registerChild(ADD_LIBRARY_FORWARD, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(TEST)) {
            return new CCHref(this, name, null);
        } else if (name.equals(ADD_LIBRARY)) {
            return new CCWizardWindow(this, addLibraryWizardModel, name);
        } else if (name.equals(ADD_LIBRARY_FORWARD)) {
            return new CCHref(this, name, null);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void initializeWizard() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        serverName = parent.getServerName();

        String commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(ADD_LIBRARY_FORWARD).toString();
        addLibraryWizardModel = AddLibraryImpl.createModel(commandChild);
        addLibraryWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            this.serverName);
    }

    public void handleAddLibraryWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleAddLibraryWizardForwardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }
}
