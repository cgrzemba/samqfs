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

// ident	$Id: FSDumpNowPopUpViewBean.java,v 1.15 2008/11/12 23:01:25 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserModel;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import javax.servlet.ServletException;

/**
 * This class serves as the viewbean for creating a recovery point immediately.
 * This page is a pop up and it is launched from the Recovery Points page.
 */

public class FSDumpNowPopUpViewBean extends CommonSecondaryViewBeanBase {
    // Page information...
    private static final String PAGE_NAME = "FSDumpNowPopUp";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/fs/FSDumpNowPopUp.jsp";

    // variable for JSP to determine the window.opener
    public static final String CHILD_PARENT_FORM = "ParentForm";

    public static final String CHILD_ERROR_MSG1 = "LocationEmptyError";
    public static final String CHILD_ERROR_MSG2 = "InvalidPathError";
    public static final String CHILD_PATH_CHOOSER = "pathChooser";
    private static final String SELECTED_PATH = "selectedPath";

    private CCPageTitleModel pageTitleModel = null;
    protected RemoteFileChooserModel pathChooserModel = null;
    protected CCPropertySheetModel propertySheetModel  = null;

    // flag to indicate whether we're createing a new dump schedule or
    // editing an existing one
    private boolean newSchedule = true;

    /**
     * Constructor
     */
    public FSDumpNowPopUpViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        createPageTitleModel();
        createPropertySheetModel();
        createFileChooserModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CHILD_PATH_CHOOSER, RemoteFileChooserControl.class);
        registerChild(CHILD_PARENT_FORM, CCStaticTextField.class);
        registerChild(CHILD_ERROR_MSG1, CCHiddenField.class);
        registerChild(CHILD_ERROR_MSG2, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        name = name == null ? "" : name;
        TraceUtil.trace3("Entering with child name ".concat(name));
        View child = null;

        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CHILD_PATH_CHOOSER)) {
            // Create an filechooser child.
            child = new RemoteFileChooserControl(this, pathChooserModel, name);
        } else if (
            PropertySheetUtil.isChildSupported(propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                            this, propertySheetModel, name);
        } else if (name.equals(CHILD_PARENT_FORM)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_ERROR_MSG1) ||
                   name.equals(CHILD_ERROR_MSG2)) {
            child = new CCHiddenField(this, name, null);
        } else {
            TraceUtil.trace3("Exiting with error");
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

        // Check if submit button needs to be disabled after submit is clicked
        super.beginDisplay(event);

        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("FSDumpNow.title", getFSName()));

         String parentForm =
            (String) RequestManager.getRequestContext().getRequest().
                getParameter(Constants.Parameters.PARENT_PAGE);
        ((CCStaticTextField) getChild(CHILD_PARENT_FORM)).setValue(parentForm);
        ((CCHiddenField) getChild(CHILD_ERROR_MSG1)).setValue(
            SamUtil.getResourceString("FSScheduleDump.error.locationEmpty"));
        ((CCHiddenField) getChild(CHILD_ERROR_MSG2)).setValue(
            SamUtil.getResourceString("FSDumpNow.error.invalidPath"));

        try {
            loadPropertySheetModel();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "loadPropertySheetModel()",
                "failed to populate data",
                getServerName());

            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FSDumpNow.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * retrieve the selected file system name
     *
     */
    private String getFSName() {
        String fsName = (String) getPageSessionAttribute(
           Constants.PageSessionAttributes.FS_NAME);

        if (fsName == null) {
            fsName = RequestManager.getRequest().
                getParameter(Constants.Parameters.FS_NAME);
            setPageSessionAttribute(Constants.PageSessionAttributes.FS_NAME,
                    fsName);
        }
        return fsName;
    }

    /**
     * retrieve selected path name
     */
    private String getSelectedPathName() {
        String pathName = (String) getPageSessionAttribute(SELECTED_PATH);

        if (pathName == null) {
            pathName = RequestManager.getRequest().getParameter(SELECTED_PATH);
            setPageSessionAttribute(SELECTED_PATH, pathName);
        }
        return pathName;
    }

    /**
     * Create pagetitle model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/FSPopupPageTitle.xml");
        }

        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * handler for the Dump Popup 'Submit' button
     */
    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String fileSystemName = getFSName();

        // get the dump path
        String dumpPath =
            ((RemoteFileChooserControl) getChild(CHILD_PATH_CHOOSER)).
                getDisplayFieldStringValue("browsetextfield");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                "Starting backing up metadata for fs " + fileSystemName);

            long jobID =
                sysModel.getSamQFSSystemFSManager().startMetadataDump(
                    fileSystemName, dumpPath);

            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                "Done backing up metadata for fs " + fileSystemName);

            if (jobID != BaseJob.INVALID_JOB_ID) {
                // build jobID href
                StringBuffer href = new StringBuffer();

                href.
                    append("<a href=\"../fs/FSSummary?FSSummary.").
                    append("FileSystemSummaryView.JobIdHref=").
                    append(jobID).append(",").
                    append(Constants.Jobs.JOB_TYPE_METADATA_DUMP).
                    append(",Current").append("\" ").
                    append("name=\"FSSummary.FileSystemSummaryView.").
                    append(FileSystemSummaryView.CHILD_JOBID_HREF).append("\"").
                    append("onclick=\"javascript:").
                    append("var f=document.FSSummaryForm;").
                    append("if (f != null) ").
                    append("{f.action=this.href;f.submit();").
                    append("return false}\"").append(" > ").
                    append(SamUtil.getResourceString("FSRestore.jobIdLink")).
                    append("</a>");

                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.dumpMetadataJobStarted",
                        new String[] {fileSystemName,
                                      String.valueOf(jobID),
                                      Long.toString(jobID)}),
                serverName);
            } else {
                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.dumpMetadataJobCompleted",
                        fileSystemName),
                    serverName);
            }
            // if we get this far, dumb was successful
            setSubmitSuccessful(true);
        } catch (SamFSException ex) {
            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "FSSummary.error.dumpNow",
                                  ex.getSAMerrno(),
                                  ex.getMessage(),
                                  serverName);
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create propertysheet model
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/FSDumpNowPropertySheet.xml");
        }

        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    private void createFileChooserModel() {
        pathChooserModel = new RemoteFileChooserModel(getServerName(), 50);
        pathChooserModel.setFileListBoxHeight(15);
        pathChooserModel.setHomeDirectory("/");
        pathChooserModel.setProductNameAlt(
            "secondaryMasthead.productNameAlt");
        pathChooserModel.setProductNameSrc(
            "secondaryMasthead.productNameSrc");
        pathChooserModel.setPopupMode(true);
    }

    /**
     * Load the data for propertysheet model
     */
    public void loadPropertySheetModel() throws SamFSException {
        TraceUtil.trace3("Entering");

        String fileSystemName = getFSName();
        String location = getSelectedPathName();

        if (location != null) {
            // we'return editing an existing schedule, set the flag
            newSchedule = false;

            // Add a dump file name like:  <fs name>-Y-m-dTH:M
            StringBuffer defPath = new StringBuffer().append(location);
            if (!location.endsWith("/")) {
                defPath.append("/");
            }
            defPath.append(fileSystemName);

            // NOw add on date and time
            GregorianCalendar now = new GregorianCalendar();
            SimpleDateFormat fmt = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm");
            defPath.append("-")
                   .append(fmt.format(now.getTime()));

            RemoteFileChooserControl pathChooser = (RemoteFileChooserControl)
                                            getChild(CHILD_PATH_CHOOSER);
            pathChooser.setDisplayFieldValue(
                                    RemoteFileChooserControl.BROWSED_FILE_NAME,
                                    defPath.toString());
        } else {
            // we're creating a new dump schedule
            newSchedule = true;
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * handler for the Dump Popup 'Cancel' button
     */
    public void handleCancelRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }
}
