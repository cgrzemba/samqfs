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

// ident	$Id: DiskCopyOptionsView.java,v 1.12 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.web.ui.view.html.CCDropDownMenu;
import java.util.List;

public class DiskCopyOptionsView extends CopyOptionsViewBase {
    private static final String PS_XML =
        "/jsp/archive/DiskCopyOptionsPropertySheet.xml";

    public DiskCopyOptionsView(View parent, String name) {
        super(parent, PS_XML, name);
    }

    protected void initializeDropDownMenus() {
        // sort method
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(SORT_METHOD);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.SortMethod.labels,
            SelectableGroupHelper.SortMethod.values));

        // unarchive time reference
        dropDown = (CCDropDownMenu)getChild(UNARCHIVE_TIME_REF);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.UATimeReference.labels,
            SelectableGroupHelper.UATimeReference.values));

        // offline copy method
        dropDown = (CCDropDownMenu)getChild(OFFLINE_METHOD);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.OfflineCopyMethod.labels,
            SelectableGroupHelper.OfflineCopyMethod.values));

        // maximum size for archive units
        dropDown = (CCDropDownMenu)getChild(MAX_SIZE_ARCHIVE_UNIT);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Sizes.labels,
            SelectableGroupHelper.Sizes.values));

        // start age unit
        dropDown = (CCDropDownMenu)getChild(START_AGE_UNIT);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Times.labels,
            SelectableGroupHelper.Times.values));

        // start size unit
        dropDown = (CCDropDownMenu)getChild(START_SIZE_UNIT);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Sizes.labels,
            SelectableGroupHelper.Sizes.values));
    }

    public void loadCopyOptions() throws SamFSException {
        loadCommonCopyOptions();
    }

    public List validateCopyOptions() throws SamFSException {
        List errors = super.validateCommonCopyOptions();

        // if now errors were encountered, persist the copy changes
        if (errors.size() == 0) {
            ArchivePolicy thePolicy =
                getCurrentArchiveCopy().getArchivePolicy();

            thePolicy.updatePolicy();
        }

        return errors;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        initializeDropDownMenus();
    }

    /**
     * imlements the CCpagelet interface
     *
     * @see com.sun.web.ui.common.CCPagelet#getPagelet
     */
    public String getPageletUrl() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        Integer copyMediaType = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE);

        if (copyMediaType.intValue() == BaseDevice.MTYPE_DISK ||
            copyMediaType.intValue() == BaseDevice.MTYPE_STK_5800) {
            return "/jsp/archive/DiskCopyOptionsPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
