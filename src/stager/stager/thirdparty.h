/*
 * thirdparty.h - provide support for staging files on third party media
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

#ifndef THIRDPARTY_H
#define	THIRDPARTY_H

#pragma ident "$Revision: 1.14 $"

/*
 * SAM-FS headers.
 */
#include "pub/mig.h"

/*
 * Local headers.
 */
#include "stage_reqs.h"
#include "stream.h"

/*
 * Migration toolkit library.
 */
typedef struct ThirdPartyInfo {
	/*
	 * API entry point names.
	 */
	int (*mig_initialize)(int);
	int (*mig_stage_file_req)(tp_stage_t *);
	int (*mig_cancel_stage_req)(tp_stage_t *);

	dev_ent_t	*dev;
	dtype_t		equ_type;
	StreamInfo_t	*stream;
	pthread_t	tid;

} ThirdPartyInfo_t;

int SendToMigrator(int id);
int InitMigration(dev_ent_t *dev);
void *Migrator(void *arg);
void MigrationRequest(FileInfo_t *file);
void CancelThirdPartyRequest(int id);

#endif /* THIRDPARTY_H */
