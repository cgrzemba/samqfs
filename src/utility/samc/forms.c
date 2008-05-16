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
#pragma ident	"$Revision: 1.9 $"

/* Solaris header files */
#include <stdlib.h>
#include <string.h>

/* other header files */
#include <form.h>
#include "mgmt/types.h"		/* fsize_t & reset_values defined here */
#include "mgmt/release.h"	/* age_prio_type needed for conversion */
#include "config/cparamsdefs.h"
#include "samc.h"
#include "forms.h"
#include "log.h"

#define	VAL_BUF	0
#define	DFL_BUF	1
#define	DFL_STR "dflt"		/* use default value */
#define	AUTO_STR "auto"		/* automatically pick a value */
#define	MAX_FIELDS 100
#define	FRM_BORDER 2

/* default settings for TYPE_ENUM fields */
int checkunique = TRUE;	/* auto-complete field value only if unique match */
int checkcase   = TRUE;	/* case sensitive */

/*
 * need to check against the current license in order to decide
 * which fields are displayed
 */
extern int LICENSE;

FIELD *
mklabel(FIELD_RECORD *rec) {
	FIELD * f;
	PTRACE(3, "forms.c:mklabel(%x,%s)", rec, Str(rec->val));
	if (NULL != (f = new_field(1, strlen(rec->val),
	    rec->frow, rec->fcol, 0, 0))) {
		    set_field_buffer(f, VAL_BUF, rec->val);
		    field_opts_off(f, O_ACTIVE);
	    }
	return (f);
}
FIELD *
mkhlabel(FIELD_RECORD *rec) {
	FIELD * f = mklabel(rec);
	if (f)
		field_opts_off(f, O_VISIBLE);
	return (f);
}

FIELD *
mknewpage() {
	FIELD *f = new_field(1, 1, 1, 1, 0, 0);
	field_opts_off(f, O_ACTIVE);
	set_new_page(f, TRUE);
	return (f);
}

static FIELD *
mkeditable(FIELD_RECORD *rec) {
	FIELD * f = new_field(rec->rows, rec->cols,
	    rec->frow, rec->fcol, 0,
	    1);	/* one additional buffer to store default value */
	if (f) {
		set_field_back(f, A_UNDERLINE);
		field_opts_off(f, O_AUTOSKIP);
		set_field_buffer(f, DFL_BUF, rec->dfl);

		TRACE("forms.c: rec->val: %s", (rec->val ? rec->val : "null"));
		/* if no value specified then use default */
		if (rec->val == NULL && rec->dfl != NULL) {
			char *valbuf = (char *)malloc(strlen(DFL_STR) +
			    strlen(rec->dfl) + 4);
			sprintf(valbuf, "%s (%s)", DFL_STR, rec->dfl);
			set_field_buffer(f, VAL_BUF, valbuf);
			free(valbuf);
		} else
			set_field_buffer(f, VAL_BUF, rec->val);

		/* use userptr to store type conversion information */
		set_field_userptr(f, (void *)rec->cinfo);

		/* set field_status to FALSE = not changed by user */
		set_field_status(f, FALSE);
		//	TRACE("forms.c: ed.field created");

	}
	return (f);
}

FIELD *
mkstring(FIELD_RECORD *rec) {
	FIELD *f;
	int maxlen = 0;
	if (1 != rec->rows) {
		maxlen = rec->rows; /* else field is not growable */
		rec->rows = 1;
	}
	f = mkeditable(rec);
	if (maxlen) {		/* if growable (scrollable) field */
		field_opts_off(f, O_STATIC);	/* make it so and */
		set_max_field(f, maxlen);	/* set max buf len */
	}
	return (f);
}
FIELD *
mkhstring(FIELD_RECORD *rec) {
	FIELD *f = mkstring(rec);
	if (f)
		field_opts_off(f, O_VISIBLE);
	return (f);
}

static int check() { return (TRUE); }
/* this type allows blanks inside a string */
static FIELDTYPE * TYPE_STRWBLANKS = NULL;

FIELD *
mkstrbla(FIELD_RECORD *rec) {
	FIELD *f = mkstring(rec);
	TRACE("forms.c:MKSTRBLAnks");
	if (f) {
		if (!TYPE_STRWBLANKS)
			TYPE_STRWBLANKS = new_fieldtype(check, check);
		set_field_type(f, TYPE_STRWBLANKS);
	}
	return (f);
}

