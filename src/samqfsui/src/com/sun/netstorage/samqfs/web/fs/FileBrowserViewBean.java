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

// ident	$Id: FileBrowserViewBean.java,v 1.30 2008/12/16 00:12:10 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.Filter;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.Properties;
import javax.servlet.ServletException;

/**
 * main container for the view
 */
public class FileBrowserViewBean extends CommonViewBeanBase {
    public static final String PAGE_NAME = "FileBrowser";

    private static final String CONTAINER_VIEW = "FileBrowserView";

    // Mode Component
    public static final String MODE_RADIO = "ModeRadio";
    public static final String RECOVERY_MENU = "RecoveryMenu";
    public static final String RECOVERY_MENU_HREF  = "RecoveryMenuHref";
    public static final String LIVE_DATA_HREF = "LiveDataHref";

    // Property Sheet Components
    public static final String DIR = "Dir";
    public static final String ENTRIES = "Entries";
    public static final String FILTER_CRITERIA = "FilterCriteria";
    public static final String BASE_PATH = "BasePath";
    public static final String BASE_PATH_MENU_HREF = "BasePathMenuHref";
    public static final String APPLY = "Apply";

    // Page Session Attribute to keep track of the current browsing directory
    public static final String CURRENT_DIRECTORY = "CurrentDirectory";

    // Page Session Attribute to keep track if the mode is live, or if it is
    // in recovery point mode.  MODE will contain the snapshot path if it is
    // the latter case.
    public static final String MODE = "Mode";

    // Contain all the file name entries in the table for javascript
    public static final String FILE_NAMES = "fileNames";

    // Determine if current directory is/sub-dir of an archiving file system
    // If not, Archive/Release/Stage should not be enabled
    public static final String IS_ARCHIVING = "isArchiving";

    // Determine if current directory supports recovery mode
    public static final String SUPPORT_RECOVERY  = "supportRecovery";

    // Keep track of the file system name & mount pt for view details pop up
    public static final String FS_INFO = "fsInfo";

    // Page Session Attribute to keep track of the unmounted file system name
    public static final String UNMOUNTED_FS_NAME = "unmountedFSName";

    // Keep track of the recovery point path name for restore pop up
    public static final String SNAP_PATH = "snapPath";

    // Determine if the mount file system msg has to be shown or not
    public static final String MOUNT_MESSAGE = "MountMessage";

    // href handler if user wants to mount the unmounted fs
    public static final String MOUNT_HREF = "MountHref";

    // Page Session Attribute to keep track of the Maximum Entries
    public static final String MAX_ENTRIES = "max_entries";

    // Selected File Name
    public static final String SELECTED_FILE = "SelectedFile";
    public static final String SELECTED_FILE_IS_DIR = "SelectedFileIsDir";

    // Role of the user
    public static final String ROLE = "Role";

    // filter handlers
    private static final String FILTER_RTN_VALUE  = "filterReturnValue";
    private static final String FILTER_HANDLER = "filterHandler";
    public  static final String FILTER_VALUE = "filterValue";
    private static final String FILTER_PROMPT = "filterPrompt";
    private static final String FILTER_PAGE_TITLE = "filterPageTitle";

    // Property Sheet component that is referenced
    public static final String MAX_LABEL = "maxLabel";

    public static final String LIVE_DATA = "live";

    // Keep track of what the user is performing.  This is needed to determine
    // what error message to show when listing the directory comes back with
    // error code 31212.  If user is toggling between live & recovery pt mode,
    // we need to redirect the user to the top level of the mount point if
    // the directory of which user is browsing does not exist in that mode.
    public static final String IS_TOGGLE = "is_toggle";

    // Keep track of the last used snapshot date for the error message above
    public static final String LAST_USED_RECOVERY_PT_DATE = "recovery_pt_date";


    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // Property Sheet on top of the page
    private CCPropertySheetModel propertySheetModel = null;

    // Maximum entries allowed to be retrieved in the page
    public static int maxEntries = 1024;

    // Keep track if base path menu is used
    public static boolean basePathMenuClicked = false;

