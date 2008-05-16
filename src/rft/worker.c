/*
 * worker.c - worker for data transfer engine.
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

#pragma ident "$Revision: 1.13 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/sam_rft.h"

/* Local headers. */
#include "rft_defs.h"

ssize_t readn(int fd, char *ptr, size_t nbytes);

/*
 * Worker thread for data transfer engine.
 */
void *
Worker(
	void *arg)
{
	int status;
	Recv_t *recv;
	ssize_t nbytes_read;
	ssize_t nbytes_written;

	Worker_t *worker = (Worker_t *)arg;
	Crew_t *crew = worker->crew;

	status = pthread_detach(pthread_self());

	Trace(TR_DEBUG, "[t@%d] Rft worker thread started %d",
	    pthread_self(), worker->seqnum);

	for (;;) {

		/*
		 * Wait for work to do.  If worker->first is not NULL,
		 * there is data to receive.  If worker->item_ready is TRUE,
		 * there is data to send.
		 */
		status = pthread_mutex_lock(&worker->mutex);

		while (worker->first == NULL && worker->item_ready == FALSE) {
			status = pthread_cond_wait(&worker->request,
			    &worker->mutex);
			ASSERT(status == 0);
		}

		/*
		 * If worker->first not NULL, there is data to receive
		 * from client.
		 */
		if (worker->first) {
			recv = worker->first;
			worker->first = recv->next;
			if (worker->first == NULL) {
				worker->last = NULL;
			}

			status = pthread_mutex_unlock(&worker->mutex);

			/*
			 * We have work, read data from socket.
			 */

			Trace(TR_DEBUG, "[t@%d] Read socket %d for %d bytes",
			    pthread_self(), fileno(worker->in), recv->nbytes);

			recv->buf = worker->buf;
			nbytes_read = readn(fileno(worker->in), recv->buf,
			    recv->nbytes);
			if (nbytes_read != recv->nbytes) {
				ASSERT_NOT_REACHED();
			}

			status = pthread_mutex_lock(&crew->mutex);
			recv->done_flag = 1;
			status = pthread_cond_signal(&recv->done);

			/*
			 * Data is available in receive buffer.  Must wait for
			 * this to be picked up and written to the local file
			 * before reading more data off the socket into our
			 * receive buffer.
			 */
			while (recv->gotit_flag == 0) {
				status = pthread_cond_wait(&recv->gotit,
				    &crew->mutex);
			}

			status = pthread_cond_destroy(&recv->done);
			status = pthread_cond_destroy(&recv->gotit);
			SamFree(recv);

			status = pthread_mutex_unlock(&crew->mutex);

		} else {
			/*
			 * If worker->item_ready is TRUE, there is data to send
			 * to client.
			 */
			Send_t *send;
			size_t nbytes_netord;

			ASSERT(worker->item_ready == TRUE);
			send = worker->item;
			worker->item_ready = FALSE;

			status = pthread_mutex_unlock(&worker->mutex);

			/*
			 * Send number of bytes ready to write on data socket.
			 */
			nbytes_netord = htonl(send->nbytes);
			nbytes_written = write(fileno(worker->out),
			    &nbytes_netord, sizeof (size_t));

			if ((long)send->nbytes > 0) {

				Trace(TR_DEBUG, "[t@%d] Write socket %d "
				    "for %d bytes",
				    pthread_self(), fileno(worker->out),
				    send->nbytes);

				nbytes_written = write(fileno(worker->out),
				    send->buf, send->nbytes);

				if (send->nbytes != nbytes_written) {
					Trace(TR_ERR,
					    "[t@%d] Write error %d %d %d",
					    pthread_self(), send->nbytes,
					    nbytes_written, errno);
				}
			}

			status = pthread_mutex_lock(&worker->mutex);
			send->active_flag = FALSE;
			status = pthread_cond_signal(&send->done);
			status = pthread_mutex_unlock(&worker->mutex);
		}
	}
#ifdef	lint
	/* LINTED statement not reached */
	return (NULL);
#endif
}

/*
 * Initialize worker for data transfer engine.
 */
void
InitWorker(
	Worker_t *worker,
	size_t dataportsize)
{
	int status;

	status = pthread_mutex_init(&worker->mutex, NULL);
	ASSERT(status == 0);

	status = pthread_cond_init(&worker->request, NULL);
	ASSERT(status == 0);

	SamMalloc(worker->buf, dataportsize);
}

/*
 * Cleanup worker.  Close data sockets.
 */
void
CleanupWorker(
	Worker_t *worker)
{
	Trace(TR_DEBUG, "[t@%d] Cleanup worker", pthread_self());

	(void) fclose(worker->in);
	(void) fclose(worker->out);
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

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	FD_ZERO(&allfds);
	FD_SET(fd, &allfds);

	while (nleft > 0) {

		readfds = allfds;

		nfds = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (nfds == 0) {
			Trace(TR_MISC, "Timeout hit, client drop");
			continue;
		}

		/*
		 * If data has arrived from client.
		 */
		if (FD_ISSET(fd, &readfds)) {

#if 0
			Trace(TR_DEBUG, "Reading socket %d for %d bytes (0x%x)",
			    fd, nleft, ptr);
#endif

			nread = read(fd, ptr, nleft);

#if 0
			Trace(TR_DEBUG, "Read complete %d bytes", nread);
#endif

			if ((long)nread < 0) {
				return (nread);
			} else if ((long)nread == 0) {
				Trace(TR_MISC, "Read zero bytes, client drop");
				return (-1);
			}

			nleft -= nread;
			ptr += nread;
		}
	}

	return (nbytes - nleft);
}
