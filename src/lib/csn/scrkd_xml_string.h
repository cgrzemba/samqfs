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
#pragma ident   "$Revision: 1.5 $"

#ifndef __SCRKD_XML_STRING_H__
#define	__SCRKD_XML_STRING_H__



#ifdef	__cplusplus
extern "C" {
#endif

#define	XML_STRING_INCREMENT_MIN	32
#define	XML_STRING_INCREMENT_MAX	8192
#define	XML_STRING_INCREMENT_DOC	4096

typedef struct _xml_string {
	char *str;
	int len;
	int size;
} xml_string_t;

void xml_string_init(xml_string_t *, int);
void xml_string_clear(xml_string_t *);
void xml_string_free(xml_string_t *);

/*
 *  Return a boolean; false on a memory allocation failure,
 *  true on all other conditions.
 */
int  xml_string_add(xml_string_t *, const char *);
int  xml_string_addn(xml_string_t *, const char *, int);

#ifdef	__cplusplus
}
#endif

#endif /* __SCRKD_XML_STRING_H__ */
