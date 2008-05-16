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

// ident	$Id: VersionHighlightTiledView.java,v 1.7 2008/05/16 18:39:05 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCImageField;


public class VersionHighlightTiledView extends CommonTiledViewBase {

    public VersionHighlightTiledView(
            View parent,
            CCActionTableModel model,
            String name) {
        super(parent, model, name);

        // Set the primary model to evoke JATO's TiledView behavior,
        // which will automatically add the current model row index to
        // each qualified name.
        setPrimaryModel(model);
    }

    /**
     * Method to append alt text to images due to 508 compliance
     */
    public boolean beginThirdImageDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        CCImageField myImageField1 = (CCImageField) getChild("FirstImage");
        CCImageField myImageField2 = (CCImageField) getChild("SecondImage");
        CCImageField myImageField3 = (CCImageField) getChild("ThirdImage");

        appendAltText(myImageField1);
        appendAltText(myImageField2);
        appendAltText(myImageField3);

        return true;
    }

    /**
     * Helper method to append alt text to an image
     */
    private void appendAltText(CCImageField myImageField) {
        String imageString = (String) myImageField.getValue();
        String altString = null;
        if (imageString.equals(Constants.Image.ICON_NEW)) {
            altString = "version.highlight.versionstatus.new";
        } else if (imageString.equals(Constants.Image.ICON_UPGRADE)) {
            altString = "version.highlight.versionstatus.upgrade";
        } else if (imageString.equals(Constants.Image.ICON_AVAILABLE)) {
            altString = "version.highlight.versionstatus.available";
        } else {
            // Blank image, no alt text
            myImageField.setAlt("");
            myImageField.setTitle("");
            return;
        }

        myImageField.setAlt(SamUtil.getResourceString(altString));
        myImageField.setTitle(SamUtil.getResourceString(altString));
    }
}
