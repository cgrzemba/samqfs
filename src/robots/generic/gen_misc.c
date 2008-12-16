/*
 * gen_misc.c - misc routines for generic
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

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/external_data.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "aml/trace.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "driver/samst_def.h"

#include "element.h"
#include "generic.h"

#pragma ident "$Revision: 1.36 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;


/*
 *	is_storage - is the specified element a storage address
 *
 */
int
is_storage(
	library_t *library,
	const uint_t element)
{
	SANITY_CHECK(library != (library_t *)0);
	SANITY_CHECK(library->un != (dev_ent_t *)0)
		if (IS_GENERIC_API(library->un->type))
		return (TRUE);

	if ((element >= library->range.storage_lower &&
	    element <= library->range.storage_upper) ||
	    ((library->un->type == DT_ACL452) &&
	    (element >= library->range.ie_lower &&
	    element <= library->range.ie_upper)))
		return (TRUE);
	return (FALSE);
}


/*
 *	slot_number - given the element address, return the slot number
 *
 */
uint_t
slot_number(
	    library_t *library,
	    const uint_t element)
{
	SANITY_CHECK(library != (library_t *)0);
	SANITY_CHECK(library->un != (dev_ent_t *)0)
		if (IS_GENERIC_API(library->un->type))
		return (element);

	if ((element >= library->range.storage_lower &&
	    element <= library->range.storage_upper))
		return (element - library->range.storage_lower);

	if ((library->un->type == DT_ACL452) &&
	    (element >= library->range.ie_lower &&
	    element <= library->range.ie_upper)) {
		uint_t	  return_slot = library->range.storage_count;
		iport_state_t  *iport = library->import;
		for (; (return_slot <= library->audit_tab_len) && iport != NULL;
		    return_slot++, iport = iport->next)
			if (iport->element == element)
				return (return_slot);
	}
	return (ROBOT_NO_SLOT);
}


/*
 *	element_address - given the slot number, return the element address
 *
 */
uint_t
element_address(
		library_t *library,
		const uint_t slot)
{
	uint_t	  ex_start_slot;
	iport_state_t  *iport;

	SANITY_CHECK(library != (library_t *)0);
	SANITY_CHECK(library->un != (dev_ent_t *)0)
		if (IS_GENERIC_API(library->un->type))
		return (slot);

	ex_start_slot = library->range.storage_count;
	iport = library->import;

	if (library->un->type != DT_ACL452 ||
	    slot < library->range.storage_count)
		return (library->range.storage_lower + slot);

	for (; (ex_start_slot <= library->audit_tab_len) && (iport != NULL);
	    ex_start_slot++, iport = iport->next) {
		if (ex_start_slot == slot)
			return (iport->element);
	}
	return (ROBOT_NO_SLOT);
}


/*
 *	sam2aci_type - given a sam media type, return a aci media
 */
int
sam2aci_type(
		media_t media)
{
#if !defined(SAM_OPEN_SOURCE)
	aci_media_t	aci_media;

	switch (media) {
	case DT_VIDEO_TAPE:
		aci_media = ACI_VHS;
		break;
	case DT_SQUARE_TAPE:
	case DT_9840:
		aci_media = ACI_3480;
		break;
	case DT_EXABYTE_TAPE:
	case DT_SONYAIT:
		aci_media = ACI_8MM;
		break;
	case DT_LINEAR_TAPE:
		aci_media = ACI_DECDLT;
		break;
	case DT_DAT:
		aci_media = ACI_4MM;
		break;
	case DT_3590:
	case DT_9940:
		aci_media = ACI_3590;
		break;
	case DT_SONYDTF:
		aci_media = ACI_DTF;
		break;
	case DT_IBM3580:
		aci_media = ACI_LTO;
		break;
	case DT_3592:
		aci_media = ACI_3592;
		break;
	case DT_OPTICAL:
	case DT_ERASABLE:
	case DT_WORM_OPTICAL_12:
	case DT_WORM_OPTICAL:
		aci_media = ACI_OD_THICK;
/*	aci_media = ACI_OD_THIN; */
		break;
	default:
		aci_media = 0;
	}
	return ((int)aci_media);
#endif
}


