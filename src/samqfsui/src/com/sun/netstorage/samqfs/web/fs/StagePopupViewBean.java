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

// ident	$Id: StagePopupViewBean.java,v 1.18 2008/05/16 18:38:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.html.OptionList;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.web.model.fs.FileCopyDetails;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import java.io.File;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * This is the view bean of the stage pop up window.  This pop up window is
 * launched from the File Browser.
 */
public class StagePopupViewBean extends CommonSecondaryViewBeanBase {
    // page name & default url
    public static final String PAGE_NAME = "StagePopup";
    public static final String DEFAULT_URL = "/jsp/fs/StagePopup.jsp";

    // children
    public static final String PAGE_TITLE = "pageTitle";
    public static final String FILE_LABEL = "fileLabel";
    public static final String FILE = "file";
    public static final String STAGE_FROM_LABEL = "stageFromLabel";
    public static final String STAGE_FROM = "stageFrom";
    public static final String RECURSIVE = "recursive";

    public static final String FILE_TO_STAGE = "filetostage";
    public static final String IS_DIR = "isdir";
    public static final String FS_NAME = "fsname";
    public static final String MOUNT_POINT = "mountpoint";
    public static final String RECOVERY_POINT_PATH = "snappath";

    private CCPageTitleModel ptModel = null;

    public StagePopupViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(FILE_LABEL, CCLabel.class);
        registerChild(FILE, CCStaticTextField.class);
        registerChild(STAGE_FROM_LABEL, CCLabel.class);
        registerChild(STAGE_FROM, CCDropDownMenu.class);
        registerChild(RECURSIVE, CCCheckBox.class);
        ptModel.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(FILE_LABEL) ||
            name.equals(STAGE_FROM_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(FILE)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(STAGE_FROM)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(RECURSIVE)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        return PageTitleUtil.createModel("/jsp/fs/StagePageTitle.xml");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(evt);

        String serverName = getServerName();
        String file = getFileToStage();

        // set the file name
        ((CCStaticTextField)getChild(FILE)).setValue(file);
        try {
            FileCopyDetails [] copyDetails;

            if (isDirectory()) {
                copyDetails = new FileCopyDetails[0];
            } else {
                copyDetails = getCopyDetails();
            }

            // init 'stage from dropdown'
            String [] labels = new String[1 + copyDetails.length];
            String [] values = new String[1 + copyDetails.length];

            labels[0] = "fs.stage.stagefrom.systempicks";
            values[0] = Integer.toString(0);

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
                labels[i + 1] = buf.toString();
                values[i + 1] = Integer.toString(copyNumber);
            }

            ((CCDropDownMenu)getChild(STAGE_FROM)).
                setOptions(new OptionList(labels, values));
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "begindDisplay",
                                     "unable to load file information",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                   ALERT,
                                   "fs.stage.error.files",
                                   sfe.getSAMerrno(),
                                   sfe.getMessage(),
                                   serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleSubmitRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        // retrieve all the parameters
        String serverName = getServerName();
        String copyNumber = getDisplayFieldStringValue(STAGE_FROM);
        String file = getFileToStage();

        try {
	    SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemFSManager fsManager =
                sysModel.getSamQFSSystemFSManager();

            int recurse = isDirectory() ? Stager.RECURSIVE : 0;
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new StringBuffer("Start staging file ").
                    append(file).toString());
            long jobID = fsManager.stageFiles(Integer.parseInt(copyNumber),
                                 new String [] {file},
                                 recurse);
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new StringBuffer("Done staging file ").
                    append(file).toString());

            BaseJob job = null;
            if (jobID > 0) {
                // Job may have been very short lived and may already be done.
                job = sysModel.getSamQFSSystemJobManager().getJobById(jobID);
            }

