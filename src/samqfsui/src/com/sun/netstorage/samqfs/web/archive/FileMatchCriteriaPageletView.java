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

// ident	$Id: FileMatchCriteriaPageletView.java,v 1.8 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.web.ui.common.CCPagelet;

public class FileMatchCriteriaPageletView extends RequestHandlingViewBase
    implements CCPagelet {

    public static final String FILE_MATCH_VIEW = "FileMatchCriteriaView";

    public FileMatchCriteriaPageletView(View parent, String name) {
        super(parent, name);

        registerChildren();
    }

    public void registerChildren() {
        registerChild(FILE_MATCH_VIEW, FileMatchCriteriaView.class);
    }

    public View createChild(String name) {
        if (name.equals(FILE_MATCH_VIEW)) {
            return new FileMatchCriteriaView(this, name);
        } else {
            throw new IllegalArgumentException("invalid '" + name + "'");
        }
    }

    public void populateTableModel() {
        FileMatchCriteriaView fmcv =
            (FileMatchCriteriaView)getChild(FILE_MATCH_VIEW);

        fmcv.populateTableModel();
    }

    public String getPageletUrl() {
        Integer policyType = (Integer)
            getParentViewBean().getPageSessionAttribute(
                Constants.Archive.POLICY_TYPE);
        short type = policyType.shortValue();

        if (type == ArSet.AR_SET_TYPE_GENERAL ||
            type == ArSet.AR_SET_TYPE_NO_ARCHIVE) {
            return "/jsp/archive/FileMatchCriteriaPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
