/*
 * format.c - Library functions to support formatting structure information
 *            for display.
 *
 *            See below for a detailed usage description.
 *
 *            See src/lib/samut/sblk_show.c for examples of format functions.
 *
 *            See src/fs/cmd/fstyp/fstyp.c for example of application usage.
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

#pragma ident "$Revision: 1.10 $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <sys/inttypes.h>

#include <sam/param.h>
#include <sam/types.h>
#include <sam/format.h>

/*
 * DESCRIPTION
 *
 * Multiple SAM applications, as well as logging and tracing facilities,
 * display information from the same SAM structures, but do so in a
 * variety of ways, using a variety of human-readable names and value
 * display formats.
 *
 * The purpose of these format functions is to promote uniform display
 * of external information by making available a single source for
 * defining human-readable element names and their corresponding
 * values for each element of a structure.  However, because each
 * application has its own structure element display requirements,
 * each set of standard human-readable names and values must be able
 * to be formatted flexibly.
 *
 * Not every SAM structure requires a human-readable format definition.
 * For the SAM structures that would benefit from such a definition,
 * a formatting function is recommended.  Library functions described
 * herein are available both to support writing your structure-specific
 * formatting function, as well as to support the formatting needs
 * that can be unique to each end-user application.
 *
 * The format function specific to each SAM structure accepts a
 * reference to the appropriate type of SAM structure and a reference to
 * a sam_format_buf_t buffer.  It then takes each element of the
 * referenced SAM structure, converts that element value to a string
 * (e.g., using sprintf), and uses the following library function to
 * append a display "line" to the supplied format buffer:
 *
 *	sam_format_element_append(sam_format_buf_t *bp, char *name, char *value)
 *
 * The above function updates the sam_format_buf_t buffer with contents
 * consisting of the following 5 constructs:
 *
 *	1) Line Prefix (default "")
 *	2) Element Name String
 *	3) Line Midfix (default " = ")
 *	4) Element value String
 *	5) Line Suffix (default "\n")
 *
 * To facilitate flexible formatting of display output, each of the above
 * constructs is a uniquely managed structure (but all are of the same type).
 * Library functions are used to manage each of those constructs within the
 * sam_format_buf_t buffer.  As such, neither those constructs nor the
 * contents of the sam_format_buf_t needs to be known by the caller.
 *
 * Your structure-specific formatting function should have a style similar
 * to the following:
 *
 *	#include <sam/format.h>
 *
 *	int
 *	sam_<your_struct>_format(
 *	struct <your_struct> *struct_ptr,
 *	sam_format_buf_t *bufp)
 *	{
 *		char str[SAM_FORMAT_WIDTH_MAX];
 *		sam_format_buf_t *bp = bufp;
 *
 *		if ((struct_ptr == NULL) || (bufp == NULL)) {
 *			return(EINVAL);
 *		}
 *
 *		sprintf(str, "%d", struct_ptr->elem_1);
 *		sam_format_element_append(&bp, "elem_1", str);
 *
 *		sprintf(str, "0x%x", struct_ptr->elem_3);
 *		sam_format_element_append(&bp, "elem_3", str);
 *
 *		....
 *
 *		return(0);
 *	}
 *
 * An application calls your structure-specific formatting function to
 * fill its sam_format_buf_t buffer with structure format information.
 * The application can reference each display line in the sam_format_buf_t
 * buffer, identified by its unique element name, and manipulate it
 * independently of all other display lines to have its own unique display
 * format.  Without manipulaton, the element names and element values are
 * displayed in exactly the number of spaces that each requires, with no
 * indenting, and a separating " = " as:
 *
 * element_1 = value_1
 * elem_2 = val_2
 * ....
 *
 * To facilitate clean-columned format, library functions manage element
 * names and element values independent of their character string lengths.
 * A library function is available to configure the display field width
 * in which each element name is formatted as left-justified.  A library
 * function is also available to configure the display field width in which
 * each element value is formatted as left-justified.  Those functions
 * permit an application to generate a display of the following format:
 *
 * element_1         = value_1
 * elem_2            = val_2
 * ....
 *
 * An application may use the follwing library functions to set the default
 * element name and/or value width prior to calling your structure-specific
 * formatting function:
 *
 *	sam_format_name_width_default(int width)
 *	sam_format_value_width_default(int width)
 *
 * To subsequently reset the name or value width formatting to the factory
 * default, simply call the appropriate function above with a width value < 0.
 *
 * After an application calls your structure-specific formatting function,
 * it may require that some specific element names and/or element values be
 * displayed with a different field width.  An application may call the
 * following library functions to update the display field width of an element
 * name and/or value for a specific element (or all elements, if the specified
 * element name is NULL):
 *
 *	sam_format_element_width_name(sam_format_buf_t *bufp, char *name,
 *					int width)
 *
 *	sam_format_element_width_value(sam_format_buf_t *bufp, char *name,
 *					int width)
 *
 * Library functions are also available for an application to generate a
 * a display with a non-default prefix, midfix, and/or suffix for every
 * element of a structure.  For example, setting a default suffix to "\t"
 * could be used to produce the following output style:
 *
 * elem_1 = val_1	elem_2 = val_2	elem_3 = val_3	elem_4 = val_4
 * elem_5 = val_5	elem_6 = val_6	....
 *
 * An application may use the following library functions to change the
 * default pre/mid/suf-fix prior to calling your structure-specific
 * formatting function:
 *
 *	sam_format_prefix_default(char *prefix)
 *	sam_format_midfix_default(char *midfix)
 *	sam_format_suffix_default(char *suffix)
 *
 * To reset the prefix, midfix, or suffix formatting to the factory default,
 * simply call the appropriate function above with the value NULL.
 *
 * After calling your structure-specific formatting function, an application
 * may require that some displayed elements require "tweaking" of their
 * prefix, midfix, or suffix to produce a visually pleasing display.
 * An application may call the following library functions to update the
 * prefix, midfix, and/or suffix of a specific element (or all elements,
 * if the specified element name is NULL):
 *
 *	sam_format_element_prefix(sam_format_buf_t *bufp, char *name,
 *					char *prefix)
 *
 *	sam_format_element_midfix(sam_format_buf_t *bufp, char *name,
 *					char *midfix)
 *
 *	sam_format_element_suffix(sam_format_buf_t *bufp, char *name,
 *					char *suffix)
 *
 * When an application has completed formatting the sam_format_buf_t buffer
 * it must call a library function to display the result to standard-output.
 * An application may call the following function to display a specific
 * formatted line in the sam_format_buf_t buffer.  If the specified name is
 * NULL, all lines are displayed:
 *
 *	sam_format_print(sam_format_buf_t *bufp, char *name)
 *
 * The application may also bracket calls to the above sam_format_print
 * function with other printf() calls, as needed, to produce commented
 * output.  For example, an application could produce the following output,
 * by having the function calls printf("vtoc {\n") and printf("}\n")
 * surround a call to sam_format_print(bufp, NULL):
 *
 * vtoc {
 *	label                = SUN9.0G cyl 4924 alt
 *	boot                 = 0x0/0x0/0x0
 *	sanity               = 0x600ddeee
 *	layout               = 1
 *	name                 = ''
 *	sector_size          = 512
 *	part_count           = 8
 * }
 *
 */

