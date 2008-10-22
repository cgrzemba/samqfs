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

// ident        $Id: SharedFSBean.java,v 1.21 2008/10/22 19:52:10 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.data.provider.FieldKey;
import com.sun.data.provider.RowKey;
import com.sun.data.provider.TableDataProvider;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.fs.Host;
import com.sun.netstorage.samqfs.web.model.MemberInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.SharedHostInfo;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.SharedFSFilter;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.Select;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.component.DropDown;
import com.sun.web.ui.component.Hyperlink;
import com.sun.web.ui.component.TabSet;
import com.sun.web.ui.component.TableRowGroup;
import com.sun.web.ui.model.Option;
import com.sun.web.ui.model.OptionTitle;
import java.io.Serializable;
import javax.faces.event.ActionEvent;
import javax.servlet.http.HttpServletRequest;


public class SharedFSBean implements Serializable {

    /** Holds value of time loaded for each page. */
    protected String timeSummary = null;
    protected String timeClient = null;

    /** Holds value if user has permission to perform operations. */
    protected boolean hasPermission = false;

    // Flag to indicate that an error needs to be shown, say after calling
    // an operation and fails, we do not want this alert to be reset by the
    // getData() call when the page is refreshed
    private boolean needToShowError = false;

    /** Holds value of tab set. */
    protected TabSet tabSet = null;
    /** Holds value of summary page title. */
    protected String summaryPageTitle = null;
    /** Holds value of property jumpMenuSelectedOption. */
    protected String jumpMenuSelectedOption = null;
    /** Holds value of the file system type text. */
    protected String textType = null;
    /** Holds value of the file system mount point. */
    protected String textMountPoint = null;
    /** Holds value of the file system capacity. */
    protected String imageUsage = null;
    protected String textCapacity = null;
    /** Holds value of the file system high water mark. */
    protected String textHWM = null;
    /** Holds value of the page title of clients section. */
    protected String titleClients = null;
    /** Holds value of the client table row group. */
    protected TableRowGroup clientTableRowGroup = null;
    /** Holds value of the render value of archive related components. */
    protected boolean showArchive = false;
    /** Holds value of the mount status of the file system. */
    protected boolean mounted = true;
    /** Holds value of alert info. */
    protected boolean alertRendered = false;
    protected String alertType = null;
    protected String alertDetail = null;
    protected String alertSummary = null;

    /** Hidden fields for javascript messages */
    protected String noMultipleOpMsg = null;
    protected String noneSelectedMsg = null;
    protected String confirmRemove = null;
    protected String confirmDisable = null;
    protected String confirmUnmount = null;
    protected String confirmUnmountFS = null;

    /** Holds table information in Client Summary Page. */
    protected String clientPageTitle = null;
    protected TableRowGroup clientSummaryTableRowGroup = null;
    protected Select selectClient = new Select("clientTable");
    protected String clientTableSelectedOption = null;
    protected String clientTableFilterSelectedOption = null;

    /** Hidden field for javascript */
    protected String hiddenServerName = null;
    protected String hiddenFSName = null;

    private SharedFSTabBean tabBean = null;
    private SharedFSSummaryBean summaryBean = null;
    private SharedFSClientBean clientBean = null;

    public static final String PARAM_FILTER = "filter";

    public SharedFSBean() {
        this.tabBean = new SharedFSTabBean();
        this.summaryBean = new SharedFSSummaryBean();
        this.clientBean = new SharedFSClientBean();

        hasPermission =
            SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.CONFIG);

        // Call to the backend for first time page display
        getSummaryData();

        populateHiddenFields();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Remote Calls

