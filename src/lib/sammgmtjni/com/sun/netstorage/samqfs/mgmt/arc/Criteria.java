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

// ident	$Id: Criteria.java,v 1.20 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import java.util.Arrays;
import com.sun.netstorage.samqfs.mgmt.arc.Copy;

public class Criteria {

    // these values must match those in pub/mgmt/archive.h

    private static final long	AR_ST_path    = 0x00000001;
    private static final long	AR_ST_minsize = 0x00000002;
    private static final long	AR_ST_maxsize =	0x00000004;
    private static final long	AR_ST_name    = 0x00000008;
    private static final long	AR_ST_user    = 0x00000010;
    private static final long	AR_ST_group   = 0x00000020;
    private static final long	AR_ST_release = 0x00000040;
    private static final long	AR_ST_stage   = 0x00000080;
    private static final long	AR_ST_access  = 0x00000100;
    private static final long	AR_ST_nftv    = 0x00000200;
    private static final long	AR_ST_after   = 0x00000400;
    private static final long	AR_ST_class_name = 0x00000800;
    private static final long	AR_ST_priority = 0x00001000;
    private static final long	AR_ST_default_criteria  = 0x00004000;

    public static final short MAX_COPIES = 4;

    // private fields
    private String dataClassName;
    private String fsysName, /* may be GLOBAL */
        setName;
    private int priority;
    private String rootDir, regExp;
    private int regExpType;
    private String minSize, maxSize; // bytes. converted from a fsize_t
    private String user;
    private String group;
    private char releaseAttr, stageAttr;
    private int accessAge; // seconds
    private Copy copies[];
    private short numCopies; /* the number of non-NULL elements in copies[] */
    private byte key[];
    private long chgFlags;
    private boolean nftv;
    private String after;
    private String description;
    private String classAttrs;
    private int classID = -1;

    /**
     * private constructor
     */
    private Criteria(String dataClassName, String description,
        int priority, String fsysName,
	String setName, String rootDir, String regExp, int regExpType,
	String minSize, String maxSize,
        String user, String group, char releaseAttr, char stageAttr,
        int accessAge, Copy[] copies, short numCopies, byte key[],
        boolean nftv, String after, long initialFlags, int classID,
        String classAttrs) {
	    this.dataClassName = dataClassName;
            this.description = description;
	    this.priority = priority;
            this.fsysName = fsysName;
            this.setName = setName;
            this.rootDir = rootDir;
            this.regExp = regExp;
	    this.regExpType = regExpType;
            this.minSize = minSize;
            this.maxSize = maxSize;
            this.user = user;
            this.group = group;
            this.releaseAttr = releaseAttr;
            this.stageAttr = stageAttr;
            this.accessAge = accessAge;
            this.copies = copies;
            this.numCopies = numCopies;
            this.key = key;
            this.nftv = nftv;
	    this.after = after;
            this.chgFlags = initialFlags;
	    this.classID = classID;
	    this.classAttrs = classAttrs;
    }

    // public constructors

    /**
     *  create a brand-new Criteria
     */
    public Criteria(String fsysName, String setName) {
            this.fsysName = fsysName;
            this.setName = setName;
            this.chgFlags = 0;
    }

    /**
     *  create a brand-new Criteria with a class name
     */
    public Criteria(String dataClassName, String fsysName, String setName) {
	    this.dataClassName = dataClassName;
            this.fsysName = fsysName;
            this.setName = setName;
            this.chgFlags = 0;
    }


    /**
     *  create a brand-new Criteria with a class name and description. Once
     *  this constructor gets used the constructor that includes a class name
     *  but not a description should be removed.
     */
    public Criteria(String dataClassName, String description,
		    String fsysName, String setName) {
            this.dataClassName = dataClassName;
            this.description = description;
            this.fsysName = fsysName;
            this.setName = setName;
            this.chgFlags = 0;
    }

    /**
     *  create a Criteria based on an existing one
     */
    public Criteria(String fsName, Criteria c) {
        this(c.dataClassName, c.description, c.priority, fsName, c.setName,
             c.rootDir, c.regExp, c.regExpType, c.minSize, c.maxSize, c.user,
             c.group, c.releaseAttr, c.stageAttr, c.accessAge, c.copies,
             c.numCopies, c.key, c.nftv, c.after, c.chgFlags, c.classID,
             c.classAttrs);
    }

    // valid values for regExpType
    public static final int REGEXP = 0;
    public static final int FILE_NAME_CONTAINS = 1;
    public static final int PATH_CONTAINS = 2;
    public static final int ENDS_WITH = 4;

    // release attributes. Must match the definitions in pub/mgmt/archive.h
    public static final char NEVER_RELEASE = 'n';
    public static final char PARTIAL_RELEASE = 'p';
    public static final char ALWAYS_RELEASE = 'a';
    public static final char SET_DEFAULT_RELEASE = 'd';
    public static final char RELEASE_NOT_DEFINED = '0';

    // stage attributes. Must match the definitions in pub/mgmt/archive.h
    public static final char NEVER_STAGE = 'n';
    public static final char ASSOCIATIVE_STAGE = 'a';
    public static final char SET_DEFAULT_STAGE = 'd';
    public static final char STAGE_NOT_DEFINED = '0';

    /**
     * fsysName may have this special value
     * which must match the one in pub/mgmt/types.h
     */
    public static final String GLOBAL = "global properties";

    /**
     * compare this criteria with the one passed as an argument and
     * return true if they contain the same information.
     * note: fs name is not included in the comparison.
     */
    public boolean sameAs(Criteria c2) {
        return (Arrays.equals(this.key, c2.key));
    }

    // get, set, reset methods
    //
    // WARNING: if a criteria is changed (set/reset), it is impossible to
    // distinguish it from other criteria using the method above, until the
    // criteria is re-read from SAM-FS/QFS

