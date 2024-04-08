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

// ident    $Id: NFSShareDisplayView.java,v 1.9 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.archive.MultiTableViewBase;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.NFSOptions;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;

import java.io.IOException;
import java.util.Map;

import javax.servlet.ServletException;

public class NFSShareDisplayView extends MultiTableViewBase {

    // child name for tiled view class
    public static final
        String CHILD_TILED_VIEW = "NFSShareDisplayTiledView";

    public static final String DISPLAY_TABLE = "NFSShareDisplayTable";
    public static final String ACTIONMENU_HREF  = "ActionMenuHref";

    // Keep track of the state of this view
    // value={0|1|2} == none|add|edit
    public static final String FLAG = "flag";

    private String serverName;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NFSShareDisplayView(
        View parent, Map models, String pageName, String serverName) {
        super(parent, models, pageName);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.serverName = serverName;

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        registerChild(
            CHILD_TILED_VIEW, NFSShareDisplayTiledView.class);
        registerChild(ACTIONMENU_HREF, CCHref.class);

        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append(
            "Entering: name is ").append(name).toString());
        View child = null;
        if (name.equals(CHILD_TILED_VIEW)) {
            child = new NFSShareDisplayTiledView(
                this, getTableModel(DISPLAY_TABLE), name);
        } else if (name.equals(DISPLAY_TABLE)) {
            child = createTable(name, CHILD_TILED_VIEW);

        } else if (name.equals(ACTIONMENU_HREF)) {
            child = new CCHref(this, name, null);
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


    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // populate table headers
        initializeTableHeaders();

        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            // disable the radio button row selection column
            getTableModel(DISPLAY_TABLE).setSelectionType("none");
            ((CCDropDownMenu) getChild("ActionMenu")).setDisabled(true);
            ((CCButton) getChild("AddButton")).setDisabled(true);
        } else {
            // Disable Tooltip
            CCActionTable myTable = (CCActionTable) getChild(DISPLAY_TABLE);
            CCRadioButton myRadio = (CCRadioButton) myTable.getChild(
                CCActionTable.CHILD_SELECTION_RADIOBUTTON);
            myRadio.setTitle("");
            myRadio.setTitleDisabled("");
        }

