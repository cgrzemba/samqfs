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

// ident	$Id: SamQFSAppModelImpl.java,v 1.32 2008/10/02 03:00:26 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.iplanet.jato.model.DefaultModel;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;

public class SamQFSAppModelImpl extends DefaultModel
    implements SamQFSAppModel {

    private SamQFSSystemSharedFSManager sharedManager = null;

    public SamQFSAppModelImpl() throws SamFSException {
    }

    public SamQFSSystemModel[] getAllSamQFSSystemModels() {
        HashMap modelMap = getHostModelMap();

        return (SamQFSSystemModelImpl[]) modelMap.values()
                    .toArray(new SamQFSSystemModelImpl[0]);
    }

    public SamQFSSystemModel getSamQFSSystemModel(String hostname)
        throws SamFSException {

        if (hostname == null) {
            TraceUtil.trace1("Failed to get sysModel. Hostname is null!");
            throw new SamFSException(null, -2001);
        }

        String inetHostName = null;
        try {
            inetHostName = getInetHostName(hostname);
        } catch (UnknownHostException e) {
            throw new SamFSException("logic.unknownHost");
        }

        TraceUtil.trace2("getSamQFSSystemModel: hostname: " + hostname);
        TraceUtil.trace2("getSamQFSSystemModel: inetHostName: " + inetHostName);

        HashMap map = getHostModelMap();
        SamQFSSystemModel model =
            (SamQFSSystemModel) map.get(
                inetHostName == null ? hostname : inetHostName);

        // If the model is null look for a model that has a serverHostname
        // that matches the input. This can be important for shared file sytems
        // that may need to find the model based on the results.
        if (model == null) {
            SamQFSSystemModel [] list = getAllSamQFSSystemModels();
            if (list != null) {
                String storedName;
                for (int i = 0; i < list.length; i++) {
                    storedName = list[i].getServerHostname();
                    if (storedName != null)
                        if (storedName.compareToIgnoreCase(
                            inetHostName == null ? hostname : inetHostName)
                            == 0) {
                            model = list[i];
                            break;
                        }
                }
            }
        }
        if (model == null)
            throw new SamFSException(null, -2001);

        return model;
    }

    /**
     * This method returns the real host name retrieve by InetAddress
     * if the input host name exists in the network.
     * Otherwise return null.
     */
    public String getInetHostName(String hostName) throws UnknownHostException {

        String inetHostName = "";
        String hostip = "";

        if (!SamQFSUtil.isValidString(hostName)) {
            throw new UnknownHostException("logic.unknownHost");
        }

        InetAddress addr = InetAddress.getByName(hostName);
        inetHostName = addr.getHostName();
        byte[] ipAddr = addr.getAddress();
        for (int i = 0; i < ipAddr.length; i++) {
            if (i > 0) hostip += ".";
            hostip += ipAddr[i]&0xFF;
        }

        // check hostname
        HashMap map = getHostModelMap();
        Object o = map.get(inetHostName);
        if (o != null) {
            return inetHostName;
        }

        // check hostip
        o = map.get(hostip);
        if (o != null) {
            return inetHostName;
        }

        // If both IP and hostname is not found in map, host doesn't exist
        return null;
    }

    public void addHost(String host, boolean writeToFile)
        throws SamFSException {
        addNewHost(host, writeToFile);
    }

    protected synchronized void addNewHost(String host, boolean writeToFile)
        throws SamFSException {
        // if already exists, then reconnect.
        String inetHostName = null;

        try {
            inetHostName = getInetHostName(host);
        } catch (UnknownHostException e) {
            throw new SamFSException("logic.unknownHost");
        }

        TraceUtil.trace2("host: "+ host + " inetHostName: " + inetHostName);

        HashMap map = getHostModelMap();
        if (inetHostName != null) {
            if (writeToFile) {
                throw new SamFSException("logic.existingHost");
            }
        } else {
            map.put(host, new SamQFSSystemModelImpl(host));
            if (writeToFile) {
                rewriteHostFile(null, host);
            }
        }
        saveToHostModelMap(map);
    }

    public void removeHost(String hostname)
        throws SamFSException {
        removeExistingHost(hostname);
    }

    public synchronized void removeExistingHost(String hostname)
        throws SamFSException {
        HashMap map = getHostModelMap();
        map.remove(hostname);
        rewriteHostFile(hostname, null);
    }

    private synchronized void rewriteHostFile(
        String hostToRemove, String hostToAdd) {

        ArrayList hostnames = new ArrayList();
        BufferedReader in = null;
        PrintWriter out = null;

        try {
            String path = SamUtil.getCurrentRequest().getSession()
                .getServletContext().getRealPath(hostFileLocation);
            in = new BufferedReader(new FileReader(path));
            if (in != null) {
                String host;
                while ((host = in.readLine()) != null) {
                    hostnames.add(host);
                }
                in.close();
            }

            out = new PrintWriter(new FileWriter(path, false));
            if (out != null) {
                for (int i = 0; i < hostnames.size(); i++) {
                    String hostEntry = (String) hostnames.get(i);
                    if (hostToRemove == null) {
                        out.println(hostEntry);
                    } else {
                        continue;
                    }
                }

                if (hostToAdd != null) {
                    out.println(hostToAdd);
                }
                out.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void updateDownServers() {
        TraceUtil.trace2("Entering updateDownServers!!!");

        HashMap map = getHostModelMap();
        SamQFSSystemModel[] models = getAllSamQFSSystemModels();
        if ((models != null) && (models.length > 0)) {
            for (int i = 0; i < models.length; i++) {
                // reconnect to servers
                try {
                    models[i].reconnect();
                } catch (SamFSException samEx) {
                    samEx.printStackTrace();
                    TraceUtil.trace1(
                        "Exception caught while reconnecting to server " +
                        models[i].getHostname(), samEx);
                }

                if (models[i].isDown()) {
                    TraceUtil.trace2(
                        "Model is down! " + models[i].getHostname());

                    String hostname = models[i].getHostname();
                    SamQFSUtil.doPrint(hostname + " was down!");
                    map.remove(hostname);
                    map.put(hostname, new SamQFSSystemModelImpl(hostname));
                }
            }
        }

        // update map in session
        saveToHostModelMap(map);
    }

    public String toString() {
        HashMap map = getHostModelMap();
        StringBuffer buf = new StringBuffer();
        buf.append("No. of hosts = " + map.size() + ".\n");

        if (map.size() > 0) {
            SamQFSSystemModelImpl[] systems =
                (SamQFSSystemModelImpl[]) map.values()
                                      .toArray(new SamQFSSystemModelImpl[0]);
            for (int i = 0; i < map.size(); i++)
                buf.append(systems[i].toString());
        }

        return buf.toString();
    }

    public SamQFSSystemSharedFSManager getSamQFSSystemSharedFSManager() {
        if (sharedManager == null) {
            sharedManager = new SamQFSSystemSharedFSManagerImpl(this);
        }

        return sharedManager;
    }

    public void cleanup() {
        TraceUtil.trace2("Clean up connections appModel!");

        SamQFSSystemModel[] models = getAllSamQFSSystemModels();
        if (models != null) {
            for (int i = 0; i < models.length; i++) {
                if (models[i] != null)
                    ((SamQFSSystemModelImpl)models[i]).terminateConn();
            }
        } // end if

        if (sharedManager != null) {
            sharedManager.freeResources();
            sharedManager = null;
        }
    }

    /**
     * Check if host is already managed by the GUI
     * @param hostName - Server of which we want to check if it is being
     *  managed by the GUI
     * @return true/false
     */
    public boolean isHostBeingManaged(String hostName) {
        HashMap map = getHostModelMap();
        return map.get(hostName) != null;
    }


    private HashMap getHostModelMap() {
        HttpServletRequest request = SamUtil.getCurrentRequest();
        ServletContext sc = request.getSession().getServletContext();
        HashMap result = (HashMap)sc.getAttribute(Constants.sc.HOST_MODEL_MAP);

        if (result == null) {
            result = new HashMap();
            sc.setAttribute(Constants.sc.HOST_MODEL_MAP, result);
        }

        return result;
    }

    private void saveToHostModelMap(HashMap map) {
        HttpServletRequest request = SamUtil.getCurrentRequest();
        ServletContext sc = request.getSession().getServletContext();

        if (map == null) {
            map = new HashMap();
        }
        sc.setAttribute(Constants.sc.HOST_MODEL_MAP, map);
    }
}
