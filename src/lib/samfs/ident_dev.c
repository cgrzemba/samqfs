/*
 * ident_dev.c - identify the device.
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

#pragma ident "$Revision: 1.76 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <errno.h>
#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/scsi/scsi.h>
#include <time.h>

#include "sam/lib.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/tapes.h"
#include "aml/shm.h"
#include "sam/devnm.h"
#include "sam/devinfo.h"
#include "aml/proto.h"
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "driver/samst_def.h"
#include "aml/tapealert.h"
#include "aml/sef.h"

/* Globals */

extern shm_alloc_t master_shm, preview_shm;

typedef union {
	struct scsi_inquiry  _inquiry_data;
	char   long_inquiry[100];
}inquiry_data_t;


/* function prototypes */

void build_inquiry_info(void);
void get_serial(dev_ent_t *, int, inquiry_data_t *);
char *dirname(char *name);
boolean_t retry_open_drives(dev_ent_t *, char *, int, int *, int *);
boolean_t retry_inquiry_shared_drives(int, dev_ent_t *, char *,
	sam_extended_sense_t *, int *);
void get_capabilities(dev_ent_t *un, int fd, uchar_t *);
void get_devid(dev_ent_t *, int, uchar_t *);

/*
 * ident_dev - identify the scsi device accociated with un
 *  Entry -
 *    un - dev_ent_t *to the device
 *    mutex is held
 *  Exit -
 *    fields in device entry updated
 *    mutex still held.
 */
