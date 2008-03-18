/*
 * readcfg.h - Read configuration file definitions.
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

#if !defined(READCFG_H)
#define	READCFG_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif

#define	TOKEN_SIZE 256
#define	LINE_LENGTH 1024

/* Structures. */

/* Directive processing table entry */
typedef struct DirProc {
	char	*DpName;	/* Directive name - NULL for table terminator */
	void	(*DpFunc)(void); /* Processing function */
	int	DpType;		/* Type of processing to perform */
	int	DpMsgNum;	/* Command specific catalog message number */
} DirProc_t;

/* Directive processing types. */
typedef enum DpTypes {
	DP_value = 1,		/* Perform processing for 'name' form */
	DP_set,			/* Perform processing for 'name = value' form */
	DP_setfield,		/* Perform processing for "setfield.c" */
	DP_param,		/* Perform processing for 'name value' form */
	DP_other		/* Perform other processing */
} DpTypes_t;

/* Public functions. */
int ReadCfg(char *FileName, DirProc_t *Table, char *Dirname, char *Token,
	void (*MsgFunc)(char *msg, int lineno, char *line));
void ReadCfgError(int MsgNum, ...);
int ReadCfgGetToken(void);
void ReadCfgLookupDirname(char *dirname, DirProc_t *dirProcTable);
void ReadCfgSetTable(DirProc_t *Table);

#endif /* READCFG_H */
