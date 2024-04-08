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

// ident	$Id: ArchiveVSNMapImpl.java,v 1.17 2008/12/16 00:12:20 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.BaseVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.CatVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;
import com.sun.netstorage.samqfs.mgmt.arc.VSNOp;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;

public class ArchiveVSNMapImpl implements ArchiveVSNMap {

    private ArchiveCopy archCopy = null;
    private VSNMap map = null;
    private int archType = -1;
    private boolean rearchive = false;
    private String vsnExpression = null;
    private String startVSNname = null;
    private String endVSNname = null;
    private String poolExpression = null;
    private boolean mapUpdateNeeded = false;
    private boolean inheritedFromALLSETS = false;

    /*
     * this is set internally by logic layer for all maps that explicitely
     * show in the archiver.cmd.
     * needs to be set by the UI using willBeSaved() for all other maps, in
     * case they have been modified therefore need to show up in the file.
     */
    private boolean save = false;

    /*
     * retrieving member vsn-s and computing available space is time-consuming,
     * since all catalog entries are inspected.
     * we postpone the fetching of these fields until a get() method is called.
     */
    private boolean memberVSNsFetched = false; // covers all 3 fields below
    private ArrayList memberVSNNames = new ArrayList(); // lazy eval
    private ArrayList memberVSNPools = new ArrayList(); // lazy eval
    private long availableSpace = -1; // lazy eval

    /**
     * create an empty map
     */
    public ArchiveVSNMapImpl() {
        this.map = new VSNMap("Unknown",
                              "Unknown",
                              new String[0],
                              new String[0]);
    }

    /**
     * create an empty map for an allsets copy
     */
    public ArchiveVSNMapImpl(String allSetsCopyName) {
        this.map = new VSNMap(allSetsCopyName,
                              "Unknown",
                              new String[0],
                              new String[0]);
    }

    public ArchiveVSNMapImpl(ArchiveCopy archCopy,
                             VSNMap map) throws SamFSException {

        this.archCopy = archCopy;
        this.map = map;

        if (this.map != null) {

            archType = SamQFSUtil.getMediaTypeInteger(map.getMediaType());
            vsnExpression = SamQFSUtil.
                createVSNExpressionFromStrings(map.getVSNNames());
            poolExpression = SamQFSUtil.
                createVSNExpressionFromStrings(map.getPoolNames());
            this.save = true;
        } else {
            this.map = new VSNMap("Unknown",
                                  "Unknown",
                                  new String[0],
                                  new String[0]);
        }
    }


    /**
     * @since 4.4
     * allows the cloning of an allsets VSNMap and associate it with
     * the specified ArchiveCopy object
     * archive copy member must be set after the object is constructed.
     */
    public ArchiveVSNMapImpl(ArchiveVSNMapImpl src, String copyName) {
        this.inheritedFromALLSETS = true;
        this.map = new VSNMap(copyName,
                              src.map.getMediaType(),
                              src.map.getVSNNames(),
                              src.map.getPoolNames());

        this.archType = src.archType;
        this.vsnExpression = src.vsnExpression;
        this.startVSNname = src.startVSNname;
        this.endVSNname = src.endVSNname;
        this.poolExpression = src.poolExpression;
    }

    /**
     * @since 4.4
     */
    public boolean inheritedFromALLSETS() { return inheritedFromALLSETS; }

    /**
     * @since 4.4
     * when a policy is saved, this map will be saved only if this flag is set.
     * this is needed for policies that do not have any maps explicitely
     * configured (for instance those that inherit their vsnMaps from 'allsets')
     * and they should not be sent to the lower layer unless explicitely set
     * by user.
     */
    public void setWillBeSaved(boolean save) {
        this.save = save;
    }
    public boolean getWillBeSaved() { return save; }

    /**
     * @since 4.4
     */
    public boolean isEmpty() {
        if (map == null)
           return true;
        return (map.isEmpty());
    }

    private void updateVSNAndPoolInfo() throws SamFSException {
        availableSpace = 0;
        memberVSNNames.clear();
        updatePoolsFromPoolNames(map.getPoolNames());
        updateVSNsFromExpressionList(map.getVSNNames());
    }

