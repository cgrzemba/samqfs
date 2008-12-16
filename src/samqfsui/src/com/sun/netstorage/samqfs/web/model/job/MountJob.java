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

// ident	$Id: MountJob.java,v 1.10 2008/12/16 00:12:22 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import java.util.GregorianCalendar;

public interface MountJob extends BaseJob {

    // the VSN name and Library name in page catalog should be links
    public String getVSNName() throws SamFSException;

    public int getMediaType() throws SamFSException;

    public String getLibraryName() throws SamFSException;

    public boolean isArchiveMount() throws SamFSException;

    public long getProcessId() throws SamFSException;

    public String getInitiatingUsername() throws SamFSException;

    public GregorianCalendar getTimeInQueue() throws SamFSException;

    public int getStatusFlag();
}
