/*
 * delay_resp.c - process delayed request from the ibm library.
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

#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "aml/tapes.h"
#include "sam/defaults.h"
#include "ibm3494.h"
#include "aml/catlib.h"
#include "aml/catalog.h"
#include "aml/proto.h"

#pragma ident "$Revision: 1.24 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */


/* function prototypes */
void process_delay_message(library_t *, IBM_wait_ret_t *);
void process_unsol_message(library_t *, IBM_wait_ret_t *);
void *import_new_media(void *);
void *wait_ready_thread(void *);

mutex_t new_media_mutex;
thread_t new_media_thread = (thread_t)-1;
int new_media = -1;


/* some globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * monitor_msg - thread routine to monitor messages.
 */
void *
delay_resp(
	void *vlibrary)
{
	library_t	 *library = (library_t *)vlibrary;

	/* Should have been called with all signals blocked. */
	memset(&new_media_mutex, 0, sizeof (mutex_t));
	mutex_lock(&library->mutex);	  /* wait for initialize */
	mutex_unlock(&library->mutex);

	if (!library->options.b.shared) {
		mutex_lock(&new_media_mutex);    /* check out any new media */
		if (thr_create(NULL, 0, import_new_media, (void *)library,
		    THR_DETACHED, &new_media_thread)) {
			new_media_thread = (thread_t)-1;
			sam_syslog(LOG_INFO,
			    "delay_resp(%d): import_new_media thread: %m.",
			    LIBEQ);
		}
		mutex_unlock(&new_media_mutex);
	}

	while (1) {
		int ret_ioctl;
		IBM_wait_arg_t  args;
		IBM_wait_ret_t  *ret = &args.mtlewret;

		memset(&args, 0, sizeof (args));
		args.subcmd = LEWTIME;

		ret_ioctl = ioctl_ibmatl(library->open_fd, MTIOCLEW, &args);
		if (ret_ioctl != 0 && ret->cc != 0)
			sam_syslog(LOG_INFO,
			    "delay_resp(%d) : ioctl_ibmatl(%d): %m.", LIBEQ,
			    ret_ioctl);
		switch (ret->msg_type) {
		case DELAY_RESP_MSG:
			process_delay_message(library, ret);
			break;

		case UNSOL_ATTN_MSG:
			process_unsol_message(library, ret);
			break;

		case NO_MSG:
			continue;

		default:
			sam_syslog(LOG_INFO,
			    "delay_resp(%d):unknown message type %#x.",
			    LIBEQ, ret->msg_type);
			continue;
		}
	}
}


