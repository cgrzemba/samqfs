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

// ident	$Id: NewWizardFSTypeView.java,v 1.2 2008/10/22 20:57:04 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.io.IOException;
import javax.servlet.ServletException;

public class NewWizardFSTypeView extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewWizardFSTypeView";

    // child views
    public static final String ALERT = "Alert";
    public static final String OK = "Ok";
    public static final String QFS = "QFSRadioButton";
    public static final String UFS = "UFSRadioButton";
    public static final String ARCHIVING = "archivingCheckBox";
    public static final String SHARED = "sharedCheckBox";
    public static final String HPC = "HPCCheckBox";
    public static final String HAFS = "HAFSCheckBox";
    public static final String MATFS = "matfsCheckBox";
    public static final String ARCHIVE_MEDIA = "hasArchiveMedia";
    public static final String ARCHIVE_MEDIA_WARNING = "archiveMediaWarning";

    public NewWizardFSTypeView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardFSTypeView(View parent, Model model, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(ALERT, CCAlertInline.class);
        registerChild(QFS, CCRadioButton.class);
        registerChild(UFS, CCRadioButton.class);
        registerChild(ARCHIVING, CCCheckBox.class);
        registerChild(SHARED, CCCheckBox.class);
        registerChild(HPC, CCCheckBox.class);
        registerChild(HAFS, CCCheckBox.class);
        registerChild(MATFS, CCCheckBox.class);
        registerChild(ARCHIVE_MEDIA, CCHiddenField.class);
        registerChild(ARCHIVE_MEDIA_WARNING, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(QFS)) {
            CCRadioButton rbutton = new CCRadioButton(this, QFS, "qfs");
            rbutton.setOptions(new OptionList(
                                  new String[] {"FSWizard.new.fstype.qfs"},
                                  new String[] {"qfs"}));
            return rbutton;
        } else if (name.equals(UFS)) {
            CCRadioButton rbutton = new CCRadioButton(this, QFS, "qfs");
            rbutton.setOptions(new OptionList(
                                   new String[] {"FSWizard.new.fstype.ufs"},
                                   new String[] {"ufs"}));
            return rbutton;
        } else if (name.equals(ARCHIVING)||
            name.equals(SHARED)||
            name.equals(HPC) ||
            name.equals(HAFS) ||
            name.equals(MATFS)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(ARCHIVE_MEDIA) ||
                   name.equals(ARCHIVE_MEDIA_WARNING)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // determine if the server currently being managed has access to
        // archiving media. This is necessary inorder to warn the user in cases
        // where this is not the case.
        try {
            SamQFSSystemModel model = SamUtil.getModel(getServerName());
            boolean hasMedia = model.hasArchivingMedia();

            String msg = SamUtil
                .getResourceString("FSWizard.new.fstype.archivemedia.missing");
            ((CCHiddenField)getChild(ARCHIVE_MEDIA)).setValue(hasMedia);
            ((CCHiddenField)getChild(ARCHIVE_MEDIA_WARNING)).setValue(msg);
        } catch (SamFSException sfe) {
            TraceUtil.trace1("An Exception was caught while trying to check"
                             + " for availability of archiving media.");
            TraceUtil.trace2("Reason:" + sfe.getMessage());
        }
    }

    // implement CCWizardPage
    public String getPageletUrl() {
        return "/jsp/fs/NewWizardFSType.jsp";
    }

    /**
     * HA-FS is only supported on sun cluster.
     */
    public boolean beginHAFSCheckBoxDisplay(ChildDisplayEvent evt)
        throws ServletException, IOException {
        boolean clusterNode = false;

        try {
            clusterNode = SamUtil.isClusterNode(getServerName());
        } catch (SamFSException sfe) {
            TraceUtil.trace1("Exception caught checking if server is part" +
                             " of a cluster configuration");
            TraceUtil.trace1("Reason : " + sfe.getMessage());
        }

        return clusterNode;
    }

    /**
     * OSD-fs (mb) is only supported in 5.0+ servers
     */
    public boolean beginHPCCheckBoxDisplay(ChildDisplayEvent evt)
        throws ServletException, IOException {
        return is50OrNewer();
    }

    /**
     * MAT-fs (mat) is only supported in 5.0+ servers
     */
    public boolean beginMatfsCheckBoxDisplay(ChildDisplayEvent evt)
        throws ServletException, IOException {
        return is50OrNewer();
    }


    /**
     * retrieve the server name from the wizard model
     */
    private String getServerName() {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String serverName = (String)wm.getValue(Constants.Wizard.SERVER_NAME);

        return serverName;
    }

    /**
     * determine if the current server has the 5.0 core packages or newer
     */
    private boolean is50OrNewer() {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String apiVersion = (String)
            wm.getValue(Constants.Wizard.SERVER_API_VERSION);

        apiVersion = apiVersion == null ? "0.0" : apiVersion.substring(0, 3);
        return (apiVersion.compareTo("1.6") >= 0);
    }
}
