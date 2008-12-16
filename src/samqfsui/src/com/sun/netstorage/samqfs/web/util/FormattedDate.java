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

// ident	$Id: FormattedDate.java,v 1.8 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import java.text.DateFormat;
import java.util.Calendar;
import java.util.Date;

/**
 * Contains a DateFormat object used to format its output
 * Used for storing a date and formatter so that it can be passed to an
 * actiontable and the values in the column with be both sorted and displayed
 * correctly (Date class handles compare()).
 */
public class FormattedDate extends Date {
    private DateFormat dateFormat = null;
    private Calendar calendar = null;

    /**
     * Constructor
     * @param calendar the calendar that stores the date
     * @param format the DateFormat to use
     */
    public FormattedDate(Calendar calendar, DateFormat format) {
        setCalendar(calendar);
        setDateFormat(format);
    }

    /**
     * @return Returns the dateFormat.
     */
    public DateFormat getDateFormat() {
        return dateFormat;
    }
    /**
     * @param dateFormat The dateFormat to set.
     */
    public void setDateFormat(DateFormat dateFormat) {
        this.dateFormat = dateFormat;
    }

    /**
     * Returns the date formatted with the given DateFormat object, if given
     * If no DateFormat is given it simply returns Date.toString()
     */
    public String toString() {
        String date = "";

        /*
         * If calendar is null then a time hasnt been set so dont return
         * a string to display.
         */
        if (calendar != null) {
            if (dateFormat == null) {
                date = super.toString();
            } else {
                date = dateFormat.format(this);
            }
        }

        return date;
    }

    /**
     * @return Returns the calendar.
     */
    public Calendar getCalendar() {
        return calendar;
    }
    /**
     * @param calendar The calendar to set.
     */
    public void setCalendar(Calendar calendar) {
        this.calendar = calendar;

        // check for null
        if (calendar != null) {
            this.setTime(calendar.getTime().getTime());
        }
    }
}
