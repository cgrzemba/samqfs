/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: FSArchivePoliciesView.java,v 1.30 2008/03/17 14:43:33 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.archive.wizards.NewPolicyWizardImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import java.util.StringTokenizer;
import javax.servlet.ServletException;

/**
 * Creates the FSArchivePolicies Action Table and provides
 * handlers for the links within the table.
 */
public class FSArchivePoliciesView extends CommonTableContainerView {

    protected FSArchivePoliciesModel model;

    public static final String
        CHILD_ADD_POLICY_BUTTON = "SamQFSWizardNewPolicyButton";
    public static final String CHILD_ADD_CRITERIA_BUTTON = "AddCriteriaButton";
    public static final String CHILD_REORDER_BUTTON = "ReorderButton";
    public static final String CHILD_REMOVE_BUTTON = "RemoveButton";

    // Child View names associated with add policy criteria pop up
    public static final String
        CHILD_ADD_CRITERIA_HIDDEN_FIELD = "AddCriteriaHiddenField";
    public static final String CHILD_ADD_CRITERIA_HREF = "AddCriteriaHref";

    // Child View names associated with reorder pop-up
    public static final String
        CHILD_REORDER_NEWORDER_HIDDEN_FIELD = "ReorderNewOrderHiddenField";
    public static final String
        CHILD_REORDER_MODELSIZE_HIDDEN_FIELD = "ModelSizeHiddenField";

    public static final String CHILD_CANCEL_HREF = "CancelHref";

    public static final String
        CHILD_TILED_VIEW = "FSArchivePoliciesTiledView";

    // handler of new archive policy wizard
    public static String WIZARD_FORWARDTO = "policyForwardToVB";
    public static String CHILD_ARFRWD_TO_CMDCHILD = "policyForwardToVB";

    public static String UNREORDERABLE_CRITERIA = "unreorderableCriteria";

    private CCWizardWindowModel policyWizardWindowModel = null;

    // keeps track of whether the new policy wizard is already up or not
    private boolean newPolicyWizardRunning = false;

    private boolean writeRole = false;

    protected String serverName;
    protected String fsName;

