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

// ident	$Id: ISAllsetsPolicyDetailsView.java,v 1.7 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.util.ArrayList;
import java.util.List;

public class ISAllsetsPolicyDetailsView extends RequestHandlingViewBase
                                        implements CCPagelet, CopyFields {

    private static final String TILED_VIEW = "ISAllsetsPolicyDetailsTiledView";
    private static final String TV_PREFIX = "tiledViewPrefix";
    private static final String INSTRUCTION = "instruction";
    private static final String LABEL_SUFFIX = "Label";

    public ISAllsetsPolicyDetailsView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        registerChild(TILED_VIEW, ISAllsetsPolicyDetailsTiledView.class);
        registerChild(TV_PREFIX, CCHiddenField.class);
        registerChild(COPY_LIST, CCHiddenField.class);
        registerChild(UI_RESULT, CCHiddenField.class);
    }

    public View createChild(String name) {
        if (name.endsWith(LABEL_SUFFIX)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(TV_PREFIX) ||
                   name.equals(COPY_LIST) ||
                   name.equals(UI_RESULT)) {
            return new CCHiddenField(this, name, TILED_VIEW);
        } else if (name.equals(INSTRUCTION)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(TILED_VIEW)) {
            return new ISAllsetsPolicyDetailsTiledView(this, name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // allsets policy description
        ((CCStaticTextField)getChild(INSTRUCTION))
            .setValue("archiving.policy.basepolicy.instruction");
    }

    private String getPoolName(String exp) {
        String result = "";
        if (!exp.equals(SelectableGroupHelper.NOVAL)) {
            String []s = exp.split(":");

            result = s[0];
        }

        return result;
    }

    private int getPoolMediaType(String exp) {
        int mediaType = -1;

        if (!exp.equals(SelectableGroupHelper.NOVAL)) {
            String [] s = exp.split(":");

            mediaType = Integer.parseInt(s[1]);
        }

        return mediaType;
    }


    /**
     * handle the save button
     */
    public List save() {
        List errors = new ArrayList();
        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = parent.getPolicyName();

        DefaultModel model = (DefaultModel)
            ((ISAllsetsPolicyDetailsTiledView)getChild(TILED_VIEW))
            .getPrimaryModel();

        try {
            ArchivePolicy thePolicy = SamUtil.getModel(serverName)
                .getSamQFSSystemArchiveManager()
                .getArchivePolicy(policyName);

            // loop through all the copies and save their vsn maps
            for (int i = 0; i < 5; i++) {
                int copyNumber = i == 0 ? 5 : i;
                model.setLocation(i);

                ArchiveCopy theCopy = thePolicy.getArchiveCopy(copyNumber);
                ArchiveVSNMap theMap = theCopy.getArchiveVSNMap();
                String mediaPool =
                    getPoolName((String)model.getValue(MEDIA_POOL));
                int mediaType =
                    getPoolMediaType((String)model.getValue(MEDIA_POOL));

                String scratchPool =
                    getPoolName((String)model.getValue(SCRATCH_POOL));

                String poolExpression = "";

                if (!mediaPool.equals("")) {
                    poolExpression = poolExpression.concat(mediaPool);

                    if (!scratchPool.equals("")) {
                        poolExpression.concat(",").concat(scratchPool);
                    }

                    // set media type
                    theMap.setArchiveMediaType(mediaType);
                    theMap.setPoolExpression(poolExpression);
                }

                boolean enableRecycling = Boolean
                    .valueOf((String)model.getValue(ENABLE_RECYCLING))
                    .booleanValue();
                theCopy.setIgnoreRecycle(!enableRecycling);
            }
        } catch (SamFSException sfe) {
            // TODO
        } catch (ModelControlException mce) {
            // TODO
        }

        return errors;
    }

    public String getPageletUrl() {
        String jsp = null;

        short policyType =
            ((ISPolicyDetailsViewBean)getParentViewBean()).getPolicyType();
        if (policyType == ArSet.AR_SET_TYPE_ALLSETS_PSEUDO) {
            jsp = "/jsp/archive/ISAllsetsPolicyDetailsPagelet.jsp";
        } else {
            jsp = "/jsp/archive/BlankPagelet.jsp";
        }

        return jsp;
    }
}
