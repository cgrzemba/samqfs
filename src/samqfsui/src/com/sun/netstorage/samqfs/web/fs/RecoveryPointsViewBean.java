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

// ident	$Id: RecoveryPointsViewBean.java,v 1.13 2008/05/16 18:38:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserModel;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import java.io.IOException;
import java.util.ArrayList;
import javax.servlet.ServletException;

/**
 * main container for the view
 */
public class RecoveryPointsViewBean extends CommonViewBeanBase {
    public static final String PAGE_NAME = "RecoveryPoints";

    private static final String CONTAINER_VIEW = "RecoveryPointsView";

    // Contain all the file name entries in the table for javascript
    public static final String MESSAGES = "Messages";
    public static final String HELPER = "RetainCheckBoxHelper";
    public static final String SERVER_NAME = "ServerName";

    public static final String MENU = "currentFSValue";
    public static final String MENU_HREF = "BasePathMenuHref";

    public static final String SELECT_SS_PATH = "selectSnapshotPath";
    public static final String SNAPSHOT_PATH = "currentSnapshotPathValue";
    public static final String SNAP_PATH_MENU_HREF = "SnapPathMenuHref";
    public static final
        String POST_SELECT_SS_PATH_CMD = "postSelectSnapshotPathCmd";

    /**
     * The following key contains a directory path when user defines a custom
     * directory entry from the File Chooser.  We need to remember this path
     * and add the path to the SnapMenu when the page refreshes.  The SnapMenu
     * only contains the paths that contains INDEXED snapshots.
     */
    public static final String CUSTOM_SNAPSHOT_PATH  = "customSnapShotPath";

    // Keep track of user's role
    public static final String ROLE = "Role";

    // Page Title / Property Sheet Attributes and Components.
    private CCPageTitleModel pageTitleModel  = null;
    private CCPropertySheetModel propertySheetModel = null;
    private RemoteFileChooserModel selectSnapshotPathModel = null;

