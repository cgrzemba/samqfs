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

// ident	$Id: NFSShareAddView.java,v 1.9 2008/12/16 00:12:10 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.ContainerViewBase;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.NFSOptions;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserModel;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.filechooser.CCFileChooserWindow;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * NFSShareAddView - view in NFS Shares Page when user wants to add
 * NFS shares.
 */
public class NFSShareAddView extends RequestHandlingViewBase
    implements CCPagelet {

    // Page Children
    public static final String PAGE_TITLE = "AddPageTitle";
    public static final String PATH_CHOOSER = "pathChooser";
    public static final String BROWSE_TEXTFIELD = "browsetextfield";
    public static final String SHARE_NOW_VALUE  = "shareNowValue";
    public static final String BASE_PATH = "basePath";

    // private models for various components
    private RemoteFileChooserModel pathChooserModel;
    private CCPropertySheetModel propertySheetModel;
    private CCPageTitleModel pageTitleModel;

    // keep track of the server name that is transferred from the VB
    private String serverName;

    public NFSShareAddView(View parent, String name, String serverName) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.serverName = serverName;

        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
        pathChooserModel = createFileChooserModel();

        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * registerChildren
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(PATH_CHOOSER, RemoteFileChooserControl.class);
        pageTitleModel.registerChildren(this);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * createChild
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child = null;

        if (name.equals(PAGE_TITLE)) {
            child = new CCPageTitle(this, pageTitleModel, name);
        } else if (pageTitleModel.isChildSupported(name)) {
            // Create child from page title model.
            child =  pageTitleModel.createChild(this, name);
        } else if (name.equals(PATH_CHOOSER)) {
            child = new RemoteFileChooserControl(this, pathChooserModel, name);
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        try {
            populateBasePathMenu();
        } catch (SamFSException samEx) {
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "NFSDetailsViewBean.error.failedDir",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
            SamUtil.processException(
                samEx,
                this.getClass(),
                "NFSShareAddView()",
                "Failed to load base directory information",
                serverName);
        }

        // by default, shareNowValue setChecked(true);
        propertySheetModel.setValue("shareNowValue", Boolean.toString(true));

        TraceUtil.trace3("Exiting");
    }

    // implement the CCPagelet interface

    /**
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        return "/jsp/fs/NFSShareAddPagelet.jsp";
    }

    /**
     * Create pagetitle model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/NFSShareAddPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return  pageTitleModel;
    }

    /**
     * Create propertysheet model
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/NFSShareAddPropertySheet.xml");
        }

        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    private RemoteFileChooserModel createFileChooserModel() {
        TraceUtil.trace3("Entering");
        pathChooserModel = new RemoteFileChooserModel(serverName, 50);
        pathChooserModel.setFileListBoxHeight(15);

        pathChooserModel.setProductNameAlt(
            "secondaryMasthead.productNameAlt");
        pathChooserModel.setProductNameSrc(
            "secondaryMasthead.productNameSrc");
        pathChooserModel.setPopupMode(true);
        TraceUtil.trace3("Exiting");
        return pathChooserModel;
    }

    private void populateBasePathMenu() throws SamFSException {
        SamQFSSystemFSManager fsManager =
            SamUtil.getModel(serverName).getSamQFSSystemFSManager();

        FileSystem [] fs = fsManager.getAllFileSystems();
        GenericFileSystem[] genfs = fsManager.getNonSAMQFileSystems();

        StringBuffer labelBuf = new StringBuffer();
        StringBuffer valueBuf = new StringBuffer();

        for (int i = 0; i < genfs.length; i++) {
            // Skip entry if mount point is missing
            String mountPoint = genfs[i].getMountPoint();
            if (mountPoint != null && mountPoint.length() != 0) {
                if (labelBuf.length() > 0) {
                    labelBuf.append("###");
                    valueBuf.append("###");
                }
                labelBuf.append(mountPoint).
                    append(" (").append(genfs[i].getName()).append(")");
                valueBuf.append(genfs[i].getMountPoint());
            }
        }

        for (int i = 0; i < fs.length; i++) {
            // Skip entry if mount point is missing
            String mountPoint = fs[i].getMountPoint();
            if (mountPoint != null && mountPoint.length() != 0) {
                if (labelBuf.length() > 0) {
                    labelBuf.append("###");
                    valueBuf.append("###");
                }
                labelBuf.append(mountPoint).
                    append(" (").append(fs[i].getName()).append(")");
                valueBuf.append(fs[i].getMountPoint());
            }
        }

        String [] valueArray = valueBuf.toString().split("###");
        ((CCDropDownMenu) getChild(BASE_PATH)).setOptions(
            new OptionList(
                labelBuf.toString().split("###"),
                valueArray));

        CCFileChooserWindow folderChooser =
                (CCFileChooserWindow) getChild(PATH_CHOOSER);

        // set whatever is in the drop down as the default path
        String browsedDir = folderChooser.
            getDisplayFieldStringValue(CCFileChooserWindow.BROWSED_FILE_NAME);

        if (browsedDir == null) {
            if (valueBuf.length() > 0 && !("/".equals(valueArray[0]))) {
                folderChooser.setDisplayFieldValue(
                    CCFileChooserWindow.BROWSED_FILE_NAME,
                    new StringBuffer(
                        valueArray[0]).append("/").toString());
                return;
            } else {
                folderChooser.setDisplayFieldValue(
                    CCFileChooserWindow.BROWSED_FILE_NAME, "/");
            }
        }
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String dirName = ((ContainerViewBase) getChild(PATH_CHOOSER)).
            getDisplayFieldStringValue(BROWSE_TEXTFIELD);

        String errorMessage = validate(dirName);
        if (errorMessage != null) {
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "NFSDetailsViewBean.error.failedAdd",
                -2600,
                errorMessage,
                serverName);
            getParentViewBean().forwardTo(getRequestContext());
            TraceUtil.trace3("Exiting");
            return;
        }

        String shareState =
            "true".equals(getDisplayFieldStringValue(SHARE_NOW_VALUE)) ?
            NFSOptions.NFS_SHARED : NFSOptions.NFS_CONFIGURED;

        if (dirName != null) { // if dirName != null
            try {
                SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

                NFSOptions opts = new NFSOptions(dirName, shareState);
                GenericFileSystem fs = FSUtil.getRootFileSystem(sysModel);

                if (fs != null && opts != null) {
                    errorMessage = validateDirName(dirName);
                    if (errorMessage != null) {
                        // set error alert
                        SamUtil.setErrorAlert(
                            getParentViewBean(),
                            CommonViewBeanBase.CHILD_COMMON_ALERT,
                            "NFSDetailsViewBean.error.failedAdd",
                            -2600,
                            SamUtil.getResourceString(errorMessage, dirName),
                            serverName);
                    } else {
                        LogUtil.info(this.getClass(),
                            "handleSubmitRequest",
                            new StringBuffer().append(
                                "Start adding new NFS directory ").
                                append(dirName).toString());

                        fs.setNFSOptions(opts);

                        LogUtil.info(this.getClass(),
                            "handleSubmitRequest",
                            new StringBuffer().append(
                                "finished adding new NFS directory ").
                                append(dirName).toString());


                        SamUtil.setInfoAlert(
                            getParentViewBean(),
                            CommonViewBeanBase.CHILD_COMMON_ALERT,
                            "success.summary",
                            SamUtil.getResourceString(
                                "filesystem.nfs.msg.add", dirName),
                            serverName);

                        setFlag(0);
                    }
                }
            } catch (SamFSException ex) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "NFSShareAddView::beginDisplay()",
                    "Failed to add directory as a NFS Shared Directory",
                    serverName);
                SamUtil.setErrorAlert(
                        getParentViewBean(),
                        CommonViewBeanBase.CHILD_COMMON_ALERT,
                        "NFSDetailsViewBean.error.failedAdd",
                        ex.getSAMerrno(),
                        ex.getMessage(),
                        serverName);
            }
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }


    private boolean isSubDir(String dirName, String parent) {
        parent = "/".equals(parent) ? parent :
           new StringBuffer(parent).append("/").toString();

        return dirName.startsWith(parent);
    }

    private boolean isParentDir(String dirName, String subDir) {
        // dirName will never be "root" while this method is used
        return subDir.startsWith(
            new StringBuffer(dirName).append("/").toString());
    }

    private void setFlag(int flag) {
        if (flag >= 0 && flag <= 2) {
            getParentViewBean().
                setPageSessionAttribute(NFSShareDisplayView.FLAG,
                new Integer(flag));
        }
    }

    /**
     * This helper function validates the fully qualified path name.
     * If error occurs, return the error message that is going to be shown
     * on the top of the page as an alert.
     */
    private String validate(String inputString) {
        inputString = inputString == null ? "" : inputString;

        if (inputString.length() == 0) {
            return "common.error.locationEmpty";
        } else if (!SamUtil.isValidNonSpecialCharString(inputString)) {
            return "common.error.invalidPath";
        } else {
            // no error
            return null;
        }
    }

    /**
     * This helper function validates the directory name of which user wants
     * to share.
     *
     * 1) User cannot share subdirectories when dir is configured to be shared
     *    e.g. cannot share /src/man/man1 when /src/man appears in
     *    the Shared Directories list
     * 2) User cannot share dir when subdirectories are configured to be shared
     *    e.g. cannot share /src when /src/man appears in the Shared
     *    Directories list
     * 3) User cannot share a directory that is already shared (duplicate).
     */
    private String validateDirName(String dirName) {
        String dirNameList =
            (String) ((CCHiddenField) getParentViewBean().
                getChild(NFSDetailsViewBean.DIR_NAMES_LIST)).getValue();
        TraceUtil.trace3(
            new StringBuffer("dirNameList is ").append(dirNameList).
            append("; dirName is ").append(dirName).toString());

        // No active share directory, return success
        if (dirNameList.length() == 0) {
            return null;
        }

        // Check if dirName is "/" (root), if dirNameList is not empty,
        // return error right away because it is not allow to share the parent
        // directory of any existing NFS Shared Directories
        if (dirNameList.length() > 0 && "/".equals(dirName)) {
            return "NFSDetailsViewBean.error.failedAddParent";
        }

        String [] dirNameArray = dirNameList.split("###");

        for (int i = 0; i < dirNameArray.length; i++) {
            // Case 1
            if (dirName.length() > dirNameArray[i].length() &&
                isSubDir(dirName, dirNameArray[i])) {
                return "NFSDetailsViewBean.error.failedAddChild";

            // Case 2
            } else if (dirName.length() < dirNameArray[i].length() &&
                isParentDir(dirName, dirNameArray[i])) {
                return "NFSDetailsViewBean.error.failedAddParent";

            // Case 3
            } else if (dirName.equals(dirNameArray[i])) {
                return "NFSDetailsViewBean.error.failedAddDuplicate";
            }
        }
        // If code reaches here, it's error free :)
        return null;
    }
}
