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

// ident	$Id: AdvancedNetworkConfigSetupViewBean.java,v 1.10 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.web.model.MDSAddresses;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;

import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Advanced Network Config Setup page
 */

public class AdvancedNetworkConfigSetupViewBean
        extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "AdvancedNetworkConfigSetup";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/fs/AdvancedNetworkConfigSetup.jsp";

    // View class
    public static final
        String CONTAINER_VIEW = "AdvancedNetworkConfigSetupView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // table models
    private Map models = null;

    /**
     * Constructor
     */
    public AdvancedNetworkConfigSetupViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        registerChildren();
        initializeTableModels();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CONTAINER_VIEW, AdvancedNetworkConfigSetupView.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
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
        } else if (name.equals(CONTAINER_VIEW)) {
            child = new AdvancedNetworkConfigSetupView(this, models, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Handler function to handle the request of Submit button
     * NOTE: The handler function is located in ServerSelectionViewBean
     * due to the submit button behavior issue.
     */

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/AdvancedNetworkConfigSetupPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private void initializeTableModels() {
	models = new HashMap();
	ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

	// server table
	CCActionTableModel model =
            new CCActionTableModel(
            sc, "/jsp/fs/AdvancedNetworkConfigSetupTable.xml");

	models.put(AdvancedNetworkConfigSetupView.SETUP_TABLE, model);
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        AdvancedNetworkConfigSetupView view =
            (AdvancedNetworkConfigSetupView) getChild(CONTAINER_VIEW);
        String ipAddressString =
            (String) view.getDisplayFieldValue(
                AdvancedNetworkConfigSetupView.IP_ADDRESSES);

        String fsName = getFSName();

        String participatingHosts = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SELECTED_HOSTS);
        String [] participatingHostsArray = participatingHosts.split(",");

        String [] mdsHosts = ((String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SHARED_MDS_LIST)).split(",");

        String [] ipAddresses = ipAddressString.split(",");
        MDSAddresses [] myAddresses = new MDSAddresses[ipAddresses.length];

        TraceUtil.trace3(new StringBuffer(
            "handleSubmitRequest: fsName: ").append(fsName).append(
            " participating hosts: ").append(participatingHosts).append(
            " ipAddresses: ").append(ipAddressString).toString());

        for (int i = 0; i < ipAddresses.length; i++) {
            String [] ipAddressesForThisHost = ipAddresses[i].split(" ");
            myAddresses[i] =
                new MDSAddresses(mdsHosts[i], ipAddressesForThisHost);
        }

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();

            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new StringBuffer(
                    "Start setting advanced network configuration in ").append(
                    participatingHosts).toString());

            fsManager.setAdvancedNetworkConfigToMultipleHosts(
                participatingHostsArray, fsName,
                getMDServerName(), myAddresses);

            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new StringBuffer(
                    "Done setting advanced network configuration in ").append(
                    participatingHosts).toString());

            SamUtil.setInfoAlert(
		this,
		CommonSecondaryViewBeanBase.ALERT,
		"success.summary",
		SamUtil.getResourceString(
                    "AdvancedNetworkConfig.setup.success", participatingHosts),
		getServerName());

            setSubmitSuccessful(true);

        } catch (SamFSMultiHostException multiEx) {
            String errMsg = SamUtil.handleMultiHostException(multiEx);
            TraceUtil.trace1("SamFSMultiHostException is thrown!");
            TraceUtil.trace1(
                "Error Code: " + multiEx.getSAMerrno() + " Reason: " + errMsg);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "AdvancedNetworkConfig.setup.error.execute",
                multiEx.getSAMerrno(),
                errMsg,
                getServerName());
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitRequest",
                "Failed to set advanced network configuration!",
                getServerName());
            if (samEx.getSAMerrno() != samEx.NOT_FOUND) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonSecondaryViewBeanBase.ALERT,
                    "AdvancedNetworkConfig.setup.error.execute",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void populateSetupTableModel() {
        AdvancedNetworkConfigSetupView view =
            (AdvancedNetworkConfigSetupView) getChild(CONTAINER_VIEW);

        view.populateTableModels();
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(evt);

        // save server name to page session
        String serverName = getServerName();

        // save mds name
        String mds = getMDServerName();

        populateSetupTableModel();
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString(
                "AdvancedNetworkConfig.setup.pagetitle", getFSName()));
        TraceUtil.trace3("Exiting");
    }

    private String getFSName() {
        // first check the page session
        String fsName = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        // second check the request
        if (fsName == null) {
            fsName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

            if (fsName != null) {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.FILE_SYSTEM_NAME,
                    fsName);
            } else {
                throw new IllegalArgumentException(
                    "FS Name not supplied");
            }
        }

        return fsName;
    }

    private String getMDServerName() {
        // first check the page session
        String mds = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.SHARED_MD_SERVER);

        // second check the request
        if (mds == null) {
            mds = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SHARED_MD_SERVER);

            if (mds != null) {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.SHARED_MD_SERVER,
                    mds);
            } else {
                throw new IllegalArgumentException(
                    "MDS Name not supplied");
            }
        }
        return mds;
    }
}
