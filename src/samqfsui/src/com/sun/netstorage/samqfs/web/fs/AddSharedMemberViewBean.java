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

// ident	$Id: AddSharedMemberViewBean.java,v 1.32 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Add Server page
 */

public class AddSharedMemberViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "AddSharedMember";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/fs/AddSharedMember.jsp";

    private static final String HOST_NAMES = "HostNames";
    private static final String IP_ADDRESSES   = "IPAddresses";
    private static final String ERROR_MESSAGES = "ErrorMessages";

    // Page Title / Property Sheet Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;

    /**
     * Constructor
     */
    public AddSharedMemberViewBean() {
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
        registerChild(HOST_NAMES, CCHiddenField.class);
        registerChild(IP_ADDRESSES, CCHiddenField.class);
        registerChild(ERROR_MESSAGES, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
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
        View child = null;

        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // Hidden field to keep track of IP addresses and hosts
        } else if (name.equals(IP_ADDRESSES) ||
            name.equals(HOST_NAMES) ||
            name.equals(ERROR_MESSAGES)) {
            child = new CCHiddenField(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // Property Sheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child =
                PropertySheetUtil.createChild(this, propertySheetModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Create Page Title Model
     * @return page title model of the page
     */

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/AddSharedMemberPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Create Property Sheet Model
     * @return property sheet model of the page
     */

    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        if (propertySheetModel == null) {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/fs/AddSharedMemberPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);

        // populate page title
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("AddSharedMember.pageTitle",
                new String [] {
                    getFSName()}));

        // prepopulate fields with default values if the fields are blank
        prepopulateFields();

        // populate host name drop down, and IP address hidden fields
        populateHostInformation();

        // populate javascript error messages
        populateJavascriptMessages();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Prepopulate type radio buttons and mount point if there is no value
     * in both fields
     */
    private void prepopulateFields() {
        // Prepopulate type radio buttons and mount point
        String mountPoint =
            (String) propertySheetModel.getValue("MountPointValue");
        String type =
             (String) propertySheetModel.getValue("MemberType");

        if (type == null || type.length() == 0) {
            // set to client
            propertySheetModel.setValue("MemberType", "client");
        }

        propertySheetModel.setValue("MountPointValue", getMountPoint());

        try {
            propertySheetModel.setValue(
                "typeinlinehelp",
                SamUtil.getResourceString("AddSharedMember.inlinehelp.type",
                    new String [] {
                        new StringBuffer(
                            getMDSName()).append(" (").append(
                            getMDSHostArchitecture()).append(")").toString()}));
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to retrieve MDS information of fs " + getFSName());
            propertySheetModel.setValue(
                "typeinlinehelp",
                SamUtil.getResourceString("AddSharedMember.inlinehelp.type",
                new String [] {
                    SamUtil.getResourceString("filesystem.desc.unknown")}));
        }
    }

    /**
     * Hide the mount after add check box if the file system is not mounted
     * in the Metadata server
     */
    public boolean beginMountAfterAddValueDisplay(ChildDisplayEvent event) {
        return isMDSMounted();
    }


    /**
     * Helper function to retrieve file system name from the request or
     * the page session depending on the display cycle
     */

    private String getFSName() {
        // first check the page session
        String fsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);

        // second check the request
        if (fsName == null) {
            fsName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.FS_NAME);

            if (fsName != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.FS_NAME,
                    fsName);
            } else {
                throw new IllegalArgumentException(
                    "File System name not supplied");
            }
        }

        return fsName;
    }


    /**
     * Helper function to retrieve mount point from the request or
     * the page session depending on the display cycle
     */

    private String getMountPoint() {
        // first check the request
        String mountPoint = RequestManager.getRequest().getParameter(
            Constants.PageSessionAttributes.MOUNT_POINT);
        if (mountPoint != null) {
            setPageSessionAttribute(
                Constants.PageSessionAttributes.MOUNT_POINT,
                mountPoint);
        } else {
            mountPoint = (String) getPageSessionAttribute(
                Constants.PageSessionAttributes.MOUNT_POINT);
        }

        return mountPoint;
    }

    private boolean isMDSMounted() {
        // first check the page session
        String mdsMountedStr = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.MDS_MOUNTED);

        // second check the request
        if (mdsMountedStr == null) {
            mdsMountedStr = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.MDS_MOUNTED);

            if (mdsMountedStr != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.MDS_MOUNTED,
                    mdsMountedStr);
            } else {
                throw new IllegalArgumentException(
                    "File System name not supplied");
            }
        }

        return Boolean.valueOf(mdsMountedStr).booleanValue();
    }

    /**
     * Populate javascript error messages for validation in a hidden field
     */
    private void populateJavascriptMessages() {
        ((CCHiddenField) getChild(ERROR_MESSAGES)).setValue(
            new StringBuffer(
                SamUtil.getResourceString("AddSharedMember.error.choosehost")).
                    append("###").append(SamUtil.getResourceString(
                    "AddSharedMember.error.checksecondaryip")).append("###").
                    append(SamUtil.getResourceString(
                        "AddSharedMember.error.needmountpoint")).toString());
    }

    /**
     * Populate host name drop down, and hidden fields for ip address so
     * the Primary/Secondary IP Drop Down can be dynamically populated in the
     * client side
     */
    private void populateHostInformation() {
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            String [] hostsNotInUsed =
                fsManager.getHostsNotUsedBy(getFSName(), getServerName());

            populateHostNames(hostsNotInUsed);
            populateIPAddresses(fsManager, hostsNotInUsed);

        } catch (SamFSMultiHostException multiEx) {
            TraceUtil.trace1("SamFSMultiHostException is caught!");
            TraceUtil.trace1("Reason: " + multiEx.getMessage());

            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "AddSharedMember.error.populate.hostname",
                multiEx.getSAMerrno(),
                SamUtil.handleMultiHostException(multiEx),
                getServerName());
        } catch (SamFSException samEx) {
            TraceUtil.trace1("SamFSException is caught!");
            TraceUtil.trace1("Reason: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "populateHostNames",
                "Failed to retrieve unused hosts information!",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "AddSharedMember.error.populate.hostname",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());

        } catch (Exception ex) {
            // should never fell in this case
            TraceUtil.trace1("Unknown Exception caught!");
            TraceUtil.trace1("Reason: " + ex.getMessage());
            SamUtil.setWarningAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "error.unexpected",
                "Unknown error occurred");
        }
    }

    private void populateHostNames(
        String [] hostsNotInUsed)
        throws SamFSMultiHostException, SamFSException, Exception {

        printDebugMessages(hostsNotInUsed);

        CCDropDownMenu hostNameDropDown =
            (CCDropDownMenu) getChild("HostDropDownValue");
        hostNameDropDown.setLabelForNoneSelected(
            "AddSharedMember.dropdown.hostname.noneselect");

        StringBuffer entryDelimitor  = new StringBuffer("###");
        StringBuffer marker = new StringBuffer("!!!");
        StringBuffer bufForHostNames = new StringBuffer();

        for (int i = 0; i < hostsNotInUsed.length; i++) {
            if (i != 0) {
                bufForHostNames.append(entryDelimitor);
            }
            if (markedEntry(hostsNotInUsed[i])) {
                bufForHostNames.append(marker);
            }
            bufForHostNames.append(hostsNotInUsed[i]);
        }
        ((CCHiddenField) getChild(HOST_NAMES)).
            setValue(bufForHostNames.toString());
    }

    private void populateIPAddresses(
        SamQFSSystemSharedFSManager fsManager,
        String [] hostsNotInUsed)
        throws SamFSMultiHostException, SamFSException, Exception {

        StringBuffer entryDelimitor = new StringBuffer("###");
        StringBuffer ipDelimitor = new StringBuffer(",");
        StringBuffer bufForIP = new StringBuffer();

        for (int i = 0; i < hostsNotInUsed.length; i++) {
            if (i != 0) {
                bufForIP.append(entryDelimitor);
            }
            String [] ipAddresses = fsManager.getIPAddresses(hostsNotInUsed[i]);
            bufForIP.append(hostsNotInUsed[i]).append(ipDelimitor);
            bufForIP.append(createIPString(ipAddresses, ipDelimitor));
        }

        ((CCHiddenField) getChild(IP_ADDRESSES)).setValue(bufForIP.toString());
    }

    private StringBuffer createIPString(
        String [] ipAddresses, StringBuffer ipDelimitor) {

        StringBuffer buf = new StringBuffer();
        if (ipAddresses == null) {
            TraceUtil.trace3("Calling fsManager.getIPAddresses returns null!");
            return buf;
        }

        for (int i = 0; i < ipAddresses.length; i++) {
            if (i != 0) {
                buf.append(ipDelimitor);
            }
            buf.append(ipAddresses[i]);
        }

        return buf;
    }


    private void printDebugMessages(String [] hostsNotInUsed) {
        StringBuffer buf =
            new StringBuffer("Total Number of Hosts NOT in used: ");
        for (int i = 0; i < hostsNotInUsed.length; i++) {
            if (i + 1 == hostsNotInUsed.length) {
                buf.append(hostsNotInUsed[i]).append("<end>");
            } else {
                buf.append(hostsNotInUsed[i]).append(",");
            }
        }
        TraceUtil.trace3(buf.toString());
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        // boolean to keep track if there is any problem while calling API
        boolean hasError = false;

        boolean readOnly = Boolean.valueOf((String)
            propertySheetModel.getValue("ReadOnlyValue")).booleanValue();
        boolean mountAtBoot = Boolean.valueOf((String)
            propertySheetModel.getValue("MountAtBootValue")).booleanValue();
        boolean backGround = Boolean.valueOf((String)
            propertySheetModel.getValue("MountBackgroundValue")).booleanValue();
        boolean mountAfterAdd = isMDSMounted() ? Boolean.valueOf((String)
            propertySheetModel.getValue("MountAfterAddValue")).booleanValue() :
            isMDSMounted();
        boolean isClient = "client".equals(
            (String) propertySheetModel.getValue("MemberType"));
        String selectedHost =
            (String) propertySheetModel.getValue("HostDropDownValue");
        String primaryIP =
            (String) propertySheetModel.getValue("PrimaryIPDropDownValue");
        String secondaryIP =
            (String) propertySheetModel.getValue("SecondaryIPDropDownValue");
        String mountPoint =
            (String) propertySheetModel.getValue("MountPointValue");

        printSubmitTraceString(selectedHost, isClient, primaryIP, secondaryIP,
            mountPoint, readOnly, mountAtBoot, backGround, mountAfterAdd);

        // Put both IP addresses (if selected) into an array for API call
        String [] ipArray = createIPArray(primaryIP, secondaryIP);

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            fsManager.addHostToSharedFS(
                getFSName(),
                getServerName(),
                mountPoint,
                selectedHost,
                ipArray,
                readOnly,
                mountAtBoot,
                true,
                mountAfterAdd,
                !isClient,
                backGround);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.MOUNT_POINT, mountPoint);
        } catch (SamFSMultiHostException multiEx) {
            hasError = true;
            TraceUtil.trace1(
                "SamFSMultiHostException caught! ERROR CODE: " +
                multiEx.getSAMerrno());
            TraceUtil.trace1("Reason: " + multiEx.getMessage());
            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                SamUtil.getResourceString(
                    "AddSharedMember.error.add",
                    new String [] {
                        selectedHost,
                        isClient ?
                            SamUtil.getResourceString(
                                "AddSharedMember.radio.client") :
                            SamUtil.getResourceString(
                                "AddSharedMember.radio.pmds"),
                        getFSName()}),
                multiEx.getSAMerrno(),
                SamUtil.handleMultiHostException(multiEx),
                selectedHost);
        } catch (SamFSException ex) {
            hasError = true;
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSubmitRequest",
                "Failed to add the specified host as a shared member",
                 selectedHost);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                SamUtil.getResourceString(
                    "AddSharedMember.error.add",
                    new String [] {
                        selectedHost,
                        isClient ?
                            SamUtil.getResourceString(
                                "AddSharedMember.radio.client") :
                            SamUtil.getResourceString(
                                "AddSharedMember.radio.pmds"),
                        getFSName()}),
                ex.getSAMerrno(),
                ex.getMessage(),
                selectedHost);
        }

        if (hasError) {
            forwardTo(getRequestContext());
        } else {
            // if there is no error occurred in the calls above,
            // Try to get that file system to check if it is added or not
            // sleep for 5 seconds, give time for underlying call to execute
            try {
                SamQFSSystemModel sysModel = SamUtil.getModel(selectedHost);

                FileSystem myFS = sysModel.
                    getSamQFSSystemFSManager().getFileSystem(getFSName());
                if (myFS == null) {
                    // if there is no error occurred in the calls above,
                    // sleep for 5 seconds, give time for underlying call
                    // to be completed
                    TraceUtil.trace2("new FS not found. Wait 5 seconds ...");
                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException intEx) {
                    // impossible for other thread to interrupt this thread
                    // Continue to load the page
                    TraceUtil.trace3(
                        "InterruptedException Caught: Reason: " +
                         intEx.getMessage());
                    }
                }
            } catch (SamFSException ex) {
                hasError = true;
                SamUtil.setErrorAlert(
                    this,
                    CommonSecondaryViewBeanBase.ALERT,
                    SamUtil.getResourceString(
                        "AddSharedMember.error.add",
                        new String [] {
                            selectedHost,
                            isClient ?
                                SamUtil.getResourceString(
                                    "AddSharedMember.radio.client") :
                                SamUtil.getResourceString(
                                    "AddSharedMember.radio.pmds"),
                                getFSName()}),
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    selectedHost);

                // TODO: Is this wait necessary???
                if (ex.getSAMerrno() == ex.NOT_FOUND) {
                    TraceUtil.trace2("ex.NOT_FOUND caught again!");
                    TraceUtil.trace2("wait for 5 seconds ...");
                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException intEx) {
                       TraceUtil.trace3(
                           "InterruptedException Caught: Reason: " +
                            intEx.getMessage());
                    }
                }
            }
        }

        if (!hasError) {
            SamUtil.setInfoAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "AddSharedMember.action.add",
                    new String [] {
                        selectedHost,
                        isClient ?
                            SamUtil.getResourceString(
                               "AddSharedMember.radio.client") :
                            SamUtil.getResourceString(
                               "AddSharedMember.radio.pmds"),
                            getFSName()}),
                getServerName());

            setSubmitSuccessful(true);
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create an array of strings that contains the primary and secondary
     * IP addresses if they are selected.
     * @return the IP Array that API needs to add shared member of the fs
     */
    private String [] createIPArray(String primaryIP, String secondaryIP) {
        String [] ipArray = null;

        // Check if secondary IP is selected
        if ("---".equals(secondaryIP)) {
            ipArray = new String[1];
            ipArray[0] = primaryIP;
        } else {
            ipArray = new String[2];
            ipArray[0] = primaryIP;
            ipArray[1] = secondaryIP;
        }

        return ipArray;
    }

    /**
     * Helper function to determine if we need to mark the hosts candidates
     * in the HostNames hidden field.  Hosts will be marked if they have
     * different architecture from the MDS of the file system.  MDS and PMDS
     * of a shared file system is required, but shared client can be any
     * architecture.  Thus in the Add Shared Member Pop Up, we need to
     * dynamically populate the hosts in the drop down.
     */
    private boolean markedEntry(String hostName) throws SamFSException {
        // Mark host if it has a different architecture than the MDS
        return
            !(SamUtil.getModel(hostName).
                getArchitecture().equals(getMDSHostArchitecture()));
    }

    private String getMDSHostArchitecture() throws SamFSException {
        // first check the page session
        String arch = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.ARCHITECTURE);

        // second check the request
        if (arch == null) {
            arch = SamUtil.getModel(getMDSName()).getArchitecture();
            setPageSessionAttribute(
                Constants.PageSessionAttributes.ARCHITECTURE,
                arch);
        }

        return arch;
    }

    private String getMDSName() throws SamFSException {
        String mdsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SHARED_MD_SERVER);

        // second check the request
        if (mdsName == null) {
            mdsName =
                SamUtil.getModel(getServerName()).
                    getSamQFSSystemFSManager().
                    getFileSystem(getFSName()).getServerName();
            setPageSessionAttribute(
                Constants.PageSessionAttributes.SHARED_MD_SERVER,
                mdsName);
        }

        return mdsName;
    }

    private void printSubmitTraceString(
        String selectedHost, boolean isClient, String primaryIP,
        String secondaryIP, String mountPoint, boolean readOnly,
        boolean mountAtBoot, boolean backGround, boolean mountAfterAdd) {

        TraceUtil.trace3(
            new StringBuffer(
                "Submit Button clicked: selectedHost: ").append(selectedHost).
                append(" isClient: ").append(isClient).
                append(" primaryIP: ").append(primaryIP).
                append(" secondaryIP: ").append(secondaryIP).
                append(" mountPoint: ").append(mountPoint).
                append(" readOnly: ").append(readOnly).
                append(" mountAtBoot: ").append(mountAtBoot).
                append(" backGround: ").append(backGround).
                append(" mountAfterAdd: ").append(mountAfterAdd).toString());
    }
}