static char *sam_format_prefix = SAM_FORMAT_PREFIX_DEF;
static char *sam_format_midfix = SAM_FORMAT_MIDFIX_DEF;
static char *sam_format_suffix = SAM_FORMAT_SUFFIX_DEF;

static int sam_format_width_name = SAM_FORMAT_WIDTH_NAME_DEF;
static int sam_format_width_value = SAM_FORMAT_WIDTH_VALUE_DEF;


/*
 * ----- sam_format_prefix_default - Set default prefix
 */
int					/* Errno status code */
sam_format_prefix_default(
	char *prefix)			/* Prefix */
{
	if (prefix == NULL) {
		prefix = SAM_FORMAT_PREFIX_DEF;
	}
	sam_format_prefix = prefix;

	return (0);
}

/*
 * ----- sam_format_midfix_default - Set default midfix
 */
int					/* Errno status code */
sam_format_midfix_default(
	char *midfix)			/* Midfix */
{
	if (midfix == NULL) {
		midfix = SAM_FORMAT_MIDFIX_DEF;
	}
	sam_format_midfix = midfix;

	return (0);
}

/*
 * ----- sam_format_suffix_default - Set default suffix
 */
int					/* Errno status code */
sam_format_suffix_default(
	char *suffix)			/* Suffix */
{
	if (suffix == NULL) {
		suffix = SAM_FORMAT_SUFFIX_DEF;
	}
	sam_format_suffix = suffix;

	return (0);
}

