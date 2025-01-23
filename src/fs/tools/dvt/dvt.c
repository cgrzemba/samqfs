/*
 * dvt - device verification test.
 *
 * This is a file system disk benchmark which times large block sequential I/O.
 *
 * dvt writes to a file using a specified block size and block count.
 * Then it reads the file. The write and read transfer rate is displayed.
 * An option exists to only write a new file or only read an existing file.
 *
 * See man dvt for a detailed description of the options.
 *
 * dvt uses the concept of thread pools. dvt (the boss thread) creates a
 * specified number of threads up front. These worker threads survive for
 * the duration of the program. dvt creates I/O requests on puts these
 * requests on a work queue. Worker threads remove I/O requests from the
 * work queue and process them. When a worker thread completes an I/O
 * request, it simply removes another one from the work queue.
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

#pragma ident "$Revision: 1.17 $"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <thread.h>

/* SAM-FS headers. */
#include <lib.h>
#include "dvt.h"

#define DEC_INIT
#include <sam/lib.h>

#define DEC_INIT
#include <sam/lib.h>

/* Global data */

control_t control;		/* Work queue pointers & thread parameters */
work_t work[MAX_QUEUE];		/* Work queue */

static char *options[] = {
#define	 WRITE		1
	"w",
#define	 READ		2
	"r",
#define	 WRITE_READ	3
	"rw",
	(char *)NULL
};

static char *patterns[] = {
#define	 ONES_PAT	0
	"o",
#define	 ASCENDING	1
	"a",
#define	 ZEROS_PAT	2
	"0",
#define	 ASCENDING_FILENAME	3
	"f",
	(char *)NULL
};

/* Prototypes */

static void dvt_boss(pblock_t *pp);
static int create_buffers(pblock_t *pp, int type);
static int write_file(pblock_t *pp);
static int read_file(pblock_t *pp);
static int setup_file(pblock_t *pp, int type);
static int fill_buffer(pblock_t *pp, char *buffer, offset_t byte_offset);
static void compare_buffers(uchar_t *buf1, uchar_t *buf2, int size,
	offset_t byte_offset, int error_limit);
static int add_work(work_t *wp, int queue_size);
static void display_time(struct timeval start_time, struct timeval end_time,
	offset_t byte_offset, char *type);
static void *worker_thread(void *);