    public String getDataClassName() { return dataClassName; }
    public String getDescription() { return description; }
    public void setDescription(String description) {
        this.description = description;
    }

    public int getPriority() { return priority; }


    /*
     * classAttrs is a string of key=value pairs that describe how
     * the data class will be handled by Intellistore's archiver tasklets.
     * If no attributes are set or the Criteria object comes from a system
     * that is not an intellistore this will return null.
     *
     * The supported keys are:
     * classid=<int>
     * autoworm=<Y|N>
     * absolute_expiration_time=<date> | relative_expiration_time=<duration>
     * autodelete=<Y|N>
     * dedup= <Y|N>
     * bitbybit=<Y|N>
     * periodicaudit=<enum none/disk_only/all>
     * auditperiod=<period>
     * log_data_audti=<Y|N>
     * log_deduplication=<Y|N>
     * log_autoworm=<Y|N>
     * log_autodeletion=<Y|N>
     *
     * <date> format is: YYYYMMDDhhmm
     * <duration> and <period> are an integer value, followed by a unit among:
     * "s" (seconds)
     * "m" (minutes, 60s)
     * "h" (hours, 3 600s)
     * "d" (days, 86 400s)
     * "w" (weeks, 604 800s)
     * "y" (years, 31 536 000s)
     *
     * Only one of absolute_expiration or relative expiration is allowed
     * at a time.
     *
     * classid must be present and equal to the classid returned by
     * getClassID. For a data class that has not yet been created in
     * the backend the class id will be -1.
     */
    public String getClassAttrStr() {
	return classAttrs;
    }
    public void setClassAttrStr(String classAttrs) {
	this.classAttrs = classAttrs;
    }

    public int getClassID() { return classID; }

    public String getFilesysName() { return fsysName; }

    public String getSetName() { return setName; }
    public void setSetName(String setName) { this.setName = setName; }

    public String getRootDir() { return rootDir; }
    public void setRootDir(String rootDir) {
        this.rootDir = rootDir;
        chgFlags |= AR_ST_path;
    }
    public void resetRootDir() {
        chgFlags &= ~AR_ST_path;
    }

    public String getRegExp() { return regExp; }
    public void setRegExp(String regExp) {
        this.regExp = regExp;
        chgFlags |= AR_ST_name;
    }
    public void resetRegExp() {
        chgFlags &= ~AR_ST_name;
    }

    public int getRegExpType() { return regExpType; }
    public void setRegExpType(int regExpType) {
        this.regExpType = regExpType;
    }

    public String getMinSize() { return minSize; } /* bytes */
    public void setMinSize(String minSize) {
        this.minSize = minSize;
        chgFlags |= AR_ST_minsize;
    }
    public void resetMinSize() {
        chgFlags &= ~AR_ST_minsize;
    }

    public String getMaxSize() { return maxSize; } /* bytes */
    public void setMaxSize(String maxSize) {
        this.maxSize = maxSize;
        chgFlags |= AR_ST_maxsize;
    }
    public void resetMaxSize() {
        chgFlags &= ~AR_ST_maxsize;
    }

    public String getUser() { return user; }
    public void setUser(String user) {
        this.user = user;
        chgFlags |= AR_ST_user;
    }
    public void resetUser() {
        chgFlags &= ~AR_ST_user;
    }

    public String getGroup() { return group; }
    public void setGroup(String group) {
        this.group = group;
        chgFlags |= AR_ST_group;
    }
    public void resetGroup() {
        chgFlags &= ~AR_ST_group;
    }

    public char getReleaseAttr() { return releaseAttr; }
    public void setReleaseAttr(char releaseAttr) {
        this.releaseAttr = releaseAttr;
        chgFlags |= AR_ST_release;
    }
    public void resetReleaseAttr() {
        chgFlags &= ~AR_ST_release;
    }

    public char getStageAttr() { return stageAttr; }
    public void setStageAttr(char stageAttr) {
        this.stageAttr = stageAttr;
        chgFlags |= AR_ST_stage;
    }
    public void resetStageAttr() {
        chgFlags &= ~AR_ST_stage;
    }

    public int getAccessAge() { return accessAge; }
    public void setAccessAge(int accessAge) {
        this.accessAge = accessAge;
        chgFlags |= AR_ST_access;
    }
    public void resetAccessAge() {
        chgFlags &= ~AR_ST_access;
    }

    public String getAfterDate() { return after; }
    public void setAfterDate(String after) {
        this.after = after;
        chgFlags |= AR_ST_after;
    }
    public void resetAfterDate() {
        chgFlags &= ~AR_ST_after;
    }

    public Copy[] getCopies() { return copies; }
    public void setCopies(Copy[] copies) {
        this.copies = copies;
    }

    public short getNumCopies() { return numCopies; }
    public boolean isForDefaultPolicy() {
	return ((chgFlags & AR_ST_default_criteria) == AR_ST_default_criteria);
    }

    public String toString() {
        String s = setName + ": " + fsysName + "," + rootDir + "," +
            regExp + "," +
            minSize + "," + maxSize + "," + user + "," + group + "," +
            ((releaseAttr == '\0') ? ' ' : releaseAttr) + "," +
	    ((stageAttr == '\0') ? ' ' : stageAttr) + "," + accessAge + "s " +
            "[cf:Ox" + Long.toHexString(chgFlags) + "] " +
            numCopies + " copies" + ",nftv:" + (nftv ? "T" : "F") +
	    ",after:" + after + "\n";
        if (copies != null)
            for (int i = 0; i < copies.length; i++)
                if (copies[i] != null)
                    s += copies[i] + "\n";
                else s += "null\n";
        return s;
    }
}
