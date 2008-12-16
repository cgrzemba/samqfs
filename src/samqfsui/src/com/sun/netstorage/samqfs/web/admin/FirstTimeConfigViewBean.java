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

// ident	$Id: FirstTimeConfigViewBean.java,v 1.3 2008/12/16 00:10:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Configuration;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCImageField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

public class FirstTimeConfigViewBean extends CommonViewBeanBase {
    public static final String PAGE_NAME = "FirstTimeConfig";
    public static final String DEFAULT_URL = "/jsp/admin/FirstTimeConfig.jsp";

    // child views
    public static final String VIEW_NAME = "FirstTimeConfigView";
    public static final String PAGE_TITLE = "PageTitle";
    public static final String MORE_HREF = "moreHref";
    public static final String MORE_TEXT = "moreText";
    public static final String MORE_IMG = "moreImg";
    public static final String INSTRUCTION_TEXT = "instruction";
    public static final String NEARLINE = "nearline";
    public static final String OSD = "osd";
    public static final String EMAIL = "email";
    public static final String HREF = "Href";
    public static final String ADD_LIBRARY_HREF = "addLibraryHref";
    public static final String IMPORT_HREF = "importHref";
    public static final String DISK_VOLUME_HREF = "diskVolumeHref";
    public static final String CREATE_FS_HREF = "createFSHref";
    public static final String RECOVERY_POINT_HREF = "rpsHref";
    public static final String EMAIL_HREF = "emailHref";
    public static final String TEXT = "Text";
    public static final String LINE = "line";
    public static final String IMAGE = "Img";
    public static final String REQUIRED = "requiredLabel";

    // help text
    public static final String LIBRARY_HELP = "addLibraryHelpText";
    public static final String TAPE_HELP = "importHelpText";
    public static final String DISK_VOLUME_HELP = "diskVolumeHelpText";
    public static final String POOL_HELP = "poolHelpText";

    public static final String SERVER_NAME = "serverName";

    // keep track of the configuration object to prevent multiple C-API calls
    // from within the same request cycle
    private Configuration config = null;

    public FirstTimeConfigViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(VIEW_NAME, FirstTimeConfigView.class);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(MORE_HREF, CCHref.class);
        registerChild(ADD_LIBRARY_HREF, CCHref.class);
        registerChild(IMPORT_HREF, CCHref.class);
        registerChild(DISK_VOLUME_HREF, CCHref.class);
        registerChild(CREATE_FS_HREF, CCHref.class);
        registerChild(RECOVERY_POINT_HREF, CCHref.class);
        registerChild(EMAIL_HREF, CCHref.class);
        registerChild(MORE_TEXT, CCStaticTextField.class);
        registerChild(MORE_IMG, CCImageField.class);
        registerChild(INSTRUCTION_TEXT, CCStaticTextField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.endsWith(HREF)) {
            return new CCHref(this, name, null);
        } else if (name.endsWith(TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(NEARLINE) ||
            name.equals(OSD) ||
            name.equals(EMAIL) ||
            name.equals(REQUIRED)) {
            return new CCLabel(this, name, null);
        } else if (name.endsWith(IMAGE) ||
            name.startsWith(LINE)) {
            return new CCImageField(this, name, null);
        } else if (name.equals(INSTRUCTION_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(VIEW_NAME)) {
            return new FirstTimeConfigView(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), null);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    public Configuration getConfiguration() {
        TraceUtil.trace3("Entering");
        if (this.config == null) {
            String serverName = getServerName();
            try {
                SamQFSSystemModel model = SamUtil.getModel(serverName);
                this.config = model.getSamQFSSystemAdminManager()
                    .getConfigurationSummary();
            } catch (SamFSException sfe) {
                // log
                SamUtil.processException(sfe,
                                         this.getClass(),
                                         "getConfiguration",
                                 "Unable to retrieve configuration summary",
                                         serverName);
            }
        }

        TraceUtil.trace3("Exiting");
        return this.config;
    }

    // command handlers to intercept any submissions caused by bad javascript

    /** handle the more info href */
    public void handleMoreHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleAddLibraryHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleImportHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleDiskVolumeHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleCreateFSHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleRpsHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleEmailHrefRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        Configuration config = getConfiguration();

        if (config != null) {
            CCStaticTextField field = (CCStaticTextField)getChild(LIBRARY_HELP);
            // library count
            if (config.getLibraryCount() > 0) {
                field.setValue(
                    SamUtil.getResourceString("firsttime.library.help",
                                              "" + config.getLibraryCount()));
            } else {
                field.setValue(null);
            }

            // vsn count
            field = (CCStaticTextField)getChild(TAPE_HELP);
            if (config.getTapeCount() > 0) {
                field.setValue(
                    SamUtil.getResourceString("firsttime.tape.help", "" +
                                              config.getTapeCount()));
            } else {
                field.setValue(null);
            }

            // disk volume count
            field = (CCStaticTextField)getChild(DISK_VOLUME_HELP);
            if (config.getDiskVolumeCount() > 0) {
                field.setValue(
                    SamUtil.getResourceString("firsttime.diskvolume.help", "" +
                                              config.getDiskVolumeCount()));
            } else {
                field.setValue(null);
            }

            // volue pool count
            field = (CCStaticTextField)getChild(POOL_HELP);
            if (config.getVolumePoolCount() > 0) {
                field.setValue(SamUtil.getResourceString("firsttime.pool.help",
					   "" + config.getVolumePoolCount()));
            } else {
                field.setValue(null);
            }
        } // end if config != null


        // save the server name for popups
        ((CCHiddenField)getChild(SERVER_NAME)).setValue(serverName);

        TraceUtil.trace3("Exiting");
    }
}