void
ident_dev(dev_ent_t *un, int fd)
{
#if 	defined(FUJITSU_SIMULATOR)
	if (memcmp(un->name, "/dev/rmt/", strlen("/dev/rmt/")) == 0)
		un->type = un->equ_type = DT_LINEAR_TAPE;

#else	/* !FUJITSU_SIMULATOR */

	int		open_fd, model_index, local_open = FALSE;
	int		o_flags;
	char		*scratch;
	char		*open_name;
	sam_model_t	*model_ent;
	inquiry_id_t	*inquiry_id;
	inquiry_data_t	inquiry_data;
	int		inq_data_len;
	sam_extended_sense_t	sp;

	errno = 0;
	if (inquiry_info == (inquiry_id_t *)NULL)
		build_inquiry_info();
	o_flags = O_RDONLY | O_NONBLOCK;
	if (fd < 0) {
		if ((open_fd = open(un->name, o_flags)) < 0) {
			if (IS_TAPE(un)) {
				if ((open_fd = open((open_name =
				    samst_devname(un)), o_flags)) < 0) {
					if (!(retry_open_drives(un, un->name,
					    10, &open_fd, &local_open))) {
						DevLog(DL_SYSERR(1044),
						    open_name);
						DownDevice(un,
						    SAM_STATE_CHANGE);
						un->status.b.labeled = FALSE;
						/* check type later */
						un->scsi_type = '\0';
						if (open_name != (char *)
						    un->dt.tp.samst_name) {
							free(open_name);
						}
						return;
					}
				} else {
					INC_OPEN(un);
					if (open_name != (char *)
					    un->dt.tp.samst_name) {
						free(open_name);
					}
					local_open = TRUE;
				}
			} else {
				DevLog(DL_SYSERR(1040), un->name);
				DownDevice(un, SAM_STATE_CHANGE);
				return;
			}
		} else {
			INC_OPEN(un);
			local_open = TRUE;
		}
	} else {
		DevLog(DL_DETAIL(1167), un->name, fd);
		open_fd = fd;
	}

#define	SCSI_INQUIRY_BUFFER_SIZE 255 /* (max sysci cmd size) */
	scratch = (char *)malloc_wait(SCSI_INQUIRY_BUFFER_SIZE, 2, 0);
	mutex_lock(&un->io_mutex);
	/*
	 * Need to get back at least the minimum inquiry data
	 */
	inq_data_len = scsi_cmd(open_fd, un, SCMD_INQUIRY, 0, scratch, SCSI_INQUIRY_BUFFER_SIZE, 0, 0, (int *)NULL, &sp);

	/* successfull sgen result len was 36, all other > 132 */
	DevLog(DL_DETAIL(1164), un->name, (int)sizeof (struct scsi_inquiry), sizeof(struct uscsi_cmd), inq_data_len);
	DevLog(DL_DETAIL(1166), getpid(), un->name, open_fd, strerror(errno) );
	/* if (inq_data_len < sizeof(struct uscsi_cmd)) */
	if (inq_data_len < 56)
	{
		if ((un->flags & DVFG_SHARED) && (errno == EACCES)) {
			if (retry_inquiry_shared_drives(
			    open_fd, un, scratch, &sp, &local_open)) {
				goto success;
			} else {
			        free(scratch);
				return;
			}
		} else {
			DevLog(DL_SYSERR(1001));
			DevLog(DL_ALL(1165), un->name, (un->flags & DVFG_SHARED) );
			DevLog(DL_ALL(1166), getpid(), un->name, open_fd, strerror(errno) );
			DevLogCdb(un);
			DevLogSense(un);
			OffDevice(un, SAM_STATE_CHANGE);
			mutex_unlock(&un->io_mutex);
			if (local_open) {
				(void) close(open_fd);
				DEC_OPEN(un);
			}
			free(scratch);
			return;
		}
	}


success:
	/* get block limits for this device */
	if (IS_TAPE(un)) {
		update_block_limits(un, open_fd);
		DevLog(DL_DETAIL(1170), getpid(), un->name, open_fd, strerror(errno) );
	} else if (IS_OPTICAL(un)) {
		samst_range_t  ranges;

		ranges.low_bn = ranges.high_bn = SAMST_RANGE_NOWRITE;
		ioctl(open_fd, SAMSTIOC_RANGE, &ranges);
	}

	if (local_open) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}

	mutex_unlock(&un->io_mutex);
	if (inq_data_len > sizeof (inquiry_data)) {
		inq_data_len = sizeof (inquiry_data);
	}
	(void) memcpy(&inquiry_data._inquiry_data, scratch, inq_data_len);
	(void) memcpy(&un->vendor_id, &inquiry_data._inquiry_data.inq_vid, 8);
	(void) memcpy(&un->product_id, &inquiry_data._inquiry_data.inq_pid, 16);
	un->version = inquiry_data.long_inquiry[2];
	un->scsi_type = inquiry_data._inquiry_data.inq_dtype;
	inquiry_id = inquiry_info;
	DevLog(DL_ALL(1002), un->vendor_id, un->product_id);

	while (inquiry_id->vendor_id != (char *)NULL) {
		if (memcmp(inquiry_id->vendor_id,
		    inquiry_data._inquiry_data.inq_vid,
		    strlen(inquiry_id->vendor_id)) == 0 &&
		    memcmp(inquiry_id->product_id,
		    inquiry_data._inquiry_data.inq_pid,
		    strlen(inquiry_id->product_id)) == 0) {

			model_index = 0;
			model_ent = &sam_model[0];

			while (model_ent->short_name != (char *)NULL) {
				char  *nm = "??";

				if ((model_ent->dtype ==
				    inquiry_data._inquiry_data.inq_dtype) &&
				    (strcmp(model_ent->short_name,
				    inquiry_id->samfs_id) == 0)) {
					char rev[9];

					rev[8] = '\0';
					if (un->type != model_ent->dt) {
					/*
					 * *** N.B. Incorrect indentation below
					 */

					/*
					 * The device types in the mcf for scsi
					 * robots, tapes and optical drives are
					 * converted to generic devices types
					 * in read_mcf(), so do not
					 * complain here. Only notify if the
					 * device type is not of that group,
					 * like network robots, etc.
					 */
					if (!((un->type == DT_ROBOT &&
					    is_robot(model_ent->dt)) ||
					    (un->type == DT_TAPE &&
					    is_tape(model_ent->dt)) ||
					    (un->type == DT_OPTICAL &&
					    is_optical(model_ent->dt)))) {

						DevLog(DL_ERR(1042),
						    dt_to_nm(un->type),
						    dt_to_nm(model_ent->dt));

						sam_syslog(LOG_ERR,
						    catgets(catfd, SET, 13015,
						    "Device type mismatch eq"
						    " %d: mcf = %s,"
						    " inq = %s\n"),
						    un->eq, dt_to_nm(un->type),
						    dt_to_nm(model_ent->dt));

						DownDevice(un,
						    SAM_STATE_CHANGE);

						free(scratch);
						return;
					}
					}
					un->type = un->equ_type = model_ent->dt;
					nm = device_to_nm(un->type);

	(void) memcpy(un->revision, inquiry_data._inquiry_data.inq_revision,
	    4);

					get_serial(un, fd, &inquiry_data);

	(void) memcpy(rev, inquiry_data._inquiry_data.inq_revision, 8);

					if (un->serial[0]) {
						DevLog(DL_ALL(1003),
						    un->serial, rev);
					} else {
						DevLog(DL_ALL(1004), rev);
					}
					DevLog(DL_ALL(1005),
					    model_ent->long_name, nm);

					if (IS_TAPELIB(un) ||
					    (un->type == DT_DOCSTOR))

						switch (un->type) {

						case DT_IBM3584:
						/*
						 * IBM overloads
						 * vendor-specific in INQUIRY
						 * data byte 6 to indicate the
						 * existence of a
						 * barcode reader.
						 */
						un->dt.rb.status.b.barcodes =
						    (*(scratch + 6) & 0x20) ?
						    1 : 0;
						break;

						case DT_ATL1500:
						case DT_ODI_NEO:
						case DT_QUAL82xx:
						case DT_EXB210:
						case DT_ADIC100:
						case DT_ADIC1000:
						case DT_EXBX80:

						un->dt.rb.status.b.barcodes =
						    (*(scratch + 55) & 0x01);
						break;

						case DT_METD28:
						case DT_METD360:
						case DT_ACL452:
						case DT_ACL2640:
						case DT_ADIC448:
						case DT_SPECLOG:
						case DT_STK97XX:
						case DT_SONYDMS:
						case DT_SONYCSM:
						case DT_ATLP3000:
						case DT_STKLXX:
						case DT_QUANTUMC4:
						case DT_HPSLXX:
						case DT_FJNMXX:
						case DT_SL3000:
						case DT_SLPYTHON:

						un->dt.rb.status.b.barcodes =
						    TRUE;
						break;

						case DT_3570C:
						case DT_DLT2700:
						case DT_HP_C7200:
						break;

						default:
						DevLog(DL_DEBUG(1043));
						}

					else if (IS_TAPE(un)) {
						un->sector_size =
						    TAPE_SECTOR_SIZE;
						un->dt.tp.mask =
						    (uint_t)0xffffffff;

						switch (un->type) {

						case DT_9490:
						/*
						 * Determine default capacity
						 * and compression
						 */
				if (inquiry_data.long_inquiry[55] & 0x02) {

						un->dt.tp.default_capacity
						    = STK_9490_EX;
						un->dt.tp.status.b.compression
						    = TRUE;

						} else {
						un->dt.tp.default_capacity
						    = STK_9490_SZ;
						}

						un->dt.tp.mask = 0x003fffff;
						break;

						case DT_9840:
						/*
						 * Determine default capacity
						 * and compression
						 */
				if (inquiry_data.long_inquiry[55] & 0x02) {

						un->dt.tp.status.b.compression
						    = TRUE;
						}
						un->dt.tp.default_capacity
						    = STK_9840_SZ;
						break;

						case DT_9940:
						/*
						 * Determine default capacity
						 * and compression
						 */
				if (inquiry_data.long_inquiry[55] & 0x02) {
						un->dt.tp.status.b.compression
						    = TRUE;
						}
						un->dt.tp.default_capacity
						    = STK_9940_SZ;
						break;

						case DT_TITAN:
						/*
						 * Determine default capacity
						 * and compression
						 */
				if (inquiry_data.long_inquiry[55] & 0x02) {
						un->dt.tp.status.b.compression
						    = TRUE;
						}
						un->dt.tp.default_capacity
						    = STK_TITAN_SZ;
						break;

						case DT_FUJITSU_128:
						un->dt.tp.default_capacity
						    = FUJITSU_128_SZ;
						break;

						default:
						if (un->equ_type ==
						    DT_SQUARE_TAPE) {
						un->dt.tp.mask = 0x00ffffff;

						} else if (
						    un->equ_type == DT_D3) {
						un->dt.tp.mask = 0x003fffff;
						}

						un->dt.tp.default_capacity
						    = DEFLT_CAPC(un->type);
						break;
						}
					}

					un->model = model_ent->model;
					un->model_index = model_index;
					un->status.b.present = TRUE;
					if (IS_TAPE(un)) {
						un->pages = 0;
					}
					free(scratch);
					get_capabilities(un, fd,
					    (uchar_t *)&inquiry_data);
					return;
				}
				model_index++;
				model_ent++;    /* try next entry */
			}
		}
		inquiry_id++;
	}

	DevLog(DL_ERR(1011));
	DownDevice(un, SAM_STATE_CHANGE);
	free(scratch);
	return;
