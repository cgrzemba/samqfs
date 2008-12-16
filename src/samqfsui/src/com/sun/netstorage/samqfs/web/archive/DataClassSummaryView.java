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

// ident	$Id: DataClassSummaryView.java,v 1.20 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.archive.wizards.NewDataClassWizardImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import javax.servlet.ServletException;

public class DataClassSummaryView extends CommonTableContainerView {

    // Tiled View
    public static String TILED_VIEW = "DataClassSummaryTiledView";

    // Wizard stuff
    public static String WIZARD_FORWARDTO = "policyForwardToVB";
    private CCWizardWindowModel dataClassWizardWindowModel = null;
    // keeps track of whether the new policy wizard is already up or not
    private boolean newWizardRunning = false;

    // Operation drop down
    public static final String CHILD_ACTIONMENU_HREF = "ActionMenuHref";
    public static final String CHILD_ACTIONMENU = "ActionMenu";

    // Page Action Table Model
    private CCActionTableModel tableModel = null;


    public DataClassSummaryView(View parent, String name) {
        super(parent, name);

        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "DataClassSummaryTable";
        createTableModel();
        initializeNewDataClassWizard();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(WIZARD_FORWARDTO, BasicCommandField.class);
        registerChild(TILED_VIEW, DataClassSummaryTiledView.class);
        registerChild(CHILD_ACTIONMENU_HREF, CCHref.class);
        super.registerChildren(tableModel);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(WIZARD_FORWARDTO)) {
            return new BasicCommandField(this, name);
        } else if (name.equals(TILED_VIEW)) {
            return new DataClassSummaryTiledView(this, tableModel, name);
        } else if (name.equals(CHILD_ACTIONMENU_HREF)) {
            return new CCHref(this, name, null);
        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    private void initializeNewDataClassWizard() {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();
        NonSyncStringBuffer cmdChild =
            new NonSyncStringBuffer().
                append(parent.getQualifiedName()).
                append(".").
                append("DataClassSummaryView.").
                append(WIZARD_FORWARDTO);
        dataClassWizardWindowModel =
            NewDataClassWizardImpl.createModel(cmdChild.toString());
        tableModel.setModel(
            "SamQFSWizardNewDataClass", dataClassWizardWindowModel);
        dataClassWizardWindowModel.setValue(
            "SamQFSWizardNewDataClass",
            "archiving.dataclass.summary.button.add");
        dataClassWizardWindowModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            parent.getServerName());

        TraceUtil.trace3("Exiting");
    }

    private void setWizardState() {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        String temp = (String) parent.getPageSessionAttribute
            (NewDataClassWizardImpl.PAGEMODEL_NAME);
        String modelName = (String) parent.getPageSessionAttribute(
             NewDataClassWizardImpl.PAGEMODEL_NAME);
        String implName =  (String) parent.getPageSessionAttribute(
             NewDataClassWizardImpl.IMPL_NAME);
        if (modelName == null) {
            modelName =
                NewDataClassWizardImpl.PAGEMODEL_NAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            parent.setPageSessionAttribute(
                NewDataClassWizardImpl.PAGEMODEL_NAME, modelName);
        }

        dataClassWizardWindowModel.setValue(
            NewDataClassWizardImpl.PAGEMODEL_NAME, modelName);
        if (implName == null) {
            implName = NewDataClassWizardImpl.IMPL_NAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            parent.setPageSessionAttribute(
                NewDataClassWizardImpl.IMPL_NAME, implName);
        }

        dataClassWizardWindowModel.setValue(
            CCWizardWindowModel.WIZARD_NAME, implName);

        // set the server name
        String serverName = parent.getServerName();
        dataClassWizardWindowModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        TraceUtil.trace3("Exiting");
    }

    public void createTableModel() {
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/DataClassSummaryTable.xml");
    }

    /**
     * set action labels and column headers for General policies table
     */
    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");

