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

// ident	$Id: CopyInformationView.java,v 1.26 2008/05/16 19:39:26 am143972 Exp $

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
import com.sun.netstorage.samqfs.web.archive.wizards.NewCopyWizardImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.ui.taglib.WizardWindowTag;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
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
 * CopyInformationView
 *
 *  - used by custom, default, and settable defaults policies
 */
public class CopyInformationView extends CommonTableContainerView
    implements CCPagelet {
    public static final String ADD_COPY_FORWARDTO = "AddCopyForwardToVB";
    public static final String ADD_COPY = "SamQFSWizardAddCopy";

    // children
    public static final String TILED_VIEW = "CopyInformationTiledView";

    private CCWizardWindowModel addCopyWWModel = null;
    private boolean addCopyWizardRunning = false;

    // the table model
    private CCActionTableModel tableModel = null;

    public CopyInformationView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "CopyInformationTable";
        createTableModel();

        initializeCopyWizardButton();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(tableModel);
        registerChild(TILED_VIEW, CopyInformationTiledView.class);
        registerChild(ADD_COPY_FORWARDTO, BasicCommandField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ADD_COPY_FORWARDTO)) {
            return new BasicCommandField(this, name);
        } else if (name.equals(TILED_VIEW)) {
            return new CopyInformationTiledView(this, tableModel, name);
        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    private void createTableModel() {
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/CopyInformationTable.xml");
    }

    private void initializeCopyWizardButton() {
        TraceUtil.trace3("Entering");
        // retrieve the policy name so we can set it on the model
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)parent.getPageSessionAttribute(
            Constants.Archive.POLICY_NAME);

        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        buf.append(getParentViewBean().getQualifiedName())
           .append(".")
           .append(getName())
           .append(".")
           .append(ADD_COPY_FORWARDTO);
        addCopyWWModel = NewCopyWizardImpl.createModel(buf.toString());
        addCopyWWModel.setValue(WizardWindowTag.CLIENTSIDE_PARAM_JSFUNCTION,
            "getAddPolicyCopyWizardParams");
        tableModel.setModel("SamQFSWizardAddCopy", addCopyWWModel);
        addCopyWWModel.setValue("SamQFSWizardAddCopy", "archiving.add");
        addCopyWWModel.setValue(
            Constants.SessionAttributes.POLICY_NAME, policyName);
        addCopyWWModel.setValue(
           Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        TraceUtil.trace3("Exiting");
    }

    public void handleAddCopyForwardToVBRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        addCopyWizardRunning = false;

        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleSamQFSWizardAddCopyRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        addCopyWizardRunning = true;
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleEditAdvancedOptionsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        String cns = source.getDisplayFieldStringValue(
            PolicyDetailsViewBean.SELECTED_COPY);
        String cmt = source.getDisplayFieldStringValue(
            PolicyDetailsViewBean.SELECTED_COPY_MEDIA);

        Integer copyNumber = new Integer(cns);
        Integer copyMediaType = new Integer(cmt);

        source.setPageSessionAttribute(Constants.Archive.COPY_NUMBER,
                                       copyNumber);
        source.setPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE,
                                       copyMediaType);


        ViewBean target = getViewBean(CopyOptionsViewBean.class);

        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);

        TraceUtil.trace3("Exiting");
    }



    public void handleEditVSNAssignmentsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();

        String cns = source.getDisplayFieldStringValue(
            PolicyDetailsViewBean.SELECTED_COPY);
        String cmt = source.getDisplayFieldStringValue(
            PolicyDetailsViewBean.SELECTED_COPY_MEDIA);

        Integer copyNumber = new Integer(cns);
        Integer copyMediaType = new Integer(cmt);

        source.setPageSessionAttribute(Constants.Archive.COPY_NUMBER,
                                       copyNumber);
        source.setPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE,
                                       copyMediaType);
        ViewBean target = getViewBean(CopyVSNsViewBean.class);

        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
        TraceUtil.trace3("Exiting");
    }

    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");

        // column headers
        tableModel.setActionValue("CopyNumber",
                                  "archiving.policy.copy.number");
        tableModel.setActionValue("MediaType",  "archiving.media.type");
        tableModel.setActionValue("SpaceAvailable",
                                  "archiving.spaceavailable");
        tableModel.setActionValue("VSNsAvailable", "archiving.vsnsavailable");


        // button labels
        tableModel.setActionValue("SamQFSWizardAddCopy", "archiving.add");
        tableModel.setActionValue("RemoveCopy", "archiving.remove");
        tableModel.setActionValue("EditAdvancedOptions",
                                  "archiving.editoptions");
        tableModel.setActionValue("EditVSNAssignments", "archiving.editvsns");

        TraceUtil.trace3("Exiting");
    }


    public void handleRemoveCopyRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        PolicyDetailsViewBean parent =
            (PolicyDetailsViewBean)getParentViewBean();

        String cnString =
            parent.getDisplayFieldStringValue(parent.SELECTED_COPY);

        // retrieve the server na em for exceptions
        String serverName = parent.getServerName();
        try {
            int copyNumber = Integer.parseInt(cnString);

            // retrieve the policy
            String policyName = (String)
                parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
            ArchivePolicy thePolicy = SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            thePolicy.deleteArchiveCopy(copyNumber);

            // set success alert
            String temp = SamUtil.getResourceString(
                "archiving.copynumber", cnString);

            SamUtil.setInfoAlert(getParentViewBean(),
                                 PolicyDetailsViewBean.CHILD_COMMON_ALERT,
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
                                    "ArchiveConfig.warning.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "handleRemoveCriteriaRequest",
                                     "Unable to remove policy criteria",
                                     serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  sme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
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

        TraceUtil.trace3("Exiting");
        // forward to self
        parent.forwardTo(getRequestContext());
    }

    public void populateTableModel() {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);

        // disable tool tips
        ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
         getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

        NonSyncStringBuffer copyNumbers = new NonSyncStringBuffer();
        NonSyncStringBuffer copyMediaTypes = new NonSyncStringBuffer();

        int copyCount = -1;
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            ArchiveCopy [] copy = thePolicy.getArchiveCopies();
            copyCount = copy.length;
            if (copy == null) {
                return;
            }

            // clear model
            tableModel.clear();
            for (int i = 0; i < copy.length; i++) {
                if (i > 0) {
                    tableModel.appendRow();
                    copyMediaTypes.append(",");
                    copyNumbers.append(",");
                }

                Integer copyNumberInt = new Integer(copy[i].getCopyNumber());

                // now populate the table
                String copyNumber = "";

                // check for the all sets copy
                if (copy[i].getCopyNumber() == 5) {
                    copyNumber = "archiving.copy.allsets";
                } else {
                    copyNumber = SamUtil.getResourceString(
                       "archiving.copynumber",
                       Integer.toString(copy[i].getCopyNumber()));
                }

                // populate the table
                tableModel.setValue("CopyNumberText", copyNumber);
                tableModel.setValue("CopyNumberHref",
                    Integer.toString(copy[i].getCopyNumber()));

                // get media settings from the vsn map
                ArchiveVSNMap vsnMap = copy[i].getArchiveVSNMap();

                int mediaType = vsnMap.getArchiveMediaType();
                TraceUtil.trace2("Copy #" + copy[i].getCopyNumber() +
                                 "; Media Type = " + mediaType +
                                 "; available vsns = " +
                                 vsnMap.getMemberVSNNames());

                tableModel.setValue("MediaTypeText",
                                    SamUtil.getMediaTypeString(mediaType));

                tableModel.setValue("MediaTypeHidden",
                                    Integer.toString(mediaType));

                tableModel.setValue("VSNsAvailableText",
                     PolicyUtil.getVSNString(vsnMap.getMemberVSNNames()));

                tableModel.setValue("SpaceAvailableText",
                                    new Capacity(vsnMap.getAvailableSpace(),
                                                 SamQFSSystemModel.SIZE_MB));

                // set all href values to copy number
                String cn = Integer.toString(copy[i].getCopyNumber());
                tableModel.setValue("CopyNumberHidden", cn);
                tableModel.setValue("CopyOptionsHref", cn);

                copyNumbers.append(copyNumberInt);
                copyMediaTypes.append(new Integer(mediaType));

                // remove any previous row selections that might be still
                // lingering around
                tableModel.setRowSelected(false);
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateTableModel",
                                     "Unable to retrieve criteria details",
                                     serverName);

            SamUtil.setWarningAlert(getParentViewBean(),
                                    PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.warning.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "populateTableModel",
                                     "Unable to retrieve copy details",
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
                                     "populateTableModel",
                                     "Unable to retrieve copy details",
                                     serverName);

            // update confirmation alert
            SamUtil.setErrorAlert(getParentViewBean(),
                                  PolicyDetailsViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // set copy numbers

        CCHiddenField field = (CCHiddenField)
            parent.getChild(PolicyDetailsViewBean.COPY_NUMBERS);
        field.setValue(copyNumbers.toString());

        // set copy media types
        field = (CCHiddenField)
            parent.getChild(PolicyDetailsViewBean.MEDIA_TYPES);
        field.setValue(copyMediaTypes.toString());

        // disable add button if the 4 allowed copies exist
        boolean disableAddButton = false;
        if (copyCount >= 4) {
           disableAddButton = true;
        }
        ((CCButton)getChild(ADD_COPY)).setDisabled(disableAddButton);

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        initializeTableHeaders();

        // disable buttons to offset state data
        ((CCButton)getChild("RemoveCopy")).setDisabled(true);
        ((CCButton)getChild("EditAdvancedOptions")).setDisabled(true);
        ((CCButton)getChild("EditVSNAssignments")).setDisabled(true);

        // if allssets or read only ,  disable add button as well
        short policyType =
            ((PolicyDetailsViewBean)getParentViewBean()).getPolicyType();

        if (policyType ==  ArSet.AR_SET_TYPE_ALLSETS_PSEUDO) {
            ((CCButton)getChild(ADD_COPY)).setDisabled(true);
        }

        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {

            tableModel.setSelectionType(CCActionTableModel.NONE);
            ((CCButton)getChild(ADD_COPY)).setDisabled(true);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * implements the :
     * @see -  com.sun.web.ui.common.CCPagelet#getPageletUrl
     *
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String jspPath = "/jsp/archive/BlankPagelet.jsp";
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        Integer type = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_TYPE);

        short policyType = type.shortValue();

        if (policyType == ArSet.AR_SET_TYPE_GENERAL ||
            policyType == ArSet.AR_SET_TYPE_DEFAULT ||
            policyType == ArSet.AR_SET_TYPE_ALLSETS_PSEUDO ||
            policyType == ArSet.AR_SET_TYPE_UNASSIGNED ||
            policyType == ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT) {
            return "/jsp/archive/CopyInformationPagelet.jsp";
        }

        TraceUtil.trace3("Exiting");
        return jspPath;

    }
}