#endif	/* FUJITSU_SIMULATOR */
}

/*
 * build_inquiry_info - Read the inquiry_conf file and build the information.
 */
void
build_inquiry_info()
{
	int	line = 0;
	int	param, count = 0;
	char	*conf_full_name;
	char	input_line[120];
	char	scr[120];
	char	*tmp, *tmp1, *tmp2;
	FILE	*conf_file;
	inquiry_id_t	*current, *new;

	conf_full_name = malloc_wait(strlen(INQUIRY_CONF) + 1, 2, 0);
	(void) strcpy(conf_full_name, INQUIRY_CONF);

	if ((conf_file = fopen(conf_full_name, "r")) == (FILE *)NULL) {
		sam_syslog(LOG_ALERT,
		    catgets(catfd, SET, 1383, "Inquiry.conf file missing.\n"));
		exit(1);
	}

	new = malloc_wait(sizeof (inquiry_id_t) * 400, 2, 0);
	(void) memset((char *)new, 0, sizeof (inquiry_id_t) * 400);

	current = new;

	while (fgets(input_line, 120, conf_file) != (char *)NULL) {
		line++;
		/* insure a string terminator */
		input_line[119] = '\0';
		/* forget the newline */
		tmp = &input_line[strlen(input_line)-1];
		while (*tmp == ' ' && tmp != input_line) {
			*tmp = '\0';
			tmp--;
		}
		if (input_line[0] == '\n' || input_line[0] == '#') {
			continue;
		}
		tmp2 = input_line;
		if (tmp == tmp2) {    /* blank line? */
			continue;
		}

		param = 1;
		while (param < 4) {
			while (*tmp2 != '"' && tmp2 != tmp) {
				tmp2++;
			}
			if (tmp2 == tmp) {
				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 1337,
				    "In inquiry.conf: line %d is missing an"
				    " open quote on the %d parameter.\n"),
				    line, param);
				continue;
			}
			(void) memset(scr, 0, 120);
			tmp1 = scr;

			for (tmp2++; *tmp2 != '"' && tmp2 != tmp; tmp2++) {
				if (*tmp2 == '\\') {
					tmp2++;
				}
				*tmp1 = *tmp2;
				tmp1++;
			}
			switch (param++) {
			case 1:
				if ((int)strlen(scr) > 8)
					sam_syslog(LOG_WARNING,
					    catgets(catfd, SET, 1385,
					    "inquiry.conf: line %d, vendor id"
					    " greater than 8 characters.\n"),
					    line);
				current->vendor_id = strdup(scr);
				break;

			case 2:
				if ((int)strlen(scr) > 16)
					sam_syslog(LOG_WARNING,
					    catgets(catfd, SET, 1384,
					    "inquiry.conf: line %d, product id"
					    " greater than 8 characters.\n"),
					    line);
				current->product_id = strdup(scr);
				break;

			case 3:
				current->samfs_id = strdup(scr);
				break;
			}
			tmp2++;
		}
		count++;
		if (count > 399) {
			sam_syslog(LOG_INFO, catgets(catfd, SET, 2522,
			    "Too many entries in inquiry.conf: 400 max.\n"));
			(void) fclose(conf_file);
			return;
		}
		current++;
	}
	count++;
	inquiry_info = realloc(new, count * sizeof (inquiry_id_t));
	(void) fclose(conf_file);
}


/*
 * samst_devname - return the samst name for the supplied device.
 */
char *
samst_devname(dev_ent_t *un)
{
	int	size;
	char	*st_save, *end_str;
	char	*real_link, *st_name, *path_name;
	char	*sym_link_dir;
	char	*st_at_name;
	struct	stat status;

	if (memcmp(un->name, "/dev/rdst", 9) == 0) {
		DevLog(DL_DETAIL(3131), un->name, un->dt.tp.samst_name);
		return ((char *)&un->dt.tp.samst_name);
	}

	if (lstat(un->name, &status) < 0) {
		DevLog(DL_SYSERR(3133), un->name);
		return ((char *)&un->dt.tp.samst_name);
	}

	if (!S_ISLNK(status.st_mode)) {
		DevLog(DL_ERR(3134), un->name);
		return ((char *)&un->dt.tp.samst_name);
	}

	real_link = (char *)malloc_wait(1025, 2, 0);

	if ((size = readlink(un->name, real_link, 1025)) < 0) {
		free(real_link);
		DevLog(DL_SYSERR(3135), un->name);
		return ((char *)&un->dt.tp.samst_name);
	}
	*(real_link + size) = '\0';

	/* Isolate the trailing component of path, like basename(1) */
	end_str = real_link + (size - 1);
	while (*end_str != '/') {
		end_str--;
	}

	st_name = strdup(end_str+1);

	*end_str = '\0';

	if ((memcmp(st_name, "st@", 3) != 0) &&
	    (memcmp(st_name, "tape@", 5) != 0) &&
	    (memcmp(st_name, "fsct@", 5) != 0)) {
		free(real_link);
		free(st_name);
		DevLog(DL_ERR(3136), un->name);
		return ((char *)&un->dt.tp.samst_name);
	}

	st_save = strdup(st_name);
	st_at_name = strchr(st_name, '@');
	if (st_at_name == NULL) {
		DevLog(DL_ERR(3137), st_name);
		free(real_link);
		free(st_name);
		free(st_save);
		return ((char *)&un->dt.tp.samst_name);
	}

	if ((path_name = strchr(st_name, ':')) == NULL) {
		DevLog(DL_ERR(3137), st_name);
		free(real_link);
		free(st_name);
		free(st_save);
		return ((char *)&un->dt.tp.samst_name);
	}

	*path_name = '\0';
	path_name = strdup(real_link);
	if (*path_name == '/') {
		/* Symbolic link source is an absolute path. */
		if (IS_TAPE(un)) {
			/* Use st for tape drives */
			(void) sprintf(real_link, "%s/%s", path_name, st_save);
		} else {
			/* Use samst for MO drives and direct-attached robots */
			(void) sprintf(real_link, "%s/samst%s:a,raw",
			    path_name, st_at_name);
		}
		free(path_name);
		free(st_name);
		free(st_save);
		return (real_link);
	} else {
		/*
		 * Symbolic link source is relative to the directory where the
		 * symbolic link target is.
		 */
		sym_link_dir = dirname(un->name);
		if (IS_TAPE(un)) {
			/* Use st for tape drives */
			(void) sprintf(real_link, "%s/%s/%s",
			    sym_link_dir, path_name, st_save);
		} else {
			/* Use samst for MO drives and direct-attached robots */
			(void) sprintf(real_link, "%s/rmt/%s/samst%s:a,raw",
			    sym_link_dir, path_name, st_at_name);
		}
		free(path_name);
		free(st_name);
		free(st_save);
		free(sym_link_dir);
		return (real_link);
	}
}


