/*
 * signals.h - Signals processing definitions.
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

#ifndef SIGNALS_H
#define	SIGNALS_H

#ifdef sun
#pragma ident "$Revision: 1.13 $"
#endif

/* Signal handling table entry. */
typedef void (*SfFunc_t)(int sig);
struct SignalFunc {
	int		SfSig;	/* Signal number */
	SfFunc_t	SfFunc;	/* Function to call when signal received */
};

/* Signals argument */
struct SignalsArg {
	struct SignalFunc *SaSignalFuncs;	/* Signal handling table */
	SfFunc_t SaFatal;	/* Fatal signal function */
	upath_t SaCoreDir;	/* Directory for core and ptrace files */
};

/* Functions. */
pthread_t Signals(struct SignalsArg *arg);

#endif /* SIGNALS_H */
