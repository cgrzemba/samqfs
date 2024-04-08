/*
 * setfield.c - Field data manager.
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

#pragma ident "$Revision: 1.27 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Solaris headers. */
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "sam/setfield.h"
#include "sam/lint.h"

/* Public data. */
extern char *program_name;

/* Private functions. */
static int enumVal(char *name, struct EnumEntry *enumTable);
static void errMsg(void(*msgFunc)(int code, char *msg), int msgNum, ...);


/*
 * Set field values to default.
 */
void
SetFieldDefaults(
	void *st,
	struct fieldVals *table)
{
	while (table->FvName != NULL) {
		void *v;
		int type;

		v = (char *)st + table->FvLoc;
		type = table->FvType & ~FV_FLAGS;
		switch (type) {

		case DEFBITS:
			break;

		case PWR2:
		case MUL8:
		case MULL8:
		case INTERVAL:
		case INT:
		case INT16:
		case INT64: {
			struct fieldInt *vals;
			int64_t	val;

			vals = (struct fieldInt *)table->FvVals;
			if (vals->mul == 0) {
				val = vals->def;
			} else {
				val = vals->def * vals->mul;
			}

			switch (type) {
			case PWR2:
			case MUL8:
			case INTERVAL:
			case INT:
				*(int *)v = val;
				break;
			case INT16:
				*(int16_t *)v = val;
				break;
			case MULL8:
			case INT64:
				*(int64_t *)v = val;
				break;
			}
		}
		break;

		case FLOAT:
		case DOUBLE: {
			struct fieldDouble *vals;
			double	val;

			vals = (struct fieldDouble *)table->FvVals;
			if (vals->mul == 0) {
				val = vals->def;
			} else {
				val = vals->def * vals->mul;
			}
			switch (type) {
			case FLOAT:
				*(float *)v = val;
				break;
			case DOUBLE:
				*(double *)v = val;
				break;
			}
		}
		break;

		case ENUM: {
			struct fieldEnum *vals;
			int		e;

			vals = (struct fieldEnum *)table->FvVals;
			e = enumVal(vals->def, vals->enumTable);
			if (e != -1) {
				*(int *)v = e;
			}
		}
		break;

		case CLEARFLAG:
		case SETFLAG:
		case FLAG: {
			struct fieldFlag *vals;

			vals = (struct fieldFlag *)table->FvVals;
			if (strcmp(vals->def, vals->set) == 0) {
				*(uint32_t *)v |= vals->mask;
			} else {
				*(uint32_t *)v &= ~vals->mask;
			}
		}
		break;

		case FSIZE: {
			struct fieldInt *vals;

			vals = (struct fieldInt *)table->FvVals;
			*(int64_t *)v = vals->def;
		}
		break;

		case FUNC: {
			struct fieldFunc *vals;

			vals = (struct fieldFunc *)table->FvVals;
			if (vals->def != NULL) {
				vals->def(v);
			}
		}
		break;

		case MEDIA: {
			struct fieldMedia *vals;

			vals = (struct fieldMedia *)table->FvVals;
			*(media_t *)v = sam_atomedia(vals->def);
		}
		break;

		case STRING: {
			struct fieldString *vals;

			vals = (struct fieldString *)table->FvVals;
			strncpy((char *)v, vals->def, vals->length);
		}
		break;

		default:
			ASSERT(type == DEFBITS);
			break;
		}
		table++;
	}
}



/*
 * Compare field values.
 */
