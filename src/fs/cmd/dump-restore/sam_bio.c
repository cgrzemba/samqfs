/*
 *	sam_bio.c - buffered IO routines for sam's csd.
 *
 *	We share a buffer for reads and writes, so you should only be doing
 *	one of these operations at a time.  However, a separate buffer is
 *	used for reading and writing user file data.
 *
 *	To use these routines, simply open an FD to the file you
 *	wish to read or write, then start calling buffered_read or
 *	buffered_write.  Call bflush() when you're all done writing
 *	so the buffer gets flushed out before you close the output
 *	file.
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


#pragma ident "$Revision: 1.7 $"


#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sam/types.h>
#include <sam/lib.h>
#include <sam/sam_malloc.h>
#include <pub/rminfo.h>
#include <sam/fs/ino.h>
#include <errno.h>
#include <string.h>
#include <sam/fs/dirent.h>
#include "csd_defs.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include <syslog.h>

static char *bio_buffer = NULL;
static int bio_buffer_l = 0;
static int buffer_l = 0;
static int bio_buffer_offset = 0;
static int bio_buffer_end = 0;	/* for reads only */

static void allocate_buffer(void);

static char *_SrcFile = __FILE__;


/*
 * buffered replacement for read(2)
 */
size_t
buffered_read(
	int fildes,
	const void *buf,
	size_t nbyte)
{
	int available;
	int offset = 0;
	int length;
	int save = nbyte;

	if (buffer_l == 0) {
		allocate_buffer();
	}
	if (bio_buffer_end < 0) {
		return ((size_t)-1);		/* error encountered before */
	}
	while (nbyte > 0) {
		available = bio_buffer_end - bio_buffer_offset;
		length = nbyte > available ? available : nbyte;
		if (length > 0) {
			memcpy(&(((char *)buf)[offset]),
			    &bio_buffer[bio_buffer_offset], length);
			bio_buffer_offset += length;
			offset += length;
			nbyte -= length;
		} else {
			bio_buffer_end = read(fildes, bio_buffer,
			    bio_buffer_l);
			bio_buffer_offset = 0;
			if (bio_buffer_end == 0) {
				return (0);
			}
			if (bio_buffer_end < 0) {
				return ((size_t)-1);
			}
		}
	}
	return (save);
}


/*
 * buffered replacement for write(2)
 */