int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int c;
	int error = 0;
	char *opts = "c:Cde:g:i:O:o:P:pq:R:s:vW:";	/* getopts options */

	char *usage =
	    "Usage:\n\tdvt "
	    "[-c block_count] "
	    "[-C] "
	    "[-d] "
	    "[-e error_limit] "
	    "[-g striped_group]\n\t    "
	    "[-i stride] "
	    "[-o rw|r|w] "
	    "[-O byte_offset] "
	    "[-p] "
	    "[-P a|f|o|0]\n\t    "
	    "[-q queue_size] "
	    "[-R read_threads] "
	    "[-s block_size] "
	    "[-v]\n\t    "
	    "[-W write_threads] "
	    "file\n";

	pblock_t pblock = {		/* Default parameters */
		sizeof (pblock_t),	/* Parameter block size */
		100,			/* Block count */
		4096,			/* Block size */
		0,			/* Stride count */
		0,			/* Btye Offset */
		WRITE_READ,		/* Write, read or write/read */
		ONES_PAT,		/* Data pattern */
		0,			/* Error limit - 0 is infinite */
		0,			/* Directio -- 0 = no, 1 = yes */
		0,			/* Preallocate -- 0 = no, 1 = yes */
		3,			/* Maximum number of write threads */
		3,			/* Maximum number of read threads */
		3,			/* Maximum work queue size */
		-1,			/* Stripe group */
		0,			/* Verify data */
		'0'			/* Filename */
	};

	/*
	 * Process arguments.
	 */
	if (argc <= 1) {
		fprintf(stderr, "%s\n", usage);
		exit(2);
	}
	while ((c = getopt(argc, argv, opts)) != EOF) {
		switch (c) {
		case 'c':		/* Count of blocks */
			if (optarg) {
				pblock.count = atoi(optarg);
				if (pblock.count == 0)
					error++;
			} else {
				error++;
			}
			break;

		case 'C':		/* Cache (buffered) I/O */
			pblock.directio = -1;
			break;

		case 'd':		/* Directio */
			pblock.directio = 1;
			break;

		case 'e':		/* Error limit */
			if (optarg) {
				pblock.error_limit = atoi(optarg);
			} else {
				error++;
			}
			break;

		case 'g':		/* Stripe group */
			if (optarg) {
				pblock.stripe_group = atoi(optarg);
			} else {
				error++;
			}
			break;

		case 'i':		/* Increment count or stride */
			if (optarg) {
				pblock.stride = atoi(optarg);
			} else {
				error++;
			}
			break;

		case 'O':		/* Byte offset */
			if (optarg) {
				pblock.byte_offset = atoi(optarg);
			} else {
				error++;
			}
			break;

		case 'o':		/* Option - read|write|rw */
			if (optarg) {
				char *value;
				pblock.option = getsubopt(&optarg,
				    options, &value);
				if (pblock.option == -1)
					error++;
				else
					pblock.option++;
			} else {
				error++;
			}
			break;

		case 'P':		/* Data pattern */
			if (optarg) {
				char *value;
				pblock.pattern = getsubopt(&optarg,
				    patterns, &value);
				if (pblock.pattern == -1)
					error++;
			} else {
				error++;
			}
			break;

		case 'p':		/* Preallocate file */
			pblock.preallocate = 1;
			break;

		case 'q':		/* Queue limit */
			if (optarg) {
				pblock.queue_size = atoi(optarg);
				if ((pblock.queue_size == 0) ||
				    (pblock.queue_size > MAX_QUEUE))
					error++;
			} else {
				error++;
			}
			break;

		case 'R':		/* Read threads */
			if (optarg) {
				pblock.read_threads = atoi(optarg);
				if ((pblock.read_threads == 0) ||
				    (pblock.read_threads > MAX_THREADS))
					error++;
			} else {
				error++;
			}
			break;

		case 's':		/* Size of block */
			if (optarg) {
				int value = 1;
				char *p;

				p = optarg + strlen(optarg) - 1;
				if (*p == 'k') {
					value = 1024;
					*p = '\0';
				}
				pblock.size = atoi(optarg) * value;
				if ((pblock.size == 0) ||
				    (pblock.size < sizeof (pblock_t))) {
					printf("block_size (-s) = %d "
					    "must be >= %d\n",
					    pblock.size, sizeof (pblock_t));
					error++;
				}
			} else {
				error++;
			}
			break;

		case 'v':		/* Verify data */
			pblock.verify_data = 1;
			break;

		case 'W':		/* Write threads */
			if (optarg) {
				pblock.write_threads = atoi(optarg);
				if ((pblock.write_threads == 0) ||
				    (pblock.write_threads > MAX_THREADS))
					error++;
			} else {
				error++;
			}
			break;

		default:
			error++;
			break;
		}
	}

	if (optind == argc)
		error++;		/* No file name */

	if (error != 0) {
		fprintf(stderr, "%s\n", usage);
		exit(2);
	}
	strncpy(pblock.file_name, argv[optind], MAXNAMELEN);

	/*
	 * Read threads must be >= write threads.
	 */
	if (pblock.read_threads < pblock.write_threads)
		pblock.read_threads = pblock.write_threads;

	/*
	 * Queue size must be at the maximum size of the write/read threads.
	 */
	if (pblock.queue_size < pblock.read_threads)
		pblock.queue_size = pblock.read_threads;

	printf(
"WR Thr = %d, RD Thr = %d, Queue = %d, Directio = %s, Preallocate = %s\n",
	    pblock.write_threads, pblock.read_threads,
	    pblock.queue_size,
	    pblock.directio == 1 ? "on" :
	    pblock.directio == -1 ? "off" : "default",
	    pblock.preallocate ? "on" : "off");

	dvt_boss(&pblock);
	return (0);
}


