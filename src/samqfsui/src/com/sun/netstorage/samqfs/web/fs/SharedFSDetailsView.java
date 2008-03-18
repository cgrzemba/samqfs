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

// ident	$Id: SharedFSDetailsView.java,v 1.21 2008/03/17 14:43:35 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
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
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the container view of File Systems Summary page
 */

public class SharedFSDetailsView extends CommonTableContainerView {

    public static final String CHILD_ACTIONMENU_HREF = "ActionMenuHref";

    private SharedFSDetailsModel model = null;

    public static final String CHILD_TILED_VIEW = "SharedFSDetailsTiledView";
    public static final String ALL_MOUNT_HIDDEN_FIELD = "HiddenAllMount";

    public static final
        String ALL_CLIENT_MOUNT_HIDDEN_FIELD = "HiddenAllClientMount";
    public static final String MDS_MOUNTED = "MDSMounted";
    public static final String HOST_NAME_HIDDEN_FIELD = "HiddenHostName";
    public static final String FS_NAME_HIDDEN_FIELD = "HiddenFsName";

    private String fsName = null;

    // used by view bean as well
    public String partialErrMsg;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public SharedFSDetailsView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "SharedFSDetailsTable";

        fsName = (String)
            getParentViewBean().
            getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        if (fsName == null) {
            fsName = (String) getParentViewBean().getPageSessionAttribute(
                Constants.SessionAttributes.POLICY_NAME);
        }

