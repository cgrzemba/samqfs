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

// ident	$Id: RecoveryPointsTiledView.java,v 1.7 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.view.DisplayField;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;

public class RecoveryPointsTiledView extends CommonTiledViewBase {

    public RecoveryPointsTiledView(
        View parent,
        CCActionTableModel model,
        String name) {
        super(parent, model, name);
    }

    protected void mapRequestParameter(DisplayField field,
                                      Object[] childValues) {
        // Check for null first as an optimization
        if (childValues == null &&
            field.getName().equals(RecoveryPointsView.SS_RETENTION_BOX)) {
            // Skip it
        } else {
            super.mapRequestParameter(field, childValues);
        }
    }

    public boolean beginRetentionBoxDisplay(ChildDisplayEvent event) {
        TraceUtil.trace3("Entering");

        CCHiddenField hidden =
            (CCHiddenField) getChild(RecoveryPointsView.SS_RETENTION_VALUE);
        CCCheckBox checkBox  =
            (CCCheckBox) getChild(RecoveryPointsView.SS_RETENTION_BOX);

        checkBox.setChecked("true".equals(hidden.getValue()));

        TraceUtil.trace3("Exiting");
        return true;
    }
}
