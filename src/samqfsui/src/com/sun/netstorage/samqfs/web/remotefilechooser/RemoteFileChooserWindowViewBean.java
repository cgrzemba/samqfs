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

// ident	$Id: RemoteFileChooserWindowViewBean.java,v 1.19 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.util.StringTokenizer2;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.servlet.common.TagsViewBeanBase;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.filechooser.CCFileChooserWindow;
import com.sun.web.ui.view.filechooser.CCPopupEventHandlerInterface;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.masthead.CCSecondaryMasthead;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.File;
import java.io.IOException;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpSession;

public class RemoteFileChooserWindowViewBean extends TagsViewBeanBase
    implements CCPopupEventHandlerInterface {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "RemoteFileChooserWindow";

    // The URL that points to the JSP which uses this ViewBean
    public static final String DEFAULT_DISPLAY_URL =
        "/jsp/remotefilechooser/RemoteFileChooserWindow.jsp";

    public static final String REQUEST_PARAM_CURRENTDIR = "currentDirParam";

    public static final String REQUEST_PARAM_HOMEDIR = "homeDirParam";

    // the secondary masthead childview
    public static final String BROWSER_SERVER_MHEAD = "Masthead";

    // the hidden button childview
    public static final String CHILD_HIDDEN_FILELIST = "filelist";

    // the hidden button to indicate if window should be closed or not
    public static final String CHILD_HIDDEN_FIELDNAME = "fieldname";

    // the hidden object that will contain the command name that will be
    // called in the parent view when the window is closed.
    public static final String CHILD_HIDDEN_PARENT_REFRESH_CMD =
                                                        "parentRefreshCmd";

    // Hidden object to contain the name of a javascript to run when closing.
    public static final String CHILD_HIDDEN_ONCLOSE_SCRIPT = "onCloseScript";

    // the filechooser button childview
    public static final String CHILD_FILECHOOSER = "FileChooser";

    // child view to display filechooser alert messages
    public static final String CHILD_ALERT = "Alert";

    // Choose File button.
    public static final String CHOOSE_FILE_BUTTON = "ChooseButton";

    // pagetitle childview
    public static final String CHILD_PAGE_TITLE  = "PageTitle";

    public static final String FILELIST_DELIMITER = ";";

    protected CCPageTitleModel popupFCPageTitleModel;

    private RemoteFileChooserModel chooserModel = null;

    // Constructor
    public RemoteFileChooserWindowViewBean(RequestContext requestContext) {
        super(PAGE_NAME);

        TraceUtil.initTrace();
        TraceUtil.trace3("Ctor being called");

        setRequestContext(requestContext);
        setDefaultDisplayURL(DEFAULT_DISPLAY_URL);

        // Deserialize the pagesession so that attributes set in
        // the page session can be made available in the creatChild
        // method. This is needed to retrieve the URL parameters.

        deserializePageAttributes();

        popupFCPageTitleModel = createPageTitleModel();
        registerChildren();
    }

    /** Create the page title model. */
    protected CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("createPageTitleModel being called");
        // Construct a table model using XML string.
        CCPageTitleModel model = new CCPageTitleModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/remotefilechooser/RemoteFileChooserPageTitle.xml");
        return model;
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(BROWSER_SERVER_MHEAD, CCSecondaryMasthead.class);
        registerChild(CHILD_FILECHOOSER, RemoteFileChooser.class);
        registerChild(CHILD_HIDDEN_FILELIST, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FIELDNAME, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_PARENT_REFRESH_CMD, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_ONCLOSE_SCRIPT, CCHiddenField.class);
        registerChild(CHILD_PAGE_TITLE, CCPageTitle.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        popupFCPageTitleModel.registerChildren(this);
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {

        if (name.equals(CHILD_HIDDEN_FILELIST)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_FIELDNAME)) {
            RemoteFileChooserModel fileChooserModel = getFCModel();
            String fieldName = fileChooserModel.getFieldName();
            return new CCHiddenField(this, name, fieldName);
        } else if (name.equals(CHILD_HIDDEN_PARENT_REFRESH_CMD)) {
            RemoteFileChooserModel fileChooserModel = getFCModel();
            return new CCHiddenField(this, name,
                                     fileChooserModel.getParentRefreshCmd());
        } else if (name.equals(CHILD_HIDDEN_ONCLOSE_SCRIPT)) {
            RemoteFileChooserModel fileChooserModel = getFCModel();
            return new CCHiddenField(this, name,
                                     fileChooserModel.getOnClose());
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child = new CCAlertInline(this, name, null);
            return child;
        } else if (name.equals(CHILD_FILECHOOSER)) {
            RemoteFileChooserModel fileChooserModel = getFCModel();
            fileChooserModel.setPopupMode(true);
            fileChooserModel.setAlertChildView(CHILD_ALERT);
            RemoteFileChooser child =
                new RemoteFileChooser(this, fileChooserModel, name);
            return child;
        } else if (name.equals(BROWSER_SERVER_MHEAD)) {
            CCSecondaryMasthead child =
                new CCSecondaryMasthead(this, name);

            RemoteFileChooserModel fileChooserModel =
                (RemoteFileChooserModel) getFCModel();

            if (fileChooserModel != null) {
                child.setHeight(fileChooserModel.getProductNameHeight());
                child.setWidth(fileChooserModel.getProductNameWidth());
            }
            return child;

        } else if (name.equals(CHILD_PAGE_TITLE)) {
            RemoteFileChooserModel fcModel = getFCModel();
            CCButton chooseButton = (CCButton) getDisplayField("ChooseButton");
            CCButton cancelButton = (CCButton) getDisplayField("CancelButton");
            TraceUtil.trace3("fcModel get Type = " + fcModel.getType());
            if (fcModel.getType().equals(
                RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER)) {
                if (fcModel.multipleSelect()) {
                    popupFCPageTitleModel.setValue(
                        "ChooseButton", "filechooser.chooseFiles");
                } else {
                    popupFCPageTitleModel.setValue(
                        "ChooseButton", "filechooser.chooseFile");
                }
                chooseButton.setTitle("filechooser.chooseFileTooltip");
            } else if (fcModel.getType().equals(
                RemoteFileChooserModel.FILE_CHOOSER)) {
                if (fcModel.multipleSelect()) {
                    popupFCPageTitleModel.setValue(
                        "ChooseButton", "filechooser.chooseFiles");
                } else {
                    popupFCPageTitleModel.setValue(
                        "ChooseButton", "filechooser.chooseFile");
                }
                chooseButton.setTitle("filechooser.chooseFileTooltip");
            } else {
                if (fcModel.multipleSelect()) {
                    popupFCPageTitleModel.setValue(
                        "ChooseButton", "filechooser.chooseFolders");
                } else {
                    popupFCPageTitleModel.setValue(
                        "ChooseButton", "filechooser.chooseFolder");
                }
                chooseButton.setTitle("filechooser.chooseFolderTooltip");
            }
            cancelButton.setTitle("filechooser.cancelMsg");
            popupFCPageTitleModel.setValue(
                "CancelButton", "filechooser.cancel");
            CCPageTitle child =
                new CCPageTitle(this, popupFCPageTitleModel, name);
            return child;
        } else if (popupFCPageTitleModel.isChildSupported(name)) {
            // Create child from page title model.
            return popupFCPageTitleModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Get the filechooser model from the session
     */
    private RemoteFileChooserModel getFCModel() {

        if (this.chooserModel != null) {
            return this.chooserModel;
        }

        HttpServletRequest request = getRequestContext().getRequest();
        HttpSession session = request.getSession();
        String model_key =
            request.getParameter(CCFileChooserWindow.MODEL_KEY);

        // get serverName from client side
        // (we can also get rootDir and startingDir from client side if needed)
        String serverName =
            request.getParameter(Constants.Parameters.SERVER_NAME);
        TraceUtil.trace2("serverNameParam=" + serverName);

        String currentDir = request.getParameter(REQUEST_PARAM_CURRENTDIR);

        String homeDir    = request.getParameter(REQUEST_PARAM_HOMEDIR);

        if ((model_key != null) && (model_key.length() != 0)) {
            setPageSessionAttribute(
                CCFileChooserWindow.MODEL_KEY, model_key);
            this.chooserModel =
                    (RemoteFileChooserModel)session.getAttribute(model_key);
        } else {
            model_key = (String)
                getPageSessionAttribute(CCFileChooserWindow.MODEL_KEY);
            if (model_key != null) {
                setPageSessionAttribute(
                    CCFileChooserWindow.MODEL_KEY, model_key);
                this.chooserModel = (RemoteFileChooserModel)
                                            session.getAttribute(model_key);
            } else {
                this.chooserModel = null;
            }
        }

        if (serverName != null) {
            TraceUtil.trace2("setting RFC serverName to " + serverName);
            this.chooserModel.setServerName(serverName);
        }

        if (currentDir != null) {
            this.chooserModel.setCurrentDirectory(currentDir);
            TraceUtil.trace3("Remote file chooser current dir:  " + currentDir);
        }

        if (homeDir != null) {
            this.chooserModel.setHomeDirectory(homeDir);
            TraceUtil.trace3("Remote file chooser home dir:  " + homeDir);
        }

        return this.chooserModel;
    }

    /**
     * Request handler for choose button.
     */
    public void handleChooseButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        RemoteFileChooserModel fileChooserModel = (RemoteFileChooserModel)
            getFCModel();
        String formFieldName = fileChooserModel.getFieldName();
        setDisplayFieldValue(CHILD_HIDDEN_FIELDNAME, formFieldName);
        CCAlertInline alertChild = (CCAlertInline) getChild(CHILD_ALERT);
        try {
            RemoteFileChooser child = (RemoteFileChooser)
                getChild(CHILD_FILECHOOSER);
            String [] fileList = child.getSelectedResources();
            TraceUtil.trace3("fileList[" +
                (fileList != null ? fileList.length : -1) + "]");

            // Check for alert.  This can be known by checking
            // the alert child's summary which should not be null.
            boolean alertRaised = false;
            String summary = alertChild.getSummary();
            if (summary != null && summary.length() > 0) {
                alertRaised = true;
            }

            if (fileList == null) {
                // Nothing selected.
                // if folder chooser, use current directory if no error.
                // if file chooser, create alert.
                TraceUtil.trace3("File list is null");
                if (fileChooserModel.getType().equals(
                    RemoteFileChooserModel.FOLDER_CHOOSER)) {
                    if (!alertRaised) {
                        // use current directory
                        TraceUtil.trace3("User selected folder " +
                            fileChooserModel.getCurrentDirectory());

                        setDisplayFieldValue(
                            CHILD_HIDDEN_FILELIST,
                            fileChooserModel.getCurrentDirectory());
                    }
                } else {
                   // Generate an error message if the childview has not
                    // generated one already.
                    TraceUtil.trace3("User selected file " +
                        fileChooserModel.getCurrentDirectory());

                    if (!alertRaised) {
                        alertChild.setSummary("filechooser.noFileSelectedSum");
                        alertChild.setDetail("filechooser.noFileSelectedDet");
                    }
                }
            } else {
                TraceUtil.trace3("File List is not null");

                NonSyncStringBuffer buffer = new NonSyncStringBuffer();
                int i = 0;
                buffer.append(fileList[i]);
                i++;
                while (i < fileList.length) {
                    buffer.append(FILELIST_DELIMITER).append(fileList[i]);
                    i++;
                }
                TraceUtil.trace3(buffer.toString());
                String fileListStr = removeDoubleSlashes(buffer.toString());
                setDisplayFieldValue(CHILD_HIDDEN_FILELIST, fileListStr);
                // There is a bug in the chooser such that the model current
                // directory is not properly set.  Set it now if the file list
                // is one long.
                if (fileList.length == 1) {
                    if (fileChooserModel.getType().equals(
                        RemoteFileChooserModel.FOLDER_CHOOSER)) {
                        TraceUtil.trace3(
                            "Setting current dir to " + fileListStr);
                        fileChooserModel.setCurrentDirectory(fileListStr);
                    } else {
                        // File chooser... get parent
                        TraceUtil.trace3("Setting current dir to parent");
                        File file = new File(fileListStr);
                        String parent = file.getParent();
                        if (parent != null) {
                            fileChooserModel.setCurrentDirectory(parent);
                        }
                    }
                }
            }
        } catch (Exception e) {
            alertChild.setSummary("filechooser.cannotCompleteErrSum");
            alertChild.setDetail(e.getMessage());
        }
        this.forwardTo(getRequestContext());
    }

    /**
     * This method is invoked when the user DBL clicks a file in the
     * popup version of the filechooser.
     *
     * @param event The request invocation event
     */
    public void dblClickEvent(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        RemoteFileChooserModel fileChooserModel = (RemoteFileChooserModel)
            getFCModel();
        String formFieldName = fileChooserModel.getFieldName();

        try {
            RemoteFileChooser child = (RemoteFileChooser)
                getChild(CHILD_FILECHOOSER);
            TraceUtil.trace3("Got child File chooser");
            String [] fileList = child.getModel().getSelectedFiles();

            // if no files are selected it must have been a folder
            // or an error message must have been created by
            // the DBL click handler in the child view.
            if (fileList != null) {
                StringBuffer buffer = new StringBuffer();
                int i = 0;
                buffer.append(fileList[i]);
                i++;
                while (i < fileList.length) {
                    buffer.append(";").append(fileList[i]);
                    i++;
                }
                String fileListStr = removeDoubleSlashes(buffer.toString());
                setDisplayFieldValue(CHILD_HIDDEN_FILELIST, fileListStr);
                setDisplayFieldValue(CHILD_HIDDEN_FIELDNAME, formFieldName);
            } else {
                setDisplayFieldValue(CHILD_HIDDEN_FIELDNAME, formFieldName);
            }
        } catch (Exception e) {
            CCAlertInline alertChild = (CCAlertInline) getChild(CHILD_ALERT);
            alertChild.setSummary("filechooser.cannotCompleteErrSum");
            alertChild.setDetail(e.getMessage());
            setDisplayFieldValue(CHILD_HIDDEN_FIELDNAME, formFieldName);
        }
        this.forwardTo(getRequestContext());
    }

    /**
     * This method is invoked when the user enters one or more file/folder
     * names in the Selected File(s)/Folder(s) text field and then hits
     * enter.
     *
     * @param event The request invocation event
     */
    public void selectFieldEnterEvent(RequestInvocationEvent event)
        throws ServletException, IOException {

        // since the filechooser child view handles most of the
        // important stuff the rest of the code is same as that
        // in dblClickEvent().

        dblClickEvent(event);
    }

    // Seems to be a bug in the Lockhart component that can leave double slashes
    // in the selected file.  REmove those doubls slashes.
    private String removeDoubleSlashes(String bufIn) {
        StringTokenizer2 strTok = new StringTokenizer2(bufIn, "//");
        if (!strTok.hasMoreTokens()) {
            return bufIn;
        }
        NonSyncStringBuffer bufOut = new NonSyncStringBuffer();
        while (strTok.hasMoreTokens()) {
            bufOut.append(strTok.nextToken())
                  .append("/");
        }

        // Remove trailing /
        if (bufOut.charAt(bufOut.length() - 1) == '/') {
            bufOut.setLength(bufOut.length() - 1);
        }
        return bufOut.toString();
    }
}
