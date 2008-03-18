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

// ident	$Id: ArchiveVSNMap.java,v 1.10 2008/03/17 14:43:44 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;

public interface ArchiveVSNMap {

    public ArchiveCopy getArchiveCopy();

    public boolean isRearchive();

    public int getArchiveMediaType();

    public String getPoolExpression();
    public void setPoolExpression(String poolExp);

    public String getMapExpression();
    public void setMapExpression(String expression);

    public String[] getMemberVSNNames() throws SamFSException;
    public long getAvailableSpace() throws SamFSException;

    // start and end vsn methods are only needed for wizard support
    public String getMapExpressionStartVSN();
    public void setMapExpressionStartVSN(String expression);

    public String getMapExpressionEndVSN();
    public void setMapExpressionEndVSN(String expression);

    // this setter is only for new archive policy creation wizard
    public void setArchiveMediaType(int mediaType);

    /**
     * return the internal map of vsns
     */
    public VSNMap getInternalVSNMap();

    /**
     * @since 4.4
     */
    public boolean inheritedFromALLSETS();

    /*
     * next two methods were added in 4.4
     * they are useful when vsn maps that do not show in archiver.cmd
     * (but they are displayed by the UI) are modified by user.
     */
    public boolean getWillBeSaved();
    public void setWillBeSaved(boolean save);
}
