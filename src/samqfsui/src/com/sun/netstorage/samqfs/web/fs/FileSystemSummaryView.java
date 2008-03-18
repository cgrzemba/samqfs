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

// ident	$Id: FileSystemSummaryView.java,v 1.90 2008/03/17 14:43:34 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.ModelManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.archive.ArchiveActivityViewBean;
import com.sun.netstorage.samqfs.web.archive.wizards.NewPolicyWizardImpl;
import com.sun.netstorage.samqfs.web.fs.wizards.CreateFSWizardImpl;
import com.sun.netstorage.samqfs.web.fs.wizards.GrowWizardImpl;
import com.sun.netstorage.samqfs.web.jobs.JobsDetailsViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
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
import com.sun.netstorage.samqfs.web.wizard.WizardModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import java.util.ArrayList;
import javax.servlet.ServletException;

/**
 *  This class is the container view of File Systems Summary page
 */

public class FileSystemSummaryView extends CommonTableContainerView {

    public static final String CHILD_ACTIONMENU = "ActionMenu";
    public static final String CHILD_ACTIONMENU_HREF = "ActionMenuHref";
    public static final String CHILD_FILTERMENU_HREF = "FilterMenuHref";

    private String selectedFS = null;
    private FileSystemSummaryModel model = null;
    private ArrayList updatedModelIndex = new ArrayList();

    // Child View names associated with Samfsck pop up
    public static final String CHILD_HIDDEN_FIELD1 = "SamfsckHiddenField1";
    public static final String CHILD_HIDDEN_FIELD2 = "SamfsckHiddenField2";
    public static final String CHILD_SAMFSCK_HREF  = "SamfsckHref";
    public static final String CHILD_CANCEL_HREF   = "CancelHref";

    // For clicking on job link in alert after samfschk
    public static final String CHILD_JOBID_HREF = "JobIdHref";

    // Child View names associated with Grow pop up
    public static final String CHILD_HIDDEN_FIELD3 = "GrowHiddenField";
    public static final String CHILD_FSNAME_HIDDEN = "FSNAMEHiddenField";

    public static final
        String CHILD_TILED_VIEW = "FileSystemSummaryTiledView";
    public static final String GROW_BUTTON = "SamQFSWizardGrowFSButton";

    // for wizard
    public static final String CHILD_NEWFRWD_TO_CMDCHILD = "newforwardToVb";
    public static final String CHILD_ARFRWD_TO_CMDCHILD  = "archiveforwardToVb";
    public static final String CHILD_FRWD_TO_CMDCHILD    = "forwardToVb";

    private boolean wizardNewLaunched = false;
    private CCWizardWindowModel newWizWinModel;
    private CCWizardWindowModel growWizWinModel;
    private CCWizardWindowModel archiveWizWinModel;

    public static final int SETUP_QFS = 0;
    public static final int SETUP_SAM = 1;
    private int systemSetup = -1;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FileSystemSummaryView(View parent, String name, String serverName) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // Figure out if it is a SAM setup
        systemSetup =
            SamUtil.getSystemType(serverName) == SamQFSSystemModel.QFS ?
                SETUP_QFS : SETUP_SAM;

        TraceUtil.trace3(new StringBuffer(
            "serverName = ").append(serverName).toString());
        TraceUtil.trace3(new StringBuffer(
            "systemSetup = ").append(systemSetup).toString());

        String xmlSchema;
        switch (systemSetup) {
            case SETUP_QFS:
                xmlSchema = "/jsp/fs/QFSSummaryTable.xml";
                break;
            default:
                xmlSchema = "/jsp/fs/FSSummaryTable.xml";
                break;
        }

        model = new FileSystemSummaryModel(systemSetup, xmlSchema);
        CHILD_ACTION_TABLE = "FileSystemSummaryTable";

