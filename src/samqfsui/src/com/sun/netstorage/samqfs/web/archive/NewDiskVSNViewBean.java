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

// ident	$Id: NewDiskVSNViewBean.java,v 1.14 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserModel;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

/** The View Bean for the New Disk VSN Popup */
public class NewDiskVSNViewBean extends CommonSecondaryViewBeanBase {
    private static final String PAGE_NAME = "NewDiskVSN";
    private static final String DEFAULT_URL = "/jsp/archive/NewDiskVSN.jsp";

    // children
    public static final String NAME_LABEL = "vsnNameLabel";
    public static final String NAME = "vsnName";
    public static final String HOST_LABEL = "vsnHostLabel";
    public static final String HOST = "vsnHost";
    public static final String PATH_LABEL = "vsnPathLabel";
    public static final String FILE_CHOOSER = "filechooser";
    public static final String REQUIRED_MESSAGE = "allRequiredLabel";
    public static final String BROWSE_RULES = "browsingRules";
    public static final String CREATE_PATH = "createPath";
    public static final String MEDIA_TYPE_LABEL = "mediaTypeLabel";
    public static final String MEDIA_TYPE = "mediaType";
    public static final String DATA_IP_LABEL = "dataIPLabel";
    public static final String DATA_IP = "dataIP";
    public static final String PORT_LABEL = "portLabel";
    public static final String PORT = "port";
    public static final String HELP = "help";
    public static final String ADMIN_HOST_LABEL = "adminHostLabel";
    public static final String ADMIN_HOST = "adminHost";
    public static final String LAUNCH_ADMIN = "launchAdmin";

    // hidden fields
    public static final String RFC_HOSTS = "RFCCapableHosts";

    // page title
    public static final String PAGE_TITLE = "pageTitle";

    // models
    private CCPageTitleModel ptModel = null;
    private RemoteFileChooserModel fcModel = null;

    /** create a new instance of this view bean */
    public NewDiskVSNViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // create the page title model
        createPageTitleModel();
        createFileChooserModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this view's children */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();

        registerChild(NAME_LABEL, CCLabel.class);
        registerChild(HOST_LABEL, CCLabel.class);
        registerChild(PATH_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE_LABEL, CCLabel.class);
        registerChild(DATA_IP_LABEL, CCLabel.class);
        registerChild(PORT_LABEL, CCLabel.class);
        registerChild(NAME, CCTextField.class);
        registerChild(HOST, CCDropDownMenu.class);
        registerChild(REQUIRED_MESSAGE, CCLabel.class);
        registerChild(BROWSE_RULES, CCStaticTextField.class);
        registerChild(RFC_HOSTS, CCHiddenField.class);
        registerChild(FILE_CHOOSER, RemoteFileChooserControl.class);
        registerChild(CREATE_PATH, CCCheckBox.class);
        registerChild(MEDIA_TYPE, CCLabel.class);
        registerChild(DATA_IP, CCLabel.class);
        registerChild(PORT, CCLabel.class);
        registerChild(ADMIN_HOST_LABEL, CCLabel.class);
        registerChild(ADMIN_HOST, CCTextField.class);
        registerChild(LAUNCH_ADMIN, CCButton.class);
        registerChild(HELP, CCStaticTextField.class);

