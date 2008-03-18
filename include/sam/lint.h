/*
 * lint.h - Definitions for quieting lint.
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
 * NOTE:  This include file must be last.  If not, it will cause strange,
 * unexpected, and erroneous lint errors.
 */

#if !defined(LINT_H)
#define	LINT_H

#ifdef sun
#pragma ident "$Revision: 1.15 $"
#endif

#if defined(lint)

/* Macros to void return values for functions. */

#define	alarm (void)alarm
#define	atexit (void)atexit
#define	fclose (void)fclose
#define	fprintf (void)fprintf
#define	fputs (void)fputs
#define	execl (void)execl
#define	execv (void)execv
#define	kill (void)kill
#define	memccpy (void)memccpy
#define	memcpy (void)memcpy
#define	memmove (void)memmove
#define	memset (void)memset
#define	printf (void)printf
#define	shmdt (void)shmdt
#define	sigaddset (void)sigaddset
#define	sigemptyset (void)sigemptyset
#define	sigdelset (void)sigdelset
#define	sigfillset (void)sigfillset
#define	sigprocmask (void)sigprocmask
#define	sleep (void)sleep
#define	snprintf (void)snprintf
#define	sprintf (void)sprintf
#define	strftime (void)strftime
#define	strncpy (void)strncpy
#define	strcat (void)strcat
#define	strncat (void)strncat
#define	strcpy (void)strcpy
#define	unlink (void)unlink
#define	vfprintf (void)vfprintf
#define	vsprintf (void)vsprintf
#define	vsnprintf (void)vsnprintf

#define	mutex_lock (void)mutex_lock
#define	mutex_unlock (void)mutex_unlock

#endif	/* lint */

#endif /* LINT_H */
