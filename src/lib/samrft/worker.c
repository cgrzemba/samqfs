/*
 * worker.c - worker for data transfer engine.
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

#pragma ident "$Revision: 1.14 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/time.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "rft_defs.h"

/* Private data. */
static char *tracePrefix = "RFT";

ssize_t readn(int fd, char *ptr, size_t nbytes);

/*
 * Worker thread for data transfer engine.
 */
void*
Worker(
	void *arg)
{
	int rval;

	SamrftWorker_t *worker = (SamrftWorker_t *)arg;
	SamrftCrew_t *crew = worker->crew;

	Trace(TR_RFT, "%s [t@%d] Rft worker thread started %d",
	    tracePrefix, pthread_self(), worker->seqnum);

	for (;;) {
		rval = pthread_mutex_lock(&worker->mutex);

		while (worker->first == NULL && worker->item_ready == 0) {
			rval = pthread_cond_wait(&worker->request,
			    &worker->mutex);
			ASSERT(rval == 0);
		}

		if (worker->first) {
			SamrftSend_t *send;
			size_t nbytes_written;

			send = worker->first;
			worker->first = send->next;
			if (worker->first == NULL) {
				worker->last = NULL;
			}

			rval = pthread_mutex_unlock(&worker->mutex);

			/*
			 * We have work, process it.
			 */
			nbytes_written = write(fileno(worker->out),
			    send->buf, send->nbytes);
			if (nbytes_written != send->nbytes) {
				Trace(TR_RFT, " %s [t@%d] Rft write failed %d",
				    tracePrefix, pthread_self(), errno);
			}
			ASSERT(nbytes_written == send->nbytes);

			SamFree(send);

			rval = pthread_mutex_lock(&crew->mutex);
			crew->active--;
			if (crew->active <= 0) {
				rval = pthread_cond_signal(&crew->done);
			}
			rval = pthread_mutex_unlock(&crew->mutex);

		} else {
			SamrftRecv_t *recv;
			ssize_t nbytes_read;
			size_t nbytes_netord;

			ASSERT(worker->item_ready == 1);
			recv = worker->item;
			worker->item_ready = 0;
			nbytes_netord = 0;

			rval = pthread_mutex_unlock(&worker->mutex);

			/*
			 * Get number of bytes expected from data socket.
			 * Always send in network (big-endian) order.
			 */
			nbytes_read = readn(fileno(worker->in),
			    (char *)&nbytes_netord, sizeof (size_t));
			recv->nbytes = ntohl(nbytes_netord);
			if ((long)recv->nbytes > 0) {

				Trace(TR_RFT, "%s [t@%d] read socket %d "
				    "for %d bytes [0x%x]",
				    tracePrefix, pthread_self(),
				    fileno(worker->in),
				    recv->nbytes, recv->buf);

				nbytes_read = readn(fileno(worker->in),
				    recv->buf, recv->nbytes);
				if (nbytes_read != recv->nbytes) {
					Trace(TR_RFT,
					    "%s [%d] read error %d %d",
					    tracePrefix,
					    fileno(worker->in),
					    nbytes_read, errno);
				}
			}

			rval = pthread_mutex_lock(&worker->mutex);
			recv->active_flag = 0;
			rval = pthread_cond_signal(&recv->done);
			rval = pthread_mutex_unlock(&worker->mutex);
		}
	}
	/* LINTED function has no return statement */
}

/*
 * Cleanup worker.  Close data sockets.
 */
void
CleanupWorker(
	SamrftWorker_t *worker)
{
	fclose(worker->in);
	fclose(worker->out);
	SamFree(worker->item);
}

/*
 * Initialize worker for data transfer engine.
 */
void
InitWorker(
	SamrftWorker_t *worker)
{
	int rval;

	rval = pthread_mutex_init(&worker->mutex, NULL);
	ASSERT(rval == 0);

	rval = pthread_cond_init(&worker->request, NULL);
	ASSERT(rval == 0);

	SamMalloc(worker->item, sizeof (SamrftRecv_t));
	memset(worker->item, 0, sizeof (SamrftRecv_t));

	rval = pthread_cond_init(&worker->item->done, NULL);
	ASSERT(rval == 0);
}

/*
 * Read from socket.
 */
ssize_t
readn(
	int fd,
	char *ptr,
	size_t nbytes)
{
	size_t nleft;
	size_t nread;
	int nfds;
	fd_set allfds;
	fd_set readfds;
	struct timeval tv;

	nleft = nbytes;

	/* FIXME - timeout val SAMRFT_SERVER_TIMEOUT_VAL */

	tv.tv_sec = 120;
	tv.tv_usec = 0;

	FD_ZERO(&allfds);
	FD_SET(fd, &allfds);

	while (nleft > 0) {

		readfds = allfds;

		nfds = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (nfds == 0) {
			Trace(TR_RFT,
			    "%s timeout hit, no read fds available",
			    tracePrefix);
			return (nbytes - nleft);
		}

		/*
		 * If data has arrived from server.
		 */
		if (FD_ISSET(fd, &readfds)) {

			Trace(TR_RFT,
			    "%s [%d] reading socket for %d bytes (0x%x)",
			    tracePrefix, fd, nleft, ptr);

			nread = read(fd, ptr, nleft);

			Trace(TR_RFT, "%s [%d] read complete %d bytes",
			    tracePrefix, fd, nread);

			if ((long)nread <= 0) {
				Trace(TR_RFT, "%s [t@%d] Rft read failed %d",
				    tracePrefix, pthread_self(), errno);
				return ((ssize_t)-1);
			}

			nleft -= nread;
			ptr += nread;
		}
	}

	return (nbytes - nleft);
}
