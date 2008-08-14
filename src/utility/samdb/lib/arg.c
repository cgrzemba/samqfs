/*
 * 	arg.c - Process arguments in table form.
 *
 *	Description:
 *	    arg.c is based in spirit on the Control Data common deck
 *	    comcarg.  A table of arguments is handed to the argument
 *	    processor.
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
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include <sam/sam_trace.h>

#include "arg.h"
#include "util.h"

/*
 * --	Process_Command_Argument - Process Argument List.
 *
 *	Description:
 *	    Process argument list using argument processing table.
 *	    See arg.h for argument type details.
 *
 *	On Entry:
 *	    T		Argument table.
 *	    arpc	Argument count. (similar to argc)
 *	    arpv	Argument vector. (similar to argv)
 *
 *	On Return:
 *	    arpc	Updated.
 *	    arpv	Updated.
 *
 *	Returns:
 *	    Zero if complete and no errors, -1 if error, or partial
 *	    processing ordinal as defined in the argument table (T).
 */
int
Process_Command_Arguments(
	arg_t *T, 	/* Argument table */
	int *arpc, 	/* Argument count */
	char ***arpv)	/* Argument vector */
{
	int argc; 	/* Argument count */
	char **argv; 	/* Argument vector */
	arg_t *t; 	/* Argument table entry */
	char *p;

	argc = *arpc;
	argv = *arpv;

	while (--argc > 0) {
		if (**++argv != '-') {
			break;
		}

		/* Search command table */
		for (t = T; t->p != NULL; t++) {
			if (strcmp(*argv, t->p) == 0) {
				break;
			}
		}

		if (t->p == NULL) { /* Unknown command */
			Trace(TR_ERR, "Unrecognized argument: %s\n", *argv);
			continue;
		}

		/*	Pre-screen command table entry */
		switch (t->type) {
		case ARG_PART: /* Argument with partial define */
		case ARG_FALSE: /* Argument with set to false */
		case ARG_TRUE: /* Argument set to true */
		case ARG_VERS: /* Print version and exit */
			break;

		case ARG_INT: /* Argument with interger	*/
		case ARG_INTV: /* Argument with interval */
		case ARG_TIME: /* Argument with time */
		case ARG_TIMEV: /* Argument with time interval */
		case ARG_BYTES: /* Argument with bytes */
		case ARG_BYTEV: /* Argument with bytes interval	*/
		case ARG_STR: /* Argument with string */
			if (--argc <= 0) {
				Trace(TR_ERR, "Missing parameter value "
				    "for argument: %s\n", t->p);
				*arpc = argc;
				*arpv = argv;
				return (-1); /* Set error return	*/
			}

			if (t->v == NULL) {
				Trace(TR_ERR, "Null value entry"
				    "for argument: %s\n", t->p);
				continue;
			}

			break;

		default:
			Trace(TR_ERR, "Unknown type %d for argument: %s\n",
			    t->type, t->p);
			continue;
		}

		/* Process command table entry */
		switch (t->type) {
		case ARG_PART: /* Argument with partial define */
			*arpc = argc;
			*arpv = argv;
			return ((int)t->v); /* Set recall */

		case ARG_FALSE: /* Argument set to false */
			*((int *)t->v) = 0;
			break;

		case ARG_TRUE: /* Argument set to true */
			*((int *)t->v) = 1;
			break;

		case ARG_INT: /* Argument with integer	*/
			*((int *)t->v) = atoi(*++argv);
			break;

		case ARG_INTV: /* Argument with interval */
			p = strpbrk(*++argv, "/,:.");
			((size_int_t *)t->v)->l = atoi(*argv);
			if (p == NULL)
				break;
			((size_int_t *)t->v)->u = atoi(++p);
			break;

		case ARG_TIME: /* Argument with time */
			*((time_t *)t->v) = atosecs(*++argv);
			break;

		case ARG_TIMEV: /* Argument with time interval */
			p = strpbrk(*++argv, "/,:");
			((time_int_t *)t->v)->l = atosecs(*argv);
			if (p == NULL)
				break;
			((time_int_t *)t->v)->u = atosecs(++p);
			break;

		case ARG_BYTES: /* Argument with bytes */
			*((int *)t->v) = atobytes(*++argv);
			break;

		case ARG_BYTEV: /* Argument with bytes interval	*/
			p = strpbrk(*++argv, "/,:");
			((size_int_t *)t->v)->l = atobytes(*argv);
			if (p == NULL)
				break;
			((size_int_t *)t->v)->u = atobytes(++p);
			break;

		case ARG_STR: /* Argument with string */
			*((char **)t->v) = *++argv;
			break;

		case ARG_VERS: /* Print version and exit */
#ifndef _LARGEFILE_SOURCE
			printf("%s\n", VERSION);
#else
			printf("%s largefile\n", VERSION);
#endif
			exit(0);
		}
	}

	*arpc = argc;
	*arpv = argv;
	return (0); /* Set no recall */
}
