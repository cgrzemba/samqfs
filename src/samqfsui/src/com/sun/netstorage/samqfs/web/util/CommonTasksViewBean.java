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

// ident	$Id: CommonTasksViewBean.java,v 1.9 2008/05/16 18:39:06 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.ui.taglib.TaskSubsection;
import com.sun.netstorage.samqfs.web.ui.taglib.TasksSection;
import com.sun.netstorage.samqfs.web.ui.taglib.TasksSectionModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

interface CommonTasksID {
    /**
     * this interface defines the ID values for the various actions taken from
     * the common takss page
     */

    interface FirstTimeUse {
        public static final String ADD_LIBRARY = "11";
        public static final String IMPORT_VSN = "12";
        public static final String ADD_DISK_VSN = "13";
        public static final String ADD_VSN_POOL = "14";
        public static final String CREATE_FILESYSTEM = "15";
        public static final String SCHEDULE_RECOVERY_POINT = "16";
    }

    interface Archive {
        public static final String CREATE_DATA_CLASS = "21";
        public static final String CREATE_POLICY = "22";
        public static final String VIEW_POLICY = "23";
        public static final String VIEW_DATA_CLASS = "24";
    }

    interface Storage {
        public static final String VIEW_LIBRARY = "31";
        public static final String VIEW_TAPE_VSN = "32";
        public static final String VIEW_DISK_VSN = "33";
        public static final String VIEW_VSN_POOL = "34";
    }

    interface FileSystem {
        public static final String VIEW_FILESYSTEM = "41";
        public static final String VIEW_NFS_SHARE = "42";
        public static final String VIEW_FILE_BROWSER = "43";
        public static final String VIEW_RECOVERY_POINT = "44";
    }

    interface Observability {
        public static final String VIEW_EMAIL_ALERTS = "51";
        public static final String VIEW_FAULTS = "52";
        public static final String VIEW_MONITORING = "53";
    }

    interface Overview {
        public static final String OVERVIEW = "61";
    }
}