int		/* Number of differences */
SetfieldDiff(
	void *st1,
	void *st2,
	struct fieldVals *table,
	void(*msgFunc)(char *msg))
{
	int diffs;

	diffs = 0;
	while (table->FvName != NULL) {
		void	*v1, *v2;
		int		type;

		v1 = (char *)st1 + table->FvLoc;
		v2 = (char *)st2 + table->FvLoc;
		type = table->FvType & ~FV_FLAGS;
		switch (type) {

		case ENUM:
		case INTERVAL:
		case MUL8:
		case PWR2:
		case INT:
			if (*(int *)v1 == *(int *)v2) {
				goto same;
			}
			break;

		case INT16:
			if (*(int16_t *)v1 == *(int16_t *)v2) {
				goto same;
			}
			break;

		case FSIZE:
		case MULL8:
		case INT64:
			if (*(int64_t *)v1 == *(int64_t *)v2) {
				goto same;
			}
			break;

		case DOUBLE:
			if (*(double *)v1 == *(double *)v2) {
				goto same;
			}
			break;

		case FLOAT:
			if (*(float *)v1 == *(float *)v2) {
				goto same;
			}
			break;

		case SETFLAG:
		case CLEARFLAG:
		case FLAG: {
			struct fieldFlag *vals;

			vals = (struct fieldFlag *)table->FvVals;
			if (((*(uint32_t *)v1 ^ *(uint32_t *)v2)
			    & vals->mask) == 0) {
				goto same;
			}
		}
		break;

		case FUNC: {
			struct fieldFunc *vals;

			vals = (struct fieldFunc *)table->FvVals;
			if (vals->diff != NULL) {
				if (vals->diff(v1, v2)) {
					goto same;
				}
			}
		}
		break;

		case MEDIA:
			if (*(media_t *)v1 == *(media_t *)v2) {
				goto same;
			}
			break;

		case STRING:
			if (strcmp((char *)v1, (char *)v2) == 0) {
				goto same;
			}
			break;

		DEFBITS:
		default:
			goto same;
		}

		if (msgFunc != NULL) {
			msgFunc(table->FvName);
		}
		diffs++;
same:
		table++;
	}
	return (diffs);
}


/*
 * Print the values in a fieldVals defined structure.
 */
void
SetFieldPrintValues(
	char *name,
	struct fieldVals *table,
	void *st)
{
	printf("\n%s:\n", name);
	while (table->FvName != NULL) {
		printf("  %-10s %s\n", table->FvName,
		    SetFieldValueToStr(st, table, table->FvName, NULL, 0));
		table++;
	}
}


/*
 * Print the entries in a fieldVal table.
 */
void
SetFieldPrintTable(
	char *name,
	struct fieldVals *table)
{
	printf("\nFieldValueTable %s:\n", name);
	while (table->FvName != NULL) {
		int		type;

		printf("  %-10s %d  ", table->FvName, table->FvLoc);
		type = table->FvType & ~FV_FLAGS;
		switch (type) {

		case DEFBITS:
			break;

		case ENUM: {
			struct fieldEnum *vals;

			vals = (struct fieldEnum *)table->FvVals;
			printf("ENUM def \"%s\"", vals->def);
		}
		break;

		case FSIZE: {
			struct fieldFsize *vals;

			vals = (struct fieldFsize *)table->FvVals;
			printf("FSIZE def %lld, prec %d",
			    vals->def, vals->prec);
		}
		break;

		case INTERVAL:
		case MUL8:
		case PWR2:
		case INT: {
			struct fieldInt *vals;

			vals = (struct fieldInt *)table->FvVals;
			printf("INT def %lld, min %lld, max %lld, mul %lld",
			    vals->def, vals->min, vals->max, vals->mul);
		}
		break;

		case INT16: {
			struct fieldInt *vals;

			vals = (struct fieldInt *)table->FvVals;
			printf("INT16 def %lld, min %lld, max %lld, mul %lld",
			    vals->def, vals->min, vals->max, vals->mul);
		}
		break;

		case MULL8:
		case INT64: {
			struct fieldInt *vals;

			vals = (struct fieldInt *)table->FvVals;
			printf("INT64 def %lld, min %lld, max %lld, mul %lld",
			    vals->def, vals->min, vals->max, vals->mul);
		}
		break;

		case FLOAT: {
			struct fieldDouble *vals;

			vals = (struct fieldDouble *)table->FvVals;
			printf("Float def %g, min %g, max %g, mul %g",
			    vals->def, vals->min, vals->max, vals->mul);
		}
		break;

		case DOUBLE: {
			struct fieldDouble *vals;

			vals = (struct fieldDouble *)table->FvVals;
			printf("fieldDouble def %g, min %g, max %g, mul %g",
			    vals->def, vals->min, vals->max, vals->mul);
		}
		break;

		case SETFLAG:
		case CLEARFLAG:
		case FLAG: {
			struct fieldFlag *vals;

			vals = (struct fieldFlag *)table->FvVals;
			printf("FLAG def %s, mask 0x%08x, set %s, clear %s",
			    vals->def, vals->mask, vals->set, vals->clear);
		}
		break;

		case MEDIA: {
			struct fieldMedia *vals;

			vals = (struct fieldMedia *)table->FvVals;
			printf("MEDIA def %s", vals->def);
		}
		break;

		case STRING: {
			struct fieldString *vals;

			vals = (struct fieldString *)table->FvVals;
			printf("STRING def \"%s\", length %d",
			    vals->def, vals->length);
		}
		break;

		default:
			printf("??");
		}
		printf("\n");
		table++;
	}
}


