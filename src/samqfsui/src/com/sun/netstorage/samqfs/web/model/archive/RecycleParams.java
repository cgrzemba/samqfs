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

// ident	$Id: RecycleParams.java,v 1.5 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

public interface RecycleParams {

    public String getLibraryName();

    public int getHWM() throws SamFSException;
    public void setHWM(int hwm) throws SamFSException;

    public int getMinGain() throws SamFSException;
    public void setMinGain(int gain) throws SamFSException;

    public int getVSNLimit() throws SamFSException;
    public void setVSNLimit(int limit) throws SamFSException;

    public long getSizeLimit() throws SamFSException;
    public void setSizeLimit(long limit) throws SamFSException;

    public int getSizeUnit() throws SamFSException;
    public void setSizeUnit(int unit) throws SamFSException;

    public boolean isPerform() throws SamFSException;
    public void setPerform(boolean perform) throws SamFSException;
}