    // Holds the current file system name, retrieve the fs name in isArchiving()
    private String currentFSName;

    // Holds the relative path of the file system of which user is browsing
    private String relativePath;

    // Holds the mount point of the current file system
    private String mountPoint;


    // Holds the presented value of recovery point time
    private String recoveryPointTime;

    /* default constructor */
    public FileBrowserViewBean() {
        super(PAGE_NAME, "/jsp/fs/FileBrowser.jsp");

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        createPageTitleModel();
        createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this containers child views */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(FILTER_RTN_VALUE, CCHiddenField.class);
        registerChild(FILTER_VALUE, CCHiddenField.class);
        registerChild(FILTER_HANDLER, CCHref.class);
        registerChild(FILTER_PROMPT, CCHiddenField.class);
        registerChild(FILTER_PAGE_TITLE, CCHiddenField.class);
        registerChild(FILE_NAMES, CCHiddenField.class);
        registerChild(IS_ARCHIVING, CCHiddenField.class);
        registerChild(CONTAINER_VIEW, FileBrowserView.class);
        registerChild(BASE_PATH_MENU_HREF, CCHref.class);
        registerChild(MODE_RADIO, CCRadioButton.class);
        registerChild(RECOVERY_MENU, CCDropDownMenu.class);
        registerChild(RECOVERY_MENU_HREF, CCHref.class);
        registerChild(LIVE_DATA_HREF, CCHref.class);
        registerChild(SUPPORT_RECOVERY, CCHiddenField.class);
        registerChild(FS_INFO, CCHiddenField.class);
        registerChild(SNAP_PATH, CCHiddenField.class);
        registerChild(MOUNT_MESSAGE, CCHiddenField.class);
        registerChild(MOUNT_HREF, CCHref.class);
        registerChild(SELECTED_FILE, CCHiddenField.class);
        registerChild(SELECTED_FILE_IS_DIR, CCHiddenField.class);
        registerChild(ROLE, CCHiddenField.class);

        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);