/*
 * Set field values.
 */
int
SetFieldValue(
	void *st,
	struct fieldVals *table,
	char *field,
	char *value,
	void(*msgFunc)(int code, char *msg))
{
	boolean_t checkDupDef;
	void *v;
	int type;
	uint32_t *fldDef;

	/*
	 * First table entry can specify no checking for duplicate field
	 * definition.
	 */
	type = table->FvType & ~FV_FLAGS;
	if (type == DEFBITS && !(table->FvType & NO_DUP_CHK)) {
		checkDupDef = TRUE;
	} else {
		checkDupDef = FALSE;
	}

	/*
	 * Check for deprecated options that we'll map into the new
	 * names and issue a warning message.  We'll do this for 1 release.
	 */
	if (strcmp(field, "noarchive") == 0) {
		errMsg(msgFunc, 14114, field, "nosam");  /* Expires in 4.2 */
		field = "nosam";
	}
	if (strcmp(field, "archive") == 0) {
		errMsg(msgFunc, 14114, field, "sam");  /* Expires in 4.2 */
		field = "sam";
	}

	/*
	 * Intercept default retention values for WORM. Convert value
	 * to number of minutes. Check for permanent retention and variable
	 * retention (yearsYdaysDhoursHminutesM) while retaining comapabitly
	 * with the existing format (N minutes).
	 */
	if (strcmp(field, "def_retention") == 0) {
		long mins;
		char tmpstr[32];

		if (strcmp(value, "permanent") == 0 ||
		    strcmp(value, "0") == 0) {
			mins = 0;
		} else if (StrToMinutes(value, &mins) < 0) {
			mins = atoi(value);
			if (mins <= 0) {
				errMsg(msgFunc, 14101, field, value);
				errno = EINVAL;
				return (-1);
			}
		}
		snprintf(tmpstr, sizeof (tmpstr), "%ld", mins);
		strcpy(value, tmpstr);
	}

	fldDef = NULL;
	while (strcmp(field, table->FvName)) {
		type = table->FvType & ~FV_FLAGS;
		if (type == DEFBITS) {
			fldDef = (uint32_t *)(void *)((char *)st +
			    table->FvLoc);
		}
		table++;
		if (table->FvName == NULL) {
			errMsg(msgFunc, 14100, field);
			errno = ENOENT;
			return (-1);
		}
	}

	type = table->FvType & ~FV_FLAGS;
	if (fldDef != NULL && checkDupDef && !(table->FvType & NO_DUP_CHK)) {
		if (*fldDef & table->FvDefBit) {
			errMsg(msgFunc, 14112, field);
			errno = EADDRINUSE;
			return (-1);
		}
	}
	errno = EINVAL;	/* Preset it for error returns. */
	v = ((char *)st + table->FvLoc);

	/*
	 * The SET/CLEARFLAG have no associated value.
	 * Value must be NULL or empty.
	 */
	if (type == SETFLAG || type == CLEARFLAG) {
		struct fieldFlag *vals = (struct fieldFlag *)table->FvVals;

		if (value != NULL && *value != '\0') {
			errMsg(msgFunc, 14006, field);
			return (-1);
		}
		if (type == SETFLAG) {
			*(uint32_t *)v |= vals->mask;
		} else {
			*(uint32_t *)v &= ~vals->mask;
		}
		goto success;
	}

	/*
	 * Check for a value string.
	 */
	if (value == NULL) {
		errMsg(msgFunc, 14113, field);
		return (-1);
	} else if (*value == '\0') {
		errMsg(msgFunc, 14008, field);
		return (-1);
	}

	switch (type) {

	case PWR2:
	case MUL8:
	case MULL8:
	case INT:
	case INT16:
	case INT64: {
		struct fieldInt *vals;
		boolean_t neg;
		int64_t	val;

		/*
		 * Evaluate value.
		 */
		vals = (struct fieldInt *)table->FvVals;
		neg = FALSE;
		if (*value == '-') {
			neg = TRUE;
			value++;
		}
		if (StrToFsize(value, (uint64_t *)&val) != 0) {
			errMsg(msgFunc, 14101, field, value);
			return (-1);
		}
		if (neg) {
			val = -val;
		}

		/*
		 * Process minimum/maximum values.
		 */
		errno = EINVAL;
		if (val < vals->min && val > vals->max) {
			errMsg(msgFunc, 14102, field, vals->min, vals->max);
			return (-1);
		}
		if (val < vals->min) {
			errMsg(msgFunc, 14103, field, vals->min);
			return (-1);
		}
		if (val > vals->max) {
			errMsg(msgFunc, 14104, field, vals->max);
			return (-1);
		}

		if (type == PWR2) {
			/*
			 * Verify that the value is a power of 2.
			 */
			if ((val & (val - 1)) != 0) {
				errMsg(msgFunc, 14110, field);
				return (-1);
			}
		}

		if (type == MUL8 || type == MULL8) {
			/*
			 * Verify that the value is a multiple of 8.
			 */
			if ((val & 7) != 0) {
				errMsg(msgFunc, 14111, field);
				return (-1);
			}
		}

		/*
		 * Process multiplier.
		 */
		if (vals->mul != 0) {
			val *= vals->mul;
		}

		/*
		 * Set field to value.
		 */
		switch (type) {
		case PWR2:
		case MUL8:
		case INT:
			*(int *)v = val;
			break;

		case INT16:
			*(int16_t *)v = val;
			break;

		case MULL8:
		case INT64:
			*(int64_t *)v = val;
			break;
		}
	}
	break;

	case FLOAT:
	case DOUBLE: {
		struct fieldDouble *vals;
		char *p;
		double val;

		/*
		 * Evaluate value.
		 */
		vals = (struct fieldDouble *)table->FvVals;
		errno = 0;
		val = strtod(value, &p);
		if (*p != '\0' || errno == ERANGE || errno == EINVAL) {
			errMsg(msgFunc, 0, "error converting '%s' to %s",
			    value, table->FvName);
			if (errno == 0) {
				errno = EINVAL;
			}
			return (-1);
		}

		/*
		 * Process minimum/maximum values.
		 */
		errno = EINVAL;
		if (val < vals->min && val > vals->max) {
			errMsg(msgFunc, 14105, field, vals->min, vals->max);
			return (-1);
		}
		if (val < vals->min) {
			errMsg(msgFunc, 14106, field, vals->min);
			return (-1);
		}
		if (val > vals->max) {
			errMsg(msgFunc, 14107, field, vals->max);
			return (-1);
		}

		/*
		 * Process multiplier.
		 */
		if (vals->mul != 0) {
			val *= (double)vals->mul;
		}

		/*
		 * Set field to value.
		 */
		*(float *)v = val;
	}
	break;

	case ENUM: {
		struct fieldEnum *vals;
		int		e;

		vals = (struct fieldEnum *)table->FvVals;
		e = enumVal(value, vals->enumTable);
		if (e == -1) {
			errMsg(msgFunc, 14101, field, value);
			return (-1);
		}
		*(int *)v = e;
	}
	break;

	case FLAG: {
		struct fieldFlag *vals;

		vals = (struct fieldFlag *)table->FvVals;
		if (strcasecmp(value, vals->set) == 0) {
			*(uint32_t *)v |= vals->mask;
		} else if (strcasecmp(value, vals->clear) == 0) {
			*(uint32_t *)v &= ~vals->mask;
		} else {
			errMsg(msgFunc, 14101, field, value);
			return (-1);
		}
	}
	break;

	case FSIZE: {
		int64_t	val;

		if (StrToFsize(value, (uint64_t *)&val) != 0) {
			errMsg(msgFunc, 14101, field, value);
			return (-1);
		}
		*(int64_t *)v = val;
	}
	break;

	case FUNC: {
		struct fieldFunc *vals;

		vals = (struct fieldFunc *)table->FvVals;
		if (vals->set != NULL) {
			char	buf[64];

			if (vals->set(v, value, buf, sizeof (buf)) != 0) {
				errMsg(msgFunc, 0, buf);
				return (-1);
			}
		}
	}
	break;

	case INTERVAL: {
		struct fieldInt *vals;
		int		val;

		vals = (struct fieldInt *)table->FvVals;
		if (StrToInterval(value, &val) != 0) {
			if (errno == ERANGE) {
				errMsg(msgFunc, 14104, field,
				    (longlong_t)INT_MAX);
			} else {
				errMsg(msgFunc, 14101, field, value);
			}
			return (-1);
		}

		/*
		 * Process minimum/maximum values.
		 */
		if (val < vals->min || val > vals->max) {
			errMsg(msgFunc, 14102, field, vals->min, vals->max);
			return (-1);
		}

		*(int *)v = val;
	}
	break;

	case MEDIA: {
		int		m;

		m = sam_atomedia(value);
		if (m == 0) {
			errMsg(msgFunc, 14108, table->FvName, value);
			return (-1);
		}
		*(media_t *)v = m;
	}
	break;

	case STRING: {
		struct fieldString *vals;

		vals = (struct fieldString *)table->FvVals;
		if (strlen(value) >= vals->length) {
			errMsg(msgFunc, 14109, field, vals->length - 1);
			return (-1);
		}
		strncpy((char *)v, value, vals->length - 1);
	}
	break;

	DEFBITS:
	default:
		ASSERT(type == DEFBITS);
		return (-1);
	}

success:
	if (fldDef != NULL) {
		*fldDef |= table->FvDefBit;
	}
	errno = 0;
	return (0);
}


