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

// ident	$Id: RestorePopupViewBean.java,v 1.9 2008/03/17 14:43:35 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import java.io.IOException;
import javax.servlet.ServletException;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileCopyDetails;
import com.sun.netstorage.samqfs.web.model.fs.RestoreFile;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserModel;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.web.ui.view.html.CCStaticTextField;

public class RestorePopupViewBean extends CommonSecondaryViewBeanBase {
    // page name & default url
    public static final String PAGE_NAME = "RestorePopup";
    public static final String DEFAULT_URL = "/jsp/fs/RestorePopup.jsp";

    // children
    public static final String PAGE_TITLE = "pageTitle";
    public static final String HIDDEN_MOUNT_POINT = "MountPoint";
    public static final String HIDDEN_FILE_NAME = "FileName";
    public static final String REPLACE_TYPE_TEXT = "replaceTypeValue";
    public static final String RESTORE_OPTION = "stageOptionsValue";
    public static final String ORIGINAL_FILE = "restoreTypeText";

    //  keep track of file names and info
    public static final String FILE_TO_RESTORE = "file_to_restore";
    public static final String FS_NAME = "fs_name";
    public static final String MOUNT_POINT = "mount_point";
    public static final String IS_DIR = "is_dir";
    public static final String COPY_INFO = "copy_info";
    public static final String RECOVERY_POINT_PATH = "snap_path";

    // Flag to indicate restore entire recovery point
    public static final String ENTIRE_RECOVERY_POINT = "##all##";

    // File Chooser
    public static final String RESTORE_PATH_CHOOSER = "restoreToPathnameValue";
    private RemoteFileChooserModel restorePathModel = null;

    // page title model / property sheet model
    private CCPageTitleModel ptModel = null;
    private CCPropertySheetModel psModel = null;

    public RestorePopupViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = createPageTitleModel();
        psModel = createPropertySheetModel();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(RESTORE_PATH_CHOOSER, RemoteFileChooserControl.class);
        registerChild(HIDDEN_MOUNT_POINT, CCHiddenField.class);
        registerChild(HIDDEN_FILE_NAME, CCHiddenField.class);
        ptModel.registerChildren(this);
        psModel.registerChildren(this);

        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (name.equals(RESTORE_PATH_CHOOSER)) {
            createRestorePathFileChooserModel();
            return new RemoteFileChooserControl(this,
                                                 restorePathModel,
                                                 name);
        } else if (PropertySheetUtil.isChildSupported(psModel, name)) {
            return PropertySheetUtil.createChild(this, psModel, name);
        } else if (name.equals(HIDDEN_MOUNT_POINT)) {
            return new CCHiddenField(this, name, getMountPoint());
        } else if (name.equals(HIDDEN_FILE_NAME)) {
            return new CCHiddenField(this, name, getFileName());
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        return PageTitleUtil.createModel("/jsp/fs/StagePageTitle.xml");
    }