    public VSNMap getJniVSNMap() {
        if (mapUpdateNeeded) {

            // update the vsn expression
            boolean commaNeeded = false;
            StringBuffer exp = new StringBuffer();
            if (SamQFSUtil.isValidString(vsnExpression)) {
                exp.append(vsnExpression);
                commaNeeded = true;
            }
            if ((SamQFSUtil.isValidString(startVSNname)) ||
                (SamQFSUtil.isValidString(endVSNname))) {
                if (commaNeeded)
                    exp.append(",");
                exp.append(SamQFSUtil.createExpression(startVSNname,
                                                       endVSNname));
            }

            vsnExpression = exp.toString();
        }

        String vsnCopyName = map.getCopyName();
        if (vsnCopyName.equals("Unknown")) {
            ArchivePolicy archPolicy = null;
            if (archCopy != null)
                archPolicy = archCopy.getArchivePolicy();
            if (archPolicy != null) {
                String RString = "";
                if (rearchive)
                    RString = "R";
                vsnCopyName = archPolicy.getPolicyName() + "." +
                    archCopy.getCopyNumber() + RString;
            }
        }

        map.setCopyName(vsnCopyName);
        map.setMediaType(SamQFSUtil.getMediaTypeString(archType));
        map.setVSNNames(
            SamQFSUtil.getStringsFromCommaStream(getMapExpression()));
        map.setPoolNames(
            SamQFSUtil.getStringsFromCommaStream(getPoolExpression()));

        return map;
    }


    public ArchiveCopy getArchiveCopy() { return archCopy; }
    public void setArchiveCopy(ArchiveCopy copy) {
        this.archCopy = copy;
    }

    public boolean isRearchive() { return rearchive; }
    public void setRearchive(boolean rearchive) {
        this.rearchive = rearchive;
    }

    public int getArchiveMediaType() { return archType; }
    public void setArchiveMediaType(int type) {
        this.archType = type;
        map.setMediaType(SamQFSUtil.getMediaTypeString(type));
        mapUpdateNeeded = true;
        memberVSNsFetched = false; // will need to refetech them
    }

    public String getPoolExpression() { return poolExpression; }
    public void setPoolExpression(String poolExp) {
        // pool objects get updated only after updateXXX() is called
        this.poolExpression = poolExp;
        map.setPoolNames(SamQFSUtil.getStringsFromCommaStream(poolExp));
        mapUpdateNeeded = true;
        memberVSNsFetched = false; // will need to refetech them
    }

    public String getMapExpression() { return vsnExpression; }
    public void setMapExpression(String expression) {
        // individual vsn names get updated only after updateXXX() is called
        if (expression != null) {
            vsnExpression = expression;
            map.setVSNNames(SamQFSUtil.
                            getStringsFromCommaStream(expression));
            mapUpdateNeeded = true;
            memberVSNsFetched = false; // will need to refetech them
        }
    }

    public String getMapExpressionStartVSN() { return startVSNname; }
    public void setMapExpressionStartVSN(String expression) {
        if ((expression != null) && (!expression.equals(startVSNname))) {
            startVSNname = expression;
            mapUpdateNeeded = true;
            memberVSNsFetched = false; // will need to refetech them
        }
    }

    public String getMapExpressionEndVSN() { return endVSNname; }
    public void setMapExpressionEndVSN(String expression) {
        if ((expression != null) && (!expression.equals(endVSNname))) {
            endVSNname = expression;
            mapUpdateNeeded = true;
            memberVSNsFetched = false; // will need to refetech them
        }
    }

    public String[] getMemberVSNNames() throws SamFSException {
        if (!memberVSNsFetched) {
            updateVSNAndPoolInfo();
            memberVSNsFetched = true;
        }
        return (String[]) memberVSNNames.toArray(new String[0]);
    }

    public long getAvailableSpace() throws SamFSException {
        if (!memberVSNsFetched) {
            updateVSNAndPoolInfo();
            memberVSNsFetched = true;
        }
        return availableSpace;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        if (archCopy != null)
            buf.append("Archive Copy Number: ")
                .append(archCopy.getCopyNumber())
                .append("\n")
                .append("Archive Type: ")
                .append(archType)
                .append("\n")
                .append("Rearchive: ")
                .append(rearchive)
                .append("\n");

        if (memberVSNNames != null) {
            for (int i = 0; i < memberVSNNames.size(); i++) {
                buf.append("VSN: \n")
                    .append((String)memberVSNNames.get(i))
                    .append("\n");
            }
        }

        if (memberVSNPools != null) {
            for (int i = 0; i < memberVSNPools.size(); i++) {
                buf.append("VSN Pools: \n")
                    .append(((VSNPool) memberVSNPools.get(i)).getPoolName())
                    .append("\n");
            }
        }

        buf.append("Space Available: ")
            .append(availableSpace)
            .append("\n")
            .append("VSN Expression: ")
            .append(getMapExpression())
            .append("\n")
            .append("Pool Expression: ")
            .append(getPoolExpression())
            .append("\n");

        return buf.toString();
    }

