/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: AlarmSummaryImpl.java,v 1.5 2008/03/17 14:43:47 am143972 Exp $


package com.sun.netstorage.samqfs.web.model.impl.jni.alarm;


import com.sun.netstorage.samqfs.mgmt.adm.FaultSummary;

import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;


public class AlarmSummaryImpl implements AlarmSummary {


    private int critical = 0;
    private int major = 0;
    private int minor = 0;


    public AlarmSummaryImpl(FaultSummary jniFaultSummary) {

        if (jniFaultSummary != null) {
            this.critical = jniFaultSummary.critical;
            this.major = jniFaultSummary.major;
            this.minor = jniFaultSummary.minor;
        }

    }


    // getters

    public int getCriticalTotal() {

        return critical;

    }


    public int getMajorTotal() {

        return major;

    }


    public int getMinorTotal() {

        return minor;

    }

}