FIELD *
mkint(FIELD_RECORD *rec) {
	FIELD *f = mkeditable(rec);
	set_field_type(f, TYPE_INTEGER,
	    rec->prec, (long)rec->min, (long)rec->max);
	return (f);
}
FIELD *
mkhint(FIELD_RECORD *rec) {
	FIELD *f = mkint(rec);
	if (f)
		field_opts_off(f, O_VISIBLE);
	return (f);
}


FIELD *
mkdbl(FIELD_RECORD *rec) {
	FIELD *f = mkeditable(rec);
	set_field_type(f, TYPE_NUMERIC, rec->prec, rec->min, rec->max);
	return (f);
}
FIELD *
mkhdbl(FIELD_RECORD *rec) {
	FIELD *f = mkdbl(rec);
	if (f)
		field_opts_off(f, O_VISIBLE);
	return (f);
}

FIELD *
mkenum(FIELD_RECORD *rec) {
	FIELD *f = mkeditable(rec);
	// TRACE("mkenum %x", rec->keywords);
	set_field_type(f, TYPE_ENUM,
	    rec->keywords, checkcase, checkunique);
	/* user cannot directly edit by typing into the field */
	field_opts_off(f, O_EDIT);
	return (f);
}
FIELD *
mkhenum(FIELD_RECORD *rec) {
	FIELD *f = mkenum(rec);
	if (f)
		field_opts_off(f, O_VISIBLE);
	return (f);
}

FIELD *
mkenumtrig(FIELD_RECORD *rec) {
	FIELD *f;
	TRACE("forms.c:mkenumtrig %x", rec->keywords);
	f = mkenum(rec);
	/*
	 * we do not allow enum. fields to grow.
	 * For these fields, we are using the O_STATIC case to flag the fact
	 * that changing this field triggers changes in the form display:
	 * the rest of the fields in this form will be hidden/displayed
	 * when this field changes its value.
	 */
	field_opts_off(f, O_STATIC);
	return (f);
}
FIELD *
mkhenumtrig(FIELD_RECORD *rec) {
	FIELD *f = mkenumtrig(rec);
	if (f)
		field_opts_off(f, O_VISIBLE);
	return (f);
}

FIELD *
mkregexp(FIELD_RECORD *rec) {
	FIELD *f = mkeditable(rec);
	set_field_type(f, TYPE_REGEXP, rec->regexp);
	return (f);
}

FIELD *
mksize(FIELD_RECORD *rec) {
	FIELD *f = mkeditable(rec);
	set_field_type(f, TYPE_REGEXP, "^ *[1-9][0-9]*[bkMGT]{0,1} *$");
	return (f);
}

FIELD *
mktimein(FIELD_RECORD *rec) {
	FIELD *f = mkeditable(rec);
	set_field_type(f, TYPE_REGEXP, "^ *[1-9][0-9]*[smhdwy]{1} *$");
	return (f);
}

static FIELD *fields[MAX_FIELDS];

FIELD **
mkfields(FIELD_RECORD recs[]) {
	FIELD **f = fields;
	int i;
	TRACE("forms.c: mkfields");

	for (i = 0; recs[i].mkfield; ++i, ++f) {
		TRACE("forms.c:mkfields:i=%d", i);
		*f = (recs[i].mkfield) (& recs[i]);
		if (NULL == *f) {
			TRACE("forms.c: cannot create field");
			printw("\ncannot create form field!\n");
			exit(1);
		}
		/* if feature is not licensed, then do not display the field */
		if ((LICENSE & recs[i].fs_lic) != recs[i].fs_lic) {
			field_opts_off(*f, O_VISIBLE);
			field_opts_off(*f, O_ACTIVE);
		}
	}

	*f = (FIELD *) 0;
	TRACE("forms.c: mkfields done");
	return (fields);
}

char *
field_val(FIELD *fld) {
	if ((field_type(fld) == TYPE_STRWBLANKS) &&
	    /* if user defined-type was initialized */
	    (TYPE_STRWBLANKS != NULL)) {
		static char buf[200];
		int i;
		strcpy(buf, field_buffer(fld, VAL_BUF));
		i = strlen(buf) - 1;
		while (i && buf[i] == ' ')
			i--;
		if (i == 0)
			buf[0] = '\0';
		else
			buf[++i] = '\0';
		return (buf);
	} else
		return (getfirststr(field_buffer(fld, VAL_BUF)));
}

int
is_visible(FIELD *fld) {
	return (O_VISIBLE & field_opts(fld));

}

int
is_trig(FIELD *fld) {
	if (O_STATIC & field_opts(fld))
		return (0);	/* this field should trigger display changes */
	else
		return (-1);
}

