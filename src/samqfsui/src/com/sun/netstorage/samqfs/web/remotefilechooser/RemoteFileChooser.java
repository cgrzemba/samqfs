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

// ident	$Id: RemoteFileChooser.java,v 1.17 2008/05/16 18:39:05 am143972 Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.fs.RemoteFile;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCImage;
import com.sun.web.ui.model.CCFileChooserModelInterface;
import com.sun.web.ui.view.filechooser.CCFileChooser;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCImageField;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCTextField;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import javax.servlet.ServletException;


public class RemoteFileChooser extends CCFileChooser {

    /** Child view name for view by page href. */
    public static final String CHILD_PAGINATION_HREF =
        "PaginationHref";

    /** Child view name for view by page image. */
    public static final String CHILD_PAGINATION_IMAGE =
        "PaginationImage";

    /** Child view name for pagination first href. */
    public static final String CHILD_PAGINATION_FIRST_HREF =
        "PaginationFirstHref";

    /** Child view name for pagination first image. */
    public static final String CHILD_PAGINATION_FIRST_IMAGE =
        "PaginationFirstImage";

    /** Child view name for pagination last href. */
    public static final String CHILD_PAGINATION_LAST_HREF =
        "PaginationLastHref";

    /** Child view name for pagination last image. */
    public static final String CHILD_PAGINATION_LAST_IMAGE =
        "PaginationLastImage";

    /** Child view name for pagination next href. */
    public static final String CHILD_PAGINATION_NEXT_HREF =
        "PaginationNextHref";

    /** Child view name for pagination next image. */
    public static final String CHILD_PAGINATION_NEXT_IMAGE =
        "PaginationNextImage";

    /** Child view name for pagination prev href. */
    public static final String CHILD_PAGINATION_PREV_HREF =
        "PaginationPrevHref";

    /** Child view name for pagination prev image. */
    public static final String CHILD_PAGINATION_PREV_IMAGE =
        "PaginationPrevImage";

    /** Child view name for pagination page text field. */
    public static final String CHILD_PAGINATION_PAGE_TEXTFIELD =
        "PaginationPageTextField";

    /** Child view name for pagination go button. */
    public static final String CHILD_PAGINATION_GO_BUTTON =
        "PaginationGoButton";

    public static final String CHILD_CREATE_FOLDER = "createFolderButton";

    public static final String CLEAR_CACHE = "clearCache";

    public static final int TYPE_FILE = 0;
    public static final int TYPE_FOLDER = 1;
    public static final int TYPE_FILE_AND_FOLDER = 2;

