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

// ident	$Id: CriteriaDetailsView.java,v 1.18 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.web.fs.ChangeFileAttributesView;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import javax.servlet.ServletException;

public class CriteriaDetailsView extends MultiTableViewBase {
    public static final String COPY_TABLE = "CriteriaDetailsCopySettingsTable";
    public static final String FS_TABLE = "CriteriaDetailsFSTable";
    public static final String COPY_TILED_VIEW = "CopySettingsTiledView";
    public static final String FS_TILED_VIEW = "FSTiledView";

    // Pagelets for release and stage attributes
    public static final String RELEASE_ATTR_VIEW = "ReleaseAttributesView";
    public static final String STAGE_ATTR_VIEW = "StageAttributesView";

    // section label for releasing and staging
    public static final String SECTION_LABEL = "SectionLabel";

    public CriteriaDetailsView(View parent, Map models, String name) {
        super(parent, models, name);

        TraceUtil.trace3("Entering");

        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        registerChild(SECTION_LABEL, CCLabel.class);
        registerChild(COPY_TILED_VIEW, CriteriaDetailsCopyTiledView.class);
        registerChild(FS_TILED_VIEW, CriteriaDetailsFSTiledView.class);
        registerChild(RELEASE_ATTR_VIEW, ReleaseAttributesView.class);
        registerChild(STAGE_ATTR_VIEW, StageAttributesView.class);

        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(SECTION_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(COPY_TILED_VIEW)) {
            return new CriteriaDetailsCopyTiledView(
                this, getTableModel(COPY_TABLE), name);
        } else if (name.equals(FS_TILED_VIEW)) {
            return new CriteriaDetailsFSTiledView(
                this, getTableModel(FS_TABLE), name);
        } else if (name.equals(COPY_TABLE)) {
            return createTable(name, COPY_TILED_VIEW);
        } else if (name.equals(FS_TABLE)) {
            return createTable(name, FS_TILED_VIEW);
        } else if (name.equals(RELEASE_ATTR_VIEW)) {
            return new ReleaseAttributesView(
                this, name, getServerName());
        } else if (name.equals(STAGE_ATTR_VIEW)) {
            return new StageAttributesView(
                this, name, getServerName());
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null)
                return model.createChild(this, name);
        }