    public FSArchivePoliciesView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        ViewBean vb = getParentViewBean();
        serverName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        fsName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        CHILD_ACTION_TABLE = "FSArchivePoliciesTable";
        model = new FSArchivePoliciesModel(serverName, fsName);
        initializeNewPolicyWizard();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_ADD_CRITERIA_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_REORDER_NEWORDER_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(
            CHILD_REORDER_MODELSIZE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_ADD_CRITERIA_HREF, CCHref.class);
        registerChild(CHILD_CANCEL_HREF, CCHref.class);
        registerChild(CHILD_TILED_VIEW, FSArchivePoliciesTiledView.class);
        registerChild(WIZARD_FORWARDTO, BasicCommandField.class);
        super.registerChildren(model);
        registerChild(UNREORDERABLE_CRITERIA, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(CHILD_ADD_CRITERIA_HIDDEN_FIELD) ||
            name.equals(CHILD_REORDER_NEWORDER_HIDDEN_FIELD) ||
            name.equals(CHILD_REORDER_MODELSIZE_HIDDEN_FIELD) ||
            name.equals(UNREORDERABLE_CRITERIA)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_ADD_CRITERIA_HREF) ||
                   name.equals(CHILD_CANCEL_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            return new FSArchivePoliciesTiledView(this, model, name);
        } else if (name.equals(WIZARD_FORWARDTO)) {
            return new BasicCommandField(this, name);
        } else {
            return super.createChild(model, name, CHILD_TILED_VIEW);
        }
    }

    /**
     * Handler for the New Archive Policy Button.
     */
    public void handleSamQFSWizardNewPolicyButtonRequest(
        RequestInvocationEvent rie) throws ServletException, IOException {
        newPolicyWizardRunning = true;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handlePolicyForwardToVBRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // update the tables to pick up the new policy
        newPolicyWizardRunning = false;
        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Handler for the Remove Policies Button.
     */
    public void handleRemoveButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException  {

        TraceUtil.trace3("Entering");
        String policyName = null;
        int criteriaNumber = -1;
        try {
            int selectedRowIndex = getSelectedRowIndex();
            model.setRowIndex(selectedRowIndex);
            policyName = (String) model.getValue("PolicyNameHiddenField");

            criteriaNumber = Integer.parseInt((String)
                model.getValue("CriteriaNumberHiddenField"));
        } catch (ModelControlException modEx) {
            SamUtil.processException(
                modEx,
                this.getClass(),
                "handleRemoveButtonRequest()",
                "Exception occurred within framework",
                serverName);
            throw modEx;
        } catch (NumberFormatException nfe) {
            TraceUtil.trace2("Got NumberFormatException!" + nfe);
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            FileSystem fs =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }

            ArchivePolicy policy =
                sysModel.getSamQFSSystemArchiveManager().getArchivePolicy(
                    policyName);
            if (policy == null) {
                throw new SamFSException(null, -1001);
            }

            ArchivePolCriteria policyCriteria =
                policy.getArchivePolCriteria(criteriaNumber);
            if (policyCriteria == null) {
                throw new SamFSException(null, -1001);
            }

            LogUtil.info(
                this.getClass(),
                "handleRemoveButtonRequest",
                "Start removing policyCriteria " + policyName + ","
                    + criteriaNumber);

            fs.removePolCriteria(new ArchivePolCriteria[] { policyCriteria });

            LogUtil.info(
                this.getClass(),
                "handleRemoveButtonRequest",
                "Done removing policyCriteria " + policyName + ","
                    + criteriaNumber);

            showAlert("FSArchivePolicies.msg.deletePolicyCriteria",
                policyName + "," + criteriaNumber);
        } catch (SamFSException ex) {

            String processMsg = null;
            String errMsg = null;
            String errCause = null;
            boolean warning = false;

            // catch exceptions where the archiver.cmd has errors
            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
            } else if (ex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to remove policy";
                errMsg = "FSArchivePolicies.error.delete";
                errCause = ex.getMessage();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "handleRemoveButtonRequest()",
                processMsg,
                serverName);

            if (!warning) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errMsg,
                    ex.getSAMerrno(),
                    errCause,
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errMsg,
                    errCause);
            }
        }

        refreshModel();
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler for the Reorder Policies Button.
     */
    public void handleReorderButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * beginDisplay start to display the page
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        model.setRowSelected(false);

        int size = model.getNumRows();
        ((CCHiddenField)
            getChild(CHILD_REORDER_MODELSIZE_HIDDEN_FIELD)).setValue(
                Integer.toString(size));

        // fill in the hidden field used by reorder pop up to populate up/down
        // component
        // NOTE:
        // since the default policy cannot be re-ordered, and it's always
        // the last one in the list, remove it from the list here!!!
        String policyCriteriaString = model.getReorderCriteriaString();
        ((CCHiddenField)
            getChild(CHILD_REORDER_NEWORDER_HIDDEN_FIELD)).setValue(
                policyCriteriaString);

        setDisplayFieldValue(UNREORDERABLE_CRITERIA,
                             model.getNonReorderCriteriaString());

        // Since archiving requires config modifying an FS archiving
        // behavior should require config authorization as well.
        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {

            ((CCButton) getChild(CHILD_ADD_POLICY_BUTTON)).
                                setDisabled(newPolicyWizardRunning);

            ((CCButton) getChild(CHILD_ADD_CRITERIA_BUTTON)).setDisabled(false);

            // disable reorder button if table contains less than 3 records
            // Note:
            // since default policy is always present, reorder button is
            // enabled when there are two or more non-default policies
            if (model.getNumReorderCriteria() > 1) {
                ((CCButton) getChild(CHILD_REORDER_BUTTON)).setDisabled(false);
            } else {
                ((CCButton) getChild(CHILD_REORDER_BUTTON)).setDisabled(true);
            }
            ((CCButton) getChild(CHILD_REMOVE_BUTTON)).setDisabled(false);
        } else {
            ((CCButton) getChild(CHILD_ADD_POLICY_BUTTON)).setDisabled(true);
            ((CCButton) getChild(CHILD_ADD_CRITERIA_BUTTON)).setDisabled(true);
            ((CCButton) getChild(CHILD_REORDER_BUTTON)).setDisabled(true);
            ((CCButton) getChild(CHILD_REMOVE_BUTTON)).setDisabled(true);

            model.setSelectionType(CCActionTableModel.NONE);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * return the selected row index
     */
    private int getSelectedRowIndex() throws ModelControlException {
        TraceUtil.trace3("Entering");
        int index = -1;
        CCActionTable child =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);
        child.restoreStateData();
        model.beforeFirst();
        while (model.next()) {
            if (model.isRowSelected()) {
                index = model.getRowIndex();
            }
        }
        TraceUtil.trace3("Exiting");
        return index;
    }

    private void refreshModel() {

        TraceUtil.trace3("Entering");
        try {
            model.initModelRows();

        } catch (SamFSException smfex) {
            // to refresh the model
            model.clear();
            SamUtil.processException(
                smfex,
                this.getClass(),
                "refreshModel()",
                "Failed to populate archive policy",
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSArchivePolicies.error.populatePolicyCriteria",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
        TraceUtil.trace3("Exiting");
    }

    public void handleAddCriteriaHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String policyCriteriaString =
            (String) getDisplayFieldValue(CHILD_ADD_CRITERIA_HIDDEN_FIELD);
        TraceUtil.trace2("Got new policy criteria: " + policyCriteriaString);

        StringTokenizer tokens = new StringTokenizer(policyCriteriaString, ",");
        int count = tokens.countTokens();
        String[] policyNames = new String[count];
        int[] criteriaNumbers = new int[count];
        for (int i = 0; i < count; i++) {
            String token = tokens.nextToken();
            int index = token.indexOf(':');
            policyNames[i] = token.substring(0, index);
            criteriaNumbers[i] = Integer.parseInt(token.substring(index + 1));
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            FileSystem fs =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }

            ArchivePolCriteria[] policyCriteria = new ArchivePolCriteria[count];
            for (int i = 0; i < count; i++) {
                ArchivePolicy policy =
                    sysModel.getSamQFSSystemArchiveManager().getArchivePolicy(
                        policyNames[i]);
                if (policy == null) {
                    throw new SamFSException(null, -1001);
                }
                policyCriteria[i] = policy.getArchivePolCriteria(
                        criteriaNumbers[i]);

                if (policyCriteria[i] == null) {
                    throw new SamFSException(null, -1001);
                }
            }

            LogUtil.info(
                this.getClass(),
                "handleAddCriteriaHrefRequest",
                "Start adding policy criteria");

            fs.addPolCriteria(policyCriteria);

            LogUtil.info(
                this.getClass(),
                "handleAddCriteriaHrefRequest",
                "Done adding policy criteria");

            showAlert("FSArchivePolicies.msg.addPolicyCriteria", "");
        } catch (SamFSException ex) {
            String processMsg = null;
            String errMsg = null;
            String errCause = null;
            boolean warning = false;

            // catch exceptions where the archiver.cmd has errors
            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
            } else if (ex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to add policy criteria";
                errMsg = "FSArchivePolicies.error.addPolicyCriteria";
                errCause = ex.getMessage();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "handleAddCriteriaHrefRequest()",
                processMsg,
                serverName);

            if (!warning) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errMsg,
                    ex.getSAMerrno(),
                    errCause,
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errMsg,
                    errCause);
            }
        }
        refreshModel();
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    public void handleCancelHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void showAlert(String operation, String key) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(operation, key),
            serverName);
            TraceUtil.trace3("Exiting");
    }

    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");

        // disable selection radio buttons
        ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
         getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

        model.initModelRows();
        TraceUtil.trace3("Exiting");
    }

    private void initializeNewPolicyWizard() {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase view = (CommonViewBeanBase) getParentViewBean();
        StringBuffer cmdChild =
            new StringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("FSArchivePoliciesView.").
                append(WIZARD_FORWARDTO);
        policyWizardWindowModel =
            NewPolicyWizardImpl.createModel(cmdChild.toString());
        model.setModel(
            CHILD_ADD_POLICY_BUTTON, policyWizardWindowModel);
        policyWizardWindowModel.setValue(
            CHILD_ADD_POLICY_BUTTON, "FSArchivePolicies.button.AddPolicy");
        policyWizardWindowModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        TraceUtil.trace3("Exiting");
    }

    private void setWizardState() {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase view = (CommonViewBeanBase) getParentViewBean();

        String temp = (String)view.getPageSessionAttribute
            (NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String modelName = (String) view.getPageSessionAttribute(
             NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String implName = (String) view.getPageSessionAttribute(
             NewPolicyWizardImpl.WIZARDIMPLNAME);
        if (modelName == null) {
            modelName = NewPolicyWizardImpl.WIZARDPAGEMODELNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            view.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDPAGEMODELNAME, modelName);
        }

        if (implName == null) {
            implName = NewPolicyWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            view.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDIMPLNAME, implName);
        }

        policyWizardWindowModel.setValue(
            NewPolicyWizardImpl.WIZARDPAGEMODELNAME, modelName);
        policyWizardWindowModel.setValue(
            CCWizardWindowModel.WIZARD_NAME, implName);

        policyWizardWindowModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        TraceUtil.trace3("Exiting");
    }
}
