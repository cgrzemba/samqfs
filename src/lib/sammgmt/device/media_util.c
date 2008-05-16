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
#pragma	ident	"$Revision: 1.40 $"
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
#include "acssys.h"
#include "acsapi.h"

/*
 * media_util.c should be renamed to indicate that the functions in this file
 * interface with SAM Shared memory to get the configuration parameters or
 * issue requests to carry out operations. The functions in this file are
 * tightly coupled with the SAM media code and as such serve as a wrapper
 * to SAM media functionality.
 */

#define	VSN_LENGTH	6

/*
 * get all the libraries that have been added to SAM
 *
 * Obtain the libraries from SAM shared memory, if this fails, read the mcf
 * file to generate the list of libraries that would potentially be added to SAM
 *
 * If mcf file and SAM's shared memory are not in sync, i.e. user has manually
 * modified the mcf file, but sam-amld daemon is not restarted for the change to
 * take effect, this function reports just what is in SAM's shared memory
 * TBD: Should this function flag the mismatch?
 *
 */
int
get_all_libraries(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t **lib_lst)	/* OUTPUT - list of structure library_t */
{
	node_t		*n;
	library_t	*mcf_lib;


	Trace(TR_MISC, "get libraries");

	if (ISNULL(lib_lst)) {
		Trace(TR_ERR, "get libraries failed: %s", samerrmsg);
		return (-1);
	}

	if (get_libraries_from_shm(lib_lst) != 0) {

		Trace(TR_ERR, "get libraries from shm failed: %s", samerrmsg);

		/* attempt to get from the configuration files */
		if (get_all_libraries_from_MCF(lib_lst) == -1) {

			Trace(TR_ERR, "get libraries from mcf failed: %s",
			    samerrmsg);
			return (-1);
		}

		for (n = (*lib_lst)->head; n != NULL; n = n->next) {

			mcf_lib = (library_t *)n->data;

			// verify physical connectivity for direct attached libs
			if (is_special_type(mcf_lib->base_info.equ_type) != 0) {
				if (verify_library(mcf_lib) == -1) {

					Trace(TR_OPRMSG, "unable to verify the "
					    "library %s: %s",
					    mcf_lib->base_info.name,
					    samerrmsg);
				}
			}
		}
		Trace(TR_MISC, "get libraries from mcf complete");
		return (0);
	}

	Trace(TR_MISC, "get libraries complete");
	return (0);
}


/*
 * get all standalone drives that have been added to SAM
 *
 * Obtain the standalone drives from SAM's shared memory, If this fails,
 * read the mcf file to generate a list of standalone drives that would
 * potentially be added to SAM when samd is restarted.
 */
