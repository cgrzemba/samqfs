/*
 * time_string.c - Time string conversion functions.
 */

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

#pragma ident "$Revision: 1.28 $"

/* ANSI C headers. */
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/timeb.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"

#ifndef INT_MAX
#define	INT_MAX	2147483647
#endif

/*
 * Calendar time zone info used by the date/time manipulation subroutines
 * for WORM.
 */
static char *Month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"};
static int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int ldays_in_month[12] = {31, 29, 31, 30, 31, 30, 31, 31,
	30, 31, 30, 31};

/*
 * The epoch starts on 1/1/1970 UTC which is a Thursday. Sunday is the
 * fourth element in the array (third by index).
 */
#define	SUNDAY	3
static char *days_in_week[] = {"Thur", "Fri", "Sat", "Sun",
	"Mon", "Tues", "Wed"};


/*
 * An array of timezone entries containing the month, which sunday, and
 * time (hour) when daylight saving time changes occur.  The first three
 * elements of an entry are timezones.  The remaining elements comprise
 * two groups of three.  The first group contains the sunday, month, and
 * time for the daylight saving transition in the spring.  The second
 * group contains the same inforamtion for the Fall transition.  If the
 * timezone does not support daylight saving time then zeros are stored
 * in the elements.  A per element description of the first entry is
 * given below.
 *
 *	"EST", "EDT", "US/Eastern"	The time zone is in the US/Eastern.
 *
 *	1		The first Sunday
 *	3		This is the month of April (January is the 0'th month)
 *	2		2 AM
 *
 *	The elements above translate to the first Sunday in April at 2AM.
 *
 *	9		The last Sunday
 *	9		The month of October
 *	2		2AM
 *
 *	These elements translate to the last Sunday in October at 2AM.
 */
#define	LAST_SUNDAY	9

static struct dst {
	char *zone[3];
	int ssunday;
	int smonth;
	int shour;
	int wsunday;
	int wmonth;
	int whour;
} dsttable[] = {
"EST", "EDT", "US/Eastern",	2,  2, 2,  1,  10, 2, /* US Eastern */
"CST", "CDT", "US/Central",	2,  2, 2,  1,  10, 2, /* US Central */
"MST", "MDT", "US/Mountain",	2,  2, 2,  1,  10, 2, /* US Mountain */
"PST", "PDT", "US/Pacific",	2,  2, 2,  1,  10, 2, /* US Pacific */
"MST", "MST", "US/Arizona",	0,  0, 0,  0,  0, 0, /* Arizona */
"HST", "HST", "US/Hawaii",	0,  0, 0,  0,  0, 0, /* Hawaiian */
"NZ", "NZ", "NZ",		3,  2, 3,  1,  9, 2, /* New Zealand */
"HKT", "HKT", "Asia/Hong_Kong",	3,  2, 3,  1,  9, 2, /* Hong Kong/China */
"EST", "EST", "Australia/Sydney", 9,  9, 2,  9,  2, 3, /* Aust: Eastern */
"CST", "CST", "Australia/Adelaide", 9,  9, 2,  9,  2, 3, /* Aust: Central */
"WST", "WST", "Australia/West",	0,  0, 0,  0,  0, 0, /* Aust: Western */
"JST", "JST", "Japan",		0,  0, 0,  0,  0, 0, /* Japan */
"GMT", "GMT", "Greenwich",	0,  0, 0,  0,  0, 0, /* UTC */
"WET", "WET", "WET",		0,  0, 0,  0,  0, 0, /* Western Europe */
"CET", "CET", "CET",		9,  2, 1,  9,  9, 1, /* Central Europe */
"EET", "EET", "EET",		9,  2, 1,  9,  9, 1, /* Eastern Europe */
"AST", "ADT", "Canada/Atlantic", 1,  3, 2,  9,  9, 2, /* Canada Atlantic */
"EST", "EDT", "Canada/Eastern",	1,  3, 2,  9,  9, 2, /* Canada Eastern */
"CST", "CDT", "Canada/Central",	1,  3, 2,  9,  9, 2, /* Canada Central */
"MST", "MDT", "Canada/Mountain", 1,  3, 2,  9,  9, 2, /* Canada Mountain */
"NST", "NDT", "Canada/Newfoundland", 1,  3, 2,  9,  9, 2, /* Newfoundland */
"CST", "CST", "Canada/Saskatchewan", 0,  0, 0,  0,  0, 0, /* Saskatchewan */
};


