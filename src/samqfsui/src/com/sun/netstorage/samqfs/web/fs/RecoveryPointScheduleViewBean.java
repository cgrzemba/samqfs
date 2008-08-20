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

// ident	$Id: RecoveryPointScheduleViewBean.java,v 1.13 2008/08/20 20:46:50 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.admin.FirstTimeConfigViewBean;
import com.sun.netstorage.samqfs.web.admin.ScheduledTasksViewBean;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import java.util.List;
import javax.servlet.ServletException;

public class RecoveryPointScheduleViewBean extends CommonViewBeanBase {
    public static final String PAGE_NAME = "RecoveryPointSchedule";
    public static final
        String DEFAULT_URL = "/jsp/fs/RecoveryPointSchedule.jsp";

    // children symbolic constants
    public static final String VIEW = "RecoveryPointScheduleView";
    public static final String BREAD_CRUMB = "BreadCrumb";
    public static final String PAGE_TITLE  = "PageTitle";
    public static final String ERROR_MSG = "errMsg";

    // bread crumbing href
    public static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    public static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    public static final String SHARED_FS_DETAILS_HREF = "SharedFSDetailsHref";
    public static final String POLICY_SUMMARY_HREF    = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF    = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF  = "CriteriaDetailsHref";
    public static final String FS_ARCHIVE_POLICY_HREF = "FSArchivePolicyHref";
    public static final String SCHEDULED_TASKS_HREF   = "ScheduledTasksHref";

    private static final String FIRSTTIME_CONFIG =
        "from.first.time.config.page";

    private CCPageTitleModel ptModel;

