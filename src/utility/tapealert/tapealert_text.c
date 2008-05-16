/*
 * tapealert_text.c - lookup tapealert SSC2 and SMC2 client application text
 *
 *
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
#pragma ident "$Revision: 1.12 $"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "tapealert_text.h"

/*
 * local function prototypes
 */
static tapealert_severity_t get_severity(char *severity_str);
static char *strapp(char *ptr, char *buf);
static void remove_newline(char *str);
static char *readln(char *buf, int buf_len, FILE **fp);
static char *read_line(char *buf, int buf_len, FILE **fp);
static char *read_multiline(char *buf, int buf_len, FILE **fp);
static boolean_t text_read(uchar_t, uchar_t, int, tapealert_msg_t *);
static int text_sev_comp(const void *a, const void *b);

/*
 * --- text_sev_comp - qsort comparison routine
 * First compare by severity then by parameter code.
 */
static int			/* 0 equal, 1 a > b, -1 a < b */
text_sev_comp(
	const void *a,		/* tapealert event */
	const void *b)		/* tapealert event */
{
	tapealert_msg_t *tapealert_a = (tapealert_msg_t *)a;
	tapealert_msg_t *tapealert_b = (tapealert_msg_t *)b;

	/* first sort by severity */
	if (tapealert_a->scode < tapealert_b->scode)
		return (1);
	if (tapealert_a->scode > tapealert_b->scode)
		return (-1);

	/* then sort by parameter code */
	if (tapealert_a->pcode < tapealert_b->pcode)
		return (-1);
	if (tapealert_a->pcode > tapealert_b->pcode)
		return (1);

	return (0);
}

/*
 * --- tapealert_text - get text for tapealert flags
 */
void
tapealert_text(
	int version,
	int type,
	int flags_len,
	uint64_t flags,
	tapealert_text_t *text)
{
	int i;
	boolean_t found;

	memset(text, 0, sizeof (tapealert_text_t));
	for (i = 0; i < flags_len; i++) {
		if ((flags >> i) & 0x1) {
			found = text_read(version, type, i+1,
			    &text->msg[text->count]);
			if (found == B_TRUE) {
				if (text->msg[text->count].scode !=
				    SEVERITY_RSVD) {
					text->count++;
				}
			}
		}
	}

	if (text->count) {
		/* sort tapealert flags by severity then by param code */
		qsort((void*)text->msg, text->count, sizeof (tapealert_msg_t),
		    text_sev_comp);
	}
}

/*
 * --- tapealert_msg_done - free tapealert flags text
 */
static void
tapealert_msg_done(tapealert_msg_t *msg)
{
	free(msg->manual);
	free(msg->flag);
	free(msg->severity);
	free(msg->appmsg);
	free(msg->cause);
	memset(msg, 0, sizeof (tapealert_msg_t));
}

/*
 * --- tapealert_text_done - release tapealert text
 */
void
tapealert_text_done(
	tapealert_text_t *text)		/* tapealert text to free */
{
	int i;

	for (i = 0; i < text->count; i++) {
		tapealert_msg_done(&text->msg[i]);
	}
	memset(text, 0, sizeof (tapealert_text_t));
}

/*
 * --- text_read - read T10 SSC2 and SMC2 tapealert text
 */