/*
 * time_string.c - Format time as ASCII string.
 *
 *	Description:
 *		Time_string formats the specified time as a NULL terminated
 *		ASCII string.  The string contains the month, day and time
 *		in hours and  minutes unless the time is greater than six
 *		months from the current_time, then the year is supplied
 *		instead of the time.  If the specified time is 0, the
 *		string "none" is returned.
 *
 *		Examples:
 *
 *		Mar 24 18:32
 *		Mar  1  1980
 *
 *	On Entry:
 *		time	= Time to convert as returned by time(2).
 *		current_time	= Current time used to determine the six month
 *			threshold.
 *		str		= Address of an 17 character string used to
 *			assemble the time string.
 *
 *	Returns:
 *		The address of the status string (i.e., str).
 *
 */
char *
time_string(
	time_t	time,			/* Time to convert */
	time_t	current_time,		/* Current time	*/
	char	*str)			/* Converted time string */
{
	char	temp_str[40];
	long	cutoff	= 6 * 30 * 24 * 60 * 60;

	if (time == 0) {		/* If time was never set */
		(void *)strcpy(str, "none        ");
		return (str);
	}

	(void *)strcpy(temp_str, ctime(&time));

	/* If file is fairly old or in the future, show year instead */
	/* of time. */

	if (current_time - time > cutoff || current_time - time < 0L)
		(void *)strcpy(temp_str+11, temp_str+19);

	temp_str[16] = '\0';
	(void *)strcpy(str, temp_str+4);
	return (str);
}


/*
 *  TimeString() is slightly different:
 *
 *	Description:
 *		TimeString() formats the specified time as a NUL terminated
 *		ASCII string.  The string contains the year, month, day and
 *		time in hours and  minutes.  If the specified time is 0, the
 *		string "none" is returned.
 *
 *		Examples:
 *
 *		2000/03/24 18:32
 *		none
 *
 *	On Entry:
 *		time	= Time to convert as returned by time(2).
 *		str		= Address of a character string used to
 *				assemble the time string.  If str is NULL, the
 *				conversion is returned in a static buffer.
 *      size    = size of str in characters.
 *
 *	Returns:
 *		The address of the status string (i.e., str).
 *		Returned value is "none            " if time is 0 (the epoch).
 *
 */
#define	TIMEFORMAT "%Y/%m/%d %H:%M"
#define	NOTIME	"none            "

char *
TimeString(
	time_t time,		/* Value to convert. */
	char   *str,		/* converted time string */
	int    size)		/* size of *str */
{
	static char our_str[STR_FROM_TIME_BUF_SIZE];
	struct tm *tm;

	if (str == NULL) {
		str = our_str;
		size = sizeof (our_str);
	}
	if (size < STR_FROM_TIME_BUF_SIZE) {
		*str = '\0';
		return (str);
	}
	if (time == 0) {
		strncpy(str, NOTIME, size-1);
	} else {
		tm = localtime(&time);
		strftime(str, size-1, TIMEFORMAT, tm);
	}
	return (str);
}


#ifdef sun
/*
 *  Convert a date/time string to time_t value
 *  Various formats in the string are accepted; see the strptime()
 *  function for more information on what the various fmts[] entries
 *  mean.
 */
