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

// ident	$Id: FSDetailsView.java,v 1.46 2008/06/04 18:16:10 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.archive.ArchiveActivityViewBean;
import com.sun.netstorage.samqfs.web.fs.wizards.GrowWizardImpl;
import com.sun.netstorage.samqfs.web.jobs.JobsDetailsViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.wizard.CCWizardWindow;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the FSDetails page
 */

public class FSDetailsView extends CommonTableContainerView {

    protected CCPageTitleModel pageTitleModel = null;
    protected CCPropertySheetModel propertySheetModel  = null;

    protected String fsName;
    protected String serverName;

    protected int fsType = FileSystem.FS_SAMQFS;  // fs type
    protected boolean qfsStandalone = false; // standalone QFS pkg?
    protected String parents = null;
    protected String vfstabFSType = "qfs";
    protected Boolean isArchived;

    // hidden fields used to pass state to client side javascript
    public static final String
        HIDDEN_DYNAMIC_MENUOPTIONS = "HiddenDynamicMenuOptions";

    public static final String CHILD_HIDDEN_FS_NAME = "fsName";

    // Child View names associated with Samfsck pop up
    public static final String CHILD_HIDDEN_FSCK_ACTION = "SamfsckHiddenAction";
    public static final String CHILD_HIDDEN_FSCK_LOG = "SamfsckHiddenLog";
    public static final String CHILD_SAMFSCK_HREF  = "SamfsckHref";
    public static final String CHILD_CANCEL_HREF = "CancelHref";

    // For clicking on job link in alert after samfschk
    public static final String CHILD_JOBID_HREF = "JobIdHref";

    // Grow wizard
    public static final String CHILD_FRWD_TO_CMDCHILD = "forwardToVb";

    protected boolean writeRole = false;
    protected CCWizardWindowModel growWizWinModel;

    // constants used to pass state info to client side javascript
    // to dynamically enable/disable dropdown menu options
    public static final int FS_MENUOPTION_CHECK_FS = 1;
    public static final int FS_MENUOPTION_MOUNT = 2;
    public static final int FS_MENUOPTION_UNMOUNT = 3;
    public static final int FS_MENUOPTION_DELETE = 4;
    public static final int FS_MENUOPTION_ARCHIVE_ACTIVITIES = 5;
    public static final int FS_MENUOPTION_SCHEDULE_DUMP = 6;

    private boolean sharedFlag = false;

    /**
     * Constructor
     */
    public FSDetailsView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ViewBean vb = getParentViewBean();