    public RemoteFileChooser(ContainerView parent,
        RemoteFileChooserModel model, String name) {

        super(parent, model, name);
        TraceUtil.trace3("name = " + name);
        // If this is the first time displaying, clear the cache
        ViewBean pvb = getParentViewBean();
        String clearCache = (String) pvb.getPageSessionAttribute(CLEAR_CACHE);
        if (clearCache == null || clearCache.equals("true")) {
            // Clear the cache upon initial display (null) or upon request
            model.clearCachedDir();
        }
        pvb.setPageSessionAttribute(CLEAR_CACHE, "false");
        TraceUtil.initTrace();
        TraceUtil.trace3("Ctor being called");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_PAGINATION_FIRST_HREF, CCHref.class);
        registerChild(CHILD_PAGINATION_FIRST_IMAGE, CCImageField.class);
        registerChild(CHILD_PAGINATION_HREF, CCHref.class);
        registerChild(CHILD_PAGINATION_IMAGE, CCImageField.class);
        registerChild(CHILD_PAGINATION_LAST_HREF, CCHref.class);
        registerChild(CHILD_PAGINATION_LAST_IMAGE, CCImageField.class);
        registerChild(CHILD_PAGINATION_NEXT_HREF, CCHref.class);
        registerChild(CHILD_PAGINATION_NEXT_IMAGE, CCImageField.class);
        registerChild(CHILD_PAGINATION_PREV_HREF, CCHref.class);
        registerChild(CHILD_PAGINATION_PREV_IMAGE, CCImageField.class);
        registerChild(CHILD_PAGINATION_PAGE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_PAGINATION_GO_BUTTON, CCButton.class);
        registerChild(CHILD_CREATE_FOLDER, CCButton.class);
    }

    protected View createChild(String name) {
        if (name.equals(CHILD_PAGINATION_HREF)) {
            CCHref child = new CCHref(this, name, null);
            return child;
        } else if (name.equals(CHILD_PAGINATION_IMAGE)) {
            CCImageField child = new CCImageField(this, name, null);
            return child;
        } else if (name.equals(CHILD_PAGINATION_FIRST_HREF)) {
            CCHref child = new CCHref(this, name, null);
            child.setTitle("table.paginationGoToFirst");
            return child;
        } else if (name.equals(CHILD_PAGINATION_FIRST_IMAGE)) {
            CCImageField child = new CCImageField(this, name, null);
            child.setAlt("table.paginationGoToFirst");
            return child;
        } else if (name.equals(CHILD_PAGINATION_PREV_HREF)) {
            CCHref child = new CCHref(this, name, null);
            child.setTitle("table.paginationGoToPrevious");
            return child;
        } else if (name.equals(CHILD_PAGINATION_PREV_IMAGE)) {
            CCImageField child = new CCImageField(this, name, null);
            child.setAlt("table.paginationGoToPrevious");
            return child;
        } else if (name.equals(CHILD_PAGINATION_NEXT_HREF)) {
            CCHref child = new CCHref(this, name, null);
            child.setTitle("table.paginationGoToNext");
            return child;
        } else if (name.equals(CHILD_PAGINATION_NEXT_IMAGE)) {
            CCImageField child = new CCImageField(this, name, null);
            child.setAlt("table.paginationGoToNext");
            return child;
        } else if (name.equals(CHILD_PAGINATION_LAST_HREF)) {
            CCHref child = new CCHref(this, name, null);
            child.setTitle("table.paginationGoToLast");
            return child;
        } else if (name.equals(CHILD_PAGINATION_LAST_IMAGE)) {
            CCImageField child = new CCImageField(this, name, null);
            child.setAlt("table.paginationGoToLast");
            return child;
        } else if (name.equals(CHILD_PAGINATION_PAGE_TEXTFIELD)) {
            CCTextField child = new CCTextField(this, name, null);
            return child;
        } else if (name.equals(CHILD_PAGINATION_GO_BUTTON)) {
            CCButton child = new CCButton(this, name, "table.paginationGo");
            child.setTitle("table.paginationGoToPage");
            child.setAlt("table.paginationGoToPage");
            return child;
        } else if (name.equals(CHILD_CREATE_FOLDER)) {
            CCButton child = new CCButton(this, name,
                                          SamUtil.getResourceString(
                                                "browser.createFolder"));
            child.setTitle(SamUtil.getResourceString(
                            "browser.createFolderAlt"));
            child.setAlt(SamUtil.getResourceString(
                            "browser.createFolderAlt"));
            child.setType(CCButton.TYPE_SECONDARY);
            return child;
        } else {
            return super.createChild(name);
        }
    }

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");

        super.beginDisplay(event);
    }

    // initialize pagination controls.
    public void initPaginationControls() {
        if (model == null) {
            return;
        }

        TraceUtil.trace3("Entering");

        RemoteFileChooserModel rmodel = (RemoteFileChooserModel) model;

        setDisplayFieldValue(CHILD_PAGINATION_PAGE_TEXTFIELD,
            Integer.toString(rmodel.getCurrentPage() + 1));

        // Set pagination icon values.
        CCHref href = (CCHref) getChild(CHILD_PAGINATION_HREF);
        CCImageField image = (CCImageField) getChild(CHILD_PAGINATION_IMAGE);

        if (rmodel.getTotalPages() > 1) {
            image.setValue(CCImage.TABLE_SCROLL_PAGE);
            image.setHeight(Integer.parseInt(CCImage.TABLE_SCROLL_PAGE_HEIGHT));
            image.setWidth(Integer.parseInt(CCImage.TABLE_SCROLL_PAGE_WIDTH));
        } else {
            image.setValue(CCImage.TABLE_SCROLL_PAGE_DISABLED);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_SCROLL_PAGE_DISABLED_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_SCROLL_PAGE_DISABLED_WIDTH));
        }

        // Set pagination "first" icon values.
        image = (CCImageField) getChild(CHILD_PAGINATION_FIRST_IMAGE);

        if (rmodel.getCurrentPage() > 0) {
            image.setValue(CCImage.TABLE_PAGINATION_FIRST);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_FIRST_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_FIRST_WIDTH));
        } else {
            image.setValue(CCImage.TABLE_PAGINATION_FIRST_DISABLED);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_FIRST_DISABLED_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_FIRST_DISABLED_WIDTH));
        }

        // Set pagination "prev" icon values.
        image = (CCImageField) getChild(CHILD_PAGINATION_PREV_IMAGE);

        if (rmodel.getCurrentPage() > 0) {
            image.setValue(CCImage.TABLE_PAGINATION_PREV);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_PREV_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_PREV_WIDTH));
        } else {
            image.setValue(CCImage.TABLE_PAGINATION_PREV_DISABLED);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_PREV_DISABLED_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_PREV_DISABLED_WIDTH));
        }

        // Set pagination "next" icon values.
        image = (CCImageField) getChild(CHILD_PAGINATION_NEXT_IMAGE);

        if (rmodel.getCurrentPage() < (rmodel.getTotalPages() - 1)) {
            image.setValue(CCImage.TABLE_PAGINATION_NEXT);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_NEXT_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_NEXT_WIDTH));
        } else {
            image.setValue(CCImage.TABLE_PAGINATION_NEXT_DISABLED);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_NEXT_DISABLED_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_NEXT_DISABLED_WIDTH));
        }

        // Set pagination "last" icon values.
        image = (CCImageField) getChild(CHILD_PAGINATION_LAST_IMAGE);

        if (rmodel.getCurrentPage() < (rmodel.getTotalPages() - 1)) {
            image.setValue(CCImage.TABLE_PAGINATION_LAST);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_LAST_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_LAST_WIDTH));
        } else {
            image.setValue(CCImage.TABLE_PAGINATION_LAST_DISABLED);
            image.setHeight(Integer.parseInt(
                CCImage.TABLE_PAGINATION_LAST_DISABLED_HEIGHT));
            image.setWidth(Integer.parseInt(
                CCImage.TABLE_PAGINATION_LAST_DISABLED_WIDTH));
        }

        // Set pagination "go" button values.
        if (rmodel.getTotalPages() <= 1) {
            CCButton button = (CCButton) getChild(CHILD_PAGINATION_GO_BUTTON);
            CCTextField textField = (CCTextField) getChild(
                CHILD_PAGINATION_PAGE_TEXTFIELD);
            button.setDisabled(true);
            textField.setDisabled(true);
        }
    }

    /**
     * Request handler for pagination first href.
     */
    public void handlePaginationFirstHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        RemoteFileChooserModel rmodel = (RemoteFileChooserModel) model;
        rmodel.setCurrentPage(0);

        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Request handler for pagination last href.
     */
    public void handlePaginationLastHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        RemoteFileChooserModel rmodel = (RemoteFileChooserModel) model;
        rmodel.setCurrentPage(rmodel.getTotalPages() - 1);

        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Request handler for pagination next href.
     */
    public void handlePaginationNextHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        RemoteFileChooserModel rmodel = (RemoteFileChooserModel) model;
        rmodel.setCurrentPage(rmodel.getCurrentPage() + 1);

        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Request handler for pagination previous href.
     */
    public void handlePaginationPrevHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        RemoteFileChooserModel rmodel = (RemoteFileChooserModel) model;
        rmodel.setCurrentPage(rmodel.getCurrentPage() - 1);

        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Request handler for pagination go button.
     */
    public void handlePaginationGoButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String page = (String) getDisplayFieldValue(
            CHILD_PAGINATION_PAGE_TEXTFIELD);

        RemoteFileChooserModel rmodel = (RemoteFileChooserModel) model;
        try {
            rmodel.setCurrentPage(Integer.parseInt(page) - 1);
        } catch (NumberFormatException e) {
        }

        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleCreateFolderButtonRequest(RequestInvocationEvent event) {

        String folderName = getDisplayFieldStringValue(FILE_NAME_TEXT);
        try {
            if (folderName == null || folderName.length() == 0) {
                throw new SamFSException(SamUtil.getResourceString(
                                    "browser.create.error.noFolderSpecified"));
            }

            // Make sure user did not enter any paths, only directory names.
            File file = new File(folderName);
            if (file.getParent() != null) {
                throw new SamFSException(SamUtil.getResourceString(
                                    "browser.create.error.noPathsAllowed"));
            }

            // Folder name should not contain any wacky characters
            folderName = folderName.trim();
            if (!SamUtil.isValidNonSpecialCharString(folderName)) {
                throw new SamFSException(SamUtil.getResourceString(
                                        "browser.create.error.invalidChars"));
            }

            // Make sure folder does not already exist
            RemoteFileChooserModel model =
                                    (RemoteFileChooserModel) this.getModel();
            SamQFSSystemFSManager fsMgr =
                                    SamUtil.getModel(model.getServerName())
                                           .getSamQFSSystemFSManager();
            String path = model.getCurrentDirectory();
            RemoteFile[] remoteFiles = model.getDirEntries(
                                        model.getMaxEntries(), path, null);
            if (remoteFiles != null) {
                for (int i = 0; i < remoteFiles.length; i++) {
                    if (remoteFiles[i].getName().equals(folderName)) {
                        throw new SamFSException(SamUtil.getResourceString(
                                        "browser.create.error.folderExists"));
                    }
                }
            }

            // Try to create
            NonSyncStringBuffer fullPath = new NonSyncStringBuffer()
                .append(path);
            if (!path.endsWith("/")) {
                fullPath.append("/");
            }
            fullPath.append(folderName);
            fsMgr.createDirectory(fullPath.toString());

            // Clea cache so that the directory is refreshed.
            model.clearCachedDir();
        } catch (SamFSException e) {
            SamUtil.processException(
                e,
                this.getClass(),
                "handleCreateFolderButtonRequest",
                "Unable to create new folder in folder chooser.",
                this.getModel().getServerName());
            if (folderName == null) {
                folderName = "";
            }
            displayAlert(SamUtil.getResourceString("browser.create.error",
                                                   new String[] { folderName }),
                         e.getMessage(), null, null);
        }
        getParentViewBean().forwardTo(getRequestContext());
    }

    // overwrite handler for MoveUp button
    public void handleMoveUpRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        // We have to handle windows based filesystems also in
        // which case there might be a drive name followed by
        // the file name. In such situations the root directory
        // would be something like c:/.

        // In some browsers the first button (up one level) gets activated
        // when the enter key is pressed. The code below tracks this
        // scenario using a hidden field and transfers control to the
        // appropriate method when this happens.

        String flag = (String) getDisplayFieldValue(ENTER_FLAG);
        if (flag.equals(ENTER_KEY_PRESSED)) {
            handleLookInEnteredRequest(event);
        } else {
            setSortField();
            RemoteFileChooserModel model = (RemoteFileChooserModel) getModel();
            String dir = (String) getDisplayFieldValue(LOOK_IN_TEXTFIELD);
            String newDir = dir;
            int firstIndex = dir.indexOf(File.separator);
            int lastIndex = dir.lastIndexOf(File.separator);

            if (firstIndex == lastIndex) { // we are probably at root
                if (dir.length() > (firstIndex + 1)) {
                    newDir = dir.substring(0, firstIndex + 1);
                } else {
                    displayAlert("filechooser.errMoveUpErrSum",
                        "filechooser.errMoveUpErrDet", null, null);
                }
            } else {
                // strip the last separator if dir ends with one
                // example : /etc/
                if (dir.endsWith(File.separator)) {
                    newDir = dir.substring(0, lastIndex);
                }
                lastIndex = newDir.lastIndexOf(File.separator);
                if (lastIndex == 0) {
                    newDir = File.separator;
                } else {
                    newDir = newDir.substring(0, lastIndex);
                }
            }

            // now check if we've reached root_dir
            String rootDir = model.getHomeDirectory();
            if (!newDir.startsWith(rootDir)) {
                displayAlert(
                    SamUtil.getResourceString(
                        "filechooser.error.permission.summary"),
                    SamUtil.getResourceString(
                        "filechooser.error.permission.detail",
                        new String[] { newDir }),
                    null, null);
                newDir = dir;
            }
            setDisplayFieldValue(LOOK_IN_TEXTFIELD, newDir);
            setDisplayFieldValue(FILE_NAME_TEXT, null);
            model.setCurrentDirectory(newDir);
            ((CCSelectableList) getChild(FILE_LIST)).setValue(null);
            ViewBean targetView = getParentViewBean();
            targetView.forwardTo(getRequestContext());
        }
    }

    public void handleLookInEnteredRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        setSortField();
        String currentDirectory = null;
        String lookInDir =
            ((String) getDisplayFieldValue(LOOK_IN_TEXTFIELD)).trim();

        if ((lookInDir == null) || (lookInDir.length() == 0)) {
            // if directory entered is null, use the homedir instead.
            // if the user does not set the homedir value it will default
            // to the root file system
            currentDirectory = model.getHomeDirectory();
        } else {
            currentDirectory = stripExtraSeparators(lookInDir);
            String rootDir = model.getHomeDirectory();
            try {
                boolean flag = true;
                if (!currentDirectory.startsWith(rootDir)) {
                    displayAlert(
                        SamUtil.getResourceString(
                            "filechooser.error.lookin.summary"),
                        SamUtil.getResourceString(
                            "filechooser.error.lookin.detail",
                            new String[] { currentDirectory }),
                        null, null);
                    currentDirectory = model.getCurrentDirectory();
                } else if (!model.canRead(currentDirectory)) {
                        displayAlert("filechooser.cannotCompleteErrSum",
                            "filechooser.cannot_read", null, null);
                    currentDirectory = model.getCurrentDirectory();
                } else {
                    File[] list = model.getFiles(currentDirectory);
                    if (list == null) {
                        // invalid dir, set to original dir
                        // and display error message.
                        displayAlert("filechooser.cannotCompleteErrSum",
                            "filechooser.nameNotFolderErrDet",
                            null, new String [] {lookInDir});
                        currentDirectory = model.getCurrentDirectory();
                    }
                }
            } catch (Exception e) {
                displayAlert("filechooser.cannotCompleteErrSum",
                    "filechooser.nameNotFolderErrDet",
                    null,
                    new String [] {lookInDir});
                currentDirectory = model.getCurrentDirectory();
            }
        }
        model.setCurrentDirectory(currentDirectory);
        setDisplayFieldValue(LOOK_IN_TEXTFIELD, currentDirectory);
        setDisplayFieldValue(ENTER_FLAG, null);

        ViewBean targetView = getParentViewBean();
        targetView.forwardTo(getRequestContext());
    }

    /**
     * Utility method to strip extra consecutive file separators.
     */
    protected String stripExtraSeparators(String inStr) {
        if (inStr == null) {
            return null;
        }

        NonSyncStringBuffer outStr = new NonSyncStringBuffer();
        boolean repeat = false;
        int nChars = inStr.length();
        for (int i = 0; i < nChars; i++) {
            if (inStr.charAt(i) == File.separatorChar) {
                if (!repeat) {
                    outStr.append(inStr.charAt(i));
                    repeat = true;
                }
            } else {
                repeat = false;
                outStr.append(inStr.charAt(i));
            }
        }
        return outStr.toString();
    }

    /**
     * Get the list of files/folders that was selected by the user
     * The list will be an array of Strings representing the file/folder
     * names.
     * Files/folders in the selected filed/folders textfield will get
     * precedence over those selected from the listbox.
     * @return an arry of resource name strings
     */
    public String [] getSelectedResources() {

        // First check if user had entered any data in the
        // "Selected File(s)/Folder(s)" textfield.

        RemoteFileChooserModel model = (RemoteFileChooserModel) this.getModel();
        String selectedResource =
            ((String)getDisplayFieldValue(FILE_NAME_TEXT)).trim();
        String currentDirectory =
            ((String)getDisplayFieldValue(LOOK_IN_TEXTFIELD)).trim();

        // If current dir is empty get it from the model.

        if ((currentDirectory == null) || (currentDirectory.length() == 0)) {
            currentDirectory = model.getCurrentDirectory();
        }

        if ((selectedResource != null) && (selectedResource.length() > 0)) {
            currentDirectory = checkSelectedResource(selectedResource,
                currentDirectory, model);
            return model.getSelectedFiles();
        }

        // Otherwise return files/folders that were selected from the
        // list box.

        CCSelectableList fileList = (CCSelectableList) getChild(FILE_LIST);
        Object [] selectedValue = fileList.getValues();
        model.clearFiles();

        if ((selectedValue == null) || (selectedValue.length == 0)) {
            return null;
        } else {
            File[] files = null;
            try {
                files = model.getFiles(currentDirectory);
            } catch (Exception e) {
                return null;
            }

            if (model.getType().equals(
                RemoteFileChooserModel.FILE_AND_FOLDER_CHOOSER)) {
                // allow selection of files and directories
                for (int i = 0; i < selectedValue.length; i++) {
                    String strIndex = (String) selectedValue[i];
                    int index = new Integer(strIndex).intValue();
                    addSelectedFile(currentDirectory, model, files[index]);
                }
            } else if (model.getType().equals(
                CCFileChooserModelInterface.FILE_CHOOSER)) {
                try {
                    for (int i = 0; i < selectedValue.length; i++) {
                        String strIndex = (String) selectedValue[i];
                        int index = new Integer(strIndex).intValue();
                        if (model.isDirectory(
                            files[index].getAbsolutePath())) {
                            displayAlert("filechooser.cannotCompleteErrSum",
                            "filechooser.cannotSelectFolder", null, null);
                            model.clearFiles();
                            break;
                        } else {
                            addSelectedFile(
                                currentDirectory, model, files[index]);
                        }
                    }
                } catch (SamFSException e) {
                    TraceUtil.trace1(
                        "Failed in checking if path is a directory",  e);
                }
            } else {
                try {
                    for (int i = 0; i < selectedValue.length; i++) {
                        String strIndex = (String) selectedValue[i];
                        int index = new Integer(strIndex).intValue();
                        if (model.isFile(files[index].getAbsolutePath())) {
                            displayAlert("filechooser.cannotCompleteErrSum",
                                "filechooser.cannotSelectFile", null, null);
                            model.clearFiles();
                            break;
                        } else {
                            addSelectedFile(
                                currentDirectory, model, files[index]);
                        }
                    }
                } catch (SamFSException e) {
                    TraceUtil.trace1(
                        "Failed in checking if path is a file",  e);
                }
            }
        }
        return model.getSelectedFiles();
    }

    /**
     * This method checks the data entered in the selected file/folder
     * text field and takes appropriate action. Returns the currentDirectory.
     */
    protected String checkSelectedResource(String selectedResource,
        String currentDirectory, RemoteFileChooserModel model) {

        // set value to indicate file or folder chooser type (or both!).
        int chooserType;
        if (model.getType().equals(CCFileChooserModelInterface.FILE_CHOOSER)) {
            chooserType = TYPE_FILE;
        } else if (model.getType().
                    equals(CCFileChooserModelInterface.FOLDER_CHOOSER)) {
            chooserType = TYPE_FOLDER;
        } else {
            chooserType = TYPE_FILE_AND_FOLDER;
        }

        model.clearFiles();

        // Contents of Selected File textfield could be a list of files
        // separated by colons. If colon is in entry name, it will
        // be escaped (\:).  Don't split at those.
        // There is probably some clever regular expression that can do this
        // but I can't for the life of me figure out what it is...
        ArrayList tokens = new ArrayList();
        int entryStartIdx = 0;
        int startIdx = 0;
        int foundIdx = 0;
        for (int i = 0; i < selectedResource.length(); i++) {
            // Find next :
            foundIdx = selectedResource.indexOf(':',  startIdx);
            if (foundIdx == -1) {
                tokens.add(selectedResource.substring(entryStartIdx));
                break;
            }
            // Skip escaped colons (\:)
            if (foundIdx > 0 && selectedResource.charAt(foundIdx - 1) == '\\') {
                startIdx = foundIdx + 1;
                continue;
            }
            tokens.add(selectedResource.substring(entryStartIdx, foundIdx));
            startIdx = foundIdx + 1;
            entryStartIdx = startIdx;
        }

        String absolutePath = null;

        try {
            for (int i = 0; i < tokens.size(); i++) {
                // Replace escaped colons with regular ones.
                String token = (String) tokens.get(i);
                token = token.replaceAll("\\\\:", ":");

                absolutePath = getAbsolutePath(token.trim(),
                                               currentDirectory);

                // Check permissions
                if (!model.canRead(absolutePath)) {
                    displayAlert("filechooser.cannotCompleteErrSum",
                        "filechooser.cannot_read", null, null);
                    currentDirectory = model.getCurrentDirectory();
                    model.clearFiles();
                    break;
                }

                // Verify a valid file or directory entry
                if (chooserType == TYPE_FILE) {
                    if (model.isFile(absolutePath)) {
                        model.addSelectedFile(absolutePath);
                    } else if (model.isDirectory(absolutePath)) {
                        displayAlert(
                            "filechooser.cannotCompleteErrSum",
                            "filechooser.cannotSelectFolder",
                            null, null);
                        model.clearFiles();
                        break;
                    } else {
                        displayAlert(
                            "filechooser.cannotCompleteErrSum",
                            "filechooser.fileSelectError",
                            null, null);
                        model.clearFiles();
                        break;
                    }
                } else if (chooserType == TYPE_FOLDER) {
                    if (model.isDirectory(absolutePath)) {
                        model.addSelectedFile(absolutePath);
                    } else if (model.isFile(absolutePath)) {
                        displayAlert(
                            "filechooser.cannotCompleteErrSum",
                            "filechooser.cannotSelectFile",
                            null, null);
                        model.clearFiles();
                        break;
                    } else {
                        displayAlert(
                            "filechooser.cannotCompleteErrSum",
                            "filechooser.folderSelectError",
                            null, null);
                        model.clearFiles();
                        break;
                    }
                } else if (chooserType == TYPE_FILE_AND_FOLDER) {
                    if (model.isFile(absolutePath)) {
                        model.addSelectedFile(absolutePath);
                    } else if (model.isDirectory(absolutePath)) {
                        model.addSelectedFile(absolutePath);
                    } else {
                        displayAlert("filechooser.cannotCompleteErrSum",
                                "filechooser.fileSelectError", null, null);
                        model.clearFiles();
                        break;
                    }
                }
	    }
	} catch (Exception e) {
	    String key =
                chooserType == TYPE_FILE ?
                    "filechooser.fileSelectError" :
                    "filechooser.folderSelectError";
	    model.clearFiles();
            displayAlert("filechooser.cannotCompleteErrSum", key, null, null);
            currentDirectory = model.getCurrentDirectory();
        }
        return currentDirectory;
    }

    /**
     * This method checks the file name and returns its absolute
     * path by either prepending the current directory or simply
     * returning the file name.
     */
    protected String getAbsolutePath(String name, String currentDirectory) {

        boolean absolute = false;
        if (getOSName().startsWith(WINDOWS_OS)) {
            if ((name.startsWith("\\\\")) // windows UNC
                || (name.indexOf(":\\") != -1)) { // c:\types
                absolute = true;
            }
        } else {
            if (name.startsWith(File.separator)) {
                absolute = true;
            }
        }
        if (absolute) {
            return name;
        } else {
            return currentDirectory.concat(File.separator).concat(name);
        }
    }

    /**
     * This method returns the name of the OS this JVM is running on.
     */
    protected String getOSName() {
        return System.getProperty("os.name").toUpperCase();
    }

    /**
     * Add the selected file/folder in the model.
     */
    protected void addSelectedFile(String currentDirectory,
	CCFileChooserModelInterface model, File file) {

	NonSyncStringBuffer fileName =
            new NonSyncStringBuffer();

        fileName.append(currentDirectory);
        if (!currentDirectory.endsWith(File.separator)) {
            fileName.append(File.separator);
        }
            fileName.append(file.getName());
            model.addSelectedFile(fileName.toString());
    }
}