        // child with no known parent
        throw new IllegalArgumentException("invalid child '" + name + "'");
    }

    private void initializeTableHeaders() {
        // init copy table column headings
        CCActionTableModel model = getTableModel(COPY_TABLE);
        model.setActionValue("CopyNumber", "archiving.policy.copy.number");
        model.setActionValue("ArchiveAge", "archiving.archiveage");
        model.setActionValue("UnarchiveAge", "archiving.unarchiveage");
        model.setActionValue("ReleaseOptions", "archiving.releaseoptions");

        // set action button labels
        model = getTableModel(FS_TABLE);
        model.setActionValue("AddFS", "archiving.add");
        model.setActionValue("RemoveFS", "archiving.remove");
        model.setActionValue("ViewPolicies", "archiving.policy.viewpolicies");

        // init fs table table column headings
        model.setActionValue("FSName", "archiving.fs.name");
        model.setActionValue("MountPoint", "archiving.fs.mountpoint");
    }

    public void populateTableModels() {
        // the server name
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer ci = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        // populate the archive copy settings table
        try {
            CCActionTableModel model = getTableModel(COPY_TABLE);

            // force to sync with config file
            ArchivePolicy thePolicy = SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            if (thePolicy == null)
                throw new SamFSException(null, -2000);

            ArchivePolCriteria criteria =
                thePolicy.getArchivePolCriteria(ci.intValue());

            // this block is only useful when dealing with a custom policy
            if (thePolicy.getPolicyType() == ArSet.AR_SET_TYPE_GENERAL) {

            // we are dealing with a custom policy
            ArchivePolCriteriaCopy [] copies =
                criteria.getArchivePolCriteriaCopies();

            model.clear();
            for (int i = 0; i < copies.length; i++) {
                if (i > 0)
                    model.appendRow();
                model.setValue("CopyNumberText",
                    SamUtil.getResourceString("archiving.copynumber",
                    Integer.toString(
                        copies[i].getArchivePolCriteriaCopyNumber())));

                model.setValue("CopyNumberHidden",
                    Integer.toString(
                        copies[i].getArchivePolCriteriaCopyNumber()));

                // don't set a value of -1
                if (copies[i].getArchiveAge() >= 0) {
                    model.setValue("ArchiveAgeText",
                                    Long.toString(copies[i].getArchiveAge()));
                }

                model.setValue("ArchiveAgeUnits",
                    Integer.toString(copies[i].getArchiveAgeUnit()));

                // don't set a value of -1
                if (copies[i].getUnarchiveAge() >= 0) {
                    model.setValue("UnarchiveAgeText",
                                    Long.toString(copies[i].getUnarchiveAge()));
                }

                model.setValue("UnarchiveAgeUnits",
                    Integer.toString(copies[i].getUnarchiveAgeUnit()));

                // determine if release options is set
                String releaseOptions = SelectableGroupHelper.NOVAL;
                if (!copies[i].isNoRelease()) {
                    // if release is set, retrieve the value
                    if (copies[i].isRelease())
                        releaseOptions = "true";
                    else
                        releaseOptions = "false";
                }
                model.setValue("ReleaseOptionsText", releaseOptions);
            }
            } // end if custom policy

            // populate the FileSystems-using-criteria table
            model = getTableModel(FS_TABLE);
            model.clear();


            FileSystem [] fs = null;

            String fsDeletable = "false";
            // if global criteria, display all the file systems
            if (criteria.getArchivePolCriteriaProperties().isGlobal()) {
                fs = SamUtil.getModel(serverName).
                    getSamQFSSystemFSManager().getAllFileSystems();

                // set global criteria stirng
                CCStaticTextField field = (CCStaticTextField)
                parent.getChild(CriteriaDetailsViewBean.GLOBAL_CRITERIA_TEXT);
                field.setValue("archiving.criteria.globalcriteria.text");

                // disable add button
                CCButton addButton = (CCButton)getChild("AddFS");
                addButton.setDisabled(true);

                fsDeletable = "false";
            } else {
                fs = criteria.getFileSystemsForCriteria();

                fsDeletable = fs.length > 1 ? "true" : "false";
            }

            // just so we don't have to check for null below
            if (fs == null) fs = new FileSystem[0];

            // disable tool tips
            ((CCRadioButton)((CCActionTable)getChild(FS_TABLE)).
             getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

            for (int i = 0; i < fs.length; i++) {
                if (i > 0)
                    model.appendRow();
                model.setValue("FSNameHref", fs[i].getName());
                model.setValue("FSNameText", fs[i].getName());
                model.setValue("FSNameHidden", fs[i].getName());
                model.setValue("MountPointText", fs[i].getMountPoint());

                buffer.append(fs[i].getName()).append(",");

                // clear any previous table selections that may still be
                // lingering around
                model.setRowSelected(false);
            }

            CCHiddenField hf = (CCHiddenField)
                parent.getChild(CriteriaDetailsViewBean.FS_DELETABLE);
            hf.setValue(fsDeletable);
            hf = (CCHiddenField)
                parent.getChild(CriteriaDetailsViewBean.FS_LIST);
            hf.setValue(buffer.toString());

            // Set up attributes that is used by the release/stage attributes
            // pagelet
            StringBuffer fileAttBuf = new StringBuffer();
            fileAttBuf.append(Integer.toString(
                                Archiver.NEVER)).append("###");
            fileAttBuf.append(Integer.toString(
                                Releaser.WHEN_1)).append("###");
            fileAttBuf.append(Stager.NEVER);
            fileAttBuf.append("###").append(
                    Integer.toString(16));
            // Add partial release size to the buffer, -1 if not set
            fileAttBuf.append("###").append(Long.toString(-1));

            // Save file attributes to page session + max partial size of fs
            parent.setPageSessionAttribute(
                ChangeFileAttributesView.PSA_FILE_ATT,
                fileAttBuf.toString());

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

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // initialize table headers
        initializeTableHeaders();

        // always disable remove button fs table
        CCButton button = (CCButton)getChild("RemoveFS");
        button.setDisabled(true);

        // always disable view policies button
        button = (CCButton)getChild("ViewPolicies");
        button.setDisabled(true);

        // disable add fs button if no filesystem authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {

            button = (CCButton)getChild("AddFS");
            button.setDisabled(true);

            // set remove selection
            CCActionTableModel model = getTableModel(FS_TABLE);
            model.setSelectionType(CCActionTableModel.NONE);
        }
    }

    public List saveCopySettings(ArchivePolCriteria criteria) {
        List errors = new ArrayList();

        String serverName =
            ((CommonViewBeanBase)getParentViewBean()).getServerName();

        CCActionTable table = (CCActionTable)getChild(COPY_TABLE);
        try {
            table.restoreStateData();
        } catch (ModelControlException mce) {
            SamUtil.processException(mce,
                                     this.getClass(),
                                     "saveCopySettings",
                                     "unable to restore model state data",
                                     serverName);

        }

        CCActionTableModel model = (CCActionTableModel)table.getModel();
        int rowCount = model.getNumRows();


        ArchivePolCriteriaCopy criteriaCopy = null;
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

            archiveAgeStr = archiveAgeStr != null ? archiveAgeStr.trim() : "";
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
                        SamUtil.getResourceString("archiving.copynumber", s)));
                }
            } else {
                copyValid = false;
                errors.add(SamUtil.getResourceString(
                    "archiving.criteriacopy.archiveage.missing",
                    SamUtil.getResourceString("archiving.copynumber", s)));
            }

            // validate archive age unit
            String archiveAgeUnitStr = (String)model.getValue(
                CriteriaDetailsCopyTiledView.ARCHIVE_AGE_UNITS);
            if (SelectableGroupHelper.NOVAL.equals(archiveAgeUnitStr)) {
                copyValid = false;
                errors.add(SamUtil.getResourceString(
                    "archiving.criteriacopy.archiveage.unit",
                    SamUtil.getResourceString("archiving.copynumber", s)));
            } else {
                archiveAgeUnit = Integer.parseInt(archiveAgeUnitStr);
            }

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
                        SamUtil.getResourceString("archiving.copynumber", s)));
                }

                // validate units
                if (SelectableGroupHelper.NOVAL.equals(unarchiveAgeUnitStr)) {
                    copyValid = false;
                    errors.add(SamUtil.getResourceString(
                        "archiving.criteriacopy.unarchiveage.unit",
                        SamUtil.getResourceString("archiving.copynumber", s)));
                } else {
                    unarchiveAgeUnit = Integer.parseInt(unarchiveAgeUnitStr);
                }
            } else if (
                !SelectableGroupHelper.NOVAL.equals(unarchiveAgeUnitStr)) {
                // units without a size
                copyValid = false;
                errors.add(SamUtil.getResourceString(
                    "archiving.criteriacopy.unarchiveagevalue.missing",
                    SamUtil.getResourceString("archiving.copynumber", s)));
            }

            // retrieve release options
            String releaseOptionsStr = (String)model.getValue(
                CriteriaDetailsCopyTiledView.RELEASE_OPTIONS);

            if (copyValid) {
                int copyNumber = Integer.parseInt(s);
                criteriaCopy = PolicyUtil.getArchivePolCriteriaCopy(criteria,
                                                                copyNumber);
                criteriaCopy.setArchiveAge(archiveAge);
                criteriaCopy.setArchiveAgeUnit(archiveAgeUnit);
                criteriaCopy.setUnarchiveAge(unarchiveAge);
                criteriaCopy.setUnarchiveAgeUnit(unarchiveAgeUnit);

                // set the release options
                if (SelectableGroupHelper.NOVAL.equals(releaseOptionsStr)) {
                    criteriaCopy.setRelease(false);
                } else if (releaseOptionsStr.equals("true")) {
                    criteriaCopy.setRelease(true);
                    criteriaCopy.setNoRelease(false);
                } else if (releaseOptionsStr.equals("false")) {
                    criteriaCopy.setRelease(false);
                    criteriaCopy.setNoRelease(true);
                }
            } // end if copyValid
        }

        return errors;
    }

    /*
     * this should never be called, unless the javascript is broken
     */
    public void handleAddFSRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleRemoveFSRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaNumber = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        String fsName =
            parent.getDisplayFieldStringValue(CriteriaDetailsViewBean.FS_NAME);
        try {
            // get the criteria,  remove the filesystem,  and update policy
            ArchivePolicy thePolicy = SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            if (thePolicy == null)
                throw new SamFSException(null, -2000);

            ArchivePolCriteria criteria =
                thePolicy.getArchivePolCriteria(criteriaNumber.intValue());
            criteria.deleteFileSystemForCriteria(fsName);
            criteria.getArchivePolicy().updatePolicy();

            // set confirmation alert
            SamUtil.setInfoAlert(getParentViewBean(),
                                 CriteriaDetailsViewBean.CHILD_COMMON_ALERT,
                                 "success.summary",
                                 SamUtil.getResourceString(
                                    "archiving.criteria.fsdelete.success",
                                    new String [] {fsName}),
                                    serverName);
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "handleRemoveFSRequest",
                                     "Unable to remove fs from criteria",
                                    serverName);

            SamUtil.setWarningAlert(getParentViewBean(),
                                  CriteriaDetailsViewBean.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "handleRemoveFSRequest",
                                     "Unable to remove fs from criteria",
                                    serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  CriteriaDetailsViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  sme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleRemoveFSRequest",
                                     "Unable to remove fs from criteria",
                                    serverName);
            // set confirmation alert
            SamUtil.setErrorAlert(getParentViewBean(),
                                  CriteriaDetailsViewBean.CHILD_COMMON_ALERT,
                                  SamUtil.getResourceString(
                                    "archiving.criteria.fsdelete.failure",
                                    new String [] {fsName}),
                                    sfe.getSAMerrno(),
                                    sfe.getMessage(),
                                        serverName);
        }

        // recycle the page
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleViewPoliciesRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        // set the filesystem into page session
        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();

        String fsName =
            source.getDisplayFieldStringValue(CriteriaDetailsViewBean.FS_NAME);
        source.setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME, fsName);

        // target view bean
        ViewBean target = getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
    }

    private String getServerName() {
        return ((CommonViewBeanBase) getParentViewBean()).getServerName();
    }
}