/*
 * dvtboss optionally writes, thens read the file based on the parameters.
 */

static void
dvt_boss(
pblock_t *pp)
{
	int t;
	int i;
	int error = 0;

	/*
	 * Initialize the control structure.
	 */
	mutex_init(&control.queue_lock, 0, NULL);
	mutex_init(&control.queue_full_lock, 0, NULL);
	cond_init(&control.queue_not_empty, 0, NULL);
	cond_init(&control.queue_not_full, 0, NULL);
	cond_init(&control.queue_empty, 0, NULL);
	control.queue_size = 0;
	control.shutdown = 0;

	/*
	 * Create the write threads.
	 */
	for (t = 0; t < pp->write_threads; t++) {
		control.thr[t].num = t;
		if (thr_create(NULL, 0, worker_thread,
		    (void *)&control.thr[t],
		    THR_BOUND, (void *)&control.thr[t].tid)) {
			perror("main: thr_create:");
			exit(1);
		}
	}

	/*
	 * Setup the buffers if writing.
	 */
	if (pp->option & WRITE) {
		if (create_buffers(pp, WRITE))
			exit(1);
	}

	/*
	 * Write to file
	 */
	if (pp->option & WRITE) {
		error = write_file(pp);
	}


	/*
	 * Read from file
	 */
	if ((error == 0) && (pp->option & READ)) {
		/*
		 * Initialize work for read. Create additional read threads.
		 */
		for (i = 0; i < pp->queue_size; i++) {
			if (work[i].busy)
				printf("work %d busy\n", i);
			work[i].busy = 0;	/* Request is busy doing I/O */
			work[i].write = 0;	/* Thread read flag */
			work[i].first = 1;	/* This is the first time */
		}
		if (pp->write_threads < pp->read_threads) {
			for (t = pp->write_threads; t < pp->read_threads; t++) {
				control.thr[t].num = t;
				if (thr_create(NULL, 0, worker_thread,
				    (void *)&control.thr[t],
				    THR_BOUND, (void *)&control.thr[t].tid)) {
					perror("main: thr_create:");
					exit(1);
				}
			}
		}
		error = read_file(pp);
	}
	/*
	 * Signal threads to terminate.
	 */
	mutex_lock(&control.queue_lock);
	control.shutdown = 1;
	cond_broadcast(&control.queue_not_empty);
	mutex_unlock(&control.queue_lock);
	/*
	 * Wait for those threads to terminate.
	 */
	for (i = 0; i < pp->queue_size; i++) {
		if (thr_join(control.thr[i].tid, NULL, NULL)) {
			if (errno != 0) {
				perror("main: thr_join");
				exit(1);
			}
		}
	}
	exit(error);
}


/*
 * Initialize the work entries for write or read.
 * Allocate I/O buffers and compare buffers, page aligned.
 */

static int
create_buffers(
pblock_t *pp,
int type)
{
	int i;

	for (i = 0; i < pp->queue_size; i++) {
		size_t size = (size_t)((pp->size + 7) / 8) * 8;
		if ((work[i].buffer = (char *)valloc(size)) == NULL) {
			perror("main: valloc buffer");
			return (1);
		}
		if (getuid() == 0) {
			/* If superuser, lock buffers */
			if (mlock(work[i].buffer, size) != 0) {
				perror("main: mlock");
				return (1);
			}
		}
		if (pp->verify_data) {
			if ((work[i].cmp_buffer = (char *)valloc(size)) ==
			    NULL) {
				perror("main: valloc cmp_buffer");
				return (1);
			}
		}
		mutex_init(&work[i].work_lock, 0, NULL);
		work[i].num = i;	/* Work number */
		work[i].busy = 0;	/* Request is busy doing I/O */
		if (type == WRITE) {
			work[i].write = 1;	/* Thread write flag */
			/* Load initial write pattern */
			if (pp->pattern == ONES_PAT ||
			    pp->pattern == ZEROS_PAT) {
				if (fill_buffer(pp, work [i].buffer, 0))
					return (1);
			} else {
				/* Insure initial write pattern is original */
				memset(work[i].buffer, 0x3c, pp->size);
			}
		} else {
			work[i].write = 0;	/* Thread read flag */
			work[i].first = 1;	/* This is a first time read */
			/* Insure initial read pattern was from disk */
			memset(work[i].buffer, 0xc3, pp->size);
		}
	}
	return (0);
}


