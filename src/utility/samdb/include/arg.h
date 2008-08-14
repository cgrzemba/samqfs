/*
 * 	arg.h - Process Arguments in Table Form.
 *
 *	Description:
 *	    arg.h contains definitions for interfacing to arg.c for
 *	    argument processing.
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

#ifndef	_ARG_H

typedef	enum	{
	ARG_PART = 0,		/* Argument with partial define	*/
	ARG_FALSE,			/* Argument with set to false	*/
	ARG_TRUE,			/* Argument set to true		*/
	ARG_INT,			/* Argument with interger	*/
	ARG_INTV,			/* Argument with interval	*/
	ARG_TIME,			/* Argument with time		*/
	ARG_TIMEV,			/* Argument with time interval	*/
	ARG_BYTES,			/* Argument with bytes		*/
	ARG_BYTEV,			/* Argument with bytes interval	*/
	ARG_STR,			/* Argument with string		*/
	ARG_VERS			/* Print version and exit	*/
}	ARG_TYPE;


typedef	struct	{
	char		*p;		/* Argument string		*/
	ARG_TYPE	type;		/* Argument type		*/
	void		*v;		/* Argument variable		*/
}	arg_t;

typedef	struct	{			/* Time interval		*/
	time_t		l;		/* lower			*/
	time_t		u;		/* upper			*/
}	time_int_t;

typedef	struct	{			/* Size interval		*/
	int		l;		/* lower			*/
	int		u;		/* upper			*/
}	size_int_t;

int Process_Command_Arguments(arg_t *, int *, char ***);

#define	_ARG_H
#endif