/*
 * Return the value in a fieldVals defined field.
 */
char *
SetFieldValueToStr(
	void *st,
	struct fieldVals *table,
	char *field,
	char *buf,
	int bufsize)
{
	static char our_buf[32];
	void	*v;
	int		type;

	if (buf == NULL) {
		buf = our_buf;
		bufsize = sizeof (our_buf);
	}
	if (bufsize < sizeof (our_buf)) {
		*buf = '\0';
		return (buf);
	}
	while (strcmp(field, table->FvName) != 0) {
		table++;
		if (table->FvName == NULL) {
			snprintf(buf, bufsize, GetCustMsg(14100), field);
			errno = ENOENT;
			return (buf);
		}
	}
	errno = 0;
	v = (char *)st + table->FvLoc;
	type = table->FvType & ~FV_FLAGS;
	switch (type) {

	case DOUBLE:
		snprintf(buf, bufsize, "%g", *(double *)v);
		break;

	case ENUM: {
		struct fieldEnum *vals;
		int e;
		int i;

		vals = (struct fieldEnum *)table->FvVals;
		e = *(int *)v;
		for (i = 0; vals->enumTable[i].EeName != NULL; i++) {
			if (vals->enumTable[i].EeName == NULL) {
				snprintf(buf, bufsize, "%d", e);
				break;
			}
			if (vals->enumTable[i].EeValue == e) {
				buf = vals->enumTable[i].EeName;
				break;
			}
		}
	}
	break;

	case FLOAT:
		snprintf(buf, bufsize, "%g", *(float *)v);
		break;

	case SETFLAG:
	case CLEARFLAG:
	case FLAG: {
		struct fieldFlag *vals;

		vals = (struct fieldFlag *)table->FvVals;
		buf = (*(uint32_t *)v & vals->mask) ? vals->set : vals->clear;
	}
	break;

	case FSIZE: {
		struct fieldFsize *vals;

		vals = (struct fieldFsize *)table->FvVals;
		(void) StrFromFsize(*(int64_t *)v, vals->prec, buf, bufsize);
		while (*buf == ' ') {
			buf++;
		}
	}
	break;

	case FUNC: {
		struct fieldFunc *vals;

		vals = (struct fieldFunc *)table->FvVals;
		if (vals->tostr != NULL) {
			return (vals->tostr(v, buf, bufsize));
		}
	}
	break;

	case MUL8:
	case PWR2:
	case INT:
		snprintf(buf, bufsize, "%d", *(int *)v);
		break;

	case INT16:
		snprintf(buf, bufsize, "%d", *(int16_t *)v);
		break;

	case MULL8:
	case INT64:
		snprintf(buf, bufsize, "%lld", *(int64_t *)v);
		break;

	case INTERVAL: {
		(void) StrFromInterval(*(int *)v, buf, bufsize);
	}
	break;


	case MEDIA:
		buf = sam_mediatoa(*(media_t *)v);
		break;

	case STRING:
		buf = (char *)v;
		break;

	DEFBITS:
	default:
		buf = "??";
	}
	return (buf);
}


