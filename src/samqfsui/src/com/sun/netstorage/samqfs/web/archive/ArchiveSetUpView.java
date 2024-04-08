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

// ident	$Id: ArchiveSetUpView.java,v 1.15 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCHiddenField;
import java.util.Map;

public class ArchiveSetUpView extends MultiTableViewBase {
    public static String MEDIA_PARAMETERS_TABLE = "MediaParametersTable";
    public static String DRIVE_LIMITS_TABLE = "DriveLimitsTable";
    public static String MEDIA_PARAMETERS_TILED_VIEW =
        "MediaParametersTiledView";
    public static String DRIVE_LIMITS_TILED_VIEW = "DriveLimitsTiledView";

    public ArchiveSetUpView(View parent, Map models, String name) {
        super(parent, models, name);

        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(MEDIA_PARAMETERS_TILED_VIEW,
            MediaParametersTiledView.class);
        registerChild(DRIVE_LIMITS_TILED_VIEW, DriveLimitsTiledView.class);
        super.registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3("Creating child " + name);
        if (name.equals(MEDIA_PARAMETERS_TILED_VIEW)) {
            return new MediaParametersTiledView(
                this, getTableModel(MEDIA_PARAMETERS_TABLE),  name);

        } else if (name.equals(DRIVE_LIMITS_TILED_VIEW)) {
            return new DriveLimitsTiledView(
                this, getTableModel(DRIVE_LIMITS_TABLE),  name);

        } else if (name.equals(MEDIA_PARAMETERS_TABLE)) {
            return createTable(name, MEDIA_PARAMETERS_TILED_VIEW);

        } else if (name.equals(DRIVE_LIMITS_TABLE)) {
            return createTable(name, DRIVE_LIMITS_TILED_VIEW);

        } else {
            CCActionTableModel m = super.isChildSupported(name);
            if (m != null)
                return super.isChildSupported(name).createChild(this, name);
        }
        // if we get here, the named child has no known parent
        throw new IllegalArgumentException("invalid child '" + name + "'");
    }


    /**
     * set action labels and column headers for General policies table
     */
    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");

        CCActionTableModel model = getTableModel(MEDIA_PARAMETERS_TABLE);

        // init general table column headers
        model.setActionValue("MediaTypeCol",
            "ArchiveSetup.mediaParameters.mediaType");
        model.setActionValue("MaxArchiveFileSizeCol",
            "ArchiveSetup.mediaParameters.maxArchiveFileSize");
        model.setActionValue("MinSizeForOverflowCol",
            "ArchiveSetup.mediaParameters.minSizeForOverflow");
        model.setActionValue("ArchiveBufferSizeCol",
            "ArchiveSetup.mediaParameters.archiveBufferSize");
        model.setActionValue("ArchiveBufferLockCol",
            "ArchiveSetup.mediaParameters.archiveBufferLock");
        model.setActionValue("StageBufferSizeCol",
            "ArchiveSetup.mediaParameters.stageBufferSize");
        model.setActionValue("StageBufferLockCol",
            "ArchiveSetup.mediaParameters.stageBufferLock");
        model.setTitle("ArchiveSetup.mediaParameters.tabletitle");

        model = getTableModel(DRIVE_LIMITS_TABLE);
        model.setActionValue("LibraryNameCol",
            "ArchiveSetup.driveLimits.libraryName");
        model.setActionValue("MaxDrivesForArchiveCol",
            "ArchiveSetup.driveLimits.maxDrivesForArchive");
        model.setActionValue("MaxDrivesForStageCol",
            "ArchiveSetup.driveLimits.maxDrivesForStage");
        model.setTitle("ArchiveSetup.driveLimits.tabletitle");

