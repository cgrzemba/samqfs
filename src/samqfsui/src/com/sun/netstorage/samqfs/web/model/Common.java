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

// ident $Id: Common.java,v 1.8 2008/03/17 14:43:42 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import java.util.ArrayList;

/*
 * This class includes methods that are common to both implementations of the
 * logic tier (simulator and real mode).
 */
public class Common {

    public static boolean sameLib(Library l1, Library l2)
        throws SamFSException {
        return l1.getSerialNo().equals(l2.getSerialNo());
    }

    public static Library[] getAllLibrariesFromServers() throws SamFSException {

        Library[] empty = new Library[0];

        // STEP1: get all managed servers
        SamQFSAppModel app = SamQFSFactory.getSamQFSAppModel();
        SamQFSSystemModel[] models = app.getAllSamQFSSystemModels();
        if (models == null)
            return empty;

        // STEP2: get libraries from each server and eliminate duplicates
        ArrayList libs = new ArrayList();
        Library[] crtLibs;
        for (int i = 0; i < models.length; i++) {
            try {
                crtLibs =
                    models[i].getSamQFSSystemMediaManager().getAllLibraries();
                // for each library on host i
                for (int c = 0, gSz = libs.size(); c < crtLibs.length; c++) {

                    if (BaseDevice.MTYPE_HISTORIAN == crtLibs[c].getEquipType())
                        continue; // skip

                    // check if already in the global list
                    boolean found = false;
                    for (int g = 0; g < gSz; g++) {
                        if (sameLib(crtLibs[c], (Library)libs.get(g)))
                            found = true;
                    }
                    if (!found)
                        libs.add(crtLibs[c]);
                }
	    } catch (SamFSException e) {
                TraceUtil.trace1(
                    "Skipped model for which cannot get libraries");
            }
	}

        return (Library[]) libs.toArray(empty);
    }

    // compare if two libraries are sharing the same ACSLS Server by providing
    // both library objects
    public static boolean shareACSLSServer(Library l1, Library l2)
        throws SamFSException {

        if (l1.getDriverType() != Library.ACSLS ||
            l2.getDriverType() != Library.ACSLS) {
            return false;
        }

        StkNetLibParam l1Param = l1.getStkNetLibParam();
        StkNetLibParam l2Param = l2.getStkNetLibParam();

        if (l1Param == null || l2Param == null) {
            return false;
        }

        return l1Param.getAcsServerName().equals(l2Param.getAcsServerName());
    }


    // same as the one above except provoding the ACSLS Server name and
    // compare with a different Library Object
    public static boolean shareACSLSServer(String acsServerName, Library l2)
        throws SamFSException {

        StkNetLibParam l2Param = l2.getStkNetLibParam();

        if (l2Param == null) {
            return false;
        }

        return acsServerName.equals(l2Param.getAcsServerName());
    }
}
