/*
 * misc.c - misc routines.
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

#pragma ident "$Revision: 1.43 $"

static char    *_SrcFile = __FILE__;

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <ctype.h>
#include <thread.h>
#include <synch.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sam/defaults.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/archiver.h"
#include "sam/checksum.h"
#define	DEC_INIT
#include "sam/checksumf.h"
#include "aml/remote.h"
#undef DEC_INIT
#include "aml/historian.h"
#include "aml/proto.h"
#include "aml/dev_log.h"
#include "sam/nl_samfs.h"
#include "sam/names.h"
#include "pub/sam_errno.h"

#include "aml/fsd.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"


/* Some globals */

extern shm_alloc_t master_shm, preview_shm;

static void    *do_csum(void *);

typedef struct sam_csum_t {
	csum_t		*csum_val;
	int		csum_algo;
	csum_func	csum;
	char		*buffer;
	int		nbytes;
	int		done;
	sema_t		*csum_sema_done;
	sema_t		*csum_sema_go;
} sam_csum_t;

#if	defined(TRACE_ACTIVES)
int
DeC_AcTiVe(char *name, int line, dev_ent_t *un)
{
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG,
		    "dec_active from %d for eq %d (%s : %d)\n",
		    un->active, un->eq, name, line);
	return (un->active ? --(un)->active : 0);
}

int
InC_AcTiVe(char *name, int line, dev_ent_t *un)
{
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG,
		    "inc_active from %d for eq %d (%s : %d)\n",
		    un->active, un->eq, name, line);
	return (un->active++);
}
#endif				/* TRACE_ACTIVES */

#if defined(DEBUG)

/*
 * _Sanity_check
 */
void
_Sanity_check(char *SrcFile, int SrcLine)
{
	sam_syslog(LOG_WARNING,
	    "Sanity Check Failed in %s at line %d", SrcFile, SrcLine);
	abort();
}
#endif				/* defined(DEBUG) */


/*
 * change the mode of all tape devices given the path of any one. currently
 * works for st and dst drivers (/dev/rmt and /dev/rdst).
 */
void
ChangeMode(const char *name, const int mode)
{
	char	*basedev;
	char	scrdev[100];
	int	bdlength, i;
#define	NDENSITY 6
	char	density[NDENSITY] = {'l', 'm', 'h', 'c', 'u', '\0'};
#define	NDSTDEVS 16

	basedev = strdup(name);

	if (strncmp(basedev, "/dev/rmt", 8) == 0) {

		/* strip density etc. characters leaving just /dev/rmt/n(n) */

		bdlength = strlen(basedev) - 1;
		while (!isdigit(basedev[bdlength])) {
			bdlength--;
		}
		basedev[bdlength + 1] = '\0';
		strcpy(scrdev, basedev);

		for (i = 0; i < NDENSITY; i++) {
			scrdev[bdlength + 1] = density[i];
			if (density[i] == '\0')
				bdlength--;

			scrdev[bdlength + 2] = '\0';
			scrdev[bdlength + 3] = '\0';
			scrdev[bdlength + 4] = '\0';
			chmod(scrdev, mode);
			scrdev[bdlength + 2] = 'n';
			chmod(scrdev, mode);
			scrdev[bdlength + 2] = 'b';
			chmod(scrdev, mode);
			scrdev[bdlength + 3] = 'n';
			chmod(scrdev, mode);
		}
	} else if (strncmp(basedev, "/dev/rdst", 9) == 0) {

		/* strip density etc. characters leaving just /dev/rdstn(n) */

		bdlength = 8 + 1;	/* character after the "t" */
		while (isdigit(basedev[bdlength])) {
			bdlength++;
		}
		basedev[bdlength] = '\0';
		bdlength--;
		strcpy(scrdev, basedev);

		for (i = 0; i < NDSTDEVS; i++) {
			if (i > 9) {
				scrdev[bdlength + 2] = '1';
				scrdev[bdlength + 3] = '0' + (i % 10);
				scrdev[bdlength + 4] = '\0';
			} else {
				scrdev[bdlength + 2] = '0' + i;
				scrdev[bdlength + 3] = '\0';
			}
			scrdev[bdlength + 1] = '.';
			chmod(scrdev, mode);
			scrdev[bdlength + 1] = ',';
			chmod(scrdev, mode);
		}
	}
	free(basedev);
}