/*
 * The boss dvt thread loops, putting write requests in the I/O
 * request queue until the issue count is reached. The boss thread
 * waits until the request is done (not busy) before using the
 * work entry. Then it sets up the request parameters and adds the
 * entry to the work queue.
 */

static int
write_file(
pblock_t *pp)
{
	int first_block = 1;
	int ique;
	int t;
	offset_t byte_offset;
	int issue_count;
	struct timeval start_time, end_time;

	if (setup_file(pp, WRITE))
		return (1);

	ique = 0;
	issue_count = 0;
	byte_offset = pp->byte_offset;
	gettimeofday(&start_time, (struct timezone *)NULL);

	for (;;) {
		work_t *wp;

		wp = &work[ique];
		mutex_lock(&wp->work_lock);
		while (wp->busy)
			cond_wait(&wp->cv_work_done, &wp->work_lock);

		wp->busy = 1;
		wp->next = NULL;
		mutex_unlock(&wp->work_lock);

		wp->byte_offset = byte_offset;
		wp->size = pp->size;

		if ((pp->pattern == ASCENDING) ||
		    (pp->pattern == ASCENDING_FILENAME) ||
		    wp->header_block) {
			if (fill_buffer(pp, wp->buffer, byte_offset))
				return (1);
		}
		/* first time - copy in parameters */
		if (first_block && !byte_offset) {
			first_block = 0;
			memcpy(wp->buffer, pp, sizeof (pblock_t));
			wp->header_block = 1;
		}
		if (add_work(wp, pp->queue_size))
			return (1);

		if (++ique >= pp->queue_size)
			ique = 0;

		issue_count++;
		if (pp->stride == 0) {
			byte_offset += (offset_t)pp->size;
		} else {
			byte_offset += ((offset_t)pp->size * (pp->stride + 1));
		}
		if (issue_count >= pp->count)		/* If done */
			break;
	}

	/*
	 * Wait for queue to empty and I/O to complete.
	 */
	mutex_lock(&control.queue_lock);
	while (((control.queue_size != 0) || (control.io_outstanding != 0)) &&
	    (control.shutdown == 0)) {
		cond_wait(&control.queue_empty, &control.queue_lock);
	}
	if (control.shutdown) {
		mutex_unlock(&control.queue_lock);
		return (1);
	}
	mutex_unlock(&control.queue_lock);

	gettimeofday(&end_time, (struct timezone *)NULL);

	for (t = 0; t < pp->write_threads; t++) {
		(void) close(control.thr[t].fd);
	}

	display_time(start_time, end_time, byte_offset, "Write");
	return (0);
}


/*
 * The boss dvt thread loops, putting read requests in the I/O
 * request queue until the issue count is reached. The boss thread
 * waits until the request is done (not busy) before using the
 * work entry. Then it sets up the request parameters and adds the
 * entry to the work queue.
 */

