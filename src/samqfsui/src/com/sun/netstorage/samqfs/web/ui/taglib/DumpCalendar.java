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

// ident	$Id: DumpCalendar.java,v 1.10 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.ui.taglib;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.RetainCalendar;
import java.util.GregorianCalendar;

public class DumpCalendar extends RequestHandlingViewBase {

    public static final String SCHEMA1 = "1";
    public static final String SCHEMA2 = "2";
    public static final String SCHEMA3 = "3";
    public static final String SCHEMA4 = "4";

    private int intschema;

    public DumpCalendar(ContainerView parent, String name, String schema) {
        super(parent, name);

        this.intschema = Integer.parseInt(schema);
    }

    /**
     * Called as notification that the JSP has begun its display
     * processing. This method stores the model with state info
     * using the page session and hidden display fields.
     *
     * @param event The DisplayEvent.
     */
    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {

        super.beginDisplay(event);
    }


    public String getRetentionSchema() {
        return Integer.toString(this.intschema);
    }


    // Interface to call into RetainCalendar with our schema
    public boolean isSnapshotRetainedOnDay(
        GregorianCalendar theDate, GregorianCalendar today)
        throws SamFSException {
        return RetainCalendar.isSnapshotRetainedOnDay(
            theDate, today, this.intschema);
    }

    // Interface to forward call into RetainCalendar
    public int getNumMonthsForSchema() {
        return RetainCalendar.getNumMonthsForSchema(this.intschema);
    }
}
