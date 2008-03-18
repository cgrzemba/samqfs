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

// ident	$Id: ISAllsetsPolicyDetailsTiledView.java,v 1.5 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;

public class ISAllsetsPolicyDetailsTiledView
    extends RequestHandlingTiledViewBase implements CopyFields {

    public ISAllsetsPolicyDetailsTiledView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        setMaxDisplayTiles(5);
        setPrimaryModel((DefaultModel)getDefaultModel());
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(MEDIA_POOL, CCDropDownMenu.class);
        registerChild(SCRATCH_POOL, CCDropDownMenu.class);
        registerChild(ENABLE_RECYCLING, CCCheckBox.class);
        registerChild(COPY_NUMBER, CCStaticTextField.class);
        registerChild(COPY_DIV, CCStaticTextField.class);
        registerChild(COPY_OPTIONS, CCButton.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(MEDIA_POOL) ||
            name.equals(SCRATCH_POOL)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(ENABLE_RECYCLING)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(COPY_NUMBER) ||
                   name.equals(COPY_DIV)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(COPY_OPTIONS)) {
            return new CCButton(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        DefaultModel model = (DefaultModel)getPrimaryModel();
        model.setSize(5);

        String serverName = null;
        String [][] poolNames = null;
        try {
            serverName =
                ((ISPolicyDetailsViewBean)getParentViewBean()).getServerName();
            poolNames = PolicyUtil.getAllVSNPoolNames(serverName);
        } catch (SamFSException sfe) {
            // TODO:
        }

        // populate the media pool dropdown
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MEDIA_POOL);
        dropDown.setOptions(new OptionList(poolNames[0], poolNames[1]));

        // populate the scratch pool dropdown
        dropDown = (CCDropDownMenu)getChild(SCRATCH_POOL);
        dropDown.setOptions(new OptionList(poolNames[0], poolNames[1]));
    }

    /**
     * handle the edit copy options button
     */
    public void handleEditCopyOptionsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException, ModelControlException {
        int tileIndex = ((TiledViewRequestInvocationEvent)rie).getTileNumber();

        CommonViewBeanBase source =
            (CommonViewBeanBase)getParentViewBean();

        DefaultModel model = (DefaultModel)getPrimaryModel();
        model.setLocation(tileIndex);

        int copyNumber = tileIndex + 1;
        ArchiveCopy theCopy = null;
        try {
            theCopy = SamUtil.getModel(source.getServerName())
                .getSamQFSSystemArchiveManager().getArchivePolicy(
                ((ISPolicyDetailsViewBean)source).getPolicyName())
                .getArchiveCopy(copyNumber);
        } catch (SamFSException sfe) {
            // TODO
        }

        ArchiveVSNMap theMap = theCopy.getArchiveVSNMap();
        int mediaType = theMap.getArchiveMediaType();

        CommonViewBeanBase target = (CommonViewBeanBase)
            getViewBean(CopyOptionsViewBean.class);
        source.setPageSessionAttribute(Constants.Archive.COPY_NUMBER,
                                       new Integer(copyNumber));
        source.setPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE,
                                       new Integer(mediaType));
        // bread crumb
        BreadCrumbUtil.breadCrumbPathForward(source,
           PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(target);
    }

    public boolean beginCopyDivNameDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation();
        if (copyNumber == 0) {
            copyNumber = 5;
        }

        ((CCStaticTextField)getChild(COPY_DIV)).setValue(
            COPY_DIV.concat("-").concat(Integer.toString(copyNumber)));

        return true;
    }

    public boolean beginCopyNumberDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation();

        String copyString = SamUtil.getResourceString("archiving.copynumber",
            Integer.toString(copyNumber));
        if (copyNumber == 0) {
            copyNumber = 5;
            copyString = SamUtil
                .getResourceString("archiving.copynumber.allcopies");
        }

        ((CCStaticTextField)getChild(COPY_NUMBER)).setValue(copyString);
        return true;
    }

    public boolean beginMediaPoolDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        int copyNumber = getPrimaryModel().getLocation();
        if (copyNumber == 0) {
            copyNumber = 5;
        }

        ArchiveVSNMap theMap = getArchiveVSNMap(copyNumber);

        String rawPool = theMap.getPoolExpression();
        String mediaPoolName = "", scratchPoolName = "";

        if (rawPool != null) {
            String [] s = rawPool.split(",");
            if (s.length >= 2) {
                mediaPoolName = s[0].concat(":")
                    .concat(Integer.toString(theMap.getArchiveMediaType()));
                scratchPoolName = s[1].concat(":")
                    .concat(Integer.toString(theMap.getArchiveMediaType()));
            } else if (s.length == 1) {
                mediaPoolName = s[0].concat(":")
                    .concat(Integer.toString(theMap.getArchiveMediaType()));
            }
        }

        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MEDIA_POOL);
        dropDown.setValue(mediaPoolName);

        dropDown = (CCDropDownMenu)getChild(SCRATCH_POOL);
        dropDown.setValue(scratchPoolName);

        return true;
    }

    private ArchiveVSNMap getArchiveVSNMap(int copyNumber) {
        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = parent.getPolicyName();

        ArchiveVSNMap vsnMap = null;

        try {
            ArchivePolicy thePolicy = SamUtil.getModel(serverName)
                .getSamQFSSystemArchiveManager()
                .getArchivePolicy(policyName);
            vsnMap = thePolicy.getArchiveCopy(copyNumber).getArchiveVSNMap();
        } catch (SamFSException sfe) {
            // TODO:
        }

        return vsnMap;
    }
}
