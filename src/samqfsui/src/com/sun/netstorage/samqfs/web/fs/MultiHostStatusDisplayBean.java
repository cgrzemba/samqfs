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

// ident	$Id: MultiHostStatusDisplayBean.java,v 1.3 2008/10/22 20:57:04 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.model.fs.MultiHostStatus;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.AsyncServlet;
import com.sun.web.ui.model.Option;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.servlet.http.HttpServletRequest;


/** add clients */
public class MultiHostStatusDisplayBean {
    private String selectedHost;
    private MultiHostStatus status = null;

    private String serverName;
    private long jobId = -1;

    public MultiHostStatusDisplayBean() {

        this.status = initializeStatus(getCurrentJobId());
    }

    /** return the current job */
    private long getCurrentJobId() {
        if (jobId == -1) {
            // if this is the first time the page is loaded, look for the job
            // id in the request object.
            HttpServletRequest request = JSFUtil.getRequest();
            String id =
                request.getParameter(Constants.PageSessionAttributes.JOB_ID);
            if (id != null && !id.trim().equals("")) {
                try {
                    Long longId = Long.parseLong(id);
                    jobId = longId;
                } catch (NumberFormatException nfe) {
                    System.out.println("getCurrentJobI: bad job id " + id);
                }
            }
        }
        
        return jobId;
    }

    /** return the MultiHostStatus object for the current job */
    private MultiHostStatus initializeStatus(long jobId) {
        MultiHostStatus mhs = null;

        try {
            String server = getServerName();
            if (jobId == -1)
                throw new SamFSException("No Job ID provided.");

            if (server != null) {
                SamQFSSystemModel model = SamUtil.getModel(server);
                if (model == null)
                    throw new SamFSException("unable to retrieve system model" +
                        " for '" + server + "'");
                
                // begin testing
                /*
                if (jobId > -1 && jobId < 5) {
                    int index = (int)jobId;
                    mhs = new MultiHostStatus(AsyncServlet.samples[index]);
                    return mhs;
                }
                */
                // end testing

                // if we get this far, all is well.
                mhs = model.getSamQFSSystemJobManager()
                    .getMultiHostStatus(jobId);
            }
        } catch (SamFSException sfe) {
            System.out.println("initializeStatus : unable to initialize status " +
                sfe.getMessage());
        }
        
        return mhs;
    }

    // resource strings
    public String getTitleText() {
        return JSFUtil.getMessage("fs.multihoststatus.addclients.title");
    }
    
    public String getFailedHostLabel() {
        return JSFUtil.getMessage("fs.multihoststatus.addclients.failedlabel");
    }
    
    public String getHostErrorLabel() {
        return JSFUtil.getMessage("fs.multihoststatus.addclients.hosterrorlabel");
    }
    
    // status data
    public int getTotal() {
        return (status != null ? status.getTotalHostCount() : 0);
    }
    
    public int getSucceeded() {
        return (status != null ? status.getSucceededHostCount() : 0);
    }
    
    public int getFailed() {
        return (status != null ? status.getFailedHostCount() : 0);
    }
    
    public int getPending() {
        return (status != null ? status.getPendingHostCount() : 0);
    }
    
    public List<Option> getFailedHostList() {
	List<Option> optionList = new ArrayList<Option>();

	if (status == null) {
	    optionList.add(new Option("--------------------"));
	} else {
	    List<String> l = status.getHostsWithError();
	    if (l != null|| l.size() > 0) {
		Iterator<String> it = l.iterator();
	    
		while (it.hasNext()) {
		    optionList.add(new Option(it.next()));
		}
	    } else {
	    optionList.add(new Option("--------------------"));
	    }
	}

	return optionList;
    }
    
    public String getHostErrorDetails() {
        return "";
    }
    
    public String getSelected() {
        return selectedHost;
    }
    
    public void setSelectedHost(String host) {
        selectedHost = host;
    }

    // hidden fields
    public String getServerName() {
        // initialize servername if this is the first time we are loading it
        // the page
        if (serverName == null) {
            serverName = JSFUtil.getServerName();
        }

        return serverName;
    }

    public long getJobId() {
        return getCurrentJobId();
    }
    
    public void setServerName(String name) {serverName = name;}
    public void setJobId(long id) {jobId = id;}

    /* ruturns a comma delimited 'hostname=error message' string for all the
       hosts that have returned errors. This method attempts 
    */
    public String getHostErrorList() {
        List<String> hostList =
            status != null ? status.getHostsWithError() : null;
        StringBuffer buf = new StringBuffer();
        if (hostList != null) {
            Iterator<String>it = hostList.iterator();
            while (it.hasNext()) {
                String hostName = it.next();

                // hostError is of the format NNNN "ssssss" where NNNN is an
                // error code whose localized text can be looked up from the
                // resource bundle.
                String hostError = status.getHostError(hostName);

                // split error code from key
                int index = hostError.indexOf("\"");
                String errorCode = hostError.substring(0, index);
                String errorString =
                    hostError.substring(index + 1, hostError.lastIndexOf("\""));
                

                // try to retrieve the error message from the resource
                // bundle. If found, return the localized error message, if not 
                // found, the return the unlocalized error message returned by
                // the C-API
                String errorMessage = JSFUtil.getMessage(errorCode.trim());

                if (errorMessage.trim().equals(errorCode.trim())) {
                    errorMessage = errorString.trim();
                }

                // append the host and errorMessage to the message string
                buf.append(hostName)
                    .append("=")
                    .append(errorMessage)
                    .append(",");
            }
        }

        // return the encoded error string
        return buf.toString();
    }     
}