char *
getfirststr(const char *src) {
	int len = strcspn(src, "! ");
	static char firststr[80];
	strncpy(firststr, src, len);
	firststr[len] = '\0';
	return (firststr);
}

void
display_form(FORM *f) {

	WINDOW * w;
	int rows, cols;

	scale_form(f, &rows, &cols); /* get dimensions of form */

	/* create form window */

	if (w = newwin(rows + 4, COLS - 2, 0, 0)) {
		set_form_win(f, w);
		set_form_sub(f, derwin(w, rows, cols,
		    FRM_BORDER, FRM_BORDER));
		keypad(w, TRUE);
	} else
		perror("error return from newwin");

	if (post_form(f) != E_OK) /* post form */
		perror("error return from post_form");
	else
		wrefresh(w);
}

void
display_form_title(FORM *f, char *title, int pages) {
	char full_title[80];
	if (pages > 1)
		sprintf(full_title,
		    "%s (page %d/%d)", title, form_page(f) + 1, pages);
	else
		strcpy(full_title, title);
	mvwprintw(form_win(f), 2, 1, full_title);
}

/* void */
/* edit_crt_field(FORM *f, int c) { */
/* 	FIELD *fld; */
/* 	char tmp[80]; */
/* 	long len; */

/* 	fld = current_field(f); */
/* 	strcpy(tmp, field_buffer(fld, VAL_BUF)); */
/* 	len = (long) (char *) field_userptr(fld); */

/* 	if (isalnum(c)) { */
/* 		tmp[len++] = c; */
/* 		tmp[len] = 0; */
/*	    } */
/* 	if (c == KEY_backspace || c == KEY_BACKSPACE)  */
/* 		if (!len) */
/* 			beep(); */
/* 		else */
/* 			tmp[--len] = 0;	 */
/* 	set_field_buffer(fld, VAL_BUF, tmp); */
/* 	form_driver(f, REQ_END_FIELD); */
/* 	set_field_userptr(fld, (void *)len); */
/* } */


static boolean_t
convert_to_bool(char *val) {
	if (0 == strncmp(val, "yes", 3) || 0 == strncmp(val, "on", 2))
		return (B_TRUE);
	else
		return (B_FALSE);
}

static age_prio_type
convert_to_agetype(char *detailed_onoff) {
	if (0 == strcmp(detailed_onoff, "on"))
		return (DETAILED_AGE_PRIO);
	if (0 == strcmp(detailed_onoff, "off"))
		return (SIMPLE_AGE_PRIO);
	return (NOT_SET);
}
static char *
convert_agetype(age_prio_type prtype) {
	switch (prtype) {
	case SIMPLE_AGE_PRIO:
		return ("off");
	case DETAILED_AGE_PRIO:
		return ("on");

	default:
		return ("");
	}
}
/*
 * the 3 functions below convert names to IDs by doing a lookup in one of the
 * tables defined in cparamsdef.h
 */
join_method_t
joinmetid(char *name) {
	int crt = 0;
	while (join_tbl[crt].EeName) {
		if (0 == strcmp(name, join_tbl[crt].EeName))
			return (join_tbl[crt].EeValue);
		crt++;
	}
	return (-1);
}
sort_method_t
sortmetid(char *name) {
	int crt = 0;
	while (sort_method_tbl[crt].EeName) {
		if (0 == strcmp(name, sort_method_tbl[crt].EeName))
			return (sort_method_tbl[crt].EeValue);
		crt++;
	}
	return (-1);
}
offline_copy_method_t
offcpmetid(char *name) {
	int crt = 0;
	while (offline_cpy_tbl[crt].EeName) {
		if (0 == strcmp(name, offline_cpy_tbl[crt].EeName))
			return (offline_cpy_tbl[crt].EeValue);
		crt++;
	}
	return (-1);
}

