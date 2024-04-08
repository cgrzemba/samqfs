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

// ident	$Id: CommonTableContainerView.java,v 1.15 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildContentDisplayEvent;
import com.iplanet.jato.view.event.JspEndChildDisplayEvent;

import com.sun.netstorage.samqfs.web.ui.taglib.WizardWindowTag;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.view.table.CCActionTable;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.tagext.Tag;


/*
 * This is the base class for all the containerViews
 * that are used for ActionTable
 *
 * Usage:
 * 1. Create your View class as a derived class of CommonTableContainerView.
 * 2. In Constructor
 *    Set the CHILD_ACTION_TABLE to your tableName;
 *    Create the Model with your Model Class;
 * 3. In registerChildren()
 *    call super.registerChildren(), and pass in the table model;
 *
 */

public class CommonTableContainerView extends RequestHandlingViewBase {

    // Child view names (i.e. display fields).
    protected String CHILD_ACTION_TABLE = "";

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public CommonTableContainerView(View parent, String name) {
        super(parent, name);
    }

    /**
     * Register each child view.
     * @param table model
     */
    protected void registerChildren(CCActionTableModel tableModel) {
        registerChild(CHILD_ACTION_TABLE, CCActionTable.class);
        tableModel.registerChildren(this);
    }

    /**
     * Instantiate each child view.
     * @param table model
     * @param child component name
     */
    protected View createChild(CCActionTableModel tableModel, String name) {
        if (name.equals(CHILD_ACTION_TABLE)) {
            // Action table.
            CCActionTable child = new CCActionTable(this, tableModel, name);
            tableModel.setShowSelectionSortIcon(false);
            return child;
        } else if (tableModel.isChildSupported(name)) {
            // Create child from action table model.
            return tableModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Instantiate each child view.
     * @param table model
     * @param child component name
     * @param tiled view name
     */
    protected View createChild(
        CCActionTableModel tableModel,
        String name,
        String tiledViewName) {

        // e.g. tileViewName can be "FileSystemSummaryView.CHILD_TILED_VIEW"

        if (name.equals(CHILD_ACTION_TABLE)) {
            // Action table.
            CCActionTable child = new CCActionTable(this, tableModel, name);
            tableModel.setShowSelectionSortIcon(false);
            child.setTiledView((ContainerView) getChild(tiledViewName));
            return child;
        } else if (tableModel.isChildSupported(name)) {
            // Create child from action table model.
            return tableModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * since the CCActionTable doesn't recognize 'unknown' tags, use a
     * regular CCWizardWindowTag in the table XML and swap its HTML with
     * that our of special WizardWindowTag here.
     */
    public String endChildDisplay(ChildContentDisplayEvent ccde)
        throws ModelControlException {
        String childName = ccde.getChildName();

        String html = super.endChildDisplay(ccde);
        // if its one of our special wizards
        // i.e. name contains keyword "SamQFSWizard"
        if (childName.indexOf(Constants.Wizard.WIZARD_BUTTON_KEYWORD) != -1 ||
            childName.indexOf(Constants.Wizard.WIZARD_SCRIPT_KEYWORD) != -1) {
            // retrieve the html from a WizardWindowTag
            JspEndChildDisplayEvent event = (JspEndChildDisplayEvent)ccde;
            CCButtonTag sourceTag = (CCButtonTag)event.getSourceTag();
            Tag parentTag = sourceTag.getParent();

            // get the peer view of this tag
            View theView = getChild(childName);

            // instantiate the wizard tag
            WizardWindowTag tag = new WizardWindowTag();
            tag.setBundleID(sourceTag.getBundleID());
            tag.setDynamic(sourceTag.getDynamic());

            // Is this a script wizard tag?
            if (childName.indexOf(
                Constants.Wizard.WIZARD_SCRIPT_KEYWORD) != -1) {
                // This is a script wizard
                tag.setName(childName);
                tag.setWizardType(WizardWindowTag.TYPE_SCRIPT);
            }


            // try to retrieve the correct HTML from the tag
            try {
                html = tag.getHTMLStringInternal(parentTag,
                    event.getPageContext(), theView);
            } catch (JspException je) {
                throw new IllegalArgumentException("Error retrieving the " +
                    "WizardWindowTag html : " + je.getMessage());
            }
        }

        return html;
    }
}