/*
 * ----- sam_format_width_name_default - Set default width for name field
 */
int						/* Errno status code */
sam_format_width_name_default(
	int width)				/* Width */
{
	if (width < 0) {
		width = SAM_FORMAT_WIDTH_VALUE_DEF;
	}
	sam_format_width_name = width;

	return (0);
}

/*
 * ----- sam_format_width_value_default - Set default width for value field
 */
int						/* Errno status code */
sam_format_width_value_default(
	int width)				/* Width */
{
	if (width < 0) {
		width = SAM_FORMAT_WIDTH_VALUE_DEF;
	}
	sam_format_width_value = width;

	return (0);
}

/*
 * ----- sam_format_line_append - Append a line to TWS buffer (internal use)
 */
static int				/* Errno status code */
sam_format_line_append(
	sam_format_buf_t **bufp,	/* Format buffer (updated & returned) */
	int type,			/* Line type (comment or element) */
	char *name,			/* Name */
	int name_width,			/* Name field width */
	char *value,			/* Value */
	int value_width)		/* Value field width */
{
	sam_format_buf_t *bp = *bufp;
	int b = sizeof (int);
	int prefix_len = strlen(sam_format_prefix);
	int midfix_len = strlen(sam_format_midfix);
	int suffix_len = strlen(sam_format_suffix);

	if ((bufp == NULL) || (*bufp == NULL) || (name == NULL) ||
	    (value == NULL) || (name_width < 0) || (value_width < 0)) {
		return (EINVAL);
	}
	if ((type != SAM_FORMAT_TYPE_ELEMENT) &&
	    (type != SAM_FORMAT_TYPE_COMMENT)) {
		return (EINVAL);
	}

	bp->type = type;	/* Comment or element */
	bp->width = (prefix_len + name_width +	/* Line total length */
	    midfix_len + value_width + suffix_len +
	    1);				/* Count NULL-terminator */
	/* Byte length of string incl. NULL term (padded to word boundary) */
	bp->len = ((strlen(name)+b)/b) * b;
	strcpy(bp->str, name);				/* Name */

	/* LINTED pointer cast may result in improper alignment */
	bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* Prefix */
	bp->type = SAM_FORMAT_TYPE_PREFIX;
	bp->width = prefix_len;
	bp->len = ((strlen(sam_format_prefix)+b)/b) * b;
	strcpy(bp->str, sam_format_prefix);

	/* LINTED pointer cast may result in improper alignment */
	bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* Name */
	bp->type = SAM_FORMAT_TYPE_NAME;
	bp->width = name_width;
	bp->len = ((strlen(name)+b)/b) * b;
	strcpy(bp->str, name);

	/* LINTED pointer cast may result in improper alignment */
	bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* Midfix */
	bp->type = SAM_FORMAT_TYPE_MIDFIX;
	bp->width = midfix_len;
	bp->len = ((strlen(sam_format_midfix)+b)/b) * b;
	strcpy(bp->str, sam_format_midfix);

	/* LINTED pointer cast may result in improper alignment */
	bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* Value */
	bp->type = SAM_FORMAT_TYPE_VALUE;
	bp->width = value_width;
	bp->len = ((strlen(value)+b)/b) * b;
	strcpy(bp->str, value);

	/* LINTED pointer cast may result in improper alignment */
	bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* Suffix */
	bp->type = SAM_FORMAT_TYPE_SUFFIX;
	bp->width = suffix_len;
	bp->len = ((strlen(sam_format_suffix)+b)/b) * b;
	strcpy(bp->str, sam_format_suffix);

	/* LINTED pointer cast may result in improper alignment */
	bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* End of line */
	bp->type = SAM_FORMAT_TYPE_EOF;			/* New EOF */

	*bufp = bp;

	return (0);
}

