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

// ident	$Id: Alarm.java,v 1.12 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.alarm;


import java.util.GregorianCalendar;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.media.Library;


public interface Alarm {

    // static strings

    public static final int OK = -1;
    public static final int CRITICAL = 0;
    public static final int MAJOR = 1;
    public static final int MINOR = 2;
    public static final int ACTIVE = 3;
    public static final int ACKNOWLEDGED = 4;

    public static final int MAXEQ = 65534;

    // getters
    public long getAlarmID() throws SamFSException;
    public int getSeverity() throws SamFSException;
    public String getDescription() throws SamFSException;
    public GregorianCalendar getDateTimeGenerated() throws SamFSException;
    public int getStatus() throws SamFSException;
    public void setStatus(int status) throws SamFSException;

    // alarm could be from many different sources; the only
    // thing that is guaranteed is that there would be an
    // equipment ordinal number associated with the alarm;
    // among all other getter methods, only one will return
    // non-null value

    public int getAssociatedEquipOrdinal() throws SamFSException;
    public Library getAssociatedLibrary() throws SamFSException;
    public String getAssociatedLibraryName() throws SamFSException;
    public FileSystem getAssociatedFileSystem() throws SamFSException;
}
