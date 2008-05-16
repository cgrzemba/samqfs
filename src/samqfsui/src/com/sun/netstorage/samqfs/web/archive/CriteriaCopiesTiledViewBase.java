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

// ident	$Id: CriteriaCopiesTiledViewBase.java,v 1.11 2008/05/16 19:39:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCTextField;
import javax.servlet.http.HttpServletRequest;

/**
 * This class acts as the tiled view for all instances where an editable
 * criteria copy table is required. Currently, its used by both the
 * Policy Criteria Details Page and the New Criteria Wizard. Please use this
 * class in conjunction with the CriteriaDetailsCopyTable43.xml
 */
public abstract class CriteriaCopiesTiledViewBase extends CommonTiledViewBase {
    public static final String ARCHIVE_AGE = "ArchiveAgeText";
    public static final String ARCHIVE_AGE_UNITS = "ArchiveAgeUnits";
    public static final String UNARCHIVE_AGE = "UnarchiveAgeText";
    public static final String UNARCHIVE_AGE_UNITS = "UnarchiveAgeUnits";
    public static final String RELEASE_OPTIONS = "ReleaseOptionsText";

    public CriteriaCopiesTiledViewBase(View parent,
                     CCActionTableModel model,
                     String name) {
        super(parent, model, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
    }

    public void mapRequestParameters(HttpServletRequest request)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.mapRequestParameters(request);
        TraceUtil.trace3("Exiting");
    }

    public boolean beginArchiveAgeTextDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        ArchivePolCriteriaCopy [] criteriaCopies = getCriteriaCopies();
        long age = criteriaCopies[index].getArchiveAge();

        CCTextField field = (CCTextField)getChild(ARCHIVE_AGE);
        if (age != -1) {
            field.setValue(Long.toString(age));
        } else {
            field.setValue("");
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginArchiveAgeUnitsDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        ArchivePolCriteriaCopy [] criteriaCopies = getCriteriaCopies();
        int ageUnit = criteriaCopies[index].getArchiveAgeUnit();

        CCDropDownMenu field = (CCDropDownMenu)getChild(ARCHIVE_AGE_UNITS);
        field.setValue(Integer.toString(ageUnit));

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginUnarchiveAgeTextDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        ArchivePolCriteriaCopy [] criteriaCopies = getCriteriaCopies();
        long unarchiveAge = criteriaCopies[index].getUnarchiveAge();

        CCTextField field = (CCTextField)getChild(UNARCHIVE_AGE);
        if (unarchiveAge != -1) {
            field.setValue(Long.toString(unarchiveAge));
        } else {
            field.setValue("");
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginUnarchiveAgeUnitsDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        ArchivePolCriteriaCopy [] criteriaCopies = getCriteriaCopies();
        int unarchiveAgeUnit = criteriaCopies[index].getUnarchiveAgeUnit();

        CCDropDownMenu field = (CCDropDownMenu)getChild(UNARCHIVE_AGE_UNITS);
        field.setValue(Integer.toString(unarchiveAgeUnit));

        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     */
    public boolean beginReleaseOptionsTextDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        ArchivePolCriteriaCopy [] criteriaCopies = getCriteriaCopies();

        CCDropDownMenu field = (CCDropDownMenu)getChild(RELEASE_OPTIONS);

        String releaseOptions = "--";
        if (criteriaCopies[index].isNoRelease()) {
            releaseOptions = "false";
        } else if (criteriaCopies[index].isRelease()) {
            releaseOptions = "true";
        }

        field.setValue(releaseOptions);
        TraceUtil.trace3("Exiting");
        return true;
    }

    /**
     * since this class will be extended and used in all the areas where an
     * editable criteria copy table is required (criteria details, and new
     * criteria wizard for starters), the child classes will be expected to
     * implement this method and retrieve the appropriate settings.
     *
     * @return criteria copies - an array of ArchivePolCriteriaCopy
     */
    public abstract ArchivePolCriteriaCopy [] getCriteriaCopies();
}