static boolean_t		/* true successful */
	text_read(
	uchar_t version,		/* std inquiry version */
	uchar_t inq_type,		/* inquiry peripheral device type */
	int pcode,			/* tapealert flag number (1-64) */
	tapealert_msg_t *msg)		/* where to put the text */
{
/* N.B. Bad indentation here to meet cstyle requirements */
#define	SSC3 "SSC3"
#define	SSC2 "SSC2"
#define	SMC2 "SMC2"
#define	LEN 256
	FILE *fp;
	char buf [LEN];
	boolean_t found;
	int num_manuals = 0;
	char *manuals[10];
	int i;

	memset(msg, 0, sizeof (tapealert_msg_t));

	if ((fp = fopen("/opt/SUNWsamfs/doc/tapealert.text", "r")) == NULL) {
		if ((fp = fopen("./tapealert.text", "r")) == NULL) {
			return (1);
		}
	}

	/* scsi inquiry perpripal type id, use inquiry version if required */
	switch (inq_type) {
	case 1: if (version <= 4) {
			manuals[0] = SSC2;
			num_manuals = 1;
		} else {
			manuals[0] = SSC3;
			manuals[1] = SSC2;
			num_manuals = 2;
		}
		break;
	case 8: manuals[0] = SMC2;
		num_manuals = 1;
		break;
	default:
		return (1);
	}

	found = B_FALSE;
	while (found == B_FALSE) {
		tapealert_msg_done(msg);

		if ((msg->manual = read_line(buf, LEN, &fp)) == NULL) {
			break;
		}

		if (readln(buf, LEN, &fp) == NULL) {
			break;
		}
		msg->pcode = strtol(buf, NULL, 16);

		if ((msg->flag = read_line(buf, LEN, &fp)) == NULL) {
			break;
		}

		if ((msg->severity = read_line(buf, LEN, &fp)) == NULL) {
			break;
		}

		if ((msg->appmsg = read_multiline(buf, LEN, &fp)) == NULL) {
			break;
		}

		if ((msg->cause = read_multiline(buf, LEN, &fp)) == NULL) {
			break;
		}

		if (readln(buf, LEN, &fp) == NULL) {
			break;
		}

		if (pcode == msg->pcode) {
			for (i = 0; i < num_manuals; i++) {
				if (strcmp(manuals[i], msg->manual) == 0) {
					found = B_TRUE;
					break;
				}
			}
		}
	}

	fclose(fp);

	if (found == B_FALSE || msg->severity == NULL) {
		tapealert_msg_done(msg);
	} else {
		msg->scode = get_severity(msg->severity);
	}

	return (found);
}

/*
 * --- get_severity - tapealert serverity string to enum
 */
static tapealert_severity_t	/* severity enum */
get_severity(
	char *severity_str)	/* SSC2 or SMC2 severity string */
{
	tapealert_severity_t scode = SEVERITY_RSVD;

	if (strcmp(severity_str, "Critical") == 0) {
		scode = SEVERITY_CRIT;
	} else if (strcmp(severity_str, "Warning") == 0) {
		scode = SEVERITY_WARN;
	} else if (strcmp(severity_str, "Information") == 0) {
		scode = SEVERITY_INFO;
	}

	return (scode);
}

/*
 * --- strapp - string append
 * Append a string to a null or alloced string.
 * Returns new alloced string.
 */
static char *
strapp(
	char *ptr,		/* suffix string */
	char *buf)		/* prefix string */
{
	int len;

	if (buf == NULL) {
		return (NULL);
	} else if (ptr == NULL) {
		if ((ptr = (char *)malloc(strlen(buf) + 1)) == NULL) {
			return (NULL);
		}
		ptr [0] = '\0';
		strcpy(ptr, buf);
	} else {
		len = strlen(ptr) + strlen(buf) + 1;
		if ((ptr = (char *)realloc(ptr, len)) == NULL) {
			return (NULL);
		}
		strcat(ptr, buf);
	}
	return (ptr);
}

/*
 * --- remove_newline - remove newline from string
 */
static void
remove_newline(
	char *str)		/* input string */
{
	char *ptr;

	ptr = strrchr(str, '\n');
	if (ptr) {
		*ptr = '\0';
	}
}

/*
 * --- readln - read line of text into a buffer
 * Returns textfile text.
 */
static char *
readln(
	char *buf,		/* buffer to store string in */
	int buf_len,		/* length of buffer */
	FILE **fp)		/* text file stream to read from */
{
	if (fgets(buf, buf_len, *fp) == NULL) {
		return (NULL);
	}
	remove_newline(buf);
	return (buf);
}

/*
 * --- read_line - read line of text and append it to alloced string
 * Returns longer buffer of text.
 */
static char *
read_line(
	char *buf,		/* alloced buffer to store string in */
	int buf_len,		/* alloced buffer length */
	FILE **fp)		/* text file stream to read from */
{
	return (strapp(NULL, readln(buf, buf_len, fp)));
}

/*
 * --- read_multiline - read from formated SSC2 and SMC2 textfile
 * Read one or more lines that are together from formated SSC2
 * and SMC2 textfile.
 * Returns one or more lines of text with newlines.
 */
static char *
read_multiline(
	char *buf,		/* alloced buffer to store string in */
	int buf_len,		/* alloced buffer length */
	FILE **fp)		/* text file stream to read from */
{
	char *ptr = NULL;
	int len;

	for (;;) {
		if (readln(buf, buf_len, fp) == NULL) {
			free(ptr);
			return (NULL);
		}
		if ((len = strlen(buf)) < 1) {
			free(ptr);
			return (NULL);
		}
		if (buf [len-1] != '\\') {
			ptr = strapp(ptr, buf);
			break;
		}
		buf [len-1] = '\n';
		ptr = strapp(ptr, buf);
	}

	return (ptr);
}