    private CCPropertySheetModel createPropertySheetModel() {
        return
            PropertySheetUtil.createModel("/jsp/fs/RestorePropertySheet.xml");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(evt);

        String serverName = getServerName();
        String file = getFileToRestore();

        // save the fs name from http request, do not remove
        String fsName = getFSName();

        // save the recovery point from http request, do not remove
        String recoveryPoint = getRecoveryPoint();

        // Restore the entire recovery point?
        boolean restoreEntire = false;

        ((CCStaticTextField) getChild(ORIGINAL_FILE)).setValue(
            getMountPoint().concat("/").concat(getFileName()));

        CCDropDownMenu replaceType =
            (CCDropDownMenu) getChild(REPLACE_TYPE_TEXT);
        if (ENTIRE_RECOVERY_POINT.equals(file)) {
            restoreEntire = true;
            replaceType.setValue("FSRestore.restore.type.filesystem");
        } else {
            replaceType.setValue(file);
        }

        boolean dir = isDirectory();
        populateRestoreOption(serverName, restoreEntire, dir);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate the "Online status after restoring drop down.
     * Only show the first three options (without copy info) if a directory
     * or the entire recovery point content is about to restore.
     */
    private void populateRestoreOption(
        String serverName, boolean restoreEntire, boolean dir) {
        CCDropDownMenu restoreMenu = (CCDropDownMenu) getChild(RESTORE_OPTION);

        try {
            FileCopyDetails [] copyDetails;

            if (dir || restoreEntire) {
                copyDetails = new FileCopyDetails[0];
            } else {
                copyDetails = getCopyDetails();
            }

            String [] labels = new String[copyDetails.length + 3];
            String [] values = new String[copyDetails.length + 3];

            // Populate the first three selections in the restore option
            // drop down before getting into specific copy selections
            // 1. Leave <all> offline
            // 2. Bring <all> online (system picks copy)
            // 3. Bring online data marked as online in recovery point
            //    (system pick copy)
            // 4. <copy 1>
            // 5. <copy 2> if any
            // 6. <copy 3> if any
            // 7. <copy 4> if any

            labels[0] = dir ?
                "FSRestore.restore.stageOptionOffLine.all" :
                "FSRestore.restore.stageOptionOffLine";
            values[0] = Integer.toString(RestoreFile.NO_STG_COPY);

            labels[1] = dir ?
                "FSRestore.restore.stageOptionSystemPick.all" :
                "FSRestore.restore.stageOptionSystemPick";
            values[1] = Integer.toString(RestoreFile.SYS_STG_COPY);

            labels[2] = dir ?
                "FSRestore.restore.stageOptionAsInDump.all" :
                "FSRestore.restore.stageOptionAsInDump";
            values[2] = Integer.toString(RestoreFile.STG_COPY_ASINDUMP);

            // Start populating Copy Info
            for (int i = 0; i < copyDetails.length; i++) {
                int copyNumber = copyDetails[i].getCopyNumber();
                StringBuffer buf = new StringBuffer(
                    copyDetails[i].getMediaType() == BaseDevice.MTYPE_DISK ?
                        SamUtil.getResourceString(
                            "fs.filebrowser.alttext.disk",
                            new String [] {
                                Integer.toString(copyNumber)}) :
                        SamUtil.getResourceString(
                            "fs.filebrowser.alttext.tape",
                            new String [] {
                                Integer.toString(copyNumber)}).concat(
                            " (").concat(
                            SamUtil.getMediaTypeString(
                            copyDetails[i].getMediaType()).concat(")")));

                if (copyDetails[i].isDamaged()) {
                    buf.append(" - ").append(
                        SamUtil.getResourceString(
                        "fs.filebrowser.alttext.damaged"));
                }
                labels[i + 3] = buf.toString();
                values[i + 3] = Integer.toString(copyNumber);
            }

            restoreMenu.setOptions(new OptionList(labels, values));
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "beginDisplay",
                                     "Unable to load file copy information",
                                     serverName);
            SamUtil.setErrorAlert(this,
                                   ALERT,
                                   "fs.restore.loadcopyinfo.failed",
                                   sfe.getSAMerrno(),
                                   sfe.getMessage(),
                                   serverName);
        }
    }

    public void handleSubmitRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        // retrieve all the parameters
        String serverName = getServerName();
        String file = getFileToRestore();
        String recoveryPoint = getRecoveryPoint();
        boolean dir = isDirectory();

        RestoreFile restoreFile = null;

