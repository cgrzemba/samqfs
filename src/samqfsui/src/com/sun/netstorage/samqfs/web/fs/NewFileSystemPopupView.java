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

// ident	$Id: NewFileSystemPopupView.java,v 1.1 2008/06/25 23:23:25 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.fs.wizards.CreateFSWizardImpl;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import javax.servlet.ServletException;

public class NewFileSystemPopupView extends RequestHandlingViewBase {
    // child views
    public static final String NEW_FS = "newFileSystemWizard";
    public static final String NEW_FS_FORWARD = "newFileSystemWizardForwardTo";

    // model
    private CCWizardWindowModel newFSModel = null;

    public NewFileSystemPopupView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        initializeWizard();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(NEW_FS, CCWizardWindow.class);
        registerChild(NEW_FS_FORWARD, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(NEW_FS)) {
            return new CCWizardWindow(this, newFSModel, name);
        } else if (name.equals(NEW_FS_FORWARD)) {
            return new CCHref(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private void initializeWizard() {
        CommonSecondaryViewBeanBase parent =
            (CommonSecondaryViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        String commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(NEW_FS_FORWARD).toString();
        newFSModel = CreateFSWizardImpl.createModel(commandChild);
        newFSModel.setValue(Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
                            serverName);
        System.out.println("server name = " + serverName);
    }

    // handlers for the wizard related buttons
    public void handleNewFileSystemWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleNewFileSystemWizardForwardToRequest(
        RequestInvocationEvent rie) throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }
