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
 * Copyright 2024 Carsten Grzemba.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

static char    *_SrcFile = __FILE__;

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
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "aml/external_data.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/mode_sense.h"
#include "aml/robots.h"
#include "aml/tapes.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "element.h"
#include "aml/dev_log.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"

#define SN_STR_LEN 11
shm_alloc_t              master_shm, preview_shm;

typedef struct _drive_path_sn {
	char serial_num[SN_STR_LEN];
	char path[256];
} drive_path_sn;

int dps_idx = 0;
drive_path_sn dev_data[64];

library_t library_buff;
dev_ent_t	un_buf;
sam_extended_sense_t sense_buf;

char* get_LTO_gen(uint8_t code, char *type)
{
	switch (code){
	case 0x3b:
	case 0x3c: strcpy(type, "LTO5");
		break;
	case 0x3d:
	case 0x3e: strcpy(type, "LTO6");
		break;
	case 0x2d: strcpy(type, "LTO7");
		break;
	case 0x2e: strcpy(type, "LTO8");
		break;
	case 0x46: strcpy(type, "LTO9");
		break;
	default:
		sprintf(type, "%#x", code);
	}
	return (type);
}

void *lib_mode_sense( library_t *library, uchar_t pg_code, uchar_t *buffp, uint32_t buff_size)
{
        int             resid;
        int             retry_count;
        void       *retval = NULL;
        int             scsi_error = FALSE;
        enum sam_scsi_action scsi_action;
        sam_extended_sense_t *sense = library->un->sense;
        uchar_t *tmp_buffp;
        dev_ent_t       *un = library->un;
        uint32_t             xfer_size;

        /*
         * We can't read the specified page directly into the caller's buffer
         * because the MODE_SENSE command always prepends a header to the
         * page. Malloc a temporary buffer to hold specified page and the
         * mode sense header.
         */
        xfer_size = buff_size + sizeof (robot_ms_hdr_t);
        tmp_buffp = malloc(xfer_size);
        memset(tmp_buffp, 0, xfer_size);

        /* Issue the MODE_SENSE command */
	for (retry_count = 0; retry_count < 5; retry_count++) {
                (void) memset(sense, 0, sizeof (sam_extended_sense_t));
                if (scsi_cmd(library->open_fd, un, SCMD_MODE_SENSE,
                    30, tmp_buffp, xfer_size, pg_code, &resid) < 0) {
                        sam_extended_sense_t *sense = library->un->sense;

                        /*
                         * If we cannot read a mode page, just return to
                         * caller without it.
                         */
                        if (sense->es_add_code == 0x24 &&
                            sense->es_qual_code == 0x00 &&
                            (((uchar_t *)sense)[15] & 0x40) &&
                            ((uchar_t *)sense)[16] == 0 &&
                            ((uchar_t *)sense)[17] == 2) {
                                free(tmp_buffp);
                                return (NULL);
                        }
                        scsi_error = TRUE;
                        break;
                } else {
                        break;
                }
        }
       /*
         * We probably got some MODE_SENSE data. Do the following sanity
         * checks to verify we got valid data: 1) Verify we didn't exceed the
         * retry max 2) Verify there was no scsi error 3) Verify we got ALL
         * of the possible MODE_SENSE data 4) Verify we got the correct page.
         * If everything verified ok, copy the page to the user's buffer.
         */
        if (retry_count >= 5) {
                fprintf(stderr, "retries exhausted\n");
        } else if (scsi_error == TRUE) {
                fprintf(stderr, "mode sense failed page %#x\n", pg_code);
        } else if (((robot_ms_hdr_t *)tmp_buffp)->sense_data_len > xfer_size) {
                fprintf(stderr, "Mode sense would overflow\n");
                fprintf(stderr, "Io_count from %#x to %#x final %#x\n",
                    ((robot_ms_hdr_t *)tmp_buffp)->sense_data_len,
                    xfer_size,
                    pg_code);
        } else if ((*(tmp_buffp + sizeof (robot_ms_hdr_t)) & 0x3f) != pg_code) {
                fprintf(stderr, "mode sense failed page %#x\n", pg_code);
        } else {
                memcpy(buffp, tmp_buffp + sizeof (robot_ms_hdr_t), buff_size);
                retval = (void *) buffp;
        }
        free(tmp_buffp);
        return (retval);
}