        initializeNewWizard();
        initializeGrowWizard();
        if (systemSetup != SETUP_QFS) {
            initializeArchiveWizard();
        }

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_FILTERMENU_HREF, CCHref.class);
        registerChild(CHILD_ACTIONMENU_HREF, CCHref.class);
        registerChild(CHILD_CANCEL_HREF, CCHref.class);
        registerChild(CHILD_SAMFSCK_HREF, CCHref.class);
        registerChild(CHILD_JOBID_HREF, CCHref.class);
        registerChild(CHILD_HIDDEN_FIELD1, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FIELD2, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FIELD3, CCHiddenField.class);
        registerChild(CHILD_FSNAME_HIDDEN, CCHiddenField.class);
        registerChild(CHILD_TILED_VIEW, FileSystemSummaryTiledView.class);
        registerChild(CHILD_FRWD_TO_CMDCHILD, BasicCommandField.class);
        registerChild(CHILD_NEWFRWD_TO_CMDCHILD, BasicCommandField.class);
        registerChild(CHILD_ARFRWD_TO_CMDCHILD, BasicCommandField.class);
        registerChild(GROW_BUTTON, CCWizardWindow.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create the child
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (name.equals(CHILD_FILTERMENU_HREF) ||
            name.equals(CHILD_ACTIONMENU_HREF) ||
            name.equals(CHILD_CANCEL_HREF) ||
            name.equals(CHILD_SAMFSCK_HREF) ||
            name.equals(CHILD_JOBID_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_FIELD1) ||
                   name.equals(CHILD_HIDDEN_FIELD2) ||
                   name.equals(CHILD_FSNAME_HIDDEN) ||
                   name.equals(CHILD_HIDDEN_FIELD3)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            // The ContainerView used to register and create children
            // for "column" elements defined in the model's XML
            // document. Note that we're creating a seperate
            // ContainerView only to evoke JATO's TiledView behavior
            // for these elements. If TiledView behavior is not
            // required, creating a ContainerView object is not
            // necessary. By default, CCActionTable will attempt to
            // retrieve all children from it's parent (i.e., this view).
            FileSystemSummaryTiledView child = new FileSystemSummaryTiledView(
                this, model, name, getServerName());
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_FRWD_TO_CMDCHILD) ||
                   name.equals(CHILD_NEWFRWD_TO_CMDCHILD) ||
                   name.equals(CHILD_ARFRWD_TO_CMDCHILD)) {
            BasicCommandField bcf = new BasicCommandField(this, name);
            TraceUtil.trace3("Exiting");
            return bcf;
        } else if (name.equals(GROW_BUTTON)) {
            TraceUtil.trace3("Exiting");
            return new CCWizardWindow(this, growWizWinModel, name);
        } else {
            TraceUtil.trace3("Exiting");
            return super.createChild(model, name, CHILD_TILED_VIEW);
        }
    }

    /**
     * Handle request to handle the filter drop-down menu
     *  @param RequestInvocationEvent event
     */
    public void handleFilterMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        String selectedType = (String) getDisplayFieldValue("FilterMenu");
        TraceUtil.trace3(new StringBuffer(
            "FILTER Menu value is ").append(selectedType).toString());

        if (selectedType != null) {
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.FS_FILTER_MENU, selectedType);
        } else {
            getParentViewBean().removePageSessionAttribute(
                Constants.PageSessionAttributes.FS_FILTER_MENU);
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for action drop-down menu
     *  @param RequestInvocationEvent event
     */
    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        ViewBean vb = getParentViewBean();
        String processMsg = null, summary = null, op = null;
        int option = getDropDownMenuValue();
        String fsName = getSelectedFileSystemName();

        try {
            GenericFileSystem fileSystem = getSelectedFileSystem();

            switch (option) {

                case FileSystemSummaryModel.EDIT_MOUNT_OPTION:
                    ViewBean mountVB =
                        getMountViewBean((FileSystem) fileSystem);
                    BreadCrumbUtil.breadCrumbPathForward(
                        vb,
                        PageInfo.getPageInfo().getPageNumber(vb.getName()));
                    ((CommonViewBeanBase) vb).forwardTo(mountVB);
                    return;

                case FileSystemSummaryModel.MOUNT:
                    op = "FSSummary.mountfs";
                    summary    = "FSSummary.error.mount";
                    processMsg = "Failed to mount filesystem";

                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Start mounting filesystem ").
                            append(fsName).toString());
                    fileSystem.mount();

                    if (fileSystem.getState() == FileSystem.UNMOUNTED) {
                         throw new SamFSException(
                            "FSSummary.warning.mount.cause", -1099);
                    } else {
                        LogUtil.info(
                            this.getClass(),
                            "handleActionMenuHrefRequest",
                            new StringBuffer("Done mounting filesystem ").
                                append(fsName).toString());
                    }
                    break;

                case FileSystemSummaryModel.UNMOUNT:
                    op = "FSSummary.umountfs";
                    summary    = "FSSummary.error.umount";
                    processMsg = "Failed to unmount filesystem";

                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer(
                            "Start unmounting filesystem ").
                            append(fsName).toString());
                    fileSystem.unmount();
                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer(
                            "Done unmounting filesystem ").
                            append(fsName).toString());
                    break;

                case FileSystemSummaryModel.DELETE:
                    op = "FSSummary.deletefs";
                    summary    = "FSSummary.error.delete";
                    processMsg = "Failed to delete filesystem";

                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Start deleting filesystem ").
                            append(fsName).toString());
                    if (fileSystem.getFSTypeByProduct() ==
                            GenericFileSystem.FS_NONSAMQ ||
                        ((FileSystem)fileSystem).getShareStatus() ==
                            FileSystem.UNSHARED) {
                        SamUtil.getModel(getServerName()).
                            getSamQFSSystemFSManager().
                            deleteFileSystem(fileSystem);
                    } else {
                        SamQFSAppModel appModel =
                            SamQFSFactory.getSamQFSAppModel();
                        SamQFSSystemSharedFSManager sharedFSManager =
                            appModel.getSamQFSSystemSharedFSManager();
                        TraceUtil.trace3(new StringBuffer(
                            "begin shared deleting on host").append(
                            getServerName()).append("file:").append(fsName).
                            toString());
                        sharedFSManager.deleteSharedFileSystem(
                            getServerName(), fsName, null);
                        // if there is no error occurred in the calls above,
                        // sleep for 5 seconds, give time for underlying
                        // call to be completed
                        try {
                            FileSystem fileSystem1 =
                                SamUtil.getModel(
                                    getServerName()).
                                    getSamQFSSystemFSManager().
                                    getFileSystem(fsName);
                            if (fileSystem1 != null) {  // still not deleted
                                try {
                                    Thread.sleep(5000);
                                } catch (InterruptedException intEx) {
                                // impossible for other thread to
                                // interrupt this
                                // thread Continue to load the page
                                    TraceUtil.trace3(
                                        "InterruptedException: Reason: " +
                                        intEx.getMessage());
                                }
                            }
                        } catch (SamFSException e) {
                            TraceUtil.trace3("an exception on deleting");
                            if (e.getSAMerrno() != e.NOT_FOUND) {
                                throw (e);
                            }
                        }
                    }
                    LogUtil.info(
                        this.getClass(),
                        "handleActionMenuHrefRequest",
                        new StringBuffer("Done deleting filesystem ").
                            append(fsName).toString());

                    break;

                case FileSystemSummaryModel.ARCHIVE_ACTIVITIES:
                    ViewBean activityVB =
                        getViewBean(ArchiveActivityViewBean.class);
                    vb.setPageSessionAttribute(
                        Constants.PageSessionAttributes.FS_NAME,
                        fileSystem.getName());
                    BreadCrumbUtil.breadCrumbPathForward(
                        vb,
                        PageInfo.getPageInfo().getPageNumber(vb.getName()));
                    ((CommonViewBeanBase) vb).forwardTo(activityVB);
                    return;

                case FileSystemSummaryModel.SCHEDULE_DUMP:
                    ViewBean scheduleDumpVB =
                        getViewBean(RecoveryPointScheduleViewBean.class);
                    vb.setPageSessionAttribute(Constants.admin.TASK_ID,
                                       ScheduleTaskID.SNAPSHOT.getId());
                    vb.setPageSessionAttribute(
                        Constants.admin.TASK_NAME, fileSystem.getName());
                    BreadCrumbUtil.breadCrumbPathForward(
                        vb,
                        PageInfo.getPageInfo().getPageNumber(vb.getName()));
                    ((CommonViewBeanBase) vb).forwardTo(scheduleDumpVB);
                    return;

                default:
                    // Refresh if label is selected.
                    // Check-FS and Take Snapshots are pop-ups.
                    break;
            }

            // If code reaches here, operation issued successfully
            if (option != FileSystemSummaryModel.OPERATIONS_LABEL &&
                option != FileSystemSummaryModel.CHECK_FS) {
                showAlert(op, getSelectedFileSystemName());
            }

        } catch (SamFSMultiHostException multiHostEx) {
            SamUtil.doPrint("SamFSMultiHostException caught!");
            SamUtil.doPrint(new StringBuffer("error code is ").
                append(multiHostEx.getSAMerrno()).toString());
            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                summary,
                multiHostEx.getSAMerrno(),
                SamUtil.handleMultiHostException(multiHostEx),
                getServerName());

        } catch (SamFSMultiMsgException multiMsgEx) {
            // need to see if a SamFSMultiMsgException occurred
            // when attempting to delete a filesystem
            SamUtil.doPrint("SamFSMultiMsgException caught!");
            SamUtil.doPrint(new StringBuffer("error code is ").
                append(multiMsgEx.getSAMerrno()).toString());

            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error",
                multiMsgEx.getSAMerrno(),
                "ArchiveConfig.error.detail",
                getServerName());

        } catch (SamFSWarnings warning) {
            SamUtil.doPrint("SamFSWarnings caught!");
            SamUtil.setWarningAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error",
                "ArchiveConfig.warning.detail");

        } catch (SamFSException smfex) {
            SamUtil.doPrint("SamFSException caught!");
            SamUtil.doPrint(new StringBuffer("error code is ").
                append(smfex.getSAMerrno()).toString());
            SamUtil.processException(
                smfex,
                this.getClass(),
                "handleActionMenuHrefRequest()",
                processMsg,
                getServerName());
            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                summary,
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception e) {
            TraceUtil.trace1("General Exception caught!");
            TraceUtil.trace1("Reason: " + e.getMessage());
            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "error.unexpected",
                SamFSException.DEFAULT_ERROR_NO,
                e.getMessage(),
                getServerName());
        }

        vb.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     *  Handler function for new Button
     *  @param RequestInvocationEvent event
     */
    public void handleSamQFSWizardNewFSButtonRequest
        (RequestInvocationEvent event)
            throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        wizardNewLaunched = true;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Existing");
    }

    private ViewBean getMountViewBean(FileSystem fs)
        throws SamFSException {

        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        ViewBean targetView  = getViewBean(FSMountViewBean.class);
        int fsType = fs.getFSTypeByProduct();

        String mountPageType = null;

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
                getParentViewBean().setPageSessionAttribute(
                    Constants.SessionAttributes.SHARED_CLIENT_HOST,
                    Constants.PageSessionAttributes.SHARED_HOST_NONSHARED);
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.ARCHIVE_TYPE,
                    new Integer(fs.getArchivingType()));
                break;
            case FileSystem.SHARED_TYPE_MDS:
            case FileSystem.SHARED_TYPE_PMDS:
                mountPageType = (fsType == FileSystem.FS_SAMQFS) ?
                    FSMountViewBean.TYPE_SHAREDSAMQFS :
                    FSMountViewBean.TYPE_SHAREDQFS;
                getParentViewBean().setPageSessionAttribute(
                        Constants.SessionAttributes.SHARED_CLIENT_HOST,
                        Constants.PageSessionAttributes.SHARED_HOST_LOCAL);
                break;
            default:
                mountPageType = (fsType == FileSystem.FS_SAMQFS) ?
                    FSMountViewBean.TYPE_SHAREDSAMQFS :
                    FSMountViewBean.TYPE_SHAREDQFS;
                getParentViewBean().setPageSessionAttribute(
                        Constants.SessionAttributes.SHARED_CLIENT_HOST,
                        Constants.PageSessionAttributes.SHARED_HOST_CLIENT);
                break;
        }

        getParentViewBean().setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME, fs.getName());
        getParentViewBean().setPageSessionAttribute(
            Constants.SessionAttributes.MOUNT_PAGE_TYPE, mountPageType);

        return targetView;
    }

    /**
     * Handler function for View Archive button
     * @param RequestInvocationEvent event
     */
    public void handleViewPolicyButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        ViewBean targetView = getViewBean(FSArchivePoliciesViewBean.class);
        getParentViewBean().setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME,
            getSelectedFileSystemName());
        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().
                getPageNumber(getParentViewBean().getName()));

        ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler function for View Files button
     * @param RequestInvocationEvent event
     */
    public void handleViewFilesButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        try {
            String mountPoint = getSelectedFileSystem().getMountPoint();
            if (mountPoint == null || mountPoint.length() == 0) {
                throw new SamFSException(
                    "FSSummary.error.failedDetermineMountPoint");
            }
            ViewBean targetView = getViewBean(FileBrowserViewBean.class);
            targetView.setPageSessionAttribute(
                FileBrowserViewBean.CURRENT_DIRECTORY, mountPoint);
            ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
            TraceUtil.trace3("Exiting");
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to retrieve selected fs mount point!");
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleViewFilesButtonRequest()",
                "Failed to retrieve selected fs mount point!",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSSummary.error.failedDetermineMountPoint",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
            TraceUtil.trace3("Exiting with error");
            getParentViewBean().forwardTo(getRequestContext());
        }
    }

    /**
     * Handler function for Grow wizard button
     * @param RequestInvocationEvent event
     */
    public void handleSamQFSWizardGrowFSButtonRequest
        (RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        // Wizard
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler function for New Archive Policy Button
     * @param RequestInvocationEvent event
     */
    public void handleSamQFSWizardNewPolicyButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild(CHILD_ACTIONMENU)).setValue("0");

        // disable selection radio buttons
        CCActionTable theTable = (CCActionTable)getChild(CHILD_ACTION_TABLE);
        ((CCRadioButton) theTable.
            getChild(CCActionTable.CHILD_ROW_SELECTION_RADIOBUTTON)).
            setTitle("");

        model.setTitle(
            SamUtil.getResourceString("FSSummary.pageTitle1", getServerName()));

        model.setRowSelected(false);

        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
            ((CCWizardWindow)getChild("SamQFSWizardNewFSButton"))
                .setDisabled(wizardNewLaunched);
        } else {
            ((CCWizardWindow)getChild("SamQFSWizardNewFSButton"))
                .setDisabled(true);
            model.setSelectionType("none");
        }
        setNewWizardNames();
        setGrowWizardNames();

        // No Archive concept in QFS
        if (systemSetup != SETUP_QFS) {
                setArchiveWizardNames();
        }

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
     * Set up the data model and create the model for new wizard button
     */
    private void initializeNewWizard() {
        TraceUtil.trace3("Entering");
        ViewBean view = getParentViewBean();

        StringBuffer cmdChild =
            new StringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("FileSystemSummaryView.").
                append(CHILD_NEWFRWD_TO_CMDCHILD);
        newWizWinModel = CreateFSWizardImpl.createModel(cmdChild.toString());

        // set wizard window model with AT
        model.setModel("SamQFSWizardNewFSButton", newWizWinModel);
        newWizWinModel.setValue("SamQFSWizardNewFSButton",
                                "FSSummary.NewFSButton");

        TraceUtil.trace3("Exiting");
    }

    /**
     * Set up the data model and create the model for grow wizard button
     */
    private void initializeGrowWizard() {
        TraceUtil.trace3("Entering");
        ViewBean view = getParentViewBean();

        StringBuffer cmdChild =
            new StringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("FileSystemSummaryView.").
                append(CHILD_FRWD_TO_CMDCHILD);
        growWizWinModel = GrowWizardImpl.createModel(cmdChild.toString());

        model.setModel(GROW_BUTTON, growWizWinModel);
        growWizWinModel.setValue(GROW_BUTTON, "FSSummary.GrowFSButton");

        TraceUtil.trace3("Exiting");
    }


    /**
     * Set up the data model and create the model for new archive wizard button
     */
    private void initializeArchiveWizard() {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase  view = (CommonViewBeanBase)getParentViewBean();
        StringBuffer cmdChild =
            new StringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("FileSystemSummaryView.").
                append(CHILD_ARFRWD_TO_CMDCHILD);
        archiveWizWinModel =
            NewPolicyWizardImpl.createModel(cmdChild.toString());
        // set wizard window model with AT
        model.setModel("SamQFSWizardNewPolicyButton", archiveWizWinModel);
        archiveWizWinModel.setValue(
            "SamQFSWizardNewPolicyButton", "FSSummary.NewArchivePolicyButton");
        archiveWizWinModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            view.getServerName());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to get the selected row index
     */
    private int getSelectedRowIndex() throws ModelControlException {
        TraceUtil.trace3("Entering");
        int index = -1;
        CCActionTable child = (CCActionTable)getChild(CHILD_ACTION_TABLE);
        child.restoreStateData();
        Integer [] selectedRows = model.getSelectedRows();
        index = selectedRows[0].intValue();
        TraceUtil.trace3("Exiting");
        return index;
    }

    private String getSelectedValue(int index) {
        TraceUtil.trace3("Entering");
        String value = null;
        String filter = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_FILTER_MENU);
        model.setRowIndex(index);
        value = (String) model.getValue("FSHiddenField");
        TraceUtil.trace3("Exiting");
        return value;
    }

    private String getSelectedFSType(int index) {
        TraceUtil.trace3("Entering");
        String value = null;
        String filter = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_FILTER_MENU);
        if (filter == null) {
            model.setRowIndex(index);
            value = (String) model.getValue("HiddenType");
        } else {
            model.setRowIndex(
                ((Integer) updatedModelIndex.get(index)).intValue());
            value = (String) model.getValue("HiddenType");
        }
        TraceUtil.trace3("Exiting");
        return value;
    }

    private void handleFilter() {
        TraceUtil.trace3("Entering");
        String filter = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_FILTER_MENU);
        if (filter != null && filter.length() != 0) {
            ((CCDropDownMenu)
                getChild("FilterMenu")).setValue(filter);
            model.setFilter(filter);
        }

        try {
            model.initModelRows(getServerName());
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "handleFilter()",
                "Unable to populate file system",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSSummary.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());
        }
        updatedModelIndex = model.getLatestIndex();
        TraceUtil.trace3("Exiting");
    }

    public void _handleSamfsckHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        boolean checkAndRepair = false;

        String type = (String) getDisplayFieldValue(CHILD_HIDDEN_FIELD1);
        String loc  = (String) getDisplayFieldValue(CHILD_HIDDEN_FIELD2);

        TraceUtil.trace3(new StringBuffer("FSCK: type is ").
            append(type).append(", loc is ").append(loc).toString());

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            String fsName = getSelectedFileSystemName();
            GenericFileSystem fileSystem = getSelectedFileSystem();

            // check catalog file existing
            if (sysModel.doesFileExist(loc)) {
                throw new SamFSException(null, -1009);
            }

            if (type.equals("both")) {
                checkAndRepair = true;
            }
            LogUtil.info(
                this.getClass(),
                "handleSamfsckHrefRequest",
                "Start samfsck filesystem " + fsName);

            ((FileSystem) fileSystem).setFsckLogfileLocation(loc);
            long jobID = ((FileSystem) fileSystem).samfsck(checkAndRepair, loc);

            LogUtil.info(
                this.getClass(),
                "handleSamfsckHrefRequest",
                "Done samfsck filesystem " + fsName +
                "with Job ID " + jobID);

            if (jobID < 0) {
                SamUtil.setInfoAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.samfsckresult",
                        new String[] {fsName, loc}),
                    getServerName());
            } else {
                // Build href object...
                // href looks like this:
                // <a href="../fs/FSSummary?FSSummary.FileSystemSummaryView.
                //    JobIdHref=123456"
                // name="FSSummary.FileSystemSummaryView.JobIdHref"
                // onclick="
                //   javascript:var f=document.FSSummaryForm;
                //   if (f != null) {f.action=this.href;f.submit();
                //   return false}">123456</a>"

                StringBuffer href = new StringBuffer()
                    .append("<a href=\"../fs/FSSummary?")
                    .append("FSSummary.FileSystemSummaryView.JobIdHref=")
                    .append(jobID).append(",")
                    .append(Constants.Jobs.JOB_TYPE_SAMFSCK).append(",Current")
                    .append("\" ")
                    .append("name=\"FSSummary.FileSystemSummaryView.")
                    .append(CHILD_JOBID_HREF).append("\" ")
                    .append("onclick=\"")
                    .append("javascript:var f=document.FSSummaryForm;")
                    .append("if (f != null) {f.action=this.href;f.submit();")
                    .append("return false}\" > ")
                    .append(jobID).append("</a>");

                SamUtil.setInfoAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.samfsckjob",
                        new String[] {fsName, loc, href.toString()}),
                    getServerName());
            }
        } catch (SamFSException ex) {
            if (ex.getSAMerrno() == -1000) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSamfsckHrefRequest()",
                    "can not retrieve file system",
                    getServerName());
            } else if (ex.getSAMerrno() == -1009) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSamfsckHrefRequest()",
                    "catalog file exists",
                    getServerName());
            }
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString("FSSummary.error.samfsck"),
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void _handleJobIdHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");
        String jobID = getDisplayFieldStringValue(CHILD_JOBID_HREF);

        // Store the job id in a page session variable
        // The job id has the format:
        //  jobId,jobType,jobCondition
        // For samfs chk jobs, the job type is:
        //  Jobs.jobType6

        ViewBean vb = getParentViewBean();
        ViewBean targetView = getViewBean(JobsDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            vb, PageInfo.getPageInfo().getPageNumber(vb.getName()));
        vb.setPageSessionAttribute(
            Constants.PageSessionAttributes.JOB_ID, jobID);

        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleCancelHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * The request handler for the CCWizardWindow button.
     * Realize that the wizard will be displaying concurrently
     * as this ViewBean redisplays.
     */
    public void handleForwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleNewforwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        wizardNewLaunched = false;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleArchiveforwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    // populate the Actiontable's data
    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");
        handleFilter();
        TraceUtil.trace3("Exiting");
    }

    private void setNewWizardNames() {
        TraceUtil.trace3("Entering");

        String modelName =
            CreateFSWizardImpl.WIZARDPAGEMODELNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
        newWizWinModel.setValue(
            CreateFSWizardImpl.WIZARDPAGEMODELNAME, modelName);

        String implName =
            CreateFSWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
        newWizWinModel.setValue(
            CCWizardWindowModelInterface.WIZARD_NAME, implName);
        TraceUtil.trace3("Exiting");
    }

    private void setGrowWizardNames() {
        TraceUtil.trace3("Entering");

        String modelName =
            GrowWizardImpl.WIZARDPAGEMODELNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
        growWizWinModel.setValue(GrowWizardImpl.WIZARDPAGEMODELNAME, modelName);

        String implName =
            GrowWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
        growWizWinModel.setValue(
            CCWizardWindowModelInterface.WIZARD_NAME, implName);
        TraceUtil.trace3("Exiting");
    }

    private void setArchiveWizardNames() {
        TraceUtil.trace3("Entering");

        // if modelName and implName existing, retrieve it
        // else create them then store in page session

        CommonViewBeanBase view = (CommonViewBeanBase)getParentViewBean();

        String temp = (String)view.getPageSessionAttribute
            (NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String modelName = (String) view.getPageSessionAttribute(
             NewPolicyWizardImpl.WIZARDPAGEMODELNAME);
        String implName =  (String) view.getPageSessionAttribute(
             NewPolicyWizardImpl.WIZARDIMPLNAME);
        if (modelName == null) {
            modelName =
                NewPolicyWizardImpl.WIZARDPAGEMODELNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            view.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDPAGEMODELNAME, modelName);
        }

        archiveWizWinModel.setValue(
            NewPolicyWizardImpl.WIZARDPAGEMODELNAME, modelName);
        if (implName == null) {
            implName = NewPolicyWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
            view.setPageSessionAttribute(
                NewPolicyWizardImpl.WIZARDIMPLNAME, implName);
        }

        archiveWizWinModel.setValue(
            CCWizardWindowModel.WIZARD_NAME, implName);

        // set the server name
        String serverName = view.getServerName();
        archiveWizWinModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);

        TraceUtil.trace3("Exiting");
    }

    private WizardModel getWizardModel(String modelName) {
        ModelManager mm = getRequestContext().getModelManager();
        WizardModel model = (WizardModel)mm.getModel(
            com.sun.netstorage.samqfs.web.wizard.WizardModel.class,
            modelName,
            true,
            true);

        return model;
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

    private int getDropDownMenuValue() throws ModelControlException {
        String value = (String) getDisplayFieldValue(CHILD_ACTIONMENU);
        // get drop down menu selected option
        int option = 0;
        try {
            option = Integer.parseInt(value);
        } catch (NumberFormatException nfe) {
            // should not get here!!!
        }
        return option;
    }

    private String getSelectedFileSystemName() throws ModelControlException {
        int index = -1;
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

        String selectedFSName = getSelectedValue(index);
        if (selectedFSName != null && selectedFSName.length() != 0) {
            // fsName changed, saved to session
            SamUtil.setLastUsedFSName(
                ((CommonViewBeanBase)getParentViewBean()).getServerName(),
                selectedFSName);
            return selectedFSName;
        }

        throw new ModelControlException("Exception occurred within framework");
    }

    private GenericFileSystem getSelectedFileSystem()
        throws SamFSException, ModelControlException {

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        GenericFileSystem fileSystem = sysModel.getSamQFSSystemFSManager().
            getGenericFileSystem(getSelectedFileSystemName());
        if (fileSystem == null) {
            throw new SamFSException(null, -1000);
        } else {
            return fileSystem;
        }
    }
} // end of FileSystemSummaryView class
