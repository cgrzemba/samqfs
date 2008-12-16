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

// ident	$Id: SharedFSDetailsViewBean.java,v 1.21 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.ServerInfo;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Hashtable;
import javax.servlet.ServletException;
import javax.servlet.http.HttpSession;


/**
 *  This class is the view bean for the Shared File Systems Details page
 */

public class SharedFSDetailsViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "SharedFSDetails";
    private static final String DEFAULT_DISPLAY_URL =
            "/jsp/fs/SharedFSDetails.jsp";

    public static final String CHILD_BREADCRUMB = "BreadCrumb";

    // href for breadcrumb
    public static final String CHILD_FS_ARCH_POL_HREF = "FSArchivePolicyHref";
    public static final String CHILD_FS_SUM_HREF = "FileSystemSummaryHref";
    public static final String CHILD_STATICTEXT  = "StaticText";

    // Hidden fields used to dynamically disable drop down menu options
    public static final String CHILD_STATIC_ISSAM = "isSam";
    public static final String CHILD_HIDDEN_ISMOUNTED = "isMounted";

    // Hidden field - samfs license type
    public static final String
        CHILD_HIDDEN_LICENSE_TYPE = "LicenseTypeHiddenField";

    // Hidden field for confirm messages
    public static final String CONFIRM_MESSAGES  = "ConfirmMessages";

    // Hidden field for the fs Mount Point (Used in pop up)
    public static final String MOUNT_POINT = "MountPoint";

    private boolean writeRole = false;

    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;
    private CCBreadCrumbsModel breadCrumbsModel;

    // cc components from the corresponding jsp page(s)...
    private static final String CHILD_CONTAINER_VIEW = "SharedFSDetailsView";
    private static final String SUNPLEX_VIEW = "SunPlexManagerView";

    /**
     * Constructor
     */
    public SharedFSDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_FS_ARCH_POL_HREF, CCHref.class);
        registerChild(CHILD_FS_SUM_HREF, CCHref.class);
        registerChild(CHILD_STATIC_ISSAM, CCStaticTextField.class);
        registerChild(CHILD_HIDDEN_ISMOUNTED, CCHiddenField.class);
        registerChild(CHILD_CONTAINER_VIEW, SharedFSDetailsView.class);
        registerChild(CHILD_HIDDEN_LICENSE_TYPE, CCHiddenField.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CONFIRM_MESSAGES, CCHiddenField.class);
        registerChild(MOUNT_POINT, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(SUNPLEX_VIEW, SunPlexManagerView.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (name.equals(SUNPLEX_VIEW)) {
            return new SunPlexManagerView(this, name);
        } else if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            SharedFSDetailsView child = new SharedFSDetailsView(this, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("SharedFSDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            CCBreadCrumbs child =
                new CCBreadCrumbs(this, breadCrumbsModel, name);
            TraceUtil.trace3("Exiting");
            return child;
            // PropertySheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            TraceUtil.trace3("Exiting");
            return PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else if (name.equals(CHILD_HIDDEN_ISMOUNTED) ||
                   name.equals(CHILD_HIDDEN_LICENSE_TYPE)) {
                TraceUtil.trace3("Exiting");
                return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_FS_SUM_HREF) ||
                   name.equals(CHILD_FS_ARCH_POL_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_STATICTEXT) ||
                   name.equals(CHILD_STATIC_ISSAM)) {
            CCStaticTextField child = new CCStaticTextField(this, name, null);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CONFIRM_MESSAGES)) {
            return new CCHiddenField(this, name, new StringBuffer(
                SamUtil.getResourceString("SharedFSDetails.confirmMsg.delete")).
                append(ServerUtil.delimitor).append(
                SamUtil.getResourceString(
                    "SharedFSDetails.confirmMsg.unmount")).toString());
        } else if (name.equals(MOUNT_POINT)) {
            return new CCHiddenField(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Create the pagetitle model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        pageTitleModel =
            PageTitleUtil.createModel("/jsp/fs/SharedFSDetailsPageTitle.xml");
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String fsName = getFSName();
        TraceUtil.trace3("begin display: fsName is " + fsName);

        // Set page title model
        pageTitleModel.setPageTitleText(new StringBuffer(
            fsName).append(" ").append(
                SamUtil.getResourceString("FSDetails.Pagetitle")).toString());

        // Boolean to state whether FS is mounted or not
        boolean isMounted = false;

        // retrieve the fstype for that filesystem
        boolean sam = true;
        ArrayList contents = new ArrayList();
        int fType = FileSystem.FS_SAMQFS;
        int fsShared = FileSystem.UNSHARED;
        int numOfStripGrp = 0;

        String hostName = getServerName();

        try {
            FileSystem fs = getFileSystem();
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }

            // check dau size which is a special flag to indicate if there's
            // a problem reading superblock of the first device of this file
            // system
            if (fs.getDAUSize() == 1) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "FSDetails.error.populate",
                    -1008,
                    SamUtil.getResourceString("FSDetails.error.dauDetails"),
                    hostName);
            } else if (fs.getDAUSize() == 2) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "FSDetails.error.populate",
                    -1008,
                    SamUtil.getResourceString("FSDetails.error.badDevices"),
                    hostName);
            }

            if ((fType = fs.getFSTypeByProduct()) == FileSystem.FS_QFS) {
                sam = false;
            }
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay()",
                "Failed to get fstype",
                hostName);

            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSDetails.error.fstype",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                hostName);
        }

        // now check samfs license type
        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        Hashtable serverTable = (Hashtable) session.getAttribute(
            Constants.SessionAttributes.SAMFS_SERVER_INFO);
        if (serverTable != null && hostName != null) {
             ServerInfo serverInfo = (ServerInfo) serverTable.get(hostName);

            if (serverInfo != null &&
                serverInfo.getServerLicenseType() == SamQFSSystemModel.QFS) {
                    sam = false;
            }
        }

        loadPropertySheetModel(propertySheetModel);
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("SharedFSDetails.pageTitle1", fsName));

        // Set value of hidden fields used to dynamically disable dropdown menu
        ((CCStaticTextField) getChild(CHILD_STATIC_ISSAM)).setValue(
            sam ? "true" : "false");
        ((CCHiddenField) getChild(CHILD_HIDDEN_ISMOUNTED)).setValue(
            isMounted ? "true" : "false");

        // adding view here instead of constructor.  The reason is:
        // after finishing opeartion, we need to display this page again.
        // Constructor may not be called.
        SharedFSDetailsView view =
            (SharedFSDetailsView) getChild(CHILD_CONTAINER_VIEW);
        String sharedMember = (String) getPageSessionAttribute(
            Constants.SessionAttributes.SHARED_MEMBER);
        if (sharedMember == null) {
            sharedMember = "NO";
        }
        try {
            view.populateData();
            TraceUtil.trace3("shared member equals =" + sharedMember);
            TraceUtil.trace3("error message =" + view.partialErrMsg);

            if (view.partialErrMsg != null && sharedMember.equals("YES")) {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "SharedFSDetails.error.multihost",
                    view.partialErrMsg);
            } else if (view.partialErrMsg != null &&
                !sharedMember.equals("YES")) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "SharedFSDetails.error.multihost",
                    -10,
                    view.partialErrMsg,
                    hostName);
            }
        } catch (SamFSMultiHostException e) {
                String err_msg = SamUtil.handleMultiHostException(e);
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "SharedFSDetails.error.multihost",
                    e.getSAMerrno(),
                    err_msg,
                    hostName);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "SharedFSDetailsViewBean()",
                "Unable to populate FS Details table",
                hostName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSSummary.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                hostName);
        }
        setPageSessionAttribute(
            Constants.SessionAttributes.SHARED_MEMBER, "NO");
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create the PropertySheetModel
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            TraceUtil.trace3("Entering null");
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/SharedFSDetailsPropertySheet.xml");

            TraceUtil.trace3(
                "finished loading SharedFSDetailsPropertySheet.xml");
        }

        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    /**
     * Load the data for property sheet model
     */
    private void loadPropertySheetModel(
        CCPropertySheetModel propertySheetModel) {
        TraceUtil.trace3("Entering");
        propertySheetModel.clear();
        FileSystem fs = null;
        String cap = null;
        String free = null;
        String consumed  = null;
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
        String alarmType = null;
        String alarmCount = null;
        StringBuffer alarms = new StringBuffer();
        String noAlarm = "";
        String metadataPlacement = "";

        boolean error = false;
        // Field metadataServerOrigin will be needed in failover.
        // String metadataServerOrigin = null;
        String failoverStatus = null;
        String hostName = getServerName();
        int sharedTypeInt = -1;

        try {
            fs = getFileSystem();
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }
            point = fs.getMountPoint();
            eq = Integer.toString(fs.getEquipOrdinal());

            state = fs.getState() == FileSystem.MOUNTED ?
                "FSSummary.mount" : "FSSummary.unmount";

            switch (fs.getShareStatus()) {
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

            long capacity = fs.getCapacity();
            long freeSpace = fs.getAvailableSpace();
            consumed = Integer.toString(fs.getConsumedSpacePercentage());

            int sizeUnit = SamQFSSystemModel.SIZE_KB;
            cap = new Capacity(capacity, sizeUnit).toString();
            free = new Capacity(freeSpace, sizeUnit).toString();

            dau = Integer.toString(fs.getDAUSize());
            server = fs.getServerName();
            timeAbove = SamUtil.getTimeString(fs.getTimeAboveHWM());
            createDate = SamUtil.getTimeString(fs.getDateCreated());
            FileSystemMountProperties mountPro = fs.getMountProperties();
            hwm = Integer.toString(mountPro.getHWM());
            lwm = Integer.toString(mountPro.getLWM());

            String fsName = getFSName();

            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();

            sharedTypeInt = fsManager.getSharedFSType(hostName, fsName);

            failoverStatus =
                fsManager.failingover(hostName, fsName) ?
                    SamUtil.getResourceString(
                        "SharedFSDetails.failoverstatus.yes") :
                    SamUtil.getResourceString(
                        "SharedFSDetails.failoverstatus.no");

            // Find out how metadata is stored
            switch (fs.getFSType()) {
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
        } catch (SamFSException ex) {
            error = true;
            SamUtil.processException(
                ex,
                this.getClass(),
                "loadPropertySheetModel()",
                "failed to populate data",
                hostName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSDetails.error.populate",
                ex.getSAMerrno(),
                ex.getMessage(),
                hostName);
        }

        if (!error) {
            type = FSUtil.getFileSystemDescriptionString(fs);
            propertySheetModel.setValue("typeValue", type);
            propertySheetModel.setValue(
                "sharedFSTypeValue",
                FSUtil.getSharedFSDescriptionString(sharedTypeInt));
            propertySheetModel.setValue("mountValue", point);
            propertySheetModel.setValue("eqValue", eq);
            propertySheetModel.setValue("stateValue", state);

            ((CCHiddenField) getChild(MOUNT_POINT)).setValue(point);

            if (fs.getState() == FileSystem.UNMOUNTED) {
                propertySheetModel.setValue("capValue", "");
                propertySheetModel.setValue("freespaceValue", "");
                propertySheetModel.setValue("consumedValue", "");
            } else {
                propertySheetModel.setValue("capValue", cap);
                propertySheetModel.setValue("freespaceValue", free);
                propertySheetModel.setValue("consumedValue", consumed);
            }

            propertySheetModel.setValue("shareValue", share);
            propertySheetModel.setValue("dauValue", dau);
            propertySheetModel.setValue("timeaboveValue", timeAbove);
            propertySheetModel.setValue("createdateValue", createDate);
            propertySheetModel.setValue("serverValue", server);
            propertySheetModel.setValue("failoverStatusValue", failoverStatus);
            propertySheetModel.setValue(
                "metadataPlacementValue", metadataPlacement);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to enable the inline alert
     */
    private void showAlert(String op, String key) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            this,
            CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(op, key),
            getServerName());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to retrieve the FileSystem
     */
    private FileSystem getFileSystem() throws SamFSException {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        FileSystem fsList =
            sysModel.getSamQFSSystemFSManager().getFileSystem(getFSName());

        TraceUtil.trace3("Exiting");
        return fsList;
    }

    /**
     * Handle request for Href in actiontable
     */
    public void handleHrefRequest(RequestInvocationEvent event)
                throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        Class target = null;
        String index = null;
        index = (String) getDisplayFieldValue("Href");
        ViewBean targetView = null;

        // based on fs type, archiver policy page will be
        // forwarded if it is shared sam-qfs type
        if (index.equals("0")) {
            TraceUtil.trace3("Forward to Devices");
            target = FSDevicesViewBean.class;
        } else if (index.equals("1")) {
            TraceUtil.trace3("Forward to Archive Policies");
            target = FSArchivePoliciesViewBean.class;
        }
        targetView = getViewBean(target);


        String hostName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, hostName);
        String fsName = getFSName();
        TraceUtil.trace3("fs name is " + fsName);
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME, fsName);

        BreadCrumbUtil.breadCrumbPathForward(
            this,
            PageInfo.getPageInfo().getPageNumber(this.getName()));
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for back to fs link
     */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
                throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("FileSystemSummaryHref");
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            s);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for "FS Archive Policy"  link
     */
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent event)
                throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("FSArchivePolicyHref");
        ViewBean targetView = getViewBean(FSArchivePoliciesViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            this,
            PageInfo.getPageInfo().getPageNumber(this.getName()));
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for "FS Details"  link
     */
    public void handleSharedFSDetailsHrefRequest(RequestInvocationEvent event)
                throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("SharedFSDetailsHref");
        ViewBean targetView = getViewBean(SharedFSDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            this,
            PageInfo.getPageInfo().getPageNumber(this.getName()));
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * This method returns the file system name key from the page session
     * attribute.  It first retrieves the key by looking at the FS_NAME then
     * look at the POLICY_NAME if FS_NAME is null.  THIS IS NOT A BUG.  Default
     * policy has the same name as the file system and this is legitimate in
     * this special case.
     */
    private String getFSName() {
        String fsName = (String)
            getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
        if (fsName == null) {
            fsName = (String)
               getPageSessionAttribute(
               Constants.SessionAttributes.POLICY_NAME);
        }

        return (fsName == null ? "" : fsName);
    }
}
