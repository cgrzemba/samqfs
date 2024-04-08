/*
 *
 * dmpshm - compress and uuencode the shared memory segments.
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define DEC_INIT
#include "aml/shm.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.19 $"

#define	UUENCODE  "/usr/bin/uuencode"
#define	COMPRESS  "/usr/bin/compress"

void encode_memory(char *, char *, size_t);

int
/* LINTED argument unused in function */
main(int argc, char **argv)
{
	int	m_shmid, p_shmid;
	char	*m_shm, *p_shm;
	struct	shmid_ds shm_stat;


	if ((m_shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		perror("master: shmget");
		exit(1);
	}
	if ((m_shm = shmat(m_shmid, NULL, SHM_RDONLY)) == (void *) -1) {
		perror("master: shmat");
		exit(1);
	}
	if (shmctl(m_shmid, IPC_STAT, &shm_stat)) {
		perror("master: shmctl");
		exit(1);
	}
	encode_memory("master_shm.Z", m_shm, shm_stat.shm_segsz);
	shmdt(m_shm);


	if ((p_shmid = shmget(SHM_PREVIEW_KEY, 0, 0)) < 0) {
		perror("preview: shmget");
		exit(1);
	}
	if ((p_shm = shmat(p_shmid, NULL, SHM_RDONLY)) == (void *) -1) {
		perror("preview: shmat");
		exit(1);
	}
	if (shmctl(p_shmid, IPC_STAT, &shm_stat)) {
		perror("preview: shmctl");
		exit(1);
	}
	encode_memory("preview_shm.Z", p_shm, shm_stat.shm_segsz);
	shmdt(p_shm);

	return (0);
}

void
encode_memory(char *beg_name, char *address, size_t length)
{
	int	pipes[2];
	pid_t	pid;

	if (pipe(pipes)) {
		perror("encode_memory: pipe");
		exit(1);
	}
	if ((pid = fork()) == 0) {
		/* we are the child of main */
		pid_t	enc_pid;
		int	u_pipes[2];

		close(pipes[1]);
		close(0);
		dup2(pipes[0], 0);
		pipe(u_pipes);
		if ((enc_pid = fork()) == 0) {
			/* We are the child of compress */
			close(u_pipes[1]);
			close(0);
			dup2(u_pipes[0], 0);
			execl(UUENCODE, "uuencode", beg_name, NULL);
			_exit(-1);
		}
		if (enc_pid < 0) {
			perror("encode_memory: fork");
			exit(1);
		}
		close(u_pipes[0]);
		close(1);
		dup2(u_pipes[1], 1);
		execl(COMPRESS, "compress", NULL);
		_exit(-1);
	}
	if (pid < 0) {
		perror("encode_memory: fork");
		exit(1);
	}
	close(pipes[0]);
	write(pipes[1], address, length);
	close(pipes[1]);
	while (waitpid(0, NULL, 0) >= 0)
		sleep(1);
}


/*
 * Local variables:
 * eval:(ldk-c-mode)
 * End:
 */