void
convert_form_data(FORM *form, char *baseaddr) {
	FIELD **fields, *crtfld;
	int nflds = field_count(form),
	    i;
	char *fldval;
	void *dataaddr, *maskaddr;
	convinfo *cinf;
	boolean_t reset2dfl;

	TRACE("forms.c: convert_form_data(%x, %x)");
	fields = form_fields(form);
	if (NULL == baseaddr)
		TRACE("forms.c:warning:baseaddr is null");
	for (i = 0; i < nflds; i++, fields++) {
		crtfld = *fields;
		reset2dfl = B_FALSE;

		/* skip non-editable fields (labels) */
		if (! (field_opts(crtfld) & O_ACTIVE)) {
			PTRACE(3, "forms.c:label field skipped");
			continue;
		}

		/* extract field conversion information from userptr */
		if (NULL == field_userptr(crtfld)) {
			PTRACE(3, "forms.c:convinfo=null.skipped");
			continue;
		}
		cinf = (convinfo *) (void *)field_userptr(crtfld);

		fldval = field_val(crtfld);
		TRACE("forms.c:fieldval:%s.", fldval);

		if (strlen(fldval) == 0) /* no value specified */
			continue;

		TRACE("forms.c: cinf=(type:%d,doffs:%d,moffs:%d,msk:%d,"
		    "dptr:%x,mptr:%x)",
		    cinf->typeid, cinf->dataoffset,
		    cinf->maskoffset, cinf->mask,
		    cinf->dataptr, cinf->maskptr);

		/* check if should use ptr. fields instead of base+offsets */
		if (cinf->dataptr != NULL) {
			TRACE("forms.c: using PTR");
			dataaddr = cinf->dataptr;
			maskaddr = cinf->maskptr;
		} else {
			TRACE("forms.c: using OFFS");
			dataaddr = baseaddr + cinf->dataoffset;
			maskaddr = baseaddr + cinf->maskoffset;
		}

		/* check if default */
		if (0 == strncmp(fldval, DFL_STR, strlen(DFL_STR))) {
			TRACE("forms.c: use default:%s.", fldval);
			reset2dfl = B_TRUE;

			fldval = getfirststr(field_buffer(crtfld, DFL_BUF));
			TRACE("forms.c: newfieldval:%s.", fldval);
			/* check if default is "auto" */
			if (0 == strncmp(fldval, AUTO_STR, strlen(AUTO_STR))) {
				reset2dfl = B_FALSE;
				TRACE("forms.c: use auto value");
				/* we only use it for eq. numbers */
				strcpy(fldval, "0");
			}
		}

		if (maskaddr) /* if there is a mask */
		/* set flag in the mask if field changed */
			if (field_status(crtfld)) {
				TRACE("forms.c: mask update");
				*(uint32_t *)maskaddr |= cinf->mask;
				TRACE("forms.c: mask updated");
			}

		/* convert to the appropriate type */
		switch (cinf->typeid) {

		case T_INT:
			TRACE("forms.c: typeid:int");
			if (reset2dfl)
				*(int *)dataaddr = int_reset;
			else
				*(int *)dataaddr = atoi(fldval);
			break;
		case T_INT16:
			TRACE("forms.c: typeid:int16");

			*(int16_t *)dataaddr = (int16_t)atoi(fldval);
			break;
		case T_UINT:
			TRACE("forms.c: typeid:uint");
			if (reset2dfl)
				*(uint_t *)dataaddr = uint_reset;
			else
				*(uint_t *)dataaddr = (uint_t)atol(fldval);
			break;
		case T_UINT16:
			TRACE("forms.c: typeid:uint16");
			if (reset2dfl)
				*(uint16_t *)dataaddr = uint16_reset;
			else
				*(uint16_t *)dataaddr = (uint16_t)atol(fldval);
			break;
		case T_LNG:
			TRACE("forms.c: typeid:long");
			if (reset2dfl)
				*(long *)dataaddr = long_reset;
			else
				*(long *)dataaddr = atol(fldval);
			break;
		case T_LLNG:
			TRACE("forms.c: typeid:longlong %s.", fldval);
			*(long long *)dataaddr = atoll(fldval);
			TRACE("forms.c: sval=%lld", *(long long *)dataaddr);
			break;
		case T_FLT:
			TRACE("forms.c: typeid:flt");
			if (reset2dfl)
				*(float *)dataaddr = float_reset;
			else
				*(float *)dataaddr = atof(fldval);
			break;
		case T_DBL:
			TRACE("forms.c: typeid:dbl");
			if (reset2dfl)
				*(double *)dataaddr = double_reset;
			else
				*(double *)dataaddr = atof(fldval);
			break;
		case T_BLN:
			TRACE("forms.c: typeid:bln");
			*(boolean_t *)dataaddr = convert_to_bool(fldval);
			break;
		/* SAM-specific types */
		case T_SIZE:
			TRACE("forms.c: typeid:size");
			if (reset2dfl)
				*(fsize_t *)dataaddr = fsize_reset;
			else
				if (-1 == str_to_fsize(fldval,
				    (fsize_t *)dataaddr))
					TRACE("forms.c:"
					    "invalid field value:%s.", fldval);
			break;
		case T_TIME:
			TRACE("forms.c: typeid:time");
			if (reset2dfl)
				*(uint_t *)dataaddr = uint_reset;
			else
				if (-1 == str_to_interval(fldval,
				    (uint_t *)dataaddr))
					slog("samc:invalid field value:%s.",
					    fldval);
			TRACE("forms.c: data=%u.", *(uint_t *)dataaddr);
			break;
		case T_STR:
			TRACE("forms.c: typeid:str");
			if (reset2dfl)
				((char *)dataaddr)[0] = char_array_reset;
			else {
				TRACE("forms.c: fval=%s dataddr=%x",
				    fldval, dataaddr);
				strcpy((char *)dataaddr, fldval);
				TRACE("forms.c: data=%s.", (char *)dataaddr);
			}
			break;
		case T_JOIN:
			TRACE("forms.c: typeid join");
			if (reset2dfl)
				*(join_method_t *)dataaddr = JOIN_NOT_SET;
			else
				*(join_method_t *)dataaddr = joinmetid(fldval);
			break;
		case T_SORT:
			TRACE("forms.c: typeid sort");
			if (reset2dfl)
				*(sort_method_t *)dataaddr = SM_NOT_SET;
			else
				*(sort_method_t *)dataaddr = sortmetid(fldval);
			break;
		case T_OFCP:
			TRACE("forms.c: typeid ofcp");
			if (reset2dfl)
				*(offline_copy_method_t *)dataaddr =
				    OC_NOT_SET;
			else
				*(offline_copy_method_t *)dataaddr =
				    offcpmetid(fldval);
			break;
		case T_AGE:
			TRACE("forms.c: typeid age");
			if (reset2dfl)
				*(age_prio_type *)dataaddr = NOT_SET;
			else
				*(age_prio_type *)dataaddr =
				    convert_to_agetype(fldval);
		default:

			break;
		}
//		free(fldval);
	}
	TRACE("forms.c:convert_form_data exit");
}


