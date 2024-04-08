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
#pragma	ident	"$Revision: 1.7 $"
static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "pub/devstat.h"
#include "pub/lib.h"
#include "aml/device.h"
#include "aml/catalog.h"
#include "aml/shm.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/robots.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/samapi.h"
#include "sam/lib.h"
#include "sam/devnm.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/nl_samfs.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/device.h"

#include <dlfcn.h>

#define	STKAPILIB	SAM_DIR"/lib/libstkapi.so"

#define	SET_DL_ERR(errmsg) \
	samerrno = errmsg; \
	snprintf(samerrmsg, MAX_MSG_LEN, \
	    GetCustMsg(errmsg), ""); \
	strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);

#define	LOAD_FUNC_FROM_STKAPILIB(func_name, stk_host_info) \
{ static void *lib_handle; void (*lib_func1) ();\
	if (lib_handle == NULL) { \
		lib_handle = dlopen(STKAPILIB, RTLD_LAZY); \
		if (!lib_handle) { \
			SET_DL_ERR(SE_DLOPEN_ERROR); \
			return (-1); \
		} \
	} \
	Trace(TR_DEBUG, "func name is %s\n", func_name); \
	lib_func = (int (*)())dlsym(lib_handle, func_name); \
	if (lib_func == NULL) { \
		SET_DL_ERR(SE_DLSYM_ERROR); \
		return (-1); \
	} \
	if (stk_host_info != NULL) { \
		lib_func1 = (void (*)())dlsym(lib_handle, "set_stk_env"); \
		if (lib_func1 == NULL) { \
			SET_DL_ERR(SE_DLSYM_ERROR); \
			return (-1); \
		} \
		(*lib_func1) (stk_host_info); \
	} \
}


int
get_stk_phyconf_info(
ctx_t *ctx,				/* client connection */
stk_host_info_t *stk_host_info,		/* stk host info */
devtype_t equ_type,			/* type */
stk_phyconf_info_t **stk_phyconf_info   /* return - stk physical conf info */
)
{
	int (*lib_func) ();

	/* Dynamic loading of stkapi, requires setting of stk env also */
	LOAD_FUNC_FROM_STKAPILIB("get_stk_phyconf_info", stk_host_info);

	return ((*lib_func) (ctx, stk_host_info, equ_type, stk_phyconf_info));
}


/*
 * import a list of volumes to the STK ACSLS library
 */
int
stk_import_vsns(
int portnum,
char *fifo_file,
long lpool,
int vol_count,
vsn_t vsn)
{
	int (*lib_func) ();

	LOAD_FUNC_FROM_STKAPILIB("acs_import_vsns", NULL);

	return ((*lib_func) (portnum, fifo_file, lpool, vol_count, vsn));

}

int
discover_stk(
ctx_t *ctx,
sqm_lst_t *stk_host_list,
sqm_lst_t **stk_library_list)
{

	int (*lib_func) ();
	LOAD_FUNC_FROM_STKAPILIB("discover_stk", NULL);
	return ((*lib_func) (ctx, stk_host_list, stk_library_list));
}

int
get_stk_filter_volume_list(
ctx_t *ctx,			/* client connection */
stk_host_info_t *stk_host_info, /* stk host information */
char *in_str,			/* key-value pairs */
sqm_lst_t **stk_volume_list)	/* return - list of stk_volume_t */
{
	int (*lib_func) ();

	LOAD_FUNC_FROM_STKAPILIB("get_stk_filter_volume_list", stk_host_info);

	return ((*lib_func) (ctx, stk_host_info, in_str, stk_volume_list));
}
