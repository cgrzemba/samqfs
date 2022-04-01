/*
 * catalog.c - get catalog information
 *
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.20 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* SAM-FS public catalog header */
#include "pub/catalog.h"
#include "pub/sam_errno.h"

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

struct catmapping {
	struct CatalogHdr *chp;
	upath_t name;
	int	nopen;
} cat_arr[MAX_CAT] = {
	NULL, "", 0,
	NULL, "", 0,
	NULL, "", 0,
	NULL, "", 0,
	NULL, "", 0
	};

static int first = 1;	/* indicates first catalog open */

/* calls CatalogInit and returns in buf the header of catalog, which is at opening buffered in cat_arr */
int
sam_opencat(const char *path, struct sam_cat_tbl *buf, size_t bufsize)
{
	int	i, size2copy;
	struct sam_cat_tbl tbl;
	struct CatalogHdr *ctp;
	char *media;

	if (path == NULL || buf == NULL || bufsize == 0) {
		errno = EINVAL;
		return (-1);
	}

	if (first) {
		if ((CatalogInit("cat api")) < 0) {
			errno = ER_UNABLE_TO_INIT_CATALOG;
			return (-1);
		}
		first = 0;
	} else {
		/* check to see if we already have this catalog open */
		for (i = 0; i < MAX_CAT; i++) {
			if ((strncmp(cat_arr[i].name, path,
			    sizeof (upath_t))) == 0) {
				/* Already opened and mmapped. */
				cat_arr[i].nopen++;
				ctp = cat_arr[i].chp;
				goto copy;
			}
		}
	}

	/* Requested catalog not already open. */
	/* find first empty array slot */
	i = 0;
	while ((i < MAX_CAT) && (cat_arr[i].chp != NULL)) i++;
	if (i >= MAX_CAT) {
		errno = ENFILE;
		return (-1);
	}

	/* get catalog header */
	ctp = CatalogGetCatalogHeader(path);
	if (ctp == NULL) {
		/* call failed -- errno is set by CatalogGetCatalogHeader */
		return (-1);
	}

	/* fill in catmapping array entry */
	cat_arr[i].chp = ctp;
	strncpy(cat_arr[i].name, path, sizeof (upath_t));
	cat_arr[i].nopen++;

copy:
	/* copy fields of catalog table */
	tbl.audit_time = ctp->ChAuditTime;
	tbl.version = ctp->ChVersion;
	tbl.count = ctp->ChNumofEntries;
	media = ctp->ChMediaType;
	tbl.media[0] = *media++;
	tbl.media[1] = *media;
	tbl.media[2] = '\0';

	size2copy = (bufsize <= sizeof (struct sam_cat_tbl)) ? bufsize :
	    sizeof (struct sam_cat_tbl);

	(void) memcpy((void *)buf, (void *)&tbl, size2copy);

	return (i);
}

/* calls CatalogTerm and cleanup cat_arr entry addressed by cat_handle */
int
sam_closecat(int cat_handle)	/* catalog file desc. returned by sam_opencat */
{
	int i;

	/* check entry in array */
	if ((cat_arr[cat_handle].chp == NULL) ||
	    (cat_arr[cat_handle].nopen <= 0) ||
	    (cat_arr[cat_handle].name[0] == '\0')) {
		errno = EBADF;
		return (-1);
	}

	cat_arr[cat_handle].nopen--;

	if (cat_arr[cat_handle].nopen <= 0) {
		cat_arr[cat_handle].nopen = 0;
		cat_arr[cat_handle].chp = NULL;
		cat_arr[cat_handle].name[0] = '\0';
	}

	/* If no catalogs open, end all catalog processing */
	for (i = 0; i < MAX_CAT; i++) {
		if (cat_arr[i].nopen) break;
	}

	if (i >= MAX_CAT) {
		/* none open */
		CatalogTerm();
		first = 1;
	}

	return (0);
}

int
sam_getcatalog(
	int	cat_handle,	/* catalog handle returned by sam_opencat */
	uint_t	start_ent,	/* first entry in range to return */
	uint_t	end_ent,	/* last entry in range to return */
	struct	sam_cat_ent *buf,
				/* ptr to buffer to hold entries returned */
	size_t	entbufsize)	/* entry bufsize -- size of a single entry */

{
	int		i, n, size2copy;
	struct CatalogHdr	*ctp;		/* catalog table pointer */
	struct CatalogEntry	*cep;		/* catalog entry pointer */
	struct sam_cat_ent sce;			/* public sam catalog entry */

	/* check array entry for this fd */
	if ((cat_arr[cat_handle].chp == NULL) ||
	    (cat_arr[cat_handle].nopen <= 0)) {
		errno = EBADF;
		return (-1);
	}

	ctp = cat_arr[cat_handle].chp;

	if ((start_ent > end_ent) || (buf == NULL)) {
		errno = EINVAL;
		return (-1);
	}

	n = CatalogGetEntries(ctp->ChEq, start_ent, end_ent, &cep);

	if (n < 0) {
		return (-1);
	}

	if (n > (end_ent - start_ent + 1)) {
		/* twilight zone -- got more than we asked for */
		errno = EOVERFLOW;
		return (-1);
	}

	size2copy = entbufsize > sizeof (struct sam_cat_ent) ?
	sizeof (struct sam_cat_ent) : entbufsize;

	for (i = 0; i < n; i++, cep++) {
		memset((void *) &sce, 0, sizeof (struct sam_cat_ent));
		/* fill in fields for each entry */
		sce.type = 0;	/* type not in catalog 3.5.0 and later */
		sce.status = cep->CeStatus;
		strncpy(sce.media, cep->CeMtype, 3);
		sce.media[2] = '\0';
		strncpy(sce.vsn, cep->CeVsn, 32);
		sce.vsn[31] = '\0';
		sce.access = (int)cep->CeAccess;
		sce.capacity = cep->CeCapacity;
		sce.space = cep->CeSpace;
		sce.ptoc_fwa = (int)cep->m.CePtocFwa;
		sce.modification_time = cep->CeModTime;
		sce.mount_time = cep->CeMountTime;
		strncpy((char *)sce.bar_code, cep->CeBarCode, BARCODE_LEN + 1);

		/* copy over, move to the next one */
		memcpy(buf, (char *)&sce, size2copy);
		buf = (struct sam_cat_ent *)(void *)((char *)buf + entbufsize);
	}

	return (n);
}
