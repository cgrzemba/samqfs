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

// ident	$Id: ArchivePolCriteriaPropImpl.java,v 1.6 2008/12/16 00:12:20 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.DataClassAttributes;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

public class ArchivePolCriteriaPropImpl implements ArchivePolCriteriaProp {
    private ArchivePolCriteria archPolCriteria = null;
    private Criteria criteria = null;
    private boolean global = false;
    private String startingDir = null;
    private long minSize    = -1;
    private int minSizeUnit = -1;
    private long maxSize    = -1;
    private int maxSizeUnit = -1;
    private String namePattern = null;
    private int namePatternType = -1;
    private String owner = null;
    private String group = null;
    private int stageAttribs = -1;
    private int releaseAttribs = -1;
    private long accessAge = -1;
    private int accessAgeUnit = -1;
    private String className = null;
    private String afterDate = null;
    private String description = null;
    private int priority = -1;
    private DataClassAttributes classAttributes;
    private int partialSize = -1;

    // For all but CIS
    public ArchivePolCriteriaPropImpl(ArchivePolCriteria archPolCriteria,
                                      Criteria criteria) {

        String tmp;
        long sec;

        this.archPolCriteria = archPolCriteria;
        this.criteria = criteria;

        if (criteria != null) {
            // set up the properties
            if (criteria.getFilesysName().equals(Criteria.GLOBAL))
                global = true;
            this.startingDir = criteria.getRootDir();
            tmp = criteria.getMinSize();
            if ((SamQFSUtil.isValidString(tmp)) && (!tmp.equals("0"))) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.minSize = SamQFSUtil.getLongVal(size);
                this.minSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }
            tmp = criteria.getMaxSize();
            if ((SamQFSUtil.isValidString(tmp)) && (!tmp.equals("0"))) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.maxSize = SamQFSUtil.getLongVal(size);
                this.maxSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }
            this.namePattern = criteria.getRegExp();
            this.owner = criteria.getUser();
            this.group = criteria.getGroup();

            this.stageAttribs = criteria.getStageAttr();
            this.releaseAttribs = criteria.getReleaseAttrs();

            if ((this.releaseAttribs & Criteria.ATTR_PARTIAL_SIZE) != 0) {
                this.partialSize = criteria.getPartialSize();
            }

            TraceUtil.trace3(
                "Get from JNI: stageAttribs: " + this.stageAttribs);
            TraceUtil.trace3(
                "Get from JNI: releaseAttribs: " + this.releaseAttribs);
            TraceUtil.trace3("Get from JNI: partialSize: " + this.partialSize);