static int
chg_field_req(FORM *form, int req) {
	int res = form_driver(form, req);
	if (E_OK == res)
		form_driver(form, REQ_END_FIELD);
	else
		beep();
	return (res);
}

static void
more(FORM *frm, int pages) {
	int rows, cols;
	if (pages - 1) {
		scale_form(frm, &rows, &cols);
		mvprintw(rows + 5, 0, "    more (ctrl-f)");
		clrtoeol();
	}
}
/*
 * hide/display the fields after the current one
 * if fields are hidden they are also reset to their default value
 */
static void
switchdisplaystate(FORM *frm) {
	FIELD *fld = current_field(frm);
	int nfields = field_count(frm),
	    cnt = field_index(fld) + 1,
	    makevisible;
	TRACE("forms.c: switchdisplay() start:%d end:%d",
	    field_index(fld) + 1, nfields - 1);
	if (is_visible(form_fields(frm)[cnt]))
		makevisible = 0;
	else
		makevisible = 1;
	for (; cnt < nfields; cnt++) {
		fld = form_fields(frm)[cnt];
		if (makevisible)
			field_opts_on(fld, O_VISIBLE);
		else {
			char *newval;
			field_opts_off(fld, O_VISIBLE);
			/* reset hidden fields to default values */
			if (NULL != field_buffer(fld, DFL_BUF))
				if (strlen(getfirststr(field_buffer(fld,
				    DFL_BUF)))) {
				TRACE("forms.c:chk0:%s.",
				    Str(field_buffer(fld, DFL_BUF)));
				TRACE("forms.c:chk1:firstr:%s.",
				    getfirststr(field_buffer(fld, DFL_BUF)));
				newval = (char *)malloc(4 + strlen(DFL_STR) +
				    strlen(getfirststr(field_buffer(fld,
					DFL_BUF))));
				sprintf(newval, "%s (%s)", DFL_STR,
				    getfirststr(field_buffer(fld, DFL_BUF)));
				set_field_buffer(fld, VAL_BUF, newval);
				free(newval);
			} else
				set_field_buffer(fld, VAL_BUF, NULL);
//				set_field_buffer(fld, VAL_BUF,
//				field_buffer(fld, DFL_BUF));
		}
	}
}
void
checkchar(int c, FORM *f) {
	int x, y,	/* cursor coordinates */
	    xf,		/* y coordinate of the current field */
	    cols;	/* # of columns for the current field */

	if (isascii(c)) {
		getyx(form_sub(f), y, x);
		field_info(current_field(f),
		    &y, &cols, &y, &xf, &y, &y);
				/*
				 * if cursor is not on the last position
				 * (unless the field is horiz. scrollable)
				 */
		if (!(xf + cols == x + 1 &&
		    O_STATIC & field_opts(current_field(f))))
			if (E_OK != form_driver(f, c))
				TRACE("forms.c: illegal ascii "
				    "char %c[%d]", c, c);
			else
				return;
	}
	beep();
}

