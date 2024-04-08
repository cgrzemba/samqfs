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

// ident	$Id: MultiTableViewBase.java,v 1.12 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildContentDisplayEvent;
import com.iplanet.jato.view.event.JspEndChildDisplayEvent;
import com.sun.netstorage.samqfs.web.ui.taglib.WizardWindowTag;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.view.table.CCActionTable;
import java.util.Iterator;
import java.util.Map;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.tagext.Tag;

/**
 *
 */
public class MultiTableViewBase extends RequestHandlingViewBase {
    private Map models = null;
    /** Creates a new instance of MultiTableCommonViewBase */
    protected MultiTableViewBase() {
    }

    public MultiTableViewBase(View parent, Map models, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        // inialize models
        this.models = models;
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        // register all tables and their children
        Iterator it = models.keySet().iterator();
        while (it.hasNext()) {
            String name = (String)it.next();
            CCActionTableModel model = (CCActionTableModel)models.get(name);
            registerChild(name, CCActionTable.class);
            model.registerChildren(this);
        }
        TraceUtil.trace3("Exiting");
    }

    public CCActionTableModel isChildSupported(String name) {
        TraceUtil.trace3("Entering");
        CCActionTableModel result = null;
        boolean found = false;

        Iterator it = models.values().iterator();
        while (it.hasNext() && !found) {
            CCActionTableModel m = (CCActionTableModel)it.next();
            if (m.isChildSupported(name)) {
                result = m;
                found = true;
            }

	    // disable sort by selection button
	    m.setShowSelectionSortIcon(false);
        }

        TraceUtil.trace3("Exiting");
        return result;
    }

    public CCActionTableModel getTableModel(String name) {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return (CCActionTableModel)models.get(name);
    }

    public boolean isTableSupported(String name) {
        TraceUtil.trace3("Entering");
        Object table = models.get(name);

        TraceUtil.trace3("Exiting");
        return table == null ? false : true;
    }

    public CCActionTable createTable(String name) {
        TraceUtil.trace3("Entering");
        if (isTableSupported(name)) {
            return new CCActionTable(
                this, (CCActionTableModel)models.get(name), name);
        } else {
            throw new IllegalArgumentException("Unknown table '" + name +"'");
        }
    }

    public CCActionTable createTable(String name, String tiledViewName) {
        TraceUtil.trace3("Entering");
        CCActionTable table = createTable(name);
        table.setTiledView((ContainerView)getChild(tiledViewName));
        TraceUtil.trace3("Exiting");
        return table;
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
            if (childName.indexOf(Constants.Wizard.WIZARD_SCRIPT_KEYWORD)
                != -1) {
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
                    "WizardWIndowTag html : " + je.getMessage());
            }
        }

        return html;
    }
}
