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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: RetainCalendar.java,v 1.8 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;


/*
 * Code shared between GUI and user-mode application which deletes
 * snapshots. Main entry is to determine whether the snapshot for a
 * particular date should be retained by another particular date.
 * Third argument is the /schema/ which indicates which of several
 * methods we use to determine when snapshots expire.
 */

public class RetainCalendar  {

    // Listed schema. Add to list if more are defined.

    public static final int SCHEMA1 = 1;
    public static final int SCHEMA2 = 2;
    public static final int SCHEMA3 = 3;
    public static final int SCHEMA4 = 4;

    static public boolean isSnapshotRetainedOnDay(
        GregorianCalendar theDate,
        GregorianCalendar today,
        int schema) {

        GregorianCalendar date = (GregorianCalendar) theDate.clone();
        GregorianCalendar compareDate =
            (GregorianCalendar)today.clone();

        // Normalize times
        date.set(Calendar.HOUR_OF_DAY, 0);
        date.set(Calendar.MINUTE, 0);
        date.set(Calendar.SECOND, 0);
        date.set(Calendar.MILLISECOND, 0);
        compareDate.set(Calendar.HOUR_OF_DAY, 0);
        compareDate.set(Calendar.MINUTE, 0);
        compareDate.set(Calendar.SECOND, 0);
        compareDate.set(Calendar.MILLISECOND, 0);
        today.set(Calendar.HOUR_OF_DAY, 0);
        today.set(Calendar.MINUTE, 0);
        today.set(Calendar.SECOND, 0);
        today.set(Calendar.MILLISECOND, 0);

        switch (schema) {
            case SCHEMA1:
                return testSchema1(date, compareDate, today);
            case SCHEMA2:
                return testSchema2(date, compareDate, today);
            case SCHEMA3:
                return testSchema3(date, compareDate, today);
            case SCHEMA4:
                return testSchema4(date, compareDate, today);
            default:
                return true;	// Unrecognizeble schema, say retained
        }
    }