            this.accessAge = criteria.getAccessAge();
            sec = criteria.getAccessAge();
            if (sec >= 0) {
                tmp = SamQFSUtil.longToInterval(sec);
                this.accessAge = SamQFSUtil.getLongValSecond(tmp);
                this.accessAgeUnit = SamQFSUtil.getTimeUnitInteger(tmp);
            }

        } else {
            // set up a "scratch" criteria that can be used as "default"
            this.criteria = new Criteria("unknown", "unknown");
        }
    }

    // For CIS only
    public ArchivePolCriteriaPropImpl(ArchivePolCriteria archPolCriteria,
                                      Criteria criteria,
                                      String className,
                                      String classDescription) {
        String tmp;
        long sec;

        this.archPolCriteria = archPolCriteria;
        this.criteria = criteria;
        this.className = className;
        this.description = classDescription;

        if (criteria != null) {

            if (description == null) {
                this.description = criteria.getDescription();
            }

            // set up the properties
            if (criteria.getFilesysName().equals(Criteria.GLOBAL))
                global = true;
            this.startingDir = criteria.getRootDir();
            tmp = criteria.getMinSize();
            if ((SamQFSUtil.isValidString(tmp)) && (!tmp.equals("0"))) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.minSize = SamQFSUtil.getLongVal(size);
                this.minSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }
            tmp = criteria.getMaxSize();
            if ((SamQFSUtil.isValidString(tmp)) && (!tmp.equals("0"))) {
                String size = SamQFSUtil.stringToFsize(tmp);
                this.maxSize = SamQFSUtil.getLongVal(size);
                this.maxSizeUnit = SamQFSUtil.getSizeUnitInteger(size);
            }
            this.namePattern = criteria.getRegExp();

            // new in 4.6
            this.namePatternType = criteria.getRegExpType();

            this.owner = criteria.getUser();
            this.group = criteria.getGroup();
            this.stageAttribs = criteria.getStageAttr();
            this.releaseAttribs = criteria.getReleaseAttrs();
            this.accessAge = criteria.getAccessAge();
            sec = criteria.getAccessAge();
            if (sec >= 0) {
                tmp = SamQFSUtil.longToInterval(sec);
                this.accessAge = SamQFSUtil.getLongValSecond(tmp);
                this.accessAgeUnit = SamQFSUtil.getTimeUnitInteger(tmp);
            }

            this.afterDate = criteria.getAfterDate();

            // class attributes
            this.classAttributes =
                new DataClassAttributes(criteria.getClassAttrStr());
        } else {
            // set up a "scratch" criteria that can be used as "default"
            this.criteria = new Criteria(className, "unknown", "unknown");
            this.classAttributes = new DataClassAttributes();
        }
    }

    /**
     * Description is added in CIS
     */
    public String getDescription() {
        return this.description;
    }

    public void setDescription(String desc) {
        this.description = desc;
    }

    public int getPriority() {
        priority = criteria.getPriority();
        return priority;
    }

    /**
     * retrieve the class attributes for this class
     *
     * @since CIS
     * @return - DataClassAttributes dca
     */
    public DataClassAttributes getDataClassAttributes() {
        return this.classAttributes;
    }

    public ArchivePolCriteria getArchivePolCriteria() {
        return archPolCriteria;
    }

    public void setArchivePolCriteria(ArchivePolCriteria archPolCriteria) {
        this.archPolCriteria = archPolCriteria;
    }

    public Criteria getJniCriteria() {
        return criteria;
    }

    public boolean isGlobal() {
        return global;
    }

    public String getClassName() {
        if (className != null) {
            return className;
        } else {
            return "";
        }
    }

    public void setClassName(String className) {
        if (SamQFSUtil.isValidString(className)) {
            this.className = className;
        } else {
            this.className = "";
        }
    }

    public String getStartingDir() {
        return startingDir;
    }

    public void setStartingDir(String startingDir) {
        if (SamQFSUtil.isValidString(startingDir)) {
            this.startingDir = startingDir;
            criteria.setRootDir(startingDir);
        } else {
            this.startingDir = new String();
            criteria.resetRootDir();
        }
    }

    public long getMinSize() {
        return minSize;
    }

    public void setMinSize(long minSize) {
        this.minSize = minSize;
        if ((minSize != -1) && (minSizeUnit != -1)) {
            String unit = SamQFSUtil.getUnitString(minSizeUnit);
            criteria.setMinSize(SamQFSUtil.
                                fsizeToString((new Long(minSize)).toString()
                                              + unit));
        } else {
            criteria.resetMinSize();
        }
    }

    public long getMinSizeInBytes() {
        long size = -1;
        if ((minSizeUnit != -1) && (minSize >= 0))
            size = SamQFSUtil.getSizeInBytes(minSize, minSizeUnit);

        return size;
    }

    public int getMinSizeUnit() {
        return minSizeUnit;
    }

    public void setMinSizeUnit(int sizeUnit) {
        this.minSizeUnit = sizeUnit;
        if ((minSize != -1) && (minSizeUnit != -1)) {
            String unit = SamQFSUtil.getUnitString(minSizeUnit);
            criteria.setMinSize(SamQFSUtil.
                                fsizeToString((new Long(minSize)).toString()
                                              + unit));
        } else {
            criteria.resetMinSize();
        }
    }

    public long getMaxSize() {
        return maxSize;
    }

    public void setMaxSize(long maxSize) {
        this.maxSize = maxSize;
        if ((maxSize != -1) && (maxSizeUnit != -1)) {
            String unit = SamQFSUtil.getUnitString(maxSizeUnit);
            criteria.setMaxSize(SamQFSUtil.
                                fsizeToString((new Long(maxSize)).toString()
                                              + unit));
        } else {
            criteria.resetMaxSize();
        }
    }

    public long getMaxSizeInBytes() {
        long size = -1;

        if ((maxSizeUnit != -1) && (maxSize >= 0))
            size = SamQFSUtil.getSizeInBytes(maxSize, maxSizeUnit);

        return size;
    }

    public int getMaxSizeUnit() {
        return maxSizeUnit;
    }

    public void setMaxSizeUnit(int maxSizeUnit) {
        this.maxSizeUnit = maxSizeUnit;
        if ((maxSize != -1) && (maxSizeUnit != -1)) {
            String unit = SamQFSUtil.getUnitString(maxSizeUnit);
            criteria.setMaxSize(SamQFSUtil.
                                fsizeToString((new Long(maxSize)).toString()
                                              + unit));
        } else {
            criteria.resetMaxSize();
        }
    }

    public String getNamePattern() {
        return namePattern;
    }

    public void setNamePattern(String namePattern) {
        if (SamQFSUtil.isValidString(namePattern)) {
            this.namePattern = namePattern;
            criteria.setRegExp(namePattern);
        } else {
            this.namePattern = new String();
            criteria.resetRegExp();
        }
    }

    public int getNamePatternType() {
        return namePatternType;
    }

    public void setNamePatternType(int namePatternType) {
        if (namePatternType >= Criteria.REGEXP &&
            namePatternType <= Criteria.ENDS_WITH &&
            namePatternType != 3) {
            this.namePattern = namePattern;
            criteria.setRegExpType(namePatternType);
        } else {
            this.namePatternType = -1;
        }
    }

    public String getOwner() {
        return owner;
    }

    public void setOwner(String owner) {
        if (SamQFSUtil.isValidString(owner)) {
            this.owner = owner;
            criteria.setUser(owner);
        } else {
            this.owner = new String();
            criteria.resetUser();
        }
    }

    public String getGroup() {
        return group;
    }

    public void setGroup(String group) {
        if (SamQFSUtil.isValidString(group)) {
            this.group = group;
            criteria.setGroup(group);
        } else {
            this.group = new String();
            criteria.resetGroup();
        }
    }

    public int getStageAttributes() {
        return stageAttribs;
    }

    public void setStageAttributes(int attribs) {
        this.stageAttribs = attribs;
        if ((attribs == -1) ||
            (attribs == ArchivePolicy.STAGE_NO_OPTION_SET)) {
            criteria.resetStageAttr();
        } else {
            criteria.setStageAttrs(attribs);
        }
    }

    public void resetStageAttributes() {

        if (stageAttribs != 0) {
            criteria.resetStageAttr();
        }

    }

    public int getReleaseAttributes() {
        return releaseAttribs;
    }

    public void setReleaseAttributes(int attribs, int partialSize) {

        this.releaseAttribs = attribs;
        this.partialSize = partialSize;

        if ((attribs == -1) ||
            (attribs == ArchivePolicy.RELEASE_NO_OPTION_SET)) {
            criteria.resetReleaseAttr();
        } else {
            criteria.setReleaseAttrs(attribs, partialSize);

        }

    }

    public void resetReleaseAttributes() {

        if (releaseAttribs != 0) {
            criteria.resetReleaseAttr();
        }

    }

    public long getAccessAge() {
        return accessAge;
    }

    public void setAccessAge(long accessAge) {
        this.accessAge = accessAge;
        if ((accessAge != -1) && (accessAgeUnit != -1)) {
            // JNI API should have been long
            criteria.setAccessAge((int) SamQFSUtil.
                                  convertToSecond(accessAge, accessAgeUnit));
        } else {
            criteria.resetAccessAge();
        }
    }

    public int getAccessAgeUnit() {
        return accessAgeUnit;
    }

    public void setAccessAgeUnit(int accessAgeUnit) {
        this.accessAgeUnit = accessAgeUnit;
        if ((accessAge != -1) && (accessAgeUnit != -1)) {
            // JNI API should have been long
            criteria.setAccessAge((int) SamQFSUtil.
                                  convertToSecond(accessAge, accessAgeUnit));
        } else {
            criteria.resetAccessAge();
        }
    }

    public String getAfterDate() {
        return afterDate;
    }

    public void setAfterDate(String afterDate) {
        this.afterDate = afterDate;
        criteria.setAfterDate(afterDate);
    }

    public boolean isForDefaultPolicy() {
        if (criteria != null) {
            return (criteria.isForDefaultPolicy());
        } else {
            return false;
        }
    }

    public int getPartialSize() {

        return this.partialSize;
    }

    public void setPartialSize(int partialSize) {

        this.partialSize = partialSize;
        criteria.setPartialSize(partialSize);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        if (archPolCriteria == null) {
            buf.append("Policy criteria is null for this property object.\n");
        }

        buf.append("Starting Dir: ")
            .append(startingDir)
            .append("\n")
            .append("Min Size: ")
            .append(minSize)
            .append("\n")
            .append("Min Size Unit: ")
            .append(minSizeUnit)
            .append("\n")
            .append("Max Size: ")
            .append(maxSize)
            .append("\n")
            .append("Max Size Unit: ")
            .append(maxSizeUnit)
            .append("\n")
            .append("Name Pattern: ")
            .append(namePattern)
            .append("\n")
            .append("Name Pattern Type: ")
            .append(namePatternType)
            .append("\n")
            .append("Owner: ")
            .append(owner)
            .append("\n")
            .append("Group: ")
            .append(group)
            .append("\n")
            .append("Stage Attributes: ")
            .append(stageAttribs)
            .append("\n")
            .append("Release Attributes: ")
            .append(releaseAttribs)
            .append("\n")
            .append("Access Age: ")
            .append(accessAge)
            .append("\n")
            .append("Access Age Unit: ")
            .append(accessAgeUnit)
            .append("\n")
            .append("After Date: ")
            .append(afterDate)
            .append("\n");

        return buf.toString();
    }
}
