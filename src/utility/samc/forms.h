
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
#ifndef _FORMS_H
#define	_FORMS_H

#pragma ident	"$Revision: 1.11 $"

#include <form.h>

typedef FIELD * (* fieldbuilder) ();

typedef enum {
	T_INT,		/* int */
	T_INT16,	/* int16_t */
	T_UINT,		/* uint_t */
	T_UINT16,	/* uint16_t */
	T_LNG,		/* long */
	T_LLNG,		/* long long */
	T_STR,		/* string */
	T_FLT,		/* float */
	T_DBL,		/* double */
	T_BLN,		/* boolean_t */
	/* SAM-specific types */
	T_SIZE,		/* number[k|m|g] <-> fsize_t (bytes) */
	T_TIME,		/* number[h|m|...] <-> uint_t (secs.) */
	T_JOIN,		/* join_enum <-> number */
	T_SORT,		/* sort_enum <-> number */
	T_OFCP,		/* offlinecp_enum <-> number */
	T_AGE,		/* on_off_enum <-> prio_age_type */
	T_MAX
} TYPEID;

/*
 * information required to convert the field form data (char ptr)
 * to the appropriate data type
 */
typedef struct {
	TYPEID typeid;	/* form field data must be converted to this type */
			/* (this is the type of the structure field) */

	size_t dataoffset;	/* offset of the data within structure */

	size_t maskoffset;	/* offset of the mask field */
	uint32_t mask;		/* the mask bit for this field */

	/*
	 * if more then one data structure shown in the form,
	 * then pointers have to be used instead of offsets.
	 * in this case, dataoffset & maskoffset must be set to be equal.
	 */
	void *dataptr;
	void *maskptr;
} convinfo;

typedef struct {
	int fs_lic;	/* minimum license required for this field */
			/* see license.h */
	fieldbuilder mkfield;	/* function to create FIELD */
	int rows;		/* number of rows */
	int cols;		/* number of columns */
	int frow;		/* first row */
	int fcol;		/* first column */
	char *val;		/* field value */
	/* used by editable fields only */
	char *dfl;		/* field default value */
	convinfo *cinfo;	/* type conversion information */
	/* used by numeric fields only (TYPE_INTEGER or TYPE_NUMERIC) */
	int prec;		/* precision */
	double min;
	double max;
	/* used by enumerated fields (TYPE_ENUM) */
	char ** keywords;
	/* used by TYPE_REGEXP fields */
	char *regexp;

} FIELD_RECORD;


FIELD *mklabel(FIELD_RECORD *rec);
FIELD *mknewpage();

/* editable field contructors */

/*
 * create a one line editable field.
 * if rows>1 then it specifies the maximum length (growable field)
 * and the field will scroll horizontally
 */
FIELD *mkstring(FIELD_RECORD *rec);
/* mkstring blanks allowed */
FIELD *mkstrbla(FIELD_RECORD *rec);
/* create an int field */
FIELD *mkint(FIELD_RECORD *rec);
/* create a double field */
FIELD *mkdbl(FIELD_RECORD *rec);
/* create a enumerated field */
FIELD *mkenum(FIELD_RECORD *rec);
/* create a regular expression field */
FIELD *mkregexp(FIELD_RECORD *rec);
/* special types of regular expresion fields */
FIELD *mksize(FIELD_RECORD *rec); /* file size (e.g. 100k) */
FIELD *mktimein(FIELD_RECORD *rec); /* time interval (e.g. 10m) */
/*
 * create a special type of enumerated field: the rest of the fields in the
 * form will be hidden/displayed when this field changes its value.
 * In this context, the order of the fields is the order in which they are
 * defined, NOT the order in which they show on the screen.
 */
FIELD *mkenumtrig(FIELD_RECORD *rec);

/*
 * create fields but not show them when the form is first displayed.
 * should not be used in forms with input-dependent fields.
 */
FIELD *mkhlabel(FIELD_RECORD *rec);
FIELD *mkhstring(FIELD_RECORD *rec);
FIELD *mkhint(FIELD_RECORD *rec);
FIELD *mkhdbl(FIELD_RECORD *rec);
FIELD *mkhenum(FIELD_RECORD *rec);
FIELD *mkhenumtrig(FIELD_RECORD *rec);

FIELD **
mkfields(FIELD_RECORD recs[]);

/* reset 'val' field to NULL for all records (except labels) */
void reset_field_recs_val(FIELD_RECORD recs[], int numrecs);
/* initialize 'val' field */
void init_field_recs_val(FIELD_RECORD recs[], void *baseaddr);
/* initialize 'dfl' field */
void init_field_recs_dfl(FIELD_RECORD recs[], void *baseaddr);

void
display_form(FORM *f);

/*
 * return -1 if quit
 */
int
process_form(FORM *, char *title);

/*
 * take data from the fields modified by user (strings),
 * convert it to the appropriate type (as described in convinfo structure
 * pointed by userptr) and put it back at the user-specified location
 * (either a structure pointed by baseaddr or - if baseaddr is NULL -
 * the locations specified in userptr)
 */
void
convert_form_data(FORM *f, char *baseaddr);

/*
 * frees all fields and the form itself
 */
int
destroy_form(FORM *f);

char *
field_val(FIELD *fld);

/* return first space-terminated substring in a newly allocated string */
char *
getfirststr(const char *str);
#endif /* _FORMS_H */
