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


#include <stddef.h>
#include <sys/vfs.h>


#include "sam/param.h"
#include "sam/types.h"
#include "sam/sys_types.h"
#include "sam/quota.h"
#include "sam/resource.h"
#include "aml/remote.h"
#include "aml/catalog.h"
#include "sblk.h"
#include "share.h"
#include "samhost.h"
#include "dirent.h"
#include "ino_ext.h"
#include "acl.h"
#include "../src/fs/cmd/dump-restore/csd.h"
#include "../src/fs/cmd/dump-restore/old_resource.h"

/* $Revision: 1.14 $ */
