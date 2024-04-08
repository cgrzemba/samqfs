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

// ident	$Id: FileMatchCriteriaView.java,v 1.23 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
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
import com.sun.netstorage.samqfs.web.archive.wizards.NewCriteriaWizardImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.ui.taglib.WizardWindowTag;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * FileMatchCriteriaView -
 *
 * this view by the
 */
public class FileMatchCriteriaView extends CommonTableContainerView
    implements CCPagelet {

    // criteria wizard handler
    public static final String ADD_CRITERIA_WIZARD_FORWARDTO =
        "AddCriteriaForwardToVB";

    // table buttons
    public static final String ADD_CRITERIA = "SamQFSWizardAddCriteria";
    public static final String REMOVE_CRITERIA = "RemoveCriteria";


    // children
    public static final String TILED_VIEW = "FileMatchCriteriaTiledView";

    //
    public static final String TEST = "test";

    // the table model
    private CCActionTableModel tableModel = null;

    // criteria wizard model
    private CCWizardWindowModel addCriteriaWWModel = null;

    // wizard state tracker
    private boolean addCriteriaWizardRunning = false;

    /** create an instance of FileMatchCriteriaView */
    FileMatchCriteriaView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "FileMatchCriteriaTable";

        createTableModel();

        initializeCriteriaWizardButton();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * registerChildren
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(ADD_CRITERIA_WIZARD_FORWARDTO, BasicCommandField.class);
        registerChild(TILED_VIEW, FileMatchCriteriaTiledView.class);
        super.registerChildren(tableModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * createChild
     */
    public View createChild(String name) {
        if (name.equals(ADD_CRITERIA_WIZARD_FORWARDTO)) {
            return new BasicCommandField(this, name);
        } else if (name.equals(TILED_VIEW)) {
            return new FileMatchCriteriaTiledView(this, tableModel, name);
        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    private void createTableModel() {
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/FileMatchCriteriaTable.xml");
    }

    /** initlialize the new criteria wizard button */
    public void initializeCriteriaWizardButton() {
        // retrieve the policy name so we can set it on the model
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)parent.getPageSessionAttribute(
            Constants.Archive.POLICY_NAME);

        // initialize the add criteria wizard button
        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        buf.append(parent.getQualifiedName())
           .append(".")
           .append(getName())
           .append(".")
           .append(ADD_CRITERIA_WIZARD_FORWARDTO);
        addCriteriaWWModel = NewCriteriaWizardImpl.createModel(buf.toString());
        addCriteriaWWModel.setValue(WizardWindowTag.CLIENTSIDE_PARAM_JSFUNCTION,
            "getAddPolicyCriteriaWizardParams");

        tableModel.setModel("SamQFSWizardAddCriteria", addCriteriaWWModel);
        addCriteriaWWModel.setValue("SamQFSWizardAddCriteria", "archiving.add");
        addCriteriaWWModel.setValue(
            Constants.Archive.POLICY_NAME, policyName);
        addCriteriaWWModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
    }

    public void handleSamQFSWizardAddCriteriaRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        addCriteriaWizardRunning = true;
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleAddCriteriaForwardToVBRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        addCriteriaWizardRunning = false;

        getParentViewBean().forwardTo(getRequestContext());
    }


    /**
     * Edit criteria
     */
    public void handleEditCriteriaRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        String cns = source.getDisplayFieldStringValue(
            PolicyDetailsViewBean.SELECTED_CRITERIA);

        Integer criteriaNumber = new Integer(cns);
        source.setPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER,
                                       criteriaNumber);


        ViewBean target = getViewBean(CriteriaDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
    }


    // remove the selected criteria
    public void handleRemoveCriteriaRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        PolicyDetailsViewBean parent =
            (PolicyDetailsViewBean)getParentViewBean();

        String cnString =
            parent.getDisplayFieldStringValue(parent.SELECTED_CRITERIA);

        // retrieve the server name for exceptions
        String serverName = parent.getServerName();
        try {
            int criteriaNumber = Integer.parseInt(cnString);

            // retrieve the policy
            String policyName = (String)
                parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
            ArchivePolicy thePolicy = SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            thePolicy.deleteArchivePolCriteria(criteriaNumber);

            // refresh the policy list to sync criteria indeces
            SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getAllArchivePolicies();
            // set delete confirmation alert
            String temp = SamUtil.getResourceString(
                "archiving.criterianumber", cnString);
            SamUtil.setInfoAlert(parent,
                                 parent.CHILD_COMMON_ALERT,
                                 "success.summary",
                                 SamUtil.getResourceString(
                                 "archiving.criteria.delete.success", temp),
                                 serverName);

        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "handleRemoveCriteriaRequest",
                                     "Unable to remove policy criteria",
                                     serverName);

            SamUtil.setWarningAlert(getParentViewBean(),
                                    PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "handleRemoveCriteriaRequest",
                                     "Unable to remove policy criteria",
                                     serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.detail",
                                  sme.getSAMerrno(),
                                  sme.getMessage(),
                                  serverName);
        } catch (SamFSException sfe) {
            // process exception
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleRemoveCriteriaRequest",
                                     "Unable to remove policy criteria",
                                     serverName);

            // update confirmation alert
            String temp = SamUtil.getResourceString(
                "archiving.criterianumber", cnString);
            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                  SamUtil.getResourceString(
                                  "archiving.criteria.delete.failure", temp),
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // forward to self
        parent.forwardTo(getRequestContext());
    }

    /** initialize table header and radio button */
    protected void initializeTableHeaders() {
        // set the column headers
        tableModel.setActionValue("StartingDirectory",
                                  "archiving.startingdirectory");
        tableModel.setActionValue("NamePattern",
                                  "archiving.namepattern");
        tableModel.setActionValue("Owner", "archiving.owner");
        tableModel.setActionValue("Group", "archiving.group");
        tableModel.setActionValue("MinimumSize",
                                  "archiving.minimumsize");
        tableModel.setActionValue("MaximumSize",
                                  "archiving.maximumsize");
        tableModel.setActionValue("AccessAge",
                                  "archiving.accessage");
        tableModel.setActionValue("ArchiveAge",
                                  "archiving.archiveage");
        tableModel.setActionValue("MediaTypeColumn", "archiving.media.type");

        // button names
        tableModel.setActionValue("DeleteCriteria", "archiving.delete");
        tableModel.setActionValue("RemoveCriteria", "archiving.remove");
        tableModel.setActionValue("EditCriteria", "archiving.edit");
    }

    /** populate the criteria table model */
    public void populateTableModel() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String policyName = (String)parent.getPageSessionAttribute(
            Constants.Archive.POLICY_NAME);
        String serverName = parent.getServerName();
        NonSyncStringBuffer criteriaNumbers = new NonSyncStringBuffer();

        // disable tool tips
        ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
         getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            ArchivePolCriteria [] criteria = thePolicy.getArchivePolCriteria();

            // clear the model first
            tableModel.clear();

            // populate the table model
            for (int i = 0; i < criteria.length; i++) {
                if (i > 0) {
                    tableModel.appendRow();
                    criteriaNumbers.append(",");
                }

                ArchivePolCriteriaProp prop =
                    criteria[i].getArchivePolCriteriaProperties();

                Integer criteriaNumber = new Integer(criteria[i].getIndex());
                tableModel.setValue("CriteriaNumber", criteriaNumber);
                tableModel.setValue("StartingDirectoryText",
                                    prop.getStartingDir());
                tableModel.setValue("NamePatternText", prop.getNamePattern());

                tableModel.setValue("OwnerText", prop.getOwner());
                tableModel.setValue("GroupText", prop.getGroup());

                String minSize =   PolicyUtil.getSizeString(prop.getMinSize(),
                                                       prop.getMinSizeUnit());
                tableModel.setValue("MinimumSizeText", minSize);


                String maxSize = PolicyUtil.getSizeString(prop.getMaxSize(),
                                                     prop.getMaxSizeUnit());
                tableModel.setValue("MaximumSizeText", maxSize);


                String accessAge = "";
                if (prop.getAccessAge() > 0) {
                    accessAge = Long.toString(prop.getAccessAge())
                        .concat(" ").concat(
                    SamUtil.getTimeUnitL10NString(prop.getAccessAgeUnit()));
                }

                tableModel.setValue("AccessAgeText", accessAge);

                tableModel.setValue("ArchiveAgeText",
                                  PolicyUtil.getArchiveAgeString(criteria[i]));

                criteriaNumbers.append(criteriaNumber);

                // remove any old selections that may still be lingering around
                tableModel.setRowSelected(false);

                // populate the media type column
                tableModel.setValue("MediaType",
                                    PolicyUtil.getMediaTypeString(criteria[i]));
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to populate table model",
                                     serverName);

            SamUtil.setWarningAlert(getParentViewBean(),
                                    PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to populate table model",
                                     serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.detail",
                                  smme.getSAMerrno(),
                                  smme.getMessage(),
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "populateTableModel",
                                     "unable to populate table model",
                                     serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        //  set in hidden field
        CCHiddenField field = (CCHiddenField)
            parent.getChild(PolicyDetailsViewBean.CRITERIA_NUMBERS);
        field.setValue(criteriaNumbers.toString());
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // set table headers
        initializeTableHeaders();

        // disable remove & buttons to offset any
        ((CCButton)getChild("RemoveCriteria")).setDisabled(true);
        ((CCButton)getChild("EditCriteria")).setDisabled(true);

        // if readonly, disbale the add button
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {

            tableModel.setSelectionType(CCActionTableModel.NONE);
            ((CCButton)getChild(ADD_CRITERIA)).setDisabled(true);
        }
    }

    // implement the CCPagelet interface

    /**
     * return the appropriate pagelet jsp
     *
     * @see - com.sun.web.ui.common.CCPagelet#getPageletUrl
     */
    public String getPageletUrl() {
        Integer policyType = (Integer)getParentViewBean().
            getPageSessionAttribute(Constants.Archive.POLICY_TYPE);

        short type = policyType.shortValue();
        if (type == ArSet.AR_SET_TYPE_GENERAL ||
            type == ArSet.AR_SET_TYPE_NO_ARCHIVE) {
            return "/jsp/archive/FileMatchCriteriaPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
