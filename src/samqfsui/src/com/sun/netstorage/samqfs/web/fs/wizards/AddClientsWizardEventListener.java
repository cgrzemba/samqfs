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

// ident        $Id: AddClientsWizardEventListener.java,v 1.1 2008/08/27 22:17:28 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.component.Wizard;
import com.sun.web.ui.component.WizardStep;
import com.sun.web.ui.event.WizardEvent;
import com.sun.web.ui.event.WizardEventListener;
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.faces.context.FacesContext;
import javax.faces.event.AbortProcessingException;

public class AddClientsWizardEventListener implements WizardEventListener {
    // step ids
    public static final String HOST_SELECTION_METHOD = "selectionMethodPage";
    public static final String SELECTION_BY_HOSTNAME = "byhostnamePage";
    public static final String SELECTION_BY_IPADDRESS = "byipaddressPage";
    public static final String SELECTION_FROM_FILE = "fromFilePage";
    public static final String REVIEW_CLIENT_LIST = "clientListPage";
    public static final String MOUNT_OPTIONS = "mountOptionsPage";
    public static final String SUMMARY = "reviewPage";
    public static final String RESULTS = "resultsPage";

    // constants
    public static final String FS_NAME =
        Constants.PageSessionAttributes.FS_NAME;

    private AddClientsBean  wizardBean = null;
    
    public AddClientsWizardEventListener() {
    }

    public AddClientsWizardEventListener(AddClientsBean bean) {
        this.wizardBean = bean;
    }

    /**
     * handler for the various buttons on the wizard : preview, next, cancel,
     * and finish.
     */
    public boolean handleEvent(WizardEvent event)
        throws AbortProcessingException {
        Wizard wizard = event.getWizard();
        WizardStep step = event.getStep();
     
        int id = event.getEvent();
        if (id == WizardEvent.FINISH) {
            boolean result = finishWizard();
            this.wizardBean.clearWizardValues();
            return result;
        } else if (id == WizardEvent.NEXT) { // hanle the next button
            if (SELECTION_BY_HOSTNAME.equals(step.getId())) {
                System.out.println("validing host names ...");
                return processByHostName();
            } else if (SELECTION_BY_IPADDRESS.equals(step.getId())) {
                System.out.println("validating ip addresses ...");
                return processByIpAddress();
            } else if (SELECTION_FROM_FILE.equals(step.getId())) {
                System.out.println("loading hosts from file ...");
                return processLoadHostsFromFile();
            } else if (REVIEW_CLIENT_LIST.equals(step.getId())) {
                System.out.println("validating client host list ...");
                return processReviewClientList();
            } else if (MOUNT_OPTIONS.equals(step.getId())) {
                System.out.println("validating mount otpions ...");
                return processMountOptions();
            } // end process next
        } else if (id == WizardEvent.CANCEL) {
            this.wizardBean.clearWizardValues();
        }
        
        return true;
    }
    
    public boolean isTransient() {
        return false;
    }

    public void setTransient(boolean arg0) {
        // do nothing
    }

    public void restoreState(FacesContext arg0, Object arg1) {
        // do nothing
    }

    public Object saveState(FacesContext arg0) {
        return null;
    }

    protected boolean processByHostName() {
        // resolve the host names to ip addresses
        String [] rawList = this.wizardBean.getSelectedHosts();

        // there must be atleast one host name entered.
        if (rawList == null) {
            return false;
        }

        // verify all the host names are valid and return the correct ones.
        ArrayList<String>list = new ArrayList<String>();
        for (int i = 0; i < rawList.length; i++) {
            // if the name has already been validated insert it
            if (rawList[i].indexOf("(") != -1) {
                list.add(rawList[i]);
                continue;
            }

            // validate newly inserted hostnames
            InetAddress [] address = resolveHost(rawList[i]);
            if (address != null) {
                list.add(inetAddressToString(address));
            }
        }
        
        if (list.size() == 0) {
            return false;
        }

        // reset the selected hosts
        String [] result = new String[list.size()];
        result = (String [])list.toArray(result);

        this.wizardBean.setSelectedHosts(result);
        return true;
    }

    protected boolean processByIpAddress() {
        String [] rawList = this.wizardBean.getSelectedHosts();

        // there must be atleast one host selected
        if (rawList == null) return false;

        // verify the ip addresses are correct
        ArrayList<String>list = new ArrayList<String>();
        for (int i = 0; i < rawList.length; i++) {
            if (rawList[i].indexOf("(") != -1) {
                list.add(rawList[i]);
                continue;
            }

            // determine if this a range or a single ip
            List<String> hosts = resolveIPAddressRange(rawList[i]);
            Iterator<String> it = hosts.iterator();
            while (it.hasNext()) {
                String host = it.next();
                InetAddress [] address = resolveHost(host);
                if (address != null) {
                    list.add(inetAddressToString(address));
                }
            }
        }

        // there must be atleast one valid host selected
        if (list.size() == 0) {
            return false;
        }

        // reset the selected hosts
        String [] result = new String[list.size()];
        result = (String [])list.toArray(result);

        this.wizardBean.setSelectedHosts(result);
        return true;
    }