time_t					/* Time value */
StrToTime(
	char *string)
{
	enum field_flags {
		year = 0x01, month = 0x02, day = 0x04, hm = 0x08, sc = 0x10 };
	struct {
		char	*str;
		enum field_flags flags;
	} fmts[] = {
		/* ISO8601 date yyyy-mm-dd */
		{ "%Y-%m-%d",		year|month|day },
		{ "%Y-%m-%d %R",	year|month|day|hm },
		{ "%Y-%m-%d %T",	year|month|day|hm|sc },
		{ "%b %d %R",		month|day|hm },
		{ "%b %d %T",		month|day|hm|sc },
		{ "%b %d %Y",		month|day|year },
		{ "%b %d %Y %R",	month|day|year|hm },
		{ "%b %d %Y %T",	month|day|year|hm|sc },
		{ "%b %d",		month|day },
		{ "%d %b %R",		day|month|hm },
		{ "%d %b %T",		day|month|hm|sc },
		{ "%d %b %Y",		day|month|year },
		{ "%d %b %Y %R",	day|month|year|hm },
		{ "%d %b %Y %T",	day|month|year|hm|sc },
		{ "%d %b",		day|month },
		NULL
	};

	struct tm user_tm, *now_tm;
	time_t	now;
	int i;

	now = time(NULL);
	now_tm = localtime(&now);

	/*
	 * Try conversions until one succeeds.
	 */
	for (i = 0; fmts[i].str != NULL; i++) {
		char *p;

		p = (char *)strptime(string, fmts[i].str, &user_tm);
		if (p != NULL && *p == '\0') {	 /* conversion succeeded */
			/*
			 * Set undefined fields to now.
			 */
			if (!(fmts[i].flags & month))
				user_tm.tm_mon  = now_tm->tm_mon;
			if (!(fmts[i].flags & day))
				user_tm.tm_mday = now_tm->tm_mday;
			if (!(fmts[i].flags & year))
				user_tm.tm_year = now_tm->tm_year;
			if (!(fmts[i].flags & hm)) {
				user_tm.tm_hour = now_tm->tm_hour;
				user_tm.tm_min  = now_tm->tm_min;
			}
			if (!(fmts[i].flags & sc))
				user_tm.tm_sec = now_tm->tm_sec;
			user_tm.tm_wday = 0;
			user_tm.tm_yday = 0;
			/* Let mktime() figure out DST */
			user_tm.tm_isdst = -1;
			return (mktime(&user_tm));
		}
	}
	errno = EINVAL;
	return (-1);
}
#endif /* sun */


/*
 * A year is a leap year if it's divisible by 4 but not 100
 * unless it's also divisible by 400.
 */
int
leap_year(
	int year)
{
	return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}


/*
 * Convert the given time (in seconds) since the epoch to multiple
 * strings of year, month, day, hour, and minute.
 */
void
get_chg_time(
	time_t chgtime,
	char *year,
	char *month,
	char *day,
	char *hour,
	char *min)
{
	char change_time[40];

	(void *)strcpy(change_time, ctime(&chgtime));

	snprintf(year, 5, change_time + 20);

	snprintf(month, 4, change_time + 4);

	snprintf(day, 3, change_time + 8);

	snprintf(hour, 3, change_time + 11);

	snprintf(min, 3, change_time + 14);
}


/*
 * Calculate whether the end or target time falls into winter or summer
 * (ie daylight saving) time.  Provides an adjustment representing
 * the saving time. The begin year and time is a reference from which
 * the adjustment is viewed. The current time is implied if zero's
 * are passed for the begin years and minutes.
 */
