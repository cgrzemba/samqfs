/*	util.h - General utility function definitions. */

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

#include <stdio.h>
#include <sys/errno.h>

/* ----	Buffer - Buffer control structure. */
typedef	struct {
	char		*buf;		/* Pointer to buffer */
	int		bufl;		/* Length of buffer */
	char		*bufp;		/* Current buffer position */
}	Buffer;

#define	VERSION		"1.0"
#define	L_IN_BUFFER	5000		/* Input buffer length */

extern int readin(FILE *, Buffer *);
extern int readin_ln;

extern int atosecs(char *);
extern int atobytes(char *);
extern int atox(char *);
extern void init_trace(int, int);
extern void error(int, int, char *, ...);
extern int parsefields(char *, char **, int *, int);
extern char *xstrdup(char *);
extern char *xstrdup2(char *, int);

extern int get_vsn(char *, char **, char **);
