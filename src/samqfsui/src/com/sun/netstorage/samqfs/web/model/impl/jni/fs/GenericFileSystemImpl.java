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

// ident $Id: GenericFileSystemImpl.java,v 1.16 2008/03/17 14:43:49 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.fs;

import java.util.Properties;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemFSManagerImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.DiskCacheImpl;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.NFSOptions;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.fs.FS;

/**
 * basic information about a generic Solaris filesystem
 */
public class GenericFileSystemImpl implements GenericFileSystem {

    protected String hostName = new String();
    protected String name, typeName;
    protected int state = -1;
    protected String mountPoint;
    protected long capacity, avail;
    protected int consumed = 0;
    protected String nfs = "no";
    protected boolean nfsShared = false;
    protected boolean ha = false;
    protected static final GenericFileSystem[] emptyGenFSArray
	= new GenericFileSystem[0];

    // valid keys must match those defined in filesystem.h
    final String KEY_NAME = "name";
    final String KEY_MNTPT = "mountpt";
    final String KEY_TYPE = "type";
    final String KEY_STATE = "state";
    final String KEY_CAPACITY = "capacity"; // kb
    final String KEY_AVAILSPACE = "availspace"; // kb
    final String KEY_NFS = "nfs"; // either missing or "yes"


    protected GenericFileSystemImpl() {};
    public GenericFileSystemImpl(String hostName,
                                 String name, String typeName,
                                 int state, String mountPoint,
                                 long capacity, long avail) {
        this.hostName = hostName;
        this.name = name;
        this.typeName = typeName;
        this.state = state;
        this.mountPoint = mountPoint;
        this.capacity = capacity;
        this.avail    = avail;
        if (capacity != 0)
            this.consumed = (int)((capacity - avail) * 100 / capacity);
    }

    public GenericFileSystemImpl(String hostName, Properties props)
        throws SamFSException {
        this.hostName = hostName;

        name = props.getProperty(KEY_NAME);
        mountPoint = props.getProperty(KEY_MNTPT);
        typeName = props.getProperty(KEY_TYPE);

        state = GenericFileSystem.UNMOUNTED;
        String stateStr = props.getProperty(KEY_STATE);
        if (null != stateStr) {
            if (stateStr.equals("mounted"))
                state = GenericFileSystem.MOUNTED;
        }
        capacity =
            ConversionUtil.strToLongVal(props.getProperty(KEY_CAPACITY));
        avail = ConversionUtil.strToLongVal(props.getProperty(KEY_AVAILSPACE));
        if (capacity != 0)
            this.consumed = (int)((capacity - avail) * 100 / capacity);

        nfsShared = "yes".equals(props.getProperty(KEY_NFS));
        setHA(DiskCacheImpl.isHADevice(name));
    }
    public GenericFileSystemImpl(String hostName, String propsStr)
        throws SamFSException {
        this(hostName, ConversionUtil.strToProps(propsStr));
    }


    // getters

    public String getName() { return name; }
    public int getFSTypeByProduct() { return FS_NONSAMQ; }
    public String getFSTypeName() { return typeName; }

    public boolean hasNFSShares() { return nfsShared; }

    public int getState() { return state; }
    public void setState(int state) { this.state = state; }

    public String getMountPoint() { return mountPoint; }

    public boolean isHA() { return ha; }
    protected void setHA(boolean ha) { this.ha = ha; }

    public GenericFileSystem[] getHAFSInstances()
	throws SamFSMultiHostException {
	return emptyGenFSArray;
    }

    public long getCapacity() { return capacity; }
    public long getAvailableSpace() { return avail; }
    public int getConsumedSpacePercentage() { return consumed; }

    public String getHostName() { return hostName; }


    // helper methods

    protected SamQFSSystemModel getSysModel() throws SamFSException {
        SamQFSAppModel app = SamQFSFactory.getSamQFSAppModel();
        return app.getSamQFSSystemModel(hostName);
    }


    // fs operations

    protected Ctx getCtx() throws SamFSException {
        return ((SamQFSSystemModelImpl)getSysModel()).getJniContext();
    }

    public void mount() throws SamFSException {
        FS.mountByType(getCtx(), name, typeName);
        state = MOUNTED;
        ((SamQFSSystemFSManagerImpl) getSysModel().getSamQFSSystemFSManager()).
            invalidateCachedFS(name);
    }
    public void unmount() throws SamFSException {
        FS.umount(getCtx(), name); // do not force
        state = UNMOUNTED;
        mountPoint = "";
        avail = -1;
        consumed = -1;
        nfsShared = false;
    }

    public NFSOptions[] getNFSOptions() throws SamFSException {
        NFSOptions[] opts = null;

        if (mountPoint == null)
            return new NFSOptions[0];
        String[] optsStrs =
            FS.getNFSOptions(getCtx(), mountPoint);
        if (optsStrs == null)
            return new NFSOptions[0];
        else
            opts = new NFSOptions[optsStrs.length];

        for (int i = 0; i < optsStrs.length; i++)
            opts[i] = new NFSOptions(optsStrs[i]);

        return opts;
    }

    public void setNFSOptions(NFSOptions opts) throws SamFSException {
        if (mountPoint != null)
            FS.setNFSOptions(getCtx(), mountPoint, opts.toString());
        ((SamQFSSystemFSManagerImpl) getSysModel().getSamQFSSystemFSManager()).
            invalidateCachedFS(name);
    }


    public String toString() {
        String E = "=", C = ",";
        String s = KEY_NAME + E + name + C +
                   KEY_TYPE + E + typeName + C +
                   KEY_MNTPT + E + mountPoint + C +
                   KEY_STATE + E + ((state == GenericFileSystem.MOUNTED) ?
                                    "mounted" : "unmounted") + C +
                   KEY_CAPACITY + E + capacity + C +
                   KEY_AVAILSPACE + E + avail + C +
                   KEY_NFS + E + nfs;
        return s;
    }
}