static int
read_file(
pblock_t *pp)
{

	int first_block = 1;
	int ique;
	int t;
	offset_t byte_offset;
	int issue_count;
	int completion_count;
	char hdr_block[512];
	pblock_t *fp_block = (pblock_t *)&hdr_block;
	struct timeval start_time, end_time;

	if (setup_file(pp, READ))
		return (1);

	if (pp->byte_offset == 0) {
		/*
		 * Read parameter block at beginning of file
		 */
		if ((lseek(control.thr[0].fd, 0, 0)) == -1) {
			perror("read_file: seek");
			return (1);
		}
		if ((read(control.thr[0].fd, &hdr_block, 512)) != 512) {
			perror("read_file: read");
			return (1);
		}
		if (sizeof (pblock_t) != fp_block->pblock_size) {
			printf("Parameter size %d does not match file %d. "
			    "Rewrite file.\n",
			    sizeof (pblock_t), fp_block->pblock_size);
			return (1);
		}
		/*
		 * Pattern used must match pattern written. If read only, create
		 * buffers now.
		 */
		pp->pattern = fp_block->pattern;
		pp->count = fp_block->count;
		pp->size = fp_block->size;
	}
	if (pp->option == READ) {
		if (create_buffers(pp, READ))
			return (1);
	}

	ique = 0;
	issue_count = 0;
	completion_count = 0;
	byte_offset = pp->byte_offset;
	gettimeofday(&start_time, (struct timezone *)NULL);

	for (;;) {
		work_t *wp;

		wp = &work[ique];
		mutex_lock(&wp->work_lock);
		while (wp->busy)
			cond_wait(&wp->cv_work_done, &wp->work_lock);

		wp->busy = 1;
		wp->next = NULL;	/* Forward link */
		mutex_unlock(&wp->work_lock);

		if (wp->first) {	/* If first buffer, not yet read */
			wp->first = 0;
		} else {
			completion_count++;
			/* If verify data, compare buffer */
			if (pp->verify_data) {
				compare_buffers((uchar_t *)wp->buffer,
				    (uchar_t *)wp->cmp_buffer, wp->size,
				    wp->byte_offset, pp->error_limit);
			}
		}
		wp->byte_offset = byte_offset;
		wp->size = pp->size;
		if (pp->verify_data) {
			if (fill_buffer(pp, wp->cmp_buffer, byte_offset))
				return (1);
			/* If first time, copyin parameters */
			if (first_block && !byte_offset) {
				first_block = 0;
				memcpy(wp->cmp_buffer, (char *)fp_block,
				    sizeof (pblock_t));
			}
		}

		if (++ique >= pp->queue_size)
			ique = 0;

		if (completion_count >= pp->count)	/* If done */
			break;
		if (issue_count >= pp->count)	/* Loop to compare last buf */
			continue;

		if (add_work(wp, pp->queue_size))
			return (1);

		issue_count++;
		if (pp->stride == 0) {
			byte_offset += (offset_t)pp->size;
		} else {
			byte_offset += ((offset_t)pp->size * (pp->stride + 1));
		}
	}

	/*
	 * Wait for queue to empty and I/O to complete.
	 */
	mutex_lock(&control.queue_lock);
	while (((control.queue_size != 0) || (control.io_outstanding != 0)) &&
	    (control.shutdown == 0)) {
		cond_wait(&control.queue_empty, &control.queue_lock);
	}
	if (control.shutdown) {
		mutex_unlock(&control.queue_lock);
		return (1);
	}
	mutex_unlock(&control.queue_lock);

	gettimeofday(&end_time, (struct timezone *)NULL);

	for (t = 0; t < pp->read_threads; t++) {
		(void) close(control.thr[t].fd);
	}

	display_time(start_time, end_time, byte_offset, "Read");

	return (0);
}


/*
 * Set up for write/read file. Open the file for each worker thread based
 * on the filemode. Each worker thread opens the file because the file_offset
 * must be unique (out of order seeks can occur with threads). The first open
 * for write truncates the file.
 */

