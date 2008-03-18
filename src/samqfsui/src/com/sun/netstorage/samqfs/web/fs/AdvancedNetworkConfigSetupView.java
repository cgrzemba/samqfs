/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident    $Id: AdvancedNetworkConfigSetupView.java,v 1.6 2008/03/17 14:43:32 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.archive.MultiTableViewBase;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;

import java.util.Map;

public class AdvancedNetworkConfigSetupView extends MultiTableViewBase {

    // child name for tiled view class
    public static final
        String CHILD_TILED_VIEW = "AdvancedNetworkConfigSetupTiledView";

    public static final String SETUP_TABLE = "AdvancedNetworkConfigSetupTable";

    public static final String INSTRUCTION = "Instruction";

    // Hidden fields for javascript to grab user selection without restoring
    // tiled view data for performance improvements
    public static final String NO_OF_MDS    = "NumberOfMDS";
    public static final String IP_ADDRESSES = "IPAddresses";


    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AdvancedNetworkConfigSetupView(
        View parent, Map models, String name) {
        super(parent, models, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(
            CHILD_TILED_VIEW, AdvancedNetworkConfigSetupTiledView.class);
        registerChild(INSTRUCTION, CCStaticTextField.class);
        registerChild(NO_OF_MDS, CCHiddenField.class);
        registerChild(IP_ADDRESSES, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append(
            "Entering: name is ").append(name).toString());

        View child = null;
        if (name.equals(CHILD_TILED_VIEW)) {
            child = new AdvancedNetworkConfigSetupTiledView(
                this, getTableModel(SETUP_TABLE), name);
        } else if (name.equals(SETUP_TABLE)) {
            child = createTable(name, CHILD_TILED_VIEW);
        } else if (name.equals(INSTRUCTION)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(NO_OF_MDS) ||
            name.equals(IP_ADDRESSES)) {
            child = new CCHiddenField(this, name, null);
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null) {
                child = super.isChildSupported(name).createChild(this, name);
            }
        }

        if (child == null) {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }


    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // populate table headers
        initializeTableHeaders();

        // populate instruction
        ((CCStaticTextField) getChild(INSTRUCTION)).setValue(
            SamUtil.getResourceString(
                "AdvancedNetworkConfig.setup.instructions",
                getSelectedHostsString()));

        TraceUtil.trace3("Exiting");
    }

    private void setSuccessAlert(String msg, String item) {
        TraceUtil.trace3("Entering");

        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonSecondaryViewBeanBase.ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            "");

        TraceUtil.trace3("Exiting");
    }

    private void initializeTableHeaders() {
        CCActionTableModel model = getTableModel(SETUP_TABLE);
        model.setRowSelected(false);

        model.setActionValue(
            "NameColumn",
            "AdvancedNetworkConfig.setup.heading.hostname");
        model.setActionValue(
            "PrimaryIPColumn",
            "AdvancedNetworkConfig.setup.heading.primaryip");
        model.setActionValue(
            "SecondaryIPColumn",
            "AdvancedNetworkConfig.setup.heading.secondaryip");

    }

    public void populateTableModels() {
        populateSetupTable();
    }

    private void populateSetupTable() {
        String [] mdsNames = getMDSNames();

        // selected host is not used in this method
        // getSelectedHosts need to be called to save the information from
        // the request before it gets overwritten!
        String [] selectedHosts = getSelectedHosts();

        // Retrieve the handle of the Server Selection Table
        CCActionTableModel model = getTableModel(SETUP_TABLE);
        model.clear();

        for (int i = 0; i < mdsNames.length; i++) {
            // append new row
            if (i > 0) {
                model.appendRow();
            }

            model.setValue("NameText", mdsNames[i]);
        }

        // Fill in the hidden field
        ((CCHiddenField) getChild(NO_OF_MDS)).setValue(
            Integer.toString(mdsNames.length));

        TraceUtil.trace3("Exiting");
    }

    private String [] getSelectedHosts() {
        String [] selectedHosts = getSelectedHostsString().split(",");
        if (selectedHosts == null || selectedHosts.length == 0) {
            return new String[0];
        } else {
            return selectedHosts;
        }
    }

    private String getSelectedHostsString() {
        // first check the page session
        String selectedHosts = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.SELECTED_HOSTS);

        // second check the request
        if (selectedHosts == null) {
            selectedHosts = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SELECTED_HOSTS);

            if (selectedHosts != null) {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.SELECTED_HOSTS,
                    selectedHosts);
            } else {
                throw new IllegalArgumentException(
                    "Selected hosts not supplied");
            }
        }

        return selectedHosts;
    }

    private String [] getMDSNames() {
        // first check the page session
        String mdsNames = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.SHARED_MDS_LIST);

        // second check the request
        if (mdsNames == null) {
            mdsNames = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SHARED_MDS_LIST);
            if (mdsNames != null) {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.SHARED_MDS_LIST,
                    mdsNames);
            } else {
                throw new IllegalArgumentException(
                    "Metadata hosts not supplied");
            }
        }

        if (mdsNames == null || mdsNames.length() == 0) {
            return new String[0];
        } else {
            return mdsNames.split(",");
        }
    }

} // end of AdvancedNetworkConfigSetupView class
