/*
 * regexp.c - Regular expression processor for VSNs.
 *
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


#pragma ident "$Revision: 1.11 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Solaris headers. */
#include <syslog.h>
#include <libgen.h>
#include <errno.h>

/* Local headers. */
#include "recycler.h"

#if !defined(lint)
/* Private functions. */
static void regerr(int ercode);

/* Private data. */
static jmp_buf jbuf;

/* Public data. */
char *regexp_msg;
int regexp_size;

/* regexp(5) definitions. */
#define	INIT char *sp = instring;
#define	GETC(void) (*sp++)
#define	PEEKC(void) (*sp)
#define	UNGETC(void) (--sp)
#define	RETURN(ptr) return (ptr)
#define	ERROR(val) regerr(val)

#include <regexp.h>


/*
 * Compile regular expression.
 */
char *
regcmp(
	const char *string1,
	...)
{
	size_t	size;
	char	*expbuf, *p;

	size = 1024;
	if ((expbuf = malloc(size)) == NULL) {
		emit(TO_TTY|TO_SYS, LOG_ERR, 20272, size, errtext);
		exit(1);
	}
	if (setjmp(jbuf) != 0) {
		return (NULL);
	}
	p = compile((char *)string1, expbuf, expbuf+size-1, '\0');
	regexp_size = p - expbuf;
	return (expbuf);
}


/*
 * Match regular expression.
 */
char *
regex(
	const char *re,
	const char *subject,
	...)
{
	extern char *loc1;

	if (step((char *)subject, (char *)re) == 0) {
		return (NULL);
	}
	return (loc1);
}


/*
 * Process regular expression error.
 */
static void
regerr(
	int errcode)
{
	static struct {
		int num;
		char *msg;
	} errtable[] = {
		{ 11, "range endpoint too large" },
		{ 16, "bad number" },
		{ 25, "digit out of range" },
		{ 36, "illegal or missing delimiter" },
		{ 41, "no remembered search string" },
		{ 42, "() imbalance" },
		{ 43, "too many (" },
		{ 44, "more than 2 numbers given in { }" },
		{ 45, "} expected after '.'" },
		{ 46, "first number exceeds second in { }" },
		{ 49, "[ ] imbalance" },
		{ 50, "regular expression overflow" },
		{ 0,  "unknown error" }
	};
	int n;

	for (n = 0; errtable[n].num != 0 && errtable[n].num != errcode; n++) {
		;
	}
	regexp_msg = errtable[n].msg;
	longjmp(jbuf, 0);
}
#endif /* !defined(lint) */