static int
setup_file(
pblock_t *pp,
int type)
{
	int filemode;
	int nthreads;
	int t;

	if (type == WRITE) {
		nthreads = pp->write_threads;
#if defined(O_LARGEFILE)
		filemode = O_WRONLY | O_CREAT | O_LARGEFILE;
#else
		filemode = O_WRONLY | O_CREAT;
#endif
	} else {
		nthreads = pp->read_threads;
#if defined(O_LARGEFILE)
		filemode = O_RDONLY | O_LARGEFILE;
#else
		filemode = O_RDONLY;
#endif
	}
	for (t = 0; t < nthreads; t++) {
		if ((type == WRITE) && (t == 0)) {
			filemode |= O_TRUNC;
		}
		if ((control.thr[t].fd = open(pp->file_name, filemode,
		    0777)) <= 0) {
			perror("dvt: open:");
			return (1);
		}
		if (t == (nthreads-1)) {
			struct stat64 statb;
			struct statvfs64 statbuf;

			if (fstat64(control.thr[t].fd, &statb)) {
				perror("dvt: fstat:");
				return (1);
			}
			if (!S_ISREG(statb.st_mode)) {
				/* skip direct, prealloc, etc. if not REG */
				continue;
			}
			if ((fstatvfs64(control.thr[t].fd, &statbuf))) {
				perror("dvt: fstatvfs:");
				return (1);
			}
			if ((strcmp(statbuf.f_basetype, "samfs")) == 0) {
				char ops[32];
				offset_t fsize;

				if ((type == WRITE) && pp->preallocate) {
					/*
					 * Issue sam_setfa to preallocate file
					 * on a specified striped group.
					 * See man sam_setfa.
					 */
					fsize = (offset_t)pp->size *
					    (offset_t)pp->count;
					if (pp->stripe_group == -1) {
						sprintf(ops, "l%lld", fsize);
					} else {
						sprintf(ops, "g%dl%lld",
						    pp->stripe_group, fsize);
					}
					if (sam_setfa(pp->file_name, ops)) {
						perror("write_file: "
						    "sam_setfa:");
						return (1);
					}
				}
				/*
				 * Issue sam_advise for directio(r), lock
				 * buffers(p), simutaneous writes(w). See man
				 * sam_advise.
				 */
				switch (pp->directio) {
				case 1:		/* direct I/O */
					sprintf(ops, "drw");
					break;
				case 0:
					sprintf(ops, "dw");
					break;
				case -1:	/* buffered I/O */
					sprintf(ops, "dbw");
				}
				if (sam_advise(control.thr[0].fd, ops)) {
					perror("dvt: sam_advise:");
					return (1);
				}
			} else if ((strcmp(statbuf.f_basetype, "ufs")) == 0) {
#if defined(DIRECTIO_ON)
				if (pp->directio == 1) {
					if (directio(control.thr[t].fd,
					    DIRECTIO_ON) != 0) {
						perror("directio [raw]: "
						    "ioctl:");
					}
				}
				if (pp->directio == -1) {
					if (directio(control.thr[t].fd,
					    DIRECTIO_OFF) != 0) {
						perror("directio [buffered]: "
						    "ioctl:");
					}
				}
#endif
			}
		}
	}
	return (0);
}


/*
 * If the I/O request work queue is full, the dvt boss thread waits on
 * the queue_not_full condition. Then boss adds a I/O request to  the
 * tail of the I/O request work queue. The boss wakes up a sleeping
 * worker thread by signaling the queue_not_empty condition.
 */

static int
add_work(
work_t *wp,		/* Pointer to I/O request work entry */
int queue_size)		/* Size of I/O request work queue */
{

	mutex_lock(&control.queue_lock);
	while ((control.queue_size == queue_size) && (control.shutdown == 0)) {
		mutex_unlock(&control.queue_lock);
		mutex_lock(&control.queue_full_lock);
		cond_wait(&control.queue_not_full, &control.queue_full_lock);
		mutex_unlock(&control.queue_full_lock);
		mutex_lock(&control.queue_lock);
	}
	if (control.shutdown) {
		mutex_unlock(&control.queue_lock);
		return (1);
	}
	if (control.queue_size == 0) {
		control.queue_tail = control.queue_head = wp;
	} else {
		(control.queue_tail)->next = wp;
		control.queue_tail = wp;
	}
	control.queue_size++;
	cond_signal(&control.queue_not_empty);
	mutex_unlock(&control.queue_lock);
	return (0);
}


/*
 * Display the time and megabyte rate based on the start/stop time and
 * bytes transferred.
 */

