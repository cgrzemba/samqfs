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

// ident	$Id: CreateFSWizardImpl.java,v 1.108 2008/10/09 14:28:01 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;
import com.sun.netstorage.samqfs.mgmt.SamFSTimeoutException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.Copy;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;
import com.sun.netstorage.samqfs.mgmt.fs.FSArchCfg;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericMountOptions;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.impl.jni.fs.GenericMountOptionsImpl;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.ServerInfo;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.netstorage.samqfs.web.wizard.WizardResultView;
import com.sun.netstorage.samqfs.web.wizard.WizardUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;
import com.sun.web.ui.view.table.CCActionTable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.StringTokenizer;
import javax.servlet.http.HttpSession;

interface CreateFSWizardImplData {

    final String name = "CreateFSWizardImpl";
    final String title = "FSWizard.new.title";

    final Class[] pageClass = {
        NewWizardFSTypeView.class,
        FSWizardSharedMemberSelectionPageView.class,
        FSWizardMetadataDeviceSelectionPageView.class,
        FSWizardStripedGroupDeviceSelectionPageView.class,
        FSWizardDataDeviceSelectionPageView.class,
        NewWizardMountView.class,
        NewWizardStdMountView.class,
        NewWizardQFSSummaryView.class,
        NewWizardFSSummaryView.class,
        NewWizardFSStdSummaryView.class,
        NewWizardClusterNodesView.class,
        NewWizardArchiveConfigView.class,
        NewWizardAcceptQFSDefaultsView.class,
        NewWizardMetadataOptionsView.class,
        NewWizardBlockAllocationView.class,
        NewWizardArchivePolicyView.class,
        NewWizardArchiveMediaView.class,
        WizardResultView.class
    };

    final String[] pageTitle = {
        "FSWizard.new.fstype.title",
        "FSWizard.sharedMemberPage.title",
        "FSWizard.metadataDevicePage.title",
        "FSWizard.stripedGroupDevicePage.title",
        "FSWizard.dataDevicePage.title",
        "FSWizard.new.mountOptionsPage.title",
        "FSWizard.new.mountOptionsPage.title",
        "FSWizard.new.summaryPage.title",
        "FSWizard.new.summaryPage.title",
        "FSWizard.new.summaryPage.title",
        "FSWizard.new.clusternodes.title",
        "FSWizard.new.archiving.title",
        "FSWizard.new.qfsdefaults.title",
        "FSWizard.new.metadataoptions.title",
        "FSWizard.new.blockallocation.title",
        "FSWizard.new.archivepolicy.title",
        "FSWizard.new.archivemedia.title",
        "wizard.result.steptext"
    };

    final String[][] stepHelp = {
        {"FSWizard.new.fstype.help.fstype",
         "FSWizard.new.fstype.help.hafs",
         "FSWizard.new.fstype.help.archiving",
         "FSWizard.new.fstype.help.shared",
         "FSWizard.new.fstype.help.hpc",
         "FSWizard.new.fstype.help.matfs"},
        {"FSWizard.sharedMemberPage.help.text1",
         "FSWizard.sharedMemberPage.help.text2",
         "FSWizard.sharedMemberPage.help.text3",
         "FSWizard.sharedMemberPage.help.text4"},
        {"FSWizard.devicePage.help.text1",
         "FSWizard.devicePage.help.text2",
         "FSWizard.devicePage.help.text3",
         "FSWizard.devicePage.help.text4",
         "FSWizard.devicePage.help.text5",
         "FSWizard.devicePage.help.text6",
         "FSWizard.devicePage.help.text7",
         "FSWizard.devicePage.help.text8",
         "FSWizard.devicePage.help.text9",
         "FSWizard.devicePage.help.text10",
         "FSWizard.devicePage.help.text11"},
        {"FSWizard.stripedGroupDevicePage.help.text1",
         "FSWizard.stripedGroupDevicePage.help.text2",
         "FSWizard.devicePage.help.text1",
         "FSWizard.devicePage.help.text2",
         "FSWizard.devicePage.help.text3",
         "FSWizard.devicePage.help.text4",
         "FSWizard.devicePage.help.text5",
         "FSWizard.devicePage.help.text6",
         "FSWizard.devicePage.help.text7",
         "FSWizard.devicePage.help.text8",
         "FSWizard.devicePage.help.text9",
         "FSWizard.devicePage.help.text10",
         "FSWizard.devicePage.help.text11"},
        {"FSWizard.devicePage.help.text1",
         "FSWizard.devicePage.help.text2",
         "FSWizard.devicePage.help.text3",
         "FSWizard.devicePage.help.text4",
         "FSWizard.devicePage.help.text5",
         "FSWizard.devicePage.help.text6",
         "FSWizard.devicePage.help.text7",
         "FSWizard.devicePage.help.text8",
         "FSWizard.devicePage.help.text9",
         "FSWizard.devicePage.help.text10",
         "FSWizard.devicePage.help.text11"},
        {"FSWizard.new.mountOptionsPage.help.common.text1",
         "FSWizard.new.mountOptionsPage.help.qfs.text2",
         "FSWizard.new.mountOptionsPage.help.qfs.text3",
         "FSWizard.new.mountOptionsPage.help.qfs.text4"},
        {"FSWizard.new.mountOptionsPage.help.common.text1",
         "FSWizard.new.mountOptionsPage.help.ufs.text2"},
        {"FSWizard.new.summaryPage.help.text1",
         "FSWizard.new.summaryPage.help.text2"},
        {"FSWizard.new.summaryPage.help.text1",
         "FSWizard.new.summaryPage.help.text2"},
        {"FSWizard.new.summaryPage.help.text1",
         "FSWizard.new.summaryPage.help.text2"},
        {"FSWizard.new.clusternodes.help.text1",
         "FSWizard.new.clusternodes.help.text2"},
        {"FSWizard.new.archiving.help.text1",
         "FSWizard.new.archiving.help.text2",
         "FSWizard.new.archiving.help.text3",
         "FSWizard.new.archiving.help.text4",
         "FSWizard.new.archiving.help.text5",
         "FSWizard.new.archiving.help.text6",
         "FSWizard.new.archiving.help.text7",
         "FSWizard.new.archiving.help.text8"},
        {"FSWizard.new.qfsdefaults.help.text1"},
        {"FSWizard.new.metadataoptions.help.text1"},
        {"FSWizard.new.blockallocation.help.text1"},
        {"FSWizard.new.archivepolicy.help.text1"},
        {"FSWizard.new.archivemedia.help.text1"},
        {"wizard.result.help.text1",
         "wizard.result.help.text2"}
    };

    final String[] stepText = {
        "FSWizard.new.fstype.step.text",
        "FSWizard.sharedMemberPage.step.text",
        "FSWizard.metadataDevicePage.step.text",
        "FSWizard.stripedGroupDevicePage.step.text",
        "FSWizard.dataDevicePage.step.text",
        "FSWizard.new.mountOptionsPage.step.text",
        "FSWizard.new.mountOptionsPage.step.text",
        "FSWizard.new.summaryPage.step.text",
        "FSWizard.new.summaryPage.step.text",
        "FSWizard.new.summaryPage.step.text",
        "FSWizard.new.clusternodes.step.text",
        "FSWizard.new.archiving.step.text",
        "FSWizard.new.qfsdefaults.step.text",
        "FSWizard.new.metadataoptions.step.text",
        "FSWizard.new.blockallocation.step.text",
        "FSWizard.new.archivepolicy.step.text",
        "FSWizard.new.archivemedia.step.text",
        "wizard.result.steptext"
    };

    final String[] stepInstruction = {
        "FSWizard.new.fstype.instruction.text",
        "FSWizard.sharedMemberPage.instruction.text",
        "FSWizard.metadataDevicePage.instruction.text",
        "FSWizard.stripedGroupDevicePage.instruction.text",
        "FSWizard.dataDevicePage.instruction.text",
        "FSWizard.new.mountOptionsPage.instruction.text",
        "FSWizard.new.mountOptionsPage.instruction.text",
        "FSWizard.new.summaryPage.instruction.text",
        "FSWizard.new.summaryPage.instruction.text",
        "FSWizard.new.summaryPage.instruction.text",
        "FSWizard.new.clusternodes.instruction.text",
        "FSWizard.new.archiving.instruction.text",
        "FSWizard.new.qfsdefaults.instruction.text",
        "FSWizard.new.metadataoptions.instruction.text",
        "FSWizard.new.blockallocation.instruction.text",
        "FSWizard.new.archivepolicy.instruction.text",
        "FSWizard.new.archivemedia.instruction.text",
        "wizard.result.instruction"
    };

    final int PAGE_FS_TYPE = 0;
    final int PAGE_SHARED_MEMBER  = 1;
    final int PAGE_METADATA_LUN   = 2;
    final int PAGE_STRIPED_GROUP  = 3;
    final int PAGE_DATA_LUN = 4;
    final int PAGE_MOUNT = 5;
    final int PAGE_STD_MOUNT = 6;
    final int PAGE_QFS_SUMMARY = 7;
    final int PAGE_FS_SUMMARY = 8;
    final int PAGE_FS_STD_SUMMARY = 9;
    final int PAGE_CLUSTER_NODES = 10;
    final int PAGE_ARCHIVE_CONFIG = 11;
    final int PAGE_QFS_DEFAULTS = 12;
    final int PAGE_METADATA_OPTIONS = 13;
    final int PAGE_BLOCK_ALLOCATION = 14;
    final int PAGE_ARCHIVE_POLICY = 15;
    final int PAGE_ARCHIVE_MEDIA = 16;
    final int PAGE_RESULT = 17;

    // 5.0 Pages
    // 1. non-shared, non-archiving, non-hpc qfs
    // the rest of the pages will be inserted dynamically : for now
    final int [] qfs_pages = {
        PAGE_FS_TYPE,
        PAGE_QFS_DEFAULTS,
        PAGE_MOUNT,
        PAGE_DATA_LUN,
        PAGE_FS_SUMMARY,
        PAGE_RESULT
    };

    // ufs pages
    final int [] ufs_pages = {
        PAGE_FS_TYPE,
        PAGE_STD_MOUNT,
        PAGE_DATA_LUN,
        PAGE_FS_STD_SUMMARY,
        PAGE_RESULT
    };

}


public class CreateFSWizardImpl extends SamWizardImpl {
    // implements CreateFSWizardImplData {
    // symbolic constants for parameters from the New File System Popup passed
    // in via the HttpServletRequest object
    public static final String POPUP_FSTYPE = "newfspopup.fstype";
    public static final String POPUP_ARCHIVING = "newfspopup.archiving";
    public static final String POPUP_SHARED = "newfspopup.shared";
    public static final String POPUP_HAFS = "newfspopup.hafs";
    public static final String POPUP_HPC = "newfspopup.hpc";
    public static final String POPUP_MATFS = "newfspopup.matfs";

    // NEW: constant string for wizard
    public static final String WIZARDPAGEMODELNAME = "CreateFSPageModelName";
    public static final String WIZARDPAGEMODELNAME_PREFIX = "6WizardModel";
    public static final String WIZARDIMPLNAME = CreateFSWizardImplData.name;
    public static final String WIZARDIMPLNAME_PREFIX = "WizardImpl";
    public static final String WIZARDCLASSNAME =
        "com.sun.netstorage.samqfs.web.fs.wizards.CreateFSWizardImpl";

    // Constants defining FileSystem Type
    public static final String FSTYPE_KEY = "fsTypeKey";
    public static final String FSTYPE_FS  = "fs";
    public static final String FSTYPE_QFS = "qfs";
    public static final String FSTYPE_SHAREDFS  = "sharedfs";
    public static final String FSTYPE_SHAREDQFS = "sharedqfs";
    public static final String FSTYPE_UFS = "ufs";
    public static final String FSTYPE_HAFS = "hafs";
    public static final String FSTYPE_SHARED_HAFS = "sharedhafs";
    public static final String FSTYPE_HPC = "hpc";

    public static final String ARCHIVE_ENABLED_KEY = "archivingEnabled";

    // Variable to denote the type of FS selected
    private String fsType = FSTYPE_FS;

    // Variable to hold previously selected fs type values
    private String OLD_FS_TYPE = "OLD_FS_TYPE";

    // Variable to hold previously entered number of striped groups
    private String OLD_NUM_STRIPED_GROUPS = "OLD_NUM_STRIPED_GROUPS";

    // Keep this variable even if it is not currently in used.
    // Variable to hold samfs server api version number
    // api 1.0 = samfs 4.1
    // api 1.1 = samfs 4.2
    // api 1.2 = samfs 4.3
    // api 1.3 = samfs 4.4
    // api 1.4 = samfs 4.5
    // api 1.5 = samfs 4.6
    // api 1.6 = samfs 5.0
    private String samfsServerAPIVersion = "1.5";

    private short fsLicense = SamQFSSystemModel.SAMQFS; // default license type
    private boolean archivingEnabled = false;  // archive feature enabled ?
    private boolean sharedEnabled  = false;  // shared feature enabled ?
    private boolean hafsEnabled = false; // HAFS enabled
    private boolean hpcEnabled = false; // HCP FS enabled
    private boolean matfsEnabled = false; // mat fs is enabled

    // boolean to keep track of which version of dau page to process
    private boolean newDAUPage = false;

    // striped group number base
    private int striped_group_base = -1;

    //    CreateFSWizardImplData.NON_SHARED_STRIPED_GROUP_BASE;