        try {
            SamQFSSystemFSManager fsManager =
                SamUtil.getModel(serverName).getSamQFSSystemFSManager();

            // Instantiate RestoreFile object and fill in the blanks

            // Restore Entire Recovery Point
            if (ENTIRE_RECOVERY_POINT.equals(file)) {
                restoreFile = fsManager.getEntireFSRestoreFile();
                if (restoreFile == null) {
                    throw new SamFSException(
                        SamUtil.getResourceString(
                            "FSRestore.restore.error.failedGetRestoreFile"));
                }
            } else {
                TraceUtil.trace3("About to call getRestoreFile: fsName: " +
                    getFSName() + ", Recovery Point: " + recoveryPoint +
                    ", File Name: " + getFileName());

                // file/directory
                restoreFile = fsManager.getRestoreFile(getFileName());
                if (restoreFile == null) {
                    throw new SamFSException(
                        SamUtil.getResourceString(
                            "FSRestore.restore.error.failedGetRestoreFile"));
                }
            }
            // Restore path
            RemoteFileChooserControl chooser =
                (RemoteFileChooserControl) getChild(RESTORE_PATH_CHOOSER);
            String restoreFilePath = chooser.getDisplayFieldStringValue(
                                RemoteFileChooserControl.BROWSED_FILE_NAME);

            if (restoreFilePath == null || restoreFilePath.length() == 0) {
                throw new SamFSException(
                    SamUtil.getResourceString(
                        "FSRestore.restore.error.noRestorePath"));
            }
            TraceUtil.trace3("restorePath is " + restoreFilePath);
            restoreFile.setRestorePath(restoreFilePath);

            // Replace type
            int replaceType = SamQFSSystemFSManager.RESTORE_REPLACE_NEVER;
            CCDropDownMenu replaceTypeMenu =
                (CCDropDownMenu) getChild(REPLACE_TYPE_TEXT);

            try {
                replaceType =
                    Integer.parseInt((String) replaceTypeMenu.getValue());
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1("Developer's bug!!! - replaceType");
            }

            // Restore Option
            int stageOption = RestoreFile.STG_COPY_ASINDUMP;
            try {
                stageOption = Integer.parseInt(
                    getDisplayFieldStringValue(RESTORE_OPTION));
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1("Developer's bug!!! - stageOption");
            }
            restoreFile.setStageCopy(stageOption);
            TraceUtil.trace3("About to call restoreFiles: fsName: " +
                getFSName() + ", Recovery Point: " + getRecoveryPoint() +
                ", replaceType is " + replaceType);

            // Now go restore whatever user wants to restore
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new StringBuffer("Start restoring file ").
                    append(restoreFile.getFileName()).toString());
            long jobId = fsManager.restoreFiles(
                                getFSName(),
                                getRecoveryPoint(),
                                replaceType,
                                new RestoreFile[] {restoreFile});
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new StringBuffer("Done restoring file ").
                    append(restoreFile.getFileName()).toString());
            BaseJob job = null;
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            if (jobId > 0) {
                // Job may have been very short lived and may already be done.
                job = sysModel.getSamQFSSystemJobManager().getJobById(jobId);
            }

            setSubmitSuccessful(true);

            String alertMsg;
            String restoreWhat;
            if (job != null) {
                // Restore Entire Recovery Point
                if (ENTIRE_RECOVERY_POINT.equals(file)) {
                    alertMsg = "FSRestore.restore.successWithJobFileSystem";
                    restoreWhat = restoreFile.getAbsolutePath();
                } else {
                    alertMsg = "FSRestore.restore.successWithJob";
                    restoreWhat = file;
                }

                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        alertMsg,
                        new String[] { restoreWhat,
                                       restoreFilePath,
                                       String.valueOf(jobId),
                                       "",
                                       ""}),
                   serverName);
            } else {
                // Restore Entire Recovery Point
                if (ENTIRE_RECOVERY_POINT.equals(file)) {
                    alertMsg = "FSRestore.restore.successFileSystem";
                    restoreWhat = restoreFile.getAbsolutePath();
                } else {
                    alertMsg = "FSRestore.restore.success";
                    restoreWhat = file;
                }

                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        alertMsg,
                        new String[] { restoreWhat, restoreFilePath }),
                    serverName);
            }

        } catch (SamFSException sfe) {
            LogUtil.error(sfe.getMessage());
            SamUtil.processException(sfe,
                                     getClass(),
                                     "handleSubmitRequest",
                                     "Unable to restore file",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "fs.restore.job.failure",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // recycle page
        forwardTo(getRequestContext());
    }

    private void createRestorePathFileChooserModel() {
        TraceUtil.trace3("Entering");

        if (restorePathModel == null)  {
            // Create a filechooser model
            restorePathModel =
                            new RemoteFileChooserModel(getServerName(), 50);
            restorePathModel.setFileListBoxHeight(15);
            restorePathModel.setHomeDirectory("/");
            restorePathModel.setProductNameAlt(
                "secondaryMasthead.productNameAlt");
            restorePathModel.setProductNameSrc(
                "secondaryMasthead.productNameSrc");
            restorePathModel.setPopupMode(true);
        }

        TraceUtil.trace3("Exiting");
    }


    protected String getFileToRestore() {
        String file = (String) getPageSessionAttribute(FILE_TO_RESTORE);

        if (file == null) {
            file = RequestManager.getRequest().getParameter(FILE_TO_RESTORE);
            setPageSessionAttribute(FILE_TO_RESTORE, file);
        }

        return file;
    }

    protected String getMountPoint() {
        String mountPoint = (String)getPageSessionAttribute(MOUNT_POINT);

        if (mountPoint == null) {
            mountPoint = RequestManager.getRequest().getParameter(MOUNT_POINT);
            setPageSessionAttribute(MOUNT_POINT, mountPoint);
        }

        return mountPoint;
    }

    protected String getFSName() {
        String fsName = (String)getPageSessionAttribute(FS_NAME);

        if (fsName == null) {
            fsName = RequestManager.getRequest().getParameter(FS_NAME);
            setPageSessionAttribute(FS_NAME, fsName);
        }

        return fsName;
    }

    protected String getFileName() {
        String fullPath   = getFileToRestore();
        String mountPoint = getMountPoint();

        // First check if users want to restore the entire recovery point.
        // If yes, set file name to an empty string
        if (ENTIRE_RECOVERY_POINT.equals(fullPath)) {
            return "";
        }

        // mount point cannot be "/" for restore case because root directory
        // cannot be a SAM-QFS file system.
        return fullPath.substring(mountPoint.length() + 1);
    }

    protected boolean isDirectory() {
        String isDirectory = (String)getPageSessionAttribute(IS_DIR);

        if (isDirectory == null) {
            isDirectory = RequestManager.getRequest().getParameter(IS_DIR);
            setPageSessionAttribute(IS_DIR, isDirectory);
        }

        return Boolean.valueOf(isDirectory).booleanValue();
    }

    protected String getRecoveryPoint() {
        String recoveryPoint =
            (String) getPageSessionAttribute(RECOVERY_POINT_PATH);

        if (recoveryPoint == null) {
            recoveryPoint =
                RequestManager.getRequest().getParameter(RECOVERY_POINT_PATH);
            setPageSessionAttribute(RECOVERY_POINT_PATH, recoveryPoint);
        }

        return recoveryPoint;
    }

    /**
     * Retrieve the copy information of the selected file.
     * There are only three things available in each of the FileCopyDetails
     * object.
     * 1. Copy Number
     * 2. Media Type
     * 3. Damaged state
     */
    protected FileCopyDetails [] getCopyDetails() throws SamFSException {
        String copyInfo = (String) getPageSessionAttribute(COPY_INFO);

        if (copyInfo == null) {
            copyInfo = RequestManager.getRequest().getParameter(COPY_INFO);
            setPageSessionAttribute(COPY_INFO, copyInfo);
        }

        // no copy
        if (copyInfo == null || copyInfo.length() == 0) {
            return new FileCopyDetails[0];
        }

        String [] copyInfoArray = copyInfo.split("###");
        FileCopyDetails [] copyDetails =
            new FileCopyDetails[copyInfoArray.length];
        for (int i = 0; i < copyDetails.length; i++) {
            copyDetails[i] = new FileCopyDetails(
                                ConversionUtil.strToProps(copyInfoArray[i]));
        }
        return copyDetails;
    }
}