int
summer_time(
	struct dst *dst,
	int begin_year,
	int begin_mins,
	int end_year,
	int end_mins)
{
	int wday = 0, month = 0, tyear = 0;
	int day = 1, hour = 0, year = 1970;
	int num_sundays = 0;
	int *pmonth, lmins = 0;
	int cur_day = 0, res_day = 0;
	int adjust = 0, i = 0;


	/*
	 * Two passes to process begin and end dates.
	 */
	for (i = 0; i < 2; i++) {

		num_sundays = 0; wday = 0; month = 0;
		day = 1; hour = 0, year = 1970;

		if (i == 0) {
			tyear = begin_year;
			lmins = begin_mins;
		} else {
			tyear = end_year;
			lmins = end_mins;
		}

		/*
		 * Adjust for Universal time (used in the kernel).
		 */
		lmins -= (timezone/60);

		while (lmins >= 60) {
			lmins -= 60;
			hour++;

			if (hour == 24) {
				hour = 0;
				day++;
				wday = (wday + 1) % 7;
				if (wday == 3) {
					num_sundays++;
				}
			}

			if (leap_year(year)) {
				pmonth = ldays_in_month;
			} else {
				pmonth = days_in_month;
			}

			if (day > pmonth[month]) {
				day = 1;
				if (month == 11) {
					year++;
				}
				month = (month + 1) % 12;
				if (wday == 3) {
					num_sundays = 1;
				} else {
					num_sundays = 0;
				}
			} else if ((year == tyear) && (wday == SUNDAY)) {
				if (month == dst->smonth) {
					if ((num_sundays == dst->ssunday) &&
					    (hour == dst->shour)) {
						if (i == 0) {
							cur_day = 1;
						} else {
							res_day = 1;
						}
					} else if (
					    (dst->ssunday == LAST_SUNDAY) &&
					    ((day + 7) != pmonth[month]) &&
					    ((day + 8) > pmonth[month]) &&
					    (hour == dst->shour)) {
						if (i == 0) {
							cur_day = 1;
						} else {
							res_day = 1;
						}
					}
				} else if (month == dst->wmonth) {
					if ((num_sundays == dst->wsunday) &&
					    (hour == dst->whour)) {
						if (i == 0) {
							cur_day = 0;
						} else {
							res_day = 0;
						}
					} else if (
					    (dst->wsunday == LAST_SUNDAY) &&
					    ((day + 7) != pmonth[month]) &&
					    ((day + 8) > pmonth[month]) &&
					    (hour == dst->whour)) {
						if (i == 0) {
							cur_day = 0;
						} else {
							res_day = 0;
						}
					}
				}
			}
		}
	}

	/*
	 * If current time is summer and target time is winter
	 * then subtract one hour (60 minutes). Likewise if the
	 * current time is winter and target is summer add an hour.
	 * If both are either in the summer or winter, then no
	 * adjustment is necessary.
	 *
	 */

	if (cur_day != res_day) {
		if (!cur_day) {
			adjust = 60;

			/*
			 * If time is moving from summer to winter then the
			 * displayed retention period will be advanced by
			 * 1 hour. This means we'll pass through the dst hour
			 * in the fall twice.  To attempt to handle this we need
			 * to check for this condition and make the adjustment
			 * accordingly. The Final pass through the loop above is
			 * the end or target date.  So, we just need to compare
			 * the final resulting time plus one hour to what is
			 * stored in the dst array.
			 */
			if ((month == dst->wmonth) &&
			    ((dst->wsunday == LAST_SUNDAY) &&
			    ((day + 7) != pmonth[month]) &&
			    ((day + 8) > pmonth[month]) &&
			    (hour + 1 == dst->whour))) {
				adjust = 0;
			}
		} else {
			adjust = -60;
		}
	}

	return (adjust);
}


/*
 * Adjust for daylight savings time.
 */
int
dst_adjust(
	int begin_year,
	int begin,
	int end_year,
	int end)
{
	int adjust = 0, i = 0, j = 0;
	int new_mins = 0, found_tz = 0;
	time_t  now;
	char *name = NULL;
	char *curr_date, curr_year[5];
	int  cyear;

	/*
	 * Get localtime, add input minutes.  Check if we need an
	 * adjustment for the current time zone.
	 */

	if (begin_year == 0) {
		now = time(NULL);
		curr_date = ctime(&now);
		snprintf(curr_year, 5, curr_date + 20);
		cyear = atoi(curr_year);
	} else {
		cyear = begin_year;
	}

	if (begin == 0) {
		now /= 60;
	} else {
		now = begin;
	}

	new_mins = end;

	/*
	 * Get the timezone we're in to determine if the provided
	 * time is in the summer.
	 */
	name = (char *)getenv("TZ");
	if (name == NULL) {
		/* Need TZ variable set */
		return (0);
	}

	for (j = 0; j < (sizeof (dsttable)/sizeof (struct dst)); j++) {
		for (i = 0; i < 3; i++) {
			if (strcmp(name, dsttable[j].zone[i]) == 0) {
				found_tz = 1;
				break;
			}
		}
		if (found_tz) {
			break;
		}
	}

	if (found_tz) {
		if ((dsttable[j].ssunday == 0) && (dsttable[j].wsunday == 0)) {
			adjust = 0;
		} else {
			adjust  = summer_time(&dsttable[j], cyear,
			    now, end_year, new_mins);
		}
	}

	return (adjust);
}


/*
 * Convert a string to minutes.  This routine compensates for the
 * 31 bit limitation of ctime(), etc.  It is used in the WORM
 * CLI's.
 */
