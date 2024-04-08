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
#pragma ident   "$Revision: 1.6 $"

#include <strings.h>
#include <stdlib.h>
#include "scrkd_xml_string.h"

static int xml_string_init_str(xml_string_t *string);

/*
 *  xml_string_init
 *
 *  Initializes the string object.
 *
 */
void
xml_string_init(xml_string_t *string, int size) {
	if (string == NULL) {
		return;
	}

	bzero(string, sizeof (xml_string_t));

	if (size < XML_STRING_INCREMENT_MIN)
		size = XML_STRING_INCREMENT_MIN;
	else if (size > XML_STRING_INCREMENT_MAX)
		size = XML_STRING_INCREMENT_MAX;

	string->size = size;
}


/*
 *  xml_string_clear
 *
 *  Clears the contents of the string, but does not free the memory
 *  allocated for it.  Useful for re-using an allocated string for
 *  similarly sized buffers.
 *
 */
void
xml_string_clear(xml_string_t *string) {
	if (string == NULL) {
		return;
	}
	if (string->str) {
		string->str[0] = 0;
	}
	string->len = 0;
}


/*
 *  xml_string_free
 *
 *  Clears and frees the contents of the string.
 *
 */
void
xml_string_free(xml_string_t *string) {

	if (string && string->str) {
		free(string->str);
		string->str = 0;
		string->len = 0;
	}
}


/*
 *  xml_string_addn
 *
 *  Adds a character array of the specified length to the string. Allocates
 *  new memory if the current allocation is exceeded by the new length.
 *
 */
int
xml_string_addn(xml_string_t *target, const char *source, int add_len) {

	int newsize;
	char *tmp = 0;

	if (target && source) {
		if (!target->str) {
			if (!xml_string_init_str(target)) {
				return (0);
			}
		}
		if (target->len + add_len + 1 > target->size) {
			newsize = target->len + add_len + 1;
			tmp = target->str;
			target->str  = (char *)realloc(target->str, newsize);
			if (target->str) {
				target->size = newsize;
			}
		}

		if (target->str) {
			if (add_len) {
				memcpy(target->str + target->len, source,
				    add_len);
			}
			target->len += add_len;
			target->str[target->len] = 0;
		} else {
			target->str = tmp;
			return (0);
		}
	}

	return (target->len);
}


/*
 *  xml_string_add
 *
 *  Adds a 0 terminated character array to the string.
 *
 */
int
xml_string_add(xml_string_t *target, const char *source) {
	return (xml_string_addn(target, source, strlen(source)));
}

/*
 *  xml_string_init_str
 *
 *	Initializes a string to accept storage.
 *
 */
static int
xml_string_init_str(xml_string_t *string) {

	if (string == NULL) {
		return (-1);
	}
	if ((string->str = (char *)malloc(string->size))) {
		string->str[0] = 0;
		string->len    = 0;
	} else {
		string->size = 0;
	}
	return (string->size);
}
