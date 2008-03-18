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

// ident	$Id: ArchiveCopyImpl.java,v 1.22 2008/03/17 14:43:48 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.CopyParams;
import com.sun.netstorage.samqfs.mgmt.rec.RecyclerParams;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

public class ArchiveCopyImpl implements ArchiveCopy {
    private ArchivePolicy archPolicy = null;
    private ArchiveVSNMap archVSNMap = null;

    private int copyNumber = -1;
    private CopyParams param = null;

    private boolean rearchive = false;
    private String diskArchVSN = null;
    private String diskArchVSNPath = null;
    private String diskArchVSNHost = null;

    private int resMethod = -1;
    private int archSortMethod = -1;
    private int offlineCopyMethod = -1;

    private int drives = -1;
    private long maxDrives = -1;
    private int maxDrivesUnit = -1;
    private long minDrives = -1;
    private int minDrivesUnit = -1;

    private int joinMethod = -1;
    private int unarchTimeRef = -1;
    private boolean fillVSNs = false;

    private long overflowMinSize = -1;
    private int overflowMinSizeUnit = -1;
    private long archMaxSize = -1;
    private int archMaxSizeUnit = -1;
    private int bufSize = -1;
    private boolean bufLocked = false;

    private long startAge = -1;
    private int startAgeUnit = -1;
    private int startCount = -1;
    private long startSize = -1;
    private int startSizeUnit = -1;

    private long recDataSize = -1;
    private int recDataSizeUnit = -1;
    private int recHWM = -1;
    private boolean ignoreRec = false;
    private String notifyAddr = null;
    private int minGain = -1;
    private int maxVSNCount = -1;

    // TO DO
    // All the setters check for null JNI Copy Param object. This can be
    // removed as the constructor has been rewritten so that the JNI object
    // is never null
    public ArchiveCopyImpl(ArchivePolicy policy,
                           ArchiveVSNMap map,
                           int copyNumber,
                           CopyParams param) {

        String tmp;
        long sec;

        this.archPolicy = policy;
        this.archVSNMap = map;
        this.copyNumber = copyNumber;
        this.param = param;

        if (param != null) {

            this.resMethod = param.getReservationMethod();
            this.archSortMethod = SamQFSUtil.
                convertARSortMethod(param.getArchiveSortMethod());
            this.offlineCopyMethod = SamQFSUtil.
                convertAROfflineCopyMethod(param.getOfflineCopyMethod());

            this.diskArchVSN = param.getDiskArchiveVol();
            this.drives = param.getDrives();

            tmp = param.getMaxDrives();
            if (SamQFSUtil.isValidString(tmp)) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.maxDrives = SamQFSUtil.getLongVal(size);
                this.maxDrivesUnit = SamQFSUtil.getSizeUnitInteger(size);
            }

            tmp = param.getMinDrives();
            if (SamQFSUtil.isValidString(tmp)) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.minDrives = SamQFSUtil.getLongVal(size);
                this.minDrivesUnit = SamQFSUtil.getSizeUnitInteger(size);
            }

            this.joinMethod =
                SamQFSUtil.convertARJoinMethod(param.getJoinMethod());

            if (!param.getUnarchAge())
                this.unarchTimeRef = ArchivePolicy.UNARCH_TIME_REF_ACCESS;
            else
                this.unarchTimeRef =
                    ArchivePolicy.UNARCH_TIME_REF_MODIFICATION;

            this.fillVSNs = param.isFillVSNs();

