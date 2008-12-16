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

// ident	$Id: CopySettingsView.java,v 1.16 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.servlet.ServletException;

/**
 * CopySettingsView -
 *
 * used by the :
 *    - policy details page of default policies and
 *    - criteria details page of custom policies
 */
public class CopySettingsView extends CommonTableContainerView
    implements CCPagelet {

    public static final String TILED_VIEW = "CopySettingsTiledView";
    private CCActionTableModel tableModel = null;

    /** construct a child of this view */
    public CopySettingsView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "CopySettingsTable";
        createTableModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this view's child views */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(TILED_VIEW, CopySettingsTiledView.class);
        super.registerChildren(tableModel);
        TraceUtil.trace3("Exiting");
    }

    /** create a given child */
    public View createChild(String name) {
        if (name.equals(TILED_VIEW)) {
            return new CopySettingsTiledView(this, tableModel, name);
        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    /** create an instance model */
    private void createTableModel() {
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/CopySettingsTable.xml");
    }

    /** initiliaze column headers */
    private void initializeTableHeaders() {
        // column headers
        tableModel.setActionValue("CopyNumber", "archiving.policy.copy.number");
        tableModel.setActionValue("ArchiveAge", "archiving.archiveage");
        tableModel.setActionValue("UnarchiveAge", "archiving.unarchiveage");
        tableModel.setActionValue("ReleaseOptions",
                                  "archiving.releaseoptions");

        // button labels
        tableModel.setActionValue("Save", "archiving.save");
        tableModel.setActionValue("Reset", "archiving.reset");
    }

    /** populate table model */
    public void populateTableModel() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);

        // retrieve the relevant criteria number
        Integer criteriaNumber = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);
        Integer policyType = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_TYPE);
        if (criteriaNumber == null &&
            (((short)policyType.intValue()) == ArSet.AR_SET_TYPE_DEFAULT ||
             ((short)policyType.intValue()) ==
             ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT)) {

            criteriaNumber = new Integer(0);
            parent.setPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER,
                                           criteriaNumber);
        }

        // populate the archive copy settings table
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            SamQFSSystemArchiveManager manager =
                sysModel.getSamQFSSystemArchiveManager();

            // TODO: expensive
            // force to sync with config file
            manager.getAllArchivePolicies();

            // retrieve the policy criteria and its copies
            ArchivePolicy thePolicy = manager.getArchivePolicy(policyName);
            ArchivePolCriteria theCriteria =
                thePolicy.getArchivePolCriteria(criteriaNumber.intValue());

            ArchivePolCriteriaCopy [] copies =
                theCriteria.getArchivePolCriteriaCopies();

            tableModel.clear();
            for (int i = 0; i < copies.length; i++) {
                if (i > 0) {
                    tableModel.appendRow();
                }

                tableModel.setValue("CopyNumberText",
                    SamUtil.getResourceString("archiving.copynumber",
                    Integer.toString(
                        copies[i].getArchivePolCriteriaCopyNumber())));

                tableModel.setValue("CopyNumberHidden",
                    Integer.toString(
                        copies[i].getArchivePolCriteriaCopyNumber()));

                // don't set a value of -1
                if (copies[i].getArchiveAge() >= 0) {
                    tableModel.setValue("ArchiveAgeText",
                                Long.toString(copies[i].getArchiveAge()));
                }

                tableModel.setValue("ArchiveAgeUnits",
                    Integer.toString(copies[i].getArchiveAgeUnit()));

                // don't set a value of -1
                if (copies[i].getUnarchiveAge() >= 0) {
                    tableModel.setValue("UnarchiveAgeText",
                                  Long.toString(copies[i].getUnarchiveAge()));
                }

                tableModel.setValue("UnarchiveAgeUnits",
                    Integer.toString(copies[i].getUnarchiveAgeUnit()));

                // determine if release options is set
                int releaseOptions = PolicyUtil.ReleaseOptions.SPACE_REQUIRED;
                if (copies[i].isNoRelease())
                    releaseOptions += PolicyUtil.ReleaseOptions.WAIT_FOR_ALL;

                if (copies[i].isRelease())
                    releaseOptions += PolicyUtil.ReleaseOptions.IMMEDIATELY;

                tableModel.setValue("ReleaseOptionsText",
                                    Integer.toString(releaseOptions));
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateTableModels",
                                     "Unable to populate tables",
                                     serverName);

            SamUtil.setWarningAlert(parent,
                                    parent.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSException sfe) {
            // this catches both SamFSException and SamFSMultMsgException
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "populateTableModels",
                                     "Unable to populate tables",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  SamUtil.getResourceString("-2020"),
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    // handler for the save button
    /**
     * handler for the save button
     */
    public void handleSaveRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaNumber = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        // list of error messages
        List errors = new ArrayList();
        ArchivePolCriteriaCopy criteriaCopy = null;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            // retrieve the relevant objects
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            ArchivePolCriteria theCriteria =
                thePolicy.getArchivePolCriteria(criteriaNumber.intValue());

            CCActionTable table = (CCActionTable)getChild(CHILD_ACTION_TABLE);
            table.restoreStateData();
            CCActionTableModel model = (CCActionTableModel)table.getModel();
            int rowCount = model.getNumRows();



            for (int i = 0; i < rowCount; i++) {
                model.setRowIndex(i);

                String s = (String)model.getValue("CopyNumberHidden");

                long archiveAge = -1;
                int archiveAgeUnit = -1;
                long unarchiveAge = -1;
                int unarchiveAgeUnit = -1;

                boolean copyValid = true;

                // archive age
                String archiveAgeStr = (String)
                    model.getValue(CriteriaDetailsCopyTiledView.ARCHIVE_AGE);

                archiveAgeStr =
                    archiveAgeStr != null ? archiveAgeStr.trim() : "";
                if (!archiveAgeStr.equals("")) {
                    try {
                        archiveAge = Long.parseLong(archiveAgeStr);
                        if (archiveAge < 0) {
                            copyValid = false;
                            errors.add(SamUtil.getResourceString(
                                "archiving.criteriacopy.archiveage.negative",
                                SamUtil.getResourceString(
                                "archiving.copynumber", s)));
                        }
                    } catch (NumberFormatException nfe) {
                        copyValid = false;
                        errors.add(SamUtil.getResourceString(
                            "archiving.criteriacopy.archiveage.negative",
                            SamUtil.getResourceString("archiving.copynumber",
                                                      s)));
                    }
                } else {
                    copyValid = false;
                    errors.add(SamUtil.getResourceString(
                        "archiving.criteriacopy.archiveage.missing",
                        SamUtil.getResourceString("archiving.copynumber", s)));
                } // end archive age

                // validate archive age unit
                String archiveAgeUnitStr = (String)model.getValue(
                    CriteriaDetailsCopyTiledView.ARCHIVE_AGE_UNITS);

                archiveAgeUnit = Integer.parseInt(archiveAgeUnitStr);

                // validate unarchive age
                String unarchiveAgeStr = (String)
                    model.getValue(CriteriaDetailsCopyTiledView.UNARCHIVE_AGE);
                String unarchiveAgeUnitStr = (String)model.getValue(
                    CriteriaDetailsCopyTiledView.UNARCHIVE_AGE_UNITS);

                unarchiveAgeStr =
                    unarchiveAgeStr != null ? unarchiveAgeStr.trim() : "";
                if (!unarchiveAgeStr.equals("")) {
                    try {
                        unarchiveAge = Long.parseLong(unarchiveAgeStr);
                        if (unarchiveAge < 0) {
                            copyValid = false;
                            errors.add(SamUtil.getResourceString(
                                "archiving.criteriacopy.unarchiveage.negative",
                                 SamUtil.getResourceString(
                                "archiving.copynumber", s)));
                        }
                    } catch (NumberFormatException nfe) {
                        copyValid = false;
                        errors.add(SamUtil.getResourceString(
                            "archiving.criteriacopy.unarchiveage.negative",
                            SamUtil.getResourceString("archiving.copynumber",
                                                      s)));
                    }
                } // end unarchive age

                unarchiveAgeUnit = Integer.parseInt(unarchiveAgeUnitStr);

                // retrieve release options
                String releaseOptionsStr = (String)model.getValue(
                    CriteriaDetailsCopyTiledView.RELEASE_OPTIONS);

                if (copyValid) {
                    int copyNumber = Integer.parseInt(s);
                    criteriaCopy = PolicyUtil.
                        getArchivePolCriteriaCopy(theCriteria, copyNumber);
                    criteriaCopy.setArchiveAge(archiveAge);
                    criteriaCopy.setArchiveAgeUnit(archiveAgeUnit);
                    criteriaCopy.setUnarchiveAge(unarchiveAge);
                    criteriaCopy.setUnarchiveAgeUnit(unarchiveAgeUnit);

                    // save the release options
                    int options = Integer.parseInt(releaseOptionsStr);
                    switch (options) {
                    case PolicyUtil.ReleaseOptions.SPACE_REQUIRED:
                        criteriaCopy.setNoRelease(false);
                        criteriaCopy.setRelease(false);
                        break;
                    case PolicyUtil.ReleaseOptions.IMMEDIATELY:
                        criteriaCopy.setNoRelease(false);
                        criteriaCopy.setRelease(true);
                        break;
                    case PolicyUtil.ReleaseOptions.WAIT_FOR_ALL:
                        criteriaCopy.setNoRelease(true);
                        criteriaCopy.setRelease(false);
                        break;
                    default:
                    } // end switch
                } // end if copyValid
            } // for each copy


            // check if any of the copies in error
            if (errors.size() > 0) {
                // print error messages
                NonSyncStringBuffer buffer = new NonSyncStringBuffer();
                Iterator it = errors.iterator();

                buffer.append("<uL>");
                while (it.hasNext()) {
                    buffer.append("<li>");
                    buffer.append(it.next()).append("<br>");
                    buffer.append("</li>");
                }
                buffer.append("</ul>");

                // show the error list
                SamUtil.setErrorAlert(parent,
                                      parent.CHILD_COMMON_ALERT,
                                    "archiving.copysettings.validation.error",
                                      -2022,
                                      buffer.toString(),
                                      serverName);
            } else {
                // update the policy and exit
                thePolicy.updatePolicy();

                String ci =
                    SamUtil.getResourceString("archiving.criterianumber",
                                new String [] {criteriaNumber.toString()});

                // update info alert
                SamUtil.setInfoAlert(parent,
                                     parent.CHILD_COMMON_ALERT,
                                     "success.summary",
                                     SamUtil.getResourceString(
                                         "archiving.copysettings.save.success",
                                         new String [] {ci}),
                                     serverName);

            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save copy settings",
                                     serverName);

            SamUtil.setWarningAlert(parent,
                                    parent.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save copy settings",
                                     serverName);


            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save copy settings",
                                     serverName);

            String [] temp1 = {criteriaNumber.toString()};
            String [] temp2 =
                {SamUtil.getResourceString("archiving.criterianumber",
                                           temp1)};
            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  SamUtil.getResourceString(
                                      "archiving.copysettings.save.failure",
                                      temp2),
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        } catch (ModelControlException mce) {
            SamUtil.processException(mce,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save copy settings",
                                     serverName);
        }

        getParentViewBean().forwardTo(getRequestContext());
    }

    /** commence the dispaly of this view */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // initialize the table headers
        initializeTableHeaders();

        // disable save & reset when in read-only mode
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            ((CCButton)getChild("Save")).setDisabled(true);
            ((CCButton)getChild("Reset")).setDisabled(true);
        }
    }

    /**
     * @see - com.sun.web.ui.common.CCPagelet#getPageletUrl
     */
    public String getPageletUrl() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        Integer type = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_TYPE);

        if (type.shortValue() == ArSet.AR_SET_TYPE_DEFAULT ||
	    type.shortValue() == ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT) {
            return "/jsp/archive/CopySettingsPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
