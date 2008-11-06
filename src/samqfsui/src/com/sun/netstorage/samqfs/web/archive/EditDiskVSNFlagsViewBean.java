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

// ident	$Id: EditDiskVSNFlagsViewBean.java,v 1.12 2008/11/06 00:47:07 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import java.io.IOException;
import javax.servlet.ServletException;

public class EditDiskVSNFlagsViewBean extends CommonSecondaryViewBeanBase {
    private static final String PAGE_NAME = "EditDiskVSNFlags";
    private static final String DEFAULT_URL =
        "/jsp/archive/EditDiskVSNFlags.jsp";

    // child names
    public static final String BAD_MEDIA = "badMedia";
    public static final String UNAVAILABLE = "unavailable";
    public static final String READ_ONLY = "readOnly";
    public static final String LABELED = "labeled";
    public static final String UNKNOWN = "unknown";
    public static final String REMOTE = "remote";
    public static final String SUBMIT = "Submit";
    public static final String CANCEL = "Cancel";

    public static final String VSN_NAME = "selected_vsn_name";

    private static final String psFileName =
        "/jsp/archive/EditDiskVSNFlagsPropertySheet.xml";

    private static final String ptFileName =
        "/jsp/archive/EditDiskVSNFlagsPageTitle.xml";

    private CCPropertySheetModel psModel = null;
    private CCPageTitleModel ptModel = null;

    public EditDiskVSNFlagsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        psModel = PropertySheetUtil.createModel(psFileName);
        ptModel = PageTitleUtil.createModel(ptFileName);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();

        registerChild(VSN_NAME, CCHiddenField.class);
        PropertySheetUtil.registerChildren(this, psModel);
        PageTitleUtil.registerChildren(this, ptModel);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(VSN_NAME)) {
            return new CCHiddenField(this, name, null);
        } else if (PropertySheetUtil.isChildSupported(psModel, name)) {
            return PropertySheetUtil.createChild(this, psModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else  if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(evt);

        // retrieve server name and the name of the vsn being edited
        String serverName = getServerName();
        String vsnName = getSelectedVSNName();

        TraceUtil.trace3("Server Name = " + serverName);
        TraceUtil.trace3("selecte vsn = " + vsnName);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            DiskVolume vsn = sysModel.
                getSamQFSSystemMediaManager().getDiskVSN(vsnName);

            ((CCCheckBox)getChild(BAD_MEDIA)).setChecked(vsn.isBadMedia());
            ((CCCheckBox)getChild(UNAVAILABLE)).setChecked(vsn.isUnavailable());
            ((CCCheckBox)getChild(READ_ONLY)).setChecked(vsn.isReadOnly());
            ((CCCheckBox)getChild(LABELED)).setChecked(vsn.isLabeled());
            ((CCCheckBox)getChild(UNKNOWN)).setChecked(vsn.isUnknown());
            ((CCCheckBox)getChild(REMOTE)).setChecked(vsn.isRemote());
        } catch (SamFSException sfe) {
        	SamUtil.processException(sfe,
                                     getClass(),
                                     "beginDisplay",
                                     "Error retrieving disk vsn media flags",
                                     serverName);
        }

        // disable submit button if no media authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {

            ((CCButton)getChild(SUBMIT)).setDisabled(true);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     *  handler for the edit media flags popup submit button
     *
     */
    public void handleSubmitRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

    	// get server name
        String serverName = getServerName();
        String vsnName = getSelectedVSNName();

        // break the flags string into its component values
        boolean badMedia = ((CCCheckBox)getChild(BAD_MEDIA)).isChecked();
        boolean unavailable = ((CCCheckBox)getChild(UNAVAILABLE)).isChecked();
        boolean readOnly = ((CCCheckBox)getChild(READ_ONLY)).isChecked();
        boolean labeled = ((CCCheckBox)getChild(LABELED)).isChecked();

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
                                 ALERT,
                                 "success.summary",
                                 SamUtil.getResourceString(
                                     "archiving.diskvsn.mediachange.success",
                                     vsnName),
                                 serverName);
            setSubmitSuccessful(true);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleEditMediaFlagsRequest",
                                     "Error modifying media flags",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                   ALERT,
                                   SamUtil.getResourceString(
                                       "archiving.diskvsn.mediachange.failure",
                                       vsnName),
                                   sfe.getSAMerrno(),
                                   sfe.getMessage(),
                                   serverName);
        }

        TraceUtil.trace3("Exiting");
        forwardTo(getRequestContext());
    }

    /**
     *  retrieve the name of disk volume whose attributes we are setting
     *
     * @return vsnName
     */
    protected String getSelectedVSNName() {
    	String vsnName = (String)getPageSessionAttribute(VSN_NAME);

    	if (vsnName == null) {
    		vsnName =
    			RequestManager.getRequest().getParameter(VSN_NAME);
    		setPageSessionAttribute(VSN_NAME, vsnName);
    	}

    	return vsnName;
    }
}