static void
display_time(
struct timeval start_time,
struct timeval end_time,
offset_t bytes,
char *type)
{
	float ttime, xnbyte, mbytes, usec;

	ttime = end_time.tv_sec - start_time.tv_sec;
	usec = end_time.tv_usec - start_time.tv_usec;
	ttime += usec / 1000000;
	xnbyte = bytes / 1048576.;
	mbytes = xnbyte / ttime;
	printf("%s time  = %.5f sec., bytes = %llu, mbs = %.5f\n",
	    type, ttime, bytes, mbytes);
}


/*
 * Compare the data in the buffer read and the generated data buffer.
 */

static void
compare_buffers(
unsigned char *buf1,	/* Buffer read. */
unsigned char *buf2,	/* Buffer with generated data. */
int size,		/* Size of buffers. */
offset_t byte_offset,	/* Address in file. */
int error_limit)	/* Maximum number of errors before terminating */
{
	int l = 16;
	static int total_error = 0;

	while (size > 0) {
		int n;
		int error = 0;

		if (l > size)
			l = size;
		for (n = 0; n < l; n++) {
			if (*buf1++ != *buf2++)
				error++;
		}
		if (error != 0) {
			total_error += error;
			buf1 -= l;
			buf2 -= l;
			fprintf(stderr, "%#8.8llx: ", byte_offset);
			/* 16 data items in hex. */
			for (n = 0; n < l; n++) {
				fprintf(stderr, "%02x ", *buf1++);
				if ((n & 3) == 3)
					fprintf(stderr, " ");
			}
			fprintf(stderr, "\n	    ");
			for (n = 0; n < l; n++) {
				fprintf(stderr, "%02x ", *buf2++);
				if ((n & 3) == 3)
					fprintf(stderr, " ");
			}
			fprintf(stderr, "\n");
			if (error_limit && (total_error > error_limit)) {
				exit(-1);
			}
		}
		size -= l;
		byte_offset += l;
	}
}


/*
 * Fill the buffer with the data pattern.
 */

int
fill_buffer(
pblock_t *pp,		/* Parmameter block */
char *buffer,		/* Address of buffer */
offset_t cur_byte)	/* Starting byte address this buffer */
{
	int size;	/* Size in bytes for this buffer */
	int iii, num_filled;
	char *tmp, *tmp1;
	longlong_t pattern;
	offset_t byte;

	size = pp->size;
	tmp = buffer;
	num_filled = 0;
	byte = cur_byte;

	switch (pp->pattern) {
	case ASCENDING:
		/* Ascending pattern using 64 bit byte offset as the pattern. */
		pattern = byte & ~0x7;
		tmp1 = (char *)&pattern;
		if ((iii = byte & 0x7) != 0) {
			memcpy(tmp, tmp1 + iii, 8 - iii);
			num_filled += (8 - iii);
			tmp += (8 - iii);
			pattern += 8;
		}
		while (num_filled <= (size - 8)) {
			memcpy(tmp, tmp1, 8);
			num_filled += 8;
			tmp += 8;
			pattern += 8;
		}

		if ((iii = size - num_filled) != 0)
			memcpy(tmp, tmp1, iii);
		break;

	case ASCENDING_FILENAME:
		/* Ascending pattern using 64 bit byte offset as the pattern, */
		/* plus 16 characters of the filename at 8k boundaries. */
		pattern = byte & ~0x7;
		tmp1 = (char *)&pattern;
		if ((iii = byte & 0x7) != 0) {
			memcpy(tmp, tmp1 + iii, 8 - iii);
			num_filled += (8 - iii);
			tmp += (8 - iii);
			pattern += 8;
		}
		while (num_filled <= (size - 8)) {
			if ((pattern & (8192-1)) == 0) {
				memcpy(tmp, pp->file_name, 16);
				num_filled += 16;
				tmp += 16;
				pattern += 16;
			} else {
				memcpy(tmp, tmp1, 8);
				num_filled += 8;
				tmp += 8;
				pattern += 8;
			}
		}

		if ((iii = size - num_filled) != 0)
			memcpy(tmp, tmp1, iii);
		break;

	case ONES_PAT:
		memset(buffer, -1, size);
		break;

	case ZEROS_PAT:
		memset(buffer, 0, size);
		break;

	default:
		fprintf(stderr, "Internal error: unknown pattern.\n");
		return (1);
	}
	return (0);
}


