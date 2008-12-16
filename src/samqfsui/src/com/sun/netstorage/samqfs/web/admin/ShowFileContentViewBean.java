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

// ident	$Id: ShowFileContentViewBean.java,v 1.14 2008/12/16 00:10:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;

import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.media.MediaUtil;
import com.sun.netstorage.samqfs.web.model.ConfigStatus;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.media.VSNWrapper;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextArea;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.html.CCHiddenField;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for both the pop up of Config File Content
 *  and Config File status, which are launched from the Application
 *  Configuration File Status table in the Server Selection Page
 *
 *  SAMQFS_SHOW_CONTENT param defines what we want to show in the text area.
 *  true => Show the content of the config file
 *  false => Show the status of the config file
 */

public class ShowFileContentViewBean extends ShowPopUpWindowViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ShowFileContent";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/admin/ShowFileContent.jsp";

    public static final String CHILD_PREVIOUS_BUTTON = "PreviousButton";
    public static final String CHILD_NEXT_BUTTON = "NextButton";
    public static final String CHILD_GO_BUTTON = "GoButton";
    public static final String CHILD_STATIC_TEXT = "StaticText";
    public static final String CHILD_LINE_NUMBER = "LineNumber";
    public static final String CHILD_TEXTFIELD   = "TextField";

    // Page Session Attributes to keep track on line numbers for line control
    private static final String START_LINE = "psa_startLine";
    private static final String END_LINE   = "psa_endLine";

    // Hidden field for Line Control Error Message
    public static final String CHILD_ERROR_MESSAGE = "ErrorMessage";

    // Request variable to determine if this pop up is used to show staging
    // queue file list
    public static final String SAMQFS_STAGE_Q_LIST = "SAMQFS_STAGE_Q_LIST";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // page request key for matching volumes in volume pool details page
    // format: pool_name,expression
    private static final String MATCHING_VOLUME = "matching_volumes";

    /**
     * Constructor
     */
    public ShowFileContentViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        registerChildren();

        if (isResourceKeyName() && !isStagingQList()) {
            // size the text area to fit the small popup
            setWindowSizeNormal(false);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);

        if (isShowLineControl()) {
            registerChild(CHILD_PREVIOUS_BUTTON, CCButton.class);
            registerChild(CHILD_NEXT_BUTTON, CCButton.class);
            registerChild(CHILD_GO_BUTTON, CCButton.class);
            registerChild(CHILD_STATIC_TEXT, CCStaticTextField.class);
            registerChild(CHILD_LINE_NUMBER, CCStaticTextField.class);
            registerChild(CHILD_TEXTFIELD, CCTextField.class);
            registerChild(CHILD_ERROR_MESSAGE, CCHiddenField.class);
        }
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
          // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CHILD_ERROR_MESSAGE)) {
            child = new CCHiddenField(this, name,
                SamUtil.getResourceString("ShowFileContent.gotoline.error"));
        } else if (name.equals(CHILD_PREVIOUS_BUTTON) ||
            name.equals(CHILD_NEXT_BUTTON) ||
            name.equals(CHILD_GO_BUTTON)) {
            child = new CCButton(this, name, null);
        } else if (name.equals(CHILD_STATIC_TEXT) ||
            name.equals(CHILD_LINE_NUMBER)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_TEXTFIELD)) {
            child = new CCTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(new StringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }

        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        TraceUtil.trace2("isShowLineControl(): " + isShowLineControl());
        TraceUtil.trace2("isStagingQList(): " + isStagingQList());
        TraceUtil.trace2("isShowContent(): " + isShowContent());
        TraceUtil.trace2(
            "getShowMatchingVolumes(): " + getShowMatchingVolumes());

        String pageTitle = null;
        // SAM Explorer if showLineControl == true
        // otherwise check if showContent == true
        // isStagingQList == true if pop up is used for Staging Q file list
        if (isShowLineControl()) {
            pageTitle = SamUtil.getResourceString(
                "ShowFileContent.pageTitle.config.explorer", getServerName());
            ((CCStaticTextField) getChild(CHILD_LINE_NUMBER)).setValue(
                SamUtil.getResourceString(
                    "ShowFileContent.statictext.1",
                    new String [] {
                        Integer.toString(getStartLine() + 1),
                        Integer.toString(getEndLine() + 1)}));
            // Disable the Previous Button if first 200 lines are shown
            ((CCButton) getChild(CHILD_PREVIOUS_BUTTON)).
                setDisabled(getStartLine() == 0);
        } else {
            if (isStagingQList()) {
                pageTitle = SamUtil.getResourceString(
                        "Monitor.stagingqueue.pagetitle",
                        new String [] {
                            getResourceKeyName(),
                            getServerName()});
            } else if (isShowContent()) {
                pageTitle = (isResourceKeyName()) ?
                    SamUtil.getResourceString(
                        getResourceKeyName().concat(".title"),
                        getServerName()):
                    SamUtil.getResourceString(
                        "ShowFileContent.pageTitle.config.content",
                        getServerName());
            } else if (getShowMatchingVolumes() != null) {
                String [] keyArray = getShowMatchingVolumes().split(",");
                if (keyArray[0].equals(keyArray[1])) {
                    // Used for Copy volumes to show members in pool
                    pageTitle = SamUtil.getResourceString(
                        "ShowFileContent.pageTitle.matchingvolumes.pools",
                        new String [] {
                            keyArray[0]});
                } else if (keyArray[0].indexOf(".") == -1) {
                    // Used for Volume Pool
                    pageTitle = SamUtil.getResourceString(
                        "ShowFileContent.pageTitle.matchingvolumes",
                        new String [] {
                            keyArray[1],
                            keyArray[0]});
                } else {
                    // Used for Copy Volumes
                    pageTitle = SamUtil.getResourceString(
                        "ShowFileContent.pageTitle.matchingvolumes.copyvsns",
                        new String [] {
                            keyArray[1]});
                }
            } else {
                pageTitle = SamUtil.getResourceString(
                    "ShowFileContent.pageTitle.config.status",
                    getServerName());
            }
        }
        pageTitleModel.setPageTitleText(pageTitle);
        populateTextArea();

        TraceUtil.trace3("Exiting");
    }

    private void populateTextArea() {
        TraceUtil.trace2("isResourceKeyName(): " + isResourceKeyName());

        CCTextArea myTextArea =
            (CCTextArea) getChild(ShowPopUpWindowViewBeanBase.TEXT_AREA);

        try {
            myTextArea.setValue(createTextAreaString());
            if (isResourceKeyName()) {
                myTextArea.setReadOnly(true);
            }

        } catch (SamFSException samEx) {
            if (isShowContent()) {
                if (isResourceKeyName()) {
                    myTextArea.setValue("ShowFileContent.resourceKey.error");
                } else {
                    myTextArea.setValue("ShowFileContent.content.error");
                }
                TraceUtil.trace1("Failed to populate the content!");
            } else {
                if (isStagingQList()) {
                    myTextArea.setValue("Monitor.stagingqueue.populate.failed");
                } else {
                    myTextArea.setValue("ShowFileContent.status.error");
                }
                TraceUtil.trace1(
                    "Failed to populate config file/staging q content!");
            }
            TraceUtil.trace1("Reason: " + samEx.getMessage());
            return;
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/admin/ShowPopUpPageTitle.xml");
        }
        return pageTitleModel;
    }

    private String createTextAreaString()
        throws SamFSException {
        StringBuffer buf = new StringBuffer();
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        if (isShowContent()) {
            String [] content = null;

            if (isShowLineControl()) {
                // Show SAM Explorer Output Content
                // NO NEED TO check version, this table won't be shown in 4.4
                content = sysModel.getSamQFSSystemAdminManager().
                    getTxtFile(getFileName(), getStartLine(), getEndLine());
            } else {
                // Just get the resource key and get string from resource bundle
                if (isResourceKeyName()) {
                    content = new String[1];
                    content[0] =
                        SamUtil.getResourceString(
                            getResourceKeyName().concat(".info"));
                } else {
                    // Show Configuration File Content
                    // TODO: Will 10000 work all the time?
                    content = sysModel.getSamQFSSystemAdminManager().
                        tailFile(getFileName(), 10000);
                }

            }

            if (content == null) {
                buf.append(SamUtil.getResourceString(
                    "ShowFileContent.content.empty"));
            } else {
                for (int i = 0; i < content.length; i++) {
                    if (i > 0) {
                        buf.append("\n");
                    }
                    buf.append(content[i]);
                }
            }
        } else if (isStagingQList()) {
            long jobID = -1;
            try {
                jobID = Long.parseLong(getResourceKeyName());
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1(
                    "Developer's bug found. Job ID is not a long!");
                return ("Developer's bug found. Job ID is not a long!");
            }

            StageJob myJob = (StageJob)
                sysModel.getSamQFSSystemJobManager().getJobById(jobID);
            StageJobFileData [] fileData =
                myJob == null ?
                    new StageJobFileData[0] : myJob.getFileData();
            for (int i = 0; i < fileData.length; i++) {
                if (buf.length() > 0) {
                    buf.append("\n");
                }
                buf.append(fileData[i].getFileName());
            }
        } else if (getShowMatchingVolumes() != null) {
             buf = createMatchingVolumesString();
        } else {
            ConfigStatus [] myConfigStatusArray =
                sysModel.getConfigStatus();
            ConfigStatus myConfigStatus = null;
            for (int i = 0; i < myConfigStatusArray.length; i++) {
                if (myConfigStatusArray[i].getConfig().equals(getFileName())) {
                    myConfigStatus = myConfigStatusArray[i];
                }
            }
            String content = myConfigStatus.getMsg();
            content = content == null ? "" : content.trim();
            buf.append(content);
            buf.append("\n");

            String[] copies = myConfigStatus.getVSNs();
            if (copies != null && copies.length != 0) {
                buf.append("\n").append(SamUtil.getResourceString(
                    "ShowFileContent.copies")).append("\n");

                for (int i = 0; i < copies.length; i++) {
                    if (i > 0) {
                        buf.append("\n");
                    }
                    buf.append(copies[i] == null ? "" : copies[i]);
                }
            }

            if (buf.length() == 0) {
                buf.append(SamUtil.getResourceString(
                    "ShowFileContent.status.empty"));
            }
        }

        return buf.toString();
    }


    // Hide the following five components if line control is not a part
    // of the display
    public boolean beginPreviousButtonDisplay(ChildDisplayEvent event)
	throws ModelControlException {
        return isShowLineControl();
    }

    public boolean beginNextButtonDisplay(ChildDisplayEvent event)
	throws ModelControlException {
        return isShowLineControl();
    }

    public boolean beginGoButtonDisplay(ChildDisplayEvent event)
	throws ModelControlException {
        return isShowLineControl();
    }

    public boolean beginStaticTextDisplay(ChildDisplayEvent event)
	throws ModelControlException {
        return isShowLineControl();
    }

    public boolean beginTextFieldDisplay(ChildDisplayEvent event)
	throws ModelControlException {
        return isShowLineControl();
    }

    public boolean beginErrorMessageDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return isShowLineControl();
    }

    public boolean beginLineNumberDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return isShowLineControl();
    }

    // get the resourceKey
    private String getResourceKeyName() {
        // first check the page session
        String resourceKey = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.RESOURCE_KEY_NAME);

        // second check the request
        if (resourceKey == null) {
            resourceKey = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.RESOURCE_KEY_NAME);

            if (resourceKey != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.RESOURCE_KEY_NAME,
                    resourceKey);
            } else {
                throw new IllegalArgumentException("Resource Key not supplied");
            }
        }

        return resourceKey;
    }

    private String getFileName() {
        // first check the page session
        String fileName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.CONFIG_FILE_NAME);

        // second check the request
        if (fileName == null) {
            fileName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.CONFIG_FILE_NAME);

            if (fileName != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.CONFIG_FILE_NAME,
                    fileName);
            } else {
                throw new IllegalArgumentException("File Name not supplied");
            }
        }

        return fileName;
    }

    private String getShowMatchingVolumes() {
        // first check the page session
        String matchVol = (String) getPageSessionAttribute(MATCHING_VOLUME);

        // second check the request
        if (matchVol == null) {
            matchVol =
                RequestManager.getRequest().getParameter(MATCHING_VOLUME);

            if (matchVol != null) {
                setPageSessionAttribute(MATCHING_VOLUME, matchVol);
            }
        }

        return matchVol;
    }

    private StringBuffer createMatchingVolumesString() {
        String [] keyArray = getShowMatchingVolumes().split(",");
        StringBuffer buf = new StringBuffer();

        boolean reserved = false;
        VSNWrapper wrapper = null;
        int mediaType = -1;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            if (keyArray[0].indexOf(".") == -1) {
                // Use in Pool Details page, or looking at volumes of a pool
                // in the copy volumes page
                VSNPool pool = sysModel.
                    getSamQFSSystemArchiveManager().getVSNPool(keyArray[0]);
                mediaType = pool.getMediaType();

                // if both keys are equal, this pop up is trying to show the
                // participating volumes of the pool
                if (keyArray[0].equals(keyArray[1])) {
                    wrapper =
                        sysModel.getSamQFSSystemMediaManager().
                            evaluateVSNExpression(
                                mediaType, null, -1,
                                null, null, null,
                                keyArray[0],
                                SamQFSSystemMediaManager.
                                    MAXIMUM_ENTRIES_FETCHED_OTHERS);
                } else {
                    wrapper =
                        sysModel.getSamQFSSystemMediaManager().
                            evaluateVSNExpression(
                                mediaType, null, -1,
                                null, null, keyArray[1],
                                null,
                                SamQFSSystemMediaManager.
                                    MAXIMUM_ENTRIES_FETCHED_OTHERS);
                }
            } else {
                // Copy Volumes
                ArchiveVSNMap vsnMap = getVSNMap(sysModel, keyArray[0]);
                mediaType = vsnMap.getArchiveMediaType();
                wrapper =
                    sysModel.getSamQFSSystemMediaManager().
                            evaluateVSNExpression(
                                mediaType, null, -1,
                                null, null, keyArray[1],
                                null,
                                SamQFSSystemMediaManager.
                                    MAXIMUM_ENTRIES_FETCHED_OTHERS);
            }

            if (MediaUtil.isDiskType(mediaType)) {
                DiskVolume [] vsns = wrapper.getAllDiskVSNs();
                for (int i = 0; i < vsns.length; i++) {
                    buf.append(vsns[i].getName());
                    buf.append("  ");
                }
            } else {
                VSN [] vsns = wrapper.getAllTapeVSNs();
                for (int i = 0; i < vsns.length; i++) {
                    buf.append(vsns[i].getVSN());
                    if (vsns[i].isReserved()) {
                        reserved = true;
                        buf.append("*");
                    }
                    buf.append("  ");
                }
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to generate matching volume string!", samEx);
            return new StringBuffer("BAD");
        }

        if (reserved) {
            buf.append("\n\n");
            buf.append(
                SamUtil.getResourceString("archiving.reservedvsninpool", "*"));
        }
        return buf;
    }

    private ArchiveVSNMap getVSNMap(
        SamQFSSystemModel sysModel, String policyInfo) throws SamFSException {

        String [] key = policyInfo.split("\\.");
        String policyName =
            key != null && key.length > 1 ?
                key[0] : "";
        int copyNumber =
            key != null && key.length > 1 ?
                Integer.parseInt(key[1]) : -1;

        ArchivePolicy thePolicy = sysModel.
            getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
        // make sure the policy wasn't deleted in the process
        if (thePolicy == null) {
            throw new SamFSException(null, -2000);
        }

        ArchiveCopy theCopy = thePolicy.getArchiveCopy(copyNumber);
        if (theCopy == null) {
            throw new SamFSException(null, -2006);
        }
        return theCopy.getArchiveVSNMap();
    }


    private boolean isShowContent() {
        // first check the page session
        String showContent = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SHOW_CONTENT);

        // second check the request
        if (showContent == null) {
            showContent = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SHOW_CONTENT);

            if (showContent != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.SHOW_CONTENT,
                    showContent);
            } else {
                throw new IllegalArgumentException(
                    "Show Content boolean not supplied");
            }
        }

        return Boolean.valueOf(showContent).booleanValue();
    }

    private boolean isShowLineControl() {
        // first check the page session
        String showLineControl = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SHOW_LINE_CONTROL);

        // second check the request
        if (showLineControl == null) {
            showLineControl = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SHOW_LINE_CONTROL);

            if (showLineControl != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.SHOW_LINE_CONTROL,
                    showLineControl);
            } else {
                throw new IllegalArgumentException(
                    "Show Line Control boolean not supplied");
            }
        }

        return Boolean.valueOf(showLineControl).booleanValue();
    }

    private boolean isResourceKeyName() {
        // first check the page session
        String resourceKey = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.RESOURCE_KEY_NAME);
        boolean isResourceKey = false;
        // second check the request
        if (resourceKey == null) {
            resourceKey = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.RESOURCE_KEY_NAME);
        }
        isResourceKey = (resourceKey != null) ? true : false;

        return isResourceKey;
    }

    private boolean isStagingQList() {
        // first check the page session
        String stageQList =
            (String) getPageSessionAttribute(SAMQFS_STAGE_Q_LIST);

        // second check the request
        if (stageQList == null) {
            stageQList =
                RequestManager.getRequest().getParameter(SAMQFS_STAGE_Q_LIST);

            if (stageQList != null) {
                setPageSessionAttribute(SAMQFS_STAGE_Q_LIST, stageQList);
            } else {
                setPageSessionAttribute(SAMQFS_STAGE_Q_LIST, "false");
            }
        }

        return Boolean.valueOf(stageQList).booleanValue();
    }

    private int getStartLine() {
        Integer startLineObj =
            (Integer) getPageSessionAttribute(START_LINE);
        if (startLineObj == null) {
            setStartLine(0);
            return 0;
        } else {
            return startLineObj.intValue();
        }
    }

    private void setStartLine(int startLine) {
        setPageSessionAttribute(START_LINE, new Integer(startLine));
    }

    private int getEndLine() {
        Integer endLineObj =
            (Integer) getPageSessionAttribute(END_LINE);
        if (endLineObj == null) {
            setEndLine(199);
            return 199;
        } else {
            return endLineObj.intValue();
        }
    }

    private void setEndLine(int endLine) {
        setPageSessionAttribute(END_LINE, new Integer(endLine));
    }

    public void handlePreviousButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        int startLine = getStartLine() - 200;
        startLine = startLine < 0 ? 0 : startLine;

        int endLine = getEndLine() - 200;
        endLine = endLine < 199 ? 199 : endLine;

        setStartLine(startLine);
        setEndLine(endLine);

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleNextButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        int startLine = getStartLine() + 200;
        int endLine   = getEndLine() + 200;

        setStartLine(startLine);
        setEndLine(endLine);

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }


    public void handleGoButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        int goToLine = Integer.parseInt(
            ((String) getDisplayFieldValue(CHILD_TEXTFIELD)).trim());
        setStartLine(goToLine);
        setEndLine(goToLine + 200);

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * To avoid throwing java exception to the console debug log
     * @param event
     * @throws javax.servlet.ServletException
     * @throws java.io.IOException
     */
    public void handleCancelRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        getParentViewBean().forwardTo(getRequestContext());
    }
}