void
process_unsol_message(
	library_t *library,
	IBM_wait_ret_t *ret)
{
	IBM_info_t  *info = &ret->msg_info;

	switch (ret->lib_event) {
	case MT_NTF_ATTN_CSC: {	/* Category state change */
		ushort_t  category;

		memcpy(&category,
		    info->type_spec_msg_info.category_state_chg.category,
		    sizeof (category));
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "unsol(%d): Category added %#x.",
			    LIBEQ, category);

		if (category == INSERT_CATEGORY && !library->options.b.shared) {
			mutex_lock(&new_media_mutex);
			new_media++;
			if (new_media_thread == (thread_t)-1)
				if (thr_create(NULL, 0, import_new_media,
				    (void *)library,
				    THR_DETACHED, &new_media_thread)) {
					new_media_thread = (thread_t)-1;
					sam_syslog(LOG_INFO,
					    "unsol(%d): Unable to start"
					    " import_new_media thread. %m",
					    LIBEQ);
				}
				mutex_unlock(&new_media_mutex);
		}
	}
	break;

	case MT_NTF_ATTN_LMOM: { 	/* Library manager operator message  */
		char *s =
		    info->type_spec_msg_info.lib_mgr_operator_msg.operator_msg;
		char *t;

		t = s + 69;
		while (*t == ' ' && t > s)
			*t = '\0';
			sam_syslog(LOG_INFO, "unsol(%d): Opr_msg: %s.",
			    LIBEQ, s);
		}
	break;

	case MT_NTF_ATTN_IOSSC:		/* IO station state change */
		break;

	case MT_NTF_ATTN_OSC: {		/* Operational state change */
		ushort_t  new_state, bad_state;

		bad_state = BAD_STATE;
		memcpy(&new_state, info->
		    type_spec_msg_info.operational_state_chg.operational_state,
		    sizeof (short));

		mutex_lock(&library->un->mutex);
		library->un->status.b.attention = new_state & bad_state;
		library->un->status.b.stor_full = new_state & MT_LIB_SCF;

		/* If already running wait_library_ready, don't process state */
		if (!library->un->status.b.scanning) {
			if (new_state & MT_LIB_OCV)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Out of Cleaner Volumes.",
				    LIBEQ);
			if (new_state & MT_LIB_AOS)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Automated Operational State.",
				    LIBEQ);
			if (new_state & MT_LIB_POS)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Paused Operational State.",
				    LIBEQ);
			if (new_state & MT_LIB_MMOS)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Manual Mode Operational State.",
				    LIBEQ);
			if (new_state & MT_LIB_DO)
				sam_syslog(LOG_INFO, "unsol(%d): Degraded"
				" Operation.", LIBEQ);
			if (new_state & MT_LIB_SEIO)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Safety Enclose Interlock Open.",
				    LIBEQ);
			if (new_state & MT_LIB_VSNO)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Vision system Non-Operational.",
				    LIBEQ);
			if (new_state & MT_LIB_OL)
				sam_syslog(LOG_INFO, "unsol(%d): Offline.",
				    LIBEQ);
			if (new_state & MT_LIB_IR)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Intervention Required.", LIBEQ);
			if (new_state & MT_LIB_CK1)
				sam_syslog(LOG_INFO,
				    "unsol(%d): Library Manager Check 1"
				    " Condition.", LIBEQ);
			if (new_state & MT_LIB_SCF)
				sam_syslog(LOG_INFO,
				    "unsol(%d): All Storage Cells Full.",
				    LIBEQ);
			if (new_state & MT_LIB_DWD)
				sam_syslog(LOG_INFO, "unsol(%d): Dual Write"
				" Disabled.", LIBEQ);
			if (new_state & MT_LIB_EA)
				sam_syslog(LOG_INFO, "unsol(%d): Environmental"
				" Alert -- smoke detected.", LIBEQ);
			if (new_state & MT_LIB_MMMOS)
				sam_syslog(LOG_INFO, "unsol(%d) Managed Manual"
				" Mode Operational State.", LIBEQ);

			if ((new_state & bad_state) &&
			    !library->un->status.b.scanning) {
				library->un->status.b.scanning = TRUE;
				if (thr_create(NULL, 0, wait_ready_thread,
				    (void *)library, THR_DETACHED, NULL)) {
					library->un->status.b.scanning = FALSE;
					sam_syslog(LOG_INFO, "Unable to start"
					" wait_ready_thread %m.");
				}
			}
		}
		mutex_unlock(&library->un->mutex);
	}
	break;

	case MT_NTF_ATTN_VE: {		/* Volume exception */
		ushort_t cat;
		char  vol[7];

		memset(vol, 0, 7);
		memcpy(&cat, info->type_spec_msg_info.volume_exception.category,
		    sizeof (short));
		memcpy(vol,
		    info->type_spec_msg_info.volume_exception.volser, 6);
		sam_syslog(LOG_INFO,
		    "Unsol(%d):Vol-excep:cat %#x, op %#x, ex %#x, %s",
		    LIBEQ, cat,
		    (int)info->
		    type_spec_msg_info.volume_exception.operation_code,
		    (int)info->
		    type_spec_msg_info.volume_exception.exception_code,
		    vol);
		}
		break;

	case MT_NTF_ATTN_DAC:	/* Device availability change */
		break;

	case MT_NTF_ATTN_DCC:	/* Device category change */
		break;

	case MT_NTF_ERA7A:	/* Read library stats */
		break;

	default:
		if (DBG_LVL(SAM_DBG_DEBUG)) {
			sam_syslog(LOG_DEBUG, "Unsol(%d): %s(%#x), event %#x.",
			    LIBEQ, (ret->cc > HIGH_CC) ?
			    "Undefined" : cc_codes[ret->cc], ret->cc,
			    ret->lib_event);
			sam_syslog(LOG_DEBUG,
			    "Unsol(%d): len %#x, code %#x, id %#x, flg %#x.",
			    LIBEQ, info->length, info->msg_code,
			    info->msg_id, info->flags);
		}
		sam_syslog(LOG_INFO,
		    "Unsol(%d):Unknown event %#x.", LIBEQ, ret->lib_event);
	}
}


