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

// ident	$Id: RecoveryPointsView.java,v 1.20 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.model.fs.RestoreDumpFile;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import com.sun.web.ui.view.filechooser.CCFileChooserWindow;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.util.GregorianCalendar;

/** the table container view */
public class RecoveryPointsView extends CommonTableContainerView {
    public static final String TILED_VIEW = "tiled_view";

    public static final String SS_RETENTION_BOX = "retentionBox";
    public static final String SS_RETENTION_VALUE  = "retentionValue";

    public static final String JOB_ID_HREF = "jobIdHref";
    public static final String RETAIN_HREF = "RetainHref";
    public static final String FILE_NAMES = "FileNames";
    public static final String SELECTED_FILE = "SelectedFile";

    // Create Recovery Point Now Button
    public static final
        String CREATE_RECOVERY_POINT_NOW = "CreateRecoveryPointNow";

    // the table model
    private CCActionTableModel model = null;

    /** default constructor */
    public RecoveryPointsView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "RecoveryPointsTable";

        model = createTableModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** register this container view's child views */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(TILED_VIEW, RecoveryPointsTiledView.class);
        registerChild(JOB_ID_HREF, CCHref.class);
        registerChild(RETAIN_HREF, CCHref.class);
        registerChild(FILE_NAMES, CCHiddenField.class);
        registerChild(SELECTED_FILE, CCHiddenField.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    /** create an instance of the named child view */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");

        if (name.equals(TILED_VIEW)) {
            return new RecoveryPointsTiledView(this, model, name);
        } else if (name.equals(JOB_ID_HREF) ||
            name.equals(RETAIN_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(FILE_NAMES) ||
            name.equals(SELECTED_FILE)) {
            return new CCHiddenField(this, name, null);
        } else {
            return super.createChild(model, name, TILED_VIEW);
        }
    }

    /** create the table model */
    private CCActionTableModel createTableModel() {
        model = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/RecoveryPointsTable.xml");

        // initialize headers
        model.setActionValue("colFileName", "fs.recoverypoints.name");
        model.setActionValue("colFileDate", "fs.recoverypoints.date");
        model.setActionValue("colFileSize", "fs.recoverypoints.size");
        model.setActionValue("colNumEntries", "fs.recoverypoints.entries");
        model.setActionValue("colIndexed", "fs.recoverypoints.indexed");
        model.setActionValue("colCompressed", "fs.recoverypoints.compressed");
        model.setActionValue("colRetention",  "fs.recoverypoints.permanent");

        // Sort by Date, most recent snapshot stays on top
        model.setPrimarySortName("fileDateHidden");
        model.setPrimarySortOrder(CCActionTableModelInterface.DESCENDING);

        return model;
    }

    public void clearTableModel() {
        model.clear();
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

    /** populate the table model */
    public void populateTableModel(boolean hasPermission) {
        TraceUtil.trace3("Entering");

        RecoveryPointsViewBean parent =
            (RecoveryPointsViewBean) getParentViewBean();

        String fsName = parent.getCurrentFSName();

        if (fsName == null || fsName.length() == 0) {
            model.setTitle(SamUtil.getResourceString(
                "fs.recoverypoints.tabletitle.blank"));
        } else {
            model.setTitle(SamUtil.getResourceString(
                "fs.recoverypoints.tabletitle",
                new String [] {parent.getCurrentFSName()}));
        }

        String serverName = parent.getServerName();

        CCButton createRPN = (CCButton) getChild(CREATE_RECOVERY_POINT_NOW);

        try {
            if (fsName == null || fsName.length() == 0) {
                // no file system is selected yet
                createRPN.setDisabled(true);
                return;
            } else {
                createRPN.setDisabled(!hasPermission);
            }

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemFSManager fsMgr =
                sysModel.getSamQFSSystemFSManager();

            String snapshotDirectory = getCurrentSnapshotPath();

            if (snapshotDirectory == null) {
                // No current path.  Use path from schedule.
                RecoveryPointSchedule mySchedule =
                    SamUtil.getRecoveryPointSchedule(sysModel, fsName);
                snapshotDirectory =
                    mySchedule == null ?
                        null: mySchedule.getLocation();
            }

            // Check if snapshotDirectory has been populated,
            // if not, use root of file system.
            if (snapshotDirectory == null) {
                snapshotDirectory =
                    fsMgr.getFileSystem(fsName).getMountPoint();
                ((CCDropDownMenu) parent.getChild(
                    parent.SNAPSHOT_PATH)).setValue(snapshotDirectory);
            } else {
                parent.setCurrentSnapShotPath(snapshotDirectory);
            }

            setFileChooser(snapshotDirectory);

            // Table rows
            RestoreDumpFile[] dumpFiles =
                fsMgr.getAvailableDumpFiles(fsName, snapshotDirectory);
            if (dumpFiles.length == 0) {
                return;
            }

            StringBuffer buf = new StringBuffer();
            for (int i = 0; i < dumpFiles.length; i++) {
                RestoreDumpFile dumpFile = dumpFiles[i];

                if (i > 0) {
                    model.appendRow();
                }

                String fileName = dumpFile.getFileName();
                // fileName contains the full path, which we do not want to show
                // the snapshot directory to the user.  Logic layer will
                // concatenate the directory path back to fully qualify form
                // when any of the table buttons are clicked.
                // Now, extract the snapshot directory from the file name
                if ("/".equals(snapshotDirectory)) {
                    fileName = fileName.substring(1);
                } else {
                    fileName =
                        fileName.substring(snapshotDirectory.length() + 1);
                }
                model.setValue("fileNameText", fileName);
                if (buf.length() > 0) {
                    buf.append("###");
                }
                buf.append(fileName);

                model.setValue(
                    "compressedText",
                    dumpFile.isCompressed() ?
                        SamUtil.getResourceString("samqfsui.yes") :
                        SamUtil.getResourceString("samqfsui.no"));

                model.setValue(
                    "indexedText",
                    dumpFile.isBroken() ?
                        SamUtil.getResourceString("fs.filebrowser.broken") :
                        dumpFile.isProcessing() ?
                            SamUtil.getResourceString(
                                "fs.filebrowser.processing") :
                            dumpFile.isIndexed() ?
                                SamUtil.getResourceString("samqfsui.yes") :
                                SamUtil.getResourceString("samqfsui.no"));
                model.setValue(
                    "indexed", Boolean.toString(dumpFile.isIndexed()));
                model.setValue(
                    "processingOrBroken",
                    Boolean.toString(
                        dumpFile.isProcessing() || dumpFile.isBroken()));

                GregorianCalendar modTime = dumpFile.getModTime();
                model.setValue(
                    "fileDateText",
                    SamUtil.getTimeString(modTime.getTime()));
                model.setValue(
                    "fileDateHidden", new Long(modTime.getTimeInMillis()));

                Capacity dumpFileSize = new Capacity(
                            ConversionUtil.strToLongVal(dumpFile.getSize()),
                            SamQFSSystemModel.SIZE_B);
                model.setValue("fileSizeText", dumpFileSize);

                model.setValue("numEntriesText", dumpFile.getNumEntries());

                model.setValue(
                    SS_RETENTION_VALUE,
                    Boolean.toString(dumpFile.isLocked()));
            }

            ((CCHiddenField) getChild(FILE_NAMES)).setValue(buf.toString());
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                getClass(),
                "populateTableModel",
                "Failed to retrieve recovery point content!",
                serverName);

            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                SamUtil.getResourceString("fs.recoverypoints.fail.populate"),
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

         if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.FILE_OPERATOR)) {
            // disable the radio button row selection column
            model.setSelectionType("none");
        } else {
            // disable selection radio buttons
            CCActionTable theTable =
                (CCActionTable)getChild(CHILD_ACTION_TABLE);
            ((CCRadioButton) theTable.
                getChild(CCActionTable.CHILD_ROW_SELECTION_RADIOBUTTON)).
                setTitle("");

            // clear any selection that might lingering around
            model.setRowSelected(false);
        }
        TraceUtil.trace3("Exiting");
    }

