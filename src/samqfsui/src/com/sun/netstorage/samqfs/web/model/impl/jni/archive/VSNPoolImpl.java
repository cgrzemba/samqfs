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

// ident	$Id: VSNPoolImpl.java,v 1.20 2008/03/17 14:43:48 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.BaseVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.CatVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVSNPoolProps;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.mgmt.arc.VSNOp;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.web.model.impl.jni.*;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DiskVolumeImpl;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import java.util.ArrayList;

/**
 *  information about a VSN/disk volume pool
 *  disk volume pools only supported since 4.4
 */
public class VSNPoolImpl
    implements com.sun.netstorage.samqfs.web.model.archive.VSNPool {

    private SamQFSSystemModelImpl model = null;
    private com.sun.netstorage.samqfs.mgmt.arc.VSNPool jniPool = null;
    private BaseVSNPoolProps jniPoolProp = null;
    private String poolName = null;
    private int mediaType = -1;
    private String vsnExp = null;
    private ArrayList memberVSNs = new ArrayList();

    public VSNPoolImpl() {
    }

    public VSNPoolImpl(String poolName,
                       int mediaType,
                       String vsnExp,
                       ArrayList memberVSNs) {

        if (poolName != null)
            this.poolName = poolName;
        this.mediaType = mediaType;
        if (vsnExp != null)
            this.vsnExp = vsnExp;
        if (memberVSNs != null)
            this.memberVSNs = memberVSNs;
    }

    public VSNPoolImpl(SamQFSSystemModelImpl model,
                       com.sun.netstorage.samqfs.mgmt.arc.VSNPool jniPool)
        throws SamFSException {

        this.model = model;
        this.poolName = jniPool.getName();
        this.mediaType =
            SamQFSUtil.getMediaTypeInteger(jniPool.getMediaType());
        this.vsnExp =
            SamQFSUtil.createVSNExpressionFromStrings(jniPool.getVSNs());
        this.jniPool = jniPool;
        this.jniPoolProp = getJniPoolProps();
    }

    /**
     * NOTE: pool name and media type can not be changed for a pool once
     * created. Hence no setters
     */
    public String getPoolName() { return poolName; }
    public int getMediaType() {	return mediaType; }

    public long getSpaceAvailable() throws SamFSException {
        long available = 0;

        if (jniPoolProp != null)
            available = jniPoolProp.getFreeSpace();

        return available;
    }

    public String getVSNExpression() {
        return vsnExp;
    }

    public void update() throws SamFSException {

        String [] newExp = SamQFSUtil.getStringsFromCommaStream(vsnExp);
        Ctx ctx = null;
        if (model != null)
            ctx = model.getJniContext();

        if (jniPool == null) {
            jniPool = new com.sun.netstorage.samqfs.mgmt.arc.VSNPool(poolName,
                            SamQFSUtil.getMediaTypeString(mediaType), newExp);
            VSNOp.addPool(ctx, jniPool);
            jniPoolProp = getJniPoolProps();
        } else {
            jniPool.setVSNs(newExp);
            VSNOp.modifyPool(ctx, jniPool);
            jniPoolProp = getJniPoolProps();
        }

        Archiver.activateCfgThrowWarnings(ctx);
    }

    public Object clone() {
        VSNPoolImpl newPool = null;

        try {
            ArrayList list = null;
            if (memberVSNs != null) {
                list = new ArrayList();
                for (int i = 0; i < memberVSNs.size(); i++) {
                    list.add(memberVSNs.get(i));
                }
            }
            newPool = new VSNPoolImpl(getPoolName(),
                                      getMediaType(),
                                      getVSNExpression(),
                                      list);
        } catch (Exception e) {
        }

        return newPool;
    }

    public String toString() {
        long spaceAvailable = 0;
        try {
            spaceAvailable = getSpaceAvailable();
        } catch (Exception e) {
        }

        StringBuffer buf = new StringBuffer();

        buf.append("Pool Name: ")
            .append(poolName)
            .append("\n")
            .append("Media Type: ")
            .append(mediaType)
            .append("\n")
            .append("Space Available: ")
            .append(spaceAvailable)
            .append("\n")
            .append("VSN Expression: ")
            .append(vsnExp)
            .append("\n");
        try {
            if (memberVSNs != null) {
                for (int i = 0; i < memberVSNs.size(); i++)
                    buf.append("VSN Members: ")
                        .append((VSN)memberVSNs.get(i))
                        .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        buf.append("JNIPoolProps: ")
            .append(jniPoolProp);

        return buf.toString();

    }

    /**
     * @return list of all VSNs that belong to this tape pool
     * @see getMembeDiskVolumes() for disk volume pools
     */
    public VSN[] getMemberVSNs(int start,
                               int size,
                               int sortby,
                               boolean ascending) throws SamFSException {

        VSN[] list = new VSN[0];

        if (model != null) {
            CatVSNPoolProps prop;
            if ("1.3".compareTo(model.getServerAPIVersion()) > 0)
                // pre-4.4, so we are forced to call the old API
                prop = VSNOp.getPoolProps(model.getJniContext(),
                                          getPoolName(), start, size,
                                          (short) sortby, ascending);
            else
                /*
                 * call 4.4 API that passes pool obj instead of name,
                 * so that media type can be taken into account.
                 */
                prop = (CatVSNPoolProps)
                       VSNOp.getPoolPropsByPool(model.getJniContext(),
                                                jniPool,
                                                start,
                                                size,
                                                (short)sortby,
                                                ascending);

            CatEntry[] vsnFromCat = prop.getCatEntries();
            if ((vsnFromCat != null) && (vsnFromCat.length > 0)) {
                list = new VSN[vsnFromCat.length];
                for (int j = 0; j < vsnFromCat.length; j++) {
                    list[j] = ((SamQFSSystemMediaManagerImpl)
                               (model.getSamQFSSystemMediaManager())).
                        getVSN(vsnFromCat[j]);
                }
            }
        }

        return list;

    }

    /**
     * @return VSN names (exclude the reserved ones)
     * this method is time consuming. used internally by VSNMapImpl
     */
    public String[] getMemberVSNNames() throws SamFSException {
        ArrayList list = new ArrayList();
        int i;

        // TODO: this should be a settable property
        jniPoolProp = getJniPoolProps(10000);

        if (mediaType != BaseDevice.MTYPE_DISK &&
            mediaType != BaseDevice.MTYPE_STK_5800) {
            CatEntry[] cat = null;
            CatVSNPoolProps prop = (CatVSNPoolProps) jniPoolProp;
            if (prop != null) {
                cat = prop.getCatEntries();
            }
            if ((cat != null) && (cat.length > 0)) {
                for (i = 0; i < cat.length; i++)
                    if (!cat[i].isReserved())
                        list.add(cat[i].getVSN());
            }
        } else { // diskvols
            DiskVol[] dvs = null;
            DiskVSNPoolProps prop = (DiskVSNPoolProps) jniPoolProp;
            if (prop != null) {
                dvs = prop.getDiskEntries();
            }
            if ((dvs != null) && (dvs.length > 0)) {
                for (i = 0; i < dvs.length; i++)
                    list.add(dvs[i].getVolName());
            }
        }

        return (String[]) list.toArray(new String[0]);
    }

    public int getNoOfVSNsInPool() throws SamFSException {
        int no = 0;
        if (jniPoolProp == null)
            jniPoolProp = getJniPoolProps();
        if (jniPoolProp != null)
            no = jniPoolProp.getNumOfVSNs();

        return no;
    }

    public void setMemberVSNs(int mediaType,
                              String expression) throws SamFSException {

        this.memberVSNs.clear();
        this.mediaType = mediaType;
        this.vsnExp = expression;

        String [] newExp = SamQFSUtil.getStringsFromCommaStream(expression);
        Ctx ctx = null;
        if (model != null)
            ctx = model.getJniContext();

        if (jniPool == null) {

            jniPool = new com.sun.netstorage.samqfs.mgmt.arc.VSNPool(poolName,
                            SamQFSUtil.getMediaTypeString(mediaType), newExp);
            VSNOp.addPool(ctx, jniPool);
            jniPoolProp = getJniPoolProps();

        } else {
            jniPool.setVSNs(newExp);
            VSNOp.modifyPool(ctx, jniPool);
            jniPoolProp = getJniPoolProps();
        }
    }

    private BaseVSNPoolProps getJniPoolProps() throws SamFSException {
        // TODO: this should be a settable property
        return getJniPoolProps(20);
    }

    private BaseVSNPoolProps getJniPoolProps(int max)
        throws SamFSException {

        BaseVSNPoolProps props = null;
        if (model.getServerAPIVersion().compareTo("1.3") < 0)
            props = VSNOp.getPoolProps(model.getJniContext(), poolName,
                                       0, max, Media.VSN_SORT_BY_SLOT, true);
        else // 4.4 or newer server
            props = VSNOp.getPoolPropsByPool(model.getJniContext(), jniPool,
                                      0, max, Media.VSN_SORT_BY_SLOT, false);
        return props;
    }

    /**
     * @since 4.4
     * @return a list of volumes, if this is a pool of disk volumes
     */
    public DiskVolume[] getMemberDiskVolumes(int  start,
                                             int size,
                                             int sortby,
                                             boolean ascending)
        throws SamFSException {

        /* call native method to get disk vol info for this pool */
        DiskVSNPoolProps poolInfo = (DiskVSNPoolProps)
            VSNOp.getPoolPropsByPool(model.getJniContext(),
                            jniPool, start, size, (short) sortby, ascending);

        DiskVol[] vols = poolInfo.getDiskEntries();

        /* build array of DiskVolume to be consumed by ui */
        DiskVolume[] volumes = new DiskVolume[vols.length];
        for (int i = 0; i < volumes.length; i++) {
            volumes[i] = new DiskVolumeImpl(vols[i]);
        }

        return volumes;
    }
}