/*
 * ld_devices - load device interface libraries for family set.
 */
void
ld_devices(equ_t fseq)
{
	dev_ent_t	*un;

	un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
		if (un->fseq != fseq)
			continue;

		AttachDevLog(un);
		if (IS_TAPE(un)) {
			if (un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
				register int indx = un->dt.tp.drive_index;
				mutex_lock(&un->mutex);   /* aquire lock */
				mutex_lock(&IO_table[indx].mutex);
				if (!(IO_table[indx].initialized)) {
					DevLog(DL_DETAIL(3197), fseq);
					if (load_tape_io_lib(
					    &tape_IO_entries[indx],
					    &(IO_table[indx].jmp_table))) {

						(void) memccpy(un->
						    dis_mes[DIS_MES_CRIT],
						    catgets(catfd, SET, 2162,
						    "Runtime interface not"
						    " found."),
						    '\0', DIS_MES_LEN);

						DownDevice(un,
						    SAM_STATE_CHANGE);
						continue;
					}
					mutex_unlock(&IO_table[indx].mutex);
				} else {
					IO_table[indx].initialized = TRUE;
					mutex_unlock(&IO_table[indx].mutex);
				}
				mutex_unlock(&un->mutex);
			}
		}
	}
}


/*
 * get_serial - get the serial number of the device(if supported).
 * un->mutex held on entry
 * un->io_mutex aquired and released as needed.
 */