/*
 * Return value for enum.
 */
static int
enumVal(
	char *name,
	struct EnumEntry *enumTable)
{
	int		i;

	for (i = 0; enumTable[i].EeName != NULL; i++) {
		if (strcmp(enumTable[i].EeName, name) == 0) {
			return (enumTable[i].EeValue);
		}
	}
	return (-1);
}


/*
 * Compose message.
 */
static void
errMsg(
	void(*msgFunc)(int code, char *msg),
	int msgNum,				/* Message catalog number. */
	...)
{
	static char	msg_buf[128];
	va_list args;
	char	*msg;

	va_start(args, msgNum);
	if (msgNum != 0) {
		msg = GetCustMsg(msgNum);
	} else {
		msg = va_arg(args, char *);
	}
	vsnprintf(msg_buf, sizeof (msg_buf), msg, args);
	va_end(args);
	if (msgFunc != NULL) {
		msgFunc(0, msg_buf);
	} else {
		fprintf(stderr, "%s: %s\n", program_name, msg_buf);
	}
}


#if defined(TEST)

/* The fields to use. */
static struct test {
	int dec;
	int decmul;
	int oct;
	int neg;
	int big;
	uint32_t flgs;
	float flt;
	char str[17];
	media_t	med;
	short Short;
	int pwr2;
	int mul8;
	int dup;
	uint32_t fldFlags;
	int enumVal;
	fsize_t	fsize;
	int func;
	int interval;
} tst;

