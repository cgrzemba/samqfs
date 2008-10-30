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

// ident	$Id: Criteria.java,v 1.21 2008/10/30 14:42:29 pg125177 Exp $

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
    private static final long	AR_ST_default_criteria  = 0x00004000;

    public static final short MAX_COPIES = 4;

    // private fields
    private String fsysName, /* may be GLOBAL */
        setName;
    private String rootDir, regExp;
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
    private int attrFlags;
    private int partialSize;

    /**
     * private constructor
     */
    private Criteria(String fsysName,
	String setName, String rootDir, String regExp,
	String minSize, String maxSize,
        String user, String group, char releaseAttr, char stageAttr,
        int accessAge, Copy[] copies, short numCopies, byte key[],
        boolean nftv, String after, long initialFlags, int attrFlags,
	int partialSize) {
            this.fsysName = fsysName;
            this.setName = setName;
            this.rootDir = rootDir;
            this.regExp = regExp;
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
	    this.setStageAttrs(attrFlags & STAGE_ATTR_SET);
	    this.setReleaseAttrs(attrFlags & RELEASE_ATTR_SET, partialSize);
            this.chgFlags = initialFlags;
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
     *  create a brand-new Criteria. Method ignores the class name.
     */
    public Criteria(String dataClassName, String fsysName, String setName) {
	this(fsysName, setName);
    }


    /**
     *  create a Criteria based on an existing one
     */
    public Criteria(String fsName, Criteria c) {
        this(fsName, c.setName, c.rootDir, c.regExp, c.minSize,
		c.maxSize, c.user, c.group, c.releaseAttr, c.stageAttr,
		c.accessAge, c.copies, c.numCopies, c.key, c.nftv, c.after,
		c.chgFlags, c.attrFlags, c.partialSize);
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
    public static final char PARTIAL_RELEASE_SIZE = 's';
    public static final char RELEASE_NOT_DEFINED = '0';

    // stage attributes. Must match the definitions in pub/mgmt/archive.h
    public static final char NEVER_STAGE = 'n';
    public static final char ASSOCIATIVE_STAGE = 'a';
    public static final char SET_DEFAULT_STAGE = 'd';
    public static final char STAGE_NOT_DEFINED = '0';



    /* Flag values for use in the attrFlags field */
    public static final int ATTR_RESET_RELEASE_DEFAULT	= 0x00000001;
    public static final int ATTR_RELEASE_NEVER		= 0x00000002;
    public static final int ATTR_RELEASE_PARTIAL	= 0x00000004;
    public static final int ATTR_PARTIAL_SIZE		= 0x00000008;
    public static final int ATTR_RELEASE_ALWAYS		= 0x00000010;
    public static final int RELEASE_ATTR_SET		= 0x0000001f;

    public static final int ATTR_RESET_STAGE_DEFAULT   	= 0x00000020;
    public static final int ATTR_STAGE_NEVER		= 0x00000040;
    public static final int ATTR_STAGE_ASSOCIATIVE	= 0x00000080;
    public static final int STAGE_ATTR_SET		= 0x000000E0;


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

    /* Intellistore methods that are still referenced from the logic tier. */
    public String getDataClassName() { return null; }
    public String getDescription() { return null; }
    public void setDescription(String description) {; }
    public int getRegExpType() { return 0; }
    public void setRegExpType(int regExpType) {; }
    public int getPriority() { return 0; }
    public String getClassAttrStr() { return null; }
    public void setClassAttrStr(String classAttrs) {; }
    public int getClassID() { return 0; }


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

    /**
     * Partial size is only set if the attrFlags also has ATTR_PARTIAL_SIZE
     * set.
     */
    public int getPartialSize() {
	if ((attrFlags & ATTR_PARTIAL_SIZE) != 0) {
	    return partialSize;
	} else {
	    return 0;
	}
    }

    /**
     * Partial size is only set if the attrFlags also has ATTR_PARTIAL_SIZE
     * set. This function sets both the flag and the newPartialSize passed in.
     */
    public void setPartialSize(int newPartialSize) {
	attrFlags |= ATTR_PARTIAL_SIZE;
	partialSize = newPartialSize;
    }

    /*
     * Returns the attrFlags. Check with the ATTR_xxxx flags to
     * see what release attributes are set.
     */
    public int getReleaseAttrs() {
	return (attrFlags & RELEASE_ATTR_SET);
    }
    /*
     * getReleaseAttr and setReleaseAttr should not be used. Instead
     * getReleaseAttrs and setReleaseAttrs should be called.
     */
    public char getReleaseAttr() { return releaseAttr; }
    public void setReleaseAttr(char attr) {

	int newAttrFlags = 0;
	if (attr == SET_DEFAULT_RELEASE) {
	    newAttrFlags |= ATTR_RESET_RELEASE_DEFAULT;
	}

	if (attr == NEVER_RELEASE) {
	    newAttrFlags |= ATTR_RELEASE_NEVER;
	}

	if (attr == PARTIAL_RELEASE) {
	    newAttrFlags |= ATTR_RELEASE_PARTIAL;
	}

	if (attr == ALWAYS_RELEASE) {
	    newAttrFlags |= ATTR_RELEASE_ALWAYS;
	}

	/*
	 * Note that this function does not support setting
	 * partial size because it must match the old method
	 * signature.
	 */

	setReleaseAttrs(newAttrFlags, 0);
    }



    /**
     * set the release attributes to match the passed in
     * newAttrFlags. This overwrites all existing attributes
     * release attributes.
     */
    public void setReleaseAttrs(int newAttrFlags, int newPartialSize) {

	// clear the old release flags while preserving other flags
        this.attrFlags &= ~RELEASE_ATTR_SET;

	// Set the new release flags
	this.attrFlags |= newAttrFlags;

	/*
	 * For the purpose of interacting with un-patched servers the
	 * old way needs to be set aswell. However it only supports
	 * a single attribute. So track the number set. If more than one
	 * set the attribute to Z which will result in an error.
	 */
	int release_attr_set = 0;

	if ((attrFlags & ATTR_RESET_RELEASE_DEFAULT) != 0) {
	    releaseAttr = SET_DEFAULT_RELEASE;
	    release_attr_set++;
	}
	if ((attrFlags & ATTR_RELEASE_NEVER) != 0) {
	    releaseAttr = NEVER_RELEASE;
	    release_attr_set++;
	}
	if ((attrFlags & ATTR_RELEASE_PARTIAL) != 0) {
	    releaseAttr = PARTIAL_RELEASE;
	    release_attr_set++;
	}
	if ((attrFlags & ATTR_PARTIAL_SIZE) != 0) {
	    releaseAttr = PARTIAL_RELEASE_SIZE;
	    partialSize = newPartialSize;
	    release_attr_set++;
	}
	if ((attrFlags & ATTR_RELEASE_ALWAYS) != 0) {
	    releaseAttr = ALWAYS_RELEASE;
	    release_attr_set++;
	}

	if (release_attr_set != 0) {
	    chgFlags |= AR_ST_release;
	}

	/*
	 * If more than one release attribute set put an Z in release
	 * This will prevent overwriting the configuration on an unpatched
	 * server that does not support multiple attributes.
	 */
	if (release_attr_set > 1) {
	    releaseAttr = 'Z';
	}
    }
    public void resetReleaseAttr() {
        chgFlags &= ~AR_ST_release;
        attrFlags &= ~RELEASE_ATTR_SET;
	releaseAttr = '\0';
    }

    public int getStageAttrs() {
	return (attrFlags & STAGE_ATTR_SET);
    }
    /*
     * getStageAttr and setStageAttr should not be used. Instead
     * getStageAttrs and setStageAttrs should be called.
     */
    public char getStageAttr() { return stageAttr; }
    public void setStageAttr(char attr) {
	int newAttrFlags = 0;
	if (attr == SET_DEFAULT_STAGE) {
	    newAttrFlags |= ATTR_RESET_STAGE_DEFAULT;
	}
	if (attr == NEVER_STAGE) {
	    newAttrFlags |= ATTR_STAGE_NEVER;
	}
	if (attr == ASSOCIATIVE_STAGE) {
	    newAttrFlags |= ATTR_STAGE_ASSOCIATIVE;
	}

	setStageAttrs(newAttrFlags);
    }

    public void setStageAttrs(int newAttrFlags) {
	// clear the old stage flags while preserving other flags
        this.attrFlags &= ~STAGE_ATTR_SET;

	// Add in the new stage flags
	this.attrFlags |= newAttrFlags;

	/*
	 * For the purpose of interacting with un-patched servers the
	 * old way needs to be set aswell.
	 */
	int stage_attr_set = 0;

	if ((attrFlags & ATTR_RESET_STAGE_DEFAULT) != 0) {
	    stageAttr = SET_DEFAULT_STAGE;
	    stage_attr_set++;
	}
	if ((attrFlags & ATTR_STAGE_NEVER) != 0) {
	    stageAttr = NEVER_STAGE;
	    stage_attr_set++;
	}
	if ((attrFlags & ATTR_STAGE_ASSOCIATIVE) != 0) {
	    stageAttr = ASSOCIATIVE_STAGE;
	    stage_attr_set++;
	}

	if (stage_attr_set != 0) {
	    chgFlags |= AR_ST_stage;
	}

	if (stage_attr_set > 1) {
	    stageAttr = 'Z';
	}
    }
    public void resetStageAttr() {
	attrFlags &= ~STAGE_ATTR_SET;
        chgFlags &= ~AR_ST_stage;
	stageAttr = '\0';
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