void
get_serial(dev_ent_t *un, int fd, inquiry_data_t *inquiry_data)
{
	int  local_open = 0;
	int  open_fd;
	char serial[50];
	sam_extended_sense_t   *sense;

	/*
	 * Some drives keep the serial number in the vendor
	 * unique part of the basic inquiry response. Some drives dont'
	 * support serial numbers.
	 */
	switch (un->equ_type) {

	case DT_3590:
	case DT_3592:
		/* FALLTHROUGH */
	case DT_3570:
		/* FALLTHROUGH */
		/* The 35xxs keep the serial (sequence) in bytes 38-49 */
		(void) memcpy(un->serial, &inquiry_data->long_inquiry[38],
		    (SERIAL_LEN < ((49-38)+1) ? SERIAL_LEN :  ((49-38)+1)));
		return;

	case DT_VIDEO_TAPE:
		/* FALLTHROUGH */
	case DT_SQUARE_TAPE:
		/* FALLTHROUGH */
	case DT_9490:
		/* FALLTHROUGH */
	case DT_D3:
		/* FALLTHROUGH */
	case DT_SONYDTF:
		/* FALLTHROUGH */
	case DT_MULTIFUNCTION:
		/* FALLTHROUGH */
	case DT_WORM_OPTICAL:
		/* FALLTHROUGH */
	case DT_WORM_OPTICAL_12:
		return;

	default:
		break;
	}

	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	(void) memset(sense, 0, sizeof (sam_extended_sense_t));

	/* Look for vital product data page 0x80 (serial)  */
	if (fd < 0) {
		if ((open_fd = open(un->name, O_RDONLY | O_NONBLOCK)) < 0) {
			if (IS_TAPE(un)) {
				char *open_name;
				if ((open_fd = open((open_name =
				    samst_devname(un)),
				    O_RDONLY | O_NONBLOCK)) < 0) {
					if (open_name != (char *)un->
					    dt.tp.samst_name) {
						free(open_name);
					}
					return;
				} else {
					INC_OPEN(un);
					if (open_name != (char *)un->
					    dt.tp.samst_name) {
						free(open_name);
					}
					local_open = TRUE;
				}
			} else {
				return;
			}
		} else {
			INC_OPEN(un);
			local_open = TRUE;
		}
	} else {
		open_fd = fd;
	}

	mutex_lock(&un->io_mutex);
	switch (un->equ_type) {
	case DT_9840:
	case DT_9940:
	case DT_TITAN:
	{
		uchar_t buf[0xff];
		int	len;

		if ((len = scsi_cmd(open_fd, un, SCMD_INQUIRY, 0, buf, 0xff,
		    1, 0x80, NULL, NULL)) < 4 || buf[1] != 0x80) {
			break;
		}

		if ((len = buf[3]) > SERIAL_LEN) {
			len = SERIAL_LEN;
		}

		(void) memcpy(un->serial, &buf[4], len);
		break;
	}
	default:
		if (scsi_cmd(open_fd, un, SCMD_INQUIRY, 0, serial,
		    sizeof (serial), 1, 0x80, NULL, NULL) >= 0) {
			(void) memcpy(un->serial, &serial[4],
			    (SERIAL_LEN < serial[3]) ? SERIAL_LEN : serial[3]);
		}
	}

	mutex_unlock(&un->io_mutex);
	if (local_open) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}
}


/*
 * Convert the device type back to the device mnemonic, by referencing
 * the dev_nm2dt[] array.
 */
char *
dt_to_nm(int dt)
{
	dev_nm_t *device;

	for (device = dev_nm2dt; device->dt != 0; device++) {
		if (device->dt == dt) {
			return (device->nm);
		}
	}
	return ("unknown type");
}


/*
 * Isolate the path section of a file name, like dirname(1).
 */
char *
dirname(char *name)
{
	int	size;
	char	*dirname;
	char	*end_str;

	dirname = strdup(name); /* Caller must free this memory. */
	size = strlen(dirname);
	end_str = dirname + (size - 1);
	while (*end_str != '/' && end_str != dirname) {
		end_str--;
	}
	*end_str = '\0';
	return (dirname);
}


boolean_t
retry_open_drives(dev_ent_t *un, char *path, int retry, int *open_fd,
    int *local_open)
{
	int	hold_err;
	int	retries, o_flags;
	char	*open_name;

	o_flags = (O_RDONLY | O_NONBLOCK);
	*open_fd = -1;

	for (retries = retry; (retries > 0) && (*open_fd < 0); retries--) {

		if ((*open_fd = open((open_name = samst_devname(un)),
		    o_flags)) >= 0) {
			INC_OPEN(un);
			if (open_name != (char *)un->dt.tp.samst_name)
				free(open_name);
			*local_open = TRUE;
			break;
		} else {
			hold_err = errno;
			DevLog(DL_SYSERR(1012), path, hold_err);
			sleep(10);
		}
	}

	if (*open_fd < 0) {
		DevLog(DL_DETAIL(1151), path);
		return (FALSE);
	} else {
		DevLog(DL_DETAIL(1152), *open_fd, path, un->open_count);
		return (TRUE);
	}
}