/*
 * ----- sam_format_element_append - Append an element line to format buffer
 */
int					/* Errno status code */
sam_format_element_append(
	sam_format_buf_t **bufp,	/* Format buffer (updated & returned) */
	char *name,			/* Element name */
	char *value)			/* Element value */
{
	if ((name == NULL) || (value == NULL)) {
		return (EINVAL);
	}

	return (sam_format_line_append(bufp, SAM_FORMAT_TYPE_ELEMENT,
	    name, (sam_format_width_name == 0 ? strlen(name) :
	    sam_format_width_name),
	    value, (sam_format_width_value == 0 ? strlen(value) :
	    sam_format_width_value)));
}

/*
 * ----- sam_format_element_xfix - Set new pre/mid/suf-fix of element line
 *                                 (internal use)
 */
static int				/* Errno status code */
sam_format_element_xfix(
	sam_format_buf_t *bufp,		/* Format buffer */
	int xfix_type,			/* Type of xfix (pre, mid, suf) */
	char *name,			/* Target element name (NULL -> all) */
	char *xfix)			/* Pre/mid/suf-fix string */
{
	sam_format_buf_t *bp = bufp;

	if ((bp == NULL) || (name == NULL)) {
		return (EINVAL);
	}
	if ((xfix_type != SAM_FORMAT_TYPE_PREFIX) &&
	    (xfix_type != SAM_FORMAT_TYPE_MIDFIX) &&
	    (xfix_type != SAM_FORMAT_TYPE_SUFFIX)) {
		return (EINVAL);
	}

	if (xfix == NULL) {
		switch (xfix_type) {
		case SAM_FORMAT_TYPE_PREFIX:
			xfix = SAM_FORMAT_PREFIX_DEF;
			break;
		case SAM_FORMAT_TYPE_MIDFIX:
			xfix = SAM_FORMAT_MIDFIX_DEF;
			break;
		case SAM_FORMAT_TYPE_SUFFIX:
			xfix = SAM_FORMAT_SUFFIX_DEF;
			break;
		default: return (EINVAL);
		}
	}

	for (bp = bufp; bp->type != SAM_FORMAT_TYPE_EOF;
	    /* LINTED pointer cast may result in improper alignment */
	    bp = (sam_format_buf_t *)(&bp->str + bp->len)) {

		/* Find start of line */
		if (bp->type != SAM_FORMAT_TYPE_ELEMENT) {
			continue;
		}

		/* Check for matching name, if requested */
		if (name != NULL) {
			if (strcmp(name, bp->str) != 0) {
				continue;
			}
		}

		/* LINTED pointer cast may result in improper alignment */
		bp = (sam_format_buf_t *)(&bp->str + bp->len);	/* Pre */

		if (xfix_type != SAM_FORMAT_TYPE_PREFIX) {
		/* LINTED pointer cast may result in improper alignment */
			bp = (sam_format_buf_t *)(&bp->str + bp->len);
		/* LINTED pointer cast may result in improper alignment */
			bp = (sam_format_buf_t *)(&bp->str + bp->len);
			if (xfix_type != SAM_FORMAT_TYPE_MIDFIX) {
		/* LINTED pointer cast may result in improper alignment */
				bp = (sam_format_buf_t *)(&bp->str + bp->len);
		/* LINTED pointer cast may result in improper alignment */
				bp = (sam_format_buf_t *)(&bp->str + bp->len);
			}
		}

		/* New xfix must fit in current xfix string space */
		if (strlen(xfix) > (bp->len - 1)) {
			return (EINVAL);
		}

		strcpy(bp->str, xfix);

		if (name != NULL) {	/* If requested specific name, done. */
			break;
		}
	}

	return (0);
}

