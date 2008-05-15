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

// ident	$Id: Select.java,v 1.2 2008/05/15 04:34:10 kilemba Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.data.provider.RowKey;
import com.sun.web.ui.event.TableSelectPhaseListener;
import javax.faces.context.FacesContext;
import javax.faces.el.ValueBinding;

public class Select {
    private TableSelectPhaseListener listener = null;
    private String rowBinding = null;

    public Select(String sourceVar) {
        if (sourceVar == null)
            throw new UnsupportedOperationException("sourceVar is null");

        StringBuffer buf = new StringBuffer();
        buf.append("#{")
            .append(this.rowBinding)
            .append(".tableRow}");

        rowBinding = buf.toString();
        listener = new TableSelectPhaseListener();
    }

    public void clear() {
        listener.clear();
    }

    public boolean isKeepSelected() {
        return listener.isKeepSelected();
    }

    public void setKeepSelected(boolean selected) {
        listener.keepSelected(selected);
    }

    public Object getSelected() {
        return listener.getSelected(getTableRow());
    }

    public void setSelected(Object obj) {
        RowKey rowKey = getTableRow();
        if (rowKey != null) {
            listener.setSelected(rowKey, obj);
        }
    }

    public Object getSelectedValue() {
        RowKey rowKey = getTableRow();
        Object result = null;
        if (rowKey != null)
            result = rowKey.getRowId();

        return result;
    }

    public boolean getSelectedState() {
        return getSelectedState(getTableRow());
    }

    public boolean getSelectedState(RowKey rowKey) {
        return listener.isSelected(rowKey);
    }

    private RowKey getTableRow() {
        FacesContext context = FacesContext.getCurrentInstance();
        ValueBinding vb = context.getApplication()
            .createValueBinding(this.rowBinding);

        return (RowKey)vb.getValue(context);
    }
}