/*
 * Set device state to DOWN. Issue messages to logs and set DOWN.
 */
void
DownDevice(dev_ent_t *un, int source_flag)
{
	

	if (source_flag != USER_STATE_CHANGE) {
		DevLog(DL_ALL(1008));
		SendCustMsg(HERE, 9301, un->eq);
		un->state = DEV_DOWN;

		HandleMediaInOffDevice(un);
		send_notify_email(un, "dev_down.sh");
	} else {
		sam_syslog(LOG_WARNING, "Request to down device from user :"
		    " Not supported.");
		SendCustMsg(HERE, 9311, un->eq);
	}

}


/*
 * Set device state to OFF. Issue messages to logs and set OFF.
 */
void
OffDevice(dev_ent_t *un, int source_flag)
{
	DevLog(DL_ALL(1009));
	if (source_flag == USER_STATE_CHANGE) {
		sam_syslog(LOG_CRIT,
		    catgets(catfd, SET, 9312,
		    "Device %d:  State set to OFF by user."
		    " Check device log."), un->eq);
	} else {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 9302,
		    "SAM-FS attempted to set device %d to off."), un->eq);
	}
	un->state = DEV_OFF;

	HandleMediaInOffDevice(un);
	send_notify_email(un, "dev_down.sh");
}


/*
 * HandleMediaInOffDevice is called when SAM detects that a drive has gone
 * off or down.  HandleMediaInOffDevice checks the device table associated
 * with the downed/off'ed drive to see if the drive has media stuck in it.
 *
 * If the device table associated with the drive indicates that the drive has
 * media stuck in it, then this subroutine updates the catalog entry to
 * indicate that the media is Unavailable.  In the case of magneto-optical
 * media in an automated library, this code sets both sides of the MO to
 * Unavailable.
 */

void
HandleMediaInOffDevice(dev_ent_t *un)
{
	uint_t	slot;
	int	drive_eq;
	int	robot_eq;
	struct CatalogEntry	ce1;
	struct CatalogEntry	*ce1_ptr;
	struct CatalogEntry	ce2;
	struct CatalogEntry	*ce2_ptr;

	ASSERT(un != NULL);

	if (IS_HISTORIAN(un) || IS_ROBOT(un) || IS_MANUAL(un) ||
	    (un->slot == ROBOT_NO_SLOT)) {
		/*
		 * Handle media stuck in a drive. If un->slot is
		 * ROBOT_NO_SLOT, then the device is empty.
		 */

		return;
	}

	/* The device is not empty */
	(void) memset((void *) &ce1, 0, sizeof (struct CatalogEntry));
	(void) memset((void *) &ce2, 0, sizeof (struct CatalogEntry));

	ce1_ptr = (struct CatalogEntry *)NULL;
	ce2_ptr = (struct CatalogEntry *)NULL;

	robot_eq = ((un->fseq == 0) ? un->eq : un->fseq);
	drive_eq = (int)un->eq;
	slot = (int)un->slot;

	if (slot != ROBOT_NO_SLOT) {
		sam_syslog(LOG_ALERT, catgets(catfd, SET, 9352,
		    "Volume is in drive %d that is off or down."),
		    drive_eq);
	}
	/*
	 * Look up the catalog entry for the media that is stuck in the
	 * device
	 */
	if (IS_OPTICAL(un)) {
		ce1_ptr = CatalogGetCeByLoc(robot_eq, slot, 1, &ce1);
		ce2_ptr = CatalogGetCeByLoc(robot_eq, slot, 2, &ce2);
	} else {
		ce1_ptr = CatalogGetCeByLoc(robot_eq, slot, 0, &ce1);
	}

	if ((ce1_ptr != &ce1) ||
	    (IS_OPTICAL(un) && (ce2_ptr != &ce2))) {
		return;
	}
	if (!(ce1.CeStatus & CES_inuse)) {
		/*
		 * Unit table information indicates that a piece of media
		 * associated with(robot_eq and slot) is in the drive, but
		 * the catalog entry indicates that the slot is currently
		 * empty and not in-use. An unused, empty slot can not be in
		 * the drive.
		 */
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 9350,
		    "Slot %d in eq %d is empty; "
		    "empty slot can not be in drive."), slot, robot_eq);

		return;
	}
	if (ce1.CeStatus & CES_occupied) {
		/*
		 * Unit table information indicates that a piece of media
		 * associated with(robot_eq and slot) is in the drive, but
		 * the catalog entry indicates that the slot is currently
		 * occupied and the piece of media is back at home.
		 */
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 9351, "Slot %d in eq %d is "
		    "occupied; media can not be occupied "
		    "in its slot and in the drive at the same time."),
		    slot, robot_eq);

		return;
	}
	/*
	 * Catalog entry found for robot_eq, slot.  Catalog entry does not
	 * indicate that the slot is empty and does not indicate that the
	 * slot is occupied.  Media is jammed in the down drive, set the
	 * media's status in its catalog entry to Unavailable so that the
	 * archiver and stager no longer try to use the media.
	 */

	if (IS_OPTICAL(un)) {
		(void) CatalogSetFieldByLoc(robot_eq, slot, 1, CEF_Status,
		    CES_unavail, CES_unavail);
		(void) CatalogSetFieldByLoc(robot_eq, slot, 2, CEF_Status,
		    CES_unavail, CES_unavail);
	} else {
		(void) CatalogSetFieldByLoc(robot_eq, slot, 0, CEF_Status,
		    CES_unavail, CES_unavail);
	}

	if (slot != ROBOT_NO_SLOT) {
		sam_syslog(LOG_NOTICE, catgets(catfd, SET, 9353,
		    "Volume (%d:%d) in drive %d set to Unavailable."),
		    robot_eq, slot, drive_eq);
	}
}

