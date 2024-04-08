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

// ident $Id: SharedFSFilter.java,v 1.3 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

/**
 * This class encodes and decodes filter criteria that are used in the Shared
 * File System Pages since version 5.0.  This class contains the definitions of
 * the target filter objects, and the definitions of filter conditions.  The
 * filter conditions are defined as strings instead of integers because the
 * filter will eventually be saved in the management station .conf file.  Using
 * strings for filter condition will make the saved string much more readable.
 *
 * The criteria will be formed in the format of:
 * <target><linker><condition><linker><criterion>
 *
 * An empty criteria means no filtering is needed.
 */
public class SharedFSFilter {

    // Linker to form the criteria string
    public static final String LINKER = "->";

    // Filter targets
    public static final String TARGET_HOST_NAME = "hostname";
    public static final String TARGET_TYPE = "type";
    public static final String TARGET_IP_ADDRESS = "ipaddress";
    public static final String TARGET_STATUS = "status";

    // filter conditions definitions
    public static final String COND_CONTAINS = "contains";
    public static final String COND_DOES_NOT_CONTAIN = "doesnotcontains";
    public static final String COND_STARTS_WITH = "startswith";
    public static final String COND_ENDS_WITH = "endswith";
    public static final String COND_IS = "is";
    public static final String COND_IS_NOT = "isnot";

    // Status definitions
    public static final String STATUS_OK = "ok";
    public static final String STATUS_MOUNTED = "mounted";
    public static final String STATUS_ACCESS_ENABLED = "accessenabled";
    public static final String STATUS_IN_ERROR = "inerror";

    // Host type definitions
    public static final String TYPE_MDS_PMDS = "mdspmds";
    public static final String TYPE_CLIENTS = "clients";

    // preset filters
    public static final String FILTER_NONE = "";
    public static final String FILTER_OK =
        TARGET_STATUS + LINKER + COND_IS + LINKER + STATUS_OK;
    public static final String FILTER_UNMOUNTED =
        TARGET_STATUS + LINKER + COND_IS_NOT + LINKER + STATUS_MOUNTED;
    public static final String FILTER_DISABLED =
        TARGET_STATUS + LINKER + COND_IS_NOT + LINKER + STATUS_ACCESS_ENABLED;
    public static final String FILTER_IN_ERROR =
        TARGET_STATUS + LINKER + COND_IS + LINKER + STATUS_IN_ERROR;

    // Menu option to launch the advanced filter pagelet
    // NOTE: This should not be used as the content of the filter.  This
    // definition is used only to help toggling the drop down menu
    public static final String FILTER_CUSTOM = "customfilter";

    // TODO: Reevaluate this line if storage node is supported
    public static final String FILTER_FAULTS =
        TARGET_STATUS + LINKER + COND_IS + LINKER + "faults";

    public static final String FILTER_MDS_PMDS =
        TARGET_HOST_NAME + LINKER + COND_IS + LINKER + TYPE_MDS_PMDS;
    public static final String FILTER_CLIENTS =
        TARGET_HOST_NAME + LINKER + COND_IS + LINKER + TYPE_CLIENTS;

    // Constant to link multiple criterion
    public static final String FILTER_DELIMITOR = "###";

    // Class variables
    private String target = null;
    private String condition = null;
    private String content = null;

    /**
     * Construct the SharedFSFilter object by providing the advanced filter
     * criteria from the custom filter drop down menu
     * @param customFilter
     */
    public SharedFSFilter(String criteria) {
System.out.println("SharedFSFilter const: " + criteria);
        if (criteria != null || criteria.length() == 0) {
            String [] inputArray = criteria.split(LINKER);
            if (inputArray.length == 3) {
                this.target = inputArray[0];
                this.condition = inputArray[1];
                this.content = inputArray[2];
                return;
            }
        }

        // If code reaches here, bad criteria detected.
        target = "";
        condition = "";
        content = "";
        System.out.println("Bad criteria entered in ShardFSFilter!");
    }

    public SharedFSFilter(String target, String condition, String content) {
        this.target = target;
        this.condition = condition;
        this.content = content;
    }

    public String getCondition() {
        return condition;
    }

    public String getContent() {
        return content;
    }

    public String getTarget() {
        return target;
    }

    /**
     * Create the shrink option string that is used in the underlying layer.
     * Used in Shrink Wizard.
     * @Override
     * @return shrink option string
     */
    public String toString() {
        return target + LINKER +
               condition + LINKER +
               content;
    }
}