/* enum values */
static struct EnumEntry enumTable[] = {
	{ "none", 0 },
	{ "ready", 16 },
	{ "set", 256 },
	{ "go", 1000 },
	{ NULL }
};

/* fieldFlag bits. */
enum {
	TF_ONE = 0x1,
	TF_TWO = 0x2,
	TF_FOUR = 0x4,
	TF_EIGHT = 0x8
};

/* Functions. */
static void def(void *v);
static int diff(void *v1, void *v2);
static int(set)(void *v, char *value, char *buf, int bufsize);
static char *(tostr)(void *v, char *buf, int bufsize);

/* Value definitions. */
struct fieldInt t0 = { 23, 4, 48, 0 };
struct fieldInt t1 =	{ 23, 4, 48, 1024 };
struct fieldInt t2 =	{ 0123, INT_MIN, INT_MAX, 0 };
struct fieldInt t3 =	{ -1086, -10000, INT_MAX, 0 };
struct fieldInt t4 =	{ 350000, 0, INT_MAX, 0 };
struct fieldFlag t5 = { TF_ONE, "TRUE", "TRUE", "FALSE" };
struct fieldFlag t6 = { TF_TWO, "OFF", "ON", "OFF" };
struct fieldFlag t7 = { TF_FOUR, "ON", "YES", "NO" };
struct fieldFlag t8 = { TF_EIGHT, "OFF", "ON", "OFF" };
struct fieldDouble t9 = { 23.57, 0.1, 1000, 0 };
struct fieldString t10 = { "Default string", 17 };
struct fieldMedia t11 = { "mo" };
struct fieldInt t12 = { 43, SHRT_MIN, SHRT_MAX, 0 };
struct fieldInt t13 = { 64, 16, 2048, 1024 };
struct fieldInt t14 = { 8, 0, 512, 1024 };
struct fieldInt t15 = { 0, 0, 512 };
struct fieldEnum t16 = { "set", enumTable };
struct fieldFsize t17 = { 0, 2 };
struct fieldFunc t18 = { def, diff, set, tostr };
struct fieldInt t19 = { 23, 10, 4800, 0 };

