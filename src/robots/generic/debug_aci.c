 * debug_aci.c
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

#pragma ident "$Revision: 1.17 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <stdio.h>
#include <thread.h>
#include <fcntl.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "sam/devnm.h"
#include "aml/logging.h"
#include "aml/proto.h"
#include <rpc/rpc.h>
	/* this causes stuff in aci.h to not be declaired extern. */
#define	extern
#include "generic.h"


static int	tty = -1;
static char    *ok = "Ok\007\n";
extern char    *dbg_tty;
extern dev_ent_t *un;
static char	dummy[100];

	/* To serialize helper use the io_mutex */

static char    *ver = "LSCI Sim";

int
aci_qversion(char *x, char *y)
{
	mutex_lock(&un->io_mutex);
	strcpy(x, ver);
	strcpy(y, ver);
	mutex_unlock(&un->io_mutex);
	return (0);
}

int
aci_init(void)
{
	d_errno = 0;
	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	memset(dummy, '\0', 100);
	sprintf(dummy, "inititialize.\n");
	write(tty, dummy, sizeof (dummy));
	mutex_unlock(&un->io_mutex);
	return (0);
}


int
aci_force(char *drive_name)
{
	int		ret;

	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	memset(dummy, '\0', 100);
	sprintf(dummy, "aci_force %s??\007\n", drive_name);
	write(tty, dummy, sizeof (dummy));
	ret = d_errno = 0;
	read(tty, dummy, 100);
	sscanf(dummy, "%d %d", &ret, &d_errno);
	memset(dummy, '\0', 100);
	sprintf(dummy, "%d %d %s", ret, d_errno, ok);
	write(tty, dummy, sizeof (dummy));
	mutex_unlock(&un->io_mutex);
	return (ret);
}

int
aci_mount(char *volser, aci_media_t type, char *drive)
{
	int		ret;

	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	ret = d_errno = 0;
	memset(dummy, '\0', 100);
	sprintf(dummy, "aci_mount %s of type %d on %s??\007\n",
	    volser, (int)type, drive);
	write(tty, dummy, sizeof (dummy));
	read(tty, dummy, 100);
	sscanf(dummy, "%d %d", &ret, &d_errno);
	memset(dummy, '\0', 100);
	sprintf(dummy, "%d %d %s", ret, d_errno, ok);
	write(tty, dummy, sizeof (dummy));
	mutex_unlock(&un->io_mutex);
	return (ret);
}

int
aci_driveaccess(
		char *c1,
		char *c2,
		enum aci_drive_status status)
{
	int		ret;

	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		sam_syslog(LOG_DEBUG, "open tty");
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	ret = d_errno = 0;
	memset(dummy, '\0', 100);
	sprintf(dummy, "aci_driveaccess %s %s??\007\n", c1, c2);
	write(tty, dummy, sizeof (dummy));
	read(tty, dummy, 100);
	sscanf(dummy, "%d %d", &ret, &d_errno);
	memset(dummy, '\0', 100);
	sprintf(dummy, "%d %d %s", ret, d_errno, ok);
	write(tty, dummy, sizeof (dummy));
	mutex_unlock(&un->io_mutex);
	return (ret);
}

int
aci_dismount(char *c1, aci_media_t type)
{
	int		ret;

	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	ret = d_errno = 0;
	memset(dummy, '\0', 100);
	sprintf(dummy, "aci_dismount %s of type %d??\007\n", c1, (int)type);
	write(tty, dummy, sizeof (dummy));
	read(tty, dummy, 100);
	sscanf(dummy, "%d %d", &ret, &d_errno);
	memset(dummy, '\0', 100);
	sprintf(dummy, "%d %d %s", ret, d_errno, ok);
	write(tty, dummy, sizeof (dummy));
	mutex_unlock(&un->io_mutex);
	return (ret);
}

int
aci_view(char *c1, aci_media_t type, aci_vol_desc_t *volstuff)
{
	int		ret;

	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	ret = d_errno = 0;
	memset(dummy, '\0', 100);
	sprintf(dummy, "aci_view %s of type %d??\007\n", c1, (int)type);
	write(tty, dummy, sizeof (dummy));
	read(tty, dummy, 100);
	sscanf(dummy, "%d %d", &ret, &d_errno);
	memset(dummy, '\0', 100);
	sprintf(dummy, "%d %d %s", ret, d_errno, ok);
	write(tty, dummy, sizeof (dummy));
	strcpy(volstuff->volser, c1);
	mutex_unlock(&un->io_mutex);
	return (ret);
}

/*
 * For now, just return an error with d_errno = ETIMEOUT.
 */
int
aci_drivestatus(char *client, struct aci_drive_entry *drive_entry[])
{
	int		ret = -1;

	mutex_lock(&un->io_mutex);
	if (tty < 0) {
		if ((tty = open(dbg_tty, O_RDWR)) < 0) {
			d_errno = ERPC;
			mutex_unlock(&un->io_mutex);
			return (-1);
		}
	}
	ret = d_errno = 0;
	memset(dummy, '\0', 100);
	sprintf(dummy, "aci_drivestatus for client %s
	    will return ETIMEOUT\007\n", client);
	write(tty, dummy, sizeof (dummy));
	d_errno = ETIMEOUT;
	mutex_unlock(&un->io_mutex);
	return (ret);
}
