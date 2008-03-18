/*
 * build_cat.c - build a catalog from a text file of VSN definitions.
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

#pragma ident "$Revision: 1.24 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#define	_REENTRANT
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/device.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapes.h"

/* Local includes. */
#include "catlib_priv.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private functions. */
static void LineError(int MsgNum, ...);

/* Private data. */
static int errors = 0;			/* Number of errors encountered */
static int lineno = 0;			/* Input file line number */


int
main(
	int argc,
	char **argv)
{
	struct CatalogEntry *CeTable = NULL;
	struct CatalogHdr *CatHdr;
	FILE	*in_file;
	boolean_t no_media = TRUE;
	boolean_t no_slot = TRUE;
	time_t	now;
	size_t	CeTableSize;	/* Size of table of catalog entries */
	size_t	cat_size;	/* Size of catalog file */
	void	*mp;
	char	*fname;		/* Name of catalog file */
	char	*program_name;	/* Name of this command */
	char	line[1024];
	int	NumofEntries = 0;	/* Number of catalog entries */
	int	c;
	int	cat_media = 0;		/* catalog media (0 if not set) */
	int	max_index = CATALOG_TABLE_MAX;


	program_name = *argv;

	/*
	 * Process arguments.
	 */
	while ((c = getopt(argc, argv, "m:t:")) != EOF) {
		switch (c) {
		case 'm':
		case 't':
			/*
			 * Check the media option for a legal type. If
			 * option not supplied, no enforcement of each catalog
			 * entry type is performed.
			 * In the case of non-sam tapes, the catalog needs to be
			 * built with the true media type and the 'non-sam' bit
			 * set in the catalog.
			 */
			cat_media = sam_atomedia(optarg);
			if (cat_media == 0) {
				fprintf(stderr, "%s: ", program_name);
				fprintf(stderr, GetCustMsg(2755), optarg);
				errors++;
			}
			no_media = FALSE;
			break;
		default:
			errors++;
			break;
		}
	}

	/*
	 * Must have only two arguments left for the file names.
	 */
	if ((argc - optind) > 2) {
		fprintf(stderr, "%s: %s\n", program_name, GetCustMsg(18405));
		errors++;
	}
	if ((errors != 0) || ((argc - optind) < 2)) {
		fprintf(stderr,
		    "%s %s [-t media] <input file> | -  catalog\n",
		    GetCustMsg(4601), program_name);
		exit(EXIT_USAGE);
	}

	/*
	 * The next argument is the input file.
	 * Open it.
	 */
	fname = argv[optind++];
	if (strcmp(fname, "-") == 0) {
		in_file = stdin;
	} else {
		if ((in_file = fopen(fname, "r")) == NULL) {
			fprintf(stderr, "%s: ", program_name);
			fprintf(stderr, GetCustMsg(613), fname);
			perror(" ");
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * The last argument is the catalog name.
	 */
	fname = argv[optind];

	/*
	 * Read the input file and parse lines.
	 */
	now = time(NULL);

	for (lineno = 1; fgets(line, sizeof (line)-1, in_file) != NULL;
	    lineno++) {

		struct CatalogEntry *ce;
		media_t media;
		boolean_t is_bar_code = FALSE;
		boolean_t is_non_sam = FALSE;
		boolean_t flags_supplied = FALSE;
		char	*ntok;
		char	*tok, *tmp;
		int		index;

		/* Perl chomp */
		line[strlen(line)-1] = '\0';

		/*
		 * Ignore comments.
		 * Skip blank lines.
		 */
		if ((tok = strchr(line, '#')) != NULL)  *tok = '\0';
		if ((tok = strtok_r(line, " \t", &ntok)) == NULL)  continue;

		/*
		 * Parse the index.
		 */
		index = strtol(tok, NULL, 0);
		if (index < 0 || index > max_index) {
			LineError(CustMsg(18410), max_index);
			continue;
		}

		if (index >= NumofEntries) {
			int		n;

			/*
			 * Increase the size of the table.
			 */
			n = NumofEntries;
			NumofEntries = index + CATALOG_TABLE_INCR;
			if (NumofEntries > max_index)  NumofEntries = max_index;
			CeTableSize = NumofEntries *
			    sizeof (struct CatalogEntry);
			SamRealloc(CeTable, CeTableSize);
			ce = &CeTable[n];
			memset(ce, 0, (NumofEntries - n) *
			    sizeof (struct CatalogEntry));
		}
		ce = &CeTable[index];

		/*
		 * Test if an input file overwrites a previous entry.
		 * Use the mid field to save the line number for a possible
		 * error message. The mid field will be set when the catalog
		 * is used.
		 */
		if (ce->CeStatus & CES_inuse) {
			LineError(CustMsg(18411), ce->CeMid);
			continue;
		}
		ce->CeMid = lineno;

		/*
		 * Parse the VSN.
		 * dump_cat writes a "?" character into the VSN field in
		 * situations where the VSN is null on a barcoded tape.
		 * Look for this and return a null VSN.
		 */
		if ((tok = strtok_r(NULL, " \t", &ntok)) == NULL) {
			LineError(CustMsg(18412), "VSN");
			continue;
		}
		if (strcmp(tok, "?") != 0) {
			if (strlen(tok) >= sizeof (vsn_t)) {
				LineError(CustMsg(18413));
				continue;
			}
			strcpy(ce->CeVsn, tok);
		}

		/*
		 * Parse the BAR CODE.
		 * bar code may be double-quote delimited since it can
		 * contain white space.  It may not be the only such field.
		 */
		tmp = tok + strlen(tok) + 1;
		tmp = tmp + strspn(tok, " \t");
		if (*tmp == '\"') {	/* quote delimited */
			if (*(tmp+1) != '\"') {	/* if not "" */
				tok = strtok_r(tmp, "\"", &ntok);
				if (tok == NULL) {
					LineError(CustMsg(18412), "barcode");
					continue;
				}
			} else {
				tok = tmp + 1;
				*tok = '\0';
			}
		} else {
			if ((tok = strtok_r(tmp, " \t", &ntok)) == NULL) {
				LineError(CustMsg(18412), "barcode");
				continue;
			}
		}
		if (strcmp(tok, NO_BAR_CODE) != 0) {
			is_bar_code = TRUE;
			strncpy(ce->CeBarCode, tok, BARCODE_LEN);
		}

		/*
		 * Parse media type.
		 */
		tmp = tok + strlen(tok) + 1;
		if ((tok = strtok_r(tmp, " \t", &ntok)) == NULL) {
			LineError(CustMsg(18412), "media");
			continue;
		}

		/*
		 * Check the media type from the input file to see if it is a
		 * foreign tape type, meaning a za thru z9 code. If so, set the
		 * non-sam flag for later use.
		 */
		if (tok[0] == 'z') {
			/*
			 * If this media is a non-sam (type za-z9) media type,
			 * set the catalog non-sam flag and clear the labeled
			 * flag.  Then set * the media type in the catalog to
			 * the true media type as supplied in the build_cat
			 * command line.
			 */
			if (no_media) {
				LineError(CustMsg(18414));
				continue;
			}
			media = cat_media;
			is_non_sam = TRUE;
		} else {
			/*
			 * Not 'z' type, check the media to see if it is a
			 * legal type.
			 */
			if ((media = sam_atomedia(tok)) == 0) {
				LineError(CustMsg(18415), tok);
				continue;
			}

			/*
			 * If the command did not supply the media type,
			 * set catalog media type to first media type.
			 * Otherwise, media type must match the command
			 *  supplied catalog media type.
			 */
			if (no_media) {
				if (cat_media == 0)  cat_media = media;
			} else if (media != cat_media) {
				LineError(CustMsg(18416));
				continue;
			}
		}
		memmove(ce->CeMtype, sam_mediatoa(media), sizeof (ce->CeMtype));

		ce->CeCapacity = ce->CeSpace = DEFLT_CAPC(media);

		/*
		 * The remaining fields are optional.
		 */
		do { /* This is a dummy loop to make use of 'break' */

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->m.CePtocFwa = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeAccess = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeCapacity = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeSpace = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeStatus = strtol(tok, NULL, 0);
			flags_supplied = TRUE;

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeBlockSize = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeLabelTime = (time_t)strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeSlot = strtol(tok, NULL, 0);
			no_slot = FALSE;

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CePart = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeModTime = (time_t)strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->CeMountTime = (time_t)strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  break;
			ce->r.CerTime = strtol(tok, NULL, 0);

			tok = strtok_r(NULL, " \t", &ntok);
			if (tok == NULL)  {
				char	*owner = NULL;
				char	*fsname = NULL;

				if ((owner = strchr(tok, '/')) != NULL) {
					*owner++ = '\0';
					fsname = strchr(owner, '/');
					if (fsname != NULL) {
						*fsname++ = '\0';
					}
				}
				if (owner != NULL && fsname != NULL) {
					strncpy(ce->r.CerAsname, tok,
					    sizeof (ce->r.CerAsname));
					strncpy(ce->r.CerOwner, owner,
					    sizeof (ce->r.CerOwner));
					strncpy(ce->r.CerFsname, fsname,
					    sizeof (ce->r.CerFsname));
				} else {
					ce->r.CerTime = 0;
				}
			} else
				break;

			if (ntok == NULL) break;
			tmp = ntok + strspn(ntok, " \t");
			if (*tmp == '\"') {		/* quote delimited */
				tmp++;
				if ((tok = strtok(tmp, "\"")) == NULL) {
					LineError(CustMsg(18412), "location");
					break;
				}
			} else {
				if ((tok = strtok(tmp, " \t")) == NULL) {
					break;
				}
			}
			if (tok != NULL) {
				strncpy(ce->CeVolInfo, tok, VOLINFO_LEN);
			}

		} while (ce == NULL);	/* dummy compare for lint */

		/*
		 * Validate and adjust fields.
		 */
		if (ce->CeSpace > ce->CeCapacity) {
			LineError(CustMsg(18417));
			continue;
		}

		if (is_tape(media) &&
		    (ce->CeBlockSize < TAPE_SECTOR_SIZE ||
		    ce->CeBlockSize > MAX_TAPE_SECTOR_SIZE)) {
			ce->CeBlockSize = TAPE_SECTOR_SIZE;
		}

		if (is_non_sam) {
			ce->CeStatus =
			    (ce->CeStatus | CES_non_sam) & ~CES_labeled;
		}
		if (!flags_supplied) {
			ce->CeStatus |= CES_inuse | CES_occupied;
		}
		if (is_bar_code)  ce->CeStatus |= CES_bar_code;
	} /* end while reading lines */

	/*
	 * If the input file was was empty, there is a chance that CeTable
	 * was not initialized.  If so, allocate the smallest empty table
	 * based on the increment value.
	 */
	if (CeTable == NULL) {
		NumofEntries = CATALOG_TABLE_INCR;
		CeTableSize = NumofEntries * sizeof (struct CatalogEntry);
		SamRealloc(CeTable, CeTableSize);
	}

	if (errors != 0) {
		/*
		 * Do not write out catalog file if errors encountered.
		 */
		fprintf(stderr, GetCustMsg(18402), errors);
		fprintf(stderr, "\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Create the catalog file.
	 */
	if (CatalogCreateCatfile(fname, NumofEntries, &mp, &cat_size,
	    NULL) == -1) {
		fprintf(stderr, GetCustMsg(18404), fname);
		perror(" ");
		exit(EXIT_FAILURE);
	}

	/*
	 * Fill in the header.
	 * Copy the generated catalog entries.
	 */
	CatHdr = mp;
	memmove(CatHdr->ChMediaType, sam_mediatoa(cat_media),
	    sizeof (CatHdr->ChMediaType));
	CatHdr->ChAuditTime = now;
	memmove(CatHdr->ChTable, CeTable, CeTableSize);
	if (no_slot) {
		int n;

		/*
		 * Generate slot numbers if they were not supplied.
		 * The slot is based on the 331 dump-cat index.
		 * If slot is occupied, use equipment media.
		 * If the catalog was defined for optical media, generate
		 * the side to place in the partition field.
		 */
		for (n = 0; n < NumofEntries; n++) {
			struct CatalogEntry *ce;

			ce = &CatHdr->ChTable[n];
			if (cat_media == DT_ERASABLE ||
			    cat_media == DT_PLASMON_UDO ||
			    cat_media == DT_WORM_OPTICAL ||
			    cat_media == DT_WORM_OPTICAL_12 ||
			    cat_media == DT_OPTICAL) {
				ce->CeSlot = n / 2;
				ce->CePart = n % 2;
			} else
				ce->CeSlot = n;
		}
	}
	/*
	 * Tell about making the catalog.
	 */
	printf(GetCustMsg(18403), cat_size, fname);
	printf("\n");
	return (EXIT_SUCCESS);
}


/*
 * Write error messages for input lines.
 */
static void
LineError(
	int MsgNum,		/* Message number (if 0, message follows) */
	...)
{
	va_list args;

	errors++;
	va_start(args, MsgNum);
	fprintf(stderr, GetCustMsg(18400), lineno);
	fprintf(stderr, "  ");
	if (MsgNum != 0)  vfprintf(stderr, GetCustMsg(MsgNum), args);
	else {
		char *msg;

		msg = va_arg(args, char *);
		vfprintf(stderr, msg, args);
	}
	fprintf(stderr, "  %s\n", GetCustMsg(18401));
	va_end(args);
}