static struct fieldVals Test[] = {
	{ "", DEFBITS, offsetof(struct test, fldFlags), &t0 },
	{ "dec", INT, offsetof(struct test, dec), &t0 },
	{ "decmul", INT, offsetof(struct test, dec), &t1 },
	{ "oct", INT, offsetof(struct test, oct), &t2 },
	{ "neg", INT, offsetof(struct test, neg), &t3 },
	{ "big", INT, offsetof(struct test, big), &t4 },
	{ "one", FLAG, offsetof(struct test, flgs), &t5 },
	{ "two", FLAG, offsetof(struct test, flgs), &t6 },
	{ "four", FLAG, offsetof(struct test, flgs), &t7 },
	{ "eight", SETFLAG, offsetof(struct test, flgs), &t8 },
	{ "flt", FLOAT, offsetof(struct test, flt), &t9 },
	{ "str", STRING, offsetof(struct test, str), &t10 },
	{ "med", MEDIA, offsetof(struct test, med), &t11 },
	{ "short", INT, offsetof(struct test, Short), &t12 },
	{ "pwr2", PWR2, offsetof(struct test, pwr2), &t13 },
	{ "mul8", MUL8, offsetof(struct test, mul8), &t14 },
	{ "dup", INT, offsetof(struct test, dup), &t15, 0x10 },
	{ "enum", ENUM, offsetof(struct test, enumVal), &t16 },
	{ "fsize", FSIZE, offsetof(struct test, fsize), &t17 },
	{ "func", FUNC, offsetof(struct test, func), &t18 },
	{ "interval", INTERVAL, offsetof(struct test, interval), &t19 },
	{ NULL }
};

char *program_name = "test_setfield";

void
msgFunc(
	int code,
	char *msg)
{
	printf(" *** %s\n", msg);
}