        TraceUtil.trace3("Exiting");
    }

    /**
     * populate the table rows - both general and other policies tables
     *
     */
    public void populateTableModels(GlobalArchiveDirective globalDir) {
        TraceUtil.trace3("Entering");
        String serverName =
            ((CommonViewBeanBase)getParentViewBean()).getServerName();

        SamFSException exception = null;
        boolean hasException = false;

        try {
            if (globalDir != null) {
                BufferDirective [] bufferDirectives = globalDir.getBufferSize();
                BufferDirective [] archMax = globalDir.getMaxFileSize();
                BufferDirective [] ovflmin =
                    globalDir.getMinFileSizeForOverflow();
                SamUtil.doPrint(new NonSyncStringBuffer().append(
                    "archive buffer directives length = ").append(
                    bufferDirectives.length).append(
                    "arch max directive length = ").append(
                    archMax.length).append(
                    "overflow length = ").append(ovflmin.length).toString());

                SamUtil.doPrint(new NonSyncStringBuffer().append(
                    "media parameters table length is ").append(
                    bufferDirectives.length).toString());

                // populate the media parameters table
                CCActionTableModel model =
                    (CCActionTableModel)getTableModel(MEDIA_PARAMETERS_TABLE);
                model.clear();
                for (int i = 0; i < bufferDirectives.length; i++) {
                    if (i > 0) {
                        model.appendRow();
                    }

                    // Media Type
                    int type = bufferDirectives[i].getMediaType();
                    String media = SamUtil.getMediaTypeString(type);
                    media = (media != null) ? media.trim() : "";

                    SamUtil.doPrint(new NonSyncStringBuffer().append(
                        "initModelRows(): media is ").append(media).toString());
                    model.setValue("MediaType", media);
                    model.setValue("MediaTypeHiddenField", media);

                    // append the media types to a hidden field to be
                    // retrieved later
                    CCHiddenField hiddenMediaType = (CCHiddenField)
                        getParentViewBean().getChild(
                        ArchiveSetUpViewBean.CHILD_MEDIA_TYPES);

                    StringBuffer buf = new StringBuffer(); // store media types
                    String exist = (String)hiddenMediaType.getValue();
                    if (exist != null && !exist.trim().equals("")) {
                        buf.append(exist).append("###").append(media);
                    } else {
                        buf.append(media);
                    }
                    hiddenMediaType.setValue(buf.toString());

                    // Maximum Archive File Size from tiledview
                    // Minimum size for overflow from tiledview
                    // Archiving buffer size and lock from tiledview
                    // Staging buffer size and stage buffer lock from tiledview

                }

                // The number of rows is not available in the submit cycle,
                // save this in the display cycle and retrieve this
                // value in the submit cycle
                CCHiddenField hiddenMediaLen = (CCHiddenField)
                    getParentViewBean().getChild(
                        ArchiveSetUpViewBean.CHILD_MEDIA_SIZE);
                hiddenMediaLen.setValue(String.valueOf(model.getNumRows()));


                DriveDirective[] driveDirectives =
                    globalDir.getDriveDirectives();

                model = (CCActionTableModel)getTableModel(DRIVE_LIMITS_TABLE);
                model.clear();
                for (int i = 0; i < driveDirectives.length; i++) {
                    if (i > 0) model.appendRow();
                    String name = driveDirectives[i].getLibraryName();

                    Library lib = SamUtil.getModel(serverName).
                        getSamQFSSystemMediaManager().
                        getLibraryByName(name);

                    Drive[] drives = lib.getDrives();

                    model.setValue("LibraryName", name);
                    String driveCount = Integer.toString(drives.length);
                    model.setValue("LibraryHiddenField", name);
                    model.setValue("MaxDrives", driveCount);

                    model.setValue("MaxDrivesErrMsg", SamUtil.getResourceString(
                        "ArchiveSetup.error.driverange",
                        new String[] {driveCount}));


                }
                CCHiddenField field = (CCHiddenField)
                    getParentViewBean().getChild(
                        ArchiveSetUpViewBean.CHILD_LIB_SIZE);
                field.setValue(String.valueOf(model.getNumRows()));
            }

        } catch (SamFSException sfe) {
            exception = sfe;
            hasException = true;
            SamUtil.processException(exception,
                    this.getClass(),
                    "populateTableModels",
                    "Unable to populate tables",
                    serverName);

            SamUtil.setErrorAlert(getParentViewBean(),
                                  ArchiveSetUpViewBean.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {

        TraceUtil.trace3("Entering");

        initializeTableHeaders();
        // the selectionType is always none, so we need not checkRolePrivileges

        TraceUtil.trace3("Exiting");
    }


    public boolean saveArchiveSizeMediaParameters(
        String type, BufferDirective [] buffers)
		throws SamFSException {

        boolean cfg_changed = false;

        TraceUtil.trace3("Entering");

        CCActionTableModel model =
            (CCActionTableModel)getTableModel(MEDIA_PARAMETERS_TABLE);

        int size = buffers.length;
        // Get number of rows from the hiddenField CHILD_MEDIA_SIZE
        // it would not be available from TableModel
        int tableSize = Integer.parseInt(
            (String)((CCHiddenField) getParentViewBean().getChild(
                ArchiveSetUpViewBean.CHILD_MEDIA_SIZE)).getValue());

        TraceUtil.trace3("tableSize = " + tableSize + "size = " + size);
        for (int i = 0; i < tableSize && size > 0; i++) {
            model.setRowIndex(i);
            long fileSize = -1;
            // hidden field for array of media types in table

            String mediaTypeHidden =
                (String) ((CCHiddenField) getParentViewBean().getChild(
                    ArchiveSetUpViewBean.CHILD_MEDIA_TYPES)).getValue();

            String[] typeArray = mediaTypeHidden.split("###");
            String mediaType = (i < typeArray.length) ? typeArray[i] : "";
            // archive size (archmax or overflow_min depending upon type)
            mediaType = (mediaType != null) ? mediaType.trim() : "";

            TraceUtil.trace3(new NonSyncStringBuffer().append(
                "Media Type (from hidden field) is ").append(
                mediaType).toString());

            String sizeStr = null;
            String sizeUnitStr = null;
            int sizeUnit = -1;

            if (type.equals("MaxArchiveFileSize")) {
                sizeStr = (String) model.getValue("MaxArchiveFileSize");
                sizeUnitStr = (String) model.getValue("MaxArchiveFileSizeUnit");

            } else if (type.equals("MinSizeForOverflow")) {
                sizeStr = (String) model.getValue("MinSizeForOverflow");
                sizeUnitStr = (String) model.getValue("MinSizeForOverflowUnit");

            }
            sizeStr = (sizeStr != null) ? sizeStr.trim() : "";
            sizeUnitStr = (sizeUnitStr != null) ? sizeUnitStr.trim() : "";
            TraceUtil.trace3("sizeStr = " + sizeStr);
            TraceUtil.trace3("sizeUnitStr = " + sizeUnitStr);
            if (sizeUnitStr != "" &&
                !sizeUnitStr.equals(SelectableGroupHelper.NOVAL)) {
                    sizeUnit = Integer.parseInt(sizeUnitStr);
            } else {
                    sizeUnit = 0; // bytes
            }
            try {
                fileSize = (sizeStr.equals("")) ?
                    -1 :
                    Long.parseLong(sizeStr);

            } catch (NumberFormatException nfex) {
                fileSize = -1;
            }
            TraceUtil.trace3("file size = " + fileSize + " unit = " + sizeUnit);
            BufferDirective buffer = null;
            for (int j = 0; j < size; j++) {
                int mtype = buffers[j].getMediaType();
                String mString = SamUtil.getMediaTypeString(mtype);
                TraceUtil.trace3("mString = " + mString);
                if (mString.equals(mediaType)) {
                    buffer = buffers[j];
                    TraceUtil.trace3("Found a match");
                }
            }
            if ((buffer != null) && (fileSize != buffer.getSize())) {
                // convert the fileSize to size in bytes
                TraceUtil.trace3("Setting file size in bytes");
                long sizeBytes = -1; // if user input is empty, reset
                if (fileSize != -1) {
                    sizeBytes = SamQFSUtil.getSizeInBytes(fileSize, sizeUnit);
                }
                TraceUtil.trace3("File size in bytes = " + sizeBytes);

                buffer.setSize(sizeBytes);
                cfg_changed = true;
            }
        }
        TraceUtil.trace3("Exiting");
        return (cfg_changed);
    }


    public boolean saveBufferMediaParameters(
        String type, BufferDirective [] buffers)
		throws SamFSException {

        boolean cfg_changed = false;

        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Type = " + type);

        CCActionTableModel model =
            (CCActionTableModel)getTableModel(MEDIA_PARAMETERS_TABLE);

        int size = buffers.length;

        // Get number of rows from the hiddenField CHILD_MEDIA_SIZE
        // it would not be available from TableModel
        int tableSize = Integer.parseInt(
            (String)((CCHiddenField) getParentViewBean().getChild(
                ArchiveSetUpViewBean.CHILD_MEDIA_SIZE)).getValue());

        TraceUtil.trace3("tableSize = " + tableSize + "size = " + size);
        for (int i = 0; i < tableSize && size > 0; i++) {
            model.setRowIndex(i);
            int bufferSize = -1;
            String bufferSizeStr = null;
            String lock = null;

            // hidden field for media type
            String mediaTypeHidden =
                (String) ((CCHiddenField) getParentViewBean().getChild(
                    ArchiveSetUpViewBean.CHILD_MEDIA_TYPES)).getValue();

            String[] typeArray = mediaTypeHidden.split("###");
            String mediaType = (i < typeArray.length) ? typeArray[i] : "";
            // archive size (archmax or overflow_min depending upon type)
            mediaType = (mediaType != null) ? mediaType.trim() : "";

            TraceUtil.trace3(new NonSyncStringBuffer().append(
                "Media Type (from hidden field) is ").append(
                mediaType).toString());

            if (type.equals("archive")) {
                // archive buffer size
                bufferSizeStr = (String) model.getValue("ArchiveBufferSize");
                // archive buffer lock
                lock = (String) model.getValue("ArchiveBufferLock");

            } else if (type.equals("stage")) {
                // stage buffer size
                bufferSizeStr = (String) model.getValue("StageBufferSize");
                // stage lock
                lock = (String) model.getValue("StageBufferLock");
            }

            bufferSizeStr = (bufferSizeStr != null) ? bufferSizeStr.trim() : "";
            lock = (lock != null) ? lock.trim() : "";

            try {
                bufferSize = (bufferSizeStr.equals("")) ?
                    -1 :
                    Integer.parseInt(bufferSizeStr);
            } catch (NumberFormatException nfex) {
                bufferSize = -1;
            }

            // if bufferSize == -1, it will be reset to default after calling
            // buffer.setSize(-1)
            TraceUtil.trace3("Buffer size = " + bufferSize);

            BufferDirective buffer = null;
            for (int j = 0; j < size; j++) {
                int mtype = buffers[j].getMediaType();
                String mString = SamUtil.getMediaTypeString(mtype);
                if (mString.equals(mediaType))
                    buffer = buffers[j];
            }
            if (buffer != null) {
                if (bufferSize != buffer.getSize()) {
                    buffer.setSize(bufferSize);
                    cfg_changed = true;
                }
                if (lock != null && lock.equals("true")) {
                    if (!buffer.isLocked()) {
                        buffer.setLocked(true);
                        cfg_changed = true;
                    }
                } else if (lock != null && lock.equals("false")) {
                    if (buffer.isLocked()) {
                        buffer.setLocked(false);
                        cfg_changed = true;
                    }
                }

            }
        }
        TraceUtil.trace3("Exiting");
        return (cfg_changed);
    }


    public boolean saveDriveLimits(String type, DriveDirective[] drives)
        throws SamFSException {

        boolean cfg_changed = false;

        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Type = " + type);

        int size = drives.length;

        CCActionTableModel model =
            (CCActionTableModel) getTableModel(DRIVE_LIMITS_TABLE);

        // Get number of rows from the hiddenField CHILD_LIB_SIZE
        // it would not be available from TableModel
        int tableSize = Integer.parseInt(
            (String)((CCHiddenField) getParentViewBean().getChild(
            ArchiveSetUpViewBean.CHILD_LIB_SIZE)).getValue());

        for (int i = 0; i < tableSize && size > 0; i++) {

            model.setRowIndex(i);
            int maxDrives = -1;
            String maxDrivesStr = null;

            if (type.equals("archive")) {
                maxDrivesStr = (String) model.getValue("MaxDrivesForArchive");
            } else if (type.equals("stage")) {
                maxDrivesStr = (String) model.getValue("MaxDrivesForStage");
            }
            maxDrivesStr = (maxDrivesStr != null) ? maxDrivesStr.trim() : "";
            if (!maxDrivesStr.equals("")) {
                try {
                    maxDrives = Integer.parseInt(maxDrivesStr.trim());
                } catch (NumberFormatException nfex) {
                    maxDrives = -1;
                }
            }
            TraceUtil.trace3("Max drives for " + type + " : " + maxDrives);
            String library = (String) model.getValue("LibraryHiddenField");
            library = (library != null) ? library.trim() : "";
            TraceUtil.trace3("Library = " + library);
            DriveDirective drive = null;
            for (int j = 0; j < size; j++) {
                if (library != "" &&
                    library.equals(drives[j].getLibraryName())) {
                    drive = drives[j];
                }
            }

            if ((drive != null) && (maxDrives != drive.getCount())) {
                TraceUtil.trace3("Setting drive count to " + maxDrives);
                drive.setCount(maxDrives);
                cfg_changed = true;
            }
        }

        TraceUtil.trace3("Exiting");
        return (cfg_changed);
    }
}