        super.registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** create a named child view */
    public View createChild(String name) {
        if (name.equals(FILTER_RTN_VALUE) ||
            name.equals(FILTER_VALUE) ||
            name.equals(FILTER_PROMPT) ||
            name.equals(FILTER_PAGE_TITLE) ||
            name.equals(FILE_NAMES) ||
            name.equals(IS_ARCHIVING) ||
            name.equals(SUPPORT_RECOVERY) ||
            name.equals(FS_INFO) ||
            name.equals(SNAP_PATH) ||
            name.equals(MOUNT_MESSAGE) ||
            name.equals(SELECTED_FILE) ||
            name.equals(SELECTED_FILE_IS_DIR) ||
            name.equals(ROLE)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(MODE_RADIO)) {
            CCRadioButton child = new CCRadioButton(this, name, null);
            child.setOptions(new OptionList(
                new String[] {"fs.filebrowser.mode.live",
                              "fs.filebrowser.mode.recovery"},
                new String[] {"live",
                              "recovery"}));
            return child;
        } else if (name.equals(RECOVERY_MENU)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(BASE_PATH_MENU_HREF) ||
            name.equals(RECOVERY_MENU_HREF) ||
            name.equals(LIVE_DATA_HREF) ||
            name.equals(MOUNT_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(CONTAINER_VIEW)) {
            return new FileBrowserView(this, name);
        } else if (name.equals(FILTER_HANDLER)) {
            return new CCHref(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
           return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            return
                PropertySheetUtil.createChild(this, propertySheetModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private void createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
    }

    private void createPropertySheetModel() {
        if (propertySheetModel == null) {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/FileBrowserPropertySheet.xml");
        }
    }

    /** begin processing the JSP bound to this view bean */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        checkRolePrivilege();

        try {
            FileBrowserView view = (FileBrowserView) getChild(CONTAINER_VIEW);
            view.clearTableModel(basePathMenuClicked);

            // fs information is used in both populateBasePathMenu &
            // checkCurrentDirectory
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            SamQFSSystemFSManager fsManager =
                sysModel.getSamQFSSystemFSManager();
            GenericFileSystem[] genfs = fsManager.getNonSAMQFileSystems();
            FileSystem [] fs = fsManager.getAllFileSystems();

            loadPropertySheetModel(fs, genfs);

            // Figure out if current directory exists in a SAM/Q file system
            boolean archive = isArchiving(fs, genfs);

            // currentFSName is available after isArchiving()
            SamUtil.setLastUsedFSName(getServerName(), currentFSName);

            // Preset Mode (toggle only shows if it is an archiving file system,
            // and if the fs has recovery points established)
            if (archive) {
                presetMode(sysModel);
            } else {
                ((CCHiddenField) getChild(SUPPORT_RECOVERY)).setValue("false");
                // reset current mode to live
                setMode(LIVE_DATA);
            }

            // recoveryPointTime is available after presetMode().  This
            // variable is only used in the table title when table is in
            // recovery point mode.
            view.populateTableModel(
                archive, currentFSName,
                relativePath, mountPoint,
                recoveryPointTime);

            // reset
            if (basePathMenuClicked) {
                basePathMenuClicked = false;
            }

            ((CCHiddenField) getChild(IS_ARCHIVING)).setValue(
                Boolean.toString(archive));

            ((CCHiddenField) getChild(FS_INFO)).setValue(
                currentFSName.concat("###").concat(mountPoint));

            ((CCHiddenField) getChild(SNAP_PATH)).setValue(getMode());

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to display file browser page!");
            TraceUtil.trace1("Message: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay()",
                "Failed to display file browser page!",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "fs.filebrowser.fail.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        // Preset filter settings
        presetFilter();

        // reset search mount point menu
        ((CCDropDownMenu) getChild(BASE_PATH)).setValue("");
        ((CCButton) getChild(APPLY)).setValue("fs.filebrowser.apply");

        TraceUtil.trace3("Exiting");
    }

    private void loadPropertySheetModel(
        FileSystem [] fs, GenericFileSystem[] genfs) throws SamFSException {
        ((CCTextField) getChild(DIR)).setValue(getCurrentDirectory(fs, genfs));
        ((CCTextField) getChild(ENTRIES)).
            setValue(new Integer(getMaxEntries()));

        populateBasePathMenu(fs, genfs);
    }

    private boolean isArchiving(FileSystem [] fs, GenericFileSystem[] genfs) {
        String currentDirectory = getCurrentDirectory(fs, genfs);

        // reset mount message & unmounted fs name if any
        ((CCHiddenField) getChild(MOUNT_MESSAGE)).setValue("");
        removePageSessionAttribute(UNMOUNTED_FS_NAME);

        // First build the arrayList with longest mount point path in
        // the beginning of the list.  This is to ensure we get the correct
        // file system name by comparing the current directory versus the
        // mount point of all file systems.
        ArrayList temp = new ArrayList();
        for (int i = 0; i < fs.length; i++) {
            // Skip all mountPoints that are null or zero in length
            // the file system is a problematic one
            String mp = fs[i].getMountPoint();

            if (mp == null || mp.length() == 0) {
                continue;
            }
            temp.add(new FSMountPoints(fs[i].getName(), mp));
        }
        FSMountPoints [] mps = new FSMountPoints[temp.size()];
        mps = (FSMountPoints [])temp.toArray(mps);

        // Sort the FSMountPoints
        Arrays.sort(mps);

        // Now look for fsName from end of the array that contains the longest
        // mount point
        for (int i = mps.length - 1; i >= 0; i--) {
            if (currentDirectory.startsWith(mps[i].getMountPoint())) {
                 currentFSName = mps[i].getFSName();
                 mountPoint    = mps[i].getMountPoint();
                 break;
            }
        }

        // If currentFSName is not null, this is a SAM/Q file system.
        if (currentFSName != null) {
            FileSystem selectedFS = null;
            for (int i = 0; i < fs.length; i++) {
                if (currentFSName.equals(fs[i].getName())) {
                    selectedFS = fs[i];
                    TraceUtil.trace2(
                        "FB: Selected FS Name is".concat(currentFSName));
                    break;
                }
            }

            // Check if the file system is mounted, show javascript message
            // and prompt user if he/she wants to mount it
            if (selectedFS.getState() == FileSystem.UNMOUNTED) {
                ((CCHiddenField) getChild(MOUNT_MESSAGE)).setValue(
                    SamUtil.getResourceString(
                    "fs.filebrowser.mountmessage",
                    new String [] {
                        currentDirectory,
                        currentFSName}));

                // Save the fs name just in case if user wants to mount the fs
                // and handleMountRequest will retrieve the right fs name
                // from getFSName()
                setPageSessionAttribute(UNMOUNTED_FS_NAME, currentFSName);


                // file system is not mounted, treat that as a regular UFS
                // genfs[0] is root file system
                currentFSName = genfs[0].getName();
                mountPoint    = genfs[0].getMountPoint();
                relativePath =
                    currentDirectory.substring(mountPoint.length());
                return false;
            } else {
                if (currentDirectory.equals(mountPoint)) {
                    relativePath = "";
                } else {
                    relativePath =
                        currentDirectory.substring(mountPoint.length() + 1);
                }

                // selectedFS & mountPoint class variables have been populated.
                // Now check if file system is an archiving file system or not.
                // This check has been extended to include clients and pmds if
                // the metadata server is running sam. The status flag FS_SAM
                // indicates that the metadata server is running sam.
                int fsStatusFlags = selectedFS.getStatusFlag();
                boolean isArchiving =
                    (selectedFS.getArchivingType() == FileSystem.ARCHIVING ||
                    ((fsStatusFlags & FSInfo.FS_SAM) == FSInfo.FS_SAM));

                return isArchiving;
            }

        } else {
            // If code reaches here, the current directory is not a part of a
            // sam/q file system.  Search for the fs name now.  fs name will be
            // used in the view as a part of API call to retrieve dir entries.
            for (int i = 0; i < genfs.length; i++) {
                mountPoint = genfs[i].getMountPoint();

                // Skip all mountPoints that are null or zero in length
                // the file system is a problematic one
                if (mountPoint == null || mountPoint.length() == 0) {
                    continue;
                }

                if (currentDirectory.startsWith(mountPoint)) {
                    currentFSName = genfs[i].getName();
                    relativePath =
                        currentDirectory.substring(mountPoint.length());
                    break;
                }
            }
        }

        // always return false cuz a non-sam/q fs is always a non-archiving fs
        return false;
    }

    private void populateBasePathMenu(
        FileSystem [] fs, GenericFileSystem[] genfs) throws SamFSException {

        StringBuffer labelBuf =
            new StringBuffer("fs.filebrowser.dropdownhelper.label");
        StringBuffer valueBuf = new StringBuffer("");

        for (int i = 0; i < genfs.length; i++) {
            String mountPoint = genfs[i].getMountPoint();
            if (mountPoint == null || mountPoint.length() == 0) {
                // skip problematic file system
                continue;
            }
            labelBuf.append("###").append(mountPoint).
                append(" (").append(genfs[i].getName()).append(")");
            valueBuf.append("###").append(mountPoint);
        }

        for (int i = 0; i < fs.length; i++) {
            String mountPoint = fs[i].getMountPoint();
            if (mountPoint == null ||
                mountPoint.length() == 0 ||
                fs[i].isMatFS()) {
                // skip problematic file system or MAT typed file systems
                continue;
            }
            labelBuf.append("###").append(mountPoint).
                append(" (").append(fs[i].getName()).append(")");
            valueBuf.append("###").append(mountPoint);
        }

        ((CCDropDownMenu) getChild(BASE_PATH)).setOptions(
            new OptionList(
                labelBuf.toString().split("###"),
                valueBuf.toString().split("###")));
    }

    private void presetFilter() {
        // set filter settings
        Filter filter = (Filter) getPageSessionAttribute(FILTER_VALUE);
        if (filter == null) {
            filter = new Filter(Filter.VERSION_2);
        }
        try {
            ((CCHiddenField)getChild(FILTER_VALUE)).
                setValue(filter.writeEncodedString());
            ((CCStaticTextField) getChild(FILTER_CRITERIA)).
                setValue(
                    filter.toString().length() == 0 ?
                        SamUtil.getResourceString("fs.filebrowser.nofilter") :
                        filter.toString());
        } catch (UnsupportedEncodingException uee) {
            TraceUtil.trace1("unable to encode filter criteria", uee);
            // ; perhaps rethrow this so that something shows up in the ui?
        }

        // filter prompt & page title strings
        ((CCHiddenField)getChild(FILTER_PROMPT)).
            setValue(SamUtil.getResourceString("common.filter.prompt"));
        ((CCHiddenField)getChild(FILTER_PAGE_TITLE)).
            setValue(SamUtil.getResourceString("common.filter.pagetitle"));
    }

    /** handler for the apply button */
    public void handleApplyRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        String MEString = getDisplayFieldStringValue(ENTRIES);
        String directory = getDisplayFieldStringValue(DIR);
        String serverName = getServerName();

        setCurrentDirectory(directory);

        int entries = 0;
        try {
            if (MEString != null) {
                entries = Integer.parseInt(MEString);
            } else {
                entries = maxEntries;
            }
            setMaxEntries(entries);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("Incorrect max entries defined!");
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                SamUtil.getResourceString("fs.stage.error"),
                -3333,
                SamUtil.getResourceString("fs.stage.entries.nan", MEString),
                serverName);
            ((CCLabel) getChild(MAX_LABEL)).setShowError(true);
        }

        forwardTo(getRequestContext());
    }

    public void setCurrentDirectory(String directory) {
        if (directory == null || "".equals(directory.trim())) {
            directory = "/";
        } else {
            directory = removeTrailingSlashes(directory.trim());
        }
        setPageSessionAttribute(CURRENT_DIRECTORY, directory);
    }

    /**
     * The following getCurrentDirectory method is strictly used in the View
     * and TiledView.
     */
    public String getCurrentDirectory() {
        TraceUtil.trace3("Entering");

        String currentDirectory = (String)
            getPageSessionAttribute(CURRENT_DIRECTORY);
        if (currentDirectory == null) {
            // curr. dir == null return mount point
            currentDirectory = "/";
            setCurrentDirectory(currentDirectory);
        }

        TraceUtil.trace3("Exiting");
        return currentDirectory;
    }

    /**
     * The following getCurrentDirectory method is strictly used in this
     * ViewBean only.  This method takes in all the file system information
     * just in case if it needs to retrieve the mount point of the LAST USED
     * FILE SYSTEM that is saved in the session.
     */
    private String getCurrentDirectory(
        FileSystem [] fs, GenericFileSystem[] genfs) {
        TraceUtil.trace3("Entering");

        String currentDirectory = (String)
            getPageSessionAttribute(CURRENT_DIRECTORY);
        if (currentDirectory == null) {
            // current directory is null, check session for
            // last used file system
            String lastUsedFSName = SamUtil.getLastUsedFSName(getServerName());
            if (lastUsedFSName == null) {
                currentDirectory = "/";
            } else {
                // find the mount point of the last used file system
                boolean found = false;
                for (int i = 0; i < fs.length; i++) {
                    if (fs[i].getName().equals(lastUsedFSName)) {
                        found = true;
                        String mp = fs[i].getMountPoint();
                        if (mp == null || mp.length() == 0) {
                            currentDirectory = "/";
                        } else {
                            currentDirectory = mp;
                        }
                        break;
                    }
                }
                // If not found, continue searching for mount point
                if (!found) {
                    for (int i = 0; i < genfs.length; i++) {
                        if (genfs[i].getName().equals(lastUsedFSName)) {
                            found = true;
                            String mp = genfs[i].getMountPoint();
                            if (mp == null || mp.length() == 0) {
                                currentDirectory = "/";
                            } else {
                                currentDirectory = mp;
                            }
                            break;
                        }
                    }
                }
            }
            setCurrentDirectory(currentDirectory);
        }

        TraceUtil.trace3("Exiting");
        return currentDirectory;
    }

    public void setMode(String mode) {
        if (mode == null) {
            mode = LIVE_DATA;
        }
        setPageSessionAttribute(MODE, mode);
    }

    public String getMode() {
        TraceUtil.trace3("Entering");

        String mode = (String) getPageSessionAttribute(MODE);

        if (mode == null) { // mode == null return live
            mode = LIVE_DATA;
            setMode(mode);
        }

        TraceUtil.trace3("Exiting");
        return mode;
    }

    private void presetMode(SamQFSSystemModel sysModel)
        throws SamFSException {
        // currentFSName is available after calling isArchiving()
        SamQFSSystemFSManager fsManager =
            sysModel.getSamQFSSystemFSManager();

        // second argument as null to get ALL indexed snaps in diff directory
        String [] indexedSnaps = fsManager.getIndexedSnaps(currentFSName, null);
        TraceUtil.trace3("Indexed Snaps length is ".concat(
            indexedSnaps == null ? "null" :
            Integer.toString(indexedSnaps.length)));

        boolean supportRecovery = false;

        // If there is at least one recovery point that is indexed and ready
        // to be browse, enable toggle component and populate information!
        if (indexedSnaps != null && indexedSnaps.length != 0) {
            String dumpPathInfo = getIndexedDumpPathInfo(indexedSnaps);

            if (dumpPathInfo.length() == 0) {
                // no recovery point is indexed
                TraceUtil.trace2("No recovery point is indexed!");
            } else {
                // [0] contains key (date),
                // [1] contains value (snapshot file name) and date
                String [] info = dumpPathInfo.split("@@@");
                String [] dumpPath = info[0].split("###");
                String [] dumpFile = info[1].split("###");
                supportRecovery = true;
                String mode = getMode();

                CCRadioButton radio = (CCRadioButton) getChild(MODE_RADIO);
                radio.restoreStateData();
                CCDropDownMenu menu =
                    (CCDropDownMenu) getChild(RECOVERY_MENU);
                menu.setLabelForNoneSelected("fs.filebrowser.mode.selectdate");
                menu.setOptions(new OptionList(dumpPath, dumpFile));
                if (LIVE_DATA.equals(mode)) {
                    radio.setValue(LIVE_DATA);
                    menu.setDisabled(true);
                    menu.setValue("");
                } else {
                    radio.setValue("recovery");
                    menu.setDisabled(false);
                    String menuValue = findValue(mode, dumpFile);
                    menu.setValue(menuValue);
                    recoveryPointTime = extract(menuValue, false);
                }
            }
        } else {
            TraceUtil.trace2("No recovery points found for this filesystem!");
        }

        // Set toggle mode
        // hide toggle component if fs is not mounted
        ((CCHiddenField) getChild(
            SUPPORT_RECOVERY)).setValue(Boolean.toString(supportRecovery));
    }

    protected void setMaxEntries(int maxEntries) {
        setPageSessionAttribute(MAX_ENTRIES, new Integer(maxEntries));
    }

    protected int getMaxEntries() {
        Integer maxEntriesObj =
            (Integer) getPageSessionAttribute(MAX_ENTRIES);

        if (maxEntriesObj == null) {
            setPageSessionAttribute(MAX_ENTRIES, new Integer(maxEntries));
            return maxEntries;
        }

        return maxEntriesObj.intValue();
    }

    // filter handler
    public void handleFilterHandlerRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String filterString = getDisplayFieldStringValue(FILTER_RTN_VALUE);
        if (filterString != null && filterString.length() > 0) {
            try {
                Filter filter =
                    Filter.getFilterFromEncodedString(filterString);
                setPageSessionAttribute(FILTER_VALUE, filter);
            } catch (UnsupportedEncodingException uee) {
                TraceUtil.trace1("unabled to reconstitute filter", uee);
            }
        } else {
            setPageSessionAttribute(FILTER_VALUE, null);
        }

        // refresh the page
        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private String removeTrailingSlashes(String currentDirectory) {
        if ("/".equals(currentDirectory) || !currentDirectory.endsWith("/")) {
            return currentDirectory;
        } else {
            return removeTrailingSlashes(
                currentDirectory.substring(0, currentDirectory.length() - 1));
        }
    }

    /**
     * Handle request when user uses Search Mount Point Drop Down
     * @param RequestInvocationEvent event
     */
    public void handleBasePathMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String MEString = getDisplayFieldStringValue(ENTRIES);
        String directory = getDisplayFieldStringValue(DIR);
        String serverName = getServerName();

        setCurrentDirectory(directory);

        if (MEString == null) {
            setDisplayFieldValue(
                ENTRIES, Integer.toString(getMaxEntries()));
        }

        try {
            int entries = Integer.parseInt(MEString);

            this.maxEntries = entries;
            this.basePathMenuClicked = true;
        } catch (NumberFormatException nfe) {
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                SamUtil.getResourceString("fs.stage.error"),
                -3333,
                SamUtil.getResourceString("fs.stage.entries.nan", MEString),
                serverName);
        }

        // Change mode back to LIVE
        setMode(LIVE_DATA);

        // manually set mode radio button value back to "live"
        // Javascript is relying on this value to determine which operations
        // needs to be disabled
        ((CCRadioButton) getChild(MODE_RADIO)).setValue("live");

        // reset table state data so the row number, page number all are reset
        FileBrowserView view = (FileBrowserView) getChild(CONTAINER_VIEW);
        view.resetTable();

        // Reset filter if it is set.  User wants to see the content of the
        // directory that matches the filter.  It makes no sense to carry over
        // the filter criteria to the next level.
        removePageSessionAttribute(FILTER_VALUE);

        // refresh page
        forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request when user uses Search Mount Point Drop Down
     * @param RequestInvocationEvent event
     */
    public void handleRecoveryMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String date = (String) getDisplayFieldValue(RECOVERY_MENU);
        setMode(extract(date, true));

        // set IS_TOGGLE to true (live radio button is clicked!)
        setPageSessionAttribute(IS_TOGGLE, Boolean.toString(true));
        setPageSessionAttribute(
            LAST_USED_RECOVERY_PT_DATE, extract(date, false));

        // refresh page
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request when user clicks the Live Data Radio Button
     * @param RequestInvocationEvent event
     */
    public void handleLiveDataHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        setMode(LIVE_DATA);

        // set IS_TOGGLE to true (live radio button is clicked!)
        setPageSessionAttribute(IS_TOGGLE, Boolean.toString(true));

        // refresh page
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request when user clicks yes when prompted if they want to
     * mount the file system
     * @param RequestInvocationEvent event
     */
    public void handleMountHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        try {
            // fs information is used in both populateBasePathMenu &
            // checkCurrentDirectory
            SamQFSSystemFSManager fsManager =
                SamUtil.getModel(getServerName()).getSamQFSSystemFSManager();
            FileSystem myFS = fsManager.getFileSystem(getFSName());
            if (myFS == null) {
                throw new SamFSException(null, -1099);
            }
            myFS.mount();

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to mount file system!");
            TraceUtil.trace1("Message: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay()",
                "Failed to mount file system!",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSSummary.error.mount",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        // refresh page
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Return a String that holds all indexed recovery points path
     * @param RestoreDumpFile[]
     * @return String dumpPath_date@@@dumpPath_name both delimited by ###
     */
    private String getIndexedDumpPathInfo(String [] dumpPaths)
        throws SamFSException {
        StringBuffer keyBuf   = new StringBuffer();
        StringBuffer valueBuf = new StringBuffer();
        DateFormat dfmt = DateFormat.getDateTimeInstance(
                                      DateFormat.SHORT,
                                      DateFormat.SHORT);
        for (int i = dumpPaths.length - 1; i >= 0; i--) {
            if (keyBuf.length() != 0) {
                keyBuf.append("###");
                valueBuf.append("###");
            }

            TraceUtil.trace3(dumpPaths[i]);
            Properties properties =
                ConversionUtil.strToProps(dumpPaths[i]);

            String fileName = properties.getProperty("name");
            long modTime = -1;
            try {
                modTime = Long.parseLong(properties.getProperty("date"));
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1(
                    "NumberFormatException while calling getIndexedSnaps!");
                // skip this entry
                continue;
            }
            keyBuf.append(dfmt.format(new Date(modTime * 1000)));
            valueBuf.append(
                fileName.concat("!!!").concat(
                dfmt.format(new Date(modTime * 1000))));
        }

        if (keyBuf.length() == 0) {
            return "";
        } else {
            return keyBuf.toString().concat("@@@").concat(valueBuf.toString());
        }
    }

    /**
     * Extract snapPath or time string from menu value
     */
    private String extract(String menuValue, boolean snapPath) {
        String [] myArray = menuValue.split("!!!");
        if (myArray != null && myArray.length == 2) {
            if (snapPath) {
                return myArray[0];
            } else {
                return myArray[1];
            }
        }
        TraceUtil.trace1("DEVELOPER BUG found in extract() routine!");
        return "";
    }

    /**
     * Helper to find the menu value from the snap date
     */
    private String findValue(String mode, String [] dumpFile) {
        for (int i = 0; i < dumpFile.length; i++) {
            if (dumpFile[i].startsWith(mode)) {
                return dumpFile[i];
            }
        }
        return "";
    }

    /**
     * Helper to extract current file system name.
     * Note: Check unmountedFSName first, return this if it is not null.
     * We save this value in isArchiving when a sam/q file system is not
     * mounted. This getFSName() may be called from handleMountRequest!
     */
    private String getFSName() {
        String unmountedFSName =
            (String) getPageSessionAttribute(UNMOUNTED_FS_NAME);
        if (unmountedFSName != null) {
            return unmountedFSName;
        }

        String fsInfo = (String) getDisplayFieldValue(FS_INFO);
        String [] fsInfoArray = fsInfo.split("###");
        if (fsInfoArray != null && fsInfoArray.length == 2) {
            return fsInfoArray[0];
        }
        return "";
    }

    /**
     * Helper class to sort the File system mount points in length to determine
     * what file system user is currently browsing.
     */

    private class FSMountPoints implements Comparable, Comparator {

        String fsName = null, mountPoint = null;

        public FSMountPoints(String fsName, String mountPoint) {
            this.fsName = fsName;
            this.mountPoint = mountPoint;
        }

        public String getFSName() {
            return this.fsName;
        }

        public String getMountPoint() {
            return this.mountPoint;
        }

        // implement the Comparable interface
        public int compareTo(Object o) {
            return compareTo((FSMountPoints)o);
        }

        // implement the Comparator interface
        /**
         * Compares this capacity object to the passed in capacity object.
         *
         * @return If "this" mount point length is less than the passed in
         * mount point, otherwise return -1.  If "this" mount point length is
         * greater than the passed in mount point, then return 1.
         * If they are the same, return 0;
         */
        public int compareTo(FSMountPoints p) {
            int this_length = this.mountPoint.length();
            int p_length    = p.getMountPoint().length();

            if (this_length < p_length) {
                return -1;
            } else if (this_length > p_length) {
                return 1;
            } else {
                return 0;
            }
        }

        // implement the comparator interface so that action table sorting
        // will work
        /**
         * Same as compare(FSMountPoints p1, FSMountPoints p2);
         */
        public int compare(Object o1, Object o2) {
            return compare((FSMountPoints)o1, (FSMountPoints)o2);
        }

        /**
         * Same as calling p1.compareTo(c2);
         */
        public int compare(FSMountPoints p1, FSMountPoints p2) {
            return p1.compareTo(p2);
        }
    }

    /**
     * checkRolePriviledge() checks if the user is a valid Admin
     * disable all action buttons / dropdown if not
     */
    private void checkRolePrivilege() {
        TraceUtil.trace3("Entering");

        if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILE_OPERATOR)) {
            ((CCHiddenField) getChild(ROLE)).setValue("FILE");
        }

        TraceUtil.trace3("Exiting");
    }
}