void
process_delay_message(
	library_t *library,
	IBM_wait_ret_t *ret)
{
	char		vol[7];
	char		*type_str = "mistake";
	ushort_t	cat;
	IBM_info_t	*info = &ret->msg_info;
	delay_list_ent_t  *dlist;

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG,
		    "process_delay_msg(%d): %#x: cc %#x, event %#x.",
		    LIBEQ, info->msg_id, ret->cc, ret->lib_event);

	switch (ret->lib_event) {
	case MT_NTF_DEL_MC:		/* Mount complete */
		type_str = "mount";
		break;

	case MT_NTF_DEL_DC:		/* Demount complete */
		type_str = "dismount";
		break;

	case MT_NTF_DEL_AC:		/* Audit complete */
		type_str = "audit";
		break;

	case MT_NTF_DEL_EC:		/* Eject complete */
		type_str = "eject";
		break;

	case MT_NTF_TIMEOUT:		/* Timeout */
		sam_syslog(LOG_INFO,
		    "process_delay_message(%d):Timeout event.", LIBEQ);
		return;

	default:
		sam_syslog(LOG_INFO,
		    "process_delay_message(%d):Unknown event %#x.", LIBEQ,
		    ret->lib_event);
		return;
	}

	memset(vol, 0, 7);
	memcpy(&cat, info->type_spec_msg_info.typeA_del_resp_msg.category,
	    sizeof (short));
	memcpy(vol, info->type_spec_msg_info.typeA_del_resp_msg.op_volser, 6);
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "process_delay_message(%d): %s complete: "
		    "cc %#x, fgs %#x, cat %#x: %s", LIBEQ, type_str, cat,
		    (int)info->type_spec_msg_info.typeA_del_resp_msg.comp_code,
		    (int)info->
		    type_spec_msg_info.typeA_del_resp_msg.operation_flags,
		    vol);

	mutex_lock(&library->dlist_mutex);
	for (dlist = library->delay_list; dlist != NULL; dlist = dlist->next)
		if (dlist->req_id == info->msg_id)
			break;

	if (dlist == NULL) {
		mutex_unlock(&library->dlist_mutex);
		sam_syslog(LOG_INFO,
		    "process_delay_message(%d): ID %#x,  no one waiting.",
		    LIBEQ, info->msg_id);
	} else {
		if (dlist->last != NULL)
			dlist->last->next = dlist->next;
		else
			library->delay_list = dlist->next;

		if (dlist->next != NULL)
			dlist->next->last = dlist->last;

		mutex_unlock(&library->dlist_mutex);

		if (dlist->event != NULL) {
			int cc = info->
			    type_spec_msg_info.typeA_del_resp_msg.comp_code;

			if (cc == MTCC_COMPLETE)
				disp_of_event(library, dlist->event, MC_REQ_OK);
			else {
				sam_syslog(LOG_INFO,
				    "process_delay_message(%d): %s cc = %#x",
				    LIBEQ, type_str, cc);
				disp_of_event(library, dlist->event, MC_REQ_TR);
			}
		}
		free(dlist);
	}
}


