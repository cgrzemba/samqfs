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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: FirstTimeConfigView.java,v 1.4 2008/12/16 00:10:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.fs.wizards.CreateFSWizardImpl;
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
    public static final String ADD_LIBRARY = "addLibraryWizard";
    public static final String NEW_FS = "newFileSystemWizard";
    public static final String ADD_LIBRARY_FORWARD = "addLibraryWizardForward";
    public static final String NEW_FS_FORWARD = "newFileSystemWizardForward";

    private CCWizardWindowModel addLibraryWizardModel = null;
    private CCWizardWindowModel newFileSystemWizardModel =  null;

    // private String serverName = null;

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
        registerChild(NEW_FS, CCWizardWindow.class);
        registerChild(ADD_LIBRARY_FORWARD, CCHref.class);
        registerChild(NEW_FS_FORWARD, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ADD_LIBRARY)) {
            return new CCWizardWindow(this, addLibraryWizardModel, name);
        } else if (name.equals(NEW_FS)) {
            return new CCWizardWindow(this, newFileSystemWizardModel, name);
        } else if (name.equals(ADD_LIBRARY_FORWARD) ||
            name.equals(NEW_FS_FORWARD)) {
            return new CCHref(this, name, null);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void initializeWizard() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        // add library wizard
        String commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(ADD_LIBRARY_FORWARD).toString();
        addLibraryWizardModel = AddLibraryImpl.createModel(commandChild);
        addLibraryWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        // new file system wizard
        commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(NEW_FS_FORWARD).toString();
        newFileSystemWizardModel = CreateFSWizardImpl.createModel(commandChild);
        newFileSystemWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
    }

    public void handleAddLibraryWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleAddLibraryWizardForwardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleNewFileSystemWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleNewFileSystemWizardForwardRequest(
        RequestInvocationEvent rie) throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }
}