/*
 * return -1 if user abandons form editing
 */
int
process_form(FORM *f, char *form_title) {
	int done = 0, c,
	    ret = 0,	/* return code */
	    pages = 0,	/* number of pages (screens) of this form */
	    x, y,	/* cursor coordinates */
	    xf;		/* y coordinate of the current field */
	FIELD *fld;
	char valbuf[80];
	TRACE("forms.c: process_form(%x,%7s...)", f, form_title);
	cbreak();
	halfdelay(10);
	while (E_OK == set_form_page(f, pages))
		pages++;
	set_form_page(f, 0);
	form_driver(f, REQ_FIRST_FIELD);
	form_driver(f, REQ_END_FIELD);
	more(f, pages);
	form_driver(f, REQ_INS_MODE);
	while (!done) {
		leaveok(stdscr, TRUE);
#ifndef TEST
		ShowBanner();
#endif
		more(f, pages);
		refresh();
		leaveok(stdscr, FALSE);
		display_form_title(f, form_title, pages);
		pos_form_cursor(f);
		wrefresh(form_win(f));
		c = getch(); /* read a character */
		if (c == ERR)
			continue;

		switch (c) {

		case KEY_default:
			fld = current_field(f);
			if (!strcspn(field_buffer(fld, DFL_BUF), " ")) {
				/* no default value */
				beep();
				break;
			}
			sprintf(valbuf, "%s (", DFL_STR);
			strncat(valbuf, field_buffer(fld, DFL_BUF),
			    strcspn(field_buffer(fld, DFL_BUF), " "));

			strcat(valbuf, ")");
			set_field_buffer(fld, VAL_BUF, valbuf);
			form_driver(f, REQ_END_FIELD);
			break;

		case ' ':	/* next choice (enum. types only) */

			fld = current_field(f);

			if (field_type(fld) == TYPE_STRWBLANKS) {
				checkchar(c, f);
				break;
			}

			if (field_type(fld) != TYPE_ENUM) {
				beep();
				break;
			}

			/* for enumerated types */
			if (E_OK !=
			    chg_field_req(f, REQ_NEXT_CHOICE)) {
				/*
				 * if field validation failed then
				 * set to default
				 */
				set_field_buffer(fld, VAL_BUF,
				    field_buffer(fld, DFL_BUF));
				form_driver(f, REQ_END_FIELD);
			} else
				if (is_trig(fld))
					switchdisplaystate(f);
			break;

		case KEY_enter:
			/* first validate current field */
			if (E_OK != chg_field_req(f, REQ_NEXT_FIELD))
			    break;
			/* then proceed */
#ifndef TEST
			if (askw(LINES - 2, 2,
			    "Proceed? [Y/n]", 'y'))
#endif
				done = 1;
			clear();
			touchwin(form_win(f));
			pos_form_cursor(f);
			break;

		case KEY_esc: /* ESC */
#ifndef TEST
			if (askw(LINES - 2, 2,
			    "Abandon form changes? [Y/n]", 'y')) {
#endif
				ret = -1;
				done = 1;
#ifndef TEST
			}
#endif
			clear();
			touchwin(form_win(f));
			pos_form_cursor(f);
			break;
		case 0x0e: /* ^N */
		case 0x09: /* HTAB key */
			chg_field_req(f, REQ_NEXT_FIELD);
			break;
		case 0x10: /* ^P */
			chg_field_req(f, REQ_PREV_FIELD);
			break;
		case KEY_HOME:
			chg_field_req(f, REQ_FIRST_FIELD);
			break;
		case KEY_LL:
			chg_field_req(f, REQ_LAST_FIELD);
			break;
		case KEY_LEFT:
			chg_field_req(f, REQ_LEFT_FIELD);
			break;
		case KEY_RIGHT:
			chg_field_req(f, REQ_RIGHT_FIELD);
			break;
		case KEY_UP:
			chg_field_req(f, REQ_UP_FIELD);
			break;
		case KEY_DOWN:
			chg_field_req(f, REQ_DOWN_FIELD);
			break;

		case KEY_full_fwd: /* ^F */
			chg_field_req(f, REQ_NEXT_PAGE);
			break;
		case KEY_full_bwd: /* ^B */
			chg_field_req(f, REQ_PREV_PAGE);
			break;

		case KEY_help:
#ifndef TEST
			help();
#endif
			touchwin(form_win(f));
			touchwin(stdscr);
			pos_form_cursor(f);
			break;
		case KEY_backspace:
			/* if  default displayed then clear field */
			if (0 ==
			    strncmp(field_buffer(current_field(f), VAL_BUF),
				DFL_STR, strlen(DFL_STR))) {
				form_driver(f, REQ_CLR_FIELD);
				break;
			}
			/* if horiz. scrollable field then show it */
			if (!(O_STATIC & field_opts(current_field(f)))) {
				form_driver(f, REQ_BEG_FIELD);
				form_driver(f, REQ_END_FIELD);
			}
			getyx(form_sub(f), y, x);
			field_info(current_field(f), &y, &y, &y, &xf, &y, &y);
			/* if cursor on first position then skip */
			if (xf == x)
				beep();
			else
				chg_field_req(f, REQ_DEL_PREV);
			break;
		default:
			checkchar(c, f);
			break;
		} // switch
	} // done
	TRACE("forms.c:process_form exit(ret.code:%d)", ret);
	return (ret);
}

