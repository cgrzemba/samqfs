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

// ident	$Id: AdvancedNetworkConfigSetupTiledView.java,v 1.8 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCDropDownMenu;

/**
 * The ContainerView used to register and create children for "column"
 * elements defined in the model's XML document.
 *
 * This ContainerView demonstrates how to evoke JATO's TiledView
 * behavior, which automatically appends the current model row index
 * to each child's qualified name. This approach can be useful when
 * retrieving values from rows of text fields, for example.
 *
 * Note that "column" element children must be in a separate
 * ContainerView when implementing JATO's TiledView. Otherwise,
 * HREFs and buttons used for table actions will be incorrectly
 * indexed as if they were also tiled. This results in JATO
 * throwing RuntimeException due to malformed qualified field
 * names because the tile index is not found during a request
 * event.
 *
 * That said, a separate ContainerView object is not necessary
 * when TiledView behavior is not required. All children can be
 * registered and created in the same ContainerView. By default,
 * CCActionTable will attempt to retrieve all children from the
 * ContainerView given to the CCActionTable.setContainerView method.
 *
 */

/**
 * This is the TiledView class for Server Selection Page. This is used to
 * identify if an alarm HREF link is needed. Also this tiledview class is needed
 * to avoid concurrency issue.
 */

public class AdvancedNetworkConfigSetupTiledView extends CommonTiledViewBase {

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AdvancedNetworkConfigSetupTiledView(
        View parent, CCActionTableModel model, String name) {
        super(parent, model, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Method to append alt text to images due to 508 compliance
     */
    public boolean beginPrimaryIPDropDownDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        OptionList optionList = null;
        String hostName = (String) getDisplayFieldValue("NameText");

        CCDropDownMenu primaryDropDown =
            (CCDropDownMenu) getChild("PrimaryIPDropDown");
        CCDropDownMenu secondaryDropDown =
            (CCDropDownMenu) getChild("SecondaryIPDropDown");

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();

            String [] addresses = fsManager.getIPAddresses(hostName);
            optionList = new OptionList(addresses, addresses);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginPrimaryIPDropDownDisplay",
                "Failed to retrieve IPs of Metadata servers in tiled view!",
                (String) getParentViewBean().getPageSessionAttribute(
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME));
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "AdvancedNetworkConfig.setup.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                (String) getParentViewBean().getPageSessionAttribute(
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME));
        }

        primaryDropDown.setOptions(optionList);
        secondaryDropDown.setOptions(optionList);

        // For secondary ip drop down list, users are not forced to select an
        // IP and they can choose "--" as the option
        secondaryDropDown.setLabelForNoneSelected("--");
        return true;
    }
}
