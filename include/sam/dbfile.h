/*
 * dbfile.h - SAM database file access definitions.
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

#if !defined(SAM_DBFILE_H)
#define	SAM_DBFILE_H

#ifdef sun
#pragma ident "$Revision: 1.11 $"
#endif

/*
 * Macros.
 */
#define	DBFILE_IS_INIT(x) (x != NULL && x->db != NULL)
#define	DBITERATOR_IS_INIT(x) (x != NULL && x->dbc != NULL)

/*
 * Open flags.
 */
enum {
	DBFILE_CREATE = 1 << 0,		/* Create db */
	DBFILE_RDONLY = 1 << 1		/* Read only */
};

/*
 * Forward declarations.
 */
struct DBFile; typedef struct DBFile DBFile_t;

/*
 *	Define prototypes in dbfile.c
 */
int DBFileInit(DBFile_t **dbfile, char *homedir, char *datadir, char *progname);
int DBFileDestroy(DBFile_t *dbfile);
void DBFileRecover(char *homedir, char *datadir, char *progname);
void DBFileTrace(DBFile_t *dbfile, char *srcFile, int srcLine);

struct DBFile {
	void	*db;			/* database handle */
	void	*dbenv;			/* environment */

	void	*dbc;

	int	(*Open)(DBFile_t *, char *, char *, int);
	int	(*Put)(DBFile_t *, void *, unsigned int, void *,
		unsigned int, int);
	int	(*Get)(DBFile_t *, void *, unsigned int, void **);
	int	(*Del)(DBFile_t *, void *, unsigned int);
	int	(*Close)(DBFile_t *);

	int	(*BeginIterator)(DBFile_t *);
	int	(*GetIterator)(DBFile_t *, void **dbkey, void **dbdata);
	int	(*EndIterator)(DBFile_t *);

	int	(*Numof)(DBFile_t *, int *);
};

#endif /* SAM_DBFILE_H */
