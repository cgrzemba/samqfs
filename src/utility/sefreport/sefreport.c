/*
 * sefreport.c
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

#pragma ident "$Revision: 1.22 $"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <sys/int_types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sam/types.h"
#include "aml/sefstructs.h"
#include "aml/sefvals.h"
#include "aml/external_data.h"


#define	WRITE_ERROR_LOG_PG "Write Errors Counter"
#define	READ_ERROR_LOG_PG  "Read Errors Counter"
#define	NON_MEDIA_LOG_PG  "Non-Medium Error"

/* Write Error Counters log page 3. */
char *write_error_log_pg [] = {
	/* 0 */ "Errors corrected without substantial delay",
	/* 1 */ "Errors corrected with possible delays",
	/* 2 */ "Total rewrites",
	/* 3 */ "Total errors corrected",
	/* 4 */ "Total times correction algorithm processed",
	/* 5 */ "Total bytes processed",
	/* 6 */ "Total uncorrected errors"
};

/* Read Error Counters log page 3. */
char *read_error_log_pg [] = {
	/* 0 */ "Errors corrected without substantial delay",
	/* 1 */ "Errors corrected with possible delays",
	/* 2 */ "Total rewrites",
	/* 3 */ "Total errors corrected",
	/* 4 */ "Total times correction algorithm processed",
	/* 5 */ "Total bytes processed",
	/* 6 */ "Total uncorrected errors"
};

/* Non-Media Error log page 6. */
char *non_media_log_pg [] = {
	/* 0 */ "Non-medium error count"
};

int
main(
	int	argc,
	char *argv[]
)
{
	extern int	optind;
	void		usage();
	int			checkheader(struct sef_hdr *, int);
	int			checkrecord(char *, int, int);

	uchar_t		paramlen;
	char		*seffile = NULL;
	char		*timestr, *buf, *pageptr, paramstr[7];
	int			readcount, seffd, c, rec;
	int			i, bytesprinted, verbose = 0, devinfo = 0;
	int			desc = 0;
	uint16_t	pagelen, equip;
	uint16_t	paramcode;
	struct sef_hdr	header;
	struct sef_pg_hdr *pagehdr;
	int			field;
	struct stat	statbuf;
	char		**log_pg;
	int			paramcodemax;
	char		*pagename;
	char		*seffilep = NULL;


	while ((c = getopt(argc, argv, "d:vt")) != EOF) {
		switch (c) {
			case 'v':
				verbose = 1;
				break;
			case 'd':
				devinfo = 1;
				seffilep = optarg;
				break;
			case 't':
				desc = 1;
				break;
			case '?':
			default:
				usage(argv[0]);
				exit(1);
		} /* end switch */
	} /* end while */

	if (verbose) {
		desc = 0;
	}

	if (seffilep == NULL) {
		/* No file name */
		usage(argv[0]);
		exit(1);
	}

	seffile = strdup(seffilep);

	if (optind != argc) {
		/* extra stuff at end of command */
		usage(argv[0]);
		exit(1);
	}

	if ((seffd = open(seffile, O_RDONLY)) < 0) {
		printf("Error opening <%s>, errno <%d>.\n",
		    seffile, errno);
		exit(1);
	}

	if (fstat(seffd, &statbuf) < 0) {
		printf("Error determing filesize <%s>, errno <%d>.\n",
		    seffile, errno);
		exit(1);
	}

	if (statbuf.st_size <= 0) {
		/* 0 byte length sefdata file. */
		exit(0);
	}

	/*
	 * Read first sef record header.  This is struct sef_hdr.
	 */
	readcount = read(seffd, &header, sizeof (struct sef_hdr));
	if (checkheader(&header, readcount) < 0) {
		exit(1);
	}
	rec = 1;

	while (readcount != 0) {

		timestr = strdup(ctime(&header.sef_timestamp));
		/* get rid of annoying trailing \n that ctime puts on */
		timestr[strlen(timestr)-1] = '\0';
		printf("\nRecord no. %d\n", rec);
		printf("%s  %s %s %s VSN %s\n", timestr, header.sef_vendor_id,
		    header.sef_product_id, header.sef_revision, header.sef_vsn);
		BE16toH(&header.sef_eq, &equip);
		if (devinfo) {
			printf("   Eq no. %d   Dev name %s\n",
			    equip, header.sef_devname);
		}
		printf("\n");
		/* end of sef record header contents */

		/*
		 * the sef_size field in the record header is
		 * the total size of all pages in this record
		 */
		if ((buf = (char *)malloc(header.sef_size)) == NULL) {
			printf("ERROR:  Cannot malloc; errno %d.\n", errno);
			exit(1);
		}

		readcount = read(seffd, buf, header.sef_size);
		if (checkrecord(buf, readcount, header.sef_size) < 0) {
			exit(1);
		}

		pagehdr = (struct sef_pg_hdr *)buf;
		while (readcount > 0) {
			BE16toH(&pagehdr->page_len, &pagelen);


			/* setup log page decode text. */
			log_pg = NULL;
			paramcodemax = -1;
			pagename = NULL;
			if (pagehdr->page_code == SEF_WR_ERR_LOG_PG) {
				pagename = WRITE_ERROR_LOG_PG;
				log_pg = write_error_log_pg;
				paramcodemax = 6;
			} else if (pagehdr->page_code == SEF_RD_ERR_LOG_PG) {
				pagename = READ_ERROR_LOG_PG;
				log_pg = read_error_log_pg;
				paramcodemax = 6;
			} else if (pagehdr->page_code == SEF_NON_MEDIA_LOG_PG) {
				pagename = NON_MEDIA_LOG_PG;
				log_pg = non_media_log_pg;
				paramcodemax = 0;
			}

			if (!verbose) {
				printf("   PAGE CODE %x", pagehdr->page_code);
				if (pagename && desc) {
					printf(" %s", pagename);
				}
				printf("\n");
			} else {
				printf(" rec  pg cd");
			}

			/* print contents of page */
			printf("   param code  control   param value");
			if (desc) {
				printf("  description");
			}
			printf("\n");

			pageptr = (char *)pagehdr + sizeof (struct sef_pg_hdr);
			bytesprinted = 0;
			while (bytesprinted < pagelen) {
				if (verbose) {
					/*
					 * print record number and page code
					 * on each line
					 */
					printf("%4d  %3x  ",
					    rec, pagehdr->page_code);
				}
				/*
				 * print parameter code -- 2 bytes long,
				 * ranging in value from 01h to 8000h
				 */
				paramcode = (pageptr[0] << 8) | pageptr[1];
				sprintf(paramstr, "%02xh", paramcode);
				printf("   %7s   ", paramstr);
				pageptr = (char *)pageptr + 2;
				bytesprinted += 2;

				/*
				 * print control byte -- one byte
				 * print as upper 4 bits followed by
				 * lower 4 bits
				 */
				printf("    %x%xh  ", *(uchar_t *)pageptr >> 4,
				    *(uchar_t *)pageptr & 0xF);
				pageptr = (char *)pageptr + 1;
				bytesprinted += 1;

				/*
				 * print paramter value.  length varies
				 */
				paramlen = *(uchar_t *)pageptr;
				/*
				 * Advance pageptr past parameter length byte
				 */
				pageptr = (char *)pageptr + 1;
				bytesprinted += 1;

				/*
				 * Avoid odd byte memory alignment. Display
				 * paramdata without leading zeros. If
				 * paramdata is zero, then display one zero.
				 */
				printf("   0x");
				field = 0;
				for (i = 0; i < paramlen; i++) {
					if (field == 0 && pageptr [i] != 0) {
						/* First byte of number. */
						printf("%x",
						    (uchar_t)pageptr [i]);
						field = ((pageptr [i] >> 4) !=
						    0 ? 2 : 1);
					} else if (field != 0) {
						/*
						 * Pad middle of number with
						 * zeros.
						 */
						printf("%02x",
						    (uchar_t)pageptr [i]);
						field += 2;
					}
				}
				if (field == 0) {
					/* Zero number. */
					printf("0");
					field = 1;
				}

				/* fill out param value field width. */
				for (i = field; i < 11; i++) {
					printf(" ");
				}

				if (verbose) {
					printf("(%d:%x:%s)",
					    equip, pagehdr->page_code,
					    header.sef_vsn);
				} else if (log_pg && desc &&
				    (paramcode >= 0 && paramcode <=
				    paramcodemax)) {
					printf("%s", log_pg [paramcode]);
				}

				pageptr = (char *)pageptr + (int)paramlen;
				bytesprinted += (int)paramlen;

				printf("\n");
			} /* end while */  /* one log sense page printed */

			printf("\n\n");
			/* advance pagehdr pointer */
			pagehdr = (struct sef_pg_hdr *)((char *)pagehdr +
			    sizeof (struct sef_pg_hdr) + pagelen);
			readcount -= (sizeof (struct sef_pg_hdr) + pagelen);
		} /* end while */


		/*
		 * Free the malloc'd buffer for this sef record (these
		 * log sense pages).
		 */
		free(buf);

		/*
		 * Read next sef record header.  This is struct sef_hdr.
		 */
		readcount = read(seffd, &header, sizeof (struct sef_hdr));
		if (checkheader(&header, readcount) < 0) {
			exit(1);
		}
		rec++;

	} /* end while */

	return (0);

} /* end main */