/*
 * the 3 functions below convert IDs to names by doing a lookup in one of the
 * tables defined in cparamsdef.h
 */
char *
joinname(join_method_t mid) {
	int i = 0;
	if (mid == 0) /* not set */
		mid = 1; /* none */
	while (join_tbl[i].EeName) {
		if (join_tbl[i].EeValue == mid)
			return (join_tbl[i].EeName);
		i++;
	}
	return (NULL);
}
char *
sortname(sort_method_t mid) {
	int i = 0;
	if (mid == 0) /* not set */
		mid = 1; /* none */
	while (sort_method_tbl[i].EeName) {
		if (sort_method_tbl[i].EeValue == mid)
			return (sort_method_tbl[i].EeName);
		i++;
	}
	return (NULL);
}
char *
offcpname(offline_copy_method_t mid) {
	int i = 0;
	if (mid == 0) /* not set */
		mid = 1; /* none */
	while (offline_cpy_tbl[i].EeName) {
		if (offline_cpy_tbl[i].EeValue == mid)
			return (offline_cpy_tbl[i].EeName);
		i++;
	}
	return (NULL);
}

void
reset_field_recs_val(FIELD_RECORD recs[], int numrecs) {
	int i = 0;
	for (i = 0; i < numrecs; i++)
		if (recs[i].mkfield != mklabel &&
		    recs[i].mkfield != mkhlabel)
			recs[i].val = NULL;
}
static void
init_field_recs(FIELD_RECORD recs[], void *baseaddr,
    int recmember) {			/* 0 = set val,  1 = set dfl */
	FIELD_RECORD *rec;
	int irec;	/* index for recs[] */
	void *addr;
	char *maskaddr;
	char *dest;	/* where the data is stored */
	convinfo *cinfo;
	TRACE("forms.c:init_field_recs(%x,%x,%d)", recs, baseaddr, recmember);

	for (irec = 0; recs[irec].mkfield; irec++) {
		rec = &recs[irec];
		if (rec->mkfield == mklabel || rec->mkfield == mkhlabel)
			continue; /* skip labels */
		/*
		 * initialize (default) values with NULL, so that old values
		 * (set up by previous calls) are discarded
		 */
		if (recmember)
			rec->dfl = NULL;
		else
			rec->val = NULL;
		/* get conversion information */
		cinfo = rec->cinfo;
		if (NULL == cinfo)
			continue;
		TRACE("forms.c:typeid=%d,offs=%d,moffs=%d,msk=%x,"
		    "dptr=%x,mptr=%x",
		    cinfo->typeid, cinfo->dataoffset, cinfo->maskoffset,
		    cinfo->mask, cinfo->dataptr, cinfo->maskptr);

		if (recmember == 0) {
			if (cinfo->dataoffset == cinfo->maskoffset)
				/* use pointer fields instead of offsets */
				maskaddr = cinfo->maskptr;
			else
				maskaddr = (char *)baseaddr + cinfo->maskoffset;
			TRACE("forms.c: maskaddr=%x", maskaddr);
			/*
			 * check if the field has been explicitely set
			 * if not, then skip (don't set the value)
			 */
			if (maskaddr) /* if there is a mask */
				if (!(*(uint32_t *)(void *)maskaddr
				    & cinfo->mask)) {
					TRACE("forms.c: not explicitly set;"
					    " skipped.");
					continue;
				}
		}
		if (cinfo->dataoffset == cinfo->maskoffset)
			/* use pointers not offsets */
			addr = cinfo->dataptr;
		else
			addr = (char *)baseaddr + cinfo->dataoffset;

		/*
		 * compute the string that will be copied into the form field
		 * buffer at form creation time.
		 */
		switch (cinfo->typeid) {

		case T_INT:
			TRACE("forms.c: init int");
			if (*(int *)addr == int_reset)
				continue;
			dest = (char *)malloc(10);
			sprintf(dest, "%d", *(int *)addr);
			break;
		case T_INT16:
			TRACE("forms.c: init int16");
			dest = (char *)malloc(10);
			sprintf(dest, "%d", *(int16_t *)addr);
			break;
		case T_UINT:
			TRACE("forms.c: init uint");
			if (*(uint_t *)addr == uint_reset)
				continue;
			dest = (char *)malloc(10);
			sprintf(dest, "%u", *(uint_t *)addr);
			break;
		case T_UINT16:
			TRACE("forms.c: init uint16");
			if (*(uint16_t *)addr == uint16_reset)
				continue;
			dest = (char *)malloc(10);
			sprintf(dest, "%u", *(uint16_t *)addr);
			break;
		case T_LNG:
			TRACE("forms.c: init long");
			if (*(long *)addr == long_reset)
				continue;
			dest = (char *)malloc(20);
			sprintf(dest, "%ld", *(long *)addr);
			break;
		case T_LLNG:
			TRACE("forms.c: init longlong");
			dest = (char *)malloc(40);
			sprintf(dest, "%lld", *(long long *)addr);
			break;
		case T_FLT:
			TRACE("forms.c: init flt");
			if (*(float *)addr == float_reset)
				continue;
			dest = (char *)malloc(30);
			sprintf(dest, "%.2f", *(float *)addr);
			break;
		case T_DBL:
			TRACE("forms.c: init dbl");
			if (*(double *)addr == double_reset)
				continue;
			dest = (char *)malloc(30);
			sprintf(dest, "%.2f", *(double *)addr);
			break;
		case T_BLN:
			TRACE("forms.c: init bln");
			dest = (char *)malloc(4);
			if (*(boolean_t *)addr)
				strcpy(dest, "on");
			else
				strcpy(dest, "off");
			break;
		case T_SIZE:
			TRACE("forms.c: init size");
			if (*(fsize_t *)addr == fsize_reset)
				continue;
			dest = (char *)
			    strdup(fsize_to_str(*(fsize_t *)addr));
			break;
		case T_TIME:
			TRACE("formc.c: init time");
			if (*(uint_t *)addr == uint_reset)
				continue;
			dest = (char *)
			    strdup(interval_to_str(*(uint_t *)addr));
			break;
		case T_STR:
			TRACE("forms.c: init str");
			dest = NULL;
			if (*(char *)addr == char_array_reset)
				continue;
			dest = (char *)strdup(addr);
			break;
		case T_JOIN:
			TRACE("forms.c: init join");
			dest = (char *)
			    strdup(joinname(*(join_method_t *)addr));
			break;
		case T_SORT:
			TRACE("forms.c: init sort");
			dest = (char *)
			    strdup(sortname(*(sort_method_t *)addr));
			break;
		case T_OFCP:
			TRACE("forms.c: init ofcp");
			dest = (char *)
			    strdup(offcpname(*(offline_copy_method_t *)addr));
			break;
		case T_AGE:
			TRACE("forms.c: init age");
			dest = (char *)convert_agetype(*(age_prio_type *)addr);
			break;
		default:
			perror("Internal error: unrecognized TYPEID");

		} // switch
		TRACE("forms.c: dest=%s", Str(dest));
		if (recmember)
			rec->dfl = dest;
		else
			rec->val = dest;
	} // for

	TRACE("forms.c:init_field_recs() exit");
}

void
init_field_recs_val(FIELD_RECORD recs[], void *baseaddr) {
	init_field_recs(recs, baseaddr, 0);
}

void
init_field_recs_dfl(FIELD_RECORD recs[], void *baseaddr) {
	init_field_recs(recs, baseaddr, 1);
}

int
destroy_form(FORM *frm) {
	FIELD **flds = form_fields(frm);
	TRACE("forms.c: destroying form %x", frm);
	/* first disconnect the FORM from its fields */
	if (E_OK != free_form(frm))
		return (-1);
	/* then free the FIELD objects */
	while (*flds) {
		free_field(*flds);
		flds++;
	}
	TRACE("forms.c: form destroyed");
	return (0);
}