        // If shared FS on CIS configuration, user should not be allowed to
        // Add/Delete/do advanced config of shared hosts
        model = new SharedFSDetailsModel(
                "/jsp/fs/SharedFSDetailsTable.xml", getServerName(), fsName);

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_ACTIONMENU_HREF, CCHref.class);
        registerChild(ALL_MOUNT_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(ALL_CLIENT_MOUNT_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(MDS_MOUNTED, CCHiddenField.class);
        registerChild(HOST_NAME_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(FS_NAME_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_TILED_VIEW, SharedFSDetailsTiledView.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create the child
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (name.equals(CHILD_ACTIONMENU_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(ALL_MOUNT_HIDDEN_FIELD) ||
                   name.equals(ALL_CLIENT_MOUNT_HIDDEN_FIELD) ||
                   name.equals(MDS_MOUNTED) ||
                   name.equals(HOST_NAME_HIDDEN_FIELD) ||
                   name.equals(FS_NAME_HIDDEN_FIELD)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            SharedFSDetailsTiledView child =
                new SharedFSDetailsTiledView(this, model, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else {
            TraceUtil.trace3("Exiting");
            return super.createChild(model, name, "SharedFSDetailsTiledView");
        }
    }

    /**
     * Handle request for action drop-down menu
     *  @param RequestInvocationEvent event
     */
    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String op = null;
        Class target = SharedFSDetailsViewBean.class;
        int index = -1;
        String value = (String) getDisplayFieldValue("ActionMenu");

        try {
            index = getSelectedRowIndex();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "getSelectedRowIndex()",
                "Exception occurred within framework",
                getServerName());
            throw ex;
        }

        // get drop down menu selected option
        int option = 0;
        try {
            option = Integer.parseInt(value);
        } catch (NumberFormatException nfe) {
            // should not get here!!!
            TraceUtil.trace1(
                "NumberFormatException caught in handleActionMenuHrefRequest!");
            TraceUtil.trace1("Reason: " + nfe.getMessage());
        }

        String clientHost = getSelectedValue(index);
        String clientHostType = getSelectedFSType(index);

        if (option != 0) {
            try {
                // Using hostName to get model.
                SamQFSSystemModel model = SamUtil.getModel(clientHost);

                FileSystem fileSystem =
                    model.getSamQFSSystemFSManager().getFileSystem(fsName);
                if (fileSystem == null) {
                    throw new SamFSException(null, -1000);
                }
                switch (option) {
                    case 1: // edit mount options

                        ViewBean targetView = getMountViewBean(fileSystem,
                            clientHost, clientHostType);
                        ViewBean vb = getParentViewBean();
                        BreadCrumbUtil.breadCrumbPathForward(
                            vb,
                            PageInfo.getPageInfo().getPageNumber(vb.getName()));
                        ((CommonViewBeanBase) vb).forwardTo(targetView);
                        return;
                    case 2: // mount fs
                        op = "FSDetails.mountfs";
                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            "Start mounting filesystem " + fsName);
                        fileSystem.mount();
                        if (fileSystem.getState() == FileSystem.UNMOUNTED) {
                            throw new SamFSException(
                                "SharedFSDetails.warning.mount.cause", -1099);
                        } else {
                            LogUtil.info(
                                this.getClass(),
                                "handleActionMenuHrefRequest",
                                "Done mounting filesystem " + fsName);
                        }
                        break;
                    case 3: // unmount fs
                        TraceUtil.trace3("Entering umount");
                        op = "FSDetails.umountfs";
                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            "Start unmounting filesystem " + fsName);
                        fileSystem.unmount();
                        TraceUtil.trace3("exiting umount");
                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            "Done unmounting filesystem " + fsName);
                        break;
                    default:
                        // should never get here!!!
                        throw new SamFSException(null, -1000);
                }
                showAlert(op, fsName);
                target = SharedFSDetailsViewBean.class;
            } catch (SamFSException smfex) {
                // need to see if a SamFSMultiMsgException occurred
                // when attempting to delete a filesystem
                String processMsg = null;
                String errCause = null;
                String summary = null;
                boolean warning = false;

                if (smfex instanceof SamFSMultiMsgException) {
                    processMsg = Constants.Config.ARCHIVE_CONFIG;
                    summary = "ArchiveConfig.error";
                    errCause = "ArchiveConfig.error.detail";
                } else if (smfex instanceof SamFSWarnings) {
                    warning = true;
                    processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                    summary = "ArchiveConfig.error";
                    errCause = "ArchiveConfig.warning.detail";
                } else {
                    switch (option) {
                        case 1: // edit mount options
                            summary = "SharedFSDetails.error.mountoption";
                            processMsg = "Failed to run edit mount options";
                            break;
                        case 2: // mount fs
                            summary = "SharedFSDetails.error.mount";
                            processMsg = "Failed to mount filesystem";
                            break;
                        case 3: // unmount fs
                            summary = "SharedFSDetails.error.umount";
                            processMsg = "Failed to unmount filesystem";
                            break;
                        default:
                            // should never get here!!!
                    }
                    errCause = smfex.getMessage();
                }

                SamUtil.processException(
                    smfex,
                    this.getClass(),
                    "handleActionMenuHrefRequest()",
                    processMsg,
                    clientHost);
                if (!warning) {
                    SamUtil.setErrorAlert(
                        getParentViewBean(),
                        CommonViewBeanBase.CHILD_COMMON_ALERT,
                        summary,
                        smfex.getSAMerrno(),
                        errCause,
                        clientHost);
                } else {
                    SamUtil.setWarningAlert(
                        getParentViewBean(),
                        CommonViewBeanBase.CHILD_COMMON_ALERT,
                        summary,
                        errCause);
                }
                target = SharedFSDetailsViewBean.class;
            }
        } else if (option == 6) {
            // samfsck
            // should never get here!!!
        }
        ViewBean targetView = getViewBean(target);
        targetView.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    // The followng function handles mount issue.
    private ViewBean getMountViewBean(FileSystem fs,
        String clientHost, String clientHostType)
        throws SamFSException {
        ViewBean targetView = null;
        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        int fsType = fs.getFSTypeByProduct();
        int shareStatus = fs.getShareStatus();
        String mountPageType = null;

        // Based on the filesystem type, the mount page type
        // is determined.  These page types determines which kinds
        // of jsp file should be displayed.

        switch (fs.getShareStatus()) {
            case FileSystem.UNSHARED:
                switch (fsType) {
                    case FileSystem.FS_SAMQFS:
                        mountPageType = FSMountViewBean.TYPE_UNSHAREDSAMQFS;
                        break;
                    case FileSystem.FS_QFS:
                        mountPageType = FSMountViewBean.TYPE_UNSHAREDQFS;
                        break;
                    default:
                        mountPageType = FSMountViewBean.TYPE_UNSHAREDSAMFS;
                        break;
                }
                break;

            case FileSystem.SHARED_TYPE_MDS:
            case FileSystem.SHARED_TYPE_PMDS:
                mountPageType = (fsType == FileSystem.FS_SAMQFS) ?
                    FSMountViewBean.TYPE_SHAREDSAMQFS :
                    FSMountViewBean.TYPE_SHAREDQFS;
                break;

            // should not get into this case
            default:
                mountPageType = (fsType == FileSystem.FS_SAMQFS) ?
                    FSMountViewBean.TYPE_SHAREDSAMQFS :
                    FSMountViewBean.TYPE_SHAREDQFS;
                break;
        }

        targetView = getViewBean(FSMountViewBean.class);
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME, fs.getName());

        // Setup host's type and host name, so that mount option
        // page can know which host and what kinds of type should be editted.
        targetView.setPageSessionAttribute(
            Constants.SessionAttributes.SHARED_METADATA_SERVER,
            getServerName());
        targetView.setPageSessionAttribute(
            Constants.SessionAttributes.SHARED_METADATA_CLIENT, clientHost);
        targetView.setPageSessionAttribute(
            Constants.SessionAttributes.SHARED_CLIENT_HOST, clientHostType);
        targetView.setPageSessionAttribute(
            Constants.SessionAttributes.MOUNT_PAGE_TYPE, mountPageType);
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, getServerName());
        return targetView;
    }

    /**
     * Handler function for delete shared client,
     * potential metadata server, or metadata server  button
     * @param RequestInvocationEvent event
     */

    public void handleDeleteButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        int index = -1;
        int errorFlag = 0;
        try {
            index = getSelectedRowIndex();
        } catch (ModelControlException ex) {
            // forward to error handler page
            SamUtil.processException(
                ex,
                this.getClass(),
                "getSelectedRowIndex()",
                "Exception occurred within framework",
                getServerName());
            throw ex;
        }

        String selected = getSelectedValue(index); // host name
        String fs = fsName; // fs name
        int sharedType = 0;
        String mdServer = null;
        try {
            mdServer = (String) getParentViewBean().
                getPageSessionAttribute(
                Constants.SessionAttributes.SHARED_MD_SERVER);
            TraceUtil.trace3("md_server =" + mdServer);
            TraceUtil.trace3("delete_host =" + selected);
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            sharedType =
                fsManager.getSharedFSType(selected, fsName);
            // selected is host name
            fsManager.deleteSharedFileSystem(selected, fs, mdServer);
        } catch (SamFSMultiHostException e) {
            SamUtil.doPrint(new StringBuffer().
                append("error code is ").
                append(e.getSAMerrno()).toString());
            String err_msg = SamUtil.handleMultiHostException(e);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "SharedFSDetails.error.delete",
                e.getSAMerrno(),
                err_msg,
                mdServer);
            errorFlag = 1;

        } catch (SamFSException ex) {
            SamUtil.processException(ex,
                this.getClass(),
                "handleDeleteButtonRequest",
                "Failed to remove shared filesystem",
                selected);
            if (ex.getSAMerrno() != ex.NOT_FOUND) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "SharedFSDetails.error.delete",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    mdServer);
                    errorFlag = 1;
            }
        }


        // Try to get that file system to check if it is deleted or not
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(selected);
            FileSystem fileSystem =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fs);
            if (fileSystem != null) {
                // if there is no error occurred in the calls above,
                // sleep for 5 seconds, give time for underlying call
                // to be completed
                try {
                    Thread.sleep(5000);
                } catch (InterruptedException intEx) {
                    // impossible for other thread to interrupt this thread
                    // Continue to load the page
                    TraceUtil.trace3("InterruptedException Caught: Reason: " +
                       intEx.getMessage());
                }
           }
        } catch (SamFSException ex) {
           if (ex.getSAMerrno() != ex.NOT_FOUND) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "SharedFSDetails.error.delete",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    selected);
            }
        }



        // If there is no error, display success alert.
        if (errorFlag == 0) {
            setSuccessAlert("SharedFSDetails.action.delete", selected);
        }

        /* Get the serverhostname for comparison too. */
        String uname = null; // the target server's serverHostName
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            uname = appModel.getSamQFSSystemModel(getServerName()).
                getServerHostname();
        } catch (Exception e) {
            uname = getServerName();
        }

        // If there is no error, forward this page to file summary page.
        // Function forwardToTargetPage() defines this forward.
        // if (sharedType == SharedMember.TYPE_MD_SERVER && errorFlag == 0) {
        if (sharedType == SharedMember.TYPE_POTENTIAL_MD_SERVER) {
            TraceUtil.trace3("potential type");
        }
        TraceUtil.trace3(" md server " + mdServer);
        TraceUtil.trace3(" selected " + selected);
        TraceUtil.trace3(" hostname " + getServerName());
        int index1 = getServerName().indexOf(".");
        String cmpHost = null;
        if (index1 == -1) {
            cmpHost = getServerName();
        } else {
            cmpHost = getServerName().substring(0, index1);
        }
        TraceUtil.trace3("cmp hostname " + cmpHost);

        if ((mdServer.equals(selected) ||
            (cmpHost.equalsIgnoreCase(selected) && sharedType ==
            SharedMember.TYPE_POTENTIAL_MD_SERVER)) && errorFlag == 0) {
                forwardToTargetPage(true);
        } else {
            getParentViewBean().forwardTo(getRequestContext());
        }
        TraceUtil.trace3("Existing");

    }


    /**
     * Handle request for ViewDeviceButton
     */
    public void handleViewDeviceButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        ViewBean targetView = getViewBean(FSDevicesViewBean.class);
        ViewBean vb = getParentViewBean();
        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));
        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for Advanced Network Configuration Button
     */
    public void handleAdvancedButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        ViewBean targetView = getViewBean(AdvancedNetworkConfigViewBean.class);
        ViewBean vb = getParentViewBean();
        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));
        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }


    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // set the drop-down menu to default value
        CCDropDownMenu actionDropDown = (CCDropDownMenu) getChild("ActionMenu");
        actionDropDown.setValue("0");

        // get the server name translate from the server page
        model.setTitle(
            SamUtil.getResourceString("SharedFSDetails.pageTitle1", fsName));
        model.setRowSelected(false);

        // Disable Add Button if user does not either have CONFIG or
        // FILESYSTEM_OPERATOR authorizations
        if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
            ((CCButton) getChild("AddButton")).setDisabled(false);

            actionDropDown.setDisabled(false);

            ((CCButton) getChild("AdvancedButton")).setDisabled(false);
        } else {
            model.setSelectionType("none");
        }

        try {
            SharedFSDetailsData recordModel =
                new SharedFSDetailsData(getServerName(), fsName);
            boolean allMounted = true;
            boolean allClientNotMounted = true;
            boolean allClientMounted = true;
            boolean mdMounted = false;

            for (int i = 0; i < recordModel.size(); i++) {
                Object[] record = (Object[]) recordModel.get(i);

                boolean isMounted = ((Boolean) record[2]).booleanValue();
                int type = ((Integer) record[1]).intValue();

                if (isMounted) {
                    allMounted = false;
                }
                if (isMounted && type != SharedMember.TYPE_MD_SERVER) {
                    allClientNotMounted = false;
                }
                if (!isMounted && type != SharedMember.TYPE_MD_SERVER) {
                    allClientMounted = false;
                }
                if (isMounted && type == SharedMember.TYPE_MD_SERVER) {
                    mdMounted = true;
                }
            }

            CCHiddenField hostNameHiddenField = (CCHiddenField)
                getChild("HiddenHostName");
            hostNameHiddenField.setValue(getServerName());

            CCHiddenField fsNameHiddenField = (CCHiddenField)
                getChild("HiddenFsName");
            fsNameHiddenField.setValue(fsName);

            ((CCHiddenField) getChild(ALL_MOUNT_HIDDEN_FIELD)).
                setValue(Boolean.toString(allMounted));

            CCHiddenField allClientMountHiddenField =
                (CCHiddenField) getChild("HiddenAllClientMount");
            if (allClientNotMounted == true) {
                 allClientMountHiddenField.setValue("allClientUnMounted");
            } else {
                allClientMountHiddenField.setValue("notAllClientUnMounted");
            }

            // Set hidden field to indicate if file system is mounted in
            // the MDS Server
            ((CCHiddenField) getChild(MDS_MOUNTED)).
                setValue(Boolean.toString(mdMounted));
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay()",
                "Unable to populate shared file system details.",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSSummary.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());
        }

        // Disable Tooltip
        CCActionTable myTable =
            (CCActionTable) getChild("SharedFSDetailsTable");
        CCRadioButton myRadio = (CCRadioButton) myTable.getChild(
            CCActionTable.CHILD_SELECTION_RADIOBUTTON);
        myRadio.setTitle("");
        myRadio.setTitleDisabled("");

        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to setup the inline alert
     */
    private void showAlert(String operation, String key) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(operation, key),
            getServerName());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to get FileSystem
     */
    private FileSystem getFileSystem(String fsName) throws SamFSException {
        return SamUtil.getModel(getServerName()).
            getSamQFSSystemFSManager().getFileSystem(fsName);
    }

    /**
     * Function to get the selected row index
     */
    private int getSelectedRowIndex() throws ModelControlException {
        TraceUtil.trace3("Entering");
        int index = -1;
        CCActionTable child = (CCActionTable)getChild("SharedFSDetailsTable");
        child.restoreStateData();
        model.beforeFirst();
        while (model.next()) {
            if (model.isRowSelected()) {
                index = model.getRowIndex();
            }
        }
        TraceUtil.trace3("Exiting");
        return index;
    }

    private String getSelectedValue(int index) {
        TraceUtil.trace3("Entering");
        String value = null;
        ViewBean vb = getParentViewBean();
            model.setRowIndex(index);
            value = (String) model.getValue("FSHiddenField");
        TraceUtil.trace3("Exiting");
        return value;
    }


    private String getSelectedFSType(int index) {
        TraceUtil.trace3("Entering");
        String value = null;
        ViewBean vb = getParentViewBean();
            model.setRowIndex(index);
            value = (String) model.getValue("HiddenType");
        TraceUtil.trace3("Exiting");
        return value;
    }

    public void handleCancelHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }


    // populate the Actiontable's data
    public void populateData() throws SamFSException, SamFSMultiHostException {
        TraceUtil.trace3("Entering");
        model.initModelRows();
        partialErrMsg = model.partialErrMsg;
        String sharedMDServer = model.sharedMDServer;
        ViewBean vb = getParentViewBean();
        vb.setPageSessionAttribute(
            Constants.SessionAttributes.SHARED_MD_SERVER,
            sharedMDServer);
        TraceUtil.trace3("md_server = " + sharedMDServer);
        TraceUtil.trace3("Exiting");
    }

    private void setSuccessAlert(String msg, String item) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            getServerName());
        TraceUtil.trace3("Exiting");
    }


    private void forwardToTargetPage(boolean showAlert) {
        TraceUtil.trace3("Entering");

        ViewBean targetView = null;
        String s = null;

        Integer[] temp = (Integer [])
            getParentViewBean().getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        Integer[] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);

        int index = path[path.length-1].intValue();

        PageInfo pageInfo = PageInfo.getPageInfo();
        String targetName = pageInfo.getPagePath(index).getCommandField();

        TraceUtil.trace3("targetname = " + targetName);
        if (targetName.equals("FileSystemSummaryHref")) {
            targetView = getViewBean(FSSummaryViewBean.class);
            s = Integer.toString(
                BreadCrumbUtil.inPagePath(path, index, path.length-1));
            TraceUtil.trace3("FSSummary target ");
        }

        if (showAlert) {
            showAlert(targetView);
        }


        ViewBean vb = getParentViewBean();
        BreadCrumbUtil.breadCrumbPathBackward(
            vb,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        ((CommonViewBeanBase) vb).forwardTo(targetView);

        TraceUtil.trace3("exiting");
    }

    private void showAlert(ViewBean targetView) {
        TraceUtil.trace3("Entering");
        String temp = SamUtil.getResourceString(
            "SharedFSDetails.delete.success", fsName);
        TraceUtil.trace3("msg is " + temp);
        SamUtil.setInfoAlert(
            targetView,
            "Alert",
            "success.summary",
            temp,
            getServerName());
        TraceUtil.trace3("Exiting");
    }

    private String getServerName() {
        return (String) getParentViewBean().
            getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

} // end of SharedFSDetailsView class