int
checkheader(
	struct sef_hdr *hdr,
	int	bytecount
)
{
	if (bytecount < 0) {
		printf("ERROR:  Error reading sef file; errno %d.\n", errno);
		return (-1);
	}

	if ((bytecount > 0) && (bytecount != sizeof (struct sef_hdr))) {
		printf("ERROR:  Read %d bytes, expected %d.\n", bytecount,
		    sizeof (struct sef_hdr));
		return (-1);
	}

	if (hdr->sef_magic != SEFMAGIC) {
		printf("ERROR:  Bad magic.\n");
		return (-1);
	}

	if (hdr->sef_version != SEFVERSION) {
		printf("ERROR:  Bad version.  Got %d, expected %d.\n",
		    hdr->sef_version, SEFVERSION);
		return (-1);
	}

	return (0);
} /* end checkheader() */

void
usage(char *s)
{
	printf("USAGE:  %s [-v|-t] -d file\n", s);
}

int
checkrecord(
	char *buffer,
	int bytesread,
	int sizeexpected
)
{
	struct sef_pg_hdr *phdr;

	if (bytesread == 0) {
		printf("ERROR:  Unexpected end of file reached.\n");
		return (-1);
	}

	if (bytesread != sizeexpected) {
		printf("ERROR:  Read %d bytes, expected %d.\n",
		    bytesread, sizeexpected);
		return (-1);
	}

	phdr = (struct sef_pg_hdr *)buffer;
	if ((phdr->page_code < 0) || (phdr->page_len <= 0)) {
		printf("ERROR:  Log sense page header contents "
		    "make no sense.\n");
		return (-1);
	}

	return (0);
}
