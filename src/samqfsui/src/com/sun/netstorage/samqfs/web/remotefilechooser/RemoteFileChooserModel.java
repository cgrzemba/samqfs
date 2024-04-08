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

// ident	$Id: RemoteFileChooserModel.java,v 1.18 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.remotefilechooser;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.RemoteFile;
import com.sun.netstorage.samqfs.web.util.Filter;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCFileChooserModel;
import java.io.File;
import java.io.FileFilter;
import java.util.Properties;

/**
 * This is the Model Class for choosing files/folders remotely
 */

public class RemoteFileChooserModel extends CCFileChooserModel {

    // number of files to display for each page
    protected int pageSize = 50;

    // max number of files to display for each page
    protected int maxPageSize = 100;

    // max number of entries GUI can handle
    protected int max_entries = 1000;

    // current page #
    protected int currentPage = 0;

    // total pages
    protected int totalPages = 0;

    // total items
    protected int totalItems = 0;

    // file filter string
    protected String filterString = null;

    // parent page's text_field name
    protected String fieldName = null;

    // parent page's optional refresh cmd
    protected String parentRefreshCmd = null;

    protected String onCloseScript = null;

    // directory last accessed
    protected String cachedDir = null;

    // cached filter string
    protected String cachedFilterString = "*";

    // cached sort field
    protected String cachedSortField = null;

    // cached files
    protected File[] cachedFiles = null;

    // allow choosing file and folder
    public static String FILE_AND_FOLDER_CHOOSER = "both";
    // override type in CCFileChooserModel to allow choosing of file and folder
    protected String chooserType = null;

    protected SamQFSSystemFSManager fsManager;

    // Constructor
    public RemoteFileChooserModel(String serverName) {
        this(serverName, 0);
    }

    // Constructor
    public RemoteFileChooserModel(String serverName, int pageSize) {

        super();
        setServerName(serverName);

        if (pageSize > 0 && pageSize < maxPageSize) {
            this.pageSize = pageSize;
        }
        TraceUtil.initTrace();
    }

    public SamQFSSystemFSManager getFSManager() throws SamFSException {
        if (fsManager == null) {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            fsManager = sysModel.getSamQFSSystemFSManager();
            TraceUtil.trace3("Got handle to fsManager for " + getServerName());
        }
        return fsManager;
    }

    public void clearCachedDir() {
        this.cachedDir = null;
    }

    public int getMaxEntries() {
        return max_entries;
    }

    /**
     * For remote browsing, need to overwrite the following methods
     */

    /**
     * Check if the user can access the given file or folder
     * @param directory the specified directory
     */
    public boolean canRead(String directory) {
        TraceUtil.trace3("checking if we can read file/dir: " + directory);
        return directory.startsWith(getHomeDirectory());
    }

    /**
     * Set the currently selected filter String.
     * @param the currently selected file filter string
     */
    public void setFilterString(String filterString) {
        TraceUtil.trace3("setting filter string to " + filterString);
        this.filterString = filterString;
    }

    /**
     * Get the currently selected filter String.
     * @return the currently selected file filter string
     */
    public String getFilterString() {
        TraceUtil.trace3("returning filter string " + filterString);
        return filterString;
    }

    /**
     * Instantiate the filter associated with the model implementation.
     * The model implementation should implement its own filter that
     * implements the java.io.FileFilter interface. In order to instantiate
     * the filter the filterString should be passed as an argument.
     * @param filterString the user entered file filter string
     * @return an instance of the FileFilter implementation
     */
    public FileFilter instantiateFilter(String filterString) {
        TraceUtil.trace3("returning filter with filter string " + filterString);
        this.filterString = filterString;
        return super.instantiateFilter(filterString);
    }

    /**
     * Get the currently selected filter. This models filter
     * should have implemented the java.io.FileFilter interface.
     * The only filter method available to the tag is the accept()
     * method.
     * @return the currently selected file filter
     */
    public FileFilter getFileFilter() {
        TraceUtil.trace3("getFileFilter() being called ");
        return super.getFileFilter();
    }

    /**
     * Get the 1st page of files from the directory in the file system
     * The files and dirs are returned as a sorted array based on
     * the sort field chosen by the user. By default they are sorted
     * in alphabetic order.
     *
     * @param directory the specified directory
     * @return all files and sub directories in the directory
     */
    public File[] getFiles(String directory) {
        TraceUtil.trace3("getting files from dir " + directory);

        // if no directory is specified assume the home directory
        if (directory == null) {
            directory = getHomeDirectory();
        }

        String filterStr = getFilterString();
        if (filterStr == null) {
            filterStr = "*";
        }

        if (cachedDir == null || !cachedDir.equals(directory) ||
            !cachedFilterString.equals(filterStr)) {

            Properties props = new Properties();
            props.setProperty(Filter.KEY_FILENAME, filterStr);

            try {
                Filter filter = new Filter(Filter.VERSION_LATEST, props);
                cachedFiles =
                    getDirEntries(max_entries, directory, filter);

                totalItems = cachedFiles.length;
                totalPages = (totalItems + pageSize - 1) / pageSize;
                currentPage = 0;
                cachedDir = directory;
                cachedFilterString = filterStr;
                // reset the cached sort field to force resort
                cachedSortField = null;
                TraceUtil.trace3("Got " + totalItems + " files!");
            } catch (Exception ex) {
                cachedDir = null;
                TraceUtil.trace1("Couldnot get remote files in getFiles()\n"
                    + ex);
                return null;
            }
        }

        File[] files =
            getFiles(currentPage * pageSize, pageSize, getSortField());
        return files;
    }