    /**
     * Retain all snapshots for 3 weeks then delete them
     * EXCEPT FOR the first scheduled snapshot of each week
     * which is retained.
     * Retain these snapshots for 2 months then delete them
     * EXCEPT FOR the first snapshot of each month which is
     * retained.
     * Retain these snapshots for 6 months then delete them
     * EXCEPT FOR the first snapshot of each quarter which is
     * retained permanently.
     */
    static private boolean testSchema1(
        GregorianCalendar date,
        GregorianCalendar compareDate,
        GregorianCalendar today) {

        long dateMillis = date.getTimeInMillis();

        // Assume daily snapshots

        // Retain snapshots less than 3 weeks old
        compareDate.add(Calendar.DAY_OF_MONTH,  -21);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            if (dateMillis <= today.getTimeInMillis()) {
                // Retain
                return true;
            } else {
                return false;
            }
        }
        // Retain if less than 2 months old and is first of the week
        // Move to 2 months
        compareDate.add(Calendar.DAY_OF_MONTH, 21);
        compareDate.add(Calendar.MONTH, -2);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            if (
                date.get(Calendar.DAY_OF_WEEK) ==
                date.getFirstDayOfWeek()) {
                // Retain
                return true;
            } else {
                return false;
            }
        }
        // Retain if less than 6 months old and is first eekly of month
        // Move to 6 months
        compareDate.add(Calendar.MONTH, -4);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            if (
                date.get(Calendar.DAY_OF_WEEK) ==
                date.getFirstDayOfWeek()) {
                // Is there a first day of week before
                // this one this month?
                int theMonth = date.get(Calendar.MONTH);
                date.add(Calendar.DAY_OF_WEEK, -7);
                if (date.get(Calendar.MONTH) != theMonth) {
                    // This is the first of the month
                    return true;
                } else {
                    return false;
                }
            }
        }
        // Otherwise, retain is first in quarter
        int theMonth = date.get(Calendar.MONTH);
        if (
            date.get(Calendar.DAY_OF_WEEK) ==
            date.getFirstDayOfWeek()) {
            // Is there a first day of week before this one
            // this month?
            date.add(Calendar.DAY_OF_WEEK, -7);
            if (
                date.get(Calendar.MONTH) != theMonth &&
                (theMonth == Calendar.MARCH ||
                theMonth == Calendar.JUNE ||
                theMonth == Calendar.SEPTEMBER ||
                theMonth == Calendar.DECEMBER)) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }


    /**
     * Retain all snapshots for 6 weeks then delete them
     * EXCEPT FOR the first scheduled snapshot of every second week
     * which is retained.
     * Retain these snapshots for 1 year then delete them
     * EXCEPT FOR the first snapshot of each quarter which is
     * retained permanently.
     * Assume daily snapshots
     */
    static private boolean testSchema2(
        GregorianCalendar date,
        GregorianCalendar compareDate,
        GregorianCalendar today) {

        long dateMillis = date.getTimeInMillis();

        // Retain snapshots less than 6 weeks old

        compareDate.add(Calendar.DAY_OF_MONTH,  -42);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            if (dateMillis <= today.getTimeInMillis()) {
                // Retain
                return true;
            } else {
                // Can't retain future snapshots...
                return false;
            }
        }

        // Anything else moving forward has to be a snapshot on the
        // first day of every other week.

        if (
            date.get(Calendar.DAY_OF_WEEK) !=
            date.getFirstDayOfWeek()) {
            return false;
        }
        // Is it one of them "every other" days?
        // Get the first day of this week
        GregorianCalendar testDate =
            (GregorianCalendar)compareDate.clone();
        testDate.add(
            Calendar.DAY_OF_WEEK, testDate.getFirstDayOfWeek() -
            testDate.get(Calendar.DAY_OF_WEEK));

        boolean passTest = false;
        int year = date.get(Calendar.YEAR);
        int month = date.get(Calendar.MONTH);
        int day = date.get(Calendar.DAY_OF_MONTH);
        SimpleDateFormat fmt = new SimpleDateFormat();

        do {
            if (testDate.get(Calendar.YEAR) == year &&
                testDate.get(Calendar.MONTH) == month &&
                testDate.get(Calendar.DAY_OF_MONTH) == day) {
                passTest = true;
                break;
            }
            // Go back two weeks
            testDate.add(Calendar.DAY_OF_MONTH, -14);
        } while (testDate.getTimeInMillis() >= dateMillis);

        if (!passTest) {
            return false;
        }

        // Retain if less than 1 year old
        // Move to 1 year
        compareDate.add(Calendar.DAY_OF_MONTH, 42);
        compareDate.add(Calendar.YEAR, -1);

        if (dateMillis >= compareDate.getTimeInMillis()) {
            // Retain
            return true;
        }

        // Otherwise, retain if first in quarter
        int theMonth = date.get(Calendar.MONTH);
        // Is there a first day of every second week before
        // this one this month?
        date.add(Calendar.DAY_OF_WEEK, -14);
        if (
            date.get(Calendar.MONTH) != theMonth &&
            (theMonth == Calendar.MARCH ||
            theMonth == Calendar.JUNE ||
            theMonth == Calendar.SEPTEMBER ||
            theMonth == Calendar.DECEMBER)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Retain all snapshots for 3 months then delete them
     * EXCEPT FOR the first scheduled snapshot of every month
     * which is retained.
     * Retain these snapshots for 2 years then delete them
     * EXCEPT FOR the first snapshot of each year which is
     * retained permanently.
     * Assume daily snapshots
     */
    static private boolean testSchema3(
        GregorianCalendar date,
        GregorianCalendar compareDate,
        GregorianCalendar today) {

        long dateMillis = date.getTimeInMillis();

        // Retain snapshots less than 3 months old
        compareDate.add(Calendar.MONTH,  -3);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            if (dateMillis <= today.getTimeInMillis()) {
                // Retain
                return true;
            } else {
                return false;
            }
        }

        // Any else from this point on needs to be the first snapshot
        // of the month.  For daily snapshots, that's te first of
        // the month.
        if (date.get(Calendar.DAY_OF_MONTH) != 1) {
            // Not the first of the month
            return false;
        }

        // Retain if less than 2 years old and is first of the month
        // Move to 2 years
        compareDate.add(Calendar.MONTH, 3);
        compareDate.add(Calendar.YEAR, -2);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            return true;
        }

        // Otherwise, retain First snapshot of the year.
        // For us, that means January.
        if (date.get(Calendar.MONTH) == Calendar.JANUARY &&
            date.get(Calendar.DAY_OF_MONTH) == 1) {
            return true;
        }
        return false;
    }


    /**
     * Retain all snapshots for 3 weeks then delete them
     * EXCEPT FOR the first scheduled snapshot after the 5th day of each
     * week which is retained.
     * Retain these snapshots for 2 months then delete them
     * EXCEPT FOR the first snapshot after the 15th day of each month
     * which is retained.
     * Retain these snapshots for 6 months then delete them
     * EXCEPT FOR the first snapshot of each quarter which is
     * retained permanently.
     * Assume daily snapshots
     */
    static private boolean testSchema4(
        GregorianCalendar date,
        GregorianCalendar compareDate,
        GregorianCalendar today) {

        long dateMillis = date.getTimeInMillis();

        // Retain snapshots less than 3 weeks old
        compareDate.add(Calendar.DAY_OF_MONTH,  -21);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            if (dateMillis <= today.getTimeInMillis()) {
                // Retain
                return true;
            } else {
                return false;
            }
        }

        // Anything else from here on will be the first snapshot after
        // the 5th day of the week  ?????
        if (date.get(Calendar.DAY_OF_WEEK) != 5) {
            return false;
        }

        // Retain if less than 2 months old
        // Move to 2 months
        compareDate.add(Calendar.DAY_OF_MONTH, 21);
        compareDate.add(Calendar.MONTH, -2);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            return true;
        }

        // Anything else from here on will be the first snapshot after
        // the 15th day of the month  ?????
        if (date.get(Calendar.DAY_OF_MONTH) < 15) {
            return false;
        }
        // Is this the FIRST after the 15th?
        GregorianCalendar testDate = (GregorianCalendar) date.clone();
        testDate.add(Calendar.DAY_OF_MONTH,  -7);
        if (testDate.get(Calendar.DAY_OF_MONTH) >= 15) {
            // Not the first
            return false;
        }

        // Retain if less than 6 months old
        // Move to 6 momths
        compareDate.add(Calendar.MONTH, -4);
        if (dateMillis >= compareDate.getTimeInMillis()) {
            return true;
        }

        // Otherwise, retain if first in quarter
        int theMonth = date.get(Calendar.MONTH);
        if (
            theMonth == Calendar.MARCH ||
            theMonth == Calendar.JUNE ||
            theMonth == Calendar.SEPTEMBER ||
            theMonth == Calendar.DECEMBER) {
            return true;
        }
        return false;
    }

        /*
         * Returns the number of  monts needed to display the schema example.
         * Used specifically for GUI, no other use.
         */
    static public int getNumMonthsForSchema(int schema) {
        switch (schema) {
            case SCHEMA1:
                return 12;
            case SCHEMA2:
                return 18;
            case SCHEMA3:
                return 36;
            case SCHEMA4:
                return 12;
            default:
                return 12;
        }
    }
}
