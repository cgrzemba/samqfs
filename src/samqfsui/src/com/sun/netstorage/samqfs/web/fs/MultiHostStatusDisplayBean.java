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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: MultiHostStatusDisplayBean.java,v 1.5 2008/12/16 00:12:10 am143972 Exp $

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
        String key = null;
        
        if (status != null) {
            MultiHostStatus.TYPE type = status.getJobType();
            boolean succeeded =
                status.getJobStatus() == MultiHostStatus.STATUS.SUCCESS;

            if (type != null) {
                switch(type) {
                case DSP_MOUNT:
                    key = succeeded ? "fs.multihoststatus.mount.allsuccess" :
                        "fs.multihoststatus.mount.title";
                    break;
                case DSP_UMOUNT:
                    key = succeeded ? "fs.multihoststatus.umount.allsuccess" :
                        "fs.multihoststatus.umount.title";
                    break;
                case DSP_CHANGE_MOUNT_OPTIONS:
                    key = succeeded ?
                        "fs.multihoststatus.changeoptions.allsuccess" :
                        "fs.multihoststatus.changeoptions.title";
                    break;
                case DSP_ADD_CLIENTS:
                    key = succeeded ?
                        "fs.multihoststatus.addclients.allsuccess" :
                        "fs.multihoststatus.addclients.title";
                    break;
                case DSP_GROW_FS:
                    key = succeeded ?
                        "fs.multihoststatus.growfs.allsuccess" :
                        "fs.multihoststatus.growfs.title";
                    break;
                case DSP_REMOVE_CLIENTS:
                    key = succeeded ?
                        "fs.multihoststatus.removeclients.allsuccess" :
                        "fs.multihoststatus.removeclients.title";
                    break;
                case DSP_SET_ADV_NET_CONFIG:
                    key = succeeded ?
                        "fs.multihoststatus.netconfig.allsuccess" :
                        "fs.multihoststatus.netconfig.title";
                    break;
                }
            }
        } // end if (mhs)
        
        return JSFUtil.getMessage(key);
    }
    
    public String getFailedHostLabel() {
        String key = null;
        
        if (status != null) {
            MultiHostStatus.TYPE type = status.getJobType();
            if (type != null) {
                switch(type) {
                case DSP_MOUNT:
                    key = "fs.multihoststatus.mount.failedlabel";
                    break;
                case DSP_UMOUNT:
                    key = "fs.multihoststatus.umount.failedlabel";
                    break;
                case DSP_CHANGE_MOUNT_OPTIONS:
                    key = "fs.multihoststatus.changeoptions.failedlabel";
                    break;
                case DSP_ADD_CLIENTS:
                    key = "fs.multihoststatus.addclients.failedlabel";
                    break;
                case DSP_GROW_FS:
                    key = "fs.multihoststatus.growfs.failedlabel";
                    break;
                case DSP_REMOVE_CLIENTS:
                    key = "fs.multihoststatus.removeclients.failedlabel";
                    break;
                case DSP_SET_ADV_NET_CONFIG:
                    key = "fs.multihoststatus.netconfig.failedlabel";
                    break;
                }
            }
        } // end if (mhs)
        
        return JSFUtil.getMessage(key);
    }
    
    public String getHostErrorLabel() {
        return JSFUtil.getMessage("fs.multihoststatus.hosterrordetail");
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
