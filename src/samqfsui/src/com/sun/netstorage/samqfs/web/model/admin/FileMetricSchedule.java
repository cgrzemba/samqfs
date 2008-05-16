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

// ident	$Id: FileMetricSchedule.java,v 1.5 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.admin;

import java.util.Date;
import java.util.Properties;

/**
 * Currently the FileMetricSchedule is an empty subclass, it has no additional
 * properties
 */
public class FileMetricSchedule extends Schedule {

    public FileMetricSchedule(Properties props) {
        super(props);
    }

    public FileMetricSchedule(
        String fsName, int unit, Date startDate) {

        setTaskId(ScheduleTaskID.REPORT);
        setTaskName(fsName);
        setStartTime(startDate);
        setPeriodicity(1); // for now it is always daily, weekly, monthly
        setPeriodicityUnit(unit);

    }

    public Properties decodeSchedule(Properties props) {
        // nothing to be done, just return props
        return props;
    }

    public String encodeSchedule() { return " "; }
}
