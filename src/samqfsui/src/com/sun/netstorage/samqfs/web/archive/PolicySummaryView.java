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

// ident	$Id: PolicySummaryView.java,v 1.25 2008/07/23 21:25:28 kilemba Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.archive.wizards.NewPolicyWizardImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import javax.servlet.ServletException;

public class PolicySummaryView extends CommonTableContainerView {
    public static String TILED_VIEW = "PolicySummaryTiledView";

    public static String WIZARD_FORWARDTO = "policyForwardToVB";
    public static String WARNING = "SAMFSWarning";

    private CCWizardWindowModel policyWizardWindowModel = null;

    private CCActionTableModel tableModel = null;

    // keeps track of whether the new policy wizard is already up or not
    private boolean newPolicyWizardRunning = false;
    private boolean hasArchivingFS = false;

    public PolicySummaryView(View parent, String name) {
        super(parent, name);

        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "PolicySummaryTable";

        createTableModel();
        initializeNewPolicyWizard();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(WIZARD_FORWARDTO, BasicCommandField.class);
        registerChild(TILED_VIEW, PolicySummaryTiledView.class);
        super.registerChildren(tableModel);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(WIZARD_FORWARDTO)) {
            return new BasicCommandField(this, name);
        } else if (name.equals(TILED_VIEW)) {
            return new PolicySummaryTiledView(this, tableModel,  name);
        } else if (name.equals(WARNING)) {
            return new CCStaticTextField(this, name, null);
        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    private void initializeNewPolicyWizard() {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        NonSyncStringBuffer cmdChild =
            new NonSyncStringBuffer().
                append(parent.getQualifiedName()).
                append(".").
                append("PolicySummaryView.").
                append(WIZARD_FORWARDTO);
        policyWizardWindowModel =
            NewPolicyWizardImpl.createModel(cmdChild.toString());
        tableModel.setModel(
            "SamQFSWizardNewPolicy", policyWizardWindowModel);
        policyWizardWindowModel.setValue(
            "SamQFSWizardNewPolicy", "archiving.new");
        policyWizardWindowModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            parent.getServerName());

        TraceUtil.trace3("Exiting");
    }

    private void setWizardState() {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        String modelName = (String)parent.getPageSessionAttribute(
             NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String implName =  (String)parent.getPageSessionAttribute(
             NewPolicyWizardImpl.WIZARDIMPLNAME);
        if (modelName == null) {
            modelName =
                NewPolicyWizardImpl.WIZARDPAGEMODELNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            parent.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDPAGEMODELNAME, modelName);
        }

        policyWizardWindowModel.setValue(
            NewPolicyWizardImpl.WIZARDPAGEMODELNAME, modelName);
        if (implName == null) {
            implName = NewPolicyWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            parent.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDIMPLNAME, implName);
        }

        policyWizardWindowModel.setValue(
            CCWizardWindowModel.WIZARD_NAME, implName);