    protected boolean processLoadHostsFromFile() {
        String fileName = this.wizardBean.getFileName();

        // a file name must be supplied
        if (fileName == null) {
            return false;
        }

        boolean result = false;
        List<String> validHosts = new ArrayList<String>();
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(fileName));
            String aLine = null;

            while ((aLine = reader.readLine()) != null) {
                // any line can be: 1.) a single host or 2.) a list of comma
                // separated hosts 
                if (aLine.indexOf(",") == -1) { // a single host found
                    InetAddress [] address = resolveHost(aLine.trim());
                    if (address != null) {
                        validHosts.add(inetAddressToString(address));
                    }
                } else { // a comma separated list of hosts
                    String [] hosts = aLine.split(",");
                    for (int i = 0; i < hosts.length; i++) {
                        InetAddress [] address = resolveHost(hosts[i]);
                        if (address != null) {
                            validHosts.add(inetAddressToString(address));
                        }
                    }
                }
            } // end while readLine
            
            // make sure atleast one valid host was found
            if (validHosts.size() == 0) {
                result = false;
            } else {
                result = true;
                String [] temp = new String[validHosts.size()];
                temp = (String [])validHosts.toArray(temp);
                
                // set the new list of hosts
                this.wizardBean.setSelectedHosts(temp);
            }
        } catch (FileNotFoundException fnfe) {
            System.out.println("Error: fnfe : " + fnfe.getMessage());
            result = false;
        } catch (IOException ioe) {
            System.out.println("Error: ioe : " + ioe.getMessage());
            result = false;
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ioe) {
                    System.out.println("Error: " + ioe.getMessage());
                }
            }
        } // end finally

        return result;
    }

    protected boolean processReviewClientList() {
        String [] rawList = this.wizardBean.getSelectedHosts();
        
        List<String> list = new ArrayList<String>();
        
        // new entries could be hosts, ips, or ip ranges.
        return true;
    }

    protected boolean processMountOptions() {
        return true;
    }

    /**
     * convert an array of InetAddress to a string of the format :
     * hostname(ip1;ip2) to be displayed to the user
     */
    private String inetAddressToString(InetAddress [] address) {
        String result = null;
        
        if (address != null && address.length > 0 && address[0] != null) {
            result = new StringBuffer(address[0].getHostName())
                .append("(").append(address[0].getHostAddress())
                .append(")")
                .toString();
        }

        return result;
    }

    /**
     * convert a string of the format hostname(ip1;ip2) to an array of an array 
     * of hostnames to be used by the finishWizard function
     */
    private String formattedStringToHostName(String s) {
        String hostname = null;
        if (s != null && s.length() > 0 && s.indexOf("(") != -1) {
            hostname = s.substring(0, s.indexOf("("));
        }

        return hostname;
    }

    /**
     * convert an ip range to a List of the individual IP addresses that make
     * up the range
     */
    private List<String> resolveIPAddressRange(String range) {
        List<String> ipAddress = new ArrayList<String>();
        String [] temp = range.split(".");

        if (range != null && range.indexOf("-") == -1) {
            ipAddress.add(range);
        } else {
            String prefix = range.substring(0, range.lastIndexOf(".") + 1);
            String suffix = range.substring(range.lastIndexOf(".") + 1,
                                            range.length());

            String [] suffixRange = suffix.split("-");
            int start = Integer.parseInt(suffixRange[0]);
            int end = Integer.parseInt(suffixRange[1]);
            
            for (int i = start; i <= end; i++) {
                String ip = prefix + i;
                ipAddress.add(ip);
            }
        }

        return ipAddress;
    }

    /**
     * verify that the host exists and reachable, then return its InetAddress
     * 
     * @param - a string representation the name of the host or the string
     * representaion of the its ip address.
     */
    private InetAddress [] resolveHost(String host) {
        InetAddress [] address = null;

        try {
            address = InetAddress.getAllByName(host);
        } catch (UnknownHostException uhe) {
            // do nothing
        }

        return address;
    }

    /** handler for the finish button */
    protected boolean finishWizard() {
        boolean result = false;
        try {
            String serverName = JSFUtil.getServerName();
            String fsName = (String)JSFUtil.getAttribute(FS_NAME);

            SamQFSSystemModel model = SamUtil.getModel(serverName);
            String [] displayHost = wizardBean.getSelectedHosts();

            // the list contains hosts string(ip address) strings. Convert it
            // back to ip addresses
            String [] hostList = new String[displayHost.length];

            for (int i = 0; i < displayHost.length; i++) {
                hostList[i] = formattedStringToHostName(displayHost[i]);
            }
            
            // add the client list
            SamQFSSystemSharedFSManager fsManager = SamQFSFactory
                .getSamQFSAppModel().getSamQFSSystemSharedFSManager();

            long jobId = fsManager.addClients(serverName, fsName, hostList);
            JSFUtil.setAttribute(AddClientsBean.JOB_ID_KEY, jobId);
        } catch (SamFSException sfe) {
            // do nothing for now
        }
        return result;
    }
}