int
get_all_standalone_drives(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t **drv_lst)	/* a list of drive_t */
{
	node_t		*n;
	drive_t		*drive;

	Trace(TR_MISC, "get all standalone drives");

	if (ISNULL(drv_lst)) {
		Trace(TR_ERR, "get standalone drives failed: %s", samerrmsg);
		return (-1);
	}

	if (get_sdrives_from_shm(drv_lst) != 0) {

		Trace(TR_ERR, "get sdrives from shm failed: %s", samerrmsg);

		/* attempt to get from the configuration files */
		if (get_all_standalone_drives_from_MCF(drv_lst) == -1) {
			Trace(TR_ERR, "get sdrives failed: %s", samerrmsg);
			return (-1);
		}
		for (n = (*drv_lst)->head; n != NULL; n = n->next) {
			drive = (drive_t *)n->data;
			if (verify_standalone_drive(drive) == -1) {
				Trace(TR_ERR, "get sdrive failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
		return (0);
	}

	Trace(TR_MISC, "get standalone drives success");
	return (0);
}


/*
 * get all media types (2 letter mnemonic) that are available in this config
 */
int
get_all_available_media_type(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t **mtype_lst)	/* OUTPUT - list of mtype */
{
	mtype_t		mtype;

	Trace(TR_MISC, "get available mtype");

	if (ISNULL(mtype_lst)) {

		Trace(TR_ERR, "get avail mtype failed: %s", samerrmsg);
		return (-1);
	}

	if (get_available_mtype_from_shm(mtype_lst) != 0) {

		Trace(TR_MISC, "get avail mtype from config file");

		if (get_all_available_media_type_from_mcf(mtype_lst) == -1) {

			Trace(TR_ERR, "get available mtype failed: %s",
			    samerrmsg);
			return (-1);
		}

		Trace(TR_MISC, "get avail mtype from config file complete");
		return (0);
	}

	Trace(TR_MISC, "get avail mtype complete");
	return (0);
}


/*
 * Validate ANSI tape label field.
 * The labels must conform to ANSI X3.27-1987 File Structure and Labeling of
 * Magnetic Tapes for Information Interchange. The VSN must be one to six
 * characters in length. All characters must be selected from the 26 upper-case
 * letters, the 10 digits, and the following special characters:
 * !"%&'()*+,-./:;<=>?_
 * Return 1 if the string is valid, 0 if invalid
 */
int
is_ansi_tp_label(
char *s,	/* INPUT - string to be validated */
size_t size)	/* INPUT - length of string to be validated */
{
	char c;

	if (strlen(s) > size) {
		return (0);
	}

	while ((c = *s++) != '\0') {
		if (isupper(c)) {
			continue;
		}
		if (isdigit(c)) {
			continue;
		}
		if (strchr("!\"%&'()*+,-./:;<=>?_", c) == NULL) {
			return (0);
		}
	}
	return (1);
}


/*
 *	get_vsn_num() will get the number in a vsn. It always is
 *	the number beginning from a number after a character to the
 *	end.  From example: abc099 --> 99; a9b7c9 --> 9; vsn909 --> 909.
 */
int
get_vsn_num(
vsn_t begin_vsn,	/* the beginning vsn */
int *b_vsn_loc)		/* the vsn's number location of vsn string */
{

	int i;
	int new_beg_vsn_num;
	vsn_t new_beg_vsn;
	*b_vsn_loc = 0;

	for (i = strlen(begin_vsn) - 1; i >= 0; i--) {
		if (!isdigit(begin_vsn[i])) {
			*b_vsn_loc = i + 1;
			break;
		}
	}
	for (i = *b_vsn_loc; i < strlen(begin_vsn); i++) {
		new_beg_vsn[i - *b_vsn_loc] = begin_vsn[i];

	}
	new_beg_vsn[strlen(begin_vsn) - *b_vsn_loc] = '\0';
	new_beg_vsn_num = atoi(new_beg_vsn);
	return (new_beg_vsn_num);
}


/*
 *	Given a vsn, vsn's character length, and the number need
 *	to be added to that vsn, generate a new vsn and this new vsn
 *	will be used for import.
 */
char *
gen_new_vsn(
vsn_t given_vsn,		/* given VSN */
int char_len,			/* VSN string's character's length */
int add_num,			/* the number need to be added to VSN */
vsn_t new_vsn)			/* the generated new vsn name */
{
	int i;
	int new_num;
	int lead_zero = 0;
	vsn_t end_part;

	new_num = add_num + 1;
	for (i = 0; i < char_len; i++) {
		new_vsn[i] = given_vsn[i];
	}
	new_vsn[char_len] = '\0';
	snprintf(end_part, sizeof (end_part), "%d", new_num);
	lead_zero = strlen(given_vsn) - strlen(new_vsn) - strlen(end_part);
	for (i = 0; i < lead_zero; i ++) {
		strlcat(new_vsn, "0", sizeof (vsn_t));
	}
	strlcat(new_vsn, end_part, sizeof (vsn_t));
	return (new_vsn);
}


/*
 * the following robot types are considered to be a special type
 */
static char *dev_sp[] =
	{"sk", "im", "pe", "gr", "fj", "hy", "ss", "sc", "rd", NULL};


/*
 * check if the robot type is considered to be a special type
 * If it is a special type, when samd is not running, MCF
 *	information will be directly given without verification.
 */
int
is_special_type(
char *type)	/* equipment type */
{
	char *rb_type;
	int i = 0;

	for (rb_type = dev_sp[i]; rb_type != NULL; rb_type = dev_sp[i++]) {
		if (strcmp(rb_type, type) == 0) {
			return (0);
		}
	}
	return (-1);
}
