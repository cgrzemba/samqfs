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

// ident	$Id: FileDetailsPopupViewBean.java,v 1.26 2008/11/05 20:24:49 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileCopyDetails;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TimeConvertor;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.File;
import java.io.IOException;
import javax.servlet.ServletException;

public class FileDetailsPopupViewBean extends CommonSecondaryViewBeanBase {

    // page name & default url
    public static final String PAGE_NAME   = "FileDetailsPopup";
    public static final String DEFAULT_URL = "/jsp/fs/FileDetailsPopup.jsp";
    public static final String CHILD_ACTION_TABLE = "ActionTable";

    public static final String FILE_TO_VIEW = "filetoview";
    public static final String IS_ARCHIVING = "isarchiving";
    public static final String FS_NAME = "fsname";
    public static final String MOUNT_POINT = "mountpoint";
    public static final String RECOVERY_POINT_PATH = "snappath";
    public static final String IS_DIR = "isdir";

    // Hidden field for javascript to determine if it needs to bring the focus
    // to the bottom of the page
    public static final String PAGE_MODE = "PageMode";

    // Change HREF value for archive/release/stage
    public static final String CHANGE_HREF  = "ChangeHref";

    // Change File Attributes View (Pagelet)
    public static final String CHANGE_VIEW = "ChangeFileAttributesView";

    private CCPageTitleModel pageTitleModel = null;
    private CCActionTableModel tableModel   = null;

    private StringBuffer lineBreak = new StringBuffer("<br>");
    private StringBuffer tabSpace  = new StringBuffer(getSpace(8));

