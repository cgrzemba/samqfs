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

// ident	$Id: Notification.java,v 1.14 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.admin;


import com.sun.netstorage.samqfs.mgmt.SamFSException;


public interface Notification {


    public String getName() throws SamFSException;

    public void setName(String name) throws SamFSException;


    public String getEmailAddress() throws SamFSException;


    public boolean isDeviceDownNotify() throws SamFSException;

    public void setDeviceDownNotify(boolean devDown) throws SamFSException;


    public boolean isArchiverInterruptNotify() throws SamFSException;

    public void setArchiverInterruptNotify(boolean archIntr)
        throws SamFSException;


    public boolean isReqMediaNotify() throws SamFSException;

    public void setReqMediaNotify(boolean reqMedia) throws SamFSException;


    public boolean isRecycleNotify() throws SamFSException;

    public void setRecycleNotify(boolean recycle) throws SamFSException;

    public boolean isDumpInterruptNotify() throws SamFSException;

    public void setDumpInterruptNotify(boolean dumpIntr) throws SamFSException;

    public boolean isFsNospaceNotify();

    public void setFsNospaceNotify(boolean fsNospace);

    public boolean isHwmExceedNotify();
    public void setHwmExceedNotify(boolean hwmExceed);

    public boolean isAcslsErrNotify();
    public void setAcslsErrNotify(boolean acslsErr);

    public boolean isAcslsWarnNotify();
    public void setAcslsWarnNotify(boolean acslsWarn);

    public boolean isDumpWarnNotify();
    public void setDumpWarnNotify(boolean dumpWarn);

    public boolean isLongWaitTapeNotify();
    public void setLongWaitTapeNotify(boolean longWaitTape);

    public boolean isFsInconsistentNotify();
    public void setFsInconsistentNotify(boolean fsInconsistent);

    public boolean isSystemHealthNotify();
    public void setSystemHealthNotify(boolean systemHealth);

}