int
StrToMinutes(
	char *args,
	long *expiration)
{
	char cyear[5], cmonth[4], cday[3], chour[3], cmin[3];
	int years = 0, days = 0, hours = 0, minutes = 0;
	int sum = 0, err = 0;
	int i, total_mins = 0;
	int leap = 0, curr_year = 0, curr_month = 0;
	time_t curr_time;


	/*
	 * Convert provided date to minutes. The provided
	 * date is a delta from the current date/time.
	 * It is expected to be a character string with
	 * this format:
	 *
	 *  <M>y<N>d<O>h<P>m
	 *  where
	 *		y is years
	 *		d is days
	 *		h is hours
	 *		m is minutes
	 */

	if (strncmp(args, "permanent", 9) == 0) {
		*expiration = 0;
		return (0);
	}

	/*
	 * Get the current date/time.
	 */
	curr_time = time((time_t *)NULL);
	get_chg_time(curr_time, cyear, cmonth, cday, chour, cmin);

	curr_year = atoi(cyear);
	for (i = 0; i < 12; i++) {
		if (strcmp(cmonth, Month[i]) == 0) {
			break;
		}
	}
	curr_month = i;

	/*
	 * Convert provided date to minutes equivalent.
	 */
	for (i = 0; (args[i] != '\0') && (err == 0); i++) {
		if (isdigit(args[i])) {
			sum = (sum * 10) + (args[i] - '0');
		} else {
			switch (args[i]) {
				case 'y':
					years = sum;
					break;
				case 'd':
					days = sum;
					break;
				case 'h':
					hours = sum;
					break;
				case 'm':
					minutes = sum;
					break;
				default:
					err = -1;
					break;
			}
			sum = 0;
		}
	}

	/*
	 * Numeric argument passed but no delimitter.
	 */
	if (sum != 0) {
		err = -1;
	}

	/*
	 * The leap year count is incremented each time we pass
	 * through a leap year and the month of February.
	 * If the current year is a leap year and the duration
	 * is longer than a year and the current month is either
	 * January or February then increment the count.
	 * We pass through the extra day in the current year.
	 * If the target year is a leap year and the current
	 * month is March or later then increment the count
	 * as we pass through the extra day in the target year.
	 */
	for (i = curr_year; i < (years + curr_year); i++) {
		int target_year;

		target_year = i + 1;

		if ((leap_year(i) && (curr_month <= 1)) ||
		    (leap_year(target_year) && (curr_month > 1))) {
			leap++;
		}
	}

	/*
	 * Calculate total number of minutes.
	 */
	if (err == 0) {
		total_mins = (((years - leap) * 365 * MINS_IN_DAY) +
		    (leap * 366 * MINS_IN_DAY));
		total_mins += (days * 24 * 60);
		total_mins += (hours * 60);
		total_mins += minutes;

		*expiration = total_mins;
	}

	return (err);
}


/*
 * Convert the changed time and a delta number of mins to
 * a string with the following format:
 *      MM DD YYYY HH:mm
 * Where MM is month, DD is day, HH is hour, YYYY is year,
 * mm is minute.  This routine is used by "sls" to display
 * the end of a file's retention period.
 */