/*
 * ----- sam_format_element_prefix - Set new prefix string for element(s)
 */
int
sam_format_element_prefix(
	sam_format_buf_t *bufp,	/* Format buffer */
	char *name,		/* Element name (NULL -> all) */
	char *prefix)		/* New Prefix string (NULL -> default) */
{
	return (sam_format_element_xfix(bufp, SAM_FORMAT_TYPE_PREFIX, name,
	    prefix));
}

/*
 * ----- sam_format_element_midfix - Set new midfix string for element(s)
 */
int				/* Errno status code */
sam_format_element_midfix(
	sam_format_buf_t *bufp,	/* Format buffer */
	char *name,		/* Element name (NULL -> all) */
	char *midfix)		/* New midfix string (NULL -> default) */
{
	return (sam_format_element_xfix(bufp, SAM_FORMAT_TYPE_MIDFIX, name,
	    midfix));
}

/*
 * ----- sam_format_element_suffix - Set new suffix string for element(s)
 */
int				/* Errno status code */
sam_format_element_suffix(
	sam_format_buf_t *bufp,	/* Format buffer */
	char *name,		/* Element name (NULL -> all) */
	char *suffix)		/* New suffix string (NULL -> default) */
{
	return (sam_format_element_xfix(bufp, SAM_FORMAT_TYPE_SUFFIX, name,
	    suffix));
}

/*
 * ----- sam_format_element_width - Set new name width of element line
 *                                  (internal use)
 */
static int			/* Errno status code */
sam_format_element_width(
	sam_format_buf_t *bufp,	/* Format buffer */
	int width_type,		/* Type of width (name, value) */
	char *name,		/* Target name (NULL -> all) */
	int width)		/* New width (<0 -> default, 0 -> strlen) */
{
	sam_format_buf_t *bp = bufp;

	if ((bp == NULL) || (name == NULL)) {
		return (EINVAL);
	}
	if ((width_type != SAM_FORMAT_TYPE_NAME) &&
	    (width_type != SAM_FORMAT_TYPE_VALUE)) {
		return (EINVAL);
	}

	if (width < 0) {
		switch (width_type) {
		case SAM_FORMAT_TYPE_NAME:
			width = SAM_FORMAT_WIDTH_NAME_DEF;
			break;
		case SAM_FORMAT_TYPE_VALUE:
			width = SAM_FORMAT_WIDTH_VALUE_DEF;
			break;
		default: return (EINVAL);
		}
	}

	for (bp = bufp; bp->type != SAM_FORMAT_TYPE_EOF;
	    /* LINTED pointer cast may result in improper alignment */
	    bp = (sam_format_buf_t *)(&bp->str + bp->len)) {

		sam_format_buf_t *startp;
		int new_width;

		/* Find start of line */
		if (bp->type != SAM_FORMAT_TYPE_ELEMENT) {
			continue;
		}

		/* Check for matching name, if requested */
		if (name != NULL) {
			if (strcmp(name, bp->str) != 0) {
				continue;
			}
		}

		startp = bp;

		/* LINTED pointer cast may result in improper alignment */
		bp = (sam_format_buf_t *)(&bp->str + bp->len);
		/* Name */
		/* LINTED pointer cast may result in improper alignment */
		bp = (sam_format_buf_t *)(&bp->str + bp->len);

		if (width_type != SAM_FORMAT_TYPE_NAME) {
		/* LINTED pointer cast may result in improper alignment */
			bp = (sam_format_buf_t *)(&bp->str + bp->len);
			/* Value */
		/* LINTED pointer cast may result in improper alignment */
			bp = (sam_format_buf_t *)(&bp->str + bp->len);
		}

		/* If new width is 0, use strlen. */
		new_width = (width == 0 ? strlen(bp->str) : width);

		/* Update total line width */
		startp->width += (new_width - bp->width);

		/* Update field width */
		bp->width = new_width;

		if (name != NULL) {	/* If requested specific name, done. */
			break;
		}
	}

	return (0);
}

