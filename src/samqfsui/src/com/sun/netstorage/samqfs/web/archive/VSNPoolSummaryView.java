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

// ident	$Id: VSNPoolSummaryView.java,v 1.35 2008/12/16 00:10:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Creates the 'VSN Pool Summary' Action Table and contains
 * handlers for the links within the table.
 */
public class VSNPoolSummaryView extends CommonTableContainerView {
    public static final String CHILD_NEW_VSNPOOL_HREF = "NewVSNPoolHref";
    public static final String CHILD_EDIT_VSNPOOL_HREF = "EditVSNPoolHref";
    public static final String SELECTED_POOL = "selectedPool";
    public static final String CHILD_TILED_VIEW = "VSNPoolSummaryTiledView";

    private CCActionTableModel model = null;

    public VSNPoolSummaryView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "VSNPoolSummaryTable";

        createTableModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(model);
        registerChild(SELECTED_POOL, CCHiddenField.class);
        registerChild(CHILD_NEW_VSNPOOL_HREF, CCHref.class);
        registerChild(CHILD_EDIT_VSNPOOL_HREF, CCHref.class);
        registerChild(CHILD_TILED_VIEW, VSNPoolSummaryTiledView.class);

        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(SELECTED_POOL)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_NEW_VSNPOOL_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_EDIT_VSNPOOL_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            return new VSNPoolSummaryTiledView(this, model, name);
        } else {
            return super.createChild(model, name, CHILD_TILED_VIEW);
        }
    }

    private void createTableModel() {
        model = new CCActionTableModel(
           RequestManager.getRequestContext().getServletContext(),
           "/jsp/archive/VSNPoolSummaryTable.xml");
    }

    private void checkRolePrivilege() throws ModelControlException {
        TraceUtil.trace3("Entering");

        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {
            model.setSelectionType("none");
        } else {
            ((CCButton) getChild("New")).setDisabled(false);

            // disable delete & remove until user selects something
            ((CCButton)getChild("Delete")).setDisabled(true);
        }
        TraceUtil.trace3("Exiting");
    }

    // handle request for delete button
    public void handleDeleteRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String poolName = getDisplayFieldStringValue(SELECTED_POOL);

        TraceUtil.trace3("Delete VSN Pool ... name = " + poolName);
        String serverName = parent.getServerName();
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            sysModel.
                getSamQFSSystemArchiveManager().deleteVSNPool(poolName);

            showAlert("VSNPoolSummary.delete.alert", poolName);
        } catch (SamFSException smfex) {

            String processMsg = null;
            String errMsg = null;
            String errCause = null;
            boolean warning = false;

            if (smfex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
            } else if (smfex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.warning.summary";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to delete vsn pool";
                errMsg = SamUtil.getResourceString(
                            "VSNPoolSummary.error.failedDelete", poolName);
                errCause = smfex.getMessage();
            }

            SamUtil.processException(smfex, this.getClass(),
                                     "handleDeleteRequest",
                                     processMsg,
                                     serverName);
            if (!warning) {
                SamUtil.setErrorAlert(parent,
                                      parent.CHILD_COMMON_ALERT,
                                      errMsg,
                                      smfex.getSAMerrno(),
                                      errCause,
                                      serverName);
            } else {
                SamUtil.setWarningAlert(parent,
                                        parent.CHILD_COMMON_ALERT,
                                        errMsg,
                                        errCause);
            }
        }

        // sleeping so that sam-amld has a chance to start
        // which is needed in order to update the data
        try {
            Thread.sleep(5000);
        } catch (InterruptedException iex) {
            TraceUtil.trace3("InterruptedException Caught: Reason: " +
                             iex.getMessage());
        }

        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void initTableHeaders() {
        // set button labels
        model.setActionValue("New", "archiving.new");
        model.setActionValue("Delete", "archiving.delete");

        // set column headers
        model.setActionValue("VSNPoolName", "VSNPoolSummary.heading1");
        model.setActionValue("MediaType", "VSNPoolSummary.heading2");
        model.setActionValue("Members", "VSNPoolSummary.heading3");
        model.setActionValue("FreeSpace", "VSNPoolSummary.heading4");
    }

    public void populateTableModel() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        StringBuffer inUse = new StringBuffer();
        StringBuffer buffer = new StringBuffer();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            VSNPool [] pool =
                sysModel.getSamQFSSystemArchiveManager().getAllVSNPools();

            // prevent those darn null pointer exception
            if (pool == null)
                pool = new VSNPool[0];

            // disable tooltips
            ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
             getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

            // clear the model
            model.clear();
            for (int i = 0; i < pool.length; i++) {
                if (i > 0) {
                    model.appendRow();

                    buffer.append(";").append(pool[i].getPoolName());
                } else {
                    buffer.append(pool[i].getPoolName());
                }

                String poolName = pool[i].getPoolName();
                int mediaType = pool[i].getMediaType();
                Capacity freeSpace = new Capacity(pool[i].getSpaceAvailable(),
                                                  SamQFSSystemModel.SIZE_MB);
                String status = sysModel.getSamQFSSystemArchiveManager().
                    isPoolInUse(poolName) ? "true" : "false";

                // visible columns
                model.setValue("VSNPoolNameText", poolName);
                model.setValue("MediaTypeText",
                               SamUtil.getMediaTypeString(mediaType));
                model.setValue("MembersText",
                               new Integer(pool[i].getNoOfVSNsInPool()));

                model.setValue("FreeSpaceText", freeSpace);

                // and now for the extras
                model.setValue("VSNPoolHref", poolName);
                model.setValue("VSNPoolHiddenField", poolName);
                model.setValue("HiddenStatus", status);
                model.setValue("MediaTypeHidden", new Integer(mediaType));

                // disable selection
                model.setRowSelected(false);
                if (sysModel.getSamQFSSystemArchiveManager().
                    isPoolInUse(pool[i].getPoolName())) {
                        inUse.append(i).append(";");
                }

            }

            // save pool names for browser use
            ((CCHiddenField)parent
             .getChild(VSNPoolSummaryViewBean.POOL_NAMES))
                . setValue(buffer.toString());
            ((CCHiddenField)parent
             .getChild(VSNPoolSummaryViewBean.POOLS_IN_USE))
                .setValue(inUse.toString());
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     getClass(),
                                     "populateTableModel",
                                     "Unable to retrieve VSN pools",
                                     serverName);

            SamUtil.setWarningAlert(parent,
                                    parent.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     getClass(),
                                     "populate Table Model",
                                     "Unable to retrieve VSN Pools",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "populateTableModel",
                                     "Unable to retrieve VSN Pools",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        initTableHeaders();

        checkRolePrivilege();
    }

    private void showAlert(String operation, String key) {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        SamUtil.setInfoAlert(parent,
                             VSNPoolSummaryViewBean.CHILD_COMMON_ALERT,
                             "success.summary",
                             SamUtil.getResourceString(operation, key),
                             parent.getServerName());
        TraceUtil.trace3("Exiting");
    }
}
