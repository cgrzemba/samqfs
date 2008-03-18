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

// ident	$Id: ImportVSNView.java,v 1.13 2008/03/17 14:43:39 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.StkVSN;
import com.sun.netstorage.samqfs.web.archive.MultiTableViewBase;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.table.CCActionTable;

import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.servlet.ServletException;

/**
 * ImportVSNView - view in Import VSN Page for 4.5+ ACSLS Servers
 */
public class ImportVSNView extends MultiTableViewBase
    implements CCPagelet {

    public static final String VSN_TABLE  = "ImportVSNTable";
    public static final String VOLUME_IDS = "VolumeIDs";
    public static final String SELECTED_VOLUME_IDS  = "SelectedVolumeIDs";

    /** create an instance of ImportVSNView */
    public ImportVSNView(View parent, Map models, String name) {
        super(parent, models, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * registerChildren
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(VOLUME_IDS, CCHiddenField.class);
        registerChild(SELECTED_VOLUME_IDS, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * createChild
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;

        if (name.equals(VSN_TABLE)) {
            child = createTable(name);
        } else if (name.equals(VOLUME_IDS) ||
            name.equals(SELECTED_VOLUME_IDS)) {
            child = new CCHiddenField(this, name, null);
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null) {
                child = super.isChildSupported(name).createChild(this, name);
            }
        }

        if (child == null) {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /** initialize table header and radio button */
    protected void initializeTableHeaders() {
        CCActionTableModel model = getTableModel(VSN_TABLE);

        // set the column headers
        model.setActionValue("IDColumn", "ImportVSN.table.heading.volumeid");
        model.setActionValue("ACSColumn", "ImportVSN.table.heading.acs");
        model.setActionValue("LSMColumn", "ImportVSN.table.heading.lsm");
        model.setActionValue("PanelColumn", "ImportVSN.table.heading.panel");

        model.setActionValue("RowColumn", "ImportVSN.table.heading.row");
        model.setActionValue("ColumnColumn", "ImportVSN.table.heading.column");
        model.setActionValue("PoolColumn", "ImportVSN.table.heading.pool");
        model.setActionValue("StatusColumn", "ImportVSN.table.heading.status");
        model.setActionValue("ServerColumn", "ImportVSN.table.heading.server");
        model.setActionValue("MediaColumn", "ImportVSN.table.heading.media");
        model.setActionValue("TypeColumn", "ImportVSN.table.heading.type");
    }

    /** populate the criteria table model */
    public void populateTableModel(String serverName) throws SamFSException {

        String filterCriteria =
            (String) getParentViewBean().getPageSessionAttribute(
                ImportVSNViewBean.PSA_FILTER_CRITERIA);

        String libShareAcsServer =
            (String) getParentViewBean().getPageSessionAttribute(
                ImportVSNViewBean.PSA_SAME_ACSLS_LIB);

        // Retrieve the handle of the Server Selection Table
        CCActionTableModel tableModel = getTableModel(VSN_TABLE);

        CCActionTable myTable = (CCActionTable) getChild(VSN_TABLE);

        tableModel.clear();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            Library myLibrary =
                sysModel.getSamQFSSystemMediaManager().
                    getLibraryByName(getLibraryName());

            if (myLibrary == null) {
                throw new SamFSException(null, -2502);
            }

            StkVSN [] vsns = myLibrary.getVSNsForStkLib(filterCriteria);
            if (vsns == null) {
                return;
            }

            HashMap [] myMaps = createVSNHashMaps(libShareAcsServer);

            StringBuffer volumeIDs = new StringBuffer();

            // populate the table model

            for (int i = 0; i < vsns.length; i++) {
                if (i > 0) {
                    tableModel.appendRow();
                }

                // Disable Tooltip
                CCCheckBox myCheckBox =
                    (CCCheckBox) myTable.getChild(
                         CCActionTable.CHILD_SELECTION_CHECKBOX + i);
                myCheckBox.setTitle("");
                myCheckBox.setTitleDisabled("");

                // Save Volume ID for preSubmitHandler javascript
                if (volumeIDs.length() > 0) {
                    volumeIDs.append(",");
                }
                tableModel.setValue(
                    "IDText", vsns[i].getName());
                volumeIDs.append(vsns[i].getName());

                tableModel.setValue(
                    "ACSText", new Integer(vsns[i].getAcsNum()));
                tableModel.setValue(
                    "LSMText", new Integer(vsns[i].getLsmNum()));
                tableModel.setValue(
                    "PanelText", new Integer(vsns[i].getPanelNum()));
                tableModel.setValue(
                    "RowText", new Integer(vsns[i].getRowID()));
                tableModel.setValue(
                    "ColumnText", new Integer(vsns[i].getColID()));
                tableModel.setValue(
                    "PoolText", new Integer(vsns[i].getPoolID()));

                String status = vsns[i].getStatus();
                tableModel.setValue(
                    "StatusText", status);

                String serverList =
                    existsInOtherLibraries(vsns[i].getName(), myMaps);
                tableModel.setValue(
                    "ServerText", serverList);
                tableModel.setValue(
                    "MediaText", vsns[i].getMediaType());
                tableModel.setValue(
                    "TypeText", vsns[i].getUsageType());

                // User can only select the VSN which is not used by other
                // libraries, and it has to be in home status
                tableModel.setSelectionVisible(
                    serverList.length() == 0 && "home".equals(status));
            }

            // save volume ids in hidden field for preSubmitHandler javascript
            ((CCHiddenField) getChild(VOLUME_IDS)).
                setValue(volumeIDs.toString());

        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                this.getClass(),
                "populateTableModel",
                "unable to populate table model",
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ImportVSN.error.populate",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        initializeTableHeaders();
        TraceUtil.trace3("Exiting");
    }

    // implement the CCPagelet interface

    /**
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        String filterCriteria =
            (String) getParentViewBean().getPageSessionAttribute(
                ImportVSNViewBean.PSA_FILTER_CRITERIA);

        if (filterCriteria != null) {
            return "/jsp/media/ImportVSNPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }

    private String getLibraryName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
    }

    private HashMap [] createVSNHashMaps(String libShareAcsServer)
        throws SamFSException {

        // No library is sharing the same ACS Server
        if (libShareAcsServer.length() == 0) {
            return new HashMap[0];
        }

        String [] libraryNames = libShareAcsServer.split(",");
        HashMap [] myMaps = new HashMap[libraryNames.length];

        for (int i = 0; i < libraryNames.length; i++) {
            myMaps[i] = new HashMap();
            String [] entries = libraryNames[i].split("@");
            String libName    = entries[0];
            String serverName = entries[1];

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            Library testLibrary =
                sysModel.getSamQFSSystemMediaManager().
                    getLibraryByName(libName);
            if (testLibrary ==  null) {
                TraceUtil.trace1(
                    "Error encountered while creating VSN HashMap! " +
                    "ServerName: " + serverName + " LibName: " + libName);
                myMaps[i].put(serverName, null);
                continue;
            }

            myMaps[i].put(serverName, testLibrary.getVSNNamesForStkLib());
        }

        return myMaps;
    }

    /**
     * See if the vsn is shared among other libraries
     */
    private String existsInOtherLibraries(String vsnName, HashMap [] myMaps) {
        for (int i = 0; i < myMaps.length; i++) {
            Iterator it = myMaps[i].keySet().iterator();
            while (it.hasNext()) {
                String key	 = (String) it.next();
                HashMap vsnMap   = (HashMap) myMaps[i].get(key);
                if (vsnMap.containsKey(vsnName)) {
                    return key;
                }
            }
        }

        return "";
    }

    public void handleImportButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String selectedVolumeIDs =
            (String) getDisplayFieldValue(SELECTED_VOLUME_IDS);
        TraceUtil.trace2("Import: SelectedVolumeIDs are " + selectedVolumeIDs);

        String serverName =
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            Library myLibrary =
                sysModel.getSamQFSSystemMediaManager().
                    getLibraryByName(getLibraryName());

            if (myLibrary == null) {
                throw new SamFSException(null, -2502);
            }

            LogUtil.info(
                this.getClass(),
                "handleImportButtonRequest",
                new StringBuffer().append(
                    "Start importing VSNs ").append(selectedVolumeIDs).
                    toString());

            myLibrary.importVSNInACSLS(selectedVolumeIDs.split(", "));

            LogUtil.info(
                this.getClass(),
                "handleImportButtonRequest",
                new StringBuffer().append(
                    "Done importing VSNs ").append(selectedVolumeIDs).
                    toString());

            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "ImportVSN.success.importvsn", selectedVolumeIDs),
                serverName);

            // Clear PSA Filter Criteria
            getParentViewBean().removePageSessionAttribute(
                ImportVSNViewBean.PSA_FILTER_CRITERIA);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleImportButtonRequest",
                "Failed to import VSN",
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ImportVSN.error.importvsn",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
            TraceUtil.trace3("Exiting.  Fail to import VSN!");
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }
}