int
get_element_status( library_t *library, const int type, const int start, const int count, void *addr, const size_t size)
{
        dev_ent_t       *un;
        int             retry = 3, get_barcodes = 0x10;
        int             len, resid, err = 0;
        int             added_more_time = FALSE;
        char            *l_mess = library->un->dis_mes[DIS_MES_NORM];
        sam_extended_sense_t sense_buf;

        sam_extended_sense_t *sense = &sense_buf;

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
				printf(l_mess);
                                get_barcodes = 0;
                                retry++;
                                continue;
                        } else {
				fprintf(stderr, "sense key: %x\n", sense->es_key);
			}
                } else
                        break;
        } while (--retry > 0);

        if (retry <= 0) {
                /* Retries exhausted */
                fprintf(stderr, "Retries exhausted\n");
                len = -1;
        }
        return (len);
}

void trimm_str(char* str, int size) {
	for (int i=size-1; i>0; i--)
		if(str[i] == ' ')
			str[i] = '\0';
}

#define SCSI_INQUIRY_BUFFER_SIZE 255
void get_SN(const char* dev)
{
	int fd;
	dev_ent_t un;
	char scratch[SCSI_INQUIRY_BUFFER_SIZE];
	struct scsi_inquiry  inquiry_data;
	int len;
	sam_extended_sense_t    sp;

	un.sense = &sp;
	un.equ_type = DT_TAPE;
	un.dt.tp.drive_index = SAM_TAPE_DVR_DEFAULT;
	un.io_active = FALSE;
	strcpy(un.dis_mes[DIS_MES_NORM],"");
	strcpy(un.dis_mes[DIS_MES_CRIT],"");

	fd = open(dev, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr, "could not open %s, %s\n", dev, strerror(errno));
		return;
	}
	len = scsi_cmd(fd, &un, SCMD_INQUIRY, 100, scratch, SCSI_INQUIRY_BUFFER_SIZE,
	    1, 0x80, (int *)NULL, &sp);
	if(len < 0) {
		fprintf(stderr, "SCSI INQ command failed on %s, %s\n", dev, strerror(errno));
		close(fd);
		return;
	}
	snprintf(dev_data[dps_idx].serial_num, SN_STR_LEN, "%s", &scratch[4]);
	snprintf(dev_data[dps_idx].path, 256, "%s", dev);
	fprintf(stderr, "%s %s\n", dev, dev_data[dps_idx].serial_num);
	dps_idx++;
	close(fd);
}

void do_inquiry(const char* dev, char* name)
{
	int fd;
	dev_ent_t un;
	char scratch[SCSI_INQUIRY_BUFFER_SIZE];
	struct scsi_inquiry  inquiry_data;
	int len;
	sam_extended_sense_t    sp;

	un.sense = &sp;
	un.equ_type = DT_TAPE;
	un.dt.tp.drive_index = SAM_TAPE_DVR_DEFAULT;
	un.io_active = FALSE;
	strcpy(un.dis_mes[DIS_MES_NORM],"");
	strcpy(un.dis_mes[DIS_MES_CRIT],"");

	fd = open(dev, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr, "could not open %s, %s\n", dev, strerror(errno));
		return;
	}
	len = scsi_cmd(fd, &un, SCMD_INQUIRY, 100, scratch, SCSI_INQUIRY_BUFFER_SIZE,
	    0, 0x00, (int *)NULL, &sp);
	if(len < 0) {
		fprintf(stderr, "SCSI INQ command failed on %s, %s\n", dev, strerror(errno));
		close(fd);
		return;
	}
	memcpy(&inquiry_data, scratch, len<sizeof(inquiry_data) ? (size_t) len : sizeof(inquiry_data));
	trimm_str(inquiry_data.inq_pid, 16);
	strncpy(name, inquiry_data.inq_pid, 16);
	close(fd);
}

void get_all_tape_paths()
{
	char *rmtdir = "/dev/rmt";
	DIR *dirp;
	struct dirent *dp;

	if ((dirp = opendir(rmtdir)) == NULL) {
		fprintf(stderr, "couldn't open %s", rmtdir);
		return;
	}

	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			char tapepath[32];
			if (strlen(dp->d_name) < 4)
				continue;
			if (strncmp(dp->d_name+strlen(dp->d_name)-3, "cbn", 3) != 0)
				continue;

			sprintf(tapepath, "%s/%s", rmtdir, dp->d_name);
			/* (void) printf("found %s\n", tapepath); */
			get_SN(tapepath);
		}
	} while (dp != NULL);

	if (errno != 0)
		fprintf(stderr, "error reading directory %s", rmtdir);
	(void) closedir(dirp);
	fprintf(stderr, "found %d drives\n", dps_idx);
}