    /* default constructor */
    public RecoveryPointsViewBean() {
        super(PAGE_NAME, "/jsp/fs/RecoveryPoints.jsp");

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        createPageTitleModel();
        createPropertySheetModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** register this containers child views */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(MESSAGES, CCHiddenField.class);
        registerChild(HELPER, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(CONTAINER_VIEW, RecoveryPointsView.class);

        registerChild(MENU_HREF, CCHref.class);
        registerChild(SNAP_PATH_MENU_HREF, CCHref.class);
        registerChild(SELECT_SS_PATH, RemoteFileChooserControl.class);
        registerChild(POST_SELECT_SS_PATH_CMD, BasicCommandField.class);

        registerChild(ROLE, CCHiddenField.class);

        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);

        super.registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** create a named child view */
    public View createChild(String name) {
        if (name.equals(HELPER) ||
            name.equals(MESSAGES) ||
            name.equals(ROLE)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (name.equals(MENU_HREF) ||
            name.equals(SNAP_PATH_MENU_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(CONTAINER_VIEW)) {
            return new RecoveryPointsView(this, name);
        } else if (name.equals(SELECT_SS_PATH)) {
            // Create an filechooser child.
            createSSPathFileChooserModel();
            return new RemoteFileChooserControl(
                this, selectSnapshotPathModel, name);
        } else if (name.equals(POST_SELECT_SS_PATH_CMD)) {
            return new BasicCommandField(this, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        // Property Sheet Child
        } else if (
            PropertySheetUtil.isChildSupported(propertySheetModel, name)) {
            return
                PropertySheetUtil.createChild(this, propertySheetModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private void createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
    }

    private void createPropertySheetModel() {
        if (propertySheetModel == null) {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/RecoveryPointsPropertySheet.xml");
        }
    }

    /** begin processing the JSP bound to this view bean */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        boolean hasPermission = checkRolePrivilege();

        RecoveryPointsView view =
            (RecoveryPointsView) getChild(CONTAINER_VIEW);
        view.clearTableModel();

        try {
            SamQFSSystemFSManager fsManager =
                SamUtil.getModel(getServerName()).getSamQFSSystemFSManager();
            FileSystem [] fs = fsManager.getAllFileSystems();

            populateBasePathMenu(fs);

            if (null != getCurrentFSName()) {
                String [] dirs = fsManager.getIndexDirs(getCurrentFSName());
                String customDir =
                    (String) getPageSessionAttribute(CUSTOM_SNAPSHOT_PATH);
                customDir = customDir == null ? "" : customDir;

                // Skip population of directories to the menu if dirs is empty
                // and user is not giving a custom directory
                if (dirs != null && dirs.length != 0
                                 || customDir.length() != 0) {
                    populateSnapPathMenu(dirs);
                }
            }

            view.populateTableModel(hasPermission);

            ((CCHiddenField) getChild(MESSAGES)).setValue(
                SamUtil.getResourceString(
                    "fs.recoverypoints.createindexprompt").concat(
                "###").concat(
                SamUtil.getResourceString(
                    "fs.recoverypoints.deleteindexprompt")).concat(
                "###").concat(
                SamUtil.getResourceString(
                    "fs.recoverypoints.deletedumpprompt")).concat(
                "###").concat(
                SamUtil.getResourceString(
                    "fs.recoverypoints.cannotretainperm")));

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to display recovery point content!");
            TraceUtil.trace1("Message: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay()",
                "Failed to display recovery point content!",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "fs.recoverypoints.fail.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    private void populateBasePathMenu(FileSystem [] fs) throws SamFSException {
        StringBuffer labelBuf =
            new StringBuffer("fs.recoverypoints.dropdownhelper.label");
        StringBuffer valueBuf = new StringBuffer("");

        for (int i = 0; i < fs.length; i++) {
            if (fs[i].getArchivingType() == FileSystem.ARCHIVING) {
                labelBuf.append("###").append(fs[i].getMountPoint()).
                    append(" (").append(fs[i].getName()).append(")");
                valueBuf.append("###").append(fs[i].getName());
            }
        }

        String [] valueArray = valueBuf.toString().split("###");
        CCDropDownMenu menu = (CCDropDownMenu) getChild(MENU);
        menu.setOptions(
            new OptionList(
                labelBuf.toString().split("###"),
                valueArray));

        if (null != getCurrentFSName()) {
            menu.setValue(getCurrentFSName());
        }
    }

    public String getCurrentFSName() {
        TraceUtil.trace3("Entering");

        String fsName = (String) getPageSessionAttribute(
                                Constants.PageSessionAttributes.FS_NAME);
        if (fsName == null) {
            fsName = (String) getDisplayFieldValue(MENU);
            // fsName changed, saved to session
            SamUtil.setLastUsedFSName(getServerName(), fsName);
        }

        // Check session to see if user has selected any file system in this
        // session if fsName is still null
        if (fsName == null) {
            fsName = SamUtil.getLastUsedFSName(getServerName());
            // Check to see if this is a UFS file system, set fsName to null
            // if yes and ask user to select the file system instead
            if (fsName != null) {
                if (fsName.charAt(0) == '/') {
                    fsName = null;
                } else {
                    // Check to see if fsName is an archiving file system,
                    try {
                        SamQFSSystemFSManager fsMgr =
                            SamUtil.getModel(getServerName()).
                                        getSamQFSSystemFSManager();
                        if (fsMgr.getFileSystem(fsName).
                                getArchivingType() != FileSystem.ARCHIVING) {
                            fsName = null;
                        }
                    } catch (SamFSException samEx) {
                        TraceUtil.trace1(
                            "Exception caught while checking fs type!");
                        fsName = null;
                    }
                }
            }
        }

        if (fsName != null && fsName.length() != 0) {
            setPageSessionAttribute(
                Constants.PageSessionAttributes.FS_NAME, fsName);
        }

        TraceUtil.trace3("Exiting");
        return fsName;
    }

    public void setCurrentSnapShotPath(String ssName) {
        setPageSessionAttribute(
            Constants.PageSessionAttributes.SNAPSHOT_PATH, ssName);
    }

    public String getCurrentSnapShotPath() {
        return (String) getPageSessionAttribute(
                            Constants.PageSessionAttributes.SNAPSHOT_PATH);
    }

    private void createSSPathFileChooserModel() {
        TraceUtil.trace3("Entering");

        if (selectSnapshotPathModel == null)  {
            // Create a filechooser model
            selectSnapshotPathModel =
                new RemoteFileChooserModel(getServerName(), 50);
            selectSnapshotPathModel.setFileListBoxHeight(15);
            selectSnapshotPathModel.setHomeDirectory("/");
            selectSnapshotPathModel.setProductNameAlt(
                "secondaryMasthead.productNameAlt");
            selectSnapshotPathModel.setProductNameSrc(
                "secondaryMasthead.productNameSrc");
            selectSnapshotPathModel.setPopupMode(true);
        }

        TraceUtil.trace3("Exiting");
    }

    public void handlePostSelectSnapshotPathCmdRequest(
        RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");

        // Get new directory from a hidden object in the
        // remote file chooser control.  This hidden object is only available
        // when the text field is not shown.

        RemoteFileChooserControl chooser =
            (RemoteFileChooserControl) getChild(SELECT_SS_PATH);
        String newDir = (String) chooser.getDisplayFieldValue(
                                RemoteFileChooserControl.CHILD_PATH_HIDDEN);

        if (newDir != null) {
            if (newDir.endsWith("/") && newDir.length() > 1) {
                // Remove trailing slash
                newDir = newDir.substring(0, newDir.length() - 1);
            }
        }

        setCurrentSnapShotPath(newDir);
        // ((CCDropDownMenu) getChild(SNAPSHOT_PATH)).setValue(newDir);
        setPageSessionAttribute(CUSTOM_SNAPSHOT_PATH, newDir);

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request when user uses Search Mount Point Drop Down
     * @param RequestInvocationEvent event
     */
    public void handleBasePathMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        // reset snapshot path name and repopulate the path from DumpSch
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SNAPSHOT_PATH);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);
        removePageSessionAttribute(
            CUSTOM_SNAPSHOT_PATH);

        RecoveryPointsView view = (RecoveryPointsView) getChild(CONTAINER_VIEW);
        view.resetTable();

        // refresh page
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request when user uses snap path Drop Down
     * @param RequestInvocationEvent event
     */
    public void handleSnapPathMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        String selectedPath = (String) getDisplayFieldValue(SNAPSHOT_PATH);
        setCurrentSnapShotPath(selectedPath);

        // refresh page
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate snap path directories drop down
     */
    private void populateSnapPathMenu(String [] snapPaths) {
        // Check to see if user defines a custom snap path from the Filechooser
        CCDropDownMenu menu = (CCDropDownMenu) getChild(SNAPSHOT_PATH);
        String customPath =
            (String) getPageSessionAttribute(CUSTOM_SNAPSHOT_PATH);

        if (customPath == null) {
            menu.setOptions(new OptionList(snapPaths, snapPaths));

            // Check if there are any snapshot path set, use the first entry
            // in the dropdown as the default if not
            if (null == getCurrentSnapShotPath() ||
                "".equals(getCurrentSnapShotPath())) {
                setCurrentSnapShotPath(snapPaths[0]);
            }
            return;
        }
        // user actually defines a path, now add this entry to the menu if
        // the menu does not contain that entry, and selects it
        ArrayList newArray = new ArrayList();

        for (int i = 0; i < snapPaths.length; i++) {
            if (!customPath.equals(snapPaths[i])) {
                newArray.add(snapPaths[i]);
            }
        }
        newArray.add(customPath);
        String [] options =
            (String []) newArray.toArray(new String[newArray.size()]);
        menu.setOptions(new OptionList(options, options));
        menu.setValue(customPath);

        // Check if there are any snapshot path set, use the first entry
        // in the dropdown as the default if not
        if (null == getCurrentSnapShotPath() ||
            "".equals(getCurrentSnapShotPath())) {
            setCurrentSnapShotPath(customPath);
        }
    }

    /**
     * Hide snap path menu if no fs is selected
     */
    public boolean beginCurrentSnapshotPathValueDisplay(
        ChildDisplayEvent event) {
        return null != getCurrentFSName() && !"".equals(getCurrentFSName());
    }

    /**
     * checkRolePriviledge() checks if the user is a valid Admin
     * disable all action buttons / dropdown if not
     */
    private boolean checkRolePrivilege() {
        TraceUtil.trace3("Entering");
        boolean hasPermission = false;

        if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILE_OPERATOR)) {
            ((CCHiddenField) getChild(ROLE)).setValue("FILE");

            hasPermission = true;
        }
        TraceUtil.trace3("Exiting");
        return hasPermission;
    }

}
