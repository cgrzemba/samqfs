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

// ident	$Id: FSWizardDeviceSelectionPageTiledView.java,v 1.7 2008/03/17 14:43:35 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.web.ui.model.CCActionTableModel;

/**
 * This class is a Tiled view class for FSWizardDeviceSelectionPageView
 * actiontable
 */

public class FSWizardDeviceSelectionPageTiledView extends CommonTiledViewBase {

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardDeviceSelectionPageTiledView(
        View parent, CCActionTableModel model, String name) {
        super(parent, model, name);
    }
}
