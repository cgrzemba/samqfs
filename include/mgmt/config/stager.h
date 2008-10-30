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
#ifndef _CFG_STAGER_H
#define	_CFG_STAGER_H

#pragma	ident	"$Revision: 1.12 $"
/*
 * stager.h
 * used to read and change the stager configuration.
 */
#include "pub/mgmt/stage.h"

/* Logfile Event Configuration Strings */
#define	ST_START_STR	"start"
#define	ST_ERROR_STR	"error"
#define	ST_CANCEL_STR	"cancel"
#define	ST_FINISH_STR	"finish"
#define	ST_ALL_STR	"all"


/*
 * Read stager configuration from the default location and build the
 * stager_cfg_t structure.
 */
int read_stager_cfg(stager_cfg_t **cfg);


/*
 * sets samerrno for the first error it encounters and returns -1.
 * if no errors encountered 0 is returned.
 */
int verify_stager_cfg(const stager_cfg_t *cfg);


/*
 * Write the stager configuration to the default location.
 *
 * If force is false and the configuration has been modified since
 * the stager_cfg_t struture was read in, this function will
 * set errno and return -1
 *
 * If force is true the stager configuration will be writen without
 * regard to its modification time.
 */
int write_stager_cfg(const stager_cfg_t *cfg, const boolean_t force);

/*
 * dump the configuration to the specified location.  If a file exists at
 * the specified location it will be overwriten.
 */
int dump_stager_cfg(const stager_cfg_t *cfg, const char *location);


void free_stager_cfg(stager_cfg_t *stage_cfg_p);
void free_stage_drive_list(sqm_lst_t *stage_drive_list);
void free_stage_buffer_list(sqm_lst_t *stage_buffer_list);

/* from parse_stager.c */
int check_bufsize(const buffer_directive_t *b);
int check_stage_drives(const drive_directive_t *d, sqm_lst_t *libraries);
int check_maxretries(const stager_cfg_t *s);
int check_maxactive(const stager_cfg_t *s);
int check_logfile(const char *logfile);
int check_stager_stream(stream_cfg_t *ss_cfg);

int write_stager_cmd(const char *location, const stager_cfg_t *s);
int parse_stager_cmd(char *cmd_file, sqm_lst_t *libraries, stager_cfg_t **cfg);

#endif /* _CFG_STAGER_H */