        // init general table column headers
        tableModel.setActionValue(
            "Priority", "archiving.dataclass.summary.table.header.priority");
        tableModel.setActionValue(
            "ClassName", "archiving.dataclass.summary.table.header.class");
        tableModel.setActionValue(
            "PolicyName", "archiving.dataclass.summary.table.header.policy");
        tableModel.setActionValue(
            "Copy", "archiving.dataclass.summary.table.header.copy");
        tableModel.setActionValue(
            "FileSystems", "archiving.dataclass.summary.table.header.fs");
        tableModel.setActionValue("Description",
            "archiving.dataclass.summary.table.header.description");

        // set delete button label
        tableModel.setActionValue("DeleteDataClass", "archiving.delete");
        TraceUtil.trace3("Exiting");
    }

    /**
     * populate the table rows - both general and other policies tables
     * /files/arc-redesign/src/samqfsui
     */
    public void populateTableModel() {
        TraceUtil.trace3("Entering");
        SamFSException exception = null;
        boolean hasException = false;

        CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();
        String serverName = parent.getServerName();

        // disable selection radio buttons
        CCActionTable theTable = (CCActionTable)getChild(CHILD_ACTION_TABLE);
        ((CCRadioButton) theTable.
            getChild(CCActionTable.CHILD_ROW_SELECTION_RADIOBUTTON)).
            setTitle("");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolCriteria [] allCriterias =
                sysModel.getSamQFSSystemArchiveManager().getAllDataClasses();

            tableModel.clear();

            for (int i = 0; i < allCriterias.length; i++) {
                // Now populate rows
                if (i > 0) {
                    tableModel.appendRow();
                }

                ArchivePolicy thisPolicy = allCriterias[i].getArchivePolicy();
                ArchivePolCriteriaProp prop =
                    allCriterias[i].getArchivePolCriteriaProperties();

                tableModel.setValue(
                    "PriorityText", new Integer(prop.getPriority()));

                // display a non-hrefed policy name when dealing with a
                // no_archive policy
                if (thisPolicy.getPolicyType() ==
                    ArSet.AR_SET_TYPE_NO_ARCHIVE) {
                    tableModel.setValue(
                        "NoHrefPolicyName",
                        thisPolicy.getPolicyName());
                }

                tableModel.setValue("ClassNameText", prop.getClassName());
                tableModel.setValue(
                    "PolicyNameText",
                    thisPolicy.getPolicyName());
                tableModel.setValue(
                    "CopyText",
                    PolicyUtil.getPolicyCopyString(allCriterias[i]));

                // Hidden field
                // Policy_Name###Criteria_Index###Modify_Policy###Delete
                NonSyncStringBuffer buf =
                    new NonSyncStringBuffer(
                        thisPolicy.getPolicyName()).append("###").append(
                        allCriterias[i].getIndex()).append("###").append(
                            !Constants.Archive.NOARCHIVE_POLICY_NAME.equals(
                            thisPolicy.getPolicyName())).append("###").
                            append(
                                thisPolicy.getPolicyType() !=
                                ArSet.AR_SET_TYPE_DEFAULT).
                        append("###").append(prop.getClassName());
                tableModel.setValue("PolicyInfo", buf.toString());
                tableModel.setValue("ClassNameHref", buf.toString());
                tableModel.setValue("PolicyNameHref", buf.toString());

                tableModel.setValue("DescriptionText",
                                    prop.getDescription());
                tableModel.setRowSelected(false);
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to retrieve policy information",
                                     serverName);

            SamUtil.setWarningAlert(parent,
                                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error.summary",
                                    sfw.getMessage());
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to retrieve policy information",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  CommonViewBeanBase.CHILD_COMMON_ALERT,
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
                                  CommonViewBeanBase.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        initializeTableHeaders();

        setWizardState();

        // always disable general delete button
        CCButton del = (CCButton) getChild("DeleteDataClass");
        del.setDisabled(true);

        boolean hasPermission = SecurityManagerFactory.
            getSecurityManager().hasAuthorization(Authorization.CONFIG);

        // disble table selections
        ((CCHiddenField) getParentViewBean().getChild(
            DataClassSummaryViewBean.HAS_PERMISSION)).setValue(
            Boolean.toString(hasPermission));

        CCWizardWindow button =
            (CCWizardWindow) getChild("SamQFSWizardNewDataClass");

        // set new wizard button
        if (newWizardRunning || !hasPermission) {
            button.setDisabled(true);
        } else {
            // explicitly enable the button to reverse disabling
            // from 'restoreStateData()
            button.setDisabled(false);
        }

        TraceUtil.trace3("Exiting");
    }

    public void handlePolicyForwardToVBRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // update the tables to pick up the new policy
        newWizardRunning = false;
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleSamQFSWizardNewDataClassRequest(
        RequestInvocationEvent rie) throws ServletException, IOException {
        newWizardRunning = true;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * delete the selected data class
     */
    public void handleDeleteDataClassRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        CCHiddenField field =
            (CCHiddenField) getParentViewBean().getChild(
            DataClassSummaryViewBean.DATA_CLASS_NAME);

        String [] infoArray = ((String) field.getValue()).split("###");
        String className = infoArray[2];
        String serverName =
            (String)getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        try {
            // retrieve the system model
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            LogUtil.info(
                this.getClass(),
                "handleDeleteDataClassRequest",
                "Start deleting class ".concat(className));

            // delete class
            sysModel.getSamQFSSystemArchiveManager().deleteClass(className);

            LogUtil.info(
                this.getClass(),
                "handleDeleteDataClassRequest",
                "Done deleting class ".concat(className));

            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "archiving.dataclass.delete.success",
                    className),
                serverName);
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(
                sfw,
                this.getClass(),
                "handleDeleteDataClassRequest",
                "Warning while deleting data class" + className,
                serverName);

            SamUtil.setWarningAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error",
                "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(
                smme,
                this.getClass(),
                "handleDeleteDataClassRequest",
                "Unable to delete data class" + className,
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error",
                smme.getSAMerrno(),
                "ArchiveConfig.error.detail",
                serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                this.getClass(),
                "handleDeleteDataClassRequest",
                "unable to delete data class" + className,
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "archiving.dataclass.delete.failure",
                    className),
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }


        // finally refresh the page
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request to handle the operation drop-down menu
     * @param RequestInvocationEvent event
     */
    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        int value = 0;
        try {
            value = Integer.parseInt(
                (String) getDisplayFieldValue(CHILD_ACTIONMENU));
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("NumEx caught while parsing drop down selection!");
        }

        CCHiddenField field =
            (CCHiddenField) getParentViewBean().getChild(
            DataClassSummaryViewBean.DATA_CLASS_NAME);

        String [] infoArray = ((String) field.getValue()).split("###");
        String policyName = infoArray[0];
        String index = infoArray[1];
        ViewBean targetVB = null;

        switch (value) {
            // Modify Data Class
            case 1:
                targetVB = getViewBean(DataClassDetailsViewBean.class);
                getParentViewBean().setPageSessionAttribute(
                    Constants.Archive.POLICY_NAME, policyName);
                getParentViewBean().setPageSessionAttribute(
                    Constants.Archive.CRITERIA_NUMBER, new Integer(index));
                BreadCrumbUtil.breadCrumbPathForward(
                    getParentViewBean(),
                    PageInfo.getPageInfo().getPageNumber(
                        getParentViewBean().getName()));
                ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetVB);
                break;

            // Modify Policy
            case 2:
                targetVB = getViewBean(ISPolicyDetailsViewBean.class);
                getParentViewBean().setPageSessionAttribute(
                    Constants.Archive.POLICY_NAME, policyName);
                BreadCrumbUtil.breadCrumbPathForward(
                    getParentViewBean(),
                    PageInfo.getPageInfo().getPageNumber(
                        getParentViewBean().getName()));
                ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetVB);
                break;

            // Change Priority
            case 3:
                break;

            // Associate Class with policy
            case 4:
                break;

            // Rename Data Class
            case 5:
                break;

        }

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild(CHILD_ACTIONMENU)).setValue("0");

        if (value != 1 && value != 2) {
            getParentViewBean().forwardTo(getRequestContext());
        }

        TraceUtil.trace3("Existing");
    }
}