/*
 * import_new_media - the operator has done something
 *
 */
void *
import_new_media(
	void *vlibrary)
{
	int   			inv_count;
	int			status;
	req_comp_t  		err;
	library_t 		*library = (library_t *)vlibrary;
	IBM_query_info_t	*info = NULL;
	struct inv_recs		*inv_entry;
	struct VolId 		vid;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	mutex_lock(&library->un->mutex);
	while (library->un->status.b.scanning) {
		mutex_unlock(&library->un->mutex);
		sleep(5);
		mutex_lock(&library->un->mutex);
	}

	mutex_unlock(&library->un->mutex);
	mutex_lock(&new_media_mutex);

	if (new_media < 0) {
		new_media = 1;
		mutex_unlock(&new_media_mutex);
		sleep(15);
		mutex_lock(&new_media_mutex);
	}

	while (1 && new_media > 0) {
		err = view_media_category(library, 0, (void *)&info,
		    INSERT_CATEGORY);
		new_media = 0;
		if (err != MC_REQ_OK) {
			new_media_thread = (thread_t)-1;
			mutex_unlock(&new_media_mutex);
			thr_exit(NULL);
		}

		if (info == NULL) {
			new_media_thread = (thread_t)-1;
			mutex_unlock(&new_media_mutex);
			thr_exit(NULL);
		}

		memcpy(&inv_count, &info->data.cat_invent_data.no_vols[0],
		    sizeof (int));
		if (inv_count == 0) {
			new_media_thread = (thread_t)-1;
			mutex_unlock(&new_media_mutex);
			free(info);
			thr_exit(NULL);
		}
		inv_entry = &info->data.cat_invent_data.inv_rec[0];
		mutex_unlock(&new_media_mutex);

		if (inv_count > 100)
			inv_count = 100;

		while (inv_count) {
			int   k;
			char  vsn[7];

			vsn[6] = '\0';
			memcpy(&vsn[0], &inv_entry->volser[0], 6);
			for (k = 5; k >= 0; k--) {
				if (vsn[k] != ' ')
					break;
				else
					vsn[k] = '\0';
			}

			status = (CES_inuse | CES_occupied | CES_bar_code);
			memset(&vid, 0, sizeof (struct VolId));
			vid.ViEq = library->un->eq;
			vid.ViSlot = ROBOT_NO_SLOT;
			memmove(vid.ViMtype, sam_mediatoa(library->un->media),
			    sizeof (vid.ViMtype));

			ce = CatalogGetCeByBarCode(library->un->eq, vid.ViMtype,
			    vsn, &ced);
			if (! ce) {
				(void) CatalogSlotInit(&vid, status,
				    (library->status.b.two_sided) ? 2 : 0,
				    vsn, "");
				ce = CatalogGetCeByBarCode(library->un->eq,
				    vid.ViMtype,
				    vsn, &ced);
			}
			if (ce) {
				set_media_category(library, vsn,
				    INSERT_CATEGORY, library->sam_category);
			} else {
				sam_syslog(LOG_INFO,
				    "import(%d): Import failed, ejecting",
				    LIBEQ);
				set_media_category(library, vsn, 0,
				    library->sam_category);
				set_media_category(library, vsn,
				    library->sam_category,
				    EJECT_CATEGORY);
			}
			inv_count--;
			inv_entry++;
		}
		mutex_lock(&new_media_mutex);
	}
	new_media_thread = (thread_t)-1;
	mutex_unlock(&new_media_mutex);
	thr_exit(NULL);
}


void *
wait_ready_thread(
	void *vlibrary)
{
	library_t  *library = (library_t *)vlibrary;

	wait_library_ready(library);
	mutex_lock(&library->un->mutex);
	library->un->status.b.scanning = FALSE;
	mutex_unlock(&library->un->mutex);
	thr_exit(NULL);
}
