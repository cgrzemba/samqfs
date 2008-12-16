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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef	_RECYCLER_SH_WRAPPER_H
#define	_RECYCLER_SH_WRAPPER_H

#pragma ident	"$Revision: 1.17 $"

/*
 * recycler_sh_wrapper.h.
 * The header file for the
 * wrapper to the recycler.sh.
 * This module principally
 * reports on the settings within
 * recycler.sh of 'label' and 'export'
 * commands in the recycler.sh file.
 * The module is highly dependant
 * on the default format of the
 * file.
 */


#include "pub/mgmt/types.h"

#define	RECYC_SH SCRIPT_DIR"/recycler.sh"
#define	RECYC_SH_EX OPT_DIR"/examples/recycler.sh"

#define	RC_label_on	0x00000001
#define	RC_export_on	0x00000010

/* the label command-pattern */
#define	LABEL SBIN_DIR"/${1}label"

/* a substring of the 'export' mail-message */
#define	EXPORT "ready to be shelved off-site"


/* Store the common header text */
#define	HEADER "#!/bin/csh -f" \
"\n#   "SCRIPT_DIR"recycler.sh - post-process a VSN after recycler has" \
"\n#   drained it of all known active archive copies." \
"\n#   Arguments are:" \
"\n#      $1 - generic media type \"od\" or \"tp\" - used to construct the" \
"\n#           name of the appropriate label command: odlabel or tplabel" \
"\n#      $2 - VSN being post-processed" \
"\n#      $3 - MID in the library where the VSN is located" \
"\n#      $4 - equipment number of the library where the VSN is located" \
"\n#      $5 - actual media type (\"mo\", \"lt\", etc.) - used to chmed" \
"\n#           the media if required" \
"\n#      $6 - family set name of the physical library, or the string" \
"\n#           \"hy\" for the historian library.    This can be used to" \
"\n#           handle recycling of off-site media, as shown below." \
"\n#	   $7 - VSN modifier, used for optical and D2 media" \
"\n#" \
"\n#\n"

/* Store the label text */
#define	LABEL_ON_BLOCK "set stat=0" \
"\nif ( $6 != hy ) then" \
"\n    "SBIN_DIR"/chmed -R $5.$2" \
"\n    "SBIN_DIR"/chmed -W $5.$2" \
"\n    if ( $5 != \"d2\" ) then" \
"\n        if ( $1 != \"od\" ) then" \
"\n            "SBIN_DIR"/${1}label -w -vsn $2 -old $2 $4\\:$3" \
"\n			if ( $status != 0 ) then" \
"\n			    set stat = 1" \
"\n			endif" \
"\n        else" \
"\n            "SBIN_DIR"/${1}label -w -vsn $2 -old $2 $4\\:$3\\:$7" \
"\n			if ( $status != 0 ) then" \
"\n			    set stat = 1" \
"\n			endif" \
"\n		endif" \
"\n    else" \
"\n        "SBIN_DIR"/${1}label -w -vsn $2 -old $2 $4\\:$3\\:$7" \
"\n		if ( $status != 0 ) then" \
"\n			set stat = 1" \
"\n		endif" \
"\n    endif" \
"\nelse" \
"\n    mail root <</eof" \
"\nVSN $2 of type $5 is devoid of active archive" \
"\nimages.  It is currently in the historian catalog, which indicates that" \
"\nit has been exported from the on-line libraries." \
"\n" \
"\nYou should import it to the appropriate library, and relabel it using" \
"\n${1}label." \
"\n" \
"\nThis message will continue to be sent to you each time the recycler" \
"\nruns, until you relabel the VSN, or you use the SAM-FS samu" \
"\nprograms to export this medium from the historian catalog to" \
"\nsuppress this message." \
"\n/eof" \
"\nendif" \
"\necho `date` $* done >>  "VAR_DIR"/recycler.sh.log" \
"\nif ( $stat != 0 ) then" \
"\n	exit 1" \
"\nelse" \
"\n	exit 0" \
"\nendif\n"

/* Store the export text; allow for emailaddresses to be passed in */
#define	EXPORT_ON_BLOCK(emailaddr) (strcmp(emailaddr, "none") == 0) ? \
		"\nmail root <</eof" \
		"\nVSN $2 in library $4 is ready to be shelved off-site." \
		"\n/eof" \
		"\necho `date` $* done >>  "VAR_DIR"/recycler.sh.log" \
		"\nexit 0"  : \
		"\nmail %s <</eof" \
		"\nVSN $2 in library $4 is ready to be shelved off-site." \
		"\n/eof" \
		"\necho `date` $* done >>  "VAR_DIR"/recycler.sh.log" \
		"\nexit 0\n" \


/*
 * get what action [label/export] have been configured.
 *
 * Parameters
 *	ctx_t ctx	the context parameter.
 */
int get_recycl_sh_action_status(ctx_t *ctx);

/*
 * set label "on"
 *
 * Parameters:
 *	ctx_t ctx	the context parameter.
 */
int add_recycle_sh_action_label(ctx_t *ctx);

/*
 * set the export notification "on."
 *
 * Parameters:
 *	ctx_t ctx	the context parameter
 *	uname_t	emailaddr	the email addr passed in.
 */
int add_recycle_sh_action_export(ctx_t *ctx, uname_t emailaddr);

/*
 * remove action.
 *
 * Parameters:
 *	ctx_t *ctx	the context parameter.
 */
int del_recycle_sh_action(ctx_t *ctx);

#endif	/* _RECYCLER_SH_WRAPPER_H */
