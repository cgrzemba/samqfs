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

// ident	$Id: FileBrowserView.java,v 1.23 2008/04/29 19:32:14 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.web.model.DirInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileCopyDetails;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.Filter;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.File;
import java.io.IOException;
import javax.servlet.ServletException;

/** the table container view */
public class FileBrowserView extends CommonTableContainerView {
    public static final String TILED_VIEW   = "tiled_view";
    public static final String ACTION_MENU  = "ActionMenu";
    public static final String ACTION_MENU_HREF = "ActionMenuHref";

    // the table model
    private CCActionTableModel model = null;

    /** default constructor */
    public FileBrowserView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "FilesTable";

        model = createTableModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** register this container view's child views */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(TILED_VIEW, FileBrowserTiledView.class);
        registerChild(ACTION_MENU_HREF, CCHref.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    /** create an instance of the named child view */
    public View createChild(String name) {
        if (name.equals(TILED_VIEW)) {
            return new FileBrowserTiledView(this, model, name);
        } else if (name.equals(ACTION_MENU_HREF)) {
            return new CCHref(this, name, null);
        } else {
            return super.createChild(model, name, TILED_VIEW);
        }
    }

    /** create the table model */
    private CCActionTableModel createTableModel() {
        TraceUtil.trace3("Entering");

        String xmlFile = "/jsp/fs/FileBrowserTable.xml";

        TraceUtil.trace3("Exiting");
        return new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(), xmlFile);
    }

    /**
     * Use to clear table model to avoid showing tables with empty rows.
     * Must call this method first thing in beginDisplay.
     */
    public void clearTableModel(boolean basePathMenuClicked) {
        // loop through the stage file array and populate the table model
        model.clear();

        // initialize headers
        model.setActionValue("Name", "fs.filebrowser.name");
        model.setActionValue("User", "fs.filebrowser.user");
        model.setActionValue("Size", "fs.filebrowser.size");
        model.setActionValue("ModifiedDate", "fs.filebrowser.lastmod");
        model.setActionValue("Status", "fs.filebrowser.status");
        model.setActionValue("StorageTier",  "fs.filebrowser.storagetiers");

        // If base path menu is used, reset page to first page
        if (basePathMenuClicked) {
            model.setPage(1);
            model.setFirstRowIndex(0);
            model.setLastRowIndex(24);
        }
    }

