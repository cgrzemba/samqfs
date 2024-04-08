/*
 * element.c - routines for element status.
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

#pragma ident "$Revision: 1.43 $"

/* Using __FILE__ makes duplicate strings */
static char *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "aml/external_data.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "element.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"


/*	function prototypes */
void
copy_xport_status(xport_state_t *, transport_element_t *,
		transport_element_ext_t *, element_status_page_t *);
void
copy_drive_status(drive_state_t *, data_transfer_element_t *,
		data_transfer_element_ext_t *, element_status_page_t *);
void
copy_import_status(iport_state_t *, import_export_element_t *,
		import_export_element_ext_t *, element_status_page_t *,
		int *);

/*	Globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	read_element_status - request element status.
 *
 * exit -
 *	  number of bytes returned.
 *	  < 0 - error.
 */
int
read_element_status(
library_t *library,
const int type,
const int start,
const int count,
void *addr,
const int size)
{
	dev_ent_t 	*un;
	int 		retry = 3, get_barcodes;
	int 		len, resid, err = 0;
	int 		added_more_time = FALSE;
	char 		*l_mess = library->un->dis_mes[DIS_MES_NORM];
	sam_extended_sense_t *sense;

	SANITY_CHECK(library != (library_t *)0);
	SANITY_CHECK(addr != (void *)0);
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(library->un->sense);

	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);
	if (library->un->dt.rb.status.b.barcodes)
		get_barcodes = 0x10;
	else
		get_barcodes = 0;

	mutex_lock(&library->un->io_mutex);
	do {
		memset(sense, 0, sizeof (sam_extended_sense_t));
		if ((len = scsi_cmd(library->open_fd, library->un,
		    SCMD_READ_ELEMENT_STATUS, 0, addr, size, start,
		    count, type + get_barcodes, &resid)) <= 0) {
			/*
			 * If illegal request and asking for barcodes,
			 * don't ask for bar_codes and try again,
			 * does not effect retry
			 */
			if (sense->es_key == KEY_ILLEGAL_REQUEST &&
			    get_barcodes) {
				char *mess = "device does not support barcodes";

				mutex_lock(&library->un->mutex);
				library->un->dt.rb.status.b.barcodes = FALSE;
				mutex_unlock(&library->un->mutex);
				memccpy(l_mess, mess, '\0', DIS_MES_LEN);
				DevLog(DL_DETAIL(5006));
				get_barcodes = 0;
				retry++;
				continue;
			}
			TAPEALERT_SKEY(library->open_fd, library->un);
			GENERIC_SCSI_ERROR_PROCESSING(library->un,
			    library->scsi_err_tab, 0, err,
			    added_more_time, retry,
				/* DOWN_EQ code */
			    down_library(library, SAM_STATE_CHANGE);
				mutex_unlock(&library->un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				/* ILLREQ code */
				    mutex_unlock(&library->un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				    /* MACRO for cstyle */;
			/* MACRO for cstyle */)
		} else
			break;
	} while (--retry > 0);

	if (retry <= 0) {
		/* Retries exhausted */
		DevLog(DL_ERR(5201));
		len = -1;
	}
	mutex_unlock(&library->un->io_mutex);
	return (len);
}


/*
 *	status_element_range - issue init element range, read element and
 * update the local tables for the specified elements.
 *
 * NOTE: Ampex 810 doesn't seem to support init element range, but
 *		 robot always updates its elements when things change(like
 *		 media put in imex).
 * No mutex held on entry.
 */
int
status_element_range(
library_t *library,
const int type,
const uint_t start_element,
const uint_t req_count)
{
	dev_ent_t 	*un;
	int 		retry, err;
	int 		buff_size, i, num_eles;
	uint16_t 	count, ele_dest_len;
	uint_t 		current_element, last_element;
	char 		*buffer;
	element_status_page_t *status_page;
	element_status_data_t *status_data;
	xport_state_t 	*transport;
	iport_state_t 	*import;
	drive_state_t *drive;
	sam_extended_sense_t *sense;

	SANITY_CHECK(library != (library_t *)0);
	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	switch (type) {

	case TRANSPORT_ELEMENT:
		if (start_element < library->range.transport_lower ||
		    start_element > library->range.transport_upper ||
		    (start_element + req_count >
		    library->range.transport_upper + 1)) {

			DevLog(DL_ERR(5007), start_element, req_count,
			    library->range.transport_lower,
			    library->range.transport_upper);
			return (-1);
		}
		break;

	case STORAGE_ELEMENT:
		if (start_element < library->range.storage_lower ||
		    start_element > library->range.storage_upper ||
		    (start_element + req_count >
		    library->range.storage_upper + 1)) {

			DevLog(DL_ERR(5008), start_element, req_count,
			    library->range.storage_lower,
			    library->range.storage_upper);
			return (-1);
		}
		break;

	case IMPORT_EXPORT_ELEMENT:
	/* if there are no import/export elements to status, just return */
		if (req_count == 0)
			return (0);

		if (start_element < library->range.ie_lower ||
		    start_element > library->range.ie_upper ||
		    (start_element + req_count > library->range.ie_upper + 1)) {

			DevLog(DL_ERR(5009), start_element, req_count,
			    library->range.ie_lower, library->range.ie_upper);
			return (-1);
		}
		break;

	case DATA_TRANSFER_ELEMENT:
		if (start_element < library->range.drives_lower ||
		    start_element > library->range.drives_upper ||
		    (start_element + req_count >
		    library->range.drives_upper + 1)) {

			DevLog(DL_ERR(5010), start_element, req_count,
			    library->range.drives_lower,
			    library->range.drives_upper);
			return (-1);
		}
		break;
	}

	/*
	 * Some libraries don't seem to handle initialize element range
	 */


	if (library->un->equ_type != DT_HP_C7200 &&
	    library->un->equ_type != DT_FJNMXX &&
	    library->un->equ_type != DT_SL3000 &&
	    library->un->equ_type != DT_QUANTUMC4) {
		int 	added_more_time = FALSE;

		mutex_lock(&library->un->io_mutex);
		retry = 3;
		do {
			(void) memset(sense, 0, sizeof (sam_extended_sense_t));
			if (library->un->equ_type == DT_PLASMON_G) {
				int		x;

				for (x = 0; x < req_count; x++) {
					if ((err = scsi_cmd(library->open_fd,
					    library->un,
					    SCMD_INIT_SINGLE_ELEMENT_STATUS,
					    10, start_element + x))) {
						TAPEALERT_SKEY(library->open_fd,
						    library->un);
						GENERIC_SCSI_ERROR_PROCESSING(
						    library->un,
						    library->scsi_err_tab, 0,
						    err,
						    added_more_time, retry,
							/* DOWN_EQ code */
						    down_library(library,
						    SAM_STATE_CHANGE);
							mutex_unlock(&library->
							    un->io_mutex);
							return (-1);
							/* MACRO for cstyle */,
							/* ILLREQ code */
							    mutex_unlock(
							    &library->
							    un->io_mutex);
							return (-1);
							/* MACRO for cstyle */,
							    /* MACRO cstyle */;
					/* MACRO for cstyle */);
					}
				}
				break;
			} else if (library->un->equ_type == DT_HPSLXX) {
				if ((err = scsi_cmd(library->open_fd,
				    library->un,
				    SCMD_INIT_ELE_RANGE_37,
				    (req_count << 4), start_element,
				    req_count, NULL))) {
					TAPEALERT_SKEY(library->open_fd,
					    library->un);
					GENERIC_SCSI_ERROR_PROCESSING(
					    library->un,
					    library->scsi_err_tab, 0, err,
					    added_more_time, retry,
					    /* DOWN_EQ code */
					    down_library(library,
					    SAM_STATE_CHANGE);
					mutex_unlock(&library->un->io_mutex);
					return (-1);
					/* MACRO for cstyle */,
					/* ILLREQ code */
					    mutex_unlock(&library->
					    un->io_mutex);
					return (-1);
					/* MACRO for cstyle */,
					    /* MACRO for cstyle */;
				/* MACRO for cstyle */);
				}
				break;
			} else if ((err = scsi_cmd(library->open_fd,
			    library->un,
			    SCMD_INIT_ELE_RANGE,
			    (req_count << 4), start_element,
			    req_count, NULL))) {
				TAPEALERT_SKEY(library->open_fd,
				    library->un);
				GENERIC_SCSI_ERROR_PROCESSING(
				    library->un,
				    library->scsi_err_tab, 0, err,
				    added_more_time, retry,
					/* DOWN_EQ code */
				    down_library(library, SAM_STATE_CHANGE);
					mutex_unlock(&library->
					    un->io_mutex);
					return (-1);
					/* MACRO for cstyle */,
					/* ILLREQ code */
					    mutex_unlock(&library->
					    un->io_mutex);
					return (-1);
					/* MACRO for cstyle */,
					    /* MACRO for cstyle */;
			/* MACRO for cstyle */);
			} else
				break;
		} while (--retry > 0);

		mutex_unlock(&library->un->io_mutex);

		if (retry <= 0) {
			/* Retries exhausted */
			DevLog(DL_ERR(5219));
			return (-1);
		}
	}

	/*
	 * buffer size = number of elements *
	 * (size of structure) plus (size of element status data) plus (size of
	 * element status page)
	 */
	switch (type) {

	case TRANSPORT_ELEMENT:
		{
			transport_element_t *transport_descrip;

	redo_transport:
			buff_size =
			    (req_count * library->ele_dest_len) +
			    sizeof (element_status_data_t) +
			    sizeof (element_status_page_t) + 50;

			buffer = (char *)malloc_wait(buff_size, 2, 0);

			if (read_element_status(library, type, start_element,
			    req_count, buffer, buff_size) < 0) {
				DevLog(DL_ERR(5011));
				free(buffer);
				return (-1);
			}

			status_data = (element_status_data_t *)buffer;
			SANITY_CHECK(status_data != (element_status_data_t *)0);
			BE16toH(&status_data->numb_elements, &count);

			if (count != req_count) {
				DevLog(DL_DEBUG(5012), count, req_count);
			}

			status_page = (element_status_page_t *)
			    (buffer + sizeof (element_status_data_t));
			SANITY_CHECK(status_page != (element_status_page_t *)0);

			/*
			 * check the size of the element and if bigger than
			 * what we have set asside, change the size
			 * free the old buffer and try again
			 */
			BE16toH(&status_page->ele_dest_len, &ele_dest_len);
			if (ele_dest_len > library->ele_dest_len) {
				library->ele_dest_len = ele_dest_len;
				free(buffer);
				goto redo_transport;
			}
			transport_descrip = (transport_element_t *)
			    ((char *)status_page +
			    sizeof (element_status_page_t));

			if (status_page->type_code != TRANSPORT_ELEMENT) {
				DevLog(DL_ERR(5013), status_page->type_code);
				free(buffer);
				return (-1);
			}

			for (transport = library->transports;
			    transport != NULL; transport = transport->next) {

				if (transport->element == start_element) {
					break;
				}
			}

			while (transport != NULL && count--) {
				transport_element_ext_t *extension;

				if (ele_dest_len > sizeof (transport_element_t))
					extension = (transport_element_ext_t *)
					    ((char *)transport_descrip +
					    sizeof (transport_element_t));
				else
					extension = NULL;

				copy_xport_status(transport, transport_descrip,
				    extension, status_page);
				transport = transport->next;
				transport_descrip = (transport_element_t *)
				    ((char *)transport_descrip + ele_dest_len);
			}
			free(buffer);
			break;
		}

	case STORAGE_ELEMENT:
		{
			storage_element_t *storage_descrip;
			storage_element_ext_t *extension;

			last_element = 0;
			current_element = start_element;

			while (current_element < (start_element + req_count)) {
				if ((num_eles = req_count - last_element) >
				    MAX_STORE_STATUS)
					num_eles = MAX_STORE_STATUS;

		redo_storage:
				buff_size = num_eles * library->ele_dest_len +
				    sizeof (element_status_data_t) +
				    sizeof (element_status_page_t) + 50;

				buffer = malloc_wait(buff_size, 2, 0);

				if (read_element_status(library, type,
				    current_element,
				    num_eles, buffer, buff_size) < 0) {
					DevLog(DL_ERR(5014));
					free(buffer);
					return (-1);
				}

				status_data = (element_status_data_t *)buffer;
				SANITY_CHECK(status_data != (
				    element_status_data_t *)0);
				BE16toH(&status_data->numb_elements, &count);

				if (count != num_eles) {
					DevLog(DL_ERR(5015), count, num_eles);
				}
				status_page = (element_status_page_t *)
				    (buffer + sizeof (element_status_data_t));
				SANITY_CHECK(status_page != (
				    element_status_page_t *)0);

				/*
				 * check the size of the element and if bigger
				 * than what we have set aside, change the
				 * size, free the old buffer and try again
				 */
				BE16toH(&status_page->ele_dest_len,
				    &ele_dest_len);
				if (ele_dest_len > library->ele_dest_len) {
					library->ele_dest_len = ele_dest_len;
					free(buffer);
					goto redo_storage;
				}
				last_element += num_eles;

				storage_descrip = (storage_element_t *)
				    ((char *)status_page +
				    sizeof (element_status_page_t));

				if (status_page->type_code != STORAGE_ELEMENT) {
					DevLog(DL_ERR(5016),
					    status_page->type_code);
					free(buffer);
					return (-1);
				}

				for (i = 0;
				    i < num_eles;
				    i++, storage_descrip = (storage_element_t *)
				    ((char *)storage_descrip + ele_dest_len)) {
					struct VolId 	vid;
					uint16_t 		ele_addr;
					uint32_t 		status;

					BE16toH(&storage_descrip->ele_addr,
					    &ele_addr);
					if (ele_dest_len >
					    sizeof (storage_element_t))
						extension =
						    (storage_element_ext_t *)
						    ((char *)storage_descrip +
						    sizeof (storage_element_t));
					else
						extension = NULL;

					status = 0;
					memset(&vid, 0, sizeof (struct VolId));
					vid.ViFlags = VI_cart;
					vid.ViEq = library->un->eq;
					vid.ViSlot = SLOT_NUMBER(library,
					    ele_addr);

					if (library->un->equ_type ==
					    DT_PLASMON_G) {
						int media_offset, media_type;

						if (status_page->PVol &&
						    status_page->AVol) {
							media_offset = 88;
						} else if (status_page->PVol &&
						    !status_page->AVol) {
							media_offset = 52;
						} else {
							media_offset = 16;
						}

/* poor indentation due to cstyle requiremnets */
					media_type = ((uint8_t *)
					    storage_descrip)[media_offset];

					if (media_type == PLASMON_MT_UDO) {
					memmove(vid.ViMtype,
					    sam_mediatoa(DT_PLASMON_UDO),
					    sizeof (vid.ViMtype));
						if (library->un->media !=
						    DT_PLASMON_UDO) {
							status |= CES_bad_media;
						}
					} else if (media_type ==
					    PLASMON_MT_MO) {
						memmove(vid.ViMtype,
						    sam_mediatoa(DT_ERASABLE),
						    sizeof (vid.ViMtype));
						if (library->un->media !=
						    DT_ERASABLE) {
							status |= CES_bad_media;
						}
						}
					} else {
					memmove(vid.ViMtype,
					    sam_mediatoa(library->un->media),
					    sizeof (vid.ViMtype));
					}

					if (ele_addr == current_element) {

						if (!storage_descrip->access)
							status |= CES_unavail;

						/*
						 * The metrum library (D-28)
						 * returns a 83,2 for code
						 * if the 7 slot mag is
						 * not installed.
						 * This is not documented
						 * in the book.
						 */
				/*
				 * poor indentation to meet cstyle
				 * requirements.
				 */
		if (storage_descrip->except && extension != NULL) {

			switch (library->un->equ_type) {

				case DT_METD28:
					if (extension->add_sense_code == 0x83 &&
					    extension->add_sense_qual == 0x02) {
						status |= CES_unavail;
					}
					break;

				case DT_METD360:
					if (extension->add_sense_code != 0) {
						library->status.b.except = TRUE;
						DevLog(DL_ERR(5017),
						    extension->add_sense_code,
						    extension->add_sense_qual);
						return (0);
					}
					break;

				case DT_3570C:
					/* if the magazine is not available */
					if (extension->add_sense_code == 0x3b &&
					    extension->add_sense_qual == 0x11) {
						status |= CES_unavail;
					}
					break;

				default:
					break;
				}
		}

		if (storage_descrip->full) {

			status |= CES_inuse | CES_occupied;
				if (status_page->PVol && extension != NULL) {

					dtb(&(extension->PVolTag[0]),
					    BARCODE_LEN);


						if (is_barcode(
						    extension->PVolTag)) {
							status |= CES_bar_code;
						}

						if (status_page->AVol &&
						    (*extension->AVolTag !=
						    '\0')) {
							dtb(&(extension->
							    AVolTag[0]),
							    BARCODE_LEN);
						}
					}
				}
			}

			if ((storage_descrip->full) && (extension != NULL) &&
			    (*extension->PVolTag != '\0')) {
				(void) CatalogSlotInit(&vid, status,
				    (library->status.b.two_sided) ? 2 : 0,
				    (char *)extension->PVolTag,
				    (char *)extension->AVolTag);
			} else {
				(void) CatalogSlotInit(&vid, status,
				    (library->status.b.two_sided) ?
				    2 : 0, "", "");
			}

			if ((!library->status.b.two_sided) &&
			    (status & CES_inuse)) {

					struct CatalogEntry ced;
					struct CatalogEntry *ce = &ced;
					int set_default = 1;

			/*
			 * If there is a catalog entry, and the entry's
			 * capacity is zero, and not set to zero by the user,
			 * set the capacity to default.
			 *
			 * Cleaning tape capacity should be zero.
			 * This code will prevent a call to the catserver
			 * if cleaning tape capacity is zero, or set it to
			 * zero if not
			 */
			if ((ce = CatalogGetEntry(&vid, &ced)) != NULL) {
				if ((ce->CeStatus & CES_capacity_set) ||
				    (ce->CeCapacity &&
				    !(ce->CeStatus & CES_cleaning)) ||
				    (!ce->CeCapacity &&
				    (ce->CeStatus & CES_cleaning))) {
					set_default = 0;
					}
			}
			if (set_default) {
				int	capacity;

				if (!(ce->CeStatus & CES_cleaning)) {
					capacity = DEFLT_CAPC(
					    library->un->media);
				} else {
					capacity = 0;
				}
					(void) CatalogSetFieldByLoc(
					    library->un->eq,
					    vid.ViSlot, vid.ViPart,
					    CEF_Capacity, capacity, 0);
					(void) CatalogSetFieldByLoc(
					    library->un->eq,
					    vid.ViSlot, vid.ViPart,
					    CEF_Space, capacity, 0);
				}
				}
				current_element++;
				}
				free(buffer);
			}

			break;
		}

	case IMPORT_EXPORT_ELEMENT:
		{
			import_export_element_t *import_descrip;
			int 	slot;

	redo_import:
			if (library->un->type == DT_ACL452) {
				slot = SLOT_NUMBER(library,
				    library->range.storage_count);
			}

			buff_size =
			    (req_count * library->ele_dest_len) +
			    sizeof (element_status_data_t) +
			    sizeof (element_status_page_t) + 50;

			buffer = malloc_wait(buff_size, 2, 0);
			if (read_element_status(library, type,
			    start_element, req_count,
			    buffer, buff_size) < 0) {
				free(buffer);
				DevLog(DL_ERR(5018));
				return (-1);
			}

			status_data = (element_status_data_t *)buffer;
			BE16toH(&status_data->numb_elements, &count);
			if (count != req_count) {
				DevLog(DL_ERR(5019), count, req_count);
			}
			status_page = (element_status_page_t *)
			    (buffer + sizeof (element_status_data_t));

			BE16toH(&status_page->ele_dest_len, &ele_dest_len);

			if (ele_dest_len > library->ele_dest_len) {
				library->ele_dest_len = ele_dest_len;
				free(buffer);
				goto redo_import;
			}

			if (status_page->type_code != IMPORT_EXPORT_ELEMENT) {
				DevLog(DL_ERR(5020), status_page->type_code);
				free(buffer);
				return (-1);
			}

			for (import = library->import; import != NULL;
			    import = import->next) {

				if (import->element == start_element) {
					break;
				}
			}

			import_descrip = (import_export_element_t *)
			    ((char *)status_page +
			    sizeof (element_status_page_t));

			for (i = 0;
			    i < count && import != NULL;
			    i++,
			    import = import->next,
			    import_descrip = (import_export_element_t *)
			    ((char *)import_descrip + ele_dest_len)) {

				import_export_element_ext_t *extension;
				uint16_t stor_addr;

			if (ele_dest_len > sizeof (import_export_element_t)) {
				extension = (import_export_element_ext_t *)
				    ((char *)import_descrip +
				    sizeof (import_export_element_t *));
				BE16toH(&extension->stor_addr, &stor_addr);
			} else {
				extension = NULL;
			}

			copy_import_status(import, import_descrip, extension,
			    status_page, &slot);
			}

			free(buffer);
			break;
		}

	case DATA_TRANSFER_ELEMENT:
		{
			data_transfer_element_t *drive_descrip;

	redo_transfer:
			buff_size =
			    (req_count * library->ele_dest_len) +
			    sizeof (element_status_data_t) +
			    sizeof (element_status_page_t) + 50;

			buffer = malloc_wait(buff_size, 2, 0);

			if (read_element_status(library, type,
			    start_element, req_count,
			    buffer, buff_size) < 0) {
				DevLog(DL_ERR(5021));
				free(buffer);
				return (-1);
			}

			status_data = (element_status_data_t *)buffer;
			BE16toH(&status_data->numb_elements, &count);

			if (count != req_count) {
				DevLog(DL_ERR(5022), count, req_count);
			}

			status_page = (element_status_page_t *)
			    (buffer + sizeof (element_status_data_t));

			/*
			 * check the size of the element and if bigger than what
			 * we have set asside, change the size free the old
			 * buffer and try again
			 */
			BE16toH(&status_page->ele_dest_len, &ele_dest_len);

			if (ele_dest_len > library->ele_dest_len) {
				library->ele_dest_len = ele_dest_len;
				free(buffer);
				goto redo_transfer;
			}

			drive_descrip = (data_transfer_element_t *)
			    ((char *)status_page +
			    sizeof (element_status_page_t));

			if (status_page->type_code != DATA_TRANSFER_ELEMENT) {
				DevLog(DL_ERR(5023), status_page->type_code);
				free(buffer);
				return (-1);
			}

			for (drive = library->drive; drive != NULL;
			    drive = drive->next) {

				if (drive->element == start_element) {
					break;
				}
			}

			while (drive != NULL && count--) {
				data_transfer_element_ext_t *extension;

			if (ele_dest_len >
			    sizeof (data_transfer_element_t))
				extension = (data_transfer_element_ext_t *)
				    ((char *)drive_descrip +
				    sizeof (data_transfer_element_t));
			else
				extension = NULL;
			copy_drive_status(drive, drive_descrip,
			    extension, status_page);
			drive = drive->next;
			drive_descrip = (data_transfer_element_t *)
			    ((char *)drive_descrip + ele_dest_len);
			}

			free(buffer);
			break;
		}

	default:
		return (-1);
	}

	return (0);
}


/*
 *	generic copy status routines, one for each element type(cept storage)
 */
void
copy_xport_status(
xport_state_t *transport,
transport_element_t *desc,
transport_element_ext_t *ext,
element_status_page_t *element_status)
{
	dev_ent_t 	*un;
	uint16_t 	ele_addr;

	un = transport->library->un;
	BE16toH(&desc->ele_addr, &ele_addr);

	if (transport->element == ele_addr) {
		transport->status.b.full = desc->full;
		transport->status.b.except = desc->except;

		if (ext != NULL && desc->except && ext->add_sense_code) {
			DevLog(DL_DETAIL(5024), ext->add_sense_code,
			    ext->add_sense_qual);

			switch (process_res_codes(un, 0x09, ext->add_sense_code,
			    ext->add_sense_qual,
			    transport->library->scsi_err_tab)) {

			case MARK_UNAVAIL:
				transport->library->status.b.except = TRUE;
				break;

			default:
				DevLog(DL_DETAIL(5025));
				transport->status.b.except = FALSE;
				break;
			}
		}

		if (ext != NULL) {
			uint16_t stor_addr;

			transport->status.b.valid = ext->svalid;
			transport->status.b.invert = ext->invert;
			BE16toH(&ext->stor_addr, &stor_addr);
			transport->media_element = stor_addr;

			if (element_status->PVol && desc->full) {
				dtb(&(ext->PVolTag[0]), BARCODE_LEN);
				memcpy(transport->bar_code, ext->PVolTag,
				    BARCODE_LEN);
				transport->status.b.bar_code =
				    is_barcode(ext->PVolTag);
			} else {
				transport->status.b.bar_code = FALSE;
				memset(transport->bar_code, 0, BARCODE_LEN);
			}
		} else {
			transport->status.b.bar_code = FALSE;
			transport->status.b.valid = FALSE;
			memset(transport->bar_code, 0, BARCODE_LEN);
		}
	} else {
		DevLog(DL_ERR(5026), ele_addr, transport->element);
	}
}


void
copy_import_status(
iport_state_t *import,
import_export_element_t *desc,
import_export_element_ext_t *ext,
element_status_page_t *element_status,
int *slot)
{
	dev_ent_t 	*un;
	uint16_t 	ele_addr;
	uchar_t 	is_acl452 = (import->library->un->type == DT_ACL452);
	int 		lslot;
	int 		status = 0;
	sam_defaults_t *defaults;


	un = import->library->un;
	defaults = GetDefaults();

	if (is_acl452)
		lslot = *slot;

	BE16toH(&desc->ele_addr, &ele_addr);
	if (import->element == ele_addr) {
		import->status.b.inenab = desc->imp_enable;
		import->status.b.exenab = desc->exp_enable;
		import->status.b.access = desc->access;
		import->status.b.except = desc->except;
		if (ext != NULL && desc->except && ext->add_sense_code) {
			DevLog(DL_DETAIL(5027), ext->add_sense_code,
			    ext->add_sense_qual);
			switch (process_res_codes(un, 0x09, ext->add_sense_code,
			    ext->add_sense_qual,
			    import->library->scsi_err_tab)) {
			case MARK_UNAVAIL:
				import->library->status.b.except = TRUE;
				import->status.b.except = TRUE;
				import->status.b.access = FALSE;
				import->status.b.inenab =
				    import->status.b.exenab = FALSE;
				break;

			default:
				DevLog(DL_DETAIL(5028));
				import->status.b.except = FALSE;
				break;
			}
		}
		import->status.b.impexp = desc->impexp;
		import->status.b.full = desc->full;

		if (is_acl452 && lslot != ROBOT_NO_SLOT) {
			status = ((!desc->access) ? CES_unavail : 0);
			(void) CatalogSetFieldByLoc(un->eq, lslot,
			    0, CEF_Status,
			    status, CES_unavail);
			status &= ~CES_needs_audit;
			(void) CatalogSetFieldByLoc(un->eq, lslot,
			    0, CEF_Status,
			    status, CES_needs_audit);
		}

		if (ext != NULL) {
			uint16_t stor_addr;

			import->status.b.valid = ext->svalid;
			import->status.b.invert = ext->invert;
			/* Copy extension bits required for */
			/* Plasmon DVD-RAM library. */
			import->status.b.open = ext->PVolTag[4] & 0x1;
			import->status.b.tray = (ext->PVolTag[4] & 0x80) >> 7;

			BE16toH(&ext->stor_addr, &stor_addr);
			import->media_element = stor_addr;

			if (element_status->PVol && desc->full) {
				dtb(&(ext->PVolTag[0]), BARCODE_LEN);
				memcpy(import->bar_code, ext->PVolTag,
				    BARCODE_LEN);
				import->status.b.bar_code =
				    is_barcode(ext->PVolTag);

				/*
				 * For the 4/52 and its variants this
				 * is where the historian is checked
				 */
				if (is_acl452 && lslot !=
				    ROBOT_NO_SLOT) {

					import->status.b.valid = FALSE;
				if (import->status.b.bar_code) {
					(void) CatalogSetStringByLoc(un->eq,
					    lslot, 0, CEF_BarCode,
					    (char *)import->bar_code);
					status = CES_bar_code;
					(void) CatalogSetFieldByLoc(un->eq,
					    lslot, 0, CEF_Status,
					    status, CES_bar_code);
				}

			if (!(is_cleaning(import->bar_code))) {
				if ((defaults->flags &
				    DF_LABEL_BARCODE) &&
				    import->status.b.bar_code) {
					vsn_t 	lvsn;

					vsn_from_barcode(lvsn,
					    (char *)import->bar_code,
					    defaults, 6);
					(void) CatalogSetStringByLoc(un->eq,
					    lslot, 0,
					    CEF_Vsn, (char *)lvsn);
					(void) CatalogSetStringByLoc(un->eq,
					    lslot, 0,
					    CEF_MediaType,
					    sam_mediatoa(DT_LINEAR_TAPE));
					status = CES_labeled;
					(void) CatalogSetFieldByLoc(un->eq,
					    lslot, 0,
					    CEF_Status, status, CES_labeled);
					(void) CatalogSetFieldByLoc(un->eq,
					    lslot, 0,
					    CEF_Capacity,
					    DEFLT_CAPC(un->media), 0);
					(void) CatalogSetFieldByLoc(un->eq,
					    lslot, 0,
					    CEF_Space,
					    DEFLT_CAPC(un->media), 0);
			}
				} else if (import->status.b.bar_code) {
					vsn_t 	lvsn;

				/* If cleaning tape, copy over barcode */
				vsn_from_barcode(lvsn, (char *)import->bar_code,
				    defaults, 6);
				(void) CatalogSetStringByLoc(un->eq, lslot, 0,
				    CEF_Vsn, (char *)lvsn);
				if (is_acl452 && lslot !=
				    ROBOT_NO_SLOT)
					(void) CatalogSetFieldByLoc(un->eq,
					    lslot, 0,
					    CEF_Access, 20, 0);
				}
			}
		} else {
				import->status.b.bar_code = FALSE;
				memset(import->bar_code, 0, BARCODE_LEN);
				if (is_acl452 && lslot !=
				    ROBOT_NO_SLOT) {
					uchar_t bc[BARCODE_LEN];

					status = 0;
					(void) CatalogSetFieldByLoc(un->eq,
					    lslot, 0,
					    CEF_Status, status, CES_bar_code);
					import->status.b.valid = FALSE;
					memset(bc, 0, BARCODE_LEN);
					(void) CatalogSetStringByLoc(un->eq,
					    lslot, 0,
					    CEF_BarCode, (char *)bc);
				}
			}
		} else {
			import->status.b.valid = FALSE;
			import->status.b.bar_code = FALSE;
			memset(import->bar_code, 0, BARCODE_LEN);
		}
	} else {
		DevLog(DL_ERR(5029), ele_addr, import->element);
	}
}


void
copy_drive_status(
drive_state_t *drive,
data_transfer_element_t *desc,
data_transfer_element_ext_t *ext,
element_status_page_t *element_status)
{
	dev_ent_t 	*un;
	uint16_t 	ele_addr;

	un = drive->library->un;
	BE16toH(&desc->ele_addr, &ele_addr);

	if (drive->element == ele_addr) {
		drive->status.b.except = desc->except;
		drive->status.b.full = desc->full;
		drive->status.b.access = desc->access;
		if (ext != NULL && desc->except && ext->add_sense_code) {
			DevLog(DL_DETAIL(5030), ext->add_sense_code,
			    ext->add_sense_qual);

			switch (process_res_codes(un, 0x09, ext->add_sense_code,
			    ext->add_sense_qual,
			    drive->library->scsi_err_tab)) {

			case MARK_UNAVAIL:
				DevLog(DL_ERR(5030), ext->add_sense_code,
				    ext->add_sense_qual);
				drive->status.b.offline = TRUE;
				drive->status.b.access = FALSE;
				DownDevice(un, SAM_STATE_CHANGE);
				break;

			default:
				DevLog(DL_DETAIL(5031));
				drive->status.b.except = FALSE;
				break;
			}
		}
		if (ext != NULL) {
			uint16_t stor_addr;

			drive->status.b.valid = ext->svalid;
			drive->status.b.d_st_invert = ext->invert;
			BE16toH(&ext->stor_addr, &stor_addr);
			drive->media_element = stor_addr;
			if (element_status->PVol && desc->full) {
				dtb(&(ext->PVolTag[0]), BARCODE_LEN);
				memcpy(drive->bar_code, ext->PVolTag,
				    BARCODE_LEN);
				drive->status.b.bar_code =
				    is_barcode(ext->PVolTag);
			} else {
				drive->status.b.bar_code = FALSE;
				memset(drive->bar_code, 0, BARCODE_LEN);
			}
		} else {
			drive->status.b.valid = FALSE;
			drive->status.b.bar_code = FALSE;
			memset(drive->bar_code, 0, BARCODE_LEN);
		}
	} else {
		DevLog(DL_ERR(5032), ele_addr, drive->element);
	}
}