    private void updateVSNsFromExpressionList(String[] vsnExpressions)
        throws SamFSException {

        SamQFSSystemModelImpl model = null;
        if ((archCopy != null) && (archCopy.getArchivePolicy() != null)) {
            ArchivePolicyImpl policy =
                (ArchivePolicyImpl) archCopy.getArchivePolicy();
            model = (SamQFSSystemModelImpl) policy.getModel();
        }

        Ctx ctx = null;
        if (model != null) {
            ctx = model.getJniContext();
        }

        if ((vsnExpressions != null) && (ctx != null)) {

            if (!map.getMediaType().equals("dk") &&
                !map.getMediaType().equals("cb")) {
                /* tape vsn */
                CatEntry vsn = null;
                BaseVSNPoolProps mapProps =
                    VSNOp.getPoolPropsByMap(ctx,
                                            this.map,
                                            0,
                                            -1,
                                            Media.VSN_NO_SORT, false);

                availableSpace = mapProps.getFreeSpace();

                CatEntry vsnsFromCat [] =
                    ((CatVSNPoolProps) mapProps).getCatEntries();

                if (vsnsFromCat != null) {
                    for (int j = 0; j < vsnsFromCat.length; j++) {
                        vsn = vsnsFromCat[j];
                        if ((!vsn.isReserved())
                            && !memberVSNNames.contains(vsn.getVSN())) {
                            memberVSNNames.add(vsn.getVSN());
                        }
                    }
                }

            } else {
                /* disk vsn */

                TraceUtil.trace3("getting map props for " + map);
                BaseVSNPoolProps mapProps =
                    VSNOp.getPoolPropsByMap(ctx,
                                            this.map,
                                            0,
                                            -1,
                                            Media.VSN_NO_SORT, false);
                availableSpace = mapProps.getFreeSpace();
                DiskVol[] dvols =
                    ((DiskVSNPoolProps) mapProps).getDiskEntries();
                if (dvols != null) {
                    for (int i = 0; i < dvols.length; i++) {
                        String name = dvols[i].getVolName();
                        if (!memberVSNNames.contains(name))
                            memberVSNNames.add(name);
                    }
                }
            }
        } // vsn expr
    }

    private void updatePoolsFromPoolNames(String[] poolNames)
        throws SamFSException {

        memberVSNPools.clear();

        SamQFSSystemModelImpl model = null;
        if ((archCopy != null) && (archCopy.getArchivePolicy() != null)) {
            ArchivePolicyImpl policy =
                (ArchivePolicyImpl) archCopy.getArchivePolicy();
            model = (SamQFSSystemModelImpl) policy.getModel();
        }

        Ctx ctx = null;
        if (model != null) {
            ctx = model.getJniContext();
        }

        if ((ctx != null) && (poolNames != null) && (poolNames.length > 0)) {
            for (int i = 0; i < poolNames.length; i++) {
                com.sun.netstorage.samqfs.mgmt.arc.VSNPool jniPool =
                    VSNOp.getPool(ctx, poolNames[i]);
                if (jniPool != null) {
                    VSNPoolImpl pool = new VSNPoolImpl(model, jniPool);
                    memberVSNPools.add(pool);
                    availableSpace += pool.getSpaceAvailable();
                    String members[] = pool.getMemberVSNNames();
                    if (members != null) {
                        for (int j = 0; j < members.length; j++) {
                            if (memberVSNNames.indexOf(members[j]) == -1) {
                                memberVSNNames.add(members[j]);
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Expose the internal map so we can set the VSN Map Name
     *
     * NOTE: workaround for the allsets vsn map
     */
    public VSNMap getInternalVSNMap() {
        return map;
    }
}