        TraceUtil.trace3("Exiting");
    }

    private void setSuccessAlert(String msg, String item) {
        TraceUtil.trace3("Entering");

        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            serverName);

        TraceUtil.trace3("Exiting");
    }

    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");
        CCActionTableModel model = getTableModel(DISPLAY_TABLE);
        model.setRowSelected(false);

        model.setActionValue(
            "DirNameColumn",
            "filesystem.nfs.label.dirName");
        model.setActionValue(
            "StatusColumn",
            "filesystem.nfs.label.status");
        TraceUtil.trace3("Exiting");
    }

    public void populateTableModels() throws SamFSException {
        TraceUtil.trace3("Entering");
        // Retrieve the handle of the NFS Shares Table
        CCActionTableModel tableModel = getTableModel(DISPLAY_TABLE);
        tableModel.clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        populateNFSTable(sysModel, tableModel);
        TraceUtil.trace3("Exiting");
    }

    private void populateNFSTable(
        SamQFSSystemModel sysModel, CCActionTableModel model)
            throws SamFSException {

        StringBuffer sharedStatus = new StringBuffer();
        StringBuffer bufDirNames  = new StringBuffer();

        NFSOptions[] nfsOpts = sysModel.getNFSOptions();

        for (int i = 0; i < nfsOpts.length; i++) {
            if (i > 0) {
                model.appendRow();
            }
            model.setValue("DirNameText", nfsOpts[i].getDirName());
            model.setValue(
                "StatusImage",
                nfsOpts[i].isShared() ?
                    Constants.Image.ICON_AVAILABLE :
                    Constants.Image.ICON_BLANK);

            if (sharedStatus.length() > 0) {
                sharedStatus.append("###");
                bufDirNames.append("###");
            }
            bufDirNames.append(nfsOpts[i].getDirName());
            sharedStatus.append(Boolean.toString(nfsOpts[i].isShared()));
        }

        ((CCHiddenField) getParentViewBean().getChild(
            NFSDetailsViewBean.SHARE_STATE_LIST)).
                setValue(sharedStatus.toString());
        ((CCHiddenField) getParentViewBean().getChild(
            NFSDetailsViewBean.DIR_NAMES_LIST)).
                setValue(bufDirNames.toString());
    }

    public void handleAddButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        setFlag(1);
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleEditButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        setFlag(2);
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleRemoveButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        String dirName = getDirName();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            NFSOptions myOpts = getNFSOptions(sysModel, dirName);

            LogUtil.info(
                this.getClass(),
                "handleRemoveButtonRequest",
                new StringBuffer("Start removing NFS share of ").
                    append(dirName).toString());

            myOpts.setShareState(NFSOptions.NFS_NOTSHARED);
            sysModel.setNFSOptions(myOpts);

            LogUtil.info(
                this.getClass(),
                "handleRemoveButtonRequest",
                new StringBuffer("Done removing NFS share of ").
                    append(dirName).toString());

            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString("filesystem.nfs.msg.delete", dirName),
                serverName);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "NFSDetailsViewBean()",
                "Failed to remove NFS options for chosen directory",
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "filesystem.nfs.msg.delete.failed", dirName),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        setFlag(0);
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * This helper method retrieves the NFSOption object given the NFS shared
     * directory name.
     */
    private NFSOptions getNFSOptions(
        SamQFSSystemModel sysModel, String dirName)
        throws SamFSException {

        return sysModel.getNFSOption(dirName);
    }

    /**
     * Handle request for action drop-down menu
     *  @param RequestInvocationEvent event
     */

    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        // Operation String for the alert
        String op = null;

        // Figure out if it is share/unshare operation
        String value = (String) getDisplayFieldValue("ActionMenu");
        // get drop down menu selected option
        int option = 0;
        try {
            option = Integer.parseInt(value);
        } catch (NumberFormatException nfe) {
            // should not get here!!!
        }

        String dirName = getDirName();

        if (option != 0) {
            try {
                SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
                NFSOptions myOpts = getNFSOptions(sysModel, dirName);

                switch (option) {
                    case 1:
                        // Set Share
                        op = "filesystem.nfs.msg.share";
                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            new StringBuffer(
                                "Start setting NFS share of ").
                                append(dirName).append(" to SHARE").toString());
                        myOpts.setShareState(NFSOptions.NFS_SHARED);
                        sysModel.setNFSOptions(myOpts);

                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            new StringBuffer(
                                "Done setting NFS share of ").
                                append(dirName).append(" to SHARE").toString());
                        break;

                    case 2:
                        // Set Unshare
                        op = "filesystem.nfs.msg.unshare";
                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            new StringBuffer(
                                "Start setting NFS share of ").
                                append(dirName).append(
                                " to NOT SHARE").toString());
                        myOpts.setShareState(NFSOptions.NFS_CONFIGURED);
                        sysModel.setNFSOptions(myOpts);

                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            new StringBuffer(
                                "Done setting NFS share of ").
                                append(dirName).append(
                                " to NOT SHARE").toString());
                        break;
                }

                SamUtil.setInfoAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "success.summary",
                    SamUtil.getResourceString(op, dirName),
                    serverName);

            } catch (SamFSException smfex) {
                SamUtil.processException(
                    smfex,
                    this.getClass(),
                    "NFSDetailsViewBean()",
                    "Failed to perform the operation",
                    serverName);
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "NFSDetailsViewBean.error.failedOperation",
                    smfex.getSAMerrno(),
                    smfex.getMessage(),
                    serverName);

            }
        }

        setFlag(0);
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private int getSelectedIndex() {
        int index = -1;

        try {
            index = Integer.parseInt((String)
                ((CCHiddenField) getParentViewBean().getChild(
                    NFSDetailsViewBean.SELECTED_INDEX)).getValue());
        } catch (NumberFormatException numEx) {
            // should not reach here
            TraceUtil.trace1("NumberFormatException in getSelectedIndex!");
        }

        return index;
    }

    private String getDirName() {
        String dirNameList =
            (String) ((CCHiddenField) getParentViewBean().getChild(
                NFSDetailsViewBean.DIR_NAMES_LIST)).getValue();
        String [] dirNamesArray = dirNameList.split("###");
        return dirNamesArray[getSelectedIndex()];
    }

    private void setFlag(int flag) {
        if (flag >= 0 && flag <= 2) {
            getParentViewBean().
                setPageSessionAttribute(FLAG, new Integer(flag));
        }
    }

} // end of NFSShareDisplayView class