/*
 * Set device state to ON. Issue messages to logs and set ON.
 */
void
OnDevice(dev_ent_t *un)
{
	DevLog(DL_ALL(1118));
	sam_syslog(LOG_INFO,
	    catgets(catfd, SET, 9304, "Device %d:  State set to ON."), un->eq);
	un->state = DEV_ON;
}

void
send_notify_email(dev_ent_t *device, char *script)
{
	char	*execute_dir;
	char	*command_line;

	if (device == NULL)
		return;

	/* if script not supplied, use default */
	if (script == (char *)0)
		script = DEV_NOTIFY_DEFAULT;

	execute_dir = (char *)malloc_wait(strlen(DEV_NOTIFY_PATH)
	    + strlen(script) + 2, 2, 0);
	command_line = (char *)malloc_wait(strlen("   , equipment number   ")
	    + strlen((char *)device->vendor_id)
	    + strlen((char *)device->product_id)
	    + (sizeof (device->eq) * 3) + 3, 2, 0);
	sprintf(execute_dir, "%s/%s", DEV_NOTIFY_PATH, script);

	sprintf(command_line, "%s %s, equipment number %d",
	    device->vendor_id, device->product_id, device->eq);

	(void) FsdNotify(execute_dir, LOG_NOTICE, 0, command_line);

	free(execute_dir);
	free(command_line);
}


/*
 * handles_match - compare two handle, return 1 if match, 0 if nomatch.
 */
int
handles_match(sam_handle_t *handle1, sam_handle_t *handle2)
{
	if ((handle1->id.ino == handle2->id.ino) &&
	    (handle1->id.gen == handle2->id.gen) &&
	    (handle1->fseq == handle2->fseq)) {
		return (1);
	}
	return (0);
}

/*
 * error_handler - do strerror with check for null. If SAMerrno, look up in
 * catalog.
 */
char	*
error_handler(int errnum)
{
	static char	errmsg[STR_FROM_ERRNO_BUF_SIZE];

	return (StrFromErrno(errnum, errmsg, sizeof (errmsg)));
}

/*
 * post_sys_mes - post system message.
 */
int
post_sys_mes(char *message, int flags)
{
	char	*mes;

	if (flags >= DIS_MES_TYPS)
		return (-1);

	mes = &((shm_ptr_tbl_t *)master_shm.shared_memory)->dis_mes[flags][0];
	(void) strncpy(mes, message, DIS_MES_LEN - 1);
	return (0);
}

/*
 * post_dev_mes - post message to a device.
 */
int
post_dev_mes(dev_ent_t *un, char *message, int flags)
{
	char	*mes;

	if (flags >= DIS_MES_TYPS)
		return (-1);

	mes = &un->dis_mes[flags][0];
	(void) strncpy(mes, message, DIS_MES_LEN - 1);
	return (0);
}

/*
 * vsn_form_barcode - build a vsn fom the zero filled barcode.
 */
