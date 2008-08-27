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

// ident        $Id: AddClientsBean.java,v 1.2 2008/08/27 22:17:28 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.event.WizardEventListener;
import com.sun.web.ui.model.Option;
import java.io.File;
import java.util.ArrayList;
import javax.faces.event.ActionEvent;

/**
 * this bean acts as the model for the add client's wizard
 */
public class AddClientsBean {
    // constants --
    public static final String FS_NAME =
        Constants.PageSessionAttributes.FS_NAME;
    public static final String JOB_ID_KEY = "addclient.multihost.jobid";

    // host selection method
    public static final String METHOD_NAME = "hostname";
    public static final String METHOD_IP = "ipaddress";
    public static final String METHOD_FILE = "file";

    // mount options
    public static final String READ_ONLY = "ro";
    public static final String BOOT_TIME = "boot";
    public static final String BACKGROUND = "bg";

    // the actual values the wizard is interested in
    private String selectedMethod = null;
    private String [] selectedHosts = null;
    private String mountPoint = null;
    private boolean mountAfterAdd = false;
    private boolean  mountReadOnly = false;
    private boolean  mountAtBootTime = false;
    private boolean mountInBackground = false;

    // file chooser
    private File currentDirectory = null;
    private File selectedFile = null;

    private String fileName = null;

    // alert message
    private AlertBean alertBean = null;

    // the current file system
    private String fsName = null;
    private String serverName = null;

    public AddClientsBean() {
        this.selectedMethod = METHOD_NAME;
    }

    // event handlers
    public WizardEventListener getWizardEventListener() {
        return new AddClientsWizardEventListener(this);
    }

    public WizardEventListener getWizardStepEventListener() {
        return new AddClientsWizardEventListener(this);
    }

    /** re-initialize the wizard to its initial settings */
    public void clearWizardValues() {
        this.selectedMethod = METHOD_NAME;
        this.selectedHosts =  null;
        this.mountPoint = null;
        this.mountAfterAdd = false;
        this.mountReadOnly = false;
        this.mountAtBootTime = false;
        this.mountInBackground = false;
    }

    /**
     * host selection method radio button options
     */
    public Option [] getHostSelectionItems() {
        Option [] options = new Option [] {
            new Option(METHOD_NAME,
                JSFUtil.getMessage("fs.addclients.selectionmethod.hostname")),
            new Option(METHOD_IP,
                JSFUtil.getMessage("fs.addclients.selectionmethod.ipaddress")),
            new Option(METHOD_FILE,
                JSFUtil.getMessage("fs.addclients.selectionmethod.file"))};

        return options;
    }

    /** retrieve the selected selection method */
    public String getSelectedMethod() {
        return this.selectedMethod;
    }

    /** set the selected selection method */
    public void setSelectedMethod(String value) {
        this.selectedMethod = value;
    }

    public String [] getHostList() {
        return this.selectedHosts;
    }

    public String []  getSelectedHosts() {
        return this.selectedHosts;
    }

    public void setSelectedHosts(String [] hosts) {
        this.selectedHosts = hosts;
    }

    public Option [] getMountOptionList() {
        Option [] options = new Option [] {
            new Option(READ_ONLY,
                JSFUtil.getMessage("fs.addclients.mountoptions.readonly")),
            new Option(BOOT_TIME,
                JSFUtil.getMessage("fs.addclients.mountoptions.boottime")),
            new Option(BACKGROUND,
                JSFUtil.getMessage("fs.addclients.mountoptions.background"))};

        return options;
    }

    public void setMountPoint(String mp) {
        this.mountPoint = mp;
    }

    public void setMountAfterAdd(boolean set) {
        this.mountAfterAdd = set;
    }

    public boolean getMountAfterAdd() {
        return this.mountAfterAdd;
    }

    public String [] getSelectedMountOptions() {
        ArrayList<String> options = new ArrayList<String>();
        if (mountReadOnly) {
            options.add(READ_ONLY);
        }
        if (mountAtBootTime) {
            options.add(BOOT_TIME);
        }
        if (mountInBackground) {
            options.add(BACKGROUND);
        }

        String [] result = new String[options.size()];
        result = (String [])options.toArray(result);

        return result;
    }

    public void setSelectedMountOptions(String [] options) {
        if (options != null && options.length > 0) {
            // determine which checkboxes are check
            for (int i = 0; i < options.length; i++) {
                if (READ_ONLY.equals(options[i])) {
                    mountReadOnly = true;
                } else if (BOOT_TIME.equals(options[i])) {
                    mountAtBootTime = true;
                } else if (BACKGROUND.equals(options[i])) {
                    mountInBackground = true;
                }
            }
        }
    }

    // TODO: integrate file chooser
    public String getFileName() {
        return this.fileName;
    }

    public void setFileName(String s) {
        this.fileName = s;
    }

    // resource strings ...
    public String getIpAddressLabel() {
        return JSFUtil.getMessage("fs.addclients.byip.ipaddress");
    }

    public String getSelectedIPAddressesLabel() {
        return JSFUtil.getMessage("fs.addclients.byip.selectedips");
    }

    public String getClientListLabel() {
        return JSFUtil.getMessage("fs.addclients.clientlist.clientlist");
    }

    // file chooser
    public File getFileBrowserDirectory() {
        if (this.currentDirectory == null)
            return new File("/"); // start at the root directory

        return this.currentDirectory;
    }

    public void setFileBrowserDirectory(File dir) {
        this.currentDirectory = dir;
    }

    public File getSelectedFile() {
        return this.selectedFile;
    }

    public void setSelectedFile(File file) {
        this.selectedFile = file;
    }

    public void handleChooseFile(ActionEvent evt) {
        System.out.println("handle choose file");
    }


    // values for the summary page
    public Option [] getSelectedHostSummary() {
        Option [] options = new Option[this.selectedHosts.length];
        for (int i = 0; i < this.selectedHosts.length; i++) {
            options[i] = new Option(this.selectedHosts[i]);
        }

        return options;
    }

    public String getMountPoint() {
        return this.mountPoint;
    }

    public String getMountAfterAddText() {
        return (this.mountAfterAdd ? JSFUtil.getMessage("samqfsui.yes") :
                JSFUtil.getMessage("samqfsui.no"));
    }

    public String getMountReadOnlyText() {
        return (this.mountReadOnly ? JSFUtil.getMessage("samqfsui.yes") :
                JSFUtil.getMessage("samqfsui.no"));
    }

    public String getMountAtBootTimeText() {
       return (this.mountAtBootTime ? JSFUtil.getMessage("samqfsui.yes") :
               JSFUtil.getMessage("samqfsui.no"));
    }

    public String getMountInTheBackgroundText() {
        return (this.mountInBackground ? JSFUtil.getMessage("samqfsui.yes") :
                JSFUtil.getMessage("samqfsui.no"));
    }

    // alert bean
    public AlertBean getAlertBean() {
        return this.alertBean;
    }
}

/**
 * Used to supply alert messages to the various wizard steps
 */
class AlertBean {
    private boolean render;
    private String summary;
    private String detail;

    AlertBean(){}

    public boolean isRendered() {
        return this.render;
    }

    public void setRendered(boolean r) {
        this.render = r;
    }

    public String getSummary() {
        return this.summary;
    }

    public void setSummary(String s) {
        this.summary = s;
    }

    public String getDetail() {
        return this.detail;
    }

    public void setDetail(String d) {
        this.detail = d;
    }
}