void
MinToStr(
	time_t chgtime,
	long num_mins,
	char *gtime,
	char *str)
{
	int *pdm;
	char zyear[5], zmonth[4], zday[3], zhour[3], zmin[3];
	char *cp, *pyear, result[30];
	char finalhour[3], finalmin[3];
	int  j, chgmonth, dst = 0, begin_year = 0;
	int  year = 0, day = 0, hour = 0;
	int  cyear = 0, cday = 0, chour = 0, cmin = 0;
	int  mins = 0, use_ctime = 0;
	time_t begin = 0, target = 0, now = 0;
	time_t num_secs = 0;


	begin = chgtime/60;
	target = chgtime/60 + num_mins;

	/*
	 * Convert the change time from it's stored form,
	 * seconds since 1/1/1970, to it's numeric equivalent
	 * in years, days, hours, and minutes.
	 *
	 * If the number of minutes do not exceed the maximum
	 * that can be represented (INT_MAX/60), use the
	 * libc function ctime.  If not, generate the string
	 * accounting for time zones, daylight savings time, etc.
	 */
	now = time(NULL);
	if (num_mins <= ((INT_MAX - now)/60)) {
		use_ctime = 1;

		/*
		 * The number of seconds is representable
		 * in an integer.  We can use "ctime" to build
		 * the date.
		 */
		num_secs =  chgtime + (num_mins * 60);
		cp = ctime((time_t *)&num_secs);

		/*
		 * cp now points to a string containing the
		 * day of the week, month, day, time, and year.
		 * eg
		 *	Wed Mar  8 13:44:34 2006
		 *
		 * The character positions is as follows:
		 *
		 *	Wed Mar  8 13:44:34 2006
		 *	012345678911111111112222
		 *			  01234567890123
		 */

		/*
		 * Advance to year picking up the leading space
		 * which will be used to rebuild the string below.
		 */
		pyear = &cp[19];

		/*
		 * Skip day of the week and terminate
		 * string before seconds (at :34 in the
		 * example above).
		 */
		cp += 4;
		cp[12] = '\0';
		strcpy(str, cp);

		/*
		 * Concatenate year, remove new line.
		 */
		strcat(str, pyear);
		str[17] = '\0';
	}

	get_chg_time(chgtime, zyear, zmonth, zday, zhour, zmin);

	begin_year = year = atoi(zyear);

	if (leap_year(year)) {
		pdm = ldays_in_month;
	} else {
		pdm = days_in_month;
	}

	/*
	 * Account for the days in each month.
	 */
	for (chgmonth = 0; chgmonth < 12; chgmonth++) {
		if (strcmp(zmonth, Month[chgmonth]) == 0) {
			break;
		}
		day += pdm[chgmonth];
	}

	day  += (atoi(zday) -1);
	hour = atoi(zhour);
	mins = atoi(zmin) + num_mins;
	cmin = (int)num_mins;

	/*
	 * Convert the change time and the retention
	 * period to year, day, hour, minute format.
	 * Convert the retention period (gtime) to
	 * year, day, hour, minute format.
	 */
	while (mins >= 60) {
		mins -= 60;
		hour++;
		if (cmin >= 60) {
			cmin -= 60;
			chour++;
		}

		if (hour == 24) {
			hour = 0;
			day++;
			if ((day == 366) ||
			    ((day == 365) &&
			    (!leap_year(year - 1) &&
			    (!leap_year(year) && (chgmonth >= 2)) ||
			    (leap_year(year - 1) && (chgmonth >= 2))))) {
				day = 0;
				year++;
			}
		}

		if (chour == 24) {
			chour = 0;
			cday++;
			if ((cday == 366) ||
			    ((cday == 365) &&
			    (!leap_year(year - 1) &&
			    (!leap_year(year) && (chgmonth >= 2)) ||
			    (leap_year(year - 1) && (chgmonth >= 2))))) {
				cday = 0;
				cyear++;
			}
		}
	}

	/*
	 * Build and copyout the retention period.
	 */
	(void) sprintf(result, "%dy, %dd, %dh, %dm", cyear, cday, chour, cmin);

	strcpy(gtime, result);

	if (!use_ctime) {
		/*
		 * The number of seconds isn't representable
		 * in an integer.  Generate the date the hard
		 * way as the number of seconds exceed what can
		 * be represented in Solaris.
		 *
		 * We have the retention period as a date now in
		 * year, day(s), hour(s), and minute(s) format.
		 * Need to convert this to a string of the form
		 * MMM DD CCYY HH:MM.  We have the year so we
		 * need to convert the numbers of days in the
		 * year to the Month day.
		 */
		if (leap_year(year)) {
			pdm = ldays_in_month;
		} else {
			pdm = days_in_month;
		}

		/*
		 * Adjustment for daylight saving time.
		 */
		dst = dst_adjust(begin_year, begin, year, target);
		dst /= 60;

		/*
		 * Format the hour(s):minute(s).
		 */
		hour += dst;
		if (hour == 24) {
			hour = 0;
			day++;
			if ((leap_year(year) && day == 366) ||
			    (!leap_year(year) && day == 365)) {
				day = 1;
				year++;
			}
		}

		/*
		 * Need to add a day to display the current
		 * day correctly.  Calculate the month/day.
		 */
		day++;
		for (j = 0; day > pdm[j] && j < 12; j++) {
			day -= pdm[j];
		}

		if (hour == 0) {
			(void) sprintf(finalhour, "%s", "00");
		} else if (hour < 10) {
			(void) sprintf(finalhour, "0%d", hour);
		} else {
			(void) sprintf(finalhour, "%d", hour);
		}

		if (mins == 0) {
			(void) sprintf(finalmin, "%s", "00");
		} else if (mins < 10) {
			(void) sprintf(finalmin, "0%d", mins);
		} else {
			(void) sprintf(finalmin, "%d", mins);
		}

		/*
		 * Build and copyout the retention end date.
		 */
		(void) sprintf(result, "%s %d %s:%s %d",
		    Month[j], day, finalhour, finalmin, year);
		strcpy(str, result);
	}
}