/*
 * If the queue is empty, the worker thread sleeps on the queue_not_empty
 * condition condition. The worker thread removes the I/O request from
 * the head of the queue. After it removes the I/O request, it signals
 * the queue_not_full condition.
 */

static void *
worker_thread(
void *c)
{
	work_t *wp;
	thr_t *tp = (thr_t *)c;
	int n;

	for (;;) {
		wp = NULL;
		mutex_lock(&control.queue_lock);
		/*
		 * Wait until there is something in the queue.
		 */
		while ((control.queue_size == 0) && (control.shutdown == 0)) {
			cond_wait(&control.queue_not_empty,
			    &control.queue_lock);
		}
		if (control.shutdown) {
			mutex_unlock(&control.queue_lock);
			thr_exit(NULL);
		}

		/*
		 * Remove work from queue and then signal boss thread
		 * queue_not_full.
		 */
		wp = control.queue_head;
		control.queue_size--;
		if (control.queue_size == 0) {
			control.queue_head = control.queue_tail = NULL;
		} else {
			control.queue_head = wp->next;
		}
		control.io_outstanding++;
		mutex_unlock(&control.queue_lock);

		mutex_lock(&control.queue_full_lock);
		cond_signal(&control.queue_not_full);
		mutex_unlock(&control.queue_full_lock);

		/*
		 * Seek and then issue write or read call. Wait until done.
		 */
		if ((n = llseek(tp->fd, wp->byte_offset, SEEK_SET)) == -1) {
			fprintf(stderr,
			    "worker_thread: llseek error at %lld, "
			    "return = %d\n",
			    wp->byte_offset, n);
			if (n == -1)
				perror("worker_thread: llseek");
			break;
		} else {
			if (wp->write) {
				if ((n = write(tp->fd, wp->buffer,
				    wp->size)) != wp->size) {
					fprintf(stderr,
					    "worker_thread: write error "
					    "at %lld, return = %d\n",
					    wp->byte_offset, n);
					if (n == -1) {
						wp->ret_errno = errno;
						perror("worker_thread: write");
					}
					break;
				}
			} else {
				if ((n = read(tp->fd, wp->buffer, wp->size)) !=
				    wp->size) {
					fprintf(stderr,
					    "worker_thread: read error "
					    "at %lld, return = %d\n",
					    wp->byte_offset, n);
					if (n == -1) {
						wp->ret_errno = errno;
						perror("worker_thread: read");
					}
					break;
				}
			}
		}
		mutex_lock(&wp->work_lock);
		wp->busy = 0;		/* I/O completed for this work entry */
		cond_signal(&wp->cv_work_done);
		mutex_unlock(&wp->work_lock);
		wp = NULL;

		/*
		 * Signal boss thread if queue_empty and no I/O is outstanding.
		 */
		mutex_lock(&control.queue_lock);
		control.io_outstanding--;
		if ((control.queue_size == 0) &&
		    (control.io_outstanding == 0)) {
			cond_signal(&control.queue_empty);
		}
		mutex_unlock(&control.queue_lock);
	}

	/*
	 * Shut down this worker thread.
	 */
	mutex_lock(&control.queue_lock);
	control.shutdown = 1;
	control.io_outstanding--;
	cond_signal(&control.queue_empty);
	mutex_unlock(&control.queue_lock);
	mutex_lock(&control.queue_full_lock);
	cond_signal(&control.queue_not_full);
	mutex_unlock(&control.queue_full_lock);
	if (wp != NULL) {			/* move on */
		mutex_lock(&wp->work_lock);
		wp->busy = 0;		/* I/O completed for this work entry */
		cond_signal(&wp->cv_work_done);
		mutex_unlock(&wp->work_lock);
	}
	thr_exit(NULL);
	/* return (0); */
}
