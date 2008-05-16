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

// ident	$Id: CommonTasksWizardsView.java,v 1.4 2008/05/16 18:39:06 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.archive.DataClassSummaryViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean;
import com.sun.netstorage.samqfs.web.archive.wizards.NewDataClassWizardImpl;
import com.sun.netstorage.samqfs.web.archive.wizards.NewPolicyWizardImpl;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.fs.wizards.CreateFSWizardImpl;
import com.sun.netstorage.samqfs.web.media.LibrarySummaryViewBean;
import com.sun.netstorage.samqfs.web.media.wizards.AddLibraryImpl;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import javax.servlet.ServletException;

public class CommonTasksWizardsView extends RequestHandlingViewBase {
    // wizard buttons
    public static final String NEW_POLICY = "newPolicyWizard";
    public static final String NEW_DATACLASS = "newDataClassWizard";
    public static final String NEW_FILESYSTEM = "newFileSystemWizard";
    public static final String ADD_LIBRARY = "addLibraryWizard";

    // forward to's
    // these href's handle the exit of the wizard
    public static final String NEW_POLICY_FRWD = "newPolicyWizardForward";
    public static final String NEW_DATACLASS_FRWD =
        "newDataClassWizardForward";
    public static final String NEW_FILESYSTEM_FRWD =
        "newFileSystemWizardForward";
    public static final String ADD_LIBRARY_FRWD = "addLibraryWizardForward";

    // wizard models
    CCWizardWindowModel newPolicyWizardModel = null;
    CCWizardWindowModel newDataClassWizardModel = null;
    CCWizardWindowModel newFileSystemWizardModel = null;
    CCWizardWindowModel addLibraryWizardModel = null;

    String serverName = null;

    public CommonTasksWizardsView(View parent, String name) {
        super(parent, name);

        initializeWizards();
        registerChildren();
    }

    public void registerChildren() {
        registerChild(NEW_POLICY, CCWizardWindow.class);
        registerChild(NEW_DATACLASS, CCWizardWindow.class);
        registerChild(NEW_FILESYSTEM, CCWizardWindow.class);
        registerChild(ADD_LIBRARY, CCWizardWindow.class);
        registerChild(NEW_POLICY_FRWD, CCHref.class);
        registerChild(NEW_DATACLASS_FRWD, CCHref.class);
        registerChild(NEW_FILESYSTEM_FRWD, CCHref.class);
        registerChild(ADD_LIBRARY_FRWD, CCHref.class);
    }

    public View createChild(String name) {
        if (name.equals(NEW_DATACLASS)) {
            return new CCWizardWindow(this, newDataClassWizardModel, name);
        } else if (name.equals(NEW_POLICY)) {
            return new CCWizardWindow(this, newPolicyWizardModel, name);
        } else if (name.equals(NEW_FILESYSTEM)) {
            return new CCWizardWindow(this, newFileSystemWizardModel, name);
        } else if (name.equals(ADD_LIBRARY)) {
            return new CCWizardWindow(this, addLibraryWizardModel, name);
        } if (name.indexOf("WizardForward") != -1) {
            return new CCHref(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private void initializeWizards() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        this.serverName = parent.getServerName();

        String commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(NEW_POLICY_FRWD).toString();
        newPolicyWizardModel = NewPolicyWizardImpl.createModel(commandChild);
        newPolicyWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            this.serverName);

        // new dataclass
        commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(NEW_DATACLASS_FRWD).toString();
        newDataClassWizardModel =
            NewDataClassWizardImpl.createModel(commandChild);
        newDataClassWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            this.serverName);

        // new file system
        commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(NEW_FILESYSTEM_FRWD).toString();
        newFileSystemWizardModel =
            CreateFSWizardImpl.createModel(commandChild);
        newFileSystemWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            this.serverName);

        // add library
        commandChild = new StringBuffer().append(getQualifiedName())
            .append(".").append(ADD_LIBRARY_FRWD).toString();
        addLibraryWizardModel = AddLibraryImpl.createModel(commandChild);
        addLibraryWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            this.serverName);
    }

    private void setWizardState() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        // new archive policy
        String wizardPageName = (String) parent
            .getPageSessionAttribute(NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String modelName = (String) parent
            .getPageSessionAttribute(NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String implName = (String) parent
            .getPageSessionAttribute(NewPolicyWizardImpl.WIZARDIMPLNAME);
        if (modelName == null) {
            modelName = NewPolicyWizardImpl.WIZARDPAGEMODELNAME_PREFIX +
                "_"  + HtmlUtil.getUniqueValue();
            parent.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDPAGEMODELNAME,
                                    modelName);
        }

        newPolicyWizardModel.setValue(NewPolicyWizardImpl.WIZARDIMPLNAME_PREFIX,
                                      modelName);
        if (implName == null) {
            implName = NewPolicyWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            parent.setPageSessionAttribute(NewPolicyWizardImpl.WIZARDIMPLNAME,
                                    implName);
        }

        newPolicyWizardModel.setValue(CCWizardWindowModel.WIZARD_NAME,
                                      implName);
        newPolicyWizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            this.serverName);

        // new dataclass

        // new file system

        // add library
    }

    // wizards button handlers - back-up just in case the javascript goofs
    public void handleNewPolicyWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleNewDataClassWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleNewFileSystemWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleAddLibraryWizardRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    // wizard forward to's
    public void handleNewPolicyWizardForwardRequest(RequestInvocationEvent ire)
        throws ServletException, IOException {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicySummaryViewBean.class);

        // forward
        parent.forwardTo(target);
    }

    public void handleNewDataClassWizardForwardRequest(
        RequestInvocationEvent rie)
        throws ServletException, IOException {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(DataClassSummaryViewBean.class);

        // forward
        parent.forwardTo(target);
    }

    public void handleNewFileSystemWizardForwardRequest(
        RequestInvocationEvent rie)
        throws ServletException, IOException {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSSummaryViewBean.class);

        // forward
        parent.forwardTo(target);
    }

    public void handleAddLibraryWizardForwardRequest(
        RequestInvocationEvent rie)
        throws ServletException, IOException {
         CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(LibrarySummaryViewBean.class);

        // forward
        parent.forwardTo(target);
    }
}