/*
 * Convert a given date to its equivalent number of minutes
 * since 1/1/1970 (UTC).  The result is used in sfind to
 * find files whose retention period ends after this date.
 */
int
DateToMinutes(
	char    *args,
	long    *mins)
{
	char cyear[5], cmonth[3], cday[3], chour[3], cmin[3];
	int i, year, month, day, hour, minute, dst;
	int leap = 0, total_mins = 0, strsize, *pdm;
	struct timeb	ftz;


	/*
	 * Extract the year, month, day, hour, and min from
	 * the arguments.
	 *
	 * Format for args is YYYYMMDDHHmm.
	 * "YYYYMMDDHHmm" represents the date  YYYY:year, MM:month,
	 * DD:day, HH:hour, mm:minute for the search.
	 */
	strsize = strlen(args);
	if (strsize != 12) {
		return (-1);
	}

	snprintf(cyear, 5, args);
	snprintf(cmonth, 3, args + 4);
	snprintf(cday, 3, args + 6);
	snprintf(chour, 3, args + 8);
	snprintf(cmin, 3, args + 10);

	year = atoi(cyear);
	month = atoi(cmonth);
	day = atoi(cday);
	day--;		/* Current day is not complete */
	hour = atoi(chour);
	minute = atoi(cmin);

	/*
	 * Calculate the number of leap years.
	 */
	if (year < 1970) {
		return (-1);
	}

	for (i = 1970; i < year; i++) {
		if (leap_year(i)) {
			leap++;
		}
	}

	/*
	 * Adjust the number of days based on the month.
	 */
	if (month > 12) {
		return (-1);
	}

	if (leap_year(year)) {
		pdm = ldays_in_month;
	} else {
		pdm = days_in_month;
	}

	for (i = 0; i < (month - 1); i++) {
		day += pdm[i];
	}

	/*
	 * Convert passed date to minutes since 1/1/1970 (UTC).
	 */

	total_mins = (((year - 1970 - leap) * MINS_IN_YEAR) +
	    (leap * MINS_IN_LYEAR));
	total_mins += (day * MINS_IN_DAY);
	total_mins += (hour * MINS_IN_HOUR);
	total_mins += minute;

	/*
	 * Adjust for Universal time (used in the kernel).  We
	 * need the "timezone" variable set.  This is a side-effect
	 * of calling the function localtime().
	 */
	(void) localtime(&ftz.time);

	total_mins += (timezone/60);

	/*
	 * Adjust for daylight savings time.
	 */
	dst = dst_adjust(0, 0, year, total_mins);
	total_mins -= dst;

	*mins = total_mins;

	return (0);
}


#if defined(TEST)
#include <stdio.h>
#include <stdlib.h>

int
main(
	int argc,
	char *argv[])
{
	char buf[STR_FROM_TIME_BUF_SIZE];
	char *trials[] = {
	"1983-06-04",
	"1994-10-18 09:34",
	"Jun 1",
	"Jan 1 10:47",
	"13 Jun 20:47",
	"Jan 1 1994",
	"Jan 1",
	"jan 1 1970 00:00:00",
	"0",
	"",
	NULL
	};
	char **t;

	for (t = trials; *t != NULL; t++) {
		time_t tm;

		tm = StrToTime(*t);
		if (tm <= 0) {
			printf("%16s failed\n", *t);
		} else {
			printf("%16s %16s %d\n", *t,
			    TimeString(tm, buf, sizeof (buf)), tm);
		}
	}
	return (EXIT_SUCCESS);
}

#endif /* defined(TEST) */
