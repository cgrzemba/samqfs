/*
 * tp_rearchive.c. Third party api to set the rearchive flag
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

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "sam/types.h"
#include "aml/types.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/uioctl.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "aml/proto.h"
#include "pub/mig.h"

#pragma ident "$Revision: 1.19 $"

int
sam_mig_rearchive(
	char *mnt_pnt,
	char **vsns,
	char *mig_media)
{
	int count = 0;
	int expected_ino = 1;
	int fd, read_size;
	media_t media;
	union sam_di_ino *inode_buffer;

	/* check the media string to be 2 characters, first character a 'z' */
	if ((strlen(mig_media) != 2) || (*mig_media != 'z')) {
		errno = EINVAL;
		return (-1);
	}
	/*
	 * move mig_media pointer to point to second character, check it to
	 * be a lower case alpha or a numeric
	 */

	mig_media++;
	if (!(islower(*mig_media) || isdigit(*mig_media))) {
		errno = EINVAL;
		return (-1);
	}
	media = (DT_THIRD_PARTY | *mig_media);

	if (mnt_pnt == NULL) {
		errno = EFAULT;
		return (-1);
	}
	fd = OpenInodesFile(mnt_pnt);
	if (fd < 0) {
		return (-1);
	}
	if ((inode_buffer = (union sam_di_ino *)malloc(INO_BLK_FACTOR *
	    INO_BLK_SIZE)) == NULL) {
		close(fd);
		return (-1);
	}
	while ((read_size = read(fd, inode_buffer,
	    INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {
		union sam_di_ino *data = (union sam_di_ino *)inode_buffer;
		char **tmp_vsn_list;

		while (read_size > 0) {
			int cpy;
			struct sam_perm_inode *inode = &data->inode;

			read_size -= sizeof (*data);
			data++;

			if (inode->di.id.ino != expected_ino++)
				continue;

			if (inode->di.mode == 0 || S_ISEXT(inode->di.mode) ||
			    inode->di.arch_status == 0)
				continue;

			if ((inode->di.arch_status & 0x0f)
			    != inode->di.arch_status)
				continue;


			for (cpy = 0; cpy < MAX_ARCHIVE; cpy++) {
				/* If not archived at this copy */
				if (!(inode->di.arch_status & (1 << cpy)))
					continue;

				if (media != inode->di.media[cpy])
					continue;

				for (tmp_vsn_list = vsns; *tmp_vsn_list != NULL;
				    tmp_vsn_list++) {
					struct sam_ioctl_idscf arg;

					if (strcmp(*tmp_vsn_list,
					    inode->ar.image[cpy].vsn))
						continue;

					arg.id = inode->di.id;
					arg.copy = cpy;
					arg.c_flags = AR_rearch;
					arg.flags = AR_rearch;
					if (ioctl(fd, F_IDSCF, &arg) >= 0)
						count++;
					else {
						char err_msg[128];

						(void) StrFromErrno(errno,
						    err_msg, sizeof (err_msg));

						(void) fprintf(stderr,
						    "sam_mig_rearchive: "
						    "ioctl error %s: "
						    "inode %d\n",
						    err_msg,
						    (int)inode->di.id.ino);
					}
				}
			}
		}
	}

	close(fd);
	free(inode_buffer);
	return (count);
}
