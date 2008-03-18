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

// ident	$Id: RecyclerViewBean.java,v 1.13 2008/03/17 14:40:44 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.RecycleParams;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Admin Setup page
 */
public class RecyclerViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "Recycler";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/archive/Recycler.jsp";

    public static final String CHILD_RECYCLERTILED_VIEW =
        "RecyclerTableTiledView";

    public static final String CHILD_RECYCLER_TABLE = "recyclerTable";
    public static final String CHILD_LIB_SIZE = "libNumber";

    // For javascript error strings
    public static final String CHILD_HIDDEN_MESSAGES = "HiddenMessages";

    private static CCPageTitleModel pageTitleModel = null;
    private static CCPropertySheetModel propertySheetModel  = null;
    private RecyclerTableModel recyclerTableModel;

    /**
     * Constructor
     */
    public RecyclerViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        createPageTitleModel();
        createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();

        registerChild(CHILD_RECYCLER_TABLE, CCActionTable.class);
        registerChild(CHILD_RECYCLERTILED_VIEW, RecyclerTableTiledView.class);
        registerChild(CHILD_HIDDEN_MESSAGES, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(CHILD_RECYCLERTILED_VIEW)) {
            child = new RecyclerTableTiledView(this, recyclerTableModel, name);
        } else if (name.equals(CHILD_RECYCLER_TABLE)) {
            CCActionTable myChild = (CCActionTable) new CCActionTable(
                this, recyclerTableModel, name);
            myChild.setTiledView((ContainerView)
                getChild(CHILD_RECYCLERTILED_VIEW));
            propertySheetModel.setModel(
                "recyclerTable", recyclerTableModel);
            child = myChild;
        } else if (name.equals(CHILD_LIB_SIZE)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_MESSAGES)) {
            child = new CCHiddenField(this, name, getMessageString());
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild
                (this, propertySheetModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        loadPropertySheetModel();
        loadAllTables();

        // disbale save & reset buttons if no sam control
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.SAM_CONTROL)) {

            ((CCButton)getChild("SavePageButton")).setDisabled(true);
            ((CCButton)getChild("ResetPageButton")).setDisabled(true);
        } else {
            ((CCButton)getChild("SavePageButton")).setDisabled(false);
        }

        ((CCHiddenField) getChild(CHILD_LIB_SIZE)).setValue(
            Integer.toString(recyclerTableModel.getNumRows()));
        TraceUtil.trace3("Exiting");
    }

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/archive/RecyclerPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    private void createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null) {
            propertySheetModel = PropertySheetUtil.createModel(
                "jsp/archive/RecyclerPropSheet.xml");
        }

        if (recyclerTableModel == null) {
            recyclerTableModel = new RecyclerTableModel();
        }

        TraceUtil.trace3("Exiting");
    }

    private void loadPropertySheetModel() {
        TraceUtil.trace3("Entering");

        try {
            SamQFSSystemArchiveManager archiveManager =
                SamUtil.getModel(getServerName()).
                    getSamQFSSystemArchiveManager();
            propertySheetModel.setValue(
                "recyclerlogValue",
                archiveManager.getRecyclerLogFile());

            if (archiveManager.getPostRecycle() ==
                SamQFSSystemArchiveManager.RECYCLE_RELABEL) {
                propertySheetModel.setValue("postrecyclerValue", "relabel");
            } else if (archiveManager.getPostRecycle() ==
                SamQFSSystemArchiveManager.RECYCLE_EXPORT) {
                propertySheetModel.setValue("postrecyclerValue", "export");
            }
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "loadPropertySheetModel",
                "Failed to retrieve model",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "AdminSetup.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to setup the inline alert
     */
    private void showAlert() {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            this,
            CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString("AdminSetup.save"),
            getServerName());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for Button 'Save'
     */
    public void handleSavePageButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            SamQFSSystemArchiveManager archiveManager =
                sysModel.getSamQFSSystemArchiveManager();
            LogUtil.info(this.getClass(),
                "handleSavePageButtonRequest",
                "Start saving the configuration");

            saveRecycler(sysModel);
            String recyclerLog =
                ((String) getDisplayFieldValue("recyclerlogValue")).trim();
            if (recyclerLog.equals("") &&
                !archiveManager.getRecyclerLogFile().equals("")) {
                archiveManager.setRecyclerLogFile("");
            } else {
                if (!recyclerLog.equals(
                    archiveManager.getRecyclerLogFile())) {
                    archiveManager.setRecyclerLogFile(recyclerLog);
                } else if (!sysModel.doesFileExist(recyclerLog)) {
                    TraceUtil.trace3("recycler file doesn't exist");
                    archiveManager.setRecyclerLogFile(recyclerLog);
                }
            }

            String postRecycler =
                (String) getDisplayFieldValue("postrecyclerValue");
            if (postRecycler != null) {
                if (postRecycler.equals("relabel")) {
                    if (archiveManager.getPostRecycle() !=
                            SamQFSSystemArchiveManager.RECYCLE_RELABEL) {
                        archiveManager.setPostRecycle(
                            SamQFSSystemArchiveManager.RECYCLE_RELABEL);
                    }
                } else if (postRecycler.equals("export")) {
                    if (archiveManager.getPostRecycle() !=
                            SamQFSSystemArchiveManager.RECYCLE_EXPORT) {
                        archiveManager.setPostRecycle(
                            SamQFSSystemArchiveManager.RECYCLE_EXPORT);
                    }
                }
            }

            LogUtil.info(this.getClass(),
                "handleSavePageButtonRequest",
                "Done saving the configuration");
            showAlert();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSavePageButtonRequest",
                "Failed to save configuration",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "AdminSetup.error.save",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }
        this.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleCancelPageButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        this.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void saveRecycler(SamQFSSystemModel sysModel)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        RecycleParams[] recyclers = sysModel.getSamQFSSystemArchiveManager().
            getRecycleParams();
        int size = recyclers.length;
        int tableSize = recyclerTableModel.getNumRows();

        for (int i = 0; i < tableSize && size > 0; i++) {
            recyclerTableModel.setRowIndex(i);
            String libName =
                (String) recyclerTableModel.getValue("LibNameHiddenField");
            int hwm = -1, gain = -1, vsn = -1, unit = -1;
            long sizeLimit = -1;

            String hwmString = (String) recyclerTableModel.getValue("hwm");
            String gainString =
                (String) recyclerTableModel.getValue("minigain");
            String vsnString = (String) recyclerTableModel.getValue("vsnlimit");
            String sizelimitString =
                (String) recyclerTableModel.getValue("sizelimit");
            String sizeUnit =
                (String) recyclerTableModel.getValue("sizeunit");

            if (hwmString != null) {
                if (!hwmString.equals("")) {
                    try {
                        hwm = Integer.parseInt(hwmString.trim());
                    } catch (NumberFormatException nfex) {
                        SamUtil.doPrint(
                            "NumberFormatException caught (hwmString)");
                        hwm = -1;
                    }
                }
            }

            if (gainString != null) {
                if (!gainString.equals("")) {
                    try {
                        gain = Integer.parseInt(gainString.trim());
                    } catch (NumberFormatException nfex) {
                        SamUtil.doPrint(
                            "NumberFormatException caught (gainString)");
                        gain = -1;
                    }
                }
            }

            if (vsnString != null) {
                if (!vsnString.equals("")) {
                    try {
                        vsn = Integer.parseInt(vsnString.trim());
                    } catch (NumberFormatException nfex) {
                        SamUtil.doPrint(
                            "NumberFormatException caught (vsnString)");
                        vsn = -1;
                    }
                }
            }

            if (sizelimitString != null) {
                if (!sizelimitString.equals("")) {
                    try {
                        sizeLimit = Long.parseLong(sizelimitString.trim());
                    } catch (NumberFormatException nfex) {
                        SamUtil.doPrint(
                            "NumberFormatException caught (sizelimitString)");
                        sizeLimit = -1;
                    }
                    }
            }

            if (!sizeUnit.equals("dash")) {
                unit = SamUtil.getSizeUnit(sizeUnit);
            }

            String perform = (String) recyclerTableModel.getValue("report");
            RecycleParams recycler = null;

            for (int j = 0; j < size; j++) {
                String lname = recyclers[j].getLibraryName();
                if (lname.equals(libName)) {
                    recycler = recyclers[j];
                }
            }
            if (hwm != recycler.getHWM()) {
                recycler.setHWM(hwm);
            }

            if (gain != recycler.getMinGain()) {
                recycler.setMinGain(gain);
            }

            if (vsn != recycler.getVSNLimit()) {
                recycler.setVSNLimit(vsn);
            }

            if (sizeLimit != recycler.getSizeLimit()) {
                recycler.setSizeLimit(sizeLimit);
            }

            if (unit != recycler.getSizeUnit()) {
                recycler.setSizeUnit(unit);
            }

            if (perform.equals("true") && !recycler.isPerform()) {
                recycler.setPerform(true);
            } else if (perform.equals("false") && recycler.isPerform()) {
                recycler.setPerform(false);
            }
            sysModel.getSamQFSSystemArchiveManager().
                changeRecycleParams(recycler);
        }
        TraceUtil.trace3("Exiting");
    }

    private void loadAllTables() {
        // Clear all tables
        recyclerTableModel.clear();

        try {
            recyclerTableModel.initModelRows(getServerName());
            setPageSessionAttribute(
                Constants.PageSessionAttributes.RECYCLER_NUMBER,
                Integer.toString(recyclerTableModel.getNumRows()));
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "createPropertySheetModel",
                "Failed to populate recycler table",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "AdminSetup.error.recyclertable",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());
           setPageSessionAttribute(
                Constants.PageSessionAttributes.RECYCLER_NUMBER,
                "0");
        }
    }

    private String getMessageString() {
        return new NonSyncStringBuffer(
            SamUtil.getResourceString(
                "Recycler.error.hwmempty")).
            append("###").append(
            SamUtil.getResourceString(
                "Recycler.error.hwmrange")).
            append("###").append(
            SamUtil.getResourceString(
                "Recycler.error.gainempty")).
            append("###").append(
            SamUtil.getResourceString(
                "Recycler.error.gainrange")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.vsnempty")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.vsnrange")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.sizeempty")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.sizerange")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.recycler")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.recyclerspace")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.sizeunitdash")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.sizeunitnodash")).
            append("###").append(
            SamUtil.getResourceString(
                "AdminSetup.error.hwmgain")).toString();
    }
}