boolean_t
retry_inquiry_shared_drives(int open_fd, dev_ent_t *un, char *scratch,
    sam_extended_sense_t *sp, int *local_open)
{

	int i, retries = 3;

	for (i = 0; i < retries; i++) {
		if (scsi_cmd(open_fd, un, SCMD_INQUIRY, 0, scratch,
		    SCSI_INQUIRY_BUFFER_SIZE, 0, 0, (int *)NULL, sp) <
		    (int)sizeof (struct scsi_inquiry)) {
			mutex_unlock(&un->io_mutex);
			sleep(10);
			mutex_lock(&un->io_mutex);
		} else {
			return (TRUE);
		}
	}
	DevLog(DL_SYSERR(1001));
	DevLogCdb(un);
	DevLogSense(un);
	OffDevice(un, SAM_STATE_CHANGE);
	mutex_unlock(&un->io_mutex);
	if (local_open) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}
	free(scratch);
	return (FALSE);
}

void
get_capabilities(dev_ent_t *un, int fd, uchar_t *inquiry_data)
{
	int		open_fd;
	boolean_t	local_open = FALSE;

	if (fd < 0) {
		if ((open_fd = open(un->name, O_RDONLY | O_NONBLOCK)) < 0) {
			if (IS_TAPE(un)) {
				char    *open_name;
				if ((open_fd = open(
				    (open_name = samst_devname(un)),
				    O_RDONLY | O_NONBLOCK)) < 0) {
					if (open_name != (char *)un->
					    dt.tp.samst_name)
						free(open_name);
					return;
				} else {
					INC_OPEN(un);
					if (open_name != (char *)un->
					    dt.tp.samst_name)
						free(open_name);
					local_open = B_TRUE;
				}
			} else {
				return;
			}
		} else {
			INC_OPEN(un);
			local_open = B_TRUE;
		}
	} else {
		open_fd = fd;
	}

	mutex_lock(&un->io_mutex);

	get_supports_tapealert(un, open_fd);

	if (IS_TAPE(un)) {
		properties_t worm_mask, worm_properties;
		properties_t volsafe_mask, volsafe_properties;

		un->dt.tp.properties = PROPERTY_NONE;
		(void) tape_properties(un, open_fd);

		worm_mask = PROPERTY_WORM_DRIVE|PROPERTY_WORM_CAPABLE;
		worm_properties = un->dt.tp.properties & worm_mask;

		volsafe_mask = PROPERTY_VOLSAFE_DRIVE|PROPERTY_VOLSAFE_CAPABLE;
		volsafe_properties = un->dt.tp.properties & volsafe_mask;

		if (worm_properties == PROPERTY_WORM_DRIVE) {
			DevLog(DL_ALL(3258));
		} else if (worm_properties == worm_mask) {
			DevLog(DL_ALL(3259));
		} else if (volsafe_properties == PROPERTY_VOLSAFE_DRIVE) {
			DevLog(DL_ALL(3260));
		} else if (volsafe_properties == volsafe_mask) {
			DevLog(DL_ALL(3261));
		}

		if (un->dt.tp.properties & PROPERTY_ENCRYPTION_DRIVE) {
			DevLog(DL_ALL(3284), un->eq);
		}

		tapeclean(un, open_fd);
		if (un->tapeclean & TAPECLEAN_AUTOCLEAN) {
			if (un->tapeclean & TAPECLEAN_LOGSENSE) {
				DevLog(DL_ALL(3280));
			} else {
				DevLog(DL_ALL(3281));
			}
		}
		/* mutex_lock(&un->mutex); */
		un->load_timeout = LTO_TUR_TIMEOUT;
		/* mutex_unlock(&un->mutex); */
		(void) drive_timeout(un, open_fd, B_FALSE);
	}

	get_devid(un, open_fd, inquiry_data);

	get_supports_sef(un, open_fd);

	mutex_unlock(&un->io_mutex);
	if (local_open == B_TRUE) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}
}

