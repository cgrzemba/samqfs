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

// ident        $Id: AddClientsBean.java,v 1.7 2009/03/04 21:54:41 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.fs.FSUtil;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
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
    protected String mountPoint = null;
    private boolean mountAfterAdd = false;
    private boolean  mountReadOnly = false;
    private boolean  mountAtBootTime = false;
    private boolean mountInBackground = false;
    private boolean pmds = false;

    // file chooser
    private File currentDirectory = null;
    private File selectedFile = null;

    private String fileName = null;

    // alert messages
    private AlertBean alertBean = new AlertBean();

    // used by the results page to determine if the HMS link should be
    // display
    private String displayMHSLink = "false";

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
        this.selectedHosts = null;
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

    /* this method is to be called by
     * AddClientsWizardEventListener.createMountOptionsString() only. The order
     * of the elements in the returned should NOT be changed without changing
     * the implemention of
     * AddClientsWizardEventListener.createMountOptionsString()
     */
    public boolean [] getMountOptionSettings() {
        return new boolean [] {mountReadOnly, mountAtBootTime, mountInBackground};
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

    public Boolean getMakePMDS() {
        return this.pmds;
    }

    public void setMakePMDS(Boolean b) {
        this.pmds = b;
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
        if (this.mountPoint == null || this.mountPoint.length() == 0) {
            this.mountPoint = getMountPointOnMDS();
        }
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

    public String getPMDSText() {
        return (this.pmds ? JSFUtil.getMessage("samqfsui.yes") :
                JSFUtil.getMessage("samqfsui.no"));
    }

    // alert bean
    public AlertBean getAlertBean() {
        return this.alertBean;
    }

    // alert messages
    public String getAlertRendered() {
        return this.alertBean.getRendered();
    }

    public String getAlertSummary() {
        return this.alertBean.getSummary();
    }

    public String getAlertDetail() {
        return this.alertBean.getDetail();
    }

    public String getAlertType() {
        return this.alertBean.getType();
    }

    public String getAlertString() {
        return this.alertBean.toString();
    }

    // The following methods are required by the multi-host
    // status window

    // determine if the link to the MultiHostStatus Display page
    // should be displayed
    public String getDisplayMHSLink() {
        return this.displayMHSLink;
    }

    public void setDisplayMHSLink(String b) {
        this.displayMHSLink = b;
    }

    public long getJobId() {
        Long jobId = (Long)JSFUtil.getAttribute(JOB_ID_KEY);

        return jobId != null ? (long)jobId : -1L;
    }

    public String getServerName() {
        return JSFUtil.getServerName();
    }

    private String getMountPointOnMDS() {
        try {
            return
                SamUtil.getModel(JSFUtil.getServerName()).
                    getSamQFSSystemFSManager().
                    getFileSystem(FSUtil.getFSName()).getMountPoint();
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to retrieve mount point from MDS!", samEx);
        }
        return "";
    }
}

/**
 * Used to supply alert messages to the various wizard steps
 */
class AlertBean {
    private boolean render;
    private String summary;
    private String detail;
    private int type = 0; // alert types are 0, 1, 2, or 3 for "information",
    // "success", "warning", "error". Refactor this class out
    // and define these as an enum.

    AlertBean(){}

    public String getRendered() {
        return this.render ? "true" : "false";
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

    public String getType() {
        switch (this.type) {
        case 0:
            return "information";
        case 1:
            return "success";
        case 2:
            return "warning";
        case 3:
            return "error";
        }

        // if we get this far, return the default which is information
        return "information";
    }

    public void setType(int t) {
        this.type = t;
    }

    // Re-initialize the alert bean to its infant state. This is more efficient
    // that creating a new object.
    public void reset() {
        this.render = false;
        this.summary = null;
        this.detail = null;
        this.type = 0;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();
        buf.append("render = ")
            .append(getRendered())
            .append("; type = ")
            .append(getType())
            .append("; summary = ")
            .append(getSummary())
            .append("; detail = ")
            .append(getDetail());

        return buf.toString();
    }
}

