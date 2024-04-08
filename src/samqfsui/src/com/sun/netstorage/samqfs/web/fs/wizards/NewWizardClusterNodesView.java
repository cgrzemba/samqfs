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

// ident	$Id: NewWizardClusterNodesView.java,v 1.9 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 * Cluster Nodes
 */
public class NewWizardClusterNodesView extends RequestHandlingViewBase
    implements CCWizardPage {
    public static final String PAGE_NAME = "NewWizardFSCLusterNodeView";
    public static final String NODES_LABEL = "clusterNodeLabel";
    public static final String NODES = "clusterNode";
    public static final String ALERT = "Alert";

    public NewWizardClusterNodesView(View parent, Model model) {
        super(parent, PAGE_NAME);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    private void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(NODES_LABEL, CCLabel.class);
        registerChild(NODES, CCSelectableList.class);
        registerChild(ALERT, CCAlertInline.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(NODES_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(NODES)) {
            return  new CCSelectableList(this, name, null);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    /**
     * notification that the pagelet associated with this view has started
     * displaying .
     */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String serverName = (String)wm.getValue(Constants.Wizard.SERVER_NAME);

        try {
            ClusterNodeInfo [] clusterNode =
                    SamUtil.getModel(serverName).getClusterNodes();

            OptionList options = new OptionList();
            for (int i = 0; i < clusterNode.length; i++) {
                String name = clusterNode[i].getName();
                options.add(name, name);
            }
            ((CCSelectableList)getChild(NODES)).setOptions(options);
            ((CCSelectableList)getChild(NODES)).
                setLabelForNoneSelected("common.selectablelist.none.label");
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                this.getClass(),
                "beginDisplay()",
                "Failed to populate cluster information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FSWizard.new.cluster.error.populate",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    // implement CCWizardPage interface
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return "/jsp/fs/NewWizardClusterNodes.jsp";
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