/*
 * ----- sam_format_element_width_name - Set new width for element name(s)
 */
int				/* Errno status code */
sam_format_element_width_name(
	sam_format_buf_t *bufp,	/* Format buffer */
	char *name,		/* Element name (NULL -> all) */
	int width)		/* New width (<0 -> default, 0 -> strlen) */
{
	return (sam_format_element_width(bufp, SAM_FORMAT_TYPE_NAME, name,
	    width));
}

/*
 * ----- sam_format_element_width_value - Set new width for element value(s)
 */
int				/* Errno status code */
sam_format_element_width_value(
	sam_format_buf_t *bufp,	/* Format buffer */
	char *name,		/* Element name (NULL -> all) */
	int width)		/* New width (<0 -> default, 0 -> strlen) */
{
	return (sam_format_element_width(bufp, SAM_FORMAT_TYPE_VALUE, name,
	    width));
}

/*
 * ----- sam_format_print - Display formatted information
 */
int				/* Errno status code */
sam_format_print(
sam_format_buf_t *bufp,		/* Format buffer */
	char *name)		/* Element name to print (NULL -> all) */
{
	sam_format_buf_t *bp;
	char *s;
	char *c;
	int max_len = SAM_FORMAT_WIDTH_MAX;

	if (bufp == NULL) {
		return (EINVAL);
	}

	/* Assume 80 characters per line is enough for now. Grow as needed. */
	s = (char *)malloc(max_len);

	for (bp = bufp; bp->type != SAM_FORMAT_TYPE_EOF;
	    /* LINTED pointer cast may result in improper alignment */
	    bp = (sam_format_buf_t *)(&bp->str + bp->len)) {

		/* Find start of line */
		if ((bp->type != SAM_FORMAT_TYPE_ELEMENT) &&
		    (bp->type != SAM_FORMAT_TYPE_COMMENT)) {
			continue;
		}

		/* Check for matching name, if requested */
		if (name != NULL) {
			if (strcmp(name, bp->str) != 0) {
				continue;
			}
		}

		/* Check line length */
		if (bp->width > max_len) {
			free(s);
			max_len = bp->width;	/* Includes NULL-terminator */
			s = (char *)malloc(max_len);
		}

		/* Fill line buffer with spaces. */
		(void) memset(s, ' ', max_len);
		c = s;

		/* LINTED pointer cast may result in improper alignment */
		bp = (sam_format_buf_t *)(&bp->str + bp->len); /* Prefix */
		snprintf(c, bp->width+1, "%s", bp->str); /* Add pref w/ NULL */

		/* Now build this line through the suffix */
		while (bp->type != SAM_FORMAT_TYPE_SUFFIX) {
			char *st = c;

			c += strlen(bp->str);	/* NULL term of previous str */
			*c = ' ';		/* Make it a space character */
			c = st + bp->width;	/* Next field */
		/* LINTED pointer cast may result in improper alignment */
			bp = (sam_format_buf_t *)(&bp->str + bp->len);
			snprintf(c, bp->width+1, "%s", bp->str); /* Add next */
		}

		/* Print this line to stdout. */
		printf("%s", s);

		/* If requested a specific element, then we're done. */
		if (name != NULL) {
			break;
		}
	}

	free(s);

	/* LINTED function falls off bottom without returning value */
}