    public RecoveryPointScheduleViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = PageTitleUtil
            .createModel("/jsp/fs/RecoveryPointSchedulePageTitle.xml");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        PageTitleUtil.registerChildren(this, ptModel);
        registerChild(VIEW, RecoveryPointScheduleView.class);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVE_POLICY_HREF, CCHref.class);
        registerChild(SCHEDULED_TASKS_HREF, CCHref.class);
        registerChild(ERROR_MSG, CCHiddenField.class);
        super.registerChildren();
    }

    public View createChild(String name) {
        if (name.equals(VIEW)) {
            return new RecoveryPointScheduleView(this, name);
        } else if (name.equals(FS_SUMMARY_HREF) ||
                   name.equals(FS_DETAILS_HREF) ||
                   name.equals(SHARED_FS_DETAILS_HREF) ||
                   name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF) ||
                   name.equals(FS_ARCHIVE_POLICY_HREF) ||
                   name.equals(SCHEDULED_TASKS_HREF)) {
            return new CCHref(this, name, null);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(BREAD_CRUMB)) {
            CCBreadCrumbsModel bcModel = new
                CCBreadCrumbsModel("RecoveryPointSchedule.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (name.equals(ERROR_MSG)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("invaid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        String serverName = getServerName();

        try {
            RecoveryPointScheduleView view =
                (RecoveryPointScheduleView)getChild(VIEW);
            view.loadSchedule();

            // check for authorization
            if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
                ((CCButton)getChild("Save")).setDisabled(true);
            }
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                getClass(),
                "loadSchedule()",
                "failed to retrieve schedule",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "fs.recoverypointschedule.error.failedPopulate",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }

        ((CCHiddenField) getChild(ERROR_MSG)).setValue(
            SamUtil.getResourceString(
                "fs.recoverypointschedule.error.retention"));

        TraceUtil.trace3("Exiting");
    }

    /** handle save request */
    public void handleSaveRequest(RequestInvocationEvent evt)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        String serverName = getServerName();

        RecoveryPointScheduleView view =
            (RecoveryPointScheduleView)getChild(VIEW);
        try {
            List errors = view.saveSchedule();

            if (errors.size() == 0) {
                forwardToTargetPage(true);
            } else {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "fs.recoverypointschedule.save.error",
                    -1,
                    "fs.recoverypointschedule.error.incomplete",
                    serverName);
            }
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                getClass(),
                "handleSaveRequest()",
                "unable to save recovery point schedule",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "fs.recoverypointschedule.save.error",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /** handle cancel request */
    public void handleCancelRequest(RequestInvocationEvent evt)
        throws ModelControlException {
        if (isFromFirstTimeConfig()) {
            CommonViewBeanBase parent =
                (CommonViewBeanBase)getViewBean(FirstTimeConfigViewBean.class);
            forwardTo(parent);
        } else {
            forwardToTargetPage(false);
        }
    }

    private void forwardToTargetPage(boolean showAlert) {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase target = null;
        String s = null;

        if (isFromFirstTimeConfig()) {
            target = (CommonViewBeanBase)getViewBean(FSSummaryViewBean.class);
            if (showAlert) {
                SamUtil.setInfoAlert(
                    target,
                    target.CHILD_COMMON_ALERT,
                    "success.summary",
                    "fs.recoverypointschedule.save.success",
                    getServerName());
            }
            forwardTo(target);
            return;
        }

        Integer [] temp = (Integer [])
            getPageSessionAttribute(Constants.SessionAttributes.PAGE_PATH);
        Integer [] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);

        int index = path[path.length - 1].intValue();
        PageInfo pageInfo = PageInfo.getPageInfo();
        String targetName = pageInfo.getPagePath(index).getCommandField();

        if (FS_SUMMARY_HREF.equals(targetName)) {
            target = (CommonViewBeanBase)getViewBean(FSSummaryViewBean.class);
        } else if (FS_DETAILS_HREF.equals(targetName)) {
            target = (CommonViewBeanBase)getViewBean(FSDetailsViewBean.class);
        } else if (SHARED_FS_DETAILS_HREF.equals(targetName)) {
            target = (CommonViewBeanBase)
                getViewBean(SharedFSDetailsViewBean.class);
        } else if (SCHEDULED_TASKS_HREF.equals(targetName)) {
            target = (CommonViewBeanBase)
                getViewBean(ScheduledTasksViewBean.class);
        }
        s = Integer
            .toString(BreadCrumbUtil.inPagePath(path, index, path.length - 1));
        if (showAlert) {
            SamUtil.setInfoAlert(target,
                                 target.CHILD_COMMON_ALERT,
                                 "success.summary",
                                 "fs.recoverypointschedule.save.success",
                                 getServerName());
        }

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()),
            s);

        TraceUtil.trace3("Exiting");
        forwardTo(target);

    }

    /** rewind to the scheduled tasks page */
    public void handleScheduledTasksHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldStringValue(SCHEDULED_TASKS_HREF);
        ViewBean target = getViewBean(ScheduledTasksViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    /** rewind to the file system summary page */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        String s = (String) getDisplayFieldValue(FS_SUMMARY_HREF);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /** rewind to the file system details page */
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(FSDetailsViewBean.class);
        String s = (String) getDisplayFieldValue(FS_DETAILS_HREF);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /** rewind to the shared file system details page */
    public void handleSharedFSDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(SharedFSDetailsViewBean.class);
        String s = (String) getDisplayFieldValue(FS_DETAILS_HREF);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /** rewind to the archive policies for file system page */
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(FS_ARCHIVE_POLICY_HREF);
        ViewBean targetView =
            getViewBean(FSArchivePoliciesViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /** rewind to the archive policy summary page */
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        ViewBean target = getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    /** rewind to the archive policy details page */
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        ViewBean target = getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    /** rewind to the archive criteria details page */
    public void handleCriteriaDetailsHrefRequest(
        RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        ViewBean target = getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    public boolean isFromFirstTimeConfig() {
        boolean result = false;
        Boolean b = (Boolean)getPageSessionAttribute(FIRSTTIME_CONFIG);
        if (b != null) {
            result = b.booleanValue();
        }

        return result;
    }

    public void setFromFirstTimeConfig(boolean b) {
        setPageSessionAttribute(FIRSTTIME_CONFIG, new Boolean(b));
    }
}