    /**
     * Get a page of files from the directory in the file system
     * The files and dirs are returned as a sorted array based on
     * the sort field chosen by the user. By default they are sorted
     * in alphabetic order.
     *
     * @param directory the specified directory
     * @return all files and sub directories in the directory
     */
    public File[] getFiles(int start, int size, String sortField) {

        TraceUtil.trace3("getting files start = " + start + ", size = " + size +
            ", sortField = " + sortField);

        int total = cachedFiles.length;
        int num = size;

        if (cachedSortField == null || !cachedSortField.equals(sortField)) {
            sort(cachedFiles);
            cachedSortField = sortField;
        }

        if (total < start + size) {
            num = total - start;
        }

        File[] tmpFiles = new File[num];
        for (int i = 0; i < num; i++) {
            tmpFiles[i] = cachedFiles[start + i];
        }
        return tmpFiles;
    }

    public RemoteFile[] getDirEntries(int maxEntries, String dirPath,
                                      Filter filter) throws SamFSException {
        return getFSManager().getDirEntries(maxEntries, dirPath, filter);
    }

    public boolean isFile(String absolutePath) throws SamFSException {
        return isFileDirectory(true, absolutePath);
    }

    public boolean isDirectory(String absolutePath) throws SamFSException {
        return isFileDirectory(false, absolutePath);
    }

    private boolean isFileDirectory(boolean checkFile, String absolutePath)
        throws SamFSException {

        // Need to get a Remote file object for the path.
        // Use the already build getFiles methods to take advantage of caching.
        // Use a special filter that only looks for a particular file
        String savedFilter = getFilterString();
        File f = new File(absolutePath);
        setFilterString(f.getName());
        File[] files = getFiles(f.getParent());
        setFilterString(savedFilter);

        // Should only have 1 file returned
        if (files == null || files.length == 0) {
            // Something wierd.  Return a negative response
            TraceUtil.trace1("Could not get info about remote directory/file " +
                             absolutePath);
            throw new SamFSException(SamUtil.getResourceString(
                                                "filechooser.error.badPath"));
        }
        if (checkFile) {
            return files[0].isFile();
        } else {
            return files[0].isDirectory();
        }
    }

    private String checkForSimulatedMountPoint(String dirPath)
    throws SamFSException {
        boolean bFound = false;
        FileSystem[] fileSystems = getFSManager().getAllFileSystems();
        String mountPoint = null;
        if (fileSystems != null) {
            for (int i = 0; i < fileSystems.length; i++) {
                mountPoint = fileSystems[i].getMountPoint();
                if (dirPath.startsWith(mountPoint)) {
                    bFound = true;
                    break;
                }
            }
        }
        return bFound ? mountPoint : null;
    }

    // Returns path without mount point or root path
    private String stripSimulatedMountPoint(String path, String mountPoint) {
        // Strip fake mount point
        if (path.length() > mountPoint.length()) {
            path = path.substring(mountPoint.length());
        } else {
            path = "/";
        }
        return path;
    }

    public int getPageSize() {
        return pageSize;
    }

    public int getCurrentPage() {
        return currentPage;
    }

    public void setCurrentPage(int page) {
        if (page >= 0 && page <= totalPages) {
            currentPage = page;
        }
    }

    public int getTotalPages() {
        return totalPages;
    }

    public int getTotalItems() {
        return totalItems;
    }

    public void setTotalItems(int num) {
        totalItems = num;
    }

    public int getFirstRow() {
        return (currentPage * pageSize + 1);
    }

    public int getLastRow() {
        return ((currentPage + 1) * pageSize);
    }

    public String getFieldName() {
        return fieldName;
    }

    public void setFieldName(String fieldName) {
        this.fieldName = fieldName;
    }

    public String getParentRefreshCmd() {
        return parentRefreshCmd;
    }

    public void setParentRefreshCmd(String cmd) {
        this.parentRefreshCmd = cmd;
    }

    public String getOnClose() {
        return this.onCloseScript;
    }

    public void setOnClose(String script) {
        this.onCloseScript = script;
    }

    public void addSelectedFile(String file) {
        TraceUtil.trace3("adding file: " + file);
        super.addSelectedFile(file);
    }

    /**
     * Get the type of the file chooser
     *
     * @return the type of the file chooser, will be FILE_CHOOSER
     * or FOLDER_CHOOSER or FILE_AND_FOLDER_CHOOSER
     */
    public String getType() {
        return chooserType;
    }

    /**
     * Set the type of the file chooser
     *
     * @param type the specified file chooser type
     * (FILE_CHOOSER / FOLDER_CHOOSER / FILE_AND_FOLDER_CHOOSER)
     */
    public void setType(String type) {
        TraceUtil.trace3("Setting type to " + type);
        if (!type.equals(FILE_CHOOSER) &&
            !type.equals(FOLDER_CHOOSER) &&
            !type.equals(FILE_AND_FOLDER_CHOOSER)) {
            this.chooserType = FILE_CHOOSER;
        } else {
            this.chooserType = type;
        }
        TraceUtil.trace3("Set type to " + this.chooserType);
    }

    /**
     * Returns a string representation of this RemoteFileChooserModel.
     * @return the string representation of this RemoteFileChooserModel
     */
    public String toString() {

        StringBuffer buffer = new StringBuffer();

        if (chooserType == FILE_CHOOSER)
            buffer.append("Selected File(s): \n");
        else if (chooserType == FILE_AND_FOLDER_CHOOSER)
            buffer.append("Selected File/Folder(s): \n");
        else
            buffer.append("Selected Folder(s): \n");

        String dir =  getCurrentDirectory();
        String [] fileList = getSelectedFiles();

        for (int i = 0; i < fileList.length; i++) {
            buffer.append(dir)
            .append(File.separator)
            .append((String) fileList[i])
            .append("\n");
        }
        return buffer.toString();
    }
}