        // get samfsServerAPIVersion and license type from cache
        serverName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        fsName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
        vfstabFSType = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.VFSTAB_FS_TYPE);

        Integer fsTypeObj = (Integer) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_TYPE);

        isArchived = (Boolean) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.IS_ARCHIVED);

        // MUST set this flag to true if page session attribute is null
        // FS Details page can be reached from Criteria Details Page,
        // thus archive has to be true if it comes from Criteria Details Page.
        // This flag will not be null if it comes from pages in the FS Tab
        isArchived = (isArchived == null) ? Boolean.TRUE : isArchived;

        if (fsTypeObj != null) {
            fsType = fsTypeObj.intValue();
        }

        vfstabFSType = vfstabFSType == null ? "<unknown>" : vfstabFSType;
        TraceUtil.trace3("Got serverName, fsName & fsType from page session: ".
            concat(serverName).concat(", ").concat(fsName).concat(", ").concat(
            Integer.toString(fsType)).concat(", ").concat(vfstabFSType));

        qfsStandalone =
            SamUtil.getSystemType(serverName) == SamQFSSystemModel.QFS;

        pageTitleModel = createPageTitleModel();
        initializeGrowWizard();

        propertySheetModel = createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(HIDDEN_DYNAMIC_MENUOPTIONS, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FS_NAME, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FSCK_ACTION, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FSCK_LOG, CCHiddenField.class);
        registerChild(CHILD_CANCEL_HREF, CCHref.class);
        registerChild(CHILD_SAMFSCK_HREF, CCHref.class);
        registerChild(CHILD_JOBID_HREF, CCHref.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CHILD_FRWD_TO_CMDCHILD, BasicCommandField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering with name = " + name);
        if (name.equals(CHILD_HIDDEN_FS_NAME) ||
            name.equals(HIDDEN_DYNAMIC_MENUOPTIONS) ||
            name.equals(CHILD_HIDDEN_FSCK_ACTION) ||
            name.equals(CHILD_HIDDEN_FSCK_LOG)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
            // PropertySheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            TraceUtil.trace3("Exiting");
            return PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else if (name.equals(CHILD_CANCEL_HREF) ||
                   name.equals(CHILD_SAMFSCK_HREF) ||
                   name.equals(CHILD_JOBID_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_FRWD_TO_CMDCHILD)) {
            BasicCommandField bcf = new BasicCommandField(this, name);
            TraceUtil.trace3("Exiting");
            return bcf;
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        setGrowWizardNames();
        TraceUtil.trace3("Exiting");
    }

    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");

        GenericFileSystem fs = getFileSystem(fsName);
        if (fs == null) {
            propertySheetModel.setVisible("samfsSection", false);
            propertySheetModel.setVisible("archiveSection", false);
            propertySheetModel.setVisible("sharedSection", false);
            throw new SamFSException(null, -1000);
        }

        if (fsType != GenericFileSystem.FS_NONSAMQ) {
            FileSystem fileSystem = (FileSystem) fs;
            // check dau size which is a special flag to indicate if there's
            // a problem reading superblock of the first device of this fs
            // or this fs may contain bad devices
            if (fileSystem.getDAUSize() == 1) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "FSDetails.error.populate",
                    -1008,
                    SamUtil.getResourceString("FSDetails.error.dauDetails"),
                    serverName);
            } else if (fileSystem.getDAUSize() == 2) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "FSDetails.error.populate",
                    -1008,
                    SamUtil.getResourceString("FSDetails.error.badDevices"),
                    serverName);
            }
        }

        loadPropertySheetModel(fs);

        // calculate state info to pass to client side javascript to
        // dynamically enable/disable buttons and dropdown menu options
        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
            if (fsType == GenericFileSystem.FS_NONSAMQ) {
                setUFSOptions(fs);
                getParentViewBean().
                    setPageSessionAttribute("psa_UFS", Boolean.toString(true));
            } else if (fsType == GenericFileSystem.FS_QFS ||
                qfsStandalone ||
                fsType == GenericFileSystem.FS_SAM &&
                    !isArchived.booleanValue()) {
                setQFSOptions((FileSystem) fs);
            } else {
                setSAMFSOptions((FileSystem) fs);
            }
        }

        // User "File System Details" as the title for UFS
        if (fsType == GenericFileSystem.FS_NONSAMQ &&
            (fs.getFSTypeName().equals("ufs") ||
                fs.getFSTypeName().equals("vxfs"))) {
            pageTitleModel.setPageTitleText(
                SamUtil.getResourceString("FSDetails.pageTitle"));
        } else {
            pageTitleModel.setPageTitleText(
                SamUtil.getResourceString("FSDetails.pageTitle1", fsName));
            if (!sharedFlag) {
                ((CCLabel) getChild("deviceNameLabel")).setValue("");
            }
        }

        ((CCHiddenField) getChild(CHILD_HIDDEN_FS_NAME)).setValue(fsName);

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild("PageActionsMenu")).setValue("0");

        boolean growEnabled = false;
        String serverAPIVersion = "1.5";
        try {
            serverAPIVersion =
                SamUtil.getServerInfo(serverName).
                            getSamfsServerAPIVersion();
            TraceUtil.trace3("API Version: " + serverAPIVersion);
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Error getting samfs version!", samEx);
        }

        if (fsType == GenericFileSystem.FS_NONSAMQ) {
            growEnabled = false;
        } else if (
            SamUtil.isVersionCurrentOrLaterThan(serverAPIVersion, "1.6")) {
            growEnabled = !fs.isHA();
        } else {
            growEnabled = !fs.isHA() &&
                fs.getState() == FileSystem.UNMOUNTED &&
                ((FileSystem) fs).getShareStatus() == FileSystem.UNSHARED;
        }
        ((CCWizardWindow)
                getChild("SamQFSWizardGrowFSButton")).setDisabled(!growEnabled);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Deal with UFS, ZFS
     */
    protected void setUFSOptions(GenericFileSystem fs) {
        ((CCButton) getChild("EditMountOptionsButton")).setDisabled(true);

        int fsDescription = FSUtil.getFileSystemDescription(fs);

        // calculate state info for dropdown menu options
        StringBuffer menuOptions = new StringBuffer();
        if (fs.getState() == FileSystem.MOUNTED) {
            // cannot umount unix root file system & zfs
            if (!"/".equals(fs.getMountPoint()) &&
                fsDescription != FSUtil.FS_DESC_ZFS) {
                menuOptions.append(FS_MENUOPTION_UNMOUNT - 1).append(',');
            }
        } else {
            if (fsDescription != FSUtil.FS_DESC_ZFS) {
                menuOptions.append(FS_MENUOPTION_MOUNT - 1).append(',');
            }
            if (!fs.isHA()) {
                menuOptions.append(FS_MENUOPTION_DELETE - 1).append(',');
            }
        }

        // remove the trailing ',' if there's one
        int len = menuOptions.length();
        if (len > 0 && menuOptions.charAt(len - 1) == ',') {
            menuOptions.deleteCharAt(len - 1);
        }
        ((CCHiddenField) getChild(HIDDEN_DYNAMIC_MENUOPTIONS)).setValue(
            menuOptions.toString());

        propertySheetModel.setVisible("samfsSection", false);
        propertySheetModel.setVisible("archiveSection", false);
        propertySheetModel.setVisible("sharedSection", false);
    }

    protected void setQFSOptions(FileSystem fs) {
        int state = fs.getState();
        int fsShared = fs.getShareStatus();
        boolean noDeleteFlag = false;

        if (fsShared != FileSystem.UNSHARED) {
            // this is for shared fs
            noDeleteFlag = checkDeleteFlag(fs);
        }

        ((CCButton) getChild("EditMountOptionsButton")).setDisabled(false);

        // calculate state info for dropdown menu options

        StringBuffer menuOptions = new StringBuffer();
        if (fsShared == FileSystem.UNSHARED) {
            menuOptions.append(FS_MENUOPTION_CHECK_FS).append(',');
        }

        if (state == FileSystem.MOUNTED) {
            // cannot umount unix root file system
            if (!("/".equals(fs.getMountPoint()))) {
                menuOptions.append(FS_MENUOPTION_UNMOUNT).append(',');
            }
        } else { // unmounted
            menuOptions.append(FS_MENUOPTION_MOUNT).append(',');
            if (!fs.isHA() && !noDeleteFlag) {
                menuOptions.append(FS_MENUOPTION_DELETE);
            }
        }

        // remove the trailing ',' if there's one
        int len = menuOptions.length();
        if (len > 0 && menuOptions.charAt(len - 1) == ',') {
            menuOptions.deleteCharAt(len - 1);
        }
        ((CCHiddenField) getChild(HIDDEN_DYNAMIC_MENUOPTIONS)).setValue(
            menuOptions.toString());

        propertySheetModel.setVisible("archiveSection", false);
        if (fsShared == FileSystem.UNSHARED) {
            propertySheetModel.setVisible("sharedSection", false);
        }
    }

    protected void setSAMFSOptions(FileSystem fs) {
        int state = fs.getState();
        int fsShared = fs.getShareStatus();
        boolean noDeleteFlag = false;

        if (fsShared != FileSystem.UNSHARED) {
            // this is for shared fs
            noDeleteFlag = checkDeleteFlag(fs);
        }

        // disable view policy button if file system is HA
        ((CCButton) getChild("ViewPolicyButton")).setDisabled(fs.isHA());
        ((CCButton) getChild("EditMountOptionsButton")).setDisabled(false);

        // calculate state info for dropdown menu options
        StringBuffer menuOptions = new StringBuffer();
        if (fsShared == FileSystem.UNSHARED) {
            menuOptions.append(FS_MENUOPTION_CHECK_FS).append(',');
        }

        if (state == FileSystem.MOUNTED) {
            // cannot umount unix root file system
            if (!("/".equals(fs.getMountPoint()))) {
                menuOptions.append(FS_MENUOPTION_UNMOUNT).append(',');
            }
        } else {
            menuOptions.append(FS_MENUOPTION_MOUNT).append(',');
            if (!fs.isHA() && !noDeleteFlag) {
                menuOptions.append(FS_MENUOPTION_DELETE).append(',');
            }
        }

        if (!fs.isHA()) {
            menuOptions.append(FS_MENUOPTION_ARCHIVE_ACTIVITIES).append(',');
            menuOptions.append(FS_MENUOPTION_SCHEDULE_DUMP).append(',');
        }

        // remove the trailing ',' if there's one
        int len = menuOptions.length();
        if (len > 0 && menuOptions.charAt(len - 1) == ',') {
            menuOptions.deleteCharAt(len - 1);
        }
        ((CCHiddenField) getChild(HIDDEN_DYNAMIC_MENUOPTIONS)).setValue(
            menuOptions.toString());

        if (fsShared == FileSystem.UNSHARED) {
            propertySheetModel.setVisible("sharedSection", false);
        }
    }

    protected boolean checkDeleteFlag(FileSystem fs) {
        boolean noDeleteFlag = false;
        String metaDataHostName = fs.getServerName();
        TraceUtil.trace3("Shared metadata server is " + metaDataHostName);
        if (metaDataHostName.equals("")) {
            // metadata server must be mounted. Otherwise
            // metadata host name will not be empty.
            noDeleteFlag = true;
        } else {
            try {
                SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
                SamQFSSystemSharedFSManager fsManager =
                    appModel.getSamQFSSystemSharedFSManager();
                SharedMember[] sharedMember =
                    fsManager.getSharedMembers(metaDataHostName, fsName);
                if (sharedMember != null && sharedMember.length > 0) {
                    for (int i = 0; i < sharedMember.length; i++) {
                        if (sharedMember[i].isMounted()) {
                            noDeleteFlag = true;
                            break;
                        }
                   }
                }
            } catch (SamFSMultiHostException e) {
                 noDeleteFlag = true;
            } catch (SamFSException e) {
                 noDeleteFlag = true;
            }
        }
        return noDeleteFlag;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        String fsXML;

        if ((fsType == GenericFileSystem.FS_NONSAMQ) &&
            (vfstabFSType.equals("ufs") || (vfstabFSType.equals("vxfs"))
                                        || (vfstabFSType.equals("zfs")))) {
            fsXML = "/jsp/fs/UFSDetailsPageTitle.xml";
        } else if (qfsStandalone ||
            fsType == GenericFileSystem.FS_QFS ||
            (fsType == GenericFileSystem.FS_SAM &&
                !isArchived.booleanValue())) {
            fsXML = "/jsp/fs/QFSDetailsPageTitle.xml";
        } else {
            fsXML = "/jsp/fs/FSDetailsPageTitle.xml";
        }

        pageTitleModel = PageTitleUtil.createModel(fsXML);

        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Create the PropertySheetModel
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        int shared = FileSystem.UNSHARED;
        ViewBean vb = getParentViewBean();
        Integer sharedStatus = (Integer) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.SHARED_STATUS);
        if (sharedStatus != null) {
            shared = sharedStatus.intValue();
        }

        // Shared client or potentail metadata server
        if (shared != FileSystem.UNSHARED) {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/SharedFSDetailsGeneralPropertySheet.xml");
            sharedFlag = true;
        } else {
            TraceUtil.trace3("FSDETAIL Not SHARED");
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/FSDetailsPropertySheet.xml");
        }

        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    /**
     * Load the data for property sheet model
     */
    private void loadPropertySheetModel(GenericFileSystem fs)
        throws SamFSException {

        TraceUtil.trace3("Entering");
        propertySheetModel.clear();

        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        String cap = "";
        String free = "";
        String consumed  = "";
        String hwm = null;
        String point = null;
        String dau = null;
        String lwm = null;
        String timeAbove = null;
        String eq  = null;
        String server = null;
        String createDate  = null;
        String type = null;
        String state = null;
        String share = null;
        String sharedType = null;
        String deviceName = "";
        String metadataPlacement = "";

        //
        // general file system info
        //
        int fsState = fs.getState();
        long capacity = fs.getCapacity();
        long freeSpace = fs.getAvailableSpace();

        point = fs.getMountPoint();

        if (fsState == GenericFileSystem.MOUNTED) {
            state = "FSSummary.mount";
            int sizeUnit = SamQFSSystemModel.SIZE_KB;
            cap = new Capacity(capacity, sizeUnit).toString();
            free = new Capacity(freeSpace, sizeUnit).toString();
            consumed = Integer.toString(fs.getConsumedSpacePercentage());

        } else {
            state = "FSSummary.unmount";
        }

        if (fsType == GenericFileSystem.FS_NONSAMQ) {
            // Show device path for UFS
            if (fs.getFSTypeName().equals("ufs") ||
                fs.getFSTypeName().equals("vxfs")) {
                deviceName = fs.getName();
            }

        } else {
            // SAMFS/QFS details
            FileSystem fileSystem = (FileSystem) fs;
            eq = Integer.toString(fileSystem.getEquipOrdinal());

            int fsSharedStatus = fileSystem.getShareStatus();
            switch (fsSharedStatus) {
                case FileSystem.UNSHARED:
                    share = "FSSummary.unshared";
                    break;
                case FileSystem.SHARED_TYPE_UNKNOWN:
                    share = "FSSummary.shared";
                    break;
                case FileSystem.SHARED_TYPE_MDS:
                    share = "FSSummary.mds";
                    break;
                case FileSystem.SHARED_TYPE_PMDS:
                    share = "FSSummary.pmds";
                    break;
                case FileSystem.SHARED_TYPE_CLIENT:
                    share = "FSSummary.client";
                    break;
            }

            switch (fileSystem.getFSType()) {
                case FileSystem.SEPARATE_METADATA:
                    metadataPlacement = "FSDetails.metadataPlacement.separate";
                    break;

                case FileSystem.COMBINED_METADATA:
                    metadataPlacement = "FSDetails.metadataPlacement.same";
                    break;

                default:
                    metadataPlacement = "filesystem.desc.unknown";
                    break;
            }

            dau = Integer.toString(fileSystem.getDAUSize());
            server = fileSystem.getServerName();
            timeAbove = SamUtil.getTimeString(fileSystem.getTimeAboveHWM());
            createDate = SamUtil.getTimeString(fileSystem.getDateCreated());
            FileSystemMountProperties mountPro =
                fileSystem.getMountProperties();
            hwm = Integer.toString(mountPro.getHWM());
            lwm = Integer.toString(mountPro.getLWM());
            if (sharedFlag) {
                sharedType =
                    FSUtil.getSharedFSDescriptionString(
                        fileSystem.getShareStatus());
            }
        }

        type = FSUtil.getFileSystemDescriptionString(fs);

        propertySheetModel.setValue("typeValue", type);
        if (sharedFlag) {
           propertySheetModel.setValue("sharedFSTypeValue", sharedType);
        }
        propertySheetModel.setValue("mountValue", point);
        propertySheetModel.setValue("eqValue", eq);
        propertySheetModel.setValue("stateValue", state);
        propertySheetModel.setValue("capValue", cap);
        propertySheetModel.setValue("freespaceValue", free);
        propertySheetModel.setValue("consumedValue", consumed);
        propertySheetModel.setValue("shareValue", share);
        propertySheetModel.setValue("dauValue", dau);
        propertySheetModel.setValue("hwmValue", hwm);
        propertySheetModel.setValue("lwmValue", lwm);
        propertySheetModel.setValue("timeaboveValue", timeAbove);
        propertySheetModel.setValue("createdateValue", createDate);
        propertySheetModel.setValue("serverValue", server);
        propertySheetModel.setValue("deviceNameValue", deviceName);
        propertySheetModel.setValue(
            "metadataPlacementValue", metadataPlacement);

        // Populate NFS Shared Information
        propertySheetModel.setValue(
            "nfsSharedValue",
                fs.hasNFSShares() ?
                    SamUtil.getResourceString("samqfsui.yes") :
                    SamUtil.getResourceString("samqfsui.no"));

        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to enable the inline alert
     */
    private void showAlert(String op, String key) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(op, key),
            serverName);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to retrieve the FileSystem
     */
    private GenericFileSystem getFileSystem(String fsName)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        GenericFileSystem filesystem =
            sysModel.getSamQFSSystemFSManager().getGenericFileSystem(fsName);
        TraceUtil.trace3("Exiting");
        return filesystem;
    }

    /**
     * Set up the data model and create the model for wizard button
     */
    private void initializeGrowWizard() {
        TraceUtil.trace3("Entering");
        ViewBean view = getParentViewBean();

        StringBuffer cmdChild =
            new StringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("FSDetailsView.").
                append(CHILD_FRWD_TO_CMDCHILD);
        growWizWinModel = GrowWizardImpl.createModel(cmdChild.toString());
        pageTitleModel.setModel("SamQFSWizardGrowFSButton", growWizWinModel);
        growWizWinModel.setValue(
            "SamQFSWizardGrowFSButton", "FSDetails.button.grow");
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for Href in actiontable
     */
    public void handleViewPolicyButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(FSArchivePoliciesViewBean.class);
        ViewBean vb = getParentViewBean();
        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));
        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * handle request for page action menu
     */
    public void handlePageActionsMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String value = (String) getDisplayFieldValue("PageActionsMenu");
        String op = null;

        ViewBean vb = getParentViewBean();

        int option = 0;
        try {
            option = Integer.parseInt(value);
        } catch (NumberFormatException nfe) {
            // should not get here!!!
        }

        TraceUtil.trace2(new StringBuffer(
            "Handling DropDownMenu Option #").append(option).toString());

        if (option != 0) {
            try {
                GenericFileSystem fs = getFileSystem(fsName);
                if (fs != null) {
                    /**
                     * If fs is UFS or VSFS, option needs to be plus 1 because
                     * check fs does not exist in UFS drop down menu
                     */
                    if (fsType == GenericFileSystem.FS_NONSAMQ &&
                        (fs.getFSTypeName().equals("ufs") ||
                            fs.getFSTypeName().equals("vxfs"))) {
                        option = option + 1;
                    }
                    // Option 5 is Edit NFS Properties, this is available for
                    // all mounted file systems (ufs, vxfs, qfs, qfs-archiving)
                    switch (option) {
                        case FSDetailsView.FS_MENUOPTION_CHECK_FS:
                            // check fs is a pop-up
                            return;

                        case FSDetailsView.FS_MENUOPTION_MOUNT:
                            op = "FSDetails.mountfs";
                            LogUtil.info(this.getClass(),
                                "handlePageViewMenuHrefRequest",
                                "Start mounting filesystem " + fsName);
                            fs.mount();
                            if (fs.getState() == FileSystem.UNMOUNTED) {
                                throw new SamFSException(
                                    SamUtil.getResourceString(
                                        "FSDetails.warning.mount.cause"),
                                    -1099);
                            } else {
                                LogUtil.info(this.getClass(),
                                    "handlePageViewMenuHrefRequest",
                                    "Done mounting filesystem " + fsName);
                            }
                            break;

                        case FSDetailsView.FS_MENUOPTION_UNMOUNT:
                            op = "FSDetails.umountfs";
                            LogUtil.info(this.getClass(),
                                "handlePageViewMenuHrefRequest",
                                "Start unmounting filesystem " + fsName);
                            fs.unmount();
                            LogUtil.info(this.getClass(),
                                "handlePageViewMenuHrefRequest",
                                "Done unmounting filesystem " + fsName);
                            break;

                        case FSDetailsView.FS_MENUOPTION_DELETE:
                            handleDeleteButtonRequest(fs);
                            return;

                        case FSDetailsView.FS_MENUOPTION_ARCHIVE_ACTIVITIES:
                            ViewBean activityVB =
                                getViewBean(ArchiveActivityViewBean.class);
                            vb.setPageSessionAttribute(
                                Constants.PageSessionAttributes.FS_NAME,
                                fsName);
                            BreadCrumbUtil.breadCrumbPathForward(
                                vb,
                                PageInfo.getPageInfo().
                                    getPageNumber(vb.getName()));
                            ((CommonViewBeanBase) vb).forwardTo(activityVB);
                            return;

                        case FSDetailsView.FS_MENUOPTION_SCHEDULE_DUMP:
                            ViewBean scheduleDumpVB = getViewBean(
                                RecoveryPointScheduleViewBean.class);
                            // SN stands for Snapshot
                            vb.setPageSessionAttribute(
                                Constants.admin.TASK_ID, "SN");
                            vb.setPageSessionAttribute(
                                Constants.admin.TASK_NAME, fsName);

                            BreadCrumbUtil.breadCrumbPathForward(
                                vb,
                                PageInfo.getPageInfo().
                                    getPageNumber(vb.getName()));
                            ((CommonViewBeanBase) vb).forwardTo(scheduleDumpVB);
                            return;

                        default:
                            // should never get here!!!
                            throw new SamFSException(null, -1000);
                    }
                    showAlert(op, fsName);
                } else {
                    throw new SamFSException(null, -1000);
                }
            } catch (SamFSException ex) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handlePageActionsMenuHrefRequest()",
                    "failed to populate data",
                    serverName);

                String errorMsgs = null;
                switch (option) {
                    case FSDetailsView.FS_MENUOPTION_MOUNT:
                        errorMsgs = "FSDetails.error.mountfs";
                        break;
                    case FSDetailsView.FS_MENUOPTION_UNMOUNT:
                        errorMsgs = "FSDetails.error.umountfs";
                        break;
                }
                SamUtil.setErrorAlert(
                    vb,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errorMsgs,
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    serverName);
            }
        }

        vb.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for page action button 'Grow'
     */
    public void handleSamQFSWizardGrowFSButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for page action button 'Edit'
     */
    public void handleEditMountOptionsButtonRequest
        (RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        ViewBean vb = getParentViewBean();
        ViewBean targetView = null;

        try {
            FileSystem fs = (FileSystem) getFileSystem(fsName);
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }

            int fsType = fs.getFSTypeByProduct();
            int shareStatus = fs.getShareStatus();
            String mountPageType = null;

            targetView = getViewBean(FSMountViewBean.class);

            switch (shareStatus) {
                case FileSystem.UNSHARED:
                    switch (fsType) {
                        case FileSystem.FS_SAMQFS:
                            mountPageType = FSMountViewBean.TYPE_UNSHAREDSAMQFS;
                            break;
                        case FileSystem.FS_QFS:
                            mountPageType = FSMountViewBean.TYPE_UNSHAREDQFS;
                            break;
                        default:
                            mountPageType = FSMountViewBean.TYPE_UNSHAREDSAMFS;
                            break;
                    }
                    targetView.setPageSessionAttribute(
                        Constants.PageSessionAttributes.ARCHIVE_TYPE,
                        new Integer(fs.getArchivingType()));
                    break;
                default:
                    switch (fsType) {
                        case FileSystem.FS_SAMQFS:
                            mountPageType = FSMountViewBean.TYPE_SHAREDSAMQFS;
                            break;
                        default:
                            mountPageType = FSMountViewBean.TYPE_SHAREDQFS;
                            break;
                    }
            }

            targetView.setPageSessionAttribute(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME, fsName);
            targetView.setPageSessionAttribute(
                Constants.SessionAttributes.MOUNT_PAGE_TYPE, mountPageType);
            targetView.setPageSessionAttribute(
                Constants.SessionAttributes.SHARED_CLIENT_HOST,
                Constants.PageSessionAttributes.SHARED_HOST_NONSHARED);

            // if it is shared
            if (shareStatus != FileSystem.UNSHARED) {
                SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
                SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
                int sharedType = fsManager.getSharedFSType(serverName, fsName);
                if (sharedType == SharedMember.TYPE_MD_SERVER ||
                    sharedType == SharedMember.TYPE_POTENTIAL_MD_SERVER) {
                    targetView.setPageSessionAttribute(
                        Constants.SessionAttributes.SHARED_CLIENT_HOST,
                        Constants.PageSessionAttributes.SHARED_HOST_LOCAL);
                } else {
                    targetView.setPageSessionAttribute(
                        Constants.SessionAttributes.SHARED_CLIENT_HOST,
                        Constants.PageSessionAttributes.SHARED_HOST_CLIENT);
                }
           }
            BreadCrumbUtil.breadCrumbPathForward(
                vb,
                PageInfo.getPageInfo().getPageNumber(vb.getName()));
            ((CommonViewBeanBase) vb).forwardTo(targetView);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleEditMountOptionsButtonRequest()",
                "failed to retrieve fs/model",
                serverName);
            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSDetails.error.editmount",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        vb.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for page action button 'Delete'
     */
    public void handleDeleteButtonRequest(GenericFileSystem fileSystem)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        ViewBean vb = getParentViewBean();
        boolean error = false;
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            if (fileSystem == null) {
                throw new SamFSException(null, -1000);
            }

            LogUtil.info(
                this.getClass(),
                "handleDeleteButtonRequest",
                "Start deleting filesystem " + fsName);

            if (fsType != GenericFileSystem.FS_NONSAMQ &&
                ((FileSystem) fileSystem).
                    getShareStatus() != FileSystem.UNSHARED) {
                SamQFSAppModel appModel =
                    SamQFSFactory.getSamQFSAppModel();
                SamQFSSystemSharedFSManager fsManager =
                    appModel.getSamQFSSystemSharedFSManager();
                TraceUtil.trace3(
                    new StringBuffer("Begin shared deleting on host").
                    append(serverName).append("file:").append(fsName).
                        toString());
                fsManager.deleteSharedFileSystem(serverName, fsName, null);
            } else {
                sysModel.getSamQFSSystemFSManager().
                    deleteFileSystem(fileSystem);
            }
            LogUtil.info(
                this.getClass(),
                "handleDeleteButtonRequest",
                "Done deleting filesystem " + fsName);
        } catch (SamFSMultiHostException e) {
            SamUtil.doPrint(new StringBuffer().
                append("error code is ").
                append(e.getSAMerrno()).toString());
            String err_msg = SamUtil.handleMultiHostException(e);
            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSDetails.error.delete",
                e.getSAMerrno(),
                err_msg,
                serverName);
        } catch (SamFSException ex) {
            error = true;

            String processMsg = null;
            String errMsg = null;
            String errCause = null;
            boolean warning = false;
            boolean multiMsgOccurred = false;

            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
                multiMsgOccurred = true;
            } else if (ex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to delete filesystem";
                errMsg = "FSDetails.error.delete";
                errCause = ex.getMessage();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "handleDeleteButtonRequest()",
                processMsg,
                serverName);

            if (multiMsgOccurred || warning) { // fs got deleted
                ViewBean targetView = getViewBean(FSSummaryViewBean.class);
                if (multiMsgOccurred) {
                    SamUtil.setErrorAlert(
                        targetView,
                        CommonViewBeanBase.CHILD_COMMON_ALERT,
                        errMsg,
                        ex.getSAMerrno(),
                        errCause,
                        serverName);
                }
                if (warning) {
                    SamUtil.setWarningAlert(
                        targetView,
                        CommonViewBeanBase.CHILD_COMMON_ALERT,
                        errMsg,
                        errCause);
                }
                ((CommonViewBeanBase) vb).forwardTo(targetView);
                return;
            } else { // fs did not get deleted
                SamUtil.setErrorAlert(
                    vb,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errMsg,
                    ex.getSAMerrno(),
                    errCause,
                    serverName);
                vb.forwardTo(getRequestContext());
                return;
            }
        }
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        SamUtil.setInfoAlert(
            targetView,
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString("FSDetails.deletefs", fsName),
            serverName);
        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleForwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleSamfsckHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        ViewBean vb = getParentViewBean();

        boolean checkAndRepair = false;

        String type = (String) getDisplayFieldValue(CHILD_HIDDEN_FSCK_ACTION);
        String loc  = (String) getDisplayFieldValue(CHILD_HIDDEN_FSCK_LOG);

        TraceUtil.trace2("FSCK: type is " + type + ", loc is " + loc);

        try {
            FileSystem fileSystem = (FileSystem) getFileSystem(fsName);
            if (fileSystem == null) {
                throw new SamFSException(null, -1000);
            }

            // check catalog file existing
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
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
                    vb,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "success.summary",
                    SamUtil.getResourceString(
                        "FSSummary.samfsckresult",
                        new String[] {fsName, loc}),
                    serverName);
            } else {
                // Build href object...
                // href looks like this:
                // <a href="../fs/FSDetails?FSDetails.JobIdHref=123456"
                // name="FSDetails.JobIdHref"
                // onclick=
                //  "javascript:var f=document.FSDetailsForm;
                //   if (f != null) {f.action=this.href;f.submit();
                //   return false}">123456</a>"
                StringBuffer href = new StringBuffer()
                    .append("<a href=\"../fs/FSDetails?")
                    .append("FSDetails.FSDetailsView.JobIdHref=")
                    .append(jobID).append(",")
                    .append(Constants.Jobs.JOB_TYPE_SAMFSCK).append(",Current")
                    .append("\" ")
                    .append("name=\"FSDetails.FSDetailsView.")
                    .append(CHILD_JOBID_HREF).append("\" ")
                    .append("onclick=\"")
                    .append("javascript:var f=document.FSDetailsForm;")
                    .append("if (f != null) {f.action=this.href;f.submit();")
                    .append("return false}\"")
                    .append(">").append(jobID).append("</a>");

                SamUtil.setInfoAlert(
                    vb,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "success.summary",
                    SamUtil.getResourceString("FSSummary.samfsckjob",
                         new String[] {fsName, loc, href.toString()}),
                    serverName);
            }
        } catch (SamFSException ex) {
            if (ex.getSAMerrno() == -1000) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSamfsckHrefRequest()",
                    "can not retrieve file system",
                    serverName);
            } else if (ex.getSAMerrno() == -1009) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSamfsckHrefRequest()",
                    "Catalog file exists",
                    serverName);
            }
            SamUtil.setErrorAlert(
                vb,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "FSDetails.error.checkfs",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        vb.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleJobIdHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");
        String jobID = getDisplayFieldStringValue(CHILD_JOBID_HREF);

        // Store the job id in a page session variable
        // The job id has the format:
        //   jobId,jobType,jobCondition
        // For samfs chk jobs, the job type is:
        //   Jobs.jobType6

        ViewBean vb = getParentViewBean();
        ViewBean targetView = getViewBean(JobsDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            vb,
            PageInfo.getPageInfo().getPageNumber(vb.getName()));
        vb.setPageSessionAttribute(
            Constants.PageSessionAttributes.JOB_ID, jobID);

        ((CommonViewBeanBase) vb).forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleCancelHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void setGrowWizardNames() {
        TraceUtil.trace3("Entering");
        String modelName =
            GrowWizardImpl.WIZARDPAGEMODELNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
        growWizWinModel.setValue(GrowWizardImpl.WIZARDPAGEMODELNAME, modelName);

        String implName =
            GrowWizardImpl.WIZARDIMPLNAME_PREFIX + "_" +
                HtmlUtil.getUniqueValue();
        growWizWinModel.setValue(
            CCWizardWindowModelInterface.WIZARD_NAME, implName);
        TraceUtil.trace3("Exiting");
    }
}
