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

// ident	$Id: DataClassDetailsView.java,v 1.8 2008/05/16 19:39:27 am143972 Exp $

/**
 * This file is not used anymore.  We do not want to show the file system
 * information anymore in the Data Class Details Page.
 *
 * Leave this page alone a little while.  We can remove this file when the dust
 * settles.
 */

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import java.io.IOException;
import java.util.Map;
import javax.servlet.ServletException;

public class DataClassDetailsView extends MultiTableViewBase {
    public static final String FS_TABLE = "DataClassDetailsFSTable";
    public static final String FS_TILED_VIEW = "FSTiledView";
    private CCActionTableModel model = null;

    public DataClassDetailsView(View parent, Map models, String name) {
        super(parent, models, name);

        TraceUtil.trace3("Entering");

        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(FS_TILED_VIEW, DataClassDetailsFSTiledView.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(FS_TILED_VIEW)) {
            return new DataClassDetailsFSTiledView(
                this, getTableModel(FS_TABLE), name);
        } else if (name.equals(FS_TABLE)) {
            return createTable(name, FS_TILED_VIEW);
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null)
                return model.createChild(this, name);
        }

        // child with no known parent
        throw new IllegalArgumentException("invalid child '" + name + "'");
    }

    private void initializeTableHeaders() {
        // set action button labels
        model = getTableModel(FS_TABLE);
        model.setActionValue("AddFS", "archiving.add");
        model.setActionValue("RemoveFS", "archiving.remove");

        // init fs table table column headings
        model.setActionValue("FSName", "archiving.fs.name");
        model.setActionValue("MountPoint", "archiving.fs.mountpoint");
    }

    public void populateTableModels(ArchivePolCriteria criteria) {
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
            // populate the FileSystems-using-criteria table
            model = getTableModel(FS_TABLE);
            model.clear();

            FileSystem [] fs = null;

            String fsDeletable = "false";
            // if global criteria, display all the file systems
            if (criteria.getArchivePolCriteriaProperties().isGlobal()) {
                fs = SamUtil.getModel(serverName).
                    getSamQFSSystemFSManager().getAllFileSystems();

                // disable add button
                CCButton addButton = (CCButton)getChild("AddFS");
                addButton.setDisabled(true);

                fsDeletable = "false";
            } else {
                fs = criteria.getFileSystemsForCriteria();
                fsDeletable = Boolean.toString(fs.length > 1);
            }

            // just so we don't have to check for null below
            if (fs == null) fs = new FileSystem[0];

            for (int i = 0; i < fs.length; i++) {
                if (i > 0) {
                    model.appendRow();
                }
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
                parent.getChild(DataClassDetailsViewBean.FS_DELETABLE);
            hf.setValue(fsDeletable);
            hf = (CCHiddenField)
                parent.getChild(DataClassDetailsViewBean.FS_LIST);
            hf.setValue(buffer.toString());

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
            parent.getDisplayFieldStringValue(DataClassDetailsViewBean.FS_NAME);
        try {
            // get the criteria,  remove the filesystem,  and update policy
            ArchivePolicy thePolicy = SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            if (thePolicy == null) {
                throw new SamFSException(null, -2000);
            }

            ArchivePolCriteria criteria =
                thePolicy.getArchivePolCriteria(criteriaNumber.intValue());
            criteria.deleteFileSystemForCriteria(fsName);
            criteria.getArchivePolicy().updatePolicy();

            // set confirmation alert
            SamUtil.setInfoAlert(getParentViewBean(),
                                 CommonViewBeanBase.CHILD_COMMON_ALERT,
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
                                  CommonViewBeanBase.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "handleRemoveFSRequest",
                                     "Unable to remove fs from criteria",
                                    serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  CommonViewBeanBase.CHILD_COMMON_ALERT,
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
                                  CommonViewBeanBase.CHILD_COMMON_ALERT,
                                  SamUtil.getResourceString(
                                    "archiving.criteria.fsdelete.failure",
                                    new String [] {fsName}),
                                    sfe.getSAMerrno(),
                                    sfe.getMessage(),
                                        serverName);
        }

        // refresh the page
        getParentViewBean().forwardTo(getRequestContext());
    }
}