/*
 * Find a free slot in a library.
 * Search storage elements for free slot.
 */
int				/* Slot number */
FindFreeSlot(
		library_t *library)
{
	struct VolId    vid;
	dev_ent_t	*un;
	element_status_page_t *status_page;
	storage_element_t *storage_descrip;
	uint_t	  current_element, last_element;
	uchar_t	*buffer = NULL;
	int		buff_size;
	int		num_elements;
	int		slot;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;

	un = library->un;
	memset(&vid, 0, sizeof (struct VolId));

	mutex_lock(&library->un->mutex);
	slot = ROBOT_NO_SLOT;
	last_element = 0;
	current_element = library->range.storage_lower;
	vid.ViFlags = VI_cart;
	vid.ViEq = library->un->eq;
	memmove(vid.ViMtype, sam_mediatoa(library->un->media),
	    sizeof (vid.ViMtype));

	/* Read storage element status in blocks of MAX_STORE_STATUS. */
	while (current_element <= library->range.storage_upper) {
		uint16_t	count, ele_dest_len;
		int		i;
		element_status_data_t *status_data;

		if ((num_elements = library->range.storage_count -
		    last_element) > MAX_STORE_STATUS)
			num_elements = MAX_STORE_STATUS;

	redo_storage:
		buff_size = num_elements * library->ele_dest_len +
		    sizeof (element_status_data_t) +
		    sizeof (element_status_page_t) + 50;

		buffer = malloc_wait(buff_size, 2, 0);

		if (read_element_status(library, STORAGE_ELEMENT,
		    current_element,
		    num_elements, buffer, buff_size) < 0) {
			DevLog(DL_ERR(5014));
			mutex_unlock(&library->un->mutex);
			free(buffer);
			return (ROBOT_NO_SLOT);
		}

		status_data = (element_status_data_t *)buffer;
		BE16toH(&status_data->numb_elements, &count);

		if (count != num_elements) {
			DevLog(DL_ERR(5063), count, num_elements);
			num_elements = count;
		}

		/*
		 * If size of ele_dest_len returned is gt
		 * size set aside in library, change library size
		 * to ele_dest_len, free the buffer and go to redo_storage
		 */
		status_page = (element_status_page_t *)
		    (buffer + sizeof (element_status_data_t));
		BE16toH(&status_page->ele_dest_len, &ele_dest_len);
		if (ele_dest_len > library->ele_dest_len) {
			library->ele_dest_len = ele_dest_len;
			free(buffer);
			goto redo_storage;
		}
		last_element += num_elements;

		storage_descrip = (storage_element_t *)
		    ((char *)status_page + sizeof (element_status_page_t));

		for (i = 0; i < num_elements; i++) {
			if (!storage_descrip->full) {
				slot = SLOT_NUMBER(library, current_element);
				vid.ViSlot = slot;
				/*
				 * storage_descrip->full only tells us if the
				 * slot is occupied, it could still be in
				 * use.
				 */
				if ((ce = CatalogCheckSlot(&vid, &ced))
				    == NULL) {
					break;
				} else {
					slot = ROBOT_NO_SLOT;
				}
			}
			current_element++;
			storage_descrip = (storage_element_t *)
			    ((char *)storage_descrip + ele_dest_len);
		}
		free(buffer);
		if (slot != ROBOT_NO_SLOT)
			break;
	}
	if (ce == NULL && slot != ROBOT_NO_SLOT) {
		DevLog(DL_DETAIL(5338), slot);
		mutex_unlock(&library->un->mutex);
		return (slot);
	} else {
		DevLog(DL_ERR(5337));
		mutex_unlock(&library->un->mutex);
		return (ROBOT_NO_SLOT);
	}
}
