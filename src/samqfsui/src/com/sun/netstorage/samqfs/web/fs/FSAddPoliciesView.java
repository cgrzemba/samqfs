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

// ident	$Id: FSAddPoliciesView.java,v 1.15 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.table.CCActionTable;

/**
 * Creates the FSAddPolicies Action Table and provides
 * handlers for the links within the table.
 */

public class FSAddPoliciesView extends CommonTableContainerView {

    protected FSAddPoliciesModel model;

    protected String fsName = null;
    protected String serverName = null;

    public static final String
        CHILD_NUM_OF_ROWS_HIDDEN_FIELD = "NumberOfRowsHiddenField";

    public FSAddPoliciesView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "FSAddPoliciesTable";
        FSAddPoliciesViewBean vb = (FSAddPoliciesViewBean)parent;

        serverName = vb.getServerName();
        fsName = vb.getFSName();

        TraceUtil.trace3("add view : server name = " + serverName);
        TraceUtil.trace3("add view : fs name = " + fsName);

        model = new FSAddPoliciesModel(serverName, fsName);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_NUM_OF_ROWS_HIDDEN_FIELD, CCHiddenField.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(CHILD_NUM_OF_ROWS_HIDDEN_FIELD)) {
            return new CCHiddenField(this, name, null);
        } else {
            return super.createChild(model, name);
        }
    }

    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");
        CCActionTable theTable = (CCActionTable)getChild(CHILD_ACTION_TABLE);
        model.initModelRows(theTable);
        ((CCHiddenField)
            getChild(CHILD_NUM_OF_ROWS_HIDDEN_FIELD)).setValue(
                Integer.toString(model.getNumRows()));
        TraceUtil.trace3("Exiting");
    }
}
