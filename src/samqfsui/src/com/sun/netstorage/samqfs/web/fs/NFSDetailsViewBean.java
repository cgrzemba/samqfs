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

// ident	$Id: NFSDetailsViewBean.java,v 1.17 2008/12/16 00:12:10 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import java.util.HashMap;
import java.util.Map;
import javax.servlet.ServletContext;

/**
 *  This class is the view bean for the Edit NFS Options page
 */


public class NFSDetailsViewBean extends CommonViewBeanBase {

    // The "logical" name for this page.
    private static final String PAGE_NAME = "NFSDetails";

    // Display View
    public static final
        String CONTAINER_VIEW = "NFSShareDisplayView";

    // Add View (Pagelet)
    public static final String ADD_VIEW = "NFSShareAddView";

    // Edit View (Pagelet)
    public static final String EDIT_VIEW = "NFSShareEditView";

    // Hidden fields for javascript to grab user selection without restoring
    // tiled view data for performance improvements
    public static final String SHARE_STATE_LIST = "SharedStateHiddenField";
    public static final String DIR_NAMES_LIST = "DirNamesHiddenField";
    public static final String SERVER_NAME = "ServerNameHiddenField";
    public static final String SELECTED_INDEX = "SelectedIndex";

    // child that used in javascript confirm messages
    public static final String CONFIRM_MESSAGE  = "ConfirmMessageHiddenField";

    // table models
    private Map models = null;

    private CCPageTitleModel pageTitleModel;

    /**
     * Constructor
     */
    public NFSDetailsViewBean() {
        super(PAGE_NAME, "/jsp/fs/NFSDetails.jsp");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        createPageTitleModel();
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
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CONTAINER_VIEW, NFSShareDisplayView.class);
        registerChild(ADD_VIEW, NFSShareAddView.class);
        registerChild(EDIT_VIEW, NFSShareEditView.class);
        registerChild(SHARE_STATE_LIST, CCHiddenField.class);
        registerChild(DIR_NAMES_LIST, CCHiddenField.class);
        registerChild(CONFIRM_MESSAGE, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(SELECTED_INDEX, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child = null;

        if (super.isChildSupported(name)) {
            child = super.createChild(name);

        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);

        } else if (name.equals(CONTAINER_VIEW)) {
            child = new NFSShareDisplayView(
                this, models, name, getServerName());

        } else if (name.equals(SELECTED_INDEX) ||
            name.equals(SHARE_STATE_LIST) ||
            name.equals(SERVER_NAME) ||
            name.equals(DIR_NAMES_LIST)) {
            child = new CCHiddenField(this, name, null);
        // Javascript used Hidden Field
        } else if (name.equals(CONFIRM_MESSAGE)) {
            StringBuffer msg = new StringBuffer();
            msg.append(
                SamUtil.getResourceString("filesystem.nfs.delete.confirm")).
                append("###").append(
                SamUtil.getResourceString("filesystem.nfs.unshare.confirm"));

            child = new CCHiddenField(this, name, msg.toString());

        // Pagelet
        } else if (name.equals(ADD_VIEW)) {
            child = new NFSShareAddView(this, name, getServerName());

        } else if (name.equals(EDIT_VIEW)) {
            child = new NFSShareEditView(this, name, getServerName());

        } else {
             throw new IllegalArgumentException(new StringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;

    }

    private void createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
    }

    private void initializeTableModels() {
	models = new HashMap();
	ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

	// server table
	CCActionTableModel model =
            new CCActionTableModel(
            sc, "/jsp/fs/NFSShareDisplayTable.xml");

	models.put(NFSShareDisplayView.DISPLAY_TABLE, model);
    }

    public void populateSetupTableModel() throws SamFSException {
        NFSShareDisplayView view =
            (NFSShareDisplayView) getChild(CONTAINER_VIEW);
        view.populateTableModels();
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(evt);

        ((CCHiddenField) getChild(SERVER_NAME)).setValue(getServerName());

        try {
            populateSetupTableModel();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "NFSDetailsViewBean()",
                "Failed to populate NFS information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "NFSDetailsViewBean.error.failedPopulate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }
}
