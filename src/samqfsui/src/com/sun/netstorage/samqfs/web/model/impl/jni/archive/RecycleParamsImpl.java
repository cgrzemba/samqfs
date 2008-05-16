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

// ident	$Id: RecycleParamsImpl.java,v 1.5 2008/05/16 18:39:01 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.LibDev;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.rec.LibRecParams;
import com.sun.netstorage.samqfs.mgmt.rec.Recycler;
import com.sun.netstorage.samqfs.web.model.archive.RecycleParams;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.impl.jni.media.LibraryImpl;
import com.sun.netstorage.samqfs.web.model.media.Library;

public class RecycleParamsImpl implements RecycleParams {

    private SamQFSSystemModelImpl model = null;
    private LibRecParams jniPrm = null;
    private String libPath = null;
    private int hwm = -1;
    private int minGain = -1;
    private int vsnLimit = -1;
    private long sizeLimit = -1;
    private int sizeUnit = -1;
    private boolean perform = false;

    public RecycleParamsImpl() {
    }

    public RecycleParamsImpl(Library library,
                             int hwm,
                             int minGain,
                             int vsnLimit,
                             long sizeLimit,
                             int sizeUnit,
                             boolean perform) {
        this.libPath = library.getDevicePath();
        this.hwm = hwm;
        this.minGain = minGain;
        this.vsnLimit = vsnLimit;
        this.sizeLimit = sizeLimit;
        this.sizeUnit = sizeUnit;
        this.perform = perform;
    }

    public RecycleParamsImpl(SamQFSSystemModelImpl model,
                             LibRecParams jniPrm) {
        this.model = model;
        this.jniPrm = jniPrm;
        setup();
    }

    public Library getLibrary() throws SamFSException {

        Library lib = null;
        if ((model != null) && (SamQFSUtil.isValidString(libPath))) {
            LibDev jni = Media.getLibraryByPath(model.getJniContext(),
                                                libPath);
            if (jni != null)
                lib = new LibraryImpl(model, jni, false);
        }

        return lib;
    }

    public String getLibraryName() {
        return libPath;
    }

    public int getHWM() throws SamFSException {
        return hwm;
    }

    public void setHWM(int hwm) throws SamFSException {
        this.hwm = hwm;
        if (jniPrm != null) {
            if (hwm != -1)
                jniPrm.setHWM(hwm);
            else
                jniPrm.resetHWM();
        }
    }

    public int getMinGain() throws SamFSException {
        return minGain;
    }

    public void setMinGain(int gain) throws SamFSException {
        this.minGain = gain;
        if (jniPrm != null) {
            if (gain != -1)
                jniPrm.setMinGain(gain);
            else
                jniPrm.resetMinGain();
        }
    }

    public int getVSNLimit() throws SamFSException {
        return vsnLimit;
    }

    public void setVSNLimit(int limit) throws SamFSException {
        this.vsnLimit = limit;
        if (jniPrm != null) {
            if (limit != -1)
                jniPrm.setVSNCount(limit);
            else
                jniPrm.resetVSNCount();
        }
    }

    public long getSizeLimit() throws SamFSException {
        return sizeLimit;
    }

    public void setSizeLimit(long limit) throws SamFSException {
        this.sizeLimit = limit;
        if (jniPrm != null) {
            if (limit != -1) {
                if (sizeUnit != -1) {
                    String unit = SamQFSUtil.getUnitString(sizeUnit);
                    jniPrm.setDatasize(SamQFSUtil.
                          fsizeToString((new Long(limit)).toString() + unit));
                }
            } else {
                jniPrm.resetDatasize();
            }
        }
    }

    public int getSizeUnit() throws SamFSException {
        return sizeUnit;
    }

    public void setSizeUnit(int unit) throws SamFSException {
        this.sizeUnit = unit;
        if (jniPrm != null) {
            if (unit != -1) {
                if (sizeLimit != -1) {
                    String ut = SamQFSUtil.getUnitString(sizeUnit);
                    jniPrm.setDatasize(SamQFSUtil.
                        fsizeToString((new Long(sizeLimit)).toString() + ut));
                }
            } else {
                jniPrm.resetDatasize();
            }
        }
    }

    public boolean isPerform() throws SamFSException {
        return perform;
    }

    public void setPerform(boolean perform) throws SamFSException {
        this.perform = perform;
        if (jniPrm != null)
            jniPrm.setIgnore(perform);
    }

    public void changeParams() throws SamFSException {
        if (jniPrm != null)
            Recycler.setLibRecParams(model.getJniContext(), jniPrm);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();
        buf.append("Library: ")
            .append(libPath)
            .append("\n")
            .append("HWM: ")
            .append(hwm)
            .append("\n")
            .append("Min Gain: ")
            .append(minGain)
            .append("\n")
            .append("VSN Limit: ")
            .append(vsnLimit)
            .append("\n")
            .append("Size Limit: ")
            .append(sizeLimit)
            .append("\n")
            .append("Perform: ")
            .append(perform)
            .append("\n");

        return buf.toString();
    }

    private void setup() {
        if (jniPrm != null) {

            libPath = jniPrm.getPath();
            hwm = jniPrm.getHWM();
            minGain = jniPrm.getMinGain();
            vsnLimit = jniPrm.getVSNCount();
            String dataSize = jniPrm.getDatasize();
            if (SamQFSUtil.isValidString(dataSize)) {
                String  size = SamQFSUtil.stringToFsize(dataSize);
                sizeLimit = SamQFSUtil.getLongVal(size);
                sizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }
            perform = jniPrm.getIgnore();
        }
    }
}
