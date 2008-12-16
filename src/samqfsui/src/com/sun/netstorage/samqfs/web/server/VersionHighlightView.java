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

// ident	$Id: VersionHighlightView.java,v 1.15 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.MultiTableViewBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;

import java.util.HashMap;
import java.util.Map;
import javax.servlet.http.HttpSession;

/**
 * This is the view class of the Version Highlight page
 */
public class VersionHighlightView extends MultiTableViewBase {

    public static final String TILED_VIEW = "VersionHighlightTiledView";
    public static final String VERSION_TABLE = "VersionHighlightTable";

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public VersionHighlightView(View parent, Map models, String name) {
        super(parent, models, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(TILED_VIEW, VersionHighlightTiledView.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering: name is ").append(name).toString());

        if (name.equals(TILED_VIEW)) {
            return new VersionHighlightTiledView(
                this, getTableModel(VERSION_TABLE), name);
        } else if (name.equals(VERSION_TABLE)) {
            return createTable(name, TILED_VIEW);
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null)
                return model.createChild(this, name);
        }

        // child with no known parent
        throw new IllegalArgumentException("invalid child '" + name + "'");
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        initializeTableHeaders();
        TraceUtil.trace3("Exiting");
    }

    private void initializeTableHeaders() {
        CCActionTableModel model = getTableModel(VERSION_TABLE);
        model.setRowSelected(false);

        model.setActionValue(
                "NameColumn",
                "VersionHighlight.heading1");
        model.setActionValue(
                "FirstColumn",
                "VersionHighlight.heading2");
        model.setActionValue(
                "SecondColumn",
                "VersionHighlight.heading3");
        model.setActionValue(
                "ThirdColumn",
                "VersionHighlight.heading4");
         model.setActionValue(
            "SupportColumn",
            "VersionHighlight.heading5");
    }


    public void populateTableModels() throws SamFSException {
        populateVersionTable();
    }

    public void populateVersionTable() throws SamFSException {
        CCActionTableModel model = getTableModel(VERSION_TABLE);
        model.clear();

        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        HashMap myHashMap = (HashMap) session.getAttribute(
            Constants.SessionAttributes.VERSION_HIGHLIGHT);

        if (myHashMap == null) {
            // Start parsing the XML file

            try {
                SAXHandler.parseIt();
            } catch (SamFSException ex) {
                // Failed to parse version highlight XML file
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
                    "VersionHighlight.error",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    "");
                return;
            }

            // Retrieve the HashMap after parsing the XML file
            myHashMap = SAXHandler.getHashMap();

            // Save hashMap into Session
            session.setAttribute(
                Constants.SessionAttributes.VERSION_HIGHLIGHT, myHashMap);
        }

        for (int i = 0; i < myHashMap.size(); i++) {
            if (i > 0) {
                model.appendRow();
            }

            HighlightInfo highlightInfo =
                (HighlightInfo) myHashMap.get(new Integer(i));

            String featureType = highlightInfo.getFeatureType();
            featureType = featureType == null ? "" : featureType;

            // pre-populate blank images into the model,
            // overwrite them later if necessary
            model.setValue("FirstImage", Constants.Image.ICON_BLANK);
            model.setValue("SecondImage", Constants.Image.ICON_BLANK);
            model.setValue("ThirdImage", Constants.Image.ICON_BLANK);
            model.setValue("SupportText", "");

            // populate the feature name
            if (featureType.equals("summary")) {
                model.setValue(
                    "NameText",
                    new NonSyncStringBuffer("<b>").append(
                    SamUtil.getResourceString(highlightInfo.getFeatureName())).
                    append("</b>").toString());
            } else {
                String heading = "---- ";
                if (featureType.equals("detail")) {
                    heading = "-- ";
                }
                model.setValue(
                    "NameText",
                    new NonSyncStringBuffer(heading).append(
                    SamUtil.getResourceString(highlightInfo.getFeatureName())).
                    toString());
                model.setValue(
                    "SupportText",
                    highlightInfo.getServerVersion());

                String [] versionInfoArray =
                    highlightInfo.getVersionInfo().split("###");

                for (int j = 0; j < versionInfoArray.length; j++) {
                    String [] versionInfo = versionInfoArray[j].split(",");

                    if (versionInfo[0].equals(
                        "version.highlight.versionnumber.50")) {
                        model.setValue(
                            "FirstImage",
                            getImage(versionInfo[1]));
                    } else if (versionInfo[0].equals(
                        "version.highlight.versionnumber.46")) {
                        model.setValue(
                            "SecondImage",
                            getImage(versionInfo[1]));
                        model.setValue(
                            "FirstImage",
                            Constants.Image.ICON_AVAILABLE);
                    } else if (versionInfo[0].equals(
                        "version.highlight.versionnumber.45")) {
                        model.setValue(
                            "ThirdImage",
                            getImage(versionInfo[1]));
                        model.setValue(
                            "FirstImage",
                            Constants.Image.ICON_AVAILABLE);
                        model.setValue(
                            "SecondImage",
                            Constants.Image.ICON_AVAILABLE);
                    } else {
                        model.setValue(
                            "ThirdImage",
                            Constants.Image.ICON_AVAILABLE);
                        model.setValue(
                            "FirstImage",
                            Constants.Image.ICON_AVAILABLE);
                        model.setValue(
                            "SecondImage",
                            Constants.Image.ICON_AVAILABLE);
                    }
                }
            }
        }
    }

    private String getImage(String status) {
        if (status.equals("version.highlight.versionstatus.new")) {
            return Constants.Image.ICON_NEW;
        } else if (status.equals("version.highlight.versionstatus.upgrade")) {
            return Constants.Image.ICON_UPGRADE;
        } else if (status.equals("version.highlight.versionstatus.available")) {
            return Constants.Image.ICON_AVAILABLE;
        } else {
            return Constants.Image.ICON_BLANK;
        }
    }

} // end of VersionHighlightView class