        ptModel.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    /** create a named child of this view */
    public View createChild(String name) {
        if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, ptModel, name);
        } else if (name.equals(NAME_LABEL) ||
                   name.equals(HOST_LABEL) ||
                   name.equals(PATH_LABEL) ||
                   name.equals(REQUIRED_MESSAGE) ||
                   name.equals(MEDIA_TYPE_LABEL) ||
                   name.equals(DATA_IP_LABEL) ||
                   name.equals(PORT_LABEL) ||
                   name.equals(ADMIN_HOST_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(CREATE_PATH)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(BROWSE_RULES) ||
                   name.equals(HELP)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(NAME) ||
                   name.equals(DATA_IP) ||
                   name.equals(PORT) ||
                   name.equals(ADMIN_HOST)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(HOST) ||
                   name.equals(MEDIA_TYPE)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(RFC_HOSTS)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(LAUNCH_ADMIN)) {
            return new CCButton(this, name, null);
        } else if (name.equals(FILE_CHOOSER)) {
            return new RemoteFileChooserControl(this,
                   new RemoteFileChooserModel(null), name);
        } else if (ptModel.isChildSupported(name)) {
            return ptModel.createChild(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    /** creates an instance of this popup's page title model */
    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");

        ptModel = new CCPageTitleModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/NewDiskVSNPageTitle.xml");

        TraceUtil.trace3("Exiting");
    }

    /**
     * create an instance of the file chooser model
     */
    private void createFileChooserModel() {
        String serverName = RequestManager.getRequestContext().
            getRequest().getParameter(Constants.Parameters.SERVER_NAME);

        String rootDir = RequestManager.getRequestContext().
            getRequest().getParameter("rootDir");

        fcModel = new RemoteFileChooserModel(serverName, 50);

        fcModel.setHomeDirectory(rootDir);
    }


    /** called before this container generating its display content */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(evt);

        // need to retrieve this from page session
        String serverName = getServerName();

        // set fc model
        RemoteFileChooserModel fcModel = (RemoteFileChooserModel)
            ((RemoteFileChooserControl)getChild(FILE_CHOOSER)).getModel();
        fcModel.setServerName(serverName);


        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            // set the allowable media types
            OptionList mediaTypeList = new OptionList();
            mediaTypeList.add(0,
                    SamUtil.getMediaTypeString(BaseDevice.MTYPE_DISK),
                              Integer.toString(BaseDevice.MTYPE_DISK));

            // if dealing with a 4.6+ server, add the honey comb media type
            if (sysModel.getServerAPIVersion().compareTo("1.5.6") >= 0) {
                mediaTypeList.add(1,
                      SamUtil.getMediaTypeString(BaseDevice.MTYPE_STK_5800),
                      Integer.toString(BaseDevice.MTYPE_STK_5800));
            }

            ((CCDropDownMenu)getChild(MEDIA_TYPE)).setOptions(mediaTypeList);

            // populate the host dropdown
            DiskVSNHostBean bean = PolicyUtil.getSamQFSServerInfo();
            String [] hostNames = bean.getLiveHosts();
            if (hostNames == null) {
                throw new SamFSException(null, -2001);
            }

            CCDropDownMenu field = (CCDropDownMenu)getChild(HOST);
            field.setOptions(new OptionList(hostNames, hostNames));

            // set the selected value
            field.setValue(serverName);

            ((CCHiddenField)getChild(RFC_HOSTS)).
                setValue(bean.getRFCCapableHosts());

        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "beingDisplay",
                                     "Unable to retrieve host names",
                                     serverName);

        }

    }

    /**
     * safety method for the filechooser button to prevent exceptions when the
     * form is inadvertently submitted
     */
    public void handleFileChooserRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    /**
     * this method is called when the create new vsn popup's submit button
     * is clicked
     */
    public void handleSubmitRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // retrieve the server since it maybe needed multiple times
        String serverName = getServerName();

        try {
            int mediaType =
                Integer.parseInt(getDisplayFieldStringValue(MEDIA_TYPE));
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            String name = getDisplayFieldStringValue(NAME);

            if (mediaType == BaseDevice.MTYPE_STK_5800) {
                String dataIP = getDisplayFieldStringValue(DATA_IP);
                int port = Integer.parseInt(getDisplayFieldStringValue(PORT));

                sysModel.getSamQFSSystemMediaManager()
                    .createHoneyCombVSN(name, dataIP, port);
            } else {
                String host = getDisplayFieldStringValue(HOST);
                String createPath = getDisplayFieldStringValue(CREATE_PATH);

                // retrieve path
                RemoteFileChooserControl chooser =
                    (RemoteFileChooserControl)getChild(FILE_CHOOSER);
                String path =
                    chooser.getDisplayFieldStringValue("browsetextfield");

                if (createPath.equals("true") ||
                    SamUtil.getModel(host).doesFileExist(path)) {
                    if (serverName.equals(host)) {
                        host = null;
                    }
                    sysModel.getSamQFSSystemMediaManager()
                        .createDiskVSN(name, host, path);
                } else {
                    String ek =
                        "archiving.diskvsn.newvsn.error.pathnonexistent";

                    throw new SamFSException(ek, -2027);
                }
            }

            // give user feedback that disk vsn was created successfully
            SamUtil.setInfoAlert(this,
                                  ALERT,
                                  "success.summary",
                                  SamUtil.getResourceString(
                                  "archiving.diskvsn.newvsn.create.success",
                                  name),
                                  serverName);
            setSubmitSuccessful(true);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleSubmitRequest",
                                     "unable to create disk vsn",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "archiving.diskvsn.newvsn.create.failure",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
        forwardTo(getRequestContext());
    }
}
