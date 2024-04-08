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


// ident	$Id: MultiHostUtil.java,v 1.9 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import java.util.ArrayList;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.StripedGrp;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.EQ;

import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

public class MultiHostUtil {

    protected static SamQFSAppModel app = null;

    public static SamQFSAppModel getApp() throws SamFSException {
        if (app == null)
            app = SamQFSFactory.getSamQFSAppModel();
        return app;
    }

    public static int countOrdinalsRequired(DiskDev[] meta,
        DiskDev[] data, StripedGrp[] groups) {
        int count = 1; /* one for the FS itself */
        if (meta != null)
            count += meta.length;
        if (data != null)
            count += data.length;
        if (groups != null) {
            for (int i = 0; i < groups.length; i++) {
                DiskDev[] disks = groups[i].getMembers();
                count += disks.length;
            }
        }
        return count;
    }

    public static int countOrdinalsRequired(DiskCache[] meta,
        DiskCache[] data, StripedGroup[] groups) {
        int count = 1; /* one for the FS itself */
        if (meta != null)
            count += meta.length;
        if (data != null)
            count += data.length;
        if (groups != null) {
            for (int i = 0; i < groups.length; i++) {
                DiskCache[] disks = groups[i].getMembers();
                count += disks.length;
            }
        }
        return count;
    }

    public static SamQFSSystemModelImpl getSystemModel(String hostName,
        ArrayList errorHostNames, ArrayList errorExceptions) {

        SamQFSSystemModelImpl model = null;
	try {
            if (app == null) {
                app = getApp();
            }
            model = (SamQFSSystemModelImpl)
            app.getSamQFSSystemModel(hostName);
	    if (model.isDown()) {
		errorHostNames.add(hostName);
		errorExceptions.add(
                    new SamFSException("logic.hostIsDown"));
		model = null;
	    }
	} catch (SamFSException e) {
            errorHostNames.add(hostName);
            errorExceptions.add(new SamFSException(
                "logic.invalidHostName"));
	    TraceUtil.trace1("system model was null for " + hostName);
        }
        return model;
    }

    /**
     * @return an array of models. The indices of members and models will
     *  match, so that model[i] is the model for hostNames[i]
     */
    public static SamQFSSystemModelImpl[] getSystemModels(String[] hostNames)
        throws SamFSMultiHostException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        SamQFSSystemModelImpl[] models =
            new SamQFSSystemModelImpl[hostNames.length];
        for (int i = 0; i < hostNames.length; i++) {
            models[i] = getSystemModel(hostNames[i],
                                       errorHostNames, errorExceptions);
        }

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }
        return models;
    }

    /**
     * @return an array of models. The indices of members and models will
     *  match, so that model[i] is the model for members[i]
     */
    public static SamQFSSystemModelImpl[] getSystemModels(
        SharedMember[] members)
        throws SamFSMultiHostException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        SamQFSSystemModelImpl[] models =
            new SamQFSSystemModelImpl[members.length];
        for (int i = 0; i < members.length; i++) {
            models[i] = getSystemModel(members[i].getHostName(),
                                       errorHostNames, errorExceptions);
        }

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }
        return models;
    }

    // Returns the ordinal at the start of a block containing at least count
    // available ordinals.
    public static int getAvailableOrdinals(int count,
        SamQFSSystemModelImpl[] models) throws SamFSMultiHostException {

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();
        int firstAvailableOrdinal = 0;
        int currentHost = 0;

        try {
            int[] in_use = new int[0];
            EQ eq = null;
            for (currentHost = 0; currentHost < models.length; currentHost++) {
                eq = FS.getEqOrdinals(models[currentHost].getJniContext(),
                    count, in_use);
                in_use = eq.getEqsInUse();
            }
            firstAvailableOrdinal = eq.getFirstFreeEq();
        }
        catch (SamFSException e) {
            errorHostNames.add(models[currentHost].getHostname());
            errorExceptions.add(e);
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        return firstAvailableOrdinal;
    }

    /**
     * @param fsInfo look for eq-s used by this fs
     * @param hostModel look for eq-s on this host
     * @throws SamFSException if eq-s unavailable or other error occurs
     */
    protected void verifyEQsAreAvailOnNewHost(FSInfo fsInfo,
        SamQFSSystemModelImpl hostModel) throws SamFSException {

        DiskDev[] dataDevices = fsInfo.getDataDevices();
        DiskDev[] metaDevices = fsInfo.getMetadataDevices();
        StripedGrp[] stripedGroups = fsInfo.getStripedGroups();

        int countEqs = countOrdinalsRequired(
            metaDevices, dataDevices, stripedGroups);

        int[] eqs = new int[countEqs];
        int eqIndex = 0;
        for (int i = 0; i < dataDevices.length; i++) {
            eqs[eqIndex++] = dataDevices[i].getEquipOrdinal();
        }
        for (int i = 0; i < metaDevices.length; i++) {
            eqs[eqIndex++] = metaDevices[i].getEquipOrdinal();
        }
        for (int i = 0; i < stripedGroups.length; i++) {
            DiskDev[] disks = stripedGroups[i].getMembers();
            for (int j = 0; j < disks.length; j++) {
                eqs[eqIndex++] = disks[j].getEquipOrdinal();
            }
        }
            eqs[eqIndex] = fsInfo.getEqu();

        FS.checkEqOrdinals(hostModel.getJniContext(), eqs);
    }
}