    /** populate the table model */
    /*  Mount Point is only used when error 31212 is caught! */
    public void populateTableModel(
        boolean archive, String currentFSName,
        String relativePath, String mountPoint, String recoveryPointTime) {
        TraceUtil.trace3("Entering");

        // Determine if an alert needs to be shown to the user that there are
        // more entries in the current browsing directory than the maximum
        // number of entries that are fetched
        boolean hasMore = false;
        boolean hasError = false;

        FileBrowserViewBean parent = (FileBrowserViewBean) getParentViewBean();
        model.setTitle(
            parent.LIVE_DATA.equals(parent.getMode()) ?
                SamUtil.getResourceString(
                    "fs.filebrowser.tabletitle.live",
                    new String [] {
                        parent.getCurrentDirectory()}) :
                SamUtil.getResourceString(
                    "fs.filebrowser.tabletitle.recovery",
                    new String [] {
                        parent.getCurrentDirectory(),
                        recoveryPointTime}));

        String serverName = parent.getServerName();

        StringBuffer fileNames = new StringBuffer();
        DirInfo dirInfo = null;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemFSManager fsManager =
                sysModel.getSamQFSSystemFSManager();

            String dirPath = parent.getCurrentDirectory();

            Filter filter = (Filter)
                parent.getPageSessionAttribute(parent.FILTER_VALUE);

            int theseDetails =
                FileUtil.FNAME |
                FileUtil.FILE_TYPE |
                FileUtil.SIZE  |
                FileUtil.MODIFIED  |
                FileUtil.SAM_STATE |
                FileUtil.USER  |
                FileUtil.COPY_SUMMARY;

            dirInfo =
                fsManager.getAllFilesInformation(
                        currentFSName,
                        FileBrowserViewBean.LIVE_DATA.equals(parent.getMode()) ?
                            null :
                            parent.getMode(), // snapPath
                        relativePath,
                        null, // lastFile
                        parent.getMaxEntries(),
                        theseDetails,
                        filter);

            // set flag to indicate if "has more entries" alert needs to be
            // shown
            hasMore = parent.getMaxEntries() < dirInfo.getTotalCount();

            StageFile [] stageFile = dirInfo.getFileDetails();

            // Add Up One Level if current directory is not root
            if (!"/".equals(dirPath)) {
                model.setValue("NameHref", "fs.filebrowser.uponelevel");
                model.setValue("IconDir", Constants.Image.ICON_UP_ONE_LEVEL);
                model.setValue("DirNameText", "fs.filebrowser.uponelevel");
                model.setValue(
                    "IconFile", Constants.Image.ICON_BLANK_ONE_PIXEL);
                model.setValue("FileNameText", "");
                model.setValue("NameValue", "");
                model.setValue("ModifiedDateText", "");
                model.setValue("SizeText", "");
                model.setValue("UserText", "");
                model.setValue(
                    "IconDiskCache", Constants.Image.ICON_BLANK_ONE_PIXEL);
                model.setValue("TextDiskCache", "");

                // reset image fields
                for (int k = 1; k <= 4; k++) {
                    model.setValue(
                        "IconCopy".concat(Integer.toString(k)),
                        Constants.Image.ICON_BLANK);
                }
                fileNames.append("fs.filebrowser.uponelevel");
                model.setSelectionVisible(false);
            }

            // the following check has to happen after appending the
            // Up One Level entry.  Otherwise Up One Level will be missing
            // if the directory contains no entry.
            if (stageFile == null) {
                return;
            }

            // Avoid double slashes for files in root directory
            if ("/".equals(dirPath)) {
                dirPath = "";
            }

            model.setAvailableRows(stageFile.length);

            for (int i = 0; i < stageFile.length; i++) {
                StageFile file = stageFile[i];

                String name =
                    dirPath.concat(File.separator).concat(file.getName());

                if (fileNames.length() > 0) {
                    model.appendRow();
                    fileNames.append(";").append(name);
                } else {
                    fileNames.append(name);
                }

                if (file.isDirectory()) {
                    model.setValue("NameHref", file.getName());
                    model.setValue("IconDir", Constants.Image.DIR_ICON);
                    model.setValue("DirNameText", file.getName());
                    model.setValue(
                        "IconFile", Constants.Image.ICON_BLANK_ONE_PIXEL);
                    model.setValue("FileNameText", "");
                } else {
                    model.setValue("IconFile", Constants.Image.FILE_ICON);
                    model.setValue("FileNameText", file.getName());
                    model.setValue(
                        "IconDir", Constants.Image.ICON_BLANK_ONE_PIXEL);
                    model.setValue("DirNameText", "");
                }

                model.setValue("ModifiedDateText",
                    SamUtil.getTimeString(file.lastModified()));

                model.setValue("SizeText",
                               new Capacity(file.length(),
                                            SamQFSSystemModel.SIZE_B));

                model.setValue("UserText", file.getUser());

                // If current directory is not an archiving file system, don't
                // show any icons there
                if (!archive) {
                    // files are online for all non-archiving file systems
                    model.setValue(
                        "IconDiskCache",
                        Constants.Image.ICON_BLANK);
                    model.setValue("TextDiskCache", "");
                    continue;
                }

                int online = file.getOnlineStatus();
                switch (online) {
                    case StageFile.ONLINE:
                        model.setValue(
                            "IconDiskCache",
                            Constants.Image.ICON_ONLINE);
                        model.setValue(
                            "TextDiskCache",
                            "fs.filebrowser.online");
                        break;
                    case StageFile.PARTIAL_ONLINE:
                        model.setValue(
                            "IconDiskCache",
                            Constants.Image.ICON_PARTIAL_ONLINE);
                        model.setValue(
                            "TextDiskCache",
                            "fs.filebrowser.partialonline");
                        break;
                    default:
                        model.setValue(
                            "IconDiskCache",
                            Constants.Image.ICON_OFFLINE);
                        model.setValue(
                            "TextDiskCache",
                            "fs.filebrowser.offline");
                        break;
                }

                FileCopyDetails [] allDetails = file.getFileCopyDetails();

                // reset image fields
                for (int k = 1; k <= 4; k++) {
                    model.setValue(
                        "IconCopy".concat(Integer.toString(k)),
                        Constants.Image.ICON_BLANK);
                }

                boolean noValidCopy = true;
                for (int j = 0; j < allDetails.length; j++) {
                    String copyString =
                        Integer.toString(allDetails[j].getCopyNumber());

                    boolean thisCopyDamaged = allDetails[j].isDamaged();
                    noValidCopy = noValidCopy && thisCopyDamaged;

                    model.setValue(
                        "IconCopy".concat(copyString),
                        FSUtil.getImage(
                            copyString,
                            allDetails[j].getMediaType(),
                            thisCopyDamaged));
                }

                // Hidden Field for Javascript
                // <isAllCopyDamaged>###<online_status>###<isDirectory>
                model.setValue(
                    "HiddenInfo",
                    Boolean.toString(noValidCopy).concat("###").concat(
                    Integer.toString(online)).concat("###").concat(
                    Boolean.toString(file.isDirectory())));
            }

            ((CCHiddenField) parent.getChild(FileBrowserViewBean.FILE_NAMES)).
                setValue(fileNames.toString());
        } catch (SamFSException sfe) {
            // if user is toggling between live and recovery pt mode
            // and 31212 is caught, we know there is a mismatch in the content
            // of the two modes.  Simply forward the page to the mode that user
            // wants to browse, but redirect the page to the top level of the
            // mount point, and show a message to communicate to the user
            // about the inexistence of the directory.
            if (sfe.getSAMerrno() == 31212 &&
                "true".equalsIgnoreCase(
                    (String) parent.getPageSessionAttribute(
                    FileBrowserViewBean.IS_TOGGLE))) {
                String snapDate = (String) parent.getPageSessionAttribute(
                                        parent.LAST_USED_RECOVERY_PT_DATE);
                SamUtil.setInfoAlert(
                    parent,
                    parent.CHILD_COMMON_ALERT,
                    SamUtil.getResourceString(
                        "fs.filebrowser.directory.changed",
                        new String [] {mountPoint}),
                    parent.LIVE_DATA.equals(parent.getMode()) ?
                        SamUtil.getResourceString(
                        "fs.filebrowser.directory.changed.details.snapshot",
                            new String [] {
                                parent.getCurrentDirectory(),
                                snapDate}) :
                        SamUtil.getResourceString(
                        "fs.filebrowser.directory.changed.details.live",
                            new String [] {
                                parent.getCurrentDirectory(),
                                snapDate}),
                    serverName);

                // set current dir to mount point, then refresh page
                parent.setCurrentDirectory(mountPoint);
                parent.forwardTo(getRequestContext());
            } else {
                // Disable the toggle component
                ((CCHiddenField) parent.getChild(
                    parent.SUPPORT_RECOVERY)).setValue("false");

                SamUtil.processException(
                    sfe,
                    getClass(),
                    "populateTableModel",
                    "Failed to retrieve directory content!",
                    serverName);

                SamUtil.setErrorAlert(
                    parent,
                    parent.CHILD_COMMON_ALERT,
                    SamUtil.getResourceString("fs.filebrowser.fail.populate"),
                    sfe.getSAMerrno(),
                    sfe.getMessage(),
                    serverName);
                hasError = true;
            }
        }

