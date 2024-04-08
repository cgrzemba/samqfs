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

// ident	$Id: ArchiveActivityViewBean.java,v 1.13 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Activity Management page
 */

public class ArchiveActivityViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ArchiveActivity";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/archive/ArchiveActivity.jsp";

    public static final String ARCHIVE_ACTION = "archiveValue";
    public static final String STAGE_ACTION = "stageValue";

    // Propertysheet components definitions
    public static final String ARCHIVE_BUTTON = "ArchiveButton";
    public static final String STAGE_BUTTON = "StageButton";
    public static final String ARCHIVE_MENU = "ArchiveMenu";

    public static final String BREADCRUMB = "BreadCrumb";

    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;
    private CCBreadCrumbsModel breadCrumbsModel = null;

    public static final String CHILD_FS_SUM_HREF = "FileSystemSummaryHref";
    public static final String CHILD_FS_DET_HREF = "FileSystemDetailsHref";

    public static final String ERROR_MESSAGE = "ErrorMessage";

    /**
     * Constructor
     */
    public ArchiveActivityViewBean() {
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
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_FS_SUM_HREF, CCHref.class);
        registerChild(CHILD_FS_DET_HREF, CCHref.class);
        registerChild(ERROR_MESSAGE, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("ArchiveActivity.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child =
                new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_FS_SUM_HREF) ||
                   name.equals(CHILD_FS_DET_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(ERROR_MESSAGE)) {
            child = new CCHiddenField(
                this, name,
                SamUtil.getResourceString("ArchiveActivity.erroronsingle"));
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // PageTitle Child
        } else if (PropertySheetUtil.isChildSupported
            (propertySheetModel, name)) {
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

        populateRadioButtonGroups();
        populateDropDownMenus();
        loadPropertySheetModel();

        // disable save & cancel if no sam control authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.SAM_CONTROL)) {

            ((CCButton)getChild(ARCHIVE_BUTTON)).setDisabled(true);
            ((CCButton)getChild(STAGE_BUTTON)).setDisabled(true);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to create the pagetitle model based on the xml file
     */
    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to create the PropertySheet model
     * based on the xml file.
     */
    private void createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/archive/ArchiveActivityPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to retrieve the real data from API,
     * then assign these data to propertysheet model to display
     */
    private void loadPropertySheetModel() {
        TraceUtil.trace3("Entering");
        propertySheetModel.clear();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Requesthandler function to hand the submit button action
     */
    public void handleArchiveButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String message = null;
        String allFSItem = SamUtil.getResourceString("ArchiveActivity.allfs");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            String archiveOperation =
                (String) propertySheetModel.getValue(ARCHIVE_ACTION);
            String selectedFS =
                (String) propertySheetModel.getValue(ARCHIVE_MENU);
            int intOperation = Integer.parseInt(archiveOperation);

            switch (intOperation) {
                case SamQFSSystemModel.ACTIVITY_ARCHIVE_RESTART:
                    message = "ArchiveActivity.arrestart.allfs";
                    if (allFSItem.equals(selectedFS)) {
                        // All FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start restart archive all");
                        sysModel.getSamQFSSystemArchiveManager().
                            restartArchivingAll();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done restart archive all");
                    }
                    break;

                case SamQFSSystemModel.ACTIVITY_ARCHIVE_IDLE:

                    if (allFSItem.equals(selectedFS)) {
                        message = "ArchiveActivity.aridle.allfs";
                        // All FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start idle archive all");
                        sysModel.getSamQFSSystemArchiveManager().
                                                    idleArchivingAll();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done idle archive all");
                    } else {
                        message = "ArchiveActivity.aridle";
                        // Selected FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start restart archive ".concat(selectedFS));
                        sysModel.getSamQFSSystemFSManager().
                            getFileSystem(selectedFS).idleFSArchive();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done restart archive ".concat(selectedFS));
                    }
                    break;

                case SamQFSSystemModel.ACTIVITY_ARCHIVE_RUN:

                    if (allFSItem.equals(selectedFS)) {
                        message = "ArchiveActivity.arrun.allfs";
                        // All FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start run archive all");
                        sysModel.getSamQFSSystemArchiveManager().
                                                    runNowArchivingAll();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done run archive all");
                    } else {
                        message = "ArchiveActivity.arrun";
                        // Selected FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start run archive ".concat(selectedFS));
                        sysModel.getSamQFSSystemFSManager().
                            getFileSystem(selectedFS).runFSArchive();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done run archive ".concat(selectedFS));
                    }
                    break;

                case SamQFSSystemModel.ACTIVITY_ARCHIVE_RERUN:
                    message = "ArchiveActivity.arrerun.allfs";
                    if (allFSItem.equals(selectedFS)) {
                        // All FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start rerun archive all");
                        sysModel.getSamQFSSystemArchiveManager().
                                                    rerunArchivingAll();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done rerun archive all");
                    }
                    break;

                case SamQFSSystemModel.ACTIVITY_ARCHIVE_STOP:

                    if (allFSItem.equals(selectedFS)) {
                        message = "ArchiveActivity.arstop.allfs";
                        // All FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start stopping archive all");
                        sysModel.getSamQFSSystemArchiveManager().
                                                    stopArchivingAll();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done stopping archive all");
                    } else {
                        message = "ArchiveActivity.arstop";
                        // Selected FS
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Start stopping archive ".concat(selectedFS));
                        sysModel.getSamQFSSystemFSManager().
                            getFileSystem(selectedFS).stopFSArchive();
                        LogUtil.info(this.getClass(),
                            "handlePageActionButton1Request",
                            "Done stopping archive ".concat(selectedFS));
                    }
                    break;

                default:
                    /* do nothing */
                    break;
            }

            // Include FS name in the alert
            if (!allFSItem.equals(selectedFS)) {
                message = SamUtil.getResourceString(message, selectedFS);
            } else {
                message = SamUtil.getResourceString(message);
            }

            SamUtil.setInfoAlert(
                this,
                CHILD_COMMON_ALERT,
                "success.summary",
                message,
                getServerName());

        } catch (SamFSException ex) {
            TraceUtil.trace1(
                "Exception caught while running archiving action! Cause: "
                + ex.getMessage());
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSubmitJobPageButtonRequest",
                "Failed to execute the requested operation",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "ArchiveActivity.error",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
        forwardTo(getRequestContext());
    }

    /**
     * Requesthandler function to hand the submit button action
     */
    public void handleStageButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String message = null;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            // Staging
            String stageOperation =
                (String) propertySheetModel.getValue(STAGE_ACTION);
            int intOperation = Integer.parseInt(stageOperation);

            switch (intOperation) {
                case SamQFSSystemModel.ACTIVITY_STAGE_IDLE:
                    message = "ArchiveActivity.stidle";
                    // All FS
                    LogUtil.info(this.getClass(),
                        "issueStaging", "Start idle staging all");
                    sysModel.getSamQFSSystemArchiveManager().
                                                idleStagingAll();
                    LogUtil.info(this.getClass(),
                        "issueStaging", "Done idle staging all");
                    break;

                case SamQFSSystemModel.ACTIVITY_STAGE_RUN:
                    message = "ArchiveActivity.strun";
                    // All FS
                    LogUtil.info(this.getClass(),
                        "issueStaging", "Start run staging all");
                    sysModel.getSamQFSSystemArchiveManager().
                                                    runStagingAll();
                    LogUtil.info(this.getClass(),
                        "issueStaging", "Done run staging all");
                    break;

                default:
                    /* do nothing */
                    break;
            }

            SamUtil.setInfoAlert(
                this,
                CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(message),
                getServerName());

        } catch (SamFSException ex) {
            TraceUtil.trace1(
                "Exception caught while running archiving action! Cause: "
                + ex.getMessage());
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSubmitJobPageButtonRequest",
                "Failed to execute the requested operation",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "ArchiveActivity.error",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
        forwardTo(getRequestContext());
    }

    private void populateRadioButtonGroups() {
        ((CCRadioButton)getChild(ARCHIVE_ACTION)).
            setOptions(
                new OptionList(
                    SelectableGroupHelper.archiveActions.labels,
                    SelectableGroupHelper.archiveActions.values));
        ((CCRadioButton)getChild(ARCHIVE_ACTION)).
            setValue(
                String.valueOf(SamQFSSystemModel.ACTIVITY_ARCHIVE_RUN));

        ((CCRadioButton)getChild(STAGE_ACTION)).
            setOptions(
                new OptionList(
                    SelectableGroupHelper.stageActions.labels,
                    SelectableGroupHelper.stageActions.values));
        ((CCRadioButton)getChild(STAGE_ACTION)).
            setValue(
                String.valueOf(SamQFSSystemModel.ACTIVITY_STAGE_RUN));
    }

    private void populateDropDownMenus() {
        CCDropDownMenu archiveMenu = (CCDropDownMenu) getChild(ARCHIVE_MENU);

        try {
            StringBuffer buf =
                new StringBuffer(
                    SamUtil.getResourceString("ArchiveActivity.allfs"));
            SamQFSSystemFSManager fsManager =
                SamUtil.getModel(getServerName()).getSamQFSSystemFSManager();
            FileSystem [] fs = fsManager.getAllFileSystems();
            for (int i = 0; i < fs.length; i++) {
                if (fs[i].getArchivingType() == FileSystem.ARCHIVING) {
                    buf.append("###").append(fs[i].getName());
                }
            }

            archiveMenu.setOptions(
                new OptionList(
                    buf.toString().split("###"),
                    buf.toString().split("###")));

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "populateDropDownMenus",
                "Failed to populate file system drop down menus",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CHILD_COMMON_ALERT,
                "ArchiveActivity.populatefs.error",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        // Preselect FS Name if user comes from the FS Summary page
        String fsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);
        if (fsName != null) {
            archiveMenu.setValue(fsName);
        }
    }

    /**
     * Handle request for backtofs link
     */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
                throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("FileSystemSummaryHref");
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            s);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto detail href
     * @param event RequestInvocationEvent event
     */
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("FileSystemDetailsHref");
        ViewBean targetView = getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }
}
