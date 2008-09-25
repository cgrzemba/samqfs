/*
 * trap.h - Trap processing definitions
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

#ifndef _AML_TRAP_H
#define	_AML_TRAP_H

#pragma ident "$Revision: 1.11 $"

/*
 * Values for thread_type
 */
#define	NOT_THREADED    0
#define	SOLARIS_THREADS 1
#define	POSIX_THREADS   2

/*
 * This function sets up the signal handler for processing fatal traps.  For
 * the correct trap processing, this function must be called once from the main
 * thread of a process before any other threads are spawned.  This function
 * sets up a signal handler for fatal traps, sets the signal mask appropriate
 * to multithreading type, and registers the cleanup function.
 *
 * Should any thread want to add other signal handlers, they should only
 * use modify the signal mask using a "how" value of SIG_UNBLOCK.
 *
 * Arguments:
 *   thread_type: The type of multithreading the process utilizes.
 *   cleanup:     A pointer to cleanup routine to be invoked before process
 *                termination and core dump.  A NULL pointer indicates
 *                no cleanup needs to be done.
 * Returns:
 *   0:        Initialization was successful.
 *   non-zero: An errno indicating why initialization failed.
 */
int initialize_fatal_trap_processing(int thread_type, void (*cleanup)());

#endif /* _AML_TRAP_H */
