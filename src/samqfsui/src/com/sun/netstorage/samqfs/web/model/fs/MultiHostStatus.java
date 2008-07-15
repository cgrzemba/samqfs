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

// ident	$Id: MultiHostStatus.java,v 1.1 2008/07/15 17:19:45 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;

/** 
 * This is a help class for the multi-host status display view.
 */
public class MultiHostStatus {
    // job types for the multi-status display view
    public enum TYPE {DSP_MOUNT,
            DSP_UMOUNT,
            DSP_CHANGE_MOUNT_OPTIONS,
            DSP_ADD_CLIENTS,
            DSP_GROW_FS,
            DSP_REMOVE_CLIENTS,
            DSP_SET_ADV_NET_CONFIG}

    // possible job  status'
    public enum STATUS {SUCCESS,
            PARTIAL_FAILURE,
            FAILURE,
            PENDING}
            
    // the raw status string
    private String rawStatus = null;

    // Properties representation of the raw status string
    private Properties props = null;

    // current job type
    private TYPE jobType = null;

    // current status of the job
    private STATUS status = null;

    // map of host -> error
    private Map<String,String> hostErrorMap = null;

    // a list of hosts with errors
    private List<String> hostsWithError = null;

    /** 
     * hide the default constructor since an instance of this class would be
     * useless without the status string
     */
    private MultiHostStatus() {}
    public MultiHostStatus(String status) {
        this.rawStatus = status;
        this.props = ConversionUtil.strToProps(status);
    }

    /** return the integer represented by the given key */
    private int parseInteger(String key) {
        // return -1 if the key is not present
        if (key == null || this.rawStatus.indexOf(key) == -1)
            return -1;

        // else parse integer
        int rtnval = -1;
        try {
            rtnval = Integer.parseInt((String)this.props.get(key));
        } catch (NumberFormatException nfe) {
            // do nothing
        }

        return rtnval;
    }

    /** return the type of the current job */
    public TYPE getJobType() {
        return this.jobType;
    }

    /** return the current status of the job */
    public STATUS getJobStatus() {
        return this.status;
    }

    /** the total number of hosts */
    public int getTotalHostCount() {
        int hosts = parseInteger("host_count");

        return hosts == -1 ? 0 : hosts;
    }

    /** the number of hosts that have succeeded */
    public int getSucceededHostCount() {
        // hosts_responding?
        int hosts = parseInteger("hosts_responding");

        return hosts == -1 ? 0 : hosts;
    }
    

    /** the number of hosts that have failed */
    public int getFailedHostCount() {
        int result = getTotalHostCount() - 
            getPendingHostCount() - getSucceededHostCount();

        return result > 0  ? result : 0;
    }

    /** number of hosts that are pending i.e. yet to attempt the job */
    public int getPendingHostCount() {
        int hosts = parseInteger("hosts_pending");

        return hosts == -1 ? 0 : hosts;
    }

    /** parse and store the list of host with errors as well as the error codes
     * each host
     */
    private void parseHostErrors() {
        // retrieve the list of error codes in the status
        String rawErrors = (String)this.props.get("host_errors");
        if (rawErrors != null) {
            this.hostsWithError = new ArrayList<String>();
            this.hostErrorMap = new HashMap<String,String>();

            String [] codes = rawErrors.split(Character.toString(' '));

            // for each error code, find the associated hosts and append to the
            // list
            for (int i = 0; i < codes.length; i++) {
                String rawHosts = (String)this.props.get(codes[i]);
                
                if (rawHosts != null) {
                    String [] hosts = rawHosts.split(Character.toString(' '));
                    for (int j = 0; j < hosts.length; j++) {
                        this.hostsWithError.add(hosts[j]);

                        // save the host name and its corresponding error for
                        // later use
                        this.hostErrorMap.put(hosts[j], codes[i]);
                    } // end for j
                } // if rawHosts 
            } // end for i
        } // end if rawErrors
    }

    /** return the list of hosts with errors */
    public List<String> getHostsWithError() {
        if (this.hostsWithError == null)
            parseHostErrors();

        return this.hostsWithError;
    }

    /** return the error for the given host */
    public String getHostError(String host) {
        if (this.hostErrorMap == null) 
            parseHostErrors();

        String error = null;
        if (this.hostErrorMap != null)
            error = this.hostErrorMap.get(host);


        return error;
    }
}