    public FileDetailsPopupViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        tableModel = createTableModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_ACTION_TABLE, CCActionTable.class);
        registerChild(CHANGE_HREF, CCHref.class);
        registerChild(PAGE_MODE, CCHiddenField.class);
        tableModel.registerChildren(this);
        pageTitleModel.registerChildren(this);
        registerChild(CHANGE_VIEW, ChangeFileAttributesView.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(CHILD_ACTION_TABLE)) {
            // Action table.
            CCActionTable child = new CCActionTable(this, tableModel, name);
            tableModel.setShowSelectionSortIcon(false);
            return child;
        } else if (name.equals(CHANGE_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(PAGE_MODE)) {
            return new CCHiddenField(this, name, null);
        } else if (tableModel.isChildSupported(name)) {
            // Create child from action table model.
            return tableModel.createChild(this, name);
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        // Pagelet
        } else if (name.equals(CHANGE_VIEW)) {
            return new ChangeFileAttributesView(
                this, name, getServerName(), isDirectory(),
                 ChangeFileAttributesView.PAGE_FILE_DETAIL, -1);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel =
                PageTitleUtil.createModel("/jsp/admin/ShowPopUpPageTitle.xml");
        }
        return pageTitleModel;
    }

    private CCActionTableModel createTableModel() {
	// server table
        if (tableModel == null) {
            tableModel = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/fs/FileDetailsTable.xml");
            tableModel.setActionValue("Key", "");
            tableModel.setActionValue("Value", "");
        }
        return tableModel;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String fsName = getFSName();
        String fileName = null;
        boolean liveMode = getRecoveryPoint().length() == 0;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemFSManager fsManager =
                sysModel.getSamQFSSystemFSManager();
            fileName = getFileNameWithPath();
            StageFile stageFile =
                getSelectedStageFile(fsManager, fsName, fileName);

            // set table title
            tableModel.setTitle(fileName);

            // Set items that are always there

            // Name
            addEntry(
                "fs.filedetails.name",
                fileName);

            // Type
            addEntry(
                "fs.filedetails.type",
                stageFile.isDirectory() ?
                    "fs.filedetails.directory" :
                    "fs.filedetails.file",
                stageFile.getWormState() == StageFile.WORM_DISABLED ?
                    (stageFile.isDirectory() ?
                        Constants.Image.DIR_ICON :
                        Constants.Image.FILE_ICON) :
                    (stageFile.isDirectory() ?
                        Constants.Image.ICON_DIR_WORM :
                        Constants.Image.ICON_WORM));

            // Size
            addEntry(
                "fs.filedetails.size",
                new Capacity(stageFile.length(), SamQFSSystemModel.SIZE_B));


            // Online status
            int online = stageFile.getOnlineStatus();
            String onlineText, onlineImage;
            switch (online) {
                case StageFile.ONLINE:
                    onlineImage = Constants.Image.ICON_ONLINE;
                    onlineText  = SamUtil.getResourceString(
                                    "fs.filebrowser.online");
                    break;
                case StageFile.PARTIAL_ONLINE:
                    onlineImage = Constants.Image.ICON_PARTIAL_ONLINE;
                    onlineText  = SamUtil.getResourceString(
                                    "fs.filebrowser.partialonline");
                    break;
                default:
                    onlineImage = Constants.Image.ICON_BLANK_ONE_PIXEL;
                    onlineText  = SamUtil.getResourceString(
                                    "fs.filebrowser.offline");
                    break;
            }
            addEntry(
                "fs.filedetails.onlinestatus",
                onlineText,
                onlineImage);

            // Created Time
            addEntry(
                "fs.filedetails.created",
                SamUtil.getTimeString(stageFile.getCreatedTime()));

            // Last Modified Time
            addEntry(
                "fs.filedetails.lastmodified",
                SamUtil.getTimeString(stageFile.lastModified()));

            // Accessed Time
            addEntry(
                "fs.filedetails.accessed",
                SamUtil.getTimeString(stageFile.getAccessedTime()));

            // Protection
            addEntry(
                "fs.filedetails.protection",
                stageFile.getCProtection());

            // User
            addEntry(
                "fs.filedetails.user",
                stageFile.getUser());

            // Group
            addEntry(
                "fs.filedetails.group",
                stageFile.getGroup());

            if (isArchiving()) {
                // Segment Count if any
                int segmentCount = stageFile.getSegmentCount();
                if (-1 != segmentCount) {
                    addEntry(
                        "fs.filedetails.segmentcount",
                        Integer.toString(segmentCount));
                    addEntry(
                        "fs.filedetails.segmentonline",
                        Integer.toString(stageFile.getOnlineCount()));
                }

                // Segment Size if any
                long segmentSize = stageFile.getSegmentSize();
                if (-1 != segmentSize) {
                    addEntry(
                        "fs.filedetails.segmentsize",
                        new Capacity(segmentSize, SamQFSSystemModel.SIZE_MB));
                }

                // Segment Stage Ahead if any
                long segmentStageAhead = stageFile.getSegmentStageAhead();
                if (-1 != segmentStageAhead) {
                    addEntry(
                        "fs.filedetails.segmentstageahead",
                        Long.toString(segmentStageAhead));
                }

                // Arch Done if any
                int archDone = stageFile.getArchDone();
                if (-1 != archDone) {
                    addEntry(
                        "fs.filedetails.archdone",
                        archDone > 0 ?
                            (archDone > 1 ?
                                SamUtil.getResourceString("samqfsui.yes").
                                    concat(" (").concat(
                                    Integer.toString(archDone)).
                                    concat(")") :
                                SamUtil.getResourceString("samqfsui.yes")) :
                            SamUtil.getResourceString("samqfsui.no"));
                }

                // Stage Pending if any
                int stagePending = stageFile.getStagePending();
                if (-1 != stagePending) {
                    addEntry(
                        "fs.filedetails.stagepending",
                        stagePending > 0 ?
                            (stagePending > 1 ?
                                SamUtil.getResourceString("samqfsui.yes").
                                    concat(" (").concat(
                                    Integer.toString(stagePending)).
                                    concat(")") :
                                SamUtil.getResourceString("samqfsui.yes")) :
                            SamUtil.getResourceString("samqfsui.no"));
                }

                // Partial Release Size if any
                long partialRelease = stageFile.getPartialReleaseSize();
                if (-1 != partialRelease) {
                    addEntry(
                        "fs.filedetails.partialreleasesize",
                        new Capacity(
                            partialRelease, SamQFSSystemModel.SIZE_KB));
                }

                StringBuffer fileAttBuf = new StringBuffer();

                // Archiver Attribute (show default in live mode if -1)
                int archive = stageFile.getArchiveAttributes();

                String archiveAttString = null;
                if (-1 != archive) {
                    switch (archive) {
                        case FileUtil.AR_ATT_NEVER:
                            archiveAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.archiving.never");
                            fileAttBuf.append(Integer.toString(
                                Archiver.NEVER)).append("###");
                            break;

                        case FileUtil.AR_ATT_CONCURRENT:
                            archiveAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.archiving.concurrent");
                            fileAttBuf.append(Integer.toString(
                                Archiver.CONCURRENT)).append("###");
                            break;
                        case FileUtil.AR_ATT_INCONSISTENT:
                            archiveAttString = SamUtil.getResourceString(
                                "archiving.criteria.archiving.inconsistent");
                            fileAttBuf.append(Integer.toString(
                                Archiver.INCONSISTENT)).append("###");
                            break;

                        default:
                            archiveAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.archiving.defaults");
                            fileAttBuf.append(Integer.toString(
                                Archiver.DEFAULTS)).append("###");
                            break;
                    }
                } else if (liveMode) {
                    // default
                    archiveAttString =
                        SamUtil.getResourceString(
                            "archiving.criteria.archiving.defaults");
                    fileAttBuf.append(Integer.toString(
                        Archiver.DEFAULTS)).append("###");
                }
                if (archiveAttString != null) {
                    addEntry(
                        "fs.filedetails.archiveatt",
                        archiveAttString.concat(
                            getSpace(8)).concat(
                            getChangeHrefTag(SamQFSSystemFSManager.ARCHIVE)));
                }


                // Releaser Attribute (show default in live mode if -1)
                int release = stageFile.getReleaseAttributes();

                String releaseAttString = null;
                if (-1 != release) {
                    switch (release) {
                        case FileUtil.RL_ATT_NEVER:
                            releaseAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.releasing.never");
                            fileAttBuf.append(Integer.toString(
                                Releaser.NEVER)).append("###");
                            break;
                        case FileUtil.RL_ATT_WHEN1COPY:
                            releaseAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.releasing.onecopy");
                            fileAttBuf.append(Integer.toString(
                                Releaser.WHEN_1)).append("###");
                            break;
                        default:
                            releaseAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.releasing.defaults");
                            fileAttBuf.append(Integer.toString(
                                Releaser.RESET_DEFAULTS)).append("###");
                            break;
                    }
                } else if (liveMode) {
                    // default
                    releaseAttString =
                        SamUtil.getResourceString(
                            "archiving.criteria.releasing.defaults");
                    fileAttBuf.append(Integer.toString(
                        Releaser.RESET_DEFAULTS)).append("###");
                }
                if (releaseAttString != null) {
                    addEntry(
                        "fs.filedetails.releaseatt",
                        releaseAttString.concat(
                            getSpace(8)).concat(
                            getChangeHrefTag(SamQFSSystemFSManager.RELEASE)));
                }

                // Stager Attribute (show default in live mode if -1)
                int stage = stageFile.getStageAttributes();

                String stageAttString = null;
                if (-1 != stage) {
                    switch (stage) {
                        case FileUtil.ST_ATT_NEVER:
                            stageAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.staging.never");
                            fileAttBuf.append(Stager.NEVER);
                            break;
                        case FileUtil.ST_ATT_ASSOCIATIVE:
                            stageAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.staging.associative");
                            fileAttBuf.append(Stager.ASSOCIATIVE);
                            break;
                        default:
                            stageAttString =
                                SamUtil.getResourceString(
                                    "archiving.criteria.staging.defaults");
                            fileAttBuf.append(Stager.RESET_DEFAULTS);
                            break;
                    }
                } else if (liveMode) {
                    // default
                    stageAttString =
                        SamUtil.getResourceString(
                            "archiving.criteria.staging.defaults");
                    fileAttBuf.append(Stager.RESET_DEFAULTS);
                }
                if (stageAttString != null) {
                    addEntry(
                        "fs.filedetails.stageatt",
                        stageAttString.concat(
                            getSpace(8)).concat(
                            getChangeHrefTag(SamQFSSystemFSManager.STAGE)));
                }

                // Add Max Partial Size to the end of the File att string
                fileAttBuf.append("###").append(
                    Integer.toString(
                        FSUtil.getCurrentFSMaxPartialReleaseSize(
                        fsManager, getFSName())));

                // WORM information, only available in LIVE mode
                if (liveMode &&
                    stageFile.getWormState() != StageFile.WORM_DISABLED) {
                    String [] wormEntry =
                        createWormEntry(
                            stageFile.isDirectory(),
                            stageFile.getWormState(),
                            stageFile.isWormPermanent(),
                            stageFile.getWormDuration(),
                            stageFile.getWormStart(),
                            stageFile.getWormEnd());
                    addEntry(
                        wormEntry[0],
                        wormEntry[1],
                        wormEntry[2]);
                }

                FileCopyDetails [] allDetails = stageFile.getFileCopyDetails();
                for (int j = 0; j < allDetails.length; j++) {
                    String [] copyStringKeyValue =
                        createCopyString(allDetails[j]);
                    addEntry(
                        copyStringKeyValue[0],
                        copyStringKeyValue[1],
                        FSUtil.getImage(
                            Integer.toString(allDetails[j].getCopyNumber()),
                            allDetails[j].getMediaType(),
                            allDetails[j].isDamaged()));
                }

                // Add partial release size to the buffer, -1 if not set
                fileAttBuf.append("###").append(Long.toString(partialRelease));

                // Save file attributes to page session + max partial size of fs
                setPageSessionAttribute(
                    ChangeFileAttributesView.PSA_FILE_ATT,
                    fileAttBuf.toString());

            } // end if archiving
        } catch (SamFSException samEx) {
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "fs.filedetails.fail.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        // set page title
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("fs.filedetails.pagetitle",
            new String [] {fileName}));

        // set page mode hidden field
        ((CCHiddenField) getChild(
            PAGE_MODE)).setValue(Integer.toString(getPageMode()));

        TraceUtil.trace3("Exiting");
    }

    // create copy string to show in actiontable, and to create the key
    // that is shown in the left side of the table
    // String[0] = table key on the left
    // String[1] = value on the right
    private String [] createCopyString(FileCopyDetails details) {
        StringBuffer keyBuf    = new StringBuffer();
        StringBuffer valueBuf  = new StringBuffer();

        keyBuf.append("&nbsp;").append(
            SamUtil.getResourceString(
                "fs.filedetails.copy",
                    new String [] {
                        Integer.toString(details.getCopyNumber())}));
        keyBuf.append(lineBreak);
        valueBuf.append(SamUtil.getMediaTypeString(details.getMediaType()));
        valueBuf.append(lineBreak);

        // Copy 1: <Image>
        //   Created Time: Mon Mar 20 15:31:09 EST 2006
        //   VSNs: AB1234 AB1235
        //   Damaged: Yes|No
        //   <show the following if file is in multiple segments>
        //   Segment: 10
        //   Inconsistent: 2
        //   Stale: 1

        // Created Time
        keyBuf.append(tabSpace).
            append(SamUtil.getResourceString("fs.filedetails.created"));
        keyBuf.append(lineBreak);
        valueBuf.append(SamUtil.getTimeString(details.getCreatedTime()));
        valueBuf.append(lineBreak);

        // VSNs
        keyBuf.append(tabSpace).
            append(SamUtil.getResourceString("fs.filedetails.vsns"));
        keyBuf.append(lineBreak);
        valueBuf.append(details.getVSNsInString());
        valueBuf.append(lineBreak);

        // Damaged
        keyBuf.append(tabSpace).
            append(SamUtil.getResourceString("fs.filedetails.damaged"));
        keyBuf.append(lineBreak);
        valueBuf.append(
            details.isDamaged() ?
                (details.getDamagedCount() > 1 ?
                    SamUtil.getResourceString("samqfsui.yes").
                        concat(" (").concat(
                        Integer.toString(details.getDamagedCount())).
                        concat(")") :
                    SamUtil.getResourceString("samqfsui.yes")) :
                SamUtil.getResourceString("samqfsui.no"));
        valueBuf.append(lineBreak);

        // Segment
        if (-1 != details.getSegCount()) {
            keyBuf.append(tabSpace).
                append(SamUtil.getResourceString("fs.filedetails.segment"));
            keyBuf.append(lineBreak);
            valueBuf.append(Integer.toString(details.getSegCount()));
            valueBuf.append(lineBreak);
        }

        if (-1 != details.getInconsistentCount()) {
            keyBuf.append(tabSpace).append(
                SamUtil.getResourceString("fs.filedetails.inconsistent"));
            keyBuf.append(lineBreak);
            valueBuf.append(Integer.toString(details.getInconsistentCount()));
            valueBuf.append(lineBreak);
        }

        if (-1 != details.getStaleCount()) {
            keyBuf.append(tabSpace).
                append(SamUtil.getResourceString("fs.filedetails.stale"));
            keyBuf.append(lineBreak);
            valueBuf.append(Integer.toString(details.getStaleCount()));
            valueBuf.append(lineBreak);
        }

        return new String [] {
                keyBuf.toString(),
                valueBuf.toString()};
    }

    private String [] createWormEntry(
        boolean dir,
        short wormState,
        boolean wormPermanent,
        long wormDuration,
        long wormStart,
        long wormEnd) {

        StringBuffer title = new StringBuffer();
        StringBuffer content = new StringBuffer();

        // Title
        title.append(SamUtil.getResourceString("fs.filedetails.worm"));
        switch (wormState) {
            case StageFile.WORM_ACTIVE:
                content.append(SamUtil.getResourceString(
                    "fs.filedetails.worm.state.active"));
                break;
            case StageFile.WORM_EXPIRED:
                content.append(SamUtil.getResourceString(
                    "fs.filedetails.worm.state.expired"));
                break;
            case StageFile.WORM_CAPABLE:
                content.append(SamUtil.getResourceString(
                    "fs.filedetails.worm.state.capable"));
                break;
        }

        // WORM policy Duration
        title.append(lineBreak);
        content.append(lineBreak);
        title.append(tabSpace).append(
            SamUtil.getResourceString("fs.filedetails.worm.duration"));
        content.append(
            wormPermanent ?
                SamUtil.getResourceString(
                    "fs.filedetails.worm.duration.permanent") :
                TimeConvertor.newTimeConvertor(
                    wormDuration, TimeConvertor.UNIT_MIN).toString());

        if (!dir) {
            // Start WORM policy time
            title.append(lineBreak);
            content.append(lineBreak);

            title.append(tabSpace).append(
                SamUtil.getResourceString("fs.filedetails.worm.start"));
            content.append(SamUtil.getTimeString(wormStart));

            // WORM end time (only if duration is not permanent)
            if (!wormPermanent) {
                title.append(lineBreak);
                content.append(lineBreak);

                title.append(tabSpace).append(
                    SamUtil.getResourceString("fs.filedetails.worm.end"));
                content.append(SamUtil.getTimeString(wormEnd));
            }
        }

        return new String [] {
            title.toString(),
            content.toString(),
            dir ?
                Constants.Image.ICON_DIR_WORM :
                Constants.Image.ICON_WORM};
    }

    /**
     * Handle request for Change HREF
     * @param RequestInvocationEvent event
     */
    public void handleChangeHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        CCHref myHref = (CCHref) getChild(CHANGE_HREF);
        ((ChangeFileAttributesView) getChild(CHANGE_VIEW)).
            setPageMode(Integer.parseInt((String) myHref.getValue()));
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Get the mode of the page
     * -1 == not change mode
     * 0  == Change Archive Attribute mode
     * 1  == Change Release Attribute mode
     * 2  == Change Stage   Attribute mode
     */
    public int getPageMode() {
        int returnValue =
            ((ChangeFileAttributesView) getChild(CHANGE_VIEW)).getPageMode();
        return returnValue;
    }

    // For entries that has icon
    private void addEntry(
        String keyText, Object valueObject, String iconLocation)
        throws ModelControlException {

        if (tableModel.getSize() > 0) {
            tableModel.appendRow();
        }

        tableModel.setValue("KeyText", bold(keyText));
        tableModel.setValue("ValueText", valueObject);
        tableModel.setValue("ValueIcon", iconLocation);
    }

    // For entries that does not have icon
    private void addEntry(
        String keyText, Object valueObject)
        throws ModelControlException {

        if (tableModel.getSize() > 0) {
            tableModel.appendRow();
        }

        tableModel.setValue("KeyText", bold(keyText));
        tableModel.setValue("ValueText", valueObject);
        tableModel.setValue("ValueIcon", Constants.Image.ICON_BLANK_ONE_PIXEL);
    }

    // Make string bolded
    private String bold(String input) {
        return "<b>".concat(SamUtil.getResourceString(input)).concat("</b>");
    }

    /**
     * Generate space(s)
     */
    private String getSpace(int numOfSpace) {
        if (numOfSpace <= 0) {
            return "";
        }
        StringBuffer buf = new StringBuffer();
        StringBuffer space = new StringBuffer("&nbsp;");
        for (int i = 0; i < numOfSpace; i++) {
           buf.append(space);
        }
        return buf.toString();
    }

    /**
     * Generate the Change HREF link for users to change file attributes
     */
    private String getChangeHrefTag(int type) {
        // First check if user has permission to change these attributes.
        // If not, simply return an empty string
        if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILE_OPERATOR)) {
            return "";
        }

        // href looks like this:
        // <a href="../fs/FileDetailsPopup?FileDetailsPopup.ChangeHref=0"
        //  name="FileDetailsPopup.ChangeHref"
        //  onclick="
        //   javascript:var f=document.FileDetailsPopupForm;
        //   if (f != null) {f.action=this.href;f.submit();
        //   return false}">Change</a>"

        StringBuffer href = new StringBuffer()
            .append("<a href=\"../fs/").append(PAGE_NAME).append("?")
            .append(PAGE_NAME).append(".").append(CHANGE_HREF).append("=")
            .append(type).append("\" ")
            .append("name=\"").append(PAGE_NAME).append(".")
            .append(CHANGE_HREF).append("\" ")
            .append("onclick=\"")
            .append("javascript:var f=document.").append(PAGE_NAME)
            .append("Form;")
            .append("if (f != null) {f.action=this.href;f.submit();")
            .append("return false}\" > ")
            .append(SamUtil.getResourceString("fs.filedetails.changelink"))
            .append("</a>");

        return href.toString();
    }

    private boolean isArchiving() {
        String isArchiving = (String) getPageSessionAttribute(IS_ARCHIVING);

        if (isArchiving == null) {
            isArchiving =
                RequestManager.getRequest().getParameter(IS_ARCHIVING);
            setPageSessionAttribute(IS_ARCHIVING, isArchiving);
        }

        return Boolean.valueOf(isArchiving).booleanValue();
    }

    public String getFileNameWithPath() {
        String fileName = (String) getPageSessionAttribute(FILE_TO_VIEW);

        if (fileName == null) {
            fileName =
                RequestManager.getRequest().getParameter(FILE_TO_VIEW);
            setPageSessionAttribute(FILE_TO_VIEW, fileName);
        }

        return fileName;
    }

    public String getFSName() {
        String fsName = (String) getPageSessionAttribute(FS_NAME);

        if (fsName == null) {
            fsName =
                RequestManager.getRequest().getParameter(FS_NAME);
            setPageSessionAttribute(FS_NAME, fsName);
        }

        return fsName;
    }

    private String getMountPoint() {
        String mountPoint = (String) getPageSessionAttribute(MOUNT_POINT);

        if (mountPoint == null) {
            mountPoint =
                RequestManager.getRequest().getParameter(MOUNT_POINT);
            setPageSessionAttribute(MOUNT_POINT, mountPoint);
        }

        return mountPoint;
    }

    private String getCurrentDirectory() {
        String [] dirComp = getFileNameWithPath().split(File.separator);
        if (dirComp.length == 2) {
            return File.separator;
        }

        StringBuffer buf = new StringBuffer();
        for (int i = 1; i < dirComp.length - 1; i++) {
            buf.append(File.separator);
            buf.append(dirComp[i]);
        }

        return buf.toString();
    }
    private String getFileName() {
        String [] dirComp = getFileNameWithPath().split(File.separator);
        if (dirComp.length > 1) {
            return dirComp[dirComp.length - 1];
        } else {
            return "";
        }
    }

    /**
     * getRecoveryPoint returns null in live mode
     */
    protected String getRecoveryPoint() {
        String recoveryPoint =
            (String) getPageSessionAttribute(RECOVERY_POINT_PATH);

        if (recoveryPoint == null) {
            recoveryPoint =
                RequestManager.getRequest().getParameter(RECOVERY_POINT_PATH);
            setPageSessionAttribute(RECOVERY_POINT_PATH, recoveryPoint);
        }

        return recoveryPoint == null ? "" : recoveryPoint;
    }

    public boolean isDirectory() {
        String isDirectory = (String)getPageSessionAttribute(IS_DIR);

        if (isDirectory == null) {
            isDirectory = RequestManager.getRequest().getParameter(IS_DIR);
            setPageSessionAttribute(IS_DIR, isDirectory);
        }

        return Boolean.valueOf(isDirectory).booleanValue();
    }

    private StageFile getSelectedStageFile(
        SamQFSSystemFSManager fsManager, String fsName, String fileNameWithPath)
        throws SamFSException {

        String mountPoint = getMountPoint();

        String relativePathName;
        if (File.separator.equals(mountPoint)) {
            relativePathName = fileNameWithPath.substring(mountPoint.length());
        } else {
            relativePathName =
                fileNameWithPath.substring(mountPoint.length() + 1);
        }

        int theseDetails =
            FileUtil.FNAME |
            FileUtil.FILE_TYPE |
            FileUtil.SIZE |
            FileUtil.CREATED |
            FileUtil.MODIFIED |
            FileUtil.ACCESSED |
            FileUtil.USER |
            FileUtil.GROUP |
            FileUtil.WORM |
            FileUtil.CHAR_MODE |
            FileUtil.SAM_STATE |
            FileUtil.ARCHIVE_ATTS |
            FileUtil.RELEASE_ATTS |
            FileUtil.STAGE_ATTS |
            FileUtil.SEGMENT_ATTS |
            FileUtil.COPY_DETAIL;

        TraceUtil.trace2(
            "Getting File Information: fsName: " + fsName +
            " recoveryPoint: " + getRecoveryPoint() +
            " relativePathName: " + relativePathName +
            " theseDetails: " + theseDetails);

        return fsManager.getFileInformation(
                    fsName,
                    getRecoveryPoint(), // snapPath
                    relativePathName,
                    theseDetails);
    }
}