char* get_tape_path(char* sn)
{
	for(int i=0; i<dps_idx; i++) {
		if (strcmp(dev_data[i].serial_num, sn) == 0)
			return dev_data[i].path;
	}
	return NULL;
}

void lower_str(char* str) {
	for(char *p=str; *p; p++) *p=(char)tolower(*p);
}

int main(int argc, char *argv[])
{
	char       *buffer;
	uint16_t        count;
	uint16_t        ele_dest_len;
	uint16_t        start_element;
	uint8_t pg_code;
	size_t     buff_size;
	element_status_page_t *status_page;
	element_status_data_t *status_data;
	data_transfer_element_t *drive_descrip;
	robot_ms_page1d_t pg1d_buf;
	robot_ms_page1d_t *pg1d = &pg1d_buf;
	library_t *library = &library_buff;
	char famname[16];

	if (argc < 3) {
		fprintf(stderr, "build_lib_mcf changer_path eq_start\n");
		return(-1);
	}
	char* path=argv[1];
	uint16_t eq = (uint16_t) atoi(argv[2]);

	master_shm.shared_memory = 0;

	do_inquiry(path, famname);
	get_all_tape_paths();

	printf("%s %d      rb      %s on %s\n",
	    path, eq++, famname, famname);

	pg_code = 0x1d;
	library->ele_dest_len = 88;
	library->open_fd = open(path, O_RDONLY | O_NONBLOCK);
	if(library->open_fd < 0) {
		fprintf(stderr,"open %s failed %s\n", path, strerror(errno));
		return(-1);
	}
	library->un = &un_buf;
	library->un->dt.tp.drive_index = SAM_TAPE_DVR_DEFAULT;
	library->un->sense = &sense_buf;

	lib_mode_sense(library, pg_code, (uchar_t*)pg1d, sizeof (robot_ms_page1d_t));
	BE16toH(&pg1d->first_drive, &start_element);
	BE16toH(&pg1d->num_drive, &count);
	library->range.drives_count = count;
	library->range.drives_lower = start_element;
	fprintf(stderr, "library reports %d drive slots\n", count);

	buff_size = (library->range.drives_count * library->ele_dest_len) +
	    sizeof (element_status_data_t) + sizeof (element_status_page_t) + 50;
	buffer = malloc(buff_size);

	get_element_status(library, DATA_TRANSFER_ELEMENT, library->range.drives_lower,
	    library->range.drives_count, buffer, buff_size);

	status_data = (element_status_data_t *)buffer;
	status_page = (element_status_page_t *)(buffer + sizeof (element_status_data_t));
	fprintf(stderr, "%s status\n", status_page->type_code == DATA_TRANSFER_ELEMENT ? "drive" : "wrong");
	BE16toH(&status_page->ele_dest_len, &ele_dest_len);
	drive_descrip = (data_transfer_element_t *) ((char *)status_page + sizeof (element_status_page_t));
	for (int i=count; i>0; i--) {
		data_transfer_element_ext_t *extension;
		uint16_t ele_addr;
		uint16_t stor_addr;

		if (ele_dest_len > sizeof (data_transfer_element_t))
			extension =
			    (data_transfer_element_ext_t *)
			    ((char *)drive_descrip +
			    sizeof (data_transfer_element_t));
		else
			extension = NULL;

		if (drive_descrip->except && extension != NULL) {
			fprintf(stderr, "ASC %#x ASCQ %#x\n",
			    extension->add_sense_code, extension->add_sense_qual);
		}

		BE16toH(&drive_descrip->ele_addr, &ele_addr);
		fprintf(stderr, "drive addr %d access %x full %x ",
		    ele_addr, drive_descrip->access, drive_descrip->full);

		if(extension != NULL && !drive_descrip->except) {
			char serial_number[SN_STR_LEN];
			char ltogen[8];

			BE16toH(&extension->stor_addr, &stor_addr);
			strncpy(serial_number, (const char*) &extension->AVolTag[8], SN_STR_LEN);
			trimm_str(serial_number,SN_STR_LEN);
			fprintf(stderr, "SN %s Domain %c Type %s\n",
			    serial_number, extension->AVolTag[6], get_LTO_gen(extension->AVolTag[7], ltogen));
			printf("%s %d tp %s on\n",
			    get_tape_path(serial_number), eq++, famname);
		} else {
			fprintf(stderr, "\n");
			printf("%s %d tp %s off\n",
			    "/dev/null", eq++, famname);
		}

		drive_descrip = (data_transfer_element_t *) ((char *)drive_descrip + ele_dest_len);
	}
}