void
vsn_from_barcode(char *vsn, char *barcode, sam_defaults_t *defaults, int size)
{
	int		len = strlen(barcode);
	register char	*tmp_chr;

	(void) memset(vsn, 0, size + 1);
	if (defaults->flags & DF_BARCODE_LOW) {
		if (len > size)
			(void) memcpy(vsn, (barcode + (len - size)), size);
		else
			(void) strncpy(vsn, barcode, size);
	} else
		(void) strncpy(vsn, barcode, size);

	/*
	 * Tape vsns are six characters and uppercase only  with no
	 * intervening blanks.
	 */
	if (size == 6)
		for (tmp_chr = vsn; *tmp_chr != '\0'; tmp_chr++) {
			if (*tmp_chr == ' ')
				*tmp_chr = '_';	/* convert spaces to '_' */
			else
				(*tmp_chr) = toupper(*tmp_chr);
		}
}

/*
 * dtb - delete trailing blanks (delete trailing blanks in place).
 * an improved version of the DTB macro.
 */
void
dtb(
	uchar_t *string,		/* source string */
	const int llen)			/* length of source string */
{
	uchar_t	*t;
	int	len;

	len = strlen((const char *) string);
	len = (llen < len) ? llen : len;
	for (t = string + len - 1;
	    t >= string && ((*t == ' ') || !isprint(*t)); t--) {
		*t = '\0';
	}

}

/*
 * zfn - zero fill name(remove trailing blanks from string).
 */
void
zfn(
	char *istring,		/* source string */
	char *ostring,		/* destination string */
	const int len)		/* length of source string */
{
	char	*tmpi, *tmpo;

	for (tmpi = istring + len - 1, tmpo = ostring + len - 1;
	    (*tmpi == ' ' || *tmpi == '\0') && tmpi >= istring;
	    tmpi--, tmpo--) {

		*tmpo = '\0';
	}

	for (; tmpi >= istring; tmpi--, tmpo--)
		*tmpo = *tmpi;
}

struct dev_ent *
find_historian(void)
{
	dev_ent_t	*device;

	device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);


	for (; device != NULL; device = (dev_ent_t *)SHM_REF_ADDR(device->next))
		if (device->type == DT_HISTORIAN)
			return (device);

	return (NULL);
}


extern mutex_t  lock_time_mutex;
#pragma weak lock_time_mutex

/*
 * mktime is not thread safe, use a mutex(if defined) to sync calls to mktime
 */
time_t
thread_mktime(struct tm *tm_time)
{
	if (&lock_time_mutex == NULL)
		return (mktime(tm_time));
	else {
		time_t	tmp_time;

		mutex_lock(&lock_time_mutex);
		tmp_time = mktime(tm_time);
		mutex_unlock(&lock_time_mutex);
		return (tmp_time);
	}
}

/*
 * notify_tp_mount - notify a thirdyparty "device" that requested media has
 * been mounted. Mutex on un should be locked on entry. unit can be a null
 * pointer(in cancel, for example). The caller must insure that there is no
 * other activity on the unit.
 */
void
notify_tp_mount(sam_handle_t *handle, dev_ent_t *un, const int errflag)
{
	char		*ent_pnt = "notify_tp_mount";
	dev_ent_t	*tp_un;
	dev_ptr_tbl_t	*dev_ptr_tbl;

	dev_ptr_tbl = (dev_ptr_tbl_t *)
	    SHM_REF_ADDR(((shm_ptr_tbl_t *)master_shm.shared_memory)->
	    dev_table);

	if ((int)handle->fseq <= dev_ptr_tbl->max_devices &&
	    dev_ptr_tbl->d_ent[handle->fseq] != 0) {
		/* Find the thirdparty device  */
		tp_un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->
		    d_ent[handle->fseq]);
		if (IS_THIRD_PARTY(tp_un)) {
			tp_notify_mount_t *tp_notify;
			message_request_t *message;

			message = (message_request_t *)SHM_REF_ADDR(tp_un->
			    dt.tr.message);
			tp_notify = &message->message.param.tp_notify_mount;
			/* Wait for message area to idle */
			mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID)
				cond_wait(&message->cond_i, &message->mutex);

			/* We own the message area, post our message */
			tp_notify->errflag = errflag;
			if (un != NULL) {
				tp_notify->eq = un->eq;
				INC_ACTIVE(un);
				un->mtime = 0;
			}
			tp_notify->event = (void *) handle->fifo_cmd.ptr;
			message->message.magic = (uint_t)MESSAGE_MAGIC;
			message->message.command = MESS_CMD_TP_MOUNT;
			message->mtype = MESS_MT_NORMAL;
			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
			if (DBG_LVL(SAM_DBG_TMESG))
				sam_syslog(LOG_INFO, "MS:S-TP(%#x).", message);
		} else
			sam_syslog(LOG_ERR,
			    "%s: Not thirdy party device.", ent_pnt);
	} else
		sam_syslog(LOG_ERR,
		    "%s: Device out of range or not valid.", ent_pnt);
}

