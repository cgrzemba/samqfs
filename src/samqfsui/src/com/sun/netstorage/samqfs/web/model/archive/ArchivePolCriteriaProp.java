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

// ident	$Id: ArchivePolCriteriaProp.java,v 1.3 2008/03/17 14:43:44 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

/*
 * This interface maps to the ar_set_criteria structure in archive.h
 */
public interface ArchivePolCriteriaProp {

    // Added in CIS & 4.6
    public String getClassName();
    public void setClassName(String className);
    public String getDescription();
    public void setDescription(String description);
    public int getPriority();

    /**
     * retrieve the class attributes for this class
     *
     * @since CIS & 4.6
     * @return - DataClassAttributes dsa
     */
    public DataClassAttributes getDataClassAttributes();

    public ArchivePolCriteria getArchivePolCriteria();

    public void setArchivePolCriteria(ArchivePolCriteria criteria);

    public boolean isGlobal();

    public String getStartingDir();

    public void setStartingDir(String startingDir);

    public long getMinSizeInBytes();

    public long getMinSize();

    public void setMinSize(long size);

    public int getMinSizeUnit();

    public void setMinSizeUnit(int sizeUnit);

    public long getMaxSizeInBytes();

    public long getMaxSize();

    public void setMaxSize(long size);

    public int getMaxSizeUnit();

    public void setMaxSizeUnit(int sizeUnit);

    public String getNamePattern();

    public void setNamePattern(String pattern);

    public int getNamePatternType();

    public void setNamePatternType(int type);

    public String getOwner();

    public void setOwner(String owner);

    public String getGroup();

    public void setGroup(String group);

    public int getStageAttributes();

    public void setStageAttributes(int attribs);

    public int getReleaseAttributes();

    public void setReleaseAttributes(int attribs);

    public long getAccessAge();

    public void setAccessAge(long age);

    public int getAccessAgeUnit();

    public void setAccessAgeUnit(int unit);

    public String getAfterDate();

    public void setAfterDate(String afterDate);

    public boolean isForDefaultPolicy();
}
