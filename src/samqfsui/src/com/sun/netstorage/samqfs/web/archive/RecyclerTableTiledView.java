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

// ident	$Id: RecyclerTableTiledView.java,v 1.9 2008/03/17 14:40:44 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.RecycleParams;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCTextField;
import javax.servlet.http.HttpServletRequest;

/**
 * This class is the tiledview class for RecyclerTable in Admin-SetUp page
 */

public class RecyclerTableTiledView extends RequestHandlingTiledViewBase {

    private RecyclerTableModel model;

    public RecyclerTableTiledView(View parent,
        RecyclerTableModel model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.model = model;
        registerChildren();
        setPrimaryModel(model);
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        model.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    // if an action table has no rows, i.e. "No items found", but contains
    // a checkbox in its schema, when the save button is pressed,
    // a row will be added to the table and displayed
    // when the page is reloaded. Adding the row occurs in
    // super.mapRequestParameters(), so mapRequestParameters() is
    // overridden here and a call is only made to the superclass if the
    // the table does not contain 0 rows.
    // The number of rows is stored in a hidden field which is populated
    // when the action table is populated.
    public void mapRequestParameters(HttpServletRequest request)
        throws ModelControlException {
        String numRows = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.RECYCLER_NUMBER);
        if (!numRows.equals("0")) {
            super.mapRequestParameters(request);
        }
    }

    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (model.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return model.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    public boolean beginReportDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        try {
            ((CCCheckBox) getChild("report", index)).setValue(
                getEditableValue("report", index));
        } catch (Exception smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginReportDisplay",
                "Failed to retrieve recycler parameters",
                getServerName());
            throw new ModelControlException(smfex.getMessage());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginHwmDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();
        CCTextField hwm = (CCTextField) getChild("hwm", index);

        try {
            String hwmValue = getEditableValue("hwm", index);
            if (!hwmValue.equals("-1")) {
                hwm.setValue(hwmValue);
            }
        } catch (Exception smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginHwmDisplay",
                "Failed to retrieve recycler parameters",
                getServerName());
            throw new ModelControlException(smfex.getMessage());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginMinigainDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        try {
            String minigainValue = getEditableValue("mini", index);
            if (!minigainValue.equals("-1")) {
                ((CCTextField) getChild("minigain", index)).
                    setValue(minigainValue);
            }
        } catch (Exception smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginMinigainDisplay",
                "Failed to retrieve recycler parameters",
                getServerName());
            throw new ModelControlException(smfex.getMessage());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginVsnlimitDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        try {
            String vsnLimitValue = getEditableValue("vsn", index);
            if (!vsnLimitValue.equals("-1")) {
                ((CCTextField) getChild("vsnlimit", index)).
                    setValue(vsnLimitValue);
            }
        } catch (Exception smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginVsnlimitDisplay",
                "Failed to retrieve recycler parameters",
                getServerName());
            throw new ModelControlException(smfex.getMessage());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginSizelimitDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        try {
            String sizeValue = getEditableValue("size", index);
            if (!sizeValue.equals("-1")) {
                ((CCTextField) getChild("sizelimit", index)).
                    setValue(sizeValue);
            }
        } catch (Exception smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginSizelimitDisplay",
                "Failed to retrieve recycler parameters",
                getServerName());
            throw new ModelControlException(smfex.getMessage());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginSizeunitDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        try {
            ((CCDropDownMenu) getChild("sizeunit", index)).
                setValue(getEditableValue("sizeunit", index));
        } catch (Exception smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginSizeunitDisplay",
                "Failed to retrieve recycler parameters",
                getServerName());
            TraceUtil.trace1("Exception occurred: " +
                ((smfex.getMessage() != null) ?
                smfex.getMessage() : "Failed to retrieve model"));
            LogUtil.error(this.getClass(),
                "beginSizeunitDisplay",
                smfex.getMessage() != null ?
                smfex.getMessage() : "Failed to retrieve model");
            throw new ModelControlException(smfex.getMessage());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }

    private String getEditableValue(String type, int index)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        String value = null;
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        RecycleParams[] recyclers = sysModel.getSamQFSSystemArchiveManager().
            getRecycleParams();
        if (recyclers.length == 0) {
            if (type.equals("report")) {
                return "false";
            } else {
                return "";
            }
        }

        RecycleParams recycler = recyclers[index];
        if (type.equals("report")) {
            if (recycler.isPerform()) {
                value = "true";
            } else {
                value = "false";
            }
        } else if (type.equals("hwm")) {
            value = Integer.toString(recycler.getHWM());
        } else if (type.equals("mini")) {
            value = Integer.toString(recycler.getMinGain());
        } else if (type.equals("vsn")) {
            value = Integer.toString(recycler.getVSNLimit());
        } else if (type.equals("size")) {
            value = Long.toString(recycler.getSizeLimit());
        } else if (type.equals("sizeunit")) {
            int unit = recycler.getSizeUnit();
            if (unit == -1) {
                value = "dash";
            } else {
                value = SamUtil.getSizeUnitString(unit);
            }
        }
        TraceUtil.trace3("Exiting");
        return value;
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }
}