    // Variables to store allDevices, selectedMetaDevices, selectedDataDevices
    // and selectedStripedGroupDevices
    private DiskCache[] allAllocatableDevices = null;
    private ArrayList selectedDataDevicesList = null;
    private ArrayList selectedMetaDevicesList = null;
    private ArrayList selectedStripedGroupDevicesList = null;

    private ArrayList selectedPrimaryIPIndex = null;
    private ArrayList selectedSecondaryIPIndex = null;
    private ArrayList selectedClientIndex = null;
    private ArrayList selectedPotentialMetadataServerIndex = null;
    private ArrayList selectedClient = null;
    private ArrayList selectedPotentialMetadataServer = null;
    private ArrayList selectedSharedPotentialMetaServerHostList = null;
    private ArrayList selectedSharedClientHostList = null;

    private SharedMember[] allMemList = null;
    int memSize = 0;

    // Variables to store FS properties entered
    int intHWM = -1, intLWM = -1, intStripe = -1;

    // TODO: this should be moved to base class
    protected String serverName = null;

    // Static Create method called by the viewbean
    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering CreateFSWizardImpl::create()");
        return new CreateFSWizardImpl(requestContext);
    }

    // Constructor
    public CreateFSWizardImpl(RequestContext requestContext) {
        super(requestContext, WIZARDPAGEMODELNAME);
        initializeWizard(requestContext);
        initializeWizardControl(requestContext);
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return
            getWizardWindowModel(
                WIZARDIMPLNAME,
                CreateFSWizardImplData.title,
                WIZARDCLASSNAME,
                cmdChild);
    }

    // overwrite getPageClass() to put striped group number into wizardModel
    // so that it can be accessed by NewWizardStripedGroupPage
    public Class getPageClass(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        if (pages[page] == CreateFSWizardImplData.PAGE_STRIPED_GROUP) {
            wizardModel.setValue(
                Constants.Wizard.STRIPED_GROUP_NUM,
                new Integer(page - striped_group_base));
            TraceUtil.trace2("This is striped group # " +
                (page - striped_group_base));
        }
        // clear out previous errors
        wizardModel.setValue(
            Constants.Wizard.WIZARD_ERROR,
            Constants.Wizard.WIZARD_ERROR_NO);
        return super.getPageClass(pageId);
    }

    public String[] getFuturePages(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);

        int page = pageIdToPage(pageId);
        String[] futurePages = null;

        if (pages[page] == CreateFSWizardImplData.PAGE_FS_TYPE ||
            pages[page] == CreateFSWizardImplData.PAGE_QFS_DEFAULTS ||
            pages[page] == CreateFSWizardImplData.PAGE_METADATA_OPTIONS ||
            pages[page] == CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION) {
            futurePages = new String[0];
        } else {
            int howMany = pages.length - page - 1;
            futurePages = new String[howMany];
            for (int i = 0; i < howMany; i++) {
                futurePages[i] = Integer.toString(page + i);
            }
        }

        return futurePages;
    }

    public String[] getFutureSteps(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);

        int page = pageIdToPage(pageId);
        String[] futureSteps = null;
        if (pages[page] == CreateFSWizardImplData.PAGE_FS_TYPE ||
            pages[page] == CreateFSWizardImplData.PAGE_QFS_DEFAULTS ||
            pages[page] == CreateFSWizardImplData.PAGE_METADATA_OPTIONS ||
            pages[page] == CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION) {
            futureSteps = new String[0];
        } else {
            int howMany = pages.length - page - 1;
            futureSteps = new String[howMany];

            for (int i = 0; i < howMany; i++) {
                int futureStep = page + 1 + i;
                int futurePage = pages[futureStep];
                if (futurePage == CreateFSWizardImplData.PAGE_STRIPED_GROUP) {
                    if (hpcEnabled) {
                        futureSteps[i] = SamUtil.getResourceString(
                        "FSWizard.objectGroupDevicePage.title",
                        Integer.toString(futureStep - striped_group_base));
                    } else {
                    futureSteps[i] = SamUtil.getResourceString(
                        stepText[futurePage],
                        new String[] {Integer.toString(
                    futureStep - striped_group_base) });
                    }
                } else {
                    futureSteps[i] = stepText[futurePage];
                }
            }
        }

        return futureSteps;
    }

    public String getStepText(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String text = null;

        if (pages[page] == CreateFSWizardImplData.PAGE_STRIPED_GROUP) {
            if (hpcEnabled) {
                text = SamUtil
                    .getResourceString("FSWizard.objectGroupDevicePage.title",
                                       Integer.toString(
                                       page - striped_group_base));
            } else {
            text = SamUtil.getResourceString(
                stepText[pages[page]],
                new String[] { Integer.toString(
                    page - striped_group_base) });
            }
        } else if ((hpcEnabled) &&
                   (pages[page] ==
                    CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION)) {
            text = "FSWizard.new.dataallocation.step.text";
        } else {
            text = stepText[pages[page]];
        }

        return text;
    }

    public String getStepInstruction(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String text = null;


        if (pages[page] == CreateFSWizardImplData.PAGE_SHARED_MEMBER) {
            TraceUtil.trace3("METADATA Host name = " + serverName);
            text = SamUtil.getResourceString(
                stepInstruction[pages[page]],
                serverName);
        } else if ((hpcEnabled) &&
           (pages[page] == CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION)) {
            text = "FSWizard.new.dataallocation.instruction.text";
        } else {
            text = stepInstruction[pages[page]];
        }

        return text;
    }

    public String getStepTitle(String pageId) {
        TraceUtil.trace2("Entered with pageID = " + pageId);
        int page = pageIdToPage(pageId);
        String title = null;

        if (pages[page] == CreateFSWizardImplData.PAGE_STRIPED_GROUP) {
            if (hpcEnabled) {
                title = SamUtil
                    .getResourceString("FSWizard.objectGroupDevicePage.title",
                                       Integer.toString(
                                       page - striped_group_base));
            } else {
            title = SamUtil.getResourceString(
                pageTitle[pages[page]],
                new String[] { Integer.toString(
                    page - striped_group_base) });
            }
        } else if ((hpcEnabled) &&
                   (pages[page] ==
                    CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION)) {
            title = "FSWizard.new.dataallocation.step.text";
        } else {

            title = pageTitle[pages[page]];
        }

        return title;
    }

    public String [] getStepHelp(String pageId) {
        int page = pageIdToPage(pageId);
        if ((hpcEnabled) &&
            (pages[page] == CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION)) {
            return new String [] {"FSWizard.new.dataallocation.help.text"};
        } else {
            return super.getStepHelp(pageId);
        }
    }


    /**
     * This method is overwritten to Process each page of the wizard
     */
    public boolean nextStep(WizardEvent wizardEvent) {
        String pageId = wizardEvent.getPageId();
        TraceUtil.trace2("Entered with pageID = " + pageId);

        // make this wizard active
        super.nextStep(wizardEvent);

        int page = pageIdToPage(pageId);

        switch (pages[page]) {
            case CreateFSWizardImplData.PAGE_FS_TYPE:
                return processFSTypePage(wizardEvent);
            case CreateFSWizardImplData.PAGE_QFS_DEFAULTS:
                boolean result = processAcceptQFSDefaultsPage(wizardEvent);
                if (result && !isHAFS(wizardModel) && !sharedEnabled) {
                    result = populateDevicesInWizardModel();
                }
                return result;
            case CreateFSWizardImplData.PAGE_METADATA_OPTIONS:
                return processMetadataOptionsPage(wizardEvent);
            case CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION:
                return processBlockAllocationPage(wizardEvent);
            case CreateFSWizardImplData.PAGE_SHARED_MEMBER:
                boolean result2 = processSharedMemberPage(wizardEvent);
                if (result2 && sharedEnabled) {
                    result2 = populateDevicesInWizardModel();
                }
                return result2;

            case CreateFSWizardImplData.PAGE_METADATA_LUN:
                return processMetaDataDevicesPage(wizardEvent);

            case CreateFSWizardImplData.PAGE_STRIPED_GROUP:
                return processStripedGroupPage(wizardEvent);

            case CreateFSWizardImplData.PAGE_DATA_LUN:
                return processDataDevicesPage(wizardEvent);

            case CreateFSWizardImplData.PAGE_MOUNT:
                return processMountOptionsPage(wizardEvent);

            case CreateFSWizardImplData.PAGE_STD_MOUNT:
                result = processStdMountOptionsPage(wizardEvent);
                if (result)
                    result = populateDevicesInWizardModel();
                return result;

            case CreateFSWizardImplData.PAGE_ARCHIVE_CONFIG:
                return processArchiveConfigPage(wizardEvent);

            case CreateFSWizardImplData.PAGE_CLUSTER_NODES:
                boolean result3 = processClusterNodes(wizardEvent);
                if (result3)
                    result3 = populateDevicesInWizardModel();
                return result3;
        }

        return true;
    }

    /**
     * previousStep is called when the user clicks the Previous Button
     */
    public boolean previousStep(WizardEvent wizardEvent) {
        String pageId = wizardEvent.getPageId();
        TraceUtil.trace2("Entered with pageID = " + pageId);

        // make this wizard active
        super.previousStep(wizardEvent);

        int page = pageIdToPage(pageId);

        // always return true for previousStep
        return true;
    }

    /**
     * FinishStep is called when the user clicks the finish button
     */
    public boolean finishStep(WizardEvent wizardEvent) {

        String pageId = wizardEvent.getPageId();
        TraceUtil.trace2("Entered with pageID = " + pageId);

        // make sure this wizard is still active before commit
        if (super.finishStep(wizardEvent) == false) {
            return true;
        }

        // fsName, intFSType and intArchType are N/A for UFS
        if (fsType.equals(FSTYPE_UFS)) {
            TraceUtil.trace2("Creating a UFS file system");
            return createUFS(); // this will return true or false
        }

        String fsName = ((String) wizardModel.getValue(
            NewWizardMountView.CHILD_FSNAME_FIELD)).trim();
        int intFSType   = FileSystem.COMBINED_METADATA;

        String t = (String)wizardModel
            .getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
        if (NewWizardMetadataOptionsView.SEPARATE_DEVICE.equals(t)) {
            intFSType = FileSystem.SEPARATE_METADATA;
        }

        int intArchType = FileSystem.ARCHIVING;
        if (!archivingEnabled) {
            intArchType = FileSystem.NONARCHIVING;
        }
        int intEquipOrdinal = -1;
        int intShareStatus = FileSystem.UNSHARED;

        boolean boolCreateMountPoint = true; // always create mount point
        boolean boolMountAfterCreate = false;
        boolean boolMountAtBoot = false;
        boolean boolSingleAllocation = false;

        String  mountPoint = ((String) wizardModel.getWizardValue(
            NewWizardMountView.CHILD_MOUNT_FIELD)).trim();

        String mountAtBoot = (String) wizardModel.getValue(
            NewWizardMountView.CHILD_BOOT_CHECKBOX);
        if (mountAtBoot != null && mountAtBoot.equals("samqfsui.yes")) {
            boolMountAtBoot = true;
        }

        String syntax = (String) wizardModel.getValue(
            NewWizardMountView.CHILD_MOUNT_CHECKBOX);
        if (syntax != null && syntax.equals("samqfsui.yes")) {
            boolMountAfterCreate = true;
        }

        // Combined Metadata and data, need to set boolSingleAllocation to false
        // because device type has to be "md"
        if (intFSType == FileSystem.COMBINED_METADATA) {
            boolSingleAllocation = false;
        } else {
            String allocationMethod = (String)wizardModel
                .getValue(NewWizardBlockAllocationView.ALLOCATION_METHOD);
            if (NewWizardBlockAllocationView.SINGLE.equals(allocationMethod)) {
                boolSingleAllocation = true;
            }
        }

        TraceUtil.trace2("QFS Single Allocation: " + boolSingleAllocation);
        int intDAU = 16;
        try {
            Integer i = (Integer)wizardModel
                .getValue(NewWizardBlockAllocationView.BLOCK_SIZE_KB);
            if (i != null) intDAU = i.intValue();

        } catch (NumberFormatException nfe) {
            TraceUtil.trace1("Invalid DAU - " + nfe.getMessage());
        }
        SamQFSSystemModel sysModel = null;
        FileSystemMountProperties properties = null;

        try {
            sysModel = SamUtil.getModel(serverName);

            properties = sysModel.getSamQFSSystemFSManager().
                getDefaultMountProperties(
                intFSType, intArchType, intDAU, false, intShareStatus, false);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                "Failed to get Default Mount Properties",
                serverName);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return true;
        }

        // Set the mount property values that the user selected
        properties = setSelectedMountProperties(properties);

        // If intFSType = SEPARATE_METADATA and intArchType = NONARCHIVING,
        // use the nosam option to enable only the file system functionality
        // The archiving, releasing, and staging functionality is disabled.
        if (intArchType == FileSystem.NONARCHIVING) {
            properties.setArchive(false);
        }

        DiskCache[] metadataDevices =
            getSelectedDeviceArray(selectedMetaDevicesList);
        DiskCache[] dataDevices =
            getSelectedDeviceArray(selectedDataDevicesList);
        StripedGroup[] stripedGroups = null;
        try {
            stripedGroups =
                getSelectedStripedGroupsArray(selectedStripedGroupDevicesList);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                "Failed to create striped groups",
                serverName);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return true;
        }

        wizardModel.setValue(Constants.Wizard.FS_NAME, fsName);
        LogUtil.info(
            this.getClass(),
            "finishStep",
            "Start creating new FS " +  fsName);

        try {
            // cluster nodes to be used
            String [] nodeList = null;
            if (isHAFS(wizardModel)) {
                ArrayList hosts = new ArrayList();

                // start by adding the current node - make sure the name is not
                // fully qualified
                String currentServer =
                    (new StringTokenizer(serverName, ".")).nextToken();
                hosts.add(currentServer);

                Object [] nodes =
                    wizardModel.getValues(NewWizardClusterNodesView.NODES);
                for (int i = 0; i < nodes.length; i++) {
                    // prune current node as well as the '-none-' selection
                    String tempNodeName = nodes[i].toString().trim();
                    if (!tempNodeName.equals("") &&
                        !tempNodeName.equals(currentServer)) {
                        hosts.add(tempNodeName);
                    }
                }

                nodeList = new String[hosts.size()];
                nodeList = (String [])hosts.toArray(nodeList);
            }

            TraceUtil.trace2("Calling CreateFileSystem with name: " + fsName);
            if (sharedEnabled) { // create shared fs
                SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
                SamQFSSystemSharedFSManager fsManager =
                    appModel.getSamQFSSystemSharedFSManager();
                ArrayList clientHosts = (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_CLIENT);

                ArrayList potentialHosts = (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_VALUE);
                String[] clients = (String[])
                    clientHosts.toArray(new String[0]);
                String[] potentials = (String[])
                    potentialHosts.toArray(new String[0]);
                SharedMember[] memList = new SharedMember[memSize];

                TraceUtil.trace3("member list size is " +
                    Integer.toString(memSize));
                int j = 0;
                for (int i = 0; i < allMemList.length; i++) {
                    if (allMemList[i] != null) {
                        String tempName = allMemList[i].getHostName();
                        memList[j] = allMemList[i];
                        TraceUtil.trace2("member list host name is " +
                            tempName);
                        String[] temp = allMemList[i].getIPs();
                        for (int ii = 0; ii < temp.length; ii++) {
                            TraceUtil.trace2("member list ip is " + temp[ii]);
                        }
                        j = j + 1;
                    } else {
                        TraceUtil.trace2("member list host name is null");

                    }
                }

                TraceUtil.trace2("before CreateSharedFileSystem with name: " +
                    fsName);

                FSArchCfg archiveConfig = null;
                TraceUtil.trace1("Create 4.6+ Shared File System");
                if (archivingEnabled) {
                    archiveConfig = getArchiveConfig();
                }
                FileSystem fs = fsManager.createSharedFileSystem(
                    fsName,
                    mountPoint,
                    intDAU,
                    memList,
                    properties,
                    (fsType.equals(FSTYPE_SHAREDFS) ?
                        new DiskCache[0] : metadataDevices),
                    dataDevices,
                    stripedGroups,
                    boolSingleAllocation,
                    boolMountAtBoot,
                    boolCreateMountPoint,
                    boolMountAfterCreate,
                    archiveConfig,
                    hpcEnabled);
                memSize = 0;

            } else if (isHAFS(wizardModel)) {
                FileSystem fs = sysModel.getSamQFSSystemFSManager()
                    .createHAFileSystem(nodeList,
                                        fsName,
                                        intFSType,
                                        intEquipOrdinal,
                                        mountPoint,
                                        intDAU,
                                        properties,
                                        metadataDevices,
                                        dataDevices,
                                        stripedGroups,
                                        boolSingleAllocation,
                                        boolCreateMountPoint,
                                        boolMountAfterCreate);
            } else {
                TraceUtil.trace1("Create 4.6+ File System");
                FSArchCfg archiveConfig = null;
                if (archivingEnabled) {
                    archiveConfig = getArchiveConfig();
                }
                FileSystem fs = sysModel.getSamQFSSystemFSManager().
                    createFileSystem(fsName,
                                     intFSType,
                                     intArchType,
                                     intEquipOrdinal,
                                     mountPoint,
                                     intShareStatus,
                                     intDAU,
                                     properties,
                                     metadataDevices,
                                     dataDevices,
                                     stripedGroups,
                                     boolSingleAllocation,
                                     boolMountAtBoot,
                                     boolCreateMountPoint,
                                     boolMountAfterCreate,
                                     archiveConfig,
                                     matfsEnabled);
            }

            LogUtil.info(
                this.getClass(),
                "finishStep",
                "Done creating new FS " + fsName);

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "FSSummary.createfs", fsName));

            TraceUtil.trace2("Succesfully created FS with name: " + fsName);
            return true;
        } catch (SamFSMultiHostException e) {
            SamUtil.doPrint(new NonSyncStringBuffer().append("error code is ").
                  append(e.getSAMerrno()).toString());

            String errMsg = SamUtil.handleMultiHostException(e);
            if (errMsg == null) {
                TraceUtil.trace2("err is null");
                errMsg = "";
            } else {
                TraceUtil.trace2("err is " + errMsg);
            }
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                errMsg);
            int errCode = e.getSAMerrno();
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            return true;

        } catch (SamFSMultiStepOpException msex) {
            processMultiStepException(msex);
            return true;
        } catch (SamFSTimeoutException toex) {
            TraceUtil.trace2("processing samfs time out exception...");
            // RPC timed out
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_WARNING);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.warning.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceStringError("-2801"));
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(toex.getSAMerrno()));

            String processExMessage =
                "Timed out when creating " + fsName + ": "  + toex.getMessage();
            SamUtil.processException(
                toex,
                this.getClass(),
                "finishStep()",
                processExMessage,
                serverName);

            return true;
        } catch (SamFSException ex) {
            boolean multiMsgOccurred = false;
            boolean warningOccurred = false;
            String processMsg = null;

            // if there is a problem with the archiver.cmd configuration
            // file, we won't mount or create a directory, etc. either
            // If this is not done and with the way the error-handling works
            // errors can be hidden.  If there is a problem creating a
            // directory and a problem with the configuration, it would
            // have to be decided which to display

            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                multiMsgOccurred = true;
            } else if (ex instanceof SamFSWarnings) {
                warningOccurred = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
            } else {
                processMsg = "Failed to create FS" + fsName;
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                processMsg,
                serverName);

            int errCode = ex.getSAMerrno();
            if (multiMsgOccurred) {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchiveConfig.error");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "ArchiveConfig.error.detail");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            } else if (warningOccurred) {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_WARNING);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchiveConfig.error");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "ArchiveConfig.warning.detail");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            } else {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "FSWizard.new.error.summary");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    ex.getMessage());
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            }

            return true;
        } catch (Exception e) {
            e.printStackTrace();
            e.printStackTrace(System.out);
            return true;
        }

    }

    public boolean cancelStep(WizardEvent wizardEvent) {
        super.cancelStep(wizardEvent);

        // clear previous error
        wizardModel.setValue(
            Constants.Wizard.WIZARD_ERROR,
            Constants.Wizard.WIZARD_ERROR_NO);
        wizardModel.clear();

	return true;
    }

    public void closeStep(WizardEvent wizardEvent) {
        TraceUtil.trace2("Clearing out wizard model...");
        wizardModel.clear();
        TraceUtil.trace2("Done!");
    }

    // initialize wizard data members
    private void initializeWizard(RequestContext requestContext) {
        TraceUtil.trace2("Initializing wizard...");
        wizardName  = CreateFSWizardImplData.name;
        wizardTitle = CreateFSWizardImplData.title;
        pageClass   = CreateFSWizardImplData.pageClass;

        pageTitle = CreateFSWizardImplData.pageTitle;
        stepHelp  = CreateFSWizardImplData.stepHelp;
        stepText  = CreateFSWizardImplData.stepText;
        stepInstruction = CreateFSWizardImplData.stepInstruction;

        serverName = requestContext.getRequest().getParameter(
            Constants.Parameters.SERVER_NAME);
        TraceUtil.trace2("serverNameParam = " + serverName);

        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        Hashtable serverTable = (Hashtable) session.getAttribute(
            Constants.SessionAttributes.SAMFS_SERVER_INFO);

        // Leave this version & license code here just in case if we need to
        // do backward compatibility checking in the future
        if (serverTable != null && serverName != null) {
            ServerInfo serverInfo = (ServerInfo) serverTable.get(serverName);
            if (serverInfo != null) {
                // get samfs server api version number and license type
                samfsServerAPIVersion = serverInfo.getSamfsServerAPIVersion();
                TraceUtil.trace2("samfs version = " + samfsServerAPIVersion);
                fsLicense = serverInfo.getServerLicenseType();
            } // else defaults to initialized value 1.1 and samqfs
        }

        TraceUtil.trace2("samfsServerAPIVersion = " + samfsServerAPIVersion);
        TraceUtil.trace2("fsLicense = " + fsLicense);

        // store serverName, server api version number and license type
        // in wizard model so that it can be accessed in all wizard pages
        wizardModel.setValue(
            Constants.Wizard.SERVER_NAME, serverName);
        wizardModel.setValue(
            Constants.Wizard.SERVER_API_VERSION, samfsServerAPIVersion);
        wizardModel.setValue(
            Constants.Wizard.LICENSE_TYPE, new Short(fsLicense));

        // default the pages wizard pages to qfs
        pages = CreateFSWizardImplData.qfs_pages;

        setShowResultsPage(true);
        initializeWizardPages(pages);
        striped_group_base = getStripedGroupBase();

        TraceUtil.trace2("wizard initialized!");
    }

    private void initializeMetadataStorage(boolean hafs,
                                           boolean hpc,
                                           boolean matfs) {
        // default values for metadata storage
        if (hpc || hafs || matfs) {
            // metadata and data in separate devices
            wizardModel.setValue(NewWizardMetadataOptionsView.METADATA_STORAGE,
                                 NewWizardMetadataOptionsView.SEPARATE_DEVICE);
        } else {
            // metadata and data in the same device
            wizardModel.setValue(NewWizardMetadataOptionsView.METADATA_STORAGE,
                                 NewWizardMetadataOptionsView.SAME_DEVICE);
        }
    }

    private void initializeBlockAllocation() {
        String mdStorage = (String)
	    wizardModel.getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
        if (NewWizardMetadataOptionsView.SAME_DEVICE.equals(mdStorage)) {
            // default to single allocation
            wizardModel.setValue(NewWizardBlockAllocationView.ALLOCATION_METHOD,
				 NewWizardBlockAllocationView.SINGLE);
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE,
				 "64");
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE_UNIT,
				 "1");
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCKS_PER_DEVICE,
				 "2");
        } else {
            // default to dual allocation
            wizardModel.setValue(NewWizardBlockAllocationView.ALLOCATION_METHOD,
				 NewWizardBlockAllocationView.DUAL);
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE,
				 "64");
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE_UNIT,
				 "1");
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCKS_PER_DEVICE,
				 "2");
        }
    }

    /**
     * Utility method to set the selected values into the properties
     */
    private FileSystemMountProperties setSelectedMountProperties(
        FileSystemMountProperties properties) {

        if (intHWM != -1) {
            if (properties.getHWM() != intHWM) {
                properties.setHWM(intHWM);
            }
        } else {
            properties.setHWM(intHWM);
        }

        if (intLWM != -1) {
            if (properties.getLWM() != intLWM) {
                properties.setLWM(intLWM);
            }
        } else {
            properties.setLWM(intLWM);
        }

        if (intStripe != -1) {
            if (properties.getStripeWidth() != intStripe) {
                properties.setStripeWidth(intStripe);
            }
        } else {
            properties.setStripeWidth(intStripe);
        }

        boolean boolTrace = false;
        String traceString = (String) wizardModel.getValue(
            NewWizardMountView.CHILD_TRACE_DROPDOWN);
        if (traceString.equals("samqfsui.yes")) {
            boolTrace = true;
        }

        if (properties.isTrace() != boolTrace) {
            properties.setTrace(boolTrace);
        }

        // Optimize for Oracle File Systems, enable qwrite and forced-direct-i/o
        String optimizeFS = (String) wizardModel.getValue(
            NewWizardMountView.CHILD_OPTIMIZE_CHECKBOX);
        if (optimizeFS != null && optimizeFS.equals("samqfsui.yes")) {
            properties.setQuickWrite(true);
            properties.setForceDirectIO(true);
            if (sharedEnabled) {
                // set stripe width to 2 if its not set or its set to the
                // default value which is 0 for shared separate md & d
                if (intStripe == 0) {
                    properties.setStripeWidth(2);
                }

                // set values optimized for oracle.
                properties.setMultiHostWrite(true);
                properties.setReadLeaseDuration(300);
                properties.setWriteLeaseDuration(300);
                properties.setAppendLeaseDuration(300);
            }
        }

        return properties;
    }

    /**
     * process the File System Type Page
     */
    private boolean processFSTypePage(WizardEvent evt) {
        TraceUtil.trace2("Entering");

        String fsType = (String)wizardModel.getValue(NewWizardFSTypeView.QFS);

        // UFS file system
        if (FSTYPE_UFS.equals(fsType)) {
            this.fsType = FSTYPE_UFS;
            wizardModel.setValue(FSTYPE_KEY, FSTYPE_UFS);
            pages = CreateFSWizardImplData.ufs_pages;
        } else if (FSTYPE_QFS.equals(fsType)) {
            // standard qfs
            this.fsType = FSTYPE_QFS;
            wizardModel.setValue(FSTYPE_KEY, FSTYPE_QFS);
            this.pages = CreateFSWizardImplData.qfs_pages;

            // shared
            String temp = (String)
                wizardModel.getValue(NewWizardFSTypeView.SHARED);
            this.sharedEnabled = new Boolean(temp);
            wizardModel.setValue(POPUP_SHARED, this.sharedEnabled);
            if (this.sharedEnabled) {
                pages = WizardUtil.insertPagesBefore(this.pages,
                       new int [] {CreateFSWizardImplData.PAGE_SHARED_MEMBER},
                                            CreateFSWizardImplData.PAGE_MOUNT,
                                                     false);
            }

            // archiving
            temp = (String)wizardModel.getValue(NewWizardFSTypeView.ARCHIVING);
            this.archivingEnabled = new Boolean(temp);
            wizardModel.setValue(POPUP_ARCHIVING, this.archivingEnabled);
            if (this.archivingEnabled) {
                pages = WizardUtil.insertPagesBefore(this.pages,
                      new int [] {CreateFSWizardImplData.PAGE_ARCHIVE_CONFIG},
                                       CreateFSWizardImplData.PAGE_FS_SUMMARY,
                                                     true);
            }

            // hpc (mb-fs)
            temp = (String)wizardModel.getValue(NewWizardFSTypeView.HPC);
            this.hpcEnabled = new Boolean(temp);
            wizardModel.setValue(POPUP_HPC, this.hpcEnabled);

            // ha-fs
            temp = (String)wizardModel.getValue(NewWizardFSTypeView.HAFS);
            this.hafsEnabled = new Boolean(temp);
            wizardModel.setValue(POPUP_HAFS, this.hafsEnabled);
            if (this.hafsEnabled) {
                pages = WizardUtil.insertPagesBefore(this.pages,
                      new int [] {CreateFSWizardImplData.PAGE_CLUSTER_NODES},
                                           CreateFSWizardImplData.PAGE_MOUNT,
                                                     false);
            }

            // mat-fs
            temp = (String)wizardModel.getValue(NewWizardFSTypeView.MATFS);
            this.matfsEnabled = new Boolean(temp);
            wizardModel.setValue(POPUP_MATFS, this.matfsEnabled);

            // initialize the pages
            initializeMetadataStorage(this.hafsEnabled,
                                      this.hpcEnabled,
                                      this.matfsEnabled);
            initializeBlockAllocation();
        }

        initializeWizardPages(this.pages);
        this.striped_group_base = getStripedGroupBase();

        TraceUtil.trace2("Exiting");
        return true;
    }

    /**
     * private method to process the qfs defaults page
     */
    private boolean processAcceptQFSDefaultsPage(WizardEvent evt) {
        String defaults = (String)
            wizardModel.getValue(NewWizardAcceptQFSDefaultsView.ACCEPT_CHANGE);

        // 1.  if hafs or hpc or is a matfs set metadata options to separate
        // otherwise, set the same
        if (hafsEnabled || hpcEnabled || matfsEnabled) {
            // default to data and metadata on separate devices
            wizardModel.setValue(NewWizardMetadataOptionsView.METADATA_STORAGE,
                                 NewWizardMetadataOptionsView.SEPARATE_DEVICE);
        } else {
            // default to data and metadata on same device
            wizardModel.setValue(NewWizardMetadataOptionsView.METADATA_STORAGE,
                                 NewWizardMetadataOptionsView.SAME_DEVICE);
        }

        // if accepting
        if (NewWizardAcceptQFSDefaultsView.ACCEPT.equals(defaults)) {
            if (hpcEnabled || matfsEnabled) { // must pass through the data
                                              // allocation page
                this.pages = WizardUtil.insertPagesBefore(this.pages,
                    new int [] {CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION},
                                  CreateFSWizardImplData.PAGE_QFS_DEFAULTS,
                                                         false); // after
            }

            // 2. update the block allocation default values
            processMetadataOptionsPage(evt);

        } else {
            // 1. update pages to insert metadata options & block allocation

            int [] pagesToInsert =
                new int [] {CreateFSWizardImplData.PAGE_METADATA_OPTIONS,
                            CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION};

            if (hafsEnabled || hpcEnabled || matfsEnabled) {
                pagesToInsert =
                    new int [] {CreateFSWizardImplData.PAGE_BLOCK_ALLOCATION};
            }

            this.pages = WizardUtil.insertPagesBefore(this.pages,
                                                      pagesToInsert,
                                      CreateFSWizardImplData.PAGE_QFS_DEFAULTS,
                                                      false);
            if (hpcEnabled || matfsEnabled)
                processMetadataOptionsPage(evt); // this will insert the
                                                 // metadata device selection
                                                 // page to the list
            initializeWizardPages(this.pages);
            striped_group_base = getStripedGroupBase();
        }

        return true;
    }

    /**
     * process the Metadata options page
     */
    private boolean processMetadataOptionsPage(WizardEvent evt) {
        // if separate data and metadata
        String mdStorage = (String)wizardModel
            .getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
        if (NewWizardMetadataOptionsView.SEPARATE_DEVICE.equals(mdStorage)) {
            this.pages = WizardUtil.insertPagesBefore(this.pages,
                  new int [] {CreateFSWizardImplData.PAGE_METADATA_LUN},
                                   CreateFSWizardImplData.PAGE_DATA_LUN,
                                                      true);

            initializeWizardPages(this.pages);
        }

        populateDefaultBlockAllocation();
        striped_group_base = getStripedGroupBase();

        return true;
    }

    /**
     * validate the block allocation page
     */
    private boolean processBlockAllocationPage(WizardEvent evt) {
        // validate blocks per device
        String stripeStr = (String)wizardModel
            .getValue(NewWizardBlockAllocationView.BLOCKS_PER_DEVICE);
        stripeStr = stripeStr != null ? stripeStr.trim() : "";

        if (stripeStr.length() == 0) {
            // blocks per device is blank
            setWizardAlert(evt, "FSWizard.new.error.stripeValue");
            return false;
        }

        boolean valid = true;
        int blockSizeInKB = 16;
        try {
            int x = Integer.parseInt(stripeStr);
            valid = ((x >= 0) && (x <= 255));
        } catch (NumberFormatException nfe) {
            valid = false;
        } finally {
            if (!valid) {
                setWizardAlert(evt, "FSWizard.new.error.stripeValueRange");
                return false;
            }
        }


        // allocation method
        String method = (String)wizardModel
            .getValue(NewWizardBlockAllocationView.ALLOCATION_METHOD);

        // if using striped groups validate block size and number of striped
        // groups
        if (NewWizardBlockAllocationView.STRIPED.equals(method) || hpcEnabled) {
            // validate block size
            String blockStr = (String)wizardModel
                .getValue(NewWizardBlockAllocationView.BLOCK_SIZE);
            blockStr = blockStr != null ? blockStr.trim() : "";
            if (blockStr.length() == 0) {
                setWizardAlert(evt, "FSWizard.new.error.dauSize");
                return false;
            }

            valid = true;
            try {
                int x = Integer.parseInt(blockStr);
                String unit = (String)wizardModel
                    .getValue(NewWizardBlockAllocationView.BLOCK_SIZE_UNIT);
                if ("2".equals(unit)) { // MB convert to KB
                    x = x * 1024;
                }

                // block size has to be greater than or equal 16KB, less than
                // 64MB, and a multiple of 8KB
                valid = ((x >= 16) && (x <= (64 *1024)) && ((x % 8) == 0));

                // set the block size value in KB
                blockSizeInKB = x;
            } catch (NumberFormatException nfe) {
                valid = false;
            } finally {
                if (!valid) {
                    setWizardAlert(evt, "FSWizard.new.error.dauSize");
                    return false;
                }
            }

            // validate number of striped groups
            String stripes = (String)wizardModel
                .getValue(NewWizardBlockAllocationView.STRIPED_GROUPS);

            // if hpc is enabled, this value is in the object groups field.
            // NOTE: Object groups behave in exactly the same way as striped
            // groups
            if (hpcEnabled) {
                stripes = (String)wizardModel
                .getValue(NewWizardBlockAllocationView.OBJECT_GROUPS);
            }

            int stripedGroups = 0;
            stripes = stripes != null ? stripes.trim() : "";
            if (stripes.length() == 0) {
                setWizardAlert(evt, hpcEnabled ?
                               "FSWizard.new.error.numObjectGroup" :
                               "FSWizard.new.error.numStripedGroup");
                return false;
            }

            valid = true;
            try {
                stripedGroups = Integer.parseInt(stripes);
                valid = (stripedGroups >= 1) && (stripedGroups <= 128);
            } catch (NumberFormatException nfe) {
                valid = false;
            } finally {
                if (!valid) {
                    setWizardAlert(evt, hpcEnabled ?
                                   "FSWizard.new.error.numObjectGroup" :
                                   "FSWizard.new.error.numStripedGroup");
                    return false;
                }
            }

            // insert striped group pages and remove PAGE_DATA_LUN
            int [] stripedGroupPages = new int[stripedGroups];
            for (int i = 0; i < stripedGroups; i++) {
                stripedGroupPages[i] =
                    CreateFSWizardImplData.PAGE_STRIPED_GROUP;
            }
            this.pages = WizardUtil.insertPagesBefore(this.pages,
                                                      stripedGroupPages,
                                          CreateFSWizardImplData.PAGE_DATA_LUN,
                                                      true);
            this.pages =
                WizardUtil.removePage(CreateFSWizardImplData.PAGE_DATA_LUN,
                                      this.pages);

        } else { // retrieve the block size for all other allocation methods
            String blockSizeStr = (String)wizardModel
                .getValue(NewWizardBlockAllocationView.BLOCK_SIZE);
            blockSizeStr = blockSizeStr != null ? blockSizeStr.trim() : "";
            try {
                blockSizeInKB = Integer.parseInt(blockSizeStr);
            } catch (NumberFormatException nfe) {
                // we shouldn't get here since these values come from a
                // dropdown menu
            }
        }

        wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE_KB,
                             new Integer(blockSizeInKB));

        initializeWizardPages(this.pages);
        striped_group_base = getStripedGroupBase();

        // page is valid
        return true;
    }

    /**
     * Private method to process the shared member page
     */
    private boolean processSharedMemberPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");

        // Get a handle to the View
        FSWizardSharedMemberSelectionPageView view =
            (FSWizardSharedMemberSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardSharedMemberSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1(
                "Exception while restoreStateData: " + ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }
        FSWizardSharedMemberSelectionPageModel memberModel =
            (FSWizardSharedMemberSelectionPageModel)
            dataTable.getModel();
        int tableSize = memberModel.getNumRows();

        String clientList = null;
        String potentialServerList = null;
        if (selectedClientIndex == null) {
            selectedClientIndex = new ArrayList();
        }
        selectedClientIndex.clear();

        if (selectedPotentialMetadataServerIndex == null) {
            selectedPotentialMetadataServerIndex = new ArrayList();
        }
        selectedPotentialMetadataServerIndex.clear();

        if (selectedPrimaryIPIndex == null) {
            selectedPrimaryIPIndex = new ArrayList();
        }
        selectedPrimaryIPIndex.clear();

        if (selectedSecondaryIPIndex == null) {
            selectedSecondaryIPIndex = new ArrayList();
        }
        selectedSecondaryIPIndex.clear();

        if (selectedClient == null) {
            selectedClient = new ArrayList();
        }
        selectedClient.clear();

        if (selectedPotentialMetadataServer == null) {
            selectedPotentialMetadataServer = new ArrayList();
        }
        selectedPotentialMetadataServer.clear();
        allMemList = null;

        allMemList = new SharedMember[tableSize + 1];
        memSize = 0;

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            for (int i = 0; i < tableSize; i++) {
                view.tableModel.setRowIndex(i);
                String client = (String)
                    view.tableModel.getValue("clientTextField");
                String potentialServer = (String)
                    view.tableModel.
                        getValue("potentialMetadataServerTextField");
                String hostName = (String)
                    view.tableModel.getValue("HiddenHostName");
                String primaryIP = (String)
                    view.tableModel.getValue("primaryIPTextField");
                String secondaryIP = (String)
                    view.tableModel.getValue("secondaryIPTextField");

                TraceUtil.trace3("PRIMARY IP = " +primaryIP);
                TraceUtil.trace3("SECONDARY IP = " +secondaryIP);

                if (client.equals("false") &&
                    potentialServer.equals("false")) {
                    selectedPrimaryIPIndex.add(null);
                    selectedSecondaryIPIndex.add(null);
                } else {
                    selectedPrimaryIPIndex.add(primaryIP);
                    selectedSecondaryIPIndex.add(secondaryIP);
                }

                // Get the cluster Name of the metadata server if metadata
                // server is in a cluster
                boolean isMetadataServerACluster =
                    SamUtil.getModel(serverName).isClusterNode();
                String clusterName =
                    isMetadataServerACluster ?
                        SamUtil.getModel(serverName).getClusterName() :
                        null;
                if (potentialServer != null && potentialServer.equals("true")) {
                    TraceUtil.trace3("Process potential metadata server");
                    try {
                        SamQFSSystemModel model = SamUtil.getModel(hostName);
                        boolean thisHostIsACluster = model.isClusterNode();

                        // If metadata is in a cluster, the PMDS has to be a
                        // cluster and it has to be in the same cluster as the
                        // metadata server
                        if (clusterName != null &&
                            (!thisHostIsACluster ||
                            !model.getClusterName().equals(clusterName))) {
                            // false situation
                            String message =
                                SamUtil.getResourceStringWithoutL10NArgs(
                                    "FSWizard.new.error.invalidclustersetup",
                                    new String [] {
                                        hostName,
                                        clusterName});
                            if (message.charAt(message.length() - 1) == '\n') {
                                message =
                                    message.substring(0, message.length() - 1);
                            }
                            setWizardAlert(wizardEvent, message);
                            return false;
                        }
                        String fsName = ((String) wizardModel.getValue(
                            NewWizardMountView.CHILD_FSNAME_FIELD)).trim();

                        FileSystem fileSystem =
                            model.getSamQFSSystemFSManager().
                                getFileSystem(fsName);
                        if (fileSystem != null) {
                            String info[] = new String[2];
                            info[0] = fsName;
                            info[1] = hostName;
                            setWizardAlert(wizardEvent,
                                SamUtil.getResourceString(
                                "FSWizard.new.error.duplicateFSName", info));
                            return false;
                        }
                    } catch (SamFSException ex) {
                        SamUtil.processException(
                            ex,
                            this.getClass(),
                            "processSharedMember()",
                            "Failed to get filesystem",
                            serverName);
                        setWizardAlert(wizardEvent,
                            ex.getLocalizedMessage());
                        return false;
                    }
                    if (primaryIP.equals(secondaryIP)) {
                        setWizardAlert(
                            wizardEvent, "FSWizard.new.error.sameIP");
                        return false;
                    }
                    String[] potMetaIP = null;
                    if (secondaryIP == null || secondaryIP.equals("---")) {
                        potMetaIP = new String[1];
                        potMetaIP[0] = primaryIP;
                    } else {
                        potMetaIP = new String[2];
                        potMetaIP[0] = primaryIP;
                        potMetaIP[1] = secondaryIP;
                    }
                    allMemList[i] = fsManager.createSharedMember(hostName,
                        potMetaIP, SharedMember.TYPE_POTENTIAL_MD_SERVER);
                    memSize = memSize + 1;
                    TraceUtil.trace3("created memberlist for " +
                         Integer.toString(i));

                    selectedPotentialMetadataServerIndex.
                        add(Integer.toString(i));
                    selectedPotentialMetadataServer.add(hostName);
                    if (potentialServerList == null) {
                        potentialServerList = hostName + "(" + primaryIP +
                            "," + secondaryIP + ")";
                    } else {
                        potentialServerList = potentialServerList + ", " +
                            hostName +
                            "(" + primaryIP + "," + secondaryIP + ")";
                    }
                } else if (client != null && client.equals("true")) {
                    TraceUtil.trace3("Process metadata client");
                    try {
                        SamQFSSystemModel model = SamUtil.getModel(hostName);
                        String fsName = ((String) wizardModel.getValue(
                            NewWizardMountView.CHILD_FSNAME_FIELD)).trim();
                        FileSystem fileSystem =
                            model.getSamQFSSystemFSManager().
                                getFileSystem(fsName);
                        if (fileSystem != null) {
                            String info[] = new String[2];
                            info[0] = fsName;
                            info[1] = hostName;
                            setWizardAlert(wizardEvent,
                                SamUtil.getResourceString(
                                "FSWizard.new.error.duplicateFSName", info));
                            return false;
                        }
                    } catch (SamFSException ex) {
                        SamUtil.processException(
                            ex,
                            this.getClass(),
                            "processSharedMember()",
                            "Failed to get filesystem",
                             serverName);
                        setWizardAlert(
                            wizardEvent,
                            ex.getLocalizedMessage());
                            return false;
                    }
                    if (primaryIP.equals(secondaryIP)) {
                        setWizardAlert(wizardEvent,
                            "FSWizard.new.error.sameIP");
                        return false;
                    }
                    String[] clientIP = null;
                    if (secondaryIP == null || secondaryIP.equals("---")) {
                        clientIP = new String[1];
                        clientIP[0] = primaryIP;
                    } else {
                        clientIP = new String[2];
                        clientIP[0] = primaryIP;
                        clientIP[1] = secondaryIP;
                    }
                    allMemList[i] = fsManager.createSharedMember(hostName,
                        clientIP, SharedMember.TYPE_CLIENT);
                    memSize = memSize + 1;
                    TraceUtil.trace3("created memberlist for " +
                        Integer.toString(i));

                    selectedClientIndex.add(Integer.toString(i));
                    selectedClient.add(hostName);
                    if (clientList == null) {
                        clientList = hostName + "(" + primaryIP +
                            "," + secondaryIP + ")";
                    } else {
                        clientList = clientList + ", " + hostName +
                            "(" + primaryIP + "," + secondaryIP + ")";
                    }
                }
            } // end for loop
            String ipAddr = (String) wizardModel.getValue(
                FSWizardSharedMemberSelectionPageView.CHILD_SERVERIP);
            String secipAddr = (String) wizardModel.getValue(
                FSWizardSharedMemberSelectionPageView.CHILD_SECSERVERIP);
            if (ipAddr != null && ipAddr.equals(secipAddr)) {
                setWizardAlert(wizardEvent, "FSWizard.new.error.sameIP");
                return false;
            }
            String[] metaIP = null;
            if (secipAddr == null || secipAddr.equals("---")) {
                metaIP = new String[1];
                metaIP[0] = ipAddr;
            } else {
                metaIP = new String[2];
                metaIP[0] = ipAddr;
                metaIP[1] = secipAddr;
            }
            if (memSize >= 5) {
                setWizardAlert(wizardEvent, "FSWizard.new.error.largerThan4");
                return false;
            }

            allMemList[tableSize] =
                fsManager.createSharedMember(serverName,
                metaIP, SharedMember.TYPE_MD_SERVER);
            memSize = memSize + 1;
            TraceUtil.trace3("membersize " + memSize);
            wizardModel.setValue(
                NewWizardQFSSummaryView.CHILD_PRIMARYIP, ipAddr);
            wizardModel.setValue(
                NewWizardQFSSummaryView.CHILD_SECONDARYIP, secipAddr);

        } catch (SamFSMultiHostException e) {
            SamUtil.doPrint(new NonSyncStringBuffer().append("error code is ").
                  append(e.getSAMerrno()).toString());
            String errMsg = SamUtil.handleMultiHostException(e);
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                errMsg);
            int errCode = e.getSAMerrno();
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            return false;
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "processSharedMember()",
                "Failed to get filesystem",
                serverName);
            int errCode = ex.getSAMerrno();
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "ArchiveConfig.error");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                "ArchiveConfig.error.detail");
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            return false;
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_MEMLIST,
            allMemList);
        wizardModel.setValue(
            Constants.Wizard.SELECTED_PRIMARYIP_INDEX,
            selectedPrimaryIPIndex);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_SECONDARYIP_INDEX,
            selectedSecondaryIPIndex);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_CLIENT,
            selectedClient);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_VALUE,
            selectedPotentialMetadataServer);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_CLIENT_INDEX,
            selectedClientIndex);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_INDEX,
            selectedPotentialMetadataServerIndex);
        wizardModel.setValue(
            NewWizardQFSSummaryView.CHILD_POTENTIAL_SERVER,
            potentialServerList);

        wizardModel.setValue(
            NewWizardQFSSummaryView.CHILD_CLIENT, clientList);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER,
            potentialServerList);

        wizardModel.setValue(
            Constants.Wizard.SELECTED_SHARED_CLIENT,
            clientList);

        return true;
    }

    /**
     * Private method to process the Data Devices page
     */
    private boolean processDataDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");
        TraceUtil.trace2("Entered processDataDevicesPage.., ");
        boolean exceed = false;

        // Get a handle to the View
        FSWizardDeviceSelectionPageView view =
            (FSWizardDeviceSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardDeviceSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1(
                "Exception while restoreStateData: " + ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }

        CCActionTableModel dataModel = (CCActionTableModel)dataTable.getModel();
        Integer [] selectedRows = dataModel.getSelectedRows();
        // If the user did not select any devices, show error
        if (selectedRows.length < 1) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.data");
            return false;
        } else {
            if (fsType.equals(FSTYPE_FS)) {
                if (selectedRows.length > Constants.Wizard.MAX_LUNS) {
                    exceed = true;
                }
            } else if (fsType.equals(FSTYPE_QFS)) {
                // TODO::
                int metaLuns = selectedMetaDevicesList == null
                    ? 0 : selectedMetaDevicesList.size();
                int usedLuns = metaLuns + selectedRows.length;
                if (usedLuns > Constants.Wizard.MAX_LUNS) {
                    exceed = true;
                }
            }
        }
        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);
        // Traverse through the selection and populate selectedDataDevicesList
        String deviceSelected = null;
        DiskCache device = null;
        if (selectedDataDevicesList == null) {
            selectedDataDevicesList = new ArrayList();
        }
        selectedDataDevicesList.clear();

        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            selectedDataDevicesList.add(deviceSelected);
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_DATADEVICES,
            selectedDataDevicesList);

        // display the error for exceed max luns
        if (exceed) {
            setWizardAlert(wizardEvent, "FSWizard.maxlun");
            return false;
        }
        // Set the value of the selection to display in the summary page
        Collections.sort(selectedDataDevicesList);

        // Generate an array of strings to check for overlapping luns
        int size = selectedDataDevicesList.size();
        String[] dataLUNs = new String[size];
        for (int i = 0; i < size; i++) {
            dataLUNs[i] = (String) selectedDataDevicesList.get(i);
        }

        SamQFSSystemModel sysModel = null;
        String[] overlapLUNs = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            overlapLUNs = sysModel.getSamQFSSystemFSManager().
                              checkSlicesForOverlaps(dataLUNs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed to check data LUNs for overlaps",
                serverName);

            wizardModel.setValue(
                Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_FAILED);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_MESSAGE, ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // if found overlapped LUNs, generate appropriate error message
        if (overlapLUNs != null && overlapLUNs.length > 0) {
            StringBuffer badLUNs =
                new StringBuffer(SamUtil.getResourceString(
                "FSWizard.error.overlapDataLUNs"));
            badLUNs.append("<br>");
            for (int i = 0; i < overlapLUNs.length; i++) {
                badLUNs.append(overlapLUNs[i]).append("<br>");
            }

            TraceUtil.trace2(badLUNs.toString());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, badLUNs.toString());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
            wizardModel.setValue(
                Constants.Wizard.ERROR_DETAIL,
                Constants.Wizard.ERROR_INLINE_ALERT);
            wizardModel.setValue(
                Constants.Wizard.ERROR_SUMMARY,
                "FSWizard.error.deviceError");
            return false;
        }

        StringBuffer deviceList = new StringBuffer();
        for (int i = 0; i < selectedDataDevicesList.size(); i++) {
            deviceList.append((String) selectedDataDevicesList.get(i)).
                append("<br>");
        }
        wizardModel.setValue(
            NewWizardFSSummaryView.CHILD_DATA_FIELD,
            deviceList.toString());

        return true;
    }

    /**
     * Private method to process the Striped Group Devices page
     */
    private boolean processStripedGroupPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");
        TraceUtil.trace2("Entered processStripedGroupPage.., ");
        boolean exceed = false;

        // Get a handle to the View
        FSWizardDeviceSelectionPageView view =
            (FSWizardDeviceSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardDeviceSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectDataDeviceStep()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1(
                "Exception while restoreStateData: " + ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }

        CCActionTableModel dataModel = (CCActionTableModel)dataTable.getModel();
        Integer [] selectedRows = dataModel.getSelectedRows();
        int groupNum = ((Integer) wizardModel.getValue(
            Constants.Wizard.STRIPED_GROUP_NUM)).intValue();
        // If the user did not select any devices, show error
        // check if the selected devices exceed 252 limit
        if (selectedRows.length < 1) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.data");
            return false;
        } else {
            int metaLuns = selectedMetaDevicesList.size();
            int totalStripeLuns = 0;
            if (selectedStripedGroupDevicesList != null) {
                for (int j = 0; j < groupNum; j++) {
                    ArrayList groupDevices =
                        (ArrayList) selectedStripedGroupDevicesList.get(j);
                    totalStripeLuns += groupDevices.size();
                }
            }
            int usedLuns =  metaLuns + totalStripeLuns + selectedRows.length;
            if (usedLuns > Constants.Wizard.MAX_LUNS) {
                exceed = true;
            }
        }
        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);
        // Traverse through the selection and populate
        // selectedStripedGroupDevicesList

        String deviceSelected = null;
        DiskCache device = null;
        if (selectedStripedGroupDevicesList == null) {
            selectedStripedGroupDevicesList = new ArrayList();
        }

        ArrayList thisGroupDevices;
        if (selectedStripedGroupDevicesList.size() <= groupNum) {
            thisGroupDevices = new ArrayList();
            selectedStripedGroupDevicesList.add(groupNum, thisGroupDevices);
        } else {
            thisGroupDevices =
                (ArrayList) selectedStripedGroupDevicesList.get(groupNum);
        }
        thisGroupDevices.clear();

        // get a list of selected devices
        // also check if they all have the same size
        long groupDeviceSize = -1;
        boolean deviceError = false;
        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            DiskCache disk = getDiskCacheObject(deviceSelected);
            if (groupDeviceSize < 0) {
                groupDeviceSize = disk.getCapacity();
            }
            if (groupDeviceSize != disk.getCapacity()) {
                deviceError = true;
            }
            thisGroupDevices.add(deviceSelected);
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES,
            selectedStripedGroupDevicesList);

        // display exceed error
        if (exceed) {
            setWizardAlert(wizardEvent, "FSWizard.maxlun");
            return false;
        }

        // we process deviceError here because of the counter display problem
        if (deviceError) {
            wizardModel.setValue(
                    Constants.Wizard.WIZARD_ERROR,
                    Constants.Wizard.WIZARD_ERROR_YES);
                wizardModel.setValue(
                    Constants.Wizard.ERROR_MESSAGE,
                    "FSWizard.error.stripedGroup.deviceSizeError");
                wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
                wizardModel.setValue(
                    Constants.Wizard.ERROR_DETAIL,
                    Constants.Wizard.ERROR_INLINE_ALERT);
                wizardModel.setValue(
                    Constants.Wizard.ERROR_SUMMARY,
                    "FSWizard.error.deviceError");
                return false;
        }

        // Set the value of the selection to display in the summary page
        Collections.sort(thisGroupDevices);

        // The following code which checks for lun overlapping is introduced
        // since api 1.1 (samfs 4.2)
        // Generate an array of strings to check for overlapping luns
        int size = thisGroupDevices.size();
        String[] dataLUNs = new String[size];
        for (int i = 0; i < size; i++) {
            dataLUNs[i] = (String) thisGroupDevices.get(i);
        }

        SamQFSSystemModel sysModel = null;
        String[] overlapLUNs = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            if (sysModel == null) {
                throw new SamFSException(null, -2001);
            }

            overlapLUNs = sysModel.getSamQFSSystemFSManager().
                              checkSlicesForOverlaps(dataLUNs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectStripedGroupStep()",
                "Failed to check striped group for overlaps",
                serverName);

            wizardModel.setValue(
                Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_FAILED);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_MESSAGE, ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // if found overlapped LUNs, generate appropriate error message
        if (overlapLUNs != null && overlapLUNs.length > 0) {
            StringBuffer badLUNs = new StringBuffer(
                SamUtil.getResourceString(
                "FSWizard.error.overlapDataLUNs"));
            badLUNs.append("<br>");
            for (int i = 0; i < overlapLUNs.length; i++) {
                badLUNs.append(overlapLUNs[i]).append("<br>");
            }

            TraceUtil.trace2(badLUNs.toString());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, badLUNs.toString());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
            wizardModel.setValue(
                Constants.Wizard.ERROR_DETAIL,
                Constants.Wizard.ERROR_INLINE_ALERT);
            wizardModel.setValue(
                Constants.Wizard.ERROR_SUMMARY,
                "FSWizard.error.deviceError");
            return false;
        }

        StringBuffer deviceList = new StringBuffer();
        for (int j = 0; j < selectedStripedGroupDevicesList.size(); j++) {
            ArrayList groupDevices =
                (ArrayList) selectedStripedGroupDevicesList.get(j);
            deviceList.append(SamUtil.getResourceString(
                "FSWizard.new.stripedGroup.deviceListing",
                new String[] { Integer.toString(j) })).append("<br>");
            for (int i = 0; i < groupDevices.size(); i++) {
                deviceList.append("&nbsp;&nbsp;&nbsp;").
                    append((String) groupDevices.get(i)).
                    append("<br>");
            }
        }

        wizardModel.setValue(
            NewWizardQFSSummaryView.CHILD_DATA_FIELD,
            deviceList.toString());

        // clear out previous error
        wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_NO);
        return true;
    }

    /**
     * Private method to process the MetaData Devices page
     */
    private boolean processMetaDataDevicesPage(WizardEvent wizardEvent) {
        TraceUtil.trace3("Entered");
        TraceUtil.trace2("Entered processMetaDataDevicesPage.., ");
        boolean result = true;
        boolean exceed = false;

        // Get a handle to the View
        FSWizardDeviceSelectionPageView view =
            (FSWizardDeviceSelectionPageView)wizardEvent.getView();
        // retrieve the dataTable
        CCActionTable dataTable = (CCActionTable)view.getChild(
            FSWizardDeviceSelectionPageView.CHILD_ACTIONTABLE);

        // Call restoreStateData to map the user selections to the table.
        try {
            dataTable.restoreStateData();
        } catch (ModelControlException ex) {
            SamUtil.processException(ex, this.getClass(),
                "processMetaDataDevicesPage()",
                "Failed in restoring statedata",
                serverName);
            TraceUtil.trace1("Exception while restoreStateData: " +
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "8001234");
            return false;
        }

        CCActionTableModel dataModel = (CCActionTableModel)dataTable.getModel();
        Integer [] selectedRows = dataModel.getSelectedRows();
        TraceUtil.trace2(
            "Number of selected rows are : " + selectedRows.length);
        // If the user did not select any devices, show error
        if (selectedRows.length < 1) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.metadata");
            return false;
        } else if (selectedRows.length > Constants.Wizard.MAX_LUNS) {
            exceed = true;
        }

        // Traverse through the selection and populate selectedDataDevicesList
        String deviceSelected = null;
        DiskCache device = null;
        if (selectedMetaDevicesList == null) {
            selectedMetaDevicesList = new ArrayList();
        }
        selectedMetaDevicesList.clear();

        for (int i = 0; i < selectedRows.length; i++) {
            dataModel.setRowIndex(selectedRows[i].intValue());
            deviceSelected = (String)dataModel.getValue("HiddenDevicePath");
            selectedMetaDevicesList.add(deviceSelected);
        }

        wizardModel.setValue(
            Constants.Wizard.SELECTED_METADEVICES,
            selectedMetaDevicesList);

        // display error for exceed maxlun
        if (exceed) {
            setWizardAlert(wizardEvent, "FSWizard.maxlun");
            return false;
        }

        // Set the value of the selection to display in the summary page
        Collections.sort(selectedMetaDevicesList);

        // The following code which checks for lun overlapping is introduced
        // since api 1.1 (samfs 4.2)
        // Generate an array of strings to check for overlapping luns
        int size = selectedMetaDevicesList.size();
        String[] dataLUNs = new String[size];
        for (int i = 0; i < size; i++) {
            dataLUNs[i] = (String) selectedMetaDevicesList.get(i);
        }

        SamQFSSystemModel sysModel = null;
        String[] overlapLUNs = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            if (sysModel == null) {
                throw new SamFSException(null, -2001);
            }

            overlapLUNs = sysModel.getSamQFSSystemFSManager().
                              checkSlicesForOverlaps(dataLUNs);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSelectMetaDataDeviceStep()",
                "Failed to check metadata LUNs for overlaps",
                serverName);

            wizardModel.setValue(
                Constants.Wizard.FINISH_RESULT,
                Constants.Wizard.RESULT_FAILED);
            wizardModel.setValue(
                Constants.Wizard.DETAIL_MESSAGE, ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // if found overlapped LUNs, generate appropriate error message
        if (overlapLUNs != null && overlapLUNs.length > 0) {
            StringBuffer badLUNs = new StringBuffer(
                SamUtil.getResourceString(
                "FSWizard.error.overlapMetadataLUNs"));
            badLUNs.append("<br>");
            for (int i = 0; i < overlapLUNs.length; i++) {
                badLUNs.append(overlapLUNs[i]).append("<br>");
            }
            TraceUtil.trace2(badLUNs.toString());
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE, badLUNs.toString());
            wizardModel.setValue(Constants.Wizard.ERROR_CODE, "1007");
            wizardModel.setValue(
                Constants.Wizard.ERROR_DETAIL,
                Constants.Wizard.ERROR_INLINE_ALERT);
            wizardModel.setValue(
                Constants.Wizard.ERROR_SUMMARY,
                "FSWizard.error.deviceError");
            return false;
        }

        StringBuffer deviceList = new StringBuffer();
        for (int i = 0; i < selectedMetaDevicesList.size(); i++) {
            deviceList.append((String) selectedMetaDevicesList.get(i)).
                append("<br>");
        }

        wizardModel.setValue(
            NewWizardQFSSummaryView.CHILD_META_FIELD,
            deviceList.toString());

        return result;
    }

    /**
     * Private method to validate the data entered in the Std Mount Options page
     */
    private boolean processStdMountOptionsPage(WizardEvent wizardEvent) {
        String mountPoint = ((String) wizardModel.getWizardValue(
            NewWizardStdMountView.CHILD_MOUNT_FIELD)).trim();

        if (mountPoint == null || mountPoint.length() < 1) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.mountpoint");
            return false;
        } else {
            if (!SamUtil.isValidNonSpecialCharString(mountPoint)) {
                setWizardAlert(
                    wizardEvent, "FSWizard.new.error.invalidmountpoint");
                return false;
            // Check if the mount point given is absolute path or not
            } else if (!mountPoint.startsWith("/")) {
                setWizardAlert(
                    wizardEvent, "FSWizard.new.error.mountpoint.absolutePath");
                return false;
            }
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private boolean processMountOptionsPage(WizardEvent wizardEvent) {
        String mountPoint = ((String) wizardModel.getWizardValue(
            NewWizardMountView.CHILD_MOUNT_FIELD)).trim();

        // fsName is not used for ufs file systems
        boolean invalidFSName = false;
        String fsName = (String) wizardModel.getValue(
            NewWizardMountView.CHILD_FSNAME_FIELD);
        // Check if fsName is empty
        if (fsName == null || fsName.trim().length() <= 0) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.fsname");
            invalidFSName = true;
        } else {
            fsName = fsName.trim();
            // Check if fsName is valid
            if (fsName.indexOf("/") != -1 ||
                !SamUtil.isValidFSNameString(fsName)) {
                setWizardAlert(
                    wizardEvent, "FSWizard.new.error.invalidfsname");
                invalidFSName = true;
            }
        }

        // Check if the FileSystem already exists
        if (!invalidFSName && fileSystemExists(fsName)) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.fsnameExists");
            invalidFSName = true;
        }

        if (mountPoint == null || mountPoint.length() < 1) {
            setWizardAlert(wizardEvent, "FSWizard.new.error.mountpoint");
            return false;
        } else {
            if (!SamUtil.isValidNonSpecialCharString(mountPoint)) {
                setWizardAlert(
                    wizardEvent, "FSWizard.new.error.invalidmountpoint");
                return false;
            // Check if the mount point given is absolute path or not
            } else if (!mountPoint.startsWith("/")) {
                setWizardAlert(
                    wizardEvent, "FSWizard.new.error.mountpoint.absolutePath");
                return false;
            }
        }

        if (fsType.equals(FSTYPE_SHAREDQFS) ||
            fsLicense == SamQFSSystemModel.QFS) {
            // hwm and lwm not applicable
        } else {
            String hwmString = ((String) wizardModel.getWizardValue(
                NewWizardMountView.CHILD_HWM_FIELD)).trim();
            String lwmString = ((String) wizardModel.getWizardValue(
                NewWizardMountView.CHILD_LWM_FIELD)).trim();

            int defaultHWM = 80, defaultLWM = 60;

            // Get default HWM and LWM if necessary
            if (hwmString.length() <= 0 || lwmString.length() <= 0) {
                int intFSType   = FileSystem.COMBINED_METADATA;
                String t = (String)wizardModel
                    .getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
                if (NewWizardMetadataOptionsView.SEPARATE_DEVICE.equals(t)) {
                    intFSType = FileSystem.SEPARATE_METADATA;
                }

                int intArchType = FileSystem.ARCHIVING;
                if (!archivingEnabled) {
                    intArchType = FileSystem.NONARCHIVING;
                }

                int intShareStatus = FileSystem.UNSHARED;
                int intDAU = Integer.parseInt((String)wizardModel.getValue(
                    NewWizardBlockAllocationView.BLOCK_SIZE_KB));

                SamQFSSystemModel sysModel = null;
                FileSystemMountProperties properties = null;

                try {
                    sysModel = SamUtil.getModel(serverName);
                    properties = sysModel.getSamQFSSystemFSManager().
                                     getDefaultMountProperties(
                        intFSType,
                        intArchType,
                        intDAU,
                        false,
                        intShareStatus,
                        false);

                    defaultHWM = properties.getHWM();
                    defaultLWM = properties.getLWM();
                } catch (SamFSException ex) {
                    SamUtil.processException(
                        ex,
                        this.getClass(),
                        "processMountOptionsPage()",
                        "Failed to get Default Mount Properties",
                        serverName);

                    TraceUtil.trace1("Exception while processMountOptionsPage: "
                        + ex.getMessage());
                    wizardModel.setValue(
                        Constants.Wizard.WIZARD_ERROR,
                        Constants.Wizard.WIZARD_ERROR_YES);
                    wizardModel.setValue(
                        Constants.Wizard.ERROR_MESSAGE, ex.getMessage());
                    wizardModel.setValue(
                        Constants.Wizard.ERROR_CODE, "8001234");
                    return false;
                }
            }

            // Check the validity of HWM
            if (hwmString.length() > 0) {
                try {
                    intHWM = Integer.parseInt(hwmString);
                } catch (NumberFormatException e) {
                    setWizardAlert(wizardEvent, "FSWizard.new.error.hwmrange");
                    return false;
                }
                if (intHWM < 0 || intHWM > 100) {
                    setWizardAlert(wizardEvent, "FSWizard.new.error.hwmrange");
                    return false;
                }
            } else {
                intHWM = defaultHWM;
            }

            // Check the validity of LWM
            if (lwmString.length() > 0) {
                try {
                    intLWM = Integer.parseInt(lwmString);
                } catch (NumberFormatException e) {
                    setWizardAlert(wizardEvent, "FSWizard.new.error.lwmrange");
                    return false;
                }
                if (intLWM < 0 || intLWM > 100) {
                    setWizardAlert(wizardEvent, "FSWizard.new.error.lwmrange");
                    return false;
                }
            } else {
                intLWM = defaultLWM;
            }

            // Set error message based on which default value is being used
            if (intLWM >= intHWM) {
                if (hwmString.length() > 0) {
                    setWizardAlert(wizardEvent, "FSWizard.new.error.hwmblwm");
                } else {
                    setWizardAlert(wizardEvent, "FSWizard.new.error.lwmbhwm");
                }
                return false;
            }
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    /** process the cluster node selection page */
    private boolean processClusterNodes(WizardEvent event) {
        TraceUtil.trace3("Entering");
        Object [] values = (Object [])
            wizardModel.getValues(NewWizardClusterNodesView.NODES);

        for (int i = 0; values != null && i < values.length; i++) {
            TraceUtil.trace3((1+i) + ") -> " + values[i]);
            String val = values[i].toString();
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private boolean processArchiveConfigPage(WizardEvent evt) {
        // policy type
        String policyType = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.POLICY_TYPE_EXISTING);
        if (policyType == null || policyType.length() == 0) {
            setWizardAlert(evt,
                           "FSWizard.new.archiving.error.policytype");
            return false;
        }

        // Archive log file can be applied to existing and new policies
        // so check it here.
        String logFile = (String) wizardModel
            .getValue(NewWizardArchiveConfigView.LOG_FILE);
        logFile = logFile == null ? "" : logFile.trim();

        // It is okay to not specify the log file
        if (logFile.length() != 0 && !SamUtil.isWellFormedPath(logFile)) {
            setWizardAlert(evt, "FSWizard.new.archiving.error.logfile");
            return false;
        }

        String policyName = null;
        if ("existing".equals(policyType)) {
            policyName = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.EXISTING_POLICY_NAME);

            if (policyName == null) {
                setWizardAlert(evt,
                   "FSWizard.new.archiving.error.noexistingpolicy");
                return false;
            }

            // if we get this far, nothing else to validate
            return true;
        } else {
            policyName = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.NEW_POLICY_NAME);
        }

        if (policyName == null) {
            setWizardAlert(evt, "FSWizard.new.archiving.error.policyname");
            return false;
        }

        // copy 1  archive age
        String age = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_ONE);
        if (!isPositiveInteger(age)) {
            setWizardAlert(evt, "FSWizard.new.archiving.error.age");
            return false;
        }

        // copy 2 archive age
        String enableString = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ENABLE_COPY_TWO);
        if ("true".equals(enableString)) {
            age = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_TWO);
            if (!isPositiveInteger(age)) {
                setWizardAlert(evt, "FSWizard.new.archiving.error.age");
                return false;
            }
        }

        // copy 3 archive age
        enableString = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ENABLE_COPY_THREE);
        if ("true".equals(enableString)) {
            age = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_THREE);
            if (!isPositiveInteger(age)) {
                setWizardAlert(evt, "FSWizard.new.archiving.error.age");
                return false;
            }
        }

        // copy 4 archive age
        enableString = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ENABLE_COPY_FOUR);
        if ("true".equals(enableString)) {
            age = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_FOUR);
            if (!isPositiveInteger(age)) {
                setWizardAlert(evt, "FSWizard.new.archiving.error.age");
                return false;
            }
        }

        return true;
    }

    /**
     * utility method to validate numbers
     */
    private boolean isPositiveInteger(String s) {
        boolean result = true;
        if (s == null || s.length() == 0) {
            result = false;
        } else {
            try {
                int i = Integer.parseInt(s);
            } catch (NumberFormatException nfe) {
                result = false;
            }
        }

        return result;
    }

    /**
     * Utility method to return an Array of DataDevices
     * from an ArrayList of device paths
     */
    private DiskCache[] getSelectedDeviceArray(ArrayList selectedDevicePaths) {
        if (selectedDevicePaths == null) {
            return new DiskCache[0];
        }

        int numDevices = selectedDevicePaths.size();
        DiskCache[] selectedDevices;

        if (sharedEnabled) {
            selectedDevices = new SharedDiskCache[numDevices];
        } else {
            selectedDevices = new DiskCache[numDevices];
        }
        Iterator iter = selectedDevicePaths.iterator();
        int i = 0;
        while (iter.hasNext()) {
            selectedDevices[i] = getDiskCacheObject((String)iter.next());
            i++;
        }

        return selectedDevices;
    }

    private DiskCache getDiskCacheObject(String devicePath) {
        for (int i = 0; i < allAllocatableDevices.length; i++) {
            if (allAllocatableDevices[i].getDevicePath().equals(devicePath)) {
                return allAllocatableDevices[i];
            }
        }

        // Should not reach here!! the selection is from this List
        // so there should be a match
        TraceUtil.trace1(
            "Selection device does not exist in Current Devicelist");
        return null;
    }

    /**
     * Utility method to return an Array of StripedGroup
     */
    private StripedGroup[] getSelectedStripedGroupsArray(ArrayList groupList)
        throws SamFSException {

        if (groupList == null) {
            return new StripedGroup[0];
        }

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        int numGroups = groupList.size();
        StripedGroup[] stripedGroups = new StripedGroup[numGroups];
        for (int i = 0; i < numGroups; i++) {
            ArrayList groupDevices = (ArrayList) groupList.get(i);
            DiskCache[] disks = getSelectedDeviceArray(groupDevices);
            stripedGroups[i] = sysModel.getSamQFSSystemFSManager().
                createStripedGroup("",  disks);
        }

        return stripedGroups;
    }

    /**
     * All Allocatable Units are stored wizard Model.
     * This way we can avoid calling the backend everytime the user hits
     * the previous button.
     * Also this will help in keeping track of all the selections that the user
     * made during the course of the wizard, and will he helpful to remove
     * those entries for the Metadata LUN selection page.
     */
    private DiskCache[] getAllAllocatableUnits() throws SamFSException {
        TraceUtil.trace3("Entering");

        // DO NOT check if allAllocatableDevices is null before running
        // disk discovery.  Please see comment in populateDevicesInWizardModel()
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        if (sysModel == null) {
            throw new SamFSException(null, -2501);
        }

        String [] nodeList = null;
        if (isHAFS(wizardModel)) {
            ArrayList hosts = new ArrayList();

            // start by adding the current node - make sure the name is not
            // domain qualified
            String currentServer =
                (new StringTokenizer(serverName, ".")).nextToken();
            hosts.add(currentServer);

            Object [] nodes =
                wizardModel.getValues(NewWizardClusterNodesView.NODES);
            for (int i = 0; i < nodes.length; i++) {
                // prune current node as well as the '-none-' selection
                String tempNodeName = nodes[i].toString().trim();
                if (!tempNodeName.equals("") &&
                    !tempNodeName.equals(currentServer)) {
                    hosts.add(tempNodeName);
                }
            }

            nodeList = new String[hosts.size()];
            nodeList = (String [])hosts.toArray(nodeList);
        }

        allAllocatableDevices =
            sysModel.getSamQFSSystemFSManager().
                discoverAvailableAllocatableUnits(nodeList);

        return allAllocatableDevices;
    }

    /**
     * All Allocatable Units are stored wizard Model.
     * This way we can avoid calling the backend everytime the user hits
     * the previous button.
     * Also this will help in keeping track of all the selections that the user
     * made during the course of the wizard, and will he helpful to remove
     * those entries for the Metadata LUN selection page.
     */
    private SharedDiskCache[] getAllSharedAllocatableUnits()
        throws SamFSMultiHostException, SamFSException {
        TraceUtil.trace3("Entering");

        ArrayList clientHosts = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_CLIENT);
        ArrayList potentialHosts = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_VALUE);

        // initialize these if null
        if (clientHosts == null) clientHosts = new ArrayList<String>();
        if (potentialHosts == null) potentialHosts = new ArrayList<String>();

        String[] clients = (String[]) clientHosts.toArray(new String[0]);
        String[] potentialServers = (String[])
            potentialHosts.toArray(new String[0]);
        String[] serverHosts = new String[potentialServers.length + 1];
        serverHosts[0] = serverName;
        for (int i = 1; i < potentialServers.length + 1; i++) {
            serverHosts[i] = potentialServers[i-1];
        }

        // DO NOT check if allAllocatableDevices is null before running
        // disk discovery.  Please see comment in populateDevicesInWizardModel()
        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        SamQFSSystemSharedFSManager fsManager =
            appModel.getSamQFSSystemSharedFSManager();

        allAllocatableDevices = (SharedDiskCache[])
            fsManager.discoverAllocatableUnitsForShared(
                serverHosts, clients, !isHAFS(wizardModel));

        TraceUtil.trace3("all device = " + allAllocatableDevices.toString());

        return (SharedDiskCache[]) allAllocatableDevices;
    }

    private boolean populateDevicesInWizardModel() {
        // We cannot rely on "devices" to determine
        // if we need to run disk discovery.  What if the user choose "Shared
        // Client" and click next, the click previous and choose "PMDS"?  We
        // need to rediscover the disks again, no matter what.
        // Simply checking if devices is null to determine if we need to
        // run discovery has another serious problem.  What if the user tries
        // to create a non-shared FS?  The current code will do the non-shared
        // discovery and have all the Diskcache saved.  Now the user goes back
        // to the first page and select the Shared CheckBox, no discovery
        // happens anymore!!!!  This will toast your wizard right away.

        try {
            if (sharedEnabled) {
                // cast to SharedDiskCache
                allAllocatableDevices = (SharedDiskCache[])
                    getAllSharedAllocatableUnits();
            } else {
                allAllocatableDevices = getAllAllocatableUnits();
            }
        } catch (SamFSMultiHostException e) {
            SamUtil.doPrint(new NonSyncStringBuffer().append("error code is ").
                append(e.getSAMerrno()).toString());
            String errMsg = SamUtil.handleMultiHostException(e);
            String err = "failed to populate shared device \n" + errMsg;
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE,
                errMsg);
            wizardModel.setValue(
                Constants.Wizard.ERROR_CODE,
                Integer.toString(e.getSAMerrno()));
            return false;
        } catch (SamFSException ex) {
            TraceUtil.trace1("Exception getting AllocatableUnits" +
                ex.getMessage(), ex);

            SamUtil.processException(
                ex,
                this.getClass(),
                "populateDevicesInWizardModel()",
                "Failed to populate device table",
                serverName);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.ERROR_CODE,
                Integer.toString(ex.getSAMerrno()));
            return false;
        }

        // update the ALLOCATABLE_DEVICE in the wizard model as well
        wizardModel.setValue(
            Constants.Wizard.ALLOCATABLE_DEVICES,
            allAllocatableDevices);
        return true;
    }

    private boolean fileSystemExists(String fsName) {
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            String[] fsNames =
                sysModel.getSamQFSSystemFSManager().getAllFileSystemNames();
            for (int i = 0; i < fsNames.length; i++) {
                if (fsNames[i].equals(fsName)) {
                    return true;
                }
            }

            fsNames = sysModel.getSamQFSSystemFSManager().
                                    getFileSystemNamesAllTypes();
            for (int i = 0; i < fsNames.length; i++) {
                if (fsNames[i].equals(fsName)) {
                    return true;
                }
            }
        } catch (SamFSException ex) {
            TraceUtil.trace1("Exception in checking if fileSystemExists()" +
                ex.getMessage());

            SamUtil.processException(
                ex,
                this.getClass(),
                "populateDevicesInWizardModel()",
                "Failed to populate device table",
                serverName);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_ERROR,
                Constants.Wizard.WIZARD_ERROR_YES);
            wizardModel.setValue(
                Constants.Wizard.ERROR_MESSAGE,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.ERROR_CODE,
                Integer.toString(ex.getSAMerrno()));
        }
        return false;
    }

    private void setWizardAlert(WizardEvent wizardEvent, String message) {
        wizardEvent.setSeverity(WizardEvent.ACKNOWLEDGE);
        wizardEvent.setErrorMessage(message);
    }

    private void processMultiStepException(SamFSMultiStepOpException msex) {
        String fsType = (String)wizardModel.getValue(FSTYPE_KEY);

        // Check if fsType is selected
        String fsName = "";
        if (FSTYPE_UFS.equals(fsType)) {
            fsName = ((String) wizardModel.getWizardValue(
                NewWizardMountView.CHILD_FSNAME_FIELD)).trim();
        }

        String processExMessage = null;
        String summaryMessage   = null;

        int samerrno = msex.getSAMerrno();
        // Set the result to be failed with MultiStepException
        // and the code to be the errNo and detail message to be the one sent
        // by the lower API
        wizardModel.setValue(
            Constants.AlertKeys.OPERATION_RESULT,
            Constants.AlertKeys.OPERATION_FAILED);
        wizardModel.setValue(
            Constants.Wizard.DETAIL_CODE,
            Integer.toString(samerrno));
        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
            msex.getMessage());

        // Set the Summary message to the type,
        // depending on which step it failed.
        int failedStep = msex.getFailedStep();
        switch (failedStep) {
            case 2:
                // Failed to create MountPoint
                summaryMessage = "FSWizard.new.error.createMountFailed";
                break;

            case 3:
                // Failed to create archiving information
                summaryMessage = "FSWizard.new.error.archivingInfoError";
                break;

            case 4:
                // archiver.cmd has error
                summaryMessage = "FSWizard.new.error.archiverFileError";
                break;

            case 5:
                // Failed to Mount
                summaryMessage = "FSWizard.new.error.mountFailed";
                break;

            case 6:
                // archiver.cmd has warnings
                summaryMessage = "FSWizard.new.error.archiverFileWarning";
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_WARNING);
                break;

            default:
                // FileSystem Creation failed
                summaryMessage = "FSWizard.new.error.summary";
                break;
        }

        wizardModel.setValue(
            Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY, summaryMessage);

        processExMessage =
            "Failed to create FS - " + fsName + ": "  + msex.getMessage();
        SamUtil.processException(
            msex,
            this.getClass(),
            "finishStep()",
            processExMessage,
            serverName);
    }

    private boolean createUFS() {
        boolean boolReadOnly = false;
        boolean boolCreateMountPoint = true; // always create mount point
        boolean boolMountAfterCreate = false;
        boolean boolMountAtBoot = false;
        boolean boolNoSetUID = false;

        DiskCache[] dataDevices =
            getSelectedDeviceArray(selectedDataDevicesList);
        for (int j = 0; j < dataDevices.length; j++) {
            TraceUtil.trace3("Chosen device " + dataDevices[j]);
        }
        String  mountPoint = ((String) wizardModel.getWizardValue(
            NewWizardStdMountView.CHILD_MOUNT_FIELD)).trim();
        TraceUtil.trace3("Mount point = " + mountPoint);
        String readOnly = (String) wizardModel.getValue(
            NewWizardStdMountView.CHILD_READONLY_CHECKBOX);
        if (readOnly != null && readOnly.equals("samqfsui.yes")) {
            boolReadOnly = true;
        }

        String noSetUID = (String) wizardModel.getValue(
            NewWizardStdMountView.CHILD_NOSETUID_CHECKBOX);
        if (noSetUID != null && noSetUID.equals("samqfsui.yes")) {
            boolNoSetUID = true;
        }

        String mountAtBoot = (String) wizardModel.getValue(
            NewWizardStdMountView.CHILD_BOOT_CHECKBOX);
        if (mountAtBoot != null && mountAtBoot.equals("samqfsui.yes")) {
            boolMountAtBoot = true;
        }

        String mountAfterCreate = (String) wizardModel.getValue(
            NewWizardStdMountView.CHILD_MOUNT_AFTER_CREATE_CHECKBOX);
        if (mountAfterCreate != null &&
            mountAfterCreate.equals("samqfsui.yes")) {
            boolMountAfterCreate = true;
        }

        GenericMountOptions mountOpts =
            new GenericMountOptionsImpl(boolReadOnly, boolNoSetUID);
        TraceUtil.trace3("Set mount options");

        SamQFSSystemModel sysModel = null;
        try {
            sysModel = SamUtil.getModel(serverName);
            TraceUtil.trace3("Creating UFS with device " + dataDevices[0]);
            GenericFileSystem ufs =
                sysModel.getSamQFSSystemFSManager().createUFS(
                    dataDevices[0],
                    mountPoint,
                    mountOpts,
                    boolMountAtBoot,
                    boolCreateMountPoint,
                    boolMountAfterCreate);

            LogUtil.info(
                this.getClass(),
                "finishStep",
                "Done creating new FS " + mountPoint);
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "FSSummary.createfs", mountPoint));

            TraceUtil.trace2("Succesfully created FS with name: " + mountPoint);
            return true;
        } catch (SamFSMultiStepOpException msex) {
            processMultiStepException(msex);
            return true;
        } catch (SamFSTimeoutException toex) {
            TraceUtil.trace2("processing samfs time out exception...");
            // RPC timed out
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_WARNING);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.warning.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceStringError("-2801"));
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE,
                Integer.toString(toex.getSAMerrno()));

            String processExMessage =
                "Timed out when creating " +
                mountPoint + ": "  + toex.getMessage();
            SamUtil.processException(
                toex,
                this.getClass(),
                "finishStep()",
                processExMessage,
                serverName);

            return true;
        } catch (SamFSException ex) {
            boolean multiMsgOccurred = false;
            boolean warningOccurred = false;
            String processMsg = null;
            processMsg = "Failed to create FS " + mountPoint;

            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                processMsg,
                serverName);

            int errCode = ex.getSAMerrno();
            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_FAILED);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "FSWizard.new.error.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                ex.getMessage());
            wizardModel.setValue(
                Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));

            return true;
         } catch (Exception e) {
            e.printStackTrace();
            e.printStackTrace(System.out);
            return true;
        }
    }

    /**
     * determine if the current wizard [represented by the wizard model] is
     * creating an HAFS.
     */
    public static final boolean isHAFS(SamWizardModel model) {
        Boolean hafs =  (Boolean)model.getValue(POPUP_HAFS);

        return hafs != null ? hafs : false;
    }

    private FSArchCfg getArchiveConfig() {
        String policyName = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.EXISTING_POLICY_NAME);
        String policyType = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.POLICY_TYPE_EXISTING);

        // log file is valid for existing policies and new ones
        // so get it here and use it for either one.
        String logFile = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.LOG_FILE);

        if ("existing".equals(policyType)) {
            return new FSArchCfg(policyName, logFile, null, null);
        }

        // new policy name
        policyName = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.NEW_POLICY_NAME);

        // norelease
        boolean noRelease = "true".equals((String)
            wizardModel.getValue(NewWizardArchiveConfigView.NO_RELEASE));

        // copy 1
        //  archive age
        String temp = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_ONE);
        long age = Long.parseLong(temp);
        temp = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_ONE_UNIT);
        int unit = Integer.parseInt(temp);
        age = SamQFSUtil.convertToSecond(age, unit);
        Copy copy1 = new Copy();
        copy1.setCopyNumber(1);
        copy1.setArchiveAge(age);
        copy1.setNoRelease(noRelease);

        ArrayList vsnmaps = new ArrayList();
        ArrayList copies = new ArrayList();

        // vsn map
        temp = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.MEDIA_ONE);
        String [] media = temp.split(":");
        VSNMap map1 = null;
        String copyName = policyName.concat(".1");
        if (media != null && media.length == 2) {
            String mediaType =
                SamQFSUtil.getMediaTypeString(Integer.parseInt(media[1]));
            if (media[0].equals(".")) { // vsn name
                map1 = new VSNMap(copyName,
                                  mediaType,
                                  new String [] {media[0]},
                                  null);
            } else { // pool name
                map1 = new VSNMap(copyName,
                                  mediaType,
                                  null,
                                  new String [] {media[0]});
            }
        }
        vsnmaps.add(map1);
        copies.add(copy1);

        temp = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ENABLE_COPY_TWO);

        // copy 2
        if ("true".equals(temp)) {
        media = ((String)wizardModel
            .getValue(NewWizardArchiveConfigView.MEDIA_TWO)).split(":");
        if (media != null && media.length == 2) { // copy two media
            // archive age
            temp = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_TWO);
            age = Long.parseLong(temp);
            temp = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_TWO_UNIT);
            unit = Integer.parseInt(temp);
            age = SamQFSUtil.convertToSecond(age, unit);

            Copy copy = new Copy();
            copy.setCopyNumber(2);
            copy.setArchiveAge(age);
            copy.setNoRelease(noRelease);

            // vsn map
            VSNMap map = null;
            copyName = policyName.concat(".2");
            String mediaType =
                SamQFSUtil.getMediaTypeString(Integer.parseInt(media[1]));

            if (media[0].equals(".")) { // vsn name
                map = new VSNMap(copyName,
                                 mediaType,
                                 new String [] {media[0]},
                                 null);
            } else { // pool name
                map = new VSNMap(copyName,
                                 mediaType,
                                 null,
                                 new String [] {media[0]});
            }

            vsnmaps.add(map);
            copies.add(copy);
        }
        }

        temp = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ENABLE_COPY_THREE);

        // copy 3
        if ("true".equals(temp)) {
        media = ((String)wizardModel
            .getValue(NewWizardArchiveConfigView.MEDIA_THREE)).split(":");
        if (media != null && media.length == 2) { // copy three media
            // archive age
            temp = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_THREE);
            age = Long.parseLong(temp);
            temp = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_THREE_UNIT);
            unit = Integer.parseInt(temp);
            age = SamQFSUtil.convertToSecond(age, unit);

            Copy copy = new Copy();
            copy.setCopyNumber(3);
            copy.setArchiveAge(age);
            copy.setNoRelease(noRelease);

            // vsn map
            VSNMap map = null;
            copyName = policyName.concat(".3");
            String mediaType =
                SamQFSUtil.getMediaTypeString(Integer.parseInt(media[1]));

            if (media[0].equals(".")) { // vsn name
                map = new VSNMap(copyName,
                                 mediaType,
                                 new String [] {media[0]},
                                 null);
            } else { // pool name
                map = new VSNMap(copyName,
                                 mediaType,
                                 null,
                                 new String [] {media[0]});
            }

            vsnmaps.add(map);
            copies.add(copy);
        }
        }

        temp = (String)wizardModel
            .getValue(NewWizardArchiveConfigView.ENABLE_COPY_FOUR);

        // copy 4
        if ("true".equals(temp)) {
        media = ((String)wizardModel
            .getValue(NewWizardArchiveConfigView.MEDIA_FOUR)).split(":");
        if (media != null && media.length == 2) { // copy four media
            // archive age
            temp = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_FOUR);
            age = Long.parseLong(temp);
            temp = (String)wizardModel
                .getValue(NewWizardArchiveConfigView.ARCHIVE_AGE_FOUR_UNIT);
            unit = Integer.parseInt(temp);
            age = SamQFSUtil.convertToSecond(age, unit);

            Copy copy = new Copy();
            copy.setCopyNumber(4);
            copy.setArchiveAge(age);
            copy.setNoRelease(noRelease);

            // vsn map
            VSNMap map = null;
            copyName = policyName.concat(".4");
            String mediaType =
                SamQFSUtil.getMediaTypeString(Integer.parseInt(media[1]));

            if (media[0].equals(".")) { // vsn name
                map = new VSNMap(copyName,
                                 mediaType,
                                 new String [] {media[0]},
                                 null);
            } else { // pool name
                map = new VSNMap(copyName,
                                 mediaType,
                                 null,
                                 new String [] {media[0]});
            }

            vsnmaps.add(map);
            copies.add(copy);
        }
        }

        VSNMap [] maps = (VSNMap [])vsnmaps.toArray(new VSNMap[0]);
        Copy [] copy = (Copy [])copies.toArray(new Copy[0]);

        return new FSArchCfg(policyName, logFile, maps, copy);
    }

    private int [] insertArchiveConfigPage(int [] pages) {
        if (pages == null || pages.length == 0)
            return pages;

        // check if the archive config is already inserted
        for (int i = 0; i < pages.length; i++) {
            if (pages[i] == CreateFSWizardImplData.PAGE_ARCHIVE_CONFIG) {
                return pages;
            }
        }

        // create a new array and insert the archive config page before the
        // summary page
        int [] newPages = new int[pages.length + 1];
        for (int i = 0, j = 0; i < pages.length; i++, j++) {
            newPages[j] = pages[i];
            if (i == pages.length - 3) {
                newPages[++j] = CreateFSWizardImplData.PAGE_ARCHIVE_CONFIG;
            }
        }

        return newPages;
    }

    private void populateDefaultBlockAllocation() {
        String mdStorage = (String)
            wizardModel.getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
        if (NewWizardMetadataOptionsView.SAME_DEVICE.equals(mdStorage)) {
            wizardModel.setValue(NewWizardBlockAllocationView.ALLOCATION_METHOD,
                NewWizardBlockAllocationView.SINGLE);
            wizardModel.setValue(
                NewWizardBlockAllocationView.BLOCK_SIZE_DROPDOWN, "64");
            wizardModel.setValue(
                NewWizardBlockAllocationView.BLOCKS_PER_DEVICE, "0");
        } else { // separate data and metadata
            String allocationMethod = hpcEnabled ?
                NewWizardBlockAllocationView.STRIPED :
                NewWizardBlockAllocationView.DUAL;

            wizardModel.setValue(NewWizardBlockAllocationView.ALLOCATION_METHOD,
                                 allocationMethod);

            wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE, "64");
            wizardModel.setValue(NewWizardBlockAllocationView.BLOCK_SIZE_UNIT,
                "1");
            if (hafsEnabled) { // set the default blocks per device to 2
                wizardModel.setValue(
                    NewWizardBlockAllocationView.BLOCKS_PER_DEVICE, "2");
            } else { // set default blocks per device to 0
                wizardModel.setValue(
                    NewWizardBlockAllocationView.BLOCKS_PER_DEVICE, "0");
            }
        }
    }

    private int getStripedGroupBase() {
        int base = -1;
        boolean found = false;

        for (int i = 0; !found && i < pages.length; i++) {
            if (pages[i] == CreateFSWizardImplData.PAGE_STRIPED_GROUP) {
                base = i;
                found = true;
            }
        }

        return base;
    }

}