            tmp = param.getOverflowMinSize();
            if (SamQFSUtil.isValidString(tmp)) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.overflowMinSize = SamQFSUtil.getLongVal(size);
                this.overflowMinSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }

            tmp = param.getArchMax();
            if (SamQFSUtil.isValidString(tmp)) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.archMaxSize = SamQFSUtil.getLongVal(size);
                this.archMaxSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }

            this.bufSize = param.getBufSize();
            if ((this.bufSize < 2) || (this.bufSize > 1024))
                this.bufSize = -1;
            this.bufLocked = param.isBufferLocked();

            sec = param.getStartAge();
            if (sec >= 0) {
                tmp = SamQFSUtil.longToInterval(sec);
                this.startAge = SamQFSUtil.getLongValSecond(tmp);
                this.startAgeUnit = SamQFSUtil.getTimeUnitInteger(tmp);
            }

            this.startCount = param.getStartCount();

            tmp = param.getStartSize();
            if (SamQFSUtil.isValidString(tmp)) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.startSize = SamQFSUtil.getLongVal(size);
                this.startSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }

            RecyclerParams recP = param.getRecyclerParams();
            if (recP != null) {
                if (SamQFSUtil.isValidString(recP.getDatasize())) {
                    String  size =
                        SamQFSUtil.stringToFsize(recP.getDatasize());
                    this.recDataSize = SamQFSUtil.getLongVal(size);
                    this.recDataSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
                }
                this.recHWM = recP.getHWM();
                this.ignoreRec = recP.getIgnore();
                this.notifyAddr = recP.getEmailAddr();
                this.minGain = recP.getMinGain();
                this.maxVSNCount = recP.getVSNCount();
            }

        } else {
            this.param = new CopyParams("Unknown");
        }
    }

    public CopyParams getJniCopyParams() {
        if (param.getName().equals("Unknown")) {
            ArchivePolicy pol = getArchivePolicy();
            if (pol != null) {
                param = new CopyParams(pol.getPolicyName()
                                       + "."
                                       + getCopyNumber(),
                                       param);
            }
        }

        return param;
    }

    public ArchivePolicy getArchivePolicy() {
        return archPolicy;
    }

    public void setArchivePolicy(ArchivePolicy archPolicy) {
        this.archPolicy = archPolicy;
    }

    public int getCopyNumber() {
        return copyNumber;
    }

    public void setCopyNumber(int copyNumber) {
        this.copyNumber = copyNumber;
    }

    /**
     * return the VSN Map settings for this copy as seen by the archiver,
     * putting into consideration the 'allsets' settings. This method uses the
     * following algorithm to determine the effective copy vsn map.
     *
     * 1. If the copy has an explicit vsn setting, use that
     * 2. If not, use the copy number's allsets policy setting
     * 3. If none exists, use the allsets policy's allsets copy
     * 4. Lastly, return an empty map.
     *
     * @deprecated - short-lived
     */
    public ArchiveVSNMap _getEffectiveArchiveVSNMap() throws SamFSException {
        if (archVSNMap != null)
            return archVSNMap;

        // retrieve the parent policy
        SamQFSSystemModel model = getArchivePolicy().getModel();

        // find the allsets policy settings and return those
        ArchivePolicy allsets = model.getSamQFSSystemArchiveManager().
            getArchivePolicy(ArchivePolicy.POLICY_NAME_ALLSETS);

        // get the equivalent all sets copy
        ArchiveCopy matchingAllsetsCopy =
            allsets.getArchiveCopy(getCopyNumber());

        if (matchingAllsetsCopy != null)
            archVSNMap = matchingAllsetsCopy.getArchiveVSNMap();

        if (archVSNMap == null)
            // finally try the allsets copy of the allsets policy
            archVSNMap = allsets.getArchiveCopy(5).getArchiveVSNMap();

        // if still null, create an empty one
        if (archVSNMap == null) {
            archVSNMap = new ArchiveVSNMapImpl(this, null);

            // TODO: Only do this for new Maps ... until
            // a proper clone can be implemented
            // set the copy name to prevent allsets duplicates
            String copyName = getArchivePolicy().getPolicyName()
                .concat(".")
                .concat(Integer.toString(getCopyNumber()));

            archVSNMap.getInternalVSNMap().setCopyName(copyName);
        }

        return archVSNMap;
    }

    public ArchiveVSNMap getArchiveVSNMap() {
        return archVSNMap;
    }

    public void setArchiveVSNMap(ArchiveVSNMap archVSNMap) {
        this.archVSNMap = archVSNMap;
    }

    public boolean isRearchive() {
        return rearchive;
    }

    public void setRearchive(boolean rearchive) {
        this.rearchive = rearchive;
    }

    public String getDiskArchiveVSN() {
        return diskArchVSN;
    }

    public void setDiskArchiveVSN(String vsn) {
        this.diskArchVSN = vsn;
        if (param != null) {
            if (SamQFSUtil.isValidString(vsn))
                param.setDiskArchiveVol(vsn);
            else
                param.resetDiskArchiveVol();
        }
    }

    public String getDiskArchiveVSNPath() {
        return diskArchVSNPath;
    }

    public void setDiskArchiveVSNPath(String path) {
        this.diskArchVSNPath = path;
    }

    public String getDiskArchiveVSNHost() {
        return diskArchVSNHost;
    }

    public void setDiskArchiveVSNHost(String hostname) {
        this.diskArchVSNHost = hostname;
    }

    public int getReservationMethod() {
        return resMethod;
    }

    public void setReservationMethod(int method) {
        this.resMethod = method;
        if (param != null) {
            if (method != -1)
                param.setReservationMethod((short)method);
            else
                param.reserReservationMethod();
        }
    }

    public int getArchiveSortMethod() {
        return archSortMethod;
    }

    public void setArchiveSortMethod(int method) {
        this.archSortMethod = method;
        if (param != null) {
            if (method != -1)
                param.setArchiveSortMethod(SamQFSUtil.
                                           convertARSortMethodJni(method));
            else
                param.resetArchiveSortMethod();
        }
    }

    public int getOfflineCopyMethod() {
        return offlineCopyMethod;
    }

    public void setOfflineCopyMethod(int method) {
        this.offlineCopyMethod = method;
        if (param != null) {
            if (method != -1)
                param.setOfflineCopyMethod(SamQFSUtil.
                                   convertAROfflineCopyMethodJni(method));
            else
                param.resetOfflineCopyMethod();
        }
    }

    public int getDrives() {
        return drives;
    }


    public void setDrives(int drives) {
        this.drives = drives;
        if (param != null) {
            if (drives != -1)
                param.setDrives(drives);
            else
                param.resetDrives();
        }
    }

    public long getMaxDrives() {
        return maxDrives;
    }

    public void setMaxDrives(long size) {
        this.maxDrives = size;
        if (param != null) {
            if (size != -1) {
                if (maxDrivesUnit != -1) {
                    String unit = SamQFSUtil.getUnitString(maxDrivesUnit);
                    param.setMaxDrives(SamQFSUtil.
                        fsizeToString((new Long(maxDrives)).toString() + unit));
                }
            } else {
                param.resetMaxDrives();
            }
        }
    }

    public int getMaxDrivesUnit() {
        return maxDrivesUnit;
    }

    public void setMaxDrivesUnit(int sizeUnit) {
        this.maxDrivesUnit = sizeUnit;
        if (param != null) {
            if (sizeUnit != -1) {
                if (maxDrives != -1) {
                    String unit = SamQFSUtil.getUnitString(maxDrivesUnit);
                    param.setMaxDrives(SamQFSUtil.
                      fsizeToString((new Long(maxDrives)).toString() + unit));
                }
            } else {
                param.resetMaxDrives();
            }
        }

    }


    public long getMinDrives() {

	return minDrives;

    }


    public void setMinDrives(long size) {

	this.minDrives = size;
        if (param != null) {
            if (size != -1) {
                if (minDrivesUnit != -1) {
                    String unit = SamQFSUtil.getUnitString(minDrivesUnit);
                    param.setMinDrives(SamQFSUtil.
                      fsizeToString((new Long(minDrives)).toString() + unit));
                }
            } else {
                param.resetMinDrives();
            }
        }
    }

    public int getMinDrivesUnit() {
        return minDrivesUnit;
    }

    public void setMinDrivesUnit(int sizeUnit) {
        this.minDrivesUnit = sizeUnit;
        if (param != null) {
            if (sizeUnit != -1) {
                if (minDrives != -1) {
                    String unit = SamQFSUtil.getUnitString(minDrivesUnit);
                    param.setMinDrives(SamQFSUtil.
                      fsizeToString((new Long(minDrives)).toString() + unit));
                }
            } else {
                param.resetMinDrives();
            }
        }
    }

    public int getJoinMethod() {
        return joinMethod;
    }

    public void setJoinMethod(int method) {
        this.joinMethod = method;
        if (param != null) {
            if (method != -1) {
                param.setJoinMethod(SamQFSUtil.convertARJoinMethodJni(method));
            } else {
                param.resetJoinMethod();

            }
        }
    }

    public int getUnarchiveTimeReference() {
        return unarchTimeRef;
    }

    public void setUnarchiveTimeReference(int ref) {
        this.unarchTimeRef = ref;
        if (param != null) {
            if (ref == -1)
                param.resetUnarchAge();
            else if (ref == ArchivePolicy.UNARCH_TIME_REF_MODIFICATION)
                param.setUnarchAge(true);
            else if (ref == ArchivePolicy.UNARCH_TIME_REF_ACCESS)
                param.setUnarchAge(false);
        }
    }

    public boolean isFillVSNs() {
        return fillVSNs;
    }

    public void setFillVSNs(boolean fillVSN) {
        this.fillVSNs = fillVSN;
        if (param != null)
            param.setFillVSNs(fillVSN);
    }


    public long getOverflowMinSize() {
        return overflowMinSize;
    }

    public void setOverflowMinSize(long size) {
        this.overflowMinSize = size;
        if (param != null) {
            if (size != -1) {
                if (overflowMinSizeUnit != -1) {
                String unit = SamQFSUtil.getUnitString(overflowMinSizeUnit);
                param.setOverflowMinSize(SamQFSUtil.
                    fsizeToString((new Long(overflowMinSize)).toString() +
                                  unit));
                }
            } else {
                param.resetOverflowMinSize();
            }
        }
    }

    public int getOverflowMinSizeUnit() {
        return overflowMinSizeUnit;
    }

    public void setOverflowMinSizeUnit(int sizeUnit) {
        this.overflowMinSizeUnit = sizeUnit;
        if (param != null) {
            if (sizeUnit != -1) {
                if (overflowMinSize != -1) {
                String unit = SamQFSUtil.getUnitString(overflowMinSizeUnit);
                param.setOverflowMinSize(SamQFSUtil.
                    fsizeToString((new Long(overflowMinSize)).toString() +
                                  unit));
                }
            } else {
                param.resetOverflowMinSize();
            }
        }
    }

    public long getArchiveMaxSize() {
        return archMaxSize;
    }

    public void setArchiveMaxSize(long size) {
        this.archMaxSize = size;
        if (param != null) {
            if (size != -1) {
                if (archMaxSizeUnit != -1) {
                String unit = SamQFSUtil.getUnitString(archMaxSizeUnit);
                param.setArchMax(SamQFSUtil.
                    fsizeToString((new Long(archMaxSize)).toString() + unit));
                }
            } else {
                param.resetArchMax();
            }
        }
    }

    public int getArchiveMaxSizeUnit() {
        return archMaxSizeUnit;
    }

    public void setArchiveMaxSizeUnit(int sizeUnit) {
        this.archMaxSizeUnit = sizeUnit;
        if (param != null) {
            if (sizeUnit != -1) {
                if (archMaxSize != -1) {
                String unit = SamQFSUtil.getUnitString(archMaxSizeUnit);
                param.setArchMax(SamQFSUtil.
                    fsizeToString((new Long(archMaxSize)).toString() + unit));
                }
            } else {
                param.resetArchMax();
            }
        }
    }

    public int getBufferSize() {
        return bufSize;
    }

    public void setBufferSize(int size) {
        this.bufSize = size;
        if (param != null) {
            if (size != -1)
                param.setBufSize(size);
            else
                param.resetBufSize();
        }
    }

    public boolean isBufferLocked() {
        return bufLocked;
    }

    public void setBufferLocked(boolean lock) {
        this.bufLocked = lock;
        if (param != null)
            param.setBufferLocked(lock);
    }

    public long getStartAge() {
        return startAge;
    }

    public void setStartAge(long age) {
        this.startAge = age;
        if (param != null) {
            if (age != -1) {
                if (startAgeUnit != -1)
                    param.setStartAge(SamQFSUtil.
                                      convertToSecond(startAge, startAgeUnit));
            } else
                param.resetStartAge();
        }
    }

    public int getStartAgeUnit() {
        return startAgeUnit;
    }

    public void setStartAgeUnit(int ageUnit) {
        this.startAgeUnit = ageUnit;
        if (param != null) {
            if (ageUnit != -1) {
                if (startAge != -1)
                    param.setStartAge(SamQFSUtil.
                                      convertToSecond(startAge, startAgeUnit));
            } else
                param.resetStartAge();
        }
    }

    public int getStartCount() {
        return startCount;
    }

    public void setStartCount(int count) {
        this.startCount = count;
        if (param != null) {
            if (count != -1) {
                param.setStartCount(count);
            } else {
                param.resetStartCount();
            }
        }
    }

    public long getStartSize() {
        return startSize;
    }

    public void setStartSize(long size) {
        this.startSize = size;
        if (param != null) {
            if (size != -1) {
                if (startSizeUnit != -1) {
                String unit = SamQFSUtil.getUnitString(startSizeUnit);
                param.setStartSize(SamQFSUtil.
                    fsizeToString((new Long(startSize)).toString() + unit));
                }
            } else {
                param.resetStartSize();
            }
        }
    }

    public int getStartSizeUnit() {
        return startSizeUnit;
    }

    public void setStartSizeUnit(int sizeUnit) {
        this.startSizeUnit = sizeUnit;
        if (param != null) {
            if (sizeUnit != -1) {
                if (startSize != -1) {
                String unit = SamQFSUtil.getUnitString(startSizeUnit);
                param.setStartSize(SamQFSUtil.
                    fsizeToString((new Long(startSize)).toString() + unit));
                }
            } else {
                param.resetStartSize();
            }
        }
    }

    public long getRecycleDataSize() {
        return recDataSize;
    }

    public void setRecycleDataSize(long size) {
        this.recDataSize = size;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            if (size != -1) {
                if (recDataSizeUnit != -1) {
                String unit = SamQFSUtil.getUnitString(recDataSizeUnit);
                rec.setDatasize(SamQFSUtil.
                  fsizeToString((new Long(recDataSize)).toString() + unit));
                }
            } else {
                rec.resetDatasize();
            }
            param.setRecyclerParams(rec);
        }
    }

    public int getRecycleDataSizeUnit() {
        return recDataSizeUnit;
    }

    public void setRecycleDataSizeUnit(int sizeUnit) {
        this.recDataSizeUnit = sizeUnit;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            if (sizeUnit != -1) {
                if (recDataSize != -1) {
                String unit = SamQFSUtil.getUnitString(recDataSizeUnit);
                rec.setDatasize(SamQFSUtil.
                  fsizeToString((new Long(recDataSize)).toString() + unit));
                }
            } else {
                rec.resetDatasize();
            }
            param.setRecyclerParams(rec);
        }
    }

    public int getRecycleHWM() {
        return recHWM;
    }

    public void setRecycleHWM(int hwm) {
        this.recHWM = hwm;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            if (hwm != -1)
                rec.setHWM(hwm);
            else
                rec.resetHWM();
            param.setRecyclerParams(rec);
        }
    }

    public boolean isIgnoreRecycle() {
        return ignoreRec;
    }

    public void setIgnoreRecycle(boolean ignore) {
        this.ignoreRec = ignore;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            rec.setIgnore(ignore);
            param.setRecyclerParams(rec);
        }
    }

    public String getNotificationAddress() {
        return notifyAddr;
    }

    public void setNotificationAddress(String addr) {
        this.notifyAddr = addr;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            if (SamQFSUtil.isValidString(addr))
                rec.setEmailAddr(addr);
            else
                rec.resetEmailAddr();
            param.setRecyclerParams(rec);
        }
    }

    public int getMinGain() {
        return minGain;
    }

    public void setMinGain(int gain) {
        this.minGain = gain;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            if (gain != -1)
                rec.setMinGain(gain);
            else
                rec.resetMinGain();
            param.setRecyclerParams(rec);
        }
    }

    public int getMaxVSNCount() {
        return maxVSNCount;
    }

    public void setMaxVSNCount(int count) {
        this.maxVSNCount = count;
        if (param != null) {
            RecyclerParams rec = param.getRecyclerParams();
            if (rec == null) {
                rec = new RecyclerParams();
                param.setRecyclerParams(rec);
            }
            if (count != -1)
                rec.setVSNCount(count);
            else
                rec.resetVSNCount();
            param.setRecyclerParams(rec);
        }
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        if (archPolicy != null)
            buf.append("Archive Policy: ")
                .append(archPolicy.getPolicyName())
                .append("\n")
                .append("Copy Number: ")
                .append(copyNumber)
                .append("\n")
                .append("Rearchive: ")
                .append(rearchive)
                .append("\n")
                .append("DiskArchive VSN: ")
                .append(diskArchVSN)
                .append("\n")
                .append("DiskArchive VSN Path: ")
                .append(diskArchVSNPath)
                .append("\n")
                .append("DiskArchive VSN Host: ")
                .append(diskArchVSNHost)
                .append("\n")
                .append("Reservation Method: ")
                .append(resMethod)
                .append("\n")
                .append("Archive Sort Method: ")
                .append(archSortMethod)
                .append("\n")
                .append("Offline Copy Method: ")
                .append(offlineCopyMethod)
                .append("\n")
                .append("Drives: ")
                .append(drives)
                .append("\n")
                .append("Max Drives: ")
                .append(maxDrives)
                .append("\n")
                .append("Max Drives Unit: ")
                .append(maxDrivesUnit)
                .append("\n")
                .append("Min Drives: ")
                .append(minDrives)
                .append("\n")
                .append("Min Drives Unit: ")
                .append(minDrivesUnit)
                .append("\n")
                .append("Join Method: ")
                .append(joinMethod)
                .append("\n")
                .append("Unarchive Time Reference: ")
                .append(unarchTimeRef)
                .append("\n")
                .append("Fill VSNs: ")
                .append(fillVSNs)
                .append("\n")
                .append("Overflow Min Size: ")
                .append(overflowMinSize)
                .append("\n")
                .append("Overflow Min Size Unit: ")
                .append(overflowMinSizeUnit)
                .append("\n")
                .append("Archive Max Size: ")
                .append(archMaxSize)
                .append("\n")
                .append("Archive Max Size Unit: ")
                .append(archMaxSizeUnit)
                .append("\n")
                .append("Buffer Size: ")
                .append(bufSize)
                .append("\n")
                .append("Buffer Locked: ")
                .append(bufLocked)
                .append("\n")
                .append("Start Age: ")
                .append(startAge)
                .append("\n")
                .append("Start Age Unit: ")
                .append(startAgeUnit)
                .append("\n")
                .append("Start Count: ")
                .append(startCount)
                .append("\n")
                .append("Start Size: ")
                .append(startSize)
                .append("\n")
                .append("Start Size Unit: ")
                .append(startSizeUnit)
                .append("\n")
                .append("Recycle Data Size: ")
                .append(recDataSize)
                .append("\n")
                .append("Recycle Data Size Unit: ")
                .append(recDataSizeUnit)
                .append("\n")
                .append("Recycle HWM: ")
                .append(recHWM)
                .append("\n")
                .append("Ignore Recycle: ")
                .append(ignoreRec)
                .append("\n")
                .append("Notification Address: ")
                .append(notifyAddr)
                .append("\n")
                .append("Min Gain: ")
                .append(minGain)
                .append("\n")
                .append("Max VSN Count: ")
                .append(maxVSNCount)
                .append("\n");

        if (archVSNMap != null)
            buf.append("Archive VSNMap: ")
                .append(archVSNMap.toString())
                .append("\n");

        return buf.toString();
    }
}