void
get_devid(dev_ent_t *un, int fd, uchar_t *inquiry_data)
{
	uchar_t 		buf[0xff];
	sam_extended_sense_t	*sense;
	int 			resid;
	int 			i, j, k;
	int 			code_set;
	int			id_type;
	int 			id_len;
	int			page_len;
	int			len;
	int			inc;
	int			adr16;
	int			multip;
	int			wbus16;
	int			sync;
	int			clocking;
#define		PTYPE_NUM 6
	char			*ptype[PTYPE_NUM] = {"fcp", "pScsi", "ssa",
					"sbp", "srp", "iScsi"};

	/* Log interface type FC or pSCSI determined from std inquiry */
	adr16 = inquiry_data[6] & 0x1;
	multip = (inquiry_data[6] >> 4) & 0x1;
	wbus16 = (inquiry_data[7] >> 5) & 0x1;
	sync = (inquiry_data[7] >> 4) & 0x1;
	clocking = (inquiry_data[56] >> 2) & 0x3;
	DevLog(DL_DETAIL(1159), adr16, multip, wbus16, sync, clocking);

	un->devid.multiport = (inquiry_data[6] & 0x10);

	memset(&un->devid, 0, sizeof (dev_id_t));
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	memset(sense, 0, sizeof (sam_extended_sense_t));

	/* Read inquiry page 0x83 'Device Identification: Vendor, Serial, Type, Model, Element Addr'*/
	if (inquiry_data[2] < 3 ||
	    (page_len = scsi_cmd(fd, un, SCMD_INQUIRY, 0, buf, 0xff, 1, 0x83,
	    NULL, NULL)) < 8 || buf[1] != 0x83) {
		return;
	}

	len = (int)buf[3]+4;
	if (page_len > len) {
		page_len = len;
	}

	/* Store device identifiers. */
	for (i = 4; i < page_len && un->devid.count < IDENT_NUM; i += inc) {

		code_set = buf[i] & 0xf;
		id_type = buf[i+1] & 0xf;
		id_len = buf[i+3];
		inc = id_len + 4;

		if (id_len < 1 || !(code_set == 1 || code_set == 2) ||
		    !(id_type >= 0 && id_type <= 4)) {
			DevLog(DL_DETAIL(2104), id_len, code_set, id_type);
			break;
		}

		if (code_set == 1) {
			char tmp[3];

			for (j = 0, k = 0; j < id_len && k < IDENT_LEN-1;
			    j++, k++) {
				sprintf(tmp, "%02x", (int)(buf[i+4+j]));
				strcat(un->devid.data[un->devid.count].ident,
				    tmp);
			}
		} else if (code_set == 2) {
			if (id_len > IDENT_LEN-1) {
				id_len = IDENT_LEN-1;
			}
			memcpy(un->devid.data[un->devid.count].ident,
			    &buf[i+4], id_len);
			un->devid.data[un->devid.count].ident[id_len] = '\0';
		}
		un->devid.data[un->devid.count].type = id_type;
		if (id_type == 2 || id_type == 3) {
			DevLog(DL_DETAIL(1160),
			    un->devid.data[un->devid.count].ident,
			    un->devid.data[un->devid.count].type);
		}
		un->devid.count++;

		if (id_type == 4 && code_set == 1) {
			memcpy(&un->devid.port_id, &buf[i+4], 4);
			un->devid.port_id_valid = B_TRUE;
			DevLog(DL_DETAIL(1161), un->devid.port_id);
		}
	}

	/* Lun protocol. */
	memset(buf, 0, len);
	if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 30, buf, len, 0x18,
	    &resid) >= 3 && (buf[0] & 0x3f) == 0x18) {

		un->devid.protocol.lun_valid = B_TRUE;
		un->devid.protocol.lun = buf[2] & 0xf;
		if (un->devid.protocol.lun >= 0 &&
		    un->devid.protocol.lun < PTYPE_NUM) {
			DevLog(DL_DETAIL(1162), un->devid.protocol.lun,
			    ptype[un->devid.protocol.lun]);
		} else {
			DevLog(DL_DETAIL(1162), un->devid.protocol.lun, "\0");
		}
	}

	/* Port protocol. */
	memset(buf, 0, len);
	if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 30, buf, len, 0x19,
	    &resid) >= 3 && (buf[0] & 0x3f) == 0x19) {
		un->devid.protocol.port_valid = B_TRUE;
		un->devid.protocol.port = buf[2] & 0xf;
		if (un->devid.protocol.port >= 0 &&
		    un->devid.protocol.port < PTYPE_NUM) {
			DevLog(DL_DETAIL(1163), un->devid.protocol.port,
			    ptype[un->devid.protocol.port]);
		} else {
			DevLog(DL_DETAIL(1163), un->devid.protocol.port, "\0");
		}
	}
}
