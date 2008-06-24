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

// ident	$Id: Configuration.java,v 1.2 2008/06/11 21:16:19 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.admin;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;

/**
 * This class holds the configuration information used to display the first
 * time configuration checklist.
 */
public class Configuration {
    private int libraryCount;
    private List <String> libraryName;
    private int tapeCount;
    private int qfsCount;
    private int diskVolumeCount;
    private int poolCount;
    private int protoCount;
    private List<String> protoName;
    private int storageNodeCount;
    private int clientCount;

    private String rawConfig;
    private Configuration() {
    }

    public Configuration(String rawconfig) {
        this.rawConfig = rawconfig;
        parseConfiguration(this.rawConfig);
    }

    private void parseConfiguration(String rawconfig) {
        Properties props = ConversionUtil.strToProps(rawconfig);


        // parse individual attributes here-
        // library count
        this.libraryCount = parseInteger(props.getProperty("lib_count"));

        // library names
        this.libraryName = parseStrings(props.getProperty("lib_names"));

        // tape count
        this.tapeCount = parseInteger(props.getProperty("tape_count"));

        // qfs count
        this.qfsCount = parseInteger(props.getProperty("qfs_count"));

        // disk volumes count
        this.diskVolumeCount =
            parseInteger(props.getProperty("disk_vols_count"));

        // volume pool count
        this.poolCount = parseInteger(props.getProperty("volume_pools"));

        // proto fs count
        this.protoCount = parseInteger(props.getProperty("object_qfs_protos"));

        // proto fs names
        this.protoName = parseStrings(props.getProperty("object_qfs_names"));

        // storage node count
        this.storageNodeCount =
            parseInteger(props.getProperty("storage_nodes"));

        // client count
        this.clientCount = parseInteger(props.getProperty("clients"));
    }

    /**
     * parse a given string into its integer form
     * @param String - integer in string form.
     * @return int - the resulting integer value.
     */
    private int parseInteger(String s) {
        int result = 0;

        if (s != null && !s.equals("")) {
            try {
                result = Integer.parseInt(s);
            } catch (NumberFormatException nfe) {
                // Nothing to do
            }
        }

        return result;
    }

    /**
     * break a space delimited string to its component strings in a list.
     *
     * @param String - space delimited strings
     * @return List<String> - of the component strings
     */
    private List<String> parseStrings(String s) {
        List <String> result = new ArrayList<String>();

        if (s != null && !s.equals("")) {
            char separator = ' ';
            String [] token = null;
            try {
                token = ConversionUtil.strToArray(s, separator);
            } catch (SamFSException sfe) {
                // nothing to do here
            }
            for (int i = 0; i < token.length; i++) {
                result.add(token[i].trim());
            }
        }

        return result;
    }

    // getters
    public int getLibraryCount() {
        return this.libraryCount;
    }

    public List<String> getLibraryName() {
        return this.libraryName;
    }

    public int getTapeCount() {
        return this.tapeCount;
    }

    public int getQFSCount() {
        return this.qfsCount;
    }

    public int getDiskVolumeCount() {
        return this.diskVolumeCount;
    }

    public int getVolumePoolCount() {
        return this.poolCount;
    }

    public int getProtoFSCount() {
        return this.protoCount;
    }

    public List<String> getProtoFSName() {
        return this.protoName;
    }

    public int getStorageNodeCount() {
        return this.storageNodeCount;
    }

    public int getClientCount() {
        return this.clientCount;
    }
}