size_t
buffered_write(
	int fildes,
	const void *buf,
	size_t nbyte)
{
	int allowed;
	int nput;
	int offset = 0;
	int length;
	int save = nbyte;

	if (buffer_l == 0) {
		allocate_buffer();
	}

	allowed = bio_buffer_l - bio_buffer_offset;
	while (nbyte > 0) {
		length = nbyte > allowed ? allowed : nbyte;
		memcpy(&(((char *)bio_buffer)[bio_buffer_offset]),
		    &(((char *)buf)[offset]), length);
		bio_buffer_offset += length;
		offset += length;
		nbyte -= length;
		if (allowed == 0) {
			nput = write(fildes, bio_buffer, bio_buffer_offset);
			if (nput != bio_buffer_offset) {
				PostEvent(MISC_CLASS, "CannotWrite", 13500,
				    LOG_ERR, catgets(catfd, SET, 13500,
				    "Cannot write to samfsdump file"),
				    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
				error(1, errno, catgets(catfd, SET, 13500,
				    "Cannot write to samfsdump file"));
			}
			bio_buffer_offset = 0;
		}
		allowed = bio_buffer_l - bio_buffer_offset;
	}

	return (save);
}


/*
 * buffered replacement for write(2)
 */
void
bflush(int fildes)
{
	int allowed;
	int nput = 0;
	int to_block;

	if (buffer_l == 0) {
		return;
	}
	if (bio_buffer_offset != 0) {
		if (block_size) {	/* check for blocking factor pad */
			allowed = bio_buffer_l - bio_buffer_offset;
			to_block = allowed % block_size;
			if (to_block >= sizeof (csd_fhdr_t)) {
				csd_fhdr_t pad_hdr;

				pad_hdr.magic = CSD_FMAGIC;
				pad_hdr.flags = CSD_FH_PAD;
				pad_hdr.namelen = to_block -
				    sizeof (csd_fhdr_t);
				memcpy(
				    &(((char *)bio_buffer)[bio_buffer_offset]),
				    (char *)&pad_hdr, sizeof (csd_fhdr_t));
				bio_buffer_offset += sizeof (csd_fhdr_t);
				to_block -= sizeof (csd_fhdr_t);
			}
			if (to_block > 0) {	/* zero to a block boundary */
				memset(
				    &(((char *)bio_buffer)[bio_buffer_offset]),
				    '\0', to_block);
				bio_buffer_offset += to_block;
			}
		}
		nput = write(fildes, bio_buffer, bio_buffer_offset);
		if (nput != bio_buffer_offset) {
			PostEvent(MISC_CLASS, "CannotWrite", 13500,
			    LOG_ERR, catgets(catfd, SET, 13500,
			    "Cannot write to samfsdump file"),
			    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
			error(1, errno, catgets(catfd, SET, 13500,
			    "Cannot write to samfsdump file"));
		}
		bio_buffer_offset = 0;
	}
}


void
writecheck(
	void *buffer,
	size_t size,
	int msgNum)
{
	size_t nput;
	char    msgbuf[MAX_MSGBUF_SIZE];	/* sysevent message buffer */

	if ((nput = buffered_write(CSD_fd, buffer, size)) != size) {
		char	*explanation;

		explanation = catgets(catfd, SET, msgNum, "??");
		/* Send sysevent to generate SNMP trap */
		snprintf(msgbuf, sizeof (msgbuf), catgets(catfd, SET, 13501,
		    "Could only write %d bytes to samfsdump file, "
		    "tried %d, %s"),
		    nput, size, explanation);
		PostEvent(MISC_CLASS, "IncompleteWrite", 13501, LOG_ERR,
		    msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		error(1, errno, "%s", msgbuf);
	}
}


void
readcheck(
	void *buffer,
	size_t size,
	int msgNum)
{
	size_t ngot;

	if (((ngot = buffered_read(CSD_fd, buffer, size)) != size) &&
	    ngot != (size_t)-1) {
		char *explanation;

		explanation = catgets(catfd, SET, msgNum, "??");
		error(1, errno,
		    catgets(catfd, SET, 13502,
		    "Corrupt samfsdump file.  read(%d) returned %d bytes, %s"),
		    size, ngot, explanation);
	} else if (ngot == (size_t)-1) {
		char *explanation;

		explanation = catgets(catfd, SET, msgNum, "??");
		error(1, errno,
		    catgets(catfd, SET, 13503,
		    "System error return from read of samfsdump file, %s"),
		    explanation);
	}
}


/*
 * Allocate buffer space for dumping and restoring
 * csd and file data. The buffer size is a bit of a mish-mash.
 * The calculation is as follows:
 * 1) Take the maximum of read_buffer_size, write_buffer_size and block_size.
 * 2) If double buffering selected, multiple by two.
 * 3) Add 3 * tar header size (the purpose is to estimate a maximum tar
 *	  header size, so to not force unaligned data read and/or writes
 *	  of file data).
 */
void
allocate_buffer()
{
	int bsize;

	bsize = (read_buffer_size > write_buffer_size) ?
	    read_buffer_size : write_buffer_size;
	if (block_size > bsize) {
		bsize = block_size;
	}
	if (Directio) {
		bsize = bsize * 2;	/* double buffer */
	}
	bio_buffer_l = bsize;
	if (block_size) {
		/* force blocking factor */
		bio_buffer_l -= (bsize % block_size);
	}
	bsize += (3 * TAR_RECORDSIZE);
	SamMalloc(bio_buffer, bsize);
	buffer_l = bsize;
}


/*
 * copy_file_data_to_dump - Copy file data from a specified file
 * to the dump file. The copy is to EOF plus a pad to a TAR_RECORDSIZE
 * boundary, if needed. This copy doesn't currently deal with sparse files.
 */
void
copy_file_data_to_dump(int fd, u_longlong_t fsize, char *name)
{
	int allowed;
	int to_block;
	size_t rd_size;
	ssize_t wr_done;

	if (debugging) {
		fprintf(stderr, "copy_file_data_to_dump(%lld)\n", fsize);
	}
	allowed = bio_buffer_l - bio_buffer_offset;
	to_block = allowed % TAR_RECORDSIZE;
	/* if a small file, and it fits */
	if ((u_longlong_t)to_block >= fsize) {
		errno = 0;
		rd_size = (size_t)fsize;
		if (read(fd, &(((char *)bio_buffer)[bio_buffer_offset]),
		    rd_size) != rd_size) {
			error(1, errno, catgets(catfd, SET, 13504,
			    "Error reading from file %s"),
			    name);
		}
		to_block -= rd_size;
		bio_buffer_offset += rd_size;
		fsize = 0;
	}
	if (to_block > 0) {		/* zero to a block boundary */
		memset(&(((char *)bio_buffer)[bio_buffer_offset]), '\0',
		    to_block);
		bio_buffer_offset += to_block;
	}
	for (;;) {
		allowed = bio_buffer_l - bio_buffer_offset;
		if (allowed <= 0) {
			int wr_error = 0;

			errno = 0;
			if (allowed == 0) {
				wr_done = write(CSD_fd, bio_buffer,
				    bio_buffer_offset);
				if (wr_done != bio_buffer_offset) {
					wr_error++;
				}
				bio_buffer_offset = 0;
			} else {
				wr_error++;
			}
			if (wr_error) {
				PostEvent(MISC_CLASS, "CannotWrite", 13500,
				    LOG_ERR, catgets(catfd, SET, 13500,
				    "Cannot write to samfsdump file"),
				    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
				error(1, errno, catgets(catfd, SET, 13500,
				    "Cannot write to samfsdump file"));
			}
		}
		if (fsize == 0) {
			break;
		}
		errno = 0;
		allowed = bio_buffer_l - bio_buffer_offset;
		rd_size = (size_t)((fsize > (u_longlong_t)allowed) ?
		    allowed : fsize);
		if (read(fd, &(((char *)bio_buffer)[bio_buffer_offset]),
		    rd_size) < 0) {
			error(1, errno, catgets(catfd, SET, 13504,
			    "Error reading from file %s"), name);
		}
		bio_buffer_offset += rd_size;
		fsize -= rd_size;
		if (fsize == 0) {	/* pad to TAR_RECORDSIZE boundary */
			allowed = bio_buffer_l - bio_buffer_offset;
			to_block = allowed % TAR_RECORDSIZE;
			if (to_block > 0) {	/* zero to a block boundary */
				memset(
				    &(((char *)bio_buffer)[bio_buffer_offset]),
				    '\0', to_block);
				bio_buffer_offset += to_block;
			}
		}
	}
}


/*
 * copy_file_data_fr_dump - Copy file data from a CSD dump file
 * to the specified (and open) file. The file data will be filled
 * to the next TAR_RECORDSIZE (512) boundary, if needed.
 */
u_longlong_t
copy_file_data_fr_dump(
	int fd,
	u_longlong_t nbyte,
	char *name)
{
	size_t wr_size;
	ssize_t wr_done;
	u_longlong_t wr_total;
	int to_block;
	int pad_not_checked = 1;
	int available;
	int writing_data = 1;

	wr_total = 0;
	if (name == NULL) {
		writing_data = 0;
	}

	for (;;) {
		available = bio_buffer_end - bio_buffer_offset;
		/* fill the buffer, so we have something */
		if (available == 0) {
			errno = 0;
			if (nbyte == 0) {
				break;
			}
			bio_buffer_end = read(CSD_fd, bio_buffer,
			    bio_buffer_l);
			bio_buffer_offset = 0;
			if (bio_buffer_end <= 0) {
				error(1, errno, catgets(catfd, SET, 13503,
				    "System error return from read of "
				    "samfsdump file, %s"),
				    "during file data read");

				if (bio_buffer_end == 0) {
					return (0);
				} else {
					return (1);
				}
			}
			available = bio_buffer_end - bio_buffer_offset;
		}
		if (pad_not_checked) {
			to_block = bio_buffer_offset % TAR_RECORDSIZE;
			if (to_block) {
				to_block = TAR_RECORDSIZE - to_block;
			}
			/* data in space left */
			if (nbyte <= (u_longlong_t)to_block) {
				errno = 0;
				wr_size = (size_t)nbyte;
				if (writing_data) {
					if ((wr_done = write(fd,
					    &bio_buffer[bio_buffer_offset],
					    wr_size)) != wr_size) {
						error(0, errno,
						    catgets(catfd, SET, 13505,
						    "Error writing to data "
						    "file %s"), name);
						return (wr_total);
					}
				} else {
					wr_done = wr_size;
				}
				nbyte = 0;
				wr_total = wr_done;
				to_block -= wr_done;
				bio_buffer_offset += wr_done;
			}
			/* skip pad to boundary */
			bio_buffer_offset += to_block;
			pad_not_checked = 0;
			continue;
		}
		wr_size = (size_t)(((u_longlong_t)available > nbyte) ?
		    nbyte : available);
		if (writing_data) {
			errno = 0;
			if ((wr_done = write(fd,
			    &bio_buffer[bio_buffer_offset],
			    wr_size)) != wr_size) {
				error(0, errno, catgets(catfd, SET, 13505,
				    "Error writing to data file %s"), name);
				return (wr_total);
			}
		} else {
			wr_done = wr_size;
		}
		nbyte -= wr_done;
		wr_total += wr_done;
		bio_buffer_offset += wr_done;
		if (nbyte == 0) {
			if (debugging) {
				fprintf(stderr,
				    "copy_file_data_fr_dump(%lld)\n", wr_total);
				fprintf(stderr,
				    "copy_file_data_fr_dump(%d/%d)\n",
				    bio_buffer_offset, bio_buffer_end);
			}
			available = bio_buffer_end - bio_buffer_offset;
			wr_size = bio_buffer_offset % TAR_RECORDSIZE;
			if (wr_size) {
				bio_buffer_offset += (TAR_RECORDSIZE -
				    wr_size);
			}
			if (debugging) {
				fprintf(stderr,
				    "copy_file_data_fr_dump+(%d/%d)\n",
				    bio_buffer_offset, bio_buffer_end);
			}
			break;
		}
	}
	return (wr_total);
}


/*
 * skip_file_data - skip past dumped file data.
 */
void
skip_file_data(u_longlong_t nbytes)
{
	(void) copy_file_data_fr_dump(-1, nbytes, NULL);
}


/*
 * skip_pad_data - skip past a dumped pad space.
 */
void
skip_pad_data(long nbytes)
{
	int available;
	int skipped;

	while (nbytes) {
		available = bio_buffer_end - bio_buffer_offset;
		/* fill the buffer, so we have something */
		if (available == 0) {
			errno = 0;
			bio_buffer_end = read(CSD_fd, bio_buffer,
			    bio_buffer_l);
			bio_buffer_offset = 0;
			if (bio_buffer_end <= 0) {
				error(1, errno, catgets(catfd, SET, 13503,
				    "System error return from read of "
				    "samfsdump file, %s"),
				    "during dump file pad read");
				return;
			}
			available = bio_buffer_end - bio_buffer_offset;
		}
		skipped = ((long)available > nbytes) ? nbytes : available;
		nbytes -= skipped;
		bio_buffer_offset += skipped;
	}
}