/*
 * send a preview message to every robot and remote sam client
 */
void
signal_preview()
{
	dev_ent_t	*device = NULL;
	message_request_t *message;

	device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; device != NULL;
	    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if ((IS_ROBOT(device) ||
		    IS_RSC(device)) && (device->state < DEV_IDLE) &&
		    (device->status.b.ready && device->status.b.present)) {
			if (DBG_LVL(SAM_DBG_EVENT))
				sam_syslog(LOG_DEBUG,
				    "signal_preview: %d.", device->eq);
			/*
			 * By law, the robots and remote sam client look the
			 * same in the first few positions of their "dt"
			 * entry.
			 */
			message = (message_request_t *)SHM_REF_ADDR(device->
			    dt.rb.message);
			mutex_lock(&message->mutex);
			while (message->mtype != MESS_MT_VOID)
				cond_wait(&message->cond_i, &message->mutex);

			(void) memset(&message->message, 0,
			    sizeof (sam_message_t));
			message->message.magic = (uint_t)MESSAGE_MAGIC;
			message->message.command = MESS_CMD_PREVIEW;
			message->mtype = MESS_MT_NORMAL;
			cond_signal(&message->cond_r);
			mutex_unlock(&message->mutex);
		}
	}
}

/*
 * set_bad_media - set the bad media flag and turn off position bit This will
 * usually be called when we are unable to determine the the EOD or clean up
 * the EOD markers.
 *
 * un and un->entry mutex will be acquired
 */
void
set_bad_media(dev_ent_t *un)
{
	int	eq;

	mutex_lock(&un->mutex);
	un->status.bits |= DVST_BAD_MEDIA;
	un->status.bits &= ~DVST_POSITION;
	mutex_unlock(&un->mutex);

	if (un->slot != ROBOT_NO_SLOT) {
		mutex_lock(&un->entry_mutex);
		eq = (un->fseq ? un->fseq : un->eq);
		(void) CatalogSetFieldByLoc(eq, un->slot, un->i.ViPart,
		    CEF_Status, CES_bad_media, 0);
		mutex_unlock(&un->entry_mutex);

	}
	DevLog(DL_ERR(3044));
	sam_syslog(LOG_WARNING, catgets(catfd, SET, 9303,
	    "Device %d, VSN %s:  Bad medium.  Check device log."),
	    un->eq, un->vsn);
}


int
start_csum_thread(sam_csum_t *sam_csum)
{
	thread_t	csum_id;

	if (thr_create(NULL, SM_THR_STK, do_csum, (void *) sam_csum,
	    (THR_BOUND), &csum_id)) {
		return (-1);
	}
	return (csum_id);
}

static void *
do_csum(void *param)
{
	sam_csum_t	*sam_csum = (sam_csum_t *)param;

	while (!sam_csum->done) {
		if (sema_wait(sam_csum->csum_sema_go) != 0)
			sam_syslog(LOG_ERR,
			    "sema_wait failed at %d: %m", __LINE__);
		if (sam_csum->done)
			break;
		if (sam_csum->csum_algo & CS_USER_BIT) {
			sam_csum->csum(0, sam_csum->csum_algo, sam_csum->buffer,
			    sam_csum->nbytes, sam_csum->csum_val);
		} else {
			sam_csum->csum(0, sam_csum->buffer, sam_csum->nbytes,
			    sam_csum->csum_val);
		}
		if (sema_post(sam_csum->csum_sema_done) != 0)
			sam_syslog(LOG_ERR,
			    "sema_post failed at %d: %m", __LINE__);
	}
	thr_exit(NULL);
	return (NULL);
}
