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

// ident	$Id: AsyncServlet.java,v 1.2 2008/12/16 00:12:25 am143972 Exp $


package com.sun.netstorage.samqfs.web.util;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemJobManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.MultiHostStatus;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

public class AsyncServlet extends HttpServlet {
    // list of actions supported by this servlet.
    public static final String MULTI_HOST_STATUS = "multi_host_status";
    
    public static final String ACTION = "requested_action";
    public static final String SERVER_NAME =
        Constants.PageSessionAttributes.SAMFS_SERVER_NAME;
    public static final String JOB_ID = Constants.PageSessionAttributes.JOB_ID;
    
    @Override
    public void doGet(HttpServletRequest req, HttpServletResponse res)
        throws ServletException, IOException {
        processRequest(req, res);
    }
    
    @Override
    public void doPost(HttpServletRequest req, HttpServletResponse res)
        throws ServletException, IOException {
        processRequest(req, res);
    }
    
    public void processRequest(HttpServletRequest req, HttpServletResponse res)
        throws ServletException, IOException {
        String action = req.getParameter(ACTION);
        
        if (MULTI_HOST_STATUS.equals(action)) {
            handleMultiHostStatusRequest(req, res);
        }
    }
    
    private void handleMultiHostStatusRequest(HttpServletRequest req,
                                              HttpServletResponse res)
        throws ServletException, IOException {
        
        PrintWriter out = res.getWriter();
        String temp = req.getParameter(JOB_ID);
        long jobId = (temp != null) ? Long.parseLong(temp) : -1;
        String serverName = req.getParameter(SERVER_NAME);
        
        try {
            SamQFSSystemModel model = getSamQFSSystemModel(serverName);
            SamQFSSystemJobManager jobManager =
                model.getSamQFSSystemJobManager();
            //MultiHostStatus mhs = jobManager.getMultiHostStatus(jobId);

	    int index = (int)jobId;
            MultiHostStatus mhs = new MultiHostStatus(samples[index]);
            // construct the xml stream required.
            StringBuffer buf = new StringBuffer();
            buf.append("<?xml version=\"1.0\" ?>")
                .append("<mhs>")
                .append("<status>")
                .append(mhs.getJobStatus())
                .append("</status>")
                .append("<total>")
                .append(mhs.getTotalHostCount())
                .append("</total>")
                .append("<succeeded>")
                .append(mhs.getSucceededHostCount())
                .append("</succeeded>")
                .append("<failed>")
                .append(mhs.getFailedHostCount())
                .append("</failed>")
                .append("<pending>")
                .append(mhs.getPendingHostCount())
                .append("</pending>");
            
            // append the list of hosts that have failed together with their
            // corresponding error messages
            List <String> hostNames = mhs.getHostsWithError();
            if (hostNames != null && hostNames.size() > 0){
                Iterator <String>it = hostNames.iterator();
                
                while (it.hasNext()) {
                    String host = it.next();
                    String error = mhs.getHostError(host);
                    
                    buf.append("<host>")
                        .append("<hostname>")
                        .append(host)
                        .append("</hostname>")
                        .append("<hosterror>")
                        .append(error)
                        .append("</hosterror>")
                        .append("</host>");
                }
            }
            
            // close the document.
            buf.append("</mhs>");

            // finally print the string out
            out.println(buf.toString());
        } catch (SamFSException sfe) {
            System.out.println("SamFSException retrieving MultiHostStatus : " + sfe.getMessage());
        } finally {
            out.close();
        }
    }
    
    private SamQFSSystemModel getSamQFSSystemModel(String serverName)
        throws SamFSException {
        ServletContext sc = getServletContext();
        Map modelMap = (Map)sc.getAttribute(Constants.sc.HOST_MODEL_MAP);
        
        SamQFSSystemModel model =  null;
        // 1. see if we can get the model by the given host name
        if (modelMap != null) {
            model = (SamQFSSystemModel)modelMap.get(serverName);
            if (model != null) {
                return model;
            }
        }
        
        // 2. construct an InetAddress
        try {
            InetAddress address = InetAddress.getByName(serverName);
            if (address != null) {
                // 2.1 search by host name
                model = (SamQFSSystemModel)modelMap.get(address.getHostName());
                if (model == null) {
                    // 2.2 search by host name
                    model = (SamQFSSystemModel)
                        modelMap.get(address.getHostAddress());
                }
            }
        } catch (UnknownHostException uhe) {
            System.out.println("Error retrieving host inet address : " + uhe.getMessage());
        }
        
        if (model == null)
            throw new SamFSException("Unable to retrive the model");
        
        return model;
    }
    
    // some sample mult-host stati
public static final String [] samples = new String [] {
"type=SAMADISPATCHJOB,operation=DSP_MOUNT,activity_id=524295,starttime=1215444823,host_count=4,hosts_responding=4,hosts_pending=0,fsname = fssh2,error_hosts=nws-bur-24-91 ,nws-bur-24-91=30801 \"nws-bur-24-91: RPC: Program not registered\" ,ok_hosts=nws-bur-24-66 nws-bur-24-68 nws-bur-24-69 ,status=partial_failure,error=32305,error_msg=\"Message 32305\"",
"type=SAMADISPATCHJOB,operation=DSP_UMOUNT,activity_id=524293,starttime=1215444671,host_count=3,hosts_responding=3,hosts_pending=0,fsname = fssh2,error_hosts=nws-bur-24-66 nws-bur-24-68 nws-bur-24-69 ,nws-bur-24-66=30012 \"umount: warning: fssh2 not in mnttab \" ,nws-bur-24-68=30012 \"umount: warning: fssh2 not in mnttab \" ,nws-bur-24-69=30012 \"umount: warning: fssh2 not in mnttab \" ,status=failure,error=32306,error_msg=\"Message 32306\"",
"type=SAMADISPATCHJOB,operation=DSP_UMOUNT,activity_id=524291,starttime=1215444232,host_count=3,hosts_responding=3,hosts_pending=0,fsname = fssh2,ok_hosts=nws-bur-24-66 nws-bur-24-68 nws-bur-24-69 ,status=success",
"type=SAMADISPATCHJOB,operation=DSP_CHANGE_MOUNT_OPTS,activity_id=524290,starttime=1215437477,host_count=3,hosts_responding=3,hosts_pending=0,fsname = fssh2,ok_hosts=nws-bur-24-66 nws-bur-24-68 nws-bur-24-69 ,status=success",
"type=SAMADISPATCHJOB,operation=DSP_CHANGE_MOUNT_OPTS,activity_id=524289,starttime=1215437427,host_count=3,hosts_responding=3,hosts_pending=0,fsname = fssh2,ok_hosts=nws-bur-24-66 nws-bur-24-68 nws-bur-24-69 ,status=success"
};

}