            SamUtil.setInfoAlert(
                this,
                ALERT,
                "success.summary",
                jobID <= 0 ?
                    SamUtil.getResourceString(
                        "fs.stage.job.submitted",
                        file) :
                    SamUtil.getResourceString(
                        "fs.stage.job.submittedwithjobid",
                        new String [] {
                            file,
                            String.valueOf(jobID)}),
                        serverName);

            setSubmitSuccessful(true);
        } catch (SamFSException sfe) {
            LogUtil.error(sfe.getMessage());
            SamUtil.processException(sfe,
                                     getClass(),
                                     "handleSubmitRequest",
                                     "unable to stage file",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "fs.stage.job.failure",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // recycle page
        forwardTo(getRequestContext());
    }

    /**
     * Retrieve the copy information of the selected file.
     * There are only three things available in each of the FileCopyDetails
     * object.
     */
    private FileCopyDetails [] getCopyDetails() throws SamFSException {
        SamQFSSystemFSManager fsManager =
            SamUtil.getModel(getServerName()).getSamQFSSystemFSManager();
        StageFile stageFile =
            getSelectedStageFile(fsManager, getFSName(), getFileToStage());

        return stageFile.getFileCopyDetails();
    }

    private StageFile getSelectedStageFile(
        SamQFSSystemFSManager fsManager, String fsName, String fileNameWithPath)
        throws SamFSException {

        String mountPoint = getMountPoint();

        String relativePathName;
        if (File.separator.equals(mountPoint)) {
            relativePathName = fileNameWithPath.substring(mountPoint.length());
        } else {
            relativePathName =
                fileNameWithPath.substring(mountPoint.length() + 1);
        }

        int theseDetails = FileUtil.FNAME
                         | FileUtil.FILE_TYPE
                         | FileUtil.SIZE
                         | FileUtil.CREATED
                         | FileUtil.MODIFIED
                         | FileUtil.ACCESSED
                         | FileUtil.CHAR_MODE
                         | FileUtil.SAM_STATE
                         | FileUtil.COPY_DETAIL;

        return fsManager.getFileInformation(
                    fsName,
                    getRecoveryPoint(), // snapPath
                    relativePathName,
                    theseDetails);
    }

    protected String getFileToStage() {
        String file = (String)getPageSessionAttribute(FILE_TO_STAGE);

        if (file == null) {
            file = RequestManager.getRequest().getParameter(FILE_TO_STAGE);
            setPageSessionAttribute(FILE_TO_STAGE, file);
        }

        return file;
    }

    protected boolean isDirectory() {
        String isDirectory = (String)getPageSessionAttribute(IS_DIR);

        if (isDirectory == null) {
            isDirectory = RequestManager.getRequest().getParameter(IS_DIR);
            setPageSessionAttribute(IS_DIR, isDirectory);
        }

        return Boolean.valueOf(isDirectory).booleanValue();
    }

    public String getFSName() {
        String fsName = (String) getPageSessionAttribute(FS_NAME);

        if (fsName == null) {
            fsName =
                RequestManager.getRequest().getParameter(FS_NAME);
            setPageSessionAttribute(FS_NAME, fsName);
        }

        return fsName;
    }

    private String getMountPoint() {
        String mountPoint = (String) getPageSessionAttribute(MOUNT_POINT);

        if (mountPoint == null) {
            mountPoint =
                RequestManager.getRequest().getParameter(MOUNT_POINT);
            setPageSessionAttribute(MOUNT_POINT, mountPoint);
        }
        return mountPoint;
    }

    /**
     * getRecoveryPoint returns null in live mode
     */
    protected String getRecoveryPoint() {
        String recoveryPoint =
            (String) getPageSessionAttribute(RECOVERY_POINT_PATH);

        if (recoveryPoint == null) {
            recoveryPoint =
                RequestManager.getRequest().getParameter(RECOVERY_POINT_PATH);
            setPageSessionAttribute(RECOVERY_POINT_PATH, recoveryPoint);
        }
        return recoveryPoint == null ? "" : recoveryPoint;
    }
}