        // reset IS_TOGGLE for the next display cycle
        getParentViewBean().
            removePageSessionAttribute(FileBrowserViewBean.IS_TOGGLE);

        if (hasMore && !hasError) {
            SamUtil.setInfoAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "fs.filebrowser.moreentries.summary",
                    new String [] {
                        Integer.toString(dirInfo.getTotalCount()),
                        parent.getCurrentDirectory()}),
                "fs.filebrowser.moreentries.details",
                parent.getServerName());
        }


        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // disable selection radio buttons
        CCActionTable theTable = (CCActionTable)getChild(CHILD_ACTION_TABLE);
        ((CCRadioButton) theTable.
            getChild(CCActionTable.CHILD_ROW_SELECTION_RADIOBUTTON)).
            setTitle("");

        // clear any selection that might lingering around
        model.setRowSelected(false);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for action drop-down menu
     *  @param RequestInvocationEvent event
     */
    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        int option = getDropDownMenuValue();
        FileBrowserViewBean parent = (FileBrowserViewBean) getParentViewBean();
        String selectedFileInfo =
            (String) parent.getDisplayFieldValue(parent.SELECTED_FILE);
        String [] fileInfo = selectedFileInfo.split("###");
        boolean dir =
            fileInfo == null || fileInfo.length != 2 ?
                false :
                "true".equals(fileInfo[1]);
        String selectedFileName =
            fileInfo == null || fileInfo.length != 2 ?
                "" : fileInfo[0];
        long jobID = -1;
        String operation;

        try {
            SamQFSSystemModel sysModel =
                SamUtil.getModel(parent.getServerName());
            SamQFSSystemFSManager fsManager =
                sysModel.getSamQFSSystemFSManager();
            switch (option) {
                // Archive
                case 1:
                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Start archiving file ").
                            append(selectedFileName).toString());
                    jobID = fsManager.archiveFiles(
                                new String [] {selectedFileName},
                                dir ?
                                    Archiver.RECURSIVE: 0);
                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Done archiving file ").
                            append(selectedFileName).toString());
                    operation =
                        jobID <= 0 ?
                            "fs.filebrowser.archive.success":
                            "fs.filebrowser.archive.successwithjobid";
                    break;
                // Release
                case 2:
                    operation = "fs.filebrowser.release.success";
                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Start releasing file ").
                            append(selectedFileName).toString());
                    jobID = fsManager.releaseFiles(
                                new String [] {selectedFileName},
                                dir ?
                                    Releaser.RECURSIVE : 0,
                                0);
                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Done releasing file ").
                            append(selectedFileName).toString());
                    operation =
                        jobID <= 0 ?
                            "fs.filebrowser.release.success" :
                            "fs.filebrowser.release.successwithjobid";
                    break;
                // Stage (popup)
                case 3:
                // Restore (popup)
                case 4:
                // Restore entire snapshot (popup)
                case 5:
                default:
                    operation = "";
                    break;
            }

            BaseJob job = null;
            if (jobID > 0) {
                // Job may have been very short lived and may already be done.
                job = sysModel.getSamQFSSystemJobManager().getJobById(jobID);
            }

            SamUtil.setInfoAlert(
                getParentViewBean(),
                parent.CHILD_COMMON_ALERT,
                "success.summary",
                jobID <= 0 ?
                    SamUtil.getResourceString(
                        operation,
                        selectedFileName) :
                    SamUtil.getResourceString(
                        operation,
                        new String [] {
                            selectedFileName,
                            String.valueOf(jobID)}),
                parent.getServerName());

        } catch (SamFSException samEx) {
            String errMsg;
            if (option == 1) {
                errMsg = SamUtil.getResourceString(
                            "fs.filebrowser.archive.failed", selectedFileName);
            } else {
                errMsg = SamUtil.getResourceString(
                            "fs.filebrowser.release.failed", selectedFileName);
            }
            LogUtil.error(errMsg);
            SamUtil.processException(
                samEx,
                getClass(),
                "handleActionMenuHrefRequest",
                errMsg,
                parent.getServerName());

            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                errMsg,
                samEx.getSAMerrno(),
                samEx.getMessage(),
                parent.getServerName());
        }

        // refresh page
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    private int getDropDownMenuValue() throws ModelControlException {
        String value = (String) getDisplayFieldValue(ACTION_MENU);
        // get drop down menu selected option
        int option = 0;
        try {
            option = Integer.parseInt(value);
        } catch (NumberFormatException nfe) {
            // should not get here!!!
        }
        return option;
    }

    public void resetTable() {
        try {
            CCActionTable theTable =
                (CCActionTable)getChild(CHILD_ACTION_TABLE);
            theTable.resetStateData();
        } catch (ModelControlException modelEx) {
            TraceUtil.trace1("Failed to reset table state data!");
        }
    }
}
