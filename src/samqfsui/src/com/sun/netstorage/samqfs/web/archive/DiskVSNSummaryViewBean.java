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

// ident	$Id: DiskVSNSummaryViewBean.java,v 1.13 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * This page manages the Disk VSNs and is only available in the 4.4+ releases
 */
public class DiskVSNSummaryViewBean extends CommonViewBeanBase {
    private static final String PAGE_NAME = "DiskVSNSummary";
    private static final String DEFAULT_URL =
        "/jsp/archive/DiskVSNSummary.jsp";

    // children
    public static final String CONTAINER_VIEW = "DiskVSNSummaryView";
    public static final String PAGE_TITLE = "pageTitle";

    // hidden fields for the popup helper
    private static final String NAME = "name";
    private static final String HOST = "host";
    private static final String PATH = "path";
    private static final String CREATE_VSN = "createDiskVSN";
    public static final String VSNS = "vsns";
    public static final String MESSAGES = "messages";
    public static final String FLAGS = "flags";
    public static final String CREATE_PATH = "createPath";

    // javascript helpers
    public static final String SERVER_NAME = "server_name";
    public static final String VSN_NAME = "selected_vsn_name";
    public static final String EDIT_FLAGS = "editMediaFlags";

    /** create a new instance of this view bean */
    public DiskVSNSummaryViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register the children of this container */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(CONTAINER_VIEW, DiskVSNSummaryView.class);
        registerChild(NAME, CCHiddenField.class);
        registerChild(HOST, CCHiddenField.class);
        registerChild(PATH, CCHiddenField.class);
        registerChild(VSNS, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(MESSAGES, CCHiddenField.class);
        registerChild(CREATE_VSN, CCHref.class);
        registerChild(EDIT_FLAGS, CCHref.class);
        registerChild(VSN_NAME, CCHiddenField.class);
        registerChild(FLAGS, CCHiddenField.class);
        registerChild(CREATE_PATH, CCHiddenField.class);

        TraceUtil.trace3("Exiting");
    }

    /** create a named child */
    public View createChild(String name) {
        if (name.equals(CREATE_VSN) ||
            name.equals(EDIT_FLAGS)) {
            return new CCHref(this, name, null);
        } else if (name.equals(NAME) ||
                   name.equals(HOST) ||
                   name.equals(PATH) ||
                   name.equals(VSNS) ||
                   name.equals(SERVER_NAME) ||
                   name.equals(MESSAGES) ||
                   name.equals(VSN_NAME) ||
                   name.equals(FLAGS) ||
                   name.equals(CREATE_PATH)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CONTAINER_VIEW)) {
            return new DiskVSNSummaryView(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else {
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }
    }

    /** begin page display */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        DiskVSNSummaryView view = (DiskVSNSummaryView)getChild(CONTAINER_VIEW);
        view.populateTableModel();

        // attach the error messages for client-side validation
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        for (int i = 0; i < messages.length; i++) {
            buffer.append(SamUtil.getResourceString(messages[i])).append(";");
        }
        CCHiddenField field = (CCHiddenField)getChild(MESSAGES);
        field.setValue(buffer.toString());

        field = (CCHiddenField)getChild(SERVER_NAME);
        field.setValue(getServerName());

        TraceUtil.trace3("Exiting");
    }

    /**
     * this method is called when the create new vsn popup's submit button
     * is clicked
     */
    public void handleCreateDiskVSNRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // retrieve the server since it maybe needed multiple times
        String serverName = getServerName();

        try {
            String name = getDisplayFieldStringValue(NAME);
            String host = getDisplayFieldStringValue(HOST);
            String path = getDisplayFieldStringValue(PATH);
            String createPath = getDisplayFieldStringValue(CREATE_PATH);

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            if (createPath.equals("true") ||
                SamUtil.getModel(host).doesFileExist(path)) {
                if (serverName.equals(host)) {
                    host = null;
                }
                sysModel.getSamQFSSystemMediaManager().
                    createDiskVSN(name, host, path);
            } else {
                throw new SamFSException(
                    "archiving.diskvsn.newvsn.error.pathnonexistent", -2027);
            }

            // give user feedback that disk vsn was created successfully
            SamUtil.setInfoAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "success.summary",
                                  SamUtil.getResourceString(
                                  "archiving.diskvsn.newvsn.create.success",
                                  name),
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleCreateVSNRequest",
                                     "unable to create disk vsn",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "archiving.diskvsn.newvsn.create.failure",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
        forwardTo(getRequestContext());
    }

    /** handler for the edit media flags popup submit button */
    public void handleEditMediaFlagsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        // contains ';' delimited boolean list in the order
        // badMedia;Unvailable;readOnly;labeled
        String flags = getDisplayFieldStringValue(FLAGS);
        String vsnName = getDisplayFieldStringValue(VSN_NAME);
        String serverName = getServerName();

        // break the flags string into its component values
        boolean badMedia, unavailable, readOnly, labeled;

        String [] flagArray = flags.split(";");
        badMedia = (Boolean.valueOf(flagArray[0])).booleanValue();
        unavailable = (Boolean.valueOf(flagArray[1])).booleanValue();
        readOnly = (Boolean.valueOf(flagArray[2])).booleanValue();
        labeled = (Boolean.valueOf(flagArray[3])).booleanValue();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            DiskVolume vsn =
                sysModel.getSamQFSSystemMediaManager().getDiskVSN(vsnName);

            // set the flags
            vsn.setBadMedia(badMedia);
            vsn.setUnavailable(unavailable);
            vsn.setReadOnly(readOnly);
            vsn.setLabeled(labeled);

            sysModel.getSamQFSSystemMediaManager().updateDiskVSNFlags(vsn);

            // user feedback
            SamUtil.setInfoAlert(this,
                                 CHILD_COMMON_ALERT,
                                 "success.summary",
                                 SamUtil.getResourceString(
                                     "archiving.diskvsn.mediachange.success",
                                     vsnName),
                                 serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleEditMediaFlagsRequest",
                                     "Error modifying media flags",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                   CHILD_COMMON_ALERT,
                                   SamUtil.getResourceString(
                                       "archiving.diskvsn.mediachange.failure",
                                       vsnName),
                                   sfe.getSAMerrno(),
                                   sfe.getMessage(),
                                   serverName);
        }

        forwardTo(getRequestContext());
    }

    // error messages for client-side validation.
    // NOTE: These have to be localized at run time to ensure that they are
    // translated to right language.
    private static final String [] messages = {
        "archiving.diskvsn.newvsn.error.namenull",
        "archiving.diskvsn.newvsn.error.nameused",
        "archiving.diskvsn.newvsn.error.pathnull",
        "archiving.diskvsn.newvsn.error.pathinvalid",
        "archiving.diskvsn.newvsn.error.alphanumeric",
        "archiving.diskvsn.newvsn.error.dataipnull",
        "archiving.diskvsn.newvsn.error.portnull",
        "archiving.diskvsn.newvsn.error.portinvalid"
    };
}
