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

// ident        $Id: AddClientsWizardEventListener.java,v 1.7 2009/03/04 21:54:41 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.sun.netstorage.samqfs.mgmt.fs.Host;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
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

        // reset the wizard alert mesages
        this.wizardBean.getAlertBean().reset();

        int id = event.getEvent();
        if (id == WizardEvent.FINISH) {
            boolean result = finishWizard();
            this.wizardBean.clearWizardValues();
            return result;
        } else if (id == WizardEvent.NEXT) { // hanle the next button
            if (HOST_SELECTION_METHOD.equals(step.getId())) {
                TraceUtil.trace3("validating host selection method ...");
                return processHostSelectionMethod();
            }else if (SELECTION_BY_HOSTNAME.equals(step.getId())) {
                TraceUtil.trace3("validating host names ...");
                return processByHostName();
            } else if (SELECTION_BY_IPADDRESS.equals(step.getId())) {
                TraceUtil.trace3("validating ip addresses ...");
                return processByIpAddress();
            } else if (SELECTION_FROM_FILE.equals(step.getId())) {
                TraceUtil.trace3("loading hosts from file ...");
                return processLoadHostsFromFile();
            } else if (REVIEW_CLIENT_LIST.equals(step.getId())) {
                TraceUtil.trace3("validating client host list ...");
                return processReviewClientList();
            } else if (MOUNT_OPTIONS.equals(step.getId())) {
                TraceUtil.trace3("validating mount options ...");
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

    protected boolean processHostSelectionMethod() {
        AlertBean alertBean = this.wizardBean.getAlertBean();
        alertBean.reset();
        String sm = this.wizardBean.getSelectedMethod();

        if (sm == null) {
            alertBean.setType(3);
            alertBean.setSummary("fs.addclients.error.summary");
            alertBean.setDetail("fs.addclients.selectionmethod.null");
            alertBean.setRendered(true);

            return false;
        }

        return true;
    }

    protected boolean processByHostName() {
        // resolve the host names to ip addresses
        String [] rawList = this.wizardBean.getSelectedHosts();
        AlertBean alertBean = this.wizardBean.getAlertBean();
        alertBean.reset();

        // there must be atleast one host name entered.
        if (rawList == null) {
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.nohosts"));
            alertBean.setRendered(true);
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
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.nohosts"));
            alertBean.setRendered(true);

            this.wizardBean.setSelectedHosts(null);
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
        AlertBean alertBean = this.wizardBean.getAlertBean();
        alertBean.reset();

        // there must be atleast one host selected
        if (rawList == null) {

            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.nohosts"));
            alertBean.setRendered(true);
            return false;
        }

        // verify the ip addresses are correct
        ArrayList<String>list = new ArrayList<String>();
        for (int i = 0; i < rawList.length; i++) {
            if (rawList[i].indexOf("(") != -1) {
                list.add(rawList[i]);
                continue;
            }

            // If the current entry that has not been validated is not
            // a single ip address or an ip address range drop it
            // (Users have already elected to make entries based on ip
            // address and ip ranges
            boolean isValidIpOrRange= isValidIPOrIPRange(rawList[i]);
            if (!isValidIpOrRange) {
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
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.nohosts"));
            alertBean.setRendered(true);

            this.wizardBean.setSelectedHosts(null);
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
        AlertBean alertBean = this.wizardBean.getAlertBean();
        alertBean.reset();

        // a file name must be supplied
        if (fileName == null) {
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.nofsname"));
            alertBean.setRendered(true);

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
                alertBean.setType(3);
                alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
                alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.filenohosts"));
                alertBean.setRendered(true);

                result = false;
            } else {
                result = true;
                String [] temp = new String[validHosts.size()];
                temp = (String [])validHosts.toArray(temp);

                // set the new list of hosts
                this.wizardBean.setSelectedHosts(temp);
            }
        } catch (FileNotFoundException fnfe) {
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.readinfile"));
            alertBean.setDetail(fnfe.getMessage());
            alertBean.setRendered(true);
            result = false;
        } catch (IOException ioe) {
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.readinfile"));
            alertBean.setDetail(ioe.getMessage());
            alertBean.setRendered(true);
            result = false;
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ioe) {
                    TraceUtil.trace1("Error: " + ioe.getMessage());
                }
            }
        } // end finally

        return result;
    }

    protected boolean processReviewClientList() {
        AlertBean alertBean = this.wizardBean.getAlertBean();
        String [] rawList = this.wizardBean.getSelectedHosts();

        if (rawList == null || rawList.length == 0) {
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.error.nohosts"));
            alertBean.setRendered(true);

            this.wizardBean.setSelectedHosts(null);

            return false;
        }

        // new entries could be hosts, ips, or ip ranges.
        return true;
    }

    protected boolean processMountOptions() {
        AlertBean alertBean = this.wizardBean.getAlertBean();
        alertBean.reset();

        boolean valid =
            SamUtil.isValidMountPoint(this.wizardBean.getMountPoint());

        if (!valid) {
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.mountoptions.mountpoint.invalid"));
            alertBean.setRendered(true);
        }

        return valid;
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
    private Host formattedStringToHostName(String s) {
        String hostname = null;
        if (s != null && s.length() > 0 && s.indexOf("(") != -1) {
            hostname = s.substring(0, s.indexOf("("));
        }

        // now retrieve the ip addresses
        String ipFragment = s.substring(s.indexOf("(") + 1, s.length() - 1);
        String [] ipAddress = ipFragment.split(",");

        return new Host(hostname, ipAddress, 0, false);
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
            try {
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
            } catch (NumberFormatException nfe) {
                TraceUtil.trace1("Unable to translate range: "
                                   + nfe.getMessage());
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
        AlertBean alertBean = this.wizardBean.getAlertBean();
        alertBean.reset();

        try {
            String serverName = JSFUtil.getServerName();
            String fsName = (String)JSFUtil.getAttribute(FS_NAME);

            SamQFSSystemModel model = SamUtil.getModel(serverName);
            String [] displayHost = wizardBean.getSelectedHosts();

            // the list contains hosts string(ip address) strings. Convert it
            // back to ip addresses
            Host [] hostList = new Host[displayHost.length];
            for (int i = 0; i < displayHost.length; i++) {
                hostList[i] = formattedStringToHostName(displayHost[i]);
            }

            String options = createMountOptionsString();

            // add the client list
            SamQFSSystemSharedFSManager fsManager = SamQFSFactory
                .getSamQFSAppModel().getSamQFSSystemSharedFSManager();

            long jobId = fsManager.addClients(serverName,
                                              fsName,
                                              hostList,
                                              options);


            // if -1 is return, an error occured during job
            // submission. Inform the user
            if (jobId == -1)
                throw new SamFSException("An error occured while submitting job");

            JSFUtil.setAttribute(AddClientsBean.JOB_ID_KEY, jobId);

            // if we get this far, we were successful
            alertBean.setType(1);
            alertBean.setSummary(JSFUtil.getMessage("success.summary"));
            alertBean.setDetail(JSFUtil.getMessage("fs.addclients.success.detail"));
            alertBean.setRendered(true);

	    // since the job was submitted successfully, display the HMS link
	    this.wizardBean.setDisplayMHSLink("true");
        } catch (SamFSException sfe) {
            // do nothing for now
            alertBean.setType(3);
            alertBean.setSummary(JSFUtil.getMessage("fs.addclients.error.summary"));
            alertBean.setDetail(sfe.getMessage());
            alertBean.setRendered(true);

	    this.wizardBean.setDisplayMHSLink("false");
        } finally {
            // clear the wizard bean so that its ready for the next launch
            this.wizardBean.clearWizardValues();
        }

        // always return true so we can display the results page
        return true;
    }

    /** validate that the given string is a valid ip address or a valid ip
     * address range of the format xx.xx.xx.xx-xx
     */
    private boolean isValidIPOrIPRange(String s) {
        // if the string is null or empty, return false
        if (s == null || s.length()  == 0)
            return false;

        try {
            String [] tokens = s.split("\\."); // dot-delimited
            // both ip addresses and ip ranges should have a have 4 dot
            // delimited tokens
            if (tokens == null || tokens.length != 4)
                return false;

            // the first three tokens should be numbers
            boolean valid = true;
            for (int i = 0; valid && i < 3; i++) {
                // will through a SamFSException if token[i] is not a number
                Integer num = ConversionUtil.strToInteger(tokens[i]);
            }

            // the last token should be either a number or two numbers
            // separated by a dash. i.e nn or nn-nn
            if (tokens[3].indexOf("-") != -1) { // we have a range
                String [] range = tokens[3].split("-"); // dash-delimited

                // a range should only have two tokens
                if (range == null || range.length != 2)
                    return false;

                // check that the two tokens are numbers. NOTE, a
                // SamFSException exception will be thrown if the token is not
                // a number.
                Integer num = ConversionUtil.strToInteger(range[0]);
                num = ConversionUtil.strToInteger(range[1]);

                // if we get this far, we must have a valid ip address range
                return true;
            } else {
                // we must have a single ip, check if the last token is a valid
                // number
                Integer num = ConversionUtil.strToInteger(tokens[3]);

                // if we get this far, we must have a valid single ip address
                return true;
            }

        } catch (SamFSException sfe) {
            TraceUtil.trace1("Error in 'isValidIPOrIPRange(...)' " +
                               sfe.getMessage());
            return false;
        }

        // we really should have return by now.
    }


    /**
     * create an options key=value pair string.
     *
     * options is a key value string supporting the following options:
     * mount_point="/fully/qualified/path"
     * mount_fs= yes | no
     * mount_at_boot = yes | no
     * bg_mount = yes | no
     * read_only = yes | no
     * potential_mds = yes | no
     *
     */
    private String createMountOptionsString() {
        StringBuffer buf = new StringBuffer();

        buf.append("mount_point=")
            .append(this.wizardBean.getMountPoint())
            .append(",mount_fs=")
            .append(this.wizardBean.getMountAfterAdd() ? "yes" : "no")
            .append(",potential_mds=")
            .append(this.wizardBean.getMakePMDS() ? "yes" : "no");

        // append the check box group
        boolean [] mo = this.wizardBean.getMountOptionSettings();
        buf.append(",read_only=")
            .append(mo[0] ? "yes" : "no") // mount read only
            .append(",mount_at_boot=")
            .append(mo[1] ? "yes" : "no") // mount at boot time
            .append(",bg_mount=")
            .append(mo[2] ? "yes" : "no"); // mount in the background

        return buf.toString();
    }
}
