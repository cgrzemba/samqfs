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

// ident	$Id: FSArchiveDirective.java,v 1.4 2008/05/16 18:38:59 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

public interface FSArchiveDirective {

    public String getFileSystemName();

    public String getFSArchiveLogfile();
    public void setFSArchiveLogfile(String logFile);

    public long getFSInterval();
    public void setFSInterval(long interval);

    public int getFSIntervalUnit();
    public void setFSIntervalUnit(int unit);

    // the return values are defined in GlobalArchiveDirective.java
    public int getFSArchiveScanMethod();
    public void setFSArchiveScanMethod(int method);

    // this method needs to be called for any change in
    // FSArchiveDirective to take effect, i.e. after the
    // client is done calling changeXXX() or setXXX() methods,
    // this method needs to be called for these changes to be
    // effective
    public void changeFSDirective() throws SamFSException;
}
