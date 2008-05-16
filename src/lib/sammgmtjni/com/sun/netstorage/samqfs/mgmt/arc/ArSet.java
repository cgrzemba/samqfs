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

// ident	$Id: ArSet.java,v 1.11 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import java.util.Arrays;
import com.sun.netstorage.samqfs.mgmt.arc.*;

public class ArSet {

    // these values must match those in pub/mgmt/archive_set.h

    public static final short AR_SET_TYPE_DEFAULT = 0;
    public static final short AR_SET_TYPE_GENERAL = 1;
    public static final short AR_SET_TYPE_NO_ARCHIVE = 2;
    public static final short AR_SET_TYPE_ALLSETS_PSEUDO = 3;
    public static final short AR_SET_TYPE_UNASSIGNED = 4;
    public static final short AR_SET_TYPE_EXPLICIT_DEFAULT = 5;

    public static final short MAX_COPIES = 4;

    // private fields

    private String setName;
    private short setType;
    private Criteria[] crits;
    private CopyParams[] copies;
    private CopyParams[] rearchCopies;
    private VSNMap[] maps;
    private VSNMap[] rearchMaps;
    private String description; /* Support is only gauranteed in IntelliStor */

    /**
     * private constructor
     */
    public ArSet(String setName, short setType, Criteria[] crits,
                 CopyParams[] copies, CopyParams[] rearchCopies,
                 VSNMap[] maps, VSNMap[] rearchMaps) {
        this.setName = setName;
        this.setType = setType;
        this.crits = crits;
        this.copies = copies;
        this.rearchCopies = rearchCopies;
        this.maps = maps;
        this.rearchMaps = rearchMaps;
	this.description = null;
    }

    /**
     * private constructor. This includes the description field. This
     * field may not be supported in core SAM but is required for IntelliStor.
     */
    public ArSet(String setName, String description, short setType,
		 Criteria[] crits, CopyParams[] copies,
		 CopyParams[] rearchCopies, VSNMap[] maps,
		 VSNMap[] rearchMaps) {
        this.setName = setName;
        this.setType = setType;
        this.crits = crits;
        this.copies = copies;
        this.rearchCopies = rearchCopies;
        this.maps = maps;
        this.rearchMaps = rearchMaps;
        this.description = description;
    }

    public String getArSetName() { return setName; }

    public String getDescription() { return description; }
    public void setDescription(String description) {
        this.description = description;
    }

    public short getArSetType() { return setType; }

    public Criteria[] getCriteria() { return crits; }

    public void setCriteria(Criteria[] crits) {
        this.crits = crits;
    }

    public CopyParams[] getCopies() { return copies; }

    public void setCopies(CopyParams[] copies) {
        this.copies = copies;
    }

    public CopyParams[] getRearchCopies() { return rearchCopies; }

    public void setRearchCopies(CopyParams[] rearchCopies) {
        this.rearchCopies = rearchCopies;
    }

    public VSNMap[] getMaps() { return maps; }

    public void setMaps(VSNMap[] maps) {
        this.maps = maps;
    }

    public VSNMap[] getRearchMaps() { return rearchMaps; }

    public void setRearchMaps(VSNMap[] rearchMaps) {
        this.rearchMaps = rearchMaps;
    }

    public String toString() {
        String strCrits = "";
        if (crits != null) {
            for (int i = 0; i < crits.length; i++) {
                strCrits += crits[i] + "\n";
            }
        }

        String strCopies = "";
        if (copies != null) {
            for (int i = 0; i < copies.length; i++) {
                strCopies += copies[i] + "\n";
            }
        }

        String strRearchCopies = "";
        if (rearchCopies != null) {
            for (int i = 0; i < rearchCopies.length; i++) {
                strRearchCopies += rearchCopies[i] + "\n";
            }
        }

        String strMaps = "";
        if (maps != null) {
            for (int i = 0; i < maps.length; i++) {
                strMaps += maps[i] + "\n";
            }
        }

        String strRearchMaps = "";
        if (rearchMaps != null) {
            for (int i = 0; i < rearchMaps.length; i++) {
                strRearchMaps += rearchMaps[i] + "\n";
            }
        }

	String strDesc = "";
	if (description != null) {
	    strDesc += "description: " + description;
	}

        String s = "ArSet name: " + setName + "\n" +
                   strDesc +
                   "type: " + setType + "\n" +
                   "Criteria: \n" + rearchCopies +
                   "strCrits: \n" + strCopies +
                   "Rearch CopyParams: \n" + strRearchCopies +
                   "VSNMaps: \n" + strMaps +
                   "Rearch VSNMaps: \n" + strRearchMaps;

        return s;
    }
}