    private void setFileChooser(String snapshotDirectory) {
        RecoveryPointsViewBean vb =
            (RecoveryPointsViewBean) getParentViewBean();
        CCFileChooserWindow folderChooser =
            (CCFileChooserWindow) vb.getChild(vb.SELECT_SS_PATH);

        // set whatever is in the drop down as the default path
        folderChooser.setDisplayFieldValue(
            RemoteFileChooserControl.CHILD_PATH_HIDDEN, snapshotDirectory);
        ((CCDropDownMenu) vb.getChild(
            vb.SNAPSHOT_PATH)).setValue(snapshotDirectory);
    }

    // Request Handlers for Recovery Points Action Table
    public void handleCreateIndexRequest(RequestInvocationEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        handleButtons((short) 0);

        TraceUtil.trace3("Exiting");
    }

    public void handleDeleteIndexRequest(RequestInvocationEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        handleButtons((short) 1);

        TraceUtil.trace3("Exiting");
    }

    public void handleDeleteDumpRequest(RequestInvocationEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        handleButtons((short) 2);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler of all the buttons in the Recovery Points Tabke
     * button == 0 (Create Index)
     * button == 1 (Delete Index)
     * button == 2 (Delete entry)
     */
    private void handleButtons(short button) throws ModelControlException {
        RecoveryPointsViewBean parent =
            (RecoveryPointsViewBean) getParentViewBean();
        String serverName = parent.getServerName();
        String errorMessage = "", functionName = "";

        String fsName = parent.getCurrentFSName();
        String ssPath = parent.getCurrentSnapShotPath();
        String dumpFileName = getSelectedSnapshotFileName();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemFSManager fsMgr =
                sysModel.getSamQFSSystemFSManager();

            if (dumpFileName != null) {
                switch (button) {
                    case 0:
                        errorMessage  = "fs.recoverypoints.startindex.failed";
                        functionName  = "handleCreateIndexRequest";

                        LogUtil.info(
                            this.getClass(),
                            functionName,
                            new StringBuffer("Start creating index for ").
                                append(fsName).toString());
                        long jobId =
                            fsMgr.enableDumpFileForUse(
                                fsName, ssPath, dumpFileName);
                        LogUtil.info(
                            this.getClass(),
                            functionName,
                            new StringBuffer("Done creating index for ").
                                append(fsName).toString());
                        BaseJob job = null;
                        if (jobId > 0) {
                            // Job may be short lived and may already be done
                            job = sysModel.
                                getSamQFSSystemJobManager().getJobById(jobId);
                        }
                        if (job != null) {
                            String hrefHtml = getJobIdHrefTag(jobId,
                                                BaseJob.TYPE_ENABLE_DUMP_STR);
                            SamUtil.setInfoAlert(
                                parent,
                                parent.CHILD_COMMON_ALERT,
                                "success.summary",
                                SamUtil.getResourceString(
                                   "fs.recoverypoints.startindex.success",
                                   new String[] { dumpFileName,
                                                  String.valueOf(jobId),
                                                  hrefHtml }),
                                serverName);
                        } else {
                            SamUtil.setInfoAlert(
                                parent,
                                parent.CHILD_COMMON_ALERT,
                                "success.summary",
                                SamUtil.getResourceString(
                                   "fs.recoverypoints.doneindex.success",
                                   dumpFileName),
                                serverName);
                        }
                        break;

                    case 1:
                        errorMessage  = "fs.recoverypoints.deleteindex.failed";
                        functionName  = "handleDeleteIndexRequest";
                        LogUtil.info(
                            this.getClass(),
                            functionName,
                            new StringBuffer("Start deleting index for ").
                                append(fsName).toString());
                        fsMgr.cleanDump(fsName, ssPath, dumpFileName);
                        LogUtil.info(
                            this.getClass(),
                            functionName,
                            new StringBuffer("Done deleting index for ").
                                append(fsName).toString());
                        SamUtil.setInfoAlert(
                            parent,
                            parent.CHILD_COMMON_ALERT,
                            "success.summary",
                            SamUtil.getResourceString(
                               "fs.recoverypoints.deleteindex.success",
                               dumpFileName),
                            serverName);
                        break;

                    case 2:
                        errorMessage  = "fs.recoverypoints.deletedump.failed";
                        functionName  = "handleDeleteDumpRequest";
                        LogUtil.info(
                            this.getClass(),
                            functionName,
                            new StringBuffer(
                                "Start deleting recovery point for ").
                                append(fsName).toString());
                        fsMgr.deleteDump(fsName, ssPath, dumpFileName);
                        LogUtil.info(
                            this.getClass(),
                            functionName,
                            new StringBuffer(
                                "Done deleting recovery point for ").
                                append(fsName).toString());
                        SamUtil.setInfoAlert(
                            parent,
                            parent.CHILD_COMMON_ALERT,
                            "success.summary",
                            SamUtil.getResourceString(
                               "fs.recoverypoints.deletedump.success",
                               dumpFileName),
                            serverName);
                        break;
                }
            }
        } catch (SamFSException ex) {
            LogUtil.error(ex.getMessage());
            String error = SamUtil.getResourceString(
                                errorMessage, new String [] {dumpFileName});
            SamUtil.processException(
                ex,
                this.getClass(),
                functionName,
                error,
                serverName);
            TraceUtil.trace1(error.concat(" Reason: ").concat(ex.getMessage()));
            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                error,
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        // Refresh page
	parent.forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    private String getSelectedSnapshotFileName() throws ModelControlException {
        TraceUtil.trace3("Entering");

        String dumpFileName =
            (String) ((CCHiddenField) getChild(SELECTED_FILE)).getValue();
        TraceUtil.trace3("Exiting");
        return dumpFileName;
    }

    // This method generates the HTML for an href tag that will link to
    // the job details page of the given job id
    private String getJobIdHrefTag(long jobId, String jobType) {
        // href looks like this:
        // <a href="../fs/RecoveryPoints?RecoveryPoints.RecoveryPointsView.
        //  jobIdHref=123456,Job.jobType.x,curent"
        //  name="RecoveryPoints.RecoveryPointsView.jobIdHref"
        //  onclick="javascript:var f=document.RecoveryPointsForm;
        //   if (f != null) {f.action=this.href;f.submit();
        //   return false}">Click here to view job details</a>"

        StringBuffer href = new StringBuffer()
            .append("<a href=\"../fs/RecoveryPoints?")
            .append("RecoveryPoints.RecoveryPointsView.")
            .append(JOB_ID_HREF).append("=")
            .append(jobId).append(",")
            .append(jobType).append(",Current\" ")
            .append("name=\"RecoveryPoints.RecoveryPointsView.")
            .append(JOB_ID_HREF).append("\" ")
            .append("onclick=\"")
            .append("javascript:var f=document.RecoveryPointsForm;")
            .append("if (f != null) {f.action=this.href;f.submit();")
            .append("return false}\" > ")
            .append(SamUtil.getResourceString("fs.recoverypoints.jobIdLink"))
            .append("</a>");

        return href.toString();
    }

    /**
     * Job id alert link handler
     */
    public void handleJobIdHrefRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");

        RecoveryPointsViewBean parent =
            (RecoveryPointsViewBean) getParentViewBean();
        String jobID = getDisplayFieldStringValue(JOB_ID_HREF);
        parent.setPageSessionAttribute(
            Constants.PageSessionAttributes.JOB_ID, jobID);
        parent.setPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME,
            parent.getCurrentFSName());
        parent.setPageSessionAttribute(
            Constants.PageSessionAttributes.SNAPSHOT_PATH,
            parent.getCurrentSnapShotPath());

        // Copy page session attributes so that if they click on the
        // breadcrumb to come back the state of the page is restored.
        // These attributes are not the "sticky" kind with teh SAMQFS prefix.
        // parent.forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * handler when user clicks on the retain permanently checkbox
     */
    public void handleRetainHrefRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");

        RecoveryPointsViewBean parent =
            (RecoveryPointsViewBean) getParentViewBean();
        String ssPath = parent.getCurrentSnapShotPath();
        String serverName = parent.getServerName();

        String helper = (String) parent.getDisplayFieldValue(parent.HELPER);
        String [] helperArray = helper.split("###");

        // Set Data
        try {
            // Check Permission
            if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILE_OPERATOR)) {
                throw new SamFSException("common.nopermission");
            }

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemFSManager fsMgr =
                sysModel.getSamQFSSystemFSManager();