    private void getSummaryData() {
        String fsName = getFSName();
        String serverName = JSFUtil.getServerName();

        TraceUtil.trace3("REMOTE call: getSummaryData()");
        TraceUtil.trace3("serverName: " + serverName + " fsName: " + fsName);

        timeSummary = Long.toString(System.currentTimeMillis());

        if (serverName != null) {
            try {
                SamQFSSystemSharedFSManager sharedFSManager =
                                                getSharedFSManager();
                MemberInfo [] infos = sharedFSManager.getSharedFSSummaryStatus(
                            serverName, fsName);
                FileSystem thisFS = getFileSystem();
                showArchive = thisFS.getArchivingType() == FileSystem.ARCHIVING;
                mounted = thisFS.getState() == FileSystem.MOUNTED;

                summaryBean.populateSummary(infos, thisFS);

                if (!needToShowError) {
                    this.alertRendered = false;
                }

            } catch (SamFSException sfe) {
                TraceUtil.trace1("SamFSException caught!", sfe);
                LogUtil.error(this, sfe);
                sfe.printStackTrace();
                SamUtil.processException(
                    sfe,
                    this.getClass(),
                    "getSummaryData",
                    sfe.getMessage(),
                    serverName);
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                        "SharedFS.message.populate.summary.failed"),
                    sfe.getMessage());
            }
            needToShowError = false;
        }
    }

    /**
     * Remote call and get client/storage node data
     *
     * @param filter page filter short number
     */
    private void getData() {
        String fsName = getFSName();
        String serverName = JSFUtil.getServerName();

        TraceUtil.trace3("REMOTE call: getData()");
        TraceUtil.trace3("serverName: " + serverName + " fsName: " + fsName);

        if (serverName != null) {
            try {
                SamQFSSystemSharedFSManager sharedFSManager =
                                                getSharedFSManager();
                SharedFSFilter [] filters = getFilters();
                SharedHostInfo [] infos =
                    sharedFSManager.getSharedFSHosts(
                        serverName,
                        fsName,
                        Host.MDS | Host.CLIENTS,
                        filters);
                TraceUtil.trace3("info length: " +
                    (infos == null ? "0" : Integer.toString(infos.length)));

                clientBean.populate(infos, showArchive, filters);
                timeClient = Long.toString(System.currentTimeMillis());

                if (!needToShowError) {
                    this.alertRendered = false;
                }

            } catch (SamFSException sfe) {
                TraceUtil.trace1("SamFSException caught!", sfe);
                LogUtil.error(this, sfe);
                sfe.printStackTrace();
                SamUtil.processException(
                    sfe,
                    this.getClass(),
                    "getData",
                    sfe.getMessage(),
                    serverName);
                setAlertInfo(
                    Constants.Alert.ERROR,
                    JSFUtil.getMessage(
                            "SharedFS.message.populate.client.failed"),
                    sfe.getMessage());
            }
            needToShowError = false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Permission
    public boolean isHasPermission() {
        return hasPermission;
    }

    public void setHasPermission(boolean hasPermission) {
        this.hasPermission = hasPermission;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Time when page is loaded
    public String getTimeSummary() {
        return timeSummary;
    }

    public void setTimeSummary(String timeSummary) {
        this.timeSummary = timeSummary;
    }

    public String getTimeClient() {
        return timeClient;
    }

    public void setTimeClient(String timeClient) {
        this.timeClient = timeClient;
    }

    public String getSummaryPageTitle() {
        summaryPageTitle = JSFUtil.getMessage("SharedFS.title", getFSName());
        return summaryPageTitle;
    }

    public void setSummaryPageTitle(String summaryPageTitle) {
        this.summaryPageTitle = summaryPageTitle;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Tab Set
    public TabSet getTabSet() {
        this.tabSet = tabBean.getMyTabSet();
        return tabSet;
    }
    public void setTabSet(TabSet tabSet){
        this.tabSet = tabSet;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Breadcrumbs
    public Hyperlink [] getBreadCrumbsSummary(){
        return summaryBean.getBreadCrumbs();
    }

    public Hyperlink [] getBreadCrumbsClient(){
        return clientBean.getBreadCrumbs(getFSName());
    }

    ////////////////////////////////////////////////////////////////////////////
    // Alert

    public String getAlertDetail() {
        if (!alertRendered) {
            alertDetail = null;
        }
        return alertDetail;
    }

    public void setAlertDetail(String alertDetail) {
        this.alertDetail = alertDetail;
    }

    public String getAlertSummary() {
        if (!alertRendered) {
            alertSummary = null;
        }
        return alertSummary;
    }

    public void setAlertSummary(String alertSummary) {
        this.alertSummary = alertSummary;
    }

    public String getAlertType() {
        if (!alertRendered){
            alertType = Constants.Alert.INFO;
        }
        return alertType;
    }

    public void setAlertType(String alertType) {
        this.alertType = alertType;
    }

    public boolean isAlertRendered() {
        return alertRendered;
    }

    public void setAlertRendered(boolean alertRendered) {
        this.alertRendered = alertRendered;
    }

    public void setAlertInfo(String type, String summary, String detail) {
        if (!Constants.Alert.INFO.equals(type)) {
            needToShowError = true;
        }
        alertRendered = true;
        this.alertType = type;
        this.alertSummary = summary;
        this.alertDetail = JSFUtil.getMessage(detail);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Property Sheet Components

    /** Return the array of jump menu options */
    public Option[] getJumpMenuOptions() {
        // Call to the backend
        getSummaryData();

        return summaryBean.getJumpMenuOptions(mounted);
    }

    /** Get the value of property jumpMenuSelectedOption */
    public String getJumpMenuSelectedOption() {
        return this.jumpMenuSelectedOption;
    }

    /** Set the value of property jumpMenuSelectedOption */
    public void setJumpMenuSelectedOption(String jumpMenuSelectedOption) {
        this.jumpMenuSelectedOption = jumpMenuSelectedOption;
    }

    public void handleViewPolicies(ActionEvent event) {

        String params =
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME
                 + "=" + JSFUtil.getServerName();

        params += "&" + Constants.PageSessionAttributes.FILE_SYSTEM_NAME
             + "=" + getFSName();

        // Check util/PageInfo.java, 36 is the number for SharedFSSummary
        params += "&" + Constants.SessionAttributes.PAGE_PATH + "=" +
            // FS Summary
            PageInfo.getPageInfo().
                getPageNumber(FSSummaryViewBean.PAGE_NAME).toString() +
            "," +
            // Shared FS Summary
            PageInfo.getPageInfo().
                getPageNumber(SharedFSSummaryBean.PAGE_NAME).toString();
        JSFUtil.redirectPage("/fs/FSArchivePolicies", params);
    }

    public void handleViewDevices(ActionEvent event) {
        String params =
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME
                 + "=" + JSFUtil.getServerName();

        params += "&" + Constants.PageSessionAttributes.FILE_SYSTEM_NAME
             + "=" + getFSName();

        // Construct necessary information for breadcrumb links
        params += "&" + Constants.SessionAttributes.PAGE_PATH + "=" +
            // FS Summary
            PageInfo.getPageInfo().
                getPageNumber(FSSummaryViewBean.PAGE_NAME).toString() +
            "," +
            // Shared FS Summary
            PageInfo.getPageInfo().
                getPageNumber(SharedFSSummaryBean.PAGE_NAME).toString();
        JSFUtil.redirectPage("/fs/FSDevices", params);
    }

    public void handleJumpMenuSelection(ActionEvent event) {
        DropDown dropDown = (DropDown) event.getComponent();
        String selected = (String) dropDown.getSelected();

        if (summaryBean.menuOptions[0][1].equals(selected)){
            forwardToMountOptionsPage();
        } else if (summaryBean.menuOptions[1][1].equals(selected)) {
            executeCommand(true);
        } else if (summaryBean.menuOptions[2][1].equals(selected)) {
            executeCommand(false);
        } else if (summaryBean.menuOptions[3][1].equals(selected)) {
            forwardToClientPage(SharedFSFilter.FILTER_NONE);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Property Sheet Section ONE

    public String getTextHWM() {
        return summaryBean.getTextHWM();
    }

    public void setTextHWM(String textHWM) {
        this.textHWM = textHWM;
    }

    public String getTextCapacity() {
        return summaryBean.getTextCapacity();
    }

    public void setTextCapacity(String textCapacity) {
        this.textCapacity = textCapacity;
    }

    public String getImageUsage() {
        return summaryBean.getImageUsage();
    }

    public void setImageUsage(String imageUsage) {
        this.imageUsage = imageUsage;
    }

    public String getTextMountPoint() {
        return summaryBean.getTextMountPoint();
    }

    public void setTextMountPoint(String textMountPoint) {
        this.textMountPoint = textMountPoint;
    }

    public String getTextType() {
        return summaryBean.getTextType();
    }

    public void setTextType(String textType) {
        this.textType = textType;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Property Sheet Client Table

    public String getTitleClients() {
        return summaryBean.getTitleClients();
    }

    public void setTitleClients(String titleClients) {
        this.titleClients = titleClients;
    }

    public TableDataProvider getClientList() {
        return summaryBean.getClientList();
    }

    public TableRowGroup getClientTableRowGroup() {
        return clientTableRowGroup;
    }

    public void setClientTableRowGroup(TableRowGroup clientTableRowGroup) {
        this.clientTableRowGroup = clientTableRowGroup;
    }

    public String getClientPageTitle() {
        clientPageTitle = clientBean.getPageTitle(getFSName());
        return clientPageTitle;
    }

    public void setClientPageTitle(String clientPageTitle) {
        this.clientPageTitle = clientPageTitle;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Helper methods

    private String getFSName() {
        return FSUtil.getFSName();
    }

    private FileSystem getFileSystem() throws SamFSException {
        String fsName = getFSName();

        TraceUtil.trace3("Remote Call: Retrieve fs object: " + fsName);

        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        if (appModel == null) {
            throw new SamFSException("App model is null");
        }
        SamQFSSystemModel sysModel = SamUtil.getModel(JSFUtil.getServerName());
        return sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
    }

    private SamQFSSystemSharedFSManager getSharedFSManager()
        throws SamFSException {

        TraceUtil.trace3("Remote Call: Retrieve shared fs mgr obj!");

        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();

        if (appModel == null) {
            throw new SamFSException("App model is null");
        }

        SamQFSSystemSharedFSManager sharedFSManager =
                        appModel.getSamQFSSystemSharedFSManager();
        if (sharedFSManager == null) {
            throw new SamFSException("shared fs manager is null");
        }
        return sharedFSManager;
    }

    private void forwardToMountOptionsPage() {
        // TODO: NEED TO pass more attributes for shared fs
        String params =
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME
                 + "=" + JSFUtil.getServerName();
        params += "&" + Constants.PageSessionAttributes.FILE_SYSTEM_NAME
             + "=" + getFSName();
        params += "&" + Constants.PageSessionAttributes.ARCHIVE_TYPE
             + "=" + (isShowArchive() ? FileSystem.ARCHIVING :
                                        FileSystem.NONARCHIVING);
        // TODO: Need to pass the correct attribute for shared fs
        /*
        params += "&" + Constants.SessionAttributes.MOUNT_PAGE_TYPE
             + "=" + (isShowArchive()? FSMountViewBean.TYPE_SHAREDQFS :
                                      FSMountViewBean.TYPE_SHAREDSAMQFS);
         */
        params += "&" + Constants.SessionAttributes.MOUNT_PAGE_TYPE
             + "=" + (isShowArchive()? FSMountViewBean.TYPE_UNSHAREDQFS :
                                      FSMountViewBean.TYPE_UNSHAREDSAMQFS);
        params += "&" + Constants.SessionAttributes.SHARED_CLIENT_HOST
             + "=" + "PUT_SOMETHING";
        // Check util/PageInfo.java, 36 is the number for SharedFSSummary
        params += "&" + Constants.SessionAttributes.PAGE_PATH + "=" +
            // FS Summary
            PageInfo.getPageInfo().
                getPageNumber(FSSummaryViewBean.PAGE_NAME).toString() +
            "," +
            // Shared FS Summary
            PageInfo.getPageInfo().
                getPageNumber(SharedFSSummaryBean.PAGE_NAME).toString();

        JSFUtil.redirectPage("/fs/FSMount", params);
    }

    private void forwardToClientPage(String filter) {
        JSFUtil.redirectPage(
            "/faces/jsp/fs/SharedFSClient.jsp",
            PARAM_FILTER + "=" + filter);
    }

    private void executeCommand(boolean mount){
        try {
            // Check if user has permission to perform this operation
            if (!hasPermission) {
                throw new SamFSException("common.nopermission");
            }

            SamQFSSystemSharedFSManager sharedFSManager =
                                                getSharedFSManager();

            if (mount) {
                LogUtil.info(
                    this.getClass(),
                    "executeCommand",
                    "Start mounting shared fs: " + getFSName());

                sharedFSManager.mountClients(
                    JSFUtil.getServerName(),
                    getFSName(),
                    new String [] {JSFUtil.getServerName()});

                LogUtil.info(
                    this.getClass(),
                    "executeCommand",
                    "Done mounting shared fs: " + getFSName());
            } else {
                LogUtil.info(
                    this.getClass(),
                    "executeCommand",
                    "Start un-mounting shared fs: " + getFSName());

                sharedFSManager.unmountClients(
                    JSFUtil.getServerName(),
                    getFSName(),
                    new String [] {JSFUtil.getServerName()});

                LogUtil.info(
                    this.getClass(),
                    "executeCommand",
                    "Done un-mounting shared fs: " + getFSName());
            }
            setAlertInfo(
                Constants.Alert.INFO,
                mount ?
                    JSFUtil.getMessage("SharedFS.message.mount.ok",
                                       getFSName()):
                    JSFUtil.getMessage("SharedFS.message.unmount.ok",
                                       getFSName()),
                null);

        } catch (SamFSException samEx){
            TraceUtil.trace1("SamFSException caught!", samEx);
            LogUtil.error(this, samEx);
            SamUtil.processException(
                samEx,
                this.getClass(),
                "executeCommand",
                samEx.getMessage(),
                JSFUtil.getServerName());
            setAlertInfo(
                Constants.Alert.ERROR,
                mount ?
                    JSFUtil.getMessage("SharedFS.message.mount.failed",
                                       getFSName()):
                    JSFUtil.getMessage("SharedFS.message.unmount.failed",
                                       getFSName()),
                samEx.getMessage());
        }
    }

    private void populateHiddenFields() {
        noMultipleOpMsg = JSFUtil.getMessage("common.morethanoneselected");
        noneSelectedMsg = JSFUtil.getMessage("common.noneselected");
        confirmRemove = JSFUtil.getMessage("SharedFS.message.remove");
        confirmDisable = JSFUtil.getMessage("SharedFS.message.disable");
        confirmUnmount = JSFUtil.getMessage("SharedFS.message.unmount");
        confirmUnmountFS = JSFUtil.getMessage("SharedFS.message.unmountfs");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Bean getters and setters

    public boolean isShowArchive() {
        return showArchive;
    }

    public void setShowArchive(boolean showArchive) {
        this.showArchive = showArchive;
    }

    public boolean isMounted() {
        return mounted;
    }

    public void setMounted(boolean mounted) {
        this.mounted = mounted;
    }

    public String getNoMultipleOpMsg() {
        return noMultipleOpMsg;
    }

    public void setNoMultipleOpMsg(String noMultipleOpMsg) {
        this.noMultipleOpMsg = noMultipleOpMsg;
    }

    public String getNoneSelectedMsg() {
        return noneSelectedMsg;
    }

    public void setNoneSelectedMsg(String noneSelectedMsg) {
        this.noneSelectedMsg = noneSelectedMsg;
    }

    public String getConfirmDisable() {
        return confirmDisable;
    }

    public void setConfirmDisable(String confirmDisable) {
        this.confirmDisable = confirmDisable;
    }

    public String getConfirmRemove() {
        return confirmRemove;
    }

    public void setConfirmRemove(String confirmRemove) {
        this.confirmRemove = confirmRemove;
    }

    public String getConfirmUnmount() {
        return confirmUnmount;
    }

    public void setConfirmUnmount(String confirmUnmount) {
        this.confirmUnmount = confirmUnmount;
    }

    public String getConfirmUnmountFS() {
        return confirmUnmountFS;
    }

    public void setConfirmUnmountFS(String confirmUnmountFS) {
        this.confirmUnmountFS = confirmUnmountFS;
    }

    public String getHiddenServerName() {
        hiddenServerName = JSFUtil.getServerName();
        return hiddenServerName;
    }

    public void setHiddenServerName(String hiddenServerName) {
        this.hiddenServerName = hiddenServerName;
    }

    public String getHiddenFSName() {
        hiddenFSName = FSUtil.getFSName();
        return hiddenFSName;
    }

    public void setHiddenFSName(String hiddenFSName) {
        this.hiddenFSName = hiddenFSName;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Client Summary Page Methods (SharedFSClient.jsp)
    public String getClientTableTitle() {
        return clientBean.getClientTableTitle();
    }

    public TableDataProvider getClientSummaryList() {
        getData();
        return clientBean.getClientList();
    }

    public TableRowGroup getClientSummaryTableRowGroup() {
        return clientSummaryTableRowGroup;
    }

    public void setClientSummaryTableRowGroup(
        TableRowGroup clientSummaryTableRowGroup) {
        this.clientSummaryTableRowGroup = clientSummaryTableRowGroup;
    }

    public void handleRemoveClient(ActionEvent event) {
        String [] selectedClients = getSelectedKeys();

        try {
            SamQFSSystemSharedFSManager sharedFSManager =
                                                getSharedFSManager();
            LogUtil.info(
                    this.getClass(),
                    "handleRemoveClient",
                    "Start removing client for shared fs: " + getFSName() +
                    " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

            sharedFSManager.removeClients(JSFUtil.getServerName(),
                                          getFSName(),
                                          selectedClients);
            LogUtil.info(
                    this.getClass(),
                    "handleRemoveClient",
                    "Done removing client for shared fs: " + getFSName() +
                    " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));


            setAlertInfo(
                Constants.Alert.INFO,
                JSFUtil.getMessage(
                    "SharedFS.message.removeclients.ok",
                    ConversionUtil.arrayToStr(selectedClients, ',')),
                null);

            // Reset Filter
            resetFilter();

        } catch (SamFSException samEx) {
            TraceUtil.trace1("SamFSException caught!", samEx);
            LogUtil.error(this, samEx);
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleRemoveClient",
                samEx.getMessage(),
                JSFUtil.getServerName());
            setAlertInfo(
                Constants.Alert.ERROR,
                JSFUtil.getMessage(
                    "SharedFS.message.removeclients.failed",
                    ConversionUtil.arrayToStr(selectedClients, ',')),
                samEx.getMessage());
        }
    }

    public Select getSelectClient() {
        return selectClient;
    }

    public void setSelectClient(Select selectClient) {
        this.selectClient = selectClient;
    }

    public String getClientTableSelectedOption() {
        return clientTableSelectedOption;
    }

    public void setClientTableSelectedOption(String clientTableSelectedOption) {
        this.clientTableSelectedOption = clientTableSelectedOption;
    }

    public String getClientTableFilterSelectedOption() {
        this.clientTableFilterSelectedOption =
            clientBean.getClientTableFilterSelectedOption();
        return clientTableFilterSelectedOption;
    }

    public void setClientTableFilterSelectedOption(
        String clientTableFilterSelectedOption) {
        this.clientTableFilterSelectedOption = clientTableFilterSelectedOption;
    }

    /** Return the array of jump menu options */
    public Option[] getClientTableMenuOptions() {
        return clientBean.getJumpMenuOptions();
    }

    /** Return the array of filter menu options */
    public Option[] getClientTableFilterOptions() {
        return clientBean.getFilterMenuOptions();
    }

    public String getParamCriteriaAll() {
        return SharedFSFilter.FILTER_NONE;
    }

    public String getParamCriteriaOk() {
        return SharedFSFilter.FILTER_OK;
    }

    public String getParamCriteriaUnmounted() {
        return SharedFSFilter.FILTER_UNMOUNTED;
    }

    public String getParamCriteriaOff() {
        return SharedFSFilter.FILTER_DISABLED;
    }

    public String getParamCriteriaInError() {
        return SharedFSFilter.FILTER_IN_ERROR;
    }

    public void handleTableMenuSelection(ActionEvent event) {
        String [] selectedClients = getSelectedKeys();
        String selected = getClientTableSelectedOption();

        TraceUtil.trace3("handleTableMenuSelection: selected: " + selected);

        if (clientBean.menuOptions[0][1].equals(selected)) {
            // Reset Filter
            JSFUtil.removeAttribute(PARAM_FILTER);

            forwardToMountOptionsPage();
            return;
        }

        String message = "", errorMessage = "";

        try {
            SamQFSSystemSharedFSManager sharedFSManager =
                                                getSharedFSManager();
            // mount
            if (clientBean.menuOptions[1][1].equals(selected)) {
                message = "SharedFS.message.mountclients.ok";
                errorMessage = "SharedFS.message.mountclients.failed";
                if (!hasPermission) {
                    TraceUtil.trace1(
                        "User has no permission to mount fs " + getFSName());
                    throw new SamFSException("common.nopermission");
                }
                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Start mounting clients for shared fs: " + getFSName() +
                    " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

                int jobId = sharedFSManager.mountClients(
                    JSFUtil.getServerName(), getFSName(), selectedClients);
System.out.println("Mounting Clients Job ID: " + jobId);
                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Done mounting clients for shared fs: " + getFSName() +
                    " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

            // unmount
            } else if (clientBean.menuOptions[2][1].equals(selected)) {
                message = "SharedFS.message.unmountclients.ok";
                errorMessage = "SharedFS.message.unmountclients.failed";
                if (!hasPermission) {
                    TraceUtil.trace1(
                        "User has no permission to unmount fs " + getFSName());
                    throw new SamFSException("common.nopermission");
                }
                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Start un-mounting clients for shared fs: " + getFSName() +
                    " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

                int jobId = sharedFSManager.unmountClients(
                    JSFUtil.getServerName(), getFSName(), selectedClients);
System.out.println("Unounting Clients Job ID: " + jobId);
                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Done un-mounting clients for shared fs: " + getFSName() +
                    " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

            // Enable Access
            } else if (clientBean.menuOptions[3][1].equals(selected)) {
                message = "SharedFS.message.enableclients.ok";
                errorMessage = "SharedFS.message.enableclients.failed";
                if (!hasPermission) {
                    TraceUtil.trace1(
                        "User has no permission to enable access fs " +
                        getFSName());
                    throw new SamFSException("common.nopermission");
                }
                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Start enabling access clients for shared fs: " +
                    getFSName() + " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

                sharedFSManager.setClientState(
                    JSFUtil.getServerName(), getFSName(),
                    selectedClients, true);

                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Done enabling access clients for shared fs: " +
                    getFSName() + " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

            // Disable Access
            } else if (clientBean.menuOptions[4][1].equals(selected)) {
                message = "SharedFS.message.disableclients.ok";
                errorMessage = "SharedFS.message.disableclients.failed";
                if (!hasPermission) {
                    TraceUtil.trace1(
                        "User has no permission to disable access fs " +
                        getFSName());
                    throw new SamFSException("common.nopermission");
                }
                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Start disabling access clients for shared fs: " +
                    getFSName() + " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

                sharedFSManager.setClientState(
                    JSFUtil.getServerName(), getFSName(),
                    selectedClients, false);

                LogUtil.info(
                    this.getClass(),
                    "handleTableMenuSelection",
                    "Done disabling access clients for shared fs: " +
                    getFSName() + " Clients: " +
                    ConversionUtil.arrayToStr(selectedClients, ','));

            } else {
                // Should never happen!
                TraceUtil.trace1("Client Bean: Unknown operation: " + selected);
            }

            setAlertInfo(
                Constants.Alert.INFO,
                JSFUtil.getMessage(
                    message,
                    ConversionUtil.arrayToStr(selectedClients, ',')),
                null);

            // Reset Filter
            resetFilter();

        } catch (SamFSException samEx) {
            TraceUtil.trace1("SamFSException caught!", samEx);
            LogUtil.error(this, samEx);
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleTableMenuSelection",
                samEx.getMessage(),
                JSFUtil.getServerName());
            setAlertInfo(
                Constants.Alert.ERROR,
                JSFUtil.getMessage(
                    errorMessage,
                    ConversionUtil.arrayToStr(selectedClients, ',')),
                samEx.getMessage());
        }

        // reset menu
        clientTableSelectedOption = OptionTitle.NONESELECTED;
    }

    public void handleFilterChange(ActionEvent event) {
        DropDown dropDown = (DropDown) event.getComponent();
        String selected = (String) dropDown.getSelected();
        forwardToClientPage(selected);
    }

    /**
     * Retrieve the selected row keys form the table
     * @return an array of Strings that hold the keys of selected rows
     */
    protected String [] getSelectedKeys() {
        TableDataProvider provider = getClientSummaryList();
        RowKey [] rows = getClientSummaryTableRowGroup().getSelectedRowKeys();

        TraceUtil.trace3(
            "RowKeys selected: " + (rows == null ? 0 : rows.length));
System.out.println("RowKeys selected: " + (rows == null ? 0 : rows.length));
        String [] selected = null;
        if (rows != null && rows.length >= 1) {
            selected = new String[rows.length];

            FieldKey field = provider.getFieldKey("name");
            for (int i = 0; i < rows.length; i++) {
                selected[i] = (String) provider.getValue(field, rows[i]);
                TraceUtil.trace3("Selected: " + selected[i]);
            }
        }

        // safe to clear the selection
        getSelectClient().clear();

        return selected;
    }

    /**
     * Method to retrieve filter criteria from the request, and save it in the
     * session.
     */
    public SharedFSFilter [] getFilters() {
        HttpServletRequest request = JSFUtil.getRequest();
        String criteria = request.getParameter(PARAM_FILTER);

System.out.println("getFilters: request criteria:" + criteria + "end");
        // criteria is null, means request contains no param
        if (criteria == null) {
            criteria = (String) JSFUtil.getAttribute(PARAM_FILTER);
            if (criteria == null || criteria.length() == 0) {
                return new SharedFSFilter[0];
            } else {
                return convertStrToFilter(criteria);
            }
        } else if (criteria.length() == 0) {
            return new SharedFSFilter[0];
        } else {
            JSFUtil.setAttribute(PARAM_FILTER, criteria);
            return convertStrToFilter(criteria);
        }
    }

    private void resetFilter() {
        JSFUtil.removeAttribute(PARAM_FILTER);

        // safe to clear the selection
        getSelectClient().clear();
    }

    private SharedFSFilter [] convertStrToFilter(String criteria) {
        String [] filterArr =
            criteria.split(SharedFSFilter.FILTER_DELIMITOR);
        SharedFSFilter [] filters =
            new SharedFSFilter[filterArr.length];
        for (int i = 0; i < filterArr.length; i++) {
            filters[i] = new SharedFSFilter(filterArr[i]);
        }
        return filters;
    }
}
