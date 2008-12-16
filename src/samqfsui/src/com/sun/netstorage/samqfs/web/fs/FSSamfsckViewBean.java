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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: FSSamfsckViewBean.java,v 1.21 2008/12/16 00:12:10 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;

/**
 *  This class is the view bean for the FSSamfsck page
 */

public class FSSamfsckViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "FSSamfsck";
    private static final String DEFAULT_DISPLAY_URL = "/jsp/fs/FSSamfsck.jsp";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    private boolean error = false;

    // Child View Names used to set Javascript Values
    public static final String CHILD_RADIO = "fsckType";
    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_TEXTFIELD = "TextField";
    public static final String CHILD_MOUNTHIDDEN = "MountHidden";

    // Variable for JSP to determine the window.opener
    public static final String CHILD_PARENT_FORM = "ParentForm";

    /**
     * Constructor
     */
    public FSSamfsckViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_RADIO, CCRadioButton.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_MOUNTHIDDEN, CCHiddenField.class);
        registerChild(CHILD_PARENT_FORM, CCStaticTextField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_MOUNTHIDDEN)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_PARENT_FORM)) {
            TraceUtil.trace3("Exiting");
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_TEXTFIELD)) {
            TraceUtil.trace3("Exiting");
            return new CCTextField(this, name, null);
        } else if (name.equals(CHILD_LABEL)) {
            TraceUtil.trace3("Exiting");
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_STATICTEXT)) {
            CCStaticTextField child = new CCStaticTextField(this, name, null);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_RADIO)) {
            return new CCRadioButton(this, name, null);
            // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        // let the parent class do its bit
        super.beginDisplay(event);

        RequestContext rq = RequestManager.getRequestContext();
        HttpServletRequest httprq = rq.getRequest();

        String parentForm = (String)
            httprq.getParameter(Constants.Parameters.PARENT_PAGE);
        String serverName = getServerName();
        try {
            setPathName();
        } catch (SamFSException samEx) {
            // disable Submit button
            ((CCButton)getChild(SUBMIT)).setDisabled(true);

            SamUtil.processException(
                samEx,
                this.getClass(),
                "setPathName()",
                "Failed to retrieve preference information",
                serverName);

            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FSSamfsck.error",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        // set the default value for radiobutton to 'check'
        ((CCRadioButton) getChild(CHILD_RADIO)).setValue("check");

        TraceUtil.trace2("parentForm = " + parentForm);
        ((CCStaticTextField) getChild(CHILD_PARENT_FORM)).setValue(parentForm);

        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel =
                PageTitleUtil.createModel("/jsp/fs/FSPopupPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Function to retrieve the real data from API,
     * then assign these data to save file path
     */
    private void setPathName() throws SamFSException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String fsName = getFSName();

        String location = "";
        int mounted = 0;

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        FileSystem fs = sysModel.getSamQFSSystemFSManager().
                            getFileSystem(fsName);
        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        location = fs.getFsckLogfileLocation();
        mounted  = fs.getState();

        SamUtil.doPrint("mounted is " + mounted);
        SamUtil.doPrint("location is " + location);

        ((CCTextField) getChild("TextField")).setValue(location);

        CCRadioButton radio = (CCRadioButton)getChild(CHILD_RADIO);

        if (mounted == FileSystem.MOUNTED) {
            ((CCHiddenField) getChild("MountHidden")).setValue("yes");

            // while we are here, set the valid check type radio button options
            radio.setOptions(new OptionList(
                               new String [] {"FSSamfsck.fscktype1"},
                               new String [] {"check"}));

        } else {
            ((CCHiddenField) getChild("MountHidden")).setValue("no");

            // while we are here, set the valid check type radio button options
            radio.setOptions(new OptionList(
                new String [] {"FSSamfsck.fscktype1", "FSSamfsck.fscktype2"},
                new String [] {"check", "both"}));

        }

        // default to check fs without repairing
        if (radio.stringValue() == null) {
            radio.setValue("check");
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * retrieve the selected file system name
     *
     */
    String getFSName() {
        String fsName = (String)getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);

        if (fsName == null) {
            fsName = RequestManager.getRequest()
                .getParameter(Constants.Parameters.FS_NAME);
            setPageSessionAttribute(Constants.PageSessionAttributes.FS_NAME,
                                    fsName);
        }

        return fsName;
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String fsName = getFSName();
        boolean checkAndRepair = false;

        String type = getDisplayFieldStringValue(CHILD_RADIO);
        String loc  = getDisplayFieldStringValue(CHILD_TEXTFIELD);
        SamUtil.doPrint("FSC2K: type is " + type + ", loc is " + loc);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            FileSystem fileSystem =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fileSystem == null) {
                throw new SamFSException(null, -1000);
            }

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

            fileSystem.setFsckLogfileLocation(loc);
            long jobID = fileSystem.samfsck(checkAndRepair, loc);

            LogUtil.info(
                this.getClass(),
                "handleSamfsckHrefRequest",
                "Done samfsck filesystem " + fsName +
                "with Job ID " + jobID);

            if (jobID < 0) {
                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.samfsckresult",
                        new String[] {fsName, loc}),
                    serverName);
            } else {
                // Build href object...
                // href looks like this:
                // <a href="../fs/FSSummary?FSSummary.FileSystemSummaryView.
                //  JobIdHref=123456"
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
                    .append("name=\"")
                    .append("FSSummary.FileSystemSummaryView.")
                    .append(FileSystemSummaryView.CHILD_JOBID_HREF)
                    .append("\" ")
                    .append("onclick=\"")
                    .append("javascript:var f=document.FSSummaryForm;")
                    .append("if (f != null) {f.action=this.href;f.submit();")
                    .append("return false}\"")
                    .append(">").append(jobID).append("</a>");

                SamUtil.setInfoAlert(
                    this,
                    ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.samfsckjob",
                        new String[] {fsName, loc, Long.toString(jobID)}),
                    serverName);

                // if we get this far, submit was successful
                setSubmitSuccessful(true);
            }
        } catch (SamFSException ex) {
            if (ex.getSAMerrno() == -1000) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSubmitRequest()",
                    "can not retrieve file system",
                    serverName);
            } else if (ex.getSAMerrno() == -1009) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSubmitRequest()",
                    "catalog file exists",
                    serverName);
            }
            SamUtil.setErrorAlert(
                                  this,
                                  ALERT,
                                  "FSSummary.error.samfsck",
                                  ex.getSAMerrno(),
                                  ex.getMessage(),
                                  serverName);
        }
        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }
}