public class CommonTasksViewBean extends CommonViewBeanBase
    implements CommonTasksID {
    public static final String PAGE_NAME   = "CommonTasks";
    public static final String DEFAULT_URL = "/jsp/util/CommonTasks.jsp";
    public static final String PAGE_TITLE = "PageTitle";

    // task categories
    public static final String FIRST_TIME = "firstTimeUseTasks";
    public static final String ARCHIVE = "archiveTasks";
    public static final String STORAGE = "storageAdministrationTasks";
    public static final String FILESYSTEM = "fileSystemTasks";
    public static final String OBSERVABILITY = "observabilityTasks";
    public static final String OVERVIEW = "overviewTasks";

    // helper href & hidden field to handle task clicks
    public static final String FORWARD_TO_HREF = "forwardToPage";
    public static final String FORWARD_TO_PAGE = "toPageName";
    public static final String SERVER_NAME = "serverName";
    public static final String DESCRIPTION = "description";

    // wizards view
    public static final String WIZARD_VIEW = "CommonTasksWizardsView";
    private Boolean isqfs = null;

    public CommonTasksViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        registerChildren();
    }

    public void registerChildren() {
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(FORWARD_TO_HREF, CCHref.class);
        registerChild(FORWARD_TO_PAGE, CCHiddenField.class);
        registerChild(WIZARD_VIEW, CommonTasksWizardsView.class);
        super.registerChildren();
    }

    public View createChild(String name) {
        if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(FORWARD_TO_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(FORWARD_TO_PAGE)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals("test")) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else if (name.equals(FIRST_TIME)) {
            return new TasksSection(getFirstTimeUseModel());
        } else if (name.equals(ARCHIVE)) {
            return new TasksSection(getArchiveModel());
        } else if (name.equals(STORAGE)) {
            return new TasksSection(getStorageModel());
        } else if (name.equals(FILESYSTEM)) {
            return new TasksSection(getFileSystemModel());
        } else if (name.equals(OBSERVABILITY)) {
            return new TasksSection(getObservabilityModel());
        } else if (name.equals(OVERVIEW)) {
            return new TasksSection(getOverviewModel());
        } else if (name.equals(DESCRIPTION)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(WIZARD_VIEW)) {
            return new CommonTasksWizardsView(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    /** overview model */
    private TasksSectionModel getOverviewModel() {
        TasksSectionModel model = new TasksSectionModel();
        model.setName("");

        TaskSubsection sb = new TaskSubsection(Overview.OVERVIEW);
        sb.setOnClick("return launchOverview();");
        sb.setName("commontasks.overview");
        sb.setHelp("commontasks.overview.help.title",
                   "commontasks.overview.help.content");
        model.addSubsection(sb);

        return model;
    }

    /** define the first time use tasks */
    private TasksSectionModel getFirstTimeUseModel() {
        TasksSectionModel model = new TasksSectionModel();
        model.setName("commontasks.firstuse");

        if (!isQFSOnly()) {
        TaskSubsection sb = new TaskSubsection(FirstTimeUse.ADD_LIBRARY);
        sb.setOnClick("return launchWizard('addLibraryWizard');");
        sb.setName("commontasks.add.library");
        sb.setHelp("commontasks.add.library.help.title",
                    "commontasks.add.library.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(FirstTimeUse.IMPORT_VSN);
        sb.setOnClick("return importTapeVSN();");
        sb.setName("commontasks.importvsn");
        sb.setHelp("commontasks.importvsn.help.title",
                   "commontasks.importvsn.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(FirstTimeUse.ADD_DISK_VSN);
        sb.setOnClick("return addDiskVSN();");
        sb.setName("commontasks.add.diskvsn");
        sb.setHelp("commontasks.add.diskvsn.help.title",
                   "commontasks.add.diskvsn.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(FirstTimeUse.ADD_VSN_POOL);
        sb.setOnClick("return addVSNPool();");
        sb.setName("commontasks.create.vsnpool");
        sb.setHelp("commontasks.create.vsnpool.help.title",
                   "commontasks.create.vsnpool.help.content");
        model.addSubsection(sb);

        }

        TaskSubsection sb = new TaskSubsection(FirstTimeUse.CREATE_FILESYSTEM);
        sb.setOnClick("return launchWizard('newFileSystemWizard');");
        sb.setName("commontasks.create.filesystem");
        sb.setHelp("commontasks.create.filesystem.help.title",
                   "commontasks.create.filesystem.help.content");
        model.addSubsection(sb);

        if (!isQFSOnly()) {
        sb = new TaskSubsection(FirstTimeUse.SCHEDULE_RECOVERY_POINT);
        sb.setOnClick("return scheduleRecoveryPoint();");
        sb.setName("commontasks.schedule.recoverypoint");
        sb.setHelp("commontasks.schedule.recoverypoint.help.title",
                   "commontasks.schedule.recoverypoint.help.content");
        model.addSubsection(sb);
        }
        return model;
    }

    /** define the archiving tasks */
    private TasksSectionModel getArchiveModel() {
        TasksSectionModel model = new TasksSectionModel();
        model.setName("commontasks.archive");

        TaskSubsection sb = new TaskSubsection(Archive.CREATE_DATA_CLASS);
        sb.setOnClick("return launchWizard('newDataClassWizard');");
        sb.setName("commontasks.create.dataclass");
        sb.setHelp("commontasks.create.dataclass.help.title",
                   "commontasks.create.dataclass.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Archive.CREATE_POLICY);
        sb.setOnClick("return launchWizard('newPolicyWizard');");
        sb.setName("commontasks.create.policy");
        sb.setHelp("commontasks.create.policy.help.title",
                   "commontasks.create.policy.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Archive.VIEW_POLICY);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/archive/PolicySummary');");
        sb.setName("commontasks.view.policy");
        sb.setHelp("commontasks.view.policy.help.title",
                   "commontasks.view.policy.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Archive.VIEW_DATA_CLASS);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/archive/DataClassSummary');");
        sb.setName("commontasks.view.dataclass");
        sb.setHelp("commontasks.view.dataclass.help.title",
                   "commontasks.view.dataclass.help.content");
        model.addSubsection(sb);

        return model;
    }

    /** define the file system related tasks */
    private TasksSectionModel getFileSystemModel() {
        TasksSectionModel model = new TasksSectionModel();
        model.setName("commontasks.filesystems");

        TaskSubsection sb = new TaskSubsection(FileSystem.VIEW_FILESYSTEM);
        sb.setOnClick("return forwardToPage('/samqfsui/fs/FSSummary');");
        sb.setName("commontasks.view.filesystem");
        sb.setHelp("commontasks.view.filesystem.help.title",
                   "commontasks.view.filesystem.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(FileSystem.VIEW_NFS_SHARE);
        sb.setOnClick("return forwardToPage('/samqfsui/fs/NFSDetails');");
        sb.setName("commontasks.view.nfsshares");
        sb.setHelp("commontasks.view.nfsshares.help.title",
                   "commontasks.view.nfsshares.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(FileSystem.VIEW_FILE_BROWSER);
        sb.setOnClick("return forwardToPage('/samqfsui/fs/FileBrowser');");
        sb.setName("commontasks.view.filebrowser");
        sb.setHelp("commontasks.view.filebrowser.help.title",
                   "commontasks.view.filebrowser.help.content");
        model.addSubsection(sb);

        if (!isQFSOnly()) {
        sb = new TaskSubsection(FileSystem.VIEW_RECOVERY_POINT);
        sb.setOnClick("return forwardToPage('/samqfsui/fs/RecoveryPoints');");
        sb.setName("commontasks.view.recoverypoint");
        sb.setHelp("commontasks.view.recoverypoint.help.title",
                   "commontasks.view.recoverypoint.help.content");
        model.addSubsection(sb);
        }

        return model;
    }

    /** define storage tasks */
    private TasksSectionModel getStorageModel() {
        TasksSectionModel model = new TasksSectionModel();
        model.setName("commontasks.storage");

        TaskSubsection sb = new TaskSubsection(Storage.VIEW_LIBRARY);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/media/LibrarySummary');");
        sb.setName("commontasks.view.medialibrary");
        sb.setHelp("commontasks.view.medialibrary.help.title",
                   "commontasks.view.medialibrary.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Storage.VIEW_TAPE_VSN);
        sb.setOnClick("return forwardToPage('/samqfsui/media/VSNSummary');");
        sb.setName("commontasks.view.tapevsn");
        sb.setHelp("commontasks.view.tapevsn.help.title",
                   "commontasks.view.tapevsn.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Storage.VIEW_DISK_VSN);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/archive/DiskVSNSummary');");
        sb.setName("commontasks.view.diskvsn");
        sb.setHelp("commontasks.view.diskvsn.help.title",
                   "commontasks.view.diskvsn.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Storage.VIEW_VSN_POOL);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/archive/VSNPoolSummary');");
        sb.setName("commontasks.view.vsnpool");
        sb.setHelp("commontasks.view.vsnpool.help.title",
                   "commontasks.view.vsnpool.help.content");
        model.addSubsection(sb);

        return model;
    }

    /** define observability tasks */
    private TasksSectionModel getObservabilityModel() {
        TasksSectionModel model = new TasksSectionModel();
        model.setName("commontasks.observability");

        TaskSubsection sb =
            new TaskSubsection(Observability.VIEW_EMAIL_ALERTS);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/admin/AdminNotification');");
        sb.setName("commontasks.view.emailalerts");
        sb.setHelp("commontasks.view.emailalerts.help.title",
                   "commontasks.view.emailalerts.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Observability.VIEW_FAULTS);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/alarms/CurrentAlarmSummary');");
        sb.setName("commontasks.view.faults");
        sb.setHelp("commontasks.view.faults.help.title",
                   "commontasks.view.faults.help.content");
        model.addSubsection(sb);

        sb = new TaskSubsection(Observability.VIEW_MONITORING);
        sb.setOnClick(
            "return forwardToPage('/samqfsui/monitoring/SystemMonitoring');");
        sb.setName("commontasks.view.monitoring");
        sb.setHelp("commontasks.view.monitoring.help.title",
                   "commontasks.view.monitoring.help.content");
        model.addSubsection(sb);

        return model;
    }

    public void handleForwardToPageRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        String toPage = getDisplayFieldStringValue(FORWARD_TO_HREF);

        toPage = getDisplayFieldStringValue(FORWARD_TO_PAGE);
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // set the server name here so that the pages that don't require any
        // additional information can navigate from the common tasks page
        // directly to the page without a trip to the server
        ((CCHiddenField)getChild(SERVER_NAME)).setValue(getServerName());
    }

    public boolean isQFSOnly() {
        if (this.isqfs == null) {
            // init isqfs variable
            isqfs = new Boolean(SamUtil.getSystemType(getServerName()) ==
                                SamQFSSystemModel.QFS);
        }

        return this.isqfs.booleanValue();
    }
}