        // set the server name
        String serverName = parent.getServerName();
        policyWizardWindowModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        TraceUtil.trace3("Exiting");
    }

    public void createTableModel() {
        String xmlFile = "/jsp/archive/PolicySummaryTable.xml";
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(), xmlFile);
    }

    /**
     * set action labels and column headers for General policies table
     */
    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");

        // init general table column headers
        tableModel.setActionValue("PolicyName", "archiving.policy.name");

        tableModel.setActionValue("PolicyType", "archiving.policy.type");
        tableModel.setActionValue("CopyCount",
                                  "archiving.policy.numberofcopies");
        tableModel.setActionValue("FileSystems",
                                  "archiving.policy.filesystems");
        // set delete button label
        tableModel.setActionValue("DeletePolicy", "archiving.delete");

        // set configure allsets button label
        tableModel.setActionValue("ConfigureAllSets",
                                  "archiving.allsets.button");

        TraceUtil.trace3("Exiting");
    }

    /**
     * populate the table rows - both general and other policies tables
     * /files/arc-redesign/src/samqfsui
     */
    public void populateTableModel() {
        TraceUtil.trace3("Entering");
        NonSyncStringBuffer types = new NonSyncStringBuffer();
        NonSyncStringBuffer names = new NonSyncStringBuffer();

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        // disable selection radio buttons
        ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
         getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy [] policy = sysModel.
                getSamQFSSystemArchiveManager().getAllArchivePolicies();

            tableModel.clear();
            int counter = 0;
            for (int i = 0; i < policy.length; i++) {
                // Skip allsets
                if (ArchivePolicy.POLICY_NAME_ALLSETS.
                        equals(policy[i].getPolicyName())) {
                    continue;
                } else {
                    counter++;
                }

                if (counter > 0)
                    tableModel.appendRow();

                tableModel.setValue("PolicyNameHref",
                                    policy[i].getPolicyName());
                tableModel.setValue("PolicyNameText",
                                    policy[i].getPolicyName());
                tableModel.setValue("PolicyTypeValue",
                                   Integer.toString(policy[i].getPolicyType()));
                tableModel.setValue("PolicyTypeText",
                                    PolicyUtil.getPolicyTypeString(policy[i]));
                tableModel.setValue("CopyCountText",
                           new Integer(policy[i].getArchiveCopies().length));
                tableModel.setValue("FileSystemsText",
                                    PolicyUtil.getPolicyFSString(policy[i]));

                names.append(policy[i].getPolicyName()).append(";");
                types.append(policy[i].getPolicyType()).append(";");

                // remove any prior selections that may still be lingering
                // around
                tableModel.setRowSelected(false);
            }

            this.hasArchivingFS = sysModel.hasArchivingFileSystem();
            System.out.println("has archiving fs = " + this.hasArchivingFS);
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to retrieve policy information",
                                     serverName);

            SamUtil.setWarningAlert(parent,
                                    PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error.summary",
                                    sfw.getMessage());
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to retrieve policy information",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to retrieve policy information",
                                     serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // set hidden fields
        CCHiddenField child = (CCHiddenField)
            parent.getChild(PolicySummaryViewBean.POLICY_TYPES);
        child.setValue(types.toString());

        // general policy names
        child = (CCHiddenField)
            parent.getChild(PolicySummaryViewBean.POLICY_NAMES);
        child.setValue(names.toString());

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        initializeTableHeaders();

        setWizardState();

        // always disable general delete button
        CCButton del = (CCButton)getChild("DeletePolicy");
        del.setDisabled(true);

        boolean hasPermission = SecurityManagerFactory.
            getSecurityManager().hasAuthorization(Authorization.CONFIG);

        // disble table selections
        if (!hasPermission) {
            tableModel.setSelectionType(CCActionTableModel.NONE);

        }

        CCWizardWindow button =
            (CCWizardWindow)getChild("SamQFSWizardNewPolicy");

        // set new wizard button
        if (newPolicyWizardRunning || !hasPermission || !this.hasArchivingFS) {
            button.setDisabled(true);
        } else {
            // explicitly enable the button to reverse disabling
            // from 'restoreStateData()
            button.setDisabled(false);
        }

        // if no archiving file systems found, show a message stating so
        if (!this.hasArchivingFS) {
            ((CCStaticTextField)getChild(WARNING)).setValue(
              SamUtil.getResourceString("archiving.summary.nosamfs.warning"));
        }

        TraceUtil.trace3("Exiting");
    }

    public void handlePolicyForwardToVBRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // update the tables to pick up the new policy
        newPolicyWizardRunning = false;
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleSamQFSWizardNewPolicyRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        newPolicyWizardRunning = true;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * delete the selected policy
     */
    public void handleDeletePolicyRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        CCHiddenField field = (CCHiddenField)
            getParentViewBean().getChild(PolicySummaryViewBean.POLICY_NAME);
        String policyName = (String)field.getValue();
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        try {
            // retrieve the system model
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            // delete the policy
            sysModel.getSamQFSSystemArchiveManager().
                deleteArchivePolicy(policyName);
            // set success alert
            SamUtil.setInfoAlert(parent,
                                 PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                 "success.summary",
                                 SamUtil.getResourceString(
                                     "archiving.policy.delete.success",
                                     policyName),
                                 serverName);
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "handleDeletePolicyRequest",
                                     "policy deleted",
                                     serverName);

            SamUtil.setWarningAlert(parent,
                                    PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     this.getClass(),
                                     "handleDeletePolicyRequest",
                                     "Unable to delete policy",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleDeletePolicyRequest",
                                     "unable to delete policy",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  PolicySummaryViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // finally refresh the page
        parent.forwardTo(getRequestContext());
    }

    public void handleConfigureAllSetsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

         CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();

         parent.setPageSessionAttribute(
             Constants.Archive.POLICY_NAME,
             ArchivePolicy.POLICY_NAME_ALLSETS);
         parent.setPageSessionAttribute(
             Constants.Archive.POLICY_TYPE,
             new Integer((int) ArSet.AR_SET_TYPE_ALLSETS_PSEUDO));

         // breadcrumbing
         BreadCrumbUtil.breadCrumbPathForward(parent,
            PageInfo.getPageInfo().getPageNumber(parent.getName()));

         // finally forward to the viewbean
         parent.forwardTo((CommonViewBeanBase)
             getViewBean(PolicyDetailsViewBean.class));
    }
}