            fsMgr.setIsDumpRetainedPermanently(
                parent.getCurrentFSName(),
                parent.getCurrentSnapShotPath(),
                helperArray[0],
                Boolean.valueOf(helperArray[1]).booleanValue());

            String alertMsg = "";
            if (Boolean.valueOf(helperArray[1]).booleanValue()) {
                alertMsg =
                    SamUtil.getResourceString(
                        "fs.recoverypoints.success.retainedPermanently",
                        helperArray[0]);
            } else {
                alertMsg =
                    SamUtil.getResourceString(
                        "fs.recoverypoints.success.notRetainedPermanently",
                        helperArray[0]);
            }
            SamUtil.setInfoAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "success.summary",
                alertMsg,
                serverName);

        } catch (SamFSException e) {
            SamUtil.processException(
                e,
                this.getClass(),
                "handleRetainHrefRequest()",
                "Failed to set the delete permanently value",
                serverName);
            TraceUtil.trace1(
                "Exception setting retain permanently:  " + e.getMessage());
            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "fs.recoverypoints.error.retainPermanently",
                e.getSAMerrno(),
                e.getMessage(),
                serverName);
        }

        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * This helper method retrieves the current snapshot directory which the
     * user is currently browsing on.  It has built in checks to see if the
     * snapshot directory still exists in the snapShot drop down menu in the
     * ViewBean.
     * @return the selected snapshot directory that is shown on the page
     */
    private String getCurrentSnapshotPath() {
	RecoveryPointsViewBean parent =
            (RecoveryPointsViewBean) getParentViewBean();
	String currentPath = (String) parent.getCurrentSnapShotPath();

	// if currentPath is null (no snapShot path is set, simply return.
	if (null == currentPath) {
	    return null;
	}

	// Check to see if the currentPath value still exists in the Recovery
	// Points drop down menu.  For directories that do not have an indexed
	// recovery points, we no longer show the directories in the drop down
	// menu.  In cases where user delete indexes in a particular directory
	// that does not have at least one indexed recovery point, we need to
	// reset the current snapshot path back to the default path to avoid
	// discrepancies shown on the page.
	CCDropDownMenu menu =
	    (CCDropDownMenu) parent.getChild(parent.SNAPSHOT_PATH);
	OptionList optionList = (OptionList) menu.getOptions();
	String valueLabel = optionList.getValueLabel(currentPath);
	if (null == valueLabel) {
	    // the currentPath no longer exists, return null so the default
	    // snapshot directory will be used.
	    TraceUtil.trace3(
		"getCurrentSnapshotPath: currentPath no longer exists! " +
		currentPath);
	    return null;
	} else {
	    // currentPath still exists, use this value in the menu
	    return currentPath;
	}
    }
}
