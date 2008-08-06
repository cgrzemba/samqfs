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

// ident	$Id: SiteInformationViewBean.java,v 1.12 2008/08/06 17:41:51 ronaldso Exp $

package com.sun.netstorage.samqfs.web.server;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.Common;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.media.MediaUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCStaticTextField;


/**
 *  This class is the view bean for the Site Information page
 */

public class SiteInformationViewBean extends ServerCommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "SiteInformation";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/server/SiteInformation.jsp";
    private static final int TAB_NAME = ServerTabsUtil.SITE_INFORMATION_TAB;

    private static final String CHILD_STATIC_TEXT = "StaticText";

    // Page Title and Property Sheet Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;

    /**
     * Constructor
     */
    public SiteInformationViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL, TAB_NAME);
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
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CHILD_STATIC_TEXT, CCStaticTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // PropertySheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else if (name.equals(CHILD_STATIC_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            // Create Property Sheet Model
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/server/SiteInformationPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        loadContent();
        TraceUtil.trace3("Exiting");
    }

    /**
     * To populate the servers and media devices information of the site
     */
    private void loadContent() {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel [] allSystemModel = null;

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            if (appModel == null) {
                throw new SamFSException(null, -2501);
            }

            // try to re-establish connection to DOWN servers
            appModel.updateDownServers();

            allSystemModel = appModel.getAllSamQFSSystemModels();

            if (allSystemModel == null) {
                return;
            } else if (allSystemModel.length == 0) {
                return;
            } else {
                loadServerContent(allSystemModel);
                loadMediaContent();
            }
        } catch (SamFSException samEx) {
            // Special Case
            // No need to call ProcessException
            propertySheetModel.setValue("ServerText",
                SamUtil.getResourceString("SiteInformation.server.failed"));
            propertySheetModel.setValue("MediaText",
                SamUtil.getResourceString("SiteInformation.media.failed"));
            return;
        }
    }

    /**
     * To populate media devices content
     */
    private void loadMediaContent()
        throws SamFSException {
        TraceUtil.trace3("Entering");

        NonSyncStringBuffer allMediaStringBuf = new NonSyncStringBuffer();

        // Get a handle of a server and call getAllLibrariesFromAllHosts
        // this function will return ALL libraries from ALL hosts
        // with no duplication
        // Grab all libraries objects, and pass to helper method to
        // generate the string as designed
        // If exception occurred, set error message in the MediaText value

        Library allLibraries[] = Common.getAllLibrariesFromServers();

        // If there's no library, use MediaText defaultValue
        if (allLibraries == null || allLibraries.length == 0) {
            propertySheetModel.setVisible("media", false);
            return;
        } else {
            propertySheetModel.setVisible("media", true);
        }

        for (int i = 0; i < allLibraries.length; i++) {
            if (allMediaStringBuf.length() != 0) {
                allMediaStringBuf.append("<br /><br />");
            }
            allMediaStringBuf.append(
                MediaUtil.createLibraryEntry(allLibraries[i]));
        }

        // If there's nothing in buffer, use default String
        if (allMediaStringBuf.length() != 0) {
            propertySheetModel.setValue(
                "MediaText", allMediaStringBuf.toString());
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * To populate server content
     */
    private void loadServerContent(SamQFSSystemModel [] allSystemModel)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        NonSyncStringBuffer allServerStringBuf = new NonSyncStringBuffer();

        // Go thru each sysModel, generate a server entry for each model

        String serverName = null;

        // Cluster Node goes first, always skip storage nodes
        for (int i = 0; i < allSystemModel.length; i++) {
            if (allSystemModel[i].isDown() ||
                !allSystemModel[i].isClusterNode() ||
                allSystemModel[i].isStorageNode()) {
                continue;
            }

            serverName = allSystemModel[i].getHostname();

            try {
                if (allServerStringBuf.length() != 0) {
                    allServerStringBuf.append("<br /><br />");
                }

                allServerStringBuf.append(
                    ServerUtil.createServerEntry(allSystemModel[i]));
            } catch (SamFSException samEx) {
                // If there's any problem connecting to a server, skip it
                TraceUtil.trace1("Error connecting to " +
                    allSystemModel[i].getHostname() +
                " while compiling site information.");
                TraceUtil.trace1("Reason: " + samEx.getMessage());
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "initModelRows()",
                    "Unknown exception occur while connecting to server",
                    serverName);
            }
        }

        // always skip storage nodes
        for (int i = 0; i < allSystemModel.length; i++) {
            if (allSystemModel[i].isDown() ||
                allSystemModel[i].isClusterNode() ||
                allSystemModel[i].isStorageNode()) {
                continue;
            }

            serverName = allSystemModel[i].getHostname();

            try {
                if (allServerStringBuf.length() != 0) {
                    allServerStringBuf.append("<br /><br />");
                }

                allServerStringBuf.append(
                    ServerUtil.createServerEntry(allSystemModel[i]));
            } catch (SamFSException samEx) {
                // If there's any problem connecting to a server, skip it
                TraceUtil.trace1("Error connecting to " +
                    allSystemModel[i].getHostname() +
                " while compiling site information.");
                TraceUtil.trace1("Reason: " + samEx.getMessage());
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "initModelRows()",
                    "Unknown exception occur while connecting to server",
                    serverName);
            }
        }

        // If there's nothing in buffer, use default String
        if (allServerStringBuf.length() != 0) {
            propertySheetModel.setValue(
                "ServerText", allServerStringBuf.toString().trim());
        }

        TraceUtil.trace3("Exiting");
    }
}