int
main(void)
{
	struct {
		char	*field;
		char	*value;
		boolean_t good;
	} trials[] = {
		{ "bad", "1", FALSE },
		{ "dec", "3", FALSE },
		{ "dec", "4", TRUE },
		{ "dec", "48", TRUE },
		{ "dec", "49", FALSE },
		{ "neg", "-3034", TRUE },
		{ "big", "2147483647", TRUE },
		{ "big", "3.5M", TRUE },
		{ "one", "bad", FALSE },
		{ "one", "FALSE", TRUE },
		{ "two", "OFF", TRUE },
		{ "two", NULL, FALSE },
		{ "four", "YES", TRUE },
		{ "eight", "ANYTHING", FALSE },
		{ "eight", NULL, TRUE },
		{ "flt", "-4.358e2", FALSE },
		{ "flt", "4.358e2", TRUE },
		{ "str", "Now is the time for a long string", FALSE },
		{ "str", "A good string", TRUE },
		{ "med", "xx", FALSE },
		{ "med", "lt", TRUE },
		{ "short", "32768", FALSE },
		{ "short", "-32769", FALSE },
		{ "pwr2", "56", FALSE },
		{ "mul8", "15", FALSE },
		{ "dup", "1", TRUE },
		{ "dup", "1", FALSE },
		{ "enum", "bad", FALSE },
		{ "enum", "go", TRUE },
		{ "fsize", "4.35G", TRUE },
		{ "func", "GOOD", TRUE },
		{ "func", "not_good", FALSE },
		{ "interval", "32m", TRUE },

		NULL
	};
	char	buf[80];
	char	*val;
	int		n;
	int		errors = 0;

	SetFieldPrintTable("test", Test);
	SetFieldDefaults(&tst, Test);
	SetFieldPrintValues("Default Test", Test, &tst);

	printf("\n\nTest trials:\n");
	for (n = 0; trials[n].field != NULL; n++) {
		printf("%d: %s = %s  ", n+1, trials[n].field,
		    trials[n].value == NULL ? "NULL" : trials[n].value);
		if (SetFieldValue(&tst, Test,
		    trials[n].field, trials[n].value, msgFunc) == -1) {
			if (trials[n].good) {
				fprintf(stderr, "Trial %d failed\n", n+1);
				errors++;
			}
		} else {
			printf("\n");
			if (!trials[n].good) {
				fprintf(stderr, "Trial %d failed\n", n+1);
				errors++;
			}
		}
	}
	SetFieldPrintValues("Results", Test, &tst);

	val = SetFieldValueToStr(&tst, Test, "not_here", buf, sizeof (buf));
	printf("%s %d\n", val, errno);
	if (errors == 0) {
		return (EXIT_SUCCESS);
	}
	fprintf(stderr, "Test setfield Failed errors = %d.\n", errors);
	return (EXIT_FAILURE);
}


static void
def(
	void *v)
{
	int *val = (int *)v;

	*val = 6676;
}


static int
diff(
	void *v1,
	void *v2)
{
	int *val1 = (int *)v1;
	int *val2 = (int *)v2;

	return (*val1 == *val2);
}


static int
set(
	void *v,
	char *value,
	char *buf,
	int bufsize)
{
	int *val = (int *)v;
	int retval;

	if (strcmp(value, "GOOD") == 0) {
		*val = 6294;
		retval = 0;
	} else {
		strncpy(buf, "value must be GOOD", bufsize-1);
		errno = EINVAL;
		retval = -1;
	}
	return (retval);
}


static char *
tostr(
	void *v,
	char *buf,
	int bufsize)
{
	int *val = (int *)v;
	char *ret;

	if (*val == 6676) {
		ret = "default";
	} else if (*val == 6294) {
		ret = "GOOD";
	} else {
		ret = "don't know";
	}
	return (ret);
}


#endif /* defined(TEST) */
