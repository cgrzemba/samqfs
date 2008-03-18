/*
 * setfield.h - Set field value definitions.
 */

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

/*
 * The setfield functions allow manipulation of data fields in a structure
 * from various user interface programs (commands and GUIs) in a
 * consistant and simple manner.  The interface uses a table of data field
 * descriptions for the fields in the structure.  The data descriptions
 * include identification of the field in the structure, the type of data
 * in the field, a default value and limits for the value.  The functions
 * that operate using the table are in src/lib/samut/setfield.c.
 *
 * The SetFieldValue() function receives two text strings - the name of
 * the field, and the value, and the pointers to the struct and the
 * control table.  The field is identified (it may have an alias) and the
 * value converted appropriately and stored in the field of the struct.
 * Individual bits are handled by the bit masks.  An error message
 * function may be provided in the call.
 *
 * The data descriptions are placed in the header file that defines the
 * structure.  The descriptions are lines of the form:
 *
 * TYPE name[=struct_field][+|-def_bit] default limits
 *
 * Where:
 * 	TYPE 	the setfield datatype.  This determines how to process the
 * 		user supplied value string.
 *
 * 	name	The name of the data field as supplied by the user to the
 * 		interface program.
 *
 * 	=struct_field	The name of the field in the struct if not the same
 * 			as 'name'.
 *
 * 	+def_bit
 * 	-def_bit	The name of a 'defined' bit mask for the field.
 *			This is to define the presence of user supplied
 *			data in the field.  If '+' is used, multiple
 * 			definitions of the same field will be detected.
 *
 * 	default		The default value of the field.  SetFieldDefaults()
 *			will use this value to initialize the structure.
 *
 *
 * An 'awk' program, 'include/sam/mkhc.awk' processes the descriptions in the
 * '.h' file and produces the C table definitions in the matching '.hc' file.
 * I.e:  mount.h produces mount.hc.
 *
 * The declaration section is identified to mkhc.awk with the following line:
 * #ifdef SETFIELD_DEFS
 *
 * Each structure being declared uses the following line:
 *
 * #define STRUCT 'struct_name' 'table_name'
 * I.e.
 * #define STRUCT sam_fs_info MountParams
 *
 * If a 'defined' flags field is to be used, set the name of it with:
 * #define DEFINED 'def_flags_field_name'
 * There may be more than one of these.  Group all the descriptions using it
 * together.  (see 'include/sam/archset.h).
 *
 * The data description lines.  See below for a synopsis of the data lines.
 * For examples, see include/sam/{archset,defaults,mount}.h
 *
 * The declaration section ends with an '#endif' line.
 * #endif
 *
 * mkhc.awk will copy '#include' lines to the .hc file.
 *
 *
 * Maintenance - additions to the 'setfield' process:
 * Adding a field to a struct is pretty straightforward.  Add a data
 * description line to the list in the header file.
 *
 * If you need to add a 'type',
 * 1. add the enum FV_type, and values structure definition to
 *    include/sam/setfield.h,
 * 2. add code to process the new type in src/lib/setfield.c, and
 * 3. add code to the awk program to generate the control structures for
 *    setfield.c to process the new type.
 */

#if !defined(SETFIELD_H)
#define	SETFIELD_H

#ifdef sun
#pragma ident "$Revision: 1.18 $"
#endif

#include <limits.h>
#include <stddef.h>

/* Define field value table entry. */

struct fieldVals {
	char		*FvName;	/* Name of field */
	uint16_t	FvType;		/* Data type in field */
	int		FvLoc;		/* Offset of field in structure */
	void		*FvVals;	/* Values structure (Default, min, */
					/* max etc. */
	uint32_t	FvDefBit;	/* Bit to set when field defined */
};

enum FV_type {		/* arg 1, arg 2, arg 3, arg 4 */
	CLEARFLAG,	/* bit-mask for flag, default, set value, */
			/* clear value */
	DEFBITS,	/* 'loc' is offset of defined bits field.  If first */
			/* entry 'vals' is NULL do not check for duplicate */
			/* definition */
	DOUBLE,		/* default, minimum value, maximum value, multiplier */
	ENUM,		/* default, ENUMs table */
	FLAG,		/* bit-mask for flag, default, set value, */
			/* clear value */
	FLOAT,		/* default, minimum value, maximum value, multiplier */
	FUNC,		/* default, compare, setvalue, valuetostring */
			/* functions */
	FSIZE,		/* default, precision for display */
	INT,		/* default, minimum value, maximum value, multiplier */
	INT16,		/* default, minimum value, maximum value, multiplier */
	INT64,		/* default, minimum value, maximum value, multiplier */
	MEDIA,		/* default */
	MUL8,		/* default, minimum value, maximum value, multiplier */
	MULL8,		/* default, minimum value, maximum value, multiplier */
	PWR2,		/* default, minimum value, maximum value, multiplier */
	SETFLAG,	/* bit-mask for flag, default, set value, */
			/* clear value */
	STRING,		/* default, length of string */
	INTERVAL,	/* default, minimum value, maximum value */
	FV_MAX
};

/* Flag in FvType. */
#define	FV_FLAGS	0xFF00
#define	NO_DUP_CHK	0x8000	/* Do not check for duplicate definition */

/*
 * Values structures.
 * The 'vals' field in the fieldVals entry points to one of these.
 */
struct fieldDouble	{ double def; double min; double max; double mul; };
struct fieldEnum	{ char *def; struct EnumEntry *enumTable; };
struct fieldFlag	{ uint32_t mask; char *def; char *set; char *clear; };
struct fieldFsize	{ int64_t def; int prec; };
struct fieldFunc	{ void(*def)(void *v); int(*diff)(void *v1, void *v2);
	int(*set)(void *v, char *value, char *buf, int bufsize);
	char *(*tostr)(void *v, char *buf, int bufsize); };
struct fieldInt		{ int64_t def; int64_t min; int64_t max;
	int64_t mul; };
struct fieldMedia	{ char *def; };
struct fieldString	{ char *def; int length; };

/*
 * ENUMs table entry.
 * The table is an array of EnumEntry-s
 * Terminated with an entry with a NULL EeName.
 */
struct EnumEntry {
	char	*EeName;	/* Name of enum value */
	int	EeValue;	/* Value to use */
};

void SetFieldDefaults(void *st, struct fieldVals *table);
int SetfieldDiff(void *st1, void *st2, struct fieldVals *table,
	void(*MsgFunc)(char *msg));
void SetFieldPrintTable(char *name, struct fieldVals *table);
void SetFieldPrintValues(char *name, struct fieldVals *table, void *st);
int SetFieldValue(void *st, struct fieldVals *table, char *field, char *value,
	void(*MsgFunc)(int code, char *msg));
char *SetFieldValueToStr(void *st, struct fieldVals *table, char *field,
	char *buf, int bufsize);

#endif /* SETFIELD_H */
