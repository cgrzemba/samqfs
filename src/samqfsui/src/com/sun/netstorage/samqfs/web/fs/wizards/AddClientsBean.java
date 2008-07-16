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

// ident        $Id: AddClientsBean.java,v 1.1 2008/07/16 23:47:28 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.model.Option;
import java.util.ArrayList;
import java.util.List;
import java.io.File;
import javax.faces.event.ActionEvent;

public class AddClientsBean {
    // host selection method
    public static final String METHOD_NAME = "hostname";
    public static final String METHOD_IP = "ipaddress";
    public static final String METHOD_FILE = "file";
    
    // mount options
    public static final String READ_ONLY = "ro";
    public static final String BOOT_TIME = "boot";
    public static final String BACKGROUND = "bg";
    
    // default to specify hosts by host name
    private String selectedMethod = null;
    private List<Option> selectedHosts = null;

    // file chooser
    private File currentDirectory = null;
    private File selectedFile = null;

    public AddClientsBean(){
        // this.selectedMethod = METHOD_NAME;
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

    public List<Option> getHostList() {
        if (this.selectedHosts == null) {
            this.selectedHosts = new ArrayList<Option>();
            this.selectedHosts.add(new Option("blank", " "));
        }
        
        return this.selectedHosts;
    }
    
    public List<String> getSelectedHosts() {
        return new ArrayList<String>();
    }
    
    public void setSelectedHosts(List<String> hosts) {
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
    
    public String [] getSelectedMountOptions() {
        return new String[0];
    }
    
    public void setSelectedMountOptions(String [] options) {
        
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
}

